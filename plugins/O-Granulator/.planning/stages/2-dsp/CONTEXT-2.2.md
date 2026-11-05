# Stage 2 — DSP · Phase 2.2 (Four streams, randomisation, pitch/scale, window morph, normalisation) — Context

**Plugin:** O-Granulator
**Stage:** 2 of 4 (DSP)
**Phase:** 2.2 of 6
**GSD phase:** discuss
**Date:** 2026-11-04
**Branch:** `main` (trunk-based) @ `6508c635`
**Participants:** Taylor Brook, Claude

---

## Entry Check — contract checksums

All six re-hashed this session. Each one matches STATUS.md frontmatter byte for byte. REQUIREMENTS.md is the 2.1-verify revision `c74b5ebc…`.

| Contract | SHA-256 (prefix) | Result |
|---|---|---|
| `BRIEF.md` | `f9c748a8…` | ✅ |
| `REQUIREMENTS.md` | `c74b5ebc…` | ✅ |
| `parameter-spec-draft.md` | `d0b1457e…` | ✅ |
| `parameter-spec.md` | `f5ce3595…` | ✅ |
| `research/ARCHITECTURE.md` | `f0cf753b…` | ✅ |
| `ROADMAP.md` | `79d69576…` | ✅ |

---

## Discussion Summary

ARCHITECTURE §4–§8, §10 and §11 specify 2.2. Discuss reopened only one point: the scale-quantiser rule, where ARCHITECTURE §5 (Algorithm Details) conflicts with the LOCKED parameter spec. Discuss also settled one gap the architecture leaves open, the law for `sN_gain_rand`. Execution defaults for the rest are recorded below. D1–D3 from CONTEXT-2.1 still hold: grain-engine 1.1.0 is consumed unmodified, G1–G3 stay plugin-local, the ring is 41 s at every rate, and 2.2 has no human listening checkpoint.

---

## Approach Decisions (locked this session)

| # | Decision | Choice | Rationale |
|---|---|---|---|
| D4 | Scale quantiser rule | **GrainScatter 3.x parity.** With `sN_scale` ≠ Off, the spawner picks **uniformly among the active degrees** (collection degrees tiled across periods, filtered by `degreeMasks[s][scale]`) that lie inside `±pitch_rand` st. The pick is added to the **unquantised** base pitch: `st = sN_pitch + degreeCents/100 (+ mod)`. Then the result is clamped to ±24 st. The base `sN_pitch` is **never** snapped. If the mask is empty, or no active degree lies inside the span, the offset is 0 (base pitch). With `pitch_rand` = 0, the offset is 0. With Off, the offset is the continuous `pitch_rand·(2u−1)`. | Matches the locked spec tooltips ("Quantises Pitch Random … counted from unison"; "Empty mask … grains stay at base Pitch") and the sibling plugin. **Erratum against ARCHITECTURE Algorithm Details §5:** its interval-mode `q = quantise(st)` nearest-degree snap is superseded. Absolute mode (pitch follow + `scale_root`) is 2.5 and should use the same pick-from-active-set rule, with the set rooted at `scale_root`. |
| D5 | `sN_gain_rand` law | **Cut-only, in dB, with no make-up gain.** `gainDb = −gain_rand · u · 24 dB`, so 100 % reaches −24 dB at most. It never boosts. The draw is consumed only when `gain_rand > 0`. The value is latched at spawn and multiplied into the grain gain. The output law **ignores** it: N_eff and g_out do not change, so mean level drops as the knob rises. This is intended. | Peak, cap and feedback headroom stay unchanged. Every level-flatness gate runs at `gain_rand` 0. |

---

## Requirements Confirmed (Phase 2.2 scope — ROADMAP § Phase 2.2)

**Components:**
- 4 StreamSchedulers with an (offset, stream) onset merge. Remove the 2.1 `s2–s4` force-off mask; 2.1 sized everything `[4]` already.
- Stream-fair stealing: `progress(g) = 2·overQuota[g.stream] + (g.stream == spawning) + age·invLen`, with **parameter-derived** water-filled quotas over `grain_limit` (demand `N_s = density_s·size_s`). Never derive quotas from occupancy.
- WindowMorphBank (plugin-local, gap G1): 65 tables × 2048, with half-window μ₁ and μ₂ per table. The skew warp follows WindowLut's tilt (peak at 0.5 + 0.45·skew). Morph step and skew are latched at spawn.
- The scale quantiser (D4) over the pitch-collections 1.0.0 tables, with index mapping per the spec's `sN_scale` list (0 = Off; Custom = 31 reads the shared custom table).
- Stereo spread: a per-grain random constant-power pan around the stream pan. At spread 0 no draw is made and the result is deterministic.
- Gain rand (D5).
- Output law: incoherent `g_out` from N_eff (cap-aware) and the μ₂ ratio, in the **single-normalisation** `M_m(x)` form from `OutputLaw.h` (ARCHITECTURE §10 erratum; worst case P/μ₂ = 43.02). It is κ-blended toward `getForwardNorm` by `κ(spray, pitchRand, jitter)`. Break-points may be retuned by measurement.
- Stream mixer: level, pan (balance), mute and solo (additive), each with a 10 ms ramp. No parameter writes come from listeners. Muted or non-soloed streams keep spawning and **keep their pool quota**, so unmuting is instant (as the spec says).

**Test criteria (gate):**
- [ ] **First, before any spawner or pool change:** an isolated steal-free bit-identity row. Set `grain_limit` 128 with N < 128 and compare against the native pool path. This is the gap carried from VERIFICATION-2.1.
- [ ] Each stream is audible on its own, and disabled streams spawn nothing: zero draws and zero onsets (FUNC-03).
- [ ] Total grains never exceed the cap, and a dense stream cannot starve the others (FUNC-03, PERF-02).
- [ ] Quantised grains land on active degrees within 1 cent across all 31 collections, incl. Bohlen-Pierce (tritave period) and Custom. Every pick is a member of the active set within ±span. An empty mask gives the base pitch exactly (DSP-01).
- [ ] Window morph RMS stays within ±1 dB across morph × skew (DSP-03). Level stays flat within ±1.5 dB across density 1→200, using averaged stationary-noise renders plus an old-vs-old control row, on both noise and sine sources (κ blend). This also confirms that the 2.1 +7 dB hot spot at spray/jitter/pitch 0 is gone.
- [ ] reverseProb is exact at 0 % and 100 %, and the 50 % row falls within binomial bounds. Spray 0 and spread 0 are deterministic (DSP-04).
- [ ] Gain rand: at 0 it is bit-identical to a no-draw render; at 100 % every grain gain lies in [−24 dB, 0 dB].
- [ ] Steal click gate: tonal source, ~0.001 threshold, with pool overflow verified as non-vacuous (QUAL-01).
- [ ] Mute/solo/level/pan ramps cause no step. Run them against the same-motion reference.
- [ ] Block-size null stays bit-identical with 4 streams and all randomisation on, seeded, plus a different-seed control (PERF-03).
- [ ] Allocation gate re-run with 4 streams, plus scale and mask changes and a custom-table swap (PERF-01).

---

## Carry-ins (now due)

| Item | Handling |
|---|---|
| Sort the active pitch table (pitch-collections `JustSevenLimit` unsorted at source) | Sort the **active** table at rebuild time, inside the plugin. The module stays untouched (D1 spirit). Gate: 7-Limit Just picks are monotone by index. |
| Audio thread owns its custom-table copy | The message thread parses `customScale.text`, then publishes the result through a double buffer and a `customVersion` atomic. The audio thread copies it at the grid when the version changes. No locks, no allocation. |
| Steal-free bit-identity row | First gate row (above). |
| §10 single-normalisation output law | Use `OutputLaw.h`'s `M_m(x)` form; do not re-derive it from ARCHITECTURE. |
| Unity gain is never derived from `get()` on skewed ranges | Still applies to level, pan and the new mixer ramps. |

## Execution Defaults (Claude's call, recorded for plan)

- **Active-degree table per stream:** a fixed array (≤ 512, the GS bound) rebuilt on the audio thread only when (scale, mask, pitch_rand span, customVersion) changes, as in GS `rebuildIfNeeded`. It is sorted, has no allocation, and is clamped to ±24 st minus the base pitch.
- **Draw slots:** pitch, spread and gain each get a fixed CounterRng slot. Each draw happens only when its control is > 0, and always **before obtain** (refused or stolen spawns consume identical draws).
- **Pitch-rand ±span with scale on:** the span is symmetric, `[−pitch_rand, +pitch_rand]` st (no Direction param in Granulator).
- **κ blend:** start from the ARCHITECTURE break-points (0.25·len, 0.5 st, jitter). Research and plan may retune by measurement against the ±1.5 dB gate.
- **WindowMorphBank:** built in the constructor (≈ 0.5 MB, message thread), in `Source/dsp/WindowMorphBank.h`. It is a G1 promotion candidate after ship.
- **Mute/solo audibility mask** is evaluated at the control grid. A 10 ms linear gain ramp runs per stream on the stream bus.
- `scale_root` and `pitch_follow_sN` stay unused until 2.5.

## Constraints Identified

- Quotas, gains and κ come only from latched parameters at the grid, never from `liveCount` or `liveWeight()` (block-size leak).
- Ties in onset order are broken by (offset, stream index). The merge must be identical at every host block size.
- A disabled stream makes no scheduler calls and no draws. Enabling it resets the clock so the first onset fires on the next sample.
- The custom-mask reset when the custom degree list changes happens on the message thread, never on the audio thread.
- Every per-pass array stays fixed at 32 samples.

## Open Questions (for research)

- Water-filling form: confirm that the closed-form water-fill over 4 demands, with disabled streams at demand 0, is continuous in the knobs, so quota changes don't step g_out. (Muted and non-soloed streams keep their demand, as set in the mixer item above.)
- κ break-points: measure the coherent/incoherent crossover on sine vs noise at 4 overlaps before retuning.
- Spread × stream pan: does spread compose as `pan_grain = clamp(pan_s + spread·(2u−1))`, or by rotation, so that a hard-panned stream at 100 % spread doesn't pile up at the edge? Pick a composition that keeps constant power.
- Per-stream N_eff under a shared cap: confirm `quota_s` (not `grain_limit`) feeds `M_m` once 4 streams compete.

## Next Phase

Ready for: research (`/plugin-research O-Granulator 2-dsp`, Phase 2.2).
