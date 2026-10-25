#include "CrushJointsLookAndFeel.h"

namespace CrushJoints
{
    const juce::Colour CrushJointsLookAndFeel::background   { 0xff0b1a2e };
    const juce::Colour CrushJointsLookAndFeel::panel        { 0xff12263f };
    const juce::Colour CrushJointsLookAndFeel::panelLight   { 0xff1a3352 };
    const juce::Colour CrushJointsLookAndFeel::outline      { 0xff2c4a72 };
    const juce::Colour CrushJointsLookAndFeel::accent       { 0xfff2c94c };
    const juce::Colour CrushJointsLookAndFeel::accentBright { 0xffffe066 };
    const juce::Colour CrushJointsLookAndFeel::text         { 0xffe8eef7 };
    const juce::Colour CrushJointsLookAndFeel::dimText      { 0xff8fa6c3 };

    CrushJointsLookAndFeel::CrushJointsLookAndFeel()
    {
        setColour(juce::Slider::rotarySliderFillColourId, accent);
        setColour(juce::Slider::rotarySliderOutlineColourId, outline);
        setColour(juce::Slider::thumbColourId, accent);
        setColour(juce::Slider::trackColourId, accent);
        setColour(juce::Slider::backgroundColourId, panel);
        setColour(juce::Slider::textBoxTextColourId, text);
        setColour(juce::Slider::textBoxBackgroundColourId, background);
        setColour(juce::Slider::textBoxOutlineColourId, outline);
        setColour(juce::Slider::textBoxHighlightColourId, accent.withAlpha(0.3f));

        setColour(juce::Label::textColourId, text);

        setColour(juce::TextButton::buttonColourId, panel);
        setColour(juce::TextButton::buttonOnColourId, accent);
        setColour(juce::TextButton::textColourOffId, text);
        setColour(juce::TextButton::textColourOnId, background);

        setColour(juce::ToggleButton::textColourId, text);
        setColour(juce::ToggleButton::tickColourId, accent);
        setColour(juce::ToggleButton::tickDisabledColourId, dimText);

        setColour(juce::ComboBox::backgroundColourId, panel);
        setColour(juce::ComboBox::textColourId, text);
        setColour(juce::ComboBox::outlineColourId, outline);
        setColour(juce::ComboBox::arrowColourId, accent);
        setColour(juce::ComboBox::buttonColourId, panel);

        setColour(juce::GroupComponent::outlineColourId, outline);
        setColour(juce::GroupComponent::textColourId, accent);

        setColour(juce::PopupMenu::backgroundColourId, panel);
        setColour(juce::PopupMenu::textColourId, text);
        setColour(juce::PopupMenu::headerTextColourId, accent);
        setColour(juce::PopupMenu::highlightedBackgroundColourId, accent);
        setColour(juce::PopupMenu::highlightedTextColourId, background);

        setColour(juce::AlertWindow::backgroundColourId, panel);
        setColour(juce::AlertWindow::textColourId, text);
        setColour(juce::AlertWindow::outlineColourId, outline);
        setColour(juce::TextEditor::backgroundColourId, background);
        setColour(juce::TextEditor::textColourId, text);
        setColour(juce::TextEditor::outlineColourId, outline);
        setColour(juce::TextEditor::focusedOutlineColourId, accent);
    }

    void CrushJointsLookAndFeel::drawRotarySlider(juce::Graphics& g,
                                                  int x, int y, int width, int height,
                                                  float sliderPosProportional,
                                                  float rotaryStartAngle,
                                                  float rotaryEndAngle,
                                                  juce::Slider& slider)
    {
        const auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(4.0f);
        const auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;
        const auto centre = bounds.getCentre();
        const auto lineWidth = juce::jmax(2.0f, radius * 0.13f);
        const auto arcRadius = radius - lineWidth * 0.5f;

        const auto angle = rotaryStartAngle
                         + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

        const auto trackColour = slider.isEnabled()
                                   ? slider.findColour(juce::Slider::rotarySliderOutlineColourId)
                                   : outline.withAlpha(0.45f);
        const auto fillColour = slider.isEnabled()
                                  ? slider.findColour(juce::Slider::rotarySliderFillColourId)
                                  : dimText.withAlpha(0.55f);

        juce::Path track;
        track.addCentredArc(centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                            rotaryStartAngle, rotaryEndAngle, true);
        g.setColour(trackColour);
        g.strokePath(track, juce::PathStrokeType(lineWidth,
                                                 juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));

        if (sliderPosProportional > 0.0001f)
        {
            juce::Path valueArc;
            valueArc.addCentredArc(centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                                   rotaryStartAngle, angle, true);
            g.setColour(fillColour);
            g.strokePath(valueArc, juce::PathStrokeType(lineWidth,
                                                        juce::PathStrokeType::curved,
                                                        juce::PathStrokeType::rounded));
        }

        const auto bodyRadius = juce::jmax(1.0f, arcRadius - lineWidth * 0.95f);

        g.setColour((slider.isMouseOverOrDragging() ? panelLight.brighter(0.12f) : panelLight));
        g.fillEllipse(centre.x - bodyRadius, centre.y - bodyRadius,
                      bodyRadius * 2.0f, bodyRadius * 2.0f);

        g.setColour(outline);
        g.drawEllipse(centre.x - bodyRadius, centre.y - bodyRadius,
                      bodyRadius * 2.0f, bodyRadius * 2.0f,
                      juce::jmax(1.0f, lineWidth * 0.45f));

        juce::Path pointer;
        const auto pointerWidth = juce::jmax(1.6f, lineWidth * 0.55f);
        pointer.addRoundedRectangle(-pointerWidth * 0.5f,
                                    -bodyRadius * 0.84f,
                                    pointerWidth,
                                    bodyRadius * 0.52f,
                                    pointerWidth * 0.5f);
        pointer.applyTransform(juce::AffineTransform::rotation(angle).translated(centre));
        g.setColour(slider.isEnabled() ? accent : dimText);
        g.fillPath(pointer);
    }

    void CrushJointsLookAndFeel::drawButtonBackground(juce::Graphics& g,
                                                      juce::Button& button,
                                                      const juce::Colour&,
                                                      bool shouldDrawButtonAsHighlighted,
                                                      bool shouldDrawButtonAsDown)
    {
        auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);
        const bool isOn = button.getToggleState();

        auto fill = isOn ? accent : panel;

        if (shouldDrawButtonAsDown)
            fill = fill.brighter(0.15f);
        else if (shouldDrawButtonAsHighlighted)
            fill = fill.brighter(0.08f);

        g.setColour(fill);
        g.fillRoundedRectangle(bounds, 4.0f);

        g.setColour(isOn ? accentBright : outline);
        g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
    }

    void CrushJointsLookAndFeel::drawToggleButton(juce::Graphics& g,
                                                  juce::ToggleButton& button,
                                                  bool shouldDrawButtonAsHighlighted,
                                                  bool shouldDrawButtonAsDown)
    {
        auto bounds = button.getLocalBounds().reduced(1);
        auto tickBounds = bounds.removeFromLeft(bounds.getHeight()).reduced(3);

        g.setColour(button.getToggleState() ? accent
                                            : (shouldDrawButtonAsHighlighted || shouldDrawButtonAsDown
                                                   ? panelLight.brighter(0.12f)
                                                   : panel));
        g.fillRoundedRectangle(tickBounds.toFloat(), 3.0f);

        g.setColour(button.getToggleState() ? accentBright : outline);
        g.drawRoundedRectangle(tickBounds.toFloat(), 3.0f, 1.0f);

        g.setColour(button.findColour(juce::ToggleButton::textColourId));
        g.setFont(juce::Font(juce::FontOptions(13.0f, juce::Font::bold)));
        g.drawText(button.getButtonText(), bounds.reduced(6, 0),
                   juce::Justification::centredLeft, true);
    }

    void CrushJointsLookAndFeel::drawComboBox(juce::Graphics& g,
                                              int width, int height,
                                              bool isButtonDown,
                                              int buttonX, int buttonY,
                                              int buttonW, int buttonH,
                                              juce::ComboBox& box)
    {
        juce::ignoreUnused(buttonX, buttonY, buttonW, buttonH);

        auto bounds = juce::Rectangle<int>(0, 0, width, height).toFloat().reduced(1.0f);
        g.setColour(isButtonDown ? panelLight : panel);
        g.fillRoundedRectangle(bounds, 4.0f);
        g.setColour(outline);
        g.drawRoundedRectangle(bounds, 4.0f, 1.0f);

        const auto arrow = juce::Rectangle<float>(static_cast<float>(width) - 20.0f, 0.0f, 14.0f, static_cast<float>(height));
        juce::Path path;
        path.addTriangle(arrow.getX(), arrow.getCentreY() - 2.0f,
                         arrow.getRight(), arrow.getCentreY() - 2.0f,
                         arrow.getCentreX(), arrow.getCentreY() + 4.0f);
        g.setColour(box.isEnabled() ? accent : dimText);
        g.fillPath(path);
    }

    void CrushJointsLookAndFeel::drawGroupComponentOutline(juce::Graphics& g,
                                                           int width, int height,
                                                           const juce::String& componentText,
                                                           const juce::Justification&,
                                                           juce::GroupComponent&)
    {
        auto bounds = juce::Rectangle<int>(0, 0, width, height).toFloat().reduced(1.0f);

        g.setColour(panel);
        g.fillRoundedRectangle(bounds, 6.0f);
        g.setColour(outline);
        g.drawRoundedRectangle(bounds, 6.0f, 1.0f);

        if (componentText.isNotEmpty())
        {
            g.setColour(accent);
            g.setFont(juce::Font(juce::FontOptions(13.0f, juce::Font::bold)));
            g.drawText(componentText, bounds.removeFromTop(20.0f).withTrimmedLeft(10.0f),
                       juce::Justification::centredLeft, true);
        }
    }
}
