cmake_minimum_required(VERSION 3.22)
project(LowEndPilot VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT DEFINED JUCE_DIR)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/JUCE-src/CMakeLists.txt")
        set(JUCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/JUCE-src" CACHE PATH "JUCE source tree")
    else()
        set(JUCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../My-808/external/JUCE-src" CACHE PATH "JUCE source tree")
    endif()
endif()
if(NOT EXISTS "${JUCE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "JUCE source not found. Pass -DJUCE_DIR=<path to JUCE>.")
endif()

add_subdirectory("${JUCE_DIR}" "${CMAKE_BINARY_DIR}/JUCE")

juce_add_plugin(LowEndPilot
    COMPANY_NAME "Ricky"
    BUNDLE_ID "com.ricky.lowendpilot"
    PLUGIN_MANUFACTURER_CODE Rcky
    PLUGIN_CODE LePt
    FORMATS VST3 Standalone
    PRODUCT_NAME "DU - Butt-Kicker"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
)

juce_generate_juce_header(LowEndPilot)
juce_add_binary_data(DUArtwork SOURCES assets/du-background.png)
target_sources(LowEndPilot PRIVATE src/PluginProcessor.cpp src/PluginProcessor.h
    src/PluginEditor.cpp src/PluginEditor.h)
target_compile_definitions(LowEndPilot PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)
target_link_libraries(LowEndPilot PRIVATE
    DUArtwork
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

add_executable(LowEndPilotSmoke tests/Smoke.cpp)
target_include_directories(LowEndPilotSmoke PRIVATE src "${CMAKE_BINARY_DIR}/LowEndPilot_artefacts/JuceLibraryCode")
target_link_libraries(LowEndPilotSmoke PRIVATE LowEndPilot DUArtwork juce::juce_audio_utils juce::juce_dsp)
target_compile_definitions(LowEndPilotSmoke PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
