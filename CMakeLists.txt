cmake_minimum_required(VERSION 3.22)
project(HelpyCords VERSION 2.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE must be a subfolder called "JUCE" next to this file
add_subdirectory(JUCE)

# -----------------------------------------------------------------------
# Plugin target — builds VST3 + Standalone (Standalone = .exe you can run
# directly without a DAW, useful for testing)
# -----------------------------------------------------------------------
juce_add_plugin(HelpyCords
    PRODUCT_NAME        "HelpyCords"
    COMPANY_NAME        "WorldOfNormies"
    PLUGIN_MANUFACTURER_CODE "WOFN"
    PLUGIN_CODE         "HCRD"
    FORMATS             VST3 Standalone
    VST3_CATEGORIES     "Instrument" "Synth"
    IS_SYNTH            TRUE
    NEEDS_MIDI_INPUT    TRUE
    NEEDS_MIDI_OUTPUT   TRUE
    COPY_PLUGIN_AFTER_BUILD TRUE
)

# Source files
target_sources(HelpyCords PRIVATE
    HelpyCordsPlugin.cpp
    HelpyCordsPlugin.h
    HelpyCordsEditor.cpp
    HelpyCordsEditor.h
)

# JUCE module links
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

# Compiler flags
if(MSVC)
    # /W3 but NOT /WX — JUCE headers have warnings we don't control
    target_compile_options(HelpyCords PRIVATE /W3 /permissive-)
else()
    target_compile_options(HelpyCords PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()

# -----------------------------------------------------------------------
# Print output paths after build so you know where to find the files
# -----------------------------------------------------------------------
message(STATUS "Build directory: ${CMAKE_BINARY_DIR}")
message(STATUS "VST3 will be copied to the system VST3 folder automatically.")
