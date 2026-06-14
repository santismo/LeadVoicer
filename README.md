# Lead Voicer MIDI FX

Lead Voicer is a Logic Pro AU MIDI FX prototype that turns one incoming MIDI note into a voice-led chord.

The default behavior is melody-first: the input note is treated as the top note of a voice-led chord. Other roles are available for root, bass, guide-tone, inner-voice, random, and weighted auto behavior.

## Build

```sh
cmake -B build -S . -DJUCE_DIR=/Users/santiagotrejo/Desktop/JUCE
cmake --build build --config Release
```

The AU is copied by JUCE after build. If Logic does not show it immediately, rescan Audio Units or restart Logic.

## V1 Controls

- Multi-select key and scale source
- Melody/input-note role
- Style and playability target
- Chord size, complexity, voice-leading, outside harmony, variation, and repeat chance
- Strum/rake direction and speed
- Min/max note range
- Randomize and reset buttons

All generated notes currently output on the same MIDI channel as the incoming note.
