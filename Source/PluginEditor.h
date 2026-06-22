#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

#include <array>
#include <memory>

class SoliVoicerLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    SoliVoicerLookAndFeel();
    void drawLinearSlider (juce::Graphics&, int, int, int, int, float, float, float,
                           const juce::Slider::SliderStyle, juce::Slider&) override;
    void drawComboBox (juce::Graphics&, int, int, bool, int, int, int, int, juce::ComboBox&) override;
};

class SoliVoicerMidiDragButton final : public juce::TextButton
{
public:
    using juce::TextButton::TextButton;
    std::function<void()> onDragStart;

    void mouseDown (const juce::MouseEvent& event) override;
    void mouseDrag (const juce::MouseEvent& event) override;
    void mouseUp (const juce::MouseEvent& event) override;

private:
    bool dragStarted = false;
};

class SoliVoicerAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                             private juce::Timer
{
public:
    explicit SoliVoicerAudioProcessorEditor (SoliVoicerAudioProcessor&);
    ~SoliVoicerAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

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
    void updatePerformanceSubStyleChoices();
    void updateRecorderControls();
    void randomizeAllSettings();
    void randomizeVoicingSettings();
    void randomizeKeyScaleSettings();
    void randomizePerformanceSettings();
    void resetDefaults();
    void paintChordizerTimeline (juce::Graphics&);
    void paintGroupFrame (juce::Graphics&, juce::Rectangle<int>, const juce::String&);
    void paintRecordedMidi (juce::Graphics&);
    void beginRecordedMidiDrag();
    void layoutSliderGrid (juce::Rectangle<int>, const std::vector<std::pair<juce::Slider*, juce::Label*>>&);

    SoliVoicerAudioProcessor& processorRef;
    SoliVoicerLookAndFeel lookAndFeel;
    std::unique_ptr<juce::TooltipWindow> tooltipWindow;
    Soli::ChordizerSnapshot chordizerSnapshot;
    SoliVoicerAudioProcessor::RecordedMidiSnapshot recordedMidi;
    juce::Rectangle<int> timelineBounds;
    juce::Rectangle<int> midiRecordBounds;
    juce::Rectangle<int> midiShapeBounds;
    juce::Rectangle<int> voicingGroupBounds;
    juce::Rectangle<int> performanceGroupBounds;
    double timelineScrollPpq = 0.0;
    double lastTimelinePlayheadPpq = -1.0;

    juce::Label titleLabel;
    juce::Label chordLabel;
    juce::Label linkStatusLabel;
    juce::Label midiRecordLabel;
    juce::Label midiRecordStatusLabel;
    juce::TextButton randomButton { "Randomize All" };
    juce::TextButton randomVoicingButton { "Randomize Voicing" };
    juce::TextButton randomKeyScaleButton { "Random Keys/Scales" };
    juce::TextButton resetButton { "Reset" };
    juce::TextButton randomPerformanceButton { "Randomize Performance" };
    juce::TextButton recordMidiButton { "Record MIDI" };
    SoliVoicerMidiDragButton dragMidiButton { "Drag MIDI" };
    juce::TextButton clearMidiButton { "Clear MIDI" };

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
    std::unique_ptr<SliderAttachment> performanceComplexityAttachment;
    std::unique_ptr<SliderAttachment> densityAttachment;
    std::unique_ptr<SliderAttachment> syncopationAttachment;
    std::unique_ptr<SliderAttachment> swingAttachment;
    std::unique_ptr<SliderAttachment> humanizeAttachment;
    std::unique_ptr<SliderAttachment> gateAttachment;
    std::unique_ptr<ButtonAttachment> doubleTimeAttachment;

    bool syncingMasks = false;
    bool updatingSubStyleChoices = false;
    int lastSourceMode = -1;
    int lastOutputMode = -1;
    int lastPerformanceStyle = -1;
    bool midiShapeDragArmed = false;
    juce::File lastMidiExportFile;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SoliVoicerAudioProcessorEditor)
};
