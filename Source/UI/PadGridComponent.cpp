#include "PadGridComponent.h"

#include "../PluginProcessor.h"

class PadGridComponent::PadComponent : public juce::Component
{
public:
    PadComponent (int idx, GranularDrumsProcessor& processor, std::function<void(int)> onClickCb, std::function<void(int)> onRandomizeCb)
        : index (idx), onClick (onClickCb), onRandomize (onRandomizeCb)
    {
        pitchSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        pitchSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        
        pitchAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            processor.getAPVTS(), "pitch" + juce::String (idx), pitchSlider);
            
        addAndMakeVisible (pitchSlider);
        pitchSlider.onValueChange = [this] { repaint(); };
        pitchSlider.setVisible (false);

        decaySlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        decaySlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        
        decayAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            processor.getAPVTS(), "decay" + juce::String (idx), decaySlider);
            
        addAndMakeVisible (decaySlider);
        decaySlider.onValueChange = [this] { repaint(); };
        decaySlider.setVisible (false);

        monoButton.setButtonText ("MONO");
        monoButton.setClickingTogglesState (true);
        monoButton.setColour (juce::TextButton::buttonColourId, juce::Colour::fromString ("#FF222222"));
        monoButton.setColour (juce::TextButton::textColourOffId, juce::Colour::fromString ("#FFA0A0A0"));
        monoButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour::fromString ("#FFFF9F1C"));
        monoButton.setColour (juce::TextButton::textColourOnId, juce::Colour::fromString ("#FF0C0C0C"));
        
        monoAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
            processor.getAPVTS(), "mono" + juce::String (idx), monoButton);
            
        addAndMakeVisible (monoButton);
        monoButton.onClick = [this] { repaint(); };
        monoButton.setVisible (false);
    }

    void setWaveformData (const juce::AudioBuffer<float>* buffer, int start, int num, const juce::String& cat)
    {
        audioBuffer = buffer;
        startSample = start;
        numSamples = num;
        category = cat;
        
        bool isVisible = audioBuffer != nullptr && numSamples > 0;
        pitchSlider.setVisible (isVisible);
        decaySlider.setVisible (isVisible);
        monoButton.setVisible (isVisible);
        
        if (isVisible)
        {
            juce::Colour accentColour;
            if (category == "kick")
                accentColour = juce::Colour::fromString ("#FFA35C50");
            else if (category == "snare")
                accentColour = juce::Colour::fromString ("#FF7C9482");
            else if (category == "hat")
                accentColour = juce::Colour::fromString ("#FFC4A673");
            else
                accentColour = juce::Colour::fromString ("#FF6C809A");

            pitchSlider.setColour (juce::Slider::rotarySliderFillColourId, accentColour);
            decaySlider.setColour (juce::Slider::rotarySliderFillColourId, accentColour);
            monoButton.setColour (juce::TextButton::buttonOnColourId, accentColour);
        }
        
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced (0.75f);
        
        juce::Colour bgColour = juce::Colour::fromString("#FF222222"); // Lighter matte grey
        juce::Colour borderColour = juce::Colour::fromString("#FF444444"); // Distinct border
        juce::Colour textColour = juce::Colour::fromString("#FFA0A0A0");
        
        juce::Colour accentColour;
        if (category == "kick")
            accentColour = juce::Colour::fromString ("#FFA35C50");
        else if (category == "snare")
            accentColour = juce::Colour::fromString ("#FF7C9482");
        else if (category == "hat")
            accentColour = juce::Colour::fromString ("#FFC4A673");
        else
            accentColour = juce::Colour::fromString ("#FF6C809A");
            
        // Invert colours on click or play highlight
        if (isDown || isPlayingHighlight)
        {
            bgColour = juce::Colour::fromString("#FFEFEBE0");
            textColour = juce::Colour::fromString("#FF1A1A1A");
            borderColour = juce::Colour::fromString("#FFEFEBE0");
        }
        else if (audioBuffer != nullptr && numSamples > 0)
        {
            textColour = accentColour.withAlpha (0.6f);
        }
        
        // Background
        g.setColour (bgColour);
        g.fillRoundedRectangle (bounds, 6.0f);
        
        // Outline
        g.setColour (borderColour);
        g.drawRoundedRectangle (bounds, 6.0f, 1.5f);
        
        if (audioBuffer != nullptr && numSamples > 0)
        {
            int width = getWidth();
            float height = getHeight();
            float halfHeight = height / 2.0f;
            float samplesPerPixel = (float)numSamples / (float)width;
            
            const float* readPtr = audioBuffer->getReadPointer (0);
            
            std::vector<float> maxVals ((size_t)width, 0.0f);
            std::vector<float> minVals ((size_t)width, 0.0f);
            juce::Path wavePath;
            
            float globalMax = 0.0001f;
            for (int i = startSample; i < startSample + numSamples && i < audioBuffer->getNumSamples(); ++i)
                if (std::abs(readPtr[i]) > globalMax) globalMax = std::abs(readPtr[i]);
            
            for (int x = 0; x < width; ++x)
            {
                int startIdx = startSample + (int)(x * samplesPerPixel);
                int endIdx = startSample + (int)((x + 1) * samplesPerPixel);
                if (endIdx > startSample + numSamples) endIdx = startSample + numSamples;
                if (endIdx > audioBuffer->getNumSamples()) endIdx = audioBuffer->getNumSamples();
                
                float minVal = 0.0f;
                float maxVal = 0.0f;
                
                for (int i = startIdx; i < endIdx; ++i)
                {
                    float s = readPtr[i];
                    if (s < minVal) minVal = s;
                    if (s > maxVal) maxVal = s;
                }
                
                // Normalize locally so it's always visible!
                minVal /= globalMax;
                maxVal /= globalMax;
                
                maxVals[(size_t)x] = maxVal;
                minVals[(size_t)x] = minVal;
            }
            
            float padH = halfHeight * 0.8f;
            wavePath.startNewSubPath (0.0f, halfHeight - maxVals[0] * padH);
            for (int x = 1; x < width; ++x)
                wavePath.lineTo ((float)x, halfHeight - maxVals[(size_t)x] * padH);
                
            for (int x = width - 1; x >= 0; --x)
                wavePath.lineTo ((float)x, halfHeight - minVals[(size_t)x] * padH);
                
            wavePath.closeSubPath();
            
            if (! (isDown || isPlayingHighlight))
            {
                // Vertical gradient for the waveform fill
                juce::ColourGradient cg (accentColour.withAlpha (0.70f), 0.0f, halfHeight - padH,
                                         accentColour.withAlpha (0.10f), 0.0f, halfHeight + padH, false);
                g.setGradientFill (cg);
                g.fillPath (wavePath);
                
                // Stroke the waveform outline with category color at 50% opacity
                g.setColour (accentColour.withAlpha (0.50f));
                g.strokePath (wavePath, juce::PathStrokeType (1.0f));
            }
            else
            {
                g.setColour (juce::Colour::fromString ("#FF1A1A1A"));
                g.fillPath (wavePath);
            }
            
            // Draw PAD Number overlay so it still looks like a pad
            float padScale = height / 59.0f;
            g.setColour (textColour);
            g.setFont (juce::FontOptions(12.0f * padScale));
            g.drawText (juce::String (index + 1), getLocalBounds().reduced (juce::roundToInt (6.0f * padScale)), juce::Justification::bottomLeft, false);
            
            // Randomize Button icon
            float buttonSize = 18.0f * padScale;
            auto rBounds = getLocalBounds().removeFromRight((int)buttonSize).removeFromBottom((int)buttonSize).toFloat().reduced(4.0f * padScale);
            
            // Draw button background first (accent color!)
            g.setColour (accentColour);
            g.fillRoundedRectangle (rBounds, 2.0f * padScale);
            
            // Draw border and text (black for contrast!)
            g.setColour (juce::Colour::fromString("#FF0C0C0C"));
            g.drawRoundedRectangle (rBounds, 2.0f * padScale, 1.0f);
            g.setFont(juce::FontOptions(8.0f * padScale).withStyle("Bold"));
            g.drawText("R", rBounds, juce::Justification::centred, false);
 
            // Draw Pitch value next to slider
            float pitchVal = (float) pitchSlider.getValue();
            g.setColour (textColour);
            g.setFont (juce::FontOptions (9.0f * padScale));
            juce::String pitchStr = (pitchVal >= 0.0f ? "+" : "") + juce::String (pitchVal, 1) + "st";
            auto sliderBounds = pitchSlider.getBounds();
            g.drawText (pitchStr, sliderBounds.getX() - juce::roundToInt (42.0f * padScale), sliderBounds.getY() + juce::roundToInt (4.0f * padScale), 
                        juce::roundToInt (40.0f * padScale), juce::roundToInt (14.0f * padScale), 
                        juce::Justification::centredRight, false);
 
            // Draw Decay value next to slider
            float decayVal = (float) decaySlider.getValue();
            g.setColour (textColour);
            g.setFont (juce::FontOptions (9.0f * padScale));
            juce::String decayStr = juce::String (decayVal, 2) + "s";
            auto decayBounds = decaySlider.getBounds();
            g.drawText (decayStr, decayBounds.getRight() + juce::roundToInt (2.0f * padScale), decayBounds.getY() + juce::roundToInt (4.0f * padScale), 
                        juce::roundToInt (40.0f * padScale), juce::roundToInt (14.0f * padScale), 
                        juce::Justification::centredLeft, false);
        }
        else
        {
            float padScale = (float)getHeight() / 59.0f;
            g.setColour (textColour.withAlpha(0.4f));
            g.setFont (juce::FontOptions(14.0f * padScale));
            g.drawText ("PAD " + juce::String (index + 1), getLocalBounds(), juce::Justification::centred, false);
        }
    }
    
    void resized() override
    {
        float padScale = (float)getHeight() / 59.0f;
        float sliderSize = 24.0f * padScale;
        
        decaySlider.setBounds (getLocalBounds().removeFromLeft (juce::roundToInt (sliderSize + 4.0f * padScale))
                                              .removeFromTop (juce::roundToInt (sliderSize + 4.0f * padScale))
                                              .reduced (juce::roundToInt (2.0f * padScale)));
                                              
        pitchSlider.setBounds (getLocalBounds().removeFromRight (juce::roundToInt (sliderSize + 4.0f * padScale))
                                              .removeFromTop (juce::roundToInt (sliderSize + 4.0f * padScale))
                                              .reduced (juce::roundToInt (2.0f * padScale)));

        int monoW = juce::roundToInt (42.0f * padScale);
        int monoH = juce::roundToInt (14.0f * padScale);
        monoButton.setBounds ((getWidth() - monoW) / 2, juce::roundToInt (6.0f * padScale), monoW, monoH);
    }
    
    void mouseDown (const juce::MouseEvent& e) override
    {
        float padScale = (float)getHeight() / 59.0f;
        float buttonSize = 18.0f * padScale;
        auto rBounds = getLocalBounds().removeFromRight((int)buttonSize).removeFromBottom((int)buttonSize);
        if (rBounds.contains(e.getPosition()) && audioBuffer != nullptr)
        {
            if (onRandomize) onRandomize(index);
            return;
        }
        
        if (onClick) onClick(index);
        
        // Visual flash
        isDown = true;
        repaint();
    }
    
    void mouseUp (const juce::MouseEvent&) override
    {
        isDown = false;
        repaint();
    }

    void setIsPlayingHighlight (bool playing)
    {
        if (isPlayingHighlight != playing)
        {
            isPlayingHighlight = playing;
            repaint();
        }
    }

private:
    int index;
    std::function<void(int)> onClick;
    std::function<void(int)> onRandomize;
    bool isDown = false;
    bool isPlayingHighlight = false;
    
    const juce::AudioBuffer<float>* audioBuffer = nullptr;
    int startSample = 0;
    int numSamples = 0;
    juce::String category;

    juce::Slider pitchSlider;
    juce::Slider decaySlider;
    juce::TextButton monoButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> pitchAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> decayAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> monoAttachment;
};

PadGridComponent::PadGridComponent (GranularDrumsProcessor& processor)
{
    for (int i = 0; i < 16; ++i)
    {
        auto pad = std::make_unique<PadComponent> (i, processor,
            [this] (int idx) {
                if (onPadClicked) onPadClicked (idx);
            },
            [this] (int idx) {
                if (onPadRandomizeClicked) onPadRandomizeClicked (idx);
            }
        );
        
        addAndMakeVisible (pad.get());
        pads.push_back (std::move (pad));
    }
}

void PadGridComponent::setPadWaveformData (int padIndex, const juce::AudioBuffer<float>* buffer, int startSample, int numSamples, const juce::String& category)
{
    if (padIndex >= 0 && (size_t)padIndex < pads.size())
        pads[(size_t)padIndex]->setWaveformData (buffer, startSample, numSamples, category);
}

void PadGridComponent::setPadPlaying (int padIndex, bool isPlaying)
{
    if (padIndex >= 0 && (size_t)padIndex < pads.size())
        pads[(size_t)padIndex]->setIsPlayingHighlight (isPlaying);
}

PadGridComponent::~PadGridComponent()
{
}

void PadGridComponent::paint (juce::Graphics& /*g*/)
{
    // Transparent, so PluginEditor's background shows through
}

void PadGridComponent::resized()
{
    float padScale = (float) getHeight() / 236.0f; // 236 is approx default height at 1000x700
    int gap = juce::roundToInt (12.0f * padScale);
    if (gap < 4) gap = 4;
    
    auto bounds = getLocalBounds().reduced (juce::roundToInt (10.0f * padScale));
    
    int cols = 4;
    int rows = 4;
    int padWidth = bounds.getWidth() / cols;
    int padHeight = bounds.getHeight() / rows;

    int padIndex = 0;
    for (int r = 0; r < rows; ++r)
    {
        for (int c = 0; c < cols; ++c)
        {
            if (padIndex < 16)
            {
                auto padBounds = juce::Rectangle<int> (
                    bounds.getX() + c * padWidth,
                    bounds.getY() + r * padHeight,
                    padWidth,
                    padHeight
                ).reduced (gap / 2);
                
                pads[(size_t)padIndex]->setBounds (padBounds);
                padIndex++;
            }
        }
    }
}
