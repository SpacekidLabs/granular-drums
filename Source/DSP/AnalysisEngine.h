#pragma once

#include <JuceHeader.h>

#include "HPSSProcessor.h"
#include "OnsetVotingEngine.h"

class AnalysisEngine : public juce::Thread
{
public:
    AnalysisEngine();
    ~AnalysisEngine() override;

    void run() override;

    void startAnalysis (const juce::File& audioFile);
    bool isAnalyzing() const;
    float getProgress() const;

    std::function<void(const std::vector<OnsetMarker>&, const juce::AudioBuffer<float>&)> onAnalysisFinished;

private:
    std::atomic<bool> analyzing { false };
    std::atomic<float> progress { 0.0f };
    juce::File currentFile;

    juce::AudioFormatManager formatManager;
    juce::AudioBuffer<float> sourceBuffer;
    
    HPSSProcessor hpssProcessor;
    OnsetVotingEngine onsetEngine;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AnalysisEngine)
};
