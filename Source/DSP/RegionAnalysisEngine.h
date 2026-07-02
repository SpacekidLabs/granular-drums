#pragma once

#include <JuceHeader.h>
#include <vector>

struct RegionMarker
{
    int sampleIndex = 0;
    int lengthInSamples = 0;
    float confidence = 0.0f;
    juce::String category = "texture";
};

class RegionAnalysisEngine
{
public:
    RegionAnalysisEngine();
    ~RegionAnalysisEngine();

    std::vector<RegionMarker> findRegions (const juce::AudioBuffer<float>& buffer, double sampleRate);

private:
    struct Candidate
    {
        RegionMarker marker;
        float rms = 0.0f;
        float brightness = 0.0f;
        float lowRatio = 0.0f;
        float midRatio = 0.0f;
        float highRatio = 0.0f;
        float zeroCrossingRate = 0.0f;
        float stereoWidth = 0.0f;
        float variation = 0.0f;
        float timePosition = 0.0f;
    };

    static float featureDistance (const Candidate& a, const Candidate& b);
    static juce::String classifyRegion (const Candidate& candidate);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RegionAnalysisEngine)
};
