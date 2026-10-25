#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>

#include "PluginProcessor.h"

#include <iostream>
#include <cstdlib>

namespace
{
    int failures = 0;

    void check(bool condition, const juce::String& message)
    {
        if (! condition)
        {
            ++failures;
            std::cerr << "FAIL: " << message << '\n';
        }
    }

    void checkClose(float actual, float expected, float tolerance, const juce::String& message)
    {
        if (std::abs(actual - expected) > tolerance)
        {
            ++failures;
            std::cerr << "FAIL: " << message
                      << " (expected " << expected << ", got " << actual << ")\n";
        }
    }

    juce::RangedAudioParameter* getParam(CrushJointsAudioProcessor& processor, const char* id)
    {
        return processor.getAPVTS().getParameter(id);
    }

    void setValue(CrushJointsAudioProcessor& processor, const char* id, float value)
    {
        auto* parameter = getParam(processor, id);
        parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
    }

    float getValue(CrushJointsAudioProcessor& processor, const char* id)
    {
        auto* parameter = getParam(processor, id);
        return parameter->convertFrom0to1(parameter->getValue());
    }

    void testABComparison()
    {
        CrushJointsAudioProcessor processor;

        processor.setActiveABSlot(0);
        setValue(processor, "tube", 0.8f);

        processor.setActiveABSlot(1);
        checkClose(getValue(processor, "tube"), 0.0f, 1.0e-4f, "B starts from defaults");

        processor.setActiveABSlot(0);
        checkClose(getValue(processor, "tube"), 0.8f, 1.0e-4f, "A restores the edited value");

        processor.copyActiveABToInactive(); // A -> B
        setValue(processor, "tube", 0.2f);

        processor.setActiveABSlot(1);
        checkClose(getValue(processor, "tube"), 0.8f, 1.0e-4f, "copy A to B");

        processor.setActiveABSlot(0);
        checkClose(getValue(processor, "tube"), 0.2f, 1.0e-4f, "A retains its edit after copy");
    }

    void testProgramsAndState()
    {
        CrushJointsAudioProcessor processor;

        check(processor.getNumPrograms() >= 12, "factory program count");
        check(processor.getCurrentProgram() == 0, "initial program is Default");

        processor.setCurrentProgram(3); // GTR Crunch
        check(processor.getCurrentProgram() == 3, "current program index");
        checkClose(getValue(processor, "tube"), 0.4f, 1.0e-4f, "GTR Crunch tube");
        checkClose(getValue(processor, "cutoff"), 6000.0f, 1.0e-3f, "GTR Crunch cutoff");
        check(processor.getProgramName(3) == "GTR Crunch", "GTR Crunch name");

        // Set up a known A/B pair, then save and mutate the state.
        processor.setActiveABSlot(0);
        setValue(processor, "tube", 0.6f);
        processor.setActiveABSlot(1);
        setValue(processor, "tube", 0.3f);
        processor.setCurrentProgram(3); // active B becomes GTR Crunch

        juce::MemoryBlock state;
        processor.getStateInformation(state);

        processor.setCurrentProgram(0);
        setValue(processor, "tube", 0.9f);

        processor.setStateInformation(state.getData(), static_cast<int>(state.getSize()));

        check(processor.getCurrentProgram() == 3, "program index survives state restore");
        check(processor.getActiveABSlot() == 1, "active A/B slot survives state restore");
        checkClose(getValue(processor, "tube"), 0.4f, 1.0e-4f, "parameters survive state restore");

        processor.setActiveABSlot(0);
        checkClose(getValue(processor, "tube"), 0.6f, 1.0e-4f, "slot A survives state restore");

        processor.setActiveABSlot(1);
        checkClose(getValue(processor, "tube"), 0.4f, 1.0e-4f, "slot B survives state restore");
    }

    void renderEditorSnapshot()
    {
        if (std::getenv("CRUSHJOINTS_RENDER_UI") == nullptr)
            return;

        CrushJointsAudioProcessor processor;
        processor.setParameterLocked(CrushJointsAudioProcessor::lockableTube, true);
        processor.setParameterLocked(CrushJointsAudioProcessor::lockableRes, true);
        std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());

        if (editor == nullptr)
            return;

        editor->setVisible(true);

        for (const auto& size : { std::pair<int, int> { 688, 352 },
                                  std::pair<int, int> { 860, 440 },
                                  std::pair<int, int> { 1290, 660 } })
        {
            editor->setSize(size.first, size.second);

            const auto image = editor->createComponentSnapshot(editor->getLocalBounds());
            const juce::File file("/tmp/crush-editor-" + juce::String(size.first)
                                  + "x" + juce::String(size.second) + ".png");
            file.deleteFile();
            auto stream = file.createOutputStream();

            if (stream != nullptr)
            {
                juce::PNGImageFormat png;
                png.writeImageToStream(image, *stream);
                std::cout << "Wrote " << file.getFullPathName() << "\n";
            }
        }
    }

    void testParameterLocksAndRandomize()
    {
        CrushJointsAudioProcessor processor;

        for (int i = 0; i < CrushJointsAudioProcessor::numLockableParameters; ++i)
        {
            check(! processor.isParameterLocked(i), "locks start disabled");
            processor.setParameterLocked(i, true);
        }

        setValue(processor, "tube", 0.42f);
        setValue(processor, "mech", 0.17f);
        setValue(processor, "cutoff", 5000.0f);
        setValue(processor, "res", 0.3f);
        setValue(processor, "amount", 0.6f);
        setValue(processor, "volume", -3.0f);
        setValue(processor, "mix", 0.8f);
        setValue(processor, "tube_on", 1.0f);
        setValue(processor, "lpf_on", 0.0f);
        setValue(processor, "phat", 0.0f);
        setValue(processor, "comp_on", 1.0f);
        setValue(processor, "master_on", 1.0f);

        const float expectedTube = getValue(processor, "tube");
        const float expectedMech = getValue(processor, "mech");
        const float expectedCutoff = getValue(processor, "cutoff");
        const float expectedRes = getValue(processor, "res");
        const float expectedAmount = getValue(processor, "amount");
        const float expectedVolume = getValue(processor, "volume");
        const float expectedMix = getValue(processor, "mix");

        for (int i = 0; i < 20; ++i)
            processor.randomizeUnlockedParameters();

        checkClose(getValue(processor, "tube"), expectedTube, 1.0e-6f, "locked tube is untouched");
        checkClose(getValue(processor, "mech"), expectedMech, 1.0e-6f, "locked mech is untouched");
        checkClose(getValue(processor, "cutoff"), expectedCutoff, 1.0e-3f, "locked cutoff is untouched");
        checkClose(getValue(processor, "res"), expectedRes, 1.0e-6f, "locked res is untouched");
        checkClose(getValue(processor, "amount"), expectedAmount, 1.0e-6f, "locked amount is untouched");
        checkClose(getValue(processor, "volume"), expectedVolume, 1.0e-3f, "locked volume is untouched");
        checkClose(getValue(processor, "mix"), expectedMix, 1.0e-6f, "locked mix is untouched");

        // On/off switches are never randomised, even when unlocked.
        checkClose(getValue(processor, "tube_on"), 1.0f, 0.0f, "tube_on is not randomised");
        checkClose(getValue(processor, "lpf_on"), 0.0f, 0.0f, "lpf_on is not randomised");
        checkClose(getValue(processor, "phat"), 0.0f, 0.0f, "phat is not randomised");
        checkClose(getValue(processor, "comp_on"), 1.0f, 0.0f, "comp_on is not randomised");
        checkClose(getValue(processor, "master_on"), 1.0f, 0.0f, "master_on is not randomised");

        // Unlocking one continuous parameter lets randomize change it.
        processor.setParameterLocked(CrushJointsAudioProcessor::lockableTube, false);
        bool tubeChanged = false;

        for (int i = 0; i < 50 && ! tubeChanged; ++i)
        {
            processor.randomizeUnlockedParameters();
            tubeChanged = std::abs(getValue(processor, "tube") - expectedTube) > 1.0e-5f;
        }

        check(tubeChanged, "unlocked tube is randomised");

        // Lock state survives a plugin-state round-trip.
        processor.setParameterLocked(CrushJointsAudioProcessor::lockableMech, true);
        processor.setParameterLocked(CrushJointsAudioProcessor::lockableCutoff, false);

        juce::MemoryBlock state;
        processor.getStateInformation(state);
        processor.setParameterLocked(CrushJointsAudioProcessor::lockableMech, false);

        processor.setStateInformation(state.getData(), static_cast<int>(state.getSize()));

        check(processor.isParameterLocked(CrushJointsAudioProcessor::lockableMech),
              "locked state survives state restore");
        check(! processor.isParameterLocked(CrushJointsAudioProcessor::lockableCutoff),
              "unlocked state survives state restore");
    }

    void testCustomPresetFromProcessor()
    {
        const auto tempFile = juce::File::getSpecialLocation(juce::File::tempDirectory)
                                  .getNonexistentChildFile("CrushJointsProcessorPresets", ".xml");

        {
            CrushJointsAudioProcessor processor(tempFile);

            check(processor.getNumFactoryPresets() == 12, "12 factory presets exposed");
            check(! processor.isUserPreset(0), "factory preset is not user-owned");

            setValue(processor, "tube", 0.55f);
            setValue(processor, "cutoff", 2345.0f);

            const int index = processor.saveUserPreset("Processor Test");
            check(index == 12, "processor saves a user preset at the end of the bank");
            check(processor.getNumPrograms() == 13, "saved preset extends the program list");
            check(processor.isUserPreset(12), "saved preset is user-owned");
            check(processor.getCurrentProgram() == 12, "saved preset becomes current");
            check(processor.getProgramName(12) == "Processor Test", "saved preset name");

            check(processor.deleteUserPreset(12), "processor deletes a user preset");
            check(processor.getNumPrograms() == 12, "delete restores the factory count");
            check(processor.getCurrentProgram() == 11, "delete falls back to the previous program");
        }

        tempFile.deleteFile();

        // A processor created with the same file should not see the deleted preset.
        {
            CrushJointsAudioProcessor processor(tempFile);
            check(processor.getNumPrograms() == 12, "deleted preset does not reappear");
        }

        tempFile.deleteFile();
    }
}

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInitialiser;

    renderEditorSnapshot();

    testABComparison();
    testProgramsAndState();
    testParameterLocksAndRandomize();
    testCustomPresetFromProcessor();

    if (failures == 0)
    {
        std::cout << "All processor tests passed.\n";
        return 0;
    }

    std::cerr << failures << " test(s) failed.\n";
    return 1;
}
