#include "GranularDrumsLookAndFeel.h"

GranularDrumsLookAndFeel::GranularDrumsLookAndFeel()
{
    // Flat matte very dark grey
    setColour (juce::ResizableWindow::backgroundColourId, juce::Colour::fromString("#FF121212"));
    
    // Stark off-white / bone text
    setColour (juce::Label::textColourId, juce::Colour::fromString("#FFEFEBE0"));
    
    // Slider colours
    setColour (juce::Slider::thumbColourId, juce::Colour::fromString("#FFEFEBE0"));
    setColour (juce::Slider::rotarySliderFillColourId, juce::Colour::fromString("#FFEFEBE0"));
    setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour::fromString("#FF2A2A2A"));
    
    // Button colours
    setColour (juce::TextButton::buttonColourId, juce::Colour::fromString("#FF252525"));
    setColour (juce::TextButton::buttonOnColourId, juce::Colour::fromString("#FFEFEBE0"));
    setColour (juce::TextButton::textColourOffId, juce::Colour::fromString("#FFEFEBE0"));
    setColour (juce::TextButton::textColourOnId, juce::Colour::fromString("#FF121212"));
}

void GranularDrumsLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                             float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle,
                                             juce::Slider& slider)
{
    auto radius = juce::jmin (width / 2.0f, height / 2.0f) - 3.5f;
    auto centreX = (float) x + (float) width * 0.5f;
    auto centreY = (float) y + (float) height * 0.5f;
    auto angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

    // Draw the background track (3/4 circular track)
    juce::Path backgroundTrack;
    backgroundTrack.addCentredArc (centreX, centreY, radius, radius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (juce::Colour::fromString ("#FF222222"));
    g.strokePath (backgroundTrack, juce::PathStrokeType (2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Draw the active arc track
    if (sliderPosProportional > 0.0f)
    {
        juce::Path activeArc;
        activeArc.addCentredArc (centreX, centreY, radius, radius, 0.0f, rotaryStartAngle, angle, true);
        g.setColour (slider.findColour (juce::Slider::rotarySliderFillColourId));
        g.strokePath (activeArc, juce::PathStrokeType (2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // Draw a tiny glowing dot at the center of the dial
    g.setColour (slider.findColour (juce::Slider::rotarySliderFillColourId).withAlpha (0.4f));
    g.fillEllipse (centreX - 1.5f, centreY - 1.5f, 3.0f, 3.0f);

    // Pointer dot at the tip of the active arc
    float dotRadius = 1.5f;
    float dotX = centreX + radius * std::sin (angle);
    float dotY = centreY - radius * std::cos (angle);
    g.setColour (slider.findColour (juce::Slider::thumbColourId));
    g.fillEllipse (dotX - dotRadius, dotY - dotRadius, dotRadius * 2.0f, dotRadius * 2.0f);
}

void GranularDrumsLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour,
                                                 bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat();
    
    juce::Colour baseColour = backgroundColour;
    if (shouldDrawButtonAsDown)
    {
        baseColour = juce::Colour::fromString("#FFEFEBE0");
    }
    else if (shouldDrawButtonAsHighlighted)
    {
        baseColour = baseColour.brighter (0.15f);
    }
        
    g.setColour (baseColour);
    g.fillRect (bounds); // Sharp rectangles
    
    juce::Colour borderColour = juce::Colour::fromString("#FF444444");
    if (shouldDrawButtonAsHighlighted)
        borderColour = juce::Colour::fromString("#FFA0A0A0");
    else if (shouldDrawButtonAsDown)
        borderColour = juce::Colour::fromString("#FFEFEBE0");
        
    g.setColour (borderColour);
    g.drawRect (bounds, 1.0f);
}

void GranularDrumsLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& button,
                                           bool /*shouldDrawButtonAsHighlighted*/, bool /*shouldDrawButtonAsDown*/)
{
    float fontSize = juce::jmin (14.0f, (float)button.getHeight() * 0.6f);
    if (fontSize < 8.0f) fontSize = 8.0f;
    
    g.setFont (juce::FontOptions (fontSize).withStyle ("Bold"));
    g.setColour (button.findColour (button.getToggleState() ? juce::TextButton::textColourOnId : juce::TextButton::textColourOffId));
    
    auto cornerSize = juce::jmin (button.getWidth(), button.getHeight()) / 6;
    g.drawText (button.getButtonText(),
                button.getLocalBounds().reduced (cornerSize, 0),
                juce::Justification::centred, false);
}

