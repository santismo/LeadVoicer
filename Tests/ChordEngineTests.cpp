#include <JuceHeader.h>
#include "../Source/ChordEngine.h"

#include <algorithm>
#include <iostream>

namespace
{
bool contains (const std::vector<int>& notes, int note)
{
    return std::find (notes.begin(), notes.end(), note) != notes.end();
}

int fail (const char* message)
{
    std::cerr << "FAIL: " << message << '\n';
    return 1;
}
}

int main()
{
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
