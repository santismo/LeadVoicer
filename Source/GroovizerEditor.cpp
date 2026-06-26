#include "GroovizerEditor.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace
{
juce::Colour background() { return juce::Colour (0xff0b0f12); }
juce::Colour panel() { return juce::Colour (0xff171d22); }
juce::Colour panelRaised() { return juce::Colour (0xff222a31); }
juce::Colour line() { return juce::Colour (0xff35424c); }
juce::Colour text() { return juce::Colour (0xfff3f7f6); }
juce::Colour muted() { return juce::Colour (0xff9ba9ad); }
juce::Colour green() { return juce::Colour (0xff43d28f); }
juce::Colour amber() { return juce::Colour (0xffffbd5a); }
juce::Colour red() { return juce::Colour (0xffff6d6d); }

double visibleEnd (const GroovizerAudioProcessor::TimelineSnapshot& timeline)
{
    auto end = juce::jmax (16.0, timeline.cursorPpq + 4.0);
    for (const auto& region : timeline.regions)
        end = juce::jmax (end, region.startPpq + region.lengthPpq + 4.0);
    return end;
}
}

GroovizerLookAndFeel::GroovizerLookAndFeel()
{
    setColour (juce::ComboBox::backgroundColourId, panelRaised());
    setColour (juce::ComboBox::outlineColourId, line());
    setColour (juce::ComboBox::textColourId, text());
    setColour (juce::PopupMenu::backgroundColourId, panel());
    setColour (juce::PopupMenu::textColourId, text());
    setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xff285846));
    setColour (juce::TextButton::buttonColourId, panelRaised());
    setColour (juce::TextButton::textColourOffId, text());
    setColour (juce::ToggleButton::textColourId, text());
    setColour (juce::ToggleButton::tickColourId, green());
    setColour (juce::Slider::thumbColourId, green());
    setColour (juce::Slider::trackColourId, green());
    setColour (juce::Slider::backgroundColourId, line());
}

void GroovizerLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool,
                                         int, int, int, int, juce::ComboBox&)
{
    auto bounds = juce::Rectangle<float> (0.5f, 0.5f, static_cast<float> (width) - 1.0f,
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

void GroovizerMidiDragButton::mouseDown (const juce::MouseEvent& event)
{
    dragStarted = false;
    juce::TextButton::mouseDown (event);
}

void GroovizerMidiDragButton::mouseDrag (const juce::MouseEvent& event)
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

void GroovizerMidiDragButton::mouseUp (const juce::MouseEvent& event)
{
    const auto wasDragStarted = dragStarted;
    dragStarted = false;
    if (! wasDragStarted)
        juce::TextButton::mouseUp (event);
}

GroovizerAudioProcessorEditor::GroovizerAudioProcessorEditor (GroovizerAudioProcessor& owner)
    : AudioProcessorEditor (&owner), processorRef (owner)
{
    setLookAndFeel (&lookAndFeel);
    setResizable (true, true);
    setResizeLimits (820, 620, 1500, 1000);
    setSize (1060, 760);
    tooltipWindow = std::make_unique<juce::TooltipWindow> (this, 700);

    addAndMakeVisible (titleLabel);
    configureLabel (titleLabel, "GROOVIZER", 23.0f, true);
    addAndMakeVisible (statusLabel);
    configureLabel (statusLabel, "--", 17.0f, true);
    statusLabel.setJustificationType (juce::Justification::centredRight);

    addLabeledCombo (styleBox, styleLabel, "Groove Style", Groovizer::Engine::styleNames());
    addLabeledCombo (triggerBox, triggerLabel, "Trigger", Groovizer::Engine::triggerNames());
    addLabeledCombo (phraseBox, phraseLabel, "Phrase", Groovizer::Engine::phraseNames());
    addLabeledCombo (feelBox, feelLabel, "Feel", Groovizer::Engine::feelNames());
    addLabeledSlider (lengthSlider, lengthLabel, "Bars", 1.0, 8.0, 1.0);
    addLabeledSlider (densitySlider, densityLabel, "Density", 0.0, 1.0, 0.01);
    addLabeledSlider (swingSlider, swingLabel, "Swing", 0.0, 1.0, 0.01);
    addLabeledSlider (humanizeSlider, humanizeLabel, "Humanize", 0.0, 1.0, 0.01);
    addLabeledSlider (fillSlider, fillLabel, "Fill", 0.0, 1.0, 0.01);
    addLabeledSlider (variationSlider, variationLabel, "Variation", 0.0, 1.0, 0.01);
    addLabeledSlider (ghostsSlider, ghostsLabel, "Ghosts", 0.0, 1.0, 0.01);
    addLabeledSlider (channelSlider, channelLabel, "GM Ch", 1.0, 16.0, 1.0);

    std::array<juce::Button*, 15> buttons
    {{
        &liveButton, &stepInputButton, &timelineButton, &passInputButton,
        &addRegionButton, &randomButton, &deleteButton, &clearTimelineButton,
        &cursorBackButton, &cursorNextButton, &recordMidiButton,
        &dragTimelineButton, &dragCaptureButton, &clearCaptureButton, &panicButton
    }};
    for (auto* button : buttons)
        addAndMakeVisible (*button);

    liveButton.setTooltip ("Incoming GM drum notes trigger generated grooves immediately.");
    stepInputButton.setTooltip ("Incoming GM drum notes write groove regions at the Groovizer cursor.");
    timelineButton.setTooltip ("Play Groovizer timeline regions from Logic's transport.");
    passInputButton.setTooltip ("Pass original incoming MIDI through in addition to generated drums.");
    addRegionButton.setTooltip ("Add one groove region at the Groovizer cursor.");
    randomButton.setTooltip ("Create an idiomatic arrangement at the cursor.");
    dragTimelineButton.setTooltip ("Drag the Groovizer timeline as a MIDI file into Logic.");
    dragCaptureButton.setTooltip ("Drag the recorded output take as a MIDI file into Logic.");
    recordMidiButton.setTooltip ("Record Groovizer's generated MIDI output.");
    channelSlider.setTooltip ("GM drums normally use MIDI channel 10.");

    addAndMakeVisible (captureStatusLabel);
    configureLabel (captureStatusLabel, "No capture", 12.0f, false);

    addRegionButton.onClick = [this] { processorRef.addRegionAtCursor(); updateStatus(); repaint(); };
    randomButton.onClick = [this] { processorRef.addRandomArrangement (4); updateStatus(); repaint(); };
    deleteButton.onClick = [this] { processorRef.deleteSelectedRegion(); updateStatus(); repaint(); };
    clearTimelineButton.onClick = [this] { processorRef.clearTimeline(); updateStatus(); repaint(); };
    cursorBackButton.onClick = [this] { processorRef.moveTimelineCursorBars (-1); updateStatus(); repaint(); };
    cursorNextButton.onClick = [this] { processorRef.moveTimelineCursorBars (1); updateStatus(); repaint(); };
    panicButton.onClick = [this] { processorRef.panic(); updateStatus(); };
    recordMidiButton.setClickingTogglesState (true);
    recordMidiButton.onClick = [this]
    {
        processorRef.setMidiRecordingEnabled (recordMidiButton.getToggleState());
        updateStatus();
        repaint();
    };
    clearCaptureButton.onClick = [this]
    {
        processorRef.clearRecordedMidi();
        updateStatus();
        repaint();
    };
    dragTimelineButton.onClick = [this] { beginTimelineDrag(); };
    dragTimelineButton.onDragStart = [this] { beginTimelineDrag(); };
    dragCaptureButton.onClick = [this] { beginCaptureDrag(); };
    dragCaptureButton.onDragStart = [this] { beginCaptureDrag(); };

    auto& state = processorRef.getValueTreeState();
    styleAttachment = std::make_unique<ComboAttachment> (state, GroovizerParameterIDs::style, styleBox);
    triggerAttachment = std::make_unique<ComboAttachment> (state, GroovizerParameterIDs::trigger, triggerBox);
    phraseAttachment = std::make_unique<ComboAttachment> (state, GroovizerParameterIDs::phrase, phraseBox);
    feelAttachment = std::make_unique<ComboAttachment> (state, GroovizerParameterIDs::feel, feelBox);
    lengthAttachment = std::make_unique<SliderAttachment> (state, GroovizerParameterIDs::lengthBars, lengthSlider);
    densityAttachment = std::make_unique<SliderAttachment> (state, GroovizerParameterIDs::density, densitySlider);
    swingAttachment = std::make_unique<SliderAttachment> (state, GroovizerParameterIDs::swing, swingSlider);
    humanizeAttachment = std::make_unique<SliderAttachment> (state, GroovizerParameterIDs::humanize, humanizeSlider);
    fillAttachment = std::make_unique<SliderAttachment> (state, GroovizerParameterIDs::fill, fillSlider);
    variationAttachment = std::make_unique<SliderAttachment> (state, GroovizerParameterIDs::variation, variationSlider);
    ghostsAttachment = std::make_unique<SliderAttachment> (state, GroovizerParameterIDs::ghosts, ghostsSlider);
    channelAttachment = std::make_unique<SliderAttachment> (state, GroovizerParameterIDs::outputChannel, channelSlider);
    liveAttachment = std::make_unique<ButtonAttachment> (state, GroovizerParameterIDs::liveMode, liveButton);
    stepInputAttachment = std::make_unique<ButtonAttachment> (state, GroovizerParameterIDs::stepInput, stepInputButton);
    timelineAttachment = std::make_unique<ButtonAttachment> (state, GroovizerParameterIDs::timelineEnabled, timelineButton);
    passInputAttachment = std::make_unique<ButtonAttachment> (state, GroovizerParameterIDs::passInput, passInputButton);

    updateStatus();
    startTimerHz (30);
}

GroovizerAudioProcessorEditor::~GroovizerAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void GroovizerAudioProcessorEditor::configureLabel (juce::Label& label, const juce::String& value, float size, bool bold)
{
    label.setText (value, juce::dontSendNotification);
    label.setFont (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain));
    label.setColour (juce::Label::textColourId, bold ? text() : muted());
    label.setJustificationType (juce::Justification::centredLeft);
}

void GroovizerAudioProcessorEditor::configureCombo (juce::ComboBox& combo, const juce::StringArray& names)
{
    for (int i = 0; i < names.size(); ++i)
        combo.addItem (names[i], i + 1);
    combo.setJustificationType (juce::Justification::centredLeft);
}

void GroovizerAudioProcessorEditor::configureSlider (juce::Slider& slider, double min, double max, double interval)
{
    slider.setRange (min, max, interval);
    slider.setSliderStyle (juce::Slider::LinearHorizontal);
    slider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 54, 20);
    slider.setColour (juce::Slider::textBoxTextColourId, text());
    slider.setColour (juce::Slider::textBoxBackgroundColourId, background());
    slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
}

void GroovizerAudioProcessorEditor::addLabeledCombo (juce::ComboBox& combo, juce::Label& label, const juce::String& name, const juce::StringArray& names)
{
    addAndMakeVisible (label);
    addAndMakeVisible (combo);
    configureLabel (label, name, 12.0f, true);
    configureCombo (combo, names);
}

void GroovizerAudioProcessorEditor::addLabeledSlider (juce::Slider& slider, juce::Label& label, const juce::String& name, double min, double max, double interval)
{
    addAndMakeVisible (label);
    addAndMakeVisible (slider);
    configureLabel (label, name, 12.0f, true);
    configureSlider (slider, min, max, interval);
}

void GroovizerAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (background());
    auto bounds = getLocalBounds().toFloat().reduced (18.0f);
    g.setColour (panel());
    g.fillRoundedRectangle (bounds, 8.0f);
    g.setColour (line());
    g.drawRoundedRectangle (bounds, 8.0f, 1.0f);
    paintTimeline (g);
    paintCapture (g);
}

void GroovizerAudioProcessorEditor::paintTimeline (juce::Graphics& g)
{
    if (timelineBounds.isEmpty())
        return;

    auto bounds = timelineBounds.toFloat();
    g.setColour (panelRaised());
    g.fillRoundedRectangle (bounds, 6.0f);
    g.setColour (line());
    g.drawRoundedRectangle (bounds, 6.0f, 1.0f);

    const auto lane = timelineBounds.reduced (10, 22).toFloat();
    const auto end = visibleEnd (timeline);
    g.setColour (muted());
    g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    g.drawText ("Groove Timeline", timelineBounds.reduced (10, 4).removeFromTop (18), juce::Justification::centredLeft);
    g.setFont (juce::FontOptions (10.0f));

    for (int bar = 0; bar <= static_cast<int> (std::ceil (end / 4.0)); ++bar)
    {
        const auto x = lane.getX() + static_cast<float> ((bar * 4.0) / end) * lane.getWidth();
        g.setColour (bar % 4 == 0 ? line().withAlpha (0.8f) : line().withAlpha (0.35f));
        g.drawVerticalLine (static_cast<int> (x), lane.getY(), lane.getBottom());
        g.setColour (muted().withAlpha (0.72f));
        g.drawText (juce::String (bar + 1), static_cast<int> (x + 3), timelineBounds.getY() + 5, 34, 14, juce::Justification::centredLeft);
    }

    for (int i = 0; i < static_cast<int> (timeline.regions.size()); ++i)
    {
        const auto& region = timeline.regions[static_cast<std::size_t> (i)];
        const auto x = lane.getX() + static_cast<float> (region.startPpq / end) * lane.getWidth();
        const auto w = juce::jmax (18.0f, static_cast<float> (region.lengthPpq / end) * lane.getWidth() - 3.0f);
        auto rect = juce::Rectangle<float> (x, lane.getY() + 8.0f, w, lane.getHeight() - 16.0f);
        const auto selected = i == timeline.selectedIndex;
        g.setColour ((selected ? amber() : green()).withAlpha (0.88f));
        g.fillRoundedRectangle (rect, 5.0f);
        g.setColour (background().withAlpha (0.42f));
        g.fillRoundedRectangle (rect.removeFromBottom (rect.getHeight() * 0.34f), 5.0f);
        g.setColour (background());
        g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
        g.drawFittedText (region.name, rect.toNearestInt().reduced (6, 1), juce::Justification::centredLeft, 1);
    }

    const auto cursorX = lane.getX() + static_cast<float> (timeline.cursorPpq / end) * lane.getWidth();
    g.setColour (amber());
    g.drawVerticalLine (static_cast<int> (cursorX), lane.getY() - 3.0f, lane.getBottom() + 3.0f);

    if (timeline.hostPlaying || timeline.hostPpq > 0.0)
    {
        const auto hostWrapped = std::fmod (juce::jmax (0.0, timeline.hostPpq), end);
        const auto hostX = lane.getX() + static_cast<float> (hostWrapped / end) * lane.getWidth();
        g.setColour (red());
        g.drawVerticalLine (static_cast<int> (hostX), lane.getY() - 1.0f, lane.getBottom() + 1.0f);
    }
}

void GroovizerAudioProcessorEditor::paintCapture (juce::Graphics& g)
{
    if (captureBounds.isEmpty())
        return;

    auto bounds = captureBounds.toFloat();
    g.setColour (panelRaised());
    g.fillRoundedRectangle (bounds, 6.0f);
    g.setColour (line());
    g.drawRoundedRectangle (bounds, 6.0f, 1.0f);
    if (! recordedMidi.hasOrigin || recordedMidi.events.empty())
        return;

    struct Span { double start; double end; int note; int velocity; };
    std::vector<Span> spans;
    std::array<double, 16 * 128> starts;
    std::array<int, 16 * 128> velocities;
    starts.fill (-1.0);
    velocities.fill (90);
    auto minNote = 127;
    auto maxNote = 0;
    for (const auto& event : recordedMidi.events)
    {
        const auto& message = event.message;
        if (! message.isNoteOnOrOff())
            continue;
        const auto index = (message.getChannel() - 1) * 128 + message.getNoteNumber();
        if (message.isNoteOn())
        {
            starts[static_cast<std::size_t> (index)] = event.ppq;
            velocities[static_cast<std::size_t> (index)] = message.getVelocity();
        }
        else if (starts[static_cast<std::size_t> (index)] >= 0.0)
        {
            spans.push_back ({ starts[static_cast<std::size_t> (index)], event.ppq, message.getNoteNumber(), velocities[static_cast<std::size_t> (index)] });
            starts[static_cast<std::size_t> (index)] = -1.0;
            minNote = juce::jmin (minNote, message.getNoteNumber());
            maxNote = juce::jmax (maxNote, message.getNoteNumber());
        }
    }
    if (spans.empty())
        return;

    const auto lane = captureBounds.reduced (9, 7).toFloat();
    const auto duration = juce::jmax (0.25, recordedMidi.endPpq - recordedMidi.originPpq);
    const auto range = juce::jmax (1, maxNote - minNote + 1);
    for (const auto& span : spans)
    {
        const auto x = lane.getX() + static_cast<float> ((span.start - recordedMidi.originPpq) / duration) * lane.getWidth();
        const auto x2 = lane.getX() + static_cast<float> ((span.end - recordedMidi.originPpq) / duration) * lane.getWidth();
        const auto y = lane.getBottom() - static_cast<float> (span.note - minNote + 1) / static_cast<float> (range) * lane.getHeight();
        g.setColour ((recordedMidi.recording ? amber() : green()).withAlpha (juce::jlimit (0.45f, 1.0f, span.velocity / 127.0f)));
        g.fillRoundedRectangle (juce::Rectangle<float> (x, y, juce::jmax (2.0f, x2 - x), 5.0f), 2.0f);
    }
}

void GroovizerAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced (28);
    auto header = bounds.removeFromTop (48);
    titleLabel.setBounds (header.removeFromLeft (180));
    panicButton.setBounds (header.removeFromRight (74).reduced (5, 8));
    statusLabel.setBounds (header.reduced (8, 2));

    bounds.removeFromTop (8);
    auto toggleRow = bounds.removeFromTop (34);
    for (auto* button : { &liveButton, &stepInputButton, &timelineButton, &passInputButton })
        button->setBounds (toggleRow.removeFromLeft (112).reduced (4, 3));
    cursorBackButton.setBounds (toggleRow.removeFromRight (76).reduced (4, 3));
    cursorNextButton.setBounds (toggleRow.removeFromRight (76).reduced (4, 3));

    bounds.removeFromTop (8);
    auto comboRow = bounds.removeFromTop (58);
    const auto comboWidth = comboRow.getWidth() / 4;
    auto placeCombo = [&] (juce::ComboBox& combo, juce::Label& label)
    {
        auto cell = comboRow.removeFromLeft (comboWidth).reduced (5, 0);
        label.setBounds (cell.removeFromTop (19));
        combo.setBounds (cell);
    };
    placeCombo (styleBox, styleLabel);
    placeCombo (triggerBox, triggerLabel);
    placeCombo (phraseBox, phraseLabel);
    placeCombo (feelBox, feelLabel);

    bounds.removeFromTop (8);
    auto sliderArea = bounds.removeFromTop (116);
    const std::array<std::pair<juce::Slider*, juce::Label*>, 8> sliders
    {{
        { &lengthSlider, &lengthLabel }, { &densitySlider, &densityLabel },
        { &swingSlider, &swingLabel }, { &humanizeSlider, &humanizeLabel },
        { &fillSlider, &fillLabel }, { &variationSlider, &variationLabel },
        { &ghostsSlider, &ghostsLabel }, { &channelSlider, &channelLabel }
    }};
    const auto columns = 4;
    const auto cellW = sliderArea.getWidth() / columns;
    const auto cellH = sliderArea.getHeight() / 2;
    for (int i = 0; i < static_cast<int> (sliders.size()); ++i)
    {
        auto cell = juce::Rectangle<int> (sliderArea.getX() + (i % columns) * cellW,
                                          sliderArea.getY() + (i / columns) * cellH,
                                          cellW,
                                          cellH).reduced (6, 2);
        sliders[static_cast<std::size_t> (i)].second->setBounds (cell.removeFromTop (18));
        sliders[static_cast<std::size_t> (i)].first->setBounds (cell.removeFromTop (30));
    }

    bounds.removeFromTop (8);
    auto actionRow = bounds.removeFromTop (36);
    addRegionButton.setBounds (actionRow.removeFromLeft (112).reduced (4, 3));
    randomButton.setBounds (actionRow.removeFromLeft (142).reduced (4, 3));
    deleteButton.setBounds (actionRow.removeFromLeft (82).reduced (4, 3));
    clearTimelineButton.setBounds (actionRow.removeFromLeft (82).reduced (4, 3));
    dragTimelineButton.setBounds (actionRow.removeFromRight (126).reduced (4, 3));

    bounds.removeFromTop (8);
    timelineBounds = bounds.removeFromTop (210);

    bounds.removeFromTop (10);
    auto captureRow = bounds.removeFromTop (36);
    captureStatusLabel.setBounds (captureRow.removeFromLeft (180).reduced (4, 3));
    recordMidiButton.setBounds (captureRow.removeFromLeft (110).reduced (4, 3));
    dragCaptureButton.setBounds (captureRow.removeFromLeft (122).reduced (4, 3));
    clearCaptureButton.setBounds (captureRow.removeFromLeft (120).reduced (4, 3));
    captureBounds = bounds.removeFromTop (80);
}

void GroovizerAudioProcessorEditor::mouseDown (const juce::MouseEvent& event)
{
    if (! timelineBounds.contains (event.getPosition()))
        return;

    const auto lane = timelineBounds.reduced (10, 22);
    const auto end = visibleEnd (timeline);
    const auto x = juce::jlimit (0, lane.getWidth(), event.x - lane.getX());
    const auto ppq = (static_cast<double> (x) / juce::jmax (1, lane.getWidth())) * end;
    processorRef.selectRegionAt (ppq);
    updateStatus();
    repaint();
}

void GroovizerAudioProcessorEditor::timerCallback()
{
    updateStatus();
    repaint (timelineBounds);
    repaint (captureBounds);
}

void GroovizerAudioProcessorEditor::updateStatus()
{
    timeline = processorRef.getTimelineSnapshot();
    recordedMidi = processorRef.recordedMidiSnapshot();
    statusLabel.setText (processorRef.getLastGrooveName(), juce::dontSendNotification);
    const auto recording = recordedMidi.recording || processorRef.isMidiRecording();
    recordMidiButton.setToggleState (recording, juce::dontSendNotification);
    recordMidiButton.setButtonText (recording ? "Stop MIDI" : "Record MIDI");
    dragTimelineButton.setEnabled (! timeline.regions.empty());
    dragCaptureButton.setEnabled (! recording && ! recordedMidi.events.empty());
    clearCaptureButton.setEnabled (! recordedMidi.events.empty() || recordedMidi.hasOrigin);
    deleteButton.setEnabled (timeline.selectedIndex >= 0);
    if (recording)
        captureStatusLabel.setText ("Recording generated MIDI", juce::dontSendNotification);
    else if (! recordedMidi.events.empty())
        captureStatusLabel.setText (juce::String (recordedMidi.events.size() / 2) + " captured hits", juce::dontSendNotification);
    else
        captureStatusLabel.setText ("No capture", juce::dontSendNotification);
}

void GroovizerAudioProcessorEditor::beginTimelineDrag()
{
    auto directory = juce::File::getSpecialLocation (juce::File::tempDirectory)
        .getChildFile ("Santismo")
        .getChildFile ("Groovizer MIDI");
    const auto destination = directory.getNonexistentChildFile ("Groovizer Timeline", ".mid", false);
    if (! processorRef.writeTimelineMidiFile (destination))
        return;

    lastDragFile = destination;
    juce::StringArray files;
    files.add (destination.getFullPathName());
    juce::DragAndDropContainer::performExternalDragDropOfFiles (files, false, this);
}

void GroovizerAudioProcessorEditor::beginCaptureDrag()
{
    if (processorRef.isMidiRecording())
        return;

    auto directory = juce::File::getSpecialLocation (juce::File::tempDirectory)
        .getChildFile ("Santismo")
        .getChildFile ("Groovizer MIDI");
    const auto destination = directory.getNonexistentChildFile ("Groovizer Capture", ".mid", false);
    if (! processorRef.writeRecordedMidiFile (destination))
        return;

    lastDragFile = destination;
    juce::StringArray files;
    files.add (destination.getFullPathName());
    juce::DragAndDropContainer::performExternalDragDropOfFiles (files, false, this);
}
