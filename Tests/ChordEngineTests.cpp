#include <JuceHeader.h>
#include "../Source/ChordEngine.h"

#include <algorithm>
#include <array>
#include <iostream>
#include <set>

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

    // E Ionian + E Dorian is a selected modal palette, not an Outside or
    // Modulation effect. Notes unique to either mode must retain their proper
    // diatonic chord even when Repeat is at either extreme.
    engine.reset();
    settings.style = Soli::Style::simpleScale;
    settings.role = Soli::NoteRole::root;
    settings.keyMask = 1 << 4; // E
    settings.scaleMask = (1 << 0) | (1 << 1); // Ionian + Dorian
    settings.outside = 0.0f;
    settings.modulation = 0.0f;
    settings.phraseMemory = true;
    settings.repeatChance = 0.0f;
    const auto dorianG = engine.generate (67, 100, settings);
    const auto ionianGSharp = engine.generate (68, 100, settings);
    if (dorianG.name != "Gmaj7" || ionianGSharp.name != "Abm7")
    {
        std::cerr << "Modal outputs: " << dorianG.name << ", " << ionianGSharp.name << '\n';
        return fail ("Selected E Ionian and E Dorian did not interchange at zero Outside and Modulation.");
    }

    engine.reset();
    std::set<juce::String> sharedRootQualities;
    for (int attempt = 0; attempt < 16; ++attempt)
        sharedRootQualities.insert (engine.generate (64, 100, settings).name);
    if (sharedRootQualities.count ("Emaj7") == 0 || sharedRootQualities.count ("Em7") == 0)
        return fail ("A root shared by E Ionian and E Dorian remained pinned to one selected mode.");

    engine.reset();
    settings.repeatChance = 1.0f;
    const auto fixedDorianG = engine.generate (67, 100, settings);
    const auto fixedIonianGSharp = engine.generate (68, 100, settings);
    if (fixedDorianG.name != "Gmaj7" || fixedIonianGSharp.name != "Abm7")
        return fail ("Maximum Repeat disabled the selected modal-interchange palette.");
    if (engine.generate (67, 100, settings).name != fixedDorianG.name
        || engine.generate (68, 100, settings).name != fixedIonianGSharp.name)
        return fail ("Maximum Repeat stopped recalling an exact trigger after modal interchange.");

    settings.keyMask = 1;   // C
    settings.scaleMask = 1; // Ionian
    settings.repeatChance = 0.0f;

    engine.reset();
    settings.style = Soli::Style::closeLead;
    settings.role = Soli::NoteRole::melodyTop;
    settings.playability = Soli::Playability::piano;
    settings.minNote = 21;  // A0
    settings.maxNote = 108; // C8
    const auto pianoLow = engine.generate (21, 100, settings);
    engine.reset();
    const auto pianoHigh = engine.generate (108, 100, settings);
    if (! contains (pianoLow.notes, 21) || ! contains (pianoHigh.notes, 108))
        return fail ("Piano playability did not reach the full A0-to-C8 range.");

    settings.style = Soli::Style::simpleScale;
    settings.playability = Soli::Playability::piano;
    settings.minNote = 36;
    settings.maxNote = 96;

    engine.reset();
    settings.role = Soli::NoteRole::root;
    settings.chordSize = 4;
    const auto cMajor = engine.generate (60, 100, settings);
    if (cMajor.name != "Cmaj7" || ! containsPitchClass (cMajor.notes, 0)
        || ! containsPitchClass (cMajor.notes, 4) || ! containsPitchClass (cMajor.notes, 7)
        || ! containsPitchClass (cMajor.notes, 11))
        return fail ("Root-mode C in C Ionian did not produce Cmaj7.");

    const auto chromaticTrigger = engine.generate (61, 100, settings);
    if (chromaticTrigger.name != "C#dim7" || chromaticTrigger.notes.size() < 4
        || ! containsPitchClass (chromaticTrigger.notes, 1))
        return fail ("A chromatic Simple-mode trigger did not become a diminished passing chord on its played root.");

    const auto dMinor = engine.generate (62, 100, settings);
    if (dMinor.name != "Dm7")
        return fail ("Simple mode did not resolve Cmaj7 - Dbdim7 - Dm7 by performed root.");

    engine.reset();
    settings.phraseMemory = false;
    settings.modulation = 1.0f;
    const auto modulatedPassing = engine.generate (61, 100, settings);
    const std::array<juce::String, 4> modulationQualities { "C#m7b5", "C#m7", "C#maj7", "C#7" };
    if (std::find (modulationQualities.begin(), modulationQualities.end(), modulatedPassing.name)
            == modulationQualities.end()
        || modulatedPassing.notes.size() < 4)
        return fail ("Maximum Modulation did not turn a chromatic Simple-mode note into a context-fitting basic seventh color.");

    engine.reset();
    settings.phraseMemory = true;
    settings.modulation = 0.0f;

    const auto bDiminished = engine.generate (71, 100, settings);
    if (bDiminished.name != "Bdim7" || ! containsPitchClass (bDiminished.notes, 11)
        || ! containsPitchClass (bDiminished.notes, 2) || ! containsPitchClass (bDiminished.notes, 5)
        || ! containsPitchClass (bDiminished.notes, 8))
        return fail ("Root-mode B in C Ionian did not produce Bdim7.");

    engine.reset();
    settings.style = Soli::Style::modernOutside;
    settings.role = Soli::NoteRole::root;
    settings.phraseMemory = false;
    settings.complexity = 1.0f;
    settings.outside = 1.0f;
    settings.harmonicStability = 0.0f;
    auto foundOutsideHarmony = false;
    for (int input = 60; input < 72 && ! foundOutsideHarmony; ++input)
    {
        const auto borrowed = engine.generate (input, 100, settings);
        foundOutsideHarmony = std::any_of (borrowed.notes.begin(), borrowed.notes.end(), [&] (int note)
        {
            return std::find (cIonian.begin(), cIonian.end(), (note % 12 + 12) % 12) == cIonian.end();
        });
    }
    if (! foundOutsideHarmony)
        return fail ("Maximum Outside could not borrow harmony with only C Ionian selected.");

    engine.reset();
    settings.role = Soli::NoteRole::melodyTop;
    settings.style = Soli::Style::neoSoul;
    settings.phraseMemory = false;
    settings.complexity = 1.0f;
    settings.outside = 1.0f;
    settings.modulation = 0.0f;
    settings.repeatChance = 0.0f;
    settings.harmonicStability = 0.0f;
    std::set<juce::String> freshNames;
    auto sameTriggerFoundOutside = false;
    for (int attempt = 0; attempt < 16; ++attempt)
    {
        const auto fresh = engine.generate (72, 100, settings);
        freshNames.insert (fresh.name);
        sameTriggerFoundOutside = sameTriggerFoundOutside || std::any_of (
            fresh.notes.begin(), fresh.notes.end(), [&] (int note)
            {
                return std::find (cIonian.begin(), cIonian.end(), (note % 12 + 12) % 12) == cIonian.end();
            });
    }
    if (! sameTriggerFoundOutside)
        return fail ("Maximum Outside did not borrow tones for repeated use of one input note in C Ionian.");
    if (freshNames.size() < 2)
        return fail ("Repeat at zero did not provide fresh harmonic names for one repeated input note.");

    engine.reset();
    settings.outside = 0.0f;
    settings.modulation = 1.0f;
    auto advancedModulationFoundOutside = false;
    for (int attempt = 0; attempt < 12 && ! advancedModulationFoundOutside; ++attempt)
    {
        const auto modulated = engine.generate (72, 100, settings);
        advancedModulationFoundOutside = std::any_of (modulated.notes.begin(), modulated.notes.end(), [&] (int note)
        {
            return std::find (cIonian.begin(), cIonian.end(), (note % 12 + 12) % 12) == cIonian.end();
        });
    }
    if (! advancedModulationFoundOutside)
        return fail ("Maximum Modulation did not reach outside harmony in an advanced style with only C Ionian selected.");

    engine.reset();
    settings.outside = 1.0f;
    settings.modulation = 0.0f;
    settings.repeatChance = 1.0f;
    const auto fixedFirst = engine.generate (72, 100, settings);
    const auto fixedAgain = engine.generate (72, 100, settings);
    if (fixedFirst.name != fixedAgain.name || fixedFirst.notes != fixedAgain.notes)
        return fail ("Repeat at maximum did not recall the exact chord assigned to an input note and register.");

    engine.reset();
    settings.style = Soli::Style::simpleScale;
    settings.role = Soli::NoteRole::root;
    settings.outside = 0.0f;
    settings.modulation = 1.0f;
    settings.repeatChance = 0.0f;
    const auto borrowedSimpleC = engine.generate (60, 100, settings);
    const auto simpleBorrowedOutside = std::any_of (borrowedSimpleC.notes.begin(), borrowedSimpleC.notes.end(), [&] (int note)
    {
        return std::find (cIonian.begin(), cIonian.end(), (note % 12 + 12) % 12) == cIonian.end();
    });
    if (! simpleBorrowedOutside || borrowedSimpleC.name == "Cmaj7")
        return fail ("Maximum Modulation did not reharmonize an in-scale Simple trigger outside C Ionian.");

    engine.reset();
    settings.role = Soli::NoteRole::melodyTop;
    settings.phraseMemory = true;
    settings.outside = 1.0f;
    settings.harmonicStability = 0.72f;
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
