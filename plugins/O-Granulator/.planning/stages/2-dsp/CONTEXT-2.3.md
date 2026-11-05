# Stage 2 — DSP · Phase 2.3 (Per-grain FX + CPU budget) — Context

**Plugin:** O-Granulator
**Stage:** 2 of 4 (DSP)
**Phase:** 2.3 of 6
**GSD phase:** discuss
**Date:** 2026-11-05
**Branch:** `main` (trunk-based) @ `c10d0f2c`
**Participants:** Taylor Brook, Claude

---

## Entry Check — contract checksums

All six were re-hashed this session, and each matches the STATUS.md frontmatter byte for byte. REQUIREMENTS.md is at the 2.2-verify revision `6530b786…`.

| Contract | SHA-256 (prefix) | Result |
|---|---|---|
| `BRIEF.md` | `f9c748a8…` | ✅ |
| `REQUIREMENTS.md` | `6530b786…` | ✅ |
| `parameter-spec-draft.md` | `d0b1457e…` | ✅ |
| `parameter-spec.md` | `f5ce3595…` | ✅ |
| `research/ARCHITECTURE.md` | `f0cf753b…` | ✅ |
| `ROADMAP.md` | `79d69576…` | ✅ |

---

## Discussion Summary

ARCHITECTURE §9 (Core Components), Algorithm Details §9 and the CPU risk section already specify 2.3: the chain order, the TPT SVF coefficients, the latch-at-spawn rule, the `active` flags and the budget levers. Discuss reopened three level and character laws. Taken literally, each would ship a defect:

- **Drive** `tanh(d·x)/d`: the peak is capped at 1/d, so full drive puts a 0 dBFS grain at −24 dB.
- **Crush** mid-rise `floor(..)+0.5`: silence quantises to +½ step, which is +0.25 DC at 2 bits. Every grain becomes a windowed DC thump at about −12 dBFS.
- **BP = v1**: the peak gain is 1/k, which is +20 dB at res 100 %.

All three are now locked as errata against ARCHITECTURE (D6–D8). D1–D3 from CONTEXT-2.1 still hold: grain-engine 1.1.0 is consumed unmodified, and the only human listening checkpoints are at 2.1 and 2.4. **Phase 2.3 therefore has no listening checkpoint.** "Audibly per-grain" is gated objectively.

---

## Approach Decisions (locked this session)

| # | Decision | Choice | Rationale |
|---|---|---|---|
| D6 | Drive level law | **Unity at a −12 dBFS reference.** `y = r·tanh(d·x) / tanh(d·r)` with `r = 0.25`. A −12 dBFS grain keeps its level at every drive amount. Louder grains saturate, and the small-signal gain is `r·d / tanh(d·r)`, about +12 dB at full drive. **`d` must reach 0 continuously.** Use `d = 16·drive²` in place of ARCH's `1 + 15·drive²`, so that `y → x` as drive → 0+. Bypass exactly at drive 0, and also when `d` falls below a small guard where the law is numerically ≡ x. With ARCH's d = 1 floor, the first step above 0 would jump to a curved law with +0.18 dB small-signal gain. | Under ARCH's /d law, turning drive up makes grains much quieter. The −12 dBFS reference matches typical live-input grain level. **Erratum vs ARCHITECTURE §9 Drive and the Parameter Mapping note `d = 1 + 15·drive²`.** The parameter range and default are unchanged. The tooltip ("Soft saturation applied to each grain.") still holds. |
| D7 | Crush quantiser | **Mid-tread.** `q(x) = round(x·2^(b−1)) / 2^(b−1)`. Silence stays silent, and quiet tails gate to zero, which is the classic bitcrusher character. The hold-rate law (`fs·(1000/fs)^crush`) and the bit law (`16 − 14·crush`) are unchanged. | The mid-rise form turns silence into a DC offset of ½ LSB, up to −12 dBFS at 2 bits. **Erratum vs ARCHITECTURE Algorithm Details §9 (mid-rise formula).** |
| D8 | SVF resonance gain | **BP normalised, LP/HP raw.** BP output = `k·v1` (unity peak gain), so resonance narrows the band without boosting it. LP = `v2` and HP = `x − k·v1 − v2` keep their natural resonant peak, which is the expected synth-filter character. The output law **ignores FX**, as it ignores gain_rand (D5). N_eff and g_out are unchanged, and the FX alter the level by design. | A +20 dB BP at Q 10 would make the resonance knob mostly a level knob. LP/HP peaks are the character users expect. Headroom for the LP/HP peaks is checked by the QUAL-02 stress row. Wet safety and the 2.4 limiter path come later. **Erratum vs ARCHITECTURE Algorithm Details §9 ("BP = v1").** |

---

## Requirements Confirmed (Phase 2.3 scope — ROADMAP § Phase 2.3)

**Components** (order per ARCH §9: read → SVF → drive → crush → ring → window·gain·pan):

- **SVF:** Cytomic TPT 2-pole, stereo. The mode is LP/BP/HP from `sN_filter_type`, latched per grain; Off = inactive. Cutoff is `fc = cutoff·2^(filterRand·4·(u−0.5))`, clamped to `[20 Hz, 0.45·fs]`. `g = tan(π·fc/fs)` is computed once at spawn. `k = 2 − 1.9·res`. BP = `k·v1` (D8).
- **Drive:** the D6 law, latched `d` and normalisation.
- **Crush:** latched `crushed = u < crushProb`, drawn only when crush > 0. A fractional-phase sample-rate hold, then the D7 mid-tread quantiser.
- **Ring mod:** a quadrature rotator `c ← c·e^{jω}`, with `|c|` renormalised once per 32-sample pass. `f = ringFreq·2^(ringRand·4·(u−0.5))`, clamped to ≤ 0.45·fs. `y = x·((1−mix) + mix·Re c)`.
- **Latched `active` bit per stage** (the `Grain::fxActive` field already exists). A stage at its neutral value costs nothing: filter Off, drive 0, crush 0 or not crushed, ring_mix 0.
- **NaN kill on SVF state:** if the state is non-finite at the pass boundary, kill the grain and write 0 for the rest of the pass.
- All values are latched at spawn. Modulating an FX destination changes the next grain, not the ones already sounding (ARCH decision).

**Test criteria (gate):**

- [ ] **Neutral-FX bit identity first:** with every FX at its neutral value, renders are bit-identical to the 2.2 tree (out-of-tree null in the R12 style). This must hold before any FX row counts.
- [ ] Each FX is audibly per-grain (DSP-05), gated objectively with no listening. With rand > 0, latched per-grain values differ between grains: the cutoff and ring-frequency distributions span the ±2-oct law, and a per-grain spectral-centroid or carrier measurement varies across grains. The crush Bernoulli at 0 / 100 % is exact, and the 50 % row falls within binomial bounds.
- [ ] No bleed (DSP-05): after a resonant grain (res 100 %, tonal), the next occupant of the same slot starts from zero SVF, hold and phasor state. A stolen grain keeps its FX state in its fade slot.
- [ ] Law gates: drive keeps a −12 dBFS sine within ±0.1 dB across drive 0→100 %, and the step from 0 → 0+ is continuous (D6). Crush on silence outputs exactly 0 at every bit depth (D7). BP peak gain stays ≤ 0 dB +0.1 dB across res 0→100 % (D8).
- [ ] QUAL-02 stress: max res on every mode × max drive × max crush × ring 100 % × 128 grains × 4 streams. No NaN, Inf or denormal, and every sample is finite.
- [ ] QUAL-01 cutoff and FX sweeps: values are latched, so a sweep can only step *between* grains. Gate it as a click gate against a same-motion reference: a sweep render vs a static render.
- [ ] **CPU (PERF-02, first measurement):** 128 grains with all FX on, 4 streams, at 48 k/128, Release. Report the ratio against the harness engine-bypass baseline: ≤ 30 % of one core in the worst case, ≤ 10 % for a typical preset. Feedback and reverb don't exist until 2.4, so the full worst case (with feedback and reverb) is re-measured in 2.6. If the measurement is over budget, apply the levers in ARCH order (Hermite for rate ≤ 1 at level 0 → SIMD ×4 → pool 96) before moving on.
- [ ] Block-size null (PERF-03) re-gated with all FX on and randomised: seeded, plus a different-seed control.
- [ ] Allocation gate (PERF-01) re-run with FX on, type switches and spawns/steals.

---

## Carry-ins

| Item | Handling |
|---|---|
| W2o 0.017 dB margin (VERIFICATION-2.2) | CounterRng is **keyed by slot**, so the new FX draw slots (FilterRand, CrushProb, RingRand) append without moving any existing draw, and W2o should stay identical. If W2o flips anyway, widen the seed set. Never loosen the threshold. |
| Grain POD layout | The 2.3 fields are already in `Grain.h`. D6 needs one extra latched normaliser (`driveNorm`). The struct must stay trivially copyable (static_assert). |
| Unity gain is never derived from `get()` on skewed ranges | Applies to `filter_cutoff` (skew 0.19889) and `ring_freq` (skew 0.24562): read denormalised Hz, and never invert the skew by hand. |

## Execution Defaults (Claude's call, recorded for plan)

- **Draw slots:** FilterRand, CrushProb and RingRand are appended to the Spawner slot enum (append-only, per R10). Each is drawn only when its control is > 0 (filter_rand > 0 with filter on, crush > 0, ring_rand > 0 with ring_mix > 0). All draws happen before acquire.
- **Ring carrier start phase:** 0 (`c = 1 + 0j`). No draw, deterministic. The carrier phase relative to the source still varies across grains.
- **Cutoff default 20 kHz with rand > 0:** draws above 0.45·fs pile up at the clamp. This is accepted as-is, with no rescaling.
- **Crush hold:** the phase counter is latched per grain and starts at a fresh hold, `crushHeld = first sample`.
- **FX operate in float.** Only the read position stays in double (memory: float grain read position).
- **Kernel structure:** a per-grain branch on `fxActive`, hoisted out of the per-sample loop (one specialised inner loop per active combination, or a switch per pass). It is not a per-sample flag test. Plan may pick between these by measurement.
- **CPU harness:** reuse the 2.2 C2 row and its bypass baseline, and add a C3 row for all FX on.

## Constraints Identified

- No `juce::dsp::StateVariableTPTFilter` or `IIR::Filter` (ARCH §9, memory: IIR default ctor is first-order and allocates).
- No `tan()`, `pow()` or `exp()` per sample. These are computed only at spawn.
- The Nyquist clamp is `0.45·fs`, not a fixed 20 kHz ceiling (memory).
- `ScopedNoDenormals` already covers the SVF integrators. The NaN kill is still required, because FTZ does not catch NaN.
- Every per-pass array stays fixed at 32 samples.

## Open Questions (for research)

- **D6 small-d guard:** the threshold below which `r·tanh(d·x)/tanh(d·r)` is ≡ x in float, plus the cost of a `tanh` per sample against a rational approximation. Check how the approximation error interacts with the ±0.1 dB law gate.
- **Ring renormalisation per 32-sample pass:** the worst-case magnitude drift at 0.45·fs in float. Confirm it is below −120 dB per pass.
- **Objective "audibly per-grain" metric:** the per-grain centroid vs a latched-value census. Pick the one that a broken latch (all grains sharing one value) would fail. This is a negative control.
- **LP/HP resonant-peak headroom:** at res 100 % with a tonal source near fc and 128 grains, what does the output law's g_out leave as peak? This feeds whether 2.4 wet safety needs a note.

## Next Phase

Ready for: research (`/plugin-research O-Granulator 2-dsp`, Phase 2.3).
