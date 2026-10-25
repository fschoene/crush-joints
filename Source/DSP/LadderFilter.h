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
        
        // A/B testing support
        void copyAtoB();
        void copyBtoA();
        void toggleAB();
        bool isAActive() const { return isA; }
        
    private:
        float cutoff = 20000.0f;
        float resonance = 0.0f;
        bool filterOn = false;
        
        // A/B parameter states
        float cutoffA = 20000.0f;
        float resonanceA = 0.0f;
        bool filterOnA = false;
        
        float cutoffB = 20000.0f;
        float resonanceB = 0.0f;
        bool filterOnB = false;
        
        bool isA = true;  // Current active state
        
        // JUCE filter
        juce::dsp::StateVariableTPTFilter<float> filter;
    };
}