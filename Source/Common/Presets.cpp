#include "Presets.h"

#include "CutoffCurve.h"

#include <cmath>

namespace CrushJoints
{
    namespace
    {
        struct LegacyPresetData
        {
            const char* name;
            float distOn;
            float mech;
            float tube;
            float filterOn;
            float cutoffNormalized;
            float res;
            float compOn;
            float amount;
            float phat;
            float mix;
            float volumeNormalized;
        };

        // Original 20-program factory bank. Values are the normalized VST2
        // parameters from the original CamelCrusherPresetsA.fxb (verified
        // 2026-10-28); the cutoff and volume knob laws were measured on the
        // original (docs/MEASUREMENTS.md).
        constexpr LegacyPresetData kLegacyPresets[] = {
            { "Annihilate",        1, 0.000f, 0.340f, 0, 0.350f, 0.050f, 1, 0.905f, 1, 1.000f, 0.780f },
            { "American Crunch",   1, 0.280f, 1.000f, 1, 1.000f, 0.475f, 1, 0.815f, 1, 1.000f, 0.655f },
            { "American HiGain",   1, 1.000f, 1.000f, 1, 0.445f, 0.725f, 1, 0.815f, 1, 1.000f, 0.605f },
            { "British Clean",     1, 0.050f, 0.075f, 1, 1.000f, 0.000f, 1, 0.710f, 0, 1.000f, 0.840f },
            { "British Crunch",    1, 0.280f, 1.000f, 1, 1.000f, 0.050f, 1, 0.815f, 1, 1.000f, 0.645f },
            { "British HiGain",    1, 1.000f, 1.000f, 1, 1.000f, 1.000f, 1, 1.000f, 1, 1.000f, 0.640f },
            { "Bass Maker",        1, 0.020f, 0.625f, 1, 0.170f, 0.125f, 1, 0.725f, 1, 1.000f, 0.775f },
            { "Clean Rainbow",     0, 0.020f, 0.000f, 1, 0.825f, 1.000f, 1, 0.910f, 0, 1.000f, 0.700f },
            { "Tube Warmth",       1, 0.000f, 0.370f, 1, 1.000f, 0.190f, 0, 0.000f, 0, 1.000f, 1.000f },
            { "Dark Crunch",       1, 0.895f, 0.000f, 1, 0.945f, 0.170f, 1, 0.910f, 0, 1.000f, 0.615f },
            { "Destroyer",         1, 0.800f, 0.940f, 1, 0.580f, 1.000f, 1, 0.910f, 1, 1.000f, 0.600f },
            { "Ultra Phat",        1, 0.180f, 0.985f, 0, 1.000f, 0.175f, 1, 0.670f, 0, 1.000f, 0.665f },
            { "Distort",           1, 1.000f, 1.000f, 1, 0.690f, 1.000f, 1, 0.910f, 1, 1.000f, 0.595f },
            { "Dream",             0, 0.020f, 0.000f, 1, 0.765f, 0.935f, 1, 0.910f, 0, 1.000f, 0.650f },
            { "More Beef",         1, 0.000f, 0.200f, 0, 0.500f, 0.000f, 1, 0.780f, 0, 1.000f, 0.760f },
            { "Santana Row",       1, 0.440f, 0.580f, 1, 0.615f, 0.935f, 0, 0.910f, 0, 1.000f, 0.700f },
            { "Fuzz Filter",       1, 0.205f, 0.760f, 1, 0.430f, 0.380f, 1, 0.433f, 1, 0.270f, 1.000f },
            { "Subtle Master",     1, 0.000f, 0.105f, 1, 1.000f, 0.095f, 1, 0.100f, 0, 1.000f, 1.000f },
            { "YouWereWarned",     1, 0.940f, 0.965f, 1, 0.710f, 0.345f, 1, 0.390f, 1, 0.685f, 0.645f },
            { "Turn it to 11",     1, 1.000f, 1.000f, 1, 0.695f, 1.000f, 1, 1.000f, 1, 1.000f, 0.555f },
        };

        float legacyCutoffToHz(float normalized)
        {
            // The original knob curve, 80 Hz .. 16 kHz (docs/MEASUREMENTS.md).
            return CutoffCurve::toHz(normalized);
        }

        float legacyVolumeToDb(float normalized)
        {
            // Measured on the original: linear gain = knob ^ 2.737 (knob 0.5 is 0.15).
            // The original tops out at 0 dB; our -24..+12 dB range contains it. The
            // 20 factory presets use knob values of 0.555 or more (-14.3 dB and up).
            constexpr float kExponent = 2.737f;
            const float knob = juce::jlimit(0.0f, 1.0f, normalized);

            if (knob <= 0.0f)
                return -24.0f;

            return juce::jlimit(-24.0f, 12.0f, 20.0f * kExponent * std::log10(knob));
        }
    }

    void Presets::createFactoryPresets()
    {
        factoryPresets.push_back({
            "Default",          // name
            0.0f,               // tube
            0.0f,               // mech 
            true,               // tubeOn
            16000.0f,           // cutoff
            0.0f,               // res
            false,              // lpfOn
            0.0f,               // amount
            false,              // phat
            true,               // compOn
            0.0f,               // volume
            1.0f,               // mix
            true,               // masterOn
            PresetCategory::defaultPreset // category
        });
        
        factoryPresets.push_back({
            "DRM BigBadBeat",   // name
            0.5f,               // tube
            0.3f,               // mech
            true,               // tubeOn
            4000.0f,            // cutoff
            0.2f,               // res
            true,               // lpfOn
            0.6f,               // amount
            true,               // phat
            true,               // compOn
            0.0f,               // volume
            1.0f,               // mix
            true                // masterOn
        });
        
        factoryPresets.push_back({
            "BAS FatSub",       // name
            0.7f,               // tube
            0.0f,               // mech
            true,               // tubeOn
            800.0f,             // cutoff
            0.3f,               // res
            true,               // lpfOn
            0.5f,               // amount
            true,               // phat
            true,               // compOn
            0.0f,               // volume
            1.0f,               // mix
            true                // masterOn
        });
        
        factoryPresets.push_back({
            "GTR Crunch",       // name
            0.4f,               // tube
            0.5f,               // mech
            true,               // tubeOn
            6000.0f,            // cutoff
            0.1f,               // res
            true,               // lpfOn
            0.3f,               // amount
            false,              // phat
            true,               // compOn
            0.0f,               // volume
            1.0f,               // mix
            true                // masterOn
        });

        factoryPresets.push_back({
            "SYN WarmPad",      // name
            0.3f,               // tube
            0.0f,               // mech
            true,               // tubeOn
            3000.0f,            // cutoff
            0.4f,               // res
            true,               // lpfOn
            0.2f,               // amount
            false,              // phat
            true,               // compOn
            0.0f,               // volume
            1.0f,               // mix
            true                // masterOn
        });

        factoryPresets.push_back({
            "VOX Gritty",       // name
            0.6f,               // tube
            0.2f,               // mech
            true,               // tubeOn
            5000.0f,            // cutoff
            0.15f,              // res
            true,               // lpfOn
            0.4f,               // amount
            false,              // phat
            true,               // compOn
            0.0f,               // volume
            1.0f,               // mix
            true                // masterOn
        });

        factoryPresets.push_back({
            "MAS Loudener",     // name
            0.2f,               // tube
            0.1f,               // mech
            true,               // tubeOn
            12000.0f,           // cutoff
            0.0f,               // res
            true,               // lpfOn
            0.7f,               // amount
            true,               // phat
            true,               // compOn
            2.0f,               // volume
            1.0f,               // mix
            true                // masterOn
        });

        factoryPresets.push_back({
            "DRM LoFiHit",      // name
            0.8f,               // tube
            0.6f,               // mech
            true,               // tubeOn
            2000.0f,            // cutoff
            0.5f,               // res
            true,               // lpfOn
            0.4f,               // amount
            false,              // phat
            true,               // compOn
            -2.0f,              // volume
            1.0f,               // mix
            true                // masterOn
        });

        factoryPresets.push_back({
            "SYN Screamer",     // name
            0.3f,               // tube
            0.8f,               // mech
            true,               // tubeOn
            7000.0f,            // cutoff
            0.6f,               // res
            true,               // lpfOn
            0.5f,               // amount
            true,               // phat
            true,               // compOn
            -1.0f,              // volume
            1.0f,               // mix
            true                // masterOn
        });

        factoryPresets.push_back({
            "Gentle Warmth",    // name
            0.2f,               // tube
            0.0f,               // mech
            true,               // tubeOn
            15000.0f,           // cutoff
            0.0f,               // res
            true,               // lpfOn
            0.1f,               // amount
            false,              // phat
            true,               // compOn
            0.0f,               // volume
            0.5f,               // mix
            true                // masterOn
        });

        factoryPresets.push_back({
            "Telephone",        // name
            0.1f,               // tube
            0.0f,               // mech
            true,               // tubeOn
            3500.0f,            // cutoff
            0.3f,               // res
            true,               // lpfOn
            0.0f,               // amount
            false,              // phat
            false,              // compOn
            0.0f,               // volume
            1.0f,               // mix
            true                // masterOn
        });

        factoryPresets.push_back({
            "Destroy Everything", // name
            1.0f,               // tube
            1.0f,               // mech
            true,               // tubeOn
            1000.0f,            // cutoff
            0.7f,               // res
            true,               // lpfOn
            0.8f,               // amount
            true,               // phat
            true,               // compOn
            -4.0f,              // volume
            1.0f,               // mix
            true                // masterOn
        });
    }

    Presets::Presets(juce::File file)
        : userPresetFile(std::move(file))
    {
    }

    juce::File Presets::getDefaultUserPresetFile()
    {
        return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                   .getChildFile("CrushJoints")
                   .getChildFile("Presets.xml");
    }

    void Presets::initPresets()
    {
        // Factory presets are rebuilt from code every time; user presets are
        // loaded from disk after them.
        factoryPresets.clear();
        userPresets.clear();
        createFactoryPresets();
        createLegacyPresets();
        loadUserPresets();
    }

    int Presets::getNumPresets() const
    {
        return static_cast<int>(factoryPresets.size() + userPresets.size());
    }

    int Presets::getNumFactoryPresets() const
    {
        return static_cast<int>(factoryPresets.size());
    }

    int Presets::getNumUserPresets() const
    {
        return static_cast<int>(userPresets.size());
    }

    int Presets::getNumLegacyPresets() const
    {
        int count = 0;

        for (const auto& preset : factoryPresets)
            if (preset.category == PresetCategory::legacy)
                ++count;

        return count;
    }

    int Presets::getNumCrushClonePresets() const
    {
        int count = 0;

        for (const auto& preset : factoryPresets)
            if (preset.category == PresetCategory::crushClone)
                ++count;

        return count;
    }

    PresetCategory Presets::getPresetCategory(int index) const
    {
        if (index < 0 || index >= getNumPresets())
            return PresetCategory::crushClone;

        return isFactoryPreset(index)
                   ? factoryPresets[static_cast<size_t>(index)].category
                   : PresetCategory::user;
    }

    Preset Presets::getPreset(int index) const
    {
        if (index >= 0 && index < static_cast<int>(factoryPresets.size()))
            return factoryPresets[static_cast<size_t>(index)];

        const int userIndex = index - static_cast<int>(factoryPresets.size());

        if (userIndex >= 0 && userIndex < static_cast<int>(userPresets.size()))
            return userPresets[static_cast<size_t>(userIndex)];

        return factoryPresets.empty() ? Preset{} : factoryPresets.front();
    }

    bool Presets::isFactoryPreset(int index) const
    {
        return index >= 0 && index < static_cast<int>(factoryPresets.size());
    }

    bool Presets::isUserPreset(int index) const
    {
        const int userIndex = index - static_cast<int>(factoryPresets.size());
        return userIndex >= 0 && userIndex < static_cast<int>(userPresets.size());
    }

    void Presets::createLegacyPresets()
    {
        for (const auto& legacy : kLegacyPresets)
        {
            Preset preset;
            preset.name = legacy.name;
            preset.tube = legacy.tube;
            preset.mech = legacy.mech;
            preset.tubeOn = legacy.distOn >= 0.5f;
            preset.cutoff = legacyCutoffToHz(legacy.cutoffNormalized);
            preset.res = legacy.res;
            preset.lpfOn = legacy.filterOn >= 0.5f;
            preset.amount = legacy.amount;
            preset.phat = legacy.phat >= 0.5f;
            preset.compOn = legacy.compOn >= 0.5f;
            preset.volume = legacyVolumeToDb(legacy.volumeNormalized);
            preset.mix = legacy.mix;
            preset.masterOn = true;
            preset.category = PresetCategory::legacy;
            factoryPresets.push_back(preset);
        }
    }

    int Presets::saveUserPreset(Preset preset)
    {
        preset.name = preset.name.trim().substring(0, 64);
        preset.category = PresetCategory::user;

        if (preset.name.isEmpty())
            return -1;

        auto updated = userPresets;
        int userIndex = -1;

        for (int i = 0; i < static_cast<int>(updated.size()); ++i)
        {
            if (updated[static_cast<size_t>(i)].name.equalsIgnoreCase(preset.name))
            {
                userIndex = i;
                break;
            }
        }

        if (userIndex >= 0)
            updated[static_cast<size_t>(userIndex)] = preset;
        else
        {
            updated.push_back(preset);
            userIndex = static_cast<int>(updated.size()) - 1;
        }

        if (! writeUserPresets(updated))
            return -1;

        userPresets = std::move(updated);
        return static_cast<int>(factoryPresets.size()) + userIndex;
    }

    bool Presets::deleteUserPreset(int index)
    {
        const int userIndex = index - static_cast<int>(factoryPresets.size());

        if (userIndex < 0 || userIndex >= static_cast<int>(userPresets.size()))
            return false;

        auto updated = userPresets;
        updated.erase(updated.begin() + userIndex);

        if (! writeUserPresets(updated))
            return false;

        userPresets = std::move(updated);
        return true;
    }

    bool Presets::renameUserPreset(int index, const juce::String& newName)
    {
        const int userIndex = index - static_cast<int>(factoryPresets.size());

        if (userIndex < 0 || userIndex >= static_cast<int>(userPresets.size()))
            return false;

        const auto trimmed = newName.trim().substring(0, 64);

        if (trimmed.isEmpty())
            return false;

        for (int i = 0; i < static_cast<int>(userPresets.size()); ++i)
            if (i != userIndex && userPresets[static_cast<size_t>(i)].name.equalsIgnoreCase(trimmed))
                return false;

        auto updated = userPresets;
        updated[static_cast<size_t>(userIndex)].name = trimmed;

        if (! writeUserPresets(updated))
            return false;

        userPresets = std::move(updated);
        return true;
    }

    void Presets::loadUserPresets()
    {
        userPresets.clear();

        if (! userPresetFile.existsAsFile())
            return;

        auto xml = juce::XmlDocument::parse(userPresetFile);

        if (xml == nullptr || ! xml->hasTagName("CRUSHJOINTS_PRESETS"))
            return;

        for (auto* element = xml->getFirstChildElement();
             element != nullptr;
             element = element->getNextElement())
        {
            if (! element->hasTagName("PRESET"))
                continue;

            Preset preset;
            preset.name = element->getStringAttribute("name").trim().substring(0, 64);

            if (preset.name.isEmpty())
                continue;

            preset.tube = juce::jlimit(0.0f, 1.0f, static_cast<float>(element->getDoubleAttribute("tube", 0.0)));
            preset.mech = juce::jlimit(0.0f, 1.0f, static_cast<float>(element->getDoubleAttribute("mech", 0.0)));
            preset.tubeOn = element->getBoolAttribute("tubeOn", true);
            preset.cutoff = juce::jlimit(CutoffCurve::kMinHz, CutoffCurve::kMaxHz, static_cast<float>(element->getDoubleAttribute("cutoff", CutoffCurve::kMaxHz)));
            preset.res = juce::jlimit(0.0f, 1.0f, static_cast<float>(element->getDoubleAttribute("res", 0.0)));
            preset.lpfOn = element->getBoolAttribute("lpfOn", false);
            preset.amount = juce::jlimit(0.0f, 1.0f, static_cast<float>(element->getDoubleAttribute("amount", 0.0)));
            preset.phat = element->getBoolAttribute("phat", false);
            preset.compOn = element->getBoolAttribute("compOn", true);
            preset.volume = juce::jlimit(-24.0f, 12.0f, static_cast<float>(element->getDoubleAttribute("volume", 0.0)));
            preset.mix = juce::jlimit(0.0f, 1.0f, static_cast<float>(element->getDoubleAttribute("mix", 1.0)));
            preset.masterOn = element->getBoolAttribute("masterOn", true);
            preset.category = PresetCategory::user;

            bool duplicate = false;

            for (const auto& existing : userPresets)
            {
                if (existing.name.equalsIgnoreCase(preset.name))
                {
                    duplicate = true;
                    break;
                }
            }

            if (! duplicate)
                userPresets.push_back(preset);
        }
    }

    bool Presets::writeUserPresets(const std::vector<Preset>& presetsToWrite) const
    {
        juce::XmlElement root("CRUSHJOINTS_PRESETS");
        root.setAttribute("version", 1);

        for (const auto& preset : presetsToWrite)
        {
            auto* element = root.createNewChildElement("PRESET");

            element->setAttribute("name", preset.name);
            element->setAttribute("tube", preset.tube);
            element->setAttribute("mech", preset.mech);
            element->setAttribute("tubeOn", preset.tubeOn ? 1 : 0);
            element->setAttribute("cutoff", preset.cutoff);
            element->setAttribute("res", preset.res);
            element->setAttribute("lpfOn", preset.lpfOn ? 1 : 0);
            element->setAttribute("amount", preset.amount);
            element->setAttribute("phat", preset.phat ? 1 : 0);
            element->setAttribute("compOn", preset.compOn ? 1 : 0);
            element->setAttribute("volume", preset.volume);
            element->setAttribute("mix", preset.mix);
            element->setAttribute("masterOn", preset.masterOn ? 1 : 0);
        }

        const auto parent = userPresetFile.getParentDirectory();

        if (! parent.exists() && ! parent.createDirectory().wasOk())
            return false;

        return root.writeTo(userPresetFile);
    }
}
