#pragma once

#include <JuceHeader.h>
#include "GranularSynth.h"

class PadPlaybackEngine
{
public:
    PadPlaybackEngine();
    ~PadPlaybackEngine();

    void prepareToPlay (double sampleRate, int samplesPerBlock);
    void releaseResources();
    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages);

    void triggerPad (int padIndex, float velocity);
    bool isPadPlaying (int padIndex) const;

    // Clears all currently loaded slices
    void clearPads();

    // Assign an audio slice to a pad
    void setPadSlice (int padIndex, const juce::AudioBuffer<float>& sourceBuffer, int startSample, int numSamples, std::atomic<float>* pitchParam, std::atomic<float>* decayParam, std::atomic<float>* monoParam, const juce::String& category, std::atomic<float>* globalPitchParam, std::atomic<float>* globalDecayParam, std::atomic<float>* globalGrainSizeParam, std::atomic<float>* globalGrainSprayParam, std::atomic<float>* globalFilterResParam, std::atomic<float>* globalReverseParam, std::atomic<float>* globalAnalogDriftParam, std::atomic<float>* globalGrainSizeJitterParam, std::atomic<float>* globalGrainPitchJitterParam, std::atomic<float>* globalPanJitterParam, std::atomic<float>* globalFilterSweepTimeParam, std::atomic<float>* globalPitchSweepDepthParam, std::atomic<float>* globalLayerBalanceParam, std::atomic<float>* globalLayerDelayParam, std::atomic<float>* globalLayerDetuneParam, std::atomic<float>* globalResonatorMixParam, std::atomic<float>* globalResonatorPitchParam, std::atomic<float>* globalResonatorFeedbackParam, std::atomic<float>* globalLfoRateParam, std::atomic<float>* globalLfoShapeParam, std::atomic<float>* modSrc1Param, std::atomic<float>* modDst1Param, std::atomic<float>* modDepth1Param, std::atomic<float>* modSrc2Param, std::atomic<float>* modDst2Param, std::atomic<float>* modDepth2Param, std::atomic<float>* modSrc3Param, std::atomic<float>* modDst3Param, std::atomic<float>* modDepth3Param, std::atomic<float>* modSrc4Param, std::atomic<float>* modDst4Param, std::atomic<float>* modDepth4Param);

private:
    juce::Synthesiser synth;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PadPlaybackEngine)
};
