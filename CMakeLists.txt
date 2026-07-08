cmake_minimum_required(VERSION 3.22)

project(HadenianMonoDelay VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# Fetch JUCE 7.0.12
# ---------------------------------------------------------------------------
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        7.0.12
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

# ---------------------------------------------------------------------------
# Embedded assets (display font used for the plugin title)
# ---------------------------------------------------------------------------
juce_add_binary_data(HadenianMonoDelayAssets
    SOURCES
        Assets/Bangers-Regular.ttf
)
set_target_properties(HadenianMonoDelayAssets PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

# ---------------------------------------------------------------------------
# Plugin target
# ---------------------------------------------------------------------------
juce_add_plugin(HadenianMonoDelay
    COMPANY_NAME                "Niran Audio"
    PLUGIN_MANUFACTURER_CODE    Nrna
    PLUGIN_CODE                 Hmd1
    FORMATS                     VST3
    PRODUCT_NAME                "huh"
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     FALSE
    VST3_CATEGORIES             Fx Delay
    BUNDLE_ID                   com.niranaudio.huh
)

target_sources(HadenianMonoDelay
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/DelayEngine.cpp
        Source/PipeEditor.cpp
        Source/CustomLookAndFeel.cpp
)

target_compile_definitions(HadenianMonoDelay
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_STRICT_REFCOUNTEDPOINTER=1
)

juce_generate_juce_header(HadenianMonoDelay)

target_link_libraries(HadenianMonoDelay
    PRIVATE
        HadenianMonoDelayAssets
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
