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

        auto renderState = [](const juce::String& tag, bool masterOff)
        {
            CrushJointsAudioProcessor processor;
            processor.setParameterLocked(CrushJointsAudioProcessor::lockableTube, true);
            processor.setParameterLocked(CrushJointsAudioProcessor::lockableRes, true);

            if (masterOff)
            {
                auto* masterOn = processor.getAPVTS().getParameter("master_on");
                masterOn->setValueNotifyingHost(0.0f);
            }

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
                const juce::File file("/tmp/crush-editor-" + tag + "-"
                                      + juce::String(size.first) + "x"
                                      + juce::String(size.second) + ".png");
                file.deleteFile();
                auto stream = file.createOutputStream();

                if (stream != nullptr)
                {
                    juce::PNGImageFormat png;
                    png.writeImageToStream(image, *stream);
                    std::cout << "Wrote " << file.getFullPathName() << "\n";
                }
            }
        };

        renderState("default", false);
        renderState("master-off", true);
    }

    juce::AudioBuffer<float> renderProcessor(CrushJointsAudioProcessor& processor,
                                             const juce::AudioBuffer<float>& input)
    {
        juce::AudioBuffer<float> output;
        output.makeCopyOf(input);

        constexpr int blockSize = 512;
        juce::MidiBuffer midi;

        for (int offset = 0; offset < output.getNumSamples(); offset += blockSize)
        {
            const int count = juce::jmin(blockSize, output.getNumSamples() - offset);
            juce::AudioBuffer<float> block(output.getNumChannels(), count);

            for (int ch = 0; ch < output.getNumChannels(); ++ch)
                block.copyFrom(ch, 0, output, ch, offset, count);

            processor.processBlock(block, midi);

            for (int ch = 0; ch < output.getNumChannels(); ++ch)
                output.copyFrom(ch, offset, block, ch, 0, count);
        }

        return output;
    }

    float rmsOfBuffer(const juce::AudioBuffer<float>& buffer)
    {
        double sum = 0.0;
        const int count = buffer.getNumSamples() * buffer.getNumChannels();

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            for (int i = 0; i < buffer.getNumSamples(); ++i)
                sum += static_cast<double>(buffer.getSample(ch, i))
                     * static_cast<double>(buffer.getSample(ch, i));

        return static_cast<float>(std::sqrt(sum / juce::jmax(1, count)));
    }

    void testProcessorDspWiring()
    {
        constexpr double sampleRate = 48000.0;
        constexpr int numSamples = 96000;
        constexpr float inputAmplitude = 0.1f;
        constexpr float toneFrequency = 1000.0f;

        juce::AudioBuffer<float> input(2, numSamples);
        juce::AudioBuffer<float> lowInput(2, numSamples);

        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < numSamples; ++i)
            {
                input.setSample(ch, i,
                                inputAmplitude * std::sin(2.0f * juce::MathConstants<float>::pi
                                                          * toneFrequency
                                                          * static_cast<float>(i)
                                                          / static_cast<float>(sampleRate)));

                lowInput.setSample(ch, i,
                                   inputAmplitude * std::sin(2.0f * juce::MathConstants<float>::pi
                                                             * 80.0f
                                                             * static_cast<float>(i)
                                                             / static_cast<float>(sampleRate)));
            }

        // Filter resonance: a tone at the cutoff should get louder with Q.
        {
            CrushJointsAudioProcessor processor;
            processor.prepareToPlay(sampleRate, 512);

            setValue(processor, "tube_on", 0.0f);
            setValue(processor, "lpf_on", 1.0f);
            setValue(processor, "cutoff", toneFrequency);
            setValue(processor, "comp_on", 0.0f);
            setValue(processor, "master_on", 1.0f);
            setValue(processor, "mix", 1.0f);
            setValue(processor, "volume", 0.0f);

            setValue(processor, "res", 0.0f);
            const auto flat = renderProcessor(processor, input);

            setValue(processor, "res", 1.0f);
            const auto resonant = renderProcessor(processor, input);

            // Look at the settled second half to avoid the filter's start-up.
            juce::AudioBuffer<float> flatTail(2, numSamples / 2);
            juce::AudioBuffer<float> resonantTail(2, numSamples / 2);
            for (int ch = 0; ch < 2; ++ch)
            {
                flatTail.copyFrom(ch, 0, flat, ch, numSamples / 2, numSamples / 2);
                resonantTail.copyFrom(ch, 0, resonant, ch, numSamples / 2, numSamples / 2);
            }

            const auto flatRms = rmsOfBuffer(flatTail);
            const auto resonantRms = rmsOfBuffer(resonantTail);

            std::cout << "Filter RMS: flat=" << flatRms << " resonant=" << resonantRms << "\n";
            check(resonantRms > flatRms * 1.5f, "filter resonance clearly boosts the cutoff");
        }

        // Compressor amount and Phat must change the output.
        {
            CrushJointsAudioProcessor processor;
            processor.prepareToPlay(sampleRate, 512);

            setValue(processor, "tube_on", 0.0f);
            setValue(processor, "lpf_on", 0.0f);
            setValue(processor, "comp_on", 1.0f);
            setValue(processor, "master_on", 1.0f);
            setValue(processor, "mix", 1.0f);
            setValue(processor, "volume", 0.0f);
            setValue(processor, "phat", 0.0f);

            setValue(processor, "amount", 0.0f);
            const auto uncompressed = renderProcessor(processor, input);

            setValue(processor, "amount", 1.0f);
            const auto compressed = renderProcessor(processor, input);

            const auto uncompressedRms = rmsOfBuffer(uncompressed);
            const auto compressedRms = rmsOfBuffer(compressed);

            std::cout << "Compressor RMS: off=" << uncompressedRms
                      << " on=" << compressedRms << "\n";
            check(std::abs(compressedRms - uncompressedRms) > 0.01f,
                  "compressor amount changes the output");

            const auto compressedLow = renderProcessor(processor, lowInput);
            setValue(processor, "phat", 1.0f);
            const auto phatLow = renderProcessor(processor, lowInput);
            check(std::abs(rmsOfBuffer(phatLow) - rmsOfBuffer(compressedLow)) > 0.001f,
                  "phat mode changes the low-frequency output");

            // Phat is independent of Amount.
            setValue(processor, "amount", 0.0f);
            setValue(processor, "phat", 0.0f);
            const auto amountZeroNoPhat = renderProcessor(processor, lowInput);
            setValue(processor, "phat", 1.0f);
            const auto amountZeroPhat = renderProcessor(processor, lowInput);
            check(std::abs(rmsOfBuffer(amountZeroPhat) - rmsOfBuffer(amountZeroNoPhat)) > 0.0005f,
                  "phat mode is audible at Amount 0");
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
    testProcessorDspWiring();
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
