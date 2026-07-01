#include "AnalysisEngine.h"

AnalysisEngine::AnalysisEngine()
    : Thread ("AnalysisEngine")
{
    formatManager.registerBasicFormats();
}

AnalysisEngine::~AnalysisEngine()
{
    stopThread (4000);
}

void AnalysisEngine::startAnalysis (const juce::File& audioFile)
{
    if (isThreadRunning())
        return;

    currentFile = audioFile;
    analyzing = true;
    progress = 0.0f;
    
    startThread();
}

bool AnalysisEngine::isAnalyzing() const
{
    return analyzing.load();
}

float AnalysisEngine::getProgress() const
{
    return progress.load();
}

void AnalysisEngine::run()
{
    // Step 1: Decode Audio
    progress = 0.1f;
    std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (currentFile));
    
    if (reader != nullptr)
    {
        sourceBuffer.setSize ((int) reader->numChannels, (int) reader->lengthInSamples);
        reader->read (&sourceBuffer, 0, (int) reader->lengthInSamples, 0, true, true);
        
        // Step 2: HPSS
        progress = 0.4f;
        auto [harmonic, percussive] = hpssProcessor.process (sourceBuffer);
        
        // Step 3: Onset Detection on Percussive signal
        progress = 0.7f;
        auto markers = onsetEngine.detectOnsets (percussive, reader->sampleRate);
        
        // Dummy sleep for visual feedback during testing
        juce::Thread::sleep (1000);
        
        if (onAnalysisFinished)
            onAnalysisFinished (markers, sourceBuffer);
    }

    progress = 1.0f;
    analyzing = false;
}
