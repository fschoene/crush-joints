#pragma once

#include <juce_dsp/juce_dsp.h>

namespace CrushJoints
{
    class Distortion
    {
    public:
        Distortion();
        ~Distortion();
        
        void prepareToPlay(double sampleRate);
        void processBlock(juce::AudioBuffer<float>& buffer);
        
        // Set parameters
        void setTubeAmount(float amount);
        void setMechAmount(float amount);
        void setTubeOn(bool on);
        
        // A/B testing support
        void copyAtoB();
        void copyBtoA();
        void toggleAB();
        bool isAActive() const { return isA; }
        
    private:
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
        
        bool isA = true;  // Current active state
        
        // Polynomial coefficient tables (from reverse-camel)
        static const float disttube_coeffs[201][4];
        static const float distmech_coeffs[402][3];
        
        // Internal processing methods
        float tubeDistort(float x, float tubeNormalized);
        float mechDistort(float y, float mechNormalized);
    };
}