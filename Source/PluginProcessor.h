#pragma once

#include <JuceHeader.h>
#include "DSP/AnalysisEngine.h"
#include "DSP/PadPlaybackEngine.h"
#include "DSP/MackityProcessor.h"

class GranularDrumsProcessor  : public juce::AudioProcessor
{
public:
    GranularDrumsProcessor();
    ~GranularDrumsProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    void startAnalysis (const juce::File& file);
    void triggerPad (int padIndex);

    const juce::AudioBuffer<float>& getCurrentBuffer() const { return currentBuffer; }
    const std::vector<OnsetMarker>& getCurrentMarkers() const { return currentMarkers; }
    const std::vector<OnsetMarker>& getActiveMarkers() const { return activeMarkers; }
    int getMarkerIndexForPad (int padIndex) const { return padMarkerIndices[padIndex]; }

    void updateActiveSlices (bool isNewFileLoaded = false);
    void randomizeAllPads();
    void randomizePad (int padIndex);
    bool isPadPlaying (int padIndex) const { return playbackEngine.isPadPlaying (padIndex); }

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }
    
    bool isAnalyzing() const { return analysisEngine.isAnalyzing(); }
    float getAnalysisProgress() const { return analysisEngine.getProgress(); }

    // Sequencer API
    bool getSequencerStep (int pad, int step) const { return sequencerSteps[pad][step]; }
    void setSequencerStep (int pad, int step, bool state) { sequencerSteps[pad][step] = state; }
    bool isSequencerPlaying() const { return sequencerPlaying; }
    void setSequencerPlaying (bool play) { sequencerPlaying = play; if (play) { internalSequencerTimer = 0.0; internalSequencerStep = 0; } }
    double getSequencerBpm() const { return sequencerBpm; }
    void setSequencerBpm (double bpm) { sequencerBpm = bpm; }
    int getCurrentActiveStep() const { return currentActiveStep.load(); }
    bool isSyncedToHost() const { return isSyncedToHostCached.load(); }
    double getHostBpm() const { return hostBPMCached.load(); }
    
    void clearPattern()
    {
        for (int p = 0; p < 16; ++p)
            std::fill_n (sequencerSteps[p], 16, false);
    }
    
    void randomizePattern()
    {
        auto& r = juce::Random::getSystemRandom();
        for (int p = 0; p < 16; ++p)
        {
            double prob = 0.15;
            if (p < 4) prob = 0.3;      // Kicks
            else if (p < 8) prob = 0.2; // Snares
            else if (p < 12) prob = 0.5;// Hats
            
            for (int s = 0; s < 16; ++s)
            {
                if (p < 4 && s % 4 == 0) sequencerSteps[p][s] = (r.nextDouble() < 0.6);
                else if (p >= 4 && p < 8 && s % 8 == 4) sequencerSteps[p][s] = (r.nextDouble() < 0.7);
                else sequencerSteps[p][s] = (r.nextDouble() < prob);
            }
        }
    }

private:
    void _assignSliceToPad (int padIndex);
    void _restoreSampleState (const juce::ValueTree& state);
    void _writeSampleState (juce::ValueTree& state) const;
    bool _loadAudioFileIntoBuffer (const juce::File& file, juce::AudioBuffer<float>& destination, double& sampleRateOut) const;
    bool _restoreEmbeddedAudio (const juce::ValueTree& sampleTree);
    void _restorePadAssignments();

    juce::AudioProcessorValueTreeState apvts;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    AnalysisEngine analysisEngine;
    PadPlaybackEngine playbackEngine;
    MackityProcessor mackityProcessor;
    
    juce::AudioBuffer<float> currentBuffer;
    std::vector<OnsetMarker> currentMarkers;
    std::vector<OnsetMarker> activeMarkers;
    int padMarkerIndices[16] = {0};
    juce::String currentSamplePath;
    double currentSampleRate = 44100.0;

    // Sequencer state and tracking
    bool sequencerSteps[16][16];
    bool sequencerPlaying = false;
    double sequencerBpm = 120.0;
    std::atomic<int> currentActiveStep {0};
    double internalSequencerTimer = 0.0;
    int internalSequencerStep = 0;
    double lastPpqPosition = 0.0;

    std::atomic<bool> isSyncedToHostCached { false };
    std::atomic<double> hostBPMCached { 120.0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GranularDrumsProcessor)
};
