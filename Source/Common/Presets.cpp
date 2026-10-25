#include "Presets.h"

namespace CrushJoints
{
    void Presets::initPresets()
    {
        presets.push_back({
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
        
        presets.push_back({
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
        
        presets.push_back({
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
        
        presets.push_back({
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

        presets.push_back({
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

        presets.push_back({
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

        presets.push_back({
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

        presets.push_back({
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

        presets.push_back({
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

        presets.push_back({
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

        presets.push_back({
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

        presets.push_back({
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

    int Presets::getNumPresets() const
    {
        return static_cast<int>(presets.size());
    }

    Preset Presets::getPreset(int index) const
    {
        if (index >= 0 && index < static_cast<int>(presets.size()))
            return presets[static_cast<size_t>(index)];
        return presets[0]; // Return default preset if index is invalid
    }

    const std::vector<Preset>& Presets::getAllPresets() const
    {
        return presets;
    }
}