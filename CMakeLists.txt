cmake_minimum_required(VERSION 3.15)
project(CrushJoints VERSION 1.0.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fetch JUCE
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(JUCE)

# Create the plugin
juce_add_plugin(CrushJoints
    COMPANY_NAME "CrushJoints"
    PLUGIN_NAME "CrushJoints"
    PLUGIN_CODE "CJTS"
    IS_SYNTH false
    NEEDS_MIDI_INPUT false
    NEEDS_MIDI_OUTPUT false
    IS_VALUE_TREE_STATE true
    VST3_CATEGORY "Filter"
    AU_MAIN_COMPONENT_CLASS_NAME CrushJointsEditor
)

# Add JUCE modules
target_compile_definitions(CrushJoints
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_STRICT_MODE=1
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
        Source/Common/CamelCrusherCoeffs.cpp
        Source/Common/Presets.cpp
)

# Set include directories
target_include_directories(CrushJoints
    PRIVATE
        Source
        Source/DSP
        Source/Common
        ${JUCE_MODULES_DIR}
)