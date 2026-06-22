#pragma once

#include <JuceHeader.h>

#include <array>
#include <random>
#include <vector>

namespace Soli
{
enum class NoteRole
{
    melodyTop = 0,
    root,
    bass,
    guideTone,
    innerVoice,
    random,
    autoWeighted
};

enum class ScaleType
{
    ionian = 0,
    dorian,
    phrygian,
    lydian,
    mixolydian,
    aeolian,
    locrian,
    harmonicMinor,
    melodicMinor,
    diminished,
    wholeTone,
    blues
};

enum class Style
{
    closeLead = 0,
    bigBand,
    quartalColor,
    classical,
    gospel,
    modernOutside,
    modalFilm,
    chromaticMediant,
    baroqueCounterpoint,
    neoSoul,
    progressiveRock
};

enum class StrumMode
{
    together = 0,
    up,
    down,
    random
};

enum class Playability
{
    piano = 0,
    guitar,
    hornSection,
    orchestra,
    unrestricted
};

enum class ContextMode
{
    exact = 0,
    diatonic,
    substitutions,
    adaptive
};

struct Settings
{
    int keyMask = 1;
    int scaleMask = 1;
    NoteRole role = NoteRole::melodyTop;
    Style style = Style::closeLead;
    Playability playability = Playability::piano;
    StrumMode strumMode = StrumMode::together;
    int chordSize = 4;
    float complexity = 0.45f;
    float voiceLeading = 0.75f;
    float outside = 0.1f;
    float variation = 0.35f;
    float repeatChance = 0.15f;
    float strumSpeed = 0.0f;
    ContextMode contextMode = ContextMode::adaptive;
    float substitutionDepth = 0.35f;
    int minNote = 36;
    int maxNote = 96;
};

struct GeneratedChord
{
    std::vector<int> notes;
    juce::String name = "--";
};

class ChordEngine
{
public:
    struct ChordType
    {
        const char* suffix = "";
        std::vector<int> intervals;
        int complexity = 0;
        int colour = 0;
    };

    GeneratedChord generate (int inputNote, int velocity, const Settings& settings);
    GeneratedChord generateForContext (int inputNote,
                                       const juce::String& currentChord,
                                       const juce::String& previousContext,
                                       const juce::String& nextChord,
                                       const Settings& settings);
    void reset();

    static juce::StringArray keyNames();
    static juce::StringArray scaleNames();
    static juce::StringArray roleNames();
    static juce::StringArray styleNames();
    static juce::StringArray playabilityNames();
    static juce::StringArray strumModeNames();
    static juce::StringArray contextModeNames();

private:
    struct Candidate
    {
        int root = 0;
        const ChordType* type = nullptr;
        std::vector<int> voiced;
        float score = 0.0f;
    };

    std::vector<Candidate> buildCandidates (int inputNote, const Settings& settings, NoteRole resolvedRole) const;
    std::vector<int> voiceCandidate (int inputNote, int root, const ChordType& type, const Settings& settings, NoteRole role) const;
    float scoreCandidate (const Candidate& candidate, int inputNote, const Settings& settings) const;
    NoteRole resolveRole (const Settings& settings);
    int choosePrimaryKey (const Settings& settings, int inputNote);
    ScaleType choosePrimaryScale (const Settings& settings);
    int chooseWeightedIndex (const std::vector<Candidate>& candidates, const Settings& settings);
    int nearestChordToneIndex (const std::vector<int>& intervals, int pitchClassFromRoot) const;
    bool pitchInAnyScale (int pitchClass, int root, const Settings& settings) const;
    bool chordMostlyInScale (int root, const ChordType& type, const Settings& settings) const;
    juce::String chordName (int root, const ChordType& type) const;

    std::vector<int> previousVoicing;
    GeneratedChord previousChord;
    std::mt19937 rng { std::random_device{}() };
};
}
