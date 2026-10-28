#include "LadderFilter.h"
#include "CutoffCurve.h"

#include <cmath>

namespace CrushJoints
{
    namespace
    {
        constexpr float kSmoothingSeconds = 0.010f;
        constexpr float kMaxCutoffRatio = 0.45f; // of the sample rate
    }

    LadderFilter::LadderFilter()
    {
    }

    LadderFilter::~LadderFilter()
    {
    }

    void LadderFilter::prepareToPlay(double newSampleRate, int /*samplesPerBlock*/, int newNumChannels)
    {
        sampleRate = newSampleRate;
        numChannels = juce::jlimit(1, 2, newNumChannels);

        smoothedCutoff.reset(sampleRate, kSmoothingSeconds);
        smoothedResonance.reset(sampleRate, kSmoothingSeconds);

        reset();
    }

    void LadderFilter::reset()
    {
        ic1eq.fill(0.0f);
        ic2eq.fill(0.0f);
        needsSnap = true;
    }

    void LadderFilter::snapToTargets()
    {
        smoothedCutoff.setCurrentAndTargetValue(CutoffCurve::toNormalised(cutoff));
        smoothedResonance.setCurrentAndTargetValue(resonance);
        needsSnap = false;
    }

    LadderFilter::Coefficients LadderFilter::makeCoefficients(float cutoffNormalised, float res) const
    {
        const float hz = juce::jmin(CutoffCurve::toHz(cutoffNormalised),
                                    static_cast<float>(sampleRate) * kMaxCutoffRatio);
        const float g = std::tan(juce::MathConstants<float>::pi * hz / static_cast<float>(sampleRate));
        const float k = 1.0f / (1.0f + 6.0f * res); // 1 / Q, Q = 1 + 6 * resonance

        Coefficients c;
        c.a1 = 1.0f / (1.0f + g * (g + k));
        c.a2 = g * c.a1;
        c.a3 = g * c.a2;
        return c;
    }

    void LadderFilter::processBlock(juce::AudioBuffer<float>& buffer)
    {
        if (! filterOn)
        {
            wasOn = false;
            return;
        }

        if (! wasOn)
        {
            // Switching the filter on starts from a cleared state.
            ic1eq.fill(0.0f);
            ic2eq.fill(0.0f);
            wasOn = true;
        }

        if (needsSnap)
            snapToTargets();

        smoothedCutoff.setTargetValue(CutoffCurve::toNormalised(cutoff));
        smoothedResonance.setTargetValue(resonance);

        const int channels = juce::jmin(buffer.getNumChannels(), numChannels);
        const int numSamples = buffer.getNumSamples();

        std::array<float*, 2> data {};
        for (int ch = 0; ch < channels; ++ch)
            data[static_cast<size_t>(ch)] = buffer.getWritePointer(ch);

        Coefficients coefficients = makeCoefficients(smoothedCutoff.getCurrentValue(),
                                                     smoothedResonance.getCurrentValue());

        for (int i = 0; i < numSamples; ++i)
        {
            if (smoothedCutoff.isSmoothing() || smoothedResonance.isSmoothing())
            {
                coefficients = makeCoefficients(smoothedCutoff.getNextValue(),
                                                smoothedResonance.getNextValue());
            }

            for (int ch = 0; ch < channels; ++ch)
            {
                const auto idx = static_cast<size_t>(ch);
                const float v0 = data[idx][i];
                const float v3 = v0 - ic2eq[idx];
                const float v1 = coefficients.a1 * ic1eq[idx] + coefficients.a2 * v3;
                const float v2 = ic2eq[idx] + coefficients.a2 * ic1eq[idx] + coefficients.a3 * v3;

                ic1eq[idx] = 2.0f * v1 - ic1eq[idx];
                ic2eq[idx] = 2.0f * v2 - ic2eq[idx];

                data[idx][i] = v2;
            }
        }
    }

    void LadderFilter::setCutoff(float freq)
    {
        cutoff = juce::jlimit(CutoffCurve::kMinHz, CutoffCurve::kMaxHz, freq);
    }

    void LadderFilter::setResonance(float res)
    {
        resonance = juce::jlimit(0.0f, 1.0f, res);
    }

    void LadderFilter::setFilterOn(bool on)
    {
        filterOn = on;
    }
}
