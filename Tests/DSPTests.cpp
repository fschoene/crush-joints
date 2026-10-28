#include <juce_dsp/juce_dsp.h>

#include "DSP/Distortion.h"
#include "DSP/LadderFilter.h"
#include "DSP/Compressor.h"
#include "DSP/Master.h"
#include "Common/CamelCrusherCoeffs.h"
#include "Common/Presets.h"

#include <cmath>
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

    float rms(const juce::AudioBuffer<float>& buffer, int channel = 0)
    {
        double sum = 0.0;
        const auto* data = buffer.getReadPointer(channel);

        for (int i = 0; i < buffer.getNumSamples(); ++i)
            sum += static_cast<double>(data[i]) * data[i];

        return static_cast<float>(std::sqrt(sum / juce::jmax(1, buffer.getNumSamples())));
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

    void testFilter()
    {
        constexpr double sampleRate = 48000.0;
        constexpr int blockSize = 512;
        const int totalSamples = static_cast<int>(sampleRate);

        juce::AudioBuffer<float> highTone(1, totalSamples);
        auto* data = highTone.getWritePointer(0);

        for (int i = 0; i < totalSamples; ++i)
            data[i] = 0.5f * std::sin(2.0f * juce::MathConstants<float>::pi * 10000.0f
                                      * static_cast<float>(i) / static_cast<float>(sampleRate));

        const float inputRms = rms(highTone);

        CrushJoints::LadderFilter filter;
        filter.prepareToPlay(sampleRate, blockSize, 1);
        filter.setCutoff(1000.0f);
        filter.setResonance(0.0f);
        filter.setFilterOn(true);

        // Process in blocks to mimic a host.
        for (int offset = 0; offset < totalSamples; offset += blockSize)
        {
            const int count = juce::jmin(blockSize, totalSamples - offset);
            juce::AudioBuffer<float> block(1, count);
            block.copyFrom(0, 0, highTone, 0, offset, count);
            filter.processBlock(block);
            highTone.copyFrom(0, offset, block, 0, 0, count);
        }

        const float outputRms = rms(highTone);
        check(outputRms < inputRms * 0.05f, "low-pass attenuates 10 kHz strongly");

        // Bypass must be transparent.
        juce::AudioBuffer<float> bypassBuffer(1, 256);
        for (int i = 0; i < 256; ++i)
            bypassBuffer.setSample(0, i, 0.25f * std::sin(0.1f * static_cast<float>(i)));

        juce::AudioBuffer<float> original;
        original.makeCopyOf(bypassBuffer);

        filter.setFilterOn(false);
        filter.processBlock(bypassBuffer);

        for (int i = 0; i < 256; ++i)
            checkClose(bypassBuffer.getSample(0, i), original.getSample(0, i), 0.0f,
                       "filter bypass");
    }

    void testCompressor()
    {
        constexpr double sampleRate = 48000.0;
        constexpr int blockSize = 512;
        const int totalSamples = static_cast<int>(sampleRate);

        juce::AudioBuffer<float> buffer(2, totalSamples);

        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < totalSamples; ++i)
                buffer.setSample(ch, i, 0.9f * std::sin(2.0f * juce::MathConstants<float>::pi
                                                        * 220.0f * static_cast<float>(i)
                                                        / static_cast<float>(sampleRate)));

        CrushJoints::Compressor compressor;
        compressor.prepareToPlay(sampleRate, blockSize, 2);
        compressor.setAmount(1.0f);
        compressor.setCompressorOn(true);
        compressor.setPhatMode(false);

        for (int offset = 0; offset < totalSamples; offset += blockSize)
        {
            const int count = juce::jmin(blockSize, totalSamples - offset);
            juce::AudioBuffer<float> block(2, count);
            for (int ch = 0; ch < 2; ++ch)
                block.copyFrom(ch, 0, buffer, ch, offset, count);

            compressor.processBlock(block);

            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < count; ++i)
                    check(std::isfinite(block.getSample(ch, i)), "compressor output is finite");
        }

        check(compressor.getGainReductionDb() < -5.0f, "compressor applies gain reduction");

        // Bypass must be transparent.
        juce::AudioBuffer<float> bypass(2, 128);
        bypass.clear();
        juce::AudioBuffer<float> original;
        original.makeCopyOf(bypass);

        compressor.setCompressorOn(false);
        compressor.processBlock(bypass);

        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < 128; ++i)
                checkClose(bypass.getSample(ch, i), original.getSample(ch, i), 0.0f,
                           "compressor bypass");
    }

    struct Tone
    {
        float frequency;
        float amplitude;
    };

    // Renders steady tones through the compressor (two seconds) and returns
    // the settled second half of the left channel.
    std::vector<float> renderCompressor(float amount, bool phat, const std::vector<Tone>& tones)
    {
        constexpr double sampleRate = 48000.0;
        constexpr int blockSize = 512;
        const int totalSamples = static_cast<int>(sampleRate) * 2;

        CrushJoints::Compressor compressor;
        compressor.prepareToPlay(sampleRate, blockSize, 2);
        compressor.setAmount(amount);
        compressor.setPhatMode(phat);
        compressor.setCompressorOn(true);
        compressor.reset();

        std::vector<float> settled;

        for (int offset = 0; offset < totalSamples; offset += blockSize)
        {
            juce::AudioBuffer<float> block(2, blockSize);

            for (int i = 0; i < blockSize; ++i)
            {
                float v = 0.0f;

                for (const auto& tone : tones)
                    v += tone.amplitude * std::sin(2.0f * juce::MathConstants<float>::pi * tone.frequency
                                                   * static_cast<float>(offset + i)
                                                   / static_cast<float>(sampleRate));

                block.setSample(0, i, v);
                block.setSample(1, i, v);
            }

            compressor.processBlock(block);

            if (offset >= totalSamples / 2)
                for (int i = 0; i < blockSize; ++i)
                    settled.push_back(block.getSample(0, i));
        }

        return settled;
    }

    float rmsOf(const std::vector<float>& samples)
    {
        double sum = 0.0;
        for (float v : samples)
            sum += static_cast<double>(v) * v;

        return static_cast<float>(std::sqrt(sum / static_cast<double>(samples.empty() ? 1 : samples.size())));
    }

    // Single-bin DFT magnitude (the render holds a whole number of cycles).
    float binMagnitude(const std::vector<float>& samples, float frequency)
    {
        constexpr double sampleRate = 48000.0;
        double re = 0.0;
        double im = 0.0;

        for (size_t i = 0; i < samples.size(); ++i)
        {
            const double phase = 2.0 * juce::MathConstants<double>::pi * frequency
                               * static_cast<double>(i) / sampleRate;
            re += samples[i] * std::cos(phase);
            im += samples[i] * std::sin(phase);
        }

        return static_cast<float>(2.0 * std::sqrt(re * re + im * im)
                                  / static_cast<double>(samples.size()));
    }

    // Settled output/input RMS ratio of a single sine, in dB.
    float compressorGainDb(float amount, bool phat, float frequency, float peak)
    {
        const auto out = renderCompressor(amount, phat, { { frequency, peak } });
        return 20.0f * std::log10(rmsOf(out) / (peak / std::sqrt(2.0f)));
    }

    void testCompressorLevelBehaviour()
    {
        // Amount 0 is transparent.
        checkClose(compressorGainDb(0.0f, false, 1000.0f, 0.5f), 0.0f, 0.01f,
                   "compressor is transparent at Amount 0");

        // Auto makeup follows the curve, so a low Amount is only a small lift
        // for material below the threshold, never a big plain gain.
        {
            const float gain = compressorGainDb(0.25f, false, 1000.0f, 0.01f);
            check(gain >= 0.0f && gain < 1.5f, "low Amount adds at most a small lift");
        }

        // Hot signals are squashed progressively harder.
        float previous = 1.0f;
        for (float amount : { 0.25f, 0.5f, 0.75f, 1.0f })
        {
            const float gain = compressorGainDb(amount, false, 1000.0f, 0.9f);
            check(gain < previous, "hot signal is attenuated more at higher Amount");
            previous = gain;
        }

        // Program-level material stays near its input level for moderate
        // Amounts (makeup follows the compression curve).
        for (float amount : { 0.25f, 0.5f, 0.75f })
        {
            const float gain = compressorGainDb(amount, false, 1000.0f, 0.3f);
            check(gain > -1.5f && gain < 6.0f, "mid-level output stays near the input level");
        }

        // Quiet material is lifted more as Amount rises.
        check(compressorGainDb(1.0f, false, 1000.0f, 0.05f)
                  > compressorGainDb(0.5f, false, 1000.0f, 0.05f),
              "quiet material is lifted more at higher Amount");
    }

    void testCompressorPhat()
    {
        // Bass plus treble at equal level: Phat must tilt the balance towards
        // the bass at every Amount, and must not thin out loud material.
        const std::vector<Tone> mix { { 50.0f, 0.15f }, { 2000.0f, 0.15f } };

        for (float amount : { 0.0f, 0.5f, 1.0f })
        {
            const auto off = renderCompressor(amount, false, mix);
            const auto on = renderCompressor(amount, true, mix);

            const float balanceOff = 20.0f * std::log10(binMagnitude(off, 50.0f) / binMagnitude(off, 2000.0f));
            const float balanceOn = 20.0f * std::log10(binMagnitude(on, 50.0f) / binMagnitude(on, 2000.0f));

            std::cout << "Phat bass tilt at amount " << amount << ": " << (balanceOn - balanceOff) << " dB\n";
            check(balanceOn > balanceOff + 1.0f, "Phat tilts the balance towards the bass");
        }

        // Phat does not make hot bass quieter than the plain compressor.
        for (float amount : { 0.0f, 0.5f, 1.0f })
            check(compressorGainDb(amount, true, 50.0f, 0.9f) > compressorGainDb(amount, false, 50.0f, 0.9f) - 1.0f,
                  "Phat does not thin out hot bass");

        // Toggling Phat and Amount while running stays finite and click-free.
        {
            constexpr double sampleRate = 48000.0;
            CrushJoints::Compressor compressor;
            compressor.prepareToPlay(sampleRate, 256, 2);
            compressor.setCompressorOn(true);

            float previousSample = 0.0f;
            float maxStep = 0.0f;
            int sampleIndex = 0;

            for (int block = 0; block < 200; ++block)
            {
                compressor.setPhatMode((block / 20) % 2 == 1);
                compressor.setAmount(static_cast<float>(block % 50) / 50.0f);

                juce::AudioBuffer<float> buffer(2, 256);
                for (int i = 0; i < 256; ++i, ++sampleIndex)
                {
                    const float v = 0.4f * std::sin(2.0f * juce::MathConstants<float>::pi * 60.0f
                                                    * static_cast<float>(sampleIndex)
                                                    / static_cast<float>(sampleRate));
                    buffer.setSample(0, i, v);
                    buffer.setSample(1, i, v);
                }

                compressor.processBlock(buffer);

                for (int i = 0; i < 256; ++i)
                {
                    const float v = buffer.getSample(0, i);
                    check(std::isfinite(v), "compressor output stays finite while automated");
                    maxStep = juce::jmax(maxStep, std::abs(v - previousSample));
                    previousSample = v;
                }
            }

            check(maxStep < 0.1f, "automating Amount/Phat does not click");
        }
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
                check(legacyPreset.cutoff >= 20.0f && legacyPreset.cutoff <= 20000.0f,
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

            const auto americanCrunch = presets.getPreset(13);
            check(americanCrunch.name == "American Crunch", "American Crunch name");
            checkClose(americanCrunch.mix, 1.0f, 1.0e-6f, "American Crunch mix matches the factory FXB (100% wet)");

            const auto britishClean = presets.getPreset(15);
            check(britishClean.name == "British Clean", "British Clean name");
            checkClose(britishClean.cutoff, 20000.0f, 1.0e-3f, "British Clean open cutoff");
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
    testFilter();
    testCompressor();
    testCompressorLevelBehaviour();
    testCompressorPhat();
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
