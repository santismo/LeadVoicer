# Voicizer

Voicizer is a Logic Pro AU MIDI FX that turns simple note input into scale chords or phrase-aware, voice-led ensemble harmony. It uses the shared Logic-inspired Songizer interface and has no separate app or selectable themes.

The default **Simple Scale Chords** style treats every input as a chord trigger. In-scale notes select their diatonic seventh chord; chromatic notes never pass through as single notes and, with Modulation at zero, create useful passing harmony such as Cmaj7–C#dim7–Dm7. Raising Modulation lets those chromatic roots borrow context-fitting minor-7, major-7, dominant-7, or half-diminished colors. **Close Lead** and **Big Band** treat the input as a melodic line and create smooth four-way-close or doubled-lead soli phrases.

The arranger remembers common tones, register, phrase position, and recent harmonic function. Phrase Memory can be bypassed for independent scale/style choices, while per-logic toggles bypass Color, Lead, Outside, Stability, or Melody behavior without losing slider values. Outside evaluates harmony against the actually selected keys and scales, so a high enabled value can deliberately borrow chords even when only one key and one scale are active. Repeat assigns and recalls a chord per exact input note instead of reusing one chord for unrelated notes. Chord Bank mode listens until every raked input note is released, stores reusable chord-quality formulas as weighted cards, and applies those formulas to new melody notes in Perform mode.

The advanced editor groups Voices, Low, High, and Rake under **Voice & Range**; Color, Lead, Outside, Variation, and Repeat under **Color & Motion**; and Phrase Memory, New Phrase, Modulation, Stability, and Melody under **Phrase Logic**. Simple Scale mode keeps its compact Voice & Range section and exposes Modulation in Phrase Logic. Exact input-note/register locks appear as removable cards at the bottom. The centered chord readout latches the last observed chord until a new one replaces it.

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
