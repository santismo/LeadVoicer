#include "../Source/GroovizerEngine.h"

#include <iostream>
#include <set>

namespace
{
void require (bool condition, const char* message)
{
    if (! condition)
    {
        std::cerr << "FAIL: " << message << '\n';
        std::exit (1);
    }
}
}

int main()
{
    Groovizer::Engine engine;
    Groovizer::Settings settings;
    settings.style = 5; // Trap Hats
    settings.lengthBars = 2;
    settings.density = 0.82f;
    settings.ghosts = 0.5f;

    const auto trap = engine.generate (42, 110, settings, 12345);
    require (trap.lengthBars == 2, "length is preserved");
    require (! trap.events.empty(), "trap phrase creates events");
    require (trap.name.containsIgnoreCase ("Trap"), "phrase name includes style");

    std::set<int> notes;
    for (const auto& event : trap.events)
    {
        require (event.note >= 0 && event.note <= 127, "event note is valid MIDI");
        require (event.velocity >= 1 && event.velocity <= 127, "event velocity is valid MIDI");
        require (event.ppq >= 0.0 && event.ppq < 8.0, "event is inside phrase");
        notes.insert (event.note);
    }
    require (notes.count (36) > 0, "generated groove includes GM kick");
    require (notes.count (38) > 0, "generated groove includes GM snare");
    require (notes.count (42) > 0, "generated groove includes GM closed hat");

    settings.style = 19; // Cinematic Toms
    settings.phrase = 1; // Fill
    const auto fill = engine.generate (45, 100, settings, 999);
    auto hasTom = false;
    for (const auto& event : fill.events)
        hasTom = hasTom || event.note == 41 || event.note == 45 || event.note == 50;
    require (hasTom, "tom fill includes GM toms");

    std::cout << "Groovizer engine tests passed\n";
    return 0;
}
