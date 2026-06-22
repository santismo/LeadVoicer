# Voicizer MIDI FX

Voicizer is a Logic Pro AU MIDI FX that turns one incoming MIDI note into a voice-led chord or a tempo-synchronized chord performance.

The default behavior is melody-first: the input note is treated as the top note of a voice-led chord. Other roles are available for root, bass, guide-tone, inner-voice, random, and weighted auto behavior.

## Build

```sh
cmake -B build -S . -DJUCE_DIR=/Users/santiagotrejo/Desktop/JUCE
cmake --build build --config Release
```

The AU is copied by JUCE after build. If Logic does not show it immediately, rescan Audio Units or restart Logic.

## Harmony sources

- **Manual Harmony** uses the existing multi-key and multi-scale engine.
- **Follow Chordizer** reads Chordizer's shared chord regions and transport without modifying them. Match, diatonic, substitution, and adaptive relationship modes are available.

## Output modes

- **Held Voicing** sustains the generated voicing and supports rake direction and speed.
- **Performance** interprets the voicing at Logic's project tempo with Baroque, classical, jazz, walking, and bossa patterns. Density, sophistication, syncopation, swing, humanization, and gate are adjustable.

Generated notes output on the incoming MIDI channel by default. Enable **Multi Ch** next to Chord Size to split each generated voicing across MIDI channels from highest to lowest, with channel 1 carrying the highest note.
