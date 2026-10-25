#include "LockButton.h"
#include "CrushJointsLookAndFeel.h"

namespace CrushJoints
{
    LockButton::LockButton()
        : juce::Button("lock")
    {
        setClickingTogglesState(true);
        setTooltip("Lock this parameter so Randomize leaves it unchanged");
    }

    void LockButton::paintButton(juce::Graphics& g,
                                 bool shouldDrawButtonAsHighlighted,
                                 bool shouldDrawButtonAsDown)
    {
        auto bounds = getLocalBounds().toFloat().reduced(2.0f);
        const bool locked = getToggleState();

        auto colour = locked ? CrushJointsLookAndFeel::accent
                             : CrushJointsLookAndFeel::dimText.withAlpha(0.75f);

        if (shouldDrawButtonAsHighlighted || shouldDrawButtonAsDown)
            colour = colour.brighter(0.2f);

        g.setColour(colour);

        // Shackle: open on the left when unlocked, closed when locked.
        juce::Path shackle;
        const auto shackleRadius = bounds.getWidth() * 0.26f;
        const auto centreX = bounds.getCentreX();
        const auto centreY = bounds.getY() + bounds.getHeight() * 0.40f;
        const auto startAngle = locked ? juce::MathConstants<float>::pi
                                       : juce::MathConstants<float>::pi * 0.65f;
        shackle.addCentredArc(centreX, centreY, shackleRadius, shackleRadius,
                              0.0f, startAngle, juce::MathConstants<float>::twoPi, true);
        g.strokePath(shackle, juce::PathStrokeType(1.8f,
                                                   juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));

        auto body = bounds.withTop(bounds.getY() + bounds.getHeight() * 0.45f)
                          .reduced(bounds.getWidth() * 0.12f, 0.0f);
        g.fillRoundedRectangle(body, 2.0f);

        g.setColour(CrushJointsLookAndFeel::background);
        const auto keyholeSize = juce::jmax(1.5f, body.getWidth() * 0.16f);
        g.fillEllipse(body.getCentreX() - keyholeSize * 0.5f,
                      body.getCentreY() - keyholeSize * 0.5f,
                      keyholeSize, keyholeSize);
    }
}
