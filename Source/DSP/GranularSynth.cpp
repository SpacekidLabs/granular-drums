#include "GranularSynth.h"
#include <cmath>

static inline float interpolateCubic (float ym1, float y0, float y1, float y2, float frac)
{
    float a = -0.5f * ym1 + 1.5f * y0 - 1.5f * y1 + 0.5f * y2;
    float b = ym1 - 2.5f * y0 + 2.0f * y1 - 0.5f * y2;
    float c = -0.5f * ym1 + 0.5f * y1;
    float d = y0;
    return a * frac * frac * frac + b * frac * frac + c * frac + d;
}

//==============================================================================
GranularSound::GranularSound (const juce::String& soundName,
                              const juce::AudioBuffer<float>& sourceBuffer,
                              int startSample,
                              int numSamples,
                              const juce::BigInteger& notes,
                              int root,
                              double sweepStartPitchParam,
                              double pitchSweepLenParam,
                              double densitySweepLenParam,
                              double filterSweepLenParam,
                              double ampDecayParam,
                              double pitchOffsetParam,
                              std::atomic<float>* pitchParameterParam,
                              std::atomic<float>* decayParameterParam,
                              std::atomic<float>* monoParameterParam,
                              const juce::String& categoryParam,
                              std::atomic<float>* globalPitchParameterParam,
                              std::atomic<float>* globalDecayParameterParam,
                              std::atomic<float>* globalGrainSizeParameterParam,
                              std::atomic<float>* globalGrainSprayParameterParam,
                              std::atomic<float>* globalFilterResParameterParam,
                              std::atomic<float>* globalReverseParameterParam,
                              std::atomic<float>* globalAnalogDriftParameterParam,
                              std::atomic<float>* globalGrainSizeJitterParameterParam,
                              std::atomic<float>* globalGrainPitchJitterParameterParam,
                              std::atomic<float>* globalPanJitterParameterParam,
                              std::atomic<float>* globalFilterSweepTimeParameterParam,
                              std::atomic<float>* globalPitchSweepDepthParameterParam,
                              std::atomic<float>* globalLayerBalanceParameterParam,
                              std::atomic<float>* globalLayerDelayParameterParam,
                              std::atomic<float>* globalLayerDetuneParameterParam,
                              std::atomic<float>* globalResonatorMixParameterParam,
                              std::atomic<float>* globalResonatorPitchParameterParam,
                              std::atomic<float>* globalResonatorFeedbackParameterParam,
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
                              std::atomic<float>* modDepth4Param)
    : name (soundName), midiNotes (notes), rootNote (root),
      sweepStartPitch (sweepStartPitchParam), pitchSweepLen (pitchSweepLenParam), 
      densitySweepRatio (densitySweepLenParam), filterSweepRatio (filterSweepLenParam),
      ampDecay (ampDecayParam), pitchOffset (pitchOffsetParam), pitchParameter (pitchParameterParam),
      decayParameter (decayParameterParam), monoParameter (monoParameterParam), category (categoryParam),
      globalPitchParameter (globalPitchParameterParam), globalDecayParameter (globalDecayParameterParam),
      globalGrainSizeParameter (globalGrainSizeParameterParam),
      globalGrainSprayParameter (globalGrainSprayParameterParam),
      globalFilterResParameter (globalFilterResParameterParam),
      globalReverseParameter (globalReverseParameterParam),
      globalAnalogDriftParameter (globalAnalogDriftParameterParam),
      globalGrainSizeJitterParameter (globalGrainSizeJitterParameterParam),
      globalGrainPitchJitterParameter (globalGrainPitchJitterParameterParam),
      globalPanJitterParameter (globalPanJitterParameterParam),
      globalFilterSweepTimeParameter (globalFilterSweepTimeParameterParam),
      globalPitchSweepDepthParameter (globalPitchSweepDepthParameterParam),
      globalLayerBalanceParameter (globalLayerBalanceParameterParam),
      globalLayerDelayParameter (globalLayerDelayParameterParam),
      globalLayerDetuneParameter (globalLayerDetuneParameterParam),
      globalResonatorMixParameter (globalResonatorMixParameterParam),
      globalResonatorPitchParameter (globalResonatorPitchParameterParam),
      globalResonatorFeedbackParameter (globalResonatorFeedbackParameterParam),
      globalLfoRateParameter (globalLfoRateParam),
      globalLfoShapeParameter (globalLfoShapeParam)
{
    modSrcParameter[0] = modSrc1Param;
    modSrcParameter[1] = modSrc2Param;
    modSrcParameter[2] = modSrc3Param;
    modSrcParameter[3] = modSrc4Param;

    modDstParameter[0] = modDst1Param;
    modDstParameter[1] = modDst2Param;
    modDstParameter[2] = modDst3Param;
    modDstParameter[3] = modDst4Param;

    modDepthParameter[0] = modDepth1Param;
    modDepthParameter[1] = modDepth2Param;
    modDepthParameter[2] = modDepth3Param;
    modDepthParameter[3] = modDepth4Param;
    // Make a direct copy of the requested slice!
    int channels = sourceBuffer.getNumChannels();
    audioData.setSize (channels, numSamples);
    
    float maxAbs = 0.0001f;
    for (int ch = 0; ch < channels; ++ch)
    {
        audioData.copyFrom (ch, 0, sourceBuffer, ch, startSample, numSamples);
        const float* ptr = audioData.getReadPointer(ch);
        for (int i = 0; i < numSamples; ++i)
        {
            if (std::abs(ptr[i]) > maxAbs)
                maxAbs = std::abs(ptr[i]);
        }
    }
    
    // Normalize gain so peak is roughly 0.8
    gainMultiplier = 0.8f / maxAbs;
    if (gainMultiplier > 10.0f) gainMultiplier = 10.0f; // Cap gain

    // Apply linear fade-in and fade-out to prevent clicks and pops
    int fadeInSamples = juce::jmin (88, numSamples / 10);
    int fadeOutSamples = juce::jmin (660, numSamples / 5);
    
    for (int ch = 0; ch < channels; ++ch)
    {
        float* ptr = audioData.getWritePointer (ch);
        
        for (int i = 0; i < fadeInSamples; ++i)
        {
            float progress = (float)i / (float)fadeInSamples;
            ptr[i] *= progress;
        }
        
        for (int i = 0; i < fadeOutSamples; ++i)
        {
            int idx = numSamples - fadeOutSamples + i;
            float progress = 1.0f - ((float)i / (float)fadeOutSamples);
            ptr[idx] *= progress;
        }
    }
}

bool GranularSound::appliesToNote (int midiNote)
{
    return midiNotes[midiNote];
}

bool GranularSound::appliesToChannel (int /*midiChannel*/)
{
    return true;
}

//==============================================================================
GranularVoice::GranularVoice()
{
    grains.resize (64); // Max 64 simultaneous grains per voice
    resonatorBufferL.resize (16384, 0.0f);
    resonatorBufferR.resize (16384, 0.0f);
    resonatorWriteIdx = 0;
}

bool GranularVoice::canPlaySound (juce::SynthesiserSound* sound)
{
    return dynamic_cast<GranularSound*> (sound) != nullptr;
}

void GranularVoice::startNote (int /*midiNoteNumber*/, float velocity, juce::SynthesiserSound* sound, int /*currentPitchWheelPosition*/)
{
    if (auto* gs = dynamic_cast<GranularSound*> (sound))
    {
        currentSampleRate = getSampleRate();
        if (currentSampleRate <= 0.0)
            currentSampleRate = 44100.0;
        
        envelopePhase = 0;
        
        // Decay length jitter (+/- 15% round-robin variation)
        hitDecayJitter = 0.85 + juce::Random::getSystemRandom().nextDouble() * 0.30;
        double decayVal = gs->getDecayValue();
        ampDecayLength = (int)(currentSampleRate * decayVal * hitDecayJitter);
        
        // Volume ADR (Attack-Decay-Release) envelope parameters for snappy percussive hits
        attackSamples = (int)(currentSampleRate * 0.0015); // 1.5ms attack
        decaySamples = (int)(currentSampleRate * 0.050);  // 50ms decay
        sustainLevel = 0.05 + 0.1 * decayVal;
        releaseSamples = ampDecayLength - attackSamples - decaySamples;
        if (releaseSamples < 1)
        {
            // If the sound is extremely short, dynamically scale the envelope stages to fit
            attackSamples = (int)(ampDecayLength * 0.05);
            decaySamples = (int)(ampDecayLength * 0.25);
            releaseSamples = ampDecayLength - attackSamples - decaySamples;
            if (releaseSamples < 1) releaseSamples = 1;
        }
        
        // Filter sweep length dynamic modulation by velocity + round-robin jitter (+/- 20%)
        velocityModFilter = 0.8 + (double)velocity * 0.4;
        hitFilterSweepJitter = 0.8 + juce::Random::getSystemRandom().nextDouble() * 0.4;
        filterEnvelopeLength = (int)(currentSampleRate * gs->getFilterSweepTimeValue() * velocityModFilter * hitFilterSweepJitter);
        
        // Pitch and density sweep length round-robin jitter (+/- 20%)
        double pitchSweepJitter = 0.8 + juce::Random::getSystemRandom().nextDouble() * 0.4;
        pitchEnvelopeLength = (int)(currentSampleRate * gs->getPitchSweepLen() * pitchSweepJitter);
        
        hitDensitySweepJitter = 0.8 + juce::Random::getSystemRandom().nextDouble() * 0.4;
        densityEnvelopeLength = (int)(currentSampleRate * gs->getDensitySweepLen() * hitDensitySweepJitter);
        
        // Start pitch of the sweep modulated by velocity + round-robin sweep depth jitter (+/- 1.5 semitones)
        double velocityModPitch = ((double)velocity - 0.7) * 3.0; // Harder hits start higher
        double sweepStartPitchJitter = (juce::Random::getSystemRandom().nextDouble() * 2.0 - 1.0) * 1.5;
        double pitchSweepDepth = gs->getPitchSweepDepthValue();
        if (gs->getCategory() == "hat")
            pitchSweepDepth = 0.0;
        startPitch = pitchSweepDepth + velocityModPitch + sweepStartPitchJitter;
        
        pitchOffset = gs->getPitchOffset();
        
        // Round-robin micro-pitch jitter (analog drift)
        double driftRange = gs->getAnalogDriftValue();
        hitPitchJitter = (juce::Random::getSystemRandom().nextDouble() * 2.0 - 1.0) * driftRange;
        
        // Set grain size base and spatial spray width from global parameters (convert from ms to seconds)
        hitGrainDurationBase = gs->getGrainSizeValue() / 1000.0;
        hitGrainSpraySec = gs->getGrainSprayValue() / 1000.0;
        
        filterS1_L = 0.0;
        filterS2_L = 0.0;
        filterS1_R = 0.0;
        filterS2_R = 0.0;
        
        // Start playhead exactly at the beginning of the slice for a consistent transient attack
        mainPlayhead = 0.0;
        directPlayhead = 0.0;
        
        samplesUntilNextGrain = 0.0;
        
        // Clear resonator buffers
        std::fill (resonatorBufferL.begin(), resonatorBufferL.end(), 0.0f);
        std::fill (resonatorBufferR.begin(), resonatorBufferR.end(), 0.0f);
        resonatorWriteIdx = 0;

        // Initialize modulation states
        lfoPhase = 0.0;
        triggerVelocity = velocity;
        voiceRandomOffset = juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f;
        randomLfoVal = juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f;
        
        // Clear old grains
        for (auto& g : grains)
            g.active = false;
    }
    else
    {
        clearCurrentNote();
    }
}

void GranularVoice::stopNote (float /*velocity*/, bool allowTailOff)
{
    if (allowTailOff)
    {
        // Just let it decay (grains finish their windows)
        // For simplicity, we just clear the note.
    }
    clearCurrentNote();
}

void GranularVoice::pitchWheelMoved (int) {}
void GranularVoice::controllerMoved (int, int) {}

void GranularVoice::spawnNewGrain (double startPos, double rate, int sourceLength)
{
    auto* playingSound = dynamic_cast<GranularSound*> (getCurrentlyPlayingSound().get());
    if (playingSound == nullptr)
        return;

    // Find an inactive grain
    for (auto& g : grains)
    {
        if (! g.active)
        {
            g.active = true;
            
            // Calculate progress through amplitude envelope
            double progress = ampDecayLength > 0 ? (double)envelopePhase / (double)ampDecayLength : 0.0;
            if (progress > 1.0) progress = 1.0;
            
            // Add random jitter to grain position (using the per-hit randomized spray width)
            double jitterSamples = currentSampleRate * hitGrainSpraySec;
            
            // Ensure jitter doesn't exceed a reasonable portion of the slice
            double maxJitter = sourceLength * 0.25;
            if (jitterSamples > maxJitter)
                jitterSamples = maxJitter;
                
            double offset = (juce::Random::getSystemRandom().nextDouble() * 2.0 - 1.0) * jitterSamples;
            
            // Jitter grain duration dynamically using global grain size jitter
            double sizeJitter = playingSound->getGrainSizeJitterValue();
            double grainDurationMs = hitGrainDurationBase * std::max (0.01, (1.0 - sizeJitter) + juce::Random::getSystemRandom().nextDouble() * (2.0 * sizeJitter));
            double grainSamples = currentSampleRate * grainDurationMs;
            
            // Ensure grain duration doesn't exceed the slice length
            double minGrainSamples = juce::jmin (currentSampleRate * 0.002, sourceLength * 0.9);
            if (minGrainSamples < 4.0) minGrainSamples = 4.0; // absolute minimum to prevent divide by zero
            
            if (grainSamples > sourceLength * 0.5)
                grainSamples = sourceLength * 0.5;
            if (grainSamples < minGrainSamples)
                grainSamples = minGrainSamples;
                
            g.windowInc = 1.0 / grainSamples;
            
            // 2% round-robin rate jitter
            double rateJitter = 0.98 + juce::Random::getSystemRandom().nextDouble() * 0.04;
            
            double baseReverseChance = playingSound->getReverseChanceValue();
            double reverseChance = baseReverseChance * (0.5 + progress * 0.5);
            bool reverse = juce::Random::getSystemRandom().nextDouble() < reverseChance;
            
            if (reverse)
            {
                g.rate = -rate * rateJitter;
                g.position = startPos + offset + grainSamples;
                
                // Clamp position so it stays within [grainSamples, sourceLength - 1]
                if (g.position >= sourceLength)
                    g.position = (double)sourceLength - 1.0;
                if (g.position < grainSamples)
                    g.position = grainSamples;
            }
            else
            {
                g.rate = rate * rateJitter;
                g.position = startPos + offset;
                
                // Clamp position so it stays within [0, sourceLength - grainSamples - 1]
                double maxStartPos = (double)sourceLength - grainSamples - 1.0;
                if (maxStartPos < 0.0) maxStartPos = 0.0;
                
                if (g.position > maxStartPos)
                    g.position = maxStartPos;
                if (g.position < 0.0)
                    g.position = 0.0;
            }
            
            // Micro-pitch detuning chorus using global grain pitch jitter parameter
            double pitchJitterVal = playingSound->getGrainPitchJitterValue();
            double maxDetuneSemitones = pitchJitterVal * (1.0 + progress * 2.0);
            double detuneVal = (juce::Random::getSystemRandom().nextDouble() * 2.0 - 1.0) * maxDetuneSemitones;
            double detuneRateMult = std::pow (2.0, detuneVal / 12.0);
            g.rate *= detuneRateMult;
            
            g.windowPhase = 0.0;
            g.amp = 0.9f + juce::Random::getSystemRandom().nextFloat() * 0.2f;
            
            // Dynamic stereo space using global pan jitter parameter
            float panJitterVal = playingSound->getPanJitterValue();
            float maxPanJitter = juce::jmin (1.0f, panJitterVal * (1.0f + (float)progress));
            float panJitter = (float)(juce::Random::getSystemRandom().nextDouble() * 2.0 - 1.0) * maxPanJitter;
            g.panL = std::cos (juce::MathConstants<float>::pi * 0.25f * (1.0f + panJitter));
            g.panR = std::sin (juce::MathConstants<float>::pi * 0.25f * (1.0f + panJitter));
            break;
        }
    }
}

void GranularVoice::renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    auto* playingSound = dynamic_cast<GranularSound*> (getCurrentlyPlayingSound().get());
    if (playingSound == nullptr || playingSound->getAudioData().getNumSamples() == 0)
        return;

    const auto& sourceData = playingSound->getAudioData();
    const int sourceLength = sourceData.getNumSamples();

    // Dynamically update envelopes and release decay to instantly respond to slider changes
    double decayVal = playingSound->getDecayValue();
    ampDecayLength = (int)(currentSampleRate * decayVal * hitDecayJitter);
    
    attackSamples = (int)(currentSampleRate * 0.0015);
    decaySamples = (int)(currentSampleRate * 0.050);
    sustainLevel = 0.05 + 0.1 * decayVal;
    releaseSamples = ampDecayLength - attackSamples - decaySamples;
    if (releaseSamples < 1)
    {
        attackSamples = (int)(ampDecayLength * 0.05);
        decaySamples = (int)(ampDecayLength * 0.25);
        releaseSamples = ampDecayLength - attackSamples - decaySamples;
        if (releaseSamples < 1) releaseSamples = 1;
    }
    
    filterEnvelopeLength = (int)(currentSampleRate * playingSound->getFilterSweepTimeValue() * velocityModFilter * hitFilterSweepJitter);
    densityEnvelopeLength = (int)(currentSampleRate * playingSound->getDensitySweepLen() * hitDensitySweepJitter);
    
    // Dynamically update grain size and spray to respond instantly to slider changes
    hitGrainDurationBase = playingSound->getGrainSizeValue() / 1000.0;
    hitGrainSpraySec = playingSound->getGrainSprayValue() / 1000.0;

    double layerBalance = playingSound->getLayerBalanceValue();
    double layerDelay = playingSound->getLayerDelayValue();
    double layerDetune = playingSound->getLayerDetuneValue();
    int layerDelaySamples = (int)(currentSampleRate * (layerDelay / 1000.0));
    double layerDetuneMult = std::pow (2.0, layerDetune / 12.0);
    
    float resFeedback = playingSound->getResonatorFeedbackValue();

    for (int i = 0; i < numSamples; ++i)
    {
        // Advance global LFO
        double lfoRate = playingSound->getGlobalLfoRate();
        lfoPhase += lfoRate / currentSampleRate;
        if (lfoPhase >= 1.0)
        {
            lfoPhase = std::fmod (lfoPhase, 1.0);
            randomLfoVal = juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f;
        }

        // Compute LFO shape value
        float lfoVal = 0.0f;
        int shape = playingSound->getGlobalLfoShape();
        if (shape == 0) // Sine
            lfoVal = std::sin ((float)(juce::MathConstants<double>::twoPi * lfoPhase));
        else if (shape == 1) // Triangle
            lfoVal = (lfoPhase < 0.5) ? (float)(4.0 * lfoPhase - 1.0) : (float)(3.0 - 4.0 * lfoPhase);
        else if (shape == 2) // Saw
            lfoVal = (float)(2.0 * lfoPhase - 1.0);
        else if (shape == 3) // Random
            lfoVal = randomLfoVal;

        // Accumulate modulations
        float modPitch = 0.0f;
        float modDecay = 0.0f;
        float modGrainSize = 0.0f;
        float modFilterCutoff = 0.0f;
        float modResoPitch = 0.0f;
        float modResoMix = 0.0f;

        for (int slot = 0; slot < 4; ++slot)
        {
            int src = playingSound->getModSrc (slot);
            int dst = playingSound->getModDst (slot);
            float depth = playingSound->getModDepth (slot);
            
            if (src == 0 || dst == 0 || depth == 0.0f)
                continue;
                
            float srcVal = 0.0f;
            if (src == 1) // LFO
                srcVal = lfoVal;
            else if (src == 2) // Velocity
                srcVal = triggerVelocity;
            else if (src == 3) // Random
                srcVal = voiceRandomOffset;
                
            float modAmount = srcVal * depth;
            
            if (dst == 1) // Pitch
                modPitch += modAmount;
            else if (dst == 2) // Decay
                modDecay += modAmount;
            else if (dst == 3) // Grain Size
                modGrainSize += modAmount;
            else if (dst == 4) // Filter Cutoff
                modFilterCutoff += modAmount;
            else if (dst == 5) // Reso Pitch
                modResoPitch += modAmount;
            else if (dst == 6) // Reso Mix
                modResoMix += modAmount;
        }

        // Calculate transient-to-granular crossfade weights (smooth S-curve)
        double directMix = 1.0;
        double granularMix = 0.0;
        
        int transientStartFade = (int)(currentSampleRate * 0.008); // 8ms
        int transientEndFade = (int)(currentSampleRate * 0.020);   // 20ms
        
        if (envelopePhase < transientStartFade)
        {
            directMix = 1.0;
            granularMix = 0.0;
        }
        else if (envelopePhase < transientEndFade)
        {
            double progress = (double)(envelopePhase - transientStartFade) / (double)(transientEndFade - transientStartFade);
            double smoothProgress = progress * progress * (3.0 - 2.0 * progress);
            directMix = 1.0 - smoothProgress;
            granularMix = smoothProgress;
        }
        else
        {
            directMix = 0.0;
            granularMix = 1.0;
        }

        // Cap playhead at 60ms before the end of the slice to sustain the granular tail
        double maxPlayhead = (double)sourceLength - (currentSampleRate * 0.060);
        if (maxPlayhead < 0.0) maxPlayhead = 0.0;
        if (mainPlayhead > maxPlayhead)
            mainPlayhead = maxPlayhead;

        // 1. Update sweep envelopes
        double pitchProgress = pitchEnvelopeLength > 0 ? (double)envelopePhase / (double)pitchEnvelopeLength : 1.0;
        if (pitchProgress > 1.0) pitchProgress = 1.0;
        
        double densityProgress = densityEnvelopeLength > 0 ? (double)envelopePhase / (double)densityEnvelopeLength : 1.0;
        if (densityProgress > 1.0) densityProgress = 1.0;
        
        double filterProgress = filterEnvelopeLength > 0 ? (double)envelopePhase / (double)filterEnvelopeLength : 1.0;
        if (filterProgress > 1.0) filterProgress = 1.0;
        
        // Exponential-like decay curve
        double pitchCurve = std::pow (1.0 - pitchProgress, 6.0); // Faster drop
        double densityCurve = std::pow (1.0 - densityProgress, 4.0);
        double filterCurve = std::pow (1.0 - filterProgress, 3.0); // Slower drop

        // Map curve to Pitch (from startPitch + pitchOffset + dynamicAdjustment down to pitchOffset + dynamicAdjustment)
        float padPitchAdj = playingSound->getPadPitchAdjustment();
        double pitchSemitones = pitchOffset + padPitchAdj + hitPitchJitter + (pitchCurve * startPitch) + (double)(modPitch * 24.0f);
        currentPitch = std::pow (2.0, pitchSemitones / 12.0);

        // Map curve to Density (from 150 grains/sec down to 5 grains/sec for a shattered tail)
        currentDensity = 5.0 + densityCurve * 145.0;
        
        // Map curve to Filter Cutoff (from 20000Hz down to 1200Hz to keep tail crisp and audible)
        double cutoff = 1200.0 + filterCurve * 18800.0 + (double)(modFilterCutoff * 8000.0f);
        bool useHighPass = (playingSound->getCategory() == "hat");
        if (useHighPass)
        {
            // For hats, sweep highpass from 6000Hz down to 200Hz
            cutoff = 200.0 + filterCurve * 5800.0 + (double)(modFilterCutoff * 8000.0f);
        }

        double g = std::tan (juce::MathConstants<double>::pi * cutoff / currentSampleRate);
        if (g < 0.001) g = 0.001;
        if (g > 20.0) g = 20.0; // clamp near Nyquist
        
        double filterQ = playingSound->getFilterResValue();
        double r = 1.0 / filterQ; // 1 / Q damping coefficient
        double a1 = 1.0 / (1.0 + g * (g + r));
        
        // Modulate Decay parameter
        decayVal = playingSound->getDecayValue() + (double)(modDecay * 2.0f);
        decayVal = juce::jlimit (0.01, 3.0, decayVal);
        ampDecayLength = (int)(currentSampleRate * decayVal * hitDecayJitter);
        
        attackSamples = (int)(currentSampleRate * 0.0015);
        decaySamples = (int)(currentSampleRate * 0.050);
        sustainLevel = 0.05 + 0.1 * decayVal;
        releaseSamples = ampDecayLength - attackSamples - decaySamples;
        if (releaseSamples < 1)
        {
            attackSamples = (int)(ampDecayLength * 0.05);
            decaySamples = (int)(ampDecayLength * 0.25);
            releaseSamples = ampDecayLength - attackSamples - decaySamples;
            if (releaseSamples < 1) releaseSamples = 1;
        }

        // Multi-stage Amplitude Envelope (Attack-Decay-Release) for punchy transient and smooth tail
        double currentAmp = 0.0;
        if (envelopePhase < attackSamples)
        {
            currentAmp = (double)envelopePhase / (double)attackSamples;
        }
        else if (envelopePhase < attackSamples + decaySamples)
        {
            double progress = (double)(envelopePhase - attackSamples) / (double)decaySamples;
            currentAmp = sustainLevel + (1.0 - sustainLevel) * std::pow (1.0 - progress, 3.0);
        }
        else
        {
            double progress = (double)(envelopePhase - attackSamples - decaySamples) / (double)releaseSamples;
            if (progress > 1.0) progress = 1.0;
            currentAmp = sustainLevel * std::pow (1.0 - progress, 3.0);
        }
        
        // Dynamic boost for sparse grains to preserve granular textures as density drops
        double densityBoost = 1.0 + (1.0 - (currentDensity - 5.0) / 145.0) * 1.5;
        
        if (envelopePhase >= ampDecayLength || (envelopePhase > attackSamples && currentAmp <= 0.001))
        {
            clearCurrentNote();
            break;
        }

        // 2. Grain scheduling (applying layer delay and detune)
        if (envelopePhase >= layerDelaySamples)
        {
            if (samplesUntilNextGrain <= 0.0)
            {
                // Modulate Grain Size
                double grainSizeMod = (double)modGrainSize * 50.0; // +/- 50ms
                double grainSizeMs = playingSound->getGrainSizeValue() + grainSizeMod;
                grainSizeMs = juce::jlimit (5.0, 100.0, grainSizeMs);
                hitGrainDurationBase = grainSizeMs / 1000.0;

                double granularPitch = currentPitch * layerDetuneMult;
                spawnNewGrain (mainPlayhead, granularPitch, sourceLength);
                double densityJitter = 0.9 + juce::Random::getSystemRandom().nextDouble() * 0.2; // 90% to 110%
                samplesUntilNextGrain = (currentSampleRate / currentDensity) * densityJitter;
            }
            samplesUntilNextGrain -= 1.0;
        }

        // 3. Render direct transient path (if directMix > 0.0)
        float directOutL = 0.0f;
        float directOutR = 0.0f;

        if (directMix > 0.0)
        {
            int posInt = (int)directPlayhead;
            double frac = directPlayhead - posInt;

            if (posInt >= 0 && posInt + 1 < sourceLength)
            {
                int idx0 = posInt - 1;
                int idx1 = posInt;
                int idx2 = posInt + 1;
                int idx3 = posInt + 2;

                if (idx0 < 0) idx0 = 0;
                if (idx1 < 0) idx1 = 0;
                if (idx1 >= sourceLength) idx1 = sourceLength - 1;
                if (idx2 >= sourceLength) idx2 = sourceLength - 1;
                if (idx3 >= sourceLength) idx3 = sourceLength - 1;

                float s0_L = sourceData.getSample (0, idx0);
                float s1_L = sourceData.getSample (0, idx1);
                float s2_L = sourceData.getSample (0, idx2);
                float s3_L = sourceData.getSample (0, idx3);
                float sampleL = interpolateCubic (s0_L, s1_L, s2_L, s3_L, (float)frac);

                float sampleR = sampleL;
                if (sourceData.getNumChannels() > 1)
                {
                    float s0_R = sourceData.getSample (1, idx0);
                    float s1_R = sourceData.getSample (1, idx1);
                    float s2_R = sourceData.getSample (1, idx2);
                    float s3_R = sourceData.getSample (1, idx3);
                    sampleR = interpolateCubic (s0_R, s1_R, s2_R, s3_R, (float)frac);
                }

                if (playingSound->isMono())
                {
                    float sampleMono = sampleL;
                    if (sourceData.getNumChannels() > 1)
                        sampleMono = (sampleL + sampleR) * 0.5f;

                    directOutL = sampleMono * (float)currentAmp * 0.7071f;
                    directOutR = sampleMono * (float)currentAmp * 0.7071f;
                }
                else
                {
                    directOutL = sampleL * (float)currentAmp;
                    directOutR = sampleR * (float)currentAmp;
                }
            }
            
            // Advance direct playhead
            directPlayhead += currentPitch;
        }

        // 4. Render active grains
        float granularOutL = 0.0f;
        float granularOutR = 0.0f;

        for (auto& grain : grains)
        {
            if (grain.active)
            {
                // Hann Window
                double window = 0.5 * (1.0 - std::cos (juce::MathConstants<double>::twoPi * grain.windowPhase));

                int posInt = (int)grain.position;
                double frac = grain.position - posInt;

                if (posInt >= 0 && posInt + 1 < sourceLength)
                {
                    // 4-point cubic Hermite interpolation
                    int idx0 = posInt - 1;
                    int idx1 = posInt;
                    int idx2 = posInt + 1;
                    int idx3 = posInt + 2;

                    if (idx0 < 0) idx0 = 0;
                    if (idx1 < 0) idx1 = 0;
                    if (idx1 >= sourceLength) idx1 = sourceLength - 1;
                    if (idx2 >= sourceLength) idx2 = sourceLength - 1;
                    if (idx3 >= sourceLength) idx3 = sourceLength - 1;

                    float s0_L = sourceData.getSample (0, idx0);
                    float s1_L = sourceData.getSample (0, idx1);
                    float s2_L = sourceData.getSample (0, idx2);
                    float s3_L = sourceData.getSample (0, idx3);
                    float sampleL = interpolateCubic (s0_L, s1_L, s2_L, s3_L, (float)frac);

                    float sampleR = sampleL;
                    if (sourceData.getNumChannels() > 1)
                    {
                        float s0_R = sourceData.getSample (1, idx0);
                        float s1_R = sourceData.getSample (1, idx1);
                        float s2_R = sourceData.getSample (1, idx2);
                        float s3_R = sourceData.getSample (1, idx3);
                        sampleR = interpolateCubic (s0_R, s1_R, s2_R, s3_R, (float)frac);
                    }

                    if (playingSound->isMono())
                    {
                        float sampleMono = sampleL;
                        if (sourceData.getNumChannels() > 1)
                            sampleMono = (sampleL + sampleR) * 0.5f;

                        granularOutL += sampleMono * (float)window * grain.amp * (float)(currentAmp * densityBoost) * 0.7071f;
                        granularOutR += sampleMono * (float)window * grain.amp * (float)(currentAmp * densityBoost) * 0.7071f;
                    }
                    else
                    {
                        granularOutL += sampleL * (float)window * grain.amp * (float)(currentAmp * densityBoost) * grain.panL;
                        granularOutR += sampleR * (float)window * grain.amp * (float)(currentAmp * densityBoost) * grain.panR;
                    }
                }

                // Advance grain
                grain.position += grain.rate;
                grain.windowPhase += grain.windowInc;

                if (grain.windowPhase >= 1.0 || grain.position >= sourceLength || grain.position < 0.0)
                    grain.active = false;
            }
        }

        // Mix direct and granular signals based on crossfade weights and Layer Balance
        float directWeight = (1.0f - (float)layerBalance) * 2.0f;
        float granularWeight = (float)layerBalance * 2.0f;
        float outL = directOutL * (float)directMix * directWeight + granularOutL * (float)granularMix * granularWeight;
        float outR = directOutR * (float)directMix * directWeight + granularOutR * (float)granularMix * granularWeight;

        // Apply gain matching
        outL *= playingSound->getGainMultiplier();
        outR *= playingSound->getGainMultiplier();

        // Modulate Resonator Mix & Pitch
        float localResMix = playingSound->getResonatorMixValue() + modResoMix;
        localResMix = juce::jlimit (0.0f, 1.0f, localResMix);

        float localResPitch = playingSound->getResonatorPitchValue() + modResoPitch * 36.0f; // +/- 36 semitones
        localResPitch = juce::jlimit (24.0f, 96.0f, localResPitch);

        // Resonant Feedback Loop (Granular Resonator)
        if (localResMix > 0.0f)
        {
            double resFreq = 440.0 * std::pow (2.0, (double)(localResPitch - 69.0f) / 12.0);
            double delayInSamples = currentSampleRate / resFreq;
            
            if (delayInSamples < 2.0) delayInSamples = 2.0;
            if (delayInSamples > 16000.0) delayInSamples = 16000.0;
            
            float delayReadPos = (float)resonatorWriteIdx - (float)delayInSamples;
            while (delayReadPos < 0.0f) delayReadPos += 16384.0f;
            
            size_t idx0 = (size_t)((int)delayReadPos & 16383);
            size_t idx1 = (idx0 + 1) & 16383;
            float frac = (float)(delayReadPos - (double)idx0);
            
            float delayedL = (1.0f - frac) * resonatorBufferL[idx0] + frac * resonatorBufferL[idx1];
            float delayedR = (1.0f - frac) * resonatorBufferR[idx0] + frac * resonatorBufferR[idx1];
            
            float resL = outL + delayedL * resFeedback;
            float resR = outR + delayedR * resFeedback;
            
            resonatorBufferL[(size_t)resonatorWriteIdx] = resL;
            resonatorBufferR[(size_t)resonatorWriteIdx] = resR;
            resonatorWriteIdx = (resonatorWriteIdx + 1) & 16383;
            
            outL = outL * (1.0f - localResMix) + resL * localResMix;
            outR = outR * (1.0f - localResMix) + resR * localResMix;
        }

        // 2-pole Zero-Delay Feedback State Variable Filter
        double hp_L = ((double)outL - (r + g) * filterS1_L - filterS2_L) * a1;
        double bp_L = g * hp_L + filterS1_L;
        double lp_L = g * bp_L + filterS2_L;
        filterS1_L = 2.0 * bp_L - filterS1_L;
        filterS2_L = 2.0 * lp_L - filterS2_L;

        double hp_R = ((double)outR - (r + g) * filterS1_R - filterS2_R) * a1;
        double bp_R = g * hp_R + filterS1_R;
        double lp_R = g * bp_R + filterS2_R;
        filterS1_R = 2.0 * bp_R - filterS1_R;
        filterS2_R = 2.0 * lp_R - filterS2_R;

        double filteredL = useHighPass ? hp_L : lp_L;
        double filteredR = useHighPass ? hp_R : lp_R;

        // Mix to output
        if (outputBuffer.getNumChannels() >= 1)
            outputBuffer.addSample (0, startSample + i, (float)filteredL);
        if (outputBuffer.getNumChannels() >= 2)
            outputBuffer.addSample (1, startSample + i, (float)filteredR);

        // 4. Advance main playhead (reads through file at normal speed)
        mainPlayhead += 1.0;
        envelopePhase++;
    }
}
