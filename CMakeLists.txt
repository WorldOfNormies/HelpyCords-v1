cmake_minimum_required(VERSION 3.22)
project(HelpyCords VERSION 2.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add JUCE as a subdirectory (JUCE folder must be in the same folder as this file)
add_subdirectory(JUCE)

# Create the plugin target
juce_add_plugin(HelpyCords
    PRODUCT_NAME "HelpyCords"
    COMPANY_NAME "Agustin"
    PLUGIN_MANUFACTURER_CODE "HELP"
    PLUGIN_CODE "CORD"
    FORMATS VST3
    VST3_CATEGORIES "Instrument" "Synth"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    NEEDS_WEBVIEW OFF
    NEEDS_WEB_BROWSER OFF
    COPY_PLUGIN_AFTER_BUILD FALSE
)

# FIX: This generates JuceHeader.h - without this line the build fails with
#      "Cannot open include file: 'JuceHeader.h'"
juce_generate_juce_header(HelpyCords)

# Add source files
target_sources(HelpyCords PRIVATE
    HelpyCordsPlugin.cpp
    HelpyCordsPlugin.h
    HelpyCordsEditor.cpp
    HelpyCordsEditor.h
)

# Link JUCE modules
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

if(MSVC)
    target_compile_options(HelpyCords PRIVATE /W3 /permissive-)
else()
    target_compile_options(HelpyCords PRIVATE -Wall -Wextra)
endif()
