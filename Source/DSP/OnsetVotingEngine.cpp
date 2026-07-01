#include "OnsetVotingEngine.h"

OnsetVotingEngine::OnsetVotingEngine()
{
}

OnsetVotingEngine::~OnsetVotingEngine()
{
}

std::vector<OnsetMarker> OnsetVotingEngine::detectOnsets (const juce::AudioBuffer<float>& percussiveBuffer, double sampleRate)
{
    std::vector<OnsetMarker> markers;
    if (percussiveBuffer.getNumSamples() == 0)
        return markers;

    const float* data = percussiveBuffer.getReadPointer (0);
    int numSamples = percussiveBuffer.getNumSamples();
    
    int hopSize = 256; // ~5.8ms at 44.1kHz
    std::vector<float> energies;
    energies.reserve ((size_t)(numSamples / hopSize) + 1);
    
    for (int i = 0; i < numSamples - hopSize; i += hopSize)
    {
        float energy = 0.0f;
        for (int j = 0; j < hopSize; ++j)
        {
            float s = data[i + j];
            energy += s * s;
        }
        energies.push_back (energy);
    }
    
    if (energies.empty()) return markers;
    
    // Lockout window of 40ms to prevent double-triggering
    int minSkipHops = juce::roundToInt (sampleRate * 0.04 / hopSize);
    if (minSkipHops < 1) minSkipHops = 1;
    
    int hopsSinceLastOnset = minSkipHops;
    for (size_t i = 1; i < energies.size(); ++i)
    {
        hopsSinceLastOnset++;
        
        // Calculate local average energy over a sliding window of 11 hops
        float localSum = 0.0f;
        int count = 0;
        for (int k = -5; k <= 5; ++k)
        {
            int idx = (int)i + k;
            if (idx >= 0 && idx < (int)energies.size())
            {
                localSum += energies[(size_t)idx];
                count++;
            }
        }
        float localMean = count > 0 ? localSum / (float)count : 0.0f;
        
        // Adaptive threshold scaling with local energy to track transient dynamics in both quiet and loud sections
        float adaptiveThreshold = localMean * 0.12f + 0.006f;
        
        float energyDiff = energies[i] - energies[i - 1];
        
        // Trigger if energy difference is positive and exceeds the local adaptive threshold
        if (energyDiff > adaptiveThreshold && hopsSinceLastOnset >= minSkipHops)
        {
            int candidateIdx = (int)i * hopSize;
            
            // Search window: 15ms forward to find transient peak
            int searchLen = juce::roundToInt (sampleRate * 0.015);
            int peakIdx = candidateIdx;
            float maxVal = 0.0f;
            
            for (int k = 0; k < searchLen; ++k)
            {
                int checkIdx = candidateIdx + k;
                if (checkIdx >= numSamples) break;
                
                float absVal = std::abs (data[checkIdx]);
                if (absVal > maxVal)
                {
                    maxVal = absVal;
                    peakIdx = checkIdx;
                }
            }
            
            // Align marker to peak with a tiny 1.5ms pre-onset padding (approx. 66 samples at 44.1kHz)
            int padding = juce::roundToInt (sampleRate * 0.0015);
            int sampleIdx = peakIdx - padding;
            if (sampleIdx < 0) sampleIdx = 0;
            if (sampleIdx >= numSamples) sampleIdx = numSamples - 1;
            
            markers.push_back ({ sampleIdx, energyDiff });
            hopsSinceLastOnset = 0;
        }
    }
    
    return markers;
}
