#pragma once

#include <JuceHeader.h>

class GranularDrumsLookAndFeel : public juce::LookAndFeel_V4
{
public:
    GranularDrumsLookAndFeel();
    ~GranularDrumsLookAndFeel() override = default;

    // Custom drawing methods for dials, buttons, etc.
    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider&) override;
                           
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
                               
    void drawButtonText (juce::Graphics&, juce::TextButton&,
                         bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
};
