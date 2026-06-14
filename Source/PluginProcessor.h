#pragma once

#include <JuceHeader.h>
#include "ChordEngine.h"

#include <array>
#include <mutex>
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
    void panic();

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

    juce::AudioProcessorValueTreeState parameters;
    Soli::ChordEngine engine;
    std::array<ActiveChord, 16 * 128> activeChords;
    std::array<int, 16 * 128> generatedNoteRefs {};
    std::array<juce::int64, 16> lastLeadNoteSample {};
    juce::int64 processedSamples = 0;
    std::vector<PendingMidi> pendingMidi;
    mutable std::mutex nameMutex;
    juce::String lastChordName = "Chord: --";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SoliVoicerAudioProcessor)
};
