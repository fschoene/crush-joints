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
        juce::ignoreUnused(sampleRate);

        // 2 channels, factor 2 => 4x oversampling.
        oversampler = std::make_unique<juce::dsp::Oversampling<float>>(
            2,
            2,
            juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
            true,
            false);

        oversampler->initProcessing(static_cast<size_t>(samplesPerBlock));
        prepared = true;
    }

    void Oversampler::processBlock(juce::AudioBuffer<float>& buffer)
    {
        if (!prepared || oversampler == nullptr)
            return;

        // Round-trip the buffer through the oversampler. Any processing that
        // benefits from a higher sample rate can be inserted between the
        // up- and down-sampling calls.
        auto block = juce::dsp::AudioBlock<float>(buffer);
        oversampler->processSamplesUp(block);
        oversampler->processSamplesDown(block);
    }
}
