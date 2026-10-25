#pragma once

#include <juce_dsp/juce_dsp.h>

namespace CrushJoints
{
    class Oversampler
    {
    public:
        Oversampler();
        ~Oversampler();
        
        void prepareToPlay(double sampleRate, int samplesPerBlock);
        void processBlock(juce::AudioBuffer<float>& buffer);
        
    private:
        juce::dsp::Oversampler<float> oversampler;
        bool prepared = false;
    };
}