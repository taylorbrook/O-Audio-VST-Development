# O-GrainScatter Notes

## Status
- **Current Status:** Ideated
- **Version:** N/A
- **Type:** Audio Effect (Granular Stutter Engine)

## Lifecycle Timeline

- **2026-02-06:** Ideated — Creative brief and requirements created
- **2026-02-06:** Research reference: `research/stutter-effects/path-a-granular-stutter-engine.md`

## Heritage

Built on the granular engine from the Scatter plugin (TACHES):
- 64-voice grain pool, Lagrange3rd delay buffer, Hann window, scale quantization
- Extended with beat-sync, freeze, pitch ladder, Euclidean rhythms, texture morph

## Known Issues

None (not yet implemented)

## Additional Notes

### Unique Value Proposition
"Harmonic Stutter" — the only stutter effect combining 64-voice granular synthesis, musical scale quantization, beat-synchronized triggering, and density-based texture morphing.

### Key Parameters (~20 total)
- Core granular: Grain Size, Density, Pitch Random, Pan Random, Scale, Root, Reverse, Feedback, Dry/Wet
- Beat sync: Sync Mode, Probability, Repeats
- Texture/Pitch: Texture (stutter-to-cloud morph), Pitch Mode (Random/Ladder/Pendulum), Freeze
- Euclidean: Pulses, Steps

### Differentiation from O-Freeze
O-Freeze = spectral freeze (FFT-based, static textures)
O-GrainScatter = granular stutter (time-domain, rhythmic/evolving grain effects)
