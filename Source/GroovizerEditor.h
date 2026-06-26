#pragma once

#include <JuceHeader.h>
#include "GroovizerProcessor.h"

#include <memory>

class GroovizerLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    GroovizerLookAndFeel();
    void drawComboBox (juce::Graphics&, int, int, bool, int, int, int, int, juce::ComboBox&) override;
};

class GroovizerMidiDragButton final : public juce::TextButton
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

class GroovizerAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                            private juce::Timer
{
public:
    explicit GroovizerAudioProcessorEditor (GroovizerAudioProcessor&);
    ~GroovizerAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    void timerCallback() override;
    void configureLabel (juce::Label&, const juce::String&, float size, bool bold = false);
    void configureCombo (juce::ComboBox&, const juce::StringArray&);
    void configureSlider (juce::Slider&, double min, double max, double interval);
    void addLabeledCombo (juce::ComboBox&, juce::Label&, const juce::String&, const juce::StringArray&);
    void addLabeledSlider (juce::Slider&, juce::Label&, const juce::String&, double min, double max, double interval);
    void paintTimeline (juce::Graphics&);
    void paintCapture (juce::Graphics&);
    void updateStatus();
    void beginTimelineDrag();
    void beginCaptureDrag();

    GroovizerAudioProcessor& processorRef;
    GroovizerLookAndFeel lookAndFeel;
    std::unique_ptr<juce::TooltipWindow> tooltipWindow;
    GroovizerAudioProcessor::TimelineSnapshot timeline;
    GroovizerAudioProcessor::RecordedMidiSnapshot recordedMidi;
    juce::Rectangle<int> timelineBounds;
    juce::Rectangle<int> captureBounds;
    juce::File lastDragFile;

    juce::Label titleLabel;
    juce::Label statusLabel;
    juce::Label captureStatusLabel;
    juce::Label styleLabel;
    juce::Label triggerLabel;
    juce::Label phraseLabel;
    juce::Label feelLabel;
    juce::Label lengthLabel;
    juce::Label densityLabel;
    juce::Label swingLabel;
    juce::Label humanizeLabel;
    juce::Label fillLabel;
    juce::Label variationLabel;
    juce::Label ghostsLabel;
    juce::Label channelLabel;

    juce::ComboBox styleBox;
    juce::ComboBox triggerBox;
    juce::ComboBox phraseBox;
    juce::ComboBox feelBox;
    juce::Slider lengthSlider;
    juce::Slider densitySlider;
    juce::Slider swingSlider;
    juce::Slider humanizeSlider;
    juce::Slider fillSlider;
    juce::Slider variationSlider;
    juce::Slider ghostsSlider;
    juce::Slider channelSlider;

    juce::ToggleButton liveButton { "Live" };
    juce::ToggleButton stepInputButton { "Step In" };
    juce::ToggleButton timelineButton { "Timeline" };
    juce::ToggleButton passInputButton { "Pass In" };
    juce::TextButton addRegionButton { "Add Region" };
    juce::TextButton randomButton { "Random Arrange" };
    juce::TextButton deleteButton { "Delete" };
    juce::TextButton clearTimelineButton { "Clear" };
    juce::TextButton cursorBackButton { "< Bar" };
    juce::TextButton cursorNextButton { "Bar >" };
    juce::TextButton recordMidiButton { "Record MIDI" };
    GroovizerMidiDragButton dragTimelineButton { "Drag Timeline" };
    GroovizerMidiDragButton dragCaptureButton { "Drag Capture" };
    juce::TextButton clearCaptureButton { "Clear Capture" };
    juce::TextButton panicButton { "Panic" };

    std::unique_ptr<ComboAttachment> styleAttachment;
    std::unique_ptr<ComboAttachment> triggerAttachment;
    std::unique_ptr<ComboAttachment> phraseAttachment;
    std::unique_ptr<ComboAttachment> feelAttachment;
    std::unique_ptr<SliderAttachment> lengthAttachment;
    std::unique_ptr<SliderAttachment> densityAttachment;
    std::unique_ptr<SliderAttachment> swingAttachment;
    std::unique_ptr<SliderAttachment> humanizeAttachment;
    std::unique_ptr<SliderAttachment> fillAttachment;
    std::unique_ptr<SliderAttachment> variationAttachment;
    std::unique_ptr<SliderAttachment> ghostsAttachment;
    std::unique_ptr<SliderAttachment> channelAttachment;
    std::unique_ptr<ButtonAttachment> liveAttachment;
    std::unique_ptr<ButtonAttachment> stepInputAttachment;
    std::unique_ptr<ButtonAttachment> timelineAttachment;
    std::unique_ptr<ButtonAttachment> passInputAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GroovizerAudioProcessorEditor)
};
