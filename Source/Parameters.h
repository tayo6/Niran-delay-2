#pragma once
#include <JuceHeader.h>
#include <array>

//==============================================================================
// Parameter IDs
namespace ParamIDs
{
    static const juce::String sync        { "sync" };
    static const juce::String division    { "division" };
    static const juce::String delayMs     { "delayMs" };
    static const juce::String feedback    { "feedback" };
    static const juce::String mix         { "mix" };
    static const juce::String loFilter    { "loFilter" };
    static const juce::String hiFilter    { "hiFilter" };
    static const juce::String width       { "width" };
}

//==============================================================================
// Sync division table — matches the design spec exactly.
// index -> { display name, number of pipes per bar, length in quarter-note beats }
struct DivisionInfo
{
    const char* name;
    int         numPipes;
    double      beats; // length of one repeat, in quarter notes
};

static const std::array<DivisionInfo, 17> kDivisionTable { {
    { "1/1",     1,  4.0 },
    { "1/2",     2,  2.0 },
    { "1/4",     4,  1.0 },
    { "1/8",     8,  0.5 },
    { "1/16",   16,  0.25 },
    { "1/32",   32,  0.125 },
    { "1/64",   64,  0.0625 },
    { "1/2.",    3,  3.0 },
    { "1/4.",    6,  1.5 },
    { "1/8.",   12,  0.75 },
    { "1/16.",  24,  0.375 },
    { "1/32.",  48,  0.1875 },
    { "1/4T",    3,  0.6666666667 },
    { "1/8T",    6,  0.3333333333 },
    { "1/16T",  12,  0.1666666667 },
    { "1/32T",  24,  0.0833333333 },
    { "1/64T",  48,  0.0416666667 }
} };

static const int kDefaultDivisionIndex = 2; // "1/4"
static const int kMaxPipes = 128;

//==============================================================================
inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { ParamIDs::sync, 1 }, "Sync", true));

    juce::StringArray divisionNames;
    for (auto& d : kDivisionTable)
        divisionNames.add (d.name);

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { ParamIDs::division, 1 }, "Delay Division", divisionNames, kDefaultDivisionIndex));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::delayMs, 1 }, "Delay Time",
        juce::NormalisableRange<float> (1.0f, 2000.0f, 0.1f, 0.35f), 250.0f,
        juce::AudioParameterFloatAttributes().withLabel ("ms")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::feedback, 1 }, "Feedback",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::mix, 1 }, "Mix",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::loFilter, 1 }, "Lo Filter",
        juce::NormalisableRange<float> (20.0f, 20000.0f, 1.0f, 0.25f), 20.0f,
        juce::AudioParameterFloatAttributes().withLabel ("Hz")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::hiFilter, 1 }, "Hi Filter",
        juce::NormalisableRange<float> (20.0f, 20000.0f, 1.0f, 0.25f), 20000.0f,
        juce::AudioParameterFloatAttributes().withLabel ("Hz")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::width, 1 }, "Width",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    return { params.begin(), params.end() };
}
