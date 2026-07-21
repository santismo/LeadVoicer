# Voicizer

Voicizer is a Logic Pro AU MIDI FX that turns simple note input into scale chords or phrase-aware, voice-led ensemble harmony. It uses the shared Logic-inspired Songizer interface and has no separate app or selectable themes.

The bundled **happy bday** setup is now the canonical fresh-instance preset and the target of the Reset button. Its complete voicing, range, phrase, performance, and chord-bank settings are built into Voicizer rather than depending on a user preset file. **Simple Scale Chords** treats every input as a chord trigger. In-scale notes select their diatonic seventh chord; chromatic notes never pass through as single notes and, with Modulation at zero, create useful passing harmony such as Cmaj7–C#dim7–Dm7. Raising Modulation lets chromatic and in-scale roots borrow context-fitting minor-7, major-7, dominant-7, half-diminished, or diminished colors. **Close Lead** and **Big Band** treat the input as a melodic line and create smooth four-way-close or doubled-lead soli phrases.

The arranger remembers common tones, register, phrase position, and recent harmonic function. Phrase Memory remains independently toggleable. Multiple scales selected under one key form a normal modal-interchange palette: shared notes can move between those modes and notes unique to a selected mode are interpreted inside that mode even with Outside and Modulation at zero. Color, Lead, Outside, Stability, and Melody now bypass naturally at zero instead of using separate switches. Outside and Modulation evaluate the actual selected tonalities, so high values can deliberately borrow chord roots and tones even when only one key and one scale are selected. Repeat controls automatic consistency per exact input note and register: zero favors a fresh valid alternative, while 100% recalls the assigned chord until New Phrase without disabling the selected modal palette for other triggers. Exact chord locks remain available for deliberate permanent choices. Chord Bank mode listens until every raked input note is released, stores reusable chord-quality formulas as weighted cards, and applies those formulas to new melody notes in Perform mode.

The advanced editor groups Voices, Low, High, and Rake under **Voice & Range**; Color, Lead, Outside, and Repeat under **Color & Motion**; and Phrase Memory, New Phrase, Modulation, Stability, and Melody under **Phrase Logic**. Piano and Piano Hands allow the Low and High controls to span the full A0-to-C8 acoustic-piano range. Simple Scale mode keeps its compact Voice & Range section and exposes Modulation in Phrase Logic. Exact input-note/register locks appear as removable cards at the bottom. The centered chord readout latches the last observed chord and recognizes detailed extensions and inversions from the actual generated notes.

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
