#include "MainWaveformDisplay.h"

MainWaveformDisplay::MainWaveformDisplay()
{
}

MainWaveformDisplay::~MainWaveformDisplay()
{
}

void MainWaveformDisplay::setWaveformData (const juce::AudioBuffer<float>* buffer, const std::vector<OnsetMarker>* markers)
{
    audioBuffer = buffer;
    onsetMarkers = markers;
    repaint();
}

void MainWaveformDisplay::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Background and highlight border
    if (isAnalyzing)
    {
        g.setColour (juce::Colour::fromString("#FF121212"));
        g.fillRoundedRectangle (bounds, 6.0f);
        
        g.setColour (juce::Colour::fromString("#FF3A3A3A"));
        g.drawRoundedRectangle (bounds, 6.0f, 1.0f);
        
        // Draw dynamic loading screen
        g.setColour (juce::Colour::fromString("#FFEFEBE0"));
        g.setFont (juce::FontOptions (18.0f).withStyle ("Bold"));
        juce::String percent = juce::String (juce::roundToInt (analysisProgress * 100.0f)) + "%";
        g.drawText ("ANALYZING SAMPLE... " + percent, getLocalBounds().translated(0, -10), juce::Justification::centred, false);
        
        // Draw progress bar track
        float barW = bounds.getWidth() * 0.5f;
        float barH = 8.0f;
        float barX = (bounds.getWidth() - barW) * 0.5f;
        float barY = bounds.getHeight() * 0.5f + 15.0f;
        
        g.setColour (juce::Colour::fromString("#FF2A2A2A"));
        g.fillRoundedRectangle (barX, barY, barW, barH, 4.0f);
        
        // Fill progress
        g.setColour (juce::Colour::fromString("#FFFF9F1C"));
        g.fillRoundedRectangle (barX, barY, barW * analysisProgress, barH, 4.0f);
        return;
    }
    
    if (isHovering)
    {
        g.setColour (juce::Colour::fromString("#FF1A1A1A"));
        g.fillRoundedRectangle (bounds, 6.0f);
        
        // Gold highlight border
        g.setColour (juce::Colour::fromString("#FFFF9F1C"));
        g.drawRoundedRectangle (bounds, 6.0f, 2.5f);
        
        // Translucent shimmer overlay
        g.setColour (juce::Colour::fromString("#FFFF9F1C").withAlpha (0.08f));
        g.fillRoundedRectangle (bounds, 6.0f);
    }
    else
    {
        g.setColour (juce::Colour::fromString("#FF121212"));
        g.fillRoundedRectangle (bounds, 6.0f);
        
        g.setColour (juce::Colour::fromString("#FF3A3A3A"));
        g.drawRoundedRectangle (bounds, 6.0f, 1.0f);
    }
    
    if (audioBuffer != nullptr && audioBuffer->getNumSamples() > 0 && onsetMarkers != nullptr)
    {
        int width = getWidth();
        float height = getHeight();
        float halfHeight = height / 2.0f;
        int numSamples = audioBuffer->getNumSamples();
        float samplesPerPixel = (float)numSamples / (float)width;
        
        const float* readPtr = audioBuffer->getReadPointer (0);
        
        // Draw zero-crossing line (subtle dark grey)
        g.setColour (juce::Colour::fromString("#FF2A2A2A"));
        g.drawLine (0.0f, halfHeight, (float)width, halfHeight, 1.0f);
        
        // Draw Waveform
        g.setColour (juce::Colour::fromString("#FFEFEBE0"));
        
        std::vector<float> maxVals ((size_t)width, 0.0f);
        std::vector<float> minVals ((size_t)width, 0.0f);
        
        // Find global peak amplitude with a stepped fast scan
        float globalMax = 0.0001f;
        int peakStep = juce::jmax (1, numSamples / 2000);
        for (int i = 0; i < numSamples; i += peakStep)
        {
            float val = std::abs (readPtr[i]);
            if (std::isnan (val) || std::isinf (val))
                continue;
            if (val > globalMax)
                globalMax = val;
        }
        
        // Scan envelope with downsampled steps if samplesPerPixel is large
        int scanStep = juce::jmax (1, juce::roundToInt (samplesPerPixel / 15.0f));
        for (int x = 0; x < width; ++x)
        {
            int startSample = (int)(x * samplesPerPixel);
            int endSample = juce::jmax (startSample + 1, (int)((x + 1) * samplesPerPixel));
            if (endSample > numSamples) endSample = numSamples;
            if (startSample >= numSamples) startSample = numSamples - 1;
            
            float minVal = 0.0f;
            float maxVal = 0.0f;
            
            for (int i = startSample; i < endSample; i += scanStep)
            {
                if (i >= numSamples) break;
                float s = readPtr[i];
                if (std::isnan (s) || std::isinf (s))
                    continue;
                if (s < minVal) minVal = s;
                if (s > maxVal) maxVal = s;
            }
            maxVals[(size_t)x] = maxVal / globalMax;
            minVals[(size_t)x] = minVal / globalMax;
        }
        
        float padH = halfHeight * 0.85f; // Padding so it looks neat
        
        // 1. Draw the filled waveform path (semi-translucent bone off-white)
        juce::Path wavePath;
        wavePath.startNewSubPath (0.0f, halfHeight - maxVals[0] * padH);
        for (int x = 1; x < width; ++x)
            wavePath.lineTo ((float)x, halfHeight - maxVals[(size_t)x] * padH);
            
        for (int x = width - 1; x >= 0; --x)
            wavePath.lineTo ((float)x, halfHeight - minVals[(size_t)x] * padH);
            
        wavePath.closeSubPath();
        
        // Waveform gradient (glowing gold to transparent center)
        juce::Colour waveAccent = juce::Colour::fromString("#FFFF9F1C");
        juce::ColourGradient verticalGrad (waveAccent.withAlpha (0.35f), 0.0f, halfHeight - padH,
                                           waveAccent.withAlpha (0.35f), 0.0f, halfHeight + padH, false);
        verticalGrad.addColour (0.5, juce::Colour::fromString("#FFEFEBE0").withAlpha (0.06f));
        
        g.setGradientFill (verticalGrad);
        g.fillPath (wavePath);
        
        // 2. Draw solid lines for the top and bottom envelopes for a premium high-contrast look
        juce::Path topPath;
        topPath.startNewSubPath (0.0f, halfHeight - maxVals[0] * padH);
        for (int x = 1; x < width; ++x)
            topPath.lineTo ((float)x, halfHeight - maxVals[(size_t)x] * padH);
        
        g.setColour (juce::Colour::fromString("#FFEFEBE0").withAlpha (0.85f));
        g.strokePath (topPath, juce::PathStrokeType (1.2f));
        
        juce::Path bottomPath;
        bottomPath.startNewSubPath (0.0f, halfHeight - minVals[0] * padH);
        for (int x = 1; x < width; ++x)
            bottomPath.lineTo ((float)x, halfHeight - minVals[(size_t)x] * padH);
            
        g.setColour (juce::Colour::fromString("#FFEFEBE0").withAlpha (0.85f));
        g.strokePath (bottomPath, juce::PathStrokeType (1.2f));
        
        // Draw Markers (color-coded by category, limit to first 16 pads)
        int markerCount = 0;
        for (const auto& marker : *onsetMarkers)
        {
            if (markerCount >= 16)
                break;
                
            juce::Colour markerColour;
            if (marker.category == "kick")
                markerColour = juce::Colour::fromString ("#FFA35C50");
            else if (marker.category == "snare")
                markerColour = juce::Colour::fromString ("#FF7C9482");
            else if (marker.category == "hat")
                markerColour = juce::Colour::fromString ("#FFC4A673");
            else
                markerColour = juce::Colour::fromString ("#FF6C809A");
                
            float xPos = (float)marker.sampleIndex / (float)numSamples * (float)width;
            g.setColour (markerColour.withAlpha (0.40f));
            g.drawLine (xPos, 0, xPos, height, 1.0f);
            
            // Draw numerical label at the top of each slice marker inside a neat rounded dark tag
            g.setColour (juce::Colour::fromString("#FF1A1A1A").withAlpha (0.85f));
            g.fillRoundedRectangle (xPos + 2.0f, 3.0f, 15.0f, 12.0f, 3.0f);
            
            g.setColour (markerColour.withAlpha (0.95f));
            g.setFont (juce::FontOptions (9.0f).withStyle ("Bold"));
            g.drawText (juce::String (markerCount + 1), juce::roundToInt (xPos + 2.0f), 3, 15, 12, juce::Justification::centred, false);
            
            markerCount++;
        }

        // Draw selection window and boundary handles
        float startX = selectionStartRatio * (float)width;
        float endX = selectionEndRatio * (float)width;
        
        // Draw unselected region shading (dark overlay)
        g.setColour (juce::Colours::black.withAlpha (0.45f));
        if (startX > 0.0f)
            g.fillRect (0.0f, 0.0f, startX, height);
        if (endX < (float)width)
            g.fillRect (endX, 0.0f, (float)width - endX, height);
            
        // Draw vertical selection boundary lines (gold/orange accent)
        g.setColour (juce::Colour::fromString ("#FFFF9F1C"));
        g.drawLine (startX, 0.0f, startX, height, 2.0f);
        g.drawLine (endX, 0.0f, endX, height, 2.0f);
        
        // Draw handles for visual cues (with ridges)
        g.fillRoundedRectangle (startX - 5.0f, 0.0f, 10.0f, 14.0f, 3.0f);
        g.fillRoundedRectangle (endX - 5.0f, height - 14.0f, 10.0f, 14.0f, 3.0f);
        
        g.setColour (juce::Colour::fromString ("#FF121212"));
        g.drawLine (startX, 3.0f, startX, 11.0f, 1.5f);
        g.drawLine (endX, height - 11.0f, endX, height - 3.0f, 1.5f);
        
        if (isHovering)
        {
            g.setColour (juce::Colour::fromString("#FFFF9F1C"));
            g.setFont (juce::FontOptions (18.0f).withStyle ("Bold"));
            g.drawText ("DROP NEW SOUND TO REPLACE", getLocalBounds(), juce::Justification::centred, false);
        }
    }
    else
    {
        if (isHovering)
        {
            g.setColour (juce::Colour::fromString("#FFFF9F1C"));
            g.setFont (juce::FontOptions (24.0f).withStyle ("Bold"));
            g.drawText ("DROP SOUND TO SPLICE", getLocalBounds(), juce::Justification::centred, false);
        }
        else
        {
            g.setColour (juce::Colour::fromString("#FFEFEBE0").withAlpha(0.3f));
            float scale = (float)getHeight() / 248.0f;
            g.setFont (juce::FontOptions(24.0f * scale));
            g.drawText (currentMessage, getLocalBounds(), juce::Justification::centred, false);
        }
    }
}

void MainWaveformDisplay::resized()
{
}

bool MainWaveformDisplay::isInterestedInFileDrag (const juce::StringArray& files)
{
    for (auto file : files)
    {
        if (file.endsWithIgnoreCase (".wav") || 
            file.endsWithIgnoreCase (".aiff") ||
            file.endsWithIgnoreCase (".mp3") ||
            file.endsWithIgnoreCase (".flac"))
            return true;
    }
    return false;
}

void MainWaveformDisplay::fileDragEnter (const juce::StringArray& files, int x, int y)
{
    juce::ignoreUnused (files, x, y);
    isHovering = true;
    repaint();
}

void MainWaveformDisplay::fileDragExit (const juce::StringArray& files)
{
    juce::ignoreUnused (files);
    isHovering = false;
    repaint();
}

void MainWaveformDisplay::filesDropped (const juce::StringArray& files, int x, int y)
{
    juce::ignoreUnused (x, y);
    isHovering = false;

    for (auto path : files)
    {
        juce::File file (path);
        if (file.existsAsFile())
        {
            currentMessage = "Analyzing: " + file.getFileName();
            repaint();
            
            if (onFileDropped)
                onFileDropped (file);
                
            break; // Only take the first file
        }
    }
}

void MainWaveformDisplay::setAnalysisState (bool analyzing, float progress)
{
    if (isAnalyzing != analyzing || std::abs (analysisProgress - progress) > 0.005f)
    {
        isAnalyzing = analyzing;
        analysisProgress = progress;
        repaint();
    }
}

void MainWaveformDisplay::setSelectionRatios (float start, float end)
{
    selectionStartRatio = start;
    selectionEndRatio = end;
    repaint();
}

void MainWaveformDisplay::mouseMove (const juce::MouseEvent& event)
{
    if (audioBuffer == nullptr || audioBuffer->getNumSamples() == 0)
        return;

    float x = (float)event.x;
    float startX = selectionStartRatio * (float)getWidth();
    float endX = selectionEndRatio * (float)getWidth();

    if (std::abs (x - startX) <= 8.0f || std::abs (x - endX) <= 8.0f)
        setMouseCursor (juce::MouseCursor::LeftRightResizeCursor);
    else
        setMouseCursor (juce::MouseCursor::NormalCursor);
}

void MainWaveformDisplay::mouseDown (const juce::MouseEvent& event)
{
    if (audioBuffer == nullptr || audioBuffer->getNumSamples() == 0)
        return;

    float x = (float)event.x;
    float startX = selectionStartRatio * (float)getWidth();
    float endX = selectionEndRatio * (float)getWidth();

    isDraggingStart = false;
    isDraggingEnd = false;

    if (std::abs (x - startX) <= 8.0f && std::abs (x - startX) <= std::abs (x - endX))
    {
        isDraggingStart = true;
        setMouseCursor (juce::MouseCursor::LeftRightResizeCursor);
    }
    else if (std::abs (x - endX) <= 8.0f)
    {
        isDraggingEnd = true;
        setMouseCursor (juce::MouseCursor::LeftRightResizeCursor);
    }
}

void MainWaveformDisplay::mouseDrag (const juce::MouseEvent& event)
{
    if (audioBuffer == nullptr || audioBuffer->getNumSamples() == 0)
        return;

    float x = (float)event.x;
    float w = (float)getWidth();
    if (w <= 0.0f) return;

    float ratio = x / w;

    if (isDraggingStart)
    {
        selectionStartRatio = juce::jlimit (0.0f, selectionEndRatio - 0.02f, ratio);
        repaint();
    }
    else if (isDraggingEnd)
    {
        selectionEndRatio = juce::jlimit (selectionStartRatio + 0.02f, 1.0f, ratio);
        repaint();
    }
}

void MainWaveformDisplay::mouseUp (const juce::MouseEvent& event)
{
    juce::ignoreUnused (event);
    isDraggingStart = false;
    isDraggingEnd = false;
    setMouseCursor (juce::MouseCursor::NormalCursor);

    if (onSelectionFinished)
        onSelectionFinished (selectionStartRatio, selectionEndRatio);
}
