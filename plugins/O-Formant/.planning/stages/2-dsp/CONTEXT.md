# Stage 2: DSP - Context

## Discussion Summary

**Date:** 2026-04-04
**Participants:** User, Claude

## Requirements Confirmed

- Phase 2.1 scope confirmed: LF glottal source + formant filter bank + vowel morpher + ADSR + aspiration noise (all 5 components together, since none produce useful output in isolation)
- Phase 2.2: Vibrato, pitch glide, MPE expression, consonant engine (must-have for v1, not deferrable)
- Phase 2.3: Stereo spread, output gain, optimization, parameter smoothing verification
- Consonant engine (KLATT noise branch, plosive burst, sibilance) is a hard requirement for v1 release

## Constraints Identified

- CPU usage target is relaxed -- "acceptable for a dedicated synth track" rather than strict <5% single core
- No latency constraints (zero latency architecture, no oversampling in v1)
- 32-bit float precision (per-voice DSP is lightweight)

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Glottal source method | **Mipmapped wavetable** (not direct LF + PolyBLEP) | Better runtime performance, inherent anti-aliasing via mipmap levels, eliminates Newton-Raphson convergence issues at extreme Rd values. Upfront table generation cost is acceptable. |
| Edge case handling | **Focus on musical sweet spot first** | Wavetable eliminates the main edge case concern (LF model convergence). Remaining edge case (formant filter Nyquist clamping) is straightforward and handled by existing clamping strategy in ARCHITECTURE.md. |
| Consonant engine priority | **Must-have for v1** | Core differentiator -- fricatives, plosives, and sibilants are essential to the vocal character. Not deferrable. |
| CPU performance target | **Relaxed** | No strict <5% target. Optimize where easy, but don't sacrifice quality or implementation clarity for CPU savings. |
| Sonic reference | **Highest quality and flexibility per research** | No single reference plugin -- target the best quality achievable by the source-filter model as documented in research (LF model fidelity, accurate formant data, KLATT consonant topology). |

## Architecture Deviation: Wavetable vs Direct LF

**IMPORTANT:** The ARCHITECTURE.md specifies "Direct LF time-domain computation with PolyBLEP" for v1 and wavetable as a v1.1 upgrade path. **User has chosen to go directly to wavetable for v1.**

### Implications

1. **LFGlottalSource implementation changes:**
   - Pre-compute LF waveforms across Rd range (e.g., 128 Rd steps x 2048 samples per table)
   - Mipmap levels for anti-aliasing at different pitches (replaces PolyBLEP)
   - Runtime: table lookup + interpolation (bilinear between Rd steps and mipmap levels)
   - Table generation: can run at plugin init or be baked into binary as constexpr data

2. **What this eliminates:**
   - No per-sample Newton-Raphson solvers for alpha/epsilon
   - No PolyBLEP/PolyBLAMP corrections
   - No runtime Fant 1995 regression (computed once during table generation)
   - No convergence issues at extreme Rd values

3. **What this adds:**
   - Wavetable generation code (offline or at init)
   - Mipmap construction (typically log2(tableSize) levels)
   - Bilinear interpolation between Rd steps and between mipmap levels
   - Memory: ~128 * 2048 * sizeof(float) * mipmapLevels per voice (shared, read-only)

4. **What remains unchanged:**
   - All other components (formant bank, vowel morpher, consonant engine, ADSR, vibrato, etc.)
   - Parameter ranges and defaults
   - Signal flow and routing
   - Formant filter Nyquist clamping still required

## Phase Breakdown (Confirmed)

### Phase 2.1: Core Vocal Engine
- LFGlottalSource with mipmapped wavetable (deviation from arch)
- AspirationNoise mixer (breathiness control)
- FormantBiquad struct + FormantFilterBank (5 parallel BPFs)
- VowelMorpher (Shepard interpolation, log-domain formant blending)
- VoiceEnvelope (juce::ADSR integration)
- **Exit criteria:** Playing MIDI notes produces vowel sounds, XY morph works, ADSR shapes amplitude

### Phase 2.2: Modulation and Expression
- VibratoLFO (sine, rate/depth/delay)
- PitchGlide (exponential portamento)
- MPE integration (pressure->breathiness, slide->vowelY, velocity->attack)
- ConsonantEngine (KLATT noise branch, tone control, sibilance, auto-consonant plosive burst)
- **Exit criteria:** Vibrato audible, glide works, MPE expression per-note, consonants shape attacks

### Phase 2.3: Output Stage and Polish
- StereoSpread (per-voice pan by pitch)
- OutputGain (juce::dsp::Gain with dB control)
- Parameter smoothing verification (no zipper noise)
- Performance optimization (skip consonant when level=0, early-out silent voices)
- Filter state reset on voice release
- **Exit criteria:** Stereo field, gain control, no artifacts, pluginval passes

## Open Questions

- Wavetable size and Rd resolution to be determined during research phase (128 Rd steps x 2048 samples is the starting estimate from ARCHITECTURE.md)
- Whether table generation happens at plugin init or is baked as constexpr data
- Exact mipmap interpolation strategy (linear vs cubic between table entries)

## Next Phase

Ready for: **research** phase (investigate wavetable LF implementation details, confirm formant data tables, research mipmap strategies for glottal wavetables)
