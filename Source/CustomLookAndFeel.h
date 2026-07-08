#pragma once
#include <JuceHeader.h>

namespace HadenianColours
{
    static const juce::Colour panelBg        { 0xffEDE9E1 }; // dirty white chassis
    static const juce::Colour panelBgDarker  { 0xffE2DDD2 };
    static const juce::Colour panelBorder    { 0xffC9C2B3 };
    static const juce::Colour sectionDivider { 0xffD3CDBF };

    static const juce::Colour knobBody       { 0xff2E2C28 }; // near-black knob, dirty-white chassis
    static const juce::Colour knobBodyLight  { 0xff44413B };
    static const juce::Colour knobRim        { 0xff171613 };
    static const juce::Colour knobPointer    { 0xffEDE9E1 };

    static const juce::Colour textDark       { 0xff2A2823 };
    static const juce::Colour textMuted      { 0xff716B5C };

    static const juce::Colour displayBg      { 0xffDFDACD };
    static const juce::Colour displayText    { 0xff2A2823 };
    static const juce::Colour displayBorder  { 0xffB9B19E };

    static const juce::Colour accentBlue     { 0xff3E6FA6 };
    static const juce::Colour accentBlueOff  { 0xffB9B19E };

    static const juce::Colour gridLine       { 0xffD3CDBF };
    static const juce::Colour pipeGreen      { 0xff5FA35A };
    static const juce::Colour pipeYellow     { 0xffD9B23C };
    static const juce::Colour pipeRed        { 0xffC0453B };
}

class HadenianLookAndFeel : public juce::LookAndFeel_V4
{
public:
    HadenianLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                            float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle,
                            juce::Slider&) override;

    void drawToggleButton (juce::Graphics&, juce::ToggleButton&,
                            bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    juce::Font getLabelFont (juce::Label&) override;
};
