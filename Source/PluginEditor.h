#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "GUI/CrushJointsLookAndFeel.h"
#include "GUI/LockButton.h"

using CrushJointsLookAndFeel = CrushJoints::CrushJointsLookAndFeel;
using LockButton = CrushJoints::LockButton;

//==============================================================================
class CrushJointsAudioProcessorEditor : public juce::AudioProcessorEditor,
                                        private juce::ChangeListener,
                                        private juce::Timer
{
public:
    explicit CrushJointsAudioProcessorEditor(CrushJointsAudioProcessor&);
    ~CrushJointsAudioProcessorEditor() override;

    //==============================================================================
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    class ContentComponent : public juce::Component
    {
    public:
        void paint(juce::Graphics& g) override;
    };

    void changeListenerCallback(juce::ChangeBroadcaster*) override;
    void timerCallback() override;
    void updateFromProcessor();
    void updateModuleVisuals();
    void layoutContent();

    void addToContent(juce::Component& component) { contentComponent.addAndMakeVisible(component); }
    void showSavePresetDialog();
    void showDeletePresetDialog();

    CrushJointsAudioProcessor& processorRef;
    juce::AudioProcessorValueTreeState& apvts;

    CrushJointsLookAndFeel lookAndFeel;
    ContentComponent contentComponent;

    // Header / preset bar
    juce::Label titleLabel;
    juce::Label abLabel;
    juce::TextButton abButtonA;
    juce::TextButton abButtonB;
    juce::TextButton copyABButton;
    juce::Label presetLabel;
    juce::ComboBox presetBox;
    juce::TextButton savePresetButton;
    juce::TextButton deletePresetButton;
    juce::TextButton randomizeButton;

    juce::Label valueReadout;

    // GUI Components
    juce::GroupComponent distortionGroup;
    juce::Label tubeLabel;
    juce::Label mechLabel;
    juce::Slider tubeSlider;
    juce::Slider mechSlider;
    LockButton tubeLock;
    LockButton mechLock;
    juce::ToggleButton tubeOnButton;

    juce::GroupComponent filterGroup;
    juce::Label cutoffLabel;
    juce::Label resLabel;
    juce::Slider cutoffSlider;
    juce::Slider resSlider;
    LockButton cutoffLock;
    LockButton resLock;
    juce::ToggleButton lpfOnButton;

    juce::GroupComponent compressorGroup;
    juce::Label amountLabel;
    juce::Slider amountSlider;
    LockButton amountLock;
    juce::ToggleButton phatButton;
    juce::ToggleButton compOnButton;

    juce::GroupComponent masterGroup;
    juce::Label volumeLabel;
    juce::Label mixLabel;
    juce::Slider volumeSlider;
    juce::Slider mixSlider;
    LockButton volumeLock;
    LockButton mixLock;
    juce::ToggleButton masterOnButton;

    // Parameter attachments keep the widgets in sync with the APVTS.
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SliderAttachment> tubeAttachment, mechAttachment;
    std::unique_ptr<SliderAttachment> cutoffAttachment, resAttachment;
    std::unique_ptr<SliderAttachment> amountAttachment;
    std::unique_ptr<SliderAttachment> volumeAttachment, mixAttachment;

    std::unique_ptr<ButtonAttachment> tubeOnAttachment, lpfOnAttachment;
    std::unique_ptr<ButtonAttachment> phatAttachment, compOnAttachment, masterOnAttachment;

    bool updatingPresetBox = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CrushJointsAudioProcessorEditor)
};
