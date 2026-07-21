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

// Each arranging style owns its useful amount of harmonic choice. This keeps
// conservative styles dependable and gives colour-focused styles room to
// explore without asking the player to manage a separate Variation control.
float styleVariation (Style style) noexcept
{
    switch (style)
    {
        case Style::simpleScale:          return 0.00f;
        case Style::closeLead:            return 0.16f;
        case Style::bigBand:              return 0.20f;
        case Style::quartalColor:         return 0.38f;
        case Style::classical:            return 0.12f;
        case Style::gospel:               return 0.30f;
        case Style::modernOutside:        return 0.62f;
        case Style::modalFilm:            return 0.42f;
        case Style::chromaticMediant:     return 0.52f;
        case Style::baroqueCounterpoint:  return 0.18f;
        case Style::neoSoul:              return 0.48f;
        case Style::progressiveRock:      return 0.44f;
        case Style::openFifths:           return 0.10f;
        case Style::dropTwo:              return 0.20f;
        case Style::ambientSpread:        return 0.42f;
        case Style::latinShells:          return 0.20f;
        case Style::openTriads:           return 0.14f;
        case Style::powerStack:           return 0.12f;
        case Style::dropThree:            return 0.22f;
        case Style::spreadTenths:         return 0.20f;
        case Style::clusterCloud:         return 0.58f;
        case Style::pedalPoint:           return 0.18f;
        case Style::gospelShout:          return 0.34f;
        case Style::jazzShells:           return 0.24f;
        case Style::orchestralLush:       return 0.36f;
        case Style::guitarOpen:           return 0.16f;
        case Style::hornSoli:             return 0.20f;
        case Style::wholeToneDream:       return 0.60f;
    }

    return 0.25f;
}

float harmonicReach (const Settings& settings) noexcept
{
    // Outside is the direct chromatic-harmony control. Modulation reaches into
    // the same borrowed pool so it remains meaningful in every harmony style,
    // including a project with only one key and one scale selected.
    return juce::jlimit (0.0f, 1.0f, juce::jmax (settings.outside, settings.modulation * 0.90f));
}

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

bool pitchInSelectedKeyScale (int pitchClass, const Settings& settings)
{
    const auto keyMask = clampMask (settings.keyMask, keyCount);
    const auto scaleMask = clampMask (settings.scaleMask, scaleCount);
    for (int key = 0; key < keyCount; ++key)
    {
        if (! maskContains (keyMask, key))
            continue;
        for (int scale = 0; scale < scaleCount; ++scale)
        {
            if (! maskContains (scaleMask, scale))
                continue;
            const auto& intervals = scaleIntervals (static_cast<ScaleType> (scale));
            if (std::find (intervals.begin(), intervals.end(), mod12 (pitchClass - key)) != intervals.end())
                return true;
        }
    }
    return false;
}

const std::vector<ChordEngine::ChordType>& chordTypes()
{
    static const std::vector<ChordEngine::ChordType> types
    {
        { "",         { 0, 4, 7 },                0, 0 },
        { "m",        { 0, 3, 7 },                0, 0 },
        { "dim",      { 0, 3, 6 },                0, 1 },
        { "sus2",     { 0, 2, 7 },                0, 1 },
        { "sus4",     { 0, 5, 7 },                0, 1 },
        { "5",        { 0, 7 },                   0, 0 },
        { "aug",      { 0, 4, 8 },                1, 2 },
        { "cluster",  { 0, 1, 2, 7 },             2, 4 },
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

juce::String detailedChordName (const juce::String& fallbackName, const std::vector<int>& notes)
{
    if (fallbackName.isEmpty() || notes.empty())
        return fallbackName;

    const auto rootLength = fallbackName.length() > 1
                         && (fallbackName[1] == '#' || fallbackName[1] == 'b') ? 2 : 1;
    const auto root = parsePitchClass (fallbackName.substring (0, rootLength));
    if (root < 0)
        return fallbackName;

    std::vector<int> actual;
    actual.reserve (notes.size());
    for (const auto note : notes)
        actual.push_back (mod12 (note - root));
    std::sort (actual.begin(), actual.end());
    actual.erase (std::unique (actual.begin(), actual.end()), actual.end());

    struct Quality
    {
        const char* suffix;
        std::initializer_list<int> pitchClasses;
    };
    static const std::array<Quality, 45> qualities
    {{
        { "maj13",   { 0, 2, 4, 5, 7, 9, 11 } },
        { "13",      { 0, 2, 4, 5, 7, 9, 10 } },
        { "m13",     { 0, 2, 3, 5, 7, 9, 10 } },
        { "maj9#11", { 0, 2, 4, 6, 7, 11 } },
        { "9#11",    { 0, 2, 4, 6, 7, 10 } },
        { "m11",     { 0, 2, 3, 5, 7, 10 } },
        { "maj11",   { 0, 2, 4, 5, 7, 11 } },
        { "11",      { 0, 2, 4, 5, 7, 10 } },
        { "7b9b13",  { 0, 1, 4, 7, 8, 10 } },
        { "7#9b13",  { 0, 3, 4, 7, 8, 10 } },
        { "mMaj9",   { 0, 2, 3, 7, 11 } },
        { "maj9",    { 0, 2, 4, 7, 11 } },
        { "9",       { 0, 2, 4, 7, 10 } },
        { "m9",      { 0, 2, 3, 7, 10 } },
        { "m9b5",    { 0, 2, 3, 6, 10 } },
        { "7b9",     { 0, 1, 4, 7, 10 } },
        { "7#9",     { 0, 3, 4, 7, 10 } },
        { "7b13",    { 0, 4, 7, 8, 10 } },
        { "6/9",     { 0, 2, 4, 7, 9 } },
        { "m6/9",    { 0, 2, 3, 7, 9 } },
        { "maj7#11", { 0, 4, 6, 7, 11 } },
        { "7#11",    { 0, 4, 6, 7, 10 } },
        { "maj7#5",  { 0, 4, 8, 11 } },
        { "maj7b5",  { 0, 4, 6, 11 } },
        { "7#5",     { 0, 4, 8, 10 } },
        { "7b5",     { 0, 4, 6, 10 } },
        { "mMaj7",   { 0, 3, 7, 11 } },
        { "m7b5",    { 0, 3, 6, 10 } },
        { "dim7",    { 0, 3, 6, 9 } },
        { "maj7",    { 0, 4, 7, 11 } },
        { "7",       { 0, 4, 7, 10 } },
        { "m7",      { 0, 3, 7, 10 } },
        { "add9",    { 0, 2, 4, 7 } },
        { "madd9",   { 0, 2, 3, 7 } },
        { "6",       { 0, 4, 7, 9 } },
        { "m6",      { 0, 3, 7, 9 } },
        { "7sus4",   { 0, 5, 7, 10 } },
        { "cluster", { 0, 1, 2, 7 } },
        { "",        { 0, 4, 7 } },
        { "m",       { 0, 3, 7 } },
        { "dim",     { 0, 3, 6 } },
        { "aug",     { 0, 4, 8 } },
        { "sus2",    { 0, 2, 7 } },
        { "sus4",    { 0, 5, 7 } },
        { "5",       { 0, 7 } }
    }};

    auto name = fallbackName;
    for (const auto& quality : qualities)
    {
        std::vector<int> recognized (quality.pitchClasses);
        if (recognized == actual)
        {
            name = pcName (root) + quality.suffix;
            break;
        }
    }

    const auto bass = mod12 (notes.front());
    if (bass != root)
        name += "/" + pcName (bass);
    return name;
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

struct DiatonicChord
{
    int root = 0;
    const char* suffix = "";
    std::vector<int> intervals;
};

DiatonicChord makeDiatonicChord (int key, ScaleType scale, int degree, bool useSeventh)
{
    const auto& scaleNotes = scaleIntervals (scale);
    degree = juce::jlimit (0, static_cast<int> (scaleNotes.size()) - 1, degree);
    const auto rootAbsolute = key + scaleNotes[static_cast<std::size_t> (degree)];
    const auto voiceCount = useSeventh ? 4 : 3;

    DiatonicChord result;
    result.root = mod12 (rootAbsolute);
    result.intervals.reserve (static_cast<std::size_t> (voiceCount));
    for (int voice = 0; voice < voiceCount; ++voice)
    {
        const auto scaleIndex = degree + voice * 2;
        const auto octave = scaleIndex / static_cast<int> (scaleNotes.size());
        const auto pitch = key + scaleNotes[static_cast<std::size_t> (scaleIndex % static_cast<int> (scaleNotes.size()))]
                         + octave * 12;
        result.intervals.push_back (pitch - rootAbsolute);
    }

    const auto pitchClassAt = [&] (std::size_t index) { return mod12 (result.intervals[index]); };
    const auto third = pitchClassAt (1);
    const auto fifth = pitchClassAt (2);
    if (! useSeventh)
        result.suffix = third == 3 && fifth == 6 ? "dim"
                      : third == 3 && fifth == 7 ? "m"
                      : third == 4 && fifth == 8 ? "aug" : "";
    else
    {
        const auto seventh = pitchClassAt (3);
        result.suffix = third == 4 && fifth == 7 && seventh == 11 ? "maj7"
                      : third == 3 && fifth == 7 && seventh == 10 ? "m7"
                      : third == 4 && fifth == 7 && seventh == 10 ? "7"
                      : third == 3 && fifth == 6 && seventh == 10 ? "m7b5"
                      : third == 3 && fifth == 6 && seventh == 9 ? "dim7"
                      : third == 3 && fifth == 7 && seventh == 11 ? "mMaj7" : "7";
    }

    // The requested Simple major-scale leading-tone chord is a fully
    // diminished seventh rather than the usual half-diminished seventh.
    if (useSeventh && scale == ScaleType::ionian
        && degree == static_cast<int> (scaleNotes.size()) - 1)
    {
        result.intervals = { 0, 3, 6, 9 };
        result.suffix = "dim7";
    }
    return result;
}

bool chordContainsPitchClass (const DiatonicChord& chord, int pitchClass)
{
    return std::any_of (chord.intervals.begin(), chord.intervals.end(), [&] (int interval)
    {
        return mod12 (chord.root + interval) == mod12 (pitchClass);
    });
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
    return { "Simple Scale Chords", "Close Lead", "Big Band", "Quartal Color", "Classical", "Gospel", "Modern Outside", "Modal Film", "Chromatic Mediant", "Baroque Counterpoint", "Neo-Soul", "Progressive Rock", "Open Fifths", "Drop 2", "Ambient Spread", "Latin Shells", "Open Triads", "Power Stack", "Drop 3", "Spread Tenths", "Cluster Cloud", "Pedal Point", "Gospel Shout", "Jazz Shells", "Orchestral Lush", "Guitar Open", "Horn Soli", "Whole-Tone Dream" };
}

juce::StringArray ChordEngine::playabilityNames()
{
    return { "Piano", "Guitar", "Horn Section", "Orchestra", "Unrestricted",
             "Sax Soli", "Brass Section", "SATB Voices", "Strings", "Piano Hands",
             "Low Reeds", "Synth Stack" };
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
    previousScaleDegree = -1;
    previousInputNote = -1;
    phrasePosition = 0;
    phraseKey = -1;
    phraseScale = -1;
    registerCentre = 66.0f;
    recentInputNotes.clear();
    recentScaleDegrees.clear();
    hasPerInputChord.fill (false);
    freeChoiceCounter = 0;
}

GeneratedChord ChordEngine::generate (int inputNote, int, const Settings& settings)
{
    inputNote = juce::jlimit (0, 127, inputNote);
    const auto choicePosition = settings.phraseMemory ? static_cast<juce::uint32> (phrasePosition)
                                                      : ++freeChoiceCounter;
    const auto deterministicSeed = static_cast<juce::uint32> (0x5a17c0deu)
                                 ^ choicePosition * 0x85ebca6bu
                                 ^ static_cast<juce::uint32> (inputNote) * 0xc2b2ae35u;
    rng.seed (deterministicSeed);

    const auto noteIndex = static_cast<std::size_t> (inputNote);
    if (hasPerInputChord[noteIndex] && settings.repeatChance > 0.0f)
    {
        std::uniform_real_distribution<float> repeat (0.0f, 1.0f);
        if (settings.repeatChance >= 0.999f || repeat (rng) < settings.repeatChance)
        {
            const auto cached = perInputChords[noteIndex];
            previousVoicing = cached.notes;
            previousChord = cached;
            previousInputNote = inputNote;
            ++phrasePosition;
            return cached;
        }
    }

    if (settings.style == Style::simpleScale)
        return generateSimpleScaleChord (inputNote, settings);
    if (settings.style == Style::closeLead || settings.style == Style::bigBand)
        return generateLeadSoliChord (inputNote, settings, settings.style == Style::bigBand);

    const auto role = resolveRole (settings);
    auto candidates = buildCandidates (inputNote, settings, role);

    if (candidates.empty())
    {
        GeneratedChord fallback;
        fallback.notes = { inputNote };
        fallback.name = pcName (inputNote);
        previousVoicing = fallback.notes;
        previousChord = fallback;
        perInputChords[noteIndex] = fallback;
        hasPerInputChord[noteIndex] = true;
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
    return finishGeneratedChord (std::move (result), inputNote, settings, false, -1, false);
}

GeneratedChord ChordEngine::generateSimpleScaleChord (int inputNote, const Settings& settings)
{
    const auto key = choosePrimaryKey (settings, inputNote);
    const auto scale = choosePrimaryScale (settings, key, inputNote);
    const auto& scaleNotes = scaleIntervals (scale);
    const auto inputPitchClass = mod12 (inputNote);
    const auto scaleDegree = [&]
    {
        const auto relative = mod12 (inputPitchClass - key);
        const auto found = std::find (scaleNotes.begin(), scaleNotes.end(), relative);
        return found == scaleNotes.end() ? -1 : static_cast<int> (std::distance (scaleNotes.begin(), found));
    }();
    const auto useSeventh = settings.chordSize >= 4;

    // Chromatic keys are useful as connective harmony in Simple mode. At zero
    // Modulation they remain dependable fully-diminished passing chords. As
    // Modulation rises, a performed chromatic root can instead borrow a basic
    // seventh quality whose tones and voice movement fit the selected harmony.
    if (scaleDegree < 0)
    {
        ChordType passing { "dim7", { 0, 3, 6, 9 }, 1, 0 };
        if (settings.modulation > 0.0f)
        {
            std::uniform_real_distribution<float> modulationChance (0.0f, 1.0f);
            if (modulationChance (rng) < settings.modulation)
            {
                const std::array<ChordType, 4> alternatives
                {{
                    { "m7b5", { 0, 3, 6, 10 }, 1, 1 },
                    { "m7",   { 0, 3, 7, 10 }, 1, 1 },
                    { "maj7", { 0, 4, 7, 11 }, 1, 1 },
                    { "7",    { 0, 4, 7, 10 }, 1, 1 }
                }};

                const auto toneIsInSelectedHarmony = [&] (int pitchClass)
                {
                    return std::find (scaleNotes.begin(), scaleNotes.end(),
                                      mod12 (pitchClass - key)) != scaleNotes.end();
                };
                auto bestScore = std::numeric_limits<float>::lowest();
                auto bestIndex = 0;
                for (int index = 0; index < static_cast<int> (alternatives.size()); ++index)
                {
                    const auto& alternative = alternatives[static_cast<std::size_t> (index)];
                    auto insideTones = 0;
                    for (const auto interval : alternative.intervals)
                        if (toneIsInSelectedHarmony (inputPitchClass + interval))
                            ++insideTones;

                    auto previewSettings = settings;
                    previewSettings.chordSize = juce::jmax (4, settings.chordSize);
                    const auto preview = voiceCandidate (inputNote, inputPitchClass, alternative,
                                                         previewSettings, NoteRole::root);
                    auto score = static_cast<float> (insideTones) * 18.0f;
                    if (settings.phraseMemory)
                        score -= averageMotion (preview, previousVoicing) * settings.voiceLeading;

                    // Half-diminished is especially effective one semitone
                    // below a diatonic destination; the rotating preference
                    // keeps higher settings from collapsing to one color.
                    const auto nextScaleTone = std::any_of (scaleNotes.begin(), scaleNotes.end(), [&] (int interval)
                    {
                        return mod12 (key + interval - inputPitchClass) == 1;
                    });
                    if (index == 0 && nextScaleTone)
                        score += 18.0f;
                    const auto rotatingFlavor = mod12 (inputPitchClass + phrasePosition
                                                      + static_cast<int> (freeChoiceCounter)) % 4;
                    if (index == rotatingFlavor)
                        score += settings.modulation * 14.0f;
                    std::uniform_real_distribution<float> noise (0.0f,
                        styleVariation (settings.style) * settings.modulation * 12.0f);
                    score += noise (rng);
                    if (score > bestScore)
                    {
                        bestScore = score;
                        bestIndex = index;
                    }
                }
                passing = alternatives[static_cast<std::size_t> (bestIndex)];
            }
        }
        auto passingSettings = settings;
        passingSettings.chordSize = juce::jmax (4, settings.chordSize);
        GeneratedChord result {
            voiceCandidate (inputNote, inputPitchClass, passing, passingSettings, NoteRole::root),
            pcName (inputPitchClass) + passing.suffix
        };
        return finishGeneratedChord (std::move (result), inputNote, settings, false, -1, true);
    }

    struct Choice
    {
        int degree = 0;
        DiatonicChord chord;
        std::vector<int> notes;
        float score = std::numeric_limits<float>::lowest();
    };
    std::vector<Choice> choices;

    const auto addDegree = [&] (int degree)
    {
        const auto chord = makeDiatonicChord (key, scale, degree, useSeventh);
        ChordType type { chord.suffix, chord.intervals, useSeventh ? 1 : 0, 0 };
        auto notes = voiceCandidate (inputNote, chord.root, type, settings, NoteRole::root);
        auto score = 100.0f - (settings.phraseMemory ? averageMotion (notes, previousVoicing) * settings.voiceLeading : 0.0f);
        if (degree == 0 || degree == 3 || degree == 4)
            score += 7.0f;
        const auto suffix = juce::String (chord.suffix);
        if (suffix == "maj7" || suffix == "m7" || suffix == "7" || suffix == "dim7")
            score += 14.0f;
        score -= repeatAvoidancePenalty (inputNote, pcName (chord.root) + chord.suffix, settings);
        choices.push_back ({ degree, chord, std::move (notes), score });
    };

    addDegree (scaleDegree);

    // Modulation can deliberately reharmonize an in-scale trigger too. At high
    // values the borrowed quality is required to contain at least one tone
    // outside the selected scale, while the played note remains its root.
    if (settings.modulation > 0.0f)
    {
        std::uniform_real_distribution<float> borrowChance (0.0f, 1.0f);
        if (borrowChance (rng) < settings.modulation)
        {
            const std::array<ChordType, 5> borrowedTypes
            {{
                { "maj7", { 0, 4, 7, 11 }, 1, 1 },
                { "m7",   { 0, 3, 7, 10 }, 1, 1 },
                { "7",    { 0, 4, 7, 10 }, 1, 1 },
                { "m7b5", { 0, 3, 6, 10 }, 1, 2 },
                { "dim7", { 0, 3, 6, 9 },  1, 2 }
            }};
            for (const auto& borrowedType : borrowedTypes)
            {
                if (juce::String (borrowedType.suffix) == choices.front().chord.suffix)
                    continue;
                auto outsideTones = 0;
                for (const auto interval : borrowedType.intervals)
                    if (! pitchInSelectedKeyScale (inputPitchClass + interval, settings))
                        ++outsideTones;
                if (outsideTones == 0)
                    continue;

                auto notes = voiceCandidate (inputNote, inputPitchClass, borrowedType, settings, NoteRole::root);
                auto score = 82.0f + settings.modulation * (42.0f + outsideTones * 24.0f);
                if (settings.phraseMemory)
                    score -= averageMotion (notes, previousVoicing) * settings.voiceLeading * 0.55f;
                score -= repeatAvoidancePenalty (inputNote,
                    pcName (inputPitchClass) + borrowedType.suffix, settings);
                choices.push_back ({ scaleDegree,
                    { inputPitchClass, borrowedType.suffix, borrowedType.intervals },
                    std::move (notes), score });
            }
        }
    }

    if (choices.empty())
    {
        const auto fallbackDegree = scaleDegree;
        const auto chord = makeDiatonicChord (key, scale, fallbackDegree, useSeventh);
        ChordType type { chord.suffix, chord.intervals, useSeventh ? 1 : 0, 0 };
        choices.push_back ({ fallbackDegree, chord,
                             voiceCandidate (inputNote, chord.root, type, settings, NoteRole::root), 0.0f });
    }

    const auto chosen = std::max_element (choices.begin(), choices.end(), [] (const auto& left, const auto& right)
    {
        return left.score < right.score;
    });
    GeneratedChord result { chosen->notes, pcName (chosen->chord.root) + chosen->chord.suffix };
    return finishGeneratedChord (std::move (result), inputNote, settings, false,
                                 chosen->degree, false);
}

GeneratedChord ChordEngine::generateLeadSoliChord (int inputNote, const Settings& settings, bool bigBand)
{
    const auto key = choosePrimaryKey (settings, inputNote);
    const auto scale = choosePrimaryScale (settings, key, inputNote);
    const auto& scaleNotes = scaleIntervals (scale);
    const auto inputPitchClass = mod12 (inputNote);
    const auto inputIsInScale = std::find (scaleNotes.begin(), scaleNotes.end(),
                                           mod12 (inputPitchClass - key)) != scaleNotes.end();

    struct SoliChoice
    {
        int degree = 0;
        DiatonicChord chord;
        std::vector<int> notes;
        float score = std::numeric_limits<float>::lowest();
    };
    std::vector<SoliChoice> choices;
    for (int degree = 0; degree < static_cast<int> (scaleNotes.size()); ++degree)
    {
        const auto chord = makeDiatonicChord (key, scale, degree, true);
        const auto containsMelody = chordContainsPitchClass (chord, inputPitchClass);
        if (inputIsInScale && ! containsMelody)
            continue;

        auto soliSettings = settings;
        soliSettings.chordSize = 4;
        ChordType type { chord.suffix, chord.intervals, 1, 0 };
        const auto leadRole = ! settings.melodyLogicEnabled ? resolveRole (settings)
                            : settings.melodyImportance >= 0.66f ? NoteRole::melodyTop
                            : settings.melodyImportance >= 0.33f ? NoteRole::innerVoice
                                                                : NoteRole::guideTone;
        auto notes = voiceCandidate (inputNote, chord.root, type, soliSettings, leadRole);
        notes.erase (std::remove_if (notes.begin(), notes.end(), [inputNote] (int note) { return note > inputNote; }), notes.end());
        if (std::find (notes.begin(), notes.end(), inputNote) == notes.end())
            notes.push_back (inputNote);
        std::sort (notes.begin(), notes.end());
        while (notes.size() > 4)
            notes.erase (notes.begin());

        auto score = 140.0f - (settings.phraseMemory ? averageMotion (notes, previousVoicing) : 0.0f)
                                  * (0.8f + settings.voiceLeading * 1.6f + settings.harmonicStability);
        if (! inputIsInScale)
        {
            auto nearestChordTone = 6;
            for (const auto interval : chord.intervals)
            {
                const auto distance = mod12 (inputPitchClass - mod12 (chord.root + interval));
                nearestChordTone = std::min (nearestChordTone, std::min (distance, 12 - distance));
            }
            score -= static_cast<float> (nearestChordTone) * 15.0f;
            if (settings.phraseMemory && degree == previousScaleDegree)
                score += (settings.fastInput ? 82.0f : 52.0f) * settings.harmonicStability;
            if (settings.phraseMemory && previousScaleDegree == 4 && degree == 0)
                score += 18.0f * settings.harmonicStability;
            score += harmonicReach (settings) * 18.0f;
        }
        if (degree == 0 || degree == 3 || degree == 4)
            score += 8.0f + settings.harmonicStability * 12.0f;
        const auto cadencePoint = settings.phraseMemory && phrasePosition >= 3 && phrasePosition % 4 == 3;
        if (cadencePoint && degree == 0)
            score += 30.0f * settings.harmonicStability;
        if (settings.phraseMemory && previousScaleDegree == 4 && degree == 0)
            score += 34.0f * settings.harmonicStability;
        if (settings.phraseMemory && previousScaleDegree == 1 && degree == 4)
            score += 24.0f * settings.harmonicStability;
        score -= repeatAvoidancePenalty (inputNote, pcName (chord.root) + chord.suffix, settings);
        choices.push_back ({ degree, chord, std::move (notes), score });
    }

    // Close-lead and big-band lines may use a chromatic root as a real borrowed
    // seventh chord when Outside or Modulation is high, instead of merely
    // relabelling another diatonic chord while adding the input as a tension.
    const auto reach = harmonicReach (settings);
    if (reach > 0.35f)
    {
        std::uniform_real_distribution<float> borrowChance (0.0f, 1.0f);
        if (reach >= 0.95f || borrowChance (rng) < reach)
        {
            const std::array<ChordType, 5> borrowedTypes
            {{
                { "maj7", { 0, 4, 7, 11 }, 1, 1 },
                { "m7",   { 0, 3, 7, 10 }, 1, 1 },
                { "7",    { 0, 4, 7, 10 }, 1, 1 },
                { "m7b5", { 0, 3, 6, 10 }, 1, 2 },
                { "dim7", { 0, 3, 6, 9 },  1, 2 }
            }};
            for (const auto& borrowedType : borrowedTypes)
            {
                auto outsideTones = 0;
                for (const auto interval : borrowedType.intervals)
                    if (! pitchInSelectedKeyScale (inputPitchClass + interval, settings))
                        ++outsideTones;
                if (outsideTones == 0)
                    continue;

                auto soliSettings = settings;
                soliSettings.chordSize = 4;
                auto notes = voiceCandidate (inputNote, inputPitchClass, borrowedType,
                                             soliSettings, NoteRole::melodyTop);
                auto score = 105.0f + reach * (54.0f + outsideTones * 20.0f);
                if (settings.phraseMemory)
                    score -= averageMotion (notes, previousVoicing) * settings.voiceLeading * 0.65f;
                score -= repeatAvoidancePenalty (inputNote,
                    pcName (inputPitchClass) + borrowedType.suffix, settings);
                choices.push_back ({ -1,
                    { inputPitchClass, borrowedType.suffix, borrowedType.intervals },
                    std::move (notes), score });
            }
        }
    }

    if (choices.empty())
    {
        const auto relative = mod12 (inputPitchClass - key);
        auto nearestDegree = 0;
        auto nearestDistance = 12;
        for (int degree = 0; degree < static_cast<int> (scaleNotes.size()); ++degree)
        {
            const auto distance = std::min (mod12 (relative - scaleNotes[static_cast<std::size_t> (degree)]),
                                            mod12 (scaleNotes[static_cast<std::size_t> (degree)] - relative));
            if (distance < nearestDistance)
            {
                nearestDistance = distance;
                nearestDegree = degree;
            }
        }
        const auto chord = makeDiatonicChord (key, scale, nearestDegree, true);
        ChordType type { chord.suffix, chord.intervals, 1, 0 };
        auto soliSettings = settings;
        soliSettings.chordSize = 4;
        choices.push_back ({ nearestDegree, chord,
                             voiceCandidate (inputNote, chord.root, type, soliSettings,
                                             ! settings.melodyLogicEnabled ? resolveRole (settings)
                                           : settings.melodyImportance >= 0.66f ? NoteRole::melodyTop
                                           : settings.melodyImportance >= 0.33f ? NoteRole::innerVoice
                                                                               : NoteRole::guideTone), 0.0f });
    }

    const auto chosen = std::max_element (choices.begin(), choices.end(), [] (const auto& left, const auto& right)
    {
        return left.score < right.score;
    });
    auto notes = chosen->notes;
    if (bigBand)
    {
        std::sort (notes.begin(), notes.end());
        if (notes.size() >= 3)
        {
            const auto dropIndex = notes.size() - 2;
            if (notes[dropIndex] != inputNote && notes[dropIndex] - 12 >= settings.minNote)
                notes[dropIndex] -= 12;
        }
        if (inputNote - 12 >= settings.minNote)
            notes.push_back (inputNote - 12); // Miller-style doubled lead.
        std::sort (notes.begin(), notes.end());
        notes.erase (std::unique (notes.begin(), notes.end()), notes.end());
    }

    GeneratedChord result { notes, pcName (chosen->chord.root) + chosen->chord.suffix };
    return finishGeneratedChord (std::move (result), inputNote, settings, true,
                                 chosen->degree, false);
}

GeneratedChord ChordEngine::finishGeneratedChord (GeneratedChord result,
                                                   int inputNote,
                                                   const Settings& settings,
                                                   bool melodyDriven,
                                                   int scaleDegree,
                                                   bool chromatic)
{
    auto rangeMin = juce::jlimit (0, 126, settings.minNote);
    auto rangeMax = juce::jlimit (rangeMin + 1, 127, settings.maxNote);
    auto maxVoices = 12;
    auto preferredCentre = 66.0f;
    const auto applyPreset = [&] (int low, int high, int voices, float centre)
    {
        rangeMin = juce::jmax (rangeMin, low);
        rangeMax = juce::jmin (rangeMax, high);
        if (rangeMax <= rangeMin)
        {
            rangeMin = juce::jlimit (0, 126, low);
            rangeMax = juce::jlimit (rangeMin + 1, 127, high);
        }
        maxVoices = voices;
        preferredCentre = centre;
    };

    switch (settings.playability)
    {
        case Playability::piano: applyPreset (21, 108, 10, 64.0f); break;
        case Playability::guitar: applyPreset (40, 88, 6, 62.0f); break;
        case Playability::hornSection: applyPreset (46, 84, 8, 65.0f); break;
        case Playability::orchestra: applyPreset (24, 108, 24, 64.0f); break;
        case Playability::saxSoli: applyPreset (46, 82, 5, 64.0f); break;
        case Playability::brassSection: applyPreset (40, 86, 6, 62.0f); break;
        case Playability::vocalSATB: applyPreset (40, 81, 4, 61.0f); break;
        case Playability::strings: applyPreset (36, 100, 8, 66.0f); break;
        case Playability::pianoHands: applyPreset (21, 108, 8, 64.0f); break;
        case Playability::lowReeds: applyPreset (28, 67, 5, 48.0f); break;
        case Playability::synthStack: applyPreset (24, 108, 12, 66.0f); break;
        case Playability::unrestricted: break;
    }

    auto& notes = result.notes;
    std::sort (notes.begin(), notes.end());
    notes.erase (std::unique (notes.begin(), notes.end()), notes.end());
    const auto sourceNotes = notes;

    for (std::size_t index = 0; index < notes.size(); ++index)
    {
        const auto target = settings.phraseMemory && index < previousVoicing.size() ? previousVoicing[index]
                          : registerCentre + (static_cast<float> (index) - static_cast<float> (notes.size() - 1) * 0.5f) * 5.0f;
        auto voiced = octaveNear (mod12 (notes[index]), static_cast<int> (std::round (target)));
        while (voiced < rangeMin)
            voiced += 12;
        while (voiced > rangeMax)
            voiced -= 12;
        notes[index] = voiced;
    }

    // Preserve exact common tones whenever the new harmony contains them.
    for (const auto prior : settings.phraseMemory ? previousVoicing : std::vector<int> {})
    {
        const auto match = std::find_if (notes.begin(), notes.end(), [prior] (int note)
        {
            return mod12 (note) == mod12 (prior);
        });
        if (match != notes.end() && prior >= rangeMin && prior <= rangeMax)
            *match = prior;
    }

    if (melodyDriven && settings.melodyLogicEnabled && settings.melodyImportance >= 0.66f)
    {
        for (auto& note : notes)
            if (note != inputNote)
                while (note >= inputNote && note - 12 >= rangeMin)
                    note -= 12;
        notes.erase (std::remove_if (notes.begin(), notes.end(), [inputNote] (int note) { return note > inputNote; }), notes.end());
        if (inputNote >= rangeMin && inputNote <= rangeMax
            && std::find (notes.begin(), notes.end(), inputNote) == notes.end())
            notes.push_back (inputNote);
    }
    else if (melodyDriven && settings.melodyLogicEnabled && settings.melodyImportance >= 0.33f
             && inputNote >= rangeMin && inputNote <= rangeMax
             && std::find (notes.begin(), notes.end(), inputNote) == notes.end())
    {
        notes.push_back (inputNote);
    }

    // A melodic trigger should never be left exposed merely because register
    // correction moved its supporting voices above the lead note.
    if (melodyDriven && settings.melodyLogicEnabled && settings.melodyImportance >= 0.66f && notes.size() < 4)
    {
        for (const auto sourceNote : sourceNotes)
        {
            auto support = octaveNear (mod12 (sourceNote), inputNote - 5);
            while (support >= inputNote)
                support -= 12;
            while (support < rangeMin)
                support += 12;
            if (support >= rangeMin && support <= rangeMax && support < inputNote
                && std::find (notes.begin(), notes.end(), support) == notes.end())
                notes.push_back (support);
            if (notes.size() >= 4)
                break;
        }
    }

    std::sort (notes.begin(), notes.end());
    notes.erase (std::unique (notes.begin(), notes.end()), notes.end());
    while (static_cast<int> (notes.size()) > maxVoices)
        notes.erase (notes.begin() + (melodyDriven ? 0 : static_cast<int> (notes.size() / 2)));

    if (! notes.empty())
    {
        const auto mean = std::accumulate (notes.begin(), notes.end(), 0.0f)
                        / static_cast<float> (notes.size());
        registerCentre = registerCentre * 0.72f + mean * 0.22f + preferredCentre * 0.06f;
    }

    previousVoicing = notes;
    result.name = detailedChordName (result.name, notes);
    previousChord = result;
    perInputChords[static_cast<std::size_t> (juce::jlimit (0, 127, inputNote))] = result;
    hasPerInputChord[static_cast<std::size_t> (juce::jlimit (0, 127, inputNote))] = true;
    previousInputNote = inputNote;
    if (settings.phraseMemory && scaleDegree >= 0)
        previousScaleDegree = scaleDegree;
    ++phrasePosition;
    recentInputNotes.push_back (inputNote);
    recentScaleDegrees.push_back (chromatic ? -1 : scaleDegree);
    if (recentInputNotes.size() > 8)
        recentInputNotes.erase (recentInputNotes.begin());
    if (recentScaleDegrees.size() > 8)
        recentScaleDegrees.erase (recentScaleDegrees.begin());
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
            score += harmonicReach (settings) * 20.0f - 16.0f;
        score -= averageMotion (notes, previousVoicing) * settings.voiceLeading * 0.85f;
        score -= std::abs (static_cast<int> (notes.size()) - settings.chordSize) * 2.5f;
        if (candidate.chord.root == current.root && candidate.chord.suffix == current.suffix)
            score += (1.0f - settings.substitutionDepth) * 24.0f;

        scored.push_back ({ { std::move (notes), candidate.chord.displayName }, score });
    }

    std::sort (scored.begin(), scored.end(), [] (const auto& a, const auto& b) { return a.score > b.score; });
    const auto pool = juce::jlimit (1, static_cast<int> (scored.size()),
                                    1 + static_cast<int> (std::round (styleVariation (settings.style) * 4.0f
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
    if (settings.phraseMemory && phraseKey >= 0 && maskContains (keyMask, phraseKey))
        return phraseKey;

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
        if (settings.phraseMemory && ! previousVoicing.empty())
            score -= static_cast<float> (std::abs (mod12 (key - mod12 (previousVoicing.front())))) * 0.12f;

        std::uniform_real_distribution<float> noise (0.0f, styleVariation (settings.style) * 4.0f
                                                           + harmonicReach (settings) * 3.0f);
        score += noise (rng);

        if (score > bestScore)
        {
            bestScore = score;
            bestKey = key;
        }
    }

    if (settings.phraseMemory)
        phraseKey = bestKey;
    return bestKey;
}

ScaleType ChordEngine::choosePrimaryScale (const Settings& settings, int key, int inputNote)
{
    const auto scaleMask = clampMask (settings.scaleMask, scaleCount);
    std::vector<double> weights;
    std::vector<int> indexes;
    weights.reserve (scaleCount);
    indexes.reserve (scaleCount);

    // Multiple selected scales are one intentional modal palette, rather than
    // alternatives from which a single scale is pinned for the whole phrase.
    // If the performed note belongs to only some selected modes, restrict the
    // choice to those modes so it is never mistaken for an outside note.
    const auto relativeInput = mod12 (inputNote - key);
    auto anySelectedScaleContainsInput = false;
    for (int i = 0; i < scaleCount; ++i)
    {
        if (! maskContains (scaleMask, i))
            continue;
        const auto& intervals = scaleIntervals (static_cast<ScaleType> (i));
        anySelectedScaleContainsInput = anySelectedScaleContainsInput
            || std::find (intervals.begin(), intervals.end(), relativeInput) != intervals.end();
    }

    for (int i = 0; i < scaleCount; ++i)
    {
        if (! maskContains (scaleMask, i))
            continue;

        const auto& intervals = scaleIntervals (static_cast<ScaleType> (i));
        const auto containsInput = std::find (intervals.begin(), intervals.end(), relativeInput) != intervals.end();
        if (anySelectedScaleContainsInput && ! containsInput)
            continue;

        auto weight = 1.0;
        if (settings.style == Style::classical || settings.style == Style::baroqueCounterpoint)
            weight += (i == 0 || i == 5 || i == 7) ? 2.0 : 0.0;
        if (settings.style == Style::quartalColor || settings.style == Style::modalFilm || settings.style == Style::neoSoul)
            weight += (i == 1 || i == 3 || i == 4 || i == 8) ? 2.0 : 0.0;
        if (settings.style == Style::progressiveRock)
            weight += (i == 3 || i == 7 || i == 8 || i == 10) ? 2.0 : 0.0;
        if (settings.style == Style::modernOutside)
            weight += static_cast<double> (harmonicReach (settings)) * 3.0;
        if (settings.style == Style::simpleScale)
            weight += (i == 0 || i == 1 || i == 4 || i == 5) ? 1.5 : 0.0;
        if (settings.style == Style::ambientSpread)
            weight += (i == 1 || i == 3 || i == 8) ? 2.0 : 0.0;
        if (settings.style == Style::latinShells)
            weight += (i == 0 || i == 4 || i == 5) ? 1.5 : 0.0;

        // Phrase Memory gives the last mode a modest continuity preference,
        // but no longer locks it. Every fourth generated chord gently invites
        // another selected mode, producing musical modal interchange while
        // leaving Outside and Modulation at zero.
        if (settings.phraseMemory && phraseScale >= 0)
        {
            if (i == phraseScale)
                weight *= phrasePosition > 0 && phrasePosition % 4 == 3 ? 0.2 : 1.35;
            else if (phrasePosition > 0 && phrasePosition % 4 == 3)
                weight *= 1.8;
        }

        indexes.push_back (i);
        weights.push_back (weight);
    }

    std::discrete_distribution<int> pick (weights.begin(), weights.end());
    const auto selected = indexes[static_cast<size_t> (pick (rng))];
    if (settings.phraseMemory)
        phraseScale = selected;
    return static_cast<ScaleType> (selected);
}

std::vector<ChordEngine::Candidate> ChordEngine::buildCandidates (int inputNote, const Settings& settings, NoteRole role) const
{
    std::vector<Candidate> candidates;
    candidates.reserve (96);
    const auto simpleScale = settings.style == Style::simpleScale;
    const auto reach = harmonicReach (settings);

    for (int root = 0; root < 12; ++root)
    {
        if (simpleScale && ! pitchInSelectedKeyScale (root, settings))
            continue;

        // A selected key is a tonality, not a single allowed chord root. At
        // zero reach, all diatonic roots remain available; chromatic roots enter
        // progressively through Outside or Modulation.
        if (! pitchInSelectedKeyScale (root, settings) && (simpleScale || reach < 0.2f))
            continue;

        for (const auto& type : chordTypes())
        {
            if (simpleScale)
            {
                const auto suffix = juce::String (type.suffix);
                const auto basicChord = suffix.isEmpty() || suffix == "m" || suffix == "dim"
                                     || suffix == "6" || suffix == "m6" || suffix == "maj7"
                                     || suffix == "m7" || suffix == "7" || suffix == "m7b5"
                                     || suffix == "dim7";
                if (! basicChord || type.intervals.size() > 4)
                    continue;
            }

            if (simpleScale && ! std::all_of (type.intervals.begin(), type.intervals.end(), [&] (int interval)
            {
                return pitchInSelectedKeyScale (root + interval, settings);
            }))
                continue;

            const auto noteInterval = mod12 (inputNote - root);
            const auto containsInput = std::any_of (type.intervals.begin(), type.intervals.end(), [&] (int interval)
            {
                return mod12 (interval) == noteInterval;
            });

            if (! containsInput && role != NoteRole::bass)
                continue;

            if (static_cast<float> (type.complexity) / 5.0f > settings.complexity + reach * 0.45f + 0.12f)
                continue;

            if (! chordMostlyInScale (root, type, settings) && (simpleScale || reach < 0.35f))
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
    switch (settings.playability)
    {
        case Playability::guitar: maxChordSize = 6; break;
        case Playability::hornSection: maxChordSize = 8; break;
        case Playability::saxSoli: maxChordSize = 5; break;
        case Playability::brassSection: maxChordSize = 6; break;
        case Playability::vocalSATB: maxChordSize = 4; break;
        case Playability::strings: maxChordSize = 8; break;
        case Playability::pianoHands: maxChordSize = 8; break;
        case Playability::lowReeds: maxChordSize = 5; break;
        case Playability::synthStack: maxChordSize = 12; break;
        case Playability::orchestra:
        case Playability::unrestricted: maxChordSize = 24; break;
        case Playability::piano: break;
    }

    auto requestedSize = juce::jlimit (2, maxChordSize, settings.chordSize);
    if (settings.playability == Playability::guitar)
        requestedSize = juce::jmin (requestedSize, 6);
    else if (settings.playability == Playability::hornSection)
        requestedSize = juce::jlimit (3, 5, requestedSize);
    else if (settings.playability == Playability::saxSoli)
        requestedSize = juce::jlimit (3, 5, requestedSize);
    else if (settings.playability == Playability::brassSection)
        requestedSize = juce::jlimit (3, 6, requestedSize);
    else if (settings.playability == Playability::vocalSATB)
        requestedSize = 4;
    else if (settings.playability == Playability::lowReeds)
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
    const auto shift = role == NoteRole::melodyTop ? 0 : inputNote - anchorAbsolute;

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
        const auto expansionMinNote = juce::jlimit (0, 127, settings.minNote);
        const auto expansionMaxNote = juce::jlimit (expansionMinNote + 1, 127, settings.maxNote);
        auto expanded = notes;
        expanded.reserve (24);

        for (const auto baseNote : notes)
        {
            for (int octave = -3; octave <= 4; ++octave)
            {
                const auto candidate = baseNote + octave * 12;
                if (candidate < expansionMinNote || candidate > expansionMaxNote)
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

    const auto dropVoiceFromTop = [&] (int rank)
    {
        if (static_cast<int> (notes.size()) <= rank)
            return;
        for (int index = static_cast<int> (notes.size()) - rank; index >= 0; --index)
        {
            if (notes[static_cast<std::size_t> (index)] == inputNote)
                continue;
            if (notes[static_cast<std::size_t> (index)] - 12 >= minNote)
                notes[static_cast<std::size_t> (index)] -= 12;
            return;
        }
    };

    if (settings.style == Style::openTriads || settings.style == Style::dropTwo)
        dropVoiceFromTop (2);
    else if (settings.style == Style::dropThree)
        dropVoiceFromTop (3);
    else if (settings.style == Style::spreadTenths)
    {
        if (! notes.empty() && notes.front() != inputNote && notes.front() - 12 >= minNote)
            notes.front() -= 12;
        if (notes.size() > 2 && notes.back() != inputNote && notes.back() + 12 <= maxNote)
            notes.back() += 12;
    }
    else if (settings.style == Style::clusterCloud || settings.style == Style::hornSoli)
    {
        const auto radius = settings.style == Style::clusterCloud ? 6 : 10;
        for (auto& note : notes)
        {
            if (note == inputNote)
                continue;
            while (note < inputNote - radius && note + 12 <= maxNote)
                note += 12;
            while (note > inputNote + radius && note - 12 >= minNote)
                note -= 12;
        }
    }

    if (settings.style == Style::pedalPoint || settings.style == Style::powerStack
        || settings.style == Style::orchestralLush)
    {
        auto pedal = minNote + mod12 (root - minNote);
        if (pedal <= maxNote)
            notes.push_back (pedal);
        if (settings.style == Style::orchestralLush)
        {
            while (pedal + 12 <= maxNote)
                pedal += 12;
            notes.push_back (pedal);
        }
    }

    for (auto& note : notes)
    {
        while (note < minNote)
            note += 12;
        while (note > maxNote)
            note -= 12;
    }
    if (inputNote >= minNote && inputNote <= maxNote
        && std::find (notes.begin(), notes.end(), inputNote) == notes.end())
        notes.push_back (inputNote);
    std::sort (notes.begin(), notes.end());
    notes.erase (std::unique (notes.begin(), notes.end()), notes.end());

    return notes;
}

float ChordEngine::scoreCandidate (const Candidate& candidate, int inputNote, const Settings& settings) const
{
    auto score = 100.0f;
    const auto keyMask = clampMask (settings.keyMask, keyCount);
    const auto reach = harmonicReach (settings);

    if (chordMostlyInScale (candidate.root, *candidate.type, settings))
        score += 40.0f - reach * 18.0f;
    else
        score += reach * 126.0f - 45.0f;

    auto keyDistance = 6;
    for (int key = 0; key < 12; ++key)
    {
        if (! maskContains (keyMask, key))
            continue;
        const auto up = mod12 (candidate.root - key);
        keyDistance = std::min (keyDistance, std::min (up, 12 - up));
    }

    score -= static_cast<float> (keyDistance) * 0.8f;
    if (pitchInSelectedKeyScale (candidate.root, settings))
        score += 9.0f * (1.0f - reach);
    else
        score += reach * 16.0f;
    if (keyDistance == 3 || keyDistance == 4)
        score += reach * 10.0f;

    score -= std::abs (static_cast<int> (candidate.voiced.size()) - settings.chordSize) * 7.0f;
    if (settings.phraseMemory)
        score -= averageMotion (candidate.voiced, previousVoicing) * settings.voiceLeading;
    score -= static_cast<float> (candidate.type->complexity) * (1.0f - settings.complexity) * 12.0f;

    if (settings.style == Style::bigBand && candidate.voiced.back() == inputNote)
        score += 20.0f;
    const auto suffix = juce::String (candidate.type->suffix);

    const auto alteredDominant = suffix == "7alt" || suffix == "7b9#11";
    if (alteredDominant && settings.style != Style::modernOutside
        && settings.style != Style::progressiveRock && settings.style != Style::wholeToneDream)
        score -= (1.0f - reach) * 42.0f + settings.harmonicStability * 38.0f;

    if (settings.style == Style::quartalColor && suffix == "sus11")
        score += 28.0f;
    if (settings.style == Style::classical && candidate.type->complexity <= 2)
        score += 22.0f;
    if (settings.style == Style::gospel && (suffix == "6" || suffix == "13"))
        score += 18.0f;
    if (settings.style == Style::modernOutside)
        score += static_cast<float> (candidate.type->colour) * 8.0f + reach * 30.0f;
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
    if (settings.style == Style::simpleScale)
    {
        const auto basicSeventh = suffix == "maj7" || suffix == "m7" || suffix == "7"
                               || suffix == "m7b5" || suffix == "dim7";
        score += basicSeventh ? 72.0f
               : (suffix == "6" || suffix == "m6") ? 28.0f
               : candidate.type->complexity == 0 ? 18.0f : -40.0f;
    }
    if (settings.style == Style::openFifths)
        score += (suffix == "sus2" || suffix == "sus4" || suffix == "") ? 25.0f : 0.0f;
    if (settings.style == Style::dropTwo)
        score += (suffix == "maj7" || suffix == "m7" || suffix == "7" || suffix == "6") ? 24.0f : 0.0f;
    if (settings.style == Style::ambientSpread)
        score += (suffix == "maj9" || suffix == "m9" || suffix == "sus11" || suffix == "maj13#11") ? 27.0f : 0.0f;
    if (settings.style == Style::latinShells)
        score += (suffix == "6" || suffix == "m6" || suffix == "7" || suffix == "m7") ? 25.0f : 0.0f;
    if (settings.style == Style::openTriads)
        score += (suffix.isEmpty() || suffix == "m" || suffix == "dim" || suffix == "sus2" || suffix == "sus4") ? 34.0f : -8.0f;
    if (settings.style == Style::powerStack)
        score += (suffix == "5" || suffix == "sus2" || suffix == "sus4") ? 40.0f : 0.0f;
    if (settings.style == Style::dropThree)
        score += (suffix == "maj7" || suffix == "m7" || suffix == "7" || suffix == "m7b5") ? 30.0f : 0.0f;
    if (settings.style == Style::spreadTenths)
        score += (suffix.isEmpty() || suffix == "m" || suffix == "6" || suffix == "m6" || suffix == "maj7" || suffix == "m7") ? 28.0f : 0.0f;
    if (settings.style == Style::clusterCloud)
        score += (suffix == "cluster" || suffix == "maj9" || suffix == "m9" || suffix == "sus11") ? 38.0f : 0.0f;
    if (settings.style == Style::pedalPoint)
        score += maskContains (keyMask, candidate.root) ? 32.0f : 0.0f;
    if (settings.style == Style::gospelShout)
        score += (suffix == "6" || suffix == "13" || suffix == "7" || suffix == "dim7") ? 34.0f : 0.0f;
    if (settings.style == Style::jazzShells)
        score += (suffix == "maj7" || suffix == "m7" || suffix == "7" || suffix == "m7b5") ? 36.0f : 0.0f;
    if (settings.style == Style::orchestralLush)
        score += (suffix == "maj9" || suffix == "m9" || suffix == "m11" || suffix == "maj13#11") ? 35.0f : 0.0f;
    if (settings.style == Style::guitarOpen)
        score += (suffix.isEmpty() || suffix == "m" || suffix == "6" || suffix == "m6" || suffix == "sus2" || suffix == "sus4") ? 33.0f : 0.0f;
    if (settings.style == Style::hornSoli)
        score += (suffix == "6" || suffix == "m6" || suffix == "maj7" || suffix == "m7" || suffix == "7") ? 32.0f : 0.0f;
    if (settings.style == Style::wholeToneDream)
        score += (suffix == "aug" || suffix == "7b9#11" || suffix == "maj13#11") ? 40.0f : 0.0f;

    score -= repeatAvoidancePenalty (inputNote, chordName (candidate.root, *candidate.type), settings);

    return score;
}

float ChordEngine::repeatAvoidancePenalty (int inputNote, const juce::String& candidateName,
                                           const Settings& settings) const
{
    const auto index = static_cast<std::size_t> (juce::jlimit (0, 127, inputNote));
    if (! hasPerInputChord[index] || settings.repeatChance >= 0.999f)
        return 0.0f;

    const auto& previousName = perInputChords[index].name;
    const auto sameHarmony = previousName == candidateName
                          || previousName.startsWith (candidateName + "/");
    return sameHarmony ? (1.0f - settings.repeatChance) * 72.0f : 0.0f;
}

int ChordEngine::chooseWeightedIndex (const std::vector<Candidate>& candidates, const Settings& settings)
{
    const auto effectiveVariation = styleVariation (settings.style)
                                  * (1.0f - settings.harmonicStability * 0.82f);
    const auto poolSize = juce::jlimit (1, static_cast<int> (candidates.size()), 1 + static_cast<int> (std::round (effectiveVariation * 10.0f)));
    std::vector<double> weights;
    weights.reserve (static_cast<size_t> (poolSize));

    const auto best = candidates.front().score;
    for (int i = 0; i < poolSize; ++i)
    {
        const auto scoreDelta = best - candidates[static_cast<size_t> (i)].score;
        weights.push_back (std::exp (-scoreDelta / juce::jmax (2.0f, 8.0f + effectiveVariation * 35.0f)));
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

bool ChordEngine::chordMostlyInScale (int root, const ChordType& type, const Settings& settings) const
{
    const auto keyMask = clampMask (settings.keyMask, keyCount);
    const auto scaleMask = clampMask (settings.scaleMask, scaleCount);
    auto bestRatio = 0.0f;
    for (int key = 0; key < keyCount; ++key)
    {
        if (! maskContains (keyMask, key))
            continue;
        for (int scale = 0; scale < scaleCount; ++scale)
        {
            if (! maskContains (scaleMask, scale))
                continue;
            const auto& selectedScale = scaleIntervals (static_cast<ScaleType> (scale));
            auto inside = 0;
            for (const auto interval : type.intervals)
            {
                const auto pitch = mod12 (root + interval - key);
                if (std::find (selectedScale.begin(), selectedScale.end(), pitch) != selectedScale.end())
                    ++inside;
            }
            bestRatio = juce::jmax (bestRatio,
                static_cast<float> (inside) / static_cast<float> (type.intervals.size()));
        }
    }
    return bestRatio >= 0.67f;
}

juce::String ChordEngine::chordName (int root, const ChordType& type) const
{
    return pcName (root) + chordSuffixName (type.suffix);
}
}
