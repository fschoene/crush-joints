#include "Compressor.h"
#include <cmath>

namespace CrushJoints
{
    namespace
    {
        constexpr float kMinAmount = 0.001f;
        constexpr float kKneeWidthDb = 6.0f;
        constexpr float kPhatShelfGain = 2.5f; // linear -> about +8 dB at 120 Hz
    }

    Compressor::Compressor()
    {
    }

    Compressor::~Compressor()
    {
    }

    void Compressor::prepareToPlay(double newSampleRate, int samplesPerBlock, int newNumChannels)
    {
        sampleRate = newSampleRate;
        numChannels = juce::jlimit(1, 2, newNumChannels);

        const auto coefficients = juce::dsp::IIR::Coefficients<float>::makeLowShelf(
            sampleRate, 120.0f, 0.7f, kPhatShelfGain);

        for (auto& shelf : phatShelf)
        {
            shelf.coefficients = coefficients;
            shelf.reset();
        }

        dryBuffer.setSize(numChannels, juce::jmax(1, samplesPerBlock), false, false, true);
        reset();
    }

    void Compressor::reset()
    {
        envelopeState = 0.0f;
        gainReductionDb = 0.0f;
        dryBuffer.clear();

        for (auto& shelf : phatShelf)
            shelf.reset();
    }

    float Compressor::computeEnvelope(float inputLevel)
    {
        // Faster attack/release at higher amounts.
        const float attackMs = juce::jmap(amount, 10.0f, 1.0f);
        const float releaseMs = juce::jmap(amount, 150.0f, 50.0f);

        const float attackCoeff = std::exp(-1.0f / (attackMs * 0.001f * static_cast<float>(sampleRate)));
        const float releaseCoeff = std::exp(-1.0f / (releaseMs * 0.001f * static_cast<float>(sampleRate)));

        const float coeff = (inputLevel > envelopeState) ? attackCoeff : releaseCoeff;
        envelopeState = coeff * envelopeState + (1.0f - coeff) * inputLevel;

        return envelopeState;
    }

    float Compressor::computeGainDb(float envelopeDb) const
    {
        // amount 0 -> 0 dB threshold / 1:1 ratio
        // amount 1 -> -30 dB threshold / 8:1 ratio
        const float threshold = juce::jmap(amount, 0.0f, -30.0f);
        const float ratio = juce::jmap(amount, 1.0f, 8.0f);

        if (envelopeDb < threshold - kKneeWidthDb * 0.5f)
            return 0.0f;

        if (envelopeDb > threshold + kKneeWidthDb * 0.5f)
            return (threshold + (envelopeDb - threshold) / ratio) - envelopeDb;

        const float x = envelopeDb - threshold + kKneeWidthDb * 0.5f;
        return ((1.0f / ratio - 1.0f) * x * x) / (2.0f * kKneeWidthDb);
    }

    void Compressor::processBlock(juce::AudioBuffer<float>& buffer)
    {
        if (! compressorOn || amount < kMinAmount)
        {
            gainReductionDb = 0.0f;
            return;
        }

        const int channels = juce::jmin(buffer.getNumChannels(), numChannels);
        const int numSamples = buffer.getNumSamples();

        if (phatMode)
            dryBuffer.makeCopyOf(buffer, true);

        const float makeupGain = juce::Decibels::decibelsToGain(amount * 12.0f);

        for (int i = 0; i < numSamples; ++i)
        {
            float peak = 0.0f;

            for (int ch = 0; ch < channels; ++ch)
                peak = juce::jmax(peak, std::abs(buffer.getSample(ch, i)));

            const float envelope = computeEnvelope(peak);
            const float envelopeDb = juce::Decibels::gainToDecibels(juce::jmax(envelope, 1.0e-8f), -80.0f);
            const float gainDb = computeGainDb(envelopeDb);

            gainReductionDb = gainDb;

            const float gain = juce::Decibels::decibelsToGain(gainDb) * makeupGain;

            for (int ch = 0; ch < channels; ++ch)
                buffer.setSample(ch, i, buffer.getSample(ch, i) * gain);
        }

        if (phatMode && dryBuffer.getNumSamples() >= numSamples)
        {
            const float phatMix = juce::jmap(amount, 0.2f, 0.65f);

            for (int ch = 0; ch < channels; ++ch)
            {
                float* wet = buffer.getWritePointer(ch);
                const float* dry = dryBuffer.getReadPointer(ch);

                for (int i = 0; i < numSamples; ++i)
                {
                    const float shelved = phatShelf[static_cast<size_t>(ch)].processSample(wet[i]);
                    wet[i] = dry[i] * (1.0f - phatMix) + shelved * phatMix;
                }
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

    //==============================================================================
    // A/B testing methods
    void Compressor::copyAtoB()
    {
        amountB = amountA;
        phatModeB = phatModeA;
        compressorOnB = compressorOnA;
    }

    void Compressor::copyBtoA()
    {
        amountA = amountB;
        phatModeA = phatModeB;
        compressorOnA = compressorOnB;
    }

    void Compressor::toggleAB()
    {
        isA = ! isA;

        if (isA)
        {
            amount = amountA;
            phatMode = phatModeA;
            compressorOn = compressorOnA;
        }
        else
        {
            amount = amountB;
            phatMode = phatModeB;
            compressorOn = compressorOnB;
        }
    }
}
