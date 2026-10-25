#include "Compressor.h"

namespace CrushJoints
{
    Compressor::Compressor()
    {
    }

    Compressor::~Compressor()
    {
    }

    void Compressor::prepareToPlay(double sampleRate)
    {
        compressor.prepare({static_cast<float>(sampleRate), 2, 1});
        
        // Setup phat chain for later
        phatChain.prepare({static_cast<float>(sampleRate), 2, 1});
        
        // Set default parameters
        compressor.setThreshold(0.0f);
        compressor.setRatio(1.0f);
        compressor.setAttackTime(10.0f);
        compressor.setReleaseTime(100.0f);
        compressor.setMakeupGain(0.0f);
        
        // Setup phat gain
        phatGain.setGainDecibels(8.0f); // +8dB boost
    }

    void Compressor::processBlock(juce::AudioBuffer<float>& buffer)
    {
        // TODO: Implement compressor processing with:
        // 1. Amount parameter mapping to threshold/ratio/attack/release/makeup gain
        // 2. Phat mode processing when enabled
        // 3. Gain reduction metering
        // 4. Proper bypass handling
        
        // For now, this demonstrates the structure
        
        /*
        if (compressorOn)
        {
            // Process with current compressor settings
            // In a real implementation, we'd:
            // 1. Calculate parameters based on amount (0-1)
            // 2. Apply the processor chain
            // 3. Handle phat mode blending
            
            // This is a placeholder that shows the conceptual structure
            if (phatMode)
            {
                // Apply phat processing when enabled
                // This would blend compressed signal with dry signal
                // with a low shelf filter boost at 120 Hz
            }
            
            // Process the audio through the main compressor
            // compressor.process(...);
        }
        // If bypassed, do nothing - pass through unchanged
        */
    }

    void Compressor::setAmount(float amt)
    {
        amount = juce::jlimit(0.0f, 1.0f, amt);
        // In a full implementation, this would map amount to 
        // threshold, ratio, attack, release, and makeup gain
    }

    void Compressor::setPhatMode(bool on)
    {
        phatMode = on;
    }

    void Compressor::setCompressorOn(bool on)
    {
        compressorOn = on;
    }
    
    // A/B Testing Methods
    void Compressor::copyAtoB()
    {
        amountB = amountA;
        phatModeB = phatModeA;
        compressorOnB = compressorOnA;
    }
    
    void Compressor::copyBtoA()
    {
        amountA = amountB;
        phatModeA = phatModeB;
        compressorOnA = compressorOnB;
    }
    
    void Compressor::toggleAB()
    {
        isA = !isA;
        if (isA)
        {
            amount = amountA;
            phatMode = phatModeA;
            compressorOn = compressorOnA;
        }
        else
        {
            amount = amountB;
            phatMode = phatModeB;
            compressorOn = compressorOnB;
        }
    }
}