# O-IntonationPad Changelog

## [1.4.0] - 2026-02-18

### Fixed
- **Tuning panel layout**: Fixed broken 4-item CSS grid (intervals, viz-toggle, viz-container, controls as separate grid items) to proper 3-column structure matching O-Bells v2.0.0 (`tuning-intervals-column` | `tuning-center-column` wrapping viz-toggle + viz-container | `tuning-controls-panel`)
- **CSS grid columns**: Updated from `140px 1fr 200px` to `160px 1fr 220px`
- **Visualization container**: Changed from visible bordered box to transparent seamless container

### Added
- **Larger pitch circle**: SVG upgraded from 188x188 to 320x320 viewBox with proportionally larger spokes and dots
- **Note highlighting on pitch circle**: Active MIDI notes now highlight their corresponding scale degree spokes in red with thicker strokes (via `noteOn`/`noteOff` + `updateSpokeHighlights()`)
- **Note name labels**: `getNoteLabel()` shows chromatic note names (C, C#, D...) for 12-note scales instead of raw degree numbers; degree numbers used for non-12 scales
- **TrueKeys with actual frequencies**: Upgraded from simple MIDI-note-difference calculation to real frequency-based interval reporting using `updateHeldNotes(midi[], freq[])` bridge — shows note names (e.g. "C4 -> E4 (M3)"), actual cent values, and interval identification (m2, M2, m3, M3, P4, TT, P5, m6, M6, m7, M7, P8)
- **Total span display**: TrueKeys shows total cent span when 3+ notes are held
- **MIDI-to-note-name helper**: `midiToNoteName()` converts MIDI numbers to readable names (e.g. 60 -> "C4")
- **Interval identification**: `identifyInterval()` maps cent values to common interval names with 15c tolerance

### Changed
- `activeScaleDegrees` Set tracks which scale degrees are currently sounding for spoke highlighting
- `spokeElements` array stored for fast in-place color updates without full SVG redraw
- Polar canvas enlarged from 180x180 to 300x300
- Interval degree column widened from 20px to 24px with `flex-shrink: 0`
- TrueKeys view uses `.tk-grid` layout with accent border-left styling

## [1.3.0] - 2026-02-17

### Added
- **Full tuning module integration**: Replaced 5-button tuning selector with the `scala-tuning-engine` module v2.1.0 — matching O-Bells and O-Lyrica tuning functionality
- **5 visualization modes**: Pitch circle, linear keyboard, interval ruler, harmonic series, and lattice views
- **Embedded tuning library**: 24+ built-in tuning presets across Historical, World, Experimental, and Mathematical categories
- **Scale generator**: Create EDO, harmonic series, and rank-2 temperaments directly in the UI
- **Scala file I/O**: Import/export `.scl` and `.kbm` files for interoperability with other microtonal software
- **Editable intervals**: Click any interval in the table to fine-tune individual scale degrees
- **Master tune control**: A4 reference tuning (400-480 Hz, default 440 Hz)
- **Octave stretch**: Adjustable octave ratio (1190-1210 cents, default 1200)
- **Pitch bend range**: Configurable pitch bend (1-48 semitones)
- **HTML export**: Export tuning documentation as a standalone HTML page
- **Tuning panel note highlighting**: Active MIDI notes forwarded to tuning panel via `noteOn`/`noteOff` bridge

### Changed
- Replaced old `TuningSystem` class (5 presets) with `TuningEngine` + `ScaleGenerator` + `TuningExporter` + `EmbeddedTunings`
- APVTS parameters: removed `tuningSystem` choice, added `tuning_masterTune`, `tuning_octaveStretch`, `tuning_pitchBendRange`, `tuning_temperamentPreset`
- 24 native functions registered for C++ ↔ WebView communication (matching O-Bells pattern)
- State persistence now saves custom intervals, scale name, tonic, and active preset in ValueTree

### Breaking
- Saved DAW sessions with old `tuningSystem` parameter will reset to default tuning (12-TET)

## [1.2.0] - 2026-02-17

### Added
- **Real-time complexity fading on held notes**: Changing the complexity knob smoothly fades chord extensions (7th, 9th, 11th, 13th) in/out on already-sounding notes with ~250ms crossfades
- **Real-time voice count fading on held notes**: Changing the voice count knob smoothly fades additional voices in/out on held notes (same 250ms crossfade)
- **Real-time inversion crossfade on held notes**: Changing the inversion knob crossfades each voice between its base pitch and a randomly-chosen octave-shifted pitch (±1 octave, 250ms crossfade). Each voice has a random threshold so inversions engage progressively as the knob increases
- **Per-voice complexity threshold system**: Triad voices (root, 3rd, 5th) always at full volume; extensions fade based on their complexity tier
- **Gain-aware UI**: Keyboard, frequency list, and pitch circle all reflect per-voice gain with opacity fading — during inversion crossfades, both the base and inverted notes appear simultaneously with their respective gains

### Changed
- Each sub-voice now has dual oscillators (base + inverted pitch) initialized at note-on; all 12 sub-voices always active
- Voice count, complexity, and inversion are all applied as three independent real-time smoothed gain multipliers
- Voice distribution assigns intervals sequentially (root, 3rd, 5th, 7th, 9th...) instead of skipping intervals

## [1.1.0] - 2026-02-17

### Added
- **Live note visualization on Voice tab**: Mini piano keyboard (C2-B5, 4 octaves) highlights all active chord tones in real-time as you play
- **Frequency/cent detail list**: Shows each sounding voice's note name, exact frequency (Hz), and cent deviation from 12-TET — making tuning differences visible
- **Pitch circle active note highlighting**: Tuning tab's pitch circle now lights up active pitch classes when notes are playing (uses existing but previously unwired API)
- **C++ to WebView event bridge**: New `emitEventIfBrowserIsVisible("activeNotes")` timer at 30Hz pushes chord data from audio engine to UI

### Technical
- Added `SubVoiceInfo` struct to `WavetableVoice` for storing MIDI note + frequency per sub-oscillator
- Added `getActiveNotes()` API to `PluginProcessor` for collecting active voice data across all synthesiser voices
- `PluginEditor` now inherits from `juce::Timer` for periodic UI updates

## [1.0.0] - 2026-02-17

### Initial Release
- Wavetable pad synthesizer with JI-harmonic wavetable (256 frames)
- 1-note chord mode: single MIDI note generates 2-12 voice chords
- 5 tuning systems: 12-TET, Just Intonation, Pythagorean, Historical, Scala
- 10 scale modes with scale-degree chord quality analysis
- 3 randomization axes: inversion, timing, detune
- Global LFO modulating wavetable position
- ADSR envelope, state-variable lowpass filter
- 4-tab WebView UI with Ouaricon Naturalist aesthetic
- Interactive pitch circle visualization on Tuning tab
