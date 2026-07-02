# Changelog - Scatter

## [1.0.0] - 2025-11-14

### Initial Release

**Plugin Type:** Audio Effect (Granular Delay)

**Description:** Granular reversed delay with beautiful stuttering grains, randomized pitch (quantized to musical scales), and randomized stereo placement.

### Features

#### Core DSP
- 64-voice polyphonic granular synthesis engine
- Hann windowing for smooth grain envelopes (click-free)
- Density-based grain scheduling with overlap control
- Lagrange3rd interpolation for smooth pitch shifting

#### Pitch & Harmony
- Playback rate pitch shifting (±7 semitones)
- Scale quantization system (5 scales):
  - Chromatic (all notes)
  - Major
  - Minor
  - Pentatonic
  - Blues
- Root note transposition (C through B)
- Random pitch generation scaled by pitch_random parameter

#### Spatial Processing
- Per-grain stereo pan randomization
- Independent L/R channel processing
- Reverse playback (50/50 probability per grain)
- Feedback loop for evolving textures (stability-capped at 0.95)

#### UI & Visualization
- WebView-based interface (550×600px)
- Cream/spacey/textured skeuomorphic design
- 7 rotary knobs with 3D shadows and gradients
- 2 combo boxes for scale and root note selection
- Real-time particle field visualization (200×200px)
  - X-axis: Time position in delay buffer
  - Y-axis: Pitch shift amount
  - Glow intensity: Stereo pan position
- 60fps smooth animation via requestAnimationFrame

#### Parameters (9 total)
- **delay_time** (100-2000ms): Delay buffer size
- **grain_size** (5-500ms): Individual grain duration
- **density** (0-100%): Grain spawn rate
- **pitch_random** (0-100%): Amount of pitch randomization
- **scale** (Choice): Musical scale for quantization
- **root_note** (Choice): Root transposition
- **pan_random** (0-100%): Amount of stereo randomization
- **feedback** (0-100%): Feedback amount
- **mix** (0-100%): Dry/wet blend

#### Factory Presets (5)
- **01_Default**: Balanced starting point
- **02_Shimmer**: High pitch randomization with reverb-like feedback
- **03_Stutter**: Dense, short grains for rhythmic effects
- **04_Ambient_Wash**: Long grains with high feedback
- **05_Granular_Reverse**: Moderate density with pitch variation

### Technical Details

- **Formats:** VST3, AU, Standalone
- **JUCE Version:** 8.x
- **Sample Rates:** 44.1kHz, 48kHz, 96kHz tested
- **Real-time Safe:** No heap allocations in processBlock()
- **Thread Safety:** Lock-free parameter reads, atomic APVTS access
- **Validation:** Passes pluginval strictness level 5

### Architecture Highlights
- Pre-allocated grain voice array (64 voices)
- juce::dsp::DelayLine with Lagrange3rd interpolation
- juce::dsp::WindowingFunction for Hann envelopes
- juce::dsp::DryWetMixer for latency-compensated mixing
- Thread-safe grain position streaming (30Hz C++ → JavaScript)
- No audio thread locks for visualization updates

### Performance
- **Estimated CPU:** ~50-65% single core at 48kHz with 64 voices
- **Latency:** delay_time parameter (user-controlled)
- **Memory:** ~192KB per channel for 2000ms delay buffer at 48kHz

### Known Limitations
- Reverse playback is always 50/50 probability (no parameter control)
- Tempo sync not implemented in v1.0.0 (free-running delay)
- Voice count is fixed at 64 (no dynamic adjustment)

---

**Built with:** Plugin Freedom System
**Generated with:** [Claude Code](https://claude.com/claude-code)
