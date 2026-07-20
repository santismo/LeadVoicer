# Voicizer

Voicizer is a Logic Pro AU MIDI FX that turns simple note input into scale chords or phrase-aware, voice-led ensemble harmony. It uses the shared Logic-inspired Songizer interface and has no separate app or selectable themes.

The default **Simple Scale Chords** style treats every input as a chord trigger. In-scale notes select their diatonic seventh chord; non-scale notes never pass through as single notes and instead trigger the next functional chord suggested by the preceding harmony. **Close Lead** and **Big Band** treat the input as a melodic line and create smooth four-way-close or doubled-lead soli phrases.

The arranger remembers common tones, register, phrase position, and recent harmonic function. Chord Bank mode listens until every raked input note is released, stores reusable chord-quality formulas as weighted cards, and applies those formulas to new melody notes in Perform mode. The interface provides Harmonic Stability, Melody Importance, New Phrase, and practical range presets for piano, guitar, horns, orchestra, sax soli, brass, SATB, strings, piano hands, low reeds, and synth stacks.

## Installation

The Songizer installer places `Voicizer.component` in `~/Library/Audio/Plug-Ins/Components`. Fully quit and relaunch Logic Pro after replacing a build.

The release build is ad-hoc signed. If macOS blocks the component, move it to the folder above, run the following command in Terminal, and restart Logic Pro:

```sh
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/Components/Voicizer.component
```

## Build

```sh
cmake -B build -S . -DJUCE_DIR=/Users/santiagotrejo/Desktop/JUCE
cmake --build build --config Release
```

The AU build is produced as `Voicizer.component`.

If Logic does not show them immediately, rescan Audio Units or restart Logic.

All generated notes output on the same MIDI channel as the incoming trigger.
