# Stage 2 — DSP · Phase 2.4 (Freeze, feedback network, output reverb) — Context

**Plugin:** O-Granulator
**Stage:** 2 of 4 (DSP)
**Phase:** 2.4 of 6
**GSD phase:** discuss
**Date:** 2026-11-05
**Branch:** `main` (trunk-based) @ `6aed8017`
**Participants:** Taylor Brook, Claude

---

## Entry Check — contract checksums

All six were re-hashed this session. Each one matches the STATUS.md frontmatter. REQUIREMENTS.md is at the 2.3-verify revision `2f00a3c7…`.

| Contract | SHA-256 (prefix) | Result |
|---|---|---|
| `BRIEF.md` | `f9c748a8…` | ✅ |
| `REQUIREMENTS.md` | `2f00a3c7…` | ✅ |
| `parameter-spec-draft.md` | `d0b1457e…` | ✅ |
| `parameter-spec.md` | `f5ce3595…` | ✅ |
| `research/ARCHITECTURE.md` | `f0cf753b…` | ✅ |
| `ROADMAP.md` | `79d69576…` | ✅ |

---

## Discussion Summary

ARCHITECTURE already specifies most of 2.4:
- the head-stop freeze and its four anti-buzz/anti-silence mitigations (Algorithm §3)
- the Write Arbiter (§3)
- the feedback chain order and four-layer runaway guard (§12, Algorithm §4)
- the coherent tap law with the Σlevel normaliser (§10, §11)
- the ReverbUnit placement (§13)
- the per-pass order (Processing Chain, steps 3, 6–8)

Discuss reopened four points:
- the wet-safety form, which would otherwise break the neutral null and move every carried golden
- the R19 resonant headroom carried from 2.3
- the ReverbUnit G4 binding route
- the loop saturation law

**Scope note.** `Engine.h` comments say "reverb, wet safety — 2.6". ROADMAP is the contract and puts both in **2.4**. Update the comment while this code is open.

**2.4 is the second (and last) Stage-2 human listening checkpoint** (D3, CONTEXT-2.1).

---

## Approach Decisions (locked this session)

| # | Decision | Choice | Rationale |
|---|---|---|---|
| D9 | Wet safety form | **Knee soft-clip, identity below −6 dBFS.** Per sample on the wet bus, with T = 0.501187 (−6 dBFS) and A = 1.9953 (+6 dBFS): \|x\| ≤ T → `y = x` (bit-exact); \|x\| > T → `y = sign(x)·(T + (A−T)·tanh((\|x\|−T)/(A−T)))`. The curve is C¹ at the knee (slope 1), and the asymptote is +6.0 dBFS, so DSP-06's peak bound still holds. Position is unchanged: ReverbUnit → wet safety → dry/wet. | ARCH's full-range `1.9953·tanh(x/1.9953)` alters every wet sample (−0.2 dB at −6 dBFS). That would break the neutral null against the 2.3 tree, re-golden all 175 carried rows and eat the fragile 0.017 dB W2o margin. The knee keeps normal-level material bitwise untouched. **Erratum vs ARCHITECTURE §14.** |
| D10 | R19 resonant LP/HP headroom | **Half make-down, latched per grain.** LP and HP are multiplied by `m = 1/√P(k)`, where P(k) is the analytic peak magnitude (linear) of that mode at the latched k. Result at res 100 %: about +10 dB instead of +20. **m is exactly 1.0f wherever P(k) ≤ 1** (the no-peak region, which includes res 0/20 %), so non-resonant renders stay bit-identical. Compute m at spawn (no per-sample `sqrt`). BP keeps `k·v1` (D8). Off is unchanged. | Some resonance boost remains, which is the synth-filter character. The extreme corner drops by 10 dB before D9 has to catch it (+38 → about +28 dBFS pre-safety worst case). **Amends D8 (LP/HP no longer raw).** Any 2.3 row with LP/HP at res in the peaking region re-baselines. The D8 BP law gate and every non-resonant row must stay identical. |
| D11 | ReverbUnit binding (G4) | **reverb-unit 1.2.0: add a non-breaking `bind(std::atomic<float>* enabled, size, damp, predelay, mix, mod, shimmer)` overload.** The existing APVTS `bind()` is unchanged, so no other consumer moves. The plugin owns these atomics and writes modulated values into them at control ticks (2.5 adds the modulation; 2.4 writes the base values). The module goes through `/module-create`-style versioning: bump `module.yaml`, add a CHANGELOG entry, record O-Granulator as a consumer at 1.2.0. | The module stays the single source and no fork is added (FORK-REVIEW drift). This is a non-breaking addition, so existing consumers don't need a rebuild. |
| D12 | Loop saturation law | **Keep ARCH:** `tanh(d·x)/d`, `d = 1 + 7·sat`. The guard is always present (d = 1 at sat 0). | The small-signal loop gain is exactly 1, which is the stability guarantee (Algorithm §4 layer 1). The ceiling falls from 0 dB to −18 dB at sat 100 %: louder content is compressed harder, and that is the runaway guard. This is the proven O-ReverseDelay topology. Judge it at the listening checkpoint. Fallback if it reads as "saturation = quieter": `d = 1 + 3·sat`. That needs no other change, because AGC + D9 still bound the loop. |

Unchanged from earlier phases: D1–D3 (grain-engine 1.1.0 unmodified, G2/G3 local workarounds, listening only at 2.1 and 2.4), D4–D8 except as D10 amends D8.

---

## Requirements Confirmed (Phase 2.4 scope — ROADMAP § Phase 2.4)

**Goal:** FUNC-02, FUNC-05, DSP-06, DSP-07. This phase also advances QUAL-01 (freeze toggle), QUAL-02 (max feedback) and PERF-01 (freeze).

**Components:**

- **Write Arbiter (ARCH §3, Algorithm §3)**
  - Live: push `inGain·in[n] + fbDelay[n mod 32]`.
  - Frozen: **no push** (head-stop).
  - Rising edge: latch `heldSpan = min(bufferLength, writeCount − margin(2) − 1)`.
  - Falling edge: a 10 ms raised-cosine **content** crossfade, `w·live + (1−w)·ring.levelSample(0, ch, writeCount − heldSpan)`. Never a gain ramp on the input.
  - Freeze edges are sampled at control-grid ticks (absolute 32-sample grid), so they are block-size invariant.
- **Freeze anti-buzz/anti-silence (all required):**
  1. Effective spray `max(spray, kFreezeMinSpray)`, where kFreezeMinSpray = ±½ grain length in source samples.
  2. The heldSpan latch (no silent region).
  3. A 5 ms forced release on engage, for live grains whose remaining reach would cross the stopped head. This uses the 2.2 steal-fade path, which is the local G2 workaround.
  4. The 10 ms content crossfade on release.
  - While frozen, the position base is the stopped head. The spawner reach clamp must hold against the frozen writeCount.
- **Feedback tap (ARCH §10, §11):** `Σ_s level_s·g_fb_s·bus_s / max(1, Σ_s level_s)` over audible streams. It is taken pre-g_out (×g_fb/g_out on the bus), and normalised **before** the tap. Laws are computed from latched params at the control grid, never from `pool.liveWeight()`.
- **Feedback chain (ARCH §12), in order:**
  1. `×0.95·feedback` (linear)
  2. TPT HP 12 dB/oct (`fb_low_cut`)
  3. TPT LP 12 dB/oct (`fb_high_cut`, clamped to ≤ 0.45·fs)
  4. 4-allpass Schroeder diffuser (4.7/8.3/13.9/21.7 ms, coeff 0.7) as a **wet/dry mix** (`fb_diffusion`), never a coefficient scale
  5. D12 `tanh(d·x)/d`
  6. Loop AGC: peak env 1 ms / 400 ms, `gain = min(1, 0.7/env)`
  7. NaN scrub: a non-finite value resets the chain and writes 0
  8. z⁻³² (`fbDelay`)

  Diffuser delay lines are sized at prepare from fs (allocated on the message thread). The chain is **computed even while frozen**, and its return is discarded.
- **Output reverb (ARCH §13, D11):**
  - ReverbUnit 1.2.0 with plugin-owned atomics.
  - Fixed internals: predelay 20 ms, mod 0.2, shimmer 0, enabled 1.
  - `reverb_mix/size/damp` map to mix/size/damp.
  - It runs once per pass (≤ 32 samples) over a `setDataToReferTo` buffer (no allocation). SmoothedValue targets update only at pass boundaries on the absolute grid.
- **Wet safety (D9)** after the reverb, before the equal-power dry/wet.
- **R19 make-down (D10)** in `GrainFx.h` LP/HP, latched at spawn.

**Test criteria (gate):**

- [ ] **Neutral null first:** feedback 0, freeze off, reverb_mix 0, wet below the −6 dBFS knee. Renders must be bit-identical to the 2.3 tree (out-of-tree, N0/R12 style, vs `6aed8017`). This must hold before any 2.4 row counts. Rows with LP/HP resonance in the peaking region are excluded and re-baselined under D10. Negative controls: feedback 1 %, reverb_mix 1 %, freeze toggle, and a hot wet above the knee must each DIFFER.
- [ ] **FUNC-02:** frozen level-0 ring hash identical across 10 s of new input. Freeze on/off click gate vs a same-motion control (QUAL-01). Frozen crest factor at spray 0 ≤ 1.5× live. Freeze 0.5 s after load is not silent. 60 s frozen drift < 0.5 dB.
- [ ] **FUNC-05:** feedback > 0 recirculates. Tone (HP and LP), saturation and diffusion **each** change the loop output (each one vs a same-settings control). Freeze + feedback: the return is discarded and the chain stays warm. Gate this by showing the chain state is non-zero and evolving while frozen, and that the ring stays untouched.
- [ ] **DSP-06 / QUAL-02:** 60 s at feedback 100 %, density 200, size 500 ms, pitch +12, 4 streams at +6 dB, res 100 %. Peak ≤ +6 dBFS, no NaN/Inf/denormal.
- [ ] **Loop decay:** dB/generation < 0 with input removed, at every setting corner, crediting the 48.6 ms diffuser group delay, measured in the linear regime at −52 dBFS (memory: diffusion slows dB/s, not dB/generation).
- [ ] **D9 law rows:** identity below the knee, exactly (`y == x`); C¹ at T; asymptote < A; slope ≤ 1 everywhere.
- [ ] **D10 law rows:** m == 1.0f for every res where P(k) ≤ 1. LP/HP peak at res 100 % = +10 dB ± 0.1. BP is unchanged (the D8 gate still passes).
- [ ] **DSP-07:** reverb is audible (wet energy and tail present at reverb_mix > 0). The tail fades out on mix → 0 with no step (50 ms ramp), and is not cut.
- [ ] **PERF-03:** block-size null 32/441/512/4096/variable with feedback 80 % + freeze toggles + reverb at shared boundaries, every randomisation ON, seeded, plus a different-seed control.
- [ ] **PERF-01:** allocation gate over freeze toggles, feedback on/off, reverb mix changes.
- [ ] **CPU (INFO, the budget gate is in 2.6):** report C4 = C3w + feedback 100 % + diffusion 100 % + reverb on against the 30 % budget.

---

## Listening Checkpoint (human, end of 2.4 execute or at verify)

Generic editor, **monitors down** (resonant corners). Listen for:
1. Freeze at spray 0: buzz vs acceptable texture (the head-stop vs virtual-head fallback decision, ARCH Algorithm §3).
2. Feedback character at 50/80/100 %, sweeping saturation (D12: does more saturation read as "quieter"? If so, use the `1+3·sat` fallback).
3. Diffusion and tone in the loop.
4. Reverb size/damp range.
5. **R22:** crush 100 % silences grains below −12 dBFS. Is that acceptable at typical −18 dBFS live levels?
6. D10 resonance at res 100 %: still characterful, not harsh.

---

## Carry-ins

| Item | Handling |
|---|---|
| R19 (2.3) | Resolved by D10 + D9. |
| R22 (2.3) | Listening checkpoint item 5. D7 is unchanged unless the ear says otherwise. |
| B2 "defaults" processor unseeded (VERIFICATION-2.3 deviation 4) | **Seed it in 2.4** (deterministic seed via state) so the three B2 rows rejoin the carried-row diff. |
| W2o 0.017 dB margin | D9 identity below the knee leaves W2o untouched. If it flips anyway, widen the seed set. Never loosen the threshold. |
| `Engine.h` stage comments | Reverb and wet safety are 2.4, not 2.6. Fix the comment. |
| Unity gain never derived from `get()` on skewed ranges | Applies to `fb_low_cut` (skew 0.28906) and `fb_high_cut` (skew 0.40781): read denormalised Hz. |

## Execution Defaults (Claude's call, recorded for plan)

- Feedback chain state is in **float**, per sample, with no per-segment numerics (R13 rule). The fb HP/LP coefficients are recomputed at control ticks, with targets ramped at the grid (zipper-free on cutoff sweeps). `tan()` is computed only at ticks.
- The fb HP/LP use the same TPT SVF form as `GrainFx.h` (shared free function). No `juce::dsp::IIR` (memory: default ctor is first order and allocates).
- Diffuser allpass delays round to whole samples at prepare. The 48.6 ms group-delay credit is computed from the rounded lengths at the test rate.
- The AGC envelope runs per sample. Its coefficients are derived from fs at prepare.
- **reverb_mix 0 bitwise:** after the ReverbUnit's 50 ms ramp settles at 0, its output must equal its input bitwise, or the call must be skipped. If skipping, keep the tank warm, or accept a cold tank on re-enable, whichever research shows avoids a click. This is needed for the neutral null.
- `freeze` is a Bool param read at control ticks. Edges come from the latched value. No parameter writes from listeners.
- The ring hash for FUNC-02 is FNV-1a over level 0, both channels, the full capacity (deterministic, harness-side).

## Constraints Identified

- Every per-pass array stays at 32 samples. The feedback write uses `fbDelay[n mod 32]`, valid because `kCell ≤ kFbDelay` (static_assert exists).
- No overdub in v1.0 (MipRing cannot rebuild octave levels in the past).
- Freeze never applies a gain ramp to the ring input. Only a content crossfade is allowed.
- The Nyquist clamp is 0.45·fs on the fb LP and the SVF (memory: SC LPF Nyquist clamp).
- No allocation on the audio thread. Diffuser/reverb buffers are allocated in `prepare()` on the message thread.
- `ScopedNoDenormals` covers the loop. The NaN scrub is still required, because FTZ does not catch NaN.

## Open Questions (for research)

- **Forced release on engage:** compute exact "remaining reach crosses the stopped head" per grain (forward and reverse, rate up to 4), and check that the 5 ms fade fits inside the margin(level) guard.
- **kFreezeMinSpray at the 41 s ring and heldSpan < grain length:** the clamp interplay when heldSpan is tiny (freeze 0.5 s after load at 500 ms grains).
- **Coherent tap law under freeze:** the grain wash of held material is maximally self-similar. Confirm the dB/generation < 0 holds with freeze released into a hot fbDelay.
- **D9 tanh cost:** `std::tanh` only above the knee, branch per sample. Is a branch-free form needed for SIMD later (2.6)?
- **ReverbUnit 1.2.0 `process()` on a ≤ 32-sample referenced buffer:** confirm the 50 ms ramp and SmoothedValue advance per sample (block-size invariant), and check the bitwise behaviour at mix 0.
- **D10 P(k) closed form** for LP/HP through the prewarped bilinear map (RESEARCH-2.3 `svf.py`), and whether P depends on g (fc/fs) enough to need a 2-D latch rather than k only.

## Next Phase

Ready for: research (`/plugin-research O-Granulator 2-dsp`, Phase 2.4).
