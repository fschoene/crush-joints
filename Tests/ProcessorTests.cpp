#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "PluginProcessor.h"

#include <iostream>

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

    testABComparison();
    testProgramsAndState();
    testCustomPresetFromProcessor();

    if (failures == 0)
    {
        std::cout << "All processor tests passed.\n";
        return 0;
    }

    std::cerr << failures << " test(s) failed.\n";
    return 1;
}
