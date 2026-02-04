# O-FreqPulse Notes

## Status
- **Current Status:** 📦 Installed
- **Version:** 1.1.0
- **Type:** Audio Effect (Spectral Sequencer)

## Lifecycle Timeline

- **2026-02-03:** Ideation complete — creative brief and requirements documented
- **2026-02-03 (Stage 1):** Foundation + Shell complete — 165 parameters, APVTS, build working
- **2026-02-03 (Stage 2):** DSP complete — FFT spectral processing, step sequencing, Euclidean generator
- **2026-02-03 (Stage 3):** GUI complete — WebView UI with 2D step grid, playhead, naturalist aesthetic
- **2026-02-04 (v1.0.0):** Stage 4 complete — 12 factory presets, pluginval Level 5 passed, auval passed
- **2026-02-04 (v1.1.0):** Added Clear and Random buttons per lane

## Concept Summary

A rhythmic gate that combines FFT spectral processing with step sequencing. Different rhythmic patterns for different frequency regions, with Euclidean rhythm generation per band.

**Key Features:**
- 4-band spectral processing with configurable crossovers
- Per-band step sequencer (4/8/16/32 steps)
- Euclidean rhythm generation per band
- Visual frequency × time grid
- Tempo-synced with swing control
- Clear/Random buttons per lane (v1.1.0)

**Technical Approach:**
- FFT-based (2048 samples, 4× overlap)
- Overlap-add reconstruction
- ~46ms latency (reported to DAW)

## Known Issues

None

## Additional Notes

### Design Decisions
- **Hybrid interaction model:** Discrete bands by default, paint mode planned for v1.2
- **Modular architecture:** Core v1.0 focused, expansion roadmap defined
- **All use cases supported:** Rhythmic gating, spectral animation, polyrhythmic layers

### Complexity Assessment
- **DSP:** High (FFT processing, multi-band, sequencing)
- **UI:** Medium-High (2D grid, real-time visualization)
- **Parameters:** ~165 total (many are step grid states)

### Future Versions
- v1.2: Paint mode for step grid, per-step attack/release
- v1.3: LFO modulation, envelope follower
- v2.0: Spectral freeze functionality
