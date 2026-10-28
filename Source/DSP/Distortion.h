#pragma once

#include <juce_dsp/juce_dsp.h>

namespace CrushJoints
{
    /**
        Tube + Mech distortion, using the reverse-camel coefficient tables.

        The original runs the stages in the order Mech, then Tube, and its
        Filter sits between them (docs/MEASUREMENTS.md section 7), so the two
        stages are exposed separately. processBlock() runs both back to back
        for use without a filter. At amount 0 the table entries are
        near-linear, so a stage is effectively transparent.
    */
    class Distortion
    {
    public:
        Distortion();
        ~Distortion();

        void prepareToPlay(double sampleRate);
        void reset();
        void processBlock(juce::AudioBuffer<float>& buffer);
        void processMech(juce::AudioBuffer<float>& buffer);
        void processTube(juce::AudioBuffer<float>& buffer);

        // Set parameters (normalised 0..1); the "on" switch covers both stages.
        void setTubeAmount(float amount);
        void setMechAmount(float amount);
        void setTubeOn(bool on);

    private:
        struct TubeCoeffs
        {
            float gain, a, b, c;
        };

        struct MechCoeffs
        {
            float gain, b, c;
        };

        static TubeCoeffs getTubeCoeffs(float amount);
        static MechCoeffs getMechCoeffs(float amount);
        static float shapeTube(float x, const TubeCoeffs& coeffs);
        static float shapeMech(float y, const MechCoeffs& coeffs);

        float tubeAmount = 0.0f;
        float mechAmount = 0.0f;
        bool tubeOn = true;

    };
}
