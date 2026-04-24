cmake_minimum_required(VERSION 3.22)
project(HelpyCords VERSION 2.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add JUCE as a subdirectory
add_subdirectory(JUCE)

# Create the plugin target
juce_add_plugin(HelpyCords
    PRODUCT_NAME "HelpyCords"
    COMPANY_NAME "Agustin"
    # Ensure these codes are exactly 4 characters
    PLUGIN_MANUFACTURER_CODE "Agst" 
    PLUGIN_CODE "HCrd"
    FORMATS VST3
    VST3_CATEGORIES "Instrument" "Synth"
    # CRITICAL: This defines it as a synthesizer
    IS_SYNTH TRUE                  
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    NEEDS_WEBVIEW OFF
    NEEDS_WEB_BROWSER OFF
    COPY_PLUGIN_AFTER_BUILD TRUE

    # FIX: Helps with metadata generation and DAW discovery
    VST3_AUTO_MANIFEST TRUE
)

# This helps resolve metadata issues in some DAWs and enables #include <JuceHeader.h>
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
        juce::juce_recommended_video_link_flags
)

# FIX: Suppress the VST2/VST3 parameter conflict error entirely.
# This often prevents the plugin from loading if the DAW checks for legacy VST2 compatibility.
target_compile_definitions(HelpyCords
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
)

# Compiler-specific settings to reduce warnings and ensure strict compliance
if(MSVC)
    target_compile_options(HelpyCords PRIVATE /W2 /permissive-)
else()
    target_compile_options(HelpyCords PRIVATE -Wall -Wextra)
endif()
