# O-Freeze Changelog

All notable changes to O-Freeze are documented in this file.

## [1.0.1] - 2026-02-02

### Fixed

- **Smooth knob animation**: Replaced CSS transition with JavaScript `requestAnimationFrame` animation loop
  - **Root cause**: CSS `transition: stroke-dashoffset 0.1s ease` caused jerky snap/jump behavior
  - **Solution**: Exponential smoothing interpolation (factor 0.15) for fluid knob movement
  - Pattern applied from O-Detune v1.1.1 fix

## [1.0.0] - 2026-02-01

### Initial Release

First public release of O-Freeze granular freeze effect.

### Features

- **8-Grain Granular Engine**
  - 200ms grains with 87.5% overlap
  - Asymmetric Blackman-Harris window (60% attack / 40% release)
  - 2-second freeze buffer supporting up to 192kHz sample rate

- **Dual Trigger Modes**
  - Manual: Button-controlled freeze
  - Threshold: Automatic RMS-based gate with 3dB hysteresis

- **Parameters**
  - FREEZE: Manual trigger button
  - THRESHOLD: Auto-freeze level (-60 to 0 dB)
  - MODE: Manual/Threshold selection
  - DRIFT: Grain position randomization (0-100%)
  - MIX: Dry/Wet blend (0-100%)

- **UI**
  - WebView-based botanical design
  - Paper texture background with anatomical overlay
  - Organic SVG freeze button with pulse animation
  - Botanical rotary knobs with vine arc indicators
  - Mode toggle with conditional disabled states
  - Grain activity particles (6 animated)

- **Audio**
  - Smooth crossfades (50ms in, 100ms out)
  - juce::dsp::DryWetMixer integration
  - Sample-accurate stereo processing

### Formats

- VST3 (macOS)
- AU (macOS)

### Known Limitations

- macOS only (Windows/Linux builds planned for future release)
- No factory presets in V1.0.0

---

*Ouaricon Development*
