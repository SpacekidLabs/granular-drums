#include "SequencerGridComponent.h"

SequencerGridComponent::SequencerGridComponent (GranularDrumsProcessor& proc)
    : processor (proc)
{
    // Play Button setup
    playButton.setButtonText ("PLAY");
    playButton.setColour (juce::TextButton::buttonColourId, juce::Colour::fromString ("#FF252525"));
    playButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour::fromString ("#FFC4A673"));
    playButton.setColour (juce::TextButton::textColourOffId, juce::Colour::fromString ("#FFEFEBE0"));
    playButton.setColour (juce::TextButton::textColourOnId, juce::Colour::fromString ("#FF121212"));
    addAndMakeVisible (playButton);
    playButton.onClick = [this] {
        bool isPlaying = !processor.isSequencerPlaying();
        processor.setSequencerPlaying (isPlaying);
        playButton.setButtonText (isPlaying ? "STOP" : "PLAY");
    };

    // BPM Slider setup
    bpmSlider.setSliderStyle (juce::Slider::LinearBar);
    bpmSlider.setRange (60.0, 240.0, 1.0);
    bpmSlider.setValue (processor.getSequencerBpm(), juce::dontSendNotification);
    bpmSlider.setColour (juce::Slider::trackColourId, juce::Colour::fromString ("#FFC4A673"));
    bpmSlider.setColour (juce::Slider::thumbColourId, juce::Colour::fromString ("#FFEFEBE0"));
    bpmSlider.setColour (juce::Slider::textBoxTextColourId, juce::Colour::fromString ("#FFEFEBE0"));
    addAndMakeVisible (bpmSlider);
    bpmSlider.onValueChange = [this] {
        processor.setSequencerBpm (bpmSlider.getValue());
    };

    bpmLabel.setText ("BPM:", juce::dontSendNotification);
    bpmLabel.setColour (juce::Label::textColourId, juce::Colour::fromString ("#FFA0A0A0"));
    bpmLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (bpmLabel);

    // Randomize Pattern Button setup
    randomizeButton.setButtonText ("RANDOMIZE PATTERN");
    randomizeButton.setColour (juce::TextButton::buttonColourId, juce::Colour::fromString ("#FF252525"));
    randomizeButton.setColour (juce::TextButton::textColourOffId, juce::Colour::fromString ("#FFEFEBE0"));
    addAndMakeVisible (randomizeButton);
    randomizeButton.onClick = [this] {
        processor.randomizePattern();
        repaint();
    };

    // Clear Pattern Button setup
    clearButton.setButtonText ("CLEAR");
    clearButton.setColour (juce::TextButton::buttonColourId, juce::Colour::fromString ("#FF252525"));
    clearButton.setColour (juce::TextButton::textColourOffId, juce::Colour::fromString ("#FFEFEBE0"));
    addAndMakeVisible (clearButton);
    clearButton.onClick = [this] {
        processor.clearPattern();
        repaint();
    };

    // Sync status setup
    syncLabel.setText ("INTERNAL CLOCK", juce::dontSendNotification);
    syncLabel.setColour (juce::Label::textColourId, juce::Colour::fromString ("#FFA0A0A0"));
    syncLabel.setColour (juce::Label::backgroundColourId, juce::Colour::fromString ("#FF1A1A1A"));
    syncLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (syncLabel);

    startTimerHz (30);
}

SequencerGridComponent::~SequencerGridComponent()
{
    stopTimer();
}

void SequencerGridComponent::timerCallback()
{
    bool isSynced = processor.isSyncedToHost();
    if (isSynced)
    {
        double bpm = processor.getHostBpm();
        bpmSlider.setValue (bpm, juce::dontSendNotification);
    }

    if (isSynced)
    {
        syncLabel.setText ("SYNCED TO DAW", juce::dontSendNotification);
        syncLabel.setColour (juce::Label::textColourId, juce::Colour::fromString ("#FF7C9482")); // Sage green for sync
        playButton.setEnabled (false);
        bpmSlider.setEnabled (false);
    }
    else
    {
        syncLabel.setText ("INTERNAL CLOCK", juce::dontSendNotification);
        syncLabel.setColour (juce::Label::textColourId, juce::Colour::fromString ("#FFA0A0A0"));
        playButton.setEnabled (true);
        bpmSlider.setEnabled (true);
        playButton.setButtonText (processor.isSequencerPlaying() ? "STOP" : "PLAY");
    }

    repaint();
}

juce::Colour SequencerGridComponent::getCategoryColour (const juce::String& category) const
{
    if (category == "kick")
        return juce::Colour::fromString ("#FFA35C50");
    else if (category == "snare")
        return juce::Colour::fromString ("#FF7C9482");
    else if (category == "hat")
        return juce::Colour::fromString ("#FFC4A673");
    else
        return juce::Colour::fromString ("#FF6C809A");
}

void SequencerGridComponent::paint (juce::Graphics& g)
{
    float scale = (float) getWidth() / 960.0f;
    if (scale < 0.5f) scale = 0.5f;

    // Draw grid background shading
    g.setColour (juce::Colour::fromString ("#FF121212"));
    g.fillRoundedRectangle ((float)gridX - 5.0f, (float)gridY - 5.0f, (float)gridW + 10.0f, (float)gridH + 10.0f, 6.0f * scale);
    g.setColour (juce::Colour::fromString ("#FF222222"));
    g.drawRoundedRectangle ((float)gridX - 5.0f, (float)gridY - 5.0f, (float)gridW + 10.0f, (float)gridH + 10.0f, 6.0f * scale, 1.0f);

    for (int pad = 0; pad < 16; ++pad)
    {
        int y = gridY + pad * (cellH + gapY);

        juce::String category = "snare";
        int markerIdx = processor.getMarkerIndexForPad (pad);
        const auto& activeMarkers = processor.getActiveMarkers();
        if (markerIdx >= 0 && (size_t)markerIdx < activeMarkers.size())
            category = activeMarkers[(size_t)markerIdx].category;

        juce::Colour categoryColour = getCategoryColour (category);

        // Row label
        g.setColour (categoryColour.withAlpha (0.75f));
        g.setFont (juce::FontOptions (11.0f * scale).withStyle ("Bold"));
        juce::String padNum = juce::String (pad + 1);
        if (padNum.length() == 1) padNum = "0" + padNum;
        juce::String trackLabel = padNum + " " + category.toUpperCase();
        g.drawText (trackLabel, 5, y, gridX - 15, cellH, juce::Justification::centredLeft, true);

        // Grid Cells
        for (int step = 0; step < 16; ++step)
        {
            int x = gridX + step * (cellW + gapX);
            auto cellBounds = juce::Rectangle<int> (x, y, cellW, cellH).toFloat();

            bool isActive = processor.getSequencerStep (pad, step);

            if (isActive)
            {
                g.setColour (categoryColour);
                g.fillRoundedRectangle (cellBounds, 3.0f * scale);
                g.setColour (juce::Colour::fromString ("#FF0C0C0C"));
                g.drawRoundedRectangle (cellBounds, 3.0f * scale, 0.75f);
            }
            else
            {
                g.setColour (juce::Colour::fromString ("#FF1A1A1A"));
                g.fillRoundedRectangle (cellBounds, 3.0f * scale);

                bool isBeatStart = (step % 4 == 0);
                g.setColour (isBeatStart ? juce::Colour::fromString ("#FF444444") : juce::Colour::fromString ("#FF262626"));
                g.drawRoundedRectangle (cellBounds, 3.0f * scale, 0.75f);
            }
        }
    }

    // Playhead highlight
    bool isPlaying = processor.isSequencerPlaying();
    if (auto* playHead = processor.getPlayHead())
        if (auto positionInfo = playHead->getPosition())
            if (positionInfo->getIsPlaying())
                isPlaying = true;

    int activeStep = processor.getCurrentActiveStep();
    if (isPlaying && activeStep >= 0 && activeStep < 16)
    {
        int activeX = gridX + activeStep * (cellW + gapX);
        auto playheadBounds = juce::Rectangle<int> (activeX, gridY - 2, cellW, gridH + 4).toFloat();

        g.setColour (juce::Colours::white.withAlpha (0.08f));
        g.fillRoundedRectangle (playheadBounds, 3.5f * scale);
        g.setColour (juce::Colours::white.withAlpha (0.35f));
        g.drawRoundedRectangle (playheadBounds, 3.5f * scale, 1.2f);
    }
}

void SequencerGridComponent::resized()
{
    float scale = (float) getWidth() / 960.0f;
    if (scale < 0.5f) scale = 0.5f;

    int barH = juce::roundToInt (35.0f * scale);

    auto bounds = getLocalBounds();
    auto topBar = bounds.removeFromTop (barH);

    int btnW = juce::roundToInt (75.0f * scale);
    int gap = juce::roundToInt (10.0f * scale);

    playButton.setBounds (topBar.removeFromLeft (btnW).reduced (0, juce::roundToInt (2.0f * scale)));
    topBar.removeFromLeft (gap);

    bpmLabel.setBounds (topBar.removeFromLeft (juce::roundToInt (35.0f * scale)));
    bpmSlider.setBounds (topBar.removeFromLeft (juce::roundToInt (90.0f * scale)).reduced (0, juce::roundToInt (4.0f * scale)));
    topBar.removeFromLeft (gap);

    randomizeButton.setBounds (topBar.removeFromLeft (juce::roundToInt (140.0f * scale)).reduced (0, juce::roundToInt (2.0f * scale)));
    topBar.removeFromLeft (gap);

    clearButton.setBounds (topBar.removeFromLeft (btnW).reduced (0, juce::roundToInt (2.0f * scale)));

    syncLabel.setBounds (topBar.removeFromRight (juce::roundToInt (120.0f * scale)).reduced (0, juce::roundToInt (4.0f * scale)));

    gridX = juce::roundToInt (90.0f * scale);
    gridY = barH + juce::roundToInt (10.0f * scale);
    gridW = getWidth() - gridX - juce::roundToInt (10.0f * scale);
    gridH = getHeight() - gridY - juce::roundToInt (10.0f * scale);

    gapX = juce::roundToInt (4.0f * scale);
    gapY = juce::roundToInt (4.0f * scale);

    cellW = (gridW - 15 * gapX) / 16;
    cellH = (gridH - 15 * gapY) / 16;
}

void SequencerGridComponent::mouseDown (const juce::MouseEvent& e)
{
    handleMouseGridInteraction (e.getPosition(), false);
}

void SequencerGridComponent::mouseDrag (const juce::MouseEvent& e)
{
    handleMouseGridInteraction (e.getPosition(), true);
}

void SequencerGridComponent::handleMouseGridInteraction (const juce::Point<int>& pos, bool isDragState)
{
    if (pos.x < gridX || pos.x > gridX + gridW || pos.y < gridY || pos.y > gridY + gridH)
        return;

    int step = (pos.x - gridX) / (cellW + gapX);
    int pad = (pos.y - gridY) / (cellH + gapY);

    if (step >= 0 && step < 16 && pad >= 0 && pad < 16)
    {
        if (isDragState)
        {
            if (pad == lastClickedPad && step == lastClickedStep)
                return;
        }

        lastClickedPad = pad;
        lastClickedStep = step;

        bool currentVal = processor.getSequencerStep (pad, step);
        processor.setSequencerStep (pad, step, !currentVal);
        repaint();
    }
}
