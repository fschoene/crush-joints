#pragma once

#include <juce_dsp/juce_dsp.h>

namespace CrushJoints
{
    /**
        Tube + Mech distortion, following the reverse-camel black-box analysis.

        The two stages run in series exactly like the reference implementation:
        the input is shaped by the Tube table, then the result is shaped by the
        Mech table. Both stages are always active; at amount 0 their table
        entries are near-linear, so the stage is effectively transparent.
    */
    class Distortion
    {
    public:
        Distortion();
        ~Distortion();

        void prepareToPlay(double sampleRate);
        void reset();
        void processBlock(juce::AudioBuffer<float>& buffer);

        // Set parameters (normalised 0..1)
        void setTubeAmount(float amount);
        void setMechAmount(float amount);
        void setTubeOn(bool on);

        // A/B testing support
        void copyAtoB();
        void copyBtoA();
        void toggleAB();
        bool isAActive() const { return isA; }

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

        // A/B parameter states
        float tubeAmountA = 0.0f;
        float mechAmountA = 0.0f;
        bool tubeOnA = true;

        float tubeAmountB = 0.0f;
        float mechAmountB = 0.0f;
        bool tubeOnB = true;

        bool isA = true;
    };
}
