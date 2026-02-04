# O-Detune Changelog

## [1.4.1] - 2026-02-03

### Fixed

- **Mono-safe toggle no longer causes noise**
  - Root cause: Division in side/mid ratio calculation amplified tiny floating-point noise when signal was near zero
  - Fix: Added noise floor check (-120 dB) to skip processing on silent signals
  - Ratio calculation now only happens when mid signal is above noise floor

## [1.4.0] - 2026-02-03

### Changed

- **UI streamlined - Advanced panel removed**
  - Pre-Delay and Feedback knobs moved to Output section (as smaller knobs)
  - Randomization knob now appears in Unison panel only when Distribution = "Random"
  - Advanced section completely removed for cleaner interface

- **Tempo Sync moved under Wobble Rate knob**
  - Sync toggle now directly below the Rate knob in Wobble panel
  - When Sync enabled, Rate display shows musical divisions (1/4, 1/8, 1/16, etc.) instead of Hz
  - Same tempo sync behavior as O-Tremolo

### Technical Notes

- Window height reduced from 520px to 480px
- Output row now uses 5-column grid layout
- New small knob size class (44px) for Pre-Delay and Feedback
- Musical divisions mapped from normalized rate value (0.0 = 4 bars, 1.0 = 1/32)

## [1.3.9] - 2026-02-03

### Removed

- **Character panel completely removed** (Drive, Color, Age parameters)
  - User-requested simplification of plugin interface
  - Removes 3 APVTS parameters: `drive`, `color`, `age`
  - Removes tube saturation, tone shaping, and tape hiss/drift processing
  - Reduces CPU usage by eliminating character DSP processing chain

### Technical Notes

- Removed from PluginProcessor.cpp: Character parameter definitions, `processDrive()` helper, color filter processing, age hiss/envelope follower, filter drift modulation
- Removed from PluginProcessor.h: `smoothedDrive`, `smoothedColor`, `smoothedAge`, `colorFilterL/R`, `filterDriftPhase`, `envelopeL/R`, `processDrive()` declaration
- Removed from PluginEditor: `driveRelay`, `colorRelay`, `ageRelay` and their attachments
- Removed from UI: Character panel HTML, CSS styles, JavaScript knob configs and formatters
- Parameter count reduced from 21 to 18

## [1.3.8] - 2026-02-03

### Fixed

- **Age hiss is now dynamic (envelope-following)** - No more constant hiss
  - Root cause: Hiss noise was generated at constant level based only on Age parameter value
  - The noise did not respond to input signal level, creating constant background hiss
  - Fix: Added envelope follower that tracks input signal amplitude
  - Hiss is now proportional to signal level (silent when no audio passes through)
  - Fast attack (~1ms) catches transients, slower release (~50ms) provides natural decay

### Technical Notes

- Added `envelopeL` and `envelopeR` state variables for per-channel envelope tracking
- Envelope follower uses asymmetric attack/release coefficients
- Envelope scaled by 3x for sensitivity, clamped to 1.0 max
- Hiss level = `ageMix * 0.05 * envScale` (was just `ageMix * 0.05`)

## [1.3.7] - 2026-02-03

### Fixed

- **Character panel (Drive, Color, Age) now has audible effect** (critical bug fix)
  - Root cause: Smoothed values for Color and Age were never advanced with `getNextValue()`
  - The parameter smoothing system requires calling `getNextValue()` per-sample to progress the ramp
  - Color and Age used `getCurrentValue()` for threshold checks but never advanced the smoothing state
  - Result: Parameters were stuck at their initial values and never responded to user adjustment
  - Fix: Restructured Color and Age processing to advance smoothed values per-sample
  - Both parameters now properly respond to knob/automation changes in real-time

### Technical Notes

- `SmoothedValue<float>` requires `getNextValue()` to be called to progress the interpolation
- `getCurrentValue()` only reads current state without advancing
- Color filter now updates coefficients per-sample (was per-block) for smoother automation
- Age hiss level now calculated per-sample for proper parameter tracking

## [1.3.6] - 2026-02-03

### Fixed

- **Restored wobble engine functionality** (was accidentally not working in v1.3.5)
  - Restored from v1.3.0 backup and carefully reapplied only unison changes
  - Wobble engine code is now identical to the working v1.3.0 version
  - Unison engine updated with new chorus algorithm (from v1.3.4/v1.3.5)

### Technical Notes

- Wobble engine unchanged from v1.3.0
- Unison engine uses per-voice sine LFO (from v1.3.4)
- Gain compensation: 1/N with always-on tanh() saturation (from v1.3.5)
- voiceRandomOffsets initialized in prepareToPlay for Random distribution

## [1.3.5] - 2026-02-03

### Fixed

- **Eliminated clipping on loud signals** (especially with Random distribution)
  - Root cause: `1/sqrt(N)` gain compensation wasn't enough when LFO phases aligned
  - Fix: Changed to `1/N` gain compensation (more conservative)
  - Added always-on `tanh()` soft saturation (not just above threshold)
  - This provides gentle compression that increases naturally with level

### Technical Notes

- Previous: `gainCompensation = 1/sqrt(N)`, conditional clipper above 0.9
- Now: `gainCompensation = 1/N`, always-on `tanh(s)` saturation
- `tanh(x)` is transparent at low levels, compresses smoothly at high levels

## [1.3.4] - 2026-02-03

### Changed

- **Complete rewrite of unison engine using classic chorus algorithm**
  - Previous approach (drift modulation with smoothing) still had click artifacts
  - New approach: Per-voice sine LFO modulating delay time (proven click-free design)
  - This is how classic chorus pedals work (Boss CE-1, Roland Dimension D, etc.)

### How It Works Now

- Each voice has a sine LFO at a slightly different rate
- LFO continuously modulates delay time (sine is always smooth)
- Detune parameter controls modulation depth (50 cents ≈ ±3ms sweep)
- Distribution affects LFO rate spread between voices:
  - Linear: Even rate spread
  - Exponential: Outer voices have more rate difference
  - Random: Uses stored random values to vary rates (no updates during playback)

### Technical Notes

- Base LFO rate: 0.5 Hz (2-second cycle)
- Max modulation depth: ±3ms around 50ms center
- Sine wave is continuous everywhere (no discontinuities at phase wrap)
- Random offsets now only affect LFO rate, not detune values directly

## [1.3.3] - 2026-02-03

### Fixed

- **Eliminated remaining clicks in unison engine** (especially with Random distribution)
  - Root cause: When detune values changed (random offset updates), driftRate could flip sign causing sudden direction reversal
  - Fix: Added dual-layer smoothing system
    1. Per-voice detune smoothing (α=0.0002, ~100ms time constant)
    2. Per-voice delay time smoothing (α=0.001, ~20ms time constant)
  - All parameter changes now interpolate smoothly over time

### Changed

- Reduced drift range from ±10ms to ±5ms for subtler chorusing effect
- Detune direction now based on smoothed detune value (prevents sign-flip clicks)

### Technical Notes

- `smoothedVoiceDetunes[voice]` prevents sudden detune jumps from random offsets
- `smoothedDelayTimes[voice]` provides final safety net for delay discontinuities
- Exponential smoothing: `value += (target - value) * coefficient`

## [1.3.2] - 2026-02-03

### Fixed

- **Eliminated clicks in unison engine** (critical bug fix)
  - Root cause: Sawtooth drift phase wrapped abruptly from 1.0→0.0, causing ~20ms delay jump
  - Fix: Replaced sawtooth with triangle wave modulation
  - Triangle wave reverses direction smoothly at boundaries (no discontinuity)
  - Creates chorus-like oscillation around target pitch (sounds natural)

### Changed

- Reduced drift range from ±20ms to ±10ms for faster cycles and more stable pitch perception
- Delay offset now centered around centerDelay (±driftRange/2) for symmetric modulation

### Technical Notes

- Triangle wave: phase 0→0.5 rising, 0.5→1 falling
- Direction reversal at 0.5 creates smooth pitch oscillation
- Smaller drift range = more frequent direction changes = less perceived pitch drift

## [1.3.1] - 2026-02-03

### Fixed

- **Unison engine now produces audible pitch detuning** (critical bug fix)
  - Root cause: Static delay times don't create pitch shift - delay-based pitch shifting requires continuously modulating delay time
  - Fix: Implemented per-voice sawtooth drift phases that continuously modulate delay time
  - Each voice now has a drifting delay offset that creates actual pitch variation
  - Drift phases staggered at init for richer chorusing effect
  - Proper pitch ratio calculation: 2^(cents/1200) determines drift rate

- **Fixed glitch distortion when Spread + Random distribution used with loud signals**
  - Root cause #1: Random distribution double-applied `voiceRandomOffsets` (once in distribution calc, again in effectiveDetune)
  - Root cause #2: Voice accumulation had no output limiting, causing clipping on loud signals
  - Root cause #3: Random offsets refreshed too frequently (1024 samples), causing audible jumps
  - Fix: Random offsets only applied once per distribution mode
  - Fix: Added soft clipper after voice accumulation (tanh compression above 0.9)
  - Fix: Increased random refresh interval to 4096 samples with smoothed interpolation

### Changed

- Voice gain compensation changed from `1/N` to `1/sqrt(N)` for better loudness consistency across voice counts
- Random offset interpolation now uses 70/30 blend toward new target (smoother transitions)

### Technical Notes

- Drift range: ±20ms around 50ms center delay
- Drift rate derived from pitch ratio: `driftRate = 1 - pitchRatio`
- Soft clipper threshold: 0.9, uses `tanh(x * 1.5) / 1.5` for gentle saturation

## [1.3.0] - 2026-02-03

### Added

- **All 14 placeholder parameters now functional** - Complete parameter implementation
  - Wobble: Era presets (60s/70s/80s), multi-waveform LFO (Sine/Triangle/Random), tempo sync
  - Unison: Voice count (2/3/4/5/7), distribution modes (Linear/Exp/Random), spread, randomization
  - Character: Drive (tube saturation), Color (tone shaping with age drift), Age (hiss + filter drift)
  - Output: Width (M/S stereo spread), Pre-delay, Feedback, Mono-safe

### Changed

- **Mono-safe now forces width to 0** - Enabling mono-safe automatically sets stereo width to mono
  - Width slider becomes disabled (grayed out) when mono-safe is active
  - Previous width value is preserved and restored when mono-safe is disabled
  - Width automation is ignored while mono-safe is active

### Technical Notes

- SmoothedValue used for all 12 continuous parameters (zipper-free automation)
- Era presets affect wobble depth and age drift intensity
- DSP helper functions: generateLFO, processDrive, processWidth, processMonoSafe
- Real-time safe: all buffers preallocated, no allocations in processBlock

## [1.2.0] - 2026-02-02

### Added

- **Blend-responsive panel opacity** - Wobble and Unison panels now fade based on blend knob position
  - Blend at 0%: Wobble panel fully visible, Unison panel faded (35% opacity)
  - Blend at 100%: Wobble panel faded, Unison panel fully visible
  - Provides clear visual feedback for which engine is active
  - Smooth 200ms CSS transition between states
  - Minimum 35% opacity ensures controls remain usable

### Technical Notes

- Panel opacity updates via blend parameter's valueChangedEvent
- Initial state synced on load from JUCE parameter value

## [1.1.1] - 2026-02-02

### Fixed

- **UI knobs now move smoothly** (visual animation improvement)
  - Root cause: Knob visuals relied solely on JUCE parameter callbacks without frontend animation interpolation. Backend updates fire at audio block rate, causing discrete visual jumps.
  - Fix: Implemented `requestAnimationFrame` loop with exponential smoothing interpolation (factor 0.15) for vine arc SVG updates
  - Removed CSS `transition` property on `.knob-vine` stroke-dashoffset (now handled by JS)
  - Text value displays update immediately (no lag) while visual arcs interpolate smoothly

### Added

- **Mouse wheel support for knobs** - scroll up/down to adjust values with fine control (±2% per scroll tick)

### Technical Notes

- Animation system tracks target vs current normalized values per knob
- Loop runs only when knobs are animating (no idle CPU cost)
- Smoothing factor 0.15 provides balance between responsiveness and smoothness

## [1.1.0] - 2026-02-02

### Changed

- **Complete UI redesign to Ouaricon Naturalist aesthetic**
  - Replaced dark gradient background with aged parchment paper texture
  - Added nudibranch (sea slug) botanical illustration overlay at 32% opacity
  - Converted all knobs from CSS indicator style to SVG vine-arc design (O-Freeze style)
  - Changed typography from system sans-serif to Georgia serif with brown earth tones
  - Restyled panels with subtle translucent backgrounds and brown borders
  - Updated dropdowns with parchment background and custom arrow indicator
  - Converted toggles to botanical green theme matching Ouaricon brand
  - Restyled slider with green thumb matching accent palette

### Visual Elements

- **Color Palette:** Paper (#F5E6D3), Text (#3C2F2F, #8b7355), Accent green (#5a7a6a)
- **Typography:** Georgia/Times New Roman serif, letter-spacing for labels
- **Knob Design:** 52px standard / 64px large SVG circles with animated vine arc fill
- **Botanical Overlay:** Nudibranch illustration positioned right side

### Technical Notes

- Window size changed from 600x400 to 600x520 to accommodate improved layout
- Added paper1.jpg and slug.png to BinaryData resources
- All parameter bindings preserved from v1.0.1 (no DSP changes)

## [1.0.1] - 2026-02-02

### Fixed

- **UI knobs and dropdowns now respond to interaction** (critical bug fix)
  - Root cause #1: ComboBox API used wrong method names (`getChosenItemIndex` → `getChoiceIndex`)
  - Root cause #2: Build system cache issue - HTML changes weren't being picked up by ninja incremental builds
  - Fix: Changed to correct JUCE 8 API `getChoiceIndex()`/`setChoiceIndex()` + clean rebuild required

### Technical Notes

- **JUCE 8 ComboBox API:** Uses `getChoiceIndex()`/`setChoiceIndex()`, NOT `getChosenItemIndex()`/`setChosenItemIndex()`
- **Build cache issue:** When modifying WebView HTML/JS, delete `juce_binarydata_*` directory and run `cmake ..` to force BinaryData regeneration
- Files modified: Source/ui/public/index.html (3 method calls corrected)

## [1.0.0] - 2026-02-01

### Initial Release

**O-Detune** - Colorful lo-fi detuning plugin that combines analog tape wobble with unison thickness in one mono-safe package.

### Features

#### Dual-Engine Architecture
- **Wobble Engine**: Delay-based pitch modulation with tape-style wow/flutter
  - Rate: 0.1-10 Hz (slow wow to fast flutter)
  - Depth: 0-100 cents pitch deviation
  - Era presets: 60s (Ampex), 70s (Teac), 80s (Cassette)
  - LFO shapes: Sine, Triangle, Random
  - Tempo sync support

- **Unison Engine**: Multi-voice pitch shifting for supersaw-style thickness
  - 3-voice detuning (fixed in v1.0, expansion planned)
  - Detune range: 0-50 cents
  - Linear distribution
  - Stereo spread control

- **Blend Control**: Crossfade between engines (0 = Wobble, 1 = Unison)

#### Character Processing
- **Drive**: Subtle warmth to tube-style saturation (tanh waveshaping)
- **Color**: Tone shaping from dark (low-pass) to bright (high-shelf)
- **Age**: Combined degradation (hiss + filter drift)

#### Output Section
- **Width**: Stereo spread (0-200%, mono to extra-wide)
- **Mix**: Wet/dry blend with latency compensation
- **Focus Filter**: Frequency-selective processing (20Hz-20kHz)
- **Mono-Safe**: Guaranteed mono compatibility toggle

#### Advanced Controls
- **Pre-Delay**: 0-50ms spatial depth
- **Feedback**: 0-80% recirculation
- **Randomization**: Per-voice variation amount

### Technical Details
- 21 automatable parameters
- Latency: 50ms (2400 samples @ 48kHz)
- CPU-efficient delay-based architecture
- Real-time safe DSP (no allocations in processBlock)
- WebView UI with colorful lo-fi aesthetic

### Factory Presets
1. **Default** - Balanced starting point
2. **Thick Vocals** - 3-voice unison for vocal thickening
3. **Supersaw Synth** - Wide 5-voice detuning for synths
4. **70s Tape Wobble** - Authentic Teac-style pitch variation
5. **Cassette Lo-Fi** - Degraded 80s tape character
6. **Hybrid Wobble Unison** - Combined wobbling unison voices

### Supported Formats
- VST3 (macOS)
- Audio Units (AU) (macOS)
- Standalone application

### System Requirements
- macOS 11+ (Apple Silicon native)
- DAW supporting VST3 or AU plugins

---

*Developed by Ouaricon Development*
*Taylor Brook*
