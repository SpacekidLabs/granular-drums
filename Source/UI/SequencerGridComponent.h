#pragma once

#include <JuceHeader.h>
#include "../PluginProcessor.h"

class SequencerGridComponent : public juce::Component, public juce::Timer
{
public:
    SequencerGridComponent (GranularDrumsProcessor&);
    ~SequencerGridComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;
    
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;

private:
    void handleMouseGridInteraction (const juce::Point<int>& pos, bool isDragState);
    juce::Colour getCategoryColour (const juce::String& category) const;

    GranularDrumsProcessor& processor;

    juce::TextButton playButton;
    juce::Slider bpmSlider;
    juce::Label bpmLabel;
    juce::TextButton clearButton;
    juce::TextButton randomizeButton;
    juce::Label syncLabel;

    // Grid coordinates
    int gridX = 100;
    int gridY = 50;
    int gridW = 500;
    int gridH = 300;
    int cellW = 30;
    int cellH = 18;
    int gapX = 4;
    int gapY = 4;

    int lastClickedPad = -1;
    int lastClickedStep = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SequencerGridComponent)
};
