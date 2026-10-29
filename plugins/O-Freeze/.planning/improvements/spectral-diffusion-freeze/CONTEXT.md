# Context: Spectral Diffusion Freeze

**Plugin:** O-Freeze
**Milestone:** spectral-diffusion-freeze
**Created:** 2026-10-28
**Phase:** Discuss

---

## Original Request

> spectral freeze mode with Diffusion β control: multi-frame magnitude average, per-bin random-walk drift, phase advance + β·U(−π,π), phase locking near β=0 (research §3.4)

Source: `research/granular-synthesis-2026-update.md` §3.4 and roadmap item #7 ("O-Freeze v3 candidate").

---

## Requirements Gathered

### Core Requirements

1. **Engine selector**
   - New `ENGINE` choice param: Granular / Spectral. Default Granular.
   - Acceptance: with Engine = Granular the render is bit-exact vs v2.8.0 (reuse `tests/blocksize-null` harness, same seeds).

2. **Spectral freeze engine (STFT, fixed 8192 / 4× overlap)**
   - Hann analysis × Hann synthesis, hop 2048, OLA normalised by 1/1.5.
   - DC and Nyquist bins kept real.
   - Acceptance: steady-state frozen output carries signal, no periodic hop-rate AM, no clicks at engage/release (existing 50 ms / 100 ms crossfade smoother still gates the wet).

3. **Multi-frame magnitude average at capture**
   - On engage, magnitudes averaged over N frames around the freeze point, N from new `AVG_FRAMES` param (range 1–16, default 8; 1 = single frame).
   - Frames come from the existing 2 s capture ring (8192 + 15·2048 ≈ 38.9 k samples ≤ ring at any rate up to 192 k).
   - Acceptance: N = 1 vs N = 8 measurably reduces spectral "peakiness"/metallic ringing (e.g. lower spectral-flatness variance of the frozen frame on a noisy source).

4. **Per-bin random-walk magnitude drift (reuses Drift)**
   - Drift % maps to per-bin independent random walks, ±0 → ±3 dB depth, 0.2–3 Hz rate. Drift 0 = static magnitudes.
   - Draws from `CounterRng` with its own slot(s); deterministic per seed and block-size independent (control on the sample clock, not per host block).

5. **Diffusion β phase advance**
   - New `DIFFUSION` param 0–100 % (β 0–1), default 0 %.
   - φ ← φ + ω̂·H + β·U(−π,π) per bin per hop. ω̂ = per-bin true instantaneous frequency estimated at capture (phase-vocoder freeze at β = 0, PaulStretch-style wash at β = 1).
   - Phase locking (Laroche & Dolson 1999, identity/scaled phase locking around spectral peaks) applied when β is near 0, faded out as β rises — no hard switch.
   - LFO (existing Rate/Depth/Shape) modulates Diffusion in Spectral mode.

6. **Detune in Spectral mode**
   - Detune maps to a per-bin pitch spread: each bin's phase increment scaled by a fixed random factor within ±Detune cents (seeded, drawn once per engage).

7. **UI**
   - Engine toggle, Diffusion knob, Avg Frames control. Grain Size / Grain Count / Reverse dim (disabled) in Spectral; Diffusion / Avg Frames dim in Granular.
   - EN / FR / zh-Hans labels and tooltips; check-i18n, fr/zh lint, check-ui-labels pass.

8. **Determinism**
   - Spectral renders are bit-exact across host block sizes (32 / 441 / 7 vs 512) and reproducible from the saved `rngSeed`.

### Scope Boundaries

**In scope:**
- ENGINE, DIFFUSION, AVG_FRAMES params + state/automation
- STFT freeze engine, RT-safe (all FFT buffers preallocated in prepareToPlay, `juce::dsp::FFT` real transform)
- Reuse of Drift / Detune / LFO / Mix / Freeze / Threshold / Mode in Spectral
- Gate extensions (block-size null, β sweep, determinism, alloc), i18n

**Out of scope:**
- User-selectable FFT size (fixed 8192)
- Latency reporting / PDC (wet-only latency, report 0 — same as today)
- Blending the two engines simultaneously
- Spectral time-stretch / scrub (freeze only)
- Sample-rate-scaled FFT size (fixed 8192 at all rates — research to confirm acceptable at 96/192 k)

---

## User Preferences

### Interaction Style
- [x] Real-time parameter response (Diffusion, Drift, Detune live during a freeze)
- [ ] Preset-based switching
- [x] Automation-friendly (Engine switch while frozen must not click — research to decide crossfade)

### Quality Priorities
1. Smooth, non-metallic sustain (the point of §3.4)
2. Existing granular behaviour untouched (bit-exact at Engine = Granular)
3. Deterministic / block-size-independent renders (house standard since v2.8.0)

### Breaking Changes
- [ ] OK to break existing presets
- [x] Must maintain preset compatibility (old sessions load as Granular, unchanged)
- [x] Must maintain automation compatibility (append params, no reorder)

---

## Plugin Context

### Current State
- **Version:** 2.8.0
- **Stage:** 4 complete (improve cycle)
- **Last change:** v2.8.0 seeded counter RNG, fractional onsets, per-sample threshold gate, 32-sample control tick; block-size null gate.

### Relevant Existing Features
- 2 s capture ring (`freezeBuffer`, preallocated for 192 k)
- Freeze crossfade smoother (50 ms in / 100 ms out)
- `CounterRng.h` slots (grains, engages, LFO cycles)
- Control tick every 32 samples (LFO, drift target)
- Threshold gate per sample

### Known Limitations
- Latency 0 reported; STFT wet adds ~W samples of delay (acceptable: capture-then-sustain).
- 8192 at 192 k = 43 ms window — thinner smoothing than at 48 k.
- 12 params today; three new ones appended.

---

## Questions Answered

**Q1: How does spectral freeze relate to the existing granular engine?**
> New Engine param (Granular / Spectral). Granular stays bit-exact as default.

**Q2: FFT size / latency policy?**
> Fixed 8192, 4× overlap.

**Q3: How should latency be reported?**
> Wet-only, no PDC (report 0, as now).

**Q4: Which controls does Spectral mode expose beyond Diffusion β?**
> Reuse Drift for the per-bin random walk; add an Average Frames param; reuse Detune (per-bin pitch spread) and LFO (modulates Diffusion).

---

## Success Criteria

1. [ ] Engine = Granular bit-exact vs v2.8.0 (block-size null harness, same seeds)
2. [ ] Spectral renders bit-exact across block sizes 32/441/7 vs 512, and reproducible from seed
3. [ ] β sweep: β = 0 locked-phase output is tonal/stable on a chord; β = 1 is diffuse (measurably lower inter-hop phase coherence); monotone in between
4. [ ] Avg Frames N = 8 measurably less metallic than N = 1 (objective metric chosen in research)
5. [ ] 0 allocations in processBlock with Spectral engaged
6. [ ] No click on engage / release / Engine switch while frozen
7. [ ] check-i18n, fr/zh lint, check-ui-labels PASS
8. [ ] Build succeeds without warnings; pluginval 10, auval PASS

---

## Next Phase

Research phase:
- STFT engine layout inside the per-sample loop (hop scheduling on the sample clock, block-size independence)
- Instantaneous-frequency estimation at capture; phase-locking variant + β crossfade law
- Random-walk parameterisation; CounterRng slot allocation
- Engine-switch crossfade; CPU at 8192/4× stereo
- Objective metrics for "metallic" and β coherence

---

*Generated by improve-milestone discuss phase*
