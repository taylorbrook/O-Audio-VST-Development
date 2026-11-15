# O-GranulatorInstrument Changelog

## [1.3.1] - 2026-11-14

### Changed

- **All 20 Drones now follow the keyboard (Key Track 100 % on every stream).** They tracked only 0–50 %, so most keys played nearly the same pitch. Every Keys and Pads preset already tracked fully; that is now enforced by a test across all three categories.
- **Drones · Bassoon Pedal** was a fixed C1 pedal. It now sits two octaves under the played note, with its reed line an octave under; played at C3 it sounds as before. Its description and Wind Tunnel's are updated to match.

### Notes

- Only the 20 Drones presets change; no parameter, range or session-format change. Sessions that use a Drones preset keep their saved values.
- The factory preset files are rewritten once on first launch of 1.3.1; your own presets are not touched.

## [1.3.0] - 2026-11-14

### Added

- **96 new factory presets, 160 in all.** Drones, Expressive, Keys, Motion, Pads, Rhythmic and Textures each gain 12 and now hold 20; Live keeps its 8 templates. Every one of the 41 built-in sources is now played by at least two presets.
  - Keys: Marimba, Kalimba Hall, FM Tines, Ice Bells, Crystal Bowl, Chapel Organ, Gamelan Plate, Drift Guitar, Guitar Harmonics, Sus Chords, Poly Saw, Bowed Stroke.
  - Pads: Cathedral Choir, Low Strings, Organ Swell, Minor Haze, Sus Horizon, Analog Sweep, Shimmer Veil, Reed Consort, Crystal Halo, Breath Cloud, Overtone Glow, Deep Floor.
  - Drones: Sub Pressure, Bassoon Pedal, Organ Fifths, Endless Bow, Saw Monolith, Shimmer Rise, Wind Tunnel, Minor Shroud, Plate Gong, Long Exhale, Hearth Glow, Undertow.
  - Textures: Gravel Rain, Bubble Drift, Iron Swarm, Scrap Yard, Paper Cuts, Twig Storm, Geiger Dust, Cicada Night, Breath Static, Ice Shards, Crystal Rain, Gale Shimmer.
  - Rhythmic: Shuffle Loop, Triplet Chop, Click Hats, Stick Groove, Snip Claps, Iron Shaker, Marimba Pulse, Kalimba Steps, Bubble Seq, Sub Pump, Bell Dots, Minor Chop.
  - Motion: Saw Sweeper, Sus Carousel, Shimmer Spiral, Drift Warble, Reed Pulsar, Bubble Orbit, Plate Spin, Crystal Phase, Vowel Wander, Tide Machine, Garden Loops, Bell Scatter.
  - Expressive: Reed Press, Bow Press, Slide Vox, MPE Shimmer (MPE); Saw Glide, Sub Legato, Deep Slur, Drift Slide (mono with glide); Wheel Organ, Velo Glass, Key Breath, Wheel Gust (velocity, key and mod wheel).
- **A new category, Field (12 presets),** built on the five field recordings: Brook Bank, Rain on Stream, Stream Bed, Park Morning, Distant Park, Park Memory, Shoreline, Sea Cave, Forest Walk, Campfire, Garden Rain, Garden Dusk.
- **A two-pane preset menu.** The groups on the left (Drones, Expressive, Field, Keys, Live, Motion, Pads, Rhythmic, Textures, then User), the presets of the selected group on the right, by short name; the preset bar keeps the full name. Hovering a group shows its presets. Up and Down move within a pane, Left and Right between panes, Home and End jump, Enter loads, Escape or Tab closes. "Load from file…" sits at the foot of the group list. Opening the menu selects the current preset. Group names are in English, French and Chinese.
- **Your own presets have their own group, User,** listed last. A user preset never lands in a factory group, even when its name starts like one.

### Changed

- **Eight factory presets re-voiced on a better-fitting source:** Keys · Kalimba (now the Kalimba source), Keys · Glass Mallet (Glass, with a lower sustain so a held note does not re-strike), Drones · Organ Loft (Organ), Drones · Hollow Wind (Wind), Pads · Warm Choir (Choir), Pads · Velvet Strings (Bowed, and the two streams no longer detuned against each other), Textures · Ember Field (Fire), Textures · Rain Glass (Rain for the first stream). Their levels were matched to 1.2.0 within 1.4 dB.
- Descriptions corrected where they did not match the sound: Glass Mallet, Orbit Lines, Crush Tides, Rust Cloud, Gated Pad.
- The other 56 factory presets from 1.0.0 are unchanged; they render bit for bit as in 1.2.0.

### Notes

- No parameter, range or session-format change. Sessions and presets from 1.0.0, 1.1.0 and 1.2.0 load unchanged.
- The factory preset files are rewritten once on first launch of 1.3.0 (the bank changed); your own presets are not touched.
- A preset that uses an off-C3 source (Crystal, Guitar Overtones, Guitar Drift, Contrabassoon, Sub) sets that stream's Root Note and Root Fine itself, so it plays in tune.
- The bank's loudness spread is 6.0 dB (gated), down from 7.0 dB in 1.2.0; the new presets sit within 2 dB of each other.
- Preset names are not translated.

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
