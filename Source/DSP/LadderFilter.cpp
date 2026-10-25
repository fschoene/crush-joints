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
        // TODO: Implement filter processing with:
        // 1. Check if filter is enabled
        // 2. Apply filter with current cutoff/res parameters 
        // 3. Handle bypass logic appropriately
        // 4. Apply any special behaviors per original specifications
        
        // For now, this is a placeholder that demonstrates the structure
        
        // Example pseudo-implementation:
        /*
        if (filterOn)
        {
            // Apply the filter to all channels
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            {
                // Configure filter coefficients based on current settings
                float qValue = resonance * 4.2f; // Map resonance to appropriate range
                qValue = jlimit(0.707f, 4.2f, qValue); // Clamp between valid range
                
                // Set filter coefficients for this channel
                filter.setParameters({cutoff, qValue, 1.0f}); // Assuming 1.0 as gain
                
                // Process the buffer channel
                filter.process(juce::dsp::ProcessContextReplacing<float>(buffer));
            }
        }
        // If filter is bypassed, do nothing - pass through unchanged
        */
    }

    void LadderFilter::setCutoff(float freq)
    {
        // Clamp frequency to valid range: 20Hz to 20kHz
        cutoff = juce::jlimit(20.0f, 20000.0f, freq);
    }

    void LadderFilter::setResonance(float res)
    {
        // Map resonance value (0.0 to 1.0) to appropriate Q factor range
        // Originally described as ~5 effective at maximum, which matches ~4.2 Q
        resonance = juce::jlimit(0.0f, 1.0f, res);
    }

    void LadderFilter::setFilterOn(bool on)
    {
        filterOn = on;
    }
    
    // A/B Testing Methods
    void LadderFilter::copyAtoB()
    {
        cutoffB = cutoffA;
        resonanceB = resonanceA;
        filterOnB = filterOnA;
    }
    
    void LadderFilter::copyBtoA()
    {
        cutoffA = cutoffB;
        resonanceA = resonanceB;
        filterOnA = filterOnB;
    }
    
    void LadderFilter::toggleAB()
    {
        isA = !isA;
        if (isA)
        {
            cutoff = cutoffA;
            resonance = resonanceA;
            filterOn = filterOnA;
        }
        else
        {
            cutoff = cutoffB;
            resonance = resonanceB;
            filterOn = filterOnB;
        }
    }
}