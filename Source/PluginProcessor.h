#pragma once

#include <JuceHeader.h>
#include "ChordEngine.h"
#include "ChordizerLink.h"

#include <array>
#include <mutex>
#include <random>
#include <vector>

namespace ParameterIDs
{
    static constexpr auto keyMask = "keyMask";
    static constexpr auto scaleMask = "scaleMask";
    static constexpr auto role = "role";
    static constexpr auto style = "style";
    static constexpr auto playability = "playability";
    static constexpr auto strumMode = "strumMode";
    static constexpr auto chordSize = "chordSize";
    static constexpr auto complexity = "complexity";
    static constexpr auto voiceLeading = "voiceLeading";
    static constexpr auto outside = "outside";
    static constexpr auto variation = "variation";
    static constexpr auto repeatChance = "repeatChance";
    static constexpr auto strumSpeed = "strumSpeed";
    static constexpr auto minNote = "minNote";
    static constexpr auto maxNote = "maxNote";
    static constexpr auto sourceMode = "sourceMode";
    static constexpr auto outputMode = "outputMode";
    static constexpr auto contextMode = "contextMode";
    static constexpr auto substitutionDepth = "substitutionDepth";
    static constexpr auto performanceStyle = "performanceStyle";
    static constexpr auto performanceSubStyle = "performanceSubStyle";
    static constexpr auto performanceComplexity = "performanceComplexity";
    static constexpr auto rhythmDensity = "rhythmDensity";
    static constexpr auto syncopation = "syncopation";
    static constexpr auto swing = "swing";
    static constexpr auto humanize = "humanize";
    static constexpr auto gate = "gate";
    static constexpr auto doubleTime = "doubleTime";
}

class SoliVoicerAudioProcessor final : public juce::AudioProcessor
{
public:
    SoliVoicerAudioProcessor();
    ~SoliVoicerAudioProcessor() override = default;

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
    juce::String getLastChordName() const;
    Soli::ChordizerSnapshot getChordizerSnapshot() const;
    void panic();

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

    void setMidiRecordingEnabled (bool shouldRecord);
    bool isMidiRecording() const;
    void clearRecordedMidi();
    RecordedMidiSnapshot recordedMidiSnapshot() const;
    bool writeRecordedMidiFile (const juce::File& destination) const;

    static juce::StringArray sourceModeNames();
    static juce::StringArray outputModeNames();
    static juce::StringArray performanceStyleNames();
    static juce::StringArray performanceSubStyleNames (int styleIndex);
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    struct ActiveChord
    {
        std::vector<int> notes;
        int channel = 1;
    };

    struct PendingMidi
    {
        juce::MidiMessage message;
        int samplesUntil = 0;
    };

    struct PerformanceChannel
    {
        bool held = false;
        int inputNote = -1;
        int velocity = 100;
        int channel = 1;
        int step = 0;
        int phraseVariant = 0;
        int articulationVariant = 0;
        float intensityBias = 0.0f;
        double nextStepPpq = -1.0;
        juce::String contextName;
        std::vector<int> voicing;
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

    static int activeIndex (int channel, int note) noexcept;
    static int refIndex (int channel, int note) noexcept;
    Soli::Settings readSettings() const;
    void emitPendingMidi (int blockSamples, juce::MidiBuffer& output);
    void scheduleMidiEvent (const juce::MidiMessage& message, int sampleOffset, int blockSamples, juce::MidiBuffer& output);
    void releaseActiveChord (int channel, int inputNote, int samplePosition, int blockSamples, juce::MidiBuffer& output);
    bool releaseOtherActiveChordsOnChannel (int channel, int keepInputNote, int samplePosition, int blockSamples, juce::MidiBuffer& output);
    std::vector<int> applyFastLeadSafety (const std::vector<int>& notes, int inputNote, const Soli::Settings& settings, bool fastLead) const;
    int scaleVelocityForVoicing (int velocity, int noteCount, const Soli::Settings& settings, bool fastLead) const;
    void transitionLeadChordOnChannel (int channel, int inputNote, int velocity, const std::vector<int>& newNotes, int samplePosition, int blockSamples, juce::MidiBuffer& output, const Soli::Settings& settings);
    void replaceActiveChord (int channel, int inputNote, int velocity, const std::vector<int>& newNotes, int samplePosition, int blockSamples, juce::MidiBuffer& output, const Soli::Settings& settings);
    void sendGeneratedNoteOn (int channel, int note, int velocity, int samplePosition, int blockSamples, juce::MidiBuffer& output);
    void sendGeneratedNoteOff (int channel, int note, int samplePosition, int blockSamples, juce::MidiBuffer& output);
    Transport readTransport() const;
    Soli::GeneratedChord generateChord (int inputNote, int velocity, const Soli::Settings& settings, double ppq);
    void startPerformance (int channel, int inputNote, int velocity, const std::vector<int>& notes, double ppq);
    void stopPerformance (int channel, int inputNote, int samplePosition, juce::MidiBuffer& output);
    void renderPerformance (const Transport& transport,
                            int rangeStartSample,
                            int rangeEndSample,
                            int blockSamples,
                            juce::MidiBuffer& output);
    std::vector<int> performanceNotes (const PerformanceChannel& state,
                                       int style,
                                       int subStyle,
                                       int step,
                                       double eventPpq,
                                       int beatsPerBar,
                                       float sophistication) const;
    void clearPerformance (juce::MidiBuffer* output = nullptr, int samplePosition = 0);
    void recordOutputMidi (const juce::MidiBuffer& output, const Transport& transport, int blockSamples);
    void closeRecordedNotesLocked (double closePpq) const;

    juce::AudioProcessorValueTreeState parameters;
    Soli::ChordEngine engine;
    Soli::ChordizerLink chordizerLink;
    std::array<ActiveChord, 16 * 128> activeChords;
    std::array<int, 16 * 128> generatedNoteRefs {};
    std::array<juce::int64, 16> lastLeadNoteSample {};
    juce::int64 processedSamples = 0;
    std::vector<PendingMidi> pendingMidi;
    std::array<PerformanceChannel, 16> performanceChannels;
    Soli::ChordizerContext lastChordizerContext;
    double lastTransportPpq = -1.0;
    bool lastTransportPlaying = false;
    std::mt19937 performanceRandom { std::random_device{}() };
    mutable std::mutex nameMutex;
    juce::String lastChordName = "--";
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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SoliVoicerAudioProcessor)
};
