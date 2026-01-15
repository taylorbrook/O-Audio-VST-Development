# Ouaricon Marimba Changelog

All notable changes to this project will be documented in this file.

## [1.9.9] - 2026-01-14

### Fixed
- **Effects tab layout: Compressor module no longer overlaps EQ module**
  - Moved compressor panel from `top: 95px` to `top: 109px`
  - Now has proper 4px gap between EQ and Compressor modules

## [1.9.8] - 2026-01-14

### Fixed
- **Output gain and VU meter moved to end of processing chain**
  - Root cause: Output gain was applied twice - once per-voice during synthesis, then again after effects
  - Fix: Removed gain application from `MarimbaVoice::renderNextBlock`; now applied only once after EQ/Compressor
  - VU meter now shows true final output level after all processing

### Changed
- **Signal chain clarified and documented**:
  ```
  Synth → Body Resonance → EQ (if enabled) → Compressor (if enabled) → Output Gain → VU Meter
  ```

### Technical Details
- MarimbaVoice.cpp: Removed `* outputGain` from finalSample calculation (line 131)
- PluginProcessor.cpp: Removed `voice->setOutputGain()` call; gain applied once via `buffer.applyGain()`
- No change to parameter behavior - output knob works identically, just applied at correct point in chain

## [1.9.7] - 2026-01-14

### Fixed
- **Compressor attack clicking eliminated** - Added 3ms look-ahead buffer
  - Root cause: Gain was applied to same sample that triggered detection
  - Transient leading edge passed through at full level, then suddenly attenuated
  - Fix: Audio delayed by 3ms; detection runs on current input, gain applied to delayed audio
  - Gain changes now happen BEFORE transients arrive, eliminating discontinuities
- **Autogain now matches OuariconComp standalone** - Theoretical formula replaces slow tracking
  - Root cause: Old autogain used measured GR × 0.6 with extremely slow smoothing (0.0005 coeff)
  - Fix: Now uses `autoGainDB = -threshold × (1 - 1/ratio)` - same as standalone compressor
  - Result: Full loudness compensation, instant response

### Technical Details
- Compressor module updated to v1.3.0 (`modules/effects/compressor-unit/cpp/`)
- Look-ahead implemented via circular delay buffer (stereo, sized for block + lookahead)
- Bypass mode also uses delay buffer to maintain consistent latency
- Autogain formula: at -20dB threshold, 4:1 ratio = 15dB makeup (was ~6dB × slow ramp)

## [1.9.6] - 2026-01-14

### Changed
- **Compressor module updated** - Synced with module system v1.2.3 cleanup
  - Added named constants for magic numbers (MIN_DB, AUTOGAIN_COEFF, etc.)
  - Moved version history from headers to module CHANGELOG
  - Code condensed and formatting improved (no DSP changes)
- **Added PLUGIN_VERSION** to CMakeLists.txt for proper version tracking

### Technical Details
- CompressorUnit.h: Named constants, cleaner code structure
- compressor-unit.js: Constants for UI values, CSS compacted, template-generated meter segments
- No functional changes - identical audio behavior to v1.9.5

## [1.9.5] - 2026-01-14

### Fixed
- **Compressor clicking on enable** - Smooth bypass-to-enabled transition
  - Root cause: `smoothedGainDB` retained stale value during bypass, causing gain jump when re-enabled
  - Fix: Bypass now smoothly ramps gain toward unity (0 dB) each block
  - Envelope also resets during bypass so compression starts fresh
- **GR meter now correctly shows 0 when bypassed** - Verified meter behavior
- **Increased minimum smoothing time** from 5ms to 10ms for better click prevention

### Technical Details
- Compressor module DSP updated to v1.2.3 (`modules/effects/compressor-unit/cpp/`)
- Bypass handling: iterates through buffer samples ramping `smoothedGainDB` toward 0
- When enabled, gain is already near unity - no discontinuity

## [1.9.4] - 2026-01-14

### Fixed
- **Compressor clicking fully resolved** - Complete overhaul of gain smoothing DSP
  - Root cause 1: Smoothing coefficient was sample-rate independent (faster at higher rates)
  - Root cause 2: Smoothing in linear domain caused non-uniform perceptual response
  - Root cause 3: Gain smoother ignored attack/release settings

### Technical Details
- Compressor module DSP updated to v1.2.2 (`modules/effects/compressor-unit/cpp/`)
- **Sample-rate independent smoothing:** Coefficient now calculated in `prepare()` using:
  `gainSmoothCoeff = 1 - exp(-1000 / (5ms × sampleRate))` - consistent ~5ms at any rate
- **dB domain smoothing:** `smoothedGainDB` replaces `smoothedGainLinear` for perceptually
  uniform response across all gain reduction levels
- **Attack/release-aware gain changes:**
  - Gain decreasing (compression engaging) → uses attack coefficient
  - Gain increasing (compression releasing) → uses release coefficient
  - Minimum 5ms smoothing always applied to prevent clicks

## [1.9.3] - 2026-01-14

### Fixed
- **Compressor clicking at high gain reduction** - DSP clicking/popping eliminated
  - Root cause: Gain was applied sample-by-sample without smoothing
  - Fix: Added gain coefficient smoothing (`GAIN_SMOOTH_COEFF = 0.005`)
  - Smoothed gain interpolates toward target to prevent abrupt discontinuities

### Technical Details
- Compressor module DSP updated to v1.2.1 (`modules/effects/compressor-unit/cpp/`)
- New member: `smoothedGainLinear` - tracks smoothed gain coefficient
- Smoothing preserves transient response while eliminating audible artifacts

## [1.9.2] - 2026-01-14

### Changed
- **Effects tab module dimensions** - Both EQ and Compressor modules resized
  - 10px shorter vertically (padding reduced from 8px to 3px)
  - 100px wider (horizontal padding increased from 10px to 60px)
- **Compressor knobs updated** to canonical 10-segment Ouaricon seed cross-section
  - Pattern now matches EQ inner dial and official Ouaricon naturalist aesthetic
  - 36° segments with 1° brown dividers (was 17° segments with 20 divisions)

### Technical Details
- EQ module updated to v1.2.0 (`modules/effects/analog-eq-unit/`)
- Compressor module updated to v1.2.0 (`modules/effects/compressor-unit/`)
- CSS padding changed: `.eq-unit-compact` and `.comp-unit-compact` now use `3px 60px`

## [1.9.1] - 2026-01-14

### Changed
- **Compressor UI redesigned** - Compact single-row layout matching EQ module
  - **COMP button** (left) - Title/bypass toggle (green when ON)
  - **Centered knobs** - 4 knobs aligned with EQ bands above
  - **AUTO button** (right) - Autogain toggle for automatic makeup gain
  - **GR meter** (far right) - Vertical LED meter
- Layout: `[COMP] | [Thresh] [Ratio] [Attack] [Release] | [AUTO] | [GR]`

### Added
- **Autogain feature** - Automatic makeup gain compensates for compression
  - When enabled, applies makeup gain equal to peak gain reduction
  - Maintains perceived loudness while compressing
- New parameter: `fx_comp_autogain` (boolean, default OFF)

### Technical Details
- Compressor module now uses same dark background as EQ (`#2a2318`)
- UI height reduced from 100px to ~80px (compact row)
- Button styles match EQ module toggles

## [1.9.0] - 2026-01-14

### Added
- **Compressor Module** in Effects tab - Compact dynamics processor below EQ
  - **Threshold** (-60 to 0 dB) - Input level where compression starts
  - **Ratio** (1:1 to 20:1) - Amount of gain reduction applied
  - **Attack** (0.1-100 ms) - How quickly compression engages
  - **Release** (10-1000 ms) - How quickly compression releases
  - **Fixed 6dB soft knee** for musical response
  - **Clickable title** for bypass toggle (ON by default)
  - **Vertical GR LED meter** showing real-time gain reduction
- Naturalist seed-knob aesthetic matching plugin visual design
- Module uses Ouaricon Module System (`modules/effects/compressor-unit/`)

### Technical Details
- Compressor processes audio after EQ: Synth → Body Resonance → EQ → **Compressor** → Output Gain
- 5 new parameters with `fx_comp_` prefix for DAW automation:
  - `fx_comp_enabled` (bool, default ON)
  - `fx_comp_threshold` (float, -60 to 0 dB, default -20)
  - `fx_comp_ratio` (float, 1 to 20, default 2)
  - `fx_comp_attack` (float, 0.1-100 ms, default 10)
  - `fx_comp_release` (float, 10-1000 ms, default 100)
- WebView relays: 4 slider, 1 toggle
- GR meter updates via `compressorGR` event at 30Hz
- UI positioned at top: 130px in Effects tab (below EQ at top: 10px)

### Signal Flow
```
Input → LF Shelf → LMF Bell → HMF Bell → HF Shelf → Saturation → Compressor → Output Gain → Output
        (if on)    (if on)    (if on)    (if on)    (if on)      (if on)
```

## [1.8.1] - 2026-01-14

### Changed
- **Compact EQ module** - Redesigned to half height for better UI space efficiency
  - Single-row layout: [EQ bypass] | [LF] [LMF] [HMF] [HF] | [ANALOG]
  - Removed header section and footer (output knob)
  - Smaller dual-ring knobs (60px → 46px) with proportionally scaled SVG notches

### Added
- **EQ bypass toggle** - "EQ" button on left side serves as:
  - Module title/label
  - Master EQ on/off switch (OFF by default = bypassed)
  - When OFF, bands and analog toggle are dimmed and non-interactive
- New parameter: `fx_eq_enabled` (boolean, default OFF)

### Removed
- Output gain knob from EQ module (DSP parameter still exists for preset compatibility)
- EQ header section with "ANALOG EQ" title

### Technical Details
- Analog toggle moved inline with band knobs (right side)
- EQ module uses `eq-unit-compact` CSS class with `.bypassed` state
- WebView relay added: `eqEnabledRelay` with attachment to `fx_eq_enabled`
- DSP: `eqUnit.process()` only called when `fx_eq_enabled` is true

## [1.8.0] - 2026-01-13

### Added
- **Analog EQ Module** in Effects tab - 4-band analog-style parametric EQ
  - **LF Shelf** (30-500 Hz) - Low frequency boost/cut
  - **LMF Bell** (100-2000 Hz) - Low-mid parametric with WIDE/MED/TIGHT Q selection
  - **HMF Bell** (500-8000 Hz) - High-mid parametric with WIDE/MED/TIGHT Q selection
  - **HF Shelf** (2000-20000 Hz) - High frequency boost/cut
  - **Analog saturation** circuit for subtle warmth (toggleable)
  - **Output gain** trim (±12 dB)
- **Dual-ring knob design** - Outer ring controls frequency, inner dial controls gain
  - Click position determines which parameter is adjusted
  - Double-click to reset to center (0 dB gain, center frequency)
  - Drag vertically to adjust values
  - Tooltip shows both values on hover
- Module uses Ouaricon Module System (first effects module integration)

### Technical Details
- EQ processes audio at end of chain: Synth → Body Resonance → **Analog EQ** → Output Gain
- 16 new parameters with `fx_eq_` prefix for DAW automation
- Uses AnalogEQUnit from `modules/effects/analog-eq-unit/`
- WebView relays: 9 slider, 5 toggle, 2 combobox
- UI positioned in top 1/4 of Effects tab (80px), leaving room for future modules

### Signal Flow
```
Input → LF Shelf → LMF Bell → HMF Bell → HF Shelf → Saturation → Output Gain → Output
        (if on)    (if on)    (if on)    (if on)    (if on)
```

## [1.7.0] - 2026-01-13

### Added
- **New EFFECTS tab** - Third tab in the UI for future effects controls
  - Empty placeholder tab ready for effects implementation
  - Parallax tree background shifts further left when navigating to Effects tab
  - Consistent visual transition behavior matching Sound → Tuning navigation

### Technical Details
- Added `.botanical-overlay.effects-position` CSS class (right: -180px, opacity: 0.12)
- Added `#effects-tab` content container
- Updated tab switching JavaScript to handle three-tab navigation
- Tree parallax progression: Sound (-60px) → Tuning (-120px) → Effects (-180px)

## [1.6.2] - 2026-01-11

### Changed
- **UI refinements:**
  - Velocity knob now same size as other control knobs (small class)
  - Velocity curve display scaled to 75% (105x75px)
  - Tone dial verified aligned with Resonance dial above (left: 200px)
- **Preset system updated:**
  - Added STRIKE_POSITION, OVERTONE_DAMPING, TONE to preset format
  - Updated all 10 factory presets with new parameter values
  - Added 2 new presets: "Pad Marimba" and "Staccato Marimba"
  - Factory presets now regenerate on plugin load to include new parameters

### Factory Presets (v1.6.2)
| Preset | Strike | Damping | Tone | Character |
|--------|--------|---------|------|-----------|
| Default Marimba | 0.5 | 0.5 | 0.75 | Natural, balanced |
| Bright Marimba | 0.15 | 0.3 | 0.95 | Edge strike, shimmery |
| Soft Marimba | 0.5 | 0.6 | 0.5 | Center, warm |
| Pad Marimba | 0.5 | 0.1 | 0.7 | Long sustain, shimmer |
| Staccato Marimba | 0.5 | 0.9 | 0.6 | Tight, focused |

## [1.6.1] - 2026-01-11

### Changed
- **UI refinement:** Made all 6 Sound panel knobs the same size (small) and shifted both rows up to prevent label overlap with lower GUI elements
- **2x more extreme parameter ranges** for all 6 timbre controls:
  - **Mallet Hardness:** Duration 2-25ms (was 5-20ms), filter 800Hz-14kHz (was 2-8kHz)
  - **Bar Material:** Mode boost 0.4x-4.0x (was 1.0x-2.0x) - now can attenuate or strongly boost
  - **Resonance:** Decay 0.15-10s (was 0.5-5s) - staccato to pad-like sustain
  - **Strike Position:** Mode multipliers doubled - much stronger edge/center contrast
  - **Overtone Damping:** Factor 0.02-0.9 (was 0.1-0.5) - from shimmering to very tight
  - **Tone:** Cutoff 400Hz-20kHz (was 2kHz-20kHz) - much darker low end possible

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
