#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

//==============================================================================
class CrushJointsAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit CrushJointsAudioProcessorEditor(CrushJointsAudioProcessor&);
    ~CrushJointsAudioProcessorEditor() override;

    //==============================================================================
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void randomizeParameters();

    CrushJointsAudioProcessor& processor;
    juce::AudioProcessorValueTreeState& apvts;

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

    // Parameter attachments keep the widgets in sync with the APVTS.
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SliderAttachment> tubeAttachment, mechAttachment;
    std::unique_ptr<SliderAttachment> cutoffAttachment, resAttachment;
    std::unique_ptr<SliderAttachment> amountAttachment;
    std::unique_ptr<SliderAttachment> volumeAttachment, mixAttachment;

    std::unique_ptr<ButtonAttachment> tubeOnAttachment, lpfOnAttachment;
    std::unique_ptr<ButtonAttachment> phatAttachment, compOnAttachment, masterOnAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CrushJointsAudioProcessorEditor)
};
