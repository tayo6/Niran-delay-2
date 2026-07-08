#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "CustomLookAndFeel.h"
#include "PipeEditor.h"

class HadenianMonoDelayAudioProcessorEditor : public juce::AudioProcessorEditor,
                                               private juce::Timer
{
public:
    explicit HadenianMonoDelayAudioProcessorEditor (HadenianMonoDelayAudioProcessor&);
    ~HadenianMonoDelayAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void updateDelayKnobBinding();
    juce::Rectangle<int> layoutKnobWithLabel (juce::Rectangle<int> area, juce::Slider& slider,
                                               juce::Label& label, int knobSize);

    HadenianMonoDelayAudioProcessor& audioProcessor;
    HadenianLookAndFeel laf;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    // Top row
    juce::Slider delaySlider, feedbackSlider, mixSlider;
    juce::Label  delayLabel, feedbackLabel, mixLabel;
    juce::ToggleButton syncButton { "SYNC" };

    std::unique_ptr<SliderAttachment> divisionAttachment;
    std::unique_ptr<SliderAttachment> delayMsAttachment;
    std::unique_ptr<SliderAttachment> feedbackAttachment;
    std::unique_ptr<SliderAttachment> mixAttachment;
    std::unique_ptr<ButtonAttachment> syncAttachment;

    // Pipe editor
    PipeEditor pipeEditor;

    // Bottom row
    juce::Slider loFilterSlider, hiFilterSlider, widthSlider;
    juce::Label  loFilterLabel, hiFilterLabel, widthLabel;

    std::unique_ptr<SliderAttachment> loFilterAttachment;
    std::unique_ptr<SliderAttachment> hiFilterAttachment;
    std::unique_ptr<SliderAttachment> widthAttachment;

    bool lastSyncState = true;

    juce::Typeface::Ptr titleTypeface;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HadenianMonoDelayAudioProcessorEditor)
};
