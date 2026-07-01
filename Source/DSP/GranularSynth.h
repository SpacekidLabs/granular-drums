#pragma once

#include <JuceHeader.h>
#include <vector>

//==============================================================================
/**
    Holds the audio slice directly in memory.
*/
class GranularSound : public juce::SynthesiserSound
{
public:
    GranularSound (const juce::String& name,
                   const juce::AudioBuffer<float>& sourceBuffer,
                   int startSample,
                   int numSamples,
                   const juce::BigInteger& midiNotes,
                   int midiRootNote,
                   double sweepStartPitch,
                   double pitchSweepLen,
                   double densitySweepLen,
                   double filterSweepLen,
                   double ampDecay,
                   double pitchOffset,
                   std::atomic<float>* pitchParameter,
                   std::atomic<float>* decayParameter,
                   std::atomic<float>* monoParameter,
                   const juce::String& category,
                   std::atomic<float>* globalPitchParameter,
                   std::atomic<float>* globalDecayParameter,
                   std::atomic<float>* globalGrainSizeParameter,
                   std::atomic<float>* globalGrainSprayParameter,
                   std::atomic<float>* globalFilterResParameter,
                   std::atomic<float>* globalReverseParameter,
                   std::atomic<float>* globalAnalogDriftParameter,
                   std::atomic<float>* globalGrainSizeJitterParameter,
                   std::atomic<float>* globalGrainPitchJitterParameter,
                   std::atomic<float>* globalPanJitterParameter,
                   std::atomic<float>* globalFilterSweepTimeParameter,
                   std::atomic<float>* globalPitchSweepDepthParameter,
                   std::atomic<float>* globalLayerBalanceParameter,
                   std::atomic<float>* globalLayerDelayParameter,
                   std::atomic<float>* globalLayerDetuneParameter,
                   std::atomic<float>* globalResonatorMixParameter,
                   std::atomic<float>* globalResonatorPitchParameter,
                   std::atomic<float>* globalResonatorFeedbackParameter,
                   std::atomic<float>* globalLfoRateParam,
                   std::atomic<float>* globalLfoShapeParam,
                   std::atomic<float>* modSrc1Param,
                   std::atomic<float>* modDst1Param,
                   std::atomic<float>* modDepth1Param,
                   std::atomic<float>* modSrc2Param,
                   std::atomic<float>* modDst2Param,
                   std::atomic<float>* modDepth2Param,
                   std::atomic<float>* modSrc3Param,
                   std::atomic<float>* modDst3Param,
                   std::atomic<float>* modDepth3Param,
                   std::atomic<float>* modSrc4Param,
                   std::atomic<float>* modDst4Param,
                   std::atomic<float>* modDepth4Param);

    ~GranularSound() override = default;

    bool appliesToNote (int midiNote) override;
    bool appliesToChannel (int midiChannel) override;

    const juce::AudioBuffer<float>& getAudioData() const { return audioData; }
    int getRootNote() const { return rootNote; }
    
    double getSweepStartPitch() const { return sweepStartPitch; }
    double getPitchSweepLen() const { return pitchSweepLen; }
    double getDensitySweepLen() const { return getDecayValue() * densitySweepRatio; }
    double getFilterSweepLen() const { return getDecayValue() * filterSweepRatio; }
    double getAmpDecay() const { return ampDecay; }
    float getGainMultiplier() const { return gainMultiplier; }
    double getPitchOffset() const { return pitchOffset; }
    float getPadPitchAdjustment() const {
        float padAdj = pitchParameter != nullptr ? pitchParameter->load() : 0.0f;
        float globAdj = globalPitchParameter != nullptr ? globalPitchParameter->load() : 0.0f;
        return padAdj + globAdj;
    }
    float getDecayValue() const {
        float baseDecay = decayParameter != nullptr ? decayParameter->load() : (float)ampDecay;
        float globalDecayMult = globalDecayParameter != nullptr ? globalDecayParameter->load() : 1.0f;
        return baseDecay * globalDecayMult;
    }
    float getGrainSizeValue() const {
        return globalGrainSizeParameter != nullptr ? globalGrainSizeParameter->load() : 30.0f;
    }
    float getGrainSprayValue() const {
        return globalGrainSprayParameter != nullptr ? globalGrainSprayParameter->load() : 10.0f;
    }
    float getFilterResValue() const {
        return globalFilterResParameter != nullptr ? globalFilterResParameter->load() : 0.707f;
    }
    float getReverseChanceValue() const {
        return globalReverseParameter != nullptr ? globalReverseParameter->load() : 0.1f;
    }
    float getAnalogDriftValue() const {
        return globalAnalogDriftParameter != nullptr ? globalAnalogDriftParameter->load() : 0.75f;
    }
    float getGrainSizeJitterValue() const {
        return globalGrainSizeJitterParameter != nullptr ? globalGrainSizeJitterParameter->load() : 0.2f;
    }
    float getGrainPitchJitterValue() const {
        return globalGrainPitchJitterParameter != nullptr ? globalGrainPitchJitterParameter->load() : 1.0f;
    }
    float getPanJitterValue() const {
        return globalPanJitterParameter != nullptr ? globalPanJitterParameter->load() : 0.45f;
    }
    float getFilterSweepTimeValue() const {
        return globalFilterSweepTimeParameter != nullptr ? globalFilterSweepTimeParameter->load() : 0.3f;
    }
    float getPitchSweepDepthValue() const {
        return globalPitchSweepDepthParameter != nullptr ? globalPitchSweepDepthParameter->load() : 18.0f;
    }
    float getLayerBalanceValue() const {
        return globalLayerBalanceParameter != nullptr ? globalLayerBalanceParameter->load() : 0.5f;
    }
    float getLayerDelayValue() const {
        return globalLayerDelayParameter != nullptr ? globalLayerDelayParameter->load() : 0.0f;
    }
    float getLayerDetuneValue() const {
        return globalLayerDetuneParameter != nullptr ? globalLayerDetuneParameter->load() : 0.0f;
    }
    float getResonatorMixValue() const {
        return globalResonatorMixParameter != nullptr ? globalResonatorMixParameter->load() : 0.0f;
    }
    float getResonatorPitchValue() const {
        return globalResonatorPitchParameter != nullptr ? globalResonatorPitchParameter->load() : 48.0f;
    }
    float getResonatorFeedbackValue() const {
        return globalResonatorFeedbackParameter != nullptr ? globalResonatorFeedbackParameter->load() : 0.5f;
    }
    bool isMono() const { return monoParameter != nullptr && monoParameter->load() > 0.5f; }
    const juce::String& getCategory() const { return category; }

    float getGlobalLfoRate() const { return globalLfoRateParameter != nullptr ? globalLfoRateParameter->load() : 1.0f; }
    int getGlobalLfoShape() const { return globalLfoShapeParameter != nullptr ? (int)globalLfoShapeParameter->load() : 0; }
    int getModSrc (int slotIndex) const {
        if (slotIndex >= 0 && slotIndex < 4 && modSrcParameter[slotIndex] != nullptr)
            return (int)modSrcParameter[slotIndex]->load();
        return 0;
    }
    int getModDst (int slotIndex) const {
        if (slotIndex >= 0 && slotIndex < 4 && modDstParameter[slotIndex] != nullptr)
            return (int)modDstParameter[slotIndex]->load();
        return 0;
    }
    float getModDepth (int slotIndex) const {
        if (slotIndex >= 0 && slotIndex < 4 && modDepthParameter[slotIndex] != nullptr)
            return modDepthParameter[slotIndex]->load();
        return 0.0f;
    }

private:
    juce::String name;
    juce::AudioBuffer<float> audioData;
    juce::BigInteger midiNotes;
    int rootNote;
    
    double sweepStartPitch;
    double pitchSweepLen;
    double densitySweepRatio;
    double filterSweepRatio;
    double ampDecay;
    double pitchOffset;
    std::atomic<float>* pitchParameter;
    std::atomic<float>* decayParameter;
    std::atomic<float>* monoParameter;
    juce::String category;
    std::atomic<float>* globalPitchParameter;
    std::atomic<float>* globalDecayParameter;
    std::atomic<float>* globalGrainSizeParameter;
    std::atomic<float>* globalGrainSprayParameter;
    std::atomic<float>* globalFilterResParameter;
    std::atomic<float>* globalReverseParameter;
    std::atomic<float>* globalAnalogDriftParameter;
    std::atomic<float>* globalGrainSizeJitterParameter;
    std::atomic<float>* globalGrainPitchJitterParameter;
    std::atomic<float>* globalPanJitterParameter;
    std::atomic<float>* globalFilterSweepTimeParameter;
    std::atomic<float>* globalPitchSweepDepthParameter;
    std::atomic<float>* globalLayerBalanceParameter;
    std::atomic<float>* globalLayerDelayParameter;
    std::atomic<float>* globalLayerDetuneParameter;
    std::atomic<float>* globalResonatorMixParameter;
    std::atomic<float>* globalResonatorPitchParameter;
    std::atomic<float>* globalResonatorFeedbackParameter;
    std::atomic<float>* globalLfoRateParameter;
    std::atomic<float>* globalLfoShapeParameter;
    std::atomic<float>* modSrcParameter[4];
    std::atomic<float>* modDstParameter[4];
    std::atomic<float>* modDepthParameter[4];
    float gainMultiplier = 1.0f;
};

//==============================================================================
/**
    Granular voice that enforces percussive transients via pitch/density sweeps.
*/
class GranularVoice : public juce::SynthesiserVoice
{
public:
    GranularVoice();
    ~GranularVoice() override = default;

    bool canPlaySound (juce::SynthesiserSound* sound) override;
    void startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound* sound, int currentPitchWheelPosition) override;
    void stopNote (float velocity, bool allowTailOff) override;
    void pitchWheelMoved (int newPitchWheelValue) override;
    void controllerMoved (int controllerNumber, int newControllerValue) override;
    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;

private:
    struct Grain
    {
        double position = 0.0;     // Position in source buffer (samples)
        double rate = 1.0;         // Playback rate
        double windowPhase = 0.0;  // 0.0 to 1.0
        double windowInc = 0.0;    // Phase increment per sample
        bool active = false;
        float amp = 1.0f;
        float panL = 1.0f;
        float panR = 1.0f;
    };

    void processEnvelope (int numSamples);
    void spawnNewGrain (double startPos, double rate, int sourceLength);

    std::vector<Grain> grains;
    
    // Envelope / Modulators
    double currentSampleRate = 44100.0;
    int envelopePhase = 0;       // Samples since note start
    int pitchEnvelopeLength = 0; // Length of pitch sweep in samples
    int densityEnvelopeLength = 0; // Length of density sweep in samples
    int filterEnvelopeLength = 0; // Length of LPG filter sweep in samples
    int ampDecayLength = 0;      // Length of amplitude decay in samples
    int attackSamples = 0;
    int decaySamples = 0;
    int releaseSamples = 0;
    double sustainLevel = 0.25;
    double startPitch = 36.0;    // Starting pitch sweep value
    double pitchOffset = 0.0;    // Steady-state pitch offset in semitones
    double hitPitchJitter = 0.0; // Round-robin micro-pitch offset
    
    double currentDensity = 0.0; // Grains per second
    double currentPitch = 1.0;   // Playback rate multiplier
    
    double hitDecayJitter = 1.0;
    double hitFilterSweepJitter = 1.0;
    double hitDensitySweepJitter = 1.0;
    double velocityModFilter = 1.0;
    double hitGrainDurationBase = 0.030;
    double hitGrainSpraySec = 0.010;
    
    // SVF Filter State (Zero-Delay Feedback 2-pole)
    double filterS1_L = 0.0;
    double filterS2_L = 0.0;
    double filterS1_R = 0.0;
    double filterS2_R = 0.0;
    
    double samplesUntilNextGrain = 0.0;
    
    // Playhead progression
    double mainPlayhead = 0.0;
    double directPlayhead = 0.0;

    // Resonator Delay Buffers
    std::vector<float> resonatorBufferL;
    std::vector<float> resonatorBufferR;
    int resonatorWriteIdx = 0;

    // Modulation Matrix / LFO states
    double lfoPhase = 0.0;
    float randomLfoVal = 0.0f;
    float voiceRandomOffset = 0.0f;
    float triggerVelocity = 0.0f;
};
