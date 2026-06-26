#pragma once

#include <JuceHeader.h>
#include "GroovizerEngine.h"

#include <atomic>
#include <array>
#include <cstdint>
#include <mutex>
#include <random>
#include <vector>

namespace GroovizerParameterIDs
{
    static constexpr auto style = "style";
    static constexpr auto trigger = "trigger";
    static constexpr auto phrase = "phrase";
    static constexpr auto feel = "feel";
    static constexpr auto lengthBars = "lengthBars";
    static constexpr auto density = "density";
    static constexpr auto swing = "swing";
    static constexpr auto humanize = "humanize";
    static constexpr auto fill = "fill";
    static constexpr auto variation = "variation";
    static constexpr auto ghosts = "ghosts";
    static constexpr auto liveMode = "liveMode";
    static constexpr auto stepInput = "stepInput";
    static constexpr auto timelineEnabled = "timelineEnabled";
    static constexpr auto passInput = "passInput";
    static constexpr auto outputChannel = "outputChannel";
}

class GroovizerAudioProcessor final : public juce::AudioProcessor
{
public:
    struct GrooveRegion
    {
        double startPpq = 0.0;
        double lengthPpq = 4.0;
        int triggerNote = 36;
        int velocity = 100;
        std::uint32_t seed = 1;
        Groovizer::Settings settings;
        juce::String name;
    };

    struct TimelineSnapshot
    {
        bool enabled = true;
        bool hostPlaying = false;
        double hostPpq = 0.0;
        double cursorPpq = 0.0;
        int selectedIndex = -1;
        std::vector<GrooveRegion> regions;
    };

    struct RecordedMidiEvent
    {
        double ppq = 0.0;
        juce::MidiMessage message;
    };

    struct RecordedMidiSnapshot
    {
        bool recording = false;
        bool hasOrigin = false;
        double originPpq = 0.0;
        double endPpq = 0.0;
        double bpm = 120.0;
        int numerator = 4;
        int denominator = 4;
        std::vector<RecordedMidiEvent> events;
    };

    GroovizerAudioProcessor();
    ~GroovizerAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return true; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getValueTreeState() noexcept { return parameters; }
    juce::String getLastGrooveName() const;
    TimelineSnapshot getTimelineSnapshot() const;
    RecordedMidiSnapshot recordedMidiSnapshot() const;

    void setTimelineCursor (double ppq);
    void moveTimelineCursorBars (int bars);
    void selectRegionAt (double ppq);
    void addRegionAtCursor();
    void addRandomArrangement (int regionCount);
    void deleteSelectedRegion();
    void clearTimeline();
    void panic();

    void setMidiRecordingEnabled (bool shouldRecord);
    bool isMidiRecording() const;
    void clearRecordedMidi();
    bool writeRecordedMidiFile (const juce::File& destination) const;
    bool writeTimelineMidiFile (const juce::File& destination) const;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    struct PendingMidi
    {
        juce::MidiMessage message;
        int samplesUntil = 0;
    };

    struct Transport
    {
        bool valid = false;
        bool playing = false;
        double ppq = 0.0;
        double bpm = 120.0;
        int numerator = 4;
        int denominator = 4;
    };

    static int refIndex (int channel, int note) noexcept;
    Groovizer::Settings readSettings() const;
    int outputChannel() const;
    Transport readTransport() const;
    void emitPendingMidi (int blockSamples, juce::MidiBuffer& output);
    void scheduleMidiEvent (const juce::MidiMessage& message, int sampleOffset, int blockSamples, juce::MidiBuffer& output);
    void emitPhrase (const Groovizer::Phrase& phrase,
                     int baseSample,
                     const Transport& transport,
                     int blockSamples,
                     juce::MidiBuffer& output);
    void renderTimeline (const Transport& transport, int blockSamples, juce::MidiBuffer& output);
    void insertRegionAtCursor (int triggerNote, int velocity, const Groovizer::Settings& settings);
    void recordOutputMidi (const juce::MidiBuffer& output, const Transport& transport, int blockSamples);
    void closeRecordedNotesLocked (double closePpq) const;
    bool writeMidiEventsToFile (const juce::File& destination,
                                const std::vector<RecordedMidiEvent>& events,
                                double originPpq,
                                double endPpq,
                                double bpm,
                                int numerator,
                                int denominator,
                                const juce::String& trackName) const;

    juce::AudioProcessorValueTreeState parameters;
    Groovizer::Engine engine;
    std::vector<PendingMidi> pendingMidi;
    juce::int64 processedSamples = 0;
    std::atomic<std::uint32_t> seedCounter { 1 };
    mutable std::mutex timelineMutex;
    std::vector<GrooveRegion> timelineRegions;
    double timelineCursorPpq = 0.0;
    int selectedRegion = -1;
    std::atomic<double> lastHostPpq { 0.0 };
    std::atomic<bool> lastHostPlaying { false };
    bool lastTransportPlaying = false;
    double lastTransportPpq = -1.0;
    mutable std::mutex nameMutex;
    juce::String lastGrooveName = "--";
    mutable std::mutex recordingMutex;
    mutable std::vector<RecordedMidiEvent> recordedMidiEvents;
    mutable std::array<int, 16 * 128> recordedNoteRefs {};
    mutable bool midiRecording = false;
    mutable bool midiRecordHasOrigin = false;
    mutable double midiRecordOriginPpq = 0.0;
    mutable double midiRecordEndPpq = 0.0;
    mutable double midiRecordBpm = 120.0;
    mutable int midiRecordNumerator = 4;
    mutable int midiRecordDenominator = 4;
    mutable juce::int64 midiRecordOriginSample = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GroovizerAudioProcessor)
};
