# Santismo MIDI FX

Voicizer is a Logic Pro AU MIDI FX that turns one incoming MIDI note into a voice-led chord or a tempo-synchronized chord performance.

Groovizer is a Logic Pro AU MIDI FX that turns GM drum input into generated groove phrases. It has a plugin-owned groove timeline, live trigger mode, step input to the timeline, MIDI capture, and drag-out MIDI files for Logic tracks.

The default behavior is melody-first: the input note is treated as the top note of a voice-led chord. Other roles are available for root, bass, guide-tone, inner-voice, random, and weighted auto behavior.

## Build

```sh
cmake -B build -S . -DJUCE_DIR=/Users/santiagotrejo/Desktop/JUCE
cmake --build build --config Release
```

The AUs are copied by JUCE after build:

- `~/Library/Audio/Plug-Ins/Components/Voicizer.component`
- `~/Library/Audio/Plug-Ins/Components/Groovizer.component`

If Logic does not show them immediately, rescan Audio Units or restart Logic.

## Harmony sources

- **Manual Harmony** uses the existing multi-key and multi-scale engine.
- **Follow Chordizer** reads Chordizer's shared chord regions and transport without modifying them. Match, diatonic, substitution, and adaptive relationship modes are available.

## Output modes

- **Held Voicing** sustains the generated voicing and supports rake direction and speed.
- **Performance** interprets the voicing at Logic's project tempo with Baroque, classical, jazz, walking, and bossa patterns. Density, sophistication, syncopation, swing, humanization, and gate are adjustable.

All generated notes currently output on the same MIDI channel as the incoming note.

## Groovizer

- **GM drum mapping**: generated drum output uses General MIDI drum note layout, with channel 10 as the default output channel.
- **Timeline**: add regions from the editor, click the timeline to place/select, use random arrange for idiomatic phrase blocks, and drag the whole timeline out as a `.mid` file.
- **Live mode**: incoming drum notes trigger generated grooves immediately.
- **Step input**: incoming drum notes write groove regions at Groovizer's internal cursor, then advance the cursor.
- **MIDI capture**: records generated output and lets the captured take be dragged into Logic.
