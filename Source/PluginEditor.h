#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "../SongizerLogicLookAndFeel.h"

#include <array>
#include <memory>

class SoliVoicerLookAndFeel final : public SongizerLogicLookAndFeel
{
public:
    SoliVoicerLookAndFeel();
    void setSkin (int skinIndex);
    void drawLinearSlider (juce::Graphics&, int, int, int, int, float, float, float,
                           const juce::Slider::SliderStyle, juce::Slider&) override;
    void drawComboBox (juce::Graphics&, int, int, bool, int, int, int, int, juce::ComboBox&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool, bool) override;
};

class SoliVoicerAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                             private juce::Timer
{
public:
    explicit SoliVoicerAudioProcessorEditor (SoliVoicerAudioProcessor&);
    ~SoliVoicerAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    void timerCallback() override;
    void configureSlider (juce::Slider&, const juce::String& tooltip);
    void configureCombo (juce::ComboBox&, const juce::StringArray&, const juce::String& tooltip);
    void configureLabel (juce::Label&, const juce::String&);
    void addSlider (juce::Slider&, juce::Label&, const juce::String&, const juce::String&);
    void addCombo (juce::ComboBox&, juce::Label&, const juce::String&, const juce::StringArray&, const juce::String&);
    void configureMaskToggle (juce::ToggleButton&, const juce::String&);
    void updateMaskToggles();
    void commitMask (const juce::String&, const std::array<juce::ToggleButton, 12>&, int);
    void setParameterValue (const juce::String&, float);
    void updateModeVisibility();
    void refreshChordBankCards();
    void layoutChordBankCards();
    void selectChordBankCard (int index);
    void deleteSelectedChordBankCard();
    void refreshLockedChordCards();
    void layoutLockedChordCards();
    void selectLockedChord (int inputNote);
    void deleteSelectedLockedChord();
    void updatePerformanceSubStyleChoices();
    void applySkin (int skinIndex);
    void randomizeAllSettings();
    void randomizeVoicingSettings();
    void randomizeKeyScaleSettings();
    void randomizePerformanceSettings();
    void resetDefaults();
    void paintChordizerTimeline (juce::Graphics&);
    void paintGroupFrame (juce::Graphics&, juce::Rectangle<int>, const juce::String&);
    void layoutSliderGrid (juce::Rectangle<int>, const std::vector<std::pair<juce::Slider*, juce::Label*>>&);

    SoliVoicerAudioProcessor& processorRef;
    SoliVoicerLookAndFeel lookAndFeel;
    std::unique_ptr<juce::TooltipWindow> tooltipWindow;
    Soli::ChordizerSnapshot chordizerSnapshot;
    juce::Rectangle<int> timelineBounds;
    juce::Rectangle<int> voicingGroupBounds;
    juce::Rectangle<int> voiceShapeGroupBounds;
    juce::Rectangle<int> rangeGestureGroupBounds;
    juce::Rectangle<int> phraseLogicGroupBounds;
    juce::Rectangle<int> inputLocksGroupBounds;
    juce::Rectangle<int> performanceGroupBounds;
    double timelineScrollPpq = 0.0;

    juce::Label titleLabel;
    juce::Label chordLabel;
    juce::Label linkStatusLabel;
    juce::TextButton randomButton { "Randomize All" };
    juce::TextButton randomVoicingButton { "Randomize Voicing" };
    juce::TextButton randomKeyScaleButton { "Random Keys/Scales" };
    juce::TextButton resetButton { "Reset" };
    juce::TextButton newPhraseButton { "New Phrase" };
    juce::TextButton randomPerformanceButton { "Randomize Performance" };
    juce::TextButton chordBankListenButton { "Listen" };
    juce::TextButton chordBankPerformButton { "Perform" };
    juce::TextButton chordBankDeleteButton { "Delete" };
    juce::TextButton chordBankClearButton { "Clear Bank" };
    juce::Component chordBankContent;
    juce::Viewport chordBankViewport;
    juce::Label chordBankCardsLabel;
    std::vector<std::unique_ptr<juce::TextButton>> chordBankCardButtons;
    std::vector<std::unique_ptr<juce::Slider>> chordBankProbabilityDials;
    juce::ToggleButton phraseMemoryButton { "Phrase Memory" };
    juce::ToggleButton complexityEnabledButton;
    juce::ToggleButton voiceLeadingEnabledButton;
    juce::ToggleButton outsideEnabledButton;
    juce::ToggleButton stabilityEnabledButton;
    juce::ToggleButton melodyEnabledButton;
    juce::TextButton lockLastChordButton { "Lock Last Chord" };
    juce::TextButton unlockSelectedButton { "Unlock Selected" };
    juce::TextButton unlockAllButton { "Unlock All" };
    juce::Component lockedChordContent;
    juce::Viewport lockedChordViewport;
    juce::Label lockedChordEmptyLabel;
    std::vector<std::unique_ptr<juce::TextButton>> lockedChordButtons;
    juce::ComboBox sourceModeBox;
    juce::ComboBox outputModeBox;
    juce::ComboBox contextModeBox;
    juce::ComboBox roleBox;
    juce::ComboBox styleBox;
    juce::ComboBox playabilityBox;
    juce::ComboBox strumModeBox;
    juce::ComboBox performanceStyleBox;
    juce::ComboBox performanceSubStyleBox;
    juce::ToggleButton doubleTimeButton;

    juce::Label sourceModeLabel;
    juce::Label outputModeLabel;
    juce::Label contextModeLabel;
    juce::Label roleLabel;
    juce::Label styleLabel;
    juce::Label playabilityLabel;
    juce::Label strumModeLabel;
    juce::Label performanceStyleLabel;
    juce::Label performanceSubStyleLabel;
    juce::Label keyLabel;
    juce::Label scaleLabel;

    std::array<juce::ToggleButton, 12> keyToggles;
    std::array<juce::ToggleButton, 12> scaleToggles;

    juce::Slider chordSizeSlider;
    juce::Slider complexitySlider;
    juce::Slider voiceLeadingSlider;
    juce::Slider outsideSlider;
    juce::Slider variationSlider;
    juce::Slider repeatSlider;
    juce::Slider strumSpeedSlider;
    juce::Slider minNoteSlider;
    juce::Slider maxNoteSlider;
    juce::Slider substitutionSlider;
    juce::Slider harmonicStabilitySlider;
    juce::Slider melodyImportanceSlider;
    juce::Slider modulationSlider;
    juce::Slider performanceComplexitySlider;
    juce::Slider densitySlider;
    juce::Slider syncopationSlider;
    juce::Slider swingSlider;
    juce::Slider humanizeSlider;
    juce::Slider gateSlider;

    juce::Label chordSizeLabel;
    juce::Label complexityLabel;
    juce::Label voiceLeadingLabel;
    juce::Label outsideLabel;
    juce::Label variationLabel;
    juce::Label repeatLabel;
    juce::Label strumSpeedLabel;
    juce::Label minNoteLabel;
    juce::Label maxNoteLabel;
    juce::Label substitutionLabel;
    juce::Label harmonicStabilityLabel;
    juce::Label melodyImportanceLabel;
    juce::Label modulationLabel;
    juce::Label performanceComplexityLabel;
    juce::Label densityLabel;
    juce::Label syncopationLabel;
    juce::Label swingLabel;
    juce::Label humanizeLabel;
    juce::Label gateLabel;

    std::unique_ptr<ComboAttachment> sourceModeAttachment;
    std::unique_ptr<ComboAttachment> outputModeAttachment;
    std::unique_ptr<ComboAttachment> contextModeAttachment;
    std::unique_ptr<ComboAttachment> roleAttachment;
    std::unique_ptr<ComboAttachment> styleAttachment;
    std::unique_ptr<ComboAttachment> playabilityAttachment;
    std::unique_ptr<ComboAttachment> strumModeAttachment;
    std::unique_ptr<ComboAttachment> performanceStyleAttachment;
    std::unique_ptr<ComboAttachment> performanceSubStyleAttachment;
    std::unique_ptr<SliderAttachment> chordSizeAttachment;
    std::unique_ptr<SliderAttachment> complexityAttachment;
    std::unique_ptr<SliderAttachment> voiceLeadingAttachment;
    std::unique_ptr<SliderAttachment> outsideAttachment;
    std::unique_ptr<SliderAttachment> variationAttachment;
    std::unique_ptr<SliderAttachment> repeatAttachment;
    std::unique_ptr<SliderAttachment> strumSpeedAttachment;
    std::unique_ptr<SliderAttachment> minNoteAttachment;
    std::unique_ptr<SliderAttachment> maxNoteAttachment;
    std::unique_ptr<SliderAttachment> substitutionAttachment;
    std::unique_ptr<SliderAttachment> harmonicStabilityAttachment;
    std::unique_ptr<SliderAttachment> melodyImportanceAttachment;
    std::unique_ptr<SliderAttachment> modulationAttachment;
    std::unique_ptr<SliderAttachment> performanceComplexityAttachment;
    std::unique_ptr<SliderAttachment> densityAttachment;
    std::unique_ptr<SliderAttachment> syncopationAttachment;
    std::unique_ptr<SliderAttachment> swingAttachment;
    std::unique_ptr<SliderAttachment> humanizeAttachment;
    std::unique_ptr<SliderAttachment> gateAttachment;
    std::unique_ptr<ButtonAttachment> doubleTimeAttachment;
    std::unique_ptr<ButtonAttachment> phraseMemoryAttachment;
    std::unique_ptr<ButtonAttachment> complexityEnabledAttachment;
    std::unique_ptr<ButtonAttachment> voiceLeadingEnabledAttachment;
    std::unique_ptr<ButtonAttachment> outsideEnabledAttachment;
    std::unique_ptr<ButtonAttachment> stabilityEnabledAttachment;
    std::unique_ptr<ButtonAttachment> melodyEnabledAttachment;

    bool syncingMasks = false;
    bool updatingSubStyleChoices = false;
    int lastPerformanceStyle = -1;
    bool lastChordBankMode = false;
    bool lastSimpleMode = true;
    int selectedChordBankCard = -1;
    int lastChordBankCardCount = 0;
    int selectedLockedInputNote = -1;
    int lastLockedChordCount = 0;
    juce::File lastMidiExportFile;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SoliVoicerAudioProcessorEditor)
};
