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

int positiveMod12 (int value)
{
    value %= 12;
    return value < 0 ? value + 12 : value;
}

struct ChordQualityDefinition
{
    const char* symbol;
    std::vector<int> pitchClasses;
    std::vector<int> formulaIntervals;
};

const std::vector<ChordQualityDefinition>& chordQualityDefinitions()
{
    // pitchClasses are used for recognition. formulaIntervals retain compound
    // extensions so an incoming Melody Top note can actually represent the 9th,
    // 11th, or 13th instead of an arbitrary captured root.
    static const std::vector<ChordQualityDefinition> definitions
    {
        { "maj13",     { 0, 2, 4, 5, 7, 9, 11 }, { 0, 4, 7, 11, 14, 17, 21 } },
        { "13",        { 0, 2, 4, 5, 7, 9, 10 }, { 0, 4, 7, 10, 14, 17, 21 } },
        { "m13",       { 0, 2, 3, 5, 7, 9, 10 }, { 0, 3, 7, 10, 14, 17, 21 } },
        { "maj9#11",   { 0, 2, 4, 6, 7, 11 },    { 0, 4, 7, 11, 14, 18 } },
        { "9#11",      { 0, 2, 4, 6, 7, 10 },    { 0, 4, 7, 10, 14, 18 } },
        { "m11",       { 0, 2, 3, 5, 7, 10 },    { 0, 3, 7, 10, 14, 17 } },
        { "maj11",     { 0, 2, 4, 5, 7, 11 },    { 0, 4, 7, 11, 14, 17 } },
        { "11",        { 0, 2, 4, 5, 7, 10 },    { 0, 4, 7, 10, 14, 17 } },
        { "7b9b13",    { 0, 1, 4, 7, 8, 10 },    { 0, 4, 7, 10, 13, 20 } },
        { "7#9b13",    { 0, 3, 4, 7, 8, 10 },    { 0, 4, 7, 10, 15, 20 } },
        { "mMaj9",     { 0, 2, 3, 7, 11 },       { 0, 3, 7, 11, 14 } },
        { "maj9",      { 0, 2, 4, 7, 11 },       { 0, 4, 7, 11, 14 } },
        { "9",         { 0, 2, 4, 7, 10 },       { 0, 4, 7, 10, 14 } },
        { "m9",        { 0, 2, 3, 7, 10 },       { 0, 3, 7, 10, 14 } },
        { "m9b5",      { 0, 2, 3, 6, 10 },       { 0, 3, 6, 10, 14 } },
        { "7b9",       { 0, 1, 4, 7, 10 },       { 0, 4, 7, 10, 13 } },
        { "7#9",       { 0, 3, 4, 7, 10 },       { 0, 4, 7, 10, 15 } },
        { "7b13",      { 0, 4, 7, 8, 10 },       { 0, 4, 7, 10, 20 } },
        { "6/9",       { 0, 2, 4, 7, 9 },        { 0, 4, 7, 9, 14 } },
        { "m6/9",      { 0, 2, 3, 7, 9 },        { 0, 3, 7, 9, 14 } },
        { "maj7#11",   { 0, 4, 6, 7, 11 },       { 0, 4, 7, 11, 18 } },
        { "7#11",      { 0, 4, 6, 7, 10 },       { 0, 4, 7, 10, 18 } },
        { "maj7#5",    { 0, 4, 8, 11 },          { 0, 4, 8, 11 } },
        { "maj7b5",    { 0, 4, 6, 11 },          { 0, 4, 6, 11 } },
        { "7#5",       { 0, 4, 8, 10 },          { 0, 4, 8, 10 } },
        { "7b5",       { 0, 4, 6, 10 },          { 0, 4, 6, 10 } },
        { "mMaj7",     { 0, 3, 7, 11 },          { 0, 3, 7, 11 } },
        { "m7b5",      { 0, 3, 6, 10 },          { 0, 3, 6, 10 } },
        { "dim7",      { 0, 3, 6, 9 },           { 0, 3, 6, 9 } },
        { "maj7",      { 0, 4, 7, 11 },          { 0, 4, 7, 11 } },
        { "7",         { 0, 4, 7, 10 },          { 0, 4, 7, 10 } },
        { "m7",        { 0, 3, 7, 10 },          { 0, 3, 7, 10 } },
        { "add9",      { 0, 2, 4, 7 },           { 0, 4, 7, 14 } },
        { "madd9",     { 0, 2, 3, 7 },           { 0, 3, 7, 14 } },
        { "6",         { 0, 4, 7, 9 },           { 0, 4, 7, 9 } },
        { "m6",        { 0, 3, 7, 9 },           { 0, 3, 7, 9 } },
        { "7sus4",     { 0, 5, 7, 10 },          { 0, 5, 7, 10 } },
        { "maj",       { 0, 4, 7 },              { 0, 4, 7 } },
        { "m",         { 0, 3, 7 },              { 0, 3, 7 } },
        { "dim",       { 0, 3, 6 },              { 0, 3, 6 } },
        { "aug",       { 0, 4, 8 },              { 0, 4, 8 } },
        { "sus2",      { 0, 2, 7 },              { 0, 2, 7 } },
        { "sus4",      { 0, 5, 7 },              { 0, 5, 7 } },
        { "5",         { 0, 7 },                 { 0, 7 } }
    };
    return definitions;
}

std::vector<int> pitchClassFormula (const std::vector<int>& intervals)
{
    std::vector<int> result;
    result.reserve (intervals.size());
    for (const auto interval : intervals)
        result.push_back (positiveMod12 (interval));
    std::sort (result.begin(), result.end());
    result.erase (std::unique (result.begin(), result.end()), result.end());
    return result;
}

const ChordQualityDefinition* exactChordQuality (const std::vector<int>& intervals)
{
    const auto pitchClasses = pitchClassFormula (intervals);
    for (const auto& definition : chordQualityDefinitions())
        if (definition.pitchClasses == pitchClasses)
            return &definition;
    return nullptr;
}

void normaliseChordBankCard (SoliVoicerAudioProcessor::ChordBankCard& card)
{
    if (const auto* definition = exactChordQuality (card.intervals))
    {
        card.name = definition->symbol;
        card.intervals = definition->formulaIntervals;
    }
    else
    {
        card.name = "custom";
        card.intervals = pitchClassFormula (card.intervals);
    }

    // Captured absolute roots and inversions are intentionally discarded. A
    // Chord Bank card is a quality formula, not a remembered named chord.
    card.rootPitchClass = 0;
    card.bassPitchClass = 0;
}

SoliVoicerAudioProcessor::ChordBankCard analyseChordBankCard (std::vector<int> notes)
{
    SoliVoicerAudioProcessor::ChordBankCard card;
    if (notes.empty())
        return card;
    std::sort (notes.begin(), notes.end());
    notes.erase (std::unique (notes.begin(), notes.end()), notes.end());
    card.bassPitchClass = positiveMod12 (notes.front());
    std::vector<int> pitchClasses;
    for (const auto note : notes)
        pitchClasses.push_back (positiveMod12 (note));
    std::sort (pitchClasses.begin(), pitchClasses.end());
    pitchClasses.erase (std::unique (pitchClasses.begin(), pitchClasses.end()), pitchClasses.end());

    int bestRoot = pitchClasses.front();
    const ChordQualityDefinition* bestQuality = nullptr;
    int bestScore = -10000;
    for (const auto root : pitchClasses)
    {
        std::vector<int> actual;
        for (const auto pitch : pitchClasses)
            actual.push_back (positiveMod12 (pitch - root));
        std::sort (actual.begin(), actual.end());
        for (const auto& quality : chordQualityDefinitions())
        {
            int matches = 0;
            for (const auto interval : quality.pitchClasses)
                if (std::find (actual.begin(), actual.end(), interval) != actual.end())
                    ++matches;
            const auto missing = static_cast<int> (quality.pitchClasses.size()) - matches;
            const auto extra = static_cast<int> (actual.size()) - matches;
            const auto exactBonus = missing == 0 && extra == 0 ? 200 : 0;
            const auto bassBonus = root == card.bassPitchClass ? 5 : 0;
            const auto score = exactBonus + matches * 12 - missing * 15 - extra * 8 + bassBonus;
            if (score > bestScore)
            {
                bestScore = score;
                bestRoot = root;
                bestQuality = &quality;
            }
        }
    }

    if (bestQuality != nullptr)
    {
        card.name = bestQuality->symbol;
        card.intervals = bestQuality->formulaIntervals;
    }
    else
    {
        card.name = "custom";
        for (const auto pitch : pitchClasses)
            card.intervals.push_back (positiveMod12 (pitch - bestRoot));
        std::sort (card.intervals.begin(), card.intervals.end());
    }
    card.rootPitchClass = 0;
    card.bassPitchClass = 0;
    return card;
}

}

SoliVoicerAudioProcessor::SoliVoicerAudioProcessor()
    : AudioProcessor (BusesProperties()),
      parameters (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    lastLeadNoteSample.fill (-1);
    for (auto& mask : visualVoicedNoteMasks)
        mask.store (0, std::memory_order_relaxed);
    for (auto& mask : visualInputNoteMasks)
        mask.store (0, std::memory_order_relaxed);
}

void SoliVoicerAudioProcessor::prepareToPlay (double, int)
{
    processedSamples = 0;
    lastLeadNoteSample.fill (-1);
    pendingMidi.clear();
    performanceChannels = {};
    lastTransportPpq = -1.0;
    lastTransportPlaying = false;
    {
        const juce::SpinLock::ScopedLockType lock (chordBankLock);
        chordBankHeld = {};
        chordBankFinalizeAfterSample = -1;
        chordBankCapture.clear();
    }
    for (auto& mask : visualVoicedNoteMasks)
        mask.store (0, std::memory_order_relaxed);
    for (auto& mask : visualInputNoteMasks)
        mask.store (0, std::memory_order_relaxed);
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

void SoliVoicerAudioProcessor::refreshVisualVoicing() noexcept
{
    juce::uint64 masks[2] { 0, 0 };
    juce::uint64 inputMasks[2] { 0, 0 };
    const auto include = [] (juce::uint64 (&destination)[2], int note)
    {
        if (note < 0 || note >= 128)
            return;
        destination[note / 64] |= static_cast<juce::uint64> (1) << (note % 64);
    };

    for (std::size_t index = 0; index < activeChords.size(); ++index)
    {
        const auto& active = activeChords[index];
        if (! active.notes.empty())
            include (inputMasks, static_cast<int> (index % 128));
        for (const auto note : active.notes)
            include (masks, note);
    }
    for (const auto& state : performanceChannels)
        if (state.held)
        {
            include (inputMasks, state.inputNote);
            for (const auto note : state.voicing)
                include (masks, note);
        }

    visualVoicedNoteMasks[0].store (masks[0], std::memory_order_release);
    visualVoicedNoteMasks[1].store (masks[1], std::memory_order_release);
    visualInputNoteMasks[0].store (inputMasks[0], std::memory_order_release);
    visualInputNoteMasks[1].store (inputMasks[1], std::memory_order_release);
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
    settings.harmonicStability = *parameters.getRawParameterValue (ParameterIDs::harmonicStability);
    settings.melodyImportance = *parameters.getRawParameterValue (ParameterIDs::melodyImportance);
    settings.minNote = static_cast<int> (*parameters.getRawParameterValue (ParameterIDs::minNote));
    settings.maxNote = static_cast<int> (*parameters.getRawParameterValue (ParameterIDs::maxNote));
    return settings;
}

void SoliVoicerAudioProcessor::setChordBankListening (bool shouldListen) noexcept
{
    const auto wasListening = chordBankListening.exchange (shouldListen, std::memory_order_acq_rel);
    if (wasListening != shouldListen)
        pendingNewPhrase.store (true, std::memory_order_release);

    {
        juce::String finalizedName;
        {
            const juce::SpinLock::ScopedLockType lock (chordBankLock);
            if (shouldListen)
            {
                chordBankHeld = {};
                chordBankFinalizeAfterSample = -1;
                chordBankCapture.clear();
            }
            else
            {
                chordBankHeld = {};
                finalizedName = finalizeChordBankCaptureLocked();
            }
        }

        if (finalizedName.isNotEmpty())
        {
            const std::lock_guard<std::mutex> nameLock (nameMutex);
            lastChordName = finalizedName;
        }
    }
}

std::vector<SoliVoicerAudioProcessor::ChordBankCard> SoliVoicerAudioProcessor::getChordBankCards() const
{
    const juce::SpinLock::ScopedLockType lock (chordBankLock);
    return chordBank;
}

void SoliVoicerAudioProcessor::setChordBankCardProbability (int index, float probability)
{
    const juce::SpinLock::ScopedLockType lock (chordBankLock);
    if (index >= 0 && index < static_cast<int> (chordBank.size()))
        chordBank[static_cast<std::size_t> (index)].probability = juce::jlimit (0.0f, 1.0f, probability);
}

void SoliVoicerAudioProcessor::removeChordBankCard (int index)
{
    const juce::SpinLock::ScopedLockType lock (chordBankLock);
    if (index >= 0 && index < static_cast<int> (chordBank.size()))
        chordBank.erase (chordBank.begin() + index);
}

void SoliVoicerAudioProcessor::clearChordBank()
{
    const juce::SpinLock::ScopedLockType lock (chordBankLock);
    chordBank.clear();
    chordBankHeld = {};
    chordBankFinalizeAfterSample = -1;
    chordBankCapture.clear();
}

void SoliVoicerAudioProcessor::captureChordBankNoteOn (int channel, int note)
{
    const auto channelIndex = static_cast<std::size_t> (juce::jlimit (0, 15, channel - 1));
    note = juce::jlimit (0, 127, note);
    const juce::SpinLock::ScopedLockType lock (chordBankLock);
    chordBankHeld[channelIndex][static_cast<std::size_t> (note)] = true;
    chordBankFinalizeAfterSample = -1;
    if (std::find (chordBankCapture.begin(), chordBankCapture.end(), note) == chordBankCapture.end())
        chordBankCapture.push_back (note);
}

void SoliVoicerAudioProcessor::captureChordBankNoteOff (int channel, int note, juce::int64 absoluteSample)
{
    const auto channelIndex = static_cast<std::size_t> (juce::jlimit (0, 15, channel - 1));
    note = juce::jlimit (0, 127, note);
    const juce::SpinLock::ScopedLockType lock (chordBankLock);
    chordBankHeld[channelIndex][static_cast<std::size_t> (note)] = false;
    if (anyChordBankNoteHeldLocked())
        return;

    // A very short release debounce tolerates event jitter without collecting
    // a run of separate melodic notes into a false chord.
    const auto graceSamples = static_cast<juce::int64> (juce::jmax (1.0, getSampleRate() * 0.03));
    chordBankFinalizeAfterSample = absoluteSample + graceSamples;
}

void SoliVoicerAudioProcessor::captureChordBankAllNotesOff (int channel, juce::int64 absoluteSample)
{
    const auto channelIndex = static_cast<std::size_t> (juce::jlimit (0, 15, channel - 1));
    const juce::SpinLock::ScopedLockType lock (chordBankLock);
    chordBankHeld[channelIndex].fill (false);
    if (! anyChordBankNoteHeldLocked())
        chordBankFinalizeAfterSample = absoluteSample;
}

bool SoliVoicerAudioProcessor::anyChordBankNoteHeldLocked() const noexcept
{
    for (const auto& channel : chordBankHeld)
        if (std::any_of (channel.begin(), channel.end(), [] (bool held) { return held; }))
            return true;
    return false;
}

juce::String SoliVoicerAudioProcessor::finalizeChordBankCaptureLocked()
{
    juce::String finalizedName;
    std::array<bool, 12> pitchClasses {};
    for (const auto note : chordBankCapture)
        pitchClasses[static_cast<std::size_t> (positiveMod12 (note))] = true;
    const auto pitchClassCount = static_cast<int> (std::count (pitchClasses.begin(), pitchClasses.end(), true));
    if (pitchClassCount >= 3)
    {
        auto card = analyseChordBankCard (chordBankCapture);
        if (card.name.isNotEmpty())
        {
            finalizedName = card.name;
            const auto duplicate = std::find_if (chordBank.begin(), chordBank.end(), [&card] (const auto& existing)
            {
                return existing.name == card.name && existing.intervals == card.intervals;
            });
            if (duplicate == chordBank.end())
            {
                if (chordBank.size() >= 24)
                    chordBank.erase (chordBank.begin());
                chordBank.push_back (std::move (card));
            }
        }
    }
    chordBankCapture.clear();
    chordBankFinalizeAfterSample = -1;
    return finalizedName;
}

void SoliVoicerAudioProcessor::finalizeExpiredChordBankCaptures (juce::int64 absoluteSample)
{
    juce::String finalizedName;
    {
        const juce::SpinLock::ScopedLockType lock (chordBankLock);
        if (chordBankFinalizeAfterSample >= 0 && absoluteSample >= chordBankFinalizeAfterSample)
        {
            const auto name = finalizeChordBankCaptureLocked();
            if (name.isNotEmpty())
                finalizedName = name;
        }
    }

    if (finalizedName.isNotEmpty())
    {
        const std::lock_guard<std::mutex> lock (nameMutex);
        lastChordName = finalizedName;
    }
}

Soli::GeneratedChord SoliVoicerAudioProcessor::generateChordBankVoicing (int inputNote, const Soli::Settings& settings)
{
    ChordBankCard card;
    {
        const juce::SpinLock::ScopedLockType lock (chordBankLock);
        if (chordBank.empty())
            return {};
        std::array<double, 24> weights {};
        auto hasPositiveWeight = false;
        for (std::size_t index = 0; index < chordBank.size(); ++index)
        {
            const auto& candidate = chordBank[index];
            const auto weight = static_cast<double> (juce::jlimit (0.0f, 1.0f, candidate.probability));
            weights[index] = weight;
            hasPositiveWeight = hasPositiveWeight || weight > 0.0;
        }
        if (! hasPositiveWeight)
            std::fill (weights.begin(), weights.begin() + static_cast<std::ptrdiff_t> (chordBank.size()), 1.0);
        std::discrete_distribution<std::size_t> chooseCard (
            weights.begin(), weights.begin() + static_cast<std::ptrdiff_t> (chordBank.size()));
        card = chordBank[chooseCard (performanceRandom)];
    }

    if (card.intervals.empty())
        return {};

    auto role = settings.role;
    if (role == Soli::NoteRole::random)
    {
        std::uniform_int_distribution<int> chooseRole (0, 4);
        role = static_cast<Soli::NoteRole> (chooseRole (performanceRandom));
    }
    else if (role == Soli::NoteRole::autoWeighted)
    {
        std::discrete_distribution<int> chooseRole { 58, 6, 4, 12, 20 };
        role = static_cast<Soli::NoteRole> (chooseRole (performanceRandom));
    }

    auto anchorIndex = 0;
    if (role == Soli::NoteRole::melodyTop)
    {
        anchorIndex = static_cast<int> (card.intervals.size()) - 1;
    }
    else if (role == Soli::NoteRole::guideTone)
    {
        // Prefer a seventh as the guide tone, then a third. This lets a played
        // melodic line act like a conventional jazz guide-tone line.
        for (int index = 0; index < static_cast<int> (card.intervals.size()); ++index)
        {
            const auto pitchClass = positiveMod12 (card.intervals[static_cast<std::size_t> (index)]);
            if (pitchClass == 3 || pitchClass == 4)
                anchorIndex = index;
            if (pitchClass == 10 || pitchClass == 11)
                anchorIndex = index;
        }
    }
    else if (role == Soli::NoteRole::innerVoice)
    {
        anchorIndex = static_cast<int> (card.intervals.size()) / 2;
        anchorIndex = juce::jlimit (1, static_cast<int> (card.intervals.size()) - 1, anchorIndex);
    }

    const auto anchorInterval = card.intervals[static_cast<std::size_t> (anchorIndex)];
    const auto root = inputNote - anchorInterval;

    Soli::GeneratedChord result;
    result.name = card.name;
    result.notes.reserve (card.intervals.size());
    for (int index = 0; index < static_cast<int> (card.intervals.size()); ++index)
    {
        auto note = root + card.intervals[static_cast<std::size_t> (index)];
        if (index == anchorIndex)
        {
            note = inputNote;
        }
        else
        {
            while (note < settings.minNote) note += 12;
            while (note > settings.maxNote) note -= 12;

            if (role == Soli::NoteRole::melodyTop)
                while (note > inputNote) note -= 12;
            else if (role == Soli::NoteRole::bass)
                while (note < inputNote) note += 12;
        }
        result.notes.push_back (juce::jlimit (0, 127, note));
    }
    std::sort (result.notes.begin(), result.notes.end());
    result.notes.erase (std::unique (result.notes.begin(), result.notes.end()), result.notes.end());

    // The trigger must remain audible in exactly the role requested, even if
    // a restrictive range causes another formula tone to fold by an octave.
    if (std::find (result.notes.begin(), result.notes.end(), inputNote) == result.notes.end())
        result.notes.push_back (juce::jlimit (0, 127, inputNote));
    std::sort (result.notes.begin(), result.notes.end());
    return result;
}

juce::StringArray SoliVoicerAudioProcessor::sourceModeNames()
{
    return { "Scale / Harmony", "Chord Bank" };
}

juce::StringArray SoliVoicerAudioProcessor::outputModeNames()
{
    // Both values intentionally render the same held-voicing behavior.
    return { "Held Voicing", "Held Voicing (legacy)" };
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
    for (const auto note : active.notes)
        sendGeneratedNoteOff (active.channel, note, samplePosition, blockSamples, output);
    active.notes.clear();
    refreshVisualVoicing();
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

    if (releasedAny)
        refreshVisualVoicing();
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
    refreshVisualVoicing();

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
    refreshVisualVoicing();

    auto orderedNotes = notesToStart;
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
    juce::ignoreUnused (ppq);
    return engine.generate (inputNote, velocity, settings);
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
    state.step = 0;
    std::uniform_int_distribution<int> phraseDistribution (0, 17);
    std::uniform_int_distribution<int> articulationDistribution (0, 7);
    std::uniform_real_distribution<float> intensityDistribution (-0.12f, 0.14f);
    state.phraseVariant = phraseDistribution (performanceRandom);
    state.articulationVariant = articulationDistribution (performanceRandom);
    state.intensityBias = intensityDistribution (performanceRandom);
    state.nextStepPpq = ppq;
    state.contextName = lastChordizerContext.current;
    refreshVisualVoicing();
}

void SoliVoicerAudioProcessor::stopPerformance (int channel,
                                                int inputNote,
                                                int samplePosition,
                                                juce::MidiBuffer& output)
{
    auto& state = performanceChannels[static_cast<std::size_t> (juce::jlimit (0, 15, channel - 1))];
    if (! state.held || state.inputNote != inputNote)
        return;

    state = {};
    pendingMidi.erase (std::remove_if (pendingMidi.begin(), pendingMidi.end(), [channel] (const PendingMidi& event)
    {
        return event.message.isForChannel (channel);
    }), pendingMidi.end());
    output.addEvent (juce::MidiMessage::allNotesOff (channel), juce::jmax (0, samplePosition));
    refreshVisualVoicing();
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
                    scheduleMidiEvent (juce::MidiMessage::noteOn (state.channel, note,
                                                                 static_cast<juce::uint8> (velocity)),
                                       sampleOffset, blockSamples, output);
                    scheduleMidiEvent (juce::MidiMessage::noteOff (state.channel, note),
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
            output->addEvent (juce::MidiMessage::allNotesOff (state.channel), juce::jmax (0, samplePosition));
        state = {};
    }
    pendingMidi.clear();
    refreshVisualVoicing();
}

void SoliVoicerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    buffer.clear();

    juce::MidiBuffer output;
    const auto blockSamples = buffer.getNumSamples();
    const auto blockStartSample = processedSamples;
    if (pendingNewPhrase.exchange (false, std::memory_order_acq_rel))
    {
        for (int channel = 1; channel <= 16; ++channel)
            output.addEvent (juce::MidiMessage::allNotesOff (channel), 0);
        for (auto& active : activeChords)
            active.notes.clear();
        generatedNoteRefs.fill (0);
        lastLeadNoteSample.fill (-1);
        pendingMidi.clear();
        performanceChannels = {};
        engine.reset();
        refreshVisualVoicing();
        const std::lock_guard<std::mutex> lock (nameMutex);
        lastChordName = "--";
    }
    emitPendingMidi (blockSamples, output);
    const auto settings = readSettings();
    const auto transport = readTransport();
    const auto chordBankMode = parameters.getRawParameterValue (ParameterIDs::sourceMode)->load() > 0.5f;
    const auto chordBankIsListening = chordBankListening.load (std::memory_order_acquire);
    constexpr auto performanceMode = false;
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
                    output.addEvent (juce::MidiMessage::allNotesOff (state.channel), 0);
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
        const auto absoluteSample = blockStartSample + samplePosition;
        if (chordBankMode && chordBankIsListening)
            finalizeExpiredChordBankCaptures (absoluteSample);
        juce::ignoreUnused (renderedUntilSample);
        renderedUntilSample = samplePosition;

        if (message.isNoteOn())
        {
            const auto channel = message.getChannel();
            const auto inputNote = message.getNoteNumber();
            const auto velocity = message.getVelocity();

            if (chordBankMode && chordBankIsListening)
            {
                captureChordBankNoteOn (channel, inputNote);
                output.addEvent (message, samplePosition);
                continue;
            }

            const auto channelIndex = static_cast<size_t> (juce::jlimit (0, 15, channel - 1));
            const auto previousLeadSample = lastLeadNoteSample[channelIndex];
            const auto fastLead = previousLeadSample >= 0
                               && getSampleRate() > 0.0
                               && absoluteSample - previousLeadSample <= static_cast<juce::int64> (0.14 * getSampleRate());
            lastLeadNoteSample[channelIndex] = absoluteSample;

            const auto eventPpq = transport.valid ? transport.ppq + samplePosition * ppqPerSample : 0.0;
            auto eventSettings = settings;
            eventSettings.fastInput = fastLead;
            auto generated = chordBankMode ? generateChordBankVoicing (inputNote, eventSettings)
                                           : generateChord (inputNote, velocity, eventSettings, eventPpq);
            // Chord Bank reproduces the recorded chord quality directly. Style
            // and Playability intentionally do not rewrite its notes; Role sets
            // register placement and the existing Rake controls onset spread.
            const auto safeNotes = chordBankMode ? generated.notes
                                                 : applyFastLeadSafety (generated.notes, inputNote, eventSettings, fastLead);
            const auto safeVelocity = chordBankMode ? velocity
                                                    : scaleVelocityForVoicing (velocity, static_cast<int> (generated.notes.size()), eventSettings, fastLead);
            transitionLeadChordOnChannel (channel,
                                          inputNote,
                                          safeVelocity,
                                          safeNotes,
                                          samplePosition,
                                          blockSamples,
                                          output,
                                          eventSettings);

            {
                const std::lock_guard<std::mutex> lock (nameMutex);
                lastChordName = generated.name;
            }
        }
        else if (message.isNoteOff())
        {
            if (chordBankMode && chordBankIsListening)
            {
                captureChordBankNoteOff (message.getChannel(), message.getNoteNumber(), absoluteSample);
                output.addEvent (message, samplePosition);
            }
            else
            {
                releaseActiveChord (message.getChannel(), message.getNoteNumber(), samplePosition, blockSamples, output);
            }
        }
        else if (message.isAllNotesOff() || message.isAllSoundOff())
        {
            if (chordBankMode && chordBankIsListening)
                captureChordBankAllNotesOff (message.getChannel(), absoluteSample);
            clearPerformance();
            for (auto& active : activeChords)
                active.notes.clear();
            generatedNoteRefs.fill (0);
            refreshVisualVoicing();
            engine.reset();
            output.addEvent (message, samplePosition);
        }
        else
        {
            output.addEvent (message, samplePosition);
        }
    }

    if (chordBankMode && chordBankIsListening)
        finalizeExpiredChordBankCaptures (blockStartSample + blockSamples);

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
    auto state = parameters.copyState();
    juce::StringArray encodedCards;
    {
        const juce::SpinLock::ScopedLockType lock (chordBankLock);
        for (const auto& card : chordBank)
        {
            juce::StringArray intervals;
            for (const auto interval : card.intervals)
                intervals.add (juce::String (interval));
            encodedCards.add (card.name + "|" + juce::String (card.rootPitchClass) + "|"
                              + juce::String (card.bassPitchClass) + "|" + intervals.joinIntoString (",")
                              + "|" + juce::String (card.probability, 4));
        }
    }
    state.setProperty ("chordBankData", encodedCards.joinIntoString (";"), nullptr);
    state.setProperty ("chordBankListening", chordBankListening.load (std::memory_order_acquire), nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void SoliVoicerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (parameters.state.getType()))
        {
            auto state = juce::ValueTree::fromXml (*xml);
            parameters.replaceState (state);
            std::vector<ChordBankCard> restored;
            const auto rows = juce::StringArray::fromTokens (state.getProperty ("chordBankData").toString(), ";", "");
            for (const auto& row : rows)
            {
                const auto fields = juce::StringArray::fromTokens (row, "|", "");
                if (fields.size() < 4)
                    continue;
                ChordBankCard card;
                card.name = fields[0];
                card.rootPitchClass = fields[1].getIntValue();
                card.bassPitchClass = fields[2].getIntValue();
                const auto intervals = juce::StringArray::fromTokens (fields[3], ",", "");
                for (const auto& interval : intervals)
                    card.intervals.push_back (interval.getIntValue());
                card.probability = fields.size() >= 5 ? juce::jlimit (0.0f, 1.0f, fields[4].getFloatValue()) : 1.0f;
                if (! card.intervals.empty())
                {
                    normaliseChordBankCard (card);
                    restored.push_back (std::move (card));
                }
            }
            {
                const juce::SpinLock::ScopedLockType lock (chordBankLock);
                chordBank = std::move (restored);
            }
            chordBankListening.store (static_cast<bool> (state.getProperty ("chordBankListening", true)), std::memory_order_release);
        }
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

std::array<juce::uint64, 2> SoliVoicerAudioProcessor::getVisualVoicedNoteMasks() const noexcept
{
    return { visualVoicedNoteMasks[0].load (std::memory_order_acquire),
             visualVoicedNoteMasks[1].load (std::memory_order_acquire) };
}

std::array<juce::uint64, 2> SoliVoicerAudioProcessor::getVisualInputNoteMasks() const noexcept
{
    return { visualInputNoteMasks[0].load (std::memory_order_acquire),
             visualInputNoteMasks[1].load (std::memory_order_acquire) };
}

void SoliVoicerAudioProcessor::panic()
{
    for (auto& active : activeChords)
        active.notes.clear();
    generatedNoteRefs.fill (0);
    lastLeadNoteSample.fill (-1);
    pendingMidi.clear();
    performanceChannels = {};
    engine.reset();
    for (auto& mask : visualVoicedNoteMasks)
        mask.store (0, std::memory_order_release);
    for (auto& mask : visualInputNoteMasks)
        mask.store (0, std::memory_order_release);

    const std::lock_guard<std::mutex> lock (nameMutex);
    lastChordName = "--";
}

void SoliVoicerAudioProcessor::startNewPhrase()
{
    pendingNewPhrase.store (true, std::memory_order_release);
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
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { ParameterIDs::outside, 1 }, "Outside Harmony", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.05f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { ParameterIDs::variation, 1 }, "Variation", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.35f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { ParameterIDs::repeatChance, 1 }, "Repeat Chance", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.15f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { ParameterIDs::strumSpeed, 1 }, "Strum Speed", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterInt> (juce::ParameterID { ParameterIDs::minNote, 1 }, "Min Note", 0, 126, 36));
    params.push_back (std::make_unique<juce::AudioParameterInt> (juce::ParameterID { ParameterIDs::maxNote, 1 }, "Max Note", 1, 127, 96));
    addChoice (ParameterIDs::sourceMode, "Harmony Source", sourceModeNames(), 0);
    addChoice (ParameterIDs::outputMode, "Output Mode", outputModeNames(), 0);
    addChoice (ParameterIDs::contextMode, "Chord Relationship", Soli::ChordEngine::contextModeNames(), 3);
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { ParameterIDs::substitutionDepth, 1 }, "Substitution Depth", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.35f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { ParameterIDs::harmonicStability, 1 }, "Harmonic Stability", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.72f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { ParameterIDs::melodyImportance, 1 }, "Melody Importance", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.88f));
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
