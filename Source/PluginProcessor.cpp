#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <algorithm>
#include <cmath>
#include <random>

namespace
{
juce::StringArray withIndexOneIds (const juce::StringArray& names)
{
    juce::StringArray ids;
    for (auto name : names)
        ids.add (name);
    return ids;
}
}

SoliVoicerAudioProcessor::SoliVoicerAudioProcessor()
    : AudioProcessor (BusesProperties()),
      parameters (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    lastLeadNoteSample.fill (-1);
}

void SoliVoicerAudioProcessor::prepareToPlay (double, int)
{
    processedSamples = 0;
    lastLeadNoteSample.fill (-1);
    pendingMidi.clear();
}

bool SoliVoicerAudioProcessor::isBusesLayoutSupported (const BusesLayout&) const
{
    return true;
}

int SoliVoicerAudioProcessor::activeIndex (int channel, int note) noexcept
{
    return juce::jlimit (0, 15, channel - 1) * 128 + juce::jlimit (0, 127, note);
}

int SoliVoicerAudioProcessor::refIndex (int channel, int note) noexcept
{
    return activeIndex (channel, note);
}

Soli::Settings SoliVoicerAudioProcessor::readSettings() const
{
    Soli::Settings settings;
    settings.keyMask = static_cast<int> (*parameters.getRawParameterValue (ParameterIDs::keyMask));
    settings.scaleMask = static_cast<int> (*parameters.getRawParameterValue (ParameterIDs::scaleMask));
    settings.role = static_cast<Soli::NoteRole> (static_cast<int> (*parameters.getRawParameterValue (ParameterIDs::role)));
    settings.style = static_cast<Soli::Style> (static_cast<int> (*parameters.getRawParameterValue (ParameterIDs::style)));
    settings.playability = static_cast<Soli::Playability> (static_cast<int> (*parameters.getRawParameterValue (ParameterIDs::playability)));
    settings.strumMode = static_cast<Soli::StrumMode> (static_cast<int> (*parameters.getRawParameterValue (ParameterIDs::strumMode)));
    settings.chordSize = static_cast<int> (*parameters.getRawParameterValue (ParameterIDs::chordSize));
    settings.complexity = *parameters.getRawParameterValue (ParameterIDs::complexity);
    settings.voiceLeading = *parameters.getRawParameterValue (ParameterIDs::voiceLeading);
    settings.outside = *parameters.getRawParameterValue (ParameterIDs::outside);
    settings.variation = *parameters.getRawParameterValue (ParameterIDs::variation);
    settings.repeatChance = *parameters.getRawParameterValue (ParameterIDs::repeatChance);
    settings.strumSpeed = *parameters.getRawParameterValue (ParameterIDs::strumSpeed);
    settings.minNote = static_cast<int> (*parameters.getRawParameterValue (ParameterIDs::minNote));
    settings.maxNote = static_cast<int> (*parameters.getRawParameterValue (ParameterIDs::maxNote));
    return settings;
}

void SoliVoicerAudioProcessor::emitPendingMidi (int blockSamples, juce::MidiBuffer& output)
{
    std::vector<PendingMidi> remaining;
    remaining.reserve (pendingMidi.size());

    for (auto& event : pendingMidi)
    {
        if (event.samplesUntil < blockSamples)
        {
            output.addEvent (event.message, juce::jlimit (0, blockSamples - 1, event.samplesUntil));
        }
        else
        {
            event.samplesUntil -= blockSamples;
            remaining.push_back (event);
        }
    }

    pendingMidi = std::move (remaining);
}

void SoliVoicerAudioProcessor::scheduleMidiEvent (const juce::MidiMessage& message, int sampleOffset, int blockSamples, juce::MidiBuffer& output)
{
    if (sampleOffset < blockSamples)
        output.addEvent (message, juce::jlimit (0, blockSamples - 1, sampleOffset));
    else
        pendingMidi.push_back ({ message, sampleOffset });
}

void SoliVoicerAudioProcessor::releaseActiveChord (int channel, int inputNote, int samplePosition, int blockSamples, juce::MidiBuffer& output)
{
    auto& active = activeChords[static_cast<size_t> (activeIndex (channel, inputNote))];
    for (const auto note : active.notes)
        sendGeneratedNoteOff (active.channel, note, samplePosition, blockSamples, output);
    active.notes.clear();
}

bool SoliVoicerAudioProcessor::releaseOtherActiveChordsOnChannel (int channel, int keepInputNote, int samplePosition, int blockSamples, juce::MidiBuffer& output)
{
    auto releasedAny = false;

    for (int note = 0; note < 128; ++note)
    {
        if (note == keepInputNote)
            continue;

        auto& active = activeChords[static_cast<size_t> (activeIndex (channel, note))];
        if (active.notes.empty())
            continue;

        for (const auto generatedNote : active.notes)
            sendGeneratedNoteOff (active.channel, generatedNote, samplePosition, blockSamples, output);

        active.notes.clear();
        releasedAny = true;
    }

    return releasedAny;
}

std::vector<int> SoliVoicerAudioProcessor::applyFastLeadSafety (const std::vector<int>& notes,
                                                                int inputNote,
                                                                const Soli::Settings& settings,
                                                                bool fastLead) const
{
    const auto denseWideVoicing = settings.playability == Soli::Playability::orchestra
                               || settings.playability == Soli::Playability::unrestricted
                               || settings.chordSize > 6;

    if (! fastLead || ! denseWideVoicing || notes.size() <= 6)
        return notes;

    auto candidates = notes;
    std::sort (candidates.begin(), candidates.end());
    candidates.erase (std::unique (candidates.begin(), candidates.end()), candidates.end());

    const auto minFastNote = juce::jmax (40, settings.minNote);
    std::vector<int> playable;
    std::copy_if (candidates.begin(), candidates.end(), std::back_inserter (playable), [minFastNote] (int note)
    {
        return note >= minFastNote;
    });

    if (playable.size() < 4)
        playable = candidates;

    const auto originalTop = *std::max_element (candidates.begin(), candidates.end());
    std::sort (playable.begin(), playable.end(), [inputNote, originalTop] (int a, int b)
    {
        const auto aIsLead = a == inputNote || a == originalTop;
        const auto bIsLead = b == inputNote || b == originalTop;
        if (aIsLead != bIsLead)
            return aIsLead;

        const auto aDistance = std::abs (a - inputNote);
        const auto bDistance = std::abs (b - inputNote);
        if (aDistance != bDistance)
            return aDistance < bDistance;

        return a > b;
    });

    constexpr auto maxFastVoices = 6;
    if (playable.size() > static_cast<size_t> (maxFastVoices))
        playable.resize (static_cast<size_t> (maxFastVoices));

    std::sort (playable.begin(), playable.end());
    return playable;
}

int SoliVoicerAudioProcessor::scaleVelocityForVoicing (int velocity, int noteCount, const Soli::Settings& settings, bool fastLead) const
{
    const auto denseWideVoicing = settings.playability == Soli::Playability::orchestra
                               || settings.playability == Soli::Playability::unrestricted
                               || settings.chordSize > 6;

    if (! denseWideVoicing || noteCount <= 4)
        return juce::jlimit (1, 127, velocity);

    const auto voicePower = std::sqrt (4.0 / static_cast<double> (juce::jmax (4, noteCount)));
    const auto complexityTrim = juce::jmap (juce::jlimit (0.0f, 1.0f, settings.complexity),
                                           1.0f,
                                           0.62f);
    const auto outsideTrim = juce::jmap (juce::jlimit (0.0f, 1.0f, settings.outside),
                                        1.0f,
                                        0.88f);
    const auto fastTrim = fastLead ? 0.88 : 1.0;
    const auto scaled = static_cast<double> (velocity) * voicePower * complexityTrim * outsideTrim * fastTrim;
    return juce::jlimit (1, 127, static_cast<int> (std::round (scaled)));
}

void SoliVoicerAudioProcessor::transitionLeadChordOnChannel (int channel,
                                                             int inputNote,
                                                             int velocity,
                                                             const std::vector<int>& newNotes,
                                                             int samplePosition,
                                                             int blockSamples,
                                                             juce::MidiBuffer& output,
                                                             const Soli::Settings& settings)
{
    std::vector<int> soundingNotes;
    for (int note = 0; note < 128; ++note)
    {
        const auto& active = activeChords[static_cast<size_t> (activeIndex (channel, note))];
        if (active.notes.empty())
            continue;

        soundingNotes.insert (soundingNotes.end(), active.notes.begin(), active.notes.end());
    }

    std::sort (soundingNotes.begin(), soundingNotes.end());
    soundingNotes.erase (std::unique (soundingNotes.begin(), soundingNotes.end()), soundingNotes.end());

    auto sortedNewNotes = newNotes;
    std::sort (sortedNewNotes.begin(), sortedNewNotes.end());
    sortedNewNotes.erase (std::unique (sortedNewNotes.begin(), sortedNewNotes.end()), sortedNewNotes.end());

    std::vector<int> notesToKeep;
    std::set_intersection (soundingNotes.begin(), soundingNotes.end(),
                           sortedNewNotes.begin(), sortedNewNotes.end(),
                           std::back_inserter (notesToKeep));

    std::vector<int> notesToStop;
    std::set_difference (soundingNotes.begin(), soundingNotes.end(),
                         sortedNewNotes.begin(), sortedNewNotes.end(),
                         std::back_inserter (notesToStop));

    std::vector<int> notesToStart;
    std::set_difference (sortedNewNotes.begin(), sortedNewNotes.end(),
                         soundingNotes.begin(), soundingNotes.end(),
                         std::back_inserter (notesToStart));

    for (auto& active : activeChords)
    {
        if (active.channel == channel)
            active.notes.clear();
    }

    auto& active = activeChords[static_cast<size_t> (activeIndex (channel, inputNote))];
    active.channel = channel;
    active.notes = sortedNewNotes;

    for (const auto note : notesToKeep)
        generatedNoteRefs[static_cast<size_t> (refIndex (channel, note))] = 1;

    for (const auto note : notesToStop)
    {
        auto& refs = generatedNoteRefs[static_cast<size_t> (refIndex (channel, note))];
        while (refs > 0)
            sendGeneratedNoteOff (channel, note, samplePosition, blockSamples, output);
    }

    auto orderedNotes = notesToStart;
    auto strumMode = settings.strumMode;
    if (strumMode == Soli::StrumMode::random)
    {
        static thread_local std::mt19937 rng { std::random_device{}() };
        std::shuffle (orderedNotes.begin(), orderedNotes.end(), rng);
    }
    else if (strumMode == Soli::StrumMode::down)
    {
        std::reverse (orderedNotes.begin(), orderedNotes.end());
    }

    const auto denseComplexChord = settings.complexity > 0.65f && sortedNewNotes.size() > 5;
    const auto maxOffsetSamples = strumMode == Soli::StrumMode::together
                                ? (denseComplexChord ? juce::jmax (1, static_cast<int> (0.0025 * getSampleRate())) : 0)
                                : juce::jmax (0, static_cast<int> (settings.strumSpeed * 0.45f * getSampleRate()));
    const auto step = orderedNotes.size() <= 1 ? 0 : maxOffsetSamples / static_cast<int> (orderedNotes.size() - 1);
    const auto startOffset = notesToStop.empty() ? 0 : juce::jmin (blockSamples > 0 ? blockSamples - 1 : 0,
                                                                   juce::jmax (8, static_cast<int> (0.0015 * getSampleRate())));

    for (int i = 0; i < static_cast<int> (orderedNotes.size()); ++i)
        sendGeneratedNoteOn (channel, orderedNotes[static_cast<size_t> (i)], velocity, samplePosition + startOffset + i * step, blockSamples, output);
}

void SoliVoicerAudioProcessor::replaceActiveChord (int channel,
                                                   int inputNote,
                                                   int velocity,
                                                   const std::vector<int>& newNotes,
                                                   int samplePosition,
                                                   int blockSamples,
                                                   juce::MidiBuffer& output,
                                                   const Soli::Settings& settings)
{
    auto& active = activeChords[static_cast<size_t> (activeIndex (channel, inputNote))];
    auto oldNotes = active.notes;
    auto notesToStart = newNotes;
    auto notesToStop = oldNotes;

    std::sort (oldNotes.begin(), oldNotes.end());
    std::sort (notesToStart.begin(), notesToStart.end());
    std::sort (notesToStop.begin(), notesToStop.end());

    std::vector<int> common;
    std::set_intersection (oldNotes.begin(), oldNotes.end(),
                           notesToStart.begin(), notesToStart.end(),
                           std::back_inserter (common));

    for (const auto note : common)
    {
        notesToStart.erase (std::remove (notesToStart.begin(), notesToStart.end(), note), notesToStart.end());
        notesToStop.erase (std::remove (notesToStop.begin(), notesToStop.end(), note), notesToStop.end());
    }

    for (const auto note : notesToStop)
        sendGeneratedNoteOff (active.channel, note, samplePosition, blockSamples, output);

    active.channel = channel;
    active.notes = newNotes;

    auto orderedNotes = notesToStart;
    auto strumMode = settings.strumMode;
    if (strumMode == Soli::StrumMode::random)
    {
        static thread_local std::mt19937 rng { std::random_device{}() };
        std::shuffle (orderedNotes.begin(), orderedNotes.end(), rng);
    }
    else if (strumMode == Soli::StrumMode::down)
    {
        std::reverse (orderedNotes.begin(), orderedNotes.end());
    }

    const auto denseComplexChord = settings.complexity > 0.65f && newNotes.size() > 5;
    const auto maxOffsetSamples = strumMode == Soli::StrumMode::together
                                ? (denseComplexChord ? juce::jmax (1, static_cast<int> (0.0025 * getSampleRate())) : 0)
                                : juce::jmax (0, static_cast<int> (settings.strumSpeed * 0.45f * getSampleRate()));
    const auto step = orderedNotes.size() <= 1 ? 0 : maxOffsetSamples / static_cast<int> (orderedNotes.size() - 1);
    const auto startOffset = notesToStop.empty() ? 0 : juce::jmin (blockSamples > 0 ? blockSamples - 1 : 0,
                                                                   juce::jmax (8, static_cast<int> (0.003 * getSampleRate())));
    for (int i = 0; i < static_cast<int> (orderedNotes.size()); ++i)
        sendGeneratedNoteOn (channel, orderedNotes[static_cast<size_t> (i)], velocity, samplePosition + startOffset + i * step, blockSamples, output);
}

void SoliVoicerAudioProcessor::sendGeneratedNoteOn (int channel, int note, int velocity, int samplePosition, int blockSamples, juce::MidiBuffer& output)
{
    auto& refs = generatedNoteRefs[static_cast<size_t> (refIndex (channel, note))];
    if (refs == 0)
        scheduleMidiEvent (juce::MidiMessage::noteOn (channel, note, static_cast<juce::uint8> (velocity)), samplePosition, blockSamples, output);
    ++refs;
}

void SoliVoicerAudioProcessor::sendGeneratedNoteOff (int channel, int note, int samplePosition, int blockSamples, juce::MidiBuffer& output)
{
    auto& refs = generatedNoteRefs[static_cast<size_t> (refIndex (channel, note))];
    if (refs <= 0)
        return;

    --refs;
    if (refs == 0)
    {
        const auto before = pendingMidi.size();
        pendingMidi.erase (std::remove_if (pendingMidi.begin(), pendingMidi.end(), [channel, note] (const PendingMidi& event)
        {
            return event.message.isNoteOn() && event.message.getChannel() == channel && event.message.getNoteNumber() == note;
        }), pendingMidi.end());

        if (pendingMidi.size() == before)
            scheduleMidiEvent (juce::MidiMessage::noteOff (channel, note), samplePosition, blockSamples, output);
    }
}

void SoliVoicerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    buffer.clear();

    juce::MidiBuffer output;
    const auto blockSamples = buffer.getNumSamples();
    const auto blockStartSample = processedSamples;
    emitPendingMidi (blockSamples, output);
    const auto settings = readSettings();

    for (const auto metadata : midiMessages)
    {
        const auto message = metadata.getMessage();
        const auto samplePosition = metadata.samplePosition;

        if (message.isNoteOn())
        {
            const auto channel = message.getChannel();
            const auto inputNote = message.getNoteNumber();
            const auto velocity = message.getVelocity();
            const auto channelIndex = static_cast<size_t> (juce::jlimit (0, 15, channel - 1));
            const auto absoluteSample = blockStartSample + samplePosition;
            const auto previousLeadSample = lastLeadNoteSample[channelIndex];
            const auto fastLead = previousLeadSample >= 0
                               && getSampleRate() > 0.0
                               && absoluteSample - previousLeadSample <= static_cast<juce::int64> (0.14 * getSampleRate());
            lastLeadNoteSample[channelIndex] = absoluteSample;

            auto generated = engine.generate (inputNote, velocity, settings);
            const auto safeNotes = applyFastLeadSafety (generated.notes, inputNote, settings, fastLead);
            const auto safeVelocity = scaleVelocityForVoicing (velocity, static_cast<int> (generated.notes.size()), settings, fastLead);
            transitionLeadChordOnChannel (channel,
                                          inputNote,
                                          safeVelocity,
                                          safeNotes,
                                          samplePosition,
                                          blockSamples,
                                          output,
                                          settings);

            {
                const std::lock_guard<std::mutex> lock (nameMutex);
                lastChordName = "Chord: " + generated.name;
            }
        }
        else if (message.isNoteOff())
        {
            releaseActiveChord (message.getChannel(), message.getNoteNumber(), samplePosition, blockSamples, output);
        }
        else if (message.isAllNotesOff() || message.isAllSoundOff())
        {
            panic();
            output.addEvent (message, samplePosition);
        }
        else
        {
            output.addEvent (message, samplePosition);
        }
    }

    midiMessages.swapWith (output);
    processedSamples += blockSamples;
}

juce::AudioProcessorEditor* SoliVoicerAudioProcessor::createEditor()
{
    return new SoliVoicerAudioProcessorEditor (*this);
}

void SoliVoicerAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = parameters.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void SoliVoicerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (parameters.state.getType()))
            parameters.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::String SoliVoicerAudioProcessor::getLastChordName() const
{
    const std::lock_guard<std::mutex> lock (nameMutex);
    return lastChordName;
}

void SoliVoicerAudioProcessor::panic()
{
    for (auto& active : activeChords)
        active.notes.clear();
    generatedNoteRefs.fill (0);
    lastLeadNoteSample.fill (-1);
    pendingMidi.clear();
    engine.reset();

    const std::lock_guard<std::mutex> lock (nameMutex);
    lastChordName = "Chord: --";
}

juce::AudioProcessorValueTreeState::ParameterLayout SoliVoicerAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    auto addChoice = [&] (const char* id, const juce::String& name, const juce::StringArray& choices, int defaultIndex)
    {
        params.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { id, 1 }, name, withIndexOneIds (choices), defaultIndex));
    };

    addChoice (ParameterIDs::role, "Input Role", Soli::ChordEngine::roleNames(), 0);
    addChoice (ParameterIDs::style, "Style", Soli::ChordEngine::styleNames(), 0);
    addChoice (ParameterIDs::playability, "Playability", Soli::ChordEngine::playabilityNames(), 0);
    addChoice (ParameterIDs::strumMode, "Strum Mode", Soli::ChordEngine::strumModeNames(), 0);

    params.push_back (std::make_unique<juce::AudioParameterInt> (juce::ParameterID { ParameterIDs::keyMask, 1 }, "Keys", 1, (1 << 12) - 1, 1));
    params.push_back (std::make_unique<juce::AudioParameterInt> (juce::ParameterID { ParameterIDs::scaleMask, 1 }, "Scales", 1, (1 << Soli::ChordEngine::scaleNames().size()) - 1, 1));
    params.push_back (std::make_unique<juce::AudioParameterInt> (juce::ParameterID { ParameterIDs::chordSize, 1 }, "Chord Size", 2, 24, 4));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { ParameterIDs::complexity, 1 }, "Complexity", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.45f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { ParameterIDs::voiceLeading, 1 }, "Voice Leading", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.75f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { ParameterIDs::outside, 1 }, "Outside Harmony", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.10f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { ParameterIDs::variation, 1 }, "Variation", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.35f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { ParameterIDs::repeatChance, 1 }, "Repeat Chance", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.15f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { ParameterIDs::strumSpeed, 1 }, "Strum Speed", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterInt> (juce::ParameterID { ParameterIDs::minNote, 1 }, "Min Note", 0, 126, 36));
    params.push_back (std::make_unique<juce::AudioParameterInt> (juce::ParameterID { ParameterIDs::maxNote, 1 }, "Max Note", 1, 127, 96));

    return { params.begin(), params.end() };
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SoliVoicerAudioProcessor();
}
