#include "HPSSProcessor.h"

HPSSProcessor::HPSSProcessor()
{
}

HPSSProcessor::~HPSSProcessor()
{
}

std::pair<juce::AudioBuffer<float>, juce::AudioBuffer<float>> HPSSProcessor::process (const juce::AudioBuffer<float>& inputBuffer, int fftSize, int hopSize)
{
    juce::ignoreUnused (fftSize, hopSize);
    
    // Create empty buffers for now
    juce::AudioBuffer<float> harmonic (inputBuffer.getNumChannels(), inputBuffer.getNumSamples());
    juce::AudioBuffer<float> percussive (inputBuffer.getNumChannels(), inputBuffer.getNumSamples());
    
    harmonic.clear();
    percussive.clear();
    
    // Basic fallback: just copy input to percussive for testing
    for (int ch = 0; ch < inputBuffer.getNumChannels(); ++ch)
    {
        percussive.copyFrom (ch, 0, inputBuffer, ch, 0, inputBuffer.getNumSamples());
    }

    // TODO: Implement actual median-filter based HPSS using juce::dsp::FFT

    return { std::move (harmonic), std::move (percussive) };
}
