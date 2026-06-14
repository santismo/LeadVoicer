#include "PluginEditor.h"

namespace
{
juce::Colour bgTop()      { return juce::Colour (0xff111318); }
juce::Colour bgBottom()   { return juce::Colour (0xff07080b); }
juce::Colour panel()      { return juce::Colour (0xff1a1e25); }
juce::Colour panelLine()  { return juce::Colour (0xff313844); }
juce::Colour textMain()   { return juce::Colour (0xfff2f5f7); }
juce::Colour textMuted()  { return juce::Colour (0xff9da7b3); }
juce::Colour accent()     { return juce::Colour (0xff34d399); }
juce::Colour accentTwo()  { return juce::Colour (0xff60a5fa); }

void applyLabelStyle (juce::Label& label, float size, juce::Colour colour, bool bold = false)
{
    label.setFont (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain));
    label.setColour (juce::Label::textColourId, colour);
    label.setJustificationType (juce::Justification::centredLeft);
}

}

SoliVoicerLookAndFeel::SoliVoicerLookAndFeel()
{
    setColour (juce::Slider::thumbColourId, accent());
    setColour (juce::Slider::rotarySliderFillColourId, accent());
    setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xff2b313a));
    setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff202631));
    setColour (juce::ComboBox::outlineColourId, panelLine());
    setColour (juce::ComboBox::textColourId, textMain());
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff171b22));
    setColour (juce::PopupMenu::textColourId, textMain());
    setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xff28445a));
    setColour (juce::TextButton::buttonColourId, juce::Colour (0xff242a33));
    setColour (juce::TextButton::buttonOnColourId, accent().darker (0.35f));
    setColour (juce::TextButton::textColourOffId, textMain());
}

void SoliVoicerLookAndFeel::drawRotarySlider (juce::Graphics& g,
                                              int x,
                                              int y,
                                              int width,
                                              int height,
                                              float sliderPos,
                                              float rotaryStartAngle,
                                              float rotaryEndAngle,
                                              juce::Slider&)
{
    const auto bounds = juce::Rectangle<float> (static_cast<float> (x), static_cast<float> (y), static_cast<float> (width), static_cast<float> (height)).reduced (6.0f);
    const auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    g.setColour (juce::Colour (0xff242b35));
    g.fillEllipse (bounds.withSizeKeepingCentre (radius * 2.0f, radius * 2.0f));

    juce::Path backgroundArc;
    backgroundArc.addCentredArc (centre.x, centre.y, radius - 3.0f, radius - 3.0f, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (juce::Colour (0xff39414c));
    g.strokePath (backgroundArc, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path valueArc;
    valueArc.addCentredArc (centre.x, centre.y, radius - 3.0f, radius - 3.0f, 0.0f, rotaryStartAngle, angle, true);
    g.setColour (accent());
    g.strokePath (valueArc, juce::PathStrokeType (4.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const auto pointer = juce::Point<float> (centre.x + std::cos (angle - juce::MathConstants<float>::halfPi) * (radius - 10.0f),
                                            centre.y + std::sin (angle - juce::MathConstants<float>::halfPi) * (radius - 10.0f));
    g.setColour (textMain());
    g.fillEllipse (juce::Rectangle<float> (pointer.x - 2.5f, pointer.y - 2.5f, 5.0f, 5.0f));
}

void SoliVoicerLookAndFeel::drawComboBox (juce::Graphics& g,
                                          int width,
                                          int height,
                                          bool,
                                          int,
                                          int,
                                          int,
                                          int,
                                          juce::ComboBox&)
{
    auto bounds = juce::Rectangle<float> (0.5f, 0.5f, static_cast<float> (width) - 1.0f, static_cast<float> (height) - 1.0f);
    g.setColour (juce::Colour (0xff202631));
    g.fillRoundedRectangle (bounds, 6.0f);
    g.setColour (panelLine());
    g.drawRoundedRectangle (bounds, 6.0f, 1.0f);

    juce::Path arrow;
    arrow.startNewSubPath (bounds.getRight() - 18.0f, bounds.getCentreY() - 3.0f);
    arrow.lineTo (bounds.getRight() - 11.0f, bounds.getCentreY() + 4.0f);
    arrow.lineTo (bounds.getRight() - 4.0f, bounds.getCentreY() - 3.0f);
    g.setColour (accent());
    g.strokePath (arrow, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

SoliVoicerAudioProcessorEditor::SoliVoicerAudioProcessorEditor (SoliVoicerAudioProcessor& processor)
    : AudioProcessorEditor (&processor),
      processorRef (processor)
{
    setLookAndFeel (&lookAndFeel);
    setSize (980, 740);

    addAndMakeVisible (titleLabel);
    titleLabel.setText ("Lead Voicer", juce::dontSendNotification);
    applyLabelStyle (titleLabel, 28.0f, textMain(), true);

    subtitleLabel.setText ({}, juce::dontSendNotification);

    addAndMakeVisible (chordLabel);
    chordLabel.setText ("Chord: --", juce::dontSendNotification);
    chordLabel.setFont (juce::FontOptions (22.0f, juce::Font::bold));
    chordLabel.setColour (juce::Label::textColourId, accent());
    chordLabel.setJustificationType (juce::Justification::centredRight);

    addAndMakeVisible (randomButton);
    randomButton.onClick = [this] { randomizeSettings(); };

    addAndMakeVisible (resetButton);
    resetButton.onClick = [this] { resetDefaults(); };

    addAndMakeVisible (helpButton);
    helpButton.onClick = [this] { updateHelpState(); };
    for (auto& button : infoButtons)
    {
        configureInfoButton (button, {});
        button.setVisible (false);
    }

    addAndMakeVisible (keyLabel);
    keyLabel.setText ("Keys", juce::dontSendNotification);
    applyLabelStyle (keyLabel, 13.0f, textMuted(), true);

    addAndMakeVisible (scaleLabel);
    scaleLabel.setText ("Scales", juce::dontSendNotification);
    applyLabelStyle (scaleLabel, 13.0f, textMuted(), true);

    const auto keys = Soli::ChordEngine::keyNames();
    const auto scales = Soli::ChordEngine::scaleNames();
    for (int i = 0; i < 12; ++i)
    {
        configureMaskToggle (keyToggles[static_cast<size_t> (i)], keys[i]);
        keyToggles[static_cast<size_t> (i)].onClick = [this]
        {
            commitMaskFromToggles (ParameterIDs::keyMask, keyToggles, Soli::ChordEngine::keyNames().size());
        };

        configureMaskToggle (scaleToggles[static_cast<size_t> (i)], scales[i]);
        scaleToggles[static_cast<size_t> (i)].onClick = [this]
        {
            commitMaskFromToggles (ParameterIDs::scaleMask, scaleToggles, Soli::ChordEngine::scaleNames().size());
        };
    }

    for (auto* button : { &keyAllButton, &keyDefaultButton, &scaleAllButton, &scaleDefaultButton })
        addAndMakeVisible (*button);

    keyAllButton.onClick = [this] { setParameterValue (ParameterIDs::keyMask, static_cast<float> ((1 << Soli::ChordEngine::keyNames().size()) - 1)); updateMaskToggles(); };
    keyDefaultButton.onClick = [this] { setParameterValue (ParameterIDs::keyMask, 1.0f); updateMaskToggles(); };
    scaleAllButton.onClick = [this] { setParameterValue (ParameterIDs::scaleMask, static_cast<float> ((1 << Soli::ChordEngine::scaleNames().size()) - 1)); updateMaskToggles(); };
    scaleDefaultButton.onClick = [this] { setParameterValue (ParameterIDs::scaleMask, 1.0f); updateMaskToggles(); };

    addLabeledCombo (roleBox, roleLabel, "Input Note Role", Soli::ChordEngine::roleNames());
    addLabeledCombo (styleBox, styleLabel, "Style", Soli::ChordEngine::styleNames());
    addLabeledCombo (playabilityBox, playabilityLabel, "Playable As", Soli::ChordEngine::playabilityNames());
    addLabeledCombo (strumModeBox, strumModeLabel, "Rake", Soli::ChordEngine::strumModeNames());

    addLabeledSlider (chordSizeSlider, chordSizeLabel, "Chord Size");
    addLabeledSlider (complexitySlider, complexityLabel, "Complexity");
    addLabeledSlider (voiceLeadingSlider, voiceLeadingLabel, "Voice Leading");
    addLabeledSlider (outsideSlider, outsideLabel, "Outside");
    addLabeledSlider (variationSlider, variationLabel, "Variation");
    addLabeledSlider (repeatSlider, repeatLabel, "Repeat");
    addLabeledSlider (strumSpeedSlider, strumSpeedLabel, "Rake Speed");
    addLabeledSlider (minNoteSlider, minNoteLabel, "Min Note");
    addLabeledSlider (maxNoteSlider, maxNoteLabel, "Max Note");

    auto& state = processorRef.getValueTreeState();
    roleAttachment = std::make_unique<ComboBoxAttachment> (state, ParameterIDs::role, roleBox);
    styleAttachment = std::make_unique<ComboBoxAttachment> (state, ParameterIDs::style, styleBox);
    playabilityAttachment = std::make_unique<ComboBoxAttachment> (state, ParameterIDs::playability, playabilityBox);
    strumModeAttachment = std::make_unique<ComboBoxAttachment> (state, ParameterIDs::strumMode, strumModeBox);
    chordSizeAttachment = std::make_unique<SliderAttachment> (state, ParameterIDs::chordSize, chordSizeSlider);
    complexityAttachment = std::make_unique<SliderAttachment> (state, ParameterIDs::complexity, complexitySlider);
    voiceLeadingAttachment = std::make_unique<SliderAttachment> (state, ParameterIDs::voiceLeading, voiceLeadingSlider);
    outsideAttachment = std::make_unique<SliderAttachment> (state, ParameterIDs::outside, outsideSlider);
    variationAttachment = std::make_unique<SliderAttachment> (state, ParameterIDs::variation, variationSlider);
    repeatAttachment = std::make_unique<SliderAttachment> (state, ParameterIDs::repeatChance, repeatSlider);
    strumSpeedAttachment = std::make_unique<SliderAttachment> (state, ParameterIDs::strumSpeed, strumSpeedSlider);
    minNoteAttachment = std::make_unique<SliderAttachment> (state, ParameterIDs::minNote, minNoteSlider);
    maxNoteAttachment = std::make_unique<SliderAttachment> (state, ParameterIDs::maxNote, maxNoteSlider);

    installTooltips();
    updateMaskToggles();
    startTimerHz (12);
}

SoliVoicerAudioProcessorEditor::~SoliVoicerAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void SoliVoicerAudioProcessorEditor::configureSlider (juce::Slider& slider)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 72, 22);
    slider.setColour (juce::Slider::textBoxTextColourId, textMain());
    slider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0xff141820));
    slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colour (0x00000000));
}

void SoliVoicerAudioProcessorEditor::configureCombo (juce::ComboBox& combo, const juce::StringArray& names)
{
    combo.clear();
    for (int i = 0; i < names.size(); ++i)
        combo.addItem (names[i], i + 1);
    combo.setColour (juce::ComboBox::textColourId, textMain());
    combo.setJustificationType (juce::Justification::centredLeft);
}

void SoliVoicerAudioProcessorEditor::addLabeledSlider (juce::Slider& slider, juce::Label& label, const juce::String& text)
{
    addAndMakeVisible (label);
    label.setText (text, juce::dontSendNotification);
    applyLabelStyle (label, 13.0f, textMuted(), true);
    addAndMakeVisible (slider);
    configureSlider (slider);
}

void SoliVoicerAudioProcessorEditor::addLabeledCombo (juce::ComboBox& combo, juce::Label& label, const juce::String& text, const juce::StringArray& names)
{
    addAndMakeVisible (label);
    label.setText (text, juce::dontSendNotification);
    applyLabelStyle (label, 13.0f, textMuted(), true);
    addAndMakeVisible (combo);
    configureCombo (combo, names);
}

void SoliVoicerAudioProcessorEditor::configureMaskToggle (juce::ToggleButton& button, const juce::String& text)
{
    addAndMakeVisible (button);
    button.setButtonText (text);
    button.setColour (juce::ToggleButton::textColourId, textMain());
    button.setColour (juce::ToggleButton::tickColourId, accent());
    button.setColour (juce::ToggleButton::tickDisabledColourId, panelLine());
}

void SoliVoicerAudioProcessorEditor::configureInfoButton (juce::TextButton& button, const juce::String& tooltip)
{
    addAndMakeVisible (button);
    button.setButtonText ("i");
    button.setTooltip (tooltip);
    button.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff1f2933));
    button.setColour (juce::TextButton::textColourOffId, accent());
    button.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff28445a));
}

void SoliVoicerAudioProcessorEditor::timerCallback()
{
    chordLabel.setText (processorRef.getLastChordName(), juce::dontSendNotification);
}

void SoliVoicerAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.setGradientFill (juce::ColourGradient (bgTop(), 0.0f, 0.0f, bgBottom(), 0.0f, static_cast<float> (getHeight()), false));
    g.fillAll();

    auto bounds = getLocalBounds().toFloat().reduced (18.0f);
    g.setColour (panel().withAlpha (0.86f));
    g.fillRoundedRectangle (bounds, 10.0f);
    g.setColour (panelLine());
    g.drawRoundedRectangle (bounds, 10.0f, 1.0f);

    auto header = bounds.removeFromTop (82.0f).reduced (18.0f, 12.0f);
    juce::ColourGradient glow (accent().withAlpha (0.28f), header.getX(), header.getY(),
                               accentTwo().withAlpha (0.12f), header.getRight(), header.getBottom(), false);
    g.setGradientFill (glow);
    g.fillRoundedRectangle (header.expanded (3.0f, 4.0f), 8.0f);
}

void SoliVoicerAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced (36);
    auto header = bounds.removeFromTop (74);
    auto leftHeader = header.removeFromLeft (330);
    titleLabel.setBounds (leftHeader.removeFromTop (36));
    subtitleLabel.setBounds (leftHeader);
    helpButton.setBounds (header.removeFromRight (72).reduced (0, 20));
    resetButton.setBounds (header.removeFromRight (88).reduced (6, 20));
    randomButton.setBounds (header.removeFromRight (88).reduced (6, 20));
    chordLabel.setBounds (header.reduced (12, 12));

    bounds.removeFromTop (14);
    auto keyArea = bounds.removeFromTop (78).reduced (5, 0);
    keyLabel.setBounds (keyArea.removeFromTop (20));
    infoButtons[0].setBounds (keyLabel.getRight() - 22, keyLabel.getY(), 18, 18);
    auto keyButtons = keyArea.removeFromRight (168);
    keyAllButton.setBounds (keyButtons.removeFromTop (28));
    keyButtons.removeFromTop (6);
    keyDefaultButton.setBounds (keyButtons.removeFromTop (28));
    const auto keyWidth = keyArea.getWidth() / 12;
    for (int i = 0; i < 12; ++i)
        keyToggles[static_cast<size_t> (i)].setBounds (keyArea.getX() + i * keyWidth, keyArea.getY(), keyWidth - 3, 28);

    bounds.removeFromTop (6);
    auto scaleArea = bounds.removeFromTop (104).reduced (5, 0);
    scaleLabel.setBounds (scaleArea.removeFromTop (20));
    infoButtons[1].setBounds (scaleLabel.getRight() - 22, scaleLabel.getY(), 18, 18);
    auto scaleButtons = scaleArea.removeFromRight (168);
    scaleAllButton.setBounds (scaleButtons.removeFromTop (28));
    scaleButtons.removeFromTop (6);
    scaleDefaultButton.setBounds (scaleButtons.removeFromTop (28));
    const auto scaleGrid = scaleArea;
    const auto scaleWidth = scaleGrid.getWidth() / 6;
    for (int i = 0; i < 12; ++i)
    {
        const auto col = i % 6;
        const auto row = i / 6;
        scaleToggles[static_cast<size_t> (i)].setBounds (scaleGrid.getX() + col * scaleWidth,
                                                         scaleGrid.getY() + row * 32,
                                                         scaleWidth - 4,
                                                         28);
    }

    bounds.removeFromTop (12);
    auto comboRow = bounds.removeFromTop (76);
    const auto comboWidth = comboRow.getWidth() / 4;
    auto placeCombo = [&] (juce::ComboBox& combo, juce::Label& label)
    {
        auto cell = comboRow.removeFromLeft (comboWidth).reduced (5, 0);
        label.setBounds (cell.removeFromTop (22));
        combo.setBounds (cell.removeFromTop (34));
    };

    placeCombo (roleBox, roleLabel);
    infoButtons[2].setBounds (roleLabel.getRight() - 22, roleLabel.getY(), 18, 18);
    placeCombo (styleBox, styleLabel);
    infoButtons[3].setBounds (styleLabel.getRight() - 22, styleLabel.getY(), 18, 18);
    placeCombo (playabilityBox, playabilityLabel);
    infoButtons[4].setBounds (playabilityLabel.getRight() - 22, playabilityLabel.getY(), 18, 18);
    placeCombo (strumModeBox, strumModeLabel);
    infoButtons[5].setBounds (strumModeLabel.getRight() - 22, strumModeLabel.getY(), 18, 18);

    bounds.removeFromTop (12);
    auto sliderArea = bounds;
    const auto columns = 3;
    const auto rows = 3;
    const auto cellWidth = sliderArea.getWidth() / columns;
    const auto cellHeight = sliderArea.getHeight() / rows;

    auto placeSlider = [&] (juce::Slider& slider, juce::Label& label, int index)
    {
        const auto col = index % columns;
        const auto row = index / columns;
        auto cell = juce::Rectangle<int> (sliderArea.getX() + col * cellWidth,
                                          sliderArea.getY() + row * cellHeight,
                                          cellWidth,
                                          cellHeight).reduced (10, 4);
        label.setBounds (cell.removeFromTop (22));
        infoButtons[static_cast<size_t> (6 + index)].setBounds (label.getRight() - 22, label.getY(), 18, 18);
        slider.setBounds (cell);
    };

    placeSlider (chordSizeSlider, chordSizeLabel, 0);
    placeSlider (complexitySlider, complexityLabel, 1);
    placeSlider (voiceLeadingSlider, voiceLeadingLabel, 2);
    placeSlider (outsideSlider, outsideLabel, 3);
    placeSlider (variationSlider, variationLabel, 4);
    placeSlider (repeatSlider, repeatLabel, 5);
    placeSlider (strumSpeedSlider, strumSpeedLabel, 6);
    placeSlider (minNoteSlider, minNoteLabel, 7);
    placeSlider (maxNoteSlider, maxNoteLabel, 8);
}

void SoliVoicerAudioProcessorEditor::updateMaskToggles()
{
    auto sync = [this] (const juce::String& parameterID, std::array<juce::ToggleButton, 12>& toggles, int count)
    {
        auto* parameter = processorRef.getValueTreeState().getParameter (parameterID);
        if (parameter == nullptr)
            return;

        const auto maxMask = (1 << count) - 1;
        const auto mask = juce::jlimit (1, maxMask, static_cast<int> (parameter->convertFrom0to1 (parameter->getValue()) + 0.5f));
        for (int i = 0; i < count; ++i)
            toggles[static_cast<size_t> (i)].setToggleState ((mask & (1 << i)) != 0, juce::dontSendNotification);
    };

    const juce::ScopedValueSetter<bool> setter (syncingMaskToggles, true);
    sync (ParameterIDs::keyMask, keyToggles, Soli::ChordEngine::keyNames().size());
    sync (ParameterIDs::scaleMask, scaleToggles, Soli::ChordEngine::scaleNames().size());
}

void SoliVoicerAudioProcessorEditor::commitMaskFromToggles (const juce::String& parameterID, const std::array<juce::ToggleButton, 12>& toggles, int count)
{
    if (syncingMaskToggles)
        return;

    auto mask = 0;
    for (int i = 0; i < count; ++i)
        if (toggles[static_cast<size_t> (i)].getToggleState())
            mask |= (1 << i);

    if (mask == 0)
        mask = 1;

    setParameterValue (parameterID, static_cast<float> (mask));
    updateMaskToggles();
}

void SoliVoicerAudioProcessorEditor::setParameterValue (const juce::String& parameterID, float value)
{
    auto* parameter = processorRef.getValueTreeState().getParameter (parameterID);
    if (parameter == nullptr)
        return;

    parameter->beginChangeGesture();
    parameter->setValueNotifyingHost (parameter->convertTo0to1 (value));
    parameter->endChangeGesture();
}

void SoliVoicerAudioProcessorEditor::randomizeSettings()
{
    auto& random = juce::Random::getSystemRandom();
    const auto keyMask = 1 + random.nextInt ((1 << Soli::ChordEngine::keyNames().size()) - 1);
    const auto scaleMask = 1 + random.nextInt ((1 << Soli::ChordEngine::scaleNames().size()) - 1);

    setParameterValue (ParameterIDs::keyMask, static_cast<float> (keyMask));
    setParameterValue (ParameterIDs::scaleMask, static_cast<float> (scaleMask));
    setParameterValue (ParameterIDs::role, static_cast<float> (random.nextInt (Soli::ChordEngine::roleNames().size())));
    setParameterValue (ParameterIDs::style, static_cast<float> (random.nextInt (Soli::ChordEngine::styleNames().size())));
    setParameterValue (ParameterIDs::playability, static_cast<float> (random.nextInt (Soli::ChordEngine::playabilityNames().size())));
    setParameterValue (ParameterIDs::strumMode, static_cast<float> (random.nextInt (Soli::ChordEngine::strumModeNames().size())));
    setParameterValue (ParameterIDs::chordSize, static_cast<float> (2 + random.nextInt (23)));
    setParameterValue (ParameterIDs::complexity, random.nextFloat());
    setParameterValue (ParameterIDs::voiceLeading, random.nextFloat());
    setParameterValue (ParameterIDs::outside, random.nextFloat());
    setParameterValue (ParameterIDs::variation, random.nextFloat());
    setParameterValue (ParameterIDs::repeatChance, random.nextFloat() * 0.6f);
    setParameterValue (ParameterIDs::strumSpeed, random.nextFloat());
    setParameterValue (ParameterIDs::minNote, static_cast<float> (24 + random.nextInt (25)));
    setParameterValue (ParameterIDs::maxNote, static_cast<float> (84 + random.nextInt (32)));
    processorRef.panic();
    updateMaskToggles();
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
    processorRef.panic();
    updateMaskToggles();
}

void SoliVoicerAudioProcessorEditor::updateHelpState()
{
    if (helpButton.getToggleState())
        tooltipWindow = std::make_unique<juce::TooltipWindow> (this, 900);
    else
        tooltipWindow.reset();

    for (auto& button : infoButtons)
        button.setVisible (helpButton.getToggleState());
}

void SoliVoicerAudioProcessorEditor::installTooltips()
{
    setHelpTooltip (keyLabel, infoButtons[0], "Select one or more tonal centers. With multiple keys enabled, Lead Voicer can borrow harmony from related or contrasting key areas.");
    setHelpTooltip (scaleLabel, infoButtons[1], "Select one or more scale sources. Multiple scales allow modal interchange and color borrowing while the voice-leading score decides what fits.");
    for (auto& button : keyToggles)
        button.setTooltip ("Enable or disable this tonal center for key borrowing and modulation.");
    for (auto& button : scaleToggles)
        button.setTooltip ("Enable or disable this scale source for modal interchange.");
    setHelpTooltip (roleBox, infoButtons[2], "Chooses where the incoming MIDI note sits in the generated chord. Melody Top is the default lead-line harmonizer behavior.");
    roleLabel.setTooltip (roleBox.getTooltip());
    setHelpTooltip (styleBox, infoButtons[3], "Changes the harmonic scoring bias: close lead, large ensemble block movement, quartal color, classical tension/release, gospel, outside, modal, mediant, counterpoint, neo-soul, or progressive rock.");
    styleLabel.setTooltip (styleBox.getTooltip());
    setHelpTooltip (playabilityBox, infoButtons[4], "Constrains spacing and chord size toward piano, guitar, horn section, orchestra, or unrestricted voicings.");
    playabilityLabel.setTooltip (playabilityBox.getTooltip());
    setHelpTooltip (strumModeBox, infoButtons[5], "Rakes generated notes together, upward, downward, or in a shuffled random note order.");
    strumModeLabel.setTooltip (strumModeBox.getTooltip());
    setHelpTooltip (chordSizeSlider, infoButtons[6], "Approximate number of notes in the generated chord.");
    chordSizeLabel.setTooltip (chordSizeSlider.getTooltip());
    setHelpTooltip (complexitySlider, infoButtons[7], "Higher values allow richer extensions, altered tones, clusters, and more complex sonorities.");
    complexityLabel.setTooltip (complexitySlider.getTooltip());
    setHelpTooltip (voiceLeadingSlider, infoButtons[8], "Higher values prioritize smooth motion from the previous generated chord.");
    voiceLeadingLabel.setTooltip (voiceLeadingSlider.getTooltip());
    setHelpTooltip (outsideSlider, infoButtons[9], "Controls how far the engine can borrow from outside the selected key and scale pools.");
    outsideLabel.setTooltip (outsideSlider.getTooltip());
    setHelpTooltip (variationSlider, infoButtons[10], "Controls how often the engine chooses alternatives instead of the highest-scoring chord.");
    variationLabel.setTooltip (variationSlider.getTooltip());
    setHelpTooltip (repeatSlider, infoButtons[11], "Chance that the previous voicing is repeated instead of reharmonizing the incoming note.");
    repeatLabel.setTooltip (repeatSlider.getTooltip());
    setHelpTooltip (strumSpeedSlider, infoButtons[12], "Controls how spread out the rake is. At zero, chord notes fire together.");
    strumSpeedLabel.setTooltip (strumSpeedSlider.getTooltip());
    setHelpTooltip (minNoteSlider, infoButtons[13], "Lowest generated MIDI note allowed.");
    minNoteLabel.setTooltip (minNoteSlider.getTooltip());
    setHelpTooltip (maxNoteSlider, infoButtons[14], "Highest generated MIDI note allowed.");
    maxNoteLabel.setTooltip (maxNoteSlider.getTooltip());
    randomButton.setTooltip ("Randomizes all musical settings and selections.");
    resetButton.setTooltip ("Restores the default melody-first C Ionian voice-leading setup.");
    helpButton.setTooltip ("Turns detailed hover help on or off.");
}

void SoliVoicerAudioProcessorEditor::setHelpTooltip (juce::SettableTooltipClient& component, juce::TextButton& infoButton, const juce::String& text)
{
    component.setTooltip (text);
    infoButton.setTooltip (text);
    infoButton.onClick = [text]
    {
        juce::AlertWindow::showMessageBoxAsync (juce::AlertWindow::InfoIcon, "Lead Voicer Help", text);
    };
}
