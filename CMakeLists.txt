cmake_minimum_required(VERSION 3.22)
project(CrushJoints VERSION 1.0.0 LANGUAGES C CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Path to a local JUCE checkout. Override with:
#   cmake -B build -DJUCE_ROOT=/path/to/JUCE
set(JUCE_ROOT "/Volumes/WDB/user/flo/JUCE" CACHE PATH "Path to the JUCE source tree")

if(NOT EXISTS "${JUCE_ROOT}/CMakeLists.txt")
    message(FATAL_ERROR
        "JUCE was not found at '${JUCE_ROOT}'. "
        "Pass -DJUCE_ROOT=/path/to/JUCE when configuring.")
endif()

add_subdirectory("${JUCE_ROOT}" JUCE-Build)

# Create the plugin
juce_add_plugin(CrushJoints
    COMPANY_NAME "CrushJoints"
    PLUGIN_NAME "CrushJoints"
    PLUGIN_CODE "CJTS"
    PLUGIN_MANUFACTURER_CODE "Crsh"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    VST3_CATEGORY "Fx"
    VST3_SUBCATEGORY "Filter"
    AU_MAIN_COMPONENT_CLASS_NAME CrushJointsEditor
    FORMATS VST3 AU Standalone
)

# Add JUCE modules
target_compile_definitions(CrushJoints
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_STRICT_MODE=1
        JUCE_VST3_CAN_REPLACE_VST2=0
)

# Add JUCE modules for audio processing and GUI
target_link_libraries(CrushJoints
    PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_dsp
        juce::juce_gui_basics
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# Add source files
target_sources(CrushJoints
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/DSP/Oversampler.cpp
        Source/DSP/Distortion.cpp
        Source/DSP/LadderFilter.cpp
        Source/DSP/Compressor.cpp
        Source/DSP/Master.cpp
        Source/Common/CamelCrusherCoeffs.cpp
        Source/Common/Presets.cpp
)

# Set include directories
target_include_directories(CrushJoints
    PRIVATE
        Source
        Source/DSP
        Source/Common
)

#==============================================================================
# Offline DSP tests
option(CRUSHJOINTS_BUILD_TESTS "Build the offline DSP test executable" ON)

if(CRUSHJOINTS_BUILD_TESTS)
    juce_add_console_app(CrushJointsTests PRODUCT_NAME "CrushJointsTests")

    target_compile_definitions(CrushJointsTests
        PRIVATE
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0
            JUCE_STRICT_MODE=1
    )

    target_sources(CrushJointsTests
        PRIVATE
            Tests/DSPTests.cpp
            Source/DSP/Distortion.cpp
            Source/DSP/LadderFilter.cpp
            Source/DSP/Compressor.cpp
            Source/DSP/Master.cpp
            Source/Common/CamelCrusherCoeffs.cpp
            Source/Common/Presets.cpp
    )

    target_include_directories(CrushJointsTests
        PRIVATE
            Source
            Source/DSP
            Source/Common
    )

    target_link_libraries(CrushJointsTests
        PRIVATE
            juce::juce_dsp
            juce::juce_audio_basics
            juce::juce_core
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_warning_flags
    )

    #--------------------------------------------------------------------------
    # Processor-level tests: A/B state, program changes and plugin state.
    juce_add_console_app(CrushJointsProcessorTests PRODUCT_NAME "CrushJointsProcessorTests")

    target_compile_definitions(CrushJointsProcessorTests
        PRIVATE
            JucePlugin_Name="CrushJoints"
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0
            JUCE_STRICT_MODE=1
    )

    target_sources(CrushJointsProcessorTests
        PRIVATE
            Tests/ProcessorTests.cpp
            Source/PluginProcessor.cpp
            Source/PluginEditor.cpp
            Source/DSP/Distortion.cpp
            Source/DSP/LadderFilter.cpp
            Source/DSP/Compressor.cpp
            Source/DSP/Master.cpp
            Source/DSP/Oversampler.cpp
            Source/Common/CamelCrusherCoeffs.cpp
            Source/Common/Presets.cpp
    )

    target_include_directories(CrushJointsProcessorTests
        PRIVATE
            Source
            Source/DSP
            Source/Common
    )

    target_link_libraries(CrushJointsProcessorTests
        PRIVATE
            juce::juce_audio_processors
            juce::juce_audio_utils
            juce::juce_dsp
            juce::juce_gui_basics
            juce::juce_gui_extra
            juce::juce_audio_basics
            juce::juce_core
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_warning_flags
    )
endif()
