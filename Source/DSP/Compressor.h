#pragma once

#include <juce_dsp/juce_dsp.h>
#include <array>

namespace CrushJoints
{
    /**
        One-knob soft-knee compressor with Phat mode.

        Amount simultaneously drives threshold, ratio, attack, release and
        makeup gain. Phat mode blends the dry signal with a compressed,
        low-shelf-boosted copy.
    */
    class Compressor
    {
    public:
        Compressor();
        ~Compressor();

        void prepareToPlay(double sampleRate, int samplesPerBlock, int numChannels);
        void reset();
        void processBlock(juce::AudioBuffer<float>& buffer);

        // Set parameters
        void setAmount(float amount);
        void setPhatMode(bool on);
        void setCompressorOn(bool on);

        float getGainReductionDb() const noexcept { return gainReductionDb; }

        // A/B testing support
        void copyAtoB();
        void copyBtoA();
        void toggleAB();
        bool isAActive() const { return isA; }

    private:
        float computeEnvelope(float inputLevel);
        float computeGainDb(float envelopeDb) const;

        double sampleRate = 44100.0;
        int numChannels = 2;

        float amount = 0.0f;
        bool phatMode = false;
        bool compressorOn = true;

        // A/B parameter states
        float amountA = 0.0f;
        bool phatModeA = false;
        bool compressorOnA = true;

        float amountB = 0.0f;
        bool phatModeB = false;
        bool compressorOnB = true;

        bool isA = true;

        float envelopeState = 0.0f;
        float gainReductionDb = 0.0f;

        std::array<juce::dsp::IIR::Filter<float>, 2> phatShelf;
        juce::AudioBuffer<float> dryBuffer;
    };
}
