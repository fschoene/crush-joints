#include "PluginEditor.h"

//==============================================================================
CrushJointsAudioProcessorEditor::CrushJointsAudioProcessorEditor(CrushJointsAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize(600, 400);
}

CrushJointsAudioProcessorEditor::~CrushJointsAudioProcessorEditor()
{
}

//==============================================================================
void CrushJointsAudioProcessorEditor::paint(juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll(juce::Colours::black);

    g.setColour(juce::Colours::white);
    g.setFont(15.0f);
    g.drawFittedText("CrushJoints - CamelCrusher Clone", getLocalBounds(), juce::Justification::centred, 1);
}

void CrushJointsAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor.
}