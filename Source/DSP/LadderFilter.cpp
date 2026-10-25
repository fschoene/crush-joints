#include "LadderFilter.h"

namespace CrushJoints
{
    LadderFilter::LadderFilter()
    {
    }

    LadderFilter::~LadderFilter()
    {
    }

    void LadderFilter::prepareToPlay(double sampleRate)
    {
        filter.prepare({static_cast<float>(sampleRate), 2, 1});
        filter.setType(juce::dsp::StateVariableTPTFilter<float>::Type::lowpass);
    }

    void LadderFilter::processBlock(juce::AudioBuffer<float>& buffer)
    {
        // Placeholder for filter implementation
        // In a future phase, we'll implement the filter processing
    }

    void LadderFilter::setCutoff(float freq)
    {
        cutoff = freq;
    }

    void LadderFilter::setResonance(float res)
    {
        resonance = res;
    }

    void LadderFilter::setFilterOn(bool on)
    {
        filterOn = on;
    }
}