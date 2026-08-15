# Stage 2 — DSP · Phase 2.3 (Source Shaping and Outside-Hull Processing) — Summary

**Plugin:** O-Octagon
**Stage:** 2 of 4 — DSP
**Phase:** 2.3 of 3 — Source Shaping and Outside-Hull Processing
**GSD phase:** execute
**Date:** 2026-08-11
**Branch:** `feat/o-octagon`
**Plan:** `PLAN-2.3.md` (P25–P36) · **Research:** `RESEARCH-2.3.md` (Q1–Q10, H1–H10)

---

## Outcome

**The chain closes.** All seven of `ARCHITECTURE §5`'s steps are live. `width` reaches the shaper,
`solveSubPoint` returns the `d_hull` it previously computed and discarded, the hull trim and the air
LPF consume it, and the eight venue trims that have ridden in the snapshot since Phase 2.1 are
finally multiplied in.

**All eleven gates pass. 62 probes, 0 failures** (33 unit + 29 render-harness). Clean 3-format build
with a forced full recompile, **zero warnings and zero errors**. `auval` SUCCEEDED. pluginval
strictness 10 × 3 on VST3 and × 3 on AU, all six exit 0.

**All three `PHASE-2.3-*` markers read ZERO occurrences**, in `Source/` and in `tests/` — see
"Marker retirement" below, which is not the trivial result it looks like.

**Contract checksums re-verified at this boundary — all four byte-exact, and no pin moved:**

| Contract | SHA-256 (first 16) | Result |
|---|---|---|
| `BRIEF.md` | `697a4f32890d7420` | ✅ |
| `parameter-spec.md` | `b45f88dc5017ec2c` | ✅ |
| `research/ARCHITECTURE.md` | `a8a358f4be0ea183` | ✅ matches the 2.3 D2 re-pin |
| `ROADMAP.md` | `aec7d0ce0db9ad6c` | ✅ |

---

## What was built

| File | Change |
|---|---|
| `Source/DSP/HullProcessor.h` | **NEW.** Header-only `namespace hullproc` — `hullTrimGain`, `airCutoffHz`, five constants, four `static_assert`s |
| `Source/DSP/GainStage.h` | `juce_dsp` include, two mono `FirstOrderTPTFilter<float>`, `sampleRate`, four edge-tracking flags, `prepare()` gains `samplesPerBlock` |
| `Source/DSP/GainStage.cpp` | §5 steps 3, 4, 6 and 7 completed; three markers retired; P27 seed; P31 NaN guard |
| `Source/DSP/DbapSolver.h` | Counters 5 and 6 — `airCutoffUpdates`, `airSamplesFiltered` |
| `Source/DSP/SourceShaper.h` | Doc corrected — the "ships inert" paragraph was false as of this phase |
| `Source/PluginProcessor.h/.cpp` | `ProcessSpec` forwarding; `publishSnapshot()` sanitisation; `kVenueTrimClampDb` |
| `tests/unit/main.cpp` | Probes **AU–AX** |
| `tests/render-harness/main.cpp` | Probes **AY–BJ**, plus seven helpers |
| `.planning/REQUIREMENTS.md` | QUAL-01 and DSP-07 **method** corrections (Task 1) — no verdicts |

**No CMake change in any of the three targets.** P25's header-only shape is why, and `juce_dsp` was
already linked everywhere (H7 corrected `CONTEXT-2.3.md` constraint 9 on that point). The unit
target's link line is unchanged and still carries no `juce_dsp` — gate 11.

---

## Deviations, recorded rather than slipped

### 1. P27 — `reset(x)` at the air filter's entry edge, beyond §3.5.2's two named policies

§3.5.2 considered two re-entry policies and D2 chose the worse one. A third is strictly better than
both and satisfies D2's own stated rationale more completely than D2's own choice:

| policy | 1 kHz step at re-entry | vs. the signal's own max per-sample slew |
|---|---|---|
| resident (D2 as written) | 3.407e-01 | **520.9 %** |
| `reset(0)` (original §3.5.2) | 2.810e-01 | 429.6 % |
| **`reset(x)` — shipped** | **0** | **0.0 %** |

`processSample` computes `v = G·(x − s)`, so seeding `s = x` makes `v` exactly `0.0f` and `y == x`
**bit-exactly**, on every toolchain, at every cutoff, at every entry speed. D2 objected to
*discarding* continuity by re-zeroing; `reset(x)` discards nothing. **Probe BB confirms the entry
edge is bit-exact at both 1 kHz and 8 kHz.**

This is an implementation choice inside §3.5.2's stated intent, so no checksum moved.

### 2. P29's *position* guard — a pre-existing 2.2 hazard folded into a 2.3 site

The **trim** guard is unambiguously this phase's: FUNC-07's multiply is what arms it. The
**position** guard closes a hazard that has been live since 2.2 — a NaN speaker coordinate already
reached `dbap::solve`, where `dRaw < kMinDistance` is false for NaN and `denom < kDenomEpsilon` is
false for NaN, so it fell straight through to `setTargetValue`. Same one-line loop, same site, no
extra cost — but it is a scope addition and is recorded as one. Probe BH covers both.

### 3. P25 — `HullProcessor.h` is header-only against ROADMAP's `HullProcessor.{h,cpp}`

The same file-shape deviation as P14's `VenueGeometry.h`: two stateless free functions do not earn a
`.cpp`, and header-only means zero CMake churn in three files.

---

## The H2 erratum — §3.5.2's accepted-cost figures are wrong, and `ARCHITECTURE.md` was NOT re-pinned

§3.5.2's amendment paragraph and `REQUIREMENTS.md`'s QUAL-01 scope note both quoted the accepted
cost of D2 as *"3 dB @ 20 kHz, 0.7 dB @ 10 kHz, 0 dB at DC"*. **Two errors, both understating it.**

**Arithmetic — the 10 kHz figure is the ANALOG one-pole's** (−0.969 dB at `f/fc = 0.5`, rounded
down). The digital TPT filter at `fc = 20 kHz` is far flatter, because 20 kHz sits at 0.83 × Nyquist
and bilinear prewarping compresses the passband:

| fs | @1 kHz | @4 kHz | @8 kHz | @10 kHz | @15 kHz | @20 kHz |
|---|---|---|---|---|---|---|
| 44 100 | −0.0005 dB | −0.0081 | −0.0384 | **−0.0695** | −0.3008 | −3.0103 |
| 48 000 | −0.0013 dB | −0.0223 | −0.1027 | **−0.1798** | −0.6476 | −3.0103 |

The 20 kHz figure is right — −3.0103 dB **is** the definition of the cutoff.

**Conceptual, and this one changed the measurement.** The step at a crossing is the difference
between two signal *paths*, so the quantity is the complex `H(f) − 1`, not `| |H| − 1 |`. The
**phase lag dominates everywhere below ~15 kHz**:

| tone (fs = 48 kHz) | magnitude-only | **full \|H − 1\|** | phase term dominates by |
|---|---|---|---|
| 1 kHz | 1.542e-04 | **1.756e-02** | **114×** |
| 8 kHz | 1.176e-02 | **1.529e-01** | 13.0× |

At 44.1 kHz the 1 kHz ratio is **190×**. So the real accepted cost is **~1.8 % of a 1 kHz component
and ~15 % of an 8 kHz component, as a one-sample step** — bounded, one sample, only on a deliberate
gesture, but a real tick on HF-rich material.

**The error is in the conservative direction, so D2's decision stands a fortiori**, and *"0 dB at
DC"* remains true — which is exactly why a DC-only probe is blind to it and BB drives both
excitations.

**`ARCHITECTURE.md` was deliberately not re-edited and not re-pinned (P36).** The figure is
descriptive prose about a cost, not a specification the code implements, and re-pinning a contract
mid-phase is the thing the discipline exists to avoid; the pin has already moved twice.
`REQUIREMENTS.md` is not checksummed and was corrected in place at plan, because its QUAL-01 note
prescribed a **measurement method that Q6 proved vacuous** — leaving that standing would have put
the probes in direct conflict with the requirement document.

---

## Q6's negative result, recorded so the method is not re-proposed at Stage 3

**D3's originally specified QUAL-01 method for `airAmount` measures `+0.00000 %` excess** — zero to
nine decimals. The construction was "`max |out[n] − out[n−1]|` swept vs held, plus tolerance". It
fails because the render's maximum slew is set by the sine's own zero crossing, which occurs early,
where the swept and held renders still share a cutoff. **It would have passed for a reason
unrelated to the code.** Replaced by P34's two-render differential with a bound computed in-probe
from the sweep schedule the probe itself drives (probe BC).

---

## P33's method, stated as a method rather than as a verdict

**DSP-07 criteria 2 and 5 are NOT "bit-transparency ✓".** There is no "filter absent" build and no
"trim absent" build to render against, and fabricating one would mean a second arithmetic path
selected by a compile flag — the class of thing §3.4.3 forbids, and a path the shipping binary would
never take. The claim is made structurally:

| claim | proof | non-vacuity control |
|---|---|---|
| `hullAtten = 0` → trim is a no-op | `bitExact (hullTrimGain (0, d), 1.0f)` over 201 swept `d`, and over 13 `hullAtten` at `d = 0` (**AV**) | `hullAtten` 0 vs 0.5 outside the hull **changes** the render (**BD**) |
| `airAmount = 0` → filter is absent | `airSamplesFiltered == 0` — the branch is *counted* as never taken (**BD**) | at 0.35 outside, the counter reads **16384/16384** and the render **differs** (**BD**) |
| `d_hull = 0` → absent at any `airAmount` | renders at `airAmount ∈ {0, 0.35, 1.0}` inside the hull are **bit-identical by memcmp**, counter 0 in all three (**BD**) | the same three settings **differ** outside |

`std::pow (10.0f, -0.0f)` is **exactly `1.0f`** by C99 Annex F / IEEE 754, and `-0.0f > -100.0f` is
true so `decibelsToGain` takes the `pow` branch. No `-ffast-math` anywhere on the line.

**One precondition, stated rather than left implicit.** `processBlock` runs under
`juce::ScopedNoDenormals`, so a **denormal** `v_i` (< 1.18e-38) would flush to zero under
`v_i * 1.0f`. Reaching one requires a weight around 1e-38 — unreachable from the exposed 0–1 range
with any non-degenerate geometry — and the probes use default weights.

### DSP-07/7 is met structurally, and the honest reason is recorded

Under P27 the two reset policies §3.5.2 debated are **observationally equivalent at the boundary**: a
spurious `reset(0)` taken while the filter is inactive is overwritten by the entry re-seed before a
single sample passes through it. No measurement can distinguish them, and claiming otherwise would
be false. The criterion is met **structurally** — the reset is guarded by an `airAmount` edge, not a
`d_hull` test — and probe BD measures the property the criterion exists to protect: **12 crossings,
worst DC step 0.0009319 against a bound of 0.0041677, no click and no state corruption.**

---

## Measured values

### New in this phase

| Probe | Measurement |
|---|---|
| **AU** | §3.5.2 table exact to 1 Hz; H4 ceilings **9922.5 / 14400 / 19845 / 20000** Hz; `fc < fs/2` over 5 rates × 21 `airAmount` × 51 `d_hull`; floor 625.0 Hz @15 m → 500.0 Hz @25 m |
| **AV** | dB/m law linear over 6 × 20; floor exactly **−24.0000 dB**; **bit-exact 1.0f** over 201 `d` and over 13 `hullAtten` |
| **AW** | invariance Δ **3.0e-8** (λ=0.5) and **6.0e-8** (λ=2.0) at blur 0.25; **positive control Δ1.844e-3** at blur=1 λ=2.1 — the clamp is visible |
| **AX** | span 4.000 m ⟂ bearing, R on audience right; dy 3.55 m → **dz 1.6349 m**, flat rake equal z; rFade **1.190 m**, monotone |
| **AY** | width=0: **8/8 gL==gR bitwise**; width=4: 8/8 separated, worst **0.1636** |
| **AZ** | all 5 ramped under bound, all 5 negative controls fire; **17 of 17 parameters covered** (AS 11 + AZ 5 + BC 1) |
| **BA** | centroid crossing at width=6: worst step **0.0003724** / 0.0041677; control 0.02383 fires |
| **BB** | **entry BIT-EXACT at both tones.** Exit amplitude **0.005328** (1 kHz) and **0.046267** (8 kHz), each matching the predicted `A·\|H−1\|` to **0.000 %**. `d_hull` 0.250 m, `fc` 19599.7 Hz |
| **BC** | 8 kHz \|D\| 0.006904 ≤ 0.014221, **28.3×** the 64-grid bound (asserted); 1 kHz 3.7× (reported). maxΔG **58.2×** between grids |
| **BD** | inside: bit-identical at 0 / 0.35 / 1.0, 0 filtered. Outside: 0 → 0 filtered; 0.35 → **16384/16384** and the render differs |
| **BE** | active-filter NaN recovers in one block; **the skipped-filter hole clears at re-entry** |
| **BF** | −12 dB → **×0.25119** on lane 4 (want ×0.25119), **7/7 others bitwise unchanged**; +6 dB → ×1.99526, 7/7 unchanged; venue-scoped ratio 0.251189 → 0.251189 |
| **BG** | live venue edit mid-playback: worst step **0.0009429** / 0.0041677, and the edit demonstrably landed |
| **BH** | `trimDb` ∈ {1e30, −1e30, NaN} with `w_i = 0`: finite, **no latch**, all three. Clamp **×15.8489 = +24 dB exactly**. NaN coordinate finite |
| **BI** | 512 vs 4096 **bit-identical**; ragged 1,7,64,333,4096 vs 4096 **bit-identical**, 7 non-aligned events — with width, trim and both filters live |
| **BJ** | **cutoffs 48 == solves 24 × 2**; **powCalls 16** (probe AE intact); filtered 24576/24576; advances 12288/12288 |

### Cost added

2 `tan` + 2 `exp2` + 2 `pow` per control block = **4 500 transcendental calls/second at 48 kHz** on
top of 12 750 — about 35 % more control-rate work. **PERF-01's per-sample budget is untouched**, and
`powCalls == 16` still holds exactly because neither `std::exp2` nor `juce::Decibels` routes through
`countedPow`.

---

## Three defects the probes caught during execution

Recorded because each was a probe that would otherwise have passed while measuring nothing.

1. **A running max silently discards NaN.** The DSP-07/8 guard was first written as
   `worst = jmax (worst, |f|)`. `juce::jmax` is `a < b ? b : a`, and `worst < NaN` is **false**, so
   the maximum drops the NaN it exists to catch and the guard never fires. Replaced with a
   last-output check, which is *exactly* equivalent here because the filter state is sticky by
   construction (`s = y + v` re-derives `s` from an already non-finite value), and which cannot be
   defeated this way.

2. **A peak-based bound under-reads a sine by up to 13.4 %.** BB's exit bound first used
   `max |dry|`. At 8 kHz on a 48 kHz grid there are six samples per cycle, so depending on phase the
   sampled maximum sits at `sin 60° = 0.866` of the true amplitude — and the probe failed by
   **1.136×**, which is `1/0.866`. Worse, the 1 kHz half was *passing* at a ratio of 0.365 purely
   because a single-sample reading happened to land near a zero crossing. Replaced with an
   RMS-derived amplitude over a 2016-sample window — an integer number of cycles at both tones —
   which made the assertion an **equality against the predicted `|H−1|`, now matching to 0.000 %**.

3. **`findOutside` returned `nx = 0.0`, and a probe perturbed the position by SCALING it.** BJ's
   counter identity `airCutoffUpdates == solveRuns · 2` was passing as **0 == 0**: `onx * k` moved
   nothing, no solve ever ran, and the probe measured nothing at all. Fixed three ways — the helper
   now returns the *farthest* outside point (also making every DSP-07 probe meaningfully outside
   rather than a few centimetres out), the perturbation uses `srcZ`, and BJ now asserts
   `solvesN >= blocks` so the identity cannot be satisfied by zero.

A fourth, smaller: **AZ swept `rolloff` to 9.0 against a declared range of 3.0–6.0** and
`outputGain` to +6 against +12, so `setValueNotifyingHost` clamped and the "full-range sweep" covered
less than the criterion claims. The sweep endpoints are now read from
`getAPVTS().getParameterRange(id)`.

---

## Marker retirement — not the trivial result it looks like

All three tokens read **zero occurrences in `Source/`**, and `PHASE-2.2-REPLACE` is still 0. Counted
as occurrences rather than lines, per gate 2.

**But the first pass left `PHASE-2.3-WIDTH` alive in `tests/unit/main.cpp`**, inside probe AH's
comment, along with two other prose sites asserting that width "ships inert" — one of them in
`Source/DSP/SourceShaper.h`, where it was simply false as of this phase. Gate 2 as literally written
scopes to `Source/`, so a token surviving in `tests/` would have passed the gate and grandfathered
exactly the thing the mechanism exists to retire. All three were corrected; the final grep covers
`Source/` **and** `tests/` and reads zero.

---

## Gates — actual output, not read out of a prior document

| # | Gate | Result |
|---|---|---|
| 1 | Clean 3-format build, forced TU recompile | ✅ **zero warnings, zero errors** (VST3 + AU + Standalone) |
| 2 | Marker retirement, occurrences not lines | ✅ 0 / 0 / 0, `PHASE-2.2-REPLACE` 0, in `Source/` **and** `tests/` |
| 3 | Hardcoded output channel indices outside `ChannelMap` | ✅ zero |
| 4 | `auval -v aufx OuOc OuDv` | ✅ **AU VALIDATION SUCCEEDED** |
| 5 | pluginval strictness 10, ×3 VST3 + ×3 AU | ✅ all six exit 0 |
| 6 | Both test targets, `-DOUARICON_BUILD_TESTS=ON` | ✅ exit 0 / exit 0 — **33 + 29 = 62 probes, 0 failures** |
| 7 | `gen_dbap_reference.py --check` | ✅ exit 0, 102 cases |
| 8 | 17 parameters vs `parameter-spec.md` | ✅ 4+2+8+2+1 = 17; ranges and defaults match rows 1–17; **2.3 adds none** |
| 9 | `setLatencySamples` / `switch` on `ChannelType` / `createEditor` guard | ✅ only the "NEVER call" comment / none / guard present |
| 10 | `OOCTAGON_INSTRUMENT` scoping | ✅ the two test CMakeLists only; **six** counters |
| 11 | Unit-target link line unchanged | ✅ no `juce_dsp`; `GainStage.h` unreachable from the fast target |

---

## Requirement status — proposed, for the verify phase to rule on

Verdicts land at verify, not here. On the evidence above:

| Req | Expected | Evidence |
|---|---|---|
| **FUNC-07** | ✅ | BF (all four criteria), BH (the latch it would have armed) |
| **DSP-06** | ✅ | AY (1), AX (2, 3, 5), BA (4) |
| **DSP-07** | ✅ under §3.5.2 as amended, with the H2 erratum | AU (4), AV (1–3), BD (2, 5, 6, 7 — **P33 method**), BE (8) |
| **DSP-08** | ✅ | AW, with a positive control at the clamp |
| **QUAL-01** | ✅ under the D3/D4 scope **as corrected by P34** | AS + AZ + BC = 17/17 (criterion 1); BB (criterion 2) — **subject to D5** |
| **QUAL-04** | ✅ — **criterion 3 closes, clearing 2.2's PARTIAL** | AZ's `width` sweep |

---

## Open — the manual gate (Task 10, D5)

⚠️ **NOT BLOCKING EXECUTE. Owed before 2.3 verify closes.** One combined ~15-minute Logic session,
folding in 2.2's carried Task 12:

**Carried from 2.2** (corroboration; FUNC-03/3's gate is probe AJ, shown non-vacuous by
VERIFICATION-2.2 NC3):
- Automate `srcX` across the room — the 8 surround-meter lanes no longer move in lockstep.
- Set `w3 = 0` — that lane goes silent while the others compensate.

**New at 2.3:**
- Width audibly spreads the source as `width` goes 0 → 6.
- Air audibly dulls the source outside the hull, and is **inaudible inside** it (the D2
  default-patch claim, by ear).
- A per-speaker trim moves one lane only.
- **H2's addition — cross the hull boundary with HF-rich material, not a sine.** Measurement bounds
  the step at ~15 % of an 8 kHz component (BB measured 0.046267 amplitude against a 0.25 m
  crossing); **this is the one claim in the phase that only listening can settle.** If it ticks
  audibly, that is a **discuss-boundary** finding for Stage 3 — H3 names the lever (raising
  `fc(d_hull = 0)` toward Nyquist re-tunes the whole musical curve), so it is not a plan-phase
  change.

Record every observation verbatim, positive or negative.

---

## Carried past 2.3

Nothing beyond the three residuals named at discuss:

- **CI gap** — no test target in this repo has ever run in CI, so all 62 probes fire only under
  `-DOUARICON_BUILD_TESTS=ON` locally (`.planning/todos/pending/`, Stage 4).
- **`COMPAT-04`** — retroactive acceptance criteria owed at Stage 4.
- **`FUNC-06` and `UI-02..05`** — summary rows with no acceptance criteria, owed at **Stage 3
  discuss, before Stage 3 plan**. This is the same defect the stage has now repaired three times
  (PERF-02 and QUAL-04 at the 2.2 boundary; FUNC-07, DSP-06, DSP-07 and DSP-08 at the 2.3 boundary).

**Stage 2 closes with a stage-level `VERIFICATION.md`** at verify — 2.1 and 2.2 wrote
phase-suffixed reports only.
