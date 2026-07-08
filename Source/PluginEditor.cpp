#include "PluginEditor.h"
#include "BinaryData.h"

namespace
{
    juce::String formatPercent (double v)  { return juce::String (v, 1) + " %"; }

    juce::String formatHz (double v)
    {
        if (v < 1000.0)
            return juce::String (v, 1) + " Hz";
        return juce::String (v / 1000.0, 1) + " kHz";
    }

    double parseHz (const juce::String& text)
    {
        auto t = text.trim().toLowerCase();
        double mult = t.contains ("k") ? 1000.0 : 1.0;
        return t.getDoubleValue() * mult;
    }
}

HadenianMonoDelayAudioProcessorEditor::HadenianMonoDelayAudioProcessorEditor (HadenianMonoDelayAudioProcessor& p)
    : juce::AudioProcessorEditor (&p), audioProcessor (p), pipeEditor (p)
{
    setLookAndFeel (&laf);

    auto styleKnob = [this] (juce::Slider& s, juce::Label& l, const juce::String& text)
    {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 84, 20);
        s.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f,
                                juce::MathConstants<float>::pi * 2.8f, true);
        s.setColour (juce::Slider::textBoxTextColourId, HadenianColours::displayText);
        s.setColour (juce::Slider::textBoxBackgroundColourId, HadenianColours::displayBg);
        s.setColour (juce::Slider::textBoxOutlineColourId, HadenianColours::displayBorder);
        addAndMakeVisible (s);

        l.setText (text, juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centred);
        l.setColour (juce::Label::textColourId, HadenianColours::textMuted);
        l.setFont (juce::Font (13.0f, juce::Font::bold));
        addAndMakeVisible (l);
    };

    styleKnob (delaySlider,    delayLabel,    "DELAY");
    styleKnob (feedbackSlider, feedbackLabel, "FEEDBACK");
    styleKnob (mixSlider,      mixLabel,      "MIX");
    styleKnob (loFilterSlider, loFilterLabel, "LO FILTER");
    styleKnob (hiFilterSlider, hiFilterLabel, "HI FILTER");
    styleKnob (widthSlider,    widthLabel,    "WIDTH");

    titleTypeface = juce::Typeface::createSystemTypefaceFor (BinaryData::BangersRegular_ttf,
                                                               BinaryData::BangersRegular_ttfSize);

    feedbackSlider.textFromValueFunction = [] (double v) { return formatPercent (v); };
    mixSlider.textFromValueFunction      = [] (double v) { return formatPercent (v); };
    widthSlider.textFromValueFunction    = [] (double v) { return formatPercent (v); };
    loFilterSlider.textFromValueFunction = [] (double v) { return formatHz (v); };
    hiFilterSlider.textFromValueFunction = [] (double v) { return formatHz (v); };
    loFilterSlider.valueFromTextFunction = [] (const juce::String& t) { return parseHz (t); };
    hiFilterSlider.valueFromTextFunction = [] (const juce::String& t) { return parseHz (t); };

    addAndMakeVisible (syncButton);
    syncButton.onClick = [this] { updateDelayKnobBinding(); };

    addAndMakeVisible (pipeEditor);

    auto& apvts = audioProcessor.apvts;

    feedbackAttachment = std::make_unique<SliderAttachment> (apvts, ParamIDs::feedback, feedbackSlider);
    mixAttachment      = std::make_unique<SliderAttachment> (apvts, ParamIDs::mix, mixSlider);
    loFilterAttachment = std::make_unique<SliderAttachment> (apvts, ParamIDs::loFilter, loFilterSlider);
    hiFilterAttachment = std::make_unique<SliderAttachment> (apvts, ParamIDs::hiFilter, hiFilterSlider);
    widthAttachment    = std::make_unique<SliderAttachment> (apvts, ParamIDs::width, widthSlider);
    syncAttachment     = std::make_unique<ButtonAttachment> (apvts, ParamIDs::sync, syncButton);

    updateDelayKnobBinding();

    setResizable (true, true);
    if (auto* c = getConstrainer())
        c->setFixedAspectRatio (3.0 / 4.0);
    setResizeLimits (360, 480, 1080, 1440);
    setSize (720, 960);

    startTimerHz (10);
}

HadenianMonoDelayAudioProcessorEditor::~HadenianMonoDelayAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void HadenianMonoDelayAudioProcessorEditor::timerCallback()
{
    bool syncOn = *audioProcessor.apvts.getRawParameterValue (ParamIDs::sync) > 0.5f;
    if (syncOn != lastSyncState)
        updateDelayKnobBinding();
}

void HadenianMonoDelayAudioProcessorEditor::updateDelayKnobBinding()
{
    bool syncOn = *audioProcessor.apvts.getRawParameterValue (ParamIDs::sync) > 0.5f;

    divisionAttachment.reset();
    delayMsAttachment.reset();

    if (syncOn)
    {
        delaySlider.textFromValueFunction = [] (double v)
        {
            int idx = juce::jlimit (0, (int) kDivisionTable.size() - 1, (int) std::round (v));
            return juce::String (kDivisionTable[(size_t) idx].name);
        };
        delaySlider.valueFromTextFunction = [] (const juce::String& text)
        {
            auto trimmed = text.trim();
            for (size_t i = 0; i < kDivisionTable.size(); ++i)
                if (trimmed.equalsIgnoreCase (kDivisionTable[i].name))
                    return (double) i;
            return 0.0;
        };
        divisionAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, ParamIDs::division, delaySlider);
    }
    else
    {
        delaySlider.textFromValueFunction = [] (double v) { return juce::String (v, 1) + " ms"; };
        delaySlider.valueFromTextFunction = [] (const juce::String& text) { return text.getDoubleValue(); };
        delayMsAttachment = std::make_unique<SliderAttachment> (audioProcessor.apvts, ParamIDs::delayMs, delaySlider);
    }

    delaySlider.updateText();
    lastSyncState = syncOn;
}

juce::Rectangle<int> HadenianMonoDelayAudioProcessorEditor::layoutKnobWithLabel (
    juce::Rectangle<int> area, juce::Slider& slider, juce::Label& label, int knobSize)
{
    auto labelArea = area.removeFromTop (18);
    label.setBounds (labelArea);

    auto knobArea = area.removeFromTop (knobSize + 34);
    knobArea = knobArea.withSizeKeepingCentre (knobSize, knobSize + 34);
    slider.setBounds (knobArea);

    return area;
}

void HadenianMonoDelayAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (HadenianColours::panelBg);

    auto bounds = getLocalBounds();
    auto titleBar = bounds.removeFromTop ((int) (getHeight() * 0.062f));

    g.setColour (HadenianColours::panelBgDarker);
    g.fillRect (titleBar);
    g.setColour (HadenianColours::sectionDivider);
    g.drawLine ((float) titleBar.getX(), (float) titleBar.getBottom(),
                (float) titleBar.getRight(), (float) titleBar.getBottom(), 1.2f);

    auto iconArea = titleBar.removeFromLeft (titleBar.getHeight()).reduced ((int) (titleBar.getHeight() * 0.28f));
    juce::Path tri;
    tri.addTriangle ((float) iconArea.getX(), (float) iconArea.getY(),
                      (float) iconArea.getX(), (float) iconArea.getBottom(),
                      (float) iconArea.getRight(), (float) iconArea.getCentreY());
    g.setColour (HadenianColours::accentBlue);
    g.fillPath (tri);

    auto wordmarkArea = titleBar.removeFromRight ((int) (getWidth() * 0.34f));

    // "huh" in the playful display font
    if (titleTypeface != nullptr)
        g.setFont (juce::Font (titleTypeface).withHeight (titleBar.getHeight() * 0.72f));
    else
        g.setFont (juce::Font (titleBar.getHeight() * 0.6f, juce::Font::bold));
    g.setColour (HadenianColours::accentBlue);
    g.drawText ("huh", titleBar.withTrimmedLeft (8), juce::Justification::centredLeft);

    // Small maker / product-type wordmark on the right
    juce::Font makerFont (titleBar.getHeight() * 0.28f, juce::Font::bold);
    juce::Font typeFont (titleBar.getHeight() * 0.28f, juce::Font::plain);
    auto makerArea = wordmarkArea.removeFromTop (wordmarkArea.getHeight() / 2);
    g.setFont (makerFont);
    g.setColour (HadenianColours::textDark);
    g.drawText ("Niran Audio", makerArea, juce::Justification::centredRight);
    g.setFont (typeFont);
    g.setColour (HadenianColours::textMuted);
    g.drawText ("mono delay", wordmarkArea, juce::Justification::centredRight);
}

void HadenianMonoDelayAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop ((int) (getHeight() * 0.062f));

    auto margin = juce::jmax (10, (int) (getWidth() * 0.028f));
    bounds.reduce (margin, 0);
    bounds.removeFromTop ((int) (getHeight() * 0.015f));

    // Bottom row: LO FILTER, HI FILTER, WIDTH
    auto bottomRow = bounds.removeFromBottom ((int) (getHeight() * 0.165f));
    bounds.removeFromBottom ((int) (getHeight() * 0.015f));

    // Knob row: DELAY (+SYNC), FEEDBACK, MIX — sits directly above the bottom row
    auto topRow = bounds.removeFromBottom ((int) (getHeight() * 0.205f));
    bounds.removeFromBottom ((int) (getHeight() * 0.012f));

    // Pipe editor now takes the remaining space at the TOP, just under the title bar
    pipeEditor.setBounds (bounds);

    // --- DELAY (+SYNC), FEEDBACK, MIX ---
    int knobSize = (int) (getWidth() * 0.15f);
    int colW = topRow.getWidth() / 3;

    auto delayCol    = topRow.removeFromLeft (colW);
    auto feedbackCol = topRow.removeFromLeft (colW);
    auto mixCol      = topRow;

    auto delayRemainder = layoutKnobWithLabel (delayCol, delaySlider, delayLabel, knobSize);
    auto syncArea = delayRemainder.withSizeKeepingCentre (juce::jmin (delayCol.getWidth() - 20, 110), 22);
    syncButton.setBounds (syncArea);

    layoutKnobWithLabel (feedbackCol, feedbackSlider, feedbackLabel, knobSize);
    layoutKnobWithLabel (mixCol, mixSlider, mixLabel, knobSize);

    // --- LO FILTER, HI FILTER, WIDTH ---
    int knobSize2 = (int) (getWidth() * 0.14f);
    int colW2 = bottomRow.getWidth() / 3;

    auto loCol    = bottomRow.removeFromLeft (colW2);
    auto hiCol    = bottomRow.removeFromLeft (colW2);
    auto widthCol = bottomRow;

    layoutKnobWithLabel (loCol, loFilterSlider, loFilterLabel, knobSize2);
    layoutKnobWithLabel (hiCol, hiFilterSlider, hiFilterLabel, knobSize2);
    layoutKnobWithLabel (widthCol, widthSlider, widthLabel, knobSize2);
}
