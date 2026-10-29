# Context: Signal-adaptive grain length in freeze (extrema density)

**Plugin:** O-GrainScatter
**Milestone:** adaptive-grain-length
**Created:** 2026-10-28
**Phase:** Discuss

---

## Original Request

> signal-adaptive grain length from extrema density (Nielsen DAFx26) for freeze mode (research #11)

Source: `research/granular-synthesis-2026-update.md` §1.1, roadmap #11 (effort M, payoff medium).
Paper: Nielsen, *Keyframe Time Stretching via Extrema Sampling*, DAFx26 (pp. 168–175); reference code Capicola (AGPL-3.0). Extracted text: see RESEARCH phase.

### What we take from the paper (and what we don't)

We take **only the control signal**: the time span of the next K local extrema ("keyframes") ahead of a read point is a per-sample measure of local information density. Sparse material such as a bass note has few, widely spaced extrema, so the span is long. Dense material such as a cymbal or a transient has tightly packed extrema, so the span is short. The paper uses that span L = n[m+K] − n[m] as the OLA splice length (eq. 12). We use it as the **grain length**.

Paper details carried over:
- **Derivative:** cubic B-spline derivative kernel (4-tap FIR, zero at Nyquist) rather than a finite difference, so noise-floor ripple doesn't produce spurious extrema (§2.1, eq. 3).
- **Threshold:** a candidate is kept only if |v − v_prev| > ε, measured against the last kept extremum, which gives hysteresis. ε = 0.001 (−60 dB) per §2.2.
- **Position:** sub-sample extremum location by reverse linear interpolation of the derivative (eq. 4–5).
- **Long spans:** sparse spans before a transient get very long, so cap the maximum (§3.7 limitation 3).

**Not** taken: sparse-domain reconstruction (Hermite/smoothstep resynthesis, with its THD and spectral-contrast loss), and the 3-playhead splice engine. Grains still read the existing band-limited MipSnapshot. The sound quality of the reader is unchanged; only each grain's length adapts.

---

## Requirements Gathered

### Core Requirements

1. **New parameter `grain_adapt` ("Adapt"), 0–100 %, default 0**
   - Morphs each frozen grain's length from fixed (Grain Size) to extrema-driven, interpolating in the log domain: `len = fixed^(1−a) · adaptive^a`.
   - It acts only on grains spawned while Freeze is engaged. Live grains are unaffected at any setting.
   - Acceptance: at Adapt 0, all renders (free, sync, spatial, freeze) are bit-identical to v3.4.0. A v3.4.0 session loads with Adapt 0.

2. **Snapshot extrema analysis at freeze engage**
   - Run the extrema analysis over the captured snapshot once at engage, per slot: B-spline derivative, ε = −60 dB, sub-sample positions. The result is a sorted array of extremum positions covering the captured loop, including the wrap.
   - Must be RT-safe: storage is pre-sized in `prepare()` and nothing allocates at engage. Engage cost is bounded and measured, roughly 2 s × 48 k samples × a few ops.
   - Acceptance: 0 allocations on engage and in processBlock (alloc gate). Engage cost is reported in µs at 48 k and 96 k.

3. **Adaptive length = Grain Size as the mean, K derived per snapshot**
   - At a grain's snapshot read point, the length is the span of the next K extrema (Nielsen's L). K is chosen per snapshot so that the **mean** adaptive length across the snapshot ≈ Grain Size. Bass or sparse regions get longer grains, dense or transient regions shorter ones.
   - Clamp to the Grain Size parameter range of 10–500 ms. The ceiling is the paper's max-splice mitigation.
   - Lookup per grain spawn costs O(log M) (binary search) and nothing per sample.
   - Acceptance: on a sparse-then-dense test snapshot (low sine → noise or clicks), adaptive grains on the sparse half are measurably longer than on the dense half. Mean length is within a stated tolerance of Grain Size. Moving the Grain Size knob scales the lengths proportionally.

4. **Grains per second unchanged; level stays flat**
   - The spawn rate (Density, Sync) does not change. Overlap varies with content.
   - Density compensation already uses the live grain count N (sample-accurate), so it should track the varying lengths. Verify this rather than assume it.
   - Acceptance: a dense noise cloud at Adapt 100 vs Adapt 0 differs by ≤ ~0.5 dB in level (tolerance to be set in research), measured with a stationary source and averaged, against an old-vs-old control.

5. **UI: Adapt knob beside Size (Core Engine)**
   - Tooltips in EN, FR (`reviewed: false`) and zh-Hans (`'mt'`), stating that it acts only in Freeze.
   - Layout follows the v3.4.0 lower-panel budget. The 900 × 800 frame is unchanged. check-i18n, the fr/zh lint and check-ui-labels must pass.

### Defaults decided without asking (override in research/plan if wrong)

- **Size Random** stacks multiplicatively on top of the adapted length. It uses the same RNG slot and draw, so the RNG stream is unchanged.
- **Freeze scatter:** the ±1-grain-length minimum spread is based on the **adapted** length of the grain, so the anti-comb rule holds per grain. Check whether the read point that decides the length depends on the scatter, which would make the logic circular. Pick an order (length from the un-scattered scan point, then scatter by that length) and document it.
- **Shape Tilt** `tiltPeakFor()` is computed from the adapted length, since it already takes the per-grain length.
- **Reverse grains:** measure the K-extrema span over the region the grain actually reads, which is backwards from its start.
- **Pitch:** the span is measured in snapshot samples and converted to output length by dividing by the playback rate. Alternatively keep the length in output samples. Research should decide which matches the "grain covers K extrema of source" intent at ±36 st without breaking the read-head pre-roll guard.
- **ε fixed at −60 dB**, not exposed.
- Adapt is **latched at spawn** like Tilt, so automation never reshapes a playing grain.
- **New RNG draws:** none. Adapt is deterministic given the snapshot and read point, so the block-size null must stay bit-exact.

### Scope Boundaries

**In scope:**
- Extrema analysis over freeze snapshots, adaptive per-grain length for frozen grains, the `grain_adapt` parameter, UI knob plus tooltips (3 languages), and a new test gate.

**Out of scope:**
- Live-path adaptation, meaning analysis on the ring write path (possible follow-up).
- Sparse-domain (keyframe) reconstruction or the Nielsen time-stretch engine.
- Exposing ε or K directly.
- Promoting the analysis into the `grain-engine` module (consider after this ships; note it in research).
- Adaptive spawn rate or constant-overlap mode.

---

## User Preferences

### Interaction Style
- [x] Real-time parameter response (latched per grain)
- [ ] Preset-based switching
- [x] Automation-friendly

### Quality Priorities
1. No regression. Adapt 0 is bit-exact vs v3.4.0, and the RT-safety, alloc and block-size gates hold.
2. Audible, content-following length variation on real material (the point of the feature).
3. Level stability across Adapt settings.

### Breaking Changes
- [ ] OK to break existing presets
- [x] Must maintain preset compatibility (new param only, default 0)
- [x] Must maintain automation compatibility (AU parameter version hint 5)

---

## Plugin Context

### Current State
- **Version:** 3.4.0 (Grain Skip + Shape Tilt, 2026-10-28)
- **Last change:** skip-tilt gate 31/31. Before that, v3.3.0 band-limited reader (MipSnapshot freeze slots).

### Relevant Existing Features
- `FreezeManager.h`: two slots, each a grain-engine `MipSnapshot`. Capture ≤ 2 s, ending `margin(3)` = 506 samples before the write head. 10 ms equal-power seam crossfade. The virtual head keeps advancing, and `getSpawnBase(delay)` gives the grain's snapshot base.
- Grain spawn, `PluginProcessor.cpp` ~786–870: size → `tiltPeakFor`, scatter (frozen: ≥ 1 grain length), read-head pre-roll guard (`len·(rate−1) + margin`), frozen base, ±6 ct freeze detune.
- Density compensation (`nextDensityGain`, live N, μ₁/μ₂, ρ): per-sample live count.
- CounterRng slots are keyed per grain. Deterministic.

### Known Limitations / Risks
- Engage happens on the audio thread, so the analysis pass must fit a block budget or be spread over several blocks. Research should measure this and choose.
- Silence in the snapshot gives no extrema, so the span is unbounded and gets clamped to 500 ms. Decide behaviour for a fully silent snapshot (fall back to fixed length).
- The pre-roll guard uses the grain length. Longer adapted grains at high rate push the read point further back, so check this against snapshot length and wrap.
- Paper §3.7: the adaptation is strongest on percussive or layered material and subtle on pads. Payoff is "medium" per the roadmap.

---

## Questions Answered

| Question | Answer |
|---|---|
| Control | Amount knob `Adapt` 0–100 %, default 0, beside Size, freeze only |
| Grain Size meaning | Mean length. K derived per snapshot so mean ≈ Grain Size. Clamp 10–500 ms |
| Scope | Freeze only. Analyse snapshot at engage, O(log M) lookup per spawn |
| Density | Grains/sec unchanged. Compensation tracks live N. Verify level flat |
