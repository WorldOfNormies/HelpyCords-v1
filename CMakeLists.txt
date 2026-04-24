cmake_minimum_required(VERSION 3.22)
project(HelpyCords VERSION 2.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(JUCE)

juce_add_plugin(HelpyCords
    PRODUCT_NAME "HelpyCords"
    COMPANY_NAME "Agustin"
    PLUGIN_MANUFACTURER_CODE "Agst"
    PLUGIN_CODE "HCrd"
    FORMATS VST3
    VST3_CATEGORIES "Instrument" "Synth"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    NEEDS_WEBVIEW OFF
    NEEDS_WEB_BROWSER OFF
    COPY_PLUGIN_AFTER_BUILD FALSE

    # FIX: This disables the VST2/VST3 parameter ID conflict check.
    # The error occurs when JUCE thinks you might have both VST2 and VST3
    # formats and their parameter IDs could clash. Since we only build VST3
    # this flag tells JUCE we acknowledge there is no conflict.
    VST3_AUTO_MANIFEST TRUE
)

# FIX: Generates JuceHeader.h - required for #include <JuceHeader.h> to work
juce_generate_juce_header(HelpyCords)

target_sources(HelpyCords PRIVATE
    HelpyCordsPlugin.cpp
    HelpyCordsPlugin.h
    HelpyCordsEditor.cpp
    HelpyCordsEditor.h
)

target_link_libraries(HelpyCords
    PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_data_structures
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# Suppress the VST2/VST3 parameter conflict error entirely
target_compile_definitions(HelpyCords
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
)

if(MSVC)
    # /W2 instead of /W3 to reduce noise from JUCE deprecation warnings
    target_compile_options(HelpyCords PRIVATE /W2 /permissive-)
else()
    target_compile_options(HelpyCords PRIVATE -Wall -Wextra)
endif()
