# O-GrainScatter Notes

## Status
- **Current Status:** 📦 Installed
- **Version:** 2.4.1
- **Type:** Audio Effect (Granular Stutter Engine)

## Lifecycle Timeline

- **2026-07-08 (v2.4.1):** Code-review resolution pass — fixed all 2 critical + 12 warning CODE_REVIEW.md findings. CR-01 dead Scan knob (missing relay/attachment), CR-02 RT realloc in reset(), WR-01 block-held spatial feedback, WR-02 swing dropping off-beats + Euclidean desync, WR-03 freeze-engage memcpy, WR-04 spawn cap, WR-05 feedback/LPF NaN guards, WR-06 control-rate SH trig, WR-07 block-size clamp, WR-08 distance-LPF smoothing, WR-09 bpm<=0 guard, WR-10/WR-11 skew-correct readouts + `getParameterDefaults` native fn, WR-12 CMake version 2.1.0→2.4.1
- **2026-03-09 (v2.4.0):** Added grain scan position (`scan_position` 0-100%) — sweeps base grain read position through delay/freeze buffer (0-2 seconds)
- **2026-03-08 (v2.2.0):** Added grain size randomization (`size_random`) and per-grain amplitude randomization (`amp_random`) — both 0-100% with UI knobs
- **2026-03-08 (v2.0.3):** Fixed zipper noise on feedback/dry-wet automation in spatial mode (SmoothedValue bypass)
- **2026-03-08 (v2.0.2):** Fixed critical stack buffer overflow in binaural decode + hoaBus sizing
- **2026-02-09 (v2.0.1):** Fixed density parameter exponential curve
- **2026-02-06:** Ideated — Creative brief and requirements created
- **2026-02-06:** Research reference: `research/stutter-effects/path-a-granular-stutter-engine.md`

## Heritage

Built on the granular engine from the Scatter plugin (TACHES):
- 64-voice grain pool, Lagrange3rd delay buffer, Hann window, scale quantization
- Extended with beat-sync, freeze, pitch ladder, Euclidean rhythms, texture morph

## Known Issues

- **Spatial-mode feedback is mono (by design, since v2.4.1).** The WR-01 fix derives the per-sample feedback signal from the HOA omni (W) channel — the mono grain sum available inside the main loop — rather than the post-decode binaural stereo pair (which was one block late). The recirculated signal is mono, but the grains re-spatialize it on the next pass, so the audible field stays spatial. A future enhancement could reconstruct a per-sample stereo feedback from the W/Y channels if stereo feedback character is desired.
- **Trajectory SH targets update at a 16-sample control rate (since v2.4.1, WR-06).** Imperceptible for grains ≥10 ms (≥441 samples at 44.1 kHz); the one-pole SH smoother fills the gaps. Trajectory position and Doppler remain per-sample.

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
