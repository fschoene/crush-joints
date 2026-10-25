#include "PluginEditor.h"

namespace
{
    constexpr int designWidth = 860;
    constexpr int designHeight = 440;

    void configureKnob(juce::Slider& slider, const juce::String& tooltip)
    {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 64, 18);
        slider.setTooltip(tooltip + " (double-click to reset)");
    }

    void configureToggle(juce::ToggleButton& button, const juce::String& text)
    {
        button.setButtonText(text);
        button.setTooltip(text);
    }

    void configureLabel(juce::Label& label, const juce::String& text)
    {
        label.setText(text, juce::dontSendNotification);
        label.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
        label.setJustificationType(juce::Justification::centred);
        label.setColour(juce::Label::textColourId, CrushJoints::CrushJointsLookAndFeel::text);
        label.setInterceptsMouseClicks(false, false);
    }
}

//==============================================================================
void CrushJointsAudioProcessorEditor::ContentComponent::paint(juce::Graphics& g)
{
    juce::ColourGradient gradient(CrushJoints::CrushJointsLookAndFeel::background.brighter(0.04f),
                                  0.0f, 0.0f,
                                  CrushJoints::CrushJointsLookAndFeel::background.darker(0.25f),
                                  0.0f, static_cast<float>(getHeight()),
                                  false);
    g.setGradientFill(gradient);
    g.fillAll();
}

//==============================================================================
CrushJointsAudioProcessorEditor::CrushJointsAudioProcessorEditor(CrushJointsAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p), apvts(p.getAPVTS())
{
    setLookAndFeel(&lookAndFeel);

    contentComponent.setOpaque(true);
    addAndMakeVisible(contentComponent);

    // ---- Title ---------------------------------------------------------------
    titleLabel.setText("CRUSHJOINTS", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(juce::FontOptions(20.0f, juce::Font::bold)));
    titleLabel.setJustificationType(juce::Justification::centredLeft);
    titleLabel.setColour(juce::Label::textColourId, CrushJoints::CrushJointsLookAndFeel::accent);
    addToContent(titleLabel);

    // ---- A/B controls --------------------------------------------------------
    abLabel.setText("A/B", juce::dontSendNotification);
    abLabel.setJustificationType(juce::Justification::centredRight);
    addToContent(abLabel);

    for (auto* button : { &abButtonA, &abButtonB })
    {
        button->setClickingTogglesState(false);
        addToContent(*button);
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
    addToContent(copyABButton);

    // ---- Preset bar ----------------------------------------------------------
    presetLabel.setText("Patch", juce::dontSendNotification);
    presetLabel.setJustificationType(juce::Justification::centredRight);
    addToContent(presetLabel);

    presetBox.onChange = [this]
    {
        if (updatingPresetBox)
            return;

        const auto index = presetBox.getSelectedItemIndex();

        if (index >= 0)
            processorRef.setCurrentProgram(index);
    };
    addToContent(presetBox);

    savePresetButton.setButtonText("Save...");
    savePresetButton.setTooltip("Save the current settings as a user preset");
    savePresetButton.onClick = [this] { showSavePresetDialog(); };
    addToContent(savePresetButton);

    deletePresetButton.setButtonText("Delete");
    deletePresetButton.setTooltip("Delete the selected user preset");
    deletePresetButton.onClick = [this] { showDeletePresetDialog(); };
    addToContent(deletePresetButton);

    randomizeButton.setButtonText("Randomize");
    randomizeButton.setTooltip("Randomise all unlocked continuous parameters");
    randomizeButton.onClick = [this] { processorRef.randomizeUnlockedParameters(); };
    addToContent(randomizeButton);

    // ---- Value readout -------------------------------------------------------
    valueReadout.setFont(juce::Font(juce::FontOptions(13.0f, juce::Font::bold)));
    valueReadout.setJustificationType(juce::Justification::centred);
    valueReadout.setColour(juce::Label::textColourId, CrushJoints::CrushJointsLookAndFeel::accent);
    valueReadout.setColour(juce::Label::backgroundColourId, CrushJoints::CrushJointsLookAndFeel::panel);
    valueReadout.setColour(juce::Label::outlineColourId, CrushJoints::CrushJointsLookAndFeel::outline);
    valueReadout.setText("Preset: Default", juce::dontSendNotification);
    addToContent(valueReadout);

    // ---- Distortion ----------------------------------------------------------
    distortionGroup.setText("DISTORTION");
    addToContent(distortionGroup);

    configureKnob(tubeSlider, "Tube drive");
    configureKnob(mechSlider, "Mech drive");
    configureLabel(tubeLabel, "TUBE");
    configureLabel(mechLabel, "MECH");
    addToContent(tubeSlider);
    addToContent(mechSlider);
    addToContent(tubeLabel);
    addToContent(mechLabel);

    configureToggle(tubeOnButton, "On");
    addToContent(tubeOnButton);

    tubeAttachment   = std::make_unique<SliderAttachment>(apvts, "tube", tubeSlider);
    mechAttachment   = std::make_unique<SliderAttachment>(apvts, "mech", mechSlider);
    tubeOnAttachment = std::make_unique<ButtonAttachment>(apvts, "tube_on", tubeOnButton);

    // ---- Filter --------------------------------------------------------------
    filterGroup.setText("FILTER");
    addToContent(filterGroup);

    configureKnob(cutoffSlider, "Cutoff frequency");
    configureKnob(resSlider, "Resonance");
    configureLabel(cutoffLabel, "CUTOFF");
    configureLabel(resLabel, "RES");
    cutoffSlider.setSkewFactorFromMidPoint(1000.0);
    addToContent(cutoffSlider);
    addToContent(resSlider);
    addToContent(cutoffLabel);
    addToContent(resLabel);

    configureToggle(lpfOnButton, "On");
    addToContent(lpfOnButton);

    cutoffAttachment = std::make_unique<SliderAttachment>(apvts, "cutoff", cutoffSlider);
    resAttachment    = std::make_unique<SliderAttachment>(apvts, "res", resSlider);
    lpfOnAttachment  = std::make_unique<ButtonAttachment>(apvts, "lpf_on", lpfOnButton);

    // ---- Compressor ----------------------------------------------------------
    compressorGroup.setText("COMPRESSOR");
    addToContent(compressorGroup);

    configureKnob(amountSlider, "Compression amount");
    configureLabel(amountLabel, "AMOUNT");
    addToContent(amountSlider);
    addToContent(amountLabel);

    configureToggle(phatButton, "Phat");
    configureToggle(compOnButton, "On");
    addToContent(phatButton);
    addToContent(compOnButton);

    amountAttachment = std::make_unique<SliderAttachment>(apvts, "amount", amountSlider);
    phatAttachment   = std::make_unique<ButtonAttachment>(apvts, "phat", phatButton);
    compOnAttachment = std::make_unique<ButtonAttachment>(apvts, "comp_on", compOnButton);

    // ---- Master --------------------------------------------------------------
    masterGroup.setText("MASTER");
    addToContent(masterGroup);

    configureKnob(volumeSlider, "Output volume");
    configureKnob(mixSlider, "Dry/Wet mix");
    configureLabel(volumeLabel, "VOLUME");
    configureLabel(mixLabel, "MIX");
    addToContent(volumeSlider);
    addToContent(mixSlider);
    addToContent(volumeLabel);
    addToContent(mixLabel);

    configureToggle(masterOnButton, "On");
    addToContent(masterOnButton);

    volumeAttachment   = std::make_unique<SliderAttachment>(apvts, "volume", volumeSlider);
    mixAttachment      = std::make_unique<SliderAttachment>(apvts, "mix", mixSlider);
    masterOnAttachment = std::make_unique<ButtonAttachment>(apvts, "master_on", masterOnButton);

    // ---- Double-click defaults ----------------------------------------------
    auto setResetValue = [this](juce::Slider& slider, const char* parameterId)
    {
        auto* parameter = apvts.getParameter(parameterId);
        slider.setDoubleClickReturnValue(true,
                                         parameter->convertFrom0to1(parameter->getDefaultValue()));
    };

    volumeSlider.textFromValueFunction = [](double value)
    {
        if (std::abs(value) < 0.005)
            value = 0.0;

        return juce::String(value, 2);
    };
    volumeSlider.updateText();

    auto hookReadout = [this](juce::Slider& slider, const juce::String& name)
    {
        slider.onValueChange = [this, &slider, name]
        {
            if (slider.isMouseButtonDown() || slider.isMouseOverOrDragging())
                valueReadout.setText(name + ": " + slider.getTextFromValue(slider.getValue()),
                                     juce::dontSendNotification);
        };
    };

    hookReadout(tubeSlider, "TUBE");
    hookReadout(mechSlider, "MECH");
    hookReadout(cutoffSlider, "CUTOFF");
    hookReadout(resSlider, "RES");
    hookReadout(amountSlider, "AMOUNT");
    hookReadout(volumeSlider, "VOLUME");
    hookReadout(mixSlider, "MIX");

    setResetValue(tubeSlider, "tube");
    setResetValue(mechSlider, "mech");
    setResetValue(cutoffSlider, "cutoff");
    setResetValue(resSlider, "res");
    setResetValue(amountSlider, "amount");
    setResetValue(volumeSlider, "volume");
    setResetValue(mixSlider, "mix");

    // ---- Parameter locks -----------------------------------------------------
    const std::pair<LockButton*, int> locks[] = {
        { &tubeLock,   CrushJointsAudioProcessor::lockableTube },
        { &mechLock,   CrushJointsAudioProcessor::lockableMech },
        { &cutoffLock, CrushJointsAudioProcessor::lockableCutoff },
        { &resLock,    CrushJointsAudioProcessor::lockableRes },
        { &amountLock, CrushJointsAudioProcessor::lockableAmount },
        { &volumeLock, CrushJointsAudioProcessor::lockableVolume },
        { &mixLock,    CrushJointsAudioProcessor::lockableMix },
    };

    for (const auto& entry : locks)
    {
        auto* button = entry.first;
        const int index = entry.second;

        button->setToggleState(processorRef.isParameterLocked(index), juce::dontSendNotification);
        button->onClick = [this, button, index]
        {
            processorRef.setParameterLocked(index, button->getToggleState());
        };
        addToContent(*button);
    }

    processorRef.addChangeListener(this);
    updateFromProcessor();

    // The window can be scaled from 80% to 150%; the content is laid out once
    // in design coordinates and transformed as a whole.
    setResizable(true, true);
    setResizeLimits(juce::roundToInt(designWidth * 0.8f),
                    juce::roundToInt(designHeight * 0.8f),
                    juce::roundToInt(designWidth * 1.5f),
                    juce::roundToInt(designHeight * 1.5f));

    if (auto* constrainer = getConstrainer())
        constrainer->setFixedAspectRatio(static_cast<double>(designWidth) / designHeight);

    setSize(designWidth, designHeight);
}

CrushJointsAudioProcessorEditor::~CrushJointsAudioProcessorEditor()
{
    processorRef.removeChangeListener(this);
    setLookAndFeel(nullptr);
}

//==============================================================================
void CrushJointsAudioProcessorEditor::paint(juce::Graphics& g)
{
    // Letterbox colour around the fixed-aspect content.
    g.fillAll(juce::Colour(0xff050b14));
}

void CrushJointsAudioProcessorEditor::resized()
{
    const auto scale = juce::jmin(static_cast<float>(getWidth()) / designWidth,
                                  static_cast<float>(getHeight()) / designHeight);
    const auto visualWidth = designWidth * scale;
    const auto visualHeight = designHeight * scale;
    const auto offsetX = (static_cast<float>(getWidth()) - visualWidth) * 0.5f;
    const auto offsetY = (static_cast<float>(getHeight()) - visualHeight) * 0.5f;

    contentComponent.setBounds(juce::roundToInt(offsetX), juce::roundToInt(offsetY),
                               designWidth, designHeight);
    contentComponent.setTransform(juce::AffineTransform::scale(scale));

    layoutContent();
}

void CrushJointsAudioProcessorEditor::layoutContent()
{
    auto area = juce::Rectangle<int>(0, 0, designWidth, designHeight).reduced(12);

    auto header = area.removeFromTop(32);
    titleLabel.setBounds(header.removeFromLeft(120));

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

    area.removeFromTop(10);

    auto footer = area.removeFromBottom(24);
    valueReadout.setBounds(footer.reduced(1));
    area.removeFromBottom(8);

    auto layoutKnob = [](juce::Rectangle<int> knobArea,
                         juce::Label& label,
                         LockButton& lock,
                         juce::Slider& slider)
    {
        auto labelRow = knobArea.removeFromTop(18);
        lock.setBounds(labelRow.removeFromRight(20).reduced(1));
        label.setBounds(labelRow);

        const auto knobWidth = juce::jmin(knobArea.getWidth() - 8, 100);
        const auto knobHeight = juce::jmin(knobArea.getHeight() - 8, 160);
        slider.setBounds(knobArea.withSizeKeepingCentre(knobWidth, knobHeight));
    };

    auto layoutModule = [&](juce::Rectangle<int> bounds,
                            juce::GroupComponent& group,
                            juce::Label& label1,
                            LockButton& lock1,
                            juce::Slider& slider1,
                            juce::Label* label2,
                            LockButton* lock2,
                            juce::Slider* slider2,
                            juce::ToggleButton& toggle1,
                            juce::ToggleButton* toggle2)
    {
        group.setBounds(bounds);
        auto content = bounds.reduced(10).withTrimmedTop(20);

        auto toggleArea = content.removeFromBottom(28);
        if (toggle2 != nullptr)
        {
            toggle1.setBounds(toggleArea.removeFromLeft(toggleArea.getWidth() / 2).reduced(2, 0));
            toggle2->setBounds(toggleArea.reduced(2, 0));
        }
        else
        {
            toggle1.setBounds(toggleArea.withSizeKeepingCentre(
                juce::jmin(toggleArea.getWidth(), 90), 24));
        }

        if (slider2 != nullptr)
        {
            auto left = content.removeFromLeft(content.getWidth() / 2);
            layoutKnob(left.reduced(2), label1, lock1, slider1);
            layoutKnob(content.reduced(2), *label2, *lock2, *slider2);
        }
        else
        {
            layoutKnob(content.reduced(2), label1, lock1, slider1);
        }
    };

    auto columns = area;
    const int columnWidth = columns.getWidth() / 4;

    layoutModule(columns.removeFromLeft(columnWidth), distortionGroup,
                 tubeLabel, tubeLock, tubeSlider,
                 &mechLabel, &mechLock, &mechSlider,
                 tubeOnButton, nullptr);

    layoutModule(columns.removeFromLeft(columnWidth), filterGroup,
                 cutoffLabel, cutoffLock, cutoffSlider,
                 &resLabel, &resLock, &resSlider,
                 lpfOnButton, nullptr);

    layoutModule(columns.removeFromLeft(columnWidth), compressorGroup,
                 amountLabel, amountLock, amountSlider,
                 nullptr, nullptr, nullptr,
                 compOnButton, &phatButton);

    layoutModule(columns, masterGroup,
                 volumeLabel, volumeLock, volumeSlider,
                 &mixLabel, &mixLock, &mixSlider,
                 masterOnButton, nullptr);
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
    valueReadout.setText("Preset: " + processorRef.getProgramName(currentProgram),
                         juce::dontSendNotification);

    const std::pair<LockButton*, int> locks[] = {
        { &tubeLock,   CrushJointsAudioProcessor::lockableTube },
        { &mechLock,   CrushJointsAudioProcessor::lockableMech },
        { &cutoffLock, CrushJointsAudioProcessor::lockableCutoff },
        { &resLock,    CrushJointsAudioProcessor::lockableRes },
        { &amountLock, CrushJointsAudioProcessor::lockableAmount },
        { &volumeLock, CrushJointsAudioProcessor::lockableVolume },
        { &mixLock,    CrushJointsAudioProcessor::lockableMix },
    };

    for (const auto& entry : locks)
        entry.first->setToggleState(processorRef.isParameterLocked(entry.second),
                                    juce::dontSendNotification);
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
