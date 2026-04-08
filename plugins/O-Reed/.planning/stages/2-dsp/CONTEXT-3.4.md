# Stage 2: DSP Phase 3.4 - Context

## Discussion Summary

**Date:** 2026-04-05
**Participants:** User, Claude

## Requirements Confirmed

- Infinite sustain: reduce bore loop losses (viscothermal + bell reflection) toward zero. Breath envelope still releases naturally — only bore resonant energy persists indefinitely at 100%.
- Reverse bore: cross-fade per-segment conical scale factors from normal to inverted direction. REVERSE_BORE (0-1) flips taper so bore narrows toward bell (hichiriki-like). Only audible when BORE_CHARACTER > 0 (cylindrical is symmetric).
- Dual bore: second BoreWaveguide instance per voice, sharing same reed excitation. Reed "sees" combined impedance from weighted mix of both bores' backward waves. No artificial voice/poly limit — user decides CPU tradeoff.
- Drone pitch: **cents not semitones**. Range -2400 to +2400 cents. Existing APVTS parameter `dronePitch` must be updated from (-24, 24, 0.1 semitones) to (-2400, 2400, 1 cent). Second bore frequency = primary * 2^(dronePitch/1200). At 0: unison chorusing. At -1200: octave below. At -700: fifth below (arghul-like).
- Feedback path: cross-couples bore outputs. At 0: bores independent (mixed at output only). Higher values inject energy from bore A's forward wave into bore B's backward path and vice versa. Must remain stable at extreme values.
- Bore profile: multi-segment is higher quality, use that. BORE_PROFILE choice reshapes how BORE_CHARACTER distributes across segments — throat wider, body moderate, bell flaring — using preset taper ratios on the 5 existing segments.
- Drone bore shares primary bore's tone hole settings (no independent tone hole state).

## Constraints Identified

- Parameter change: `dronePitch` range must change from semitones to cents. This affects APVTS definition, UI relay, and any automation. Must be handled carefully (parameter version bump if needed).
- CPU budget: dual bore ~doubles bore cost per voice. Mono dual bore < 40% single core. 8-voice poly + dual bore will be heavy (~32%) but acceptable — no artificial limit.
- Stability: infinite sustain (lossless bore) + feedback path + dual bore is the most extreme combination. Must not produce runaway oscillation or NaN. Safety clip (tanh) at output is last resort; prefer clamping feedback energy.
- All 6 new features must bypass at default values to preserve Phase 3.3 regression safety.

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Infinite sustain mechanism | Interpolate visc gain toward 1.0 + reduce bell loss | Bore loss only — breath releases naturally, resonant tube energy persists |
| Reverse bore mechanism | Cross-fade scale factor array normal→inverted | Leverages existing per-segment Strategy C conical scaling |
| Dual bore reed coupling | Both bores receive same p_bore_plus, backward waves weighted-mixed | Reed sees combined impedance — physically motivated (arghul/launeddas) |
| Drone pitch unit | Cents (-2400 to +2400) | Finer control than semitones for micro-tuning drone intervals |
| Drone bore tone holes | Shared with primary bore | Simpler, one parameter set controls both |
| Bore profile quality | Multi-segment (higher quality) | Different taper rates per section (throat/body/bell) |
| Dual bore voice limit | None — user decides | No artificial poly restriction for dual bore |
| Feedback path stability | Clamp feedback energy + existing tanh/NaN guards | Prevent runaway before it hits output |

## Implementation Notes

### Infinite Sustain
- Current visc filter: `g = 0.995f` in `updateParams()`
- INFINITE_SUSTAIN (0-1): `g_eff = 0.995f + infiniteSustain * 0.005f` (approaches 1.0)
- Bell reflection loss also reduced: scale bell filter coefficient toward total reflection
- At 100%: bore loop gain ≈ 1.0, energy preserved indefinitely

### Reverse Bore
- Current: `r_at_seg = r_in + segCenters[i] * L * tan(halfAngle)` — radius increases reed→bell
- Reverse: `r_at_seg = r_in + (1.0 - segCenters[i]) * L * tan(halfAngle)` — radius increases bell→reed
- REVERSE_BORE (0-1) interpolates segment center positions between normal and reversed
- Only affects conical scaling (cylindrical bore has all scale factors = 1.0)

### Dual Bore
- New member: `BoreWaveguide bore2` in ReedWindVoice
- `bore2.prepare()` called alongside `bore.prepare()`
- Frequency: `bore2.setFrequency(primaryHz * pow(2, dronePitchCents / 1200.0f))`
- Output: `bore.getRadiatedOutput() + bore2.getRadiatedOutput()` (mixed before normalization)
- Backward wave: `prevBoreMinus = bore1_bwd * (1-feedback) + bore2_bwd * feedback` (and vice versa)
- When DUAL_BORE=off: skip bore2 processing entirely (zero CPU)

### Feedback Path
- FEEDBACK_PATH (0-1) controls cross-coupling strength
- Each bore's backward wave gets mixed with a fraction of the other bore's backward wave
- Low values: subtle chorusing/beating from coupled resonances
- High values: complex interaction — must ensure total energy doesn't grow unboundedly
- Safety: scale feedback by a maximum gain factor (<1.0) to prevent runaway

### Bore Profile (Multi-Segment)
- Simple (0): uniform halfAngle across all 5 segments (current behavior)
- Multi-segment (1): per-section taper ratios applied to 5 segments
  - Segments 0-1 (throat): narrower taper (e.g., 0.3× halfAngle)
  - Segments 2-3 (body): moderate taper (e.g., 1.0× halfAngle)
  - Segment 4 (bell): wider flare (e.g., 2.0× halfAngle)
- Ratios are preset constants, not user-adjustable — BORE_CHARACTER still controls overall magnitude

### Parameter Change Required
- `dronePitch` APVTS: change range from (-24, 24, 0.1) to (-2400, 2400, 1)
- Change label from "semitones" to "cents"
- Update PluginEditor relay if applicable
- Bump parameter version if needed for DAW project compatibility

## Cumulative Parameter Count

Phase 3.4 activates 6 new parameters (24 → 30 active):
- INFINITE_SUSTAIN, REVERSE_BORE, DUAL_BORE, DRONE_PITCH, FEEDBACK_PATH, BORE_PROFILE

## Open Questions

- None — all decisions resolved in discussion.

## Next Phase

Ready for: research phase
