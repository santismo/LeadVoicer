#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

#include <array>
#include <vector>

class SoliVoicerLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    SoliVoicerLookAndFeel();
    void drawRotarySlider (juce::Graphics& g,
                           int x,
                           int y,
                           int width,
                           int height,
                           float sliderPos,
                           float rotaryStartAngle,
                           float rotaryEndAngle,
                           juce::Slider& slider) override;
    void drawComboBox (juce::Graphics& g,
                       int width,
                       int height,
                       bool isButtonDown,
                       int buttonX,
                       int buttonY,
                       int buttonW,
                       int buttonH,
                       juce::ComboBox& box) override;
};

class SoliVoicerAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                             private juce::Timer
{
public:
    explicit SoliVoicerAudioProcessorEditor (SoliVoicerAudioProcessor& processor);
    ~SoliVoicerAudioProcessorEditor() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    void timerCallback() override;
    void configureSlider (juce::Slider& slider);
    void configureCombo (juce::ComboBox& combo, const juce::StringArray& names);
    void addLabeledSlider (juce::Slider& slider, juce::Label& label, const juce::String& text);
    void addLabeledCombo (juce::ComboBox& combo, juce::Label& label, const juce::String& text, const juce::StringArray& names);
    void configureMaskToggle (juce::ToggleButton& button, const juce::String& text);
    void configureInfoButton (juce::TextButton& button, const juce::String& tooltip);
    void updateMaskToggles();
    void commitMaskFromToggles (const juce::String& parameterID, const std::array<juce::ToggleButton, 12>& toggles, int count);
    void setParameterValue (const juce::String& parameterID, float value);
    void randomizeSettings();
    void resetDefaults();
    void updateHelpState();
    void installTooltips();
    void setHelpTooltip (juce::SettableTooltipClient& component, juce::TextButton& infoButton, const juce::String& text);

    SoliVoicerAudioProcessor& processorRef;
    SoliVoicerLookAndFeel lookAndFeel;
    std::unique_ptr<juce::TooltipWindow> tooltipWindow;

    juce::Label titleLabel;
    juce::Label subtitleLabel;
    juce::Label chordLabel;
    juce::TextButton randomButton { "Random" };
    juce::TextButton resetButton { "Default" };
    juce::ToggleButton helpButton { "Help" };
    std::array<juce::TextButton, 15> infoButtons;

    std::array<juce::ToggleButton, 12> keyToggles;
    std::array<juce::ToggleButton, 12> scaleToggles;
    juce::TextButton keyAllButton { "All Keys" };
    juce::TextButton keyDefaultButton { "C Only" };
    juce::TextButton scaleAllButton { "All Scales" };
    juce::TextButton scaleDefaultButton { "Ionian Only" };
    juce::ComboBox roleBox;
    juce::ComboBox styleBox;
    juce::ComboBox playabilityBox;
    juce::ComboBox strumModeBox;

    juce::Slider chordSizeSlider;
    juce::Slider complexitySlider;
    juce::Slider voiceLeadingSlider;
    juce::Slider outsideSlider;
    juce::Slider variationSlider;
    juce::Slider repeatSlider;
    juce::Slider strumSpeedSlider;
    juce::Slider minNoteSlider;
    juce::Slider maxNoteSlider;

    juce::Label keyLabel;
    juce::Label scaleLabel;
    juce::Label roleLabel;
    juce::Label styleLabel;
    juce::Label playabilityLabel;
    juce::Label strumModeLabel;
    juce::Label chordSizeLabel;
    juce::Label complexityLabel;
    juce::Label voiceLeadingLabel;
    juce::Label outsideLabel;
    juce::Label variationLabel;
    juce::Label repeatLabel;
    juce::Label strumSpeedLabel;
    juce::Label minNoteLabel;
    juce::Label maxNoteLabel;

    std::unique_ptr<ComboBoxAttachment> roleAttachment;
    std::unique_ptr<ComboBoxAttachment> styleAttachment;
    std::unique_ptr<ComboBoxAttachment> playabilityAttachment;
    std::unique_ptr<ComboBoxAttachment> strumModeAttachment;
    std::unique_ptr<SliderAttachment> chordSizeAttachment;
    std::unique_ptr<SliderAttachment> complexityAttachment;
    std::unique_ptr<SliderAttachment> voiceLeadingAttachment;
    std::unique_ptr<SliderAttachment> outsideAttachment;
    std::unique_ptr<SliderAttachment> variationAttachment;
    std::unique_ptr<SliderAttachment> repeatAttachment;
    std::unique_ptr<SliderAttachment> strumSpeedAttachment;
    std::unique_ptr<SliderAttachment> minNoteAttachment;
    std::unique_ptr<SliderAttachment> maxNoteAttachment;
    bool syncingMaskToggles = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SoliVoicerAudioProcessorEditor)
};
