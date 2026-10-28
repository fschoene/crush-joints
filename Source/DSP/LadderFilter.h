#pragma once

#include <juce_dsp/juce_dsp.h>
#include <array>

namespace CrushJoints
{
    /**
        Resonant 2-pole (12 dB/octave) low-pass filter.

        Measured on the original CamelCrusher (docs/MEASUREMENTS.md section 5):
        a linear bilinear-transform biquad with Q = 1 + 6 * resonance and a
        cutoff that follows the original knob curve (CutoffCurve, 80 Hz to
        16 kHz). It is implemented as a zero-delay-feedback state-variable
        filter, which has the same magnitude response and stays stable while
        the coefficients move.

        Cutoff (as a knob position) and resonance ramp linearly over 10 ms and
        the coefficients are recomputed every sample while they move, like the
        original. The class name is historical; this is not a ladder filter.
    */
    class LadderFilter
    {
    public:
        LadderFilter();
        ~LadderFilter();

        void prepareToPlay(double sampleRate, int samplesPerBlock, int numChannels);
        void reset();
        void processBlock(juce::AudioBuffer<float>& buffer);

        // Cutoff in Hz (80..16000), resonance 0..1.
        void setCutoff(float freq);
        void setResonance(float res);
        void setFilterOn(bool on);

    private:
        struct Coefficients
        {
            float a1 = 0.0f, a2 = 0.0f, a3 = 0.0f;
        };

        Coefficients makeCoefficients(float cutoffNormalised, float resonance) const;
        void snapToTargets();

        float cutoff = 16000.0f;
        float resonance = 0.0f;
        bool filterOn = false;
        bool wasOn = false;
        bool needsSnap = true;

        double sampleRate = 44100.0;
        int numChannels = 2;

        juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedCutoff;
        juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedResonance;

        std::array<float, 2> ic1eq { { 0.0f, 0.0f } };
        std::array<float, 2> ic2eq { { 0.0f, 0.0f } };
    };
}
