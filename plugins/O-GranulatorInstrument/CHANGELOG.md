# O-GranulatorInstrument Changelog

## [1.0.0] - 2026-11-12

The first release: a polyphonic granular instrument that plays four lanes of audio — your files, recordings from the sidechain, or the built-in sources — as up to four grain streams per voice.

### Added

- **Four lanes, four streams.** Drop an audio file on a lane, browse for one, or record into it. Each stream reads its own lane with its own position, scan, loop region, size, density, pitch, window, filter, drive, crush and ring modulator.
- **Record-in.** Record the sidechain into any lane with a Manual, Threshold or Next Note trigger, in seconds or in bars. A take plays at once, and Arm switches off when a take ends, so one click starts the next. Save Recording writes a take as a 24-bit WAV. With no sidechain routed, the status reads "Select a sidechain input".
- **Play it like an instrument.** Poly, Mono and Legato with glide; key tracking per stream with a root note; MPE and VST3 note expression; velocity, key, note random, mod wheel, pressure, slide and bend as modulation sources.
- **Modulation.** Four LFOs (global or per voice, free or tempo-synced), two modulation envelopes, four macros and a 16-slot matrix with 150 destinations, each stream's feedback and reverb among them. Drag a source onto any knob to route it.
- **Feedback and space.** Each stream has its own feedback, which rewrites its lane (Amount, Low Cut, High Cut, Saturate, Diffuse, and an on / off switch), and its own reverb (Mix, Size, Damp), placed before or after the feedback loop. Freeze and Reset All act on all four lanes.
- **Scales.** 32 pitch collections plus a custom scale (Scala or `edo N`), with per-stream degree masks for the pitch randomiser.
- **8 built-in sources** (harmonic, pluck, bell, vowel, noise bed, dust, drone, pulse), generated in-house, pitched at C3.
- **64 factory presets** in eight groups: Drones, Expressive, Keys, Live, Motion, Pads, Rhythmic, Textures. Presets load their lanes from the built-in sources; the Live templates keep your own lanes and set up Record-in.
- **Interface** in English, French and Simplified Chinese.

### Known issues

- MPE controllers and Dorico note-expression playback are proven in automated tests only; they have not been tried in a DAW yet.
- The Windows build is checked in CI, but the interface has not been tested by hand on Windows (file drop and browse included).
- Reaper has not been tested.
- Closing the plugin while a lane's take is still being written to an unresponsive disk can delay the close by up to 10 s.
- A recording save still writing when the host unloads the plugin's code entirely could crash; hosts on macOS do not normally unload plugin code.

### Known validator notes

- auval reports "Parameter did not retain default value" for the four LFO Rate parameters. This is a one-in-ten-million rounding difference in the normalised default (0.5 Hz stays 0.5 Hz); validation still passes, and nothing is audible.
