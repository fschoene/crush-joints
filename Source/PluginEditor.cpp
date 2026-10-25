#include "PluginEditor.h"

namespace
{
    void configureKnob(juce::Slider& slider, const juce::String& tooltip)
    {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 64, 18);
        slider.setTooltip(tooltip);
    }

    void configureToggle(juce::ToggleButton& button, const juce::String& text)
    {
        button.setButtonText(text);
        button.setTooltip(text);
    }
}

//==============================================================================
CrushJointsAudioProcessorEditor::CrushJointsAudioProcessorEditor(CrushJointsAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p), apvts(p.getAPVTS())
{
    // ---- A/B controls --------------------------------------------------------
    abLabel.setText("A/B", juce::dontSendNotification);
    abLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(abLabel);

    for (auto* button : { &abButtonA, &abButtonB })
    {
        button->setClickingTogglesState(false);
        button->setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff3b6ea5));
        addAndMakeVisible(button);
    }

    abButtonA.setButtonText("A");
    abButtonA.setTooltip("Switch to A/B slot A");
    abButtonA.onClick = [this]
    {
        processorRef.setActiveABSlot(0);
        updateFromProcessor();
    };

    abButtonB.setButtonText("B");
    abButtonB.setTooltip("Switch to A/B slot B");
    abButtonB.onClick = [this]
    {
        processorRef.setActiveABSlot(1);
        updateFromProcessor();
    };

    copyABButton.setTooltip("Copy the active slot into the other slot");
    copyABButton.onClick = [this]
    {
        processorRef.copyActiveABToInactive();
        updateFromProcessor();
    };
    addAndMakeVisible(copyABButton);

    // ---- Preset bar ----------------------------------------------------------
    presetLabel.setText("Patch", juce::dontSendNotification);
    presetLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(presetLabel);

    presetBox.onChange = [this]
    {
        if (updatingPresetBox)
            return;

        const auto index = presetBox.getSelectedItemIndex();

        if (index >= 0)
            processorRef.setCurrentProgram(index);
    };
    addAndMakeVisible(presetBox);

    savePresetButton.setButtonText("Save...");
    savePresetButton.setTooltip("Save the current settings as a user preset");
    savePresetButton.onClick = [this] { showSavePresetDialog(); };
    addAndMakeVisible(savePresetButton);

    deletePresetButton.setButtonText("Delete");
    deletePresetButton.setTooltip("Delete the selected user preset");
    deletePresetButton.onClick = [this] { showDeletePresetDialog(); };
    addAndMakeVisible(deletePresetButton);

    randomizeButton.setButtonText("Randomize");
    randomizeButton.onClick = [this] { randomizeParameters(); };
    addAndMakeVisible(randomizeButton);

    // ---- Distortion ----------------------------------------------------------
    distortionGroup.setText("Distortion");
    addAndMakeVisible(distortionGroup);

    configureKnob(tubeSlider, "Tube drive");
    configureKnob(mechSlider, "Mech drive");
    addAndMakeVisible(tubeSlider);
    addAndMakeVisible(mechSlider);

    configureToggle(tubeOnButton, "On");
    addAndMakeVisible(tubeOnButton);

    tubeAttachment   = std::make_unique<SliderAttachment>(apvts, "tube", tubeSlider);
    mechAttachment   = std::make_unique<SliderAttachment>(apvts, "mech", mechSlider);
    tubeOnAttachment = std::make_unique<ButtonAttachment>(apvts, "tube_on", tubeOnButton);

    // ---- Filter --------------------------------------------------------------
    filterGroup.setText("Filter");
    addAndMakeVisible(filterGroup);

    configureKnob(cutoffSlider, "Cutoff frequency");
    configureKnob(resSlider, "Resonance");
    cutoffSlider.setSkewFactorFromMidPoint(1000.0);
    addAndMakeVisible(cutoffSlider);
    addAndMakeVisible(resSlider);

    configureToggle(lpfOnButton, "On");
    addAndMakeVisible(lpfOnButton);

    cutoffAttachment = std::make_unique<SliderAttachment>(apvts, "cutoff", cutoffSlider);
    resAttachment    = std::make_unique<SliderAttachment>(apvts, "res", resSlider);
    lpfOnAttachment  = std::make_unique<ButtonAttachment>(apvts, "lpf_on", lpfOnButton);

    // ---- Compressor ----------------------------------------------------------
    compressorGroup.setText("Compressor");
    addAndMakeVisible(compressorGroup);

    configureKnob(amountSlider, "Compression amount");
    addAndMakeVisible(amountSlider);

    configureToggle(phatButton, "Phat");
    configureToggle(compOnButton, "On");
    addAndMakeVisible(phatButton);
    addAndMakeVisible(compOnButton);

    amountAttachment = std::make_unique<SliderAttachment>(apvts, "amount", amountSlider);
    phatAttachment   = std::make_unique<ButtonAttachment>(apvts, "phat", phatButton);
    compOnAttachment = std::make_unique<ButtonAttachment>(apvts, "comp_on", compOnButton);

    // ---- Master --------------------------------------------------------------
    masterGroup.setText("Master");
    addAndMakeVisible(masterGroup);

    configureKnob(volumeSlider, "Output volume");
    configureKnob(mixSlider, "Dry/Wet mix");
    addAndMakeVisible(volumeSlider);
    addAndMakeVisible(mixSlider);

    configureToggle(masterOnButton, "On");
    addAndMakeVisible(masterOnButton);

    volumeAttachment   = std::make_unique<SliderAttachment>(apvts, "volume", volumeSlider);
    mixAttachment      = std::make_unique<SliderAttachment>(apvts, "mix", mixSlider);
    masterOnAttachment = std::make_unique<ButtonAttachment>(apvts, "master_on", masterOnButton);

    processorRef.addChangeListener(this);
    updateFromProcessor();

    setSize(860, 440);
}

CrushJointsAudioProcessorEditor::~CrushJointsAudioProcessorEditor()
{
    processorRef.removeChangeListener(this);
}

//==============================================================================
void CrushJointsAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1e1e1e));

    g.setColour(juce::Colours::white);
    g.setFont(18.0f);
    auto titleArea = getLocalBounds().reduced(12).removeFromTop(32).removeFromLeft(120);
    g.drawText("CrushJoints", titleArea, juce::Justification::centredLeft, true);
}

void CrushJointsAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(12);

    // Header: title (painted), A/B controls, preset controls and randomize.
    auto header = area.removeFromTop(32);
    header.removeFromLeft(120);

    abLabel.setBounds(header.removeFromLeft(30));
    abButtonA.setBounds(header.removeFromLeft(28).reduced(1, 0));
    abButtonB.setBounds(header.removeFromLeft(28).reduced(1, 0));
    header.removeFromLeft(6);
    copyABButton.setBounds(header.removeFromLeft(70).reduced(1, 0));
    header.removeFromLeft(10);

    presetLabel.setBounds(header.removeFromLeft(44));
    presetBox.setBounds(header.removeFromLeft(180).reduced(1, 0));
    savePresetButton.setBounds(header.removeFromLeft(64).reduced(1, 0));
    deletePresetButton.setBounds(header.removeFromLeft(64).reduced(1, 0));
    randomizeButton.setBounds(header.removeFromRight(90).reduced(1, 0));

    area.removeFromTop(8);

    // Four module columns.
    auto columns = area;
    const int columnWidth = columns.getWidth() / 4;

    auto layoutModule = [](juce::Rectangle<int> bounds,
                           juce::GroupComponent& group,
                           juce::Slider& first,
                           juce::Slider* second,
                           juce::ToggleButton& toggle1,
                           juce::ToggleButton* toggle2)
    {
        group.setBounds(bounds);
        auto content = bounds.reduced(10).withTrimmedTop(18);

        auto toggleArea = content.removeFromBottom(26);
        toggle1.setBounds(toggleArea.removeFromLeft(toggleArea.getWidth() / 2)
                              .reduced(2, 0));
        if (toggle2 != nullptr)
            toggle2->setBounds(toggleArea.reduced(2, 0));

        if (second != nullptr)
        {
            first.setBounds(content.removeFromLeft(content.getWidth() / 2).reduced(2));
            second->setBounds(content.reduced(2));
        }
        else
        {
            first.setBounds(content.reduced(8));
        }
    };

    layoutModule(columns.removeFromLeft(columnWidth), distortionGroup, tubeSlider, &mechSlider,
                 tubeOnButton, nullptr);
    layoutModule(columns.removeFromLeft(columnWidth), filterGroup, cutoffSlider, &resSlider,
                 lpfOnButton, nullptr);
    layoutModule(columns.removeFromLeft(columnWidth), compressorGroup, amountSlider, nullptr,
                 compOnButton, &phatButton);
    layoutModule(columns, masterGroup, volumeSlider, &mixSlider, masterOnButton, nullptr);
}

//==============================================================================
void CrushJointsAudioProcessorEditor::changeListenerCallback(juce::ChangeBroadcaster*)
{
    updateFromProcessor();
}

void CrushJointsAudioProcessorEditor::updateFromProcessor()
{
    updatingPresetBox = true;
    presetBox.clear(juce::dontSendNotification);

    const int numPresets = processorRef.getNumPrograms();

    for (int i = 0; i < numPresets; ++i)
    {
        auto name = processorRef.getProgramName(i);

        if (processorRef.isUserPreset(i))
            name = "User: " + name;

        presetBox.addItem(name, i + 1);
    }

    const int currentProgram = processorRef.getCurrentProgram();
    presetBox.setSelectedId(juce::jlimit(1, juce::jmax(1, numPresets), currentProgram + 1),
                            juce::dontSendNotification);
    updatingPresetBox = false;

    const int activeSlot = processorRef.getActiveABSlot();
    abButtonA.setToggleState(activeSlot == 0, juce::dontSendNotification);
    abButtonB.setToggleState(activeSlot == 1, juce::dontSendNotification);
    copyABButton.setButtonText(activeSlot == 0 ? "A -> B" : "B -> A");

    deletePresetButton.setEnabled(processorRef.isUserPreset(currentProgram));
}

void CrushJointsAudioProcessorEditor::showSavePresetDialog()
{
    const int currentProgram = processorRef.getCurrentProgram();
    auto defaultName = processorRef.getProgramName(currentProgram);

    if (! processorRef.isUserPreset(currentProgram))
        defaultName += " Copy";

    auto* window = new juce::AlertWindow("Save Preset",
                                         "Save the current settings as a user preset:",
                                         juce::MessageBoxIconType::NoIcon);
    window->addTextEditor("name", defaultName, "Name:");
    window->addButton("Save", 1, juce::KeyPress(juce::KeyPress::returnKey));
    window->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

    window->enterModalState(true,
                            juce::ModalCallbackFunction::create([this, window](int result)
                            {
                                if (result != 1)
                                    return;

                                const auto name = window->getTextEditorContents("name");

                                if (processorRef.saveUserPreset(name) < 0)
                                {
                                    juce::AlertWindow::showMessageBoxAsync(
                                        juce::MessageBoxIconType::WarningIcon,
                                        "Save Preset",
                                        "Could not save the preset. Check that the name is not "
                                        "empty and that the preset folder is writable.");
                                }
                            }),
                            true);
}

void CrushJointsAudioProcessorEditor::showDeletePresetDialog()
{
    const int currentProgram = processorRef.getCurrentProgram();

    if (! processorRef.isUserPreset(currentProgram))
        return;

    const auto name = processorRef.getProgramName(currentProgram);

    juce::AlertWindow::showOkCancelBox(
        juce::MessageBoxIconType::QuestionIcon,
        "Delete Preset",
        "Delete user preset \"" + name + "\"?",
        "Delete",
        "Cancel",
        this,
        juce::ModalCallbackFunction::create([this, currentProgram](int result)
        {
            if (result != 0)
                processorRef.deleteUserPreset(currentProgram);
        }));
}

void CrushJointsAudioProcessorEditor::randomizeParameters()
{
    juce::Random random;

    for (auto* parameter : processorRef.getParameters())
        parameter->setValueNotifyingHost(random.nextFloat());
}
