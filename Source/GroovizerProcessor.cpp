#include "GroovizerProcessor.h"
#include "GroovizerEditor.h"

#include <algorithm>
#include <cmath>

namespace
{
juce::StringArray copyNames (const juce::StringArray& names)
{
    juce::StringArray result;
    for (auto name : names)
        result.add (name);
    return result;
}
}

GroovizerAudioProcessor::GroovizerAudioProcessor()
    : AudioProcessor (BusesProperties()),
      parameters (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    recordedNoteRefs.fill (0);
}

void GroovizerAudioProcessor::prepareToPlay (double, int)
{
    processedSamples = 0;
    pendingMidi.clear();
    lastTransportPlaying = false;
    lastTransportPpq = -1.0;
}

bool GroovizerAudioProcessor::isBusesLayoutSupported (const BusesLayout&) const
{
    return true;
}

int GroovizerAudioProcessor::refIndex (int channel, int note) noexcept
{
    return juce::jlimit (0, 15, channel - 1) * 128 + juce::jlimit (0, 127, note);
}

Groovizer::Settings GroovizerAudioProcessor::readSettings() const
{
    Groovizer::Settings settings;
    settings.style = static_cast<int> (*parameters.getRawParameterValue (GroovizerParameterIDs::style));
    settings.trigger = static_cast<int> (*parameters.getRawParameterValue (GroovizerParameterIDs::trigger));
    settings.phrase = static_cast<int> (*parameters.getRawParameterValue (GroovizerParameterIDs::phrase));
    settings.feel = static_cast<int> (*parameters.getRawParameterValue (GroovizerParameterIDs::feel));
    settings.lengthBars = static_cast<int> (*parameters.getRawParameterValue (GroovizerParameterIDs::lengthBars));
    settings.density = parameters.getRawParameterValue (GroovizerParameterIDs::density)->load();
    settings.swing = parameters.getRawParameterValue (GroovizerParameterIDs::swing)->load();
    settings.humanize = parameters.getRawParameterValue (GroovizerParameterIDs::humanize)->load();
    settings.fill = parameters.getRawParameterValue (GroovizerParameterIDs::fill)->load();
    settings.variation = parameters.getRawParameterValue (GroovizerParameterIDs::variation)->load();
    settings.ghosts = parameters.getRawParameterValue (GroovizerParameterIDs::ghosts)->load();
    return Groovizer::Engine::sanitized (settings);
}

int GroovizerAudioProcessor::outputChannel() const
{
    return juce::jlimit (1, 16, static_cast<int> (*parameters.getRawParameterValue (GroovizerParameterIDs::outputChannel)));
}

GroovizerAudioProcessor::Transport GroovizerAudioProcessor::readTransport() const
{
    Transport result;
    if (auto* playHead = getPlayHead())
    {
        if (const auto position = playHead->getPosition())
        {
            result.valid = true;
            result.playing = position->getIsPlaying();
            if (const auto ppq = position->getPpqPosition())
                result.ppq = *ppq;
            if (const auto bpm = position->getBpm())
                result.bpm = *bpm;
            if (const auto signature = position->getTimeSignature())
            {
                result.numerator = signature->numerator;
                result.denominator = signature->denominator;
            }
        }
    }
    return result;
}

void GroovizerAudioProcessor::emitPendingMidi (int blockSamples, juce::MidiBuffer& output)
{
    std::vector<PendingMidi> remaining;
    remaining.reserve (pendingMidi.size());

    for (auto& event : pendingMidi)
    {
        if (event.samplesUntil < blockSamples)
            output.addEvent (event.message, juce::jlimit (0, blockSamples - 1, event.samplesUntil));
        else
        {
            event.samplesUntil -= blockSamples;
            remaining.push_back (event);
        }
    }

    pendingMidi = std::move (remaining);
}

void GroovizerAudioProcessor::scheduleMidiEvent (const juce::MidiMessage& message,
                                                 int sampleOffset,
                                                 int blockSamples,
                                                 juce::MidiBuffer& output)
{
    if (sampleOffset < blockSamples)
        output.addEvent (message, juce::jlimit (0, juce::jmax (0, blockSamples - 1), sampleOffset));
    else
        pendingMidi.push_back ({ message, sampleOffset });
}

void GroovizerAudioProcessor::emitPhrase (const Groovizer::Phrase& phrase,
                                          int baseSample,
                                          const Transport& transport,
                                          int blockSamples,
                                          juce::MidiBuffer& output)
{
    if (getSampleRate() <= 0.0)
        return;

    const auto bpm = transport.valid ? transport.bpm : 120.0;
    const auto ppqPerSample = bpm / (60.0 * getSampleRate());
    if (ppqPerSample <= 0.0)
        return;

    const auto channel = outputChannel();
    for (const auto& event : phrase.events)
    {
        const auto startSample = baseSample + static_cast<int> (std::round (event.ppq / ppqPerSample));
        const auto durationSamples = juce::jmax (1, static_cast<int> (std::round (event.duration / ppqPerSample)));
        scheduleMidiEvent (juce::MidiMessage::noteOn (channel, event.note, static_cast<juce::uint8> (event.velocity)),
                           startSample, blockSamples, output);
        scheduleMidiEvent (juce::MidiMessage::noteOff (channel, event.note),
                           startSample + durationSamples, blockSamples, output);
    }
}

void GroovizerAudioProcessor::renderTimeline (const Transport& transport,
                                              int blockSamples,
                                              juce::MidiBuffer& output)
{
    if (! transport.valid || ! transport.playing || getSampleRate() <= 0.0
        || parameters.getRawParameterValue (GroovizerParameterIDs::timelineEnabled)->load() < 0.5f)
        return;

    const auto ppqPerSample = transport.bpm / (60.0 * getSampleRate());
    if (ppqPerSample <= 0.0)
        return;

    if (lastTransportPlaying && std::abs (transport.ppq - (lastTransportPpq + blockSamples * ppqPerSample)) > 0.35)
        pendingMidi.clear();

    std::vector<GrooveRegion> regions;
    {
        std::unique_lock<std::mutex> lock (timelineMutex, std::try_to_lock);
        if (! lock.owns_lock())
            return;
        regions = timelineRegions;
    }

    const auto rangeStart = transport.ppq;
    const auto rangeEnd = transport.ppq + static_cast<double> (blockSamples) * ppqPerSample;
    const auto channel = outputChannel();
    for (const auto& region : regions)
    {
        if (region.startPpq + region.lengthPpq < rangeStart || region.startPpq >= rangeEnd)
            continue;

        const auto phrase = engine.generate (region.triggerNote, region.velocity, region.settings, region.seed);
        for (const auto& event : phrase.events)
        {
            const auto eventPpq = region.startPpq + event.ppq;
            if (eventPpq < rangeStart || eventPpq >= rangeEnd)
                continue;

            const auto startSample = static_cast<int> (std::round ((eventPpq - transport.ppq) / ppqPerSample));
            const auto durationSamples = juce::jmax (1, static_cast<int> (std::round (event.duration / ppqPerSample)));
            scheduleMidiEvent (juce::MidiMessage::noteOn (channel, event.note, static_cast<juce::uint8> (event.velocity)),
                               startSample, blockSamples, output);
            scheduleMidiEvent (juce::MidiMessage::noteOff (channel, event.note),
                               startSample + durationSamples, blockSamples, output);
        }
    }
}

void GroovizerAudioProcessor::insertRegionAtCursor (int triggerNote, int velocity, const Groovizer::Settings& settings)
{
    const auto seed = seedCounter.fetch_add (1);
    const auto phrase = engine.generate (triggerNote, velocity, settings, seed);

    std::lock_guard<std::mutex> lock (timelineMutex);
    GrooveRegion region;
    region.startPpq = juce::jmax (0.0, timelineCursorPpq);
    region.lengthPpq = juce::jmax (1.0, static_cast<double> (settings.lengthBars * 4));
    region.triggerNote = juce::jlimit (0, 127, triggerNote);
    region.velocity = juce::jlimit (1, 127, velocity);
    region.seed = seed;
    region.settings = settings;
    region.name = phrase.name;
    timelineRegions.push_back (region);
    std::sort (timelineRegions.begin(), timelineRegions.end(), [] (const auto& a, const auto& b)
    {
        return a.startPpq < b.startPpq;
    });

    timelineCursorPpq = region.startPpq + region.lengthPpq;
    selectedRegion = static_cast<int> (timelineRegions.size()) - 1;

    const std::lock_guard<std::mutex> nameLock (nameMutex);
    lastGrooveName = phrase.name + " | " + Groovizer::Engine::roleNameForGMNote (triggerNote);
}

void GroovizerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    buffer.clear();

    juce::MidiBuffer output;
    const auto blockSamples = buffer.getNumSamples();
    const auto transport = readTransport();
    lastHostPpq.store (transport.ppq);
    lastHostPlaying.store (transport.playing);

    emitPendingMidi (blockSamples, output);
    renderTimeline (transport, blockSamples, output);

    const auto settings = readSettings();
    const auto live = parameters.getRawParameterValue (GroovizerParameterIDs::liveMode)->load() > 0.5f;
    const auto stepInput = parameters.getRawParameterValue (GroovizerParameterIDs::stepInput)->load() > 0.5f;
    const auto passInput = parameters.getRawParameterValue (GroovizerParameterIDs::passInput)->load() > 0.5f;

    for (const auto metadata : midiMessages)
    {
        const auto message = metadata.getMessage();
        const auto samplePosition = metadata.samplePosition;

        if (message.isNoteOn())
        {
            const auto triggerNote = message.getNoteNumber();
            const auto velocity = message.getVelocity();

            if (stepInput)
                insertRegionAtCursor (triggerNote, velocity, settings);

            if (live)
            {
                const auto seed = seedCounter.fetch_add (1);
                auto phrase = engine.generate (triggerNote, velocity, settings, seed);
                emitPhrase (phrase, samplePosition, transport, blockSamples, output);
                const std::lock_guard<std::mutex> lock (nameMutex);
                lastGrooveName = phrase.name + " | " + Groovizer::Engine::roleNameForGMNote (triggerNote);
            }

            if (passInput)
                output.addEvent (message, samplePosition);
        }
        else if (message.isNoteOff())
        {
            if (passInput)
                output.addEvent (message, samplePosition);
        }
        else if (message.isAllNotesOff() || message.isAllSoundOff())
        {
            pendingMidi.clear();
            output.addEvent (juce::MidiMessage::allNotesOff (outputChannel()), samplePosition);
            if (passInput)
                output.addEvent (message, samplePosition);
        }
        else
        {
            output.addEvent (message, samplePosition);
        }
    }

    recordOutputMidi (output, transport, blockSamples);
    midiMessages.swapWith (output);
    processedSamples += blockSamples;
    lastTransportPlaying = transport.playing;
    lastTransportPpq = transport.ppq;
}

juce::AudioProcessorEditor* GroovizerAudioProcessor::createEditor()
{
    return new GroovizerAudioProcessorEditor (*this);
}

void GroovizerAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    juce::ValueTree timeline ("timeline");
    {
        std::lock_guard<std::mutex> lock (timelineMutex);
        timeline.setProperty ("cursorPpq", timelineCursorPpq, nullptr);
        timeline.setProperty ("selectedRegion", selectedRegion, nullptr);
        for (const auto& region : timelineRegions)
        {
            juce::ValueTree item ("region");
            item.setProperty ("startPpq", region.startPpq, nullptr);
            item.setProperty ("lengthPpq", region.lengthPpq, nullptr);
            item.setProperty ("triggerNote", region.triggerNote, nullptr);
            item.setProperty ("velocity", region.velocity, nullptr);
            item.setProperty ("seed", static_cast<int> (region.seed), nullptr);
            item.setProperty ("name", region.name, nullptr);
            item.setProperty ("style", region.settings.style, nullptr);
            item.setProperty ("trigger", region.settings.trigger, nullptr);
            item.setProperty ("phrase", region.settings.phrase, nullptr);
            item.setProperty ("feel", region.settings.feel, nullptr);
            item.setProperty ("lengthBars", region.settings.lengthBars, nullptr);
            item.setProperty ("density", region.settings.density, nullptr);
            item.setProperty ("swing", region.settings.swing, nullptr);
            item.setProperty ("humanize", region.settings.humanize, nullptr);
            item.setProperty ("fill", region.settings.fill, nullptr);
            item.setProperty ("variation", region.settings.variation, nullptr);
            item.setProperty ("ghosts", region.settings.ghosts, nullptr);
            timeline.addChild (item, -1, nullptr);
        }
    }
    state.addChild (timeline, -1, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void GroovizerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        auto state = juce::ValueTree::fromXml (*xml);
        if (! state.isValid() || ! state.hasType (parameters.state.getType()))
            return;

        auto timeline = state.getChildWithName ("timeline");
        if (timeline.isValid())
        {
            std::lock_guard<std::mutex> lock (timelineMutex);
            timelineRegions.clear();
            timelineCursorPpq = static_cast<double> (timeline.getProperty ("cursorPpq", 0.0));
            selectedRegion = static_cast<int> (timeline.getProperty ("selectedRegion", -1));
            for (int i = 0; i < timeline.getNumChildren(); ++i)
            {
                auto item = timeline.getChild (i);
                if (! item.hasType ("region"))
                    continue;
                GrooveRegion region;
                region.startPpq = static_cast<double> (item.getProperty ("startPpq", 0.0));
                region.lengthPpq = static_cast<double> (item.getProperty ("lengthPpq", 4.0));
                region.triggerNote = static_cast<int> (item.getProperty ("triggerNote", 36));
                region.velocity = static_cast<int> (item.getProperty ("velocity", 100));
                region.seed = static_cast<std::uint32_t> (static_cast<int> (item.getProperty ("seed", 1)));
                region.name = item.getProperty ("name", "Groove").toString();
                region.settings.style = static_cast<int> (item.getProperty ("style", 0));
                region.settings.trigger = static_cast<int> (item.getProperty ("trigger", 0));
                region.settings.phrase = static_cast<int> (item.getProperty ("phrase", 0));
                region.settings.feel = static_cast<int> (item.getProperty ("feel", 0));
                region.settings.lengthBars = static_cast<int> (item.getProperty ("lengthBars", 1));
                region.settings.density = static_cast<float> (item.getProperty ("density", 0.55f));
                region.settings.swing = static_cast<float> (item.getProperty ("swing", 0.12f));
                region.settings.humanize = static_cast<float> (item.getProperty ("humanize", 0.12f));
                region.settings.fill = static_cast<float> (item.getProperty ("fill", 0.25f));
                region.settings.variation = static_cast<float> (item.getProperty ("variation", 0.42f));
                region.settings.ghosts = static_cast<float> (item.getProperty ("ghosts", 0.28f));
                region.settings = Groovizer::Engine::sanitized (region.settings);
                timelineRegions.push_back (region);
            }
            state.removeChild (timeline, nullptr);
        }

        parameters.replaceState (state);
    }
}

juce::String GroovizerAudioProcessor::getLastGrooveName() const
{
    const std::lock_guard<std::mutex> lock (nameMutex);
    return lastGrooveName;
}

GroovizerAudioProcessor::TimelineSnapshot GroovizerAudioProcessor::getTimelineSnapshot() const
{
    TimelineSnapshot snapshot;
    snapshot.enabled = parameters.getRawParameterValue (GroovizerParameterIDs::timelineEnabled)->load() > 0.5f;
    snapshot.hostPlaying = lastHostPlaying.load();
    snapshot.hostPpq = lastHostPpq.load();
    {
        std::lock_guard<std::mutex> lock (timelineMutex);
        snapshot.cursorPpq = timelineCursorPpq;
        snapshot.selectedIndex = selectedRegion;
        snapshot.regions = timelineRegions;
    }
    return snapshot;
}

void GroovizerAudioProcessor::setTimelineCursor (double ppq)
{
    std::lock_guard<std::mutex> lock (timelineMutex);
    timelineCursorPpq = juce::jmax (0.0, ppq);
}

void GroovizerAudioProcessor::moveTimelineCursorBars (int bars)
{
    std::lock_guard<std::mutex> lock (timelineMutex);
    timelineCursorPpq = juce::jmax (0.0, timelineCursorPpq + static_cast<double> (bars * 4));
}

void GroovizerAudioProcessor::selectRegionAt (double ppq)
{
    std::lock_guard<std::mutex> lock (timelineMutex);
    selectedRegion = -1;
    for (int i = 0; i < static_cast<int> (timelineRegions.size()); ++i)
    {
        const auto& region = timelineRegions[static_cast<std::size_t> (i)];
        if (ppq >= region.startPpq && ppq <= region.startPpq + region.lengthPpq)
            selectedRegion = i;
    }
    timelineCursorPpq = juce::jmax (0.0, ppq);
}

void GroovizerAudioProcessor::addRegionAtCursor()
{
    insertRegionAtCursor (36, 100, readSettings());
}

void GroovizerAudioProcessor::addRandomArrangement (int regionCount)
{
    auto settings = readSettings();
    auto& random = juce::Random::getSystemRandom();
    const std::array<int, 6> triggers {{ 36, 38, 42, 46, 49, 56 }};
    for (int i = 0; i < juce::jlimit (1, 32, regionCount); ++i)
    {
        settings.style = random.nextInt (Groovizer::Engine::styleNames().size());
        settings.phrase = i == regionCount - 1 && random.nextBool() ? 5
                        : (random.nextFloat() < 0.22f ? 1 : 0);
        settings.lengthBars = 1 + random.nextInt (juce::jmax (1, static_cast<int> (*parameters.getRawParameterValue (GroovizerParameterIDs::lengthBars))));
        settings.fill = juce::jlimit (0.0f, 1.0f, settings.fill + random.nextFloat() * 0.25f);
        insertRegionAtCursor (triggers[static_cast<std::size_t> (random.nextInt (static_cast<int> (triggers.size())))],
                              88 + random.nextInt (32),
                              settings);
    }
}

void GroovizerAudioProcessor::deleteSelectedRegion()
{
    std::lock_guard<std::mutex> lock (timelineMutex);
    if (selectedRegion < 0 || selectedRegion >= static_cast<int> (timelineRegions.size()))
        return;
    timelineRegions.erase (timelineRegions.begin() + selectedRegion);
    selectedRegion = juce::jmin (selectedRegion, static_cast<int> (timelineRegions.size()) - 1);
}

void GroovizerAudioProcessor::clearTimeline()
{
    std::lock_guard<std::mutex> lock (timelineMutex);
    timelineRegions.clear();
    selectedRegion = -1;
    timelineCursorPpq = 0.0;
}

void GroovizerAudioProcessor::panic()
{
    pendingMidi.clear();
    const std::lock_guard<std::mutex> lock (nameMutex);
    lastGrooveName = "--";
}

void GroovizerAudioProcessor::closeRecordedNotesLocked (double closePpq) const
{
    if (! midiRecordHasOrigin)
        return;

    closePpq = juce::jmax (midiRecordOriginPpq, closePpq);
    for (int channel = 1; channel <= 16; ++channel)
    {
        for (int note = 0; note < 128; ++note)
        {
            auto& refs = recordedNoteRefs[static_cast<std::size_t> (refIndex (channel, note))];
            if (refs <= 0)
                continue;

            auto off = juce::MidiMessage::noteOff (channel, note);
            off.setTimeStamp (closePpq);
            recordedMidiEvents.push_back ({ closePpq, off });
            refs = 0;
        }
    }
    midiRecordEndPpq = juce::jmax (midiRecordEndPpq, closePpq);
}

void GroovizerAudioProcessor::recordOutputMidi (const juce::MidiBuffer& output,
                                                const Transport& transport,
                                                int blockSamples)
{
    const std::lock_guard<std::mutex> lock (recordingMutex);
    if (! midiRecording || getSampleRate() <= 0.0)
        return;

    const auto bpm = transport.valid ? transport.bpm : midiRecordBpm;
    const auto ppqPerSample = bpm / (60.0 * getSampleRate());
    if (! midiRecordHasOrigin)
    {
        midiRecordOriginSample = processedSamples;
        midiRecordOriginPpq = transport.valid ? transport.ppq : 0.0;
        midiRecordEndPpq = midiRecordOriginPpq;
        midiRecordBpm = bpm;
        midiRecordNumerator = transport.valid ? transport.numerator : 4;
        midiRecordDenominator = transport.valid ? transport.denominator : 4;
        midiRecordHasOrigin = true;
    }

    const auto blockStartPpq = transport.valid
                             ? transport.ppq
                             : midiRecordOriginPpq + static_cast<double> (processedSamples - midiRecordOriginSample) * ppqPerSample;
    midiRecordEndPpq = juce::jmax (midiRecordEndPpq, blockStartPpq + static_cast<double> (juce::jmax (0, blockSamples)) * ppqPerSample);

    for (const auto metadata : output)
    {
        auto message = metadata.getMessage();
        const auto ppq = blockStartPpq + static_cast<double> (metadata.samplePosition) * ppqPerSample;
        if (message.isNoteOn())
        {
            message.setTimeStamp (ppq);
            recordedMidiEvents.push_back ({ ppq, message });
            ++recordedNoteRefs[static_cast<std::size_t> (refIndex (message.getChannel(), message.getNoteNumber()))];
        }
        else if (message.isNoteOff())
        {
            message.setTimeStamp (ppq);
            recordedMidiEvents.push_back ({ ppq, message });
            auto& refs = recordedNoteRefs[static_cast<std::size_t> (refIndex (message.getChannel(), message.getNoteNumber()))];
            refs = juce::jmax (0, refs - 1);
        }
        else if (message.isAllNotesOff() || message.isAllSoundOff())
        {
            closeRecordedNotesLocked (ppq);
        }
    }
}

void GroovizerAudioProcessor::setMidiRecordingEnabled (bool shouldRecord)
{
    const std::lock_guard<std::mutex> lock (recordingMutex);
    if (midiRecording == shouldRecord)
        return;

    if (shouldRecord)
    {
        recordedMidiEvents.clear();
        recordedMidiEvents.reserve (4096);
        recordedNoteRefs.fill (0);
        midiRecordHasOrigin = false;
        midiRecordOriginPpq = 0.0;
        midiRecordEndPpq = 0.0;
        midiRecordBpm = 120.0;
        midiRecordNumerator = 4;
        midiRecordDenominator = 4;
        midiRecordOriginSample = processedSamples;
        midiRecording = true;
    }
    else
    {
        closeRecordedNotesLocked (midiRecordEndPpq);
        midiRecording = false;
    }
}

bool GroovizerAudioProcessor::isMidiRecording() const
{
    const std::lock_guard<std::mutex> lock (recordingMutex);
    return midiRecording;
}

void GroovizerAudioProcessor::clearRecordedMidi()
{
    const std::lock_guard<std::mutex> lock (recordingMutex);
    recordedMidiEvents.clear();
    recordedNoteRefs.fill (0);
    midiRecordHasOrigin = false;
    midiRecordOriginPpq = 0.0;
    midiRecordEndPpq = 0.0;
    midiRecordOriginSample = processedSamples;
}

GroovizerAudioProcessor::RecordedMidiSnapshot GroovizerAudioProcessor::recordedMidiSnapshot() const
{
    const std::lock_guard<std::mutex> lock (recordingMutex);
    RecordedMidiSnapshot result;
    result.recording = midiRecording;
    result.hasOrigin = midiRecordHasOrigin;
    result.originPpq = midiRecordOriginPpq;
    result.endPpq = midiRecordEndPpq;
    result.bpm = midiRecordBpm;
    result.numerator = midiRecordNumerator;
    result.denominator = midiRecordDenominator;
    result.events = recordedMidiEvents;
    return result;
}

bool GroovizerAudioProcessor::writeMidiEventsToFile (const juce::File& destination,
                                                     const std::vector<RecordedMidiEvent>& sourceEvents,
                                                     double originPpq,
                                                     double endPpq,
                                                     double bpm,
                                                     int numerator,
                                                     int denominator,
                                                     const juce::String& trackName) const
{
    constexpr int ticksPerQuarter = 960;
    if (sourceEvents.empty())
        return false;

    auto events = sourceEvents;
    std::sort (events.begin(), events.end(), [] (const auto& a, const auto& b)
    {
        if (std::abs (a.ppq - b.ppq) > 0.0000001)
            return a.ppq < b.ppq;
        return a.message.isNoteOff() && b.message.isNoteOn();
    });

    juce::MidiFile file;
    file.setTicksPerQuarterNote (ticksPerQuarter);
    juce::MidiMessageSequence track;
    auto name = juce::MidiMessage::textMetaEvent (3, trackName);
    name.setTimeStamp (0.0);
    track.addEvent (name);
    auto tempo = juce::MidiMessage::tempoMetaEvent (static_cast<int> (std::llround (60000000.0 / juce::jlimit (1.0, 999.0, bpm))));
    tempo.setTimeStamp (0.0);
    track.addEvent (tempo);
    auto signature = juce::MidiMessage::timeSignatureMetaEvent (juce::jmax (1, numerator), juce::jmax (1, denominator));
    signature.setTimeStamp (0.0);
    track.addEvent (signature);

    auto lastTick = 0.0;
    for (const auto& event : events)
    {
        auto message = event.message;
        const auto tick = juce::jmax (0.0, static_cast<double> (std::llround ((event.ppq - originPpq) * ticksPerQuarter)));
        message.setTimeStamp (tick);
        track.addEvent (message);
        lastTick = juce::jmax (lastTick, tick);
    }

    const auto endTick = juce::jmax (lastTick + 1.0, static_cast<double> (std::llround ((endPpq - originPpq) * ticksPerQuarter)));
    auto end = juce::MidiMessage::endOfTrack();
    end.setTimeStamp (endTick);
    track.addEvent (end);
    track.sort();
    file.addTrack (track);

    if (! destination.getParentDirectory().createDirectory())
        return false;
    destination.deleteFile();
    juce::FileOutputStream output (destination);
    return output.openedOk() && file.writeTo (output, 1);
}

bool GroovizerAudioProcessor::writeRecordedMidiFile (const juce::File& destination) const
{
    std::vector<RecordedMidiEvent> events;
    double originPpq = 0.0;
    double endPpq = 0.0;
    double bpm = 120.0;
    int numerator = 4;
    int denominator = 4;

    {
        const std::lock_guard<std::mutex> lock (recordingMutex);
        if (! midiRecordHasOrigin || recordedMidiEvents.empty())
            return false;
        events = recordedMidiEvents;
        originPpq = midiRecordOriginPpq;
        endPpq = midiRecordEndPpq;
        bpm = midiRecordBpm;
        numerator = midiRecordNumerator;
        denominator = midiRecordDenominator;
    }

    return writeMidiEventsToFile (destination, events, originPpq, endPpq, bpm, numerator, denominator, "Groovizer MIDI Capture");
}

bool GroovizerAudioProcessor::writeTimelineMidiFile (const juce::File& destination) const
{
    std::vector<GrooveRegion> regions;
    {
        std::lock_guard<std::mutex> lock (timelineMutex);
        if (timelineRegions.empty())
            return false;
        regions = timelineRegions;
    }

    const auto channel = outputChannel();
    std::vector<RecordedMidiEvent> events;
    double endPpq = 0.0;
    for (const auto& region : regions)
    {
        const auto phrase = engine.generate (region.triggerNote, region.velocity, region.settings, region.seed);
        endPpq = juce::jmax (endPpq, region.startPpq + region.lengthPpq);
        for (const auto& event : phrase.events)
        {
            const auto start = region.startPpq + event.ppq;
            auto on = juce::MidiMessage::noteOn (channel, event.note, static_cast<juce::uint8> (event.velocity));
            on.setTimeStamp (start);
            events.push_back ({ start, on });
            auto off = juce::MidiMessage::noteOff (channel, event.note);
            off.setTimeStamp (start + event.duration);
            events.push_back ({ start + event.duration, off });
        }
    }

    return writeMidiEventsToFile (destination, events, 0.0, endPpq, 120.0, 4, 4, "Groovizer Timeline");
}

juce::AudioProcessorValueTreeState::ParameterLayout GroovizerAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    auto addChoice = [&] (const char* id, const juce::String& name, const juce::StringArray& choices, int defaultIndex)
    {
        params.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { id, 1 }, name, copyNames (choices), defaultIndex));
    };

    addChoice (GroovizerParameterIDs::style, "Groove Style", Groovizer::Engine::styleNames(), 0);
    addChoice (GroovizerParameterIDs::trigger, "Trigger", Groovizer::Engine::triggerNames(), 0);
    addChoice (GroovizerParameterIDs::phrase, "Phrase", Groovizer::Engine::phraseNames(), 0);
    addChoice (GroovizerParameterIDs::feel, "Feel", Groovizer::Engine::feelNames(), 0);
    params.push_back (std::make_unique<juce::AudioParameterInt> (juce::ParameterID { GroovizerParameterIDs::lengthBars, 1 }, "Length Bars", 1, 8, 1));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { GroovizerParameterIDs::density, 1 }, "Density", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.55f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { GroovizerParameterIDs::swing, 1 }, "Swing", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.12f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { GroovizerParameterIDs::humanize, 1 }, "Humanize", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.12f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { GroovizerParameterIDs::fill, 1 }, "Fill", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.25f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { GroovizerParameterIDs::variation, 1 }, "Variation", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.42f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { GroovizerParameterIDs::ghosts, 1 }, "Ghost Notes", juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.28f));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { GroovizerParameterIDs::liveMode, 1 }, "Live Trigger", true));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { GroovizerParameterIDs::stepInput, 1 }, "Step Input", false));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { GroovizerParameterIDs::timelineEnabled, 1 }, "Timeline Playback", true));
    params.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { GroovizerParameterIDs::passInput, 1 }, "Pass Input", false));
    params.push_back (std::make_unique<juce::AudioParameterInt> (juce::ParameterID { GroovizerParameterIDs::outputChannel, 1 }, "Output Channel", 1, 16, 10));
    return { params.begin(), params.end() };
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GroovizerAudioProcessor();
}
