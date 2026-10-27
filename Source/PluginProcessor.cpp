#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

//==============================================================================
CrushJointsAudioProcessor::CrushJointsAudioProcessor(juce::File userPresetFile)
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "parameters", 
           {
                // Distortion parameters
                std::make_unique<juce::AudioParameterFloat>("tube", "Tube", 0.0f, 1.0f, 0.0f),
                std::make_unique<juce::AudioParameterFloat>("mech", "Mech", 0.0f, 1.0f, 0.0f),
                std::make_unique<juce::AudioParameterBool>("tube_on", "Distortion On", true),
                
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
           }),
      presets(userPresetFile)
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
    
    parameterRefs = {
        tubeParam, mechParam, tubeOnParam,
        cutoffParam, resParam, lpfOnParam,
        amountParam, phatParam, compOnParam,
        volumeParam, mixParam, masterOnParam
    };

    // Initialize presets, A/B snapshots and parameter locks.
    presets.initPresets();
    initialiseABState();

    for (auto& lock : parameterLocks)
        lock.store(false);
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
    const auto input = layouts.getMainInputChannelSet();
    const auto output = layouts.getMainOutputChannelSet();

    // Stereo is the primary path. Mono input is supported for mono tracks;
    // mono-in/stereo-out produces centred stereo.
    if (input == juce::AudioChannelSet::stereo()
        && output == juce::AudioChannelSet::stereo())
        return true;

    if (input == juce::AudioChannelSet::mono()
        && output == juce::AudioChannelSet::mono())
        return true;

    if (input == juce::AudioChannelSet::mono()
        && output == juce::AudioChannelSet::stereo())
        return true;

    return false;
}

//==============================================================================
void CrushJointsAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    const int numChannels = juce::jmax(1, getTotalNumOutputChannels());

    // Setup all DSP modules
    distortion.prepareToPlay(sampleRate);
    filter.prepareToPlay(sampleRate, samplesPerBlock, numChannels);
    compressor.prepareToPlay(sampleRate, samplesPerBlock, numChannels);
    master.prepareToPlay(sampleRate, samplesPerBlock, numChannels);

    dryBuffer.setSize(numChannels, samplesPerBlock, false, false, true);
    dryBuffer.clear();

    // No oversampling in the current signal path, so the plugin reports no
    // added latency.
    setLatencySamples(0);
}

void CrushJointsAudioProcessor::releaseResources()
{
    distortion.reset();
    filter.reset();
    compressor.reset();
    master.reset();
    dryBuffer.clear();
}

//==============================================================================
void CrushJointsAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);

    const int numInputChannels = getTotalNumInputChannels();
    const int numOutputChannels = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();

    // A mono track feeding a stereo output needs the mono signal in both
    // channels before the dry capture and the DSP chain.
    if (numInputChannels == 1
        && numOutputChannels == 2
        && buffer.getNumChannels() >= 2)
    {
        buffer.copyFrom(1, 0, buffer, 0, 0, numSamples);
    }

    // Push the current parameter values into the DSP modules.
    distortion.setTubeAmount(tubeParam->get());
    distortion.setMechAmount(mechParam->get());
    distortion.setTubeOn(tubeOnParam->get());

    filter.setCutoff(cutoffParam->get());
    filter.setResonance(resParam->get());
    filter.setFilterOn(lpfOnParam->get());

    compressor.setAmount(amountParam->get());
    compressor.setPhatMode(phatParam->get());
    compressor.setCompressorOn(compOnParam->get());

    const bool masterIsOn = masterOnParam->get();
    const float mix = mixParam->get();

    master.setVolume(volumeParam->get());
    master.setMix(mix);
    master.setMasterOn(masterIsOn);

    // The dry signal is needed for the mix or for a full bypass.
    const bool needsDrySignal = (! masterIsOn) || mix < 0.999f;

    if (needsDrySignal)
    {
        jassert(buffer.getNumSamples() <= dryBuffer.getNumSamples());
        jassert(buffer.getNumChannels() <= dryBuffer.getNumChannels());
        dryBuffer.makeCopyOf(buffer, true);
    }

    // Complete signal chain: Distortion -> Filter -> Compressor -> Master
    distortion.processBlock(buffer);
    filter.processBlock(buffer);
    compressor.processBlock(buffer);
    master.processBlock(buffer, needsDrySignal ? &dryBuffer : nullptr);
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

int CrushJointsAudioProcessor::getNumFactoryPresets() const
{
    return presets.getNumFactoryPresets();
}

bool CrushJointsAudioProcessor::isUserPreset(int programIndex) const
{
    return presets.isUserPreset(programIndex);
}

CrushJoints::PresetCategory CrushJointsAudioProcessor::getPresetCategory(int programIndex) const
{
    return presets.getPresetCategory(programIndex);
}

int CrushJointsAudioProcessor::getCurrentProgram()
{
    return currentProgram;
}

void CrushJointsAudioProcessor::setCurrentProgram(int index)
{
    if (index < 0 || index >= getNumPrograms())
        return;

    applyPreset(presets.getPreset(index));
    currentProgram = index;
    storeCurrentParametersInSlot(getActiveABSlot());

    updateHostDisplay();
    sendChangeMessage();
}

const juce::String CrushJointsAudioProcessor::getProgramName(int index)
{
    if (index >= 0 && index < getNumPrograms())
        return presets.getPreset(index).name;

    return "Default";
}

void CrushJointsAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
    if (! presets.renameUserPreset(index, newName))
        return;

    updateHostDisplay();
    sendChangeMessage();
}

//==============================================================================
int CrushJointsAudioProcessor::saveUserPreset(const juce::String& name)
{
    const auto preset = captureAsPreset(name);

    if (preset.name.isEmpty())
        return -1;

    const int index = presets.saveUserPreset(preset);

    if (index < 0)
        return -1;

    currentProgram = index;
    storeCurrentParametersInSlot(getActiveABSlot());

    updateHostDisplay();
    sendChangeMessage();
    return index;
}

bool CrushJointsAudioProcessor::deleteUserPreset(int programIndex)
{
    if (! presets.isUserPreset(programIndex))
        return false;

    const bool wasCurrent = programIndex == currentProgram;
    const bool wasBeforeCurrent = programIndex < currentProgram;

    if (! presets.deleteUserPreset(programIndex))
        return false;

    if (wasCurrent)
    {
        currentProgram = juce::jlimit(0, juce::jmax(0, getNumPrograms() - 1),
                                     programIndex > 0 ? programIndex - 1 : 0);
        applyPreset(presets.getPreset(currentProgram));
        storeCurrentParametersInSlot(getActiveABSlot());
    }
    else if (wasBeforeCurrent)
    {
        --currentProgram;
    }

    updateHostDisplay();
    sendChangeMessage();
    return true;
}

//==============================================================================
// A/B comparison
int CrushJointsAudioProcessor::getActiveABSlot() const
{
    const juce::ScopedLock lock(abLock);
    return activeABSlot;
}

void CrushJointsAudioProcessor::setActiveABSlot(int slot)
{
    slot = juce::jlimit(0, 1, slot);

    ParameterSnapshot snapshotToApply {};

    {
        const juce::ScopedLock lock(abLock);

        if (slot == activeABSlot)
            return;

        // Preserve any edits made to the slot we are leaving.
        abSlots[activeABSlot] = captureParameterSnapshot();
        activeABSlot = slot;
        snapshotToApply = abSlots[slot];
    }

    applyParameterSnapshot(snapshotToApply);
    sendChangeMessage();
}

void CrushJointsAudioProcessor::copyActiveABToInactive()
{
    {
        const juce::ScopedLock lock(abLock);
        abSlots[activeABSlot] = captureParameterSnapshot();
        abSlots[1 - activeABSlot] = abSlots[activeABSlot];
    }

    sendChangeMessage();
}

bool CrushJointsAudioProcessor::isParameterLocked(int index) const
{
    if (index < 0 || index >= numLockableParameters)
        return false;

    return parameterLocks[static_cast<size_t>(index)].load();
}

void CrushJointsAudioProcessor::setParameterLocked(int index, bool locked)
{
    if (index < 0 || index >= numLockableParameters)
        return;

    parameterLocks[static_cast<size_t>(index)].store(locked);
}

void CrushJointsAudioProcessor::randomizeUnlockedParameters()
{
    juce::Random random;

    auto randomiseNormally = [&](int index, juce::RangedAudioParameter* parameter)
    {
        if (! isParameterLocked(index))
            parameter->setValueNotifyingHost(random.nextFloat());
    };

    randomiseNormally(lockableTube, tubeParam);
    randomiseNormally(lockableMech, mechParam);

    if (! isParameterLocked(lockableCutoff))
    {
        // Logarithmic sweep from 200 Hz to 18 kHz.
        const float cutoff = 200.0f * std::pow(90.0f, random.nextFloat());
        cutoffParam->setValueNotifyingHost(cutoffParam->convertTo0to1(cutoff));
    }

    randomiseNormally(lockableRes, resParam);
    randomiseNormally(lockableAmount, amountParam);

    if (! isParameterLocked(lockableVolume))
    {
        const float volumeDb = random.nextFloat() * 18.0f - 12.0f; // -12 .. +6 dB
        volumeParam->setValueNotifyingHost(volumeParam->convertTo0to1(volumeDb));
    }

    randomiseNormally(lockableMix, mixParam);

    // tube_on, lpf_on, phat, comp_on and master_on are deliberately not
    // randomised: flipping a module off would often produce silence.
}

//==============================================================================
// Parameter snapshots and preset helpers
CrushJointsAudioProcessor::ParameterSnapshot CrushJointsAudioProcessor::captureParameterSnapshot() const
{
    ParameterSnapshot snapshot {};

    for (int i = 0; i < kNumParameters; ++i)
        snapshot[static_cast<size_t>(i)] = parameterRefs[static_cast<size_t>(i)]->getValue();

    return snapshot;
}

void CrushJointsAudioProcessor::applyParameterSnapshot(const ParameterSnapshot& snapshot)
{
    for (int i = 0; i < kNumParameters; ++i)
        parameterRefs[static_cast<size_t>(i)]->setValueNotifyingHost(
            juce::jlimit(0.0f, 1.0f, snapshot[static_cast<size_t>(i)]));
}

void CrushJointsAudioProcessor::storeCurrentParametersInSlot(int slot)
{
    const juce::ScopedLock lock(abLock);
    abSlots[juce::jlimit(0, 1, slot)] = captureParameterSnapshot();
}

void CrushJointsAudioProcessor::initialiseABState()
{
    const juce::ScopedLock lock(abLock);
    abSlots[0] = captureParameterSnapshot();
    abSlots[1] = abSlots[0];
    activeABSlot = 0;
}

void CrushJointsAudioProcessor::applyPreset(const CrushJoints::Preset& preset)
{
    *tubeParam     = preset.tube;
    *mechParam     = preset.mech;
    *tubeOnParam   = preset.tubeOn;
    *cutoffParam   = preset.cutoff;
    *resParam      = preset.res;
    *lpfOnParam    = preset.lpfOn;
    *amountParam   = preset.amount;
    *phatParam     = preset.phat;
    *compOnParam   = preset.compOn;
    *volumeParam   = preset.volume;
    *mixParam      = preset.mix;
    *masterOnParam = preset.masterOn;
}

CrushJoints::Preset CrushJointsAudioProcessor::captureAsPreset(const juce::String& name) const
{
    CrushJoints::Preset preset;
    preset.name = name.trim().substring(0, 64);
    preset.tube = tubeParam->get();
    preset.mech = mechParam->get();
    preset.tubeOn = tubeOnParam->get();
    preset.cutoff = cutoffParam->get();
    preset.res = resParam->get();
    preset.lpfOn = lpfOnParam->get();
    preset.amount = amountParam->get();
    preset.phat = phatParam->get();
    preset.compOn = compOnParam->get();
    preset.volume = volumeParam->get();
    preset.mix = mixParam->get();
    preset.masterOn = masterOnParam->get();
    preset.category = CrushJoints::PresetCategory::user;
    return preset;
}

//==============================================================================
void CrushJointsAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty("currentProgram", currentProgram, nullptr);

    juce::ValueTree abState("AB");
    {
        const juce::ScopedLock lock(abLock);

        // Capture any edits made to the active slot before serialising.
        abSlots[activeABSlot] = captureParameterSnapshot();
        abState.setProperty("active", activeABSlot, nullptr);

        for (int i = 0; i < kNumParameters; ++i)
        {
            const auto index = juce::String(i);
            abState.setProperty("a" + index, static_cast<double>(abSlots[0][static_cast<size_t>(i)]), nullptr);
            abState.setProperty("b" + index, static_cast<double>(abSlots[1][static_cast<size_t>(i)]), nullptr);
        }
    }

    // The restored APVTS state may already contain an AB child; never let
    // repeated save/load cycles accumulate duplicates.
    if (auto existing = state.getChildWithName("AB"); existing.isValid())
        state.removeChild(existing, nullptr);

    state.appendChild(abState, nullptr);

    juce::ValueTree lockState("LOCKS");

    for (int i = 0; i < numLockableParameters; ++i)
        lockState.setProperty("l" + juce::String(i), parameterLocks[static_cast<size_t>(i)].load(), nullptr);

    if (auto existing = state.getChildWithName("LOCKS"); existing.isValid())
        state.removeChild(existing, nullptr);

    state.appendChild(lockState, nullptr);

    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void CrushJointsAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState == nullptr)
        return;

    auto state = juce::ValueTree::fromXml(*xmlState);

    if (! state.isValid())
        return;

    currentProgram = static_cast<int>(state.getProperty("currentProgram", 0));

    auto abState = state.getChildWithName("AB");
    auto lockState = state.getChildWithName("LOCKS");

    // Keep APVTS.state limited to parameter data; program, A/B and lock state
    // are re-attached on every save.
    state.removeProperty("currentProgram", nullptr);

    if (abState.isValid())
        state.removeChild(abState, nullptr);

    if (lockState.isValid())
        state.removeChild(lockState, nullptr);

    apvts.replaceState(state);

    {
        const juce::ScopedLock lock(abLock);

        if (abState.isValid())
        {
            activeABSlot = juce::jlimit(0, 1, static_cast<int>(abState.getProperty("active", 0)));

            for (int i = 0; i < kNumParameters; ++i)
            {
                const auto index = juce::String(i);
                const auto defaultValue = static_cast<double>(parameterRefs[static_cast<size_t>(i)]->getValue());
                abSlots[0][static_cast<size_t>(i)] = static_cast<float>(
                    static_cast<double>(abState.getProperty("a" + index, defaultValue)));
                abSlots[1][static_cast<size_t>(i)] = static_cast<float>(
                    static_cast<double>(abState.getProperty("b" + index, defaultValue)));
            }
        }
        else
        {
            // Legacy state without A/B data: both slots start from the
            // restored parameter values.
            activeABSlot = 0;
            abSlots[0] = captureParameterSnapshot();
            abSlots[1] = abSlots[0];
        }
    }

    for (int i = 0; i < numLockableParameters; ++i)
    {
        const auto propertyName = "l" + juce::String(i);
        parameterLocks[static_cast<size_t>(i)].store(
            lockState.isValid() && static_cast<bool>(lockState.getProperty(propertyName, false)));
    }

    currentProgram = juce::jlimit(0, juce::jmax(0, getNumPrograms() - 1), currentProgram);

    updateHostDisplay();
    sendChangeMessage();
}

//==============================================================================
// This creates new instances of the plugin. It is required by the JUCE
// plugin wrappers and must have exactly this signature.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CrushJointsAudioProcessor();
}