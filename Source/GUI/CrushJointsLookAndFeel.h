#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace CrushJoints
{
    /**
        CamelCrusher-inspired dark blue / yellow theme.

        The colours are deliberately high-contrast so knobs and switches stay
        readable when a module is bypassed or the window is scaled down.
    */
    class CrushJointsLookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        CrushJointsLookAndFeel();
        ~CrushJointsLookAndFeel() override = default;

        void drawRotarySlider(juce::Graphics& g,
                              int x, int y, int width, int height,
                              float sliderPosProportional,
                              float rotaryStartAngle,
                              float rotaryEndAngle,
                              juce::Slider& slider) override;

        void drawButtonBackground(juce::Graphics& g,
                                  juce::Button& button,
                                  const juce::Colour& backgroundColour,
                                  bool shouldDrawButtonAsHighlighted,
                                  bool shouldDrawButtonAsDown) override;

        void drawToggleButton(juce::Graphics& g,
                              juce::ToggleButton& button,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override;

        void drawComboBox(juce::Graphics& g,
                          int width, int height,
                          bool isButtonDown,
                          int buttonX, int buttonY, int buttonW, int buttonH,
                          juce::ComboBox& box) override;

        void drawGroupComponentOutline(juce::Graphics& g,
                                       int width, int height,
                                       const juce::String& text,
                                       const juce::Justification& justification,
                                       juce::GroupComponent& group) override;

        static const juce::Colour background;
        static const juce::Colour panel;
        static const juce::Colour panelLight;
        static const juce::Colour outline;
        static const juce::Colour accent;
        static const juce::Colour accentBright;
        static const juce::Colour text;
        static const juce::Colour dimText;
    };
}
