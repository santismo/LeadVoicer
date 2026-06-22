#include "ChordizerLink.h"

#include <fcntl.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

namespace Soli
{
namespace
{
constexpr std::uint32_t sharedMagic = 0x43545348;
constexpr std::uint32_t sharedVersion = 4;
constexpr std::size_t maximumSharedRegions = 2048;
constexpr std::size_t wireNameLength = 48;
constexpr std::size_t maximumAlternatives = 4;

struct SharedRegionWire
{
    double startPpq = 0.0, endPpq = 0.0;
    float confidence = 1.0f;
    std::uint8_t source = 0, locked = 0;
    char name[wireNameLength] {};
    char alternatives[maximumAlternatives][wireNameLength] {};
};

struct SharedSessionWire
{
    std::uint32_t magic = 0, version = 0;
    pthread_mutex_t mutex {};
    std::uint64_t revision = 1, nextID = 1;
    std::int32_t instances = 0, numerator = 4, denominator = 4;
    double playhead = 0.0, bpm = 120.0, lastMidiPpq = -1000.0;
    std::uint8_t playing = 0;
    float timelineTextScale = 1.0f, leadTextScale = 1.0f;
    std::uint32_t regionCount = 0;
    SharedRegionWire regions[maximumSharedRegions] {};
};

class WireLock
{
public:
    WireLock (SharedSessionWire* state, bool tryOnly) : wire (state)
    {
        if (wire == nullptr)
            return;
        const auto result = tryOnly ? pthread_mutex_trylock (&wire->mutex)
                                    : pthread_mutex_lock (&wire->mutex);
        locked = result == 0;
    }

    ~WireLock()
    {
        if (locked)
            pthread_mutex_unlock (&wire->mutex);
    }

    bool ownsLock() const noexcept { return locked; }

private:
    SharedSessionWire* wire = nullptr;
    bool locked = false;
};

LinkedChordRegion regionFromWire (const SharedRegionWire& wire)
{
    LinkedChordRegion result;
    result.startPpq = wire.startPpq;
    result.endPpq = wire.endPpq;
    result.confidence = wire.confidence;
    result.name = juce::String::fromUTF8 (wire.name);
    for (std::size_t i = 0; i < maximumAlternatives; ++i)
        if (wire.alternatives[i][0] != 0)
            result.alternatives.add (juce::String::fromUTF8 (wire.alternatives[i]));
    return result;
}

bool covers (const SharedRegionWire& region, double ppq)
{
    return ppq >= region.startPpq - 0.03125
        && ppq < juce::jmax (region.endPpq, region.startPpq + 0.03125);
}
}

ChordizerLink::ChordizerLink()
{
    connect();
}

ChordizerLink::~ChordizerLink()
{
    if (sharedMemory != nullptr)
        ::munmap (sharedMemory, sizeof (SharedSessionWire));
    if (sharedFile >= 0)
        ::close (sharedFile);
}

void ChordizerLink::connect()
{
    const std::lock_guard<std::mutex> guard (connectionMutex);
    if (sharedMemory != nullptr)
        return;
    const auto path = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
        .getChildFile ("Application Support")
        .getChildFile ("Santismo")
        .getChildFile ("Chordizer")
        .getChildFile ("Shared Session.session");

    sharedFile = ::open (path.getFullPathName().toRawUTF8(), O_RDWR);
    if (sharedFile < 0)
        return;

    struct stat info {};
    if (::fstat (sharedFile, &info) != 0 || info.st_size < static_cast<off_t> (sizeof (SharedSessionWire)))
    {
        ::close (sharedFile);
        sharedFile = -1;
        return;
    }

    auto* wire = static_cast<SharedSessionWire*> (::mmap (nullptr, sizeof (SharedSessionWire),
                                                          PROT_READ | PROT_WRITE, MAP_SHARED,
                                                          sharedFile, 0));
    if (wire == MAP_FAILED)
    {
        ::close (sharedFile);
        sharedFile = -1;
        return;
    }

    if (wire->magic != sharedMagic || wire->version != sharedVersion)
    {
        ::munmap (wire, sizeof (SharedSessionWire));
        ::close (sharedFile);
        sharedFile = -1;
        return;
    }

    sharedMemory = wire;
}

ChordizerSnapshot ChordizerLink::snapshot (bool tryOnly) const
{
    if (sharedMemory == nullptr && ! tryOnly)
        const_cast<ChordizerLink*> (this)->connect();
    ChordizerSnapshot result;
    auto* wire = static_cast<SharedSessionWire*> (sharedMemory);
    WireLock lock (wire, tryOnly);
    if (! lock.ownsLock())
        return result;

    result.connected = true;
    result.playing = wire->playing != 0;
    result.playheadPpq = wire->playhead;
    result.bpm = wire->bpm;
    result.numerator = wire->numerator;
    result.denominator = wire->denominator;
    result.revision = wire->revision;
    const auto count = juce::jmin<std::uint32_t> (wire->regionCount, static_cast<std::uint32_t> (maximumSharedRegions));
    result.regions.reserve (count);
    for (std::uint32_t i = 0; i < count; ++i)
        result.regions.push_back (regionFromWire (wire->regions[i]));
    return result;
}

ChordizerContext ChordizerLink::contextAt (double ppq, bool tryOnly) const
{
    ChordizerContext result;
    auto* wire = static_cast<SharedSessionWire*> (sharedMemory);
    WireLock lock (wire, tryOnly);
    if (! lock.ownsLock())
        return result;

    result.connected = true;
    const auto count = juce::jmin<std::uint32_t> (wire->regionCount, static_cast<std::uint32_t> (maximumSharedRegions));
    for (std::uint32_t i = 0; i < count; ++i)
    {
        if (! covers (wire->regions[i], ppq))
            continue;

        result.current = juce::String::fromUTF8 (wire->regions[i].name);
        result.currentStartPpq = wire->regions[i].startPpq;
        result.currentEndPpq = wire->regions[i].endPpq;
        if (i > 0)
            result.previous = juce::String::fromUTF8 (wire->regions[i - 1].name);
        if (i + 1 < count)
            result.next = juce::String::fromUTF8 (wire->regions[i + 1].name);
        break;
    }
    return result;
}
}
