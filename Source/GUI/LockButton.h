#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace CrushJoints
{
    /** Small padlock button used to exclude a parameter from Randomize. */
    class LockButton : public juce::Button
    {
    public:
        LockButton();

        void paintButton(juce::Graphics& g,
                         bool shouldDrawButtonAsHighlighted,
                         bool shouldDrawButtonAsDown) override;
    };
}
