#include "PluginEditor.h"

#include <algorithm>
#include <cmath>

namespace
{
struct SkinStyle
{
    juce::Colour background, panel, panelRaised, line, text, muted, accent, accent2;

    SkinStyle (juce::uint32 backgroundValue, juce::uint32 panelValue, juce::uint32 panelRaisedValue,
               juce::uint32 lineValue, juce::uint32 textValue, juce::uint32 mutedValue,
               juce::uint32 accentValue, juce::uint32 accent2Value)
        : background (backgroundValue), panel (panelValue), panelRaised (panelRaisedValue), line (lineValue),
          text (textValue), muted (mutedValue), accent (accentValue), accent2 (accent2Value) {}
};

const std::array<SkinStyle, 10> skins
{{
    { 0xff151b20, 0xff283139, 0xff343f47, 0xff6d7981, 0xfff3f7f5, 0xff9ba8aa, 0xff61dc9f, 0xffffc05b }, // Graphite Plex
    { 0xff222719, 0xff333d25, 0xff3f4b2e, 0xff778550, 0xfff7f9e9, 0xffa5b38a, 0xffd4fb4e, 0xfffbff92 }, // Citrus Volt
    { 0xff304c58, 0xff46646e, 0xff5a7b86, 0xff8eb4c0, 0xffeff9fb, 0xffa5c2ca, 0xff64d9ff, 0xffb9f2ff }, // Aqua Chrome
    { 0xff4b3524, 0xff684a31, 0xff7c5a3e, 0xffbb8b5b, 0xfffff2dd, 0xffc9ad91, 0xfff19b48, 0xffffda83 }, // Copper Tape
    { 0xff426370, 0xff5a7d89, 0xff7198a5, 0xff9bc0cc, 0xffeffbff, 0xffb2d0d8, 0xff7edcff, 0xffd2f7ff }, // Arctic Pixel
    { 0xff312548, 0xff48375f, 0xff5b4677, 0xff8065a4, 0xfff8efff, 0xffb8a4c8, 0xffcc83ff, 0xfff0aeff }, // Violet CRT
    { 0xff431f23, 0xff5c2a30, 0xff733840, 0xffb76560, 0xfffff3ef, 0xffc9a0a0, 0xffff5b50, 0xffffc044 }, // Flame Amp
    { 0xff213a2c, 0xff31533e, 0xff3d694d, 0xff6ba17a, 0xffeffff3, 0xff9fbda7, 0xff63d892, 0xffc9f58d }, // Forest EQ
    { 0xff503542, 0xff694756, 0xff805b69, 0xffb97b96, 0xfffff4f8, 0xffc9aab7, 0xffff759f, 0xffffd0df }, // Rose Neon
    { 0xff1f1f1f, 0xff292929, 0xff414141, 0xff555555, 0xffe6e6e6, 0xff999999, 0xff3478d4, 0xffee8318 }  // Logic Dark
}};

int activeSkinIndex = 9;
const SkinStyle& activeSkin() { return skins[static_cast<std::size_t> (juce::jlimit (0, 9, activeSkinIndex))]; }
juce::Colour background() { return activeSkin().background; }
juce::Colour panel() { return activeSkin().panel; }
juce::Colour panelRaised() { return activeSkin().panelRaised; }
juce::Colour line() { return activeSkin().line; }
juce::Colour text() { return activeSkin().text; }
juce::Colour muted() { return activeSkin().muted; }
juce::Colour green() { return activeSkin().accent; }
juce::Colour amber() { return activeSkin().accent2; }
juce::Colour inputRed() { return juce::Colour (0xffff3b4f); }
juce::Colour voicingBlue() { return juce::Colour (0xff3f8cff); }

bool squareInterface()
{
    return activeSkinIndex == 4;
}

float interfaceCorner()
{
    return squareInterface() ? 0.0f : (activeSkinIndex == 8 ? 11.0f : 4.0f);
}

[[maybe_unused]] void drawInterfaceTexture (juce::Graphics& g, juce::Rectangle<float> bounds, float alpha = 1.0f)
{
    switch (activeSkinIndex)
    {
        case 0: // Graphite Plex
            for (float y = bounds.getY(); y < bounds.getBottom(); y += 3.0f)
            {
                g.setColour (juce::Colours::white.withAlpha (((static_cast<int> (y) % 9 == 0) ? 0.045f : 0.018f) * alpha));
                g.drawHorizontalLine (static_cast<int> (y), bounds.getX(), bounds.getRight());
            }
            break;
        case 1: // Citrus Volt
            g.setColour (green().withAlpha (0.11f * alpha));
            for (float x = bounds.getX(); x < bounds.getRight(); x += 12.0f) g.drawVerticalLine (static_cast<int> (x), bounds.getY(), bounds.getBottom());
            for (float y = bounds.getY(); y < bounds.getBottom(); y += 8.0f) g.drawHorizontalLine (static_cast<int> (y), bounds.getX(), bounds.getRight());
            break;
        case 7: // Forest EQ
            g.setColour (green().withAlpha (0.045f * alpha));
            for (float x = bounds.getX(); x < bounds.getRight(); x += 10.0f)
                g.drawVerticalLine (static_cast<int> (x), bounds.getY(), bounds.getBottom());
            g.setColour (green().withAlpha (0.23f * alpha));
            {
                juce::Path trace;
                trace.startNewSubPath (bounds.getX(), bounds.getCentreY());
                for (float x = bounds.getX(); x < bounds.getRight(); x += 5.0f)
                    trace.lineTo (x, bounds.getCentreY() + std::sin ((x - bounds.getX()) * 0.11f) * bounds.getHeight() * 0.18f);
                g.strokePath (trace, juce::PathStrokeType (1.2f));
            }
            break;
        case 2: // Aqua Chrome
            for (float y = bounds.getY(); y < bounds.getBottom(); y += 16.0f)
            {
                g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.12f * alpha), bounds.getX(), y,
                                                         juce::Colours::black.withAlpha (0.11f * alpha), bounds.getX(), y + 16.0f, false));
                g.fillRect (bounds.getX(), y, bounds.getWidth(), 16.0f);
            }
            break;
        case 3: // Copper Tape
            for (float x = bounds.getX(); x < bounds.getRight(); x += 9.0f)
            {
                g.setColour (amber().withAlpha (0.055f * alpha));
                g.drawVerticalLine (static_cast<int> (x), bounds.getY(), bounds.getBottom());
            }
            break;
        case 4: // Arctic Pixel
        case 9: // Mono Dot
            for (float y = bounds.getY() + 4.0f; y < bounds.getBottom(); y += 7.0f)
                for (float x = bounds.getX() + 4.0f; x < bounds.getRight(); x += 7.0f)
                {
                    g.setColour (text().withAlpha ((activeSkinIndex == 9 ? 0.06f : 0.035f) * alpha));
                    if (activeSkinIndex == 9) g.fillEllipse (x, y, 1.4f, 1.4f); else g.fillRect (x, y, 2.0f, 2.0f);
                }
            break;
        case 5: // Violet CRT
            for (float y = bounds.getY(); y < bounds.getBottom(); y += 3.0f)
            {
                g.setColour (juce::Colours::black.withAlpha (0.18f * alpha));
                g.drawHorizontalLine (static_cast<int> (y), bounds.getX(), bounds.getRight());
            }
            break;
        case 6: // Flame Amp
            g.setColour (amber().withAlpha (0.09f * alpha));
            for (float x = bounds.getX() - bounds.getHeight(); x < bounds.getRight(); x += 16.0f)
                g.drawLine (x, bounds.getBottom(), x + bounds.getHeight(), bounds.getY(), 2.0f);
            break;
        case 8: // Rose Neon
            g.setColour (green().withAlpha (0.085f * alpha));
            for (float x = bounds.getX() - 20.0f; x < bounds.getRight() + 20.0f; x += 20.0f)
            {
                juce::Path diamond;
                diamond.startNewSubPath (x, bounds.getCentreY());
                diamond.lineTo (x + 10.0f, bounds.getCentreY() - 10.0f);
                diamond.lineTo (x + 20.0f, bounds.getCentreY());
                diamond.lineTo (x + 10.0f, bounds.getCentreY() + 10.0f);
                diamond.closeSubPath();
                g.strokePath (diamond, juce::PathStrokeType (1.0f));
            }
            break;
        default: break;
    }
}

void drawRackFrame (juce::Graphics& g, juce::Rectangle<float> bounds, const juce::String& caption)
{
    g.setColour (panel());
    g.fillRoundedRectangle (bounds, 5.0f);
    g.setColour (line().withAlpha (0.72f));
    g.drawRoundedRectangle (bounds.reduced (0.5f), 5.0f, 1.0f);

    if (caption.isNotEmpty())
    {
        auto label = bounds.reduced (7.0f, 5.0f).removeFromTop (15.0f);
        g.setColour (muted());
        g.setFont (juce::FontOptions (9.0f));
        g.drawFittedText (caption, label.toNearestInt().reduced (5, 0), juce::Justification::centredLeft, 1);
    }
}

bool isBlackPianoKey (int note)
{
    switch (note % 12)
    {
        case 1: case 3: case 6: case 8: case 10: return true;
        default: return false;
    }
}

void drawVoicingKeyboard (juce::Graphics& g, juce::Rectangle<float> bounds,
                          const std::array<juce::uint64, 2>& voicedNotes,
                          const std::array<juce::uint64, 2>& inputNotes)
{
    g.setColour (background());
    g.fillRect (bounds);
    const auto keyWidth = bounds.getWidth() / 128.0f;
    for (int note = 0; note < 128; ++note)
    {
        const auto key = juce::Rectangle<float> (bounds.getX() + note * keyWidth, bounds.getY(), keyWidth + 0.35f, bounds.getHeight());
        const auto voiced = (voicedNotes[static_cast<std::size_t> (note / 64)]
                             & (static_cast<juce::uint64> (1) << (note % 64))) != 0;
        const auto input = (inputNotes[static_cast<std::size_t> (note / 64)]
                            & (static_cast<juce::uint64> (1) << (note % 64))) != 0;
        if (input)
            g.setColour (inputRed());
        else if (voiced)
            g.setColour (voicingBlue());
        else if (isBlackPianoKey (note))
            g.setColour (juce::Colours::black);
        else
            g.setColour (text().withAlpha (0.78f));
        g.fillRect (key);
        g.setColour (background().withAlpha (0.92f));
        g.drawVerticalLine (static_cast<int> (key.getX()), key.getY(), key.getBottom());

        // Active keys are intentionally pure colour blocks: no outline, dot,
        // or text competes with the red-input / blue-voicing distinction.
    }

    g.setColour (line().withAlpha (0.82f));
    g.drawRect (bounds, 1.0f);
}

void setLabelStyle (juce::Label& label, float size, juce::Colour colour, bool bold = false)
{
    juce::ignoreUnused (bold);
    label.setFont (juce::FontOptions (size));
    label.setColour (juce::Label::textColourId, colour);
    label.setJustificationType (juce::Justification::centredLeft);
}

class ChordBankCardButton final : public juce::TextButton
{
public:
    void paintButton (juce::Graphics& g, bool highlighted, bool down) override
    {
        auto bounds = getLocalBounds().toFloat().reduced (0.5f);
        auto fill = getToggleState() ? SongizerLogicLookAndFeel::blue()
                                     : SongizerLogicLookAndFeel::raised();
        if (highlighted) fill = fill.brighter (0.07f);
        if (down) fill = fill.darker (0.10f);
        g.setColour (fill);
        g.fillRoundedRectangle (bounds, 4.0f);
        g.setColour ((getToggleState() ? SongizerLogicLookAndFeel::blue().brighter (0.18f)
                                       : SongizerLogicLookAndFeel::line()).withAlpha (0.9f));
        g.drawRoundedRectangle (bounds, 4.0f, 1.0f);

        auto textBounds = getLocalBounds().reduced (10, 4);
        textBounds.removeFromRight (52);
        g.setColour (getToggleState() ? juce::Colours::white : SongizerLogicLookAndFeel::text());
        g.setFont (juce::FontOptions (13.0f));
        g.drawFittedText (getButtonText(), textBounds, juce::Justification::centredLeft, 1);
    }
};

class ChordBankProbabilitySlider final : public juce::Slider
{
public:
    void paint (juce::Graphics& g) override
    {
        auto area = getLocalBounds().toFloat().reduced (2.0f);
        const auto diameter = juce::jmin (area.getWidth(), area.getHeight());
        auto knob = area.withSizeKeepingCentre (diameter, diameter);
        const auto centre = knob.getCentre();
        const auto radius = knob.getWidth() * 0.42f;
        constexpr auto startAngle = juce::MathConstants<float>::pi * 1.25f;
        constexpr auto endAngle = juce::MathConstants<float>::pi * 2.75f;
        const auto proportion = static_cast<float> (juce::jlimit (0.0, 1.0, getValue() / 100.0));
        const auto angle = startAngle + proportion * (endAngle - startAngle);

        g.setColour (SongizerLogicLookAndFeel::recessed());
        g.fillEllipse (knob);
        g.setColour (SongizerLogicLookAndFeel::line());
        g.drawEllipse (knob, 1.0f);

        juce::Path track;
        track.addCentredArc (centre.x, centre.y, radius, radius, 0.0f, startAngle, endAngle, true);
        g.setColour (juce::Colours::black.withAlpha (0.72f));
        g.strokePath (track, juce::PathStrokeType (3.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        if (proportion > 0.0f)
        {
            juce::Path valueArc;
            valueArc.addCentredArc (centre.x, centre.y, radius, radius, 0.0f, startAngle, angle, true);
            g.setColour (SongizerLogicLookAndFeel::blue());
            g.strokePath (valueArc, juce::PathStrokeType (3.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        const auto pointerInner = radius * 0.38f;
        const auto pointerOuter = radius * 0.78f;
        const auto direction = juce::Point<float> (std::sin (angle), -std::cos (angle));
        g.setColour (juce::Colours::white.withAlpha (0.92f));
        g.drawLine (centre.x + direction.x * pointerInner,
                    centre.y + direction.y * pointerInner,
                    centre.x + direction.x * pointerOuter,
                    centre.y + direction.y * pointerOuter,
                    2.0f);

        g.setColour (SongizerLogicLookAndFeel::text());
        g.setFont (juce::FontOptions (10.0f));
        g.drawText (juce::String (juce::roundToInt (getValue())), knob.toNearestInt(),
                    juce::Justification::centred, false);
    }
};
}

SoliVoicerLookAndFeel::SoliVoicerLookAndFeel()
{
    setSkin (0);
}

void SoliVoicerLookAndFeel::setSkin (int skinIndex)
{
    juce::ignoreUnused (skinIndex);
    activeSkinIndex = 9;
    setColour (juce::Slider::thumbColourId, juce::Colour (0xffb8b8b8));
    setColour (juce::Slider::trackColourId, SongizerLogicLookAndFeel::blue());
    setColour (juce::Slider::rotarySliderFillColourId, SongizerLogicLookAndFeel::blue());
    setColour (juce::Slider::rotarySliderOutlineColourId, line());
    setColour (juce::ComboBox::backgroundColourId, panelRaised());
    setColour (juce::ComboBox::outlineColourId, line());
    setColour (juce::ComboBox::textColourId, text());
    setColour (juce::PopupMenu::backgroundColourId, panel());
    setColour (juce::PopupMenu::textColourId, text());
    setColour (juce::PopupMenu::highlightedBackgroundColourId, green().withAlpha (0.45f));
    setColour (juce::TextButton::buttonColourId, panelRaised());
    setColour (juce::TextButton::textColourOffId, text());
}

void SoliVoicerLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                                              float sliderPos, float minSliderPos, float maxSliderPos,
                                              const juce::Slider::SliderStyle style, juce::Slider& slider)
{
    SongizerLogicLookAndFeel::drawLinearSlider (g, x, y, width, height, sliderPos,
                                                minSliderPos, maxSliderPos, style, slider);
}

void SoliVoicerLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool down,
                                          int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox& box)
{
    SongizerLogicLookAndFeel::drawComboBox (g, width, height, down, buttonX, buttonY, buttonW, buttonH, box);
}

void SoliVoicerLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour& colour,
                                                   bool highlighted, bool down)
{
    SongizerLogicLookAndFeel::drawButtonBackground (g, button, colour, highlighted, down);
}

void SoliVoicerLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                                               bool highlighted, bool down)
{
    SongizerLogicLookAndFeel::drawToggleButton (g, button, highlighted, down);
}

SoliVoicerAudioProcessorEditor::SoliVoicerAudioProcessorEditor (SoliVoicerAudioProcessor& owner)
    : AudioProcessorEditor (&owner), processorRef (owner)
{
    setLookAndFeel (&lookAndFeel);
    setResizable (true, true);
    setWantsKeyboardFocus (true);
    setResizeLimits (820, 660, 1500, 1180);
    setSize (980, 760);
    tooltipWindow = std::make_unique<juce::TooltipWindow> (this, 700);

    addAndMakeVisible (titleLabel);
    titleLabel.setText ("Voicizer", juce::dontSendNotification);
    setLabelStyle (titleLabel, 20.0f, text(), false);

    addAndMakeVisible (chordLabel);
    chordLabel.setText ("--", juce::dontSendNotification);
    setLabelStyle (chordLabel, 19.0f, green(), false);
    chordLabel.setJustificationType (juce::Justification::centredRight);

    addAndMakeVisible (randomButton);
    addAndMakeVisible (randomVoicingButton);
    addAndMakeVisible (resetButton);
    addAndMakeVisible (newPhraseButton);
    randomButton.onClick = [this] { randomizeAllSettings(); };
    randomVoicingButton.onClick = [this] { randomizeVoicingSettings(); };
    resetButton.onClick = [this] { resetDefaults(); };
    newPhraseButton.onClick = [this] { processorRef.startNewPhrase(); };
    randomButton.setTooltip ("Randomize keys, scales, and all visible voicing controls.");
    randomVoicingButton.setTooltip ("Randomize voicing settings, including the input note role.");
    resetButton.setTooltip ("Restore Voicizer defaults.");
    newPhraseButton.setTooltip ("End the current harmonic memory and begin a fresh phrase.");

    addCombo (sourceModeBox, sourceModeLabel, "Harmony Source", SoliVoicerAudioProcessor::sourceModeNames(),
              "Scale / Harmony uses key and scale selection. Chord Bank learns root-independent chord qualities, then applies them to incoming notes by Input Note Role.");
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

    addAndMakeVisible (chordBankListenButton);
    addAndMakeVisible (chordBankPerformButton);
    addAndMakeVisible (chordBankDeleteButton);
    addAndMakeVisible (chordBankClearButton);
    addAndMakeVisible (chordBankViewport);
    chordBankViewport.setViewedComponent (&chordBankContent, false);
    chordBankViewport.setScrollBarsShown (false, true);
    chordBankViewport.setScrollOnDragMode (juce::Viewport::ScrollOnDragMode::all);
    chordBankContent.addAndMakeVisible (chordBankCardsLabel);
    chordBankListenButton.onClick = [this] { processorRef.setChordBankListening (true); };
    chordBankPerformButton.onClick = [this] { processorRef.setChordBankListening (false); };
    chordBankDeleteButton.onClick = [this] { deleteSelectedChordBankCard(); };
    chordBankClearButton.onClick = [this]
    {
        processorRef.clearChordBank();
        selectedChordBankCard = -1;
        refreshChordBankCards();
    };
    chordBankListenButton.setTooltip ("Pass incoming MIDI through audibly and learn the chord quality after every note in the played or raked chord has ended. Captured roots are not stored.");
    chordBankPerformButton.setTooltip ("Apply root-independent chord-quality cards to incoming notes by Input Note Role, using their dial-weighted relative chances.");
    chordBankDeleteButton.setTooltip ("Delete the selected chord card. The Delete and Backspace keys do the same thing.");
    chordBankClearButton.setTooltip ("Remove every chord card from this bank.");
    chordBankCardsLabel.setText ("Play and release a chord to learn its quality.", juce::dontSendNotification);
    chordBankCardsLabel.setJustificationType (juce::Justification::centredLeft);
    chordBankCardsLabel.setFont (juce::FontOptions (12.0f));
    chordBankCardsLabel.setColour (juce::Label::textColourId, muted());

    addAndMakeVisible (linkStatusLabel);
    setLabelStyle (linkStatusLabel, 12.0f, muted(), true);

    addSlider (chordSizeSlider, chordSizeLabel, "Voices", "Number of generated voices.");
    addSlider (complexitySlider, complexityLabel, "Color", "Controls extensions and richer chord colors.");
    addSlider (voiceLeadingSlider, voiceLeadingLabel, "Lead", "Prioritizes smooth movement from the previous voicing.");
    addSlider (outsideSlider, outsideLabel, "Outside", "Allows notes and chord choices outside the immediate scale.");
    addSlider (variationSlider, variationLabel, "Variation", "Widens the pool of valid generated alternatives.");
    addSlider (repeatSlider, repeatLabel, "Repeat", "Chance to retain the prior voicing.");
    addSlider (strumSpeedSlider, strumSpeedLabel, "Rake", "Controls the onset spread for raked held chords.");
    addSlider (minNoteSlider, minNoteLabel, "Low", "Lowest generated MIDI note.");
    addSlider (maxNoteSlider, maxNoteLabel, "High", "Highest generated MIDI note.");
    addSlider (substitutionSlider, substitutionLabel, "Substitution Depth", "Controls how far compatible replacements may move from the Chordizer chord.");
    addSlider (harmonicStabilitySlider, harmonicStabilityLabel, "Stability", "Higher values favor functional continuity, familiar cadences, and repeatable harmony.");
    addSlider (melodyImportanceSlider, melodyImportanceLabel, "Melody", "Controls how strongly lead styles preserve the played note as the top melodic voice.");
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
    harmonicStabilityAttachment = std::make_unique<SliderAttachment> (state, ParameterIDs::harmonicStability, harmonicStabilitySlider);
    melodyImportanceAttachment = std::make_unique<SliderAttachment> (state, ParameterIDs::melodyImportance, melodyImportanceSlider);
    performanceComplexityAttachment = std::make_unique<SliderAttachment> (state, ParameterIDs::performanceComplexity, performanceComplexitySlider);
    densityAttachment = std::make_unique<SliderAttachment> (state, ParameterIDs::rhythmDensity, densitySlider);
    syncopationAttachment = std::make_unique<SliderAttachment> (state, ParameterIDs::syncopation, syncopationSlider);
    swingAttachment = std::make_unique<SliderAttachment> (state, ParameterIDs::swing, swingSlider);
    humanizeAttachment = std::make_unique<SliderAttachment> (state, ParameterIDs::humanize, humanizeSlider);
    gateAttachment = std::make_unique<SliderAttachment> (state, ParameterIDs::gate, gateSlider);
    doubleTimeAttachment = std::make_unique<ButtonAttachment> (state, ParameterIDs::doubleTime, doubleTimeButton);

    updatePerformanceSubStyleChoices();
    applySkin (0);
    updateMaskToggles();
    updateModeVisibility();
    refreshChordBankCards();
    startTimerHz (30);
}

SoliVoicerAudioProcessorEditor::~SoliVoicerAudioProcessorEditor()
{
    chordBankViewport.setViewedComponent (nullptr, false);
    setLookAndFeel (nullptr);
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

void SoliVoicerAudioProcessorEditor::applySkin (int skinIndex)
{
    juce::ignoreUnused (skinIndex);
    lookAndFeel.setSkin (0);
    setLabelStyle (titleLabel, 20.0f, text(), false);
    setLabelStyle (chordLabel, 19.0f, green(), false);
    chordLabel.setJustificationType (juce::Justification::centredRight);
    setLabelStyle (linkStatusLabel, 12.0f, muted(), true);

    for (auto* label : { &sourceModeLabel, &outputModeLabel, &contextModeLabel, &roleLabel,
                         &styleLabel, &playabilityLabel, &strumModeLabel, &performanceStyleLabel,
                         &performanceSubStyleLabel, &keyLabel, &scaleLabel,
                         &chordSizeLabel, &complexityLabel, &voiceLeadingLabel, &outsideLabel,
                         &variationLabel, &repeatLabel, &strumSpeedLabel, &minNoteLabel,
                         &maxNoteLabel, &substitutionLabel, &harmonicStabilityLabel,
                         &melodyImportanceLabel, &performanceComplexityLabel,
                         &densityLabel, &syncopationLabel, &swingLabel, &humanizeLabel, &gateLabel })
        setLabelStyle (*label, 12.0f, muted(), true);

    for (auto* combo : { &sourceModeBox, &outputModeBox, &contextModeBox, &roleBox,
                         &styleBox, &playabilityBox, &strumModeBox, &performanceStyleBox, &performanceSubStyleBox })
    {
        combo->setColour (juce::ComboBox::backgroundColourId, panelRaised());
        combo->setColour (juce::ComboBox::outlineColourId, line());
        combo->setColour (juce::ComboBox::textColourId, text());
    }

    for (auto* slider : { &chordSizeSlider, &complexitySlider, &voiceLeadingSlider, &outsideSlider,
                          &variationSlider, &repeatSlider, &strumSpeedSlider, &minNoteSlider,
                          &maxNoteSlider, &substitutionSlider, &harmonicStabilitySlider,
                          &melodyImportanceSlider, &performanceComplexitySlider,
                          &densitySlider, &syncopationSlider, &swingSlider, &humanizeSlider, &gateSlider })
    {
        slider->setColour (juce::Slider::textBoxTextColourId, text());
        slider->setColour (juce::Slider::textBoxBackgroundColourId, background());
    }

    for (auto& button : keyToggles)
    {
        button.setColour (juce::ToggleButton::textColourId, text());
        button.setColour (juce::ToggleButton::tickColourId, green());
    }
    for (auto& button : scaleToggles)
    {
        button.setColour (juce::ToggleButton::textColourId, text());
        button.setColour (juce::ToggleButton::tickColourId, green());
    }
    doubleTimeButton.setColour (juce::ToggleButton::textColourId, text());
    doubleTimeButton.setColour (juce::ToggleButton::tickColourId, green());
    repaint();
}

void SoliVoicerAudioProcessorEditor::timerCallback()
{
    chordLabel.setText (processorRef.getLastChordName(), juce::dontSendNotification);
    updateMaskToggles();

    const auto chordBankMode = sourceModeBox.getSelectedItemIndex() == 1;
    if (chordBankMode != lastChordBankMode)
    {
        lastChordBankMode = chordBankMode;
        updateModeVisibility();
    }
    if (chordBankMode)
    {
        const auto listening = processorRef.isChordBankListening();
        chordBankListenButton.setToggleState (listening, juce::dontSendNotification);
        chordBankPerformButton.setToggleState (! listening, juce::dontSendNotification);
        chordBankListenButton.setButtonText (listening ? "Listening" : "Listen");
        refreshChordBankCards();
    }
    repaint (getLocalBounds().withHeight (124));
}

void SoliVoicerAudioProcessorEditor::refreshChordBankCards()
{
    const auto bank = processorRef.getChordBankCards();
    const auto cardCount = static_cast<int> (bank.size());
    const auto needsRebuild = cardCount != static_cast<int> (chordBankCardButtons.size());

    if (cardCount > lastChordBankCardCount)
        selectedChordBankCard = cardCount - 1;
    else if (selectedChordBankCard >= cardCount)
        selectedChordBankCard = cardCount - 1;
    lastChordBankCardCount = cardCount;

    if (needsRebuild)
    {
        chordBankCardButtons.clear();
        chordBankProbabilityDials.clear();
        chordBankCardButtons.reserve (bank.size());
        chordBankProbabilityDials.reserve (bank.size());

        for (int index = 0; index < cardCount; ++index)
        {
            auto button = std::make_unique<ChordBankCardButton>();
            button->setClickingTogglesState (false);
            button->onClick = [this, index] { selectChordBankCard (index); };
            chordBankContent.addAndMakeVisible (*button);

            auto dial = std::make_unique<ChordBankProbabilitySlider>();
            dial->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
            dial->setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
            dial->setRange (0.0, 100.0, 1.0);
            dial->setDoubleClickReturnValue (true, 100.0);
            dial->setTooltip ("Relative chance this chord quality is chosen in Perform mode. Zero excludes it; equal dials give equal chances.");
            auto* dialPointer = dial.get();
            dial->onDragStart = [this, index] { selectChordBankCard (index); };
            dial->onDragEnd = [this] { grabKeyboardFocus(); };
            dial->onValueChange = [this, index, dialPointer]
            {
                processorRef.setChordBankCardProbability (index, static_cast<float> (dialPointer->getValue() / 100.0));
            };
            chordBankContent.addAndMakeVisible (*dial);

            chordBankCardButtons.push_back (std::move (button));
            chordBankProbabilityDials.push_back (std::move (dial));
        }
    }

    for (int index = 0; index < cardCount; ++index)
    {
        auto& button = *chordBankCardButtons[static_cast<std::size_t> (index)];
        auto& dial = *chordBankProbabilityDials[static_cast<std::size_t> (index)];
        button.setButtonText (juce::String (index + 1) + "  " + bank[static_cast<std::size_t> (index)].name);
        button.setToggleState (index == selectedChordBankCard, juce::dontSendNotification);
        button.setTooltip ("Select quality " + bank[static_cast<std::size_t> (index)].name
                           + ". Its dial sets the relative chance that this formula occurs in Perform mode.");
        dial.setValue (bank[static_cast<std::size_t> (index)].probability * 100.0f, juce::dontSendNotification);
    }

    const auto empty = cardCount == 0;
    chordBankCardsLabel.setVisible (empty);
    chordBankDeleteButton.setEnabled (! empty && selectedChordBankCard >= 0);
    chordBankClearButton.setEnabled (! empty);
    chordBankPerformButton.setEnabled (! empty);
    layoutChordBankCards();
}

void SoliVoicerAudioProcessorEditor::layoutChordBankCards()
{
    constexpr auto cardWidth = 156;
    constexpr auto cardHeight = 64;
    constexpr auto gap = 6;
    const auto count = static_cast<int> (chordBankCardButtons.size());
    const auto contentWidth = juce::jmax (chordBankViewport.getWidth(), gap + count * (cardWidth + gap));
    const auto contentHeight = juce::jmax (chordBankViewport.getHeight(), cardHeight + gap * 2);
    chordBankContent.setSize (contentWidth, contentHeight);
    chordBankCardsLabel.setBounds (8, 0, juce::jmax (0, contentWidth - 16), contentHeight);

    for (int index = 0; index < count; ++index)
    {
        const auto card = juce::Rectangle<int> (gap + index * (cardWidth + gap), gap, cardWidth, cardHeight);
        auto dialArea = card.withLeft (card.getRight() - 50).reduced (4, 7);
        chordBankCardButtons[static_cast<std::size_t> (index)]->setBounds (card);
        chordBankProbabilityDials[static_cast<std::size_t> (index)]->setBounds (dialArea);
    }
}

void SoliVoicerAudioProcessorEditor::selectChordBankCard (int index)
{
    const auto count = static_cast<int> (processorRef.getChordBankCards().size());
    selectedChordBankCard = juce::jlimit (-1, count - 1, index);
    grabKeyboardFocus();
    refreshChordBankCards();
}

void SoliVoicerAudioProcessorEditor::deleteSelectedChordBankCard()
{
    if (selectedChordBankCard < 0)
        return;
    processorRef.removeChordBankCard (selectedChordBankCard);
    const auto count = static_cast<int> (processorRef.getChordBankCards().size());
    selectedChordBankCard = juce::jmin (selectedChordBankCard, count - 1);
    lastChordBankCardCount = count;
    refreshChordBankCards();
    grabKeyboardFocus();
}

bool SoliVoicerAudioProcessorEditor::keyPressed (const juce::KeyPress& key)
{
    const auto keyCode = key.getKeyCode();
    if (sourceModeBox.getSelectedItemIndex() == 1
        && (keyCode == juce::KeyPress::deleteKey || keyCode == juce::KeyPress::backspaceKey))
    {
        deleteSelectedChordBankCard();
        return true;
    }
    return juce::AudioProcessorEditor::keyPressed (key);
}

void SoliVoicerAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (background());
    auto surface = getLocalBounds().toFloat().reduced (14.0f);
    drawRackFrame (g, surface, {});
    auto keyboard = surface.reduced (8.0f, 5.0f);
    keyboard.setY (surface.getY() + 70.0f);
    keyboard.setHeight (36.0f);
    drawVoicingKeyboard (g, keyboard,
                         processorRef.getVisualVoicedNoteMasks(),
                         processorRef.getVisualInputNoteMasks());
    paintGroupFrame (g, voicingGroupBounds, "Voicing");
}

void SoliVoicerAudioProcessorEditor::paintChordizerTimeline (juce::Graphics& g)
{
    if (timelineBounds.isEmpty())
        return;
    auto bounds = timelineBounds.toFloat();
    drawRackFrame (g, bounds, "CHORD TIMELINE");

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
    drawRackFrame (g, frame, title.toUpperCase());
}

void SoliVoicerAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced (28);
    auto header = bounds.removeFromTop (48);
    titleLabel.setBounds (header.removeFromLeft (150));
    randomButton.setBounds (header.removeFromRight (106).reduced (3, 8));
    resetButton.setBounds (header.removeFromRight (64).reduced (3, 8));
    newPhraseButton.setBounds (header.removeFromRight (94).reduced (3, 8));
    auto sourceModeCell = header.removeFromLeft (190).reduced (5, 0);
    sourceModeLabel.setBounds (sourceModeCell.removeFromTop (18));
    sourceModeBox.setBounds (sourceModeCell.removeFromTop (28));
    chordLabel.setBounds (header.reduced (8, 2));

    bounds.removeFromTop (8);
    bounds.removeFromTop (36); // live voicing keyboard, drawn above the control area
    bounds.removeFromTop (8);
    timelineBounds = {};
    auto manual = bounds.removeFromTop (122);
    const auto chordBankMode = sourceModeBox.getSelectedItemIndex() == 1;
    if (chordBankMode)
    {
        auto bankHeader = manual.removeFromTop (32);
        chordBankListenButton.setBounds (bankHeader.removeFromLeft (112).reduced (3, 2));
        chordBankPerformButton.setBounds (bankHeader.removeFromLeft (82).reduced (3, 2));
        chordBankClearButton.setBounds (bankHeader.removeFromRight (92).reduced (3, 2));
        chordBankDeleteButton.setBounds (bankHeader.removeFromRight (72).reduced (3, 2));
        chordBankViewport.setBounds (manual.reduced (3, 2));
        layoutChordBankCards();
    }
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

    bounds.removeFromTop (8);
    auto commonRow = bounds.removeFromTop (58);
    randomVoicingButton.setBounds (commonRow.removeFromRight (150).reduced (5, 18));
    const auto commonWidth = commonRow.getWidth() / (chordBankMode ? 2 : 4);
    auto placeCommon = [&] (juce::ComboBox& combo, juce::Label& label)
    {
        auto cell = commonRow.removeFromLeft (commonWidth).reduced (5, 0);
        label.setBounds (cell.removeFromTop (19));
        combo.setBounds (cell.removeFromTop (34));
    };
    placeCommon (roleBox, roleLabel);
    if (! chordBankMode)
    {
        placeCommon (styleBox, styleLabel);
        placeCommon (playabilityBox, playabilityLabel);
    }
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
    voicingControls.push_back ({ &harmonicStabilitySlider, &harmonicStabilityLabel });
    voicingControls.push_back ({ &melodyImportanceSlider, &melodyImportanceLabel });
    voicingControls.push_back ({ &repeatSlider, &repeatLabel });
    voicingControls.push_back ({ &strumSpeedSlider, &strumSpeedLabel });
    voicingGroupBounds = bounds;
    performanceGroupBounds = {};
    layoutSliderGrid (voicingGroupBounds.reduced (12, 24), voicingControls);
}

void SoliVoicerAudioProcessorEditor::layoutSliderGrid (
    juce::Rectangle<int> bounds,
    const std::vector<std::pair<juce::Slider*, juce::Label*>>& controls)
{
    const auto controlCount = static_cast<int> (controls.size());
    const auto columns = bounds.getWidth() >= 760 && controlCount <= 12 ? controlCount
                       : (bounds.getWidth() < 430 ? 3 : (bounds.getWidth() < 620 ? 4 : 5));
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
    const auto chordBankMode = sourceModeBox.getSelectedItemIndex() == 1;
    sourceModeBox.setVisible (true);
    sourceModeLabel.setVisible (true);
    keyLabel.setVisible (! chordBankMode);
    scaleLabel.setVisible (! chordBankMode);
    randomKeyScaleButton.setVisible (! chordBankMode);
    for (auto& button : keyToggles) button.setVisible (! chordBankMode);
    for (auto& button : scaleToggles) button.setVisible (! chordBankMode);
    chordBankListenButton.setVisible (chordBankMode);
    chordBankPerformButton.setVisible (chordBankMode);
    chordBankDeleteButton.setVisible (chordBankMode);
    chordBankClearButton.setVisible (chordBankMode);
    chordBankViewport.setVisible (chordBankMode);

    const std::array<juce::Component*, 10> retiredSourceComponents
    {{
        &outputModeBox, &outputModeLabel,
        &contextModeBox, &contextModeLabel, &substitutionSlider, &substitutionLabel,
        &linkStatusLabel, &performanceStyleBox, &performanceStyleLabel, &performanceSubStyleBox
    }};
    for (auto* component : retiredSourceComponents)
        component->setVisible (false);
    performanceSubStyleLabel.setVisible (false);
    doubleTimeButton.setVisible (false);
    randomPerformanceButton.setVisible (false);

    strumModeBox.setVisible (true);
    strumModeLabel.setVisible (true);
    repeatSlider.setVisible (true);
    repeatLabel.setVisible (true);
    strumSpeedSlider.setVisible (true);
    strumSpeedLabel.setVisible (true);
    styleBox.setVisible (! chordBankMode);
    styleLabel.setVisible (! chordBankMode);
    playabilityBox.setVisible (! chordBankMode);
    playabilityLabel.setVisible (! chordBankMode);
    randomVoicingButton.setVisible (! chordBankMode);

    const std::array<juce::Component*, 16> manualVoicingOnly
    {{
        &chordSizeSlider, &chordSizeLabel, &complexitySlider, &complexityLabel,
        &voiceLeadingSlider, &voiceLeadingLabel, &outsideSlider, &outsideLabel,
        &variationSlider, &variationLabel, &repeatSlider, &repeatLabel,
        &harmonicStabilitySlider, &harmonicStabilityLabel, &melodyImportanceSlider, &melodyImportanceLabel
    }};
    for (auto* component : manualVoicingOnly)
        component->setVisible (! chordBankMode);
    minNoteSlider.setVisible (true);
    minNoteLabel.setVisible (true);
    maxNoteSlider.setVisible (true);
    maxNoteLabel.setVisible (true);
    const std::array<juce::Component*, 16> performanceComponents
    {{
        &performanceComplexitySlider, &performanceComplexityLabel,
        &densitySlider, &densityLabel, &syncopationSlider, &syncopationLabel,
        &swingSlider, &swingLabel, &humanizeSlider, &humanizeLabel,
        &gateSlider, &gateLabel, &performanceSubStyleBox, &performanceSubStyleLabel,
        &doubleTimeButton, &randomPerformanceButton
    }};
    for (auto* component : performanceComponents)
        component->setVisible (false);

    if (chordBankMode)
        refreshChordBankCards();
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
    setParameterValue (ParameterIDs::outside, random.nextFloat() * 0.25f);
    setParameterValue (ParameterIDs::variation, random.nextFloat() * 0.8f);
    setParameterValue (ParameterIDs::harmonicStability, 0.35f + random.nextFloat() * 0.65f);
    setParameterValue (ParameterIDs::melodyImportance, 0.55f + random.nextFloat() * 0.45f);
    setParameterValue (ParameterIDs::repeatChance, random.nextFloat() * 0.45f);
    setParameterValue (ParameterIDs::strumSpeed, random.nextFloat() * 0.65f);
    setParameterValue (ParameterIDs::minNote, static_cast<float> (minNote));
    setParameterValue (ParameterIDs::maxNote, static_cast<float> (maxNote));
    processorRef.panic();
}

void SoliVoicerAudioProcessorEditor::randomizeAllSettings()
{
    randomizeKeyScaleSettings();
    randomizeVoicingSettings();
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
    setParameterValue (ParameterIDs::sourceMode, 0.0f);
    setParameterValue (ParameterIDs::outputMode, 0.0f);
    setParameterValue (ParameterIDs::chordSize, 4.0f);
    setParameterValue (ParameterIDs::complexity, 0.45f);
    setParameterValue (ParameterIDs::voiceLeading, 0.75f);
    setParameterValue (ParameterIDs::outside, 0.05f);
    setParameterValue (ParameterIDs::variation, 0.35f);
    setParameterValue (ParameterIDs::repeatChance, 0.15f);
    setParameterValue (ParameterIDs::strumSpeed, 0.0f);
    setParameterValue (ParameterIDs::minNote, 36.0f);
    setParameterValue (ParameterIDs::maxNote, 96.0f);
    setParameterValue (ParameterIDs::contextMode, 3.0f);
    setParameterValue (ParameterIDs::substitutionDepth, 0.35f);
    setParameterValue (ParameterIDs::harmonicStability, 0.72f);
    setParameterValue (ParameterIDs::melodyImportance, 0.88f);
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
