# Ouaricon Marimba Changelog

All notable changes to this project will be documented in this file.

## [1.6.0] - 2026-01-11

### Added
- **Three new timbre refinement controls** in Sound panel for deeper sound shaping:
  - **Strike Position** (0-100%, default 50%) - Simulates mallet strike location on the bar
    - Center strikes emphasize fundamental and double-octave (modes 0 & 1)
    - Edge strikes bring out higher partials (modes 2-7)
    - Based on physical nodal point modeling
  - **Overtone Damping** (0-100%, default 50%) - Controls upper harmonic decay rate
    - Low (Shimmer): All modes sustain similarly for bell-like overtones
    - High (Focused): Upper partials decay quickly for tight, woody tone
    - Adjusts damping factor from 0.1 to 0.5 per mode index
  - **Tone** (0-100%, default 75%) - Post-synthesis brightness control
    - One-pole lowpass filter on final output (2kHz–20kHz cutoff)
    - Shapes sustained sound without affecting attack character
    - "Warm" label at low values, "Bright" at high values

### Technical Notes
- New parameters: STRIKE_POSITION, OVERTONE_DAMPING, TONE (all float 0-1)
- Strike position uses mode-specific amplitude multipliers based on nodal patterns
- Overtone damping modifies getDecayTime() modeFactor (0.1–0.5 range)
- Tone filter: toneFilterCoeff = ω/(ω+1) where ω = 2π·fc/fs
- UI: 3 new small knobs added below existing Sound panel row
- All parameters integrated with APVTS, preset system, and DAW automation

## [1.5.0] - 2026-01-11

### Changed
- **Improved physical model realism** - Major overhaul of modal synthesis for more authentic marimba sound
  - **Corrected modal frequency ratios** from acoustic research measurements (Euphonics/ISMA2019)
    - Mode 2 now tuned to exactly 4.0x fundamental (double octave) - the signature of professional marimbas
    - Higher modes corrected: was [26.3, 38.2, 52.4, 68.9], now [24.22, 33.54, 42.97, 54.0]
    - Root cause: Original ratios were approximations; higher modes were 8-22% off measured values
  - **Improved mode amplitude distribution** based on spectral analysis
    - Strong fundamental + strong mode 2 (double octave) for characteristic marimba timbre
    - Faster exponential rolloff for higher modes (more natural overtone balance)
    - Material parameter now only affects modes 3+ (preserves marimba character at all settings)
  - **Enhanced body resonance** with wood-like characteristics
    - Extended IR from 75ms to 100ms for richer sustain
    - 6 resonant modes (was 3) across 180-1100 Hz range
    - Multi-stage decay envelope: quick attack, fast initial decay, slow tail
    - Individual decay rates per mode (higher frequencies decay faster)
    - Subtle early reflections for wood diffusion character

### Technical Notes
- Research sources: Euphonics.org marimba acoustics, ISMA2019 modal measurement studies
- Modal ratios now match measured professional marimba bar spectra
- Body IR now simulates resonator tube coupling more accurately

## [1.4.0] - 2026-01-11

### Added
- **Export tuning files** - Save current tuning as Scala (.scl) and keyboard mapping (.kbm) files
  - SAVE .SCL and SAVE .KBM buttons in Custom tuning mode
  - Remembers last-used export directory for convenience
  - Standard Scala format compatible with other microtonal software

### Changed
- Renamed "SCALA" button to "CUSTOM" for clarity (Custom mode allows editing intervals)
- Interval table is now **non-editable in 12-TET mode**
  - Root cause: Table was editable but changes had no effect (12-TET ignores custom intervals)
  - All inputs disabled when 12-TET is selected, editable only in Custom mode
  - Prevents user confusion about why edits don't affect tuning

## [1.3.1] - 2026-01-10

### Added
- **LOAD button** - Opens file dialog to load preset files directly
- **Preset dropdown menu** - Click preset name to show dropdown with all presets
  - Separate sections for Factory and User presets
  - Currently active preset highlighted
  - Click to instantly load any preset

### Changed
- Preset name display now shows dropdown indicator (▼)
- Improved preset browser UX with direct selection

## [1.3.0] - 2026-01-10

### Added
- **Preset System** - Save and load complete patch states including tuning
  - Factory presets: Default Marimba, Bright Marimba, Soft Marimba, Just Intonation, Pythagorean, Quarter-Comma Meantone, Baroque A=415, Concert A=442
  - User presets saved to `~/Library/Application Support/Ouaricon Marimba/Presets/User/`
  - JSON format for easy editing and sharing
  - Preset browser in header: ◀ ▶ navigation, SAVE button with file dialog
  - Each preset stores: all 7 APVTS parameters, tuning intervals (any scale size), scale name, tonic note
- **DAW Session State** now includes tuning configuration
  - Custom tuning intervals persist when saving/reloading DAW projects
  - Tonic note (transposition) is preserved
  - Scale name is restored
- New C++ PresetManager class for save/load/list operations
- Native functions for WebView: savePreset, loadPreset, getPresetList, getCurrentPreset, selectNextPreset, selectPreviousPreset, deletePreset

### Changed
- getStateInformation/setStateInformation now serialize complete tuning state (not just APVTS parameters)
- Preset name display in header updates dynamically when navigating or loading presets

## [1.2.6] - 2026-01-10

### Fixed
- Circular scale interval indicators now highlight polyphonically
  - Root cause: Only one note was tracked at a time (single atomic variable)
  - Added lock-free MIDI event queue to track all note-on AND note-off events
  - All held notes now highlight red simultaneously when playing chords
  - Velocity-based intensity: harder hits show brighter red (rgb(220,0,0)), softer hits show darker red (rgb(120,40,40))
  - Proper note-off handling ensures highlights clear when keys are released
  - Octave stacking support: multiple notes on same scale degree correctly tracked

## [1.2.5] - 2026-01-10

### Added
- Live VU meter in Sound tab now responds to audio output
  - Peak level measurement after all processing (synth + body resonance + output gain)
  - Ballistic needle motion: fast attack (0.5), slow decay (0.08)
  - Scale: -60dB to +3dB with full semicircle sweep (-90° to +90°)
  - Dynamic needle color: green (quiet) → red (loud) gradient
  - 30 FPS update rate via C++ timer and WebView events

## [1.2.4] - 2026-01-10

### Changed
- Increased dynamic range of velocity response (+6dB at max velocity)
  - Low velocity (1) remains unchanged
  - High velocity (127) now +6dB louder than before
  - Smooth linear scaling in dB between extremes
  - Independent of VEL_CURVE parameter (applied on top of curve shaping)
  - Makes the instrument more dynamically expressive

## [1.2.3] - 2026-01-10

### Fixed
- Waveform display in Sound tab now functions as a live oscilloscope
  - Root cause: Display was static (flat line), no audio data was being sent to WebView
  - Added lock-free WaveformFifo in PluginProcessor to capture audio samples
  - Added getWaveformData native function to provide 128-point downsampled waveform
  - WebView polls at 60fps using requestAnimationFrame for smooth display

### Changed
- Renamed "MALLET" knob label to "MALLET HARDNESS" for clarity
- Renamed "MATERIAL" knob label to "MATERIAL HARDNESS" for clarity
- Adjusted knob positions slightly to accommodate longer labels

## [1.2.2] - 2026-01-10

### Fixed
- Tonic now correctly sets the root note for interval calculations (not transposition)
  - In 12-TET: Tonic has no audible effect (all semitones equal)
  - In Just Intonation/Scala: Intervals are calculated FROM the tonic note
  - When tonic = D, D is the 1/1 reference; other notes tuned relative to D
  - C still plays as C, but tuned as an interval from D
- Frequency table now rebuilds when tonic changes

## [1.2.1] - 2026-01-10

### Fixed
- Tonic selector now has bi-directional navigation
  - Left arrow (◀) moves down: C → B → A# → ... → C# → C
  - Right arrow (▶) moves up: C → C# → D → ... → B → C
- Keyboard always shows C-D-E-F-G-A-B (physical layout, not relabeled)

## [1.2.0] - 2026-01-10

### Added
- Tonic note selection in Tuning tab
  - Click the "Tonic" selector to cycle through C, C#, D, ... B
  - Updates interval list labels, pitch circle, and keyboard key labels
  - Only available for 12-tone scales

### Fixed
- Scale interval indicator flash bug: UI keyboard no longer causes permanent red lines
  - Root cause: Race condition when both UI and C++ timer called flashIntervalLine()
  - The second call captured already-red color as "original", restoring to red after timeout
  - Solution: Removed direct flashIntervalLine() call from UI keyboard handler
  - C++ timer now handles all note visualization uniformly (external MIDI + UI keyboard)

## [1.1.0] - 2026-01-10

### Added
- Circular scale indicator flashes red when ANY note is played (GUI keyboard, external MIDI, or DAW)
  - C++ Timer polls processor for note-on events and calls WebView via evaluateJavascript
  - Function exported to window scope for cross-thread communication
- A4 reference pitch dial resets to 440 Hz on double-click

### Fixed
- Keyboard animation bug: adjacent black key no longer depresses when white key is clicked
  - Root cause: Black keys were children of white keys, inheriting parent transform
  - Solution: Restructured HTML so black keys are siblings, positioned absolutely
- Black key click detection: right half of black keys now responds correctly
  - Root cause: Black keys extended outside parent container, clicks hit adjacent white key
  - Solution: Black keys as siblings with explicit left positioning (not right: -11px on parent)

## [1.0.0] - 2026-01-09

### Added
- Initial release
- Physically modeled marimba synthesis with bar/mallet interaction
- Microtonality support: 12-TET, Scala file loading, MTS-ESP stub
- WebView UI with botanical paper aesthetic
- Parameters: Mallet Hardness, Bar Material, Resonance, Velocity Curve, Output Gain
- Tuning parameters: Mode selection, A4 reference pitch (400-480 Hz)
- Body resonance via convolution IR
- Playable on-screen keyboard with MIDI output
