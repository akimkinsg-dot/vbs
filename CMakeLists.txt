cmake_minimum_required(VERSION 3.22)
project(VibeSpark VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

# NOTE: Ableton Live refuses to load VST3s flagged as "MIDI effect".
# So VibeSpark is built as a VST3 *instrument* (silent stereo output) that emits MIDI.
juce_add_plugin(VibeSpark
    COMPANY_NAME "VibeSpark"
    PLUGIN_MANUFACTURER_CODE Vbsp
    PLUGIN_CODE Vspk
    FORMATS VST3
    PRODUCT_NAME "VibeSpark"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Instrument)

target_sources(VibeSpark PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/Engine.cpp)

target_compile_definitions(VibeSpark PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(VibeSpark
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags)
