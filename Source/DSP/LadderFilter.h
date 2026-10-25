#pragma once

#include <juce_dsp/juce_dsp.h>

namespace CrushJoints
{
    class LadderFilter
    {
    public:
        LadderFilter();
        ~LadderFilter();
        
        void prepareToPlay(double sampleRate);
        void processBlock(juce::AudioBuffer<float>& buffer);
        
        // Set parameters
        void setCutoff(float freq);
        void setResonance(float res);
        void setFilterOn(bool on);
        
    private:
        float cutoff = 20000.0f;
        float resonance = 0.0f;
        bool filterOn = false;
        
        // JUCE filter
        juce::dsp::StateVariableTPTFilter<float> filter;
    };
}