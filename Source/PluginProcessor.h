#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <atomic>
#include "DSP/Distortion.h"
#include "DSP/LadderFilter.h"
#include "DSP/Compressor.h"
#include "DSP/Master.h"
#include "Common/Presets.h"

//==============================================================================
class CrushJointsAudioProcessor : public juce::AudioProcessor,
                                   public juce::ChangeBroadcaster
{
public:
    //==============================================================================
    explicit CrushJointsAudioProcessor(juce::File userPresetFile = CrushJoints::Presets::getDefaultUserPresetFile());
    ~CrushJointsAudioProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    using AudioProcessor::processBlock;
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
    // Presets
    /** Saves the current parameter values as a user preset. Returns the new
        program index, or -1 if the name is invalid or the file cannot be
        written. Overwrites a user preset with the same name. */
    int saveUserPreset(const juce::String& name);

    /** Deletes a user preset. Factory presets cannot be deleted. */
    bool deleteUserPreset(int programIndex);

    bool isUserPreset(int programIndex) const;
    int getNumFactoryPresets() const;
    CrushJoints::PresetCategory getPresetCategory(int programIndex) const;

    //==============================================================================
    // A/B comparison
    int getActiveABSlot() const;
    void setActiveABSlot(int slot);
    void copyActiveABToInactive();

    //==============================================================================
    // Randomize / parameter locks
    enum LockableParameter
    {
        lockableTube = 0,
        lockableMech,
        lockableCutoff,
        lockableRes,
        lockableAmount,
        lockableVolume,
        lockableMix,
        numLockableParameters
    };

    /** Randomises the continuous parameters that are not locked. Module
        on/off and Phat switches are intentionally left unchanged. */
    void randomizeUnlockedParameters();

    bool isParameterLocked(int index) const;
    void setParameterLocked(int index, bool locked);

    //==============================================================================
    // Parameters
    juce::AudioParameterFloat* tubeParam;
    juce::AudioParameterFloat* mechParam;
    juce::AudioParameterChoice* tubeOnParam;
    
    juce::AudioParameterFloat* cutoffParam;
    juce::AudioParameterFloat* resParam;
    juce::AudioParameterChoice* lpfOnParam;
    
    juce::AudioParameterFloat* amountParam;
    juce::AudioParameterChoice* phatParam;
    juce::AudioParameterChoice* compOnParam;
    
    juce::AudioParameterFloat* volumeParam;
    juce::AudioParameterFloat* mixParam;
    juce::AudioParameterChoice* masterOnParam;

    //==============================================================================
    juce::AudioProcessorValueTreeState& getAPVTS() noexcept { return apvts; }

private:
    //==============================================================================
    static constexpr int kNumParameters = 12;
    using ParameterSnapshot = std::array<float, kNumParameters>;

    ParameterSnapshot captureParameterSnapshot() const;
    void applyParameterSnapshot(const ParameterSnapshot& snapshot);
    void storeCurrentParametersInSlot(int slot);
    void initialiseABState();

    void applyPreset(const CrushJoints::Preset& preset);
    CrushJoints::Preset captureAsPreset(const juce::String& name) const;

    juce::AudioProcessorValueTreeState apvts;

    std::array<juce::RangedAudioParameter*, kNumParameters> parameterRefs {};

    // A/B parameter snapshots (normalised 0..1 values) and active slot.
    ParameterSnapshot abSlots[2] {};
    int activeABSlot = 0;
    int currentProgram = 0;
    mutable juce::CriticalSection abLock;

    std::array<std::atomic<bool>, numLockableParameters> parameterLocks {};

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