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

    // GUI Components
    juce::GroupComponent distortionGroup;
    juce::Slider tubeSlider;
    juce::Slider mechSlider;
    juce::ToggleButton tubeOnButton;
    
    juce::GroupComponent filterGroup;
    juce::Slider cutoffSlider;
    juce::Slider resSlider;
    juce::ToggleButton lpfOnButton;
    
    juce::GroupComponent compressorGroup;
    juce::Slider amountSlider;
    juce::ToggleButton phatButton;
    juce::ToggleButton compOnButton;
    
    juce::GroupComponent masterGroup;
    juce::Slider volumeSlider;
    juce::Slider mixSlider;
    juce::ToggleButton masterOnButton;
    
    juce::Label presetLabel;
    juce::ComboBox presetBox;
    juce::TextButton randomizeButton;
    
    juce::LevelMeter levelMeter;
    
    // Component management
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CrushJointsAudioProcessorEditor)
};