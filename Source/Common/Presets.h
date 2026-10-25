#pragma once
#include <vector>
#include <juce_core/juce_core.h>

namespace CrushJoints
{
    struct Preset
    {
        juce::String name;
        float tube;
        float mech;
        bool tubeOn;
        float cutoff;
        float res;
        bool lpfOn;
        float amount;
        bool phat;
        bool compOn;
        float volume;
        float mix;
        bool masterOn;
    };

    class Presets
    {
    public:
        void initPresets();
        int getNumPresets() const;
        Preset getPreset(int index) const;
        const std::vector<Preset>& getAllPresets() const;

    private:
        std::vector<Preset> presets;
    };
}