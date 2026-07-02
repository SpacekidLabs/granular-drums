#pragma once

#include <JuceHeader.h>
#include "../DSP/RegionAnalysisEngine.h"

class MainWaveformDisplay : public juce::Component, public juce::FileDragAndDropTarget
{
public:
    MainWaveformDisplay();
    ~MainWaveformDisplay() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    // FileDragAndDropTarget overrides
    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void filesDropped (const juce::StringArray& files, int x, int y) override;
    void fileDragEnter (const juce::StringArray& files, int x, int y) override;
    void fileDragExit (const juce::StringArray& files) override;

    void setWaveformData (const juce::AudioBuffer<float>* buffer, const std::vector<RegionMarker>* markers);
    void setAnalysisState (bool analyzing, float progress);
    void setSelectionRatios (float start, float end);
    float getSelectionStartRatio() const { return selectionStartRatio; }
    float getSelectionEndRatio() const { return selectionEndRatio; }

    // Callback when a file is dropped
    std::function<void(const juce::File&)> onFileDropped;
    std::function<void(float, float)> onSelectionFinished;

    // Mouse overrides for selection dragging
    void mouseMove (const juce::MouseEvent& event) override;
    void mouseDown (const juce::MouseEvent& event) override;
    void mouseDrag (const juce::MouseEvent& event) override;
    void mouseUp (const juce::MouseEvent& event) override;

private:
    juce::String currentMessage { "DROP ANY SOUND HERE" };
    bool isHovering = false;
    bool isAnalyzing = false;
    float analysisProgress = 0.0f;
    
    float selectionStartRatio = 0.0f;
    float selectionEndRatio = 1.0f;
    bool isDraggingStart = false;
    bool isDraggingEnd = false;
    
    const juce::AudioBuffer<float>* audioBuffer = nullptr;
    const std::vector<RegionMarker>* regionMarkers = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWaveformDisplay)
};
