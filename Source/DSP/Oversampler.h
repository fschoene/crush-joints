#pragma once

#include <juce_dsp/juce_dsp.h>
#include <memory>

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
        // JUCE's oversampling class is not default-constructible, so it is
        // created in prepareToPlay() once the channel/block sizes are known.
        std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;
        bool prepared = false;
    };
}
