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
        
    private:
        float tubeAmount = 0.0f;
        float mechAmount = 0.0f;
        bool tubeOn = true;
        
        // Polynomial coefficient tables (from reverse-camel)
        static const float disttube_coeffs[101][4];
        static const float distmech_coeffs[101][3];
        
        // Internal processing methods
        float tubeDistort(float x, float tubeNormalized);
        float mechDistort(float y, float mechNormalized);
    };
}