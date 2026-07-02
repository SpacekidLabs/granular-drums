#pragma once

#include <JuceHeader.h>

#include "RegionAnalysisEngine.h"

class AnalysisEngine : public juce::Thread
{
public:
    AnalysisEngine();
    ~AnalysisEngine() override;

    void run() override;

    void startAnalysis (const juce::File& audioFile);
    bool isAnalyzing() const;
    float getProgress() const;

    std::function<void(const std::vector<RegionMarker>&,
                       const juce::AudioBuffer<float>&,
                       const juce::File&,
                       double)> onAnalysisFinished;

private:
    std::atomic<bool> analyzing { false };
    std::atomic<float> progress { 0.0f };
    juce::File currentFile;

    juce::AudioFormatManager formatManager;
    juce::AudioBuffer<float> sourceBuffer;
    
    RegionAnalysisEngine regionEngine;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AnalysisEngine)
};
