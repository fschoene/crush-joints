#include <juce_dsp/juce_dsp.h>

#include "DSP/Distortion.h"
#include "DSP/LadderFilter.h"
#include "DSP/Compressor.h"
#include "DSP/Master.h"
#include "Common/CamelCrusherCoeffs.h"
#include "Common/CutoffCurve.h"
#include "Common/Presets.h"

#include <cmath>
#include <complex>
#include <vector>
#include <iostream>
#include <iterator>

namespace
{
    int failures = 0;

    void check(bool condition, const juce::String& message)
    {
        if (! condition)
        {
            ++failures;
            std::cerr << "FAIL: " << message << '\n';
        }
    }

    void checkClose(float actual, float expected, float tolerance, const juce::String& message)
    {
        if (std::abs(actual - expected) > tolerance)
        {
            ++failures;
            std::cerr << "FAIL: " << message
                      << " (expected " << expected << ", got " << actual << ")\n";
        }
    }

    // Independent implementation of the reverse-camel reference shapers, used
    // to verify that the Distortion class uses the tables and formulas
    // correctly.
    float referenceTube(float x, float amount)
    {
        const float position = juce::jlimit(0.0f, 100.0f, amount * 100.0f);
        const int i0 = juce::jlimit(0, 100, static_cast<int>(position));
        const int i1 = juce::jmin(i0 + 1, 100);
        const float t = position - static_cast<float>(i0);

        const float gain = CrushJoints::disttube_coeffs[i0][0]
                         + t * (CrushJoints::disttube_coeffs[i1][0] - CrushJoints::disttube_coeffs[i0][0]);
        const float a = CrushJoints::disttube_coeffs[i0][1]
                      + t * (CrushJoints::disttube_coeffs[i1][1] - CrushJoints::disttube_coeffs[i0][1]);
        const float b = CrushJoints::disttube_coeffs[i0][2]
                      + t * (CrushJoints::disttube_coeffs[i1][2] - CrushJoints::disttube_coeffs[i0][2]);
        const float c = CrushJoints::disttube_coeffs[i0][3]
                      + t * (CrushJoints::disttube_coeffs[i1][3] - CrushJoints::disttube_coeffs[i0][3]);

        const float s = gain * x;

        if (s < -0.6f)
            return a * s * s + b * s + c;

        if (s > 0.6f)
            return -a * s * s + b * s - c;

        return s;
    }

    float referenceMech(float y, float amount)
    {
        const float position = juce::jlimit(0.0f, 100.0f, amount * 100.0f);
        const int i0 = juce::jlimit(0, 100, static_cast<int>(position));
        const int i1 = juce::jmin(i0 + 1, 100);
        const float t = position - static_cast<float>(i0);

        const float gain = CrushJoints::distmech_coeffs[i0][0]
                         + t * (CrushJoints::distmech_coeffs[i1][0] - CrushJoints::distmech_coeffs[i0][0]);
        const float b = CrushJoints::distmech_coeffs[i0][1]
                      + t * (CrushJoints::distmech_coeffs[i1][1] - CrushJoints::distmech_coeffs[i0][1]);
        const float c = CrushJoints::distmech_coeffs[i0][2]
                      + t * (CrushJoints::distmech_coeffs[i1][2] - CrushJoints::distmech_coeffs[i0][2]);

        const float s = gain * y;

        if (s < -0.75f)
            return b * s + c;

        if (s > 0.75f)
            return b * s - c;

        return s;
    }

    // Complete Distortion -> Master reference path using the reverse-camel
    // tables. The stage order (Mech first, then Tube) and the +/-0.9999 hard
    // clip were measured on the original plugin (docs/MEASUREMENTS.md); the
    // reverse-camel run() loop applied Tube first.
    float referenceChain(float x, float tubeAmount, float mechAmount, float mix, float volumeDb)
    {
        const int tubeIndex = juce::jlimit(0, 100, static_cast<int>(tubeAmount * 100.0f));
        const int mechIndex = juce::jlimit(0, 100, static_cast<int>(mechAmount * 100.0f));

        const float* t = CrushJoints::disttube_coeffs[tubeIndex];
        const float* m = CrushJoints::distmech_coeffs[mechIndex];

        float s = m[0] * x;
        float y = (s < -0.75f) ? (m[1] * s + m[2])
                : (s >  0.75f) ? (m[1] * s - m[2])
                : s;

        s = t[0] * y;
        y = (s < -0.6f) ? (t[1] * s * s + t[2] * s + t[3])
          : (s >  0.6f) ? (-t[1] * s * s + t[2] * s - t[3])
          : s;

        y = mix * y + (1.0f - mix) * x;
        return juce::jlimit(-0.9999f, 0.9999f, y * juce::Decibels::decibelsToGain(volumeDb));
    }

    void testCoefficientTables()
    {
        checkClose(CrushJoints::disttube_coeffs[0][0], 1.0f, 1.0e-6f, "tube entry 0 gain");
        checkClose(CrushJoints::disttube_coeffs[100][0], 4.99996f, 1.0e-4f, "tube entry 100 gain");
        checkClose(CrushJoints::distmech_coeffs[0][0], 1.0f, 1.0e-6f, "mech entry 0 gain");
        checkClose(CrushJoints::distmech_coeffs[100][0], 11.0001f, 1.0e-3f, "mech entry 100 gain");
        checkClose(CrushJoints::distmech_coeffs[100][2], -0.731707f, 1.0e-5f, "mech entry 100 c");
    }

    // Output values recorded from the original plugin (DC input, steady state;
    // tools/analysis, docs/MEASUREMENTS.md section 4). They are order
    // sensitive: Tube -> Mech would give clearly different numbers.
    void testDistortionMatchesOriginalMeasurements()
    {
        struct Case
        {
            float tube, mech;
            float expected[8];
        };

        constexpr float inputs[8] = { -0.9f, -0.6f, -0.3f, -0.1f, 0.1f, 0.3f, 0.6f, 0.9f };

        const Case cases[] = {
            { 0.5f,  0.5f,  { -0.99949f, -0.99184f, -0.97500f, -0.90000f,
                               0.90000f,  0.97500f,  0.99184f,  0.99949f } },
            { 0.25f, 0.75f, { -0.99939f, -0.99018f, -0.96993f, -0.95029f,
                               0.95029f,  0.96993f,  0.99018f,  0.99939f } },
            { 1.0f,  0.5f,  { -0.99958f, -0.99325f, -0.97934f, -0.91736f,
                               0.91736f,  0.97934f,  0.99325f,  0.99958f } },
        };

        for (const auto& c : cases)
        {
            CrushJoints::Distortion distortion;
            distortion.prepareToPlay(48000.0);
            distortion.setTubeOn(true);
            distortion.setTubeAmount(c.tube);
            distortion.setMechAmount(c.mech);

            juce::AudioBuffer<float> buffer(1, 8);

            for (int i = 0; i < 8; ++i)
                buffer.setSample(0, i, inputs[i]);

            distortion.processBlock(buffer);

            for (int i = 0; i < 8; ++i)
                checkClose(buffer.getSample(0, i), c.expected[i], 2.0e-3f,
                           "original measurement tube " + juce::String(c.tube)
                               + " mech " + juce::String(c.mech) + " sample " + juce::String(i));
        }
    }

    void testDistortionReference()
    {
        constexpr int numSamples = 64;
        constexpr float inputLevels[] = { -1.0f, -0.5f, -0.1f, 0.0f, 0.1f, 0.5f, 0.9f };
        const float amount = 0.73f;

        CrushJoints::Distortion distortion;
        distortion.prepareToPlay(48000.0);
        distortion.setTubeOn(true);
        distortion.setTubeAmount(amount);
        distortion.setMechAmount(0.0f);

        juce::AudioBuffer<float> buffer(1, numSamples);
        buffer.clear();

        for (int i = 0; i < numSamples; ++i)
            buffer.setSample(0, i, inputLevels[i % static_cast<int>(std::size(inputLevels))]);

        juce::AudioBuffer<float> expected;
        expected.makeCopyOf(buffer);

        for (int i = 0; i < numSamples; ++i)
        {
            const float tube = referenceTube(buffer.getSample(0, i), amount);
            expected.setSample(0, i, referenceMech(tube, 0.0f));
        }

        distortion.processBlock(buffer);

        for (int i = 0; i < numSamples; ++i)
            checkClose(buffer.getSample(0, i), expected.getSample(0, i), 2.0e-5f,
                       "distortion reference sample " + juce::String(i));
    }

    void testReferenceAudioPath()
    {
        struct Setting
        {
            float tube, mech, mix, volumeDb;
        };

        const Setting settings[] = {
            { 0.0f, 0.0f, 1.0f, 0.0f },
            { 0.5f, 0.0f, 1.0f, 0.0f },
            { 0.5f, 0.5f, 1.0f, 0.0f },
            { 1.0f, 1.0f, 1.0f, 0.0f },
            { 0.0f, 1.0f, 1.0f, -6.0f },
            { 0.75f, 0.25f, 0.5f, -3.0f },
        };

        constexpr int numSamples = 512;

        for (const auto& setting : settings)
        {
            juce::AudioBuffer<float> input(2, numSamples);

            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < numSamples; ++i)
                    input.setSample(ch, i, 0.3f * std::sin(0.037f * static_cast<float>(i + ch * 13)));

            juce::AudioBuffer<float> wet;
            wet.makeCopyOf(input);

            juce::AudioBuffer<float> dry;
            dry.makeCopyOf(input);

            CrushJoints::Distortion distortion;
            distortion.prepareToPlay(48000.0);
            distortion.setTubeOn(true);
            distortion.setTubeAmount(setting.tube);
            distortion.setMechAmount(setting.mech);
            distortion.processBlock(wet);

            CrushJoints::Master master;
            master.prepareToPlay(48000.0, numSamples, 2);
            master.setVolume(setting.volumeDb);
            master.setMix(setting.mix);
            master.setMasterOn(true);
            master.processBlock(wet, &dry);

            for (int i = 0; i < numSamples; ++i)
            {
                const float expected = referenceChain(input.getSample(0, i),
                                                      setting.tube, setting.mech,
                                                      setting.mix, setting.volumeDb);
                checkClose(wet.getSample(0, i), expected, 2.0e-5f,
                           "reference audio path");
            }
        }
    }

    void testDistortionTransparencyAndBypass()
    {
        CrushJoints::Distortion distortion;
        distortion.prepareToPlay(48000.0);
        distortion.setTubeAmount(0.0f);
        distortion.setMechAmount(0.0f);

        juce::AudioBuffer<float> buffer(2, 128);
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < 128; ++i)
                buffer.setSample(ch, i, std::sin(0.05f * static_cast<float>(i)) * 0.5f);

        juce::AudioBuffer<float> original;
        original.makeCopyOf(buffer);

        distortion.processBlock(buffer);

        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < 128; ++i)
                checkClose(buffer.getSample(ch, i), original.getSample(ch, i), 1.0e-3f,
                           "zero-amount distortion transparency");

        distortion.setTubeAmount(1.0f);
        distortion.setMechAmount(1.0f);
        distortion.setTubeOn(false);
        original.makeCopyOf(buffer);
        distortion.processBlock(buffer);

        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < 128; ++i)
                checkClose(buffer.getSample(ch, i), original.getSample(ch, i), 0.0f,
                           "distortion bypass");
    }

    void testCutoffCurve()
    {
        // Values read from the original plugin's cutoff display (docs/MEASUREMENTS.md).
        struct Point { float knob, hz; };
        const Point points[] = {
            { 0.00f, 80.0f }, { 0.10f, 262.0f }, { 0.25f, 577.0f }, { 0.50f, 1278.0f },
            { 0.75f, 2476.0f }, { 0.90f, 4059.0f }, { 0.95f, 5258.0f }, { 0.99f, 8040.0f },
            { 1.00f, 16000.0f },
        };

        for (const auto& point : points)
        {
            checkClose(CrushJoints::CutoffCurve::toHz(point.knob), point.hz, 1.0f,
                       "cutoff curve at knob " + juce::String(point.knob));
            checkClose(CrushJoints::CutoffCurve::toNormalised(point.hz), point.knob, 2.0e-3f,
                       "cutoff curve inverse at " + juce::String(point.hz) + " Hz");
        }

        check(CrushJoints::CutoffCurve::toNormalised(10.0f) == 0.0f, "cutoff below range clamps");
        check(CrushJoints::CutoffCurve::toNormalised(30000.0f) == 1.0f, "cutoff above range clamps");
    }

    // Magnitude of the bilinear 2-pole low-pass biquad (RBJ form) at a frequency.
    float biquadGain(float cutoffHz, float q, float frequency, double sampleRate)
    {
        const double w0 = 2.0 * juce::MathConstants<double>::pi * cutoffHz / sampleRate;
        const double alpha = std::sin(w0) / (2.0 * q);
        const double c = std::cos(w0);
        const double b0 = (1.0 - c) / 2.0, b1 = 1.0 - c, b2 = b0;
        const double a0 = 1.0 + alpha, a1 = -2.0 * c, a2 = 1.0 - alpha;

        const double w = 2.0 * juce::MathConstants<double>::pi * frequency / sampleRate;
        const std::complex<double> z1 = std::polar(1.0, -w);
        const std::complex<double> z2 = std::polar(1.0, -2.0 * w);
        return static_cast<float>(std::abs((b0 + b1 * z1 + b2 * z2) / (a0 + a1 * z1 + a2 * z2)));
    }

    // Settled output/input RMS ratio of a sine through the filter.
    float filterGain(float cutoffHz, float resonance, float frequency, float amplitude = 0.01f)
    {
        constexpr double sampleRate = 44100.0;
        constexpr int blockSize = 512;
        const int totalSamples = static_cast<int>(sampleRate);

        CrushJoints::LadderFilter filter;
        filter.prepareToPlay(sampleRate, blockSize, 1);
        filter.setCutoff(cutoffHz);
        filter.setResonance(resonance);
        filter.setFilterOn(true);

        double inSum = 0.0;
        double outSum = 0.0;

        for (int offset = 0; offset < totalSamples; offset += blockSize)
        {
            const int count = juce::jmin(blockSize, totalSamples - offset);
            juce::AudioBuffer<float> block(1, count);

            for (int i = 0; i < count; ++i)
                block.setSample(0, i, amplitude * std::sin(2.0f * juce::MathConstants<float>::pi * frequency
                                                          * static_cast<float>(offset + i)
                                                          / static_cast<float>(sampleRate)));
            juce::AudioBuffer<float> input;
            input.makeCopyOf(block);

            filter.processBlock(block);

            if (offset >= totalSamples / 2)
                for (int i = 0; i < count; ++i)
                {
                    inSum += static_cast<double>(input.getSample(0, i)) * input.getSample(0, i);
                    outSum += static_cast<double>(block.getSample(0, i)) * block.getSample(0, i);
                }
        }

        return static_cast<float>(std::sqrt(outSum / inSum));
    }

    void testFilter()
    {
        // The original is a linear 2-pole biquad with Q = 1 + 6 * resonance and the
        // displayed Hz as the cutoff (docs/MEASUREMENTS.md section 5).
        const float cutoffHz = 1278.0f;

        for (float resonance : { 0.0f, 0.5f, 1.0f })
        {
            const float q = 1.0f + 6.0f * resonance;

            for (float frequency : { 300.0f, 1278.0f, 2500.0f, 5000.0f, 9000.0f })
            {
                const float expected = biquadGain(cutoffHz, q, frequency, 44100.0);
                const float actual = filterGain(cutoffHz, resonance, frequency);
                check(std::abs(actual - expected) < 0.01f * expected + 1.0e-4f,
                      "filter matches the bilinear biquad: res " + juce::String(resonance)
                          + " at " + juce::String(frequency) + " Hz (expected "
                          + juce::String(expected) + ", got " + juce::String(actual) + ")");
            }

            // At the cutoff a 2-pole low-pass has gain Q.
            checkClose(filterGain(cutoffHz, resonance, cutoffHz), q, 0.03f * q, "gain at the cutoff equals Q");
        }

        // No saturation: the same gain at a much higher level.
        checkClose(filterGain(cutoffHz, 1.0f, cutoffHz, 0.1f), filterGain(cutoffHz, 1.0f, cutoffHz, 0.001f),
                   0.02f, "filter is linear");

        // Bypass must be transparent.
        CrushJoints::LadderFilter filter;
        filter.prepareToPlay(44100.0, 512, 1);
        filter.setCutoff(500.0f);
        filter.setFilterOn(false);

        juce::AudioBuffer<float> bypassBuffer(1, 256);
        for (int i = 0; i < 256; ++i)
            bypassBuffer.setSample(0, i, 0.25f * std::sin(0.1f * static_cast<float>(i)));

        juce::AudioBuffer<float> original;
        original.makeCopyOf(bypassBuffer);
        filter.processBlock(bypassBuffer);

        for (int i = 0; i < 256; ++i)
            checkClose(bypassBuffer.getSample(0, i), original.getSample(0, i), 0.0f, "filter bypass");
    }

    // Runs a constant input through the compressor (stereo, both channels equal)
    // and returns the output samples. `blocks` of 512 samples at 44.1 kHz.
    std::vector<float> processConstant(CrushJoints::Compressor& compressor, float value, int numSamples)
    {
        std::vector<float> out;
        out.reserve(static_cast<size_t>(numSamples));

        for (int offset = 0; offset < numSamples; offset += 512)
        {
            const int count = juce::jmin(512, numSamples - offset);
            juce::AudioBuffer<float> block(2, count);

            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < count; ++i)
                    block.setSample(ch, i, value);

            compressor.processBlock(block);

            for (int i = 0; i < count; ++i)
                out.push_back(block.getSample(0, i));
        }

        return out;
    }

    void configureCompressor(CrushJoints::Compressor& compressor, float amount, bool phat)
    {
        compressor.prepareToPlay(44100.0, 512, 2);
        compressor.setAmount(amount);
        compressor.setPhatMode(phat);
        compressor.setCompressorOn(true);
        compressor.reset();
    }

    void testCompressorStaticCurve()
    {
        // Settled outputs recorded from the original plugin for constant inputs
        // (docs/MEASUREMENTS.md section 6). 0.5 s is long enough for the 48 ms / 80 ms
        // follower to settle for every input here.
        struct Row { float amount; float expected[7]; };
        constexpr float inputs[7] = { 0.1f, 0.25f, 0.5f, 0.8f, 1.0f, 1.2f, 2.0f };

        const Row rows[] = {
            { 0.0f,  { 0.10000f, 0.25000f, 0.50000f, 0.80000f, 1.0f, 0.94203f, -0.44928f } },
            { 0.5f,  { 0.19802f, 0.45443f, 0.75752f, 0.96120f, 1.0f, 0.96120f,  0.03009f } },
            { 0.75f, { 0.33992f, 0.54161f, 0.79627f, 0.96740f, 1.0f, 0.96740f,  0.18509f } },
            { 1.0f,  { 0.43762f, 0.60946f, 0.82642f, 0.97223f, 1.0f, 0.97223f,  0.30570f } },
        };

        for (const auto& row : rows)
            for (int i = 0; i < 7; ++i)
                for (bool phat : { false, true })
                {
                    CrushJoints::Compressor compressor;
        configureCompressor(compressor, row.amount, phat);
                    const auto out = processConstant(compressor, inputs[i], 44100 / 2);
                    checkClose(out.back(), row.expected[i], 1.0e-3f,
                               "compressor static curve, amount " + juce::String(row.amount)
                                   + " input " + juce::String(inputs[i]) + (phat ? " phat" : ""));
                }

        // Quiet input gets the full makeup: G = 1 / (1 - 0.99 * Amount).
        for (float amount : { 0.25f, 0.5f, 0.9f, 1.0f })
        {
            CrushJoints::Compressor compressor;
        configureCompressor(compressor, amount, false);
            const auto out = processConstant(compressor, 0.0004f, 4410);
            checkClose(out.back() / 0.0004f, 1.0f / (1.0f - 0.99f * amount),
                       0.003f * (1.0f / (1.0f - 0.99f * amount)), "compressor low-level gain");
        }
    }

    // Output/input ratio of a constant step: `before` for 0.3 s, then `after`.
    std::vector<float> stepGains(float amount, bool phat, float before, float after, int numSamples)
    {
        CrushJoints::Compressor compressor;
        configureCompressor(compressor, amount, phat);
        processConstant(compressor, before, 44100 * 3 / 10);
        const auto out = processConstant(compressor, after, numSamples);

        std::vector<float> gains;
        for (float v : out)
            gains.push_back(v / after);

        return gains;
    }

    void testCompressorDynamics()
    {
        // Phat on: the gain falls at once and rises along a 0.25 ms ramp.
        {
            const auto g = stepGains(0.75f, true, 0.9f, 0.15f, 12);
            const float expected[] = { 1.3543f, 1.6066f, 1.8589f, 2.1112f, 2.3635f, 2.6158f, 2.7415f, 2.7415f };
            for (int i = 0; i < 8; ++i)
                checkClose(g[static_cast<size_t>(i)], expected[i], 3.0e-3f, "Phat release ramp sample " + juce::String(i));

            const auto g2 = stepGains(0.5f, true, 0.9f, 0.3f, 12);
            const float expected2[] = { 1.1801f, 1.2599f, 1.3398f, 1.4196f, 1.4994f, 1.5792f, 1.6590f, 1.7388f, 1.7492f };
            for (int i = 0; i < 9; ++i)
                checkClose(g2[static_cast<size_t>(i)], expected2[i], 3.0e-3f, "Phat release ramp (amount 0.5) sample " + juce::String(i));

            const auto attack = stepGains(0.75f, true, 0.15f, 0.9f, 4);
            checkClose(attack[0], 1.1017f, 3.0e-3f, "Phat attack is instant");
        }

        // Phat off: the envelope follows at 1.0 per 48 ms up and 1.0 per 80 ms down.
        {
            const int atMs[] = { 1, 5, 10, 20, 40 };

            const auto up = stepGains(0.75f, false, 0.3f, 0.9f, 44100 / 20);
            const float expectedUp[] = { 1.9442f, 1.7580f, 1.5790f, 1.3035f, 1.1021f };
            for (int i = 0; i < 5; ++i)
                checkClose(up[static_cast<size_t>(44100 * atMs[i] / 1000)], expectedUp[i], 5.0e-3f,
                           "slow attack at " + juce::String(atMs[i]) + " ms");

            const auto down = stepGains(0.75f, false, 0.6f, 0.3f, 44100 / 20);
            const float expectedDown[] = { 1.4665f, 1.5364f, 1.6329f, 1.8741f, 2.0023f };
            for (int i = 0; i < 5; ++i)
                checkClose(down[static_cast<size_t>(44100 * atMs[i] / 1000)], expectedDown[i], 5.0e-3f,
                           "slow release at " + juce::String(atMs[i]) + " ms");
        }

        // Stereo link: a loud left channel also reduces the quiet right channel.
        {
            constexpr int numSamples = 44100 * 2;
            CrushJoints::Compressor compressor;
        configureCompressor(compressor, 0.75f, false);
            double rightIn = 0.0;
            double rightOut = 0.0;

            for (int offset = 0; offset < numSamples; offset += 512)
            {
                juce::AudioBuffer<float> block(2, 512);

                for (int i = 0; i < 512; ++i)
                {
                    const float t = static_cast<float>(offset + i) / 44100.0f;
                    block.setSample(0, i, 0.5f * std::sin(2.0f * juce::MathConstants<float>::pi * 500.0f * t));
                    block.setSample(1, i, 0.02f * std::sin(2.0f * juce::MathConstants<float>::pi * 730.0f * t));
                }

                juce::AudioBuffer<float> input;
                input.makeCopyOf(block);
                compressor.processBlock(block);

                if (offset >= numSamples / 2)
                    for (int i = 0; i < 512; ++i)
                    {
                        rightIn += static_cast<double>(input.getSample(1, i)) * input.getSample(1, i);
                        rightOut += static_cast<double>(block.getSample(1, i)) * block.getSample(1, i);
                    }
            }

            checkClose(static_cast<float>(10.0 * std::log10(rightOut / rightIn)), 4.80f, 0.05f,
                       "compressor is stereo linked");
        }

        // Switching the module on starts from unity gain and fades the amount in.
        {
            CrushJoints::Compressor compressor;
        configureCompressor(compressor, 0.75f, false);
            processConstant(compressor, 0.3f, 22050);
            compressor.setCompressorOn(false);
            const auto bypassed = processConstant(compressor, 0.3f, 512);
            checkClose(bypassed.back(), 0.3f, 0.0f, "compressor bypass is exact");

            compressor.setCompressorOn(true);
            const auto out = processConstant(compressor, 0.3f, 44100 / 10);
            check(out.front() / 0.3f < 1.05f, "compressor fades in from unity gain");
            checkClose(out.back() / 0.3f, 2.0023f, 0.01f, "compressor settles after switching on");
        }
    }

    void testCompressorBoom()
    {
        // The Boom stage is hidden: off by default, and when on it adds the
        // low-shelf parallel leg (finite, different, and louder in the bass).
        constexpr int numSamples = 44100;
        std::vector<float> plain;
        std::vector<float> boom;

        for (bool on : { false, true })
        {
            CrushJoints::Compressor compressor;
        configureCompressor(compressor, 0.5f, false);
            compressor.setBoomMode(on);
            compressor.reset();

            auto& target = on ? boom : plain;

            for (int offset = 0; offset < numSamples; offset += 512)
            {
                juce::AudioBuffer<float> block(2, 512);

                for (int ch = 0; ch < 2; ++ch)
                    for (int i = 0; i < 512; ++i)
                        block.setSample(ch, i, 0.1f * std::sin(2.0f * juce::MathConstants<float>::pi * 60.0f
                                                               * static_cast<float>(offset + i) / 44100.0f));

                compressor.processBlock(block);

                for (int i = 0; i < 512; ++i)
                {
                    check(std::isfinite(block.getSample(0, i)), "boom output is finite");
                    target.push_back(block.getSample(0, i));
                }
            }
        }

        const auto tailRms = [](const std::vector<float>& v)
        {
            double sum = 0.0;
            for (size_t i = v.size() / 2; i < v.size(); ++i)
                sum += static_cast<double>(v[i]) * v[i];
            return std::sqrt(sum / static_cast<double>(v.size() / 2));
        };

        check(tailRms(boom) > tailRms(plain) * 1.05, "boom adds low-end level");
    }

    void testMaster()
    {
        CrushJoints::Master master;
        master.prepareToPlay(48000.0, 512, 2);

        // Volume + limiter
        {
            juce::AudioBuffer<float> buffer(2, 64);
            buffer.clear();

            for (int ch = 0; ch < 2; ++ch)
            {
                buffer.setSample(ch, 0, 1.0f);
                buffer.setSample(ch, 1, -1.0f);
                buffer.setSample(ch, 2, 0.001f);
            }

            master.setVolume(12.0f);
            master.setMix(1.0f);
            master.setMasterOn(true);
            master.processBlock(buffer, nullptr);

            checkClose(buffer.getSample(0, 0), 0.9999f, 1.0e-6f, "limiter clamps positive peak");
            checkClose(buffer.getSample(0, 1), -0.9999f, 1.0e-6f, "limiter clamps negative peak");
            check(buffer.getSample(0, 2) > 0.003f, "volume boost applied");
        }

        // Full bypass via master off
        {
            juce::AudioBuffer<float> wet(2, 32);
            juce::AudioBuffer<float> dry(2, 32);

            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < 32; ++i)
                {
                    wet.setSample(ch, i, 1.0f);
                    dry.setSample(ch, i, 0.25f);
                }

            master.setVolume(12.0f);
            master.setMix(0.0f);
            master.setMasterOn(false);
            master.processBlock(wet, &dry);

            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < 32; ++i)
                    checkClose(wet.getSample(ch, i), 0.25f, 1.0e-6f, "master off bypass");
        }

        // Dry/wet mix
        {
            juce::AudioBuffer<float> wet(1, 8);
            juce::AudioBuffer<float> dry(1, 8);
            wet.clear();
            dry.clear();

            for (int i = 0; i < 8; ++i)
            {
                wet.setSample(0, i, 1.0f);
                dry.setSample(0, i, 0.0f);
            }

            master.setVolume(0.0f);
            master.setMix(0.25f);
            master.setMasterOn(true);
            master.processBlock(wet, &dry);

            for (int i = 0; i < 8; ++i)
                checkClose(wet.getSample(0, i), 0.25f, 1.0e-6f, "dry/wet mix");
        }
    }

    void testPresets()
    {
        const auto tempFile = juce::File::getSpecialLocation(juce::File::tempDirectory)
                                  .getNonexistentChildFile("CrushJointsPresetsTest", ".xml");

        {
            CrushJoints::Presets presets(tempFile);
            presets.initPresets();

            check(presets.getNumPresets() == 32, "32 factory presets are available");
            check(presets.getNumFactoryPresets() == 32, "32 factory presets reported");
            check(presets.getNumUserPresets() == 0, "no user presets initially");
            check(presets.getNumLegacyPresets() == 20, "20 legacy presets");
            check(presets.getNumCrushClonePresets() == 11, "11 grouped CrushClone presets");

            check(presets.getPreset(0).name == "Default", "first preset is Default");
            check(presets.getPreset(0).category == CrushJoints::PresetCategory::defaultPreset,
                  "Default is outside the categories");
            check(presets.getPreset(1).category == CrushJoints::PresetCategory::crushClone,
                  "DRM BigBadBeat is a CrushClone preset");
            check(presets.getPreset(11).name == "Destroy Everything", "last CrushClone preset");
            check(presets.getPreset(11).category == CrushJoints::PresetCategory::crushClone,
                  "Destroy Everything is a CrushClone preset");
            check(presets.getPreset(12).name == "Annihilate", "first legacy preset");
            check(presets.getPreset(12).category == CrushJoints::PresetCategory::legacy,
                  "Annihilate is a legacy preset");
            check(presets.getPreset(31).name == "Turn it to 11", "last legacy preset");
            check(presets.getPreset(31).category == CrushJoints::PresetCategory::legacy,
                  "Turn it to 11 is a legacy preset");
            check(! presets.isUserPreset(0), "factory preset is not a user preset");

            const char* expectedLegacyNames[] = {
                "Annihilate", "American Crunch", "American HiGain", "British Clean",
                "British Crunch", "British HiGain", "Bass Maker", "Clean Rainbow",
                "Tube Warmth", "Dark Crunch", "Destroyer", "Ultra Phat",
                "Distort", "Dream", "More Beef", "Santana Row",
                "Fuzz Filter", "Subtle Master", "YouWereWarned", "Turn it to 11"
            };

            for (int i = 0; i < 20; ++i)
            {
                const auto legacyPreset = presets.getPreset(12 + i);
                check(legacyPreset.name == expectedLegacyNames[i],
                      "legacy preset name " + juce::String(i));
                check(legacyPreset.category == CrushJoints::PresetCategory::legacy,
                      "legacy preset category " + juce::String(i));
                check(legacyPreset.cutoff >= 80.0f && legacyPreset.cutoff <= 16000.0f,
                      "legacy preset cutoff range " + juce::String(i));
                check(legacyPreset.volume >= -24.0f && legacyPreset.volume <= 12.0f,
                      "legacy preset volume range " + juce::String(i));
            }

            const auto annihilate = presets.getPreset(12);
            checkClose(annihilate.tube, 0.34f, 1.0e-6f, "Annihilate tube");
            checkClose(annihilate.mech, 0.0f, 1.0e-6f, "Annihilate mech");
            check(! annihilate.lpfOn, "Annihilate filter is off");
            check(annihilate.phat, "Annihilate Phat is on");
            checkClose(annihilate.amount, 0.905f, 1.0e-6f, "Annihilate amount");

            // Cutoff and volume knobs use the measured laws of the original:
            // cutoff curve (80 Hz .. 16 kHz) and linear gain = knob ^ 2.737.
            checkClose(annihilate.cutoff, 824.0f, 1.0f, "Annihilate cutoff (knob 0.35)");
            checkClose(annihilate.volume, -5.9067f, 0.01f, "Annihilate volume (knob 0.78)");
            checkClose(presets.getPreset(18).cutoff, 402.0f, 1.0f, "Bass Maker cutoff (knob 0.17)");
            check(presets.getPreset(18).name == "Bass Maker", "Bass Maker position");
            checkClose(presets.getPreset(31).volume, -13.9974f, 0.01f, "Turn it to 11 volume (knob 0.555)");

            const auto americanCrunch = presets.getPreset(13);
            check(americanCrunch.name == "American Crunch", "American Crunch name");
            checkClose(americanCrunch.mix, 1.0f, 1.0e-6f, "American Crunch mix matches the factory FXB (100% wet)");

            const auto britishClean = presets.getPreset(15);
            check(britishClean.name == "British Clean", "British Clean name");
            checkClose(britishClean.cutoff, 16000.0f, 1.0e-3f, "British Clean open cutoff (knob 1.0)");
            check(! britishClean.phat, "British Clean Phat is off");
            checkClose(britishClean.mix, 1.0f, 1.0e-6f, "British Clean mix matches the factory FXB (100% wet)");

            check(presets.getPreset(10).name == "Telephone", "Telephone preset name");
            check(presets.getPreset(10).lpfOn, "Telephone filter is on");
            check(! presets.getPreset(10).compOn, "Telephone compressor is off");

            auto custom = presets.getPreset(0);
            custom.name = "My Custom";
            custom.tube = 0.42f;
            custom.mech = 0.17f;
            custom.cutoff = 4321.0f;

            const int customIndex = presets.saveUserPreset(custom);
            check(customIndex == 32, "custom preset is appended after factory presets");
            check(presets.getNumPresets() == 33, "user preset increases program count");
            check(presets.getNumUserPresets() == 1, "one user preset");
            check(presets.isUserPreset(32), "new preset is a user preset");
            check(presets.getPreset(32).category == CrushJoints::PresetCategory::user,
                  "new preset has the user category");
            check(presets.getPreset(32).name == "My Custom", "custom preset name");
            checkClose(presets.getPreset(32).tube, 0.42f, 1.0e-6f, "custom preset tube");
            checkClose(presets.getPreset(32).cutoff, 4321.0f, 1.0e-3f, "custom preset cutoff");

            custom.mech = 0.9f;
            check(presets.saveUserPreset(custom) == 32, "saving the same name overwrites");
            checkClose(presets.getPreset(32).mech, 0.9f, 1.0e-6f, "overwritten mech value");

            check(presets.renameUserPreset(32, "Renamed"), "user preset can be renamed");
            check(presets.getPreset(32).name == "Renamed", "renamed preset name");

            auto invalid = custom;
            invalid.name = "   ";
            check(presets.saveUserPreset(invalid) == -1, "empty preset name is rejected");
            check(! presets.deleteUserPreset(0), "factory presets cannot be deleted");
            check(! presets.renameUserPreset(0, "Nope"), "factory presets cannot be renamed");
        }

        // Re-open the same bank to verify XML persistence.
        {
            CrushJoints::Presets reloaded(tempFile);
            reloaded.initPresets();

            check(reloaded.getNumPresets() == 33, "user preset survives reload");
            check(reloaded.getNumFactoryPresets() == 32, "factory count survives reload");
            check(reloaded.isUserPreset(32), "reloaded preset is a user preset");
            check(reloaded.getPreset(32).category == CrushJoints::PresetCategory::user,
                  "reloaded preset keeps the user category");
            check(reloaded.getPreset(32).name == "Renamed", "reloaded preset name");
            checkClose(reloaded.getPreset(32).mech, 0.9f, 1.0e-6f, "reloaded mech value");
            check(reloaded.deleteUserPreset(32), "user preset can be deleted");
            check(reloaded.getNumPresets() == 32, "deleting restores factory count");
        }

        tempFile.deleteFile();
    }
}

int main()
{
    testCoefficientTables();
    testDistortionReference();
    testDistortionMatchesOriginalMeasurements();
    testReferenceAudioPath();
    testDistortionTransparencyAndBypass();
    testCutoffCurve();
    testFilter();
    testCompressorStaticCurve();
    testCompressorDynamics();
    testCompressorBoom();
    testMaster();
    testPresets();

    if (failures == 0)
    {
        std::cout << "All DSP tests passed.\n";
        return 0;
    }

    std::cerr << failures << " test(s) failed.\n";
    return 1;
}
