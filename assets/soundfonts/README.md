# FretStep2 SoundFonts

This folder contains older SoundFont assets that are kept in the repository for reference and compatibility experiments. The current FretStep2 web app uses internal Web Audio engines instead of loading external SoundFonts.

- `Roland.SC-55.sf2` is copied from the user's `santismo/fakebot` GitHub hosting location for use in this repository.
- `GeneralUser-GS.sf2` is from the GeneralUser GS project by S. Christian Collins. See the upstream project and license: https://github.com/mrbumpy409/GeneralUser-GS
- `909_drum_sf.sf2` is from `bratpeki/soundfonts`, which documents it as the "909 Drum Soundfont" under Creative Commons Attribution 3.0: https://github.com/bratpeki/soundfonts

The selectable sound banks in `index.html` are generated directly by FretStep2 with Web Audio. External SoundFont banks are no longer exposed in the sound menu, which keeps labels, FX routing, audition behavior, and bank switching deterministic.
