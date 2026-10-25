#pragma once

#include <juce_dsp/juce_dsp.h>

namespace CrushJoints
{
    class Master
    {
    public:
        Master();
        ~Master();
        
        void prepareToPlay(double sampleRate);
        void processBlock(juce::AudioBuffer<float>& buffer);
        
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
        float volume = 0.0f;    // -24dB to +12dB
        float mix = 1.0f;       // 0.0 to 1.0 (dry/wet)
        bool masterOn = true;   // Global bypass
        
        // A/B parameter states
        float volumeA = 0.0f;
        float mixA = 1.0f;
        bool masterOnA = true;
        
        float volumeB = 0.0f;
        float mixB = 1.0f;
        bool masterOnB = true;
        
        bool isA = true;  // Current active state
        
        // DSP components
        juce::dsp::Gain<float> gain;
        juce::dsp::ProcessorChain<juce::dsp::Gain<float>, juce::dsp::Gain<float>> limiterChain;
        juce::dsp::Gain<float> limiterGain;
    };
}