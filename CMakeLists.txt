cmake_minimum_required(VERSION 3.22)

project(SoliVoicerMidiFx VERSION 0.1.0 LANGUAGES C CXX)

set(JUCE_DIR "/Users/santiagotrejo/Desktop/JUCE" CACHE PATH "Path to a JUCE checkout or extracted JUCE folder")

if (NOT EXISTS "${JUCE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "JUCE_DIR must point to a JUCE folder. Set it with: cmake -B build -S . -DJUCE_DIR=/path/to/JUCE")
endif()

add_subdirectory("${JUCE_DIR}" JUCE-build EXCLUDE_FROM_ALL)

juce_add_plugin(SoliVoicerMidiFx
    COMPANY_NAME "santismo"
    BUNDLE_ID "com.santismo.leadvoicer"
    PLUGIN_MANUFACTURER_CODE "StSm"
    PLUGIN_CODE "LdVc"
    FORMATS AU
    PRODUCT_NAME "Lead Voicer"
    IS_SYNTH FALSE
    IS_MIDI_EFFECT TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    AU_MAIN_TYPE kAudioUnitType_MIDIProcessor
    COPY_PLUGIN_AFTER_BUILD TRUE)

juce_generate_juce_header(SoliVoicerMidiFx)

target_sources(SoliVoicerMidiFx
    PRIVATE
        Source/ChordEngine.cpp
        Source/ChordEngine.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h)

target_compile_features(SoliVoicerMidiFx PRIVATE cxx_std_17)

target_compile_definitions(SoliVoicerMidiFx
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0)

target_link_libraries(SoliVoicerMidiFx
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
