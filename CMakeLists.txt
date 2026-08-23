cmake_minimum_required(VERSION 3.22)

set(SMART_DENOISE_VERSION "0.3.0" CACHE STRING "Smart Denoise semantic version")
project(SmartDenoise VERSION ${SMART_DENOISE_VERSION} LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(FetchContent)
include(CTest)

set(SMART_DENOISE_JUCE_TAG "8.0.15" CACHE STRING "JUCE git tag to build against")
option(SMART_DENOISE_BUILD_TESTS "Build deterministic engine tests" ON)

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG ${SMART_DENOISE_JUCE_TAG}
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

set(SMART_DENOISE_ENGINE_SOURCES
    Source/DSP/SmartDenoiseEngine.cpp
    Source/DSP/SmartDenoiseEngine.h
)

juce_add_plugin(SmartDenoise
    COMPANY_NAME "Masarray"
    BUNDLE_ID "com.masarray.smartdenoise"
    PLUGIN_MANUFACTURER_CODE Masa
    PLUGIN_CODE SDns
    FORMATS VST3 Standalone
    PRODUCT_NAME "Smart Denoise"
    VERSION ${PROJECT_VERSION}
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_SYNTH FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
)

target_sources(SmartDenoise PRIVATE
    ${SMART_DENOISE_ENGINE_SOURCES}
    Source/Plugin/PluginProcessor.cpp
    Source/Plugin/PluginProcessor.h
    Source/Plugin/PluginEditor.cpp
    Source/Plugin/PluginEditor.h
)

target_include_directories(SmartDenoise PRIVATE Source)

target_compile_definitions(SmartDenoise PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(SmartDenoise PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

if(SMART_DENOISE_BUILD_TESTS)
    juce_add_console_app(SmartDenoiseTests
        PRODUCT_NAME "Smart Denoise Tests"
    )

    target_sources(SmartDenoiseTests PRIVATE
        ${SMART_DENOISE_ENGINE_SOURCES}
        tests/SmartDenoiseTests.cpp
    )

    target_include_directories(SmartDenoiseTests PRIVATE Source)

    target_compile_definitions(SmartDenoiseTests PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
    )

    target_link_libraries(SmartDenoiseTests PRIVATE
        juce::juce_audio_basics
        juce::juce_core
        PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
    )

    add_test(
        NAME SmartDenoise.Engine
        COMMAND SmartDenoiseTests
    )
endif()
