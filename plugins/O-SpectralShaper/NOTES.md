# O-SpectralShaper Notes

## Status
- **Current Status:** 📦 Installed
- **Version:** 1.1.3
- **Type:** Audio Effect (Spectral Transient Shaper)

## Lifecycle Timeline

- **2026-02-03:** Creative brief completed — per-frequency transient shaping concept finalized
- **2026-02-07:** v1.1.0 released and installed
- **2026-03-08 (v1.1.1):** Fix three critical bugs — attack/sustain time knobs, curve data race, lookahead latency reporting
- **2026-03-08 (v1.1.2):** Fix thread safety — SafePointer for callAfterDelay, curve value clamping/NaN protection, division-by-zero in NodeCurve
- **2026-03-08 (v1.1.3):** Performance optimizations — cache APVTS pointers, eliminate magic numbers, reuse FFT magnitudes, pre-allocate JSON, cache band frequencies

## Known Issues

None

## Description

Per-frequency transient control with real-time visual feedback. Unlike traditional transient shapers that treat the entire signal identically, O-SpectralShaper uses FFT analysis to detect and shape transients at specific frequencies independently.

## Key Features

- **32 logarithmic bands** for surgical frequency control
- **Per-band transient detection** — each band detects transients independently
- **Drawable curves** — freehand and node-based editing for attack/sustain
- **Real-time spectrogram** with transient heat overlay
- **Low latency (~5ms)** suitable for live mixing

## Parameters

| Parameter | Range | Default |
|-----------|-------|---------|
| Mix | 0-100% | 100% |
| Attack Time | 0.1-50ms | 10ms |
| Sustain Time | 10-500ms | 100ms |
| Sensitivity | 0-100% | 50% |
| Lookahead | 0-10ms | 2ms |
| Output Gain | -12 to +12dB | 0dB |
| Attack Curve | 32 band values | 0.0 |
| Sustain Curve | 32 band values | 0.0 |

## Target Audience

- Mix engineers seeking surgical transient control
- Producers shaping drum punch/snap per element
- Sound designers exploring spectral manipulation

## Future Features (Post v1.0.0)

- Adaptive spectral masks with auto-detection
- Learn mode for spectral profile targeting
- Instrument preset categories

## Additional Notes

Competitive positioning: 32 bands vs typical 5-8, per-band detection, spectrogram visualization with heat overlay — fills gap between simple transient shapers and complex spectral editors.
