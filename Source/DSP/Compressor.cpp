#include "Compressor.h"
#include <cmath>

namespace CrushJoints
{
    namespace
    {
        constexpr float kInputClamp = 3.3f;
        constexpr float kAttackSeconds = 0.048f;  // 1.0 of envelope per 48 ms
        constexpr float kReleaseSeconds = 0.080f; // 1.0 of envelope per 80 ms
        constexpr float kRampSeconds = 0.00025f;  // gain release ramp
        constexpr float kAmountSmoothingSeconds = 0.010f;

        // Parabola coefficient c(Amount), measured at 101 amounts (0.00 .. 1.00).
        constexpr float kCoefficient[101] = {
            1.4493f, 1.4365f, 1.4239f, 1.4114f, 1.3991f, 1.3869f,
            1.3750f, 1.3631f, 1.3514f, 1.3399f, 1.3285f, 1.3173f,
            1.3062f, 1.2952f, 1.2844f, 1.2737f, 1.2631f, 1.2527f,
            1.2424f, 1.2322f, 1.2222f, 1.2123f, 1.2025f, 1.1928f,
            1.1832f, 1.1738f, 1.1644f, 1.1552f, 1.1461f, 1.1371f,
            1.1282f, 1.1194f, 1.1107f, 1.1021f, 1.0936f, 1.0852f,
            1.0769f, 1.0686f, 1.0605f, 1.0525f, 1.0446f, 1.0367f,
            1.0290f, 1.0213f, 1.0137f, 1.0062f, 0.9988f, 0.9915f,
            0.9842f, 0.9770f, 0.9699f, 0.9629f, 0.9560f, 0.9491f,
            0.9423f, 0.9356f, 0.9289f, 0.9224f, 0.9159f, 0.9094f,
            0.9030f, 0.8967f, 0.8905f, 0.8843f, 0.8782f, 0.8721f,
            0.8662f, 0.8602f, 0.8544f, 0.8486f, 0.8428f, 0.8371f,
            0.8315f, 0.8259f, 0.8204f, 0.8149f, 0.8095f, 0.8042f,
            0.7989f, 0.7936f, 0.7884f, 0.7833f, 0.7782f, 0.7731f,
            0.7681f, 0.7632f, 0.7583f, 0.7534f, 0.7486f, 0.7439f,
            0.7391f, 0.7345f, 0.7298f, 0.7253f, 0.7207f, 0.7162f,
            0.7118f, 0.7073f, 0.7030f, 0.6986f, 0.6943f,
        };

        // Boom stage: the former Phat design, now behind the compressor.
        constexpr float kBoomShelfFrequency = 120.0f;
        constexpr float kBoomShelfGain = 2.5f; // linear -> about +8 dB
        constexpr float kBoomThresholdDb = -18.0f;
        constexpr float kBoomRatio = 6.0f;
        constexpr float kBoomKneeWidthDb = 6.0f;
        constexpr float kBoomAttackMs = 5.0f;
        constexpr float kBoomReleaseMs = 100.0f;
        constexpr float kBoomMixMin = 0.3f;
        constexpr float kBoomMixMax = 0.5f;
        constexpr float kBoomSmoothingSeconds = 0.02f;
        constexpr float kBoomMaxMakeupDb = 12.0f;
        constexpr float kBoomMakeupReferenceDb = -6.0f;

        float coefficientFor(float amount) noexcept
        {
            const float position = juce::jlimit(0.0f, 1.0f, amount) * 100.0f;
            const int index = juce::jmin(static_cast<int>(position), 99);
            const float fraction = position - static_cast<float>(index);
            return kCoefficient[index] + (kCoefficient[index + 1] - kCoefficient[index]) * fraction;
        }

        float timeToCoeff(float milliseconds, double sampleRate)
        {
            return std::exp(-1.0f / (milliseconds * 0.001f * static_cast<float>(sampleRate)));
        }
    }

    Compressor::Compressor()
    {
    }

    Compressor::~Compressor()
    {
    }

    Compressor::Curve Compressor::makeCurve(float a)
    {
        Curve result;
        result.makeup = 1.0f / (1.0f - 0.99f * a);
        result.c = coefficientFor(a);

        // The gain is G up to the larger root of G * e = 1 - c * (1 - e)^2.
        const float g = result.makeup;
        const float c = result.c;
        const float discriminant = (g - 2.0f * c) * (g - 2.0f * c) - 4.0f * c * (c - 1.0f);
        result.knee = ((2.0f * c - g) + std::sqrt(juce::jmax(discriminant, 0.0f))) / (2.0f * c);
        return result;
    }

    float Compressor::boomStaticGainDb(float levelDb)
    {
        const float halfKnee = kBoomKneeWidthDb * 0.5f;

        if (levelDb < kBoomThresholdDb - halfKnee)
            return 0.0f;

        if (levelDb > kBoomThresholdDb + halfKnee)
            return (kBoomThresholdDb + (levelDb - kBoomThresholdDb) / kBoomRatio) - levelDb;

        const float x = levelDb - kBoomThresholdDb + halfKnee;
        return ((1.0f / kBoomRatio - 1.0f) * x * x) / (2.0f * kBoomKneeWidthDb);
    }

    float Compressor::boomLegGain(float peak)
    {
        const float coeff = (peak > boomEnvelope) ? boomAttackCoeff : boomReleaseCoeff;
        boomEnvelope = coeff * boomEnvelope + (1.0f - coeff) * peak;

        const float levelDb = juce::Decibels::gainToDecibels(juce::jmax(boomEnvelope, 1.0e-8f), -80.0f);
        return juce::Decibels::decibelsToGain(boomStaticGainDb(levelDb)) * boomMakeup;
    }

    void Compressor::prepareToPlay(double newSampleRate, int /*samplesPerBlock*/, int newNumChannels)
    {
        sampleRate = newSampleRate;
        numChannels = juce::jlimit(1, 2, newNumChannels);

        attackStep = 1.0f / (kAttackSeconds * static_cast<float>(sampleRate));
        releaseStep = 1.0f / (kReleaseSeconds * static_cast<float>(sampleRate));
        rampSamples = kRampSeconds * static_cast<float>(sampleRate);

        smoothedAmount.reset(sampleRate, kAmountSmoothingSeconds);
        smoothedBoom.reset(sampleRate, kBoomSmoothingSeconds);

        const auto coefficients = juce::dsp::IIR::Coefficients<float>::makeLowShelf(
            sampleRate, kBoomShelfFrequency, 0.7f, kBoomShelfGain);

        for (auto& shelf : boomShelf)
            shelf.coefficients = coefficients;

        boomAttackCoeff = timeToCoeff(kBoomAttackMs, sampleRate);
        boomReleaseCoeff = timeToCoeff(kBoomReleaseMs, sampleRate);
        boomMakeup = juce::Decibels::decibelsToGain(
            juce::jlimit(0.0f, kBoomMaxMakeupDb, -boomStaticGainDb(kBoomMakeupReferenceDb)));

        reset();
    }

    void Compressor::reset()
    {
        envelope = 0.0f;
        boomEnvelope = 0.0f;
        phatWasOn = phatMode;
        wasOn = false;
        needsSnap = true;

        smoothedBoom.setCurrentAndTargetValue(boomMode ? 1.0f : 0.0f);
        boomWasOn = boomMode;

        for (auto& shelf : boomShelf)
            shelf.reset();

        curve = makeCurve(amount);
        gain = curve.makeup;
        anchor = gain;
    }

    float Compressor::getGainDb() const noexcept
    {
        return juce::Decibels::gainToDecibels(std::abs(gain), -100.0f);
    }

    void Compressor::processBlock(juce::AudioBuffer<float>& buffer)
    {
        if (! compressorOn)
        {
            wasOn = false;
            return;
        }

        if (! wasOn)
        {
            envelope = 0.0f;

            if (needsSnap)
            {
                // First block after prepare/reset: start at the current amount.
                smoothedAmount.setCurrentAndTargetValue(amount);
                curve = makeCurve(amount);
            }
            else
            {
                // Switching the module on starts from unity gain and the
                // amount fades in, like the original.
                smoothedAmount.setCurrentAndTargetValue(0.0f);
                curve = makeCurve(0.0f);
            }

            gain = curve.makeup;
            anchor = gain;
            wasOn = true;
            needsSnap = false;
        }

        if (phatMode != phatWasOn)
        {
            // Leaving Phat mode restarts the slow envelope from zero.
            if (! phatMode)
                envelope = 0.0f;

            phatWasOn = phatMode;
        }

        if (boomMode != boomWasOn)
        {
            if (boomMode)
            {
                for (auto& shelf : boomShelf)
                    shelf.reset();

                boomEnvelope = 0.0f;
            }

            smoothedBoom.setTargetValue(boomMode ? 1.0f : 0.0f);
            boomWasOn = boomMode;
        }

        smoothedAmount.setTargetValue(amount);

        const int channels = juce::jmin(buffer.getNumChannels(), numChannels);
        const int numSamples = buffer.getNumSamples();

        std::array<float*, 2> data {};
        for (int ch = 0; ch < channels; ++ch)
            data[static_cast<size_t>(ch)] = buffer.getWritePointer(ch);

        for (int i = 0; i < numSamples; ++i)
        {
            float currentAmount = amount;

            if (smoothedAmount.isSmoothing())
            {
                currentAmount = smoothedAmount.getNextValue();
                curve = makeCurve(currentAmount);
            }

            std::array<float, 2> input {};
            float peak = 0.0f;

            for (int ch = 0; ch < channels; ++ch)
            {
                const auto idx = static_cast<size_t>(ch);
                input[idx] = juce::jlimit(-kInputClamp, kInputClamp, data[idx][i]);
                peak = juce::jmax(peak, std::abs(input[idx]));
            }

            if (phatMode)
                envelope = peak;
            else if (peak > envelope)
                envelope = juce::jmin(peak, envelope + attackStep);
            else
                envelope = juce::jmax(peak, envelope - releaseStep);

            const float target = (envelope <= curve.knee)
                                     ? curve.makeup
                                     : (1.0f - curve.c * (1.0f - envelope) * (1.0f - envelope)) / envelope;

            const float step = (curve.makeup - anchor) / rampSamples;

            if (target <= gain + step)
            {
                gain = target;
                anchor = gain;
            }
            else
            {
                gain += step;
            }

            std::array<float, 2> output {};
            for (int ch = 0; ch < channels; ++ch)
                output[static_cast<size_t>(ch)] = input[static_cast<size_t>(ch)] * gain;

            const float boomLevel = smoothedBoom.getNextValue();

            if (boomLevel <= 0.0f)
            {
                for (int ch = 0; ch < channels; ++ch)
                    data[static_cast<size_t>(ch)][i] = output[static_cast<size_t>(ch)];

                continue;
            }

            // Boom: boost the lows of the compressed signal, squash them with a
            // heavy compressor and blend the result in.
            std::array<float, 2> shelved {};
            float shelvedPeak = 0.0f;

            for (int ch = 0; ch < channels; ++ch)
            {
                const auto idx = static_cast<size_t>(ch);
                shelved[idx] = boomShelf[idx].processSample(output[idx]);
                shelvedPeak = juce::jmax(shelvedPeak, std::abs(shelved[idx]));
            }

            const float legGain = boomLegGain(shelvedPeak);
            const float mix = juce::jmap(currentAmount, kBoomMixMin, kBoomMixMax) * boomLevel;

            for (int ch = 0; ch < channels; ++ch)
            {
                const auto idx = static_cast<size_t>(ch);
                data[idx][i] = output[idx] * (1.0f - mix) + shelved[idx] * legGain * mix;
            }
        }
    }

    void Compressor::setAmount(float newAmount)
    {
        amount = juce::jlimit(0.0f, 1.0f, newAmount);
    }

    void Compressor::setPhatMode(bool on)
    {
        phatMode = on;
    }

    void Compressor::setCompressorOn(bool on)
    {
        compressorOn = on;
    }

    void Compressor::setBoomMode(bool on)
    {
        boomMode = on;
    }
}
