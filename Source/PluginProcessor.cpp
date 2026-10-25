#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
CrushJointsAudioProcessor::CrushJointsAudioProcessor()
    : AudioProcessor(BusesLayout().withOutputs({ AudioChannelSet::stereo() })),
      apvts(*this, nullptr, "parameters", 
           {
                // Distortion parameters
                std::make_unique<juce::AudioParameterFloat>("tube", "Tube", 0.0f, 1.0f, 0.0f),
                std::make_unique<juce::AudioParameterFloat>("mech", "Mech", 0.0f, 1.0f, 0.0f),
                std::make_unique<juce::AudioParameterBool>("tube_on", "Tube On", true),
                
                // Filter parameters
                std::make_unique<juce::AudioParameterFloat>("cutoff", "Cutoff", 20.0f, 20000.0f, 20000.0f),
                std::make_unique<juce::AudioParameterFloat>("res", "Resonance", 0.0f, 1.0f, 0.0f),
                std::make_unique<juce::AudioParameterBool>("lpf_on", "Filter On", false),
                
                // Compressor parameters
                std::make_unique<juce::AudioParameterFloat>("amount", "Amount", 0.0f, 1.0f, 0.0f),
                std::make_unique<juce::AudioParameterBool>("phat", "Phat Mode", false),
                std::make_unique<juce::AudioParameterBool>("comp_on", "Compressor On", true),
                
                // Master parameters
                std::make_unique<juce::AudioParameterFloat>("volume", "Volume", -24.0f, 12.0f, 0.0f),
                std::make_unique<juce::AudioParameterFloat>("mix", "Mix", 0.0f, 1.0f, 1.0f),
                std::make_unique<juce::AudioParameterBool>("master_on", "Master On", true)
           })
{
    // Get parameter references
    tubeParam = (juce::AudioParameterFloat*)apvts.getParameter("tube");
    mechParam = (juce::AudioParameterFloat*)apvts.getParameter("mech");
    tubeOnParam = (juce::AudioParameterBool*)apvts.getParameter("tube_on");
    
    cutoffParam = (juce::AudioParameterFloat*)apvts.getParameter("cutoff");
    resParam = (juce::AudioParameterFloat*)apvts.getParameter("res");
    lpfOnParam = (juce::AudioParameterBool*)apvts.getParameter("lpf_on");
    
    amountParam = (juce::AudioParameterFloat*)apvts.getParameter("amount");
    phatParam = (juce::AudioParameterBool*)apvts.getParameter("phat");
    compOnParam = (juce::AudioParameterBool*)apvts.getParameter("comp_on");
    
    volumeParam = (juce::AudioParameterFloat*)apvts.getParameter("volume");
    mixParam = (juce::AudioParameterFloat*)apvts.getParameter("mix");
    masterOnParam = (juce::AudioParameterBool*)apvts.getParameter("master_on");
    
    // Initialize presets
    presets.initPresets();
}

CrushJointsAudioProcessor::~CrushJointsAudioProcessor()
{
}

//==============================================================================
const juce::String CrushJointsAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool CrushJointsAudioProcessor::acceptsMidi() const
{
    return false;
}

bool CrushJointsAudioProcessor::producesMidi() const
{
    return false;
}

bool CrushJointsAudioProcessor::isMidiEffect() const
{
    return false;
}

double CrushJointsAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

bool CrushJointsAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
        return false;

    if (layouts.getMainInputChannelSet() != AudioChannelSet::stereo())
        return false;

    return true;
}

//==============================================================================
void CrushJointsAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Setup all DSP modules
    distortion.prepareToPlay(sampleRate);
    filter.prepareToPlay(sampleRate);
    compressor.prepareToPlay(sampleRate);
    oversampler.prepareToPlay(sampleRate, samplesPerBlock);
}

void CrushJointsAudioProcessor::releaseResources()
{
}

//==============================================================================
void CrushJointsAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer)
{
    juce::ScopedNoDenormals noDenormals;
    
    // For now just process through the modules in sequence to show the structure
    // This will be fully implemented in the next phases
    
    // Apply distortion module
    // distortion.processBlock(buffer);
    
    // Apply filter module  
    // filter.processBlock(buffer);
    
    // Apply compressor module
    // compressor.processBlock(buffer);
    
    // Apply master processing (volume, mix, etc.)
    // The oversampler and final processing will be added in later phases
}

//==============================================================================
bool CrushJointsAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* CrushJointsAudioProcessor::createEditor()
{
    return new CrushJointsAudioProcessorEditor(*this);
}

//==============================================================================
int CrushJointsAudioProcessor::getNumPrograms()
{
    return presets.getNumPresets();
}

int CrushJointsAudioProcessor::getCurrentProgram()
{
    return 0;
}

void CrushJointsAudioProcessor::setCurrentProgram(int index)
{
    if (index >= 0 && index < getNumPrograms())
    {
        // Load preset values
        auto preset = presets.getPreset(index);
        tubeParam->setValueNotifyingHost(preset.tube);
        mechParam->setValueNotifyingHost(preset.mech);
        tubeOnParam->setValueNotifyingHost(preset.tubeOn);
        cutoffParam->setValueNotifyingHost(preset.cutoff);
        resParam->setValueNotifyingHost(preset.res);
        lpfOnParam->setValueNotifyingHost(preset.lpfOn);
        amountParam->setValueNotifyingHost(preset.amount);
        phatParam->setValueNotifyingHost(preset.phat);
        compOnParam->setValueNotifyingHost(preset.compOn);
        volumeParam->setValueNotifyingHost(preset.volume);
        mixParam->setValueNotifyingHost(preset.mix);
        masterOnParam->setValueNotifyingHost(preset.masterOn);
    }
}

const juce::String CrushJointsAudioProcessor::getProgramName(int index)
{
    if (index >= 0 && index < getNumPrograms())
    {
        return presets.getPreset(index).name;
    }
    return "Default";
}

void CrushJointsAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
    // Not implemented for now
}

//==============================================================================
void CrushJointsAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // Save parameter values
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void CrushJointsAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    // Restore parameter values
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr)
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}