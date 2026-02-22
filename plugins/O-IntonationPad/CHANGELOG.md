# O-IntonationPad Changelog

## [1.10.0] - 2026-02-21

### Added
- **Independent LFO rate per oscillator**: Each wavetable now has its own LFO rate knob (Rate A and Rate B), allowing different modulation speeds on each oscillator. The two LFOs run as independent free-running sine waves
- **New APVTS parameter**: `lfoRate2` (0.01–20 Hz, default 0.5 Hz) for OSC B's LFO rate

### Changed
- **Synth tab layout tightened**: Reduced gaps, margins, and knob sizes to ensure all four rows (OSC A, OSC B, envelope/filter, timing/detune) are fully visible without clipping
- **LFO Rate moved into oscillator rows**: Rate knob now lives alongside Pos, Depth, and Gain in each oscillator's row instead of the shared envelope row

## [1.9.0] - 2026-02-21

### Changed
- **Independent gain knobs replace mix crossfade**: Removed the single Mix knob (A/B crossfade) and replaced it with two independent Gain knobs — one for each oscillator. Both can now play at full volume simultaneously for layered sounds, or be individually controlled
- **Two-row oscillator layout**: Synth tab now shows OSC A and OSC B on separate rows, each with its own wavetable selector, position knob, LFO depth knob, and gain knob
- **New APVTS parameters**: `gainA` (default 100%) and `gainB` (default 0%) replace the old `oscMix` parameter. Saved presets using `oscMix` will need to be re-saved

## [1.8.1] - 2026-02-20

### Fixed
- **Mix, LFO B, and Position B knobs unresponsive**: The v1.8.0 dual oscillator UI controls (oscMix, wavetablePos2, lfoDepth2, wavetableBank2) had APVTS parameters and HTML elements but were missing WebSliderRelay/WebComboBoxRelay bindings in the C++ editor. `Juce.getSliderState()` returned null for these parameters, causing `setupKnob` to bail out without attaching drag handlers. Added relays, `withOptionsFrom` registrations, and parameter attachments for all four missing parameters.

## [1.8.0] - 2026-02-20

### Added
- **Dual wavetable oscillator**: Added a second independent wavetable oscillator (OSC B) with its own bank selection, morphing position, and LFO depth
- **Oscillator mix control**: New Mix knob crossfades between OSC A and OSC B (0% = pure A, 100% = pure B) with smooth per-sample interpolation
- **Independent LFO depths**: Each oscillator has its own LFO depth control while sharing the global LFO rate — allows static texture on one oscillator with modulated morphing on the other
- **5 new APVTS parameters**: `wavetableBank2`, `wavetablePos2`, `oscMix`, `lfoDepth2` — all automatable and saved with DAW sessions
- **Reorganized Synth tab**: OSC A controls on the left, Mix knob in the center, OSC B controls on the right

### Technical
- WavetableVoice now holds 72 oscillators per voice (36 for OSC A + 36 for OSC B), all sharing the same chord voicing frequencies
- Linear crossfade: `output = oscA * (1 - mix) + oscB * mix`, smoothed with the same ~250ms exponential coefficient used for gain fading

## [1.7.0] - 2026-02-20

### Added
- **Standard waveforms**: Added Sine, Square, and Triangle to the wavetable bank selector (Synth tab), bringing the total from 9 to 12 banks
- **Sine bank**: Pure fundamental at all wavetable positions — clean, uncolored tone
- **Square bank**: Band-limited square wave (odd harmonics, 1/n amplitude) with morphing from sine (position 0%) to full square (position 100%)
- **Triangle bank**: Band-limited triangle wave (odd harmonics, 1/n² amplitude, alternating phase) with morphing from sine to full triangle
- **Phase offset support**: Wavetable partial system now supports per-partial phase offsets, enabling proper alternating-sign waveforms like triangle

## [1.5.1] - 2026-02-19

### Fixed
- **Tuning tab not scrollable**: When the "Generate Scale" dropdown was expanded, controls below it were unreachable. Added vertical scrolling to the tuning tab with a themed scrollbar
- **Intervals table fixed height**: The interval list had a hardcoded `max-height: 300px` with internal scroll, regardless of scale size. Removed the constraint so the table auto-extends to fit all scale degrees (7-note, 12-note, 31-note, etc.), with tab-level scrolling handling overflow
- **Center visualization lacked visual separation from right panel**: The center visualization area (Circle, Polar, Matrix, TrueKeys, Rotation) now has its own distinct panel styling (background, border, border-radius) to visually separate it from the tuning library controls on the right

## [1.5.0] - 2026-02-19

### Added
- **Interactive interval selector on Voice tab**: Replaced the static Scale dropdown (10 preset scales) with a dynamic toggle list sourced from the tuning engine. Each interval from the currently loaded tuning appears as a clickable toggle — enable or disable individual intervals to control which scale degrees participate in chord generation
- **Works with any scale size**: The interval selector dynamically expands and contracts for 12-TET, 19-EDO, 31-EDO, Just Intonation, or any custom Scala tuning — not limited to 12-note scales
- **All/None quick-select buttons**: Instantly enable all intervals or disable all except root
- **Auto-reset on scale change**: When the tuning is changed on the Tuning tab (different interval count), the enabled intervals automatically reset to all-enabled
- **Interval labels**: Shows semitone names (m2, M2, m3, M3, P4, TT, P5, m6, M6, m7, M7) for 12-note scales, degree numbers for other scale sizes, with cents values displayed on each toggle
- **State persistence**: Enabled/disabled interval state saved and restored with DAW sessions

### Changed
- **ChordGenerator refactored**: Now accepts a dynamic set of enabled scale degree offsets instead of a hardcoded scale type index. Chord building uses enabled intervals directly with progressive complexity thresholds
- **Audio-thread cache**: Enabled degree offsets cached with atomic dirty flag for lock-free audio thread reads

### Removed
- `keyScale` APVTS parameter (10 preset scales: Major, Minor, Dorian, etc.) — replaced by the interval selector
- All 10 hardcoded scale arrays from ChordGenerator (majorScale, minorScale, dorianScale, etc.)
- Scale-degree chord quality lookup system (Major/Minor/Diminished)

### Breaking
- Saved DAW sessions with old `keyScale` parameter will have that value silently dropped. Default behavior (all intervals enabled) provides full chromatic chord voicing similar to the old Major scale with high complexity

## [1.4.1] - 2026-02-18

### Fixed
- **Chord voices always played in 12-TET regardless of temperament selection**: Regression introduced in v1.3.0 when TuningEngine replaced TuningSystem. Root cause: `setBuiltInPreset()` called `setCustomIntervals()` (which rebuilds the frequency table) while `currentMode` was still `TwelveTET` — the mode was set to `Scala` only AFTER the rebuild, so the table always got 12-TET values. Fix: set mode BEFORE `setCustomIntervals()` so the rebuild uses the correct mode.
- **Temperament mode override during state restoration**: `tuning_tuningMode` parameter (default: 12-TET) could override the mode set by a non-12-TET temperament preset during APVTS state restore. Fix: guard `tuning_tuningMode` handler so it only applies when preset is Equal12TET or Custom.
- **Restored original pitch-class tuning math for 12-note scales**: TuningEngine's linear mapping produced standard JI intervals, but the original TuningSystem used a pitch-class-based formula (12-TET base frequency * centsToRatio) that produced the plugin's characteristic wider-than-standard intervals. Fix: `calculateCustomFrequency()` now uses the original pitch-class approach for 12-note scales, preserving the intended pad character.
- **Restored Just Intonation as default temperament**: `tuning_temperamentPreset` default changed from Equal 12-TET (index 0) to Just Intonation (index 8), matching the pre-v1.3.0 `tuningSystem` parameter default. TuningEngine now explicitly initialized to JI in constructor since JUCE doesn't fire `parameterChanged` for initial values.

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
