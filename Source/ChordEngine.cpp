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

enum class ParsedQuality
{
    major,
    minor,
    dominant,
    diminished,
    suspended,
    unknown
};

struct ParsedContextChord
{
    bool valid = false;
    int root = 0;
    int bass = -1;
    juce::String suffix;
    juce::String displayName;
    std::vector<int> intervals;
    ParsedQuality quality = ParsedQuality::unknown;
};

int parsePitchClass (juce::String text)
{
    text = text.trim();
    if (text.isEmpty())
        return -1;

    const auto letter = juce::CharacterFunctions::toUpperCase (text[0]);
    auto pitch = letter == 'C' ? 0
               : letter == 'D' ? 2
               : letter == 'E' ? 4
               : letter == 'F' ? 5
               : letter == 'G' ? 7
               : letter == 'A' ? 9
               : letter == 'B' ? 11 : -100;
    if (pitch < 0)
        return -1;
    if (text.length() > 1 && (text[1] == '#' || text[1] == 'b'))
        pitch += text[1] == '#' ? 1 : -1;
    return mod12 (pitch);
}

ParsedContextChord parseContextChord (juce::String name)
{
    ParsedContextChord result;
    name = name.trim();
    if (name.isEmpty() || name == "--" || name == "N.C.")
        return result;

    auto rootLength = 1;
    if (name.length() > 1 && (name[1] == '#' || name[1] == 'b'))
        rootLength = 2;
    result.root = parsePitchClass (name.substring (0, rootLength));
    if (result.root < 0)
        return result;

    const auto slash = name.indexOfChar ('/');
    auto suffix = slash >= 0 ? name.substring (rootLength, slash) : name.substring (rootLength);
    suffix = suffix.trim().replace ("min", "m").replace ("Major", "maj").replace ("major", "maj");
    if (slash >= 0)
        result.bass = parsePitchClass (name.substring (slash + 1));

    struct Definition
    {
        const char* token;
        std::initializer_list<int> intervals;
        ParsedQuality quality;
    };

    static const std::array<Definition, 34> definitions
    {{
        { "maj13#11", { 0, 4, 7, 11, 14, 18, 21 }, ParsedQuality::major },
        { "maj9#11",  { 0, 4, 7, 11, 14, 18 }, ParsedQuality::major },
        { "mMaj9",    { 0, 3, 7, 11, 14 }, ParsedQuality::minor },
        { "mMaj7",    { 0, 3, 7, 11 }, ParsedQuality::minor },
        { "m13",      { 0, 3, 7, 10, 14, 17, 21 }, ParsedQuality::minor },
        { "m11",      { 0, 3, 7, 10, 14, 17 }, ParsedQuality::minor },
        { "maj13",    { 0, 4, 7, 11, 14, 21 }, ParsedQuality::major },
        { "maj9",     { 0, 4, 7, 11, 14 }, ParsedQuality::major },
        { "13sus4",   { 0, 5, 7, 10, 14, 21 }, ParsedQuality::suspended },
        { "9sus4",    { 0, 5, 7, 10, 14 }, ParsedQuality::suspended },
        { "7sus4",    { 0, 5, 7, 10 }, ParsedQuality::suspended },
        { "m7b5",     { 0, 3, 6, 10 }, ParsedQuality::diminished },
        { "dim7",     { 0, 3, 6, 9 }, ParsedQuality::diminished },
        { "7b9#11",   { 0, 4, 7, 10, 13, 18 }, ParsedQuality::dominant },
        { "7#9",      { 0, 4, 7, 10, 15 }, ParsedQuality::dominant },
        { "7b9",      { 0, 4, 7, 10, 13 }, ParsedQuality::dominant },
        { "7#11",     { 0, 4, 7, 10, 18 }, ParsedQuality::dominant },
        { "7b13",     { 0, 4, 7, 10, 20 }, ParsedQuality::dominant },
        { "6/9",      { 0, 4, 7, 9, 14 }, ParsedQuality::major },
        { "m6/9",     { 0, 3, 7, 9, 14 }, ParsedQuality::minor },
        { "add9",     { 0, 4, 7, 14 }, ParsedQuality::major },
        { "m(add9)",  { 0, 3, 7, 14 }, ParsedQuality::minor },
        { "maj7",     { 0, 4, 7, 11 }, ParsedQuality::major },
        { "m9",       { 0, 3, 7, 10, 14 }, ParsedQuality::minor },
        { "13",       { 0, 4, 7, 10, 14, 21 }, ParsedQuality::dominant },
        { "9",        { 0, 4, 7, 10, 14 }, ParsedQuality::dominant },
        { "m7",       { 0, 3, 7, 10 }, ParsedQuality::minor },
        { "7",        { 0, 4, 7, 10 }, ParsedQuality::dominant },
        { "dim",      { 0, 3, 6 }, ParsedQuality::diminished },
        { "aug",      { 0, 4, 8 }, ParsedQuality::major },
        { "sus4",     { 0, 5, 7 }, ParsedQuality::suspended },
        { "sus2",     { 0, 2, 7 }, ParsedQuality::suspended },
        { "m",        { 0, 3, 7 }, ParsedQuality::minor },
        { "",         { 0, 4, 7 }, ParsedQuality::major }
    }};

    const Definition* matched = &definitions.back();
    for (const auto& definition : definitions)
    {
        if (suffix == definition.token)
        {
            matched = &definition;
            break;
        }
    }

    result.valid = true;
    result.suffix = suffix;
    result.displayName = name;
    result.intervals.assign (matched->intervals.begin(), matched->intervals.end());
    result.quality = matched->quality;
    if (result.bass >= 0)
    {
        const auto bassInterval = mod12 (result.bass - result.root);
        if (std::none_of (result.intervals.begin(), result.intervals.end(),
                          [bassInterval] (int interval) { return mod12 (interval) == bassInterval; }))
            result.intervals.insert (result.intervals.begin(), bassInterval);
    }
    return result;
}

int inferMajorTonic (const ParsedContextChord& previous,
                     const ParsedContextChord& current,
                     const ParsedContextChord& next)
{
    static const std::array<int, 7> degreeRoots { 0, 2, 4, 5, 7, 9, 11 };
    static const std::array<ParsedQuality, 7> degreeQualities
    {
        ParsedQuality::major, ParsedQuality::minor, ParsedQuality::minor,
        ParsedQuality::major, ParsedQuality::dominant, ParsedQuality::minor,
        ParsedQuality::diminished
    };

    auto bestTonic = current.valid ? current.root : 0;
    auto bestScore = std::numeric_limits<float>::lowest();
    for (int tonic = 0; tonic < 12; ++tonic)
    {
        auto score = 0.0f;
        const auto scoreChord = [&] (const ParsedContextChord& chord, float weight)
        {
            if (! chord.valid)
                return;
            for (std::size_t degree = 0; degree < degreeRoots.size(); ++degree)
            {
                if (mod12 (chord.root - tonic) != degreeRoots[degree])
                    continue;
                score += weight * 4.0f;
                if (chord.quality == degreeQualities[degree]
                    || (degree == 4 && chord.quality == ParsedQuality::major))
                    score += weight * 3.0f;
                return;
            }
            score -= weight * 1.5f;
        };

        scoreChord (previous, 0.75f);
        scoreChord (current, 1.5f);
        scoreChord (next, 1.0f);
        if (current.valid && tonic == current.root)
            score += 0.5f;
        if (score > bestScore)
        {
            bestScore = score;
            bestTonic = tonic;
        }
    }
    return bestTonic;
}

bool containsPitchClass (const ParsedContextChord& chord, int pitch)
{
    return std::any_of (chord.intervals.begin(), chord.intervals.end(), [&] (int interval)
    {
        return mod12 (chord.root + interval) == mod12 (pitch);
    });
}

ParsedContextChord makeContextChord (int root,
                                     juce::String suffix,
                                     std::initializer_list<int> intervals,
                                     ParsedQuality quality)
{
    ParsedContextChord result;
    result.valid = true;
    result.root = mod12 (root);
    result.suffix = suffix;
    result.displayName = pcName (result.root) + suffix;
    result.intervals.assign (intervals.begin(), intervals.end());
    result.quality = quality;
    return result;
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

juce::StringArray ChordEngine::contextModeNames()
{
    return { "Match Chord", "Diatonic", "Substitutions", "Adaptive" };
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

GeneratedChord ChordEngine::generateForContext (int inputNote,
                                                const juce::String& currentChord,
                                                const juce::String& previousContext,
                                                const juce::String& nextChord,
                                                const Settings& settings)
{
    const auto current = parseContextChord (currentChord);
    if (! current.valid)
        return generate (inputNote, 100, settings);

    const auto previous = parseContextChord (previousContext);
    const auto next = parseContextChord (nextChord);
    const auto tonic = inferMajorTonic (previous, current, next);

    struct ContextCandidate
    {
        ParsedContextChord chord;
        float relationScore = 0.0f;
    };

    std::vector<ContextCandidate> candidates;
    const auto addCandidate = [&] (ParsedContextChord chord, float relation)
    {
        const auto duplicate = std::any_of (candidates.begin(), candidates.end(), [&] (const auto& item)
        {
            return item.chord.root == chord.root && item.chord.suffix == chord.suffix;
        });
        if (! duplicate)
            candidates.push_back ({ std::move (chord), relation });
    };

    const auto directScore = settings.contextMode == ContextMode::exact ? 100.0f
                           : settings.contextMode == ContextMode::adaptive ? 88.0f : 80.0f;
    addCandidate (current, directScore);
    const auto includeDiatonic = settings.contextMode == ContextMode::diatonic
                              || settings.contextMode == ContextMode::adaptive;
    const auto includeSubstitutions = settings.contextMode == ContextMode::substitutions
                                   || settings.contextMode == ContextMode::adaptive;

    if (includeDiatonic)
    {
        addCandidate (makeContextChord (tonic, "maj7", { 0, 4, 7, 11 }, ParsedQuality::major), 72.0f);
        addCandidate (makeContextChord (tonic + 2, "m7", { 0, 3, 7, 10 }, ParsedQuality::minor), 68.0f);
        addCandidate (makeContextChord (tonic + 4, "m7", { 0, 3, 7, 10 }, ParsedQuality::minor), 65.0f);
        addCandidate (makeContextChord (tonic + 5, "maj7", { 0, 4, 7, 11 }, ParsedQuality::major), 69.0f);
        addCandidate (makeContextChord (tonic + 7, "7", { 0, 4, 7, 10 }, ParsedQuality::dominant), 71.0f);
        addCandidate (makeContextChord (tonic + 9, "m7", { 0, 3, 7, 10 }, ParsedQuality::minor), 67.0f);
        addCandidate (makeContextChord (tonic + 11, "m7b5", { 0, 3, 6, 10 }, ParsedQuality::diminished), 58.0f);
    }

    if (includeSubstitutions)
    {
        const auto depth = settings.substitutionDepth;
        if (current.quality == ParsedQuality::major || current.quality == ParsedQuality::dominant)
            addCandidate (makeContextChord (current.root + 9, "m7", { 0, 3, 7, 10 }, ParsedQuality::minor), 57.0f + depth * 12.0f);
        if (current.quality == ParsedQuality::minor)
            addCandidate (makeContextChord (current.root + 3, "maj7", { 0, 4, 7, 11 }, ParsedQuality::major), 59.0f + depth * 12.0f);
        if (current.quality == ParsedQuality::dominant)
            addCandidate (makeContextChord (current.root + 6, "7", { 0, 4, 7, 10 }, ParsedQuality::dominant), 60.0f + depth * 18.0f);

        addCandidate (makeContextChord (current.root + 7, "7", { 0, 4, 7, 10 }, ParsedQuality::dominant), 50.0f + depth * 17.0f);
        addCandidate (makeContextChord (current.root + 2, "m7", { 0, 3, 7, 10 }, ParsedQuality::minor), 52.0f + depth * 14.0f);
        addCandidate (makeContextChord (current.root + 10, "7", { 0, 4, 7, 10 }, ParsedQuality::dominant), 48.0f + depth * 18.0f);
        addCandidate (makeContextChord (current.root + 1, "dim7", { 0, 3, 6, 9 }, ParsedQuality::diminished), 45.0f + depth * 20.0f);
    }

    struct ScoredVoicing
    {
        GeneratedChord generated;
        float score = 0.0f;
    };

    std::vector<ScoredVoicing> scored;
    scored.reserve (candidates.size());
    const auto inputPitch = mod12 (inputNote);
    static const std::array<int, 7> majorScale { 0, 2, 4, 5, 7, 9, 11 };
    const auto inputInContextScale = std::find (majorScale.begin(), majorScale.end(),
                                                mod12 (inputPitch - tonic)) != majorScale.end();

    for (const auto& candidate : candidates)
    {
        ChordType type;
        const auto suffixUtf8 = candidate.chord.suffix.toRawUTF8();
        juce::ignoreUnused (suffixUtf8);
        type.suffix = "";
        type.intervals = candidate.chord.intervals;
        type.complexity = juce::jlimit (0, 5, static_cast<int> (type.intervals.size()) - 3);

        auto notes = voiceCandidate (inputNote, candidate.chord.root, type, settings, resolveRole (settings));
        if (std::find (notes.begin(), notes.end(), inputNote) == notes.end())
            notes.push_back (inputNote);
        std::sort (notes.begin(), notes.end());
        notes.erase (std::unique (notes.begin(), notes.end()), notes.end());

        auto score = candidate.relationScore;
        if (containsPitchClass (candidate.chord, inputPitch))
            score += 38.0f;
        else if (inputInContextScale)
            score += 15.0f + settings.complexity * 8.0f;
        else
            score += settings.outside * 20.0f - 16.0f;
        score -= averageMotion (notes, previousVoicing) * settings.voiceLeading * 0.85f;
        score -= std::abs (static_cast<int> (notes.size()) - settings.chordSize) * 2.5f;
        if (candidate.chord.root == current.root && candidate.chord.suffix == current.suffix)
            score += (1.0f - settings.substitutionDepth) * 24.0f;

        scored.push_back ({ { std::move (notes), candidate.chord.displayName }, score });
    }

    std::sort (scored.begin(), scored.end(), [] (const auto& a, const auto& b) { return a.score > b.score; });
    const auto pool = juce::jlimit (1, static_cast<int> (scored.size()),
                                    1 + static_cast<int> (std::round (settings.variation * 4.0f
                                                                    + settings.substitutionDepth * 3.0f)));
    auto chosen = 0;
    if (pool > 1)
    {
        std::vector<double> weights;
        for (int i = 0; i < pool; ++i)
            weights.push_back (std::exp ((scored[static_cast<std::size_t> (i)].score - scored.front().score) / 12.0f));
        std::discrete_distribution<int> pick (weights.begin(), weights.end());
        chosen = pick (rng);
    }

    auto result = scored[static_cast<std::size_t> (chosen)].generated;
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
