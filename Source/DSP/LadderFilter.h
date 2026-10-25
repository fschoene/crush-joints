#pragma once

#include <juce_dsp/juce_dsp.h>

namespace CrushJoints
{
    /**
        24 dB/octave resonant low-pass filter.

        Uses JUCE's zero-delay-feedback state-variable filter (TPT SVF) in
        low-pass mode. Resonance 0..1 maps to Q 0.7071..4.2071, staying below
        self-oscillation.
    */
    class LadderFilter
    {
    public:
        LadderFilter();
        ~LadderFilter();

        void prepareToPlay(double sampleRate, int samplesPerBlock, int numChannels);
        void reset();
        void processBlock(juce::AudioBuffer<float>& buffer);

        // Set parameters
        void setCutoff(float freq);
        void setResonance(float res);
        void setFilterOn(bool on);

    private:
        float cutoff = 20000.0f;
        float resonance = 0.0f;
        bool filterOn = false;


        double sampleRate = 44100.0;
        int numChannels = 2;

        juce::dsp::StateVariableTPTFilter<float> filter;
    };
}
