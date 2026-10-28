#pragma once

#include <juce_dsp/juce_dsp.h>
#include <array>

namespace CrushJoints
{
    /**
        One-knob compressor, a measured model of the original CamelCrusher
        (docs/MEASUREMENTS.md section 6).

        Per sample, stereo-linked:
          1. the inputs are clamped to +-3.3 and the detector input is the
             larger of the two channel magnitudes;
          2. the envelope follows it at a constant rate (1.0 per 48 ms up,
             1.0 per 80 ms down); in Phat mode it is the magnitude itself;
          3. the gain target is G = 1 / (1 - 0.99 * Amount) up to a knee and
             P(env) / env above it, with P(e) = 1 - c(Amount) * (1 - e)^2, so a
             full-scale signal always passes at unity;
          4. the gain drops to its target at once and rises at most
             (G - anchor) / 0.25 ms per sample, where the anchor is the gain at
             the last sample that reached its target.

        Amount changes ramp over 10 ms. Switching the module on starts from
        unity gain and an empty envelope.

        The original CrushJoints Phat design (low shelf plus a heavy parallel
        compressor, blended in with Amount) is kept as an optional "Boom" stage
        behind the compressor. It is not wired to a parameter or to the editor
        yet (see docs/VERSION_2.md).
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

        // Optional extra stage, off by default and not exposed in the plugin.
        void setBoomMode(bool on);

        // Gain applied to the most recent sample, in dB (includes makeup).
        float getGainDb() const noexcept;

    private:
        struct Curve
        {
            float makeup = 1.0f; // G
            float c = 1.0f;
            float knee = 1.0f;   // e0
        };

        static Curve makeCurve(float amount);
        float boomLegGain(float peak);
        static float boomStaticGainDb(float levelDb);

        double sampleRate = 44100.0;
        int numChannels = 2;

        float amount = 0.0f;
        bool phatMode = false;
        bool phatWasOn = false;
        bool compressorOn = true;
        bool wasOn = false;
        bool needsSnap = true;

        juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedAmount;
        Curve curve;

        float envelope = 0.0f;
        float gain = 1.0f;
        float anchor = 1.0f;

        float attackStep = 0.0f;
        float releaseStep = 0.0f;
        float rampSamples = 11.0f;

        // Boom stage (hidden)
        bool boomMode = false;
        bool boomWasOn = false;
        juce::SmoothedValue<float> smoothedBoom;
        float boomEnvelope = 0.0f;
        float boomAttackCoeff = 0.0f;
        float boomReleaseCoeff = 0.0f;
        float boomMakeup = 1.0f;
        std::array<juce::dsp::IIR::Filter<float>, 2> boomShelf;
    };
}
