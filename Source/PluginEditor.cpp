#include "PluginProcessor.h"
#include "PluginEditor.h"

GranularDrumsEditor::GranularDrumsEditor (GranularDrumsProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), padGrid (p), sequencerGrid (p)
{
    setLookAndFeel (&customLookAndFeel);

    addAndMakeVisible (waveformDisplay);
    addAndMakeVisible (padGrid);
    addChildComponent (sequencerGrid);
    
    // Navigation Tabs Setup
    samplerTabButton.setButtonText ("SAMPLER");
    samplerTabButton.setToggleState (true, juce::dontSendNotification);
    samplerTabButton.setColour (juce::TextButton::buttonColourId, juce::Colour::fromString ("#FF252525"));
    samplerTabButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour::fromString ("#FFC4A673")); // Muted brass
    samplerTabButton.setColour (juce::TextButton::textColourOffId, juce::Colour::fromString ("#FFA0A0A0"));
    samplerTabButton.setColour (juce::TextButton::textColourOnId, juce::Colour::fromString ("#FF121212"));
    addAndMakeVisible (samplerTabButton);
    samplerTabButton.onClick = [this] { setTabActive (Tab::sampler); };

    sequencerTabButton.setButtonText ("SEQUENCER");
    sequencerTabButton.setToggleState (false, juce::dontSendNotification);
    sequencerTabButton.setColour (juce::TextButton::buttonColourId, juce::Colour::fromString ("#FF252525"));
    sequencerTabButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour::fromString ("#FFC4A673"));
    sequencerTabButton.setColour (juce::TextButton::textColourOffId, juce::Colour::fromString ("#FFA0A0A0"));
    sequencerTabButton.setColour (juce::TextButton::textColourOnId, juce::Colour::fromString ("#FF121212"));
    addAndMakeVisible (sequencerTabButton);
    sequencerTabButton.onClick = [this] { setTabActive (Tab::sequencer); };

    engineTabButton.setButtonText ("ENGINE");
    engineTabButton.setToggleState (false, juce::dontSendNotification);
    engineTabButton.setColour (juce::TextButton::buttonColourId, juce::Colour::fromString ("#FF252525"));
    engineTabButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour::fromString ("#FFC4A673"));
    engineTabButton.setColour (juce::TextButton::textColourOffId, juce::Colour::fromString ("#FFA0A0A0"));
    engineTabButton.setColour (juce::TextButton::textColourOnId, juce::Colour::fromString ("#FF121212"));
    addAndMakeVisible (engineTabButton);
    engineTabButton.onClick = [this] { setTabActive (Tab::engine); };

    globalRandomButton.setButtonText ("GLOBAL RANDOMIZE");
    globalRandomButton.setColour (juce::TextButton::buttonColourId, juce::Colour::fromString ("#FFFF9F1C"));
    globalRandomButton.setColour (juce::TextButton::textColourOffId, juce::Colour::fromString ("#FF0C0C0C"));
    globalRandomButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour::fromString ("#FFEFEBE0"));
    globalRandomButton.setColour (juce::TextButton::textColourOnId, juce::Colour::fromString ("#FF0C0C0C"));
    addAndMakeVisible (globalRandomButton);
    globalRandomButton.onClick = [this] { audioProcessor.randomizeAllPads(); };
    
    // Global Pitch Macro setup
    globalPitchSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    globalPitchSlider.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 45, 15);
    globalPitchSlider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    globalPitchSlider.setColour (juce::Slider::textBoxTextColourId, juce::Colour::fromString ("#FFA0A0A0"));
    globalPitchSlider.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour::fromString ("#FFC4A673"));
    globalPitchSlider.setTextValueSuffix (" st");
    globalPitchAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.getAPVTS(), "globalPitch", globalPitchSlider);
    addAndMakeVisible (globalPitchSlider);
    
    globalPitchLabel.setText ("PITCH MACRO", juce::dontSendNotification);
    globalPitchLabel.setColour (juce::Label::textColourId, juce::Colour::fromString ("#FFA0A0A0"));
    globalPitchLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (globalPitchLabel);

    // Global Decay Macro setup
    globalDecaySlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    globalDecaySlider.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 45, 15);
    globalDecaySlider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    globalDecaySlider.setColour (juce::Slider::textBoxTextColourId, juce::Colour::fromString ("#FFA0A0A0"));
    globalDecaySlider.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour::fromString ("#FFC4A673"));
    globalDecaySlider.setTextValueSuffix ("x");
    globalDecayAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.getAPVTS(), "globalDecay", globalDecaySlider);
    addAndMakeVisible (globalDecaySlider);
    
    globalDecayLabel.setText ("DECAY MACRO", juce::dontSendNotification);
    globalDecayLabel.setColour (juce::Label::textColourId, juce::Colour::fromString ("#FFA0A0A0"));
    globalDecayLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (globalDecayLabel);

    // Global Grain Size setup
    globalGrainSizeSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    globalGrainSizeSlider.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 45, 15);
    globalGrainSizeSlider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    globalGrainSizeSlider.setColour (juce::Slider::textBoxTextColourId, juce::Colour::fromString ("#FFA0A0A0"));
    globalGrainSizeSlider.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour::fromString ("#FFC4A673"));
    globalGrainSizeSlider.setTextValueSuffix (" ms");
    globalGrainSizeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.getAPVTS(), "globalGrainSize", globalGrainSizeSlider);
    addAndMakeVisible (globalGrainSizeSlider);
    
    globalGrainSizeLabel.setText ("GRAIN SIZE", juce::dontSendNotification);
    globalGrainSizeLabel.setColour (juce::Label::textColourId, juce::Colour::fromString ("#FFA0A0A0"));
    globalGrainSizeLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (globalGrainSizeLabel);

    // Global Grain Spray setup
    globalGrainSpraySlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    globalGrainSpraySlider.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 45, 15);
    globalGrainSpraySlider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    globalGrainSpraySlider.setColour (juce::Slider::textBoxTextColourId, juce::Colour::fromString ("#FFA0A0A0"));
    globalGrainSpraySlider.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour::fromString ("#FFC4A673"));
    globalGrainSpraySlider.setTextValueSuffix (" ms");
    globalGrainSprayAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.getAPVTS(), "globalGrainSpray", globalGrainSpraySlider);
    addAndMakeVisible (globalGrainSpraySlider);
    
    globalGrainSprayLabel.setText ("GRAIN SPRAY", juce::dontSendNotification);
    globalGrainSprayLabel.setColour (juce::Label::textColourId, juce::Colour::fromString ("#FFA0A0A0"));
    globalGrainSprayLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (globalGrainSprayLabel);

    // Global Filter Resonance setup
    globalFilterResSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    globalFilterResSlider.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 45, 15);
    globalFilterResSlider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    globalFilterResSlider.setColour (juce::Slider::textBoxTextColourId, juce::Colour::fromString ("#FFA0A0A0"));
    globalFilterResSlider.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour::fromString ("#FFC4A673"));
    globalFilterResSlider.setTextValueSuffix (" Q");
    globalFilterResAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.getAPVTS(), "globalFilterRes", globalFilterResSlider);
    addAndMakeVisible (globalFilterResSlider);
    
    globalFilterResLabel.setText ("FILTER Q", juce::dontSendNotification);
    globalFilterResLabel.setColour (juce::Label::textColourId, juce::Colour::fromString ("#FFA0A0A0"));
    globalFilterResLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (globalFilterResLabel);

    // Global Reverse Chance setup
    globalReverseSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    globalReverseSlider.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 45, 15);
    globalReverseSlider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    globalReverseSlider.setColour (juce::Slider::textBoxTextColourId, juce::Colour::fromString ("#FFA0A0A0"));
    globalReverseSlider.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour::fromString ("#FFC4A673"));
    globalReverseSlider.setTextValueSuffix (" %");
    globalReverseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.getAPVTS(), "globalReverse", globalReverseSlider);
    addAndMakeVisible (globalReverseSlider);
    
    globalReverseLabel.setText ("REVERSE %", juce::dontSendNotification);
    globalReverseLabel.setColour (juce::Label::textColourId, juce::Colour::fromString ("#FFA0A0A0"));
    globalReverseLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (globalReverseLabel);

    // Global Analog Drift setup
    globalAnalogDriftSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    globalAnalogDriftSlider.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 45, 15);
    globalAnalogDriftSlider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    globalAnalogDriftSlider.setColour (juce::Slider::textBoxTextColourId, juce::Colour::fromString ("#FFA0A0A0"));
    globalAnalogDriftSlider.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour::fromString ("#FFC4A673"));
    globalAnalogDriftSlider.setTextValueSuffix (" st");
    globalAnalogDriftAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.getAPVTS(), "globalAnalogDrift", globalAnalogDriftSlider);
    addAndMakeVisible (globalAnalogDriftSlider);

    globalAnalogDriftLabel.setText ("ANALOG DRIFT", juce::dontSendNotification);
    globalAnalogDriftLabel.setColour (juce::Label::textColourId, juce::Colour::fromString ("#FFA0A0A0"));
    globalAnalogDriftLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (globalAnalogDriftLabel);

    // Global Grain Size Jitter setup
    globalGrainSizeJitterSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    globalGrainSizeJitterSlider.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 45, 15);
    globalGrainSizeJitterSlider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    globalGrainSizeJitterSlider.setColour (juce::Slider::textBoxTextColourId, juce::Colour::fromString ("#FFA0A0A0"));
    globalGrainSizeJitterSlider.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour::fromString ("#FFC4A673"));
    globalGrainSizeJitterSlider.setTextValueSuffix ("");
    globalGrainSizeJitterAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.getAPVTS(), "globalGrainSizeJitter", globalGrainSizeJitterSlider);
    addAndMakeVisible (globalGrainSizeJitterSlider);

    globalGrainSizeJitterLabel.setText ("SIZE JITTER", juce::dontSendNotification);
    globalGrainSizeJitterLabel.setColour (juce::Label::textColourId, juce::Colour::fromString ("#FFA0A0A0"));
    globalGrainSizeJitterLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (globalGrainSizeJitterLabel);

    // Global Grain Pitch Jitter setup
    globalGrainPitchJitterSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    globalGrainPitchJitterSlider.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 45, 15);
    globalGrainPitchJitterSlider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    globalGrainPitchJitterSlider.setColour (juce::Slider::textBoxTextColourId, juce::Colour::fromString ("#FFA0A0A0"));
    globalGrainPitchJitterSlider.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour::fromString ("#FFC4A673"));
    globalGrainPitchJitterSlider.setTextValueSuffix (" st");
    globalGrainPitchJitterAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.getAPVTS(), "globalGrainPitchJitter", globalGrainPitchJitterSlider);
    addAndMakeVisible (globalGrainPitchJitterSlider);

    globalGrainPitchJitterLabel.setText ("PITCH JITTER", juce::dontSendNotification);
    globalGrainPitchJitterLabel.setColour (juce::Label::textColourId, juce::Colour::fromString ("#FFA0A0A0"));
    globalGrainPitchJitterLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (globalGrainPitchJitterLabel);

    // Global Stereo Pan Jitter setup
    globalPanJitterSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    globalPanJitterSlider.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 45, 15);
    globalPanJitterSlider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    globalPanJitterSlider.setColour (juce::Slider::textBoxTextColourId, juce::Colour::fromString ("#FFA0A0A0"));
    globalPanJitterSlider.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour::fromString ("#FFC4A673"));
    globalPanJitterSlider.setTextValueSuffix ("");
    globalPanJitterAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.getAPVTS(), "globalPanJitter", globalPanJitterSlider);
    addAndMakeVisible (globalPanJitterSlider);

    globalPanJitterLabel.setText ("PAN JITTER", juce::dontSendNotification);
    globalPanJitterLabel.setColour (juce::Label::textColourId, juce::Colour::fromString ("#FFA0A0A0"));
    globalPanJitterLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (globalPanJitterLabel);

    // Global Filter Sweep Time setup
    globalFilterSweepTimeSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    globalFilterSweepTimeSlider.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 45, 15);
    globalFilterSweepTimeSlider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    globalFilterSweepTimeSlider.setColour (juce::Slider::textBoxTextColourId, juce::Colour::fromString ("#FFA0A0A0"));
    globalFilterSweepTimeSlider.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour::fromString ("#FFC4A673"));
    globalFilterSweepTimeSlider.setTextValueSuffix (" s");
    globalFilterSweepTimeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.getAPVTS(), "globalFilterSweepTime", globalFilterSweepTimeSlider);
    addAndMakeVisible (globalFilterSweepTimeSlider);

    globalFilterSweepTimeLabel.setText ("SWEEP TIME", juce::dontSendNotification);
    globalFilterSweepTimeLabel.setColour (juce::Label::textColourId, juce::Colour::fromString ("#FFA0A0A0"));
    globalFilterSweepTimeLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (globalFilterSweepTimeLabel);

    // Global Pitch Sweep Depth setup
    globalPitchSweepDepthSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    globalPitchSweepDepthSlider.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 45, 15);
    globalPitchSweepDepthSlider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    globalPitchSweepDepthSlider.setColour (juce::Slider::textBoxTextColourId, juce::Colour::fromString ("#FFA0A0A0"));
    globalPitchSweepDepthSlider.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour::fromString ("#FFC4A673"));
    globalPitchSweepDepthSlider.setTextValueSuffix (" st");
    globalPitchSweepDepthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.getAPVTS(), "globalPitchSweepDepth", globalPitchSweepDepthSlider);
    addAndMakeVisible (globalPitchSweepDepthSlider);

    globalPitchSweepDepthLabel.setText ("SWEEP DEPTH", juce::dontSendNotification);
    globalPitchSweepDepthLabel.setColour (juce::Label::textColourId, juce::Colour::fromString ("#FFA0A0A0"));
    globalPitchSweepDepthLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (globalPitchSweepDepthLabel);

    // Global Layer Balance setup
    globalLayerBalanceSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    globalLayerBalanceSlider.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 45, 15);
    globalLayerBalanceSlider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    globalLayerBalanceSlider.setColour (juce::Slider::textBoxTextColourId, juce::Colour::fromString ("#FFA0A0A0"));
    globalLayerBalanceSlider.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour::fromString ("#FFC4A673"));
    globalLayerBalanceSlider.setTextValueSuffix ("");
    globalLayerBalanceAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.getAPVTS(), "globalLayerBalance", globalLayerBalanceSlider);
    addAndMakeVisible (globalLayerBalanceSlider);

    globalLayerBalanceLabel.setText ("LAYER BAL", juce::dontSendNotification);
    globalLayerBalanceLabel.setColour (juce::Label::textColourId, juce::Colour::fromString ("#FFA0A0A0"));
    globalLayerBalanceLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (globalLayerBalanceLabel);

    // Global Layer Delay setup
    globalLayerDelaySlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    globalLayerDelaySlider.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 45, 15);
    globalLayerDelaySlider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    globalLayerDelaySlider.setColour (juce::Slider::textBoxTextColourId, juce::Colour::fromString ("#FFA0A0A0"));
    globalLayerDelaySlider.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour::fromString ("#FFC4A673"));
    globalLayerDelaySlider.setTextValueSuffix (" ms");
    globalLayerDelayAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.getAPVTS(), "globalLayerDelay", globalLayerDelaySlider);
    addAndMakeVisible (globalLayerDelaySlider);

    globalLayerDelayLabel.setText ("LAYER DELAY", juce::dontSendNotification);
    globalLayerDelayLabel.setColour (juce::Label::textColourId, juce::Colour::fromString ("#FFA0A0A0"));
    globalLayerDelayLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (globalLayerDelayLabel);

    // Global Layer Detune setup
    globalLayerDetuneSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    globalLayerDetuneSlider.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 45, 15);
    globalLayerDetuneSlider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    globalLayerDetuneSlider.setColour (juce::Slider::textBoxTextColourId, juce::Colour::fromString ("#FFA0A0A0"));
    globalLayerDetuneSlider.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour::fromString ("#FFC4A673"));
    globalLayerDetuneSlider.setTextValueSuffix (" st");
    globalLayerDetuneAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.getAPVTS(), "globalLayerDetune", globalLayerDetuneSlider);
    addAndMakeVisible (globalLayerDetuneSlider);

    globalLayerDetuneLabel.setText ("LAYER DETUNE", juce::dontSendNotification);
    globalLayerDetuneLabel.setColour (juce::Label::textColourId, juce::Colour::fromString ("#FFA0A0A0"));
    globalLayerDetuneLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (globalLayerDetuneLabel);

    // Global Resonator Mix setup
    globalResonatorMixSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    globalResonatorMixSlider.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 45, 15);
    globalResonatorMixSlider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    globalResonatorMixSlider.setColour (juce::Slider::textBoxTextColourId, juce::Colour::fromString ("#FFA0A0A0"));
    globalResonatorMixSlider.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour::fromString ("#FFC4A673"));
    globalResonatorMixSlider.setTextValueSuffix ("");
    globalResonatorMixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.getAPVTS(), "globalResonatorMix", globalResonatorMixSlider);
    addAndMakeVisible (globalResonatorMixSlider);

    globalResonatorMixLabel.setText ("RESO MIX", juce::dontSendNotification);
    globalResonatorMixLabel.setColour (juce::Label::textColourId, juce::Colour::fromString ("#FFA0A0A0"));
    globalResonatorMixLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (globalResonatorMixLabel);

    // Global Resonator Pitch setup
    globalResonatorPitchSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    globalResonatorPitchSlider.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 45, 15);
    globalResonatorPitchSlider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    globalResonatorPitchSlider.setColour (juce::Slider::textBoxTextColourId, juce::Colour::fromString ("#FFA0A0A0"));
    globalResonatorPitchSlider.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour::fromString ("#FFC4A673"));
    globalResonatorPitchSlider.setTextValueSuffix ("");
    globalResonatorPitchAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.getAPVTS(), "globalResonatorPitch", globalResonatorPitchSlider);
    addAndMakeVisible (globalResonatorPitchSlider);

    globalResonatorPitchLabel.setText ("RESO PITCH", juce::dontSendNotification);
    globalResonatorPitchLabel.setColour (juce::Label::textColourId, juce::Colour::fromString ("#FFA0A0A0"));
    globalResonatorPitchLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (globalResonatorPitchLabel);

    // Global Resonator Feedback setup
    globalResonatorFeedbackSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    globalResonatorFeedbackSlider.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 45, 15);
    globalResonatorFeedbackSlider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    globalResonatorFeedbackSlider.setColour (juce::Slider::textBoxTextColourId, juce::Colour::fromString ("#FFA0A0A0"));
    globalResonatorFeedbackSlider.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour::fromString ("#FFC4A673"));
    globalResonatorFeedbackSlider.setTextValueSuffix ("");
    globalResonatorFeedbackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.getAPVTS(), "globalResonatorFeedback", globalResonatorFeedbackSlider);
    addAndMakeVisible (globalResonatorFeedbackSlider);

    globalResonatorFeedbackLabel.setText ("RESO DECAY", juce::dontSendNotification);
    globalResonatorFeedbackLabel.setColour (juce::Label::textColourId, juce::Colour::fromString ("#FFA0A0A0"));
    globalResonatorFeedbackLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (globalResonatorFeedbackLabel);

    // Master Distortion setup
    distortionEnableButton.setButtonText ("MACKITY");
    distortionEnableButton.setClickingTogglesState (true);
    distortionEnableButton.setColour (juce::TextButton::buttonColourId, juce::Colour::fromString ("#FF252525"));
    distortionEnableButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour::fromString ("#FFAC4444")); // Vintage warm lamp red
    distortionEnableButton.setColour (juce::TextButton::textColourOffId, juce::Colour::fromString ("#FFA0A0A0"));
    distortionEnableButton.setColour (juce::TextButton::textColourOnId, juce::Colour::fromString ("#FFEFEBE0"));
    addAndMakeVisible (distortionEnableButton);
    distortionEnableAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        audioProcessor.getAPVTS(), "distortionEnable", distortionEnableButton);

    distortionDriveSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    distortionDriveSlider.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 32, 12);
    distortionDriveSlider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    distortionDriveSlider.setColour (juce::Slider::textBoxTextColourId, juce::Colour::fromString ("#FFA0A0A0"));
    distortionDriveSlider.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour::fromString ("#FFC4A673"));
    addAndMakeVisible (distortionDriveSlider);
    distortionDriveAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.getAPVTS(), "distortionDrive", distortionDriveSlider);

    distortionOutputSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    distortionOutputSlider.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 32, 12);
    distortionOutputSlider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    distortionOutputSlider.setColour (juce::Slider::textBoxTextColourId, juce::Colour::fromString ("#FFA0A0A0"));
    distortionOutputSlider.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour::fromString ("#FFC4A673"));
    addAndMakeVisible (distortionOutputSlider);
    distortionOutputAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.getAPVTS(), "distortionOutput", distortionOutputSlider);

    distortionPanelLabel.setText ("MACKITY MASTER DSP", juce::dontSendNotification);
    distortionPanelLabel.setColour (juce::Label::textColourId, juce::Colour::fromString ("#FFC4A673"));
    distortionPanelLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (distortionPanelLabel);

    distortionDriveLabel.setText ("DRIVE", juce::dontSendNotification);
    distortionDriveLabel.setColour (juce::Label::textColourId, juce::Colour::fromString ("#FFA0A0A0"));
    distortionDriveLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (distortionDriveLabel);

    distortionOutputLabel.setText ("LEVEL", juce::dontSendNotification);
    distortionOutputLabel.setColour (juce::Label::textColourId, juce::Colour::fromString ("#FFA0A0A0"));
    distortionOutputLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (distortionOutputLabel);
    
    waveformDisplay.onFileDropped = [this] (const juce::File& file)
    {
        // Tell processor to start analysis
        audioProcessor.startAnalysis (file);
    };

    waveformDisplay.onSelectionFinished = [this] (float start, float end)
    {
        if (auto* startParam = audioProcessor.getAPVTS().getParameter ("selectionStart"))
            startParam->setValueNotifyingHost (start);
        if (auto* endParam = audioProcessor.getAPVTS().getParameter ("selectionEnd"))
            endParam->setValueNotifyingHost (end);
            
        audioProcessor.updateActiveSlices();
        updateWaveforms();
    };
    
    padGrid.onPadClicked = [this] (int padIndex)
    {
        audioProcessor.triggerPad (padIndex);
    };
    
    padGrid.onPadRandomizeClicked = [this] (int padIndex) {
        audioProcessor.randomizePad (padIndex);
    };

    setResizable (true, true);
    setResizeLimits (600, 420, 1600, 1120);
    if (auto* constrainer = getConstrainer())
        constrainer->setFixedAspectRatio (1000.0 / 700.0);
        
    int width = 1000;
    int height = 700;
    auto& state = audioProcessor.getAPVTS().state;
    if (state.isValid())
    {
        width = state.getProperty ("winWidth", 1000);
        height = state.getProperty ("winHeight", 700);
    }
    setSize (width, height);

    // LFO Rate Slider
    lfoRateSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    lfoRateSlider.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 45, 15);
    lfoRateSlider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    lfoRateSlider.setColour (juce::Slider::textBoxTextColourId, juce::Colour::fromString ("#FFA0A0A0"));
    lfoRateSlider.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour::fromString ("#FFC4A673"));
    lfoRateSlider.setTextValueSuffix (" Hz");
    addAndMakeVisible (lfoRateSlider);
    lfoRateAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        audioProcessor.getAPVTS(), "globalLfoRate", lfoRateSlider);

    lfoRateLabel.setText ("LFO RATE", juce::dontSendNotification);
    lfoRateLabel.setColour (juce::Label::textColourId, juce::Colour::fromString ("#FFA0A0A0"));
    lfoRateLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (lfoRateLabel);

    // LFO Shape ComboBox
    lfoShapeCombo.addItem ("Sine", 1);
    lfoShapeCombo.addItem ("Triangle", 2);
    lfoShapeCombo.addItem ("Saw", 3);
    lfoShapeCombo.addItem ("Random", 4);
    lfoShapeCombo.setColour (juce::ComboBox::backgroundColourId, juce::Colour::fromString ("#FF252525"));
    lfoShapeCombo.setColour (juce::ComboBox::outlineColourId, juce::Colour::fromString ("#FF444444"));
    lfoShapeCombo.setColour (juce::ComboBox::textColourId, juce::Colour::fromString ("#FFA0A0A0"));
    lfoShapeCombo.setColour (juce::ComboBox::arrowColourId, juce::Colour::fromString ("#FFC4A673"));
    addAndMakeVisible (lfoShapeCombo);
    lfoShapeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        audioProcessor.getAPVTS(), "globalLfoShape", lfoShapeCombo);

    lfoShapeLabel.setText ("LFO SHAPE", juce::dontSendNotification);
    lfoShapeLabel.setColour (juce::Label::textColourId, juce::Colour::fromString ("#FFA0A0A0"));
    lfoShapeLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (lfoShapeLabel);

    // Modulation Matrix (4 Slots)
    for (int i = 0; i < 4; ++i)
    {
        juce::String slotNum = juce::String (i + 1);

        // Label
        modSlotLabel[i].setText ("SLOT " + slotNum, juce::dontSendNotification);
        modSlotLabel[i].setColour (juce::Label::textColourId, juce::Colour::fromString ("#FFA0A0A0"));
        modSlotLabel[i].setJustificationType (juce::Justification::centredLeft);
        addAndMakeVisible (modSlotLabel[i]);

        // Source Combo
        modSrcCombo[i].addItem ("Off", 1);
        modSrcCombo[i].addItem ("LFO", 2);
        modSrcCombo[i].addItem ("Velocity", 3);
        modSrcCombo[i].addItem ("Random", 4);
        modSrcCombo[i].setColour (juce::ComboBox::backgroundColourId, juce::Colour::fromString ("#FF252525"));
        modSrcCombo[i].setColour (juce::ComboBox::outlineColourId, juce::Colour::fromString ("#FF444444"));
        modSrcCombo[i].setColour (juce::ComboBox::textColourId, juce::Colour::fromString ("#FFA0A0A0"));
        modSrcCombo[i].setColour (juce::ComboBox::arrowColourId, juce::Colour::fromString ("#FFC4A673"));
        addAndMakeVisible (modSrcCombo[i]);
        modSrcAttachment[i] = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
            audioProcessor.getAPVTS(), "modSrc" + slotNum, modSrcCombo[i]);

        // Destination Combo
        modDstCombo[i].addItem ("Off", 1);
        modDstCombo[i].addItem ("Pitch", 2);
        modDstCombo[i].addItem ("Decay", 3);
        modDstCombo[i].addItem ("Grain Size", 4);
        modDstCombo[i].addItem ("Filter Cutoff", 5);
        modDstCombo[i].addItem ("Reso Pitch", 6);
        modDstCombo[i].addItem ("Reso Mix", 7);
        modDstCombo[i].setColour (juce::ComboBox::backgroundColourId, juce::Colour::fromString ("#FF252525"));
        modDstCombo[i].setColour (juce::ComboBox::outlineColourId, juce::Colour::fromString ("#FF444444"));
        modDstCombo[i].setColour (juce::ComboBox::textColourId, juce::Colour::fromString ("#FFA0A0A0"));
        modDstCombo[i].setColour (juce::ComboBox::arrowColourId, juce::Colour::fromString ("#FFC4A673"));
        addAndMakeVisible (modDstCombo[i]);
        modDstAttachment[i] = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
            audioProcessor.getAPVTS(), "modDst" + slotNum, modDstCombo[i]);

        // Depth Slider
        modDepthSlider[i].setSliderStyle (juce::Slider::LinearHorizontal);
        modDepthSlider[i].setTextBoxStyle (juce::Slider::TextBoxLeft, false, 40, 15);
        modDepthSlider[i].setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        modDepthSlider[i].setColour (juce::Slider::textBoxTextColourId, juce::Colour::fromString ("#FFA0A0A0"));
        modDepthSlider[i].setColour (juce::Slider::trackColourId, juce::Colour::fromString ("#FFC4A673"));
        modDepthSlider[i].setColour (juce::Slider::thumbColourId, juce::Colour::fromString ("#FFC4A673"));
        addAndMakeVisible (modDepthSlider[i]);
        modDepthAttachment[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            audioProcessor.getAPVTS(), "modDepth" + slotNum, modDepthSlider[i]);
    }

    // Initialise UI with current state if processor already has data
    updateWaveforms();
    
    setTabActive (Tab::sampler);
    
    startTimerHz (30);
}

GranularDrumsEditor::~GranularDrumsEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void GranularDrumsEditor::timerCallback()
{
    waveformDisplay.setAnalysisState (audioProcessor.isAnalyzing(), audioProcessor.getAnalysisProgress());
    
    // Update playing highlight on pads
    for (int i = 0; i < 16; ++i)
        padGrid.setPadPlaying (i, audioProcessor.isPadPlaying (i));
}

void GranularDrumsEditor::paint (juce::Graphics& g)
{
    float scale = (float) getWidth() / 1000.0f;

    // Matte flat black background
    g.fillAll (juce::Colour::fromString("#FF0C0C0C"));
    
    // Header text (scaled dynamically)
    g.setColour (juce::Colour::fromString("#FFEFEBE0"));
    g.setFont (juce::FontOptions(24.0f * scale).withStyle("Bold"));
    g.drawText ("Granular Drums", juce::roundToInt (20.0f * scale), juce::roundToInt (10.0f * scale), 
                juce::roundToInt (170.0f * scale), juce::roundToInt (30.0f * scale), 
                juce::Justification::centredLeft, true);
    
    int numOutChannels = audioProcessor.getTotalNumOutputChannels();
    juce::String channelConfig = (numOutChannels == 1) ? "MONO" : "STEREO";
    
    g.setColour (juce::Colour::fromString("#FFFF9F1C")); // Gold accent
    g.setFont (juce::FontOptions(12.0f * scale).withStyle("Bold"));
    g.drawText (channelConfig, juce::roundToInt (205.0f * scale), juce::roundToInt (17.0f * scale), 
                juce::roundToInt (80.0f * scale), juce::roundToInt (20.0f * scale), 
                juce::Justification::centredLeft, true);

    // Draw plates and labels if we are on the ENGINE tab
    if (activeTab == Tab::engine)
    {
        int headerH = juce::roundToInt (45.0f * scale);
        
        int contentW = getWidth() - juce::roundToInt (40.0f * scale); // 20px padding left/right
        int contentH = getHeight() - headerH - juce::roundToInt (30.0f * scale); // 10px top, 20px bottom padding
        int contentX = juce::roundToInt (20.0f * scale);
        int contentY = headerH + juce::roundToInt (10.0f * scale);
        
        int panelWidth = (contentW - juce::roundToInt (20.0f * scale)) / 2; // 20px gap
        
        // Draw left plate (Macros)
        int leftPanelX = contentX;
        g.setColour (juce::Colour::fromString ("#FF141414")); // Recessed plate dark charcoal
        g.fillRoundedRectangle ((float)leftPanelX, (float)contentY, (float)panelWidth, (float)contentH, 8.0f * scale);
        g.setColour (juce::Colour::fromString ("#FF222222")); // Fine border
        g.drawRoundedRectangle ((float)leftPanelX, (float)contentY, (float)panelWidth, (float)contentH, 8.0f * scale, 1.5f * scale);
        
        // Draw right plate (Mackity DSP)
        int rightPanelX = leftPanelX + panelWidth + juce::roundToInt (20.0f * scale);
        g.setColour (juce::Colour::fromString ("#FF141414"));
        g.fillRoundedRectangle ((float)rightPanelX, (float)contentY, (float)panelWidth, (float)contentH, 8.0f * scale);
        g.setColour (juce::Colour::fromString ("#FF222222"));
        g.drawRoundedRectangle ((float)rightPanelX, (float)contentY, (float)panelWidth, (float)contentH, 8.0f * scale, 1.5f * scale);
        
        // Section titles
        g.setColour (juce::Colour::fromString ("#FFC4A673")); // Gold/brass accent
        g.setFont (juce::FontOptions (16.0f * scale).withStyle ("Bold"));
        
        g.drawText ("ENGINE CONTROLS", leftPanelX, contentY + juce::roundToInt (20.0f * scale), panelWidth, juce::roundToInt (25.0f * scale),
                    juce::Justification::centred, true);

        // Right sub-section titles
        g.setFont (juce::FontOptions (13.0f * scale).withStyle ("Bold"));
        g.drawText ("MACKITY MASTER DSP", rightPanelX, contentY + juce::roundToInt (25.0f * scale), panelWidth, juce::roundToInt (35.0f * scale),
                    juce::Justification::centred, true);
                    
        g.drawText ("MODULATOR (LFO)", rightPanelX, contentY + juce::roundToInt (140.0f * scale), panelWidth, juce::roundToInt (25.0f * scale),
                    juce::Justification::centred, true);

        g.drawText ("MODULATION MATRIX", rightPanelX, contentY + juce::roundToInt (240.0f * scale), panelWidth, juce::roundToInt (25.0f * scale),
                    juce::Justification::centred, true);
    }
}

void GranularDrumsEditor::resized()
{
    int w = getWidth();
    int h = getHeight();
    // Only update stored size if the editor is actively showing on screen,
    // to prevent initial host layout/binding calls from overwriting the stored size.
    if (isShowing() && w >= 600 && h >= 420)
    {
        auto& state = audioProcessor.getAPVTS().state;
        if (state.isValid())
        {
            state.setProperty ("winWidth", w, nullptr);
            state.setProperty ("winHeight", h, nullptr);
        }
    }

    float scale = (float) getWidth() / 1000.0f;
    auto bounds = getLocalBounds();
    
    // Header space layout
    int headerH = juce::roundToInt (45.0f * scale);
    auto headerBounds = bounds.removeFromTop (headerH);
    
    int tabW = juce::roundToInt (90.0f * scale);
    int tabH = juce::roundToInt (24.0f * scale);
    int gap = juce::roundToInt (10.0f * scale);
    
    headerBounds.removeFromRight (juce::roundToInt (20.0f * scale));
    
    auto r1 = headerBounds.removeFromRight (tabW).removeFromTop (tabH);
    engineTabButton.setBounds (r1.translated (0, (headerH - tabH) / 2));
    
    headerBounds.removeFromRight (gap);
    
    auto r2 = headerBounds.removeFromRight (tabW).removeFromTop (tabH);
    sequencerTabButton.setBounds (r2.translated (0, (headerH - tabH) / 2));
    
    headerBounds.removeFromRight (gap);
    
    auto r3 = headerBounds.removeFromRight (tabW).removeFromTop (tabH);
    samplerTabButton.setBounds (r3.translated (0, (headerH - tabH) / 2));

    // Handle visible layouts depending on active tab
    if (activeTab == Tab::sequencer)
    {
        sequencerGrid.setBounds (bounds.reduced (juce::roundToInt (20.0f * scale), juce::roundToInt (10.0f * scale)));
    }
    else if (activeTab == Tab::sampler)
    {
        // WaveformDisplay takes 35% of the remaining height
        waveformDisplay.setBounds (bounds.removeFromTop (juce::roundToInt (bounds.getHeight() * 0.35f))
                                         .reduced (juce::roundToInt (20.0f * scale), 0));
        
        // Spacing
        bounds.removeFromTop (juce::roundToInt (10.0f * scale));
        
        // Middle row containing macros and randomize button
        auto middleRow = bounds.removeFromTop (juce::roundToInt (35.0f * scale));
        int colW = middleRow.getWidth() / 3;
        
        // Left: Pitch Macro
        auto pitchMacroArea = middleRow.removeFromLeft (colW).reduced (juce::roundToInt (10.0f * scale), 0);
        globalPitchLabel.setJustificationType (juce::Justification::centredRight);
        globalPitchLabel.setFont (juce::FontOptions (10.0f * scale).withStyle ("Bold"));
        globalPitchLabel.setBounds (pitchMacroArea.removeFromLeft (juce::roundToInt (75.0f * scale)));
        globalPitchSlider.setBounds (pitchMacroArea);
        
        // Right: Decay Macro
        auto decayMacroArea = middleRow.removeFromRight (colW).reduced (juce::roundToInt (10.0f * scale), 0);
        globalDecayLabel.setJustificationType (juce::Justification::centredRight);
        globalDecayLabel.setFont (juce::FontOptions (10.0f * scale).withStyle ("Bold"));
        globalDecayLabel.setBounds (decayMacroArea.removeFromLeft (juce::roundToInt (75.0f * scale)));
        globalDecaySlider.setBounds (decayMacroArea);
        
        // Center: Randomize button
        globalRandomButton.setBounds (middleRow.reduced (juce::roundToInt (10.0f * scale), juce::roundToInt (2.0f * scale)));
        
        // Spacing
        bounds.removeFromTop (juce::roundToInt (10.0f * scale));
        
        // PadGrid takes the remaining space
        padGrid.setBounds (bounds.reduced (juce::roundToInt (20.0f * scale), 0));
    }
    else if (activeTab == Tab::engine)
    {
        // Engine Tab Layout
        int contentW = bounds.getWidth() - juce::roundToInt (40.0f * scale); // 20px padding left/right
        int contentH = bounds.getHeight() - juce::roundToInt (30.0f * scale); // 10px top, 20px bottom padding
        int contentX = bounds.getX() + juce::roundToInt (20.0f * scale);
        int contentY = bounds.getY() + juce::roundToInt (10.0f * scale);
        
        int panelWidth = (contentW - juce::roundToInt (20.0f * scale)) / 2; // 20px gap
        
        // Left Panel bounds (Macros)
        juce::Rectangle<int> leftPanelBounds (contentX, contentY, panelWidth, contentH);
        
        // Right Panel bounds (Mackity DSP)
        juce::Rectangle<int> rightPanelBounds (contentX + panelWidth + juce::roundToInt (20.0f * scale), contentY, panelWidth, contentH);
        
        // --- Left Panel Layout (Macros) ---
        auto leftArea = leftPanelBounds.reduced (juce::roundToInt (25.0f * scale));
        leftArea.removeFromTop (juce::roundToInt (35.0f * scale)); // spacer for title
        
        // Randomize button at the bottom
        auto btnArea = leftArea.removeFromBottom (juce::roundToInt (60.0f * scale));
        int btnW = juce::roundToInt (200.0f * scale);
        int btnH = juce::roundToInt (36.0f * scale);
        globalRandomButton.setBounds (btnArea.getX() + (btnArea.getWidth() - btnW) / 2,
                                      btnArea.getY() + (btnArea.getHeight() - btnH) / 2,
                                      btnW, btnH);
                                      
        // Spacing
        leftArea.removeFromBottom (juce::roundToInt (10.0f * scale));
        
        // 6 rows of knobs
        int rowH = leftArea.getHeight() / 6;
        int colW = leftArea.getWidth() / 3;
        
        // Row 1: Pitch | Decay | Analog Drift
        auto row1 = leftArea.removeFromTop (rowH);
        auto r1c1 = row1.removeFromLeft (colW).reduced (juce::roundToInt (8.0f * scale), juce::roundToInt (3.0f * scale));
        auto r1c2 = row1.removeFromLeft (colW).reduced (juce::roundToInt (8.0f * scale), juce::roundToInt (3.0f * scale));
        auto r1c3 = row1.reduced (juce::roundToInt (8.0f * scale), juce::roundToInt (3.0f * scale));
        
        globalPitchLabel.setJustificationType (juce::Justification::centred);
        globalPitchLabel.setFont (juce::FontOptions (9.0f * scale).withStyle ("Bold"));
        globalPitchLabel.setBounds (r1c1.removeFromTop (juce::roundToInt (16.0f * scale)));
        globalPitchSlider.setBounds (r1c1);
        
        globalDecayLabel.setJustificationType (juce::Justification::centred);
        globalDecayLabel.setFont (juce::FontOptions (9.0f * scale).withStyle ("Bold"));
        globalDecayLabel.setBounds (r1c2.removeFromTop (juce::roundToInt (16.0f * scale)));
        globalDecaySlider.setBounds (r1c2);

        globalAnalogDriftLabel.setJustificationType (juce::Justification::centred);
        globalAnalogDriftLabel.setFont (juce::FontOptions (9.0f * scale).withStyle ("Bold"));
        globalAnalogDriftLabel.setBounds (r1c3.removeFromTop (juce::roundToInt (16.0f * scale)));
        globalAnalogDriftSlider.setBounds (r1c3);
        
        // Row 2: Grain Size | Grain Spray | Reverse Chance
        auto row2 = leftArea.removeFromTop (rowH);
        auto r2c1 = row2.removeFromLeft (colW).reduced (juce::roundToInt (8.0f * scale), juce::roundToInt (3.0f * scale));
        auto r2c2 = row2.removeFromLeft (colW).reduced (juce::roundToInt (8.0f * scale), juce::roundToInt (3.0f * scale));
        auto r2c3 = row2.reduced (juce::roundToInt (8.0f * scale), juce::roundToInt (3.0f * scale));
        
        globalGrainSizeLabel.setJustificationType (juce::Justification::centred);
        globalGrainSizeLabel.setFont (juce::FontOptions (9.0f * scale).withStyle ("Bold"));
        globalGrainSizeLabel.setBounds (r2c1.removeFromTop (juce::roundToInt (16.0f * scale)));
        globalGrainSizeSlider.setBounds (r2c1);
        
        globalGrainSprayLabel.setJustificationType (juce::Justification::centred);
        globalGrainSprayLabel.setFont (juce::FontOptions (9.0f * scale).withStyle ("Bold"));
        globalGrainSprayLabel.setBounds (r2c2.removeFromTop (juce::roundToInt (16.0f * scale)));
        globalGrainSpraySlider.setBounds (r2c2);

        globalReverseLabel.setJustificationType (juce::Justification::centred);
        globalReverseLabel.setFont (juce::FontOptions (9.0f * scale).withStyle ("Bold"));
        globalReverseLabel.setBounds (r2c3.removeFromTop (juce::roundToInt (16.0f * scale)));
        globalReverseSlider.setBounds (r2c3);
        
        // Row 3: Grain Size Jitter | Grain Pitch Jitter | Stereo Pan Jitter
        auto row3 = leftArea.removeFromTop (rowH);
        auto r3c1 = row3.removeFromLeft (colW).reduced (juce::roundToInt (8.0f * scale), juce::roundToInt (3.0f * scale));
        auto r3c2 = row3.removeFromLeft (colW).reduced (juce::roundToInt (8.0f * scale), juce::roundToInt (3.0f * scale));
        auto r3c3 = row3.reduced (juce::roundToInt (8.0f * scale), juce::roundToInt (3.0f * scale));
        
        globalGrainSizeJitterLabel.setJustificationType (juce::Justification::centred);
        globalGrainSizeJitterLabel.setFont (juce::FontOptions (9.0f * scale).withStyle ("Bold"));
        globalGrainSizeJitterLabel.setBounds (r3c1.removeFromTop (juce::roundToInt (16.0f * scale)));
        globalGrainSizeJitterSlider.setBounds (r3c1);

        globalGrainPitchJitterLabel.setJustificationType (juce::Justification::centred);
        globalGrainPitchJitterLabel.setFont (juce::FontOptions (9.0f * scale).withStyle ("Bold"));
        globalGrainPitchJitterLabel.setBounds (r3c2.removeFromTop (juce::roundToInt (16.0f * scale)));
        globalGrainPitchJitterSlider.setBounds (r3c2);

        globalPanJitterLabel.setJustificationType (juce::Justification::centred);
        globalPanJitterLabel.setFont (juce::FontOptions (9.0f * scale).withStyle ("Bold"));
        globalPanJitterLabel.setBounds (r3c3.removeFromTop (juce::roundToInt (16.0f * scale)));
        globalPanJitterSlider.setBounds (r3c3);
        
        // Row 4: Filter Q | Filter Sweep Time | Pitch Sweep Depth
        auto row4 = leftArea.removeFromTop (rowH);
        auto r4c1 = row4.removeFromLeft (colW).reduced (juce::roundToInt (8.0f * scale), juce::roundToInt (3.0f * scale));
        auto r4c2 = row4.removeFromLeft (colW).reduced (juce::roundToInt (8.0f * scale), juce::roundToInt (3.0f * scale));
        auto r4c3 = row4.reduced (juce::roundToInt (8.0f * scale), juce::roundToInt (3.0f * scale));
        
        globalFilterResLabel.setJustificationType (juce::Justification::centred);
        globalFilterResLabel.setFont (juce::FontOptions (9.0f * scale).withStyle ("Bold"));
        globalFilterResLabel.setBounds (r4c1.removeFromTop (juce::roundToInt (16.0f * scale)));
        globalFilterResSlider.setBounds (r4c1);

        globalFilterSweepTimeLabel.setJustificationType (juce::Justification::centred);
        globalFilterSweepTimeLabel.setFont (juce::FontOptions (9.0f * scale).withStyle ("Bold"));
        globalFilterSweepTimeLabel.setBounds (r4c2.removeFromTop (juce::roundToInt (16.0f * scale)));
        globalFilterSweepTimeSlider.setBounds (r4c2);

        globalPitchSweepDepthLabel.setJustificationType (juce::Justification::centred);
        globalPitchSweepDepthLabel.setFont (juce::FontOptions (9.0f * scale).withStyle ("Bold"));
        globalPitchSweepDepthLabel.setBounds (r4c3.removeFromTop (juce::roundToInt (16.0f * scale)));
        globalPitchSweepDepthSlider.setBounds (r4c3);

        // Row 5: Layer Balance | Layer Delay | Layer Detune
        auto row5 = leftArea.removeFromTop (rowH);
        auto r5c1 = row5.removeFromLeft (colW).reduced (juce::roundToInt (8.0f * scale), juce::roundToInt (3.0f * scale));
        auto r5c2 = row5.removeFromLeft (colW).reduced (juce::roundToInt (8.0f * scale), juce::roundToInt (3.0f * scale));
        auto r5c3 = row5.reduced (juce::roundToInt (8.0f * scale), juce::roundToInt (3.0f * scale));

        globalLayerBalanceLabel.setJustificationType (juce::Justification::centred);
        globalLayerBalanceLabel.setFont (juce::FontOptions (9.0f * scale).withStyle ("Bold"));
        globalLayerBalanceLabel.setBounds (r5c1.removeFromTop (juce::roundToInt (16.0f * scale)));
        globalLayerBalanceSlider.setBounds (r5c1);

        globalLayerDelayLabel.setJustificationType (juce::Justification::centred);
        globalLayerDelayLabel.setFont (juce::FontOptions (9.0f * scale).withStyle ("Bold"));
        globalLayerDelayLabel.setBounds (r5c2.removeFromTop (juce::roundToInt (16.0f * scale)));
        globalLayerDelaySlider.setBounds (r5c2);

        globalLayerDetuneLabel.setJustificationType (juce::Justification::centred);
        globalLayerDetuneLabel.setFont (juce::FontOptions (9.0f * scale).withStyle ("Bold"));
        globalLayerDetuneLabel.setBounds (r5c3.removeFromTop (juce::roundToInt (16.0f * scale)));
        globalLayerDetuneSlider.setBounds (r5c3);

        // Row 6: Resonator Mix | Resonator Pitch | Resonator Decay
        auto row6 = leftArea;
        auto r6c1 = row6.removeFromLeft (colW).reduced (juce::roundToInt (8.0f * scale), juce::roundToInt (3.0f * scale));
        auto r6c2 = row6.removeFromLeft (colW).reduced (juce::roundToInt (8.0f * scale), juce::roundToInt (3.0f * scale));
        auto r6c3 = row6.reduced (juce::roundToInt (8.0f * scale), juce::roundToInt (3.0f * scale));

        globalResonatorMixLabel.setJustificationType (juce::Justification::centred);
        globalResonatorMixLabel.setFont (juce::FontOptions (9.0f * scale).withStyle ("Bold"));
        globalResonatorMixLabel.setBounds (r6c1.removeFromTop (juce::roundToInt (16.0f * scale)));
        globalResonatorMixSlider.setBounds (r6c1);

        globalResonatorPitchLabel.setJustificationType (juce::Justification::centred);
        globalResonatorPitchLabel.setFont (juce::FontOptions (9.0f * scale).withStyle ("Bold"));
        globalResonatorPitchLabel.setBounds (r6c2.removeFromTop (juce::roundToInt (16.0f * scale)));
        globalResonatorPitchSlider.setBounds (r6c2);

        globalResonatorFeedbackLabel.setJustificationType (juce::Justification::centred);
        globalResonatorFeedbackLabel.setFont (juce::FontOptions (9.0f * scale).withStyle ("Bold"));
        globalResonatorFeedbackLabel.setBounds (r6c3.removeFromTop (juce::roundToInt (16.0f * scale)));
        globalResonatorFeedbackSlider.setBounds (r6c3);
        
        // --- Right Panel Layout (Mackity DSP, Modulator, Matrix) ---
        auto rightArea = rightPanelBounds.reduced (juce::roundToInt (25.0f * scale));
        
        // 1. MACKITY section
        rightArea.removeFromTop (juce::roundToInt (35.0f * scale)); // spacer for title
        auto mackityArea = rightArea.removeFromTop (juce::roundToInt (80.0f * scale));
        int colW3 = mackityArea.getWidth() / 3;
        auto rEnable = mackityArea.removeFromLeft (colW3).reduced (juce::roundToInt (5.0f * scale));
        auto rDrive = mackityArea.removeFromLeft (colW3).reduced (juce::roundToInt (5.0f * scale));
        auto rLevel = mackityArea.reduced (juce::roundToInt (5.0f * scale));
        
        int enableBtnH = juce::roundToInt (28.0f * scale);
        distortionEnableButton.setBounds (rEnable.getX(), rEnable.getY() + (rEnable.getHeight() - enableBtnH) / 2, rEnable.getWidth(), enableBtnH);
        
        distortionDriveLabel.setFont (juce::FontOptions (9.0f * scale).withStyle ("Bold"));
        distortionDriveLabel.setBounds (rDrive.removeFromTop (juce::roundToInt (16.0f * scale)));
        distortionDriveSlider.setBounds (rDrive);
        
        distortionOutputLabel.setFont (juce::FontOptions (9.0f * scale).withStyle ("Bold"));
        distortionOutputLabel.setBounds (rLevel.removeFromTop (juce::roundToInt (16.0f * scale)));
        distortionOutputSlider.setBounds (rLevel);
        
        // 2. LFO Modulator section
        rightArea.removeFromTop (juce::roundToInt (25.0f * scale)); // spacer for title
        auto lfoArea = rightArea.removeFromTop (juce::roundToInt (75.0f * scale));
        int colW2 = lfoArea.getWidth() / 2;
        auto rRate = lfoArea.removeFromLeft (colW2).reduced (juce::roundToInt (10.0f * scale), juce::roundToInt (5.0f * scale));
        auto rShape = lfoArea.reduced (juce::roundToInt (10.0f * scale), juce::roundToInt (5.0f * scale));
        
        lfoRateLabel.setFont (juce::FontOptions (9.0f * scale).withStyle ("Bold"));
        lfoRateLabel.setBounds (rRate.removeFromTop (juce::roundToInt (16.0f * scale)));
        lfoRateSlider.setBounds (rRate);
        
        lfoShapeLabel.setFont (juce::FontOptions (9.0f * scale).withStyle ("Bold"));
        lfoShapeLabel.setBounds (rShape.removeFromTop (juce::roundToInt (16.0f * scale)));
        int comboH = juce::roundToInt (24.0f * scale);
        lfoShapeCombo.setBounds (rShape.getX(), rShape.getY() + (rShape.getHeight() - comboH) / 2, rShape.getWidth(), comboH);
        
        // 3. Modulation Matrix section
        rightArea.removeFromTop (juce::roundToInt (25.0f * scale)); // spacer for title
        int slotH = rightArea.getHeight() / 4;
        for (int i = 0; i < 4; ++i)
        {
            auto slotArea = rightArea.removeFromTop (slotH).reduced (0, juce::roundToInt (2.0f * scale));
            
            int lblW = juce::roundToInt (45.0f * scale);
            int srcW = juce::roundToInt (100.0f * scale);
            int dstW = juce::roundToInt (110.0f * scale);
            
            auto rLbl = slotArea.removeFromLeft (lblW);
            slotArea.removeFromLeft (juce::roundToInt (5.0f * scale));
            auto rSrc = slotArea.removeFromLeft (srcW);
            slotArea.removeFromLeft (juce::roundToInt (5.0f * scale));
            auto rDst = slotArea.removeFromLeft (dstW);
            slotArea.removeFromLeft (juce::roundToInt (5.0f * scale));
            auto rDepth = slotArea;
            
            modSlotLabel[i].setFont (juce::FontOptions (10.0f * scale).withStyle ("Bold"));
            modSlotLabel[i].setBounds (rLbl);
            
            int slotComboH = juce::roundToInt (22.0f * scale);
            modSrcCombo[i].setBounds (rSrc.getX(), rSrc.getY() + (rSrc.getHeight() - slotComboH) / 2, rSrc.getWidth(), slotComboH);
            modDstCombo[i].setBounds (rDst.getX(), rDst.getY() + (rDst.getHeight() - slotComboH) / 2, rDst.getWidth(), slotComboH);
            
            modDepthSlider[i].setBounds (rDepth);
        }
    }
}

void GranularDrumsEditor::setTabActive (Tab newTab)
{
    activeTab = newTab;
    
    samplerTabButton.setToggleState (activeTab == Tab::sampler, juce::dontSendNotification);
    sequencerTabButton.setToggleState (activeTab == Tab::sequencer, juce::dontSendNotification);
    engineTabButton.setToggleState (activeTab == Tab::engine, juce::dontSendNotification);
    
    // Sampler components visibility
    bool isSampler = (activeTab == Tab::sampler);
    waveformDisplay.setVisible (isSampler);
    padGrid.setVisible (isSampler);
    
    // Sequencer components visibility
    bool isSequencer = (activeTab == Tab::sequencer);
    sequencerGrid.setVisible (isSequencer);
    
    // Engine components visibility
    bool isEngine = (activeTab == Tab::engine);
    bool showMacrosAndRandom = (activeTab == Tab::sampler || activeTab == Tab::engine);
    globalRandomButton.setVisible (showMacrosAndRandom);
    globalPitchSlider.setVisible (showMacrosAndRandom);
    globalDecaySlider.setVisible (showMacrosAndRandom);
    globalPitchLabel.setVisible (showMacrosAndRandom);
    globalDecayLabel.setVisible (showMacrosAndRandom);
    
    globalGrainSizeSlider.setVisible (isEngine);
    globalGrainSizeLabel.setVisible (isEngine);
    globalGrainSpraySlider.setVisible (isEngine);
    globalGrainSprayLabel.setVisible (isEngine);
    globalFilterResSlider.setVisible (isEngine);
    globalFilterResLabel.setVisible (isEngine);
    globalReverseSlider.setVisible (isEngine);
    globalReverseLabel.setVisible (isEngine);
    
    globalAnalogDriftSlider.setVisible (isEngine);
    globalAnalogDriftLabel.setVisible (isEngine);
    globalGrainSizeJitterSlider.setVisible (isEngine);
    globalGrainSizeJitterLabel.setVisible (isEngine);
    globalGrainPitchJitterSlider.setVisible (isEngine);
    globalGrainPitchJitterLabel.setVisible (isEngine);
    globalPanJitterSlider.setVisible (isEngine);
    globalPanJitterLabel.setVisible (isEngine);
    globalFilterSweepTimeSlider.setVisible (isEngine);
    globalFilterSweepTimeLabel.setVisible (isEngine);
    globalPitchSweepDepthSlider.setVisible (isEngine);
    globalPitchSweepDepthLabel.setVisible (isEngine);
    
    globalLayerBalanceSlider.setVisible (isEngine);
    globalLayerBalanceLabel.setVisible (isEngine);
    globalLayerDelaySlider.setVisible (isEngine);
    globalLayerDelayLabel.setVisible (isEngine);
    globalLayerDetuneSlider.setVisible (isEngine);
    globalLayerDetuneLabel.setVisible (isEngine);
    globalResonatorMixSlider.setVisible (isEngine);
    globalResonatorMixLabel.setVisible (isEngine);
    globalResonatorPitchSlider.setVisible (isEngine);
    globalResonatorPitchLabel.setVisible (isEngine);
    globalResonatorFeedbackSlider.setVisible (isEngine);
    globalResonatorFeedbackLabel.setVisible (isEngine);
    
    distortionEnableButton.setVisible (isEngine);
    distortionDriveSlider.setVisible (isEngine);
    distortionOutputSlider.setVisible (isEngine);
    distortionPanelLabel.setVisible (false); // Always hidden, drawn directly in paint()
    distortionDriveLabel.setVisible (isEngine);
    distortionOutputLabel.setVisible (isEngine);
    
    lfoRateSlider.setVisible (isEngine);
    lfoRateLabel.setVisible (isEngine);
    lfoShapeCombo.setVisible (isEngine);
    lfoShapeLabel.setVisible (isEngine);

    for (int i = 0; i < 4; ++i)
    {
        modSrcCombo[i].setVisible (isEngine);
        modDstCombo[i].setVisible (isEngine);
        modDepthSlider[i].setVisible (isEngine);
        modSlotLabel[i].setVisible (isEngine);
    }
    
    resized();
    repaint();
}

void GranularDrumsEditor::updateWaveforms()
{
    const auto& buffer = audioProcessor.getCurrentBuffer();
    const auto& allMarkers = audioProcessor.getCurrentMarkers();
    const auto& activeMarkers = audioProcessor.getActiveMarkers();
    
    float startRatio = audioProcessor.getAPVTS().getRawParameterValue ("selectionStart")->load();
    float endRatio = audioProcessor.getAPVTS().getRawParameterValue ("selectionEnd")->load();
    
    waveformDisplay.setSelectionRatios (startRatio, endRatio);
    waveformDisplay.setWaveformData (&buffer, &allMarkers);
    
    int numSamplesInFile = buffer.getNumSamples();
    int selectionEndSample = juce::roundToInt (endRatio * numSamplesInFile);
    
    for (int i = 0; i < 16; ++i)
    {
        int mIdx = audioProcessor.getMarkerIndexForPad (i);
        if (mIdx >= 0 && (size_t)mIdx < activeMarkers.size())
        {
            int startSample = activeMarkers[(size_t)mIdx].sampleIndex;
            int endSample = selectionEndSample;
            if (activeMarkers[(size_t)mIdx].lengthInSamples > 0)
                endSample = startSample + activeMarkers[(size_t)mIdx].lengthInSamples;
            else if ((size_t)(mIdx + 1) < activeMarkers.size())
                endSample = activeMarkers[(size_t)(mIdx + 1)].sampleIndex;
            endSample = juce::jlimit (startSample, buffer.getNumSamples(), endSample);
                
            int numSamples = endSample - startSample;
            padGrid.setPadWaveformData (i, &buffer, startSample, numSamples, activeMarkers[(size_t)mIdx].category);
        }
        else
        {
            padGrid.setPadWaveformData (i, nullptr, 0, 0, "");
        }
    }
}

bool GranularDrumsEditor::isInterestedInFileDrag (const juce::StringArray& files)
{
    return waveformDisplay.isInterestedInFileDrag (files);
}

void GranularDrumsEditor::fileDragEnter (const juce::StringArray& files, int x, int y)
{
    // Convert coordinates to waveformDisplay's local space
    auto localPt = waveformDisplay.getLocalPoint (this, juce::Point<int> (x, y));
    waveformDisplay.fileDragEnter (files, localPt.x, localPt.y);
}

void GranularDrumsEditor::fileDragExit (const juce::StringArray& files)
{
    waveformDisplay.fileDragExit (files);
}

void GranularDrumsEditor::filesDropped (const juce::StringArray& files, int x, int y)
{
    auto localPt = waveformDisplay.getLocalPoint (this, juce::Point<int> (x, y));
    waveformDisplay.filesDropped (files, localPt.x, localPt.y);
}
