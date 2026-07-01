#pragma once

#include <JuceHeader.h>
#include <vector>

class HPSSProcessor
{
public:
    HPSSProcessor();
    ~HPSSProcessor();

    // Perform HPSS on the input buffer.
    // Returns a pair of buffers: {Harmonic, Percussive}
    std::pair<juce::AudioBuffer<float>, juce::AudioBuffer<float>> process (const juce::AudioBuffer<float>& inputBuffer, int fftSize = 1024, int hopSize = 256);

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HPSSProcessor)
};
