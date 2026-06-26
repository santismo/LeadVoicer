# FretStep2 SoundFonts

This folder contains SoundFonts used by the FretStep2 web app.

- `Roland.SC-55.sf2` is copied from the user's `santismo/fakebot` GitHub hosting location for use in this repository.
- `GeneralUser-GS.sf2` is from the GeneralUser GS project by S. Christian Collins. See the upstream project and license: https://github.com/mrbumpy409/GeneralUser-GS
- `909_drum_sf.sf2` is from `bratpeki/soundfonts`, which documents it as the "909 Drum Soundfont" under Creative Commons Attribution 3.0: https://github.com/bratpeki/soundfonts

Additional GM/retro banks in `index.html` are loaded from `bratpeki/soundfonts` as external GitHub raw URLs. That repo documents the included files as GitHub-size-safe and lists their licenses, including CC0 banks, GPLv2 TimGM, and public-domain eawpats. The app also exposes GM drum-kit presets from several of those banks as separate drum selectors without duplicating the downloaded file.

The TR-808 drum font is loaded as an external URL from `vigliensoni/soundfonts`.

The 8-bit console SoundFont is loaded as an external URL from `Miserlou/RJModules`, an MIT-licensed repository.
