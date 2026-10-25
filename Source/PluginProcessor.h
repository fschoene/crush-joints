#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "DSP/Distortion.h"
#include "DSP/LadderFilter.h"
#include "DSP/Compressor.h"
#include "DSP/Master.h"
#include "Common/Presets.h"

//==============================================================================
class CrushJointsAudioProcessor : public juce::AudioProcessor
{
public:
    //==============================================================================
    CrushJointsAudioProcessor();
    ~CrushJointsAudioProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    //==============================================================================
    // Parameters
    juce::AudioParameterFloat* tubeParam;
    juce::AudioParameterFloat* mechParam;
    juce::AudioParameterBool* tubeOnParam;
    
    juce::AudioParameterFloat* cutoffParam;
    juce::AudioParameterFloat* resParam;
    juce::AudioParameterBool* lpfOnParam;
    
    juce::AudioParameterFloat* amountParam;
    juce::AudioParameterBool* phatParam;
    juce::AudioParameterBool* compOnParam;
    
    juce::AudioParameterFloat* volumeParam;
    juce::AudioParameterFloat* mixParam;
    juce::AudioParameterBool* masterOnParam;

    //==============================================================================
    juce::AudioProcessorValueTreeState& getAPVTS() noexcept { return apvts; }

private:
    //==============================================================================
    juce::AudioProcessorValueTreeState apvts;

    // DSP processors
    CrushJoints::Distortion distortion;
    CrushJoints::LadderFilter filter;
    CrushJoints::Compressor compressor;
    CrushJoints::Master master;

    // Pre-allocated buffer holding the pre-effects signal for dry/wet mixing.
    juce::AudioBuffer<float> dryBuffer;

    // Preset manager
    CrushJoints::Presets presets;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CrushJointsAudioProcessor)
};