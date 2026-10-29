#pragma once

#include <juce_dsp/juce_dsp.h>

namespace CrushJoints
{
    /**
        Output stage: dry/wet mix, volume, full bypass and a hard safety
        limiter at +/-0.9999 with NaN/Inf sanitation.

        Mix and volume ramp linearly over 10 ms, as measured on the original.
        The original ramps the knob value, not the dB value, so the volume
        ramp is applied to the equivalent knob position and the measured
        gain law (gain = knob^2.737) is evaluated on every sample while it
        moves.
    */
    class Master
    {
    public:
        Master();
        ~Master();

        void prepareToPlay(double sampleRate, int samplesPerBlock, int numChannels);
        void reset();

        /** @param dryBuffer must contain the signal captured before the effect
            chain when mix < 1 or masterOn is false; otherwise it may be null. */
        void processBlock(juce::AudioBuffer<float>& buffer, const juce::AudioBuffer<float>* dryBuffer);

        // Set parameters
        void setVolume(float volumeDb);
        void setMix(float mix);
        void setMasterOn(bool on);

    private:
        void snapToTargets();

        float volume = 0.0f;    // -24 dB to +12 dB
        float mix = 1.0f;       // 0 = dry, 1 = wet
        bool masterOn = true;

        // Volume as the original's knob position, where gain = knob^2.737.
        float volumeKnob = 1.0f;
        float volumeGain = 1.0f;

        double sampleRate = 44100.0;
        juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedVolume;
        juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedMix;

        bool needsSnap = true;
        bool wasOn = false;
    };
}
