#pragma once

#include <JuceHeader.h>

#include <cstdint>
#include <mutex>
#include <vector>

namespace Soli
{
struct LinkedChordRegion
{
    double startPpq = 0.0;
    double endPpq = 0.0;
    float confidence = 1.0f;
    juce::String name;
    juce::StringArray alternatives;
};

struct ChordizerSnapshot
{
    bool connected = false;
    bool playing = false;
    double playheadPpq = 0.0;
    double bpm = 120.0;
    int numerator = 4;
    int denominator = 4;
    std::uint64_t revision = 0;
    std::vector<LinkedChordRegion> regions;
};

struct ChordizerContext
{
    bool connected = false;
    juce::String previous;
    juce::String current;
    juce::String next;
    double currentStartPpq = 0.0;
    double currentEndPpq = 0.0;
};

class ChordizerLink
{
public:
    ChordizerLink();
    ~ChordizerLink();

    ChordizerSnapshot snapshot (bool tryOnly = false) const;
    ChordizerContext contextAt (double ppq, bool tryOnly = true) const;
    bool isConnected() const noexcept { return sharedMemory != nullptr; }

private:
    void connect();

    int sharedFile = -1;
    void* sharedMemory = nullptr;
    mutable std::mutex connectionMutex;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChordizerLink)
};
}
