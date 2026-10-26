#pragma once

#include <juce_dsp/juce_dsp.h>
#include <array>

namespace CrushJoints
{
    /**
        One-knob soft-knee compressor with Phat mode.

        Amount simultaneously drives threshold, ratio, attack, release and
        makeup gain. The makeup gain is derived from the static compression
        curve, so it only compensates level that is actually being removed.

        Phat mode adds a second, harder parallel leg: the input is low-shelf
        boosted *before* a heavy compressor, so the bass drives the detector
        and comes out dense and sustained. That leg is crossfaded against the
        main compressor output.
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

        // Gain reduction of the main compressor in dB (<= 0, makeup excluded).
        float getGainReductionDb() const noexcept { return gainReductionDb; }

    private:
        struct Settings
        {
            float thresholdDb = 0.0f;
            float ratio = 1.0f;
            float attackCoeff = 0.0f;
            float releaseCoeff = 0.0f;
            float makeupGain = 1.0f;
        };

        Settings makeMainSettings(float amount) const;
        Settings makePhatSettings() const;
        static float staticGainDb(float levelDb, float thresholdDb, float ratio);
        static float trackEnvelope(float& state, float level, const Settings& settings);
        float timeToCoeff(float milliseconds) const;

        double sampleRate = 44100.0;
        int numChannels = 2;

        float amount = 0.0f;
        bool phatMode = false;
        bool compressorOn = true;
        bool phatWasOn = false;

        juce::SmoothedValue<float> smoothedAmount;
        juce::SmoothedValue<float> smoothedPhat;

        Settings mainSettings;
        Settings phatSettings;

        float mainEnvelope = 0.0f;
        float phatEnvelope = 0.0f;
        float gainReductionDb = 0.0f;

        std::array<juce::dsp::IIR::Filter<float>, 2> phatShelf;
    };
}
