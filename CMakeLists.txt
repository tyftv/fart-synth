cmake_minimum_required(VERSION 3.22)
project(FartSynth VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.9
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(FartSynth
    COMPANY_NAME "TYF"
    BUNDLE_ID "tv.tellyourfriends.fartsynth"
    PRODUCT_NAME "Fart Synth"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    PLUGIN_MANUFACTURER_CODE Tyfv
    PLUGIN_CODE Frts
    VST3_CATEGORIES Instrument Synth
    FORMATS VST3
    COPY_PLUGIN_AFTER_BUILD FALSE)

juce_add_binary_data(FartSynthData SOURCES assets/logo.png)

target_sources(FartSynth PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/ButtView.cpp
    Source/ButtArt.cpp)

target_compile_definitions(FartSynth PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(FartSynth
    PRIVATE
        FartSynthData
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
