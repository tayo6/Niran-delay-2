#include "CustomLookAndFeel.h"

HadenianLookAndFeel::HadenianLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, HadenianColours::displayText);
    setColour (juce::Slider::textBoxBackgroundColourId, HadenianColours::displayBg);
    setColour (juce::Slider::textBoxOutlineColourId, HadenianColours::displayBorder);
    setColour (juce::Label::textColourId, HadenianColours::textDark);
    setColour (juce::ToggleButton::textColourId, HadenianColours::textDark);
}

void HadenianLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                             float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                                             juce::Slider&)
{
    auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (6.0f);
    auto diameter = juce::jmin (bounds.getWidth(), bounds.getHeight());
    auto radius = diameter * 0.5f;
    auto centre = bounds.getCentre();

    const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    // Thin outer track ring (unlit)
    const float ringRadius = radius * 0.96f;
    const float ringThickness = juce::jmax (3.0f, radius * 0.10f);

    juce::Path track;
    track.addCentredArc (centre.x, centre.y, ringRadius, ringRadius, 0.0f,
                          rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (HadenianColours::panelBorder);
    g.strokePath (track, juce::PathStrokeType (ringThickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Lit value ring
    juce::Path valueTrack;
    valueTrack.addCentredArc (centre.x, centre.y, ringRadius, ringRadius, 0.0f,
                               rotaryStartAngle, angle, true);
    g.setColour (HadenianColours::accentBlue);
    g.strokePath (valueTrack, juce::PathStrokeType (ringThickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Flat matte knob disc (no gradient/bezel, mirroring bloom's flat dial face)
    const float discRadius = radius * 0.80f;
    auto discRect = juce::Rectangle<float> (discRadius * 2.0f, discRadius * 2.0f).withCentre (centre);
    g.setColour (HadenianColours::knobBody);
    g.fillEllipse (discRect);

    // Pointer: a simple light line from just outside centre to near the rim
    const float pointerInner = discRadius * 0.16f;
    const float pointerOuter = discRadius * 0.88f;
    const float pointerThickness = juce::jmax (2.2f, discRadius * 0.10f);

    juce::Path pointer;
    pointer.startNewSubPath (0.0f, -pointerInner);
    pointer.lineTo (0.0f, -pointerOuter);

    g.setColour (HadenianColours::knobPointer);
    g.strokePath (pointer,
                  juce::PathStrokeType (pointerThickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded),
                  juce::AffineTransform::rotation (angle).translated (centre));
}

void HadenianLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                                             bool shouldDrawButtonAsHighlighted, bool)
{
    auto bounds = button.getLocalBounds().toFloat().reduced (1.0f);
    auto cornerSize = bounds.getHeight() * 0.28f;

    g.setColour (HadenianColours::displayBg.darker (shouldDrawButtonAsHighlighted ? 0.05f : 0.0f));
    g.fillRoundedRectangle (bounds, cornerSize);
    g.setColour (HadenianColours::displayBorder);
    g.drawRoundedRectangle (bounds, cornerSize, 1.4f);

    auto ledDiameter = bounds.getHeight() * 0.42f;
    auto ledBounds = juce::Rectangle<float> (ledDiameter, ledDiameter)
                        .withCentre ({ bounds.getY() + bounds.getHeight() * 0.5f + bounds.getX() + ledDiameter * 0.7f,
                                       bounds.getCentreY() });
    ledBounds.setX (bounds.getX() + bounds.getHeight() * 0.32f);

    g.setColour (button.getToggleState() ? HadenianColours::accentBlue : HadenianColours::accentBlueOff);
    g.fillEllipse (ledBounds);

    g.setColour (HadenianColours::textDark);
    g.setFont (juce::Font (bounds.getHeight() * 0.5f, juce::Font::bold));
    auto textArea = bounds.withTrimmedLeft (bounds.getHeight() * 0.9f);
    g.drawText (button.getButtonText(), textArea, juce::Justification::centredLeft);
}

juce::Font HadenianLookAndFeel::getLabelFont (juce::Label& label)
{
    return juce::Font (juce::jmax (11.0f, label.getHeight() * 0.7f));
}
