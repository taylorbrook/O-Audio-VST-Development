# O-Detune Changelog

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
