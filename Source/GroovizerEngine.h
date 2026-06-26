#pragma once

#include <JuceHeader.h>

#include <cstdint>
#include <vector>

namespace Groovizer
{
struct Settings
{
    int style = 0;
    int trigger = 0;
    int phrase = 0;
    int feel = 0;
    int lengthBars = 1;
    float density = 0.55f;
    float swing = 0.12f;
    float humanize = 0.12f;
    float fill = 0.25f;
    float variation = 0.42f;
    float ghosts = 0.28f;
};

struct Event
{
    double ppq = 0.0;
    double duration = 0.125;
    int note = 36;
    int velocity = 96;
};

struct Phrase
{
    juce::String name;
    int lengthBars = 1;
    std::vector<Event> events;
};

class Engine final
{
public:
    static juce::StringArray styleNames();
    static juce::StringArray triggerNames();
    static juce::StringArray phraseNames();
    static juce::StringArray feelNames();
    static juce::String roleNameForGMNote (int note);
    static Settings sanitized (Settings settings);

    Phrase generate (int triggerNote, int velocity, const Settings& settings, std::uint32_t seed) const;
};
}
