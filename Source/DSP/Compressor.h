#pragma once

#include <juce_dsp/juce_dsp.h>

namespace CrushJoints
{
    class Compressor
    {
    public:
        Compressor();
        ~Compressor();
        
        void prepareToPlay(double sampleRate);
        void processBlock(juce::AudioBuffer<float>& buffer);
        
        // Set parameters
        void setAmount(float amount);
        void setPhatMode(bool on);
        void setCompressorOn(bool on);
        
        // A/B testing support
        void copyAtoB();
        void copyBtoA();
        void toggleAB();
        bool isAActive() const { return isA; }
        
    private:
        float amount = 0.0f;
        bool phatMode = false;
        bool compressorOn = true;
        
        // A/B parameter states
        float amountA = 0.0f;
        bool phatModeA = false;
        bool compressorOnA = true;
        
        float amountB = 0.0f;
        bool phatModeB = false;
        bool compressorOnB = true;
        
        bool isA = true;  // Current active state
        
        // JUCE compressor
        juce::dsp::Compressor<float> compressor;
        
        // For Phat mode
        juce::dsp::ProcessorChain<juce::dsp::IIR::Filter<float>, juce::dsp::Gain<float>> phatChain;
        juce::dsp::Gain<float> phatGain;
    };
}