#pragma once

#include <juce_core/juce_core.h>
#include <vector>

namespace CrushJoints
{
    struct Preset
    {
        juce::String name;
        float tube = 0.0f;
        float mech = 0.0f;
        bool tubeOn = true;
        float cutoff = 20000.0f;
        float res = 0.0f;
        bool lpfOn = false;
        float amount = 0.0f;
        bool phat = false;
        bool compOn = true;
        float volume = 0.0f;
        float mix = 1.0f;
        bool masterOn = true;
        bool isUser = false;
    };

    /**
        Preset bank: the 12 factory patches plus user patches loaded from an
        XML file in the user application data directory.

        User presets are stored outside the plugin binary so they survive
        rebuilds. Program indices are factory presets first, then user presets
        in the order they were added.
    */
    class Presets
    {
    public:
        explicit Presets(juce::File userPresetFile = getDefaultUserPresetFile());

        void initPresets();

        int getNumPresets() const;
        int getNumFactoryPresets() const;
        int getNumUserPresets() const;

        Preset getPreset(int index) const;

        bool isUserPreset(int index) const;
        bool isFactoryPreset(int index) const;

        /** Adds a user preset or overwrites a user preset with the same name.
            Returns the combined program index, or -1 on failure. */
        int saveUserPreset(Preset preset);

        /** Deletes a user preset. Factory presets cannot be deleted. */
        bool deleteUserPreset(int index);

        /** Renames a user preset. Factory presets cannot be renamed. */
        bool renameUserPreset(int index, const juce::String& newName);

        static juce::File getDefaultUserPresetFile();

        const juce::File& getUserPresetFile() const noexcept { return userPresetFile; }

    private:
        void createFactoryPresets();
        void loadUserPresets();
        bool writeUserPresets(const std::vector<Preset>& presetsToWrite) const;

        std::vector<Preset> factoryPresets;
        std::vector<Preset> userPresets;
        juce::File userPresetFile;
    };
}
