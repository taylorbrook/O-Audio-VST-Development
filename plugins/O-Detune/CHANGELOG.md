# O-Detune Changelog

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
