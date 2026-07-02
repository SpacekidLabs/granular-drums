cmake_minimum_required(VERSION 3.22)

project(GranularDrums VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Fetch JUCE
include(FetchContent)
FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4
)
FetchContent_MakeAvailable(juce)

# Add our plugin
juce_add_plugin(GranularDrums
    COMPANY_NAME "Antigravity Audio"
    PRODUCT_NAME "Granular Drums"
    BUNDLE_ID "com.antigravityaudio.granulardrums"
    PLUGIN_MANUFACTURER_CODE Anti
    PLUGIN_CODE GrDr
    FORMATS VST3 AU Standalone
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
)

juce_generate_juce_header(GranularDrums)

# Add source files
target_sources(GranularDrums
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/DSP/AnalysisEngine.cpp
        Source/DSP/AnalysisEngine.h
        Source/DSP/PadPlaybackEngine.cpp
        Source/DSP/PadPlaybackEngine.h
        Source/DSP/GranularSynth.cpp
        Source/DSP/RegionAnalysisEngine.cpp
        Source/DSP/RegionAnalysisEngine.h
        Source/UI/GranularDrumsLookAndFeel.cpp
        Source/UI/GranularDrumsLookAndFeel.h
        Source/UI/PadGridComponent.cpp
        Source/UI/PadGridComponent.h
        Source/UI/MainWaveformDisplay.cpp
        Source/UI/MainWaveformDisplay.h
        Source/UI/SequencerGridComponent.cpp
        Source/UI/SequencerGridComponent.h
)

# Compile options
target_compile_definitions(GranularDrums
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_STRICT_REFCOUNTEDPOINTER=1
        JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
)

# Link to JUCE modules
target_link_libraries(GranularDrums
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_audio_formats
        juce::juce_audio_devices
        juce::juce_core
        juce::juce_data_structures
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
