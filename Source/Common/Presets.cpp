#include "Presets.h"

namespace CrushJoints
{
    void Presets::createFactoryPresets()
    {
        factoryPresets.push_back({
            "Default",          // name
            0.0f,               // tube
            0.0f,               // mech 
            true,               // tubeOn
            20000.0f,           // cutoff
            0.0f,               // res
            false,              // lpfOn
            0.0f,               // amount
            false,              // phat
            true,               // compOn
            0.0f,               // volume
            1.0f,               // mix
            true                // masterOn
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

    int Presets::saveUserPreset(Preset preset)
    {
        preset.name = preset.name.trim().substring(0, 64);
        preset.isUser = true;

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
            preset.cutoff = juce::jlimit(20.0f, 20000.0f, static_cast<float>(element->getDoubleAttribute("cutoff", 20000.0)));
            preset.res = juce::jlimit(0.0f, 1.0f, static_cast<float>(element->getDoubleAttribute("res", 0.0)));
            preset.lpfOn = element->getBoolAttribute("lpfOn", false);
            preset.amount = juce::jlimit(0.0f, 1.0f, static_cast<float>(element->getDoubleAttribute("amount", 0.0)));
            preset.phat = element->getBoolAttribute("phat", false);
            preset.compOn = element->getBoolAttribute("compOn", true);
            preset.volume = juce::jlimit(-24.0f, 12.0f, static_cast<float>(element->getDoubleAttribute("volume", 0.0)));
            preset.mix = juce::jlimit(0.0f, 1.0f, static_cast<float>(element->getDoubleAttribute("mix", 1.0)));
            preset.masterOn = element->getBoolAttribute("masterOn", true);
            preset.isUser = true;

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
