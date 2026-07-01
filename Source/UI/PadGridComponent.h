#pragma once

#include <JuceHeader.h>

class GranularDrumsProcessor;

class PadGridComponent : public juce::Component
{
public:
    PadGridComponent (GranularDrumsProcessor&);
    ~PadGridComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    
    std::function<void(int)> onPadClicked;
    std::function<void(int)> onPadRandomizeClicked;
    
    void setPadWaveformData (int padIndex, const juce::AudioBuffer<float>* buffer, int startSample, int numSamples, const juce::String& category);
    void setPadPlaying (int padIndex, bool isPlaying);

private:
    class PadComponent;
    std::vector<std::unique_ptr<PadComponent>> pads;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PadGridComponent)
};
