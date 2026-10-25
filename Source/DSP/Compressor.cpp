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
        // Set default parameters
        compressor.setThreshold(0.0f);
        compressor.setRatio(1.0f);
        compressor.setAttackTime(10.0f);
        compressor.setReleaseTime(100.0f);
        compressor.setMakeupGain(0.0f);
    }

    void Compressor::processBlock(juce::AudioBuffer<float>& buffer)
    {
        // Placeholder for compressor implementation
        // In a future phase, we'll implement the compressor processing
    }

    void Compressor::setAmount(float amt)
    {
        amount = amt;
    }

    void Compressor::setPhatMode(bool on)
    {
        phatMode = on;
    }

    void Compressor::setCompressorOn(bool on)
    {
        compressorOn = on;
    }
}