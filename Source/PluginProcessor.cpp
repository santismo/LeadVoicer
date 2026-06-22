#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <algorithm>
#include <cmath>
#include <functional>
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
    performanceChannels = {};
    lastTransportPpq = -1.0;
    lastTransportPlaying = false;
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

bool SoliVoicerAudioProcessor::multiChannelOutEnabled() const noexcept
{
    return parameters.getRawParameterValue (ParameterIDs::multiChannelOut)->load() > 0.5f;
}

std::vector<int> SoliVoicerAudioProcessor::outputChannelsForNotes (const std::vector<int>& notes,
                                                                   int fallbackChannel) const
{
    const auto safeFallback = juce::jlimit (1, 16, fallbackChannel);
    std::vector<int> channels (notes.size(), safeFallback);
    if (! multiChannelOutEnabled())
        return channels;

    auto ranked = notes;
    std::sort (ranked.begin(), ranked.end(), std::greater<int>());
    ranked.erase (std::unique (ranked.begin(), ranked.end()), ranked.end());

    for (std::size_t i = 0; i < notes.size(); ++i)
    {
        const auto rank = std::find (ranked.begin(), ranked.end(), notes[i]);
        if (rank != ranked.end())
            channels[i] = juce::jlimit (1, 16, static_cast<int> (std::distance (ranked.begin(), rank)) + 1);
    }

    return channels;
}

int SoliVoicerAudioProcessor::outputChannelForNote (const std::vector<int>& notes,
                                                    int note,
                                                    int fallbackChannel) const
{
    const auto channels = outputChannelsForNotes (notes, fallbackChannel);
    for (std::size_t i = 0; i < notes.size(); ++i)
        if (notes[i] == note)
            return channels[i];

    return juce::jlimit (1, 16, fallbackChannel);
}

int SoliVoicerAudioProcessor::activeOutputChannel (const ActiveChord& active,
                                                  std::size_t index) noexcept
{
    if (index < active.outputChannels.size())
        return juce::jlimit (1, 16, active.outputChannels[index]);

    return juce::jlimit (1, 16, active.channel);
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
    settings.contextMode = static_cast<Soli::ContextMode> (static_cast<int> (*parameters.getRawParameterValue (ParameterIDs::contextMode)));
    settings.substitutionDepth = *parameters.getRawParameterValue (ParameterIDs::substitutionDepth);
    settings.minNote = static_cast<int> (*parameters.getRawParameterValue (ParameterIDs::minNote));
    settings.maxNote = static_cast<int> (*parameters.getRawParameterValue (ParameterIDs::maxNote));
    return settings;
}

juce::StringArray SoliVoicerAudioProcessor::sourceModeNames()
{
    return { "Manual Harmony", "Follow Chordizer" };
}

juce::StringArray SoliVoicerAudioProcessor::outputModeNames()
{
    return { "Held Voicing", "Performance" };
}

juce::StringArray SoliVoicerAudioProcessor::performanceStyleNames()
{
    return { "Contrapuntal Arpeggio", "Classical Broken Chords", "Chamber Waltz", "Counterline",
             "Guide Tone Comping", "Modern Chord Comping", "Walking Chord Bass", "Bossa Ensemble" };
}

juce::StringArray SoliVoicerAudioProcessor::performanceSubStyleNames (int styleIndex)
{
    switch (juce::jlimit (0, 7, styleIndex))
    {
        case 0: return { "Invention Arc", "Pedal Architecture", "Rising Sequence", "Turn Figures", "Echo Answer", "Continuous Lace" };
        case 1: return { "Low High Classic", "Octave Resonance", "Left Hand Roll", "Broken Inner Voices", "Answered Bass", "Chamber Ostinato" };
        case 2: return { "Vienna Bass Chord", "Soft Oom Pah", "Lifted Third Beat", "Arpeggiated Waltz", "Pedal Waltz", "Chamber Waltz" };
        case 3: return { "Two Voice Imitation", "Contrary Motion", "Suspension Chain", "Stepwise Tenor", "Canon Replies", "Inner Weave" };
        case 4: return { "Rootless Guide", "Drop Two Answers", "Charleston Shell", "Sparse Freddie", "Upper Structure", "Late Night" };
        case 5: return { "Charleston Push", "Block Answer", "Bill Evans Answer", "Backbeat Stabs", "Syncopated Clusters", "Laid Back Pocket" };
        case 6: return { "Quarter Walk", "Approach Notes", "Drop Step Bass", "Guide Tone Walk", "Chromatic Suggestion", "Stride Hints" };
        case 7: return { "Clave Soft", "Anticipated Push", "Guitar Pluck", "Low High Bossa", "Syncopated Pads", "Jobim Drift" };
        default: break;
    }
    return { "Main", "Variation 2", "Variation 3", "Variation 4", "Variation 5", "Variation 6" };
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
    for (std::size_t i = 0; i < active.notes.size(); ++i)
        sendGeneratedNoteOff (activeOutputChannel (active, i), active.notes[i], samplePosition, blockSamples, output);
    active.notes.clear();
    active.outputChannels.clear();
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

        for (std::size_t i = 0; i < active.notes.size(); ++i)
            sendGeneratedNoteOff (activeOutputChannel (active, i), active.notes[i], samplePosition, blockSamples, output);

        active.notes.clear();
        active.outputChannels.clear();
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

    const auto highComplexInitialHit = settings.complexity > 0.68f && denseWideVoicing;
    if ((! fastLead && ! highComplexInitialHit) || ! denseWideVoicing || notes.size() <= 6)
        return notes;

    auto candidates = notes;
    std::sort (candidates.begin(), candidates.end());
    candidates.erase (std::unique (candidates.begin(), candidates.end()), candidates.end());

    const auto minimumGeneratedNote = highComplexInitialHit
                                    ? juce::jmax (48, inputNote - 24)
                                    : juce::jmax (40, settings.minNote);
    std::vector<int> playable;
    std::copy_if (candidates.begin(), candidates.end(), std::back_inserter (playable), [minimumGeneratedNote] (int note)
    {
        return note >= minimumGeneratedNote;
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
    juce::ignoreUnused (noteCount, settings, fastLead);
    return juce::jlimit (1, 127, velocity);
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
    struct RoutedNote
    {
        int channel = 1;
        int note = 0;
    };

    const auto routedLess = [] (const RoutedNote& a, const RoutedNote& b)
    {
        if (a.channel != b.channel)
            return a.channel < b.channel;
        return a.note < b.note;
    };

    std::vector<RoutedNote> soundingNotes;
    for (int note = 0; note < 128; ++note)
    {
        const auto& active = activeChords[static_cast<size_t> (activeIndex (channel, note))];
        if (active.notes.empty())
            continue;

        for (std::size_t i = 0; i < active.notes.size(); ++i)
            soundingNotes.push_back ({ activeOutputChannel (active, i), active.notes[i] });
    }

    std::sort (soundingNotes.begin(), soundingNotes.end(), routedLess);
    soundingNotes.erase (std::unique (soundingNotes.begin(), soundingNotes.end(), [] (const RoutedNote& a, const RoutedNote& b)
    {
        return a.channel == b.channel && a.note == b.note;
    }), soundingNotes.end());

    auto sortedNewNotes = newNotes;
    std::sort (sortedNewNotes.begin(), sortedNewNotes.end());
    sortedNewNotes.erase (std::unique (sortedNewNotes.begin(), sortedNewNotes.end()), sortedNewNotes.end());
    const auto newOutputChannels = outputChannelsForNotes (sortedNewNotes, channel);

    std::vector<RoutedNote> routedNewNotes;
    routedNewNotes.reserve (sortedNewNotes.size());
    for (std::size_t i = 0; i < sortedNewNotes.size(); ++i)
        routedNewNotes.push_back ({ newOutputChannels[i], sortedNewNotes[i] });
    std::sort (routedNewNotes.begin(), routedNewNotes.end(), routedLess);

    std::vector<RoutedNote> notesToKeep;
    std::set_intersection (soundingNotes.begin(), soundingNotes.end(),
                           routedNewNotes.begin(), routedNewNotes.end(),
                           std::back_inserter (notesToKeep),
                           routedLess);

    std::vector<RoutedNote> notesToStop;
    std::set_difference (soundingNotes.begin(), soundingNotes.end(),
                         routedNewNotes.begin(), routedNewNotes.end(),
                         std::back_inserter (notesToStop),
                         routedLess);

    std::vector<RoutedNote> notesToStart;
    std::set_difference (routedNewNotes.begin(), routedNewNotes.end(),
                         soundingNotes.begin(), soundingNotes.end(),
                         std::back_inserter (notesToStart),
                         routedLess);

    for (auto& active : activeChords)
    {
        if (active.channel == channel)
        {
            active.notes.clear();
            active.outputChannels.clear();
        }
    }

    auto& active = activeChords[static_cast<size_t> (activeIndex (channel, inputNote))];
    active.channel = channel;
    active.notes = sortedNewNotes;
    active.outputChannels = newOutputChannels;

    for (const auto note : notesToKeep)
        generatedNoteRefs[static_cast<size_t> (refIndex (note.channel, note.note))] = 1;

    for (const auto note : notesToStop)
    {
        auto& refs = generatedNoteRefs[static_cast<size_t> (refIndex (note.channel, note.note))];
        while (refs > 0)
            sendGeneratedNoteOff (note.channel, note.note, samplePosition, blockSamples, output);
    }

    auto orderedNotes = notesToStart;
    std::sort (orderedNotes.begin(), orderedNotes.end(), [] (const RoutedNote& a, const RoutedNote& b)
    {
        return a.note < b.note;
    });
    auto strumMode = settings.strumMode;
    if (strumMode == Soli::StrumMode::random)
    {
        static thread_local std::mt19937 rng { 0x5a17c0de };
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
    {
        const auto routed = orderedNotes[static_cast<size_t> (i)];
        sendGeneratedNoteOn (routed.channel, routed.note, velocity, samplePosition + startOffset + i * step, blockSamples, output);
    }
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

    auto sortedNewNotes = newNotes;
    std::sort (sortedNewNotes.begin(), sortedNewNotes.end());
    sortedNewNotes.erase (std::unique (sortedNewNotes.begin(), sortedNewNotes.end()), sortedNewNotes.end());
    const auto newOutputChannels = outputChannelsForNotes (sortedNewNotes, channel);

    struct RoutedNote
    {
        int channel = 1;
        int note = 0;
    };

    const auto routedLess = [] (const RoutedNote& a, const RoutedNote& b)
    {
        if (a.channel != b.channel)
            return a.channel < b.channel;
        return a.note < b.note;
    };

    std::vector<RoutedNote> oldRoutedNotes;
    oldRoutedNotes.reserve (active.notes.size());
    for (std::size_t i = 0; i < active.notes.size(); ++i)
        oldRoutedNotes.push_back ({ activeOutputChannel (active, i), active.notes[i] });
    std::sort (oldRoutedNotes.begin(), oldRoutedNotes.end(), routedLess);

    std::vector<RoutedNote> newRoutedNotes;
    newRoutedNotes.reserve (sortedNewNotes.size());
    for (std::size_t i = 0; i < sortedNewNotes.size(); ++i)
        newRoutedNotes.push_back ({ newOutputChannels[i], sortedNewNotes[i] });
    std::sort (newRoutedNotes.begin(), newRoutedNotes.end(), routedLess);

    std::vector<RoutedNote> notesToStart;
    std::set_difference (newRoutedNotes.begin(), newRoutedNotes.end(),
                         oldRoutedNotes.begin(), oldRoutedNotes.end(),
                         std::back_inserter (notesToStart),
                         routedLess);

    std::vector<RoutedNote> notesToStop;
    std::set_difference (oldRoutedNotes.begin(), oldRoutedNotes.end(),
                         newRoutedNotes.begin(), newRoutedNotes.end(),
                         std::back_inserter (notesToStop),
                         routedLess);

    for (const auto note : notesToStop)
        sendGeneratedNoteOff (note.channel, note.note, samplePosition, blockSamples, output);

    active.channel = channel;
    active.notes = sortedNewNotes;
    active.outputChannels = newOutputChannels;

    auto orderedNotes = notesToStart;
    std::sort (orderedNotes.begin(), orderedNotes.end(), [] (const RoutedNote& a, const RoutedNote& b)
    {
        return a.note < b.note;
    });
    auto strumMode = settings.strumMode;
    if (strumMode == Soli::StrumMode::random)
    {
        static thread_local std::mt19937 rng { 0x5a17c0de };
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
    {
        const auto routed = orderedNotes[static_cast<size_t> (i)];
        sendGeneratedNoteOn (routed.channel, routed.note, velocity, samplePosition + startOffset + i * step, blockSamples, output);
    }
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

SoliVoicerAudioProcessor::Transport SoliVoicerAudioProcessor::readTransport() const
{
    Transport result;
    if (auto* playHead = getPlayHead())
    {
        if (const auto position = playHead->getPosition())
        {
            result.valid = true;
            result.playing = position->getIsPlaying();
            if (const auto ppq = position->getPpqPosition())
                result.ppq = *ppq;
            if (const auto bpm = position->getBpm())
                result.bpm = *bpm;
            if (const auto signature = position->getTimeSignature())
            {
                result.numerator = signature->numerator;
                result.denominator = signature->denominator;
            }
        }
    }
    return result;
}

Soli::GeneratedChord SoliVoicerAudioProcessor::generateChord (int inputNote,
                                                              int velocity,
                                                              const Soli::Settings& settings,
                                                              double ppq)
{
    const auto followsChordizer = static_cast<int> (*parameters.getRawParameterValue (ParameterIDs::sourceMode)) == 1;
    if (! followsChordizer)
        return engine.generate (inputNote, velocity, settings);

    auto context = chordizerLink.contextAt (ppq, true);
    if (context.connected && context.current.isNotEmpty())
        lastChordizerContext = context;
    else if (lastChordizerContext.current.isNotEmpty())
        context = lastChordizerContext;

    if (context.current.isEmpty())
        return engine.generate (inputNote, velocity, settings);

    auto generated = engine.generateForContext (inputNote,
                                                context.current,
                                                context.previous,
                                                context.next,
                                                settings);
    return generated;
}

void SoliVoicerAudioProcessor::startPerformance (int channel,
                                                 int inputNote,
                                                 int velocity,
                                                 const std::vector<int>& notes,
                                                 double ppq)
{
    auto& state = performanceChannels[static_cast<std::size_t> (juce::jlimit (0, 15, channel - 1))];
    state.held = true;
    state.channel = channel;
    state.inputNote = inputNote;
    state.velocity = velocity;
    state.voicing = notes;
    std::sort (state.voicing.begin(), state.voicing.end());
    state.voicing.erase (std::unique (state.voicing.begin(), state.voicing.end()), state.voicing.end());
    state.step = 0;
    std::uniform_int_distribution<int> phraseDistribution (0, 17);
    std::uniform_int_distribution<int> articulationDistribution (0, 7);
    std::uniform_real_distribution<float> intensityDistribution (-0.12f, 0.14f);
    state.phraseVariant = phraseDistribution (performanceRandom);
    state.articulationVariant = articulationDistribution (performanceRandom);
    state.intensityBias = intensityDistribution (performanceRandom);
    state.nextStepPpq = ppq;
    state.contextName = lastChordizerContext.current;
}

void SoliVoicerAudioProcessor::stopPerformance (int channel,
                                                int inputNote,
                                                int samplePosition,
                                                juce::MidiBuffer& output)
{
    auto& state = performanceChannels[static_cast<std::size_t> (juce::jlimit (0, 15, channel - 1))];
    if (! state.held || state.inputNote != inputNote)
        return;

    const auto releasedVoicing = state.voicing;
    state = {};
    std::array<bool, 16> releasedChannels {};
    if (multiChannelOutEnabled())
    {
        for (const auto outputChannel : outputChannelsForNotes (releasedVoicing, channel))
            releasedChannels[static_cast<std::size_t> (juce::jlimit (1, 16, outputChannel) - 1)] = true;
    }
    else
    {
        releasedChannels[static_cast<std::size_t> (juce::jlimit (1, 16, channel) - 1)] = true;
    }

    pendingMidi.erase (std::remove_if (pendingMidi.begin(), pendingMidi.end(), [&releasedChannels] (const PendingMidi& event)
    {
        const auto eventChannel = event.message.getChannel();
        return eventChannel >= 1
            && eventChannel <= 16
            && releasedChannels[static_cast<std::size_t> (eventChannel - 1)];
    }), pendingMidi.end());
    if (multiChannelOutEnabled())
    {
        std::array<bool, 16> sent {};
        for (int outputChannel = 1; outputChannel <= 16; ++outputChannel)
        {
            const auto safeChannel = juce::jlimit (1, 16, outputChannel);
            if (! releasedChannels[static_cast<std::size_t> (safeChannel - 1)])
                continue;
            if (sent[static_cast<std::size_t> (safeChannel - 1)])
                continue;

            sent[static_cast<std::size_t> (safeChannel - 1)] = true;
            output.addEvent (juce::MidiMessage::allNotesOff (safeChannel), juce::jmax (0, samplePosition));
        }
    }
    else
    {
        output.addEvent (juce::MidiMessage::allNotesOff (channel), juce::jmax (0, samplePosition));
    }
}

std::vector<int> SoliVoicerAudioProcessor::performanceNotes (const PerformanceChannel& state,
                                                             int style,
                                                             int subStyle,
                                                             int step,
                                                             double eventPpq,
                                                             int beatsPerBar,
                                                             float sophistication) const
{
    if (state.voicing.empty())
        return {};

    auto notes = state.voicing;
    std::sort (notes.begin(), notes.end());
    const auto last = static_cast<int> (notes.size()) - 1;
    const auto at = [&] (int index) { return notes[static_cast<std::size_t> (juce::jlimit (0, last, index))]; };
    const auto chord = [&] (std::initializer_list<int> indices)
    {
        std::vector<int> selected;
        for (const auto index : indices)
            selected.push_back (at (index));
        std::sort (selected.begin(), selected.end());
        selected.erase (std::unique (selected.begin(), selected.end()), selected.end());
        return selected;
    };
    const auto upperChord = [&]
    {
        return chord ({ juce::jmax (0, last - 2), juce::jmax (0, last - 1), last });
    };
    const auto shellChord = [&]
    {
        return chord ({ 0, juce::jmin (2, last), last });
    };
    const auto guideChord = [&]
    {
        return chord ({ juce::jmin (1, last), juce::jmin (2, last), last });
    };
    const auto innerChord = [&]
    {
        return chord ({ juce::jmin (1, last), juce::jmin (2, last), juce::jmin (3, last) });
    };
    const auto bassGuideChord = [&]
    {
        return chord ({ 0, juce::jmin (1, last), juce::jmin (3, last) });
    };
    const auto phraseChord = [&]
    {
        return chord ({ 0, juce::jmin (2, last), juce::jmax (0, last - 1), last });
    };
    const auto barLength = juce::jmax (1, beatsPerBar);
    const auto barPpq = std::fmod (juce::jmax (0.0, eventPpq), static_cast<double> (barLength));
    const auto beatInBar = juce::jlimit (0, barLength - 1, static_cast<int> (std::floor (barPpq + 0.0001)));
    const auto eighthInBar = static_cast<int> (std::floor (barPpq * 2.0 + 0.0001)) % juce::jmax (1, barLength * 2);
    const auto sixteenthInBar = static_cast<int> (std::floor (barPpq * 4.0 + 0.0001)) % juce::jmax (1, barLength * 4);
    const auto phraseBars = style == 2 ? 8 : 4;
    const auto phraseLength = static_cast<double> (barLength * phraseBars);
    const auto phrasePpq = std::fmod (juce::jmax (0.0, eventPpq), phraseLength);
    const auto phraseBar = juce::jlimit (0, phraseBars - 1, static_cast<int> (std::floor (phrasePpq / barLength)));
    const auto phraseProgress = phrasePpq / juce::jmax (1.0, phraseLength);
    const auto cadence = phrasePpq >= phraseLength - juce::jmax (1.0, barLength * 0.55);
    const auto phraseStart = phrasePpq < 0.03125;
    const auto longBeat = eighthInBar % 2 == 0;
    const auto sub = ((subStyle % 6) + 6) % 6;
    const auto phraseIndex = static_cast<int> (std::floor (juce::jmax (0.0, eventPpq) / juce::jmax (1.0, phraseLength)));
    const auto motif = juce::jlimit (0, 31, (state.phraseVariant + phraseIndex * 5 + sub * 3 + style * 7) & 31);
    const auto breath = cadence || (phraseBar > 0 && beatInBar == 0 && (motif + phraseBar) % 5 == 0);
    const auto answerBeat = ((motif + phraseBar) % 3) + 1;
    const auto phase = step & 15;
    std::vector<int> result;

    switch (style)
    {
        case 0:
        {
            std::array<int, 16> pattern { 0, 2, 1, 3, 2, 4, 3, 1, 0, 3, 1, 4, 2, 5, 3, 1 };
            if (sub == 1) pattern = { 0, last, 1, last, 2, last, 1, last, 0, last, 2, last, 3, last, 1, last };
            else if (sub == 2) pattern = { 0, 1, 2, 3, 1, 2, 3, 4, 2, 3, 4, 5, 3, 4, 2, 1 };
            else if (sub == 3) pattern = { 0, 2, 1, 3, 2, 1, 3, 2, 1, 3, 2, 4, 3, 2, 1, 0 };
            else if (sub == 4) pattern = { 0, 2, 4, 2, last, 3, 1, 3, 0, 3, 5, 3, last, 4, 2, 1 };
            else if (sub == 5) pattern = { 0, 1, 2, 3, 4, 3, 2, 1, 1, 2, 3, 4, 5, 4, 2, 0 };

            if (breath && (phase == 5 || phase == 13 || phase == 14))
                break;
            const auto patternIndex = (phase + phraseBar * 3 + motif) % static_cast<int> (pattern.size());
            result = { at (pattern[static_cast<std::size_t> (patternIndex)]) };
            if ((phraseStart || (cadence && phase == 12)) && sophistication > 0.5f)
                result = bassGuideChord();
            else if (sophistication > 0.76f && (phase + motif) % 8 == 7)
                result.push_back (at (juce::jmax (0, last - (phraseBar & 1))));
            break;
        }
        case 1:
        {
            std::array<int, 8> pattern { 0, last, 1, juce::jmax (0, last - 1), 0, juce::jmin (2, last), 1, last };
            if (sub == 1) pattern = { 0, last, 0, juce::jmax (0, last - 1), 1, last, 0, juce::jmin (2, last) };
            else if (sub == 2) pattern = { 0, 2, 1, 3, 0, 3, 1, last };
            else if (sub == 3) pattern = { 0, 1, last, 2, 0, 2, last, 1 };
            else if (sub == 4) pattern = { 0, last, 2, last, 1, juce::jmax (0, last - 1), 2, last };
            else if (sub == 5) pattern = { 0, 1, 2, last, 1, 2, 3, juce::jmax (0, last - 1) };

            if ((breath && phase % 8 == 5) || (cadence && phase > 10 && phase < 15))
                break;
            const auto patternIndex = (phase + phraseBar + motif) % static_cast<int> (pattern.size());
            result = { at (pattern[static_cast<std::size_t> (patternIndex)]) };
            if (longBeat && phraseBar == phraseBars - 1 && sophistication > 0.68f)
                result = phraseChord();
            break;
        }
        case 2:
        {
            const auto waltzBeat = static_cast<int> (std::floor (std::fmod (juce::jmax (0.0, eventPpq), 3.0) + 0.0001));
            if ((! longBeat && sophistication < 0.72f) || (breath && waltzBeat == 1 && ! longBeat))
                break;
            if (cadence && phraseBar == phraseBars - 1 && waltzBeat == 2 && ! longBeat)
                break;
            if (waltzBeat == 0)
                result = { at (sub == 4 ? juce::jmin (1, last) : 0) };
            else if ((sub == 3 || motif % 4 == 0) && waltzBeat == 2 && sophistication > 0.56f)
                result = phraseChord();
            else
                result = sub == 5 ? innerChord() : upperChord();
            break;
        }
        case 3:
        {
            const auto span = juce::jmax (1, juce::jmin (4, last + 1));
            const auto lowLine = sub == 1 ? phase + phraseBar : 15 - phase + phraseBar;
            const auto highLine = sub == 1 ? 15 - phase : phase + phraseBar;
            if ((breath && phase % 4 == 1) || (cadence && phase % 4 == 3))
                break;
            if ((phase + motif) % 2 == 0 || sophistication > 0.52f)
                result = { at (lowLine % span), at (last - (highLine % span)) };
            if (sophistication > 0.66f && phase % 4 == 2)
                result.push_back (at (1 + ((phase + phraseBar) % juce::jmax (1, last))));
            break;
        }
        case 4:
        {
            std::array<int, 16> mask { 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0 };
            if (sub == 1) mask = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0 };
            else if (sub == 2) mask = { 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0 };
            else if (sub == 3) mask = { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0 };
            else if (sub == 4) mask = { 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0 };
            else if (sub == 5) mask = { 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0 };
            const auto index = (sixteenthInBar + phraseBar * 2 + motif) % 16;
            const auto active = mask[static_cast<std::size_t> (index)] != 0
                             || (sophistication > 0.82f && cadence && sixteenthInBar == 14);
            if (active)
                result = sub == 4 ? upperChord() : (sub == 5 ? innerChord() : ((motif + phraseBar) % 4 == 0 ? guideChord() : shellChord()));
            break;
        }
        case 5:
        {
            std::array<int, 16> mask { 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0 };
            if (sub == 1) mask = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0 };
            else if (sub == 2) mask = { 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0 };
            else if (sub == 3) mask = { 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0 };
            else if (sub == 4) mask = { 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0 };
            else if (sub == 5) mask = { 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0 };
            const auto active = mask[static_cast<std::size_t> ((sixteenthInBar + phraseBar + motif) % 16)] != 0
                             || (sophistication > 0.78f && beatInBar == answerBeat && sixteenthInBar % 4 == 2);
            if (active)
                result = sub == 1 ? phraseChord() : (sub == 4 ? upperChord() : ((motif % 5 == 0) ? innerChord() : guideChord()));
            break;
        }
        case 6:
        {
            const auto walkSpan = juce::jmax (1, juce::jmin (5, static_cast<int> (notes.size())));
            if ((! longBeat && sophistication < 0.74f) || (breath && ! longBeat))
                break;
            const auto walkStep = beatInBar + phraseBar * 2 + motif + (sub == 1 && ! longBeat ? 1 : 0);
            const auto walkIndex = sub == 2 ? juce::jmax (0, walkSpan - 1 - (walkStep % walkSpan)) : walkStep % walkSpan;
            result = { at (walkIndex) };
            if ((beatInBar == 1 || beatInBar == 3 || sub >= 3) && sophistication > 0.58f && longBeat)
            {
                auto guides = guideChord();
                result.insert (result.end(), guides.begin(), guides.end());
            }
            if (cadence && beatInBar >= barLength - 1 && sophistication > 0.62f)
                result = bassGuideChord();
            break;
        }
        case 7:
        {
            std::array<int, 16> lowMask { 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0 };
            std::array<int, 16> chordMask { 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0 };
            if (sub == 1) chordMask = { 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0 };
            else if (sub == 2) chordMask = { 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 0 };
            else if (sub == 4) chordMask = { 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0 };
            const auto index = (sixteenthInBar + (phraseBar % 2) * 2 + motif) % 16;
            if (lowMask[static_cast<std::size_t> (index)] != 0)
                result = { at (0) };
            else if (chordMask[static_cast<std::size_t> (index)] != 0)
                result = sub == 2 ? chord ({ 0, juce::jmin (2, last), last }) : guideChord();
            if (cadence && sixteenthInBar >= 12 && sophistication < 0.85f)
                result.clear();
            break;
        }
        default:
            result = notes;
            break;
    }

    if (sophistication > 0.86f && result.size() == 1 && notes.size() > 3 && style != 6
        && ! cadence && phraseProgress > 0.18 && phraseProgress < 0.82)
        result.push_back (at ((step + 2) % static_cast<int> (notes.size())));
    std::sort (result.begin(), result.end());
    result.erase (std::unique (result.begin(), result.end()), result.end());
    return result;
}

void SoliVoicerAudioProcessor::renderPerformance (const Transport& transport,
                                                  int rangeStartSample,
                                                  int rangeEndSample,
                                                  int blockSamples,
                                                  juce::MidiBuffer& output)
{
    if (! transport.valid || ! transport.playing || getSampleRate() <= 0.0
        || rangeEndSample <= rangeStartSample)
        return;

    const auto density = parameters.getRawParameterValue (ParameterIDs::rhythmDensity)->load();
    const auto sophistication = parameters.getRawParameterValue (ParameterIDs::performanceComplexity)->load();
    const auto syncopation = parameters.getRawParameterValue (ParameterIDs::syncopation)->load();
    const auto swing = parameters.getRawParameterValue (ParameterIDs::swing)->load();
    const auto humanize = parameters.getRawParameterValue (ParameterIDs::humanize)->load();
    const auto gate = parameters.getRawParameterValue (ParameterIDs::gate)->load();
    const auto style = static_cast<int> (*parameters.getRawParameterValue (ParameterIDs::performanceStyle));
    const auto subStyle = static_cast<int> (*parameters.getRawParameterValue (ParameterIDs::performanceSubStyle));
    const auto doubleTime = parameters.getRawParameterValue (ParameterIDs::doubleTime)->load() > 0.5f;
    auto stepPpq = density < 0.34f ? 1.0 : (density < 0.67f ? 0.5 : 0.25);
    if (doubleTime)
        stepPpq *= 0.5;
    const auto ppqPerSample = transport.bpm / (60.0 * getSampleRate());
    const auto rangeStartPpq = transport.ppq + static_cast<double> (rangeStartSample) * ppqPerSample;
    const auto rangeEndPpq = transport.ppq + static_cast<double> (rangeEndSample) * ppqPerSample;
    const auto settings = readSettings();
    const auto beatsPerBar = juce::jmax (1, transport.numerator);

    for (auto& state : performanceChannels)
    {
        if (! state.held)
            continue;

        if (state.nextStepPpq < rangeStartPpq - 0.125 || state.nextStepPpq > rangeEndPpq + 4.0)
        {
            state.nextStepPpq = std::floor (rangeStartPpq / stepPpq) * stepPpq;
            while (state.nextStepPpq < rangeStartPpq)
                state.nextStepPpq += stepPpq;
            state.step = static_cast<int> (std::floor (state.nextStepPpq / stepPpq));
        }

        auto guard = 0;
        while (state.nextStepPpq < rangeEndPpq && guard++ < 128)
        {
            if (static_cast<int> (*parameters.getRawParameterValue (ParameterIDs::sourceMode)) == 1)
            {
                const auto context = chordizerLink.contextAt (state.nextStepPpq, true);
                if (context.current.isNotEmpty() && context.current != state.contextName)
                {
                    state.voicing = engine.generateForContext (state.inputNote,
                                                               context.current,
                                                               context.previous,
                                                               context.next,
                                                               settings).notes;
                    state.contextName = context.current;
                }
            }

            auto eventPpq = state.nextStepPpq;
            if ((state.step & 1) != 0)
                eventPpq += stepPpq * (0.30 * swing + 0.12 * syncopation);
            else if (syncopation > 0.55f && (style == 5 || style == 7) && state.step % 4 == 2)
                eventPpq += stepPpq * 0.28 * syncopation;
            auto sampleOffset = static_cast<int> (std::round ((eventPpq - transport.ppq) / ppqPerSample));

            std::uniform_real_distribution<float> bipolar (-1.0f, 1.0f);
            sampleOffset += static_cast<int> (bipolar (performanceRandom) * humanize * 0.012f * getSampleRate());
            sampleOffset = juce::jlimit (rangeStartSample, juce::jmax (rangeStartSample, blockSamples - 1),
                                         sampleOffset);

            auto notes = performanceNotes (state, style, subStyle, state.step, eventPpq, beatsPerBar, sophistication);
            if (! notes.empty())
            {
                const auto barPpq = std::fmod (juce::jmax (0.0, eventPpq), static_cast<double> (beatsPerBar));
                const auto beatInBar = static_cast<int> (std::floor (barPpq + 0.0001));
                const auto phraseBars = style == 2 ? 8 : 4;
                const auto phraseLength = static_cast<double> (beatsPerBar * phraseBars);
                const auto phrasePpq = std::fmod (juce::jmax (0.0, eventPpq), phraseLength);
                const auto phraseStart = phrasePpq < 0.03125;
                const auto cadence = phrasePpq >= phraseLength - juce::jmax (1.0, beatsPerBar * 0.55);
                const auto phraseSwell = static_cast<int> (std::sin (juce::MathConstants<double>::pi
                                                                      * (phrasePpq / juce::jmax (1.0, phraseLength))) * 7.0);
                const auto downbeatAccent = beatInBar == 0 ? 7 : 0;
                const auto compAccent = (style == 5 || style == 7) && (state.step & 1) != 0 ? -5 : 0;
                const auto phraseAccent = phraseStart ? 5 : (cadence ? -4 : phraseSwell);
                const auto densityTrim = notes.size() > 3 ? -4 : 0;
                const auto takeBias = static_cast<int> (state.intensityBias * 28.0f);
                const auto velocity = juce::jlimit (1, 127,
                    state.velocity + static_cast<int> (bipolar (performanceRandom) * humanize * 18.0f)
                    + downbeatAccent + compAccent + phraseAccent + densityTrim + takeBias);
                const auto styleGate = (style == 5 || style == 7) ? (0.10f + gate * 0.48f)
                                      : (style == 0 || style == 1 || style == 6) ? (0.26f + gate * 0.66f)
                                      : (0.18f + gate * 0.76f);
                const auto takeGate = (state.articulationVariant % 3 == 0 ? -0.08f
                                     : state.articulationVariant % 3 == 1 ? 0.0f : 0.10f);
                const auto phraseGate = cadence && notes.size() > 1 ? juce::jmin (1.0f, styleGate + 0.22f + takeGate)
                                                                     : juce::jlimit (0.06f, 1.0f, styleGate + takeGate);
                const auto noteLength = static_cast<int> ((stepPpq * phraseGate) / ppqPerSample);
                for (const auto note : notes)
                {
                    const auto outputChannel = outputChannelForNote (state.voicing, note, state.channel);
                    scheduleMidiEvent (juce::MidiMessage::noteOn (outputChannel, note,
                                                                 static_cast<juce::uint8> (velocity)),
                                       sampleOffset, blockSamples, output);
                    scheduleMidiEvent (juce::MidiMessage::noteOff (outputChannel, note),
                                       sampleOffset + juce::jmax (1, noteLength), blockSamples, output);
                }
            }

            ++state.step;
            state.nextStepPpq += stepPpq;
        }
    }
}

void SoliVoicerAudioProcessor::clearPerformance (juce::MidiBuffer* output, int samplePosition)
{
    for (auto& state : performanceChannels)
    {
        if (output != nullptr && state.held)
        {
            if (multiChannelOutEnabled())
            {
                std::array<bool, 16> sent {};
                for (const auto outputChannel : outputChannelsForNotes (state.voicing, state.channel))
                {
                    const auto safeChannel = juce::jlimit (1, 16, outputChannel);
                    if (sent[static_cast<std::size_t> (safeChannel - 1)])
                        continue;

                    sent[static_cast<std::size_t> (safeChannel - 1)] = true;
                    output->addEvent (juce::MidiMessage::allNotesOff (safeChannel), juce::jmax (0, samplePosition));
                }
            }
            else
            {
                output->addEvent (juce::MidiMessage::allNotesOff (state.channel), juce::jmax (0, samplePosition));
            }
        }
        state = {};
    }
    pendingMidi.clear();
}

void SoliVoicerAudioProcessor::closeRecordedNotesLocked (double closePpq) const
{
    if (! midiRecordHasOrigin)
        return;

    closePpq = juce::jmax (midiRecordOriginPpq, closePpq);
    for (int channel = 1; channel <= 16; ++channel)
    {
        for (int note = 0; note < 128; ++note)
        {
            auto& refs = recordedNoteRefs[static_cast<std::size_t> (refIndex (channel, note))];
            if (refs <= 0)
                continue;

            auto off = juce::MidiMessage::noteOff (channel, note);
            off.setTimeStamp (closePpq);
            recordedMidiEvents.push_back ({ closePpq, off });
            refs = 0;
        }
    }
    midiRecordEndPpq = juce::jmax (midiRecordEndPpq, closePpq);
}

void SoliVoicerAudioProcessor::recordOutputMidi (const juce::MidiBuffer& output,
                                                 const Transport& transport,
                                                 int blockSamples)
{
    const std::lock_guard<std::mutex> lock (recordingMutex);
    if (! midiRecording || getSampleRate() <= 0.0)
        return;

    const auto bpm = transport.valid ? transport.bpm : midiRecordBpm;
    const auto ppqPerSample = bpm / (60.0 * getSampleRate());
    if (! midiRecordHasOrigin)
    {
        midiRecordOriginSample = processedSamples;
        midiRecordOriginPpq = transport.valid ? transport.ppq : 0.0;
        midiRecordEndPpq = midiRecordOriginPpq;
        midiRecordBpm = bpm;
        midiRecordNumerator = transport.valid ? transport.numerator : 4;
        midiRecordDenominator = transport.valid ? transport.denominator : 4;
        midiRecordHasOrigin = true;
    }

    const auto blockStartPpq = transport.valid
                             ? transport.ppq
                             : midiRecordOriginPpq
                                + static_cast<double> (processedSamples - midiRecordOriginSample) * ppqPerSample;
    midiRecordEndPpq = juce::jmax (midiRecordEndPpq,
                                   blockStartPpq + static_cast<double> (juce::jmax (0, blockSamples)) * ppqPerSample);

    for (const auto metadata : output)
    {
        auto message = metadata.getMessage();
        const auto ppq = blockStartPpq + static_cast<double> (metadata.samplePosition) * ppqPerSample;
        if (message.isNoteOn())
        {
            message.setTimeStamp (ppq);
            recordedMidiEvents.push_back ({ ppq, message });
            ++recordedNoteRefs[static_cast<std::size_t> (refIndex (message.getChannel(), message.getNoteNumber()))];
        }
        else if (message.isNoteOff())
        {
            message.setTimeStamp (ppq);
            recordedMidiEvents.push_back ({ ppq, message });
            auto& refs = recordedNoteRefs[static_cast<std::size_t> (refIndex (message.getChannel(), message.getNoteNumber()))];
            refs = juce::jmax (0, refs - 1);
        }
        else if (message.isAllNotesOff() || message.isAllSoundOff())
        {
            closeRecordedNotesLocked (ppq);
        }
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
    const auto transport = readTransport();
    const auto performanceMode = static_cast<int> (*parameters.getRawParameterValue (ParameterIDs::outputMode)) == 1;
    const auto ppqPerSample = transport.valid && getSampleRate() > 0.0
                            ? transport.bpm / (60.0 * getSampleRate()) : 0.0;
    auto renderedUntilSample = 0;

    if (transport.valid && transport.playing && lastTransportPlaying && lastTransportPpq >= 0.0)
    {
        const auto expected = lastTransportPpq + static_cast<double> (blockSamples) * ppqPerSample;
        if (std::abs (transport.ppq - expected) > 0.25)
        {
            pendingMidi.clear();
            for (auto& state : performanceChannels)
            {
                if (state.held)
                {
                    if (multiChannelOutEnabled())
                    {
                        std::array<bool, 16> sent {};
                        for (const auto outputChannel : outputChannelsForNotes (state.voicing, state.channel))
                        {
                            const auto safeChannel = juce::jlimit (1, 16, outputChannel);
                            if (sent[static_cast<std::size_t> (safeChannel - 1)])
                                continue;

                            sent[static_cast<std::size_t> (safeChannel - 1)] = true;
                            output.addEvent (juce::MidiMessage::allNotesOff (safeChannel), 0);
                        }
                    }
                    else
                    {
                        output.addEvent (juce::MidiMessage::allNotesOff (state.channel), 0);
                    }
                }
                state.nextStepPpq = -1.0;
            }
        }
    }

    if (! performanceMode)
    {
        const auto hadPerformance = std::any_of (performanceChannels.begin(), performanceChannels.end(),
                                                 [] (const auto& state) { return state.held; });
        if (hadPerformance)
            clearPerformance (&output, 0);
    }

    for (const auto metadata : midiMessages)
    {
        const auto message = metadata.getMessage();
        const auto samplePosition = metadata.samplePosition;
        if (performanceMode && samplePosition > renderedUntilSample)
            renderPerformance (transport, renderedUntilSample, samplePosition, blockSamples, output);
        renderedUntilSample = samplePosition;

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

            const auto eventPpq = transport.valid ? transport.ppq + samplePosition * ppqPerSample : 0.0;
            auto generated = generateChord (inputNote, velocity, settings, eventPpq);
            const auto safeNotes = applyFastLeadSafety (generated.notes, inputNote, settings, fastLead);
            const auto safeVelocity = scaleVelocityForVoicing (velocity, static_cast<int> (generated.notes.size()), settings, fastLead);
            if (performanceMode)
            {
                stopPerformance (channel,
                                 performanceChannels[channelIndex].inputNote,
                                 samplePosition,
                                 output);
                startPerformance (channel, inputNote, safeVelocity, safeNotes, eventPpq);
                if (! transport.playing)
                    transitionLeadChordOnChannel (channel, inputNote, safeVelocity, safeNotes,
                                                  samplePosition, blockSamples, output, settings);
            }
            else
            {
                transitionLeadChordOnChannel (channel,
                                              inputNote,
                                              safeVelocity,
                                              safeNotes,
                                              samplePosition,
                                              blockSamples,
                                              output,
                                              settings);
            }

            {
                const std::lock_guard<std::mutex> lock (nameMutex);
                if (static_cast<int> (*parameters.getRawParameterValue (ParameterIDs::sourceMode)) == 1
                    && lastChordizerContext.current.isNotEmpty())
                    lastChordName = generated.name + "  |  " + lastChordizerContext.current;
                else
                    lastChordName = generated.name;
            }
        }
        else if (message.isNoteOff())
        {
            if (performanceMode)
                stopPerformance (message.getChannel(), message.getNoteNumber(), samplePosition, output);
            releaseActiveChord (message.getChannel(), message.getNoteNumber(), samplePosition, blockSamples, output);
        }
        else if (message.isAllNotesOff() || message.isAllSoundOff())
        {
            clearPerformance();
            for (auto& active : activeChords)
            {
                active.notes.clear();
                active.outputChannels.clear();
            }
            generatedNoteRefs.fill (0);
            engine.reset();
            if (multiChannelOutEnabled())
            {
                for (int channel = 1; channel <= 16; ++channel)
                    output.addEvent (juce::MidiMessage::allNotesOff (channel), samplePosition);
            }
            else
            {
                output.addEvent (message, samplePosition);
            }
        }
        else
        {
            output.addEvent (message, samplePosition);
        }
    }

    if (performanceMode)
        renderPerformance (transport, renderedUntilSample, blockSamples, blockSamples, output);

    recordOutputMidi (output, transport, blockSamples);
    midiMessages.swapWith (output);
    processedSamples += blockSamples;
    if (transport.valid)
    {
        lastTransportPpq = transport.ppq;
        lastTransportPlaying = transport.playing;
    }
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

Soli::ChordizerSnapshot SoliVoicerAudioProcessor::getChordizerSnapshot() const
{
    return chordizerLink.snapshot (false);
}

void SoliVoicerAudioProcessor::setMidiRecordingEnabled (bool shouldRecord)
{
    const std::lock_guard<std::mutex> lock (recordingMutex);
    if (midiRecording == shouldRecord)
        return;

    if (shouldRecord)
    {
        recordedMidiEvents.clear();
        recordedMidiEvents.reserve (4096);
        recordedNoteRefs.fill (0);
        midiRecordHasOrigin = false;
        midiRecordOriginPpq = 0.0;
        midiRecordEndPpq = 0.0;
        midiRecordBpm = 120.0;
        midiRecordNumerator = 4;
        midiRecordDenominator = 4;
        midiRecordOriginSample = processedSamples;
        midiRecording = true;
    }
    else
    {
        closeRecordedNotesLocked (midiRecordEndPpq);
        midiRecording = false;
    }
}

bool SoliVoicerAudioProcessor::isMidiRecording() const
{
    const std::lock_guard<std::mutex> lock (recordingMutex);
    return midiRecording;
}

void SoliVoicerAudioProcessor::clearRecordedMidi()
{
    const std::lock_guard<std::mutex> lock (recordingMutex);
    recordedMidiEvents.clear();
    recordedNoteRefs.fill (0);
    midiRecordHasOrigin = false;
    midiRecordOriginPpq = 0.0;
    midiRecordEndPpq = 0.0;
    midiRecordOriginSample = processedSamples;
}

SoliVoicerAudioProcessor::RecordedMidiSnapshot SoliVoicerAudioProcessor::recordedMidiSnapshot() const
{
    const std::lock_guard<std::mutex> lock (recordingMutex);
    RecordedMidiSnapshot result;
    result.recording = midiRecording;
    result.hasOrigin = midiRecordHasOrigin;
    result.originPpq = midiRecordOriginPpq;
    result.endPpq = midiRecordEndPpq;
    result.bpm = midiRecordBpm;
    result.numerator = midiRecordNumerator;
    result.denominator = midiRecordDenominator;
    result.events = recordedMidiEvents;
    return result;
}

bool SoliVoicerAudioProcessor::writeRecordedMidiFile (const juce::File& destination) const
{
    constexpr int ticksPerQuarter = 960;
    std::vector<RecordedMidiEvent> events;
    double originPpq = 0.0;
    double endPpq = 0.0;
    double bpm = 120.0;
    int numerator = 4;
    int denominator = 4;

    {
        const std::lock_guard<std::mutex> lock (recordingMutex);
        if (! midiRecordHasOrigin || recordedMidiEvents.empty())
            return false;

        events = recordedMidiEvents;
        auto openNotes = recordedNoteRefs;
        originPpq = midiRecordOriginPpq;
        endPpq = midiRecordEndPpq;
        bpm = midiRecordBpm;
        numerator = midiRecordNumerator;
        denominator = midiRecordDenominator;

        for (int channel = 1; channel <= 16; ++channel)
        {
            for (int note = 0; note < 128; ++note)
            {
                if (openNotes[static_cast<std::size_t> (refIndex (channel, note))] <= 0)
                    continue;

                auto off = juce::MidiMessage::noteOff (channel, note);
                off.setTimeStamp (endPpq);
                events.push_back ({ endPpq, off });
            }
        }
    }

    std::sort (events.begin(), events.end(), [] (const auto& a, const auto& b)
    {
        if (std::abs (a.ppq - b.ppq) > 0.0000001)
            return a.ppq < b.ppq;
        return a.message.isNoteOff() && b.message.isNoteOn();
    });

    juce::MidiFile file;
    file.setTicksPerQuarterNote (ticksPerQuarter);
    juce::MidiMessageSequence track;

    auto name = juce::MidiMessage::textMetaEvent (3, "Voicizer MIDI Capture");
    name.setTimeStamp (0.0);
    track.addEvent (name);
    auto tempo = juce::MidiMessage::tempoMetaEvent (static_cast<int> (std::llround (60000000.0 / juce::jlimit (1.0, 999.0, bpm))));
    tempo.setTimeStamp (0.0);
    track.addEvent (tempo);
    auto signature = juce::MidiMessage::timeSignatureMetaEvent (juce::jmax (1, numerator), juce::jmax (1, denominator));
    signature.setTimeStamp (0.0);
    track.addEvent (signature);

    auto lastTick = 0.0;
    for (const auto& event : events)
    {
        auto message = event.message;
        const auto tick = juce::jmax (0.0, static_cast<double> (std::llround ((event.ppq - originPpq) * ticksPerQuarter)));
        message.setTimeStamp (tick);
        track.addEvent (message);
        lastTick = juce::jmax (lastTick, tick);
    }

    const auto endTick = juce::jmax (lastTick + 1.0,
                                     static_cast<double> (std::llround ((endPpq - originPpq) * ticksPerQuarter)));
    auto end = juce::MidiMessage::endOfTrack();
    end.setTimeStamp (endTick);
    track.addEvent (end);
    track.sort();
    file.addTrack (track);

    if (! destination.getParentDirectory().createDirectory())
        return false;

    destination.deleteFile();
    juce::FileOutputStream output (destination);
    return output.openedOk() && file.writeTo (output, 1);
}

void SoliVoicerAudioProcessor::panic()
{
    for (auto& active : activeChords)
    {
        active.notes.clear();
        active.outputChannels.clear();
    }
    generatedNoteRefs.fill (0);
    lastLeadNoteSample.fill (-1);
    pendingMidi.clear();
    performanceChannels = {};
    engine.reset();

    const std::lock_guard<std::mutex> lock (nameMutex);
    lastChordName = "--";
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
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { ParameterIDs::multiChannelOut, 1 }, "Multi Channel Out", false));
    addChoice (ParameterIDs::sourceMode, "Harmony Source", sourceModeNames(), 0);
    addChoice (ParameterIDs::outputMode, "Output Mode", outputModeNames(), 0);
    addChoice (ParameterIDs::contextMode, "Chord Relationship", Soli::ChordEngine::contextModeNames(), 3);
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { ParameterIDs::substitutionDepth, 1 }, "Substitution Depth", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.35f));
    addChoice (ParameterIDs::performanceStyle, "Performance Style", performanceStyleNames(), 0);
    addChoice (ParameterIDs::performanceSubStyle, "Performance Sub Style", { "Sub Style 1", "Sub Style 2", "Sub Style 3", "Sub Style 4", "Sub Style 5", "Sub Style 6" }, 0);
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { ParameterIDs::performanceComplexity, 1 }, "Performance Sophistication", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.45f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { ParameterIDs::rhythmDensity, 1 }, "Rhythm Density", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.48f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { ParameterIDs::syncopation, 1 }, "Syncopation", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.2f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { ParameterIDs::swing, 1 }, "Swing", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { ParameterIDs::humanize, 1 }, "Humanize", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.12f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { ParameterIDs::gate, 1 }, "Gate", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.72f));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { ParameterIDs::doubleTime, 1 }, "Double Time", false));

    return { params.begin(), params.end() };
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SoliVoicerAudioProcessor();
}
