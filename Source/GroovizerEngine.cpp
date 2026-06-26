#include "GroovizerEngine.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <map>
#include <random>

namespace Groovizer
{
namespace
{
constexpr int kick = 36;
constexpr int snare = 38;
constexpr int rim = 37;
constexpr int clap = 39;
constexpr int closedHat = 42;
constexpr int pedalHat = 44;
constexpr int openHat = 46;
constexpr int lowTom = 41;
constexpr int midTom = 45;
constexpr int highTom = 50;
constexpr int crash = 49;
constexpr int ride = 51;
constexpr int cowbell = 56;
constexpr int clave = 75;

enum Style
{
    autoGroove, rockBackbeat, funkPocket, gospelShout, boomBap, trapHats,
    houseFour, technoDrive, drumAndBass, jazzRide, shuffleBlues, reggaeOneDrop,
    skaUpbeat, reggaetonDembow, afroCuban, salsaCascara, bossaBrush, latinPop,
    marchingCadence, cinematicToms, orchestralHits, analogElectro, minimalTechno,
    discoFunk, motownSoul, punkDrive, gameBeat, chiptuneBreak
};

enum PhraseType { groove, fill, pickup, breakdown, build, ending };
enum TriggerType { autoTrigger, followPad, kickFocus, snareFocus, hatFocus, tomFill, percussionFocus, crashTransition };
enum FeelType { straight, looseSwing, shuffle, laidBack, pushed, machineTight, tripletRoll };

struct Template
{
    int style = autoGroove;
    int phrase = groove;
    std::vector<double> kicks { 0.0, 8.0 };
    std::vector<double> kickExtra { 6.0, 10.0, 14.0 };
    std::vector<double> snares { 4.0, 12.0 };
    std::vector<double> claps;
    std::vector<double> hats { 0.0, 2.0, 4.0, 6.0, 8.0, 10.0, 12.0, 14.0 };
    std::vector<double> openHats;
    std::vector<double> rides;
    std::vector<double> crashes;
    std::vector<double> rims;
    std::vector<double> tomLow;
    std::vector<double> tomMid;
    std::vector<double> tomHigh;
    std::vector<double> percussion;
    std::vector<double> claves;
    std::vector<double> ghost { 5.0, 11.0 };
    std::vector<double> roll;
    float swingBias = 0.0f;
    float densityBias = 0.0f;
};

std::vector<double> all16()
{
    return { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
}

std::vector<double> eighths()
{
    return { 0, 2, 4, 6, 8, 10, 12, 14 };
}

std::vector<double> offbeats()
{
    return { 2, 6, 10, 14 };
}

void mergeSteps (std::vector<double>& target, std::initializer_list<double> values)
{
    target.insert (target.end(), values.begin(), values.end());
    std::sort (target.begin(), target.end());
    target.erase (std::unique (target.begin(), target.end()), target.end());
}

bool anyOf (int style, std::initializer_list<int> styles)
{
    return std::find (styles.begin(), styles.end(), style) != styles.end();
}

bool isKickRole (int note) { return note == 35 || note == 36; }
bool isSnareRole (int note) { return note == 37 || note == 38 || note == 39 || note == 40; }
bool isHatRole (int note) { return note == 42 || note == 44 || note == 46 || note == 51 || note == 53 || note == 59; }
bool isTomRole (int note) { return note == 41 || note == 43 || note == 45 || note == 47 || note == 48 || note == 50; }
bool isCrashRole (int note) { return note == 49 || note == 52 || note == 55 || note == 57; }
bool isPercRole (int note)
{
    return note == 54 || note == 56 || note == 58 || note == 60 || note == 61 || note == 62 || note == 63
        || note == 64 || note == 65 || note == 67 || note == 68 || note == 69 || note == 70 || note == 75 || note == 76;
}

int roleIndexForNote (int note)
{
    if (isKickRole (note)) return 0;
    if (isSnareRole (note)) return 1;
    if (isHatRole (note)) return 2;
    if (isTomRole (note)) return 3;
    if (isCrashRole (note)) return 4;
    if (isPercRole (note)) return 5;
    return 6;
}

int autoStyleForNote (int triggerNote, std::mt19937& rng)
{
    if (isTomRole (triggerNote))
        return cinematicToms;
    if (isCrashRole (triggerNote))
        return rockBackbeat;
    if (triggerNote == closedHat || triggerNote == openHat)
        return trapHats;

    static constexpr std::array<int, 8> choices
    {{
        funkPocket, rockBackbeat, boomBap, houseFour, discoFunk, motownSoul, analogElectro, gameBeat
    }};
    return choices[static_cast<std::size_t> (std::uniform_int_distribution<int> (0, static_cast<int> (choices.size()) - 1) (rng))];
}

int noteForRole (int role)
{
    switch (role)
    {
        case 0: return kick;
        case 1: return snare;
        case 2: return closedHat;
        case 3: return lowTom;
        case 4: return crash;
        case 5: return clave;
        default: break;
    }
    return rim;
}

Template makeTemplate (int style, int triggerRole, int phrase, const Settings& settings)
{
    Template t;
    t.style = style;
    t.phrase = phrase;
    const auto all = all16();
    const auto eighth = eighths();
    const auto off = offbeats();

    if (anyOf (style, { rockBackbeat, punkDrive }))
    {
        t.kicks = { 0, 8, 10 };
        t.kickExtra = { 3, 6, 14 };
        t.hats = style == punkDrive ? all : eighth;
        t.openHats = { 14 };
        t.crashes = { 0 };
    }

    if (anyOf (style, { funkPocket, gospelShout, discoFunk, motownSoul }))
    {
        t.kicks = { 0, 3, 6, 10, 14 };
        t.kickExtra = { 8, 11 };
        t.snares = { 4, 12 };
        t.hats = all;
        t.openHats = { 7, 15 };
        t.ghost = { 2, 5, 7, 11, 13, 15 };
        t.densityBias = 0.15f;
    }

    if (style == boomBap)
    {
        t.kicks = { 0, 6, 10 };
        t.kickExtra = { 2, 14 };
        t.snares = { 4, 12 };
        t.hats = eighth;
        t.ghost = { 7, 11, 15 };
        t.swingBias = 0.16f;
    }

    if (style == trapHats)
    {
        t.kicks = { 0, 7, 10, 14 };
        t.snares = { 4, 12 };
        t.hats = all;
        t.roll = { 11.5, 12.5, 13.5, 15.5 };
        t.openHats = { 7.5, 15 };
        t.densityBias = 0.22f;
    }

    if (anyOf (style, { houseFour, technoDrive, minimalTechno }))
    {
        t.kicks = { 0, 4, 8, 12 };
        t.snares.clear();
        t.claps = { 4, 12 };
        t.hats = style == minimalTechno ? off : eighth;
        t.openHats = off;
        t.kickExtra.clear();
        t.densityBias = style == technoDrive ? 0.18f : 0.0f;
    }

    if (style == drumAndBass)
    {
        t.kicks = { 0, 6, 10 };
        t.snares = { 4, 11 };
        t.hats = all;
        t.ghost = { 2, 7, 12.5, 14 };
        t.roll = { 15, 15.5 };
        t.densityBias = 0.28f;
    }

    if (anyOf (style, { jazzRide, shuffleBlues, bossaBrush }))
    {
        t.kicks = { 0, 10 };
        t.snares = { 4, 12 };
        t.hats.clear();
        t.rides = { 0, 3, 4, 7, 8, 11, 12, 15 };
        t.ghost = { 2.5, 6.5, 10.5, 14.5 };
        t.swingBias = style == shuffleBlues ? 0.45f : 0.28f;
    }

    if (anyOf (style, { reggaeOneDrop, skaUpbeat }))
    {
        t.kicks = style == reggaeOneDrop ? std::vector<double> { 8 } : std::vector<double> { 0, 8 };
        t.snares = style == reggaeOneDrop ? std::vector<double> { 8 } : std::vector<double> { 4, 12 };
        t.hats = style == skaUpbeat ? off : eighth;
        t.rims = { 4, 12 };
        t.openHats = { 14 };
        t.swingBias = 0.08f;
    }

    if (style == reggaetonDembow)
    {
        t.kicks = { 0, 3, 8, 11 };
        t.snares.clear();
        t.claps = { 4, 12 };
        t.hats = eighth;
        t.openHats = { 15 };
        t.ghost = { 7, 14 };
    }

    if (anyOf (style, { afroCuban, salsaCascara, latinPop }))
    {
        t.kicks = { 0, 7, 11 };
        t.snares.clear();
        t.hats = eighth;
        t.percussion = { 0, 3, 6, 10, 12, 15 };
        t.claves = { 0, 3, 6, 10, 14 };
        t.openHats = { 15 };
        t.swingBias = 0.06f;
    }

    if (style == marchingCadence)
    {
        t.kicks = { 0, 8 };
        t.snares = all;
        t.hats.clear();
        t.ghost = all;
        t.roll = { 13, 13.5, 14, 14.5, 15, 15.5 };
    }

    if (anyOf (style, { cinematicToms, orchestralHits }))
    {
        t.kicks = { 0, 8 };
        t.snares.clear();
        t.hats.clear();
        t.crashes = { 0 };
        t.tomLow = { 10, 14 };
        t.tomMid = { 6, 12 };
        t.tomHigh = { 4, 11, 15 };
        t.densityBias = 0.12f;
    }

    if (style == analogElectro)
    {
        t.kicks = { 0, 6, 8, 14 };
        t.snares = { 4, 12 };
        t.hats = all;
        t.openHats = { 6, 14 };
        t.claps = { 12 };
        t.densityBias = 0.12f;
    }

    if (anyOf (style, { gameBeat, chiptuneBreak }))
    {
        t.kicks = { 0, 6, 10 };
        t.snares = { 4, 12 };
        t.hats = all;
        t.openHats.clear();
        t.roll = { 7.5, 15.5 };
        t.densityBias = 0.18f;
    }

    if (settings.trigger == kickFocus || triggerRole == 0)
        mergeSteps (t.kicks, { 0, 3, 8, 11 });
    if (settings.trigger == snareFocus || triggerRole == 1)
        mergeSteps (t.ghost, { 5, 7, 11, 15 });
    if (settings.trigger == hatFocus || triggerRole == 2)
        t.hats = all;
    if (settings.trigger == tomFill || triggerRole == 3)
        t.phrase = fill;
    if (settings.trigger == percussionFocus || triggerRole == 5)
        mergeSteps (t.percussion, { 0, 3, 6, 9, 12, 15 });
    if (settings.trigger == crashTransition || triggerRole == 4)
        t.phrase = fill;

    switch (t.phrase)
    {
        case fill:
            t.kicks = { 0 };
            t.snares = { 4 };
            t.hats = { 0, 2, 4, 6 };
            t.openHats.clear();
            t.crashes = { 0 };
            t.tomHigh = { 8, 10, 15 };
            t.tomMid = { 11, 13 };
            t.tomLow = { 12, 14 };
            t.roll = { 14.5, 15.5 };
            break;
        case pickup:
            t.kicks = { 0, 6, 10 };
            t.snares = { 12 };
            t.hats = eighth;
            t.tomHigh = { 13, 15 };
            t.tomMid = { 14 };
            t.crashes.clear();
            break;
        case breakdown:
            t.kicks = { 0, 10 };
            t.snares = { 12 };
            t.claps.clear();
            t.hats = { 0, 4, 8, 12 };
            t.openHats.clear();
            t.percussion = { 3, 7, 11, 15 };
            t.ghost.clear();
            break;
        case build:
            mergeSteps (t.kicks, { 0, 4, 8, 12 });
            t.hats = all;
            mergeSteps (t.snares, { 4, 12 });
            mergeSteps (t.roll, { 12.5, 13, 13.5, 14, 14.5, 15, 15.5 });
            break;
        case ending:
            t.kicks = { 0, 8 };
            t.snares = { 4 };
            t.hats = { 0, 2, 4, 6 };
            t.crashes = { 0, 15 };
            t.tomHigh = { 8, 10 };
            t.tomMid = { 11, 13 };
            t.tomLow = { 14, 15 };
            break;
        default:
            break;
    }

    return t;
}

void addEvent (std::vector<Event>& events,
               std::mt19937& rng,
               int note,
               double step,
               int bar,
               float density,
               const Settings& settings,
               const Template& t,
               float velocity,
               float durationScale,
               float probability,
               bool core = false)
{
    std::uniform_real_distribution<float> unit (0.0f, 1.0f);
    if (unit (rng) > probability)
        return;

    constexpr auto gridPpq = 0.25;
    auto swingAmount = juce::jlimit (0.0f, 1.0f, settings.swing + t.swingBias);
    if (settings.feel == shuffle)
        swingAmount = juce::jmax (swingAmount, 0.55f);
    if (settings.feel == machineTight)
        swingAmount = 0.0f;

    auto ppq = static_cast<double> (bar * 4) + step * gridPpq;
    if (std::abs (step - std::round (step)) < 0.0001 && (static_cast<int> (std::round (step)) & 1) != 0)
        ppq += gridPpq * swingAmount * 0.45;
    if (settings.feel == laidBack && (note == snare || note == clap || note == rim))
        ppq += gridPpq * 0.08;
    if (settings.feel == pushed && (note == kick || note == crash))
        ppq -= gridPpq * 0.06;
    if (settings.feel == tripletRoll && (note == closedHat || note == snare) && static_cast<int> (std::round (step * 2.0)) % 3 == 2)
        ppq += gridPpq * 0.18;

    const auto human = settings.feel == machineTight ? 0.0f : settings.humanize;
    std::uniform_real_distribution<float> bipolar (-1.0f, 1.0f);
    ppq += static_cast<double> (bipolar (rng) * human * 0.095f);
    ppq = juce::jlimit (static_cast<double> (bar * 4), static_cast<double> (bar * 4 + 3.99), ppq);

    auto vel = velocity * (core ? 1.0f : 0.86f) * (1.0f + bipolar (rng) * (settings.variation * 0.11f + human * 0.12f));
    if (note == closedHat || note == ride)
        vel *= 0.78f + density * 0.22f;
    if ((note == snare || note == rim) && velocity < 65.0f)
        vel *= 0.72f + settings.ghosts * 0.18f;

    events.push_back ({ ppq,
                        juce::jmax (0.045, gridPpq * static_cast<double> (durationScale)),
                        note,
                        juce::jlimit (24, 127, static_cast<int> (std::round (vel))) });
}

juce::String styleDisplayName (int style)
{
    return Engine::styleNames()[juce::jlimit (0, Engine::styleNames().size() - 1, style)];
}

juce::String phraseDisplayName (int phrase)
{
    return Engine::phraseNames()[juce::jlimit (0, Engine::phraseNames().size() - 1, phrase)];
}
}

juce::StringArray Engine::styleNames()
{
    return { "Auto Groove", "Rock Backbeat", "Funk Pocket", "Gospel Shout", "Boom Bap", "Trap Hats",
             "House Four-on-Floor", "Techno Drive", "Drum and Bass", "Jazz Ride", "Shuffle Blues",
             "Reggae One Drop", "Ska Upbeat", "Reggaeton Dembow", "Afro-Cuban", "Salsa Cascara",
             "Bossa Brush", "Latin Pop", "Marching Cadence", "Cinematic Toms", "Orchestral Hits",
             "Analog Electro", "Minimal Techno", "Disco Funk", "Motown Soul", "Punk Drive",
             "Game Beat", "Chiptune Break" };
}

juce::StringArray Engine::triggerNames()
{
    return { "Auto", "Follow Pad", "Kick Focus", "Snare Focus", "Hat Focus", "Tom Fill",
             "Percussion Focus", "Crash Transition" };
}

juce::StringArray Engine::phraseNames()
{
    return { "Groove", "Fill", "Pickup", "Breakdown", "Build", "Ending" };
}

juce::StringArray Engine::feelNames()
{
    return { "Straight", "Loose Swing", "Shuffle", "Laid Back", "Pushed", "Machine Tight", "Triplet Roll" };
}

juce::String Engine::roleNameForGMNote (int note)
{
    if (isKickRole (note)) return "Kick";
    if (note == rim) return "Rim";
    if (note == clap) return "Clap";
    if (isSnareRole (note)) return "Snare";
    if (note == closedHat || note == pedalHat) return "Hat";
    if (note == openHat) return "Open Hat";
    if (note == ride || note == 53 || note == 59) return "Ride";
    if (isCrashRole (note)) return "Crash";
    if (isTomRole (note)) return "Tom";
    if (isPercRole (note)) return "Percussion";
    return "GM " + juce::String (note);
}

Settings Engine::sanitized (Settings settings)
{
    settings.style = juce::jlimit (0, styleNames().size() - 1, settings.style);
    settings.trigger = juce::jlimit (0, triggerNames().size() - 1, settings.trigger);
    settings.phrase = juce::jlimit (0, phraseNames().size() - 1, settings.phrase);
    settings.feel = juce::jlimit (0, feelNames().size() - 1, settings.feel);
    settings.lengthBars = juce::jlimit (1, 8, settings.lengthBars);
    settings.density = juce::jlimit (0.0f, 1.0f, settings.density);
    settings.swing = juce::jlimit (0.0f, 1.0f, settings.swing);
    settings.humanize = juce::jlimit (0.0f, 1.0f, settings.humanize);
    settings.fill = juce::jlimit (0.0f, 1.0f, settings.fill);
    settings.variation = juce::jlimit (0.0f, 1.0f, settings.variation);
    settings.ghosts = juce::jlimit (0.0f, 1.0f, settings.ghosts);
    return settings;
}

Phrase Engine::generate (int triggerNote, int velocity, const Settings& rawSettings, std::uint32_t seed) const
{
    auto settings = sanitized (rawSettings);
    std::mt19937 rng (seed == 0 ? 0x6772767aU : seed);
    const auto triggerRole = roleIndexForNote (triggerNote);
    auto style = settings.style == autoGroove ? autoStyleForNote (triggerNote, rng) : settings.style;
    if (settings.trigger == followPad && triggerRole == 3 && settings.phrase == groove)
        settings.phrase = fill;
    auto t = makeTemplate (style, triggerRole, settings.phrase, settings);

    std::vector<Event> events;
    events.reserve (static_cast<std::size_t> (settings.lengthBars * 40));
    const auto density = juce::jlimit (0.0f, 1.0f, settings.density + t.densityBias);
    const auto coreProb = 0.86f + density * 0.14f;
    const auto extraProb = 0.18f + density * 0.58f + settings.variation * 0.18f;
    const auto baseVelocity = static_cast<float> (juce::jlimit (1, 127, velocity));

    for (int bar = 0; bar < settings.lengthBars; ++bar)
    {
        const auto buildGain = t.phrase == build ? static_cast<float> (bar) / juce::jmax (1.0f, static_cast<float> (settings.lengthBars - 1)) : 0.0f;
        for (auto step : t.kicks) addEvent (events, rng, kick, step, bar, density, settings, t, baseVelocity, 0.82f, coreProb, true);
        for (auto step : t.kickExtra) addEvent (events, rng, kick, step, bar, density, settings, t, baseVelocity * 0.86f, 0.70f, extraProb * 0.75f);
        for (auto step : t.snares) addEvent (events, rng, snare, step, bar, density, settings, t, baseVelocity * 0.98f, 0.72f, coreProb, true);
        for (auto step : t.claps) addEvent (events, rng, clap, step, bar, density, settings, t, baseVelocity * 0.90f, 0.70f, coreProb, true);

        auto hats = t.hats;
        if (density < 0.38f)
            hats.erase (std::remove_if (hats.begin(), hats.end(), [] (double step) { return std::fmod (step, 4.0) != 0.0 && std::fmod (step, 4.0) != 2.0; }), hats.end());
        if (density > 0.72f)
            mergeSteps (hats, { 1, 3, 5, 7, 9, 11, 13, 15 });
        for (auto step : hats) addEvent (events, rng, closedHat, step, bar, density, settings, t, baseVelocity * (0.55f + density * 0.24f + buildGain * 0.12f), 0.38f, 0.72f + density * 0.28f);
        for (auto step : t.openHats) addEvent (events, rng, openHat, step, bar, density, settings, t, baseVelocity * 0.72f, 1.15f, 0.35f + density * 0.48f);
        for (auto step : t.rides) addEvent (events, rng, ride, step, bar, density, settings, t, baseVelocity * 0.70f, 0.80f, 0.75f);
        for (auto step : t.percussion) addEvent (events, rng, settings.trigger == percussionFocus ? noteForRole (triggerRole) : cowbell, step, bar, density, settings, t, baseVelocity * 0.68f, 0.45f, 0.25f + density * 0.45f);
        for (auto step : t.claves) addEvent (events, rng, clave, step, bar, density, settings, t, baseVelocity * 0.66f, 0.42f, 0.45f + density * 0.38f);
        for (auto step : t.ghost) addEvent (events, rng, (std::uniform_int_distribution<int> (0, 1) (rng) == 0 ? rim : snare), step, bar, density, settings, t, baseVelocity * 0.45f, 0.42f, settings.ghosts * (0.45f + density * 0.40f));
        for (auto step : t.tomHigh) addEvent (events, rng, highTom, step, bar, density, settings, t, baseVelocity * 0.82f, 0.80f, 0.36f + settings.fill * 0.55f + buildGain * 0.20f);
        for (auto step : t.tomMid) addEvent (events, rng, midTom, step, bar, density, settings, t, baseVelocity * 0.86f, 0.85f, 0.36f + settings.fill * 0.55f + buildGain * 0.20f);
        for (auto step : t.tomLow) addEvent (events, rng, lowTom, step, bar, density, settings, t, baseVelocity * 0.90f, 0.90f, 0.36f + settings.fill * 0.55f + buildGain * 0.20f);
        for (auto step : t.roll) addEvent (events, rng, t.phrase == build ? snare : closedHat, step, bar, density, settings, t, baseVelocity * (0.50f + buildGain * 0.32f), 0.30f, 0.18f + density * 0.30f + settings.fill * 0.35f);
        for (auto step : t.crashes)
            if (bar == 0 || bar == settings.lengthBars - 1)
                addEvent (events, rng, crash, step, bar, density, settings, t, baseVelocity * 0.90f, 2.20f, 0.55f + settings.fill * 0.40f);
    }

    if (triggerNote >= 0 && triggerNote <= 127)
    {
        const auto containsFocus = std::any_of (events.begin(), events.end(), [triggerNote] (const auto& event)
        {
            return event.note == triggerNote && event.ppq < 1.0;
        });
        if (! containsFocus)
            events.push_back ({ 0.0, 0.18, triggerNote, juce::jlimit (30, 127, velocity) });
    }

    std::sort (events.begin(), events.end(), [] (const auto& a, const auto& b)
    {
        if (std::abs (a.ppq - b.ppq) > 0.000001)
            return a.ppq < b.ppq;
        if (a.note != b.note)
            return a.note < b.note;
        return a.velocity > b.velocity;
    });

    std::vector<Event> merged;
    merged.reserve (events.size());
    for (const auto& event : events)
    {
        if (event.ppq < 0.0 || event.ppq >= settings.lengthBars * 4.0)
            continue;
        if (! merged.empty() && merged.back().note == event.note && std::abs (merged.back().ppq - event.ppq) < 0.0001)
        {
            if (event.velocity > merged.back().velocity)
                merged.back() = event;
        }
        else
        {
            merged.push_back (event);
        }
    }

    Phrase phrase;
    phrase.lengthBars = settings.lengthBars;
    phrase.name = styleDisplayName (style) + " " + phraseDisplayName (t.phrase);
    phrase.events = std::move (merged);
    return phrase;
}
}
