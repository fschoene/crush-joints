#pragma once

#include <juce_dsp/juce_dsp.h>

namespace CrushJoints
{
    /**
        Output stage: dry/wet mix, volume, full bypass and a hard safety
        limiter at +/-2.0 with NaN/Inf sanitation.
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

        // A/B testing support
        void copyAtoB();
        void copyBtoA();
        void toggleAB();
        bool isAActive() const { return isA; }

    private:
        float volume = 0.0f;    // -24 dB to +12 dB
        float mix = 1.0f;       // 0 = dry, 1 = wet
        bool masterOn = true;

        float volumeGain = 1.0f;

        // A/B parameter states
        float volumeA = 0.0f;
        float mixA = 1.0f;
        bool masterOnA = true;

        float volumeB = 0.0f;
        float mixB = 1.0f;
        bool masterOnB = true;

        bool isA = true;
    };
}
