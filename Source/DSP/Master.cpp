#include "Master.h"
#include <cmath>

namespace CrushJoints
{
    namespace
    {
        constexpr float kLimiterCeiling = 2.0f; // 8x headroom above 0 dBFS
    }

    Master::Master()
    {
    }

    Master::~Master()
    {
    }

    void Master::prepareToPlay(double sampleRate, int samplesPerBlock, int numChannels)
    {
        juce::ignoreUnused(sampleRate, samplesPerBlock, numChannels);
    }

    void Master::reset()
    {
        // Stateless apart from parameter values.
    }

    void Master::processBlock(juce::AudioBuffer<float>& buffer, const juce::AudioBuffer<float>* dryBuffer)
    {
        const int numChannels = juce::jmin(buffer.getNumChannels(), 2);
        const int numSamples = buffer.getNumSamples();

        // Master off is a full bypass: output the untouched input.
        if (! masterOn)
        {
            if (dryBuffer != nullptr)
            {
                for (int ch = 0; ch < numChannels; ++ch)
                    buffer.copyFrom(ch, 0, *dryBuffer, ch, 0, numSamples);
            }

            return;
        }

        // Dry/wet blend
        if (dryBuffer != nullptr && mix < 0.999f)
        {
            const float dryGain = 1.0f - mix;

            for (int ch = 0; ch < numChannels; ++ch)
            {
                float* wet = buffer.getWritePointer(ch);
                const float* dry = dryBuffer->getReadPointer(ch);

                for (int i = 0; i < numSamples; ++i)
                    wet[i] = wet[i] * mix + dry[i] * dryGain;
            }
        }

        // Output volume
        buffer.applyGain(volumeGain);

        // Safety limiter and sanitation
        for (int ch = 0; ch < numChannels; ++ch)
        {
            float* data = buffer.getWritePointer(ch);

            for (int i = 0; i < numSamples; ++i)
            {
                float sample = data[i];

                if (sample > kLimiterCeiling)
                    sample = kLimiterCeiling;
                else if (sample < -kLimiterCeiling)
                    sample = -kLimiterCeiling;

                if (! std::isfinite(sample))
                    sample = 0.0f;

                data[i] = sample;
            }
        }
    }

    void Master::setVolume(float volumeDb)
    {
        volume = juce::jlimit(-24.0f, 12.0f, volumeDb);
        volumeGain = juce::Decibels::decibelsToGain(volume);
    }

    void Master::setMix(float newMix)
    {
        mix = juce::jlimit(0.0f, 1.0f, newMix);
    }

    void Master::setMasterOn(bool on)
    {
        masterOn = on;
    }
}
