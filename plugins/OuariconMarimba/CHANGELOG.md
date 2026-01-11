# Ouaricon Marimba Changelog

All notable changes to this project will be documented in this file.

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
