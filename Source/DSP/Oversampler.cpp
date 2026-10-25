#include "Oversampler.h"

namespace CrushJoints
{
    Oversampler::Oversampler()
    {
    }

    Oversampler::~Oversampler()
    {
    }

    void Oversampler::prepareToPlay(double sampleRate, int samplesPerBlock)
    {
        oversampler.prepare({static_cast<float>(sampleRate), 2, 1});
        oversampler.setOversamplingFactor(juce::dsp::Oversampling<float>::factor_4);
        prepared = true;
    }

    void Oversampler::processBlock(juce::AudioBuffer<float>& buffer)
    {
        if (prepared)
        {
            // In a real implementation, this would upsample, process, then downsample.
            // For now, just pass through
        }
    }
}