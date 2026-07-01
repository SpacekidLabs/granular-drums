#pragma once

#include <JuceHeader.h>
#include <vector>

struct OnsetMarker
{
    int sampleIndex;
    float confidence;
    juce::String category = "snare";
};

class OnsetVotingEngine
{
public:
    OnsetVotingEngine();
    ~OnsetVotingEngine();

    // Detects onsets in the given percussive buffer and returns a sorted list of markers
    std::vector<OnsetMarker> detectOnsets (const juce::AudioBuffer<float>& percussiveBuffer, double sampleRate);

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OnsetVotingEngine)
};
