#include "Compressor.h"
#include <cmath>

namespace CrushJoints
{
    namespace
    {
        constexpr float kMinAmount = 0.001f;
        constexpr float kKneeWidthDb = 6.0f;
        constexpr float kMaxMakeupDb = 12.0f;
        constexpr float kSmoothingSeconds = 0.02f;

        // Makeup gain restores the level a signal at this peak level loses.
        constexpr float kMakeupReferenceDb = -6.0f;

        constexpr float kPhatShelfFrequency = 120.0f;
        constexpr float kPhatShelfGain = 2.5f; // linear -> about +8 dB

        // Fixed, harder parallel leg used by Phat mode.
        constexpr float kPhatThresholdDb = -18.0f;
        constexpr float kPhatRatio = 6.0f;
        constexpr float kPhatAttackMs = 5.0f;
        constexpr float kPhatReleaseMs = 100.0f;
        constexpr float kPhatMixMin = 0.3f;
        constexpr float kPhatMixMax = 0.5f;
    }

    Compressor::Compressor()
    {
    }

    Compressor::~Compressor()
    {
    }

    float Compressor::timeToCoeff(float milliseconds) const
    {
        return std::exp(-1.0f / (milliseconds * 0.001f * static_cast<float>(sampleRate)));
    }

    float Compressor::staticGainDb(float levelDb, float thresholdDb, float ratio)
    {
        if (levelDb < thresholdDb - kKneeWidthDb * 0.5f)
            return 0.0f;

        if (levelDb > thresholdDb + kKneeWidthDb * 0.5f)
            return (thresholdDb + (levelDb - thresholdDb) / ratio) - levelDb;

        const float x = levelDb - thresholdDb + kKneeWidthDb * 0.5f;
        return ((1.0f / ratio - 1.0f) * x * x) / (2.0f * kKneeWidthDb);
    }

    float Compressor::trackEnvelope(float& state, float level, const Settings& settings)
    {
        const float coeff = (level > state) ? settings.attackCoeff : settings.releaseCoeff;
        state = coeff * state + (1.0f - coeff) * level;
        return state;
    }

    Compressor::Settings Compressor::makeMainSettings(float a) const
    {
        Settings s;

        // amount 0 -> 0 dB threshold / 1:1 ratio
        // amount 1 -> -30 dB threshold / 8:1 ratio
        s.thresholdDb = juce::jmap(a, 0.0f, -30.0f);
        s.ratio = juce::jmap(a, 1.0f, 8.0f);

        // Faster attack/release at higher amounts.
        s.attackCoeff = timeToCoeff(juce::jmap(a, 10.0f, 1.0f));
        s.releaseCoeff = timeToCoeff(juce::jmap(a, 150.0f, 50.0f));

        const float makeupDb = juce::jlimit(
            0.0f, kMaxMakeupDb,
            -staticGainDb(kMakeupReferenceDb, s.thresholdDb, s.ratio));
        s.makeupGain = juce::Decibels::decibelsToGain(makeupDb);

        return s;
    }

    Compressor::Settings Compressor::makePhatSettings() const
    {
        Settings s;
        s.thresholdDb = kPhatThresholdDb;
        s.ratio = kPhatRatio;
        s.attackCoeff = timeToCoeff(kPhatAttackMs);
        s.releaseCoeff = timeToCoeff(kPhatReleaseMs);

        const float makeupDb = juce::jlimit(
            0.0f, kMaxMakeupDb,
            -staticGainDb(kMakeupReferenceDb, s.thresholdDb, s.ratio));
        s.makeupGain = juce::Decibels::decibelsToGain(makeupDb);

        return s;
    }

    void Compressor::prepareToPlay(double newSampleRate, int /*samplesPerBlock*/, int newNumChannels)
    {
        sampleRate = newSampleRate;
        numChannels = juce::jlimit(1, 2, newNumChannels);

        const auto coefficients = juce::dsp::IIR::Coefficients<float>::makeLowShelf(
            sampleRate, kPhatShelfFrequency, 0.7f, kPhatShelfGain);

        for (auto& shelf : phatShelf)
            shelf.coefficients = coefficients;

        smoothedAmount.reset(sampleRate, kSmoothingSeconds);
        smoothedPhat.reset(sampleRate, kSmoothingSeconds);

        phatSettings = makePhatSettings();
        reset();
    }

    void Compressor::reset()
    {
        mainEnvelope = 0.0f;
        phatEnvelope = 0.0f;
        gainReductionDb = 0.0f;

        smoothedAmount.setCurrentAndTargetValue(amount);
        smoothedPhat.setCurrentAndTargetValue(phatMode ? 1.0f : 0.0f);
        phatWasOn = phatMode;
        mainSettings = makeMainSettings(amount);

        for (auto& shelf : phatShelf)
            shelf.reset();
    }

    void Compressor::processBlock(juce::AudioBuffer<float>& buffer)
    {
        if (phatMode != phatWasOn)
        {
            if (phatMode)
            {
                for (auto& shelf : phatShelf)
                    shelf.reset();

                phatEnvelope = 0.0f;
            }

            smoothedPhat.setTargetValue(phatMode ? 1.0f : 0.0f);
            phatWasOn = phatMode;
        }

        const bool phatRunning = phatMode || smoothedPhat.isSmoothing();

        if (! compressorOn || (amount < kMinAmount && ! smoothedAmount.isSmoothing() && ! phatRunning))
        {
            gainReductionDb = 0.0f;
            smoothedAmount.setCurrentAndTargetValue(amount);
            smoothedPhat.setCurrentAndTargetValue(phatMode ? 1.0f : 0.0f);
            mainSettings = makeMainSettings(amount);
            return;
        }

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
                mainSettings = makeMainSettings(currentAmount);
            }

            const float phatLevel = smoothedPhat.getNextValue();

            std::array<float, 2> input {};
            float peak = 0.0f;

            for (int ch = 0; ch < channels; ++ch)
            {
                input[static_cast<size_t>(ch)] = data[static_cast<size_t>(ch)][i];
                peak = juce::jmax(peak, std::abs(input[static_cast<size_t>(ch)]));
            }

            const float envelope = trackEnvelope(mainEnvelope, peak, mainSettings);
            const float envelopeDb = juce::Decibels::gainToDecibels(juce::jmax(envelope, 1.0e-8f), -80.0f);
            const float gainDb = staticGainDb(envelopeDb, mainSettings.thresholdDb, mainSettings.ratio);

            gainReductionDb = gainDb;

            const float mainGain = juce::Decibels::decibelsToGain(gainDb) * mainSettings.makeupGain;

            if (phatLevel <= 0.0f)
            {
                for (int ch = 0; ch < channels; ++ch)
                    data[static_cast<size_t>(ch)][i] = input[static_cast<size_t>(ch)] * mainGain;

                continue;
            }

            // Phat leg: boost the lows first so they drive the heavy compressor.
            std::array<float, 2> shelved {};
            float shelvedPeak = 0.0f;

            for (int ch = 0; ch < channels; ++ch)
            {
                const auto idx = static_cast<size_t>(ch);
                shelved[idx] = phatShelf[idx].processSample(input[idx]);
                shelvedPeak = juce::jmax(shelvedPeak, std::abs(shelved[idx]));
            }

            const float phatEnv = trackEnvelope(phatEnvelope, shelvedPeak, phatSettings);
            const float phatEnvDb = juce::Decibels::gainToDecibels(juce::jmax(phatEnv, 1.0e-8f), -80.0f);
            const float phatGainDb = staticGainDb(phatEnvDb, phatSettings.thresholdDb, phatSettings.ratio);
            const float phatGain = juce::Decibels::decibelsToGain(phatGainDb) * phatSettings.makeupGain;

            const float phatMix = juce::jmap(currentAmount, kPhatMixMin, kPhatMixMax)
                                * phatLevel;

            for (int ch = 0; ch < channels; ++ch)
            {
                const auto idx = static_cast<size_t>(ch);
                data[idx][i] = input[idx] * mainGain * (1.0f - phatMix)
                             + shelved[idx] * phatGain * phatMix;
            }
        }
    }

    void Compressor::setAmount(float newAmount)
    {
        amount = juce::jlimit(0.0f, 1.0f, newAmount);
        smoothedAmount.setTargetValue(amount);
    }

    void Compressor::setPhatMode(bool on)
    {
        phatMode = on;
    }

    void Compressor::setCompressorOn(bool on)
    {
        compressorOn = on;
    }
}
