#include "PluginEditor.h"

#include <algorithm>
#include <cmath>

namespace
{
juce::Colour background() { return juce::Colour (0xff0b0e12); }
juce::Colour panel() { return juce::Colour (0xff171c22); }
juce::Colour panelRaised() { return juce::Colour (0xff20262e); }
juce::Colour line() { return juce::Colour (0xff343d48); }
juce::Colour text() { return juce::Colour (0xfff3f5f7); }
juce::Colour muted() { return juce::Colour (0xff9ca8b5); }
juce::Colour green() { return juce::Colour (0xff42d392); }
juce::Colour amber() { return juce::Colour (0xffffbd5a); }

void setLabelStyle (juce::Label& label, float size, juce::Colour colour, bool bold = false)
{
    label.setFont (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain));
    label.setColour (juce::Label::textColourId, colour);
    label.setJustificationType (juce::Justification::centredLeft);
}
}

SoliVoicerLookAndFeel::SoliVoicerLookAndFeel()
{
    setColour (juce::Slider::thumbColourId, green());
    setColour (juce::Slider::rotarySliderFillColourId, green());
    setColour (juce::Slider::rotarySliderOutlineColourId, line());
    setColour (juce::ComboBox::backgroundColourId, panelRaised());
    setColour (juce::ComboBox::outlineColourId, line());
    setColour (juce::ComboBox::textColourId, text());
    setColour (juce::PopupMenu::backgroundColourId, panel());
    setColour (juce::PopupMenu::textColourId, text());
    setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xff285846));
    setColour (juce::TextButton::buttonColourId, panelRaised());
    setColour (juce::TextButton::textColourOffId, text());
}

void SoliVoicerLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                                              float sliderPos, float minSliderPos, float maxSliderPos,
                                              const juce::Slider::SliderStyle style, juce::Slider& slider)
{
    if (style != juce::Slider::LinearVertical && style != juce::Slider::LinearBarVertical)
    {
        juce::LookAndFeel_V4::drawLinearSlider (g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
        return;
    }

    auto bounds = juce::Rectangle<float> (static_cast<float> (x), static_cast<float> (y),
                                          static_cast<float> (width), static_cast<float> (height)).reduced (8.0f, 6.0f);
    const auto track = juce::Rectangle<float> (bounds.getCentreX() - 3.0f, bounds.getY() + 4.0f,
                                               6.0f, juce::jmax (8.0f, bounds.getHeight() - 14.0f));
    const auto handleY = juce::jlimit (track.getY(), track.getBottom(), sliderPos);
    g.setColour (line());
    g.fillRoundedRectangle (track, 3.0f);
    g.setColour (green());
    g.fillRoundedRectangle (track.withTop (handleY), 3.0f);
    auto handle = juce::Rectangle<float> (bounds.getCentreX() - 15.0f, handleY - 5.0f, 30.0f, 10.0f);
    g.setColour (panelRaised());
    g.fillRoundedRectangle (handle, 3.0f);
    g.setColour (green());
    g.drawRoundedRectangle (handle, 3.0f, 1.4f);
}

void SoliVoicerLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool,
                                          int, int, int, int, juce::ComboBox&)
{
    const auto bounds = juce::Rectangle<float> (0.5f, 0.5f, static_cast<float> (width) - 1.0f,
                                                 static_cast<float> (height) - 1.0f);
    g.setColour (panelRaised());
    g.fillRoundedRectangle (bounds, 5.0f);
    g.setColour (line());
    g.drawRoundedRectangle (bounds, 5.0f, 1.0f);
    juce::Path arrow;
    arrow.startNewSubPath (bounds.getRight() - 17.0f, bounds.getCentreY() - 3.0f);
    arrow.lineTo (bounds.getRight() - 11.0f, bounds.getCentreY() + 3.0f);
    arrow.lineTo (bounds.getRight() - 5.0f, bounds.getCentreY() - 3.0f);
    g.setColour (green());
    g.strokePath (arrow, juce::PathStrokeType (1.8f));
}

void SoliVoicerMidiDragButton::mouseDown (const juce::MouseEvent& event)
{
    dragStarted = false;
    juce::TextButton::mouseDown (event);
}

void SoliVoicerMidiDragButton::mouseDrag (const juce::MouseEvent& event)
{
    if (! dragStarted && event.getDistanceFromDragStart() > 4)
    {
        dragStarted = true;
        if (onDragStart)
            onDragStart();
        return;
    }

    juce::TextButton::mouseDrag (event);
}

void SoliVoicerMidiDragButton::mouseUp (const juce::MouseEvent& event)
{
    const auto wasDragStarted = dragStarted;
    dragStarted = false;
    if (! wasDragStarted)
        juce::TextButton::mouseUp (event);
    else
        repaint();
}

SoliVoicerAudioProcessorEditor::SoliVoicerAudioProcessorEditor (SoliVoicerAudioProcessor& owner)
    : AudioProcessorEditor (&owner), processorRef (owner)
{
    setLookAndFeel (&lookAndFeel);
    setResizable (true, true);
    setResizeLimits (860, 720, 1500, 1120);
    setSize (1120, 880);
    tooltipWindow = std::make_unique<juce::TooltipWindow> (this, 700);

    addAndMakeVisible (titleLabel);
    titleLabel.setText ("VOICIZER", juce::dontSendNotification);
    setLabelStyle (titleLabel, 22.0f, text(), true);

    addAndMakeVisible (chordLabel);
    chordLabel.setText ("--", juce::dontSendNotification);
    setLabelStyle (chordLabel, 20.0f, green(), true);
    chordLabel.setJustificationType (juce::Justification::centredRight);

    addAndMakeVisible (randomButton);
    addAndMakeVisible (randomVoicingButton);
    addAndMakeVisible (resetButton);
    randomButton.onClick = [this] { randomizeAllSettings(); };
    randomVoicingButton.onClick = [this] { randomizeVoicingSettings(); };
    resetButton.onClick = [this] { resetDefaults(); };
    randomButton.setTooltip ("Randomize all settings except Harmony Source and Output.");
    randomVoicingButton.setTooltip ("Randomize voicing settings, including the input note role.");
    resetButton.setTooltip ("Restore Voicizer defaults.");

    addAndMakeVisible (midiRecordLabel);
    midiRecordLabel.setText ("MIDI Capture", juce::dontSendNotification);
    setLabelStyle (midiRecordLabel, 12.0f, muted(), true);
    addAndMakeVisible (midiRecordStatusLabel);
    midiRecordStatusLabel.setText ("No take", juce::dontSendNotification);
    setLabelStyle (midiRecordStatusLabel, 12.0f, muted(), true);
    addAndMakeVisible (recordMidiButton);
    addAndMakeVisible (dragMidiButton);
    addAndMakeVisible (clearMidiButton);
    recordMidiButton.setClickingTogglesState (true);
    recordMidiButton.setTooltip ("Record the generated MIDI output into a draggable MIDI take.");
    dragMidiButton.setTooltip ("Drag the recorded MIDI take into Logic.");
    clearMidiButton.setTooltip ("Clear the captured MIDI take.");
    recordMidiButton.onClick = [this]
    {
        processorRef.setMidiRecordingEnabled (recordMidiButton.getToggleState());
        updateRecorderControls();
    };
    dragMidiButton.onClick = [this] { beginRecordedMidiDrag(); };
    dragMidiButton.onDragStart = [this] { beginRecordedMidiDrag(); };
    clearMidiButton.onClick = [this]
    {
        processorRef.clearRecordedMidi();
        updateRecorderControls();
        repaint (midiRecordBounds);
    };

    addCombo (sourceModeBox, sourceModeLabel, "Harmony Source", SoliVoicerAudioProcessor::sourceModeNames(),
              "Manual uses key and scale selection. Follow Chordizer reads the shared chord timeline.");
    addCombo (outputModeBox, outputModeLabel, "Output", SoliVoicerAudioProcessor::outputModeNames(),
              "Held Voicing sustains the chord. Performance plays a host-tempo rhythmic interpretation.");
    addCombo (contextModeBox, contextModeLabel, "Relationship", Soli::ChordEngine::contextModeNames(),
              "Match follows the region literally. Diatonic and substitution modes choose compatible alternatives.");
    addCombo (roleBox, roleLabel, "Input Note Role", Soli::ChordEngine::roleNames(),
              "Controls the placement of the incoming note in the generated voicing.");
    addCombo (styleBox, styleLabel, "Voicing Style", Soli::ChordEngine::styleNames(),
              "Sets harmonic and spacing preferences for generated voicings.");
    addCombo (playabilityBox, playabilityLabel, "Playable As", Soli::ChordEngine::playabilityNames(),
              "Constrains spacing and voice count for the selected instrument family.");
    addCombo (strumModeBox, strumModeLabel, "Rake", Soli::ChordEngine::strumModeNames(),
              "Sets simultaneous, upward, downward, or randomized note onset order.");
    addCombo (performanceStyleBox, performanceStyleLabel, "Performance Style",
              SoliVoicerAudioProcessor::performanceStyleNames(),
              "Selects the tempo-synchronized chord performance pattern.");
    addCombo (performanceSubStyleBox, performanceSubStyleLabel, "Sub Style",
              SoliVoicerAudioProcessor::performanceSubStyleNames (0),
              "Named variation inside the selected performance style.");
    addAndMakeVisible (doubleTimeButton);
    doubleTimeButton.setButtonText ("Double Time");
    doubleTimeButton.setTooltip ("Runs the performance pattern at twice the rhythmic rate.");
    doubleTimeButton.setColour (juce::ToggleButton::textColourId, text());
    doubleTimeButton.setColour (juce::ToggleButton::tickColourId, green());
    addAndMakeVisible (randomPerformanceButton);
    randomPerformanceButton.onClick = [this] { randomizePerformanceSettings(); };
    randomPerformanceButton.setTooltip ("Randomize only the performance style, sub-style, and performance controls.");

    configureLabel (keyLabel, "Keys");
    configureLabel (scaleLabel, "Scales");
    addAndMakeVisible (randomKeyScaleButton);
    randomKeyScaleButton.onClick = [this] { randomizeKeyScaleSettings(); };
    randomKeyScaleButton.setTooltip ("Randomize the enabled keys and scales.");
    const auto keys = Soli::ChordEngine::keyNames();
    const auto scales = Soli::ChordEngine::scaleNames();
    for (int i = 0; i < 12; ++i)
    {
        configureMaskToggle (keyToggles[static_cast<std::size_t> (i)], keys[i]);
        keyToggles[static_cast<std::size_t> (i)].onClick = [this]
        {
            commitMask (ParameterIDs::keyMask, keyToggles, 12);
        };
        configureMaskToggle (scaleToggles[static_cast<std::size_t> (i)], scales[i]);
        scaleToggles[static_cast<std::size_t> (i)].onClick = [this]
        {
            commitMask (ParameterIDs::scaleMask, scaleToggles, 12);
        };
    }

    addAndMakeVisible (linkStatusLabel);
    setLabelStyle (linkStatusLabel, 12.0f, muted(), true);

    addSlider (chordSizeSlider, chordSizeLabel, "Chord Size", "Number of generated voices.");
    addSlider (complexitySlider, complexityLabel, "Voicing Complexity", "Controls extensions and richer chord colors.");
    addSlider (voiceLeadingSlider, voiceLeadingLabel, "Voice Leading", "Prioritizes smooth movement from the previous voicing.");
    addSlider (outsideSlider, outsideLabel, "Outside Harmony", "Allows notes and chord choices outside the immediate scale.");
    addSlider (variationSlider, variationLabel, "Variation", "Widens the pool of valid generated alternatives.");
    addSlider (repeatSlider, repeatLabel, "Repeat", "Chance to retain the prior voicing.");
    addSlider (strumSpeedSlider, strumSpeedLabel, "Rake Speed", "Controls the onset spread for raked held chords.");
    addSlider (minNoteSlider, minNoteLabel, "Lowest Note", "Lowest generated MIDI note.");
    addSlider (maxNoteSlider, maxNoteLabel, "Highest Note", "Highest generated MIDI note.");
    addSlider (substitutionSlider, substitutionLabel, "Substitution Depth", "Controls how far compatible replacements may move from the Chordizer chord.");
    addSlider (performanceComplexitySlider, performanceComplexityLabel, "Sophistication", "Adds denser and more independent performance gestures.");
    addSlider (densitySlider, densityLabel, "Rhythm Density", "Moves from quarter-note to eighth-note and sixteenth-note activity.");
    addSlider (syncopationSlider, syncopationLabel, "Syncopation", "Moves offbeats later and emphasizes displaced attacks.");
    addSlider (swingSlider, swingLabel, "Swing", "Delays alternating subdivisions.");
    addSlider (humanizeSlider, humanizeLabel, "Humanize", "Adds bounded timing and velocity variation.");
    addSlider (gateSlider, gateLabel, "Gate", "Controls the performed note duration.");

    auto& state = processorRef.getValueTreeState();
    sourceModeAttachment = std::make_unique<ComboAttachment> (state, ParameterIDs::sourceMode, sourceModeBox);
    outputModeAttachment = std::make_unique<ComboAttachment> (state, ParameterIDs::outputMode, outputModeBox);
    contextModeAttachment = std::make_unique<ComboAttachment> (state, ParameterIDs::contextMode, contextModeBox);
    roleAttachment = std::make_unique<ComboAttachment> (state, ParameterIDs::role, roleBox);
    styleAttachment = std::make_unique<ComboAttachment> (state, ParameterIDs::style, styleBox);
    playabilityAttachment = std::make_unique<ComboAttachment> (state, ParameterIDs::playability, playabilityBox);
    strumModeAttachment = std::make_unique<ComboAttachment> (state, ParameterIDs::strumMode, strumModeBox);
    performanceStyleAttachment = std::make_unique<ComboAttachment> (state, ParameterIDs::performanceStyle, performanceStyleBox);
    performanceSubStyleAttachment = std::make_unique<ComboAttachment> (state, ParameterIDs::performanceSubStyle, performanceSubStyleBox);
    chordSizeAttachment = std::make_unique<SliderAttachment> (state, ParameterIDs::chordSize, chordSizeSlider);
    complexityAttachment = std::make_unique<SliderAttachment> (state, ParameterIDs::complexity, complexitySlider);
    voiceLeadingAttachment = std::make_unique<SliderAttachment> (state, ParameterIDs::voiceLeading, voiceLeadingSlider);
    outsideAttachment = std::make_unique<SliderAttachment> (state, ParameterIDs::outside, outsideSlider);
    variationAttachment = std::make_unique<SliderAttachment> (state, ParameterIDs::variation, variationSlider);
    repeatAttachment = std::make_unique<SliderAttachment> (state, ParameterIDs::repeatChance, repeatSlider);
    strumSpeedAttachment = std::make_unique<SliderAttachment> (state, ParameterIDs::strumSpeed, strumSpeedSlider);
    minNoteAttachment = std::make_unique<SliderAttachment> (state, ParameterIDs::minNote, minNoteSlider);
    maxNoteAttachment = std::make_unique<SliderAttachment> (state, ParameterIDs::maxNote, maxNoteSlider);
    substitutionAttachment = std::make_unique<SliderAttachment> (state, ParameterIDs::substitutionDepth, substitutionSlider);
    performanceComplexityAttachment = std::make_unique<SliderAttachment> (state, ParameterIDs::performanceComplexity, performanceComplexitySlider);
    densityAttachment = std::make_unique<SliderAttachment> (state, ParameterIDs::rhythmDensity, densitySlider);
    syncopationAttachment = std::make_unique<SliderAttachment> (state, ParameterIDs::syncopation, syncopationSlider);
    swingAttachment = std::make_unique<SliderAttachment> (state, ParameterIDs::swing, swingSlider);
    humanizeAttachment = std::make_unique<SliderAttachment> (state, ParameterIDs::humanize, humanizeSlider);
    gateAttachment = std::make_unique<SliderAttachment> (state, ParameterIDs::gate, gateSlider);
    doubleTimeAttachment = std::make_unique<ButtonAttachment> (state, ParameterIDs::doubleTime, doubleTimeButton);

    updatePerformanceSubStyleChoices();
    updateRecorderControls();
    updateMaskToggles();
    updateModeVisibility();
    startTimerHz (30);
}

SoliVoicerAudioProcessorEditor::~SoliVoicerAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void SoliVoicerAudioProcessorEditor::mouseDown (const juce::MouseEvent& event)
{
    midiShapeDragArmed = midiShapeBounds.contains (event.getPosition())
                      && recordedMidi.hasOrigin
                      && ! recordedMidi.events.empty()
                      && ! processorRef.isMidiRecording();
}

void SoliVoicerAudioProcessorEditor::mouseDrag (const juce::MouseEvent& event)
{
    if (midiShapeDragArmed && event.getDistanceFromDragStart() > 4)
    {
        midiShapeDragArmed = false;
        beginRecordedMidiDrag();
    }
}

void SoliVoicerAudioProcessorEditor::mouseUp (const juce::MouseEvent&)
{
    midiShapeDragArmed = false;
}

void SoliVoicerAudioProcessorEditor::configureSlider (juce::Slider& slider, const juce::String& tooltip)
{
    slider.setSliderStyle (juce::Slider::LinearVertical);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 72, 20);
    slider.setColour (juce::Slider::textBoxTextColourId, text());
    slider.setColour (juce::Slider::textBoxBackgroundColourId, background());
    slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    slider.setTooltip (tooltip);
}

void SoliVoicerAudioProcessorEditor::configureCombo (juce::ComboBox& combo,
                                                     const juce::StringArray& names,
                                                     const juce::String& tooltip)
{
    for (int i = 0; i < names.size(); ++i)
        combo.addItem (names[i], i + 1);
    combo.setJustificationType (juce::Justification::centredLeft);
    combo.setTooltip (tooltip);
}

void SoliVoicerAudioProcessorEditor::configureLabel (juce::Label& label, const juce::String& name)
{
    addAndMakeVisible (label);
    label.setText (name, juce::dontSendNotification);
    setLabelStyle (label, 12.0f, muted(), true);
}

void SoliVoicerAudioProcessorEditor::addSlider (juce::Slider& slider, juce::Label& label,
                                                const juce::String& name, const juce::String& tooltip)
{
    configureLabel (label, name);
    addAndMakeVisible (slider);
    configureSlider (slider, tooltip);
    label.setTooltip (tooltip);
}

void SoliVoicerAudioProcessorEditor::addCombo (juce::ComboBox& combo, juce::Label& label,
                                               const juce::String& name, const juce::StringArray& choices,
                                               const juce::String& tooltip)
{
    configureLabel (label, name);
    addAndMakeVisible (combo);
    configureCombo (combo, choices, tooltip);
    label.setTooltip (tooltip);
}

void SoliVoicerAudioProcessorEditor::configureMaskToggle (juce::ToggleButton& button, const juce::String& name)
{
    addAndMakeVisible (button);
    button.setButtonText (name);
    button.setColour (juce::ToggleButton::textColourId, text());
    button.setColour (juce::ToggleButton::tickColourId, green());
}

void SoliVoicerAudioProcessorEditor::timerCallback()
{
    chordLabel.setText (processorRef.getLastChordName(), juce::dontSendNotification);
    const auto source = sourceModeBox.getSelectedItemIndex();
    const auto output = outputModeBox.getSelectedItemIndex();
    if (source != lastSourceMode || output != lastOutputMode)
        updateModeVisibility();
    if (performanceStyleBox.getSelectedItemIndex() != lastPerformanceStyle)
        updatePerformanceSubStyleChoices();
    if (source == 1)
    {
        const auto previousPlayhead = chordizerSnapshot.playheadPpq;
        chordizerSnapshot = processorRef.getChordizerSnapshot();
        linkStatusLabel.setText (chordizerSnapshot.connected
                                 ? juce::String (chordizerSnapshot.regions.size()) + " linked regions"
                                 : "Chordizer offline", juce::dontSendNotification);
        if (chordizerSnapshot.connected)
        {
            const auto playheadMoved = std::abs (chordizerSnapshot.playheadPpq - previousPlayhead) > 0.0001
                                    || std::abs (chordizerSnapshot.playheadPpq - lastTimelinePlayheadPpq) > 0.0001;
            const auto visiblePpq = juce::jmax (1, chordizerSnapshot.numerator) * 6.0;
            if (chordizerSnapshot.playing || playheadMoved)
                timelineScrollPpq = juce::jmax (0.0, chordizerSnapshot.playheadPpq - visiblePpq * 0.36);
            lastTimelinePlayheadPpq = chordizerSnapshot.playheadPpq;
        }
    }
    recordedMidi = processorRef.recordedMidiSnapshot();
    updateRecorderControls();
    updateMaskToggles();
    repaint (timelineBounds.expanded (3).getUnion (midiRecordBounds.expanded (3)));
}

void SoliVoicerAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (background());
    auto surface = getLocalBounds().toFloat().reduced (14.0f);
    g.setColour (panel());
    g.fillRoundedRectangle (surface, 7.0f);
    g.setColour (line());
    g.drawRoundedRectangle (surface, 7.0f, 1.0f);
    g.setColour (green().withAlpha (0.85f));
    g.fillRect (surface.getX(), surface.getY(), surface.getWidth(), 3.0f);
    paintGroupFrame (g, midiRecordBounds, {});
    paintGroupFrame (g, voicingGroupBounds, "Voicing");
    paintGroupFrame (g, performanceGroupBounds, "Performance");
    if (sourceModeBox.getSelectedItemIndex() == 1)
        paintChordizerTimeline (g);
    paintRecordedMidi (g);
}

void SoliVoicerAudioProcessorEditor::paintChordizerTimeline (juce::Graphics& g)
{
    if (timelineBounds.isEmpty())
        return;
    auto bounds = timelineBounds.toFloat();
    g.setColour (background());
    g.fillRoundedRectangle (bounds, 5.0f);
    g.setColour (line());
    g.drawRoundedRectangle (bounds, 5.0f, 1.0f);

    if (! chordizerSnapshot.connected)
    {
        g.setColour (muted());
        g.setFont (juce::FontOptions (13.0f));
        g.drawText ("Open a Chordizer instance to link its shared chord track.", timelineBounds,
                    juce::Justification::centred);
        return;
    }

    const auto playhead = chordizerSnapshot.playheadPpq;
    const auto beatsPerBar = juce::jmax (1, chordizerSnapshot.numerator);
    const auto visibleWidth = beatsPerBar * 6.0;
    if (timelineScrollPpq <= 0.0 && playhead > 0.0)
        timelineScrollPpq = juce::jmax (0.0, playhead - visibleWidth * 0.36);
    const auto visibleStart = juce::jmax (0.0, timelineScrollPpq);
    const auto visibleEnd = visibleStart + visibleWidth;
    const auto xForPpq = [&] (double ppq)
    {
        return bounds.getX() + static_cast<float> ((ppq - visibleStart) / (visibleEnd - visibleStart)) * bounds.getWidth();
    };

    g.saveState();
    g.reduceClipRegion (timelineBounds);
    for (double bar = std::floor (visibleStart / beatsPerBar) * beatsPerBar;
         bar <= visibleEnd; bar += beatsPerBar)
    {
        const auto x = xForPpq (bar);
        g.setColour (line().withAlpha (0.55f));
        g.drawVerticalLine (static_cast<int> (x), bounds.getY(), bounds.getBottom());
        g.setColour (muted());
        g.setFont (juce::FontOptions (10.0f));
        g.drawText (juce::String (static_cast<int> (bar / beatsPerBar) + 1),
                    static_cast<int> (x + 3), timelineBounds.getY() + 2, 28, 14,
                    juce::Justification::centredLeft);
    }

    for (const auto& region : chordizerSnapshot.regions)
    {
        if (region.endPpq <= visibleStart || region.startPpq >= visibleEnd)
            continue;
        const auto left = xForPpq (juce::jmax (region.startPpq, visibleStart));
        const auto right = xForPpq (juce::jmin (region.endPpq, visibleEnd));
        auto regionBounds = juce::Rectangle<float> (left + 1.0f, bounds.getY() + 19.0f,
                                                     juce::jmax (4.0f, right - left - 2.0f),
                                                     bounds.getHeight() - 24.0f);
        const auto hue = static_cast<float> ((region.name.hashCode() & 1023) / 1023.0);
        auto colour = juce::Colour::fromHSV (hue, 0.48f, 0.76f, 1.0f);
        g.setColour (colour);
        g.fillRoundedRectangle (regionBounds, 3.0f);
        g.setColour (text());
        g.setFont (juce::FontOptions (juce::jlimit (10.0f, 15.0f, regionBounds.getHeight() * 0.34f),
                                      juce::Font::bold));
        g.drawFittedText (region.name, regionBounds.toNearestInt().reduced (4, 1),
                          juce::Justification::centred, 1);
    }

    const auto playheadX = xForPpq (playhead);
    g.setColour (amber());
    g.fillRect (playheadX - 1.0f, bounds.getY(), 2.0f, bounds.getHeight());
    g.restoreState();
}

void SoliVoicerAudioProcessorEditor::paintGroupFrame (juce::Graphics& g,
                                                      juce::Rectangle<int> bounds,
                                                      const juce::String& title)
{
    if (bounds.isEmpty())
        return;

    auto frame = bounds.toFloat();
    g.setColour (panelRaised().withAlpha (0.46f));
    g.fillRoundedRectangle (frame, 6.0f);
    g.setColour (line());
    g.drawRoundedRectangle (frame, 6.0f, 1.0f);
    if (title.isNotEmpty())
    {
        g.setColour (muted());
        g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
        g.drawText (title, bounds.reduced (12, 4).removeFromTop (16), juce::Justification::centredLeft);
    }
}

void SoliVoicerAudioProcessorEditor::paintRecordedMidi (juce::Graphics& g)
{
    if (midiShapeBounds.isEmpty())
        return;

    auto bounds = midiShapeBounds.toFloat();
    g.setColour (background());
    g.fillRoundedRectangle (bounds, 4.0f);
    g.setColour (line().withAlpha (0.8f));
    g.drawRoundedRectangle (bounds, 4.0f, 1.0f);

    if (! recordedMidi.hasOrigin || recordedMidi.events.empty())
    {
        g.setColour (muted());
        g.setFont (juce::FontOptions (12.0f));
        g.drawText ("Empty take", midiShapeBounds, juce::Justification::centred);
        return;
    }

    struct Span
    {
        double start = 0.0;
        double end = 0.0;
        int note = 60;
        int velocity = 96;
    };

    auto events = recordedMidi.events;
    std::sort (events.begin(), events.end(), [] (const auto& a, const auto& b)
    {
        if (std::abs (a.ppq - b.ppq) > 0.0000001)
            return a.ppq < b.ppq;
        return a.message.isNoteOff() && b.message.isNoteOn();
    });

    std::array<double, 16 * 128> starts;
    std::array<int, 16 * 128> velocities;
    starts.fill (-1.0);
    velocities.fill (96);
    std::vector<Span> spans;
    auto minNote = 127;
    auto maxNote = 0;

    for (const auto& event : events)
    {
        const auto& message = event.message;
        if (! message.isNoteOnOrOff())
            continue;

        const auto index = static_cast<std::size_t> ((juce::jlimit (1, 16, message.getChannel()) - 1) * 128
                                                     + juce::jlimit (0, 127, message.getNoteNumber()));
        if (message.isNoteOn())
        {
            starts[index] = event.ppq;
            velocities[index] = juce::jlimit (1, 127, static_cast<int> (message.getVelocity()));
            minNote = juce::jmin (minNote, message.getNoteNumber());
            maxNote = juce::jmax (maxNote, message.getNoteNumber());
        }
        else if (starts[index] >= 0.0)
        {
            spans.push_back ({ starts[index], juce::jmax (starts[index] + 0.015, event.ppq),
                               message.getNoteNumber(), velocities[index] });
            starts[index] = -1.0;
            minNote = juce::jmin (minNote, message.getNoteNumber());
            maxNote = juce::jmax (maxNote, message.getNoteNumber());
        }
    }

    for (std::size_t index = 0; index < starts.size(); ++index)
    {
        if (starts[index] < 0.0)
            continue;
        const auto note = static_cast<int> (index % 128);
        spans.push_back ({ starts[index], juce::jmax (starts[index] + 0.015, recordedMidi.endPpq),
                           note, velocities[index] });
        minNote = juce::jmin (minNote, note);
        maxNote = juce::jmax (maxNote, note);
    }

    if (spans.empty())
        return;

    const auto origin = recordedMidi.originPpq;
    const auto end = juce::jmax (origin + 0.25, recordedMidi.endPpq);
    const auto duration = end - origin;
    const auto noteRange = juce::jmax (1, maxNote - minNote + 1);
    auto lane = bounds.reduced (7.0f, 5.0f);

    g.saveState();
    g.reduceClipRegion (midiShapeBounds);
    g.setColour (line().withAlpha (0.32f));
    for (auto beat = std::floor (origin); beat <= end; beat += 1.0)
    {
        const auto x = lane.getX() + static_cast<float> ((beat - origin) / duration) * lane.getWidth();
        g.drawVerticalLine (static_cast<int> (x), lane.getY(), lane.getBottom());
    }

    for (const auto& span : spans)
    {
        const auto x = lane.getX() + static_cast<float> ((span.start - origin) / duration) * lane.getWidth();
        const auto x2 = lane.getX() + static_cast<float> ((span.end - origin) / duration) * lane.getWidth();
        const auto y = lane.getBottom() - static_cast<float> (span.note - minNote + 1) / static_cast<float> (noteRange) * lane.getHeight();
        const auto h = juce::jlimit (3.0f, 9.0f, lane.getHeight() / static_cast<float> (noteRange) * 0.84f);
        const auto alpha = juce::jlimit (0.42f, 1.0f, span.velocity / 127.0f);
        g.setColour ((recordedMidi.recording ? amber() : green()).withAlpha (alpha));
        g.fillRoundedRectangle (juce::Rectangle<float> (x, y, juce::jmax (2.0f, x2 - x), h), 2.0f);
    }
    g.restoreState();
}

void SoliVoicerAudioProcessorEditor::beginRecordedMidiDrag()
{
    if (processorRef.isMidiRecording())
        return;

    auto directory = juce::File::getSpecialLocation (juce::File::tempDirectory)
        .getChildFile ("Santismo")
        .getChildFile ("Voicizer MIDI Captures");
    const auto destination = directory.getNonexistentChildFile ("Voicizer MIDI Capture", ".mid", false);
    if (! processorRef.writeRecordedMidiFile (destination))
        return;

    lastMidiExportFile = destination;
    juce::StringArray files;
    files.add (destination.getFullPathName());
    juce::DragAndDropContainer::performExternalDragDropOfFiles (files, false, this);
}

void SoliVoicerAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced (28);
    auto header = bounds.removeFromTop (48);
    titleLabel.setBounds (header.removeFromLeft (160));
    randomButton.setBounds (header.removeFromRight (116).reduced (4, 8));
    resetButton.setBounds (header.removeFromRight (72).reduced (4, 8));
    chordLabel.setBounds (header.reduced (8, 2));

    bounds.removeFromTop (8);
    auto modeRow = bounds.removeFromTop (58);
    const auto modeWidth = juce::jmin (270, modeRow.getWidth() / 3);
    auto placeCombo = [&] (juce::ComboBox& combo, juce::Label& label, int width)
    {
        auto cell = modeRow.removeFromLeft (width).reduced (5, 0);
        label.setBounds (cell.removeFromTop (19));
        combo.setBounds (cell.removeFromTop (34));
    };
    placeCombo (sourceModeBox, sourceModeLabel, modeWidth);
    placeCombo (outputModeBox, outputModeLabel, modeWidth);
    if (sourceModeBox.getSelectedItemIndex() == 1)
        placeCombo (contextModeBox, contextModeLabel, modeWidth);

    bounds.removeFromTop (8);
    if (sourceModeBox.getSelectedItemIndex() == 1)
    {
        auto linkedArea = bounds.removeFromTop (118);
        auto status = linkedArea.removeFromTop (20);
        linkStatusLabel.setBounds (status.removeFromLeft (180));
        timelineBounds = linkedArea.reduced (2, 2);
    }
    else
    {
        timelineBounds = {};
        auto manual = bounds.removeFromTop (122);
        auto keys = manual.removeFromTop (52);
        auto keyHeader = keys.removeFromTop (18);
        randomKeyScaleButton.setBounds (keyHeader.removeFromRight (140).reduced (4, 0));
        keyLabel.setBounds (keyHeader);
        const auto keyWidth = keys.getWidth() / 12;
        for (int i = 0; i < 12; ++i)
            keyToggles[static_cast<std::size_t> (i)].setBounds (keys.getX() + i * keyWidth, keys.getY(), keyWidth, 28);
        auto scales = manual.removeFromTop (70);
        scaleLabel.setBounds (scales.removeFromTop (18));
        const auto scaleWidth = scales.getWidth() / 6;
        for (int i = 0; i < 12; ++i)
            scaleToggles[static_cast<std::size_t> (i)].setBounds (scales.getX() + (i % 6) * scaleWidth,
                                                                 scales.getY() + (i / 6) * 25,
                                                                 scaleWidth, 24);
    }

    bounds.removeFromTop (8);
    midiRecordBounds = bounds.removeFromTop (62).reduced (0, 1);
    auto midiInner = midiRecordBounds.reduced (10, 6);
    auto midiTop = midiInner.removeFromTop (24);
    midiRecordLabel.setBounds (midiTop.removeFromLeft (96));
    midiRecordStatusLabel.setBounds (midiTop.removeFromLeft (150));
    clearMidiButton.setBounds (midiTop.removeFromRight (70).reduced (3, 1));
    dragMidiButton.setBounds (midiTop.removeFromRight (78).reduced (3, 1));
    recordMidiButton.setBounds (midiTop.removeFromRight (92).reduced (3, 1));
    midiShapeBounds = midiInner.reduced (0, 1);

    bounds.removeFromTop (8);
    auto commonRow = bounds.removeFromTop (58);
    randomVoicingButton.setBounds (commonRow.removeFromRight (150).reduced (5, 18));
    const auto commonWidth = commonRow.getWidth() / 4;
    auto placeCommon = [&] (juce::ComboBox& combo, juce::Label& label)
    {
        auto cell = commonRow.removeFromLeft (commonWidth).reduced (5, 0);
        label.setBounds (cell.removeFromTop (19));
        combo.setBounds (cell.removeFromTop (34));
    };
    placeCommon (roleBox, roleLabel);
    placeCommon (styleBox, styleLabel);
    placeCommon (playabilityBox, playabilityLabel);
    placeCommon (strumModeBox, strumModeLabel);

    bounds.removeFromTop (8);
    std::vector<std::pair<juce::Slider*, juce::Label*>> voicingControls
    {
        { &chordSizeSlider, &chordSizeLabel },
        { &complexitySlider, &complexityLabel },
        { &voiceLeadingSlider, &voiceLeadingLabel },
        { &outsideSlider, &outsideLabel },
        { &variationSlider, &variationLabel },
        { &minNoteSlider, &minNoteLabel },
        { &maxNoteSlider, &maxNoteLabel }
    };
    if (sourceModeBox.getSelectedItemIndex() == 1)
        voicingControls.push_back ({ &substitutionSlider, &substitutionLabel });
    voicingControls.push_back ({ &repeatSlider, &repeatLabel });
    voicingControls.push_back ({ &strumSpeedSlider, &strumSpeedLabel });
    if (outputModeBox.getSelectedItemIndex() == 0)
    {
        voicingGroupBounds = bounds;
        performanceGroupBounds = {};
        layoutSliderGrid (voicingGroupBounds.reduced (12, 24), voicingControls);
    }
    else
    {
        auto split = bounds;
        const auto voicingWidth = juce::jlimit (360, 560, static_cast<int> (split.getWidth() * 0.46f));
        voicingGroupBounds = split.removeFromLeft (voicingWidth).reduced (0, 1);
        split.removeFromLeft (8);
        performanceGroupBounds = split.reduced (0, 1);
        layoutSliderGrid (voicingGroupBounds.reduced (12, 24), voicingControls);

        auto performanceInner = performanceGroupBounds.reduced (12, 24);
        auto performanceRow = performanceInner.removeFromTop (58);
        auto placePerformanceCombo = [&] (juce::ComboBox& combo, juce::Label& label, int width)
        {
            auto cell = performanceRow.removeFromLeft (width).reduced (5, 0);
            label.setBounds (cell.removeFromTop (19));
            combo.setBounds (cell.removeFromTop (34));
        };
        const auto selectorWidth = juce::jmin (250, performanceRow.getWidth() / 3);
        placePerformanceCombo (performanceStyleBox, performanceStyleLabel, selectorWidth);
        placePerformanceCombo (performanceSubStyleBox, performanceSubStyleLabel, selectorWidth);
        randomPerformanceButton.setBounds (performanceRow.removeFromRight (168).reduced (5, 18));
        doubleTimeButton.setBounds (performanceRow.removeFromRight (130).reduced (6, 16));

        std::vector<std::pair<juce::Slider*, juce::Label*>> performanceControls
        {
            { &performanceComplexitySlider, &performanceComplexityLabel },
            { &densitySlider, &densityLabel },
            { &syncopationSlider, &syncopationLabel },
            { &swingSlider, &swingLabel },
            { &humanizeSlider, &humanizeLabel },
            { &gateSlider, &gateLabel }
        };
        layoutSliderGrid (performanceInner.reduced (0, 2), performanceControls);
    }
}

void SoliVoicerAudioProcessorEditor::layoutSliderGrid (
    juce::Rectangle<int> bounds,
    const std::vector<std::pair<juce::Slider*, juce::Label*>>& controls)
{
    const auto columns = bounds.getWidth() < 430 ? 3 : (bounds.getWidth() < 620 ? 4 : 5);
    const auto rows = juce::jmax (1, (static_cast<int> (controls.size()) + columns - 1) / columns);
    const auto cellWidth = bounds.getWidth() / columns;
    const auto cellHeight = bounds.getHeight() / rows;
    for (std::size_t i = 0; i < controls.size(); ++i)
    {
        const auto column = static_cast<int> (i) % columns;
        const auto row = static_cast<int> (i) / columns;
        auto cell = juce::Rectangle<int> (bounds.getX() + column * cellWidth,
                                          bounds.getY() + row * cellHeight,
                                          cellWidth, cellHeight).reduced (6, 2);
        controls[i].second->setBounds (cell.removeFromTop (19));
        controls[i].first->setBounds (cell);
    }
}

void SoliVoicerAudioProcessorEditor::updateModeVisibility()
{
    const auto follow = sourceModeBox.getSelectedItemIndex() == 1;
    const auto performance = outputModeBox.getSelectedItemIndex() == 1;
    lastSourceMode = sourceModeBox.getSelectedItemIndex();
    lastOutputMode = outputModeBox.getSelectedItemIndex();

    keyLabel.setVisible (! follow);
    scaleLabel.setVisible (! follow);
    randomKeyScaleButton.setVisible (! follow);
    for (auto& button : keyToggles) button.setVisible (! follow);
    for (auto& button : scaleToggles) button.setVisible (! follow);
    contextModeBox.setVisible (follow);
    contextModeLabel.setVisible (follow);
    substitutionSlider.setVisible (follow);
    substitutionLabel.setVisible (follow);
    linkStatusLabel.setVisible (follow);

    strumModeBox.setVisible (true);
    strumModeLabel.setVisible (true);
    repeatSlider.setVisible (true);
    repeatLabel.setVisible (true);
    strumSpeedSlider.setVisible (true);
    strumSpeedLabel.setVisible (true);
    performanceStyleBox.setVisible (performance);
    performanceStyleLabel.setVisible (performance);
    performanceSubStyleBox.setVisible (performance);
    performanceSubStyleLabel.setVisible (performance);
    doubleTimeButton.setVisible (performance);
    randomPerformanceButton.setVisible (performance);
    const std::array<juce::Component*, 16> performanceComponents
    {{
        &performanceComplexitySlider, &performanceComplexityLabel,
        &densitySlider, &densityLabel, &syncopationSlider, &syncopationLabel,
        &swingSlider, &swingLabel, &humanizeSlider, &humanizeLabel,
        &gateSlider, &gateLabel, &performanceSubStyleBox, &performanceSubStyleLabel,
        &doubleTimeButton, &randomPerformanceButton
    }};
    for (auto* component : performanceComponents)
        component->setVisible (performance);

    resized();
    repaint();
}

void SoliVoicerAudioProcessorEditor::updateMaskToggles()
{
    if (syncingMasks)
        return;
    const juce::ScopedValueSetter<bool> guard (syncingMasks, true);
    const auto update = [this] (const char* id, auto& toggles)
    {
        if (auto* parameter = processorRef.getValueTreeState().getParameter (id))
        {
            const auto mask = juce::jlimit (1, 4095,
                static_cast<int> (parameter->convertFrom0to1 (parameter->getValue()) + 0.5f));
            for (int i = 0; i < 12; ++i)
                toggles[static_cast<std::size_t> (i)].setToggleState ((mask & (1 << i)) != 0,
                                                                     juce::dontSendNotification);
        }
    };
    update (ParameterIDs::keyMask, keyToggles);
    update (ParameterIDs::scaleMask, scaleToggles);
}

void SoliVoicerAudioProcessorEditor::commitMask (const juce::String& id,
                                                 const std::array<juce::ToggleButton, 12>& toggles,
                                                 int count)
{
    if (syncingMasks)
        return;
    auto mask = 0;
    for (int i = 0; i < count; ++i)
        if (toggles[static_cast<std::size_t> (i)].getToggleState())
            mask |= 1 << i;
    setParameterValue (id, static_cast<float> (juce::jmax (1, mask)));
}

void SoliVoicerAudioProcessorEditor::setParameterValue (const juce::String& id, float value)
{
    if (auto* parameter = processorRef.getValueTreeState().getParameter (id))
    {
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (value));
        parameter->endChangeGesture();
    }
}

void SoliVoicerAudioProcessorEditor::updatePerformanceSubStyleChoices()
{
    const auto styleIndex = juce::jmax (0, performanceStyleBox.getSelectedItemIndex());
    if (styleIndex == lastPerformanceStyle && performanceSubStyleBox.getNumItems() > 0)
        return;

    const juce::ScopedValueSetter<bool> guard (updatingSubStyleChoices, true);
    const auto selected = juce::jmax (0, performanceSubStyleBox.getSelectedItemIndex());
    const auto names = SoliVoicerAudioProcessor::performanceSubStyleNames (styleIndex);
    performanceSubStyleBox.clear (juce::dontSendNotification);
    for (int i = 0; i < names.size(); ++i)
        performanceSubStyleBox.addItem (names[i], i + 1);
    performanceSubStyleBox.setSelectedItemIndex (juce::jlimit (0, names.size() - 1, selected),
                                                juce::dontSendNotification);
    lastPerformanceStyle = styleIndex;
}

void SoliVoicerAudioProcessorEditor::updateRecorderControls()
{
    const auto isRecording = recordedMidi.recording || processorRef.isMidiRecording();
    recordMidiButton.setToggleState (isRecording, juce::dontSendNotification);
    recordMidiButton.setButtonText (isRecording ? "Stop MIDI" : "Record MIDI");
    const auto eventCount = static_cast<int> (recordedMidi.events.size());
    dragMidiButton.setEnabled (! isRecording && eventCount > 0);
    clearMidiButton.setEnabled (eventCount > 0 || recordedMidi.hasOrigin);

    if (isRecording)
        midiRecordStatusLabel.setText ("Recording", juce::dontSendNotification);
    else if (eventCount > 0)
        midiRecordStatusLabel.setText (juce::String (eventCount / 2) + " notes captured", juce::dontSendNotification);
    else
        midiRecordStatusLabel.setText ("No take", juce::dontSendNotification);
}

void SoliVoicerAudioProcessorEditor::randomizeKeyScaleSettings()
{
    auto& random = juce::Random::getSystemRandom();
    const auto randomMask = [&random] (int total, int minCount, int maxCount)
    {
        const auto count = minCount + random.nextInt (juce::jmax (1, maxCount - minCount + 1));
        auto mask = 0;
        const auto bitCount = [] (int value)
        {
            auto result = 0;
            while (value != 0)
            {
                result += value & 1;
                value >>= 1;
            }
            return result;
        };
        while (bitCount (mask) < count)
            mask |= 1 << random.nextInt (total);
        return juce::jmax (1, mask);
    };

    setParameterValue (ParameterIDs::keyMask, static_cast<float> (randomMask (12, 1, 3)));
    setParameterValue (ParameterIDs::scaleMask, static_cast<float> (randomMask (Soli::ChordEngine::scaleNames().size(), 1, 4)));
    updateMaskToggles();
}

void SoliVoicerAudioProcessorEditor::randomizeVoicingSettings()
{
    auto& random = juce::Random::getSystemRandom();
    const auto minNote = 28 + random.nextInt (25);
    const auto maxNote = juce::jlimit (minNote + 12, 127, minNote + 36 + random.nextInt (36));

    setParameterValue (ParameterIDs::role, static_cast<float> (random.nextInt (Soli::ChordEngine::roleNames().size())));
    setParameterValue (ParameterIDs::style, static_cast<float> (random.nextInt (Soli::ChordEngine::styleNames().size())));
    setParameterValue (ParameterIDs::playability, static_cast<float> (random.nextInt (Soli::ChordEngine::playabilityNames().size())));
    setParameterValue (ParameterIDs::strumMode, static_cast<float> (random.nextInt (Soli::ChordEngine::strumModeNames().size())));
    setParameterValue (ParameterIDs::chordSize, static_cast<float> (3 + random.nextInt (7)));
    setParameterValue (ParameterIDs::complexity, random.nextFloat());
    setParameterValue (ParameterIDs::voiceLeading, 0.45f + random.nextFloat() * 0.55f);
    setParameterValue (ParameterIDs::outside, random.nextFloat() * 0.55f);
    setParameterValue (ParameterIDs::variation, random.nextFloat() * 0.8f);
    setParameterValue (ParameterIDs::repeatChance, random.nextFloat() * 0.45f);
    setParameterValue (ParameterIDs::strumSpeed, random.nextFloat() * 0.65f);
    setParameterValue (ParameterIDs::minNote, static_cast<float> (minNote));
    setParameterValue (ParameterIDs::maxNote, static_cast<float> (maxNote));
    setParameterValue (ParameterIDs::contextMode, static_cast<float> (random.nextInt (Soli::ChordEngine::contextModeNames().size())));
    setParameterValue (ParameterIDs::substitutionDepth, random.nextFloat() * 0.8f);
    processorRef.panic();
}

void SoliVoicerAudioProcessorEditor::randomizeAllSettings()
{
    randomizeKeyScaleSettings();
    randomizeVoicingSettings();
    randomizePerformanceSettings();
    updateMaskToggles();
}

void SoliVoicerAudioProcessorEditor::randomizePerformanceSettings()
{
    auto& random = juce::Random::getSystemRandom();
    const auto style = random.nextInt (SoliVoicerAudioProcessor::performanceStyleNames().size());
    setParameterValue (ParameterIDs::performanceStyle, static_cast<float> (style));
    performanceStyleBox.setSelectedItemIndex (style, juce::dontSendNotification);
    lastPerformanceStyle = -1;
    updatePerformanceSubStyleChoices();
    setParameterValue (ParameterIDs::performanceSubStyle,
                       static_cast<float> (random.nextInt (SoliVoicerAudioProcessor::performanceSubStyleNames (style).size())));
    setParameterValue (ParameterIDs::performanceComplexity, 0.25f + random.nextFloat() * 0.75f);
    setParameterValue (ParameterIDs::rhythmDensity, 0.25f + random.nextFloat() * 0.75f);
    setParameterValue (ParameterIDs::syncopation, random.nextFloat() * 0.85f);
    setParameterValue (ParameterIDs::swing, random.nextFloat() * 0.72f);
    setParameterValue (ParameterIDs::humanize, 0.04f + random.nextFloat() * 0.34f);
    setParameterValue (ParameterIDs::gate, 0.28f + random.nextFloat() * 0.64f);
    setParameterValue (ParameterIDs::doubleTime, random.nextBool() ? 1.0f : 0.0f);
    processorRef.panic();
}

void SoliVoicerAudioProcessorEditor::resetDefaults()
{
    setParameterValue (ParameterIDs::keyMask, 1.0f);
    setParameterValue (ParameterIDs::scaleMask, 1.0f);
    setParameterValue (ParameterIDs::role, 0.0f);
    setParameterValue (ParameterIDs::style, 0.0f);
    setParameterValue (ParameterIDs::playability, 0.0f);
    setParameterValue (ParameterIDs::strumMode, 0.0f);
    setParameterValue (ParameterIDs::chordSize, 4.0f);
    setParameterValue (ParameterIDs::complexity, 0.45f);
    setParameterValue (ParameterIDs::voiceLeading, 0.75f);
    setParameterValue (ParameterIDs::outside, 0.10f);
    setParameterValue (ParameterIDs::variation, 0.35f);
    setParameterValue (ParameterIDs::repeatChance, 0.15f);
    setParameterValue (ParameterIDs::strumSpeed, 0.0f);
    setParameterValue (ParameterIDs::minNote, 36.0f);
    setParameterValue (ParameterIDs::maxNote, 96.0f);
    setParameterValue (ParameterIDs::contextMode, 3.0f);
    setParameterValue (ParameterIDs::substitutionDepth, 0.35f);
    setParameterValue (ParameterIDs::performanceStyle, 0.0f);
    setParameterValue (ParameterIDs::performanceSubStyle, 0.0f);
    setParameterValue (ParameterIDs::performanceComplexity, 0.45f);
    setParameterValue (ParameterIDs::rhythmDensity, 0.48f);
    setParameterValue (ParameterIDs::syncopation, 0.20f);
    setParameterValue (ParameterIDs::swing, 0.0f);
    setParameterValue (ParameterIDs::humanize, 0.12f);
    setParameterValue (ParameterIDs::gate, 0.72f);
    setParameterValue (ParameterIDs::doubleTime, 0.0f);
    performanceStyleBox.setSelectedItemIndex (0, juce::dontSendNotification);
    lastPerformanceStyle = -1;
    updatePerformanceSubStyleChoices();
    processorRef.panic();
    updateMaskToggles();
}
