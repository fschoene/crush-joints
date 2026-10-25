#include "Distortion.h"
#include "CamelCrusherCoeffs.h"

namespace CrushJoints
{
    namespace
    {
        float lerp(float a, float b, float t) noexcept
        {
            return a + (b - a) * t;
        }
    }

    Distortion::Distortion()
    {
    }

    Distortion::~Distortion()
    {
    }

    void Distortion::prepareToPlay(double sampleRate)
    {
        juce::ignoreUnused(sampleRate);
    }

    void Distortion::reset()
    {
        // The waveshapers are stateless; nothing to reset.
    }

    //==============================================================================
    // Table lookup. Interpolating between neighbouring entries keeps automated
    // parameter changes smooth while matching the reference exactly at integer
    // knob positions.
    Distortion::TubeCoeffs Distortion::getTubeCoeffs(float amount)
    {
        const float position = juce::jlimit(0.0f, 100.0f, amount * 100.0f);
        const int index0 = juce::jlimit(0, 100, static_cast<int>(position));
        const int index1 = juce::jmin(index0 + 1, 100);
        const float t = position - static_cast<float>(index0);

        const float* c0 = disttube_coeffs[index0];
        const float* c1 = disttube_coeffs[index1];

        return { lerp(c0[0], c1[0], t),
                 lerp(c0[1], c1[1], t),
                 lerp(c0[2], c1[2], t),
                 lerp(c0[3], c1[3], t) };
    }

    Distortion::MechCoeffs Distortion::getMechCoeffs(float amount)
    {
        const float position = juce::jlimit(0.0f, 100.0f, amount * 100.0f);
        const int index0 = juce::jlimit(0, 100, static_cast<int>(position));
        const int index1 = juce::jmin(index0 + 1, 100);
        const float t = position - static_cast<float>(index0);

        const float* c0 = distmech_coeffs[index0];
        const float* c1 = distmech_coeffs[index1];

        return { lerp(c0[0], c1[0], t),
                 lerp(c0[1], c1[1], t),
                 lerp(c0[2], c1[2], t) };
    }

    //==============================================================================
    float Distortion::shapeTube(float x, const TubeCoeffs& coeffs)
    {
        const float s = coeffs.gain * x;

        if (s < -0.6f)
            return coeffs.a * s * s + coeffs.b * s + coeffs.c;

        if (s > 0.6f)
            return -coeffs.a * s * s + coeffs.b * s - coeffs.c;

        return s;
    }

    float Distortion::shapeMech(float y, const MechCoeffs& coeffs)
    {
        const float s = coeffs.gain * y;

        if (s < -0.75f)
            return coeffs.b * s + coeffs.c;

        if (s > 0.75f)
            return coeffs.b * s - coeffs.c;

        return s;
    }

    //==============================================================================
    void Distortion::processBlock(juce::AudioBuffer<float>& buffer)
    {
        if (! tubeOn)
            return;

        const auto tubeCoeffs = getTubeCoeffs(tubeAmount);
        const auto mechCoeffs = getMechCoeffs(mechAmount);

        const int numChannels = juce::jmin(buffer.getNumChannels(), 2);
        const int numSamples = buffer.getNumSamples();

        for (int ch = 0; ch < numChannels; ++ch)
        {
            float* data = buffer.getWritePointer(ch);

            for (int i = 0; i < numSamples; ++i)
            {
                const float tubeOut = shapeTube(data[i], tubeCoeffs);
                data[i] = shapeMech(tubeOut, mechCoeffs);
            }
        }
    }

    void Distortion::setTubeAmount(float amount)
    {
        tubeAmount = juce::jlimit(0.0f, 1.0f, amount);
    }

    void Distortion::setMechAmount(float amount)
    {
        mechAmount = juce::jlimit(0.0f, 1.0f, amount);
    }

    void Distortion::setTubeOn(bool on)
    {
        tubeOn = on;
    }

    //==============================================================================
    // A/B testing methods
    void Distortion::copyAtoB()
    {
        tubeAmountB = tubeAmountA;
        mechAmountB = mechAmountA;
        tubeOnB = tubeOnA;
    }

    void Distortion::copyBtoA()
    {
        tubeAmountA = tubeAmountB;
        mechAmountA = mechAmountB;
        tubeOnA = tubeOnB;
    }

    void Distortion::toggleAB()
    {
        isA = ! isA;

        if (isA)
        {
            tubeAmount = tubeAmountA;
            mechAmount = mechAmountA;
            tubeOn = tubeOnA;
        }
        else
        {
            tubeAmount = tubeAmountB;
            mechAmount = mechAmountB;
            tubeOn = tubeOnB;
        }
    }
}
