# O-FreqPulse Notes

## Status
- **Current Status:** 💡 Ideated
- **Version:** N/A
- **Type:** Audio Effect (Spectral Sequencer)

## Lifecycle Timeline

- **2026-02-03:** Ideation complete — creative brief and requirements documented

## Concept Summary

A rhythmic gate that combines FFT spectral processing with step sequencing. Different rhythmic patterns for different frequency regions, with Euclidean rhythm generation per band.

**Key Features:**
- 4-band spectral processing with configurable crossovers
- Per-band step sequencer (4/8/16/32 steps)
- Euclidean rhythm generation per band
- Visual frequency × time grid
- Tempo-synced with swing control

**Technical Approach:**
- FFT-based (2048 samples, 4× overlap)
- Overlap-add reconstruction
- ~46ms latency (reported to DAW)

## Known Issues

None (not yet implemented)

## Additional Notes

### Design Decisions
- **Hybrid interaction model:** Discrete bands by default, paint mode planned for v1.1
- **Modular architecture:** Core v1.0 focused, expansion roadmap defined
- **All use cases supported:** Rhythmic gating, spectral animation, polyrhythmic layers

### Complexity Assessment
- **DSP:** High (FFT processing, multi-band, sequencing)
- **UI:** Medium-High (2D grid, real-time visualization)
- **Parameters:** ~165 total (many are step grid states)

### Next Steps
Run `/plan` to generate DSP architecture and implementation roadmap.
