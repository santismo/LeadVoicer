cmake_minimum_required(VERSION 3.22)

project(VoicizerMidiFx VERSION 1.0.0 LANGUAGES C CXX)

set(JUCE_DIR "/Users/santiagotrejo/Desktop/JUCE" CACHE PATH "Path to a JUCE checkout or extracted JUCE folder")
option(VOICIZER_COPY_PLUGIN_AFTER_BUILD "Copy the built Audio Unit to the user plug-in folder" ON)

if (NOT EXISTS "${JUCE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "JUCE_DIR must point to a JUCE folder. Set it with: cmake -B build -S . -DJUCE_DIR=/path/to/JUCE")
endif()

add_subdirectory("${JUCE_DIR}" JUCE-build EXCLUDE_FROM_ALL)

juce_add_plugin(VoicizerMidiFx
    COMPANY_NAME "santismo"
    BUNDLE_ID "com.santismo.voicizer"
    PLUGIN_MANUFACTURER_CODE "StSm"
    PLUGIN_CODE "Vocz"
    FORMATS AU
    PRODUCT_NAME "Voicizer"
    IS_SYNTH FALSE
    IS_MIDI_EFFECT TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    AU_MAIN_TYPE kAudioUnitType_MIDIProcessor
    COPY_PLUGIN_AFTER_BUILD ${VOICIZER_COPY_PLUGIN_AFTER_BUILD})

juce_generate_juce_header(VoicizerMidiFx)

target_sources(VoicizerMidiFx
    PRIVATE
        Source/ChordEngine.cpp
        Source/ChordEngine.h
        Source/ChordizerLink.cpp
        Source/ChordizerLink.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h)

target_compile_features(VoicizerMidiFx PRIVATE cxx_std_17)

target_compile_definitions(VoicizerMidiFx
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0)

target_link_libraries(VoicizerMidiFx
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

if (APPLE)
    add_custom_command(TARGET VoicizerMidiFx_AU POST_BUILD
        COMMAND /usr/bin/codesign --force --sign - --timestamp=none
            "$<TARGET_BUNDLE_DIR:VoicizerMidiFx_AU>"
        VERBATIM)
    if (VOICIZER_COPY_PLUGIN_AFTER_BUILD)
        add_custom_command(TARGET VoicizerMidiFx_AU POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
                "$ENV{HOME}/Library/Audio/Plug-Ins/Components"
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "$<TARGET_BUNDLE_DIR:VoicizerMidiFx_AU>"
                "$ENV{HOME}/Library/Audio/Plug-Ins/Components/Voicizer.component"
            VERBATIM)
    endif()
endif()

option(VOICIZER_BUILD_TESTS "Build the Voicizer engine tests" ON)
if (VOICIZER_BUILD_TESTS)
    juce_add_console_app(VoicizerTests PRODUCT_NAME "Voicizer Tests")
    juce_generate_juce_header(VoicizerTests)
    target_sources(VoicizerTests PRIVATE Tests/ChordEngineTests.cpp Source/ChordEngine.cpp Source/ChordEngine.h)
    target_compile_features(VoicizerTests PRIVATE cxx_std_17)
    target_compile_definitions(VoicizerTests PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_link_libraries(VoicizerTests PRIVATE juce::juce_audio_utils
        juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
endif()
