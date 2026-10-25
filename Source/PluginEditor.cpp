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
    // ---- Preset bar ----------------------------------------------------------
    presetLabel.setText("Patch", juce::dontSendNotification);
    presetLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(presetLabel);

    for (int i = 0; i < processorRef.getNumPrograms(); ++i)
        presetBox.addItem(processorRef.getProgramName(i), i + 1);
    presetBox.setSelectedId(1, juce::dontSendNotification);
    presetBox.onChange = [this]
    {
        const auto index = presetBox.getSelectedItemIndex();
        if (index >= 0)
            processorRef.setCurrentProgram(index);
    };
    addAndMakeVisible(presetBox);

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

    setSize(720, 400);
}

CrushJointsAudioProcessorEditor::~CrushJointsAudioProcessorEditor()
{
}

//==============================================================================
void CrushJointsAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1e1e1e));

    g.setColour(juce::Colours::white);
    g.setFont(20.0f);
    g.drawText("CrushJoints", getLocalBounds().removeFromTop(36).reduced(16, 0),
               juce::Justification::centredLeft, true);
}

void CrushJointsAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(12);

    // Header row: title area on the left, preset controls on the right.
    auto header = area.removeFromTop(36);
    header.removeFromLeft(200);

    presetLabel.setBounds(header.removeFromLeft(64));
    presetBox.setBounds(header.removeFromLeft(200).reduced(2, 4));
    randomizeButton.setBounds(header.removeFromLeft(100).reduced(2, 4));

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

    auto distortionArea = columns.removeFromLeft(columnWidth);
    layoutModule(distortionArea, distortionGroup, tubeSlider, &mechSlider,
                 tubeOnButton, nullptr);

    auto filterArea = columns.removeFromLeft(columnWidth);
    layoutModule(filterArea, filterGroup, cutoffSlider, &resSlider,
                 lpfOnButton, nullptr);

    auto compressorArea = columns.removeFromLeft(columnWidth);
    layoutModule(compressorArea, compressorGroup, amountSlider, nullptr,
                 compOnButton, &phatButton);

    layoutModule(columns, masterGroup, volumeSlider, &mixSlider,
                 masterOnButton, nullptr);

    // The tube/mech "On" switch controls the whole distortion section.
}

void CrushJointsAudioProcessorEditor::randomizeParameters()
{
    juce::Random random;
    for (auto* parameter : processorRef.getParameters())
        parameter->setValueNotifyingHost(random.nextFloat());
}
