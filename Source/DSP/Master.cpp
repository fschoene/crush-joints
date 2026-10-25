#include "Master.h"

namespace CrushJoints
{
    Master::Master()
    {
    }

    Master::~Master()
    {
    }

    void Master::prepareToPlay(double sampleRate)
    {
        gain.prepare({static_cast<float>(sampleRate), 2, 1});
        limiterChain.prepare({static_cast<float>(sampleRate), 2, 1});
        
        // Set up limiter with 8x headroom (±2.0)
        limiterGain.setGainLinear(0.5f); // This corresponds to ±2.0 limit
    }

    void Master::processBlock(juce::AudioBuffer<float>& buffer)
    {
        // TODO: Implement master processing with:
        // 1. Volume control (-24dB to +12dB)
        // 2. Dry/wet mix control
        // 3. Global on/off switch
        // 4. Hard safety limiter at ±2.0
        // 5. Denormal/NaN/Inf killing
        
        // For now, this demonstrates the structure
        /*
        if (masterOn)
        {
            // Apply volume control
            gain.setGainDecibels(volume);
            gain.process(juce::dsp::ProcessContextReplacing<float>(buffer));
            
            // Apply mix (dry/wet blending)
            // This would blend wet signal with dry signal, 
            // but implementation would depend on how this is integrated
            
            // Apply safety limiter with ±2.0 headroom
            // limiterChain.process(...);
        }
        // If bypassed, only denormal killing would occur
        */
    }

    void Master::setVolume(float volumeDb)
    {
        // Clamp volume between -24dB and +12dB
        volume = juce::jlimit(-24.0f, 12.0f, volumeDb);
    }

    void Master::setMix(float mixVal)
    {
        // Clamp mix between 0.0 and 1.0
        mix = juce::jlimit(0.0f, 1.0f, mixVal);
    }

    void Master::setMasterOn(bool on)
    {
        masterOn = on;
    }
    
    // A/B Testing Methods
    void Master::copyAtoB()
    {
        volumeB = volumeA;
        mixB = mixA;
        masterOnB = masterOnA;
    }
    
    void Master::copyBtoA()
    {
        volumeA = volumeB;
        mixA = mixB;
        masterOnA = masterOnB;
    }
    
    void Master::toggleAB()
    {
        isA = !isA;
        if (isA)
        {
            volume = volumeA;
            mix = mixA;
            masterOn = masterOnA;
        }
        else
        {
            volume = volumeB;
            mix = mixB;
            masterOn = masterOnB;
        }
    }
}