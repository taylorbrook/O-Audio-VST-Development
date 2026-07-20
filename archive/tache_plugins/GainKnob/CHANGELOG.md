# Changelog

All notable changes to GainKnob will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.2.3] - 2025-11-10

### Fixed
- Filter state burst/peak when crossing from low-pass to high-pass eliminated
- Added `filterProcessor.reset()` when switching filter types
- Reset also applied when entering/exiting bypass zone

### Technical Details
- IIR filters maintain delay buffers with feedback (internal state)
- Switching filter types without clearing state causes transient burst
- Solution: Track previous filter type, reset state on transitions
- Brief silence at crossover (acceptable for DJ-style filter sweep)

**Research sources:**
- JUCE forum: "What causes IIRFilter to pop when frequency changed"
- JUCE docs: dsp::IIR::Filter and StateVariableFilter
- Root cause: Residual low-frequency energy in delay buffers amplified by high-pass

## [1.2.2] - 2025-11-10

### Fixed
- Filter frequency direction inverted - center position (0%) now bypasses correctly
- Low-pass: -100% = 200Hz (heavy bass), 0% = 20kHz (bypass) - was reversed
- High-pass: 0% = 20Hz (bypass), +100% = 10kHz (heavy treble)
- Center knob position (12 o'clock) now sounds unaffected as expected

### Technical Details
- Inverted low-pass formula using negative exponent
- Low-pass now decreases frequency as knob moves left from center
- High-pass increases frequency as knob moves right from center (unchanged)

## [1.2.1] - 2025-11-10

### Fixed
- Filter cutoff frequency mapping now uses exponential curve for musical sweep
- Low-pass: -100% = 200Hz, -50% = ~1.4kHz, 0% = 20kHz (was linear)
- High-pass: 0% = 20Hz, +50% = ~1kHz, +100% = 10kHz (was linear)
- Middle knob positions now provide useful filtering instead of extreme values only

### Technical Details
- Changed from linear (200Hz-20kHz) to logarithmic frequency mapping
- Formula: `cutoff = base * 10^(normalized * log10(range))`
- Added safety limits via juce::jlimit for cutoff boundaries

## [1.2.0] - 2025-11-10

### Added
- FILTER parameter (-100% to +100%, center at 0% = bypass)
- DJ-style filter knob (negative = low-pass, positive = high-pass)
- 2nd-order Butterworth IIR filters (Q=0.707 for smooth response)
- Cutoff frequency range: 200Hz to 20kHz
- Smart filter display (OFF at center, LPF/HPF when active)
- Third rotary knob in UI

### Changed
- Window width increased from 600px to 800px to accommodate three knobs
- DSP chain: Filter → Gain → Pan
- Added juce_dsp module dependency for IIR filtering

### Technical Details
- Filter bypass when value within ±0.5% of center
- Real-time coefficient updates (no audio thread allocations)
- Per-channel filter processing via ProcessorDuplicator

## [1.1.0] - 2025-11-10

### Added
- PAN parameter (-100% L to +100% R, center at 0%)
- Second rotary knob in UI for pan control
- Constant power panning algorithm (maintains equal perceived loudness)
- Smart pan value display (C for center, L/R for left/right positions)
- Stereo width control through independent channel gain adjustment

### Changed
- Window width increased from 400px to 600px to accommodate two knobs
- DSP now applies both gain and pan to stereo signal
- UI layout updated to horizontal knob arrangement

## [1.0.0] - 2025-11-10

### Added
- Initial release of GainKnob utility plugin
- Single GAIN parameter (-60 to 0 dB range)
- Clean, minimal WebView-based UI with centered rotary knob
- dB value display with -∞ representation at minimum
- Logarithmic gain conversion (dB to linear) using JUCE Decibels class
- Complete silence at -60 dB (gain multiplier = 0.0)
- Unity gain at 0 dB (gain multiplier = 1.0)
- Factory presets:
  - Unity (0 dB - no attenuation)
  - Subtle Cut (-3 dB)
  - Half Volume (-6 dB)
  - Quiet (-12 dB)
  - Silence (-60 dB)
- VST3 and AU plugin format support
- Standalone application mode
- State save/load functionality via APVTS
- Real-time safe parameter modulation
- Denormal protection in audio processing

### Technical Details
- Built with JUCE 8
- WebView UI integration using juce-frontend library
- AudioProcessorValueTreeState for parameter management
- Time-domain gain multiplication
- Zero-latency processing
- Minimal CPU usage

[1.0.0]: https://github.com/user/plugin-freedom-system/releases/tag/GainKnob-v1.0.0
