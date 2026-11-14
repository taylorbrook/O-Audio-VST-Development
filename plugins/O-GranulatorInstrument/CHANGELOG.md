# O-GranulatorInstrument Changelog

## [1.2.0] - 2026-11-13

### Added

- **33 more built-in sources, 41 in all.** 14 are recordings: Guitar Overtones, Guitar Drift, Crystal, Contrabassoon, Fire, Bubbles, Metal Grate, Stick Snaps, Scissors, and five field recordings (Stream, Park, Waves, Woods, Garden). 19 are new generated sources: Bowed, Reed, Organ, Saw Stack, Chord Minor, Chord Sus, Choir, FM Bell, Glass, Kalimba, Marimba Bar, Metal Plate, Wind, Rain, Breath, Shimmer, Sub, Clicks and Pulse Triplet.
- **The Factory list is grouped.** Seven groups in four columns: Tonal, Voice, Plucked / Struck, Texture, Drone, Rhythmic, Field. Arrow keys move through it (Left and Right change column), typing a name jumps to it, Enter picks, Escape closes.
- **A picked source plays in tune.** Picking a source from the Factory list sets that stream's Root Note and Root Fine to the source's own pitch, so the key you play is the pitch you hear. Most sources sit at C3. Crystal is Db5 +17 cents, Guitar Overtones (one harmonic note) is C5 −32 cents, Guitar Drift is D2 −10 cents, Contrabassoon is G#0 −19 cents and Sub is C1.
- **The long textures loop without a gap.** Stream, Park, Waves, Woods, Garden, Fire and Bubbles are cut so that their end runs into their start.

### Changed

- The eight sources from 1.0.0 are unchanged, and so are the 64 factory presets.

### Notes

- Picking a source changes Root Note and Root Fine only on the lane you picked it for. Loading a preset or a session, dropping a file, Browse, Record and Clear do not touch the root.
- No parameter or session-format change. Sessions and presets from 1.0.0 and 1.1.0 load unchanged.
- A session or preset saved in 1.2.0 that uses one of the 33 new sources opens in 1.1.0 or 1.0.0 with that lane shown as missing. The rest of the session loads.
- The plugin is about 16 MB larger.

## [1.1.0] - 2026-11-13

### Added

- **Pick a built-in source from an empty lane.** An empty lane now reads "Drop a file or Record or Factory". Factory opens a list of the 8 built-in sources (Harmonic, Pluck, Bell, Vowel, Noise Bed, Dust, Drone, Pulse); pick one and it loads into that lane. To change a lane that already holds audio, clear it first. Until now the built-in sources could only arrive with a preset.

### Notes

- No parameter or session-format change: a lane loaded this way is saved exactly like one a factory preset loaded, so sessions and presets from 1.0.0 load unchanged, and a 1.1.0 session opens in 1.0.0.
- A built-in source cannot be picked for a lane while a recording is running into it.

## [1.0.0] - 2026-11-13

The first release: a polyphonic granular instrument that plays four lanes of audio — your files, recordings from the sidechain, or the built-in sources — as up to four grain streams per voice.

### Added

- **Four lanes, four streams.** Drop an audio file on a lane, browse for one, or record into it. Each stream reads its own lane with its own position, scan, loop region, size, density, pitch, window, filter, drive, crush and ring modulator.
- **Record-in.** Record the sidechain into any lane with a Manual, Threshold or Next Note trigger, in seconds or in bars. A take plays at once, and Arm switches off when a take ends, so one click starts the next. Save Recording writes a take as a 24-bit WAV. With no sidechain routed, the status reads "Select a sidechain input".
- **Play it like an instrument.** Poly, Mono and Legato with glide; key tracking per stream with a root note; MPE and VST3 note expression; velocity, key, note random, mod wheel, pressure, slide and bend as modulation sources.
- **Modulation.** Four LFOs (global or per voice, free or tempo-synced), two modulation envelopes, four macros and a 16-slot matrix with 150 destinations, each stream's feedback and reverb among them. Drag a source onto any knob to route it.
- **Feedback and space.** Each stream has its own feedback, which rewrites its lane (Amount, Low Cut, High Cut, Saturate, Diffuse, and an on / off switch), and its own reverb (Mix, Size, Damp), placed before or after the feedback loop. A lane keeps its level while its stream's feedback rewrites it: it changes character lap after lap, but it does not fade. With all keys released the lane is left as it is. Freeze and Reset All act on all four lanes.
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
