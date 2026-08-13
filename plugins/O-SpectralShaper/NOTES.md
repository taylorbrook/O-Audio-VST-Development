# O-SpectralShaper Notes

## Status
- **Current Status:** 📦 Installed
- **Version:** 1.4.0
- **Type:** Audio Effect (Spectral Transient Shaper)

## Lifecycle Timeline

- **2026-02-03:** Creative brief completed — per-frequency transient shaping concept finalized
- **2026-02-07:** v1.1.0 released and installed
- **2026-03-08 (v1.1.1):** Fix three critical bugs — attack/sustain time knobs, curve data race, lookahead latency reporting
- **2026-03-08 (v1.1.2):** Fix thread safety — SafePointer for callAfterDelay, curve value clamping/NaN protection, division-by-zero in NodeCurve
- **2026-03-08 (v1.1.3):** Performance optimizations — cache APVTS pointers, eliminate magic numbers, reuse FFT magnitudes, pre-allocate JSON, cache band frequencies
- **(v1.1.4 – v1.3.1):** See CHANGELOG.md for intervening releases (curve-editor undo/redo, WOLA synthesis window, spectrum overlay, preset-manager v1.0.2 sync).
- **2026-08-12 (v1.4.0):** UI reskinned to the Ouaricon Naturalist aesthetic — aged-paper page, Garamond, seed cross-section knobs, green botanical controls, analysis displays kept as dark specimen plates in walnut frames. Fixed a sidebar overflow that had left **Lookahead, LA Time and Output Gain unreachable** (638px of content in a 418px column). Removed a watermarked Adobe Stock background texture. See CHANGELOG for detail.
- **2026-07-07 (v1.3.2):** Code-review fixes — **CR-01** SafePointer guard on preset file-dialog completions (UAF); **CR-02** factory presets authored in engineering units + `convertTo0to1` so the ATTACK/SUSTAIN skew is applied (all presets were recalling ~10–30× wrong times); **WR-01** knob readouts use `getScaledValue()`; **WR-02** curve-less presets carry explicit flat curve state; **WR-03** latency re-signalled only on change, not every block.

## Known Issues

- **Two sibling plugins still ship a watermarked stock texture:** `O-Lyrica/Resources/ui/images/paper1.jpg` and `O-Gain/Source/ui/public/images/paper1.jpg` are byte-identical (md5 `b7c865c45f2fb95a7a8651071da186e6`) to the tiled-"Adobe Stock" image removed from this plugin in v1.4.0. Both are compiled into their BinaryData and shipped. Needs a separate pass.
- **Lookahead control is inert (deferred):** enabling Lookahead delays detection and signal equally, so it produces no audible change — it only adds reported latency. True lookahead (detection ahead of the shaped output) requires an STFTProcessor detect/apply split and is deferred to a future release. The v1.3.2 fix only stopped the per-block latency re-reporting from the audio thread.

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
