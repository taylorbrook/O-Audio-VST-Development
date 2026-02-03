# O-Detune Changelog

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
