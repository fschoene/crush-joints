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
        
    private:
        float amount = 0.0f;
        bool phatMode = false;
        bool compressorOn = true;
        
        // JUCE compressor
        juce::dsp::Compressor<float> compressor;
    };
}