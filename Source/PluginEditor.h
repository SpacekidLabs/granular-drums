#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "UI/GranularDrumsLookAndFeel.h"
#include "UI/MainWaveformDisplay.h"
#include "UI/PadGridComponent.h"
#include "UI/SequencerGridComponent.h"

class GranularDrumsEditor  : public juce::AudioProcessorEditor, public juce::FileDragAndDropTarget, public juce::Timer
{
public:
    GranularDrumsEditor (GranularDrumsProcessor&);
    ~GranularDrumsEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    
    void updateWaveforms();
    
    enum class Tab
    {
        sampler,
        sequencer,
        engine
    };
    void setTabActive (Tab newTab);
    
    // Timer override
    void timerCallback() override;
    
    // FileDragAndDropTarget overrides
    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void filesDropped (const juce::StringArray& files, int x, int y) override;
    void fileDragEnter (const juce::StringArray& files, int x, int y) override;
    void fileDragExit (const juce::StringArray& files) override;

private:
    GranularDrumsProcessor& audioProcessor;
    
    GranularDrumsLookAndFeel customLookAndFeel;
    MainWaveformDisplay waveformDisplay;
    PadGridComponent padGrid;
    SequencerGridComponent sequencerGrid;

    // Navigation Tabs
    juce::TextButton samplerTabButton;
    juce::TextButton sequencerTabButton;
    juce::TextButton engineTabButton;
    Tab activeTab = Tab::sampler;

    juce::TextButton globalRandomButton;

    juce::Slider globalPitchSlider;
    juce::Slider globalDecaySlider;
    juce::Slider globalGrainSizeSlider;
    juce::Slider globalGrainSpraySlider;
    juce::Slider globalFilterResSlider;
    juce::Slider globalReverseSlider;
    juce::Slider globalAnalogDriftSlider;
    juce::Slider globalGrainSizeJitterSlider;
    juce::Slider globalGrainPitchJitterSlider;
    juce::Slider globalPanJitterSlider;
    juce::Slider globalFilterSweepTimeSlider;
    juce::Slider globalPitchSweepDepthSlider;
    juce::Slider globalLayerBalanceSlider;
    juce::Slider globalLayerDelaySlider;
    juce::Slider globalLayerDetuneSlider;
    juce::Slider globalResonatorMixSlider;
    juce::Slider globalResonatorPitchSlider;
    juce::Slider globalResonatorFeedbackSlider;

    juce::Label globalPitchLabel;
    juce::Label globalDecayLabel;
    juce::Label globalGrainSizeLabel;
    juce::Label globalGrainSprayLabel;
    juce::Label globalFilterResLabel;
    juce::Label globalReverseLabel;
    juce::Label globalAnalogDriftLabel;
    juce::Label globalGrainSizeJitterLabel;
    juce::Label globalGrainPitchJitterLabel;
    juce::Label globalPanJitterLabel;
    juce::Label globalFilterSweepTimeLabel;
    juce::Label globalPitchSweepDepthLabel;
    juce::Label globalLayerBalanceLabel;
    juce::Label globalLayerDelayLabel;
    juce::Label globalLayerDetuneLabel;
    juce::Label globalResonatorMixLabel;
    juce::Label globalResonatorPitchLabel;
    juce::Label globalResonatorFeedbackLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> globalPitchAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> globalDecayAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> globalGrainSizeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> globalGrainSprayAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> globalFilterResAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> globalReverseAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> globalAnalogDriftAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> globalGrainSizeJitterAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> globalGrainPitchJitterAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> globalPanJitterAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> globalFilterSweepTimeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> globalPitchSweepDepthAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> globalLayerBalanceAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> globalLayerDelayAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> globalLayerDetuneAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> globalResonatorMixAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> globalResonatorPitchAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> globalResonatorFeedbackAttachment;

    // Master Distortion controls
    juce::TextButton distortionEnableButton;
    juce::Slider distortionDriveSlider;
    juce::Slider distortionOutputSlider;
    juce::Label distortionPanelLabel;
    juce::Label distortionDriveLabel;
    juce::Label distortionOutputLabel;
    
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> distortionEnableAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> distortionDriveAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> distortionOutputAttachment;

    // LFO / Modulation Matrix Controls
    juce::Slider lfoRateSlider;
    juce::Label lfoRateLabel;
    juce::ComboBox lfoShapeCombo;
    juce::Label lfoShapeLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lfoRateAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> lfoShapeAttachment;

    juce::ComboBox modSrcCombo[4];
    juce::ComboBox modDstCombo[4];
    juce::Slider modDepthSlider[4];
    juce::Label modSlotLabel[4];
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modSrcAttachment[4];
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modDstAttachment[4];
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> modDepthAttachment[4];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GranularDrumsEditor)
};
