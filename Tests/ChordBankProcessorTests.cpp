#include <JuceHeader.h>
#include "PluginProcessor.h"

#include <cmath>
#include <iostream>

namespace
{
int failures = 0;

void expect (bool condition, const char* message)
{
    if (! condition)
    {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

void selectChordBankMode (SoliVoicerAudioProcessor& processor)
{
    auto* parameter = processor.getValueTreeState().getParameter (ParameterIDs::sourceMode);
    expect (parameter != nullptr, "source mode parameter exists");
    if (parameter != nullptr)
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (1.0f));
}

void selectRole (SoliVoicerAudioProcessor& processor, Soli::NoteRole role)
{
    auto* parameter = processor.getValueTreeState().getParameter (ParameterIDs::role);
    expect (parameter != nullptr, "input role parameter exists");
    if (parameter != nullptr)
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (static_cast<float> (static_cast<int> (role))));
}

void process (SoliVoicerAudioProcessor& processor, juce::MidiBuffer& midi, int samples = 64)
{
    juce::AudioBuffer<float> audio (1, samples);
    processor.processBlock (audio, midi);
}

void processSilence (SoliVoicerAudioProcessor& processor, int blocks)
{
    for (int block = 0; block < blocks; ++block)
    {
        juce::MidiBuffer midi;
        process (processor, midi);
    }
}

void testAudibleRakedCapture()
{
    SoliVoicerAudioProcessor processor;
    processor.setRateAndBufferSizeDetails (1000.0, 64);
    processor.prepareToPlay (1000.0, 64);
    selectChordBankMode (processor);
    processor.setChordBankListening (true);

    juce::MidiBuffer first;
    first.addEvent (juce::MidiMessage::noteOn (1, 60, static_cast<juce::uint8> (100)), 0);
    process (processor, first);
    expect (first.getNumEvents() == 1, "Listen passes the first held rake note through unchanged");
    expect (processor.getChordBankCards().empty(), "A held first rake note does not prematurely create a card");

    juce::MidiBuffer remainder;
    remainder.addEvent (juce::MidiMessage::noteOn (1, 64, static_cast<juce::uint8> (96)), 8);
    remainder.addEvent (juce::MidiMessage::noteOn (1, 67, static_cast<juce::uint8> (92)), 16);
    remainder.addEvent (juce::MidiMessage::noteOff (1, 60), 24);
    remainder.addEvent (juce::MidiMessage::noteOff (1, 64), 32);
    remainder.addEvent (juce::MidiMessage::noteOff (1, 67), 40);
    process (processor, remainder);
    expect (remainder.getNumEvents() == 5, "Listen passes every later raked event through unchanged");
    expect (processor.getChordBankCards().empty(), "The chord waits through the post-release rake window");

    processSilence (processor, 4);
    const auto cards = processor.getChordBankCards();
    expect (cards.size() == 1, "The complete rake becomes one chord card");
    if (! cards.empty())
        expect (cards.front().name == "maj", "C E G is stored as the root-independent major quality");
}

void testLastHeldNoteControlsFinalization()
{
    SoliVoicerAudioProcessor processor;
    processor.setRateAndBufferSizeDetails (1000.0, 64);
    processor.prepareToPlay (1000.0, 64);
    selectChordBankMode (processor);

    juce::MidiBuffer ons;
    ons.addEvent (juce::MidiMessage::noteOn (1, 62, static_cast<juce::uint8> (100)), 0);
    ons.addEvent (juce::MidiMessage::noteOn (2, 65, static_cast<juce::uint8> (100)), 8);
    ons.addEvent (juce::MidiMessage::noteOn (3, 69, static_cast<juce::uint8> (100)), 16);
    process (processor, ons);

    juce::MidiBuffer partialRelease;
    partialRelease.addEvent (juce::MidiMessage::noteOff (1, 62), 0);
    partialRelease.addEvent (juce::MidiMessage::noteOff (2, 65), 8);
    process (processor, partialRelease);
    processSilence (processor, 5);
    expect (processor.getChordBankCards().empty(), "No card is created while the last chord note remains held");

    juce::MidiBuffer finalRelease;
    finalRelease.addEvent (juce::MidiMessage::noteOff (3, 69), 0);
    process (processor, finalRelease);
    processSilence (processor, 4);
    expect (processor.getChordBankCards().size() == 1, "The all-channel card is created after the final held note ends");
}

void testSeparateSingleNotesAreIgnored()
{
    SoliVoicerAudioProcessor processor;
    processor.setRateAndBufferSizeDetails (1000.0, 64);
    processor.prepareToPlay (1000.0, 64);
    selectChordBankMode (processor);

    for (const auto note : { 60, 64, 67, 71 })
    {
        juce::MidiBuffer single;
        single.addEvent (juce::MidiMessage::noteOn (1, note, static_cast<juce::uint8> (100)), 0);
        single.addEvent (juce::MidiMessage::noteOff (1, note), 12);
        process (processor, single);
        processSilence (processor, 1);
    }
    expect (processor.getChordBankCards().empty(), "Separate single notes never become false chord cards");
}

void addCapturedChord (SoliVoicerAudioProcessor& processor, std::initializer_list<int> notes)
{
    juce::MidiBuffer chord;
    auto offset = 0;
    for (const auto note : notes)
    {
        chord.addEvent (juce::MidiMessage::noteOn (1, note, static_cast<juce::uint8> (100)), offset);
        offset += 3;
    }
    for (const auto note : notes)
    {
        chord.addEvent (juce::MidiMessage::noteOff (1, note), offset);
        offset += 3;
    }
    process (processor, chord);
    processSilence (processor, 2);
}

std::vector<int> noteOns (const juce::MidiBuffer& midi)
{
    std::vector<int> notes;
    for (const auto metadata : midi)
        if (metadata.getMessage().isNoteOn())
            notes.push_back (metadata.getMessage().getNoteNumber());
    std::sort (notes.begin(), notes.end());
    notes.erase (std::unique (notes.begin(), notes.end()), notes.end());
    return notes;
}

void testRootIndependentQualityAndRoleAnchors()
{
    SoliVoicerAudioProcessor processor;
    processor.setRateAndBufferSizeDetails (1000.0, 64);
    processor.prepareToPlay (1000.0, 64);
    selectChordBankMode (processor);

    addCapturedChord (processor, { 52, 55, 59, 62, 66 }); // Em9
    auto cards = processor.getChordBankCards();
    expect (cards.size() == 1, "Em9 creates one quality card");
    if (! cards.empty())
    {
        expect (cards.front().name == "m9", "The card says m9 without the captured E root");
        expect (cards.front().intervals == std::vector<int> ({ 0, 3, 7, 10, 14 }),
                "m9 is retained as a compound root-independent formula");
    }

    addCapturedChord (processor, { 50, 53, 57, 60, 64 }); // Dm9
    expect (processor.getChordBankCards().size() == 1,
            "The same quality captured from another root reuses the m9 card");

    processor.setChordBankListening (false);
    selectRole (processor, Soli::NoteRole::root);
    juce::MidiBuffer rootTrigger;
    rootTrigger.addEvent (juce::MidiMessage::noteOn (1, 60, static_cast<juce::uint8> (100)), 0);
    process (processor, rootTrigger);
    expect (noteOns (rootTrigger) == std::vector<int> ({ 60, 63, 67, 70, 74 }),
            "Root role applies the learned m9 formula to the incoming C");
    expect (processor.getLastChordName() == "m9", "Perform readout remains a quality, not a named root chord");
    processor.panic();
    expect (processor.getLastChordName() == "m9", "Panic leaves the last observed chord readout latched");
    processor.startNewPhrase();
    processSilence (processor, 1);
    expect (processor.getLastChordName() == "m9", "New Phrase leaves the last observed chord readout latched");

    selectRole (processor, Soli::NoteRole::melodyTop);
    juce::MidiBuffer melodyTrigger;
    melodyTrigger.addEvent (juce::MidiMessage::noteOn (1, 72, static_cast<juce::uint8> (100)), 0);
    process (processor, melodyTrigger);
    const auto melodyNotes = noteOns (melodyTrigger);
    expect (! melodyNotes.empty() && melodyNotes.back() == 72,
            "Melody Top makes the incoming note the actual top of the transposed formula");
    expect (melodyNotes == std::vector<int> ({ 58, 61, 65, 68, 72 }),
            "Melody Top transposes the full m9 formula beneath the incoming note");

    selectRole (processor, Soli::NoteRole::guideTone);
    juce::MidiBuffer guideTrigger;
    guideTrigger.addEvent (juce::MidiMessage::noteOn (1, 60, static_cast<juce::uint8> (100)), 0);
    process (processor, guideTrigger);
    expect (noteOns (guideTrigger) == std::vector<int> ({ 50, 53, 57, 60, 64 }),
            "Guide Tone treats the incoming C as the m9 formula's seventh");
}

void testProbabilityPersistenceAndDeletion()
{
    SoliVoicerAudioProcessor processor;
    processor.setRateAndBufferSizeDetails (1000.0, 64);
    processor.prepareToPlay (1000.0, 64);
    selectChordBankMode (processor);

    juce::MidiBuffer chord;
    chord.addEvent (juce::MidiMessage::noteOn (1, 60, static_cast<juce::uint8> (100)), 0);
    chord.addEvent (juce::MidiMessage::noteOn (1, 64, static_cast<juce::uint8> (100)), 4);
    chord.addEvent (juce::MidiMessage::noteOn (1, 67, static_cast<juce::uint8> (100)), 8);
    chord.addEvent (juce::MidiMessage::noteOff (1, 60), 16);
    chord.addEvent (juce::MidiMessage::noteOff (1, 64), 20);
    chord.addEvent (juce::MidiMessage::noteOff (1, 67), 24);
    process (processor, chord);
    processSilence (processor, 4);
    expect (processor.getChordBankCards().size() == 1, "Test card is captured");

    juce::MidiBuffer secondChord;
    secondChord.addEvent (juce::MidiMessage::noteOn (1, 62, static_cast<juce::uint8> (100)), 0);
    secondChord.addEvent (juce::MidiMessage::noteOn (1, 65, static_cast<juce::uint8> (100)), 4);
    secondChord.addEvent (juce::MidiMessage::noteOn (1, 69, static_cast<juce::uint8> (100)), 8);
    secondChord.addEvent (juce::MidiMessage::noteOn (1, 72, static_cast<juce::uint8> (100)), 12);
    secondChord.addEvent (juce::MidiMessage::noteOff (1, 62), 20);
    secondChord.addEvent (juce::MidiMessage::noteOff (1, 65), 24);
    secondChord.addEvent (juce::MidiMessage::noteOff (1, 69), 28);
    secondChord.addEvent (juce::MidiMessage::noteOff (1, 72), 32);
    process (processor, secondChord);
    processSilence (processor, 4);
    expect (processor.getChordBankCards().size() == 2, "A second independent chord card is captured");

    processor.setChordBankCardProbability (0, 0.0f);
    processor.setChordBankCardProbability (1, 1.0f);
    processor.setChordBankListening (false);
    juce::MidiBuffer trigger;
    trigger.addEvent (juce::MidiMessage::noteOn (1, 60, static_cast<juce::uint8> (100)), 0);
    process (processor, trigger);
    expect (processor.getLastChordName() == "m7", "A zero-weight quality card is excluded from Perform choices");

    processor.setChordBankCardProbability (0, 0.27f);
    auto cards = processor.getChordBankCards();
    expect (! cards.empty() && std::abs (cards.front().probability - 0.27f) < 0.001f,
            "Card probability follows its dial value");

    juce::MemoryBlock state;
    processor.getStateInformation (state);
    SoliVoicerAudioProcessor restored;
    restored.setStateInformation (state.getData(), static_cast<int> (state.getSize()));
    cards = restored.getChordBankCards();
    expect (! cards.empty() && std::abs (cards.front().probability - 0.27f) < 0.001f,
            "Card probability survives project state save and restore");
    expect (! cards.empty() && cards.front().name == "maj",
            "Saved cards restore as root-independent quality labels");

    restored.removeChordBankCard (0);
    cards = restored.getChordBankCards();
    expect (cards.size() == 1 && cards.front().name == "m7", "The selected card can be deleted independently");
}

void testExactInputLocksAndPersistence()
{
    SoliVoicerAudioProcessor processor;
    processor.setRateAndBufferSizeDetails (1000.0, 64);
    processor.prepareToPlay (1000.0, 64);

    juce::MidiBuffer firstTrigger;
    firstTrigger.addEvent (juce::MidiMessage::noteOn (1, 60, static_cast<juce::uint8> (100)), 0);
    process (processor, firstTrigger);
    const auto originalNotes = noteOns (firstTrigger);
    expect (originalNotes.size() >= 3, "A generated harmony is available to lock");
    expect (processor.canLockLastChord(), "The most recently generated harmony can be locked");
    processor.lockLastChord();

    const auto locks = processor.getLockedChords();
    expect (locks.size() == 1 && locks.front().inputNote == 60,
            "Lock Last Chord binds the exact input MIDI note and register");

    juce::MidiBuffer release;
    release.addEvent (juce::MidiMessage::noteOff (1, 60), 0);
    process (processor, release);
    if (auto* style = processor.getValueTreeState().getParameter (ParameterIDs::style))
        style->setValueNotifyingHost (style->convertTo0to1 (static_cast<float> (static_cast<int> (Soli::Style::modernOutside))));
    if (auto* role = processor.getValueTreeState().getParameter (ParameterIDs::role))
        role->setValueNotifyingHost (role->convertTo0to1 (static_cast<float> (static_cast<int> (Soli::NoteRole::bass))));

    juce::MidiBuffer lockedTrigger;
    lockedTrigger.addEvent (juce::MidiMessage::noteOn (1, 60, static_cast<juce::uint8> (100)), 0);
    process (processor, lockedTrigger);
    expect (noteOns (lockedTrigger) == originalNotes,
            "A lock recalls the exact stored chord and register after the generation controls change");

    juce::MemoryBlock state;
    processor.getStateInformation (state);
    SoliVoicerAudioProcessor restored;
    restored.setStateInformation (state.getData(), static_cast<int> (state.getSize()));
    expect (restored.getLockedChords().size() == 1 && restored.getLockedChords().front().notes == originalNotes,
            "Input locks survive project state save and restore");
    restored.removeLockedChord (60);
    expect (restored.getLockedChords().empty(), "A selected exact-note lock can be removed independently");
}
}

int main()
{
    juce::ScopedJuceInitialiser_GUI juce;
    testAudibleRakedCapture();
    testLastHeldNoteControlsFinalization();
    testSeparateSingleNotesAreIgnored();
    testRootIndependentQualityAndRoleAnchors();
    testProbabilityPersistenceAndDeletion();
    testExactInputLocksAndPersistence();
    if (failures == 0)
    {
        std::cout << "Chord Bank processor tests passed\n";
        return 0;
    }
    std::cerr << failures << " Chord Bank processor test(s) failed\n";
    return 1;
}
