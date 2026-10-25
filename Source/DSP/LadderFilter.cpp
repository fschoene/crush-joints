#include "LadderFilter.h"

namespace CrushJoints
{
    namespace
    {
        constexpr float kMinResonance = 0.7071f;
        constexpr float kMaxResonance = 4.2071f; // 0.7071 + 3.5
        constexpr float kBypassCutoff = 18000.0f;
        constexpr float kBypassResonance = 0.05f;
    }

    LadderFilter::LadderFilter()
    {
    }

    LadderFilter::~LadderFilter()
    {
    }

    void LadderFilter::prepareToPlay(double newSampleRate, int samplesPerBlock, int newNumChannels)
    {
        sampleRate = newSampleRate;
        numChannels = juce::jlimit(1, 2, newNumChannels);

        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = static_cast<juce::uint32>(juce::jmax(1, samplesPerBlock));
        spec.numChannels = static_cast<juce::uint32>(numChannels);

        filter.prepare(spec);
        filter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
        filter.setCutoffFrequency(juce::jlimit(20.0f, static_cast<float>(sampleRate * 0.45), cutoff));
        filter.setResonance(kMinResonance);
        filter.reset();
    }

    void LadderFilter::reset()
    {
        filter.reset();
    }

    void LadderFilter::processBlock(juce::AudioBuffer<float>& buffer)
    {
        if (! filterOn)
            return;

        // Wide-open filter with no resonance is inaudible; skip the work and
        // avoid numerical edge cases at 20 kHz.
        if (cutoff >= kBypassCutoff && resonance < kBypassResonance)
            return;

        const float maxSafeCutoff = static_cast<float>(sampleRate * 0.4);
        const float safeCutoff = juce::jlimit(20.0f, maxSafeCutoff, cutoff);
        const float safeResonance = juce::jmap(resonance, 0.0f, 1.0f, kMinResonance, kMaxResonance);

        filter.setCutoffFrequency(safeCutoff);
        filter.setResonance(safeResonance);

        const int channels = juce::jmin(buffer.getNumChannels(), numChannels);
        juce::dsp::AudioBlock<float> block(buffer);
        auto subBlock = block.getSubsetChannelBlock(0, static_cast<size_t>(channels));

        juce::dsp::ProcessContextReplacing<float> context(subBlock);
        filter.process(context);
    }

    void LadderFilter::setCutoff(float freq)
    {
        cutoff = juce::jlimit(20.0f, 20000.0f, freq);
    }

    void LadderFilter::setResonance(float res)
    {
        resonance = juce::jlimit(0.0f, 1.0f, res);
    }

    void LadderFilter::setFilterOn(bool on)
    {
        filterOn = on;
    }
}
