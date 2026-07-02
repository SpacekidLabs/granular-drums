#include "PadPlaybackEngine.h"

PadPlaybackEngine::PadPlaybackEngine()
{
    // Add 16 voices
    for (int i = 0; i < 16; ++i)
        synth.addVoice (new GranularVoice());
}

PadPlaybackEngine::~PadPlaybackEngine()
{
}

void PadPlaybackEngine::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (samplesPerBlock);
    const juce::ScopedLock lock (synthLock);
    synth.setCurrentPlaybackSampleRate (sampleRate);
}

void PadPlaybackEngine::releaseResources()
{
}

void PadPlaybackEngine::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    const juce::ScopedLock lock (synthLock);
    synth.renderNextBlock (buffer, midiMessages, 0, buffer.getNumSamples());
}

void PadPlaybackEngine::triggerPad (int padIndex, float velocity)
{
    // Map pad index (0-15) to MIDI note (e.g., starting at C1 = 36)
    int midiNote = 36 + padIndex;
    const juce::ScopedLock lock (synthLock);
    synth.noteOn (1, midiNote, velocity);
}

void PadPlaybackEngine::clearPads()
{
    const juce::ScopedLock lock (synthLock);
    synth.clearSounds();
}

void PadPlaybackEngine::setPadSlice (int padIndex, const juce::AudioBuffer<float>& sourceBuffer, int startSample, int numSamples, int soundSeed, std::atomic<float>* pitchParam, std::atomic<float>* decayParam, std::atomic<float>* monoParam, const juce::String& category, std::atomic<float>* globalPitchParam, std::atomic<float>* globalDecayParam, std::atomic<float>* globalGrainSizeParam, std::atomic<float>* globalGrainSprayParam, std::atomic<float>* globalFilterResParam, std::atomic<float>* globalReverseParam, std::atomic<float>* globalAnalogDriftParam, std::atomic<float>* globalGrainSizeJitterParam, std::atomic<float>* globalGrainPitchJitterParam, std::atomic<float>* globalPanJitterParam, std::atomic<float>* globalFilterSweepTimeParam, std::atomic<float>* globalPitchSweepDepthParam, std::atomic<float>* globalLayerBalanceParam, std::atomic<float>* globalLayerDelayParam, std::atomic<float>* globalLayerDetuneParam, std::atomic<float>* globalResonatorMixParam, std::atomic<float>* globalResonatorPitchParam, std::atomic<float>* globalResonatorFeedbackParam, std::atomic<float>* globalLfoRateParam, std::atomic<float>* globalLfoShapeParam, std::atomic<float>* modSrc1Param, std::atomic<float>* modDst1Param, std::atomic<float>* modDepth1Param, std::atomic<float>* modSrc2Param, std::atomic<float>* modDst2Param, std::atomic<float>* modDepth2Param, std::atomic<float>* modSrc3Param, std::atomic<float>* modDst3Param, std::atomic<float>* modDepth3Param, std::atomic<float>* modSrc4Param, std::atomic<float>* modDst4Param, std::atomic<float>* modDepth4Param)
{
    if (startSample < 0 || startSample + numSamples > sourceBuffer.getNumSamples() || numSamples <= 0 || sourceBuffer.getNumChannels() == 0)
        return;

    int midiNote = 36 + padIndex;
    
    juce::BigInteger notes;
    notes.setBit (midiNote);
    
    // Customize granular parameters based on texture category.
    juce::Random rand (soundSeed != 0 ? soundSeed : 1 + padIndex);
    double startPitch = 12.0 + rand.nextDouble() * 12.0;    // Default snare sweep starts +12 to +24 semitones
    double pitchSweepLen = 0.03 + rand.nextDouble() * 0.03;  // Default snare sweep length (30ms to 60ms)
    double ampDecay = decayParam != nullptr ? (double)decayParam->load() : 0.3;
    double densitySweepRatio = 0.7 + rand.nextDouble() * 0.2;
    double filterSweepRatio = 0.8 + rand.nextDouble() * 0.2;
    double pitchOffset = -6.0 + rand.nextDouble() * 12.0;    // Snare mid offset (+/- 6 semitones)
    
    if (category == "body")
    {
        startPitch = 24.0 + rand.nextDouble() * 12.0;        // Sweep starts higher (+24 to +36)
        pitchSweepLen = 0.015 + rand.nextDouble() * 0.015;   // Fast pitch drop (15ms to 30ms)
        pitchOffset = -18.0 - rand.nextDouble() * 12.0;      // Low fundamental offset (-18 to -30 semitones)
        densitySweepRatio = 0.4 + rand.nextDouble() * 0.2;   // Fast density sweep
        filterSweepRatio = 0.3 + rand.nextDouble() * 0.2;    // Fast LPF sweep
    }
    else if (category == "noise")
    {
        startPitch = 0.0;                                    // No pitch sweep on hats
        pitchSweepLen = 0.0;
        pitchOffset = 12.0 + rand.nextDouble() * 18.0;       // High-pitched (+12 to +30 semitones)
        densitySweepRatio = 0.8 + rand.nextDouble() * 0.2;   // Long density sweep
        filterSweepRatio = 0.9 + rand.nextDouble() * 0.2;    // Keep HPF open
    }
    
    auto* newSound = new GranularSound ("Pad " + juce::String(padIndex), sourceBuffer, startSample, numSamples, notes, midiNote, startPitch, pitchSweepLen, densitySweepRatio, filterSweepRatio, ampDecay, pitchOffset, pitchParam, decayParam, monoParam, category, globalPitchParam, globalDecayParam, globalGrainSizeParam, globalGrainSprayParam, globalFilterResParam, globalReverseParam, globalAnalogDriftParam, globalGrainSizeJitterParam, globalGrainPitchJitterParam, globalPanJitterParam, globalFilterSweepTimeParam, globalPitchSweepDepthParam, globalLayerBalanceParam, globalLayerDelayParam, globalLayerDetuneParam, globalResonatorMixParam, globalResonatorPitchParam, globalResonatorFeedbackParam, globalLfoRateParam, globalLfoShapeParam, modSrc1Param, modDst1Param, modDepth1Param, modSrc2Param, modDst2Param, modDepth2Param, modSrc3Param, modDst3Param, modDepth3Param, modSrc4Param, modDst4Param, modDepth4Param);

    const juce::ScopedLock lock (synthLock);

    // Remove any existing sound for this pad to prevent accumulation
    for (int i = synth.getNumSounds() - 1; i >= 0; --i)
    {
        if (auto* sound = dynamic_cast<GranularSound*> (synth.getSound(i).get()))
        {
            if (sound->getRootNote() == midiNote)
            {
                synth.removeSound(i);
            }
        }
    }

    synth.addSound (newSound);
}

bool PadPlaybackEngine::isPadPlaying (int padIndex) const
{
    int midiNote = 36 + padIndex;
    const juce::ScopedLock lock (synthLock);
    for (int i = 0; i < synth.getNumVoices(); ++i)
    {
        if (auto* voice = synth.getVoice (i))
        {
            if (voice->isVoiceActive() && voice->getCurrentlyPlayingNote() == midiNote)
                return true;
        }
    }
    return false;
}
