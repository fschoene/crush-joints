#include "Master.h"

#include <array>
#include <cmath>

namespace CrushJoints
{
    namespace
    {
        // Measured on the original: a hard clip at +/-0.9999 after volume and mix.
        constexpr float kLimiterCeiling = 0.9999f;

        // Measured on the original: a 10 ms linear ramp of the knob value.
        constexpr float kSmoothingSeconds = 0.010f;

        // Measured master-volume law: linear gain = knob ^ 2.737.
        constexpr float kVolumeExponent = 2.737f;

        float knobFromDecibels(float volumeDb) noexcept
        {
            // gain = 10^(dB/20) = knob^2.737  ->  knob = 10^(dB/(20*2.737))
            return std::pow(juce::Decibels::decibelsToGain(volumeDb), 1.0f / kVolumeExponent);
        }

        float gainFromKnob(float knob) noexcept
        {
            return std::pow(knob, kVolumeExponent);
        }
    }

    Master::Master()
    {
    }

    Master::~Master()
    {
    }

    void Master::prepareToPlay(double newSampleRate, int samplesPerBlock, int numChannels)
    {
        juce::ignoreUnused(samplesPerBlock, numChannels);

        sampleRate = newSampleRate;
        smoothedVolume.reset(sampleRate, kSmoothingSeconds);
        smoothedMix.reset(sampleRate, kSmoothingSeconds);

        needsSnap = true;
        wasOn = false;
    }

    void Master::reset()
    {
        needsSnap = true;
        wasOn = false;
    }

    void Master::snapToTargets()
    {
        smoothedVolume.setCurrentAndTargetValue(volumeKnob);
        smoothedMix.setCurrentAndTargetValue(mix);
        needsSnap = false;
    }

    void Master::processBlock(juce::AudioBuffer<float>& buffer, const juce::AudioBuffer<float>* dryBuffer)
    {
        const int numChannels = juce::jmin(buffer.getNumChannels(), 2);
        const int numSamples = buffer.getNumSamples();

        // Master off is a full bypass: output the untouched input.
        if (! masterOn)
        {
            // Turning it back on must apply the current settings at once.
            snapToTargets();
            wasOn = false;

            if (dryBuffer != nullptr)
            {
                for (int ch = 0; ch < numChannels; ++ch)
                    buffer.copyFrom(ch, 0, *dryBuffer, ch, 0, numSamples);
            }

            return;
        }

        if (needsSnap || ! wasOn)
            snapToTargets();
        else
        {
            smoothedVolume.setTargetValue(volumeKnob);
            smoothedMix.setTargetValue(mix);
        }

        wasOn = true;

        const bool smoothingVolume = smoothedVolume.isSmoothing();

        std::array<float*, 2> data {};
        for (int ch = 0; ch < numChannels; ++ch)
            data[static_cast<size_t>(ch)] = buffer.getWritePointer(ch);

        // Dry/wet blend and output volume. The original ramps the knob value,
        // so the gain law is evaluated per sample while volume moves. The
        // smoothers advance once per sample, not once per channel.
        if (dryBuffer != nullptr)
        {
            std::array<const float*, 2> dry {};

            for (int ch = 0; ch < numChannels; ++ch)
                dry[static_cast<size_t>(ch)] = dryBuffer->getReadPointer(ch);

            for (int i = 0; i < numSamples; ++i)
            {
                const float mixNow = smoothedMix.getNextValue();
                const float gainNow = smoothingVolume
                                          ? gainFromKnob(smoothedVolume.getNextValue())
                                          : volumeGain;

                for (int ch = 0; ch < numChannels; ++ch)
                {
                    const auto idx = static_cast<size_t>(ch);
                    data[idx][i] = (data[idx][i] * mixNow + dry[idx][i] * (1.0f - mixNow)) * gainNow;
                }
            }
        }
        else
        {
            for (int i = 0; i < numSamples; ++i)
            {
                const float gainNow = smoothingVolume
                                          ? gainFromKnob(smoothedVolume.getNextValue())
                                          : volumeGain;

                for (int ch = 0; ch < numChannels; ++ch)
                    data[static_cast<size_t>(ch)][i] *= gainNow;
            }
        }

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
        volumeKnob = knobFromDecibels(volume);
        volumeGain = gainFromKnob(volumeKnob);
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
