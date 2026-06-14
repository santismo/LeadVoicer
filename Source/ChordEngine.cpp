#include "ChordEngine.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>

namespace Soli
{
namespace
{
constexpr auto keyCount = 12;
constexpr auto scaleCount = 12;

int mod12 (int value) noexcept
{
    return (value % 12 + 12) % 12;
}

juce::String pcName (int pitchClass)
{
    static const std::array<const char*, 12> names { "C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B" };
    return names[static_cast<size_t> (mod12 (pitchClass))];
}

juce::String chordSuffixName (const juce::String& suffix)
{
    if (suffix == "quartal") return "sus11";
    if (suffix == "cluster") return "add9";
    return suffix;
}

const std::vector<int>& scaleIntervals (ScaleType scale)
{
    static const std::vector<int> ionian       { 0, 2, 4, 5, 7, 9, 11 };
    static const std::vector<int> dorian       { 0, 2, 3, 5, 7, 9, 10 };
    static const std::vector<int> phrygian     { 0, 1, 3, 5, 7, 8, 10 };
    static const std::vector<int> lydian       { 0, 2, 4, 6, 7, 9, 11 };
    static const std::vector<int> mixolydian   { 0, 2, 4, 5, 7, 9, 10 };
    static const std::vector<int> aeolian      { 0, 2, 3, 5, 7, 8, 10 };
    static const std::vector<int> locrian      { 0, 1, 3, 5, 6, 8, 10 };
    static const std::vector<int> harmonic     { 0, 2, 3, 5, 7, 8, 11 };
    static const std::vector<int> melodic      { 0, 2, 3, 5, 7, 9, 11 };
    static const std::vector<int> diminished   { 0, 2, 3, 5, 6, 8, 9, 11 };
    static const std::vector<int> wholeTone    { 0, 2, 4, 6, 8, 10 };
    static const std::vector<int> blues        { 0, 3, 5, 6, 7, 10 };

    switch (scale)
    {
        case ScaleType::dorian: return dorian;
        case ScaleType::phrygian: return phrygian;
        case ScaleType::lydian: return lydian;
        case ScaleType::mixolydian: return mixolydian;
        case ScaleType::aeolian: return aeolian;
        case ScaleType::locrian: return locrian;
        case ScaleType::harmonicMinor: return harmonic;
        case ScaleType::melodicMinor: return melodic;
        case ScaleType::diminished: return diminished;
        case ScaleType::wholeTone: return wholeTone;
        case ScaleType::blues: return blues;
        case ScaleType::ionian:
        default: return ionian;
    }
}

int clampMask (int mask, int count) noexcept
{
    const auto all = (1 << count) - 1;
    return juce::jlimit (1, all, mask);
}

bool maskContains (int mask, int index) noexcept
{
    return (mask & (1 << index)) != 0;
}

const std::vector<ChordEngine::ChordType>& chordTypes()
{
    static const std::vector<ChordEngine::ChordType> types
    {
        { "6",        { 0, 4, 7, 9 },             1, 1 },
        { "m6",       { 0, 3, 7, 9 },             1, 1 },
        { "maj7",     { 0, 4, 7, 11 },            1, 1 },
        { "m7",       { 0, 3, 7, 10 },            1, 1 },
        { "7",        { 0, 4, 7, 10 },            1, 1 },
        { "m7b5",     { 0, 3, 6, 10 },            2, 2 },
        { "dim7",     { 0, 3, 6, 9 },             2, 2 },
        { "9",        { 0, 4, 7, 10, 14 },        2, 2 },
        { "maj9",     { 0, 4, 7, 11, 14 },        2, 2 },
        { "m9",       { 0, 3, 7, 10, 14 },        2, 2 },
        { "13",       { 0, 4, 7, 10, 14, 21 },    3, 3 },
        { "m11",      { 0, 3, 7, 10, 14, 17 },    3, 3 },
        { "maj13#11", { 0, 4, 7, 11, 14, 18, 21 }, 4, 4 },
        { "7alt",     { 0, 4, 10, 13, 15, 20 },   4, 5 },
        { "7b9#11",   { 0, 4, 10, 13, 18 },       4, 5 },
        { "sus11",    { 0, 5, 7, 10, 14, 17 },    3, 4 },
        { "maj9",     { 0, 2, 4, 7, 11, 14 },     5, 5 }
    };

    return types;
}

int octaveNear (int pitchClass, int target)
{
    auto result = pitchClass;
    while (result < target - 6)
        result += 12;
    while (result > target + 6)
        result -= 12;
    return result;
}

float averageMotion (const std::vector<int>& a, const std::vector<int>& b)
{
    if (a.empty() || b.empty())
        return 18.0f;

    const auto count = std::min (a.size(), b.size());
    auto total = 0.0f;
    for (size_t i = 0; i < count; ++i)
        total += static_cast<float> (std::abs (a[i] - b[i]));
    total += static_cast<float> (std::abs (static_cast<int> (a.size()) - static_cast<int> (b.size())) * 8);
    return total / static_cast<float> (std::max<size_t> (1, count));
}
}

juce::StringArray ChordEngine::keyNames()
{
    return { "C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B" };
}

juce::StringArray ChordEngine::scaleNames()
{
    return { "Ionian", "Dorian", "Phrygian", "Lydian", "Mixolydian", "Aeolian", "Locrian", "Harmonic Minor", "Melodic Minor", "Diminished", "Whole Tone", "Blues" };
}

juce::StringArray ChordEngine::roleNames()
{
    return { "Melody Top", "Root", "Bass", "Guide Tone", "Inner Voice", "Random", "Auto" };
}

juce::StringArray ChordEngine::styleNames()
{
    return { "Close Lead", "Big Band", "Quartal Color", "Classical", "Gospel", "Modern Outside", "Modal Film", "Chromatic Mediant", "Baroque Counterpoint", "Neo-Soul", "Progressive Rock" };
}

juce::StringArray ChordEngine::playabilityNames()
{
    return { "Piano", "Guitar", "Horn Section", "Orchestra", "Unrestricted" };
}

juce::StringArray ChordEngine::strumModeNames()
{
    return { "Together", "Up", "Down", "Random" };
}

void ChordEngine::reset()
{
    previousVoicing.clear();
    previousChord = {};
}

GeneratedChord ChordEngine::generate (int inputNote, int, const Settings& settings)
{
    if (! previousChord.notes.empty())
    {
        std::uniform_real_distribution<float> chance (0.0f, 1.0f);
        if (chance (rng) < settings.repeatChance)
            return previousChord;
    }

    const auto role = resolveRole (settings);
    auto candidates = buildCandidates (inputNote, settings, role);

    if (candidates.empty())
    {
        GeneratedChord fallback;
        fallback.notes = { inputNote };
        fallback.name = pcName (inputNote);
        previousVoicing = fallback.notes;
        previousChord = fallback;
        return fallback;
    }

    for (auto& candidate : candidates)
        candidate.score = scoreCandidate (candidate, inputNote, settings);

    std::sort (candidates.begin(), candidates.end(), [] (const auto& a, const auto& b) { return a.score > b.score; });
    const auto chosenIndex = chooseWeightedIndex (candidates, settings);
    const auto& chosen = candidates[static_cast<size_t> (chosenIndex)];

    GeneratedChord result;
    result.notes = chosen.voiced;
    result.name = chordName (chosen.root, *chosen.type);
    previousVoicing = result.notes;
    previousChord = result;
    return result;
}

NoteRole ChordEngine::resolveRole (const Settings& settings)
{
    if (settings.role == NoteRole::random)
    {
        std::uniform_int_distribution<int> rolePick (0, 4);
        return static_cast<NoteRole> (rolePick (rng));
    }

    if (settings.role != NoteRole::autoWeighted)
        return settings.role;

    std::discrete_distribution<int> rolePick { 58, 6, 4, 12, 20 };
    return static_cast<NoteRole> (rolePick (rng));
}

int ChordEngine::choosePrimaryKey (const Settings& settings, int inputNote)
{
    const auto keyMask = clampMask (settings.keyMask, keyCount);
    auto bestKey = 0;
    auto bestScore = std::numeric_limits<float>::lowest();

    for (int key = 0; key < keyCount; ++key)
    {
        if (! maskContains (keyMask, key))
            continue;

        auto score = 20.0f;
        if (mod12 (inputNote - key) == 0)
            score += 4.0f;
        if (mod12 (inputNote - key) == 4 || mod12 (inputNote - key) == 7)
            score += 2.0f;
        if (! previousVoicing.empty())
            score -= static_cast<float> (std::abs (mod12 (key - mod12 (previousVoicing.front())))) * 0.12f;

        std::uniform_real_distribution<float> noise (0.0f, settings.variation * 4.0f + settings.outside * 3.0f);
        score += noise (rng);

        if (score > bestScore)
        {
            bestScore = score;
            bestKey = key;
        }
    }

    return bestKey;
}

ScaleType ChordEngine::choosePrimaryScale (const Settings& settings)
{
    const auto scaleMask = clampMask (settings.scaleMask, scaleCount);
    std::vector<double> weights;
    std::vector<int> indexes;
    weights.reserve (scaleCount);
    indexes.reserve (scaleCount);

    for (int i = 0; i < scaleCount; ++i)
    {
        if (! maskContains (scaleMask, i))
            continue;

        auto weight = 1.0;
        if (settings.style == Style::classical || settings.style == Style::baroqueCounterpoint)
            weight += (i == 0 || i == 5 || i == 7) ? 2.0 : 0.0;
        if (settings.style == Style::quartalColor || settings.style == Style::modalFilm || settings.style == Style::neoSoul)
            weight += (i == 1 || i == 3 || i == 4 || i == 8) ? 2.0 : 0.0;
        if (settings.style == Style::progressiveRock)
            weight += (i == 3 || i == 7 || i == 8 || i == 10) ? 2.0 : 0.0;
        if (settings.style == Style::modernOutside)
            weight += static_cast<double> (settings.outside) * 3.0;

        indexes.push_back (i);
        weights.push_back (weight);
    }

    std::discrete_distribution<int> pick (weights.begin(), weights.end());
    return static_cast<ScaleType> (indexes[static_cast<size_t> (pick (rng))]);
}

std::vector<ChordEngine::Candidate> ChordEngine::buildCandidates (int inputNote, const Settings& settings, NoteRole role) const
{
    std::vector<Candidate> candidates;
    candidates.reserve (96);
    const auto keyMask = clampMask (settings.keyMask, keyCount);

    for (int root = 0; root < 12; ++root)
    {
        if (! maskContains (keyMask, root) && settings.outside < 0.2f)
        {
            const auto relatedToSelectedKey = [&]
            {
                for (int key = 0; key < 12; ++key)
                    if (maskContains (keyMask, key) && (mod12 (root - key) == 0 || mod12 (root - key) == 5 || mod12 (root - key) == 7 || mod12 (root - key) == 9))
                        return true;
                return false;
            }();

            if (! relatedToSelectedKey)
                continue;
        }

        for (const auto& type : chordTypes())
        {
            const auto noteInterval = mod12 (inputNote - root);
            const auto containsInput = std::any_of (type.intervals.begin(), type.intervals.end(), [&] (int interval)
            {
                return mod12 (interval) == noteInterval;
            });

            if (! containsInput && role != NoteRole::bass)
                continue;

            if (static_cast<float> (type.complexity) / 5.0f > settings.complexity + settings.outside * 0.45f + 0.12f)
                continue;

            if (! chordMostlyInScale (root, type, settings) && settings.outside < 0.35f)
                continue;

            auto voiced = voiceCandidate (inputNote, root, type, settings, role);
            if (voiced.size() < 2)
                continue;

            candidates.push_back ({ root, &type, std::move (voiced), 0.0f });
        }
    }

    return candidates;
}

std::vector<int> ChordEngine::voiceCandidate (int inputNote, int root, const ChordType& type, const Settings& settings, NoteRole role) const
{
    auto maxChordSize = 10;
    if (settings.playability == Playability::guitar)
        maxChordSize = 6;
    else if (settings.playability == Playability::hornSection)
        maxChordSize = 8;
    else if (settings.playability == Playability::orchestra || settings.playability == Playability::unrestricted)
        maxChordSize = 24;

    auto requestedSize = juce::jlimit (2, maxChordSize, settings.chordSize);
    if (settings.playability == Playability::guitar)
        requestedSize = juce::jmin (requestedSize, 6);
    else if (settings.playability == Playability::hornSection)
        requestedSize = juce::jlimit (3, 5, requestedSize);

    std::vector<int> intervals = type.intervals;
    intervals.reserve (24);
    while (static_cast<int> (intervals.size()) > requestedSize)
    {
        const auto eraseIndex = static_cast<int> (intervals.size()) > 4 ? 2 : static_cast<int> (intervals.size()) - 1;
        intervals.erase (intervals.begin() + juce::jlimit (0, static_cast<int> (intervals.size()) - 1, eraseIndex));
    }

    const auto inputInterval = mod12 (inputNote - root);
    auto anchorIndex = nearestChordToneIndex (intervals, inputInterval);

    if (role == NoteRole::root)
        anchorIndex = 0;
    else if (role == NoteRole::guideTone)
    {
        for (int i = 0; i < static_cast<int> (intervals.size()); ++i)
            if (mod12 (intervals[static_cast<size_t> (i)]) == 3 || mod12 (intervals[static_cast<size_t> (i)]) == 4
                || mod12 (intervals[static_cast<size_t> (i)]) == 10 || mod12 (intervals[static_cast<size_t> (i)]) == 11)
                anchorIndex = i;
    }
    else if (role == NoteRole::bass)
    {
        anchorIndex = 0;
    }

    std::vector<int> notes;
    notes.reserve (24);
    const auto rootNearInput = octaveNear (root, inputNote - intervals[static_cast<size_t> (anchorIndex)]);
    const auto anchorAbsolute = rootNearInput + intervals[static_cast<size_t> (anchorIndex)];
    const auto shift = inputNote - anchorAbsolute;

    for (const auto interval : intervals)
    {
        auto note = rootNearInput + interval + shift;
        if (role == NoteRole::melodyTop)
        {
            while (note >= inputNote && mod12 (note) != mod12 (inputNote))
                note -= 12;
            if (mod12 (note) == mod12 (inputNote))
                note = inputNote;
            while (note > inputNote)
                note -= 12;
        }
        notes.push_back (note);
    }

    if (role == NoteRole::bass)
    {
        notes[0] = inputNote;
        for (size_t i = 1; i < notes.size(); ++i)
            while (notes[i] <= notes[i - 1])
                notes[i] += 12;
    }
    else if (role == NoteRole::melodyTop)
    {
        for (auto& note : notes)
            while (note > inputNote)
                note -= 12;
        notes.push_back (inputNote);
    }
    else
    {
        notes[static_cast<size_t> (anchorIndex)] = inputNote;
    }

    std::sort (notes.begin(), notes.end());
    notes.erase (std::unique (notes.begin(), notes.end()), notes.end());

    if (role == NoteRole::melodyTop && ! notes.empty() && notes.back() != inputNote)
    {
        notes.erase (std::remove_if (notes.begin(), notes.end(), [&] (int note) { return note > inputNote; }), notes.end());
        notes.push_back (inputNote);
    }

    const auto minNote = juce::jlimit (0, 127, settings.minNote);
    const auto maxNote = juce::jlimit (minNote + 1, 127, settings.maxNote);
    for (auto& note : notes)
    {
        while (note < minNote)
            note += 12;
        while (note > maxNote)
            note -= 12;
    }

    std::sort (notes.begin(), notes.end());
    notes.erase (std::unique (notes.begin(), notes.end()), notes.end());

    if (requestedSize > static_cast<int> (notes.size()) && (settings.playability == Playability::orchestra || settings.playability == Playability::unrestricted || settings.playability == Playability::piano))
    {
        const auto minNote = juce::jlimit (0, 127, settings.minNote);
        const auto maxNote = juce::jlimit (minNote + 1, 127, settings.maxNote);
        auto expanded = notes;
        expanded.reserve (24);

        for (const auto baseNote : notes)
        {
            for (int octave = -3; octave <= 4; ++octave)
            {
                const auto candidate = baseNote + octave * 12;
                if (candidate < minNote || candidate > maxNote)
                    continue;

                if (std::find (expanded.begin(), expanded.end(), candidate) == expanded.end())
                    expanded.push_back (candidate);

                if (static_cast<int> (expanded.size()) >= requestedSize)
                    break;
            }

            if (static_cast<int> (expanded.size()) >= requestedSize)
                break;
        }

        std::sort (expanded.begin(), expanded.end());
        if (static_cast<int> (expanded.size()) > requestedSize)
        {
            const auto eraseCount = static_cast<int> (expanded.size()) - requestedSize;
            expanded.erase (expanded.begin(), expanded.begin() + juce::jlimit (0, static_cast<int> (expanded.size()), eraseCount / 2));
            while (static_cast<int> (expanded.size()) > requestedSize)
                expanded.pop_back();
        }

        notes = std::move (expanded);
    }

    if (settings.playability == Playability::guitar)
    {
        for (size_t i = 1; i < notes.size(); ++i)
            if (notes[i] - notes[i - 1] > 12)
                notes[i] -= 12;
        std::sort (notes.begin(), notes.end());
    }

    return notes;
}

float ChordEngine::scoreCandidate (const Candidate& candidate, int inputNote, const Settings& settings) const
{
    auto score = 100.0f;
    const auto keyMask = clampMask (settings.keyMask, keyCount);

    if (chordMostlyInScale (candidate.root, *candidate.type, settings))
        score += 40.0f;
    else
        score += settings.outside * 55.0f - 25.0f;

    auto keyDistance = 6;
    for (int key = 0; key < 12; ++key)
    {
        if (! maskContains (keyMask, key))
            continue;
        const auto up = mod12 (candidate.root - key);
        keyDistance = std::min (keyDistance, std::min (up, 12 - up));
    }

    score -= static_cast<float> (keyDistance) * 0.8f;
    if (maskContains (keyMask, candidate.root))
        score += 9.0f + settings.outside * 6.0f;
    if (keyDistance == 3 || keyDistance == 4)
        score += settings.outside * 8.0f;

    score -= std::abs (static_cast<int> (candidate.voiced.size()) - settings.chordSize) * 7.0f;
    score -= averageMotion (candidate.voiced, previousVoicing) * settings.voiceLeading;
    score -= static_cast<float> (candidate.type->complexity) * (1.0f - settings.complexity) * 12.0f;

    if (settings.style == Style::bigBand && candidate.voiced.back() == inputNote)
        score += 20.0f;
    const auto suffix = juce::String (candidate.type->suffix);

    if (settings.style == Style::quartalColor && suffix == "sus11")
        score += 28.0f;
    if (settings.style == Style::classical && candidate.type->complexity <= 2)
        score += 22.0f;
    if (settings.style == Style::gospel && (suffix == "6" || suffix == "13"))
        score += 18.0f;
    if (settings.style == Style::modernOutside)
        score += static_cast<float> (candidate.type->colour) * 8.0f + settings.outside * 30.0f;
    if (settings.style == Style::modalFilm)
        score += (suffix == "maj9" || suffix == "m9" || suffix == "sus11") ? 18.0f : 0.0f;
    if (settings.style == Style::chromaticMediant)
        score += (keyDistance == 3 || keyDistance == 4) ? 24.0f : 0.0f;
    if (settings.style == Style::baroqueCounterpoint)
        score += candidate.type->complexity <= 1 ? 24.0f : -8.0f;
    if (settings.style == Style::neoSoul)
        score += (suffix == "m9" || suffix == "maj9" || suffix == "13" || suffix == "m11") ? 24.0f : 0.0f;
    if (settings.style == Style::progressiveRock)
        score += (suffix == "sus11" || suffix == "7b9#11" || suffix == "maj13#11") ? 22.0f : 0.0f;

    return score;
}

int ChordEngine::chooseWeightedIndex (const std::vector<Candidate>& candidates, const Settings& settings)
{
    const auto poolSize = juce::jlimit (1, static_cast<int> (candidates.size()), 1 + static_cast<int> (std::round (settings.variation * 10.0f)));
    std::vector<double> weights;
    weights.reserve (static_cast<size_t> (poolSize));

    const auto best = candidates.front().score;
    for (int i = 0; i < poolSize; ++i)
    {
        const auto scoreDelta = best - candidates[static_cast<size_t> (i)].score;
        weights.push_back (std::exp (-scoreDelta / juce::jmax (2.0f, 8.0f + settings.variation * 35.0f)));
    }

    std::discrete_distribution<int> pick (weights.begin(), weights.end());
    return pick (rng);
}

int ChordEngine::nearestChordToneIndex (const std::vector<int>& intervals, int pitchClassFromRoot) const
{
    auto bestIndex = 0;
    auto bestDistance = std::numeric_limits<int>::max();
    for (int i = 0; i < static_cast<int> (intervals.size()); ++i)
    {
        const auto up = mod12 (intervals[static_cast<size_t> (i)] - pitchClassFromRoot);
        const auto distance = std::min (up, 12 - up);
        if (distance < bestDistance)
        {
            bestDistance = distance;
            bestIndex = i;
        }
    }
    return bestIndex;
}

bool ChordEngine::pitchInAnyScale (int pitchClass, int root, const Settings& settings) const
{
    const auto scaleMask = clampMask (settings.scaleMask, scaleCount);
    for (int i = 0; i < scaleCount; ++i)
    {
        if (! maskContains (scaleMask, i))
            continue;

        const auto interval = mod12 (pitchClass - root);
        const auto& intervals = scaleIntervals (static_cast<ScaleType> (i));
        if (std::find (intervals.begin(), intervals.end(), interval) != intervals.end())
            return true;
    }

    return false;
}

bool ChordEngine::chordMostlyInScale (int root, const ChordType& type, const Settings& settings) const
{
    auto inside = 0;
    for (const auto interval : type.intervals)
        if (pitchInAnyScale (root + interval, root, settings))
            ++inside;

    const auto threshold = settings.outside > 0.55f ? 0.45f : 0.67f;
    return static_cast<float> (inside) / static_cast<float> (type.intervals.size()) >= threshold;
}

juce::String ChordEngine::chordName (int root, const ChordType& type) const
{
    return pcName (root) + chordSuffixName (type.suffix);
}
}
