#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <algorithm>
#include <limits>

namespace
{
constexpr const char* sampleStateName = "SAMPLE";
constexpr const char* currentMarkersStateName = "CURRENT_MARKERS";
constexpr const char* activeMarkersStateName = "ACTIVE_MARKERS";
constexpr const char* markerStateName = "MARKER";
constexpr const char* padAssignmentsStateName = "PAD_ASSIGNMENTS";

void removeChildrenWithName (juce::ValueTree& tree, const juce::Identifier& childName)
{
    for (int i = tree.getNumChildren() - 1; i >= 0; --i)
    {
        if (tree.getChild (i).hasType (childName))
            tree.removeChild (i, nullptr);
    }
}

juce::ValueTree createMarkerTree (const char* treeName, const std::vector<RegionMarker>& markers)
{
    juce::ValueTree tree (treeName);
    for (const auto& marker : markers)
    {
        juce::ValueTree markerTree (markerStateName);
        markerTree.setProperty ("sampleIndex", marker.sampleIndex, nullptr);
        markerTree.setProperty ("lengthInSamples", marker.lengthInSamples, nullptr);
        markerTree.setProperty ("confidence", marker.confidence, nullptr);
        markerTree.setProperty ("category", marker.category, nullptr);
        tree.addChild (markerTree, -1, nullptr);
    }
    return tree;
}

std::vector<RegionMarker> readMarkerTree (const juce::ValueTree& tree)
{
    std::vector<RegionMarker> markers;
    markers.reserve ((size_t) tree.getNumChildren());

    for (int i = 0; i < tree.getNumChildren(); ++i)
    {
        auto markerTree = tree.getChild (i);
        if (! markerTree.hasType (markerStateName))
            continue;

        RegionMarker marker;
        marker.sampleIndex = (int) markerTree.getProperty ("sampleIndex", 0);
        marker.lengthInSamples = (int) markerTree.getProperty ("lengthInSamples", 0);
        marker.confidence = (float) (double) markerTree.getProperty ("confidence", 0.0);
        marker.category = markerTree.getProperty ("category", "texture").toString();
        markers.push_back (marker);
    }

    return markers;
}
}

GranularDrumsProcessor::GranularDrumsProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
#else
    :
#endif
    apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    clearPattern();
    analysisEngine.onAnalysisFinished = [this] (const std::vector<RegionMarker>& markers,
                                                const juce::AudioBuffer<float>& sourceBuffer,
                                                const juce::File& sourceFile,
                                                double sourceSampleRate)
    {
        this->currentBuffer = sourceBuffer;
        this->currentMarkers = markers;
        this->currentSamplePath = sourceFile.getFullPathName();
        this->currentSampleRate = sourceSampleRate > 0.0 ? sourceSampleRate : 44100.0;
        
        // Reset selection parameters to default whole-file range (0.0 to 1.0)
        if (auto* startParam = apvts.getParameter ("selectionStart"))
            startParam->setValueNotifyingHost (0.0f);
        if (auto* endParam = apvts.getParameter ("selectionEnd"))
            endParam->setValueNotifyingHost (1.0f);
            
        updateActiveSlices (true);
        
        juce::MessageManager::callAsync ([this]() {
            if (auto* editor = dynamic_cast<GranularDrumsEditor*> (getActiveEditor()))
                editor->updateWaveforms();
        });
    };
}

GranularDrumsProcessor::~GranularDrumsProcessor()
{
}

void GranularDrumsProcessor::_assignSliceToPad (int padIndex)
{
    if (padIndex < 0 || padIndex >= 16)
        return;

    int mIdx = padMarkerIndices[padIndex];
    if (mIdx >= 0 && (size_t)mIdx < activeMarkers.size())
    {
        if (padSoundSeeds[padIndex] == 0)
            padSoundSeeds[padIndex] = _createPadSoundSeed();

        int startSample = activeMarkers[(size_t)mIdx].sampleIndex;
        int numSamplesInFile = currentBuffer.getNumSamples();
        
        float endRatio = apvts.getRawParameterValue ("selectionEnd")->load();
        int selectionEndSample = juce::roundToInt (endRatio * numSamplesInFile);
        
        int endSample = selectionEndSample;
        if (activeMarkers[(size_t)mIdx].lengthInSamples > 0)
            endSample = startSample + activeMarkers[(size_t)mIdx].lengthInSamples;
        else if ((size_t)(mIdx + 1) < activeMarkers.size())
            endSample = activeMarkers[(size_t)(mIdx + 1)].sampleIndex;

        endSample = juce::jlimit (startSample, numSamplesInFile, endSample);
            
        int numSamples = endSample - startSample;
        juce::String category = activeMarkers[(size_t)mIdx].category;
        
        // Ensure we don't grab the entire rest of the file - cap slice length to 0.5s max
        int maxSliceLength = (int)(getSampleRate() * 0.5);
        if (maxSliceLength <= 0) maxSliceLength = 22050;
        
        if (numSamples > maxSliceLength)
            numSamples = maxSliceLength;
            
        auto* pitchParam = apvts.getRawParameterValue ("pitch" + juce::String (padIndex));
        auto* decayParam = apvts.getRawParameterValue ("decay" + juce::String (padIndex));
        auto* monoParam = apvts.getRawParameterValue ("mono" + juce::String (padIndex));
        auto* globalPitchParam = apvts.getRawParameterValue ("globalPitch");
        auto* globalDecayParam = apvts.getRawParameterValue ("globalDecay");
        auto* globalGrainSizeParam = apvts.getRawParameterValue ("globalGrainSize");
        auto* globalGrainSprayParam = apvts.getRawParameterValue ("globalGrainSpray");
        auto* globalFilterResParam = apvts.getRawParameterValue ("globalFilterRes");
        auto* globalReverseParam = apvts.getRawParameterValue ("globalReverse");
        auto* globalAnalogDriftParam = apvts.getRawParameterValue ("globalAnalogDrift");
        auto* globalGrainSizeJitterParam = apvts.getRawParameterValue ("globalGrainSizeJitter");
        auto* globalGrainPitchJitterParam = apvts.getRawParameterValue ("globalGrainPitchJitter");
        auto* globalPanJitterParam = apvts.getRawParameterValue ("globalPanJitter");
        auto* globalFilterSweepTimeParam = apvts.getRawParameterValue ("globalFilterSweepTime");
        auto* globalPitchSweepDepthParam = apvts.getRawParameterValue ("globalPitchSweepDepth");
        auto* globalLayerBalanceParam = apvts.getRawParameterValue ("globalLayerBalance");
        auto* globalLayerDelayParam = apvts.getRawParameterValue ("globalLayerDelay");
        auto* globalLayerDetuneParam = apvts.getRawParameterValue ("globalLayerDetune");
        auto* globalResonatorMixParam = apvts.getRawParameterValue ("globalResonatorMix");
        auto* globalResonatorPitchParam = apvts.getRawParameterValue ("globalResonatorPitch");
        auto* globalResonatorFeedbackParam = apvts.getRawParameterValue ("globalResonatorFeedback");
        auto* globalLfoRateParam = apvts.getRawParameterValue ("globalLfoRate");
        auto* globalLfoShapeParam = apvts.getRawParameterValue ("globalLfoShape");
        auto* modSrc1Param = apvts.getRawParameterValue ("modSrc1");
        auto* modDst1Param = apvts.getRawParameterValue ("modDst1");
        auto* modDepth1Param = apvts.getRawParameterValue ("modDepth1");
        auto* modSrc2Param = apvts.getRawParameterValue ("modSrc2");
        auto* modDst2Param = apvts.getRawParameterValue ("modDst2");
        auto* modDepth2Param = apvts.getRawParameterValue ("modDepth2");
        auto* modSrc3Param = apvts.getRawParameterValue ("modSrc3");
        auto* modDst3Param = apvts.getRawParameterValue ("modDst3");
        auto* modDepth3Param = apvts.getRawParameterValue ("modDepth3");
        auto* modSrc4Param = apvts.getRawParameterValue ("modSrc4");
        auto* modDst4Param = apvts.getRawParameterValue ("modDst4");
        auto* modDepth4Param = apvts.getRawParameterValue ("modDepth4");
            
        if (numSamples > 0)
            playbackEngine.setPadSlice (padIndex, currentBuffer, startSample, numSamples, padSoundSeeds[padIndex], pitchParam, decayParam, monoParam, category, globalPitchParam, globalDecayParam, globalGrainSizeParam, globalGrainSprayParam, globalFilterResParam, globalReverseParam, globalAnalogDriftParam, globalGrainSizeJitterParam, globalGrainPitchJitterParam, globalPanJitterParam, globalFilterSweepTimeParam, globalPitchSweepDepthParam, globalLayerBalanceParam, globalLayerDelayParam, globalLayerDetuneParam, globalResonatorMixParam, globalResonatorPitchParam, globalResonatorFeedbackParam, globalLfoRateParam, globalLfoShapeParam, modSrc1Param, modDst1Param, modDepth1Param, modSrc2Param, modDst2Param, modDepth2Param, modSrc3Param, modDst3Param, modDepth3Param, modSrc4Param, modDst4Param, modDepth4Param);
    }
}

int GranularDrumsProcessor::_createPadSoundSeed() const
{
    return 1 + juce::Random::getSystemRandom().nextInt (0x3fffffff);
}

void GranularDrumsProcessor::randomizeAllPads()
{
    if (activeMarkers.empty()) return;
    suspendProcessing (true);
    playbackEngine.clearPads();
    
    for (int i = 0; i < 16; ++i)
    {
        padMarkerIndices[i] = juce::Random::getSystemRandom().nextInt ((int)activeMarkers.size());
        padSoundSeeds[i] = _createPadSoundSeed();
        _assignSliceToPad (i);
        
        // Randomize pitch parameter between -12 and +12 semitones
        if (auto* param = apvts.getParameter ("pitch" + juce::String (i)))
        {
            float randomPitch = -12.0f + juce::Random::getSystemRandom().nextFloat() * 24.0f;
            randomPitch = std::round (randomPitch);
            param->setValueNotifyingHost (param->getNormalisableRange().convertTo0to1 (randomPitch));
        }
    }
    suspendProcessing (false);
    
    juce::MessageManager::callAsync ([this]() {
        if (auto* editor = dynamic_cast<GranularDrumsEditor*> (getActiveEditor()))
            editor->updateWaveforms();
    });
}

void GranularDrumsProcessor::randomizePad (int padIndex)
{
    if (activeMarkers.empty() || padIndex < 0 || padIndex >= 16) return;
    
    suspendProcessing (true);
    padMarkerIndices[padIndex] = juce::Random::getSystemRandom().nextInt ((int)activeMarkers.size());
    padSoundSeeds[padIndex] = _createPadSoundSeed();
    _assignSliceToPad (padIndex);
    
    // Randomize pitch parameter between -12 and +12 semitones
    if (auto* param = apvts.getParameter ("pitch" + juce::String (padIndex)))
    {
        float randomPitch = -12.0f + juce::Random::getSystemRandom().nextFloat() * 24.0f;
        randomPitch = std::round (randomPitch);
        param->setValueNotifyingHost (param->getNormalisableRange().convertTo0to1 (randomPitch));
    }
    suspendProcessing (false);
    
    juce::MessageManager::callAsync ([this]() {
        if (auto* editor = dynamic_cast<GranularDrumsEditor*> (getActiveEditor()))
            editor->updateWaveforms();
    });
}

const juce::String GranularDrumsProcessor::getName() const
{
    return JucePlugin_Name;
}

bool GranularDrumsProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool GranularDrumsProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool GranularDrumsProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double GranularDrumsProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int GranularDrumsProcessor::getNumPrograms()
{
    return 1;
}

int GranularDrumsProcessor::getCurrentProgram()
{
    return 0;
}

void GranularDrumsProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String GranularDrumsProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void GranularDrumsProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

void GranularDrumsProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    playbackEngine.prepareToPlay (sampleRate, samplesPerBlock);
    mackityProcessor.prepareToPlay (sampleRate);
}

void GranularDrumsProcessor::releaseResources()
{
    playbackEngine.releaseResources();
}

bool GranularDrumsProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}

void GranularDrumsProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    int numSamples = buffer.getNumSamples();
    double sampleRate = getSampleRate();
    if (sampleRate <= 0.0) sampleRate = 44100.0;

    bool triggeredByHost = false;
    bool currentlySynced = false;
    double currentHostBPM = 120.0;

    // 1. Check DAW Playhead for Sync
    if (auto* playHead = getPlayHead())
    {
        if (auto positionInfo = playHead->getPosition())
        {
            auto optBpm = positionInfo->getBpm();
            auto optPpq = positionInfo->getPpqPosition();
            double bpm = optBpm.hasValue() ? *optBpm : 120.0;
            double ppqPosition = optPpq.hasValue() ? *optPpq : 0.0;
            bool isPlaying = positionInfo->getIsPlaying();
            
            currentlySynced = true;
            currentHostBPM = bpm;
            
            if (isPlaying)
            {
                triggeredByHost = true;
                double beatsPerSample = bpm / (60.0 * sampleRate);
                
                double startPpq = lastPpqPosition;
                double endPpq = ppqPosition;
                
                // If loop or timeline jump occurs, sync start to end position
                if (std::abs (endPpq - startPpq) > 1.0)
                    startPpq = endPpq;
                    
                double stepDur = 0.25; // 1/16th note = 0.25 beats
                
                double startStepDouble = startPpq / stepDur;
                double endStepDouble = endPpq / stepDur;
                
                int startStep = (int)std::ceil (startStepDouble);
                int endStep = (int)std::ceil (endStepDouble);
                
                for (int step = startStep; step < endStep; ++step)
                {
                    double stepPpq = (double)step * stepDur;
                    double ppqOffset = stepPpq - startPpq;
                    int sampleOffset = (int)(ppqOffset / beatsPerSample);
                    
                    if (sampleOffset >= 0 && sampleOffset < numSamples)
                    {
                        int step16 = step % 16;
                        currentActiveStep.store (step16);
                        
                        for (int pad = 0; pad < 16; ++pad)
                        {
                            if (sequencerSteps[pad][step16])
                            {
                                midiMessages.addEvent (
                                    juce::MidiMessage::noteOn (1, 36 + pad, 1.0f),
                                    sampleOffset
                                );
                            }
                        }
                    }
                }
                
                lastPpqPosition = endPpq;
            }
        }
    }

    isSyncedToHostCached.store (currentlySynced);
    hostBPMCached.store (currentHostBPM);

    // 2. Internal Clock Fallback (if DAW is stopped/standalone, and internal playing is active)
    if (!triggeredByHost && sequencerPlaying)
    {
        double samplesPerStep = (60.0 / sequencerBpm) * sampleRate * 0.25;
        
        for (int sampleIdx = 0; sampleIdx < numSamples; ++sampleIdx)
        {
            internalSequencerTimer += 1.0;
            if (internalSequencerTimer >= samplesPerStep)
            {
                internalSequencerTimer -= samplesPerStep;
                
                int step16 = internalSequencerStep % 16;
                currentActiveStep.store (step16);
                
                for (int pad = 0; pad < 16; ++pad)
                {
                    if (sequencerSteps[pad][step16])
                    {
                        midiMessages.addEvent (
                            juce::MidiMessage::noteOn (1, 36 + pad, 1.0f),
                            sampleIdx
                        );
                    }
                }
                
                internalSequencerStep = (internalSequencerStep + 1) % 16;
            }
        }
    }
    else if (!triggeredByHost && !sequencerPlaying)
    {
        internalSequencerTimer = 0.0;
        internalSequencerStep = 0;
    }

    playbackEngine.processBlock (buffer, midiMessages);

    bool distEnabled = false;
    float distDrive = 0.2f;
    float distOutput = 0.8f;

    if (auto* enableParam = apvts.getRawParameterValue ("distortionEnable"))
        distEnabled = enableParam->load() > 0.5f;

    if (auto* driveParam = apvts.getRawParameterValue ("distortionDrive"))
        distDrive = driveParam->load();

    if (auto* outputParam = apvts.getRawParameterValue ("distortionOutput"))
        distOutput = outputParam->load();

    mackityProcessor.processBlock (buffer, distDrive, distOutput, distEnabled);
}

void GranularDrumsProcessor::startAnalysis (const juce::File& file)
{
    analysisEngine.startAnalysis (file);
}

void GranularDrumsProcessor::triggerPad (int padIndex)
{
    playbackEngine.triggerPad (padIndex, 1.0f);
}

bool GranularDrumsProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* GranularDrumsProcessor::createEditor()
{
    return new GranularDrumsEditor (*this);
}

void GranularDrumsProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    removeChildrenWithName (state, juce::Identifier (sampleStateName));
    removeChildrenWithName (state, juce::Identifier ("SEQUENCER"));
    
    if (apvts.state.hasProperty ("winWidth"))
        state.setProperty ("winWidth", apvts.state.getProperty ("winWidth"), nullptr);
    if (apvts.state.hasProperty ("winHeight"))
        state.setProperty ("winHeight", apvts.state.getProperty ("winHeight"), nullptr);
    
    juce::ValueTree seqTree ("SEQUENCER");
    for (int pad = 0; pad < 16; ++pad)
    {
        juce::String stepString;
        for (int step = 0; step < 16; ++step)
            stepString += sequencerSteps[pad][step] ? "1" : "0";
        seqTree.setProperty ("pad" + juce::String (pad), stepString, nullptr);
    }
    seqTree.setProperty ("playState", sequencerPlaying ? "1" : "0", nullptr);
    seqTree.setProperty ("bpm", sequencerBpm, nullptr);
    
    state.addChild (seqTree, -1, nullptr);
    _writeSampleState (state);
    
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void GranularDrumsProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState.get() != nullptr)
    {
        if (xmlState->hasTagName (apvts.state.getType()))
        {
            auto newState = juce::ValueTree::fromXml (*xmlState);
            apvts.replaceState (newState);
            
            auto seqTree = newState.getChildWithName ("SEQUENCER");
            if (seqTree.isValid())
            {
                for (int pad = 0; pad < 16; ++pad)
                {
                    juce::String stepString = seqTree.getProperty ("pad" + juce::String (pad), "0000000000000000");
                    for (int step = 0; step < 16; ++step)
                    {
                        if (step < stepString.length())
                            sequencerSteps[pad][step] = (stepString[step] == '1');
                    }
                }
                sequencerPlaying = (seqTree.getProperty ("playState", "0") == "1");
                sequencerBpm = (double) seqTree.getProperty ("bpm", 120.0);
            }
            
            _restoreSampleState (newState);
            
            juce::MessageManager::callAsync ([this]() {
                if (auto* editor = dynamic_cast<GranularDrumsEditor*> (getActiveEditor()))
                    editor->updateWaveforms();
            });
        }
    }
}

void GranularDrumsProcessor::_writeSampleState (juce::ValueTree& state) const
{
    if (currentBuffer.getNumSamples() <= 0 || currentBuffer.getNumChannels() <= 0)
        return;

    juce::ValueTree sampleTree (sampleStateName);
    sampleTree.setProperty ("sourcePath", currentSamplePath, nullptr);
    sampleTree.setProperty ("sampleRate", currentSampleRate, nullptr);
    sampleTree.setProperty ("numChannels", currentBuffer.getNumChannels(), nullptr);
    sampleTree.setProperty ("numSamples", currentBuffer.getNumSamples(), nullptr);

    juce::MemoryBlock audioBlock;
    juce::MemoryOutputStream audioOut (audioBlock, false);

    for (int sample = 0; sample < currentBuffer.getNumSamples(); ++sample)
    {
        for (int channel = 0; channel < currentBuffer.getNumChannels(); ++channel)
        {
            const float value = juce::jlimit (-1.0f, 1.0f, currentBuffer.getSample (channel, sample));
            const auto packed = (short) juce::roundToInt (value * 32767.0f);
            audioOut.writeShort (packed);
        }
    }

    sampleTree.setProperty ("embeddedPcm16", juce::var (audioBlock), nullptr);
    sampleTree.addChild (createMarkerTree (currentMarkersStateName, currentMarkers), -1, nullptr);
    sampleTree.addChild (createMarkerTree (activeMarkersStateName, activeMarkers), -1, nullptr);

    juce::ValueTree padTree (padAssignmentsStateName);
    for (int i = 0; i < 16; ++i)
    {
        padTree.setProperty ("pad" + juce::String (i), padMarkerIndices[i], nullptr);
        padTree.setProperty ("seed" + juce::String (i), padSoundSeeds[i], nullptr);
    }
    sampleTree.addChild (padTree, -1, nullptr);

    state.addChild (sampleTree, -1, nullptr);
}

void GranularDrumsProcessor::_restoreSampleState (const juce::ValueTree& state)
{
    auto sampleTree = state.getChildWithName (sampleStateName);
    if (! sampleTree.isValid())
    {
        currentSamplePath.clear();
        currentBuffer.setSize (0, 0);
        currentMarkers.clear();
        activeMarkers.clear();
        std::fill_n (padSoundSeeds, 16, 0);
        suspendProcessing (true);
        playbackEngine.clearPads();
        suspendProcessing (false);
        return;
    }

    currentSamplePath = sampleTree.getProperty ("sourcePath", "").toString();
    currentSampleRate = (double) sampleTree.getProperty ("sampleRate", 44100.0);

    bool didRestoreAudio = false;
    if (currentSamplePath.isNotEmpty())
    {
        juce::File sourceFile (currentSamplePath);
        if (sourceFile.existsAsFile())
            didRestoreAudio = _loadAudioFileIntoBuffer (sourceFile, currentBuffer, currentSampleRate);
    }

    if (! didRestoreAudio)
        didRestoreAudio = _restoreEmbeddedAudio (sampleTree);

    if (! didRestoreAudio)
    {
        currentBuffer.setSize (0, 0);
        currentMarkers.clear();
        activeMarkers.clear();
        std::fill_n (padSoundSeeds, 16, 0);
        suspendProcessing (true);
        playbackEngine.clearPads();
        suspendProcessing (false);
        return;
    }

    currentMarkers = readMarkerTree (sampleTree.getChildWithName (currentMarkersStateName));
    activeMarkers = readMarkerTree (sampleTree.getChildWithName (activeMarkersStateName));

    auto padTree = sampleTree.getChildWithName (padAssignmentsStateName);
    for (int i = 0; i < 16; ++i)
    {
        padMarkerIndices[i] = (int) padTree.getProperty ("pad" + juce::String (i), i);
        padSoundSeeds[i] = (int) padTree.getProperty ("seed" + juce::String (i), 0);
        if (padSoundSeeds[i] == 0)
            padSoundSeeds[i] = _createPadSoundSeed();
    }

    if (activeMarkers.empty())
        updateActiveSlices();
    else
        _restorePadAssignments();
}

bool GranularDrumsProcessor::_loadAudioFileIntoBuffer (const juce::File& file, juce::AudioBuffer<float>& destination, double& sampleRateOut) const
{
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (file));
    if (reader == nullptr || reader->lengthInSamples <= 0 || reader->numChannels <= 0)
        return false;

    const auto numSamples = (int) std::min<int64_t> (reader->lengthInSamples, (int64_t) std::numeric_limits<int>::max());
    destination.setSize ((int) reader->numChannels, numSamples);
    reader->read (&destination, 0, numSamples, 0, true, true);
    sampleRateOut = reader->sampleRate > 0.0 ? reader->sampleRate : 44100.0;
    return true;
}

bool GranularDrumsProcessor::_restoreEmbeddedAudio (const juce::ValueTree& sampleTree)
{
    const int numChannels = (int) sampleTree.getProperty ("numChannels", 0);
    const int numSamples = (int) sampleTree.getProperty ("numSamples", 0);
    if (numChannels <= 0 || numSamples <= 0)
        return false;

    auto audioVar = sampleTree.getProperty ("embeddedPcm16");
    auto* audioBlock = audioVar.getBinaryData();
    if (audioBlock == nullptr || audioBlock->getSize() == 0)
        return false;

    const auto expectedBytes = (size_t) numChannels * (size_t) numSamples * sizeof (short);
    if (audioBlock->getSize() < expectedBytes)
        return false;

    currentBuffer.setSize (numChannels, numSamples);
    juce::MemoryInputStream audioIn (*audioBlock, false);

    for (int sample = 0; sample < numSamples; ++sample)
    {
        for (int channel = 0; channel < numChannels; ++channel)
        {
            const auto packed = (short) audioIn.readShort();
            currentBuffer.setSample (channel, sample, (float) packed / 32768.0f);
        }
    }

    currentSampleRate = (double) sampleTree.getProperty ("sampleRate", currentSampleRate);
    return true;
}

void GranularDrumsProcessor::_restorePadAssignments()
{
    suspendProcessing (true);
    playbackEngine.clearPads();
    for (int i = 0; i < 16; ++i)
        _assignSliceToPad (i);
    suspendProcessing (false);
}

juce::AudioProcessorValueTreeState::ParameterLayout GranularDrumsProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    for (int i = 0; i < 16; ++i)
    {
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID ("pitch" + juce::String (i), 1),
            "Pad " + juce::String (i + 1) + " Pitch",
            juce::NormalisableRange<float> (-24.0f, 24.0f, 0.1f), // 0.1st steps
            0.0f
        ));
        
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID ("decay" + juce::String (i), 1),
            "Pad " + juce::String (i + 1) + " Decay",
            juce::NormalisableRange<float> (0.01f, 3.0f, 0.01f, 0.4f), // 10ms steps, 0.4 skew
            0.3f // 300ms default for punchy drum sounds
        ));
        
        params.push_back (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID ("mono" + juce::String (i), 1),
            "Pad " + juce::String (i + 1) + " Mono",
            false
        ));
    }

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("selectionStart", 1),
        "Selection Start",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f),
        0.0f
    ));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("selectionEnd", 1),
        "Selection End",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f),
        1.0f
    ));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("globalPitch", 1),
        "Global Pitch Macro",
        juce::NormalisableRange<float> (-24.0f, 24.0f, 0.1f),
        0.0f
    ));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("globalDecay", 1),
        "Global Decay Macro",
        juce::NormalisableRange<float> (0.1f, 4.0f, 0.01f, 0.5f), // 0.1x to 4.0x, 0.5 skew
        2.0f
    ));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("globalGrainSize", 1),
        "Grain Size",
        juce::NormalisableRange<float> (5.0f, 100.0f, 0.1f, 0.5f),
        30.0f
    ));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("globalGrainSpray", 1),
        "Grain Spray",
        juce::NormalisableRange<float> (0.0f, 50.0f, 0.1f, 0.5f),
        10.0f
    ));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("globalFilterRes", 1),
        "Filter Resonance",
        juce::NormalisableRange<float> (0.1f, 5.0f, 0.01f, 0.5f),
        0.707f
    ));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("globalReverse", 1),
        "Reverse Chance",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f),
        0.1f
    ));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("globalAnalogDrift", 1),
        "Analog Drift",
        juce::NormalisableRange<float> (0.0f, 4.0f, 0.01f),
        0.75f
    ));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("globalGrainSizeJitter", 1),
        "Grain Size Jitter",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f),
        0.2f
    ));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("globalGrainPitchJitter", 1),
        "Grain Pitch Jitter",
        juce::NormalisableRange<float> (0.0f, 12.0f, 0.01f),
        1.0f
    ));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("globalPanJitter", 1),
        "Stereo Pan Jitter",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f),
        0.45f
    ));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("globalFilterSweepTime", 1),
        "Filter Sweep Time",
        juce::NormalisableRange<float> (0.01f, 2.0f, 0.01f, 0.5f),
        0.3f
    ));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("globalPitchSweepDepth", 1),
        "Pitch Sweep Depth",
        juce::NormalisableRange<float> (0.0f, 48.0f, 0.1f, 0.5f),
        18.0f
    ));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("globalLayerBalance", 1),
        "Layer Balance",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f),
        0.5f
    ));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("globalLayerDelay", 1),
        "Layer Delay",
        juce::NormalisableRange<float> (0.0f, 200.0f, 0.1f, 0.5f),
        0.0f
    ));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("globalLayerDetune", 1),
        "Layer Detune",
        juce::NormalisableRange<float> (-24.0f, 24.0f, 0.1f),
        0.0f
    ));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("globalResonatorMix", 1),
        "Resonator Mix",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f),
        0.0f
    ));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("globalResonatorPitch", 1),
        "Resonator Pitch",
        juce::NormalisableRange<float> (24.0f, 96.0f, 0.1f),
        48.0f
    ));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("globalResonatorFeedback", 1),
        "Resonator Decay",
        juce::NormalisableRange<float> (0.0f, 0.98f, 0.01f),
        0.5f
    ));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("globalLfoRate", 1),
        "LFO Rate",
        juce::NormalisableRange<float> (0.1f, 20.0f, 0.01f, 0.5f),
        1.0f
    ));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID ("globalLfoShape", 1),
        "LFO Shape",
        juce::StringArray { "Sine", "Triangle", "Saw", "Random" },
        0
    ));

    for (int i = 1; i <= 4; ++i)
    {
        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID ("modSrc" + juce::String (i), 1),
            "Slot " + juce::String (i) + " Source",
            juce::StringArray { "Off", "LFO", "Velocity", "Random" },
            0
        ));

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID ("modDst" + juce::String (i), 1),
            "Slot " + juce::String (i) + " Destination",
            juce::StringArray { "Off", "Pitch", "Decay", "Grain Size", "Filter Cutoff", "Reso Pitch", "Reso Mix" },
            0
        ));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID ("modDepth" + juce::String (i), 1),
            "Slot " + juce::String (i) + " Depth",
            juce::NormalisableRange<float> (-1.0f, 1.0f, 0.01f),
            0.0f
        ));
    }

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID ("distortionEnable", 1),
        "Master Distortion Enable",
        true
    ));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("distortionDrive", 1),
        "Master Distortion Drive",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f),
        0.2f
    ));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("distortionOutput", 1),
        "Master Distortion Output",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f),
        0.8f
    ));
    
    return { params.begin(), params.end() };
}

void GranularDrumsProcessor::updateActiveSlices (bool isNewFileLoaded)
{
    suspendProcessing (true);
    playbackEngine.clearPads();
    activeMarkers.clear();
    
    int numSamplesInFile = currentBuffer.getNumSamples();
    if (numSamplesInFile <= 0 || currentBuffer.getNumChannels() == 0)
    {
        suspendProcessing (false);
        return;
    }
        
    float startRatio = apvts.getRawParameterValue ("selectionStart")->load();
    float endRatio = apvts.getRawParameterValue ("selectionEnd")->load();
    
    if (startRatio >= endRatio - 0.01f)
        startRatio = endRatio - 0.01f;
        
    int selectionStartSample = juce::roundToInt (startRatio * numSamplesInFile);
    int selectionEndSample = juce::roundToInt (endRatio * numSamplesInFile);

    const bool usingExplicitRegions = std::any_of (currentMarkers.begin(), currentMarkers.end(), [] (const RegionMarker& marker)
    {
        return marker.lengthInSamples > 0;
    });

    if (usingExplicitRegions)
    {
        for (const auto& marker : currentMarkers)
        {
            const int regionEnd = marker.sampleIndex + marker.lengthInSamples;
            if (marker.sampleIndex < selectionEndSample && regionEnd > selectionStartSample)
                activeMarkers.push_back (marker);
        }

        if (activeMarkers.empty())
            activeMarkers.push_back ({ selectionStartSample, juce::jmax (1, selectionEndSample - selectionStartSample), 1.0f, "texture" });

        for (int i = 0; i < 16; ++i)
        {
            padMarkerIndices[i] = activeMarkers.empty() ? -1 : i % (int) activeMarkers.size();
            if (isNewFileLoaded || padSoundSeeds[i] == 0)
                padSoundSeeds[i] = _createPadSoundSeed();

            if (isNewFileLoaded)
            {
                if (auto* param = apvts.getParameter ("decay" + juce::String (i)))
                    param->setValueNotifyingHost (param->getNormalisableRange().convertTo0to1 (0.25f));
            }

            _assignSliceToPad (i);
        }

        suspendProcessing (false);
        return;
    }
    
    // Add start of selection as first marker
    activeMarkers.push_back ({ selectionStartSample, 0, 1.0f, "texture" });
    
    // Add all original markers that fall strictly within the selection window
    for (const auto& marker : currentMarkers)
    {
        if (marker.sampleIndex > selectionStartSample && marker.sampleIndex < selectionEndSample)
        {
            activeMarkers.push_back (marker);
        }
    }
    
    struct SliceInfo
    {
        int markerIndex;
        float energy;
        juce::String category;
    };
    
    std::vector<SliceInfo> kicks, snares, hats;
    
    for (size_t i = 0; i < activeMarkers.size(); ++i)
    {
        int startS = activeMarkers[i].sampleIndex;
        int endS = selectionEndSample;
        if (i + 1 < activeMarkers.size())
            endS = activeMarkers[i + 1].sampleIndex;
            
        int len = endS - startS;
        if (len <= 0) continue;
        
        float lowEnergy = 0.0f;
        float midEnergy = 0.0f;
        float highEnergy = 0.0f;
        
        int scanSamples = juce::jmin (1000, len);
        float lpState = 0.0f;
        float lpHPState = 0.0f;
        float alphaLP = 0.021f;
        float alphaHP = 0.57f;
        
        for (int n = 0; n < scanSamples; ++n)
        {
            int idx = startS + n;
            if (idx >= numSamplesInFile) break;
            
            float s = currentBuffer.getSample (0, idx);
            
            lpState = lpState + alphaLP * (s - lpState);
            lowEnergy += std::abs (lpState);
            
            lpHPState = lpHPState + alphaHP * (s - lpHPState);
            float hpVal = s - lpHPState;
            highEnergy += std::abs (hpVal);
            
            midEnergy += std::abs (s - lpState - hpVal);
        }
        
        float sumEnergy = lowEnergy + midEnergy + highEnergy + 0.0001f;
        float lowRatio = lowEnergy / sumEnergy;
        float highRatio = highEnergy / sumEnergy;
        
        float sliceTotalEnergy = 0.0f;
        int step = juce::jmax (1, len / 500);
        for (int n = 0; n < len; n += step)
        {
            int idx = startS + n;
            if (idx >= numSamplesInFile) break;
            sliceTotalEnergy += std::abs (currentBuffer.getSample (0, idx));
        }
        float avgEnergy = len > 0 ? sliceTotalEnergy / (float)(len / step) : 0.0f;
        
        SliceInfo info = { (int)i, avgEnergy, "snare" };
        
        if (lowRatio > 0.45f && lowRatio > highRatio * 1.5f)
        {
            info.category = "kick";
            kicks.push_back (info);
        }
        else if (highRatio > 0.50f && highRatio > lowRatio * 2.0f)
        {
            info.category = "hat";
            hats.push_back (info);
        }
        else
        {
            snares.push_back (info);
        }
        
        activeMarkers[i].category = info.category;
    }
    
    auto sortByEnergy = [] (const SliceInfo& a, const SliceInfo& b) { return a.energy > b.energy; };
    std::sort (kicks.begin(), kicks.end(), sortByEnergy);
    std::sort (snares.begin(), snares.end(), sortByEnergy);
    std::sort (hats.begin(), hats.end(), sortByEnergy);
    
    std::vector<SliceInfo> remainingPool;
    std::vector<bool> markerAssigned (activeMarkers.size(), false);
    
    int targetPadIndices[16];
    std::fill_n (targetPadIndices, 16, -1);
    
    int kickCount = std::min (4, (int)kicks.size());
    for (size_t i = 0; i < (size_t)kickCount; ++i)
    {
        targetPadIndices[i] = kicks[i].markerIndex;
        markerAssigned[(size_t)kicks[i].markerIndex] = true;
    }
    
    int snareCount = std::min (4, (int)snares.size());
    for (size_t i = 0; i < (size_t)snareCount; ++i)
    {
        targetPadIndices[4 + i] = snares[i].markerIndex;
        markerAssigned[(size_t)snares[i].markerIndex] = true;
    }
    
    int hatCount = std::min (4, (int)hats.size());
    for (size_t i = 0; i < (size_t)hatCount; ++i)
    {
        targetPadIndices[8 + i] = hats[i].markerIndex;
        markerAssigned[(size_t)hats[i].markerIndex] = true;
    }
    
    for (size_t i = 0; i < activeMarkers.size(); ++i)
    {
        if (! markerAssigned[i])
        {
            float totalEnergy = 0.0f;
            int startS = activeMarkers[i].sampleIndex;
            int endS = selectionEndSample;
            if (i + 1 < activeMarkers.size())
                endS = activeMarkers[i + 1].sampleIndex;
            int len = endS - startS;
            
            int step = juce::jmax (1, len / 1000);
            for (int n = 0; n < len; n += step)
            {
                int idx = startS + n;
                if (idx >= numSamplesInFile) break;
                totalEnergy += std::abs (currentBuffer.getSample (0, idx));
            }
            float avgEnergy = len > 0 ? totalEnergy / (float)(len / step) : 0.0f;
            remainingPool.push_back ({ (int)i, avgEnergy, "other" });
        }
    }
    std::sort (remainingPool.begin(), remainingPool.end(), sortByEnergy);
    
    int otherCount = std::min (4, (int)remainingPool.size());
    for (size_t i = 0; i < (size_t)otherCount; ++i)
    {
        targetPadIndices[12 + i] = remainingPool[i].markerIndex;
        markerAssigned[(size_t)remainingPool[i].markerIndex] = true;
    }
    
    if (otherCount > 0)
        remainingPool.erase (remainingPool.begin(), remainingPool.begin() + otherCount);
        
    for (int i = 0; i < 16; ++i)
    {
        if (targetPadIndices[i] == -1 && ! remainingPool.empty())
        {
            targetPadIndices[i] = remainingPool.front().markerIndex;
            remainingPool.erase (remainingPool.begin());
        }
    }
    
    for (int i = 0; i < 16; ++i)
    {
        padMarkerIndices[i] = targetPadIndices[i];
        if (isNewFileLoaded || padSoundSeeds[i] == 0)
            padSoundSeeds[i] = _createPadSoundSeed();
        
        // Apply category-specific decay defaults only when a new file is loaded
        if (isNewFileLoaded)
        {
            int mIdx = padMarkerIndices[i];
            juce::String category = "snare";
            if (mIdx >= 0 && (size_t)mIdx < activeMarkers.size())
                category = activeMarkers[(size_t)mIdx].category;
                
            if (auto* param = apvts.getParameter ("decay" + juce::String (i)))
            {
                float defaultDecay = 0.3f;
                if (category == "kick") defaultDecay = 0.22f;
                else if (category == "hat") defaultDecay = 0.08f;
                else if (category == "snare") defaultDecay = 0.28f;
                
                param->setValueNotifyingHost (param->getNormalisableRange().convertTo0to1 (defaultDecay));
            }
        }
        
        _assignSliceToPad (i);
    }

    suspendProcessing (false);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GranularDrumsProcessor();
}
