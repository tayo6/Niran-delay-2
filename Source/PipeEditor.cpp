#include "PipeEditor.h"
#include "CustomLookAndFeel.h"

PipeEditor::PipeEditor (HadenianMonoDelayAudioProcessor& processorRef)
    : processor (processorRef)
{
    setInterceptsMouseClicks (true, false);
    startTimerHz (30);
}

PipeEditor::~PipeEditor() { stopTimer(); }

void PipeEditor::timerCallback()
{
    repaint();
}

juce::Colour PipeEditor::colourForValue (float value0to127)
{
    if (value0to127 < 40.0f)  return HadenianColours::pipeGreen;
    if (value0to127 <= 100.0f) return HadenianColours::pipeYellow;
    return HadenianColours::pipeRed;
}

juce::Rectangle<float> PipeEditor::getGraphBounds() const
{
    auto b = getLocalBounds().toFloat();
    return b.withTrimmedLeft (kLeftAxisWidth)
            .withTrimmedRight (kRightMeterWidth)
            .withTrimmedTop (kTopLabelHeight);
}

juce::Rectangle<float> PipeEditor::getMeterBounds() const
{
    auto b = getLocalBounds().toFloat();
    return b.removeFromRight (kRightMeterWidth).withTrimmedTop (kTopLabelHeight).reduced (10.0f, 4.0f);
}

void PipeEditor::paint (juce::Graphics& g)
{
    auto full = getLocalBounds().toFloat();

    g.setColour (HadenianColours::panelBgDarker);
    g.fillRoundedRectangle (full, 6.0f);
    g.setColour (HadenianColours::panelBorder);
    g.drawRoundedRectangle (full.reduced (0.5f), 6.0f, 1.2f);

    auto graph = getGraphBounds();

    // --- Top labels -------------------------------------------------------
    g.setColour (HadenianColours::textMuted);
    g.setFont (juce::Font (12.0f));
    g.drawText ("1 Bar", juce::Rectangle<float> (kLeftAxisWidth, 2.0f, 80.0f, kTopLabelHeight - 4.0f),
                juce::Justification::centredLeft);

    g.setColour (HadenianColours::textDark);
    g.setFont (juce::Font (13.0f, juce::Font::bold));
    g.drawText ("Feedback Velocity", full.withTrimmedTop (2.0f).withHeight (kTopLabelHeight - 4.0f),
                juce::Justification::centred);

    g.setColour (HadenianColours::textMuted);
    g.setFont (juce::Font (12.0f));
    g.drawText ("127", full.withTrimmedRight (4.0f).withHeight (kTopLabelHeight - 4.0f).withTrimmedTop (2.0f),
                juce::Justification::centredRight);

    // --- Grid + left axis ---------------------------------------------------
    g.setColour (HadenianColours::gridLine);
    for (int i = 0; i <= 8; ++i)
    {
        float fy = graph.getY() + graph.getHeight() * (float) i / 8.0f;
        g.drawHorizontalLine ((int) fy, graph.getX(), graph.getRight());
    }

    const int numPipes = juce::jmax (1, processor.getNumPipes());
    const float slotWidth = graph.getWidth() / (float) numPipes;
    for (int i = 0; i <= numPipes; ++i)
    {
        float fx = graph.getX() + slotWidth * (float) i;
        g.drawVerticalLine ((int) fx, graph.getY(), graph.getBottom());
    }

    auto drawAxisLabel = [&] (float value, const juce::String& text)
    {
        float fy = graph.getBottom() - graph.getHeight() * (value / 127.0f);
        g.setColour (HadenianColours::textMuted);
        g.setFont (juce::Font (11.0f));
        g.drawText (text, juce::Rectangle<float> (0.0f, fy - 8.0f, kLeftAxisWidth - 6.0f, 16.0f),
                    juce::Justification::centredRight);
    };
    drawAxisLabel (127.0f, "127");
    drawAxisLabel (84.0f,  "84");
    drawAxisLabel (40.0f,  "40");
    drawAxisLabel (0.0f,   "0");

    // --- Pipes --------------------------------------------------------------
    const int playingIdx = processor.getCurrentPlayingPipeIndex();

    for (int i = 0; i < numPipes; ++i)
    {
        float value = processor.getPipeValue (i);
        float frac = juce::jlimit (0.0f, 1.0f, value / 127.0f);

        float centreX = graph.getX() + slotWidth * (i + 0.5f);
        float barWidth = juce::jmin (18.0f, slotWidth * 0.42f);
        float topY = graph.getBottom() - graph.getHeight() * frac;
        float bottomY = graph.getBottom();

        auto colour = colourForValue (value);
        bool isPlaying = (i == playingIdx);

        juce::Rectangle<float> barRect (centreX - barWidth * 0.5f, topY, barWidth, bottomY - topY);

        g.setColour (isPlaying ? colour.brighter (0.25f) : colour);
        g.fillRoundedRectangle (barRect, barWidth * 0.4f);

        // Draggable cap
        float capDiameter = barWidth * 1.35f;
        juce::Rectangle<float> cap (centreX - capDiameter * 0.5f, topY - capDiameter * 0.5f, capDiameter, capDiameter);
        g.setColour (HadenianColours::panelBg);
        g.fillEllipse (cap);
        g.setColour (colour.darker (0.2f));
        g.drawEllipse (cap, 1.6f);
    }

    // --- VU meter -------------------------------------------------------------
    auto meter = getMeterBounds();

    juce::ColourGradient meterGrad (HadenianColours::pipeRed, meter.getX(), meter.getY(),
                                     HadenianColours::pipeGreen, meter.getX(), meter.getBottom(), false);
    meterGrad.addColour (0.35, HadenianColours::pipeYellow);
    g.setGradientFill (meterGrad);
    g.fillRoundedRectangle (meter.removeFromLeft (meter.getWidth() * 0.4f), 3.0f);

    static const std::array<const char*, 8> dbLabels { "0", "-6", "-12", "-18", "-24", "-30", "-36", "-\u221e" };
    g.setColour (HadenianColours::textMuted);
    g.setFont (juce::Font (10.5f));
    for (size_t i = 0; i < dbLabels.size(); ++i)
    {
        float fy = meter.getY() + meter.getHeight() * (float) i / (float) (dbLabels.size() - 1);
        g.drawText (dbLabels[i], juce::Rectangle<float> (meter.getX() + 4.0f, fy - 7.0f, meter.getWidth(), 14.0f),
                    juce::Justification::centredLeft);
    }

    // Live level marker
    float level = processor.getCurrentPlayingLevel();
    float markerY = meter.getY() + meter.getHeight() * (1.0f - juce::jlimit (0.0f, 1.0f, level));
    juce::Path marker;
    float mx = meter.getX() - 6.0f;
    marker.addTriangle (mx, markerY, mx - 7.0f, markerY - 5.0f, mx - 7.0f, markerY + 5.0f);
    g.setColour (colourForValue (level * 127.0f));
    g.fillPath (marker);
}

void PipeEditor::resized() {}

void PipeEditor::applyMouseToPipe (const juce::MouseEvent& e)
{
    auto graph = getGraphBounds();
    const int numPipes = juce::jmax (1, processor.getNumPipes());
    const float slotWidth = graph.getWidth() / (float) numPipes;

    int idx = (int) ((e.position.x - graph.getX()) / slotWidth);
    idx = juce::jlimit (0, numPipes - 1, idx);

    float frac = 1.0f - (e.position.y - graph.getY()) / graph.getHeight();
    float value = juce::jlimit (0.0f, 127.0f, frac * 127.0f);

    processor.setPipeValue (idx, value);
    lastDraggedPipe = idx;
    repaint();
}

void PipeEditor::mouseDown (const juce::MouseEvent& e)
{
    applyMouseToPipe (e);
}

void PipeEditor::mouseDrag (const juce::MouseEvent& e)
{
    applyMouseToPipe (e);
}
