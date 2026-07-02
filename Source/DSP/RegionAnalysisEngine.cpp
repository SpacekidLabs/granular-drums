#include "RegionAnalysisEngine.h"

#include <cmath>
#include <limits>

RegionAnalysisEngine::RegionAnalysisEngine() = default;
RegionAnalysisEngine::~RegionAnalysisEngine() = default;

std::vector<RegionMarker> RegionAnalysisEngine::findRegions (const juce::AudioBuffer<float>& buffer, double sampleRate)
{
    std::vector<RegionMarker> selectedRegions;
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();
    if (numSamples <= 0 || numChannels <= 0)
        return selectedRegions;

    if (sampleRate <= 0.0)
        sampleRate = 44100.0;

    const int preferredWindowSize = juce::roundToInt (sampleRate * 0.75);
    const int windowSize = juce::jlimit (1, numSamples, preferredWindowSize);
    const int hopSize = juce::jmax (512, windowSize / 3);
    const int scanStep = juce::jmax (1, windowSize / 700);

    std::vector<Candidate> candidates;
    candidates.reserve ((size_t) (numSamples / hopSize + 1));

    for (int start = 0; start + windowSize <= numSamples; start += hopSize)
    {
        float sumSquares = 0.0f;
        float sumAbs = 0.0f;
        float diffAbs = 0.0f;
        float zeroCrossings = 0.0f;
        float lowEnergy = 0.0f;
        float midEnergy = 0.0f;
        float highEnergy = 0.0f;
        float sideEnergy = 0.0f;
        float midStereoEnergy = 0.0f;
        float previous = 0.0f;
        bool hasPrevious = false;

        float lpState = 0.0f;
        float lpHPState = 0.0f;
        constexpr float alphaLP = 0.021f;
        constexpr float alphaHP = 0.57f;

        int scanned = 0;
        for (int offset = 0; offset < windowSize; offset += scanStep)
        {
            const int idx = start + offset;
            float mono = buffer.getSample (0, idx);
            float right = mono;
            if (numChannels > 1)
            {
                right = buffer.getSample (1, idx);
                mono = (mono + right) * 0.5f;
            }

            sumSquares += mono * mono;
            sumAbs += std::abs (mono);

            if (hasPrevious)
            {
                diffAbs += std::abs (mono - previous);
                if ((mono >= 0.0f) != (previous >= 0.0f))
                    zeroCrossings += 1.0f;
            }
            previous = mono;
            hasPrevious = true;

            lpState = lpState + alphaLP * (mono - lpState);
            lowEnergy += std::abs (lpState);

            lpHPState = lpHPState + alphaHP * (mono - lpHPState);
            const float hpVal = mono - lpHPState;
            highEnergy += std::abs (hpVal);
            midEnergy += std::abs (mono - lpState - hpVal);

            if (numChannels > 1)
            {
                const float mid = (buffer.getSample (0, idx) + right) * 0.5f;
                const float side = (buffer.getSample (0, idx) - right) * 0.5f;
                midStereoEnergy += std::abs (mid);
                sideEnergy += std::abs (side);
            }

            ++scanned;
        }

        if (scanned <= 0)
            continue;

        Candidate candidate;
        candidate.rms = std::sqrt (sumSquares / (float) scanned);
        if (candidate.rms < 0.002f)
            continue;

        const float spectralSum = lowEnergy + midEnergy + highEnergy + 0.0001f;
        candidate.lowRatio = lowEnergy / spectralSum;
        candidate.midRatio = midEnergy / spectralSum;
        candidate.highRatio = highEnergy / spectralSum;
        candidate.brightness = candidate.highRatio;
        candidate.zeroCrossingRate = zeroCrossings / (float) scanned;
        candidate.stereoWidth = numChannels > 1 ? sideEnergy / (midStereoEnergy + sideEnergy + 0.0001f) : 0.0f;
        candidate.variation = diffAbs / (sumAbs + 0.0001f);
        candidate.timePosition = (float) start / (float) juce::jmax (1, numSamples - windowSize);
        candidate.marker.sampleIndex = start;
        candidate.marker.lengthInSamples = windowSize;
        candidate.marker.confidence = candidate.rms + candidate.variation * 0.5f + candidate.stereoWidth * 0.25f;
        candidate.marker.category = classifyRegion (candidate);

        candidates.push_back (candidate);
    }

    if (candidates.empty())
    {
        selectedRegions.push_back ({ 0, juce::jmin (numSamples, windowSize), 1.0f, "texture" });
        return selectedRegions;
    }

    std::sort (candidates.begin(), candidates.end(), [] (const Candidate& a, const Candidate& b)
    {
        return a.marker.confidence > b.marker.confidence;
    });

    std::vector<Candidate> selected;
    selected.reserve (16);
    selected.push_back (candidates.front());

    while (selected.size() < 16 && selected.size() < candidates.size())
    {
        int bestIndex = -1;
        float bestScore = -1.0f;

        for (size_t i = 0; i < candidates.size(); ++i)
        {
            const auto& candidate = candidates[i];
            bool alreadySelected = false;
            float minDistance = std::numeric_limits<float>::max();

            for (const auto& chosen : selected)
            {
                if (candidate.marker.sampleIndex == chosen.marker.sampleIndex)
                {
                    alreadySelected = true;
                    break;
                }

                minDistance = juce::jmin (minDistance, featureDistance (candidate, chosen));
            }

            if (alreadySelected)
                continue;

            const float score = minDistance * 0.72f + candidate.marker.confidence * 0.28f;
            if (score > bestScore)
            {
                bestScore = score;
                bestIndex = (int) i;
            }
        }

        if (bestIndex < 0)
            break;

        selected.push_back (candidates[(size_t) bestIndex]);
    }

    std::sort (selected.begin(), selected.end(), [] (const Candidate& a, const Candidate& b)
    {
        return a.marker.sampleIndex < b.marker.sampleIndex;
    });

    selectedRegions.reserve (selected.size());
    for (const auto& candidate : selected)
        selectedRegions.push_back (candidate.marker);

    return selectedRegions;
}

float RegionAnalysisEngine::featureDistance (const Candidate& a, const Candidate& b)
{
    const float loudness = std::abs (a.rms - b.rms) * 3.0f;
    const float brightness = std::abs (a.brightness - b.brightness);
    const float low = std::abs (a.lowRatio - b.lowRatio);
    const float mid = std::abs (a.midRatio - b.midRatio);
    const float high = std::abs (a.highRatio - b.highRatio);
    const float noise = std::abs (a.zeroCrossingRate - b.zeroCrossingRate) * 2.0f;
    const float width = std::abs (a.stereoWidth - b.stereoWidth);
    const float variation = std::abs (a.variation - b.variation);
    const float time = std::abs (a.timePosition - b.timePosition) * 0.35f;

    return loudness + brightness + low + mid + high + noise + width + variation + time;
}

juce::String RegionAnalysisEngine::classifyRegion (const Candidate& candidate)
{
    if (candidate.lowRatio > 0.50f && candidate.lowRatio > candidate.highRatio * 1.4f)
        return "body";
    if (candidate.highRatio > 0.48f || candidate.zeroCrossingRate > 0.25f)
        return "noise";
    if (candidate.variation > 0.22f || candidate.stereoWidth > 0.28f)
        return "motion";
    return "grit";
}
