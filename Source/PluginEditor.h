#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

//==============================================================================
class CrushJointsAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    CrushJointsAudioProcessorEditor(CrushJointsAudioProcessor&);
    ~CrushJointsAudioProcessorEditor() override;

    //==============================================================================
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    CrushJointsAudioProcessor& processor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CrushJointsAudioProcessorEditor)
};