#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

/**
    PipeEditor
    ----------
    Replaces the "cube grid" section of the original reference UI with an
    FL-Studio-piano-roll-style velocity lane. Each vertical "pipe" represents
    one delay repeat within a 1-bar cycle; its height (0-127) sets that
    repeat's feedback contribution. A VU-style meter on the right animates
    in real time to show the level of whichever repeat is currently playing.
*/
class PipeEditor : public juce::Component, private juce::Timer
{
public:
    explicit PipeEditor (HadenianMonoDelayAudioProcessor& processorRef);
    ~PipeEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;

private:
    void timerCallback() override;

    juce::Rectangle<float> getGraphBounds() const;
    juce::Rectangle<float> getMeterBounds() const;
    void applyMouseToPipe (const juce::MouseEvent& e);
    static juce::Colour colourForValue (float value0to127);

    HadenianMonoDelayAudioProcessor& processor;

    static constexpr float kLeftAxisWidth  = 42.0f;
    static constexpr float kRightMeterWidth = 62.0f;
    static constexpr float kTopLabelHeight = 26.0f;

    int lastDraggedPipe = -1;
};
