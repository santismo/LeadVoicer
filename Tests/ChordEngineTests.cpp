#include <JuceHeader.h>
#include "../Source/ChordEngine.h"

#include <algorithm>
#include <array>
#include <iostream>

namespace
{
bool contains (const std::vector<int>& notes, int note)
{
    return std::find (notes.begin(), notes.end(), note) != notes.end();
}

bool containsPitchClass (const std::vector<int>& notes, int pitchClass)
{
    return std::any_of (notes.begin(), notes.end(), [pitchClass] (int note)
    {
        return (note % 12 + 12) % 12 == pitchClass;
    });
}

int fail (const char* message)
{
    std::cerr << "FAIL: " << message << '\n';
    return 1;
}
}

int main()
{
    const auto styles = Soli::ChordEngine::styleNames();
    if (styles.size() < 28 || styles[0] != "Simple Scale Chords")
        return fail ("Simple Scale Chords is not the first style or the expanded style library is incomplete.");
    if (Soli::ChordEngine::playabilityNames().size() < 12)
        return fail ("The ensemble range preset library is incomplete.");

    Soli::ChordEngine engine;
    Soli::Settings settings;
    settings.variation = 0.0f;
    settings.repeatChance = 0.0f;
    settings.role = Soli::NoteRole::melodyTop;
    settings.contextMode = Soli::ContextMode::exact;

    const auto exact = engine.generateForContext (67, "Cmaj7", {}, {}, settings);
    if (exact.name != "Cmaj7")
        return fail ("Exact follow mode changed the source chord.");
    if (! contains (exact.notes, 67))
        return fail ("Exact follow mode omitted the incoming note.");

    engine.reset();
    const auto tension = engine.generateForContext (66, "Cmaj7", {}, {}, settings);
    if (! contains (tension.notes, 66))
        return fail ("A non-chord incoming note was not retained as a tension.");

    engine.reset();
    settings.style = Soli::Style::simpleScale;
    settings.keyMask = 1;   // C
    settings.scaleMask = 1; // Ionian
    settings.outside = 1.0f;
    const auto simple = engine.generate (60, 100, settings);
    const std::array<int, 7> cIonian { 0, 2, 4, 5, 7, 9, 11 };
    if (simple.notes.size() < 3 || std::any_of (simple.notes.begin(), simple.notes.end(), [&] (int note)
    {
        return std::find (cIonian.begin(), cIonian.end(), note % 12) == cIonian.end();
    }))
        return fail ("Simple Scale Chords produced notes outside the selected key and scale.");
    if (! simple.name.contains ("7"))
        return fail ("Simple Scale Chords did not prefer a basic seventh chord.");

    engine.reset();
    settings.role = Soli::NoteRole::root;
    settings.chordSize = 4;
    const auto cMajor = engine.generate (60, 100, settings);
    if (cMajor.name != "Cmaj7" || ! containsPitchClass (cMajor.notes, 0)
        || ! containsPitchClass (cMajor.notes, 4) || ! containsPitchClass (cMajor.notes, 7)
        || ! containsPitchClass (cMajor.notes, 11))
        return fail ("Root-mode C in C Ionian did not produce Cmaj7.");

    const auto chromaticTrigger = engine.generate (61, 100, settings);
    if (chromaticTrigger.name != "Fmaj7" || chromaticTrigger.notes.size() < 3
        || containsPitchClass (chromaticTrigger.notes, 1))
        return fail ("A chromatic Simple-mode trigger passed through or failed to advance I to IV.");

    const auto bDiminished = engine.generate (71, 100, settings);
    if (bDiminished.name != "Bdim7" || ! containsPitchClass (bDiminished.notes, 11)
        || ! containsPitchClass (bDiminished.notes, 2) || ! containsPitchClass (bDiminished.notes, 5)
        || ! containsPitchClass (bDiminished.notes, 8))
        return fail ("Root-mode B in C Ionian did not produce Bdim7.");

    engine.reset();
    settings.role = Soli::NoteRole::melodyTop;
    settings.style = Soli::Style::closeLead;
    const auto closeC = engine.generate (72, 100, settings);
    const auto closeD = engine.generate (74, 100, settings);
    if (closeC.notes.empty() || closeD.notes.empty() || closeC.notes.back() != 72 || closeD.notes.back() != 74)
        return fail ("Close Lead did not keep the played melody on top of its scale-informed soli voicing.");

    engine.reset();
    settings.style = Soli::Style::bigBand;
    const auto miller = engine.generate (72, 100, settings);
    if (! contains (miller.notes, 72) || ! contains (miller.notes, 60))
        return fail ("Big Band did not create its doubled-lead soli voicing.");

    engine.reset();
    settings.style = Soli::Style::simpleScale;
    settings.role = Soli::NoteRole::root;
    settings.playability = Soli::Playability::vocalSATB;
    settings.chordSize = 9;
    const auto satb = engine.generate (60, 100, settings);
    if (satb.notes.size() > 4 || std::any_of (satb.notes.begin(), satb.notes.end(), [] (int note)
    {
        return note < 40 || note > 81;
    }))
        return fail ("SATB range preset exceeded four voices or its practical range.");

    engine.reset();
    settings.playability = Soli::Playability::piano;
    settings.chordSize = 4;
    settings.style = Soli::Style::closeLead;
    settings.role = Soli::NoteRole::melodyTop;
    settings.melodyImportance = 0.9f;
    settings.harmonicStability = 0.8f;
    engine.generate (72, 100, settings);
    settings.fastInput = false;
    const auto slowChromatic = engine.generate (73, 100, settings);
    settings.fastInput = true;
    const auto fastChromatic = engine.generate (73, 100, settings);
    if (slowChromatic.notes.size() < 4 || fastChromatic.notes.size() < 4
        || slowChromatic.notes.back() != 73 || fastChromatic.notes.back() != 73)
        return fail ("Close Lead exposed a chromatic melody note without a full supporting voicing.");
    if (slowChromatic.name.containsIgnoreCase ("alt") || fastChromatic.name.containsIgnoreCase ("alt"))
        return fail ("Close Lead used an altered dominant for an ordinary chromatic passing note.");
    for (const auto note : { 75, 78, 80, 82 })
    {
        settings.fastInput = true;
        const auto chromaticPhraseChord = engine.generate (note, 100, settings);
        if (chromaticPhraseChord.notes.size() < 4 || chromaticPhraseChord.notes.back() != note
            || chromaticPhraseChord.name.containsIgnoreCase ("alt"))
            return fail ("A chromatic Close Lead phrase lost its supporting harmony or overused altered chords.");
    }

    engine.reset();
    settings.style = Soli::Style::closeLead;
    settings.outside = 0.1f;
    settings.contextMode = Soli::ContextMode::substitutions;
    settings.substitutionDepth = 1.0f;
    settings.variation = 1.0f;
    for (int attempt = 0; attempt < 24; ++attempt)
    {
        const auto substitute = engine.generateForContext (65, "G7", "Dm7", "Cmaj7", settings);
        if (substitute.name.isNotEmpty() && contains (substitute.notes, 65))
        {
            std::cout << "Voicizer chord engine tests passed\n";
            return 0;
        }
    }
    return fail ("Substitution mode did not produce an anchored candidate.");
}
