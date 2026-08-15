# Stage 2 — DSP · Phase 2.3 (Source Shaping and Outside-Hull Processing) — Verification

**Plugin:** O-Octagon
**Stage:** 2 of 4 — DSP
**Phase:** 2.3 of 3 — Source Shaping and Outside-Hull Processing
**GSD phase:** verify
**Date:** 2026-08-11
**Branch:** `feat/o-octagon`
**Verifies:** FUNC-07, DSP-06, DSP-07, DSP-08, QUAL-01, QUAL-04/3

---

## Verdict

**✅ VERIFIED.** Stage 2 closes. Ready for Stage 3: **yes**, with no blockers.

Every automated gate was **re-run from scratch at verify** against a forced full recompile
(`rm -rf build/plugins/O-Octagon`, 119 steps), not read out of `SUMMARY-2.3.md`. All eleven passed;
**62 probes, 0 failures** (33 unit + 29 render harness); none of A–AT regressed. Six requirements
close ✅, including **QUAL-04, whose 2.2 partial is cleared**.

**Eight negative controls were run as new work at verify** — the 2.2 discipline, applied at greater
depth because 2.3's diff is three lines and a filter pair, which is exactly the shape of change that
can look delivered while measuring nothing. Every load-bearing claim was broken deliberately and
every gate failed loudly and diagnostically. **Two of the eight corrected a piece of evidence
attribution** (NC6, NC7/NC8) and one found a cross-phase dependency nobody had stated (NC1).

The tree was proved **byte-identical to its pre-NC state** afterwards (26 files, `shasum -a 256`), and
the restored build's render-harness output is **byte-identical to the pre-NC run**.

**The D5 manual Logic gate remains OPEN and is carried to Stage 3 discuss** — by decision at this
boundary. It is corroboration for everything except QUAL-01 criterion 2's *audible* clause, which
measurement bounds but cannot conclude. See "Residual" below.

---

## Entry check — contract checksums

Recomputed at this boundary, not read out of a prior document
(`pattern_promotion_checksum_pins_replaced_file`).

| Contract | Recomputed SHA-256 | vs `STATUS.md` |
|---|---|---|
| `BRIEF.md` | `697a4f32890d7420cdef85bafbf8fe45775bf805cf1ff7b449ed2c14f6b9fbd6` | ✅ byte-exact |
| `parameter-spec.md` | `b45f88dc5017ec2c1a9da49ba35242d01903000a4ff199d16758e1b6cbb9e02f` | ✅ byte-exact |
| `research/ARCHITECTURE.md` | `a8a358f4be0ea1834da1540c01550c438d4c445dfbfe3d79478df0429b6d4408` | ✅ byte-exact — the 2.3 D2 re-pin |
| `ROADMAP.md` | `aec7d0ce0db9ad6c78cb1c9e9574a0a2f8ddb1cf258e6e4b701f2e2e0137ee29` | ✅ byte-exact |

**No pin moved at 2.3.** P36 is honoured: `ARCHITECTURE.md` carries the same hash it was issued at
2.3 discuss, and H2's figure correction lives as an erratum in `SUMMARY-2.3.md` and in
`REQUIREMENTS.md`'s QUAL-01 note — neither of which is checksummed — rather than as a third re-pin.

---

## Goal-Backward Analysis

### Original goals (from `CONTEXT-2.3.md`)

1. **Close the chain** — all seven of `ARCHITECTURE §5`'s steps live: `width` reaches the shaper,
   `solveSubPoint`'s `d_hull` stops being discarded, the hull trim and air LPF consume it, and the
   eight venue trims carried since 2.1 are multiplied in.
2. **Retire all three `PHASE-2.3-*` markers** to zero occurrences.
3. **Make the default patch bit-transparent** (D2: the air skip condition is `airAmount · d_hull == 0`).
4. **Close FUNC-07, DSP-06, DSP-07, DSP-08, QUAL-01, and QUAL-04's carried criterion 3.**
5. **Write the criteria that were owed** for FUNC-07 / DSP-06 / DSP-07 / DSP-08 *before* plan.

### Deliverables (from `SUMMARY-2.3.md`, confirmed by code inspection at verify)

| Goal | Delivered | Confirmed at verify by |
|---|---|---|
| 1 | `GainStage.cpp:181` reads `p[params::width]`; `:203-204` return and consume `dHullL/R`; `:211-218` fold the trim; `:224-225` set both cutoffs; `:279-280` multiply `snapshot.trimLin[i]` | line-by-line read of `updateControl` and `renderChunk`; NC3, NC4, NC6 |
| 2 | 0 / 0 / 0 occurrences in `Source/` **and** `tests/`; `PHASE-2.2-REPLACE` still 0 | gate 2, counted as occurrences |
| 3 | `airActiveL/R = airAmount > 0 && d_hull > 0` (`GainStage.cpp:238-239`) | probe BD; NC4 breaks it and BD says `FILTER RAN INSIDE THE HULL` |
| 4 | probes AU–BJ, 16 new | requirements table below |
| 5 | four `###` sections present in `REQUIREMENTS.md`, written at the discuss boundary | read at verify; all four carry criteria |

### Goal achievement

| Goal | Status | Evidence |
|---|---|---|
| 1 — the chain closes | ✅ Achieved | Every one of the three marker sites is live and each is proven live by a negative control that makes a named probe fail |
| 2 — markers retired | ✅ Achieved | 0/0/0 in `Source/` and `tests/`, occurrences not lines |
| 3 — default patch bit-transparent | ✅ Achieved | BD: renders at `airAmount ∈ {0, 0.35, 1.0}` inside the hull are bit-identical by `memcmp`, counter 0 in all three |
| 4 — six requirements close | ✅ Achieved | FUNC-07, DSP-06, DSP-07, DSP-08, QUAL-01, QUAL-04 all ✅ |
| 5 — criteria debt cleared | ✅ Achieved | Re-checked at verify: the only rows still lacking criteria are the known dated debt (FUNC-06, UI-02..05 → Stage 3; COMPAT-04 → Stage 4) |

---

## Automated Checks — all eleven re-run at verify

| # | Gate | Verify-phase result |
|---|---|---|
| 1 | Clean 3-format build + both test targets, forced TU recompile | ✅ exit 0, 119 steps, **zero `warning:` / `error:` / `FAILED`** |
| 2 | Marker retirement, occurrences not lines | ✅ **0 / 0 / 0**, `PHASE-2.2-REPLACE` **0** — in `Source/` **and** `tests/` |
| 3 | Hardcoded output channel indices outside `ChannelMap` | ✅ 2 hits, both INPUT reads bounded by `numIn`; the only output writes are `speakerToBuffer[i]` (REAL) and a loop var bounded by `jmin(numOut, 8)` (SAFE) |
| 4 | `auval -v aufx OuOc OuDv` | ✅ **AU VALIDATION SUCCEEDED**, exit 0 |
| 5 | pluginval s10, VST3 ×3 / AU ×3 | ✅ all six exit 0, **zero `FAILED`** |
| 6 | Both test targets, `-DOUARICON_BUILD_TESTS=ON` | ✅ exit 0 / exit 0 — **33 + 29 = 62 probes, 0 failures** |
| 7 | `gen_dbap_reference.py --check` | ✅ exit 0 — **102 cases** |
| 8 | 17 parameters vs `parameter-spec.md` | ✅ **17/17 across three sides** (see below) |
| 9 | `setLatencySamples` / `switch` on `ChannelType` / `createEditor` guard | ✅ absent (only the "NEVER call" comment) / absent / present |
| 10 | `OOCTAGON_INSTRUMENT` scoping | ✅ **0** in the plugin target; 1 each in the two test `CMakeLists.txt`; **six** counters |
| 11 | Unit-target link line unchanged | ✅ **no `juce_dsp`** in `tests/unit/CMakeLists.txt`; `GainStage.h` is not in its `target_sources` |

### Gate 1, stated precisely — the 2.2 correction still applies

`0` matches for `warning:` / `error:` / `FAILED` across compile and link. An unfiltered `-i warning`
grep returns **40** on a cold-configure log — **39 × `JUCE_BUNDLE_ID … contains spaces` emitted once
per plugin target repo-wide (none of them O-Octagon's)** plus 1 concurrentqueue CMake deprecation.
Identical to the count recorded at 2.2 verify (issue 1). Recorded again so a rerun on a cold build
dir is not read as a regression.

### Gate 8, stated precisely — three sides, none hand-transcribed

`parameter-spec.md` parsed programmatically (17 rows, the `w1…w8` row expanded), `PluginProcessor.cpp`
parsed for `makeFloat(...)` + the weights loop (17), and the `auval` runtime dump parsed for parameter
names (17). **ID, display name, range and default match 17/17 between spec and source; the `auval`
name set matches the source name set exactly.** 2.3 adds no parameter.

---

## Negative controls — new work at verify

Execute *asserted* that its load-bearing gates work. Verify **measured** it. Each claim that carries a
requirement on its own was broken deliberately in the source, the affected test target rebuilt and
run, then reverted. **The tree was proved byte-identical to baseline afterwards** and the restored
build's render output is byte-identical to the pre-NC run.

| # | What was broken | Expected to fail | Result |
|---|---|---|---|
| NC1 | P27's `reset(x)` entry seed (state left resident — D2 as written) | BB | **BB, AS, AZ all FAIL** |
| NC2 | P29's `trimDb` sanitisation at `publishSnapshot()` | BH | **BH FAILS** |
| NC3 | FUNC-07's `* snapshot.trimLin[i]` multiply | BF | **BF, BH FAIL** |
| NC4 | the `d_hull > 0` half of D2's skip condition | BD | **BD, BB, Q′ FAIL** |
| NC5 | H4's Nyquist-safe ceiling (literal `20000`) | AU | **AU FAILS** |
| NC6 | `width` reverted to the 2.2 literal `0.0f` | AY | **AY FAILS — but AZ still PASSES** |
| NC7 | P31's per-block NaN guard | BE | **BE FAILS (half 1)** |
| NC8 | P31's guard **and** P27's seed together | BE half 2 | **BE reports `RE-ENTERED POISONED`** |

### NC1 — P27's seed is load-bearing far beyond BB, and 2.2's probe AS now depends on it

Removing the seed makes **BB** report `entry NOT BIT-EXACT — P27's SEED IS GONE` at both tones and
blows the DC step to **0.1367314 against the 0.0041677 bound (33×)**. The exit amplitudes are
unaffected (`err 0.000%` at both tones), which is correct — the exit edge is inherent to D2 and the
seed does not touch it.

**The finding nobody had stated: probe AS fails too**, at **0.3097257 against 0.0041677 (74×)**, and
AZ's `hullAtten` sweep fails at **0.1763727**. AS is 2.2's QUAL-04/1-2 probe. Its position sweep
traverses the hull boundary, so from 2.3 onward **QUAL-04 criteria 1 and 2 rest on a Phase 2.3
mechanism**. Anyone who later "simplifies" the seed away will see three probes fail across two
phases, which is the right outcome — but the dependency should be on the record rather than
rediscovered.

### NC2 — H5's permanent-silence latch is real, and P29 is what closes it

With the `jlimit`/`isfinite` guard removed, BH reports
`trimDb 1e30 w=0: NON-FINITE` and `clamp: xinf vs ±24 dB — NOT CLAMPED`. RESEARCH-2.3's H5 was
not a theoretical hazard.

Two sub-results worth keeping, both exactly as `PluginProcessor.cpp`'s comment predicts: **`trimDb = NaN`
stays finite even unguarded** (`NaN > -100.0f` is false, so `decibelsToGain` returns `0.0f`) and
**`trimDb = -1e30` stays finite** (gain 0). **Only the `+inf` path kills the plugin.** The guard is
therefore not redundant on two of its three inputs — it is the only thing standing on the third.

### NC3 — FUNC-07's multiply is exactly one line, and BF measures all four of its criteria

Dropping `* snapshot.trimLin[i]` makes **BF** fail on every clause at once:
`-12 dB on lane 4: x1.00000 (want x0.25119) WRONG`, `6 dB … WRONG`, `trim step 0.0000000`, and
`venue-scoped: ratio 1.000000 -> 1.000000 — THE TRIM MOVED WITH THE MUSICAL STATE`. BH fails
alongside it (the clamp becomes unobservable). BF is not vacuous.

### NC4 — the D2 amendment's payoff, and a cross-phase dependency

Removing the `d_hull > 0` conjunct makes **BD** report
`inside: air 0/0.35/1.0 first diff ch0 @1 … filtered samples 16384 — FILTER RAN INSIDE THE HULL`.
DSP-07 criterion 6 — the amendment's entire point — rests on BD and BD measures it.

**Probe Q′ fails too**, at `max |out − in| = 0.319313765`. Q′ is 2.2's FUNC-01 unity probe. It
passes today *because* the default patch is bit-transparent inside the hull. **The D2 amendment is
therefore load-bearing for a Phase 2.2 requirement**, not only for 2.3's DSP-07/6.

### NC5 — H4 was a correctness bug, not defensiveness

Restoring the literal `20000` ceiling makes **AU** report `ceilings WRONG; PAST NYQUIST`. At 22.05
and 32 kHz the cutoff would sit above Nyquist, where `tan(π·fc/fs)` is negative and the one-pole is
not a lowpass at all.

### NC6 — QUAL-04/3's attribution is corrected: **AY is the gate, AZ alone is vacuous**

Reverting `widthMetres` to the 2.2 literal makes **AY** fail with
`width=4: 0/8 pairs separated, worst 0.0000000 — WIDTH IS NOT REACHING THE SHAPER`.

**But AZ passes** — `width [0.0..6.0] 0.0004728/0.0041677`. Of course it does: with `width` wired to
nothing, the swept render is bit-identical to the held one, the measured step is zero, and zero is
under any bound. `SUMMARY-2.3.md` proposed QUAL-04 criterion 3 on "AZ's `width` sweep". **That is
half the evidence.** QUAL-04/3 = **AY (the control is live) + AZ (and it does not zipper)**, and the
criterion is written that way below. This is the same class of correction as 2.2's NC3 (AI is not
AJ) — two probes that look interchangeable in a results table and are not.

### NC7 / NC8 — BE's two halves, and why half 2's pass is over-determined

Removing P31's per-block guard makes **BE** fail half 1: `active-filter NaN LATCHED`. DSP-07/8 rests
on the guard, and BE half 1 measures it.

**Half 2 — the "skipped-filter hole" — still reported `re-entry clean` under NC1 (seed removed, guard
present) and under NC7 (guard removed, seed present). It only fails under NC8, with both removed**, where
it reports `RE-ENTERED POISONED`.

The construction is sound and the hole is real: half 2 poisons the filter while it is **active**, so
in the shipping build the per-block output guard cleans the state at the end of that very block,
before the source ever moves inside. **Two independent mechanisms close the same hole, and half 2's
pass cannot attribute the closure to either one.** That is defence in depth, not a defect — but
`SUMMARY-2.3.md`'s framing ("P27 closes DSP-07's skipped-filter NaN hole for free", "probe BE probes
both") over-reads what a green BE proves in the shipping configuration. Stated correctly: **BE half 1
measures P31. Half 2 measures the pair, and NC8 is what separates them.**

---

## Requirements Verification

**Stage:** stage-2 phase 2.3 · **Requirements on this phase's traceability line:** 6

| Requirement | Priority | Status | Evidence |
|---|---|---|---|
| **FUNC-07** — per-speaker calibration trim | should | ✅ Complete | BF (all four criteria), BH (the latch the multiply would have armed). **NC3** proves BF non-vacuous |
| **DSP-06** — stereo sub-point geometry | should | ✅ Complete | AY (1), AX (2, 3, 5), BA (4). **NC6** proves AY non-vacuous |
| **DSP-07** — outside-hull distance processing | should | ✅ Complete *(under §3.5.2 as amended by D2, with the H2 erratum)* | AU (4), AV (1–3), BD (2, 5, 6, 7 — P33 method), BE (8). **NC4, NC5, NC7** |
| **DSP-08** — room-size-independent blur | should | ✅ Complete | AW, with a positive control at the clamp (Δ1.844e-3 at `blur=1, λ=2.1`) |
| **QUAL-01** — no audio artifacts | must | ✅ Complete *(under the D3/D4 scope as corrected by P34; audibility clause carried to D5)* | AS (11) + AZ (5) + BC (1) = 17/17 for criterion 1; BB + BD for criterion 2 |
| **QUAL-04** — no zipper noise | should | ✅ **Complete — 2.2's partial cleared** | criterion 3 by **AY + AZ** (NC6) |

**Requirements Summary — this phase:** ✅ Complete: **6** · ⚠️ Partial: **0** · ❌ Failed: **0**

### QUAL-01's audibility clause, named rather than absorbed

Criterion 2 reads "*rapid puck movement across the hull boundary produces no audible discontinuity*".
Measurement closes everything about it that measurement can:

- **The entry edge is bit-exact** — BB, both tones, and NC1 shows the claim fails loudly if the seed
  is dropped.
- **The exit step matches the predicted `A·|H_20k(f) − 1|` to 0.000 %** at 1 kHz (0.005328) and
  8 kHz (0.046267) — an equality against a predicted number, not a ceiling.
- **The DC path is continuous** — BD, 12 crossings, worst step 0.0009319 against 0.0041677.

What measurement **cannot** conclude is whether ~15 % of an 8 kHz component, as a one-sample step on
a deliberate gesture, ticks audibly on HF-rich material. That is D5's H2 item and it is carried, not
closed. If it does tick, `RESEARCH-2.3` H3 names the lever (raising `fc(d_hull = 0)` toward Nyquist),
and pulling it re-tunes the whole musical curve — so it is a **Stage-3 discuss** finding, not a fix.
The requirement is ticked ✅ because every measurable clause is met and the residual is a named,
bounded, single-gesture artifact with a documented owner.

### DSP-07 criteria 2 and 5 — the method survives verify as a method

P33's structural proof was re-read at verify and re-run, not restated. `hullTrimGain(0, d)` is
`bitExact(…, 1.0f)` over 201 swept `d` and 13 `hullAtten` (AV); `airSamplesFiltered == 0` counts the
never-taken branch (BD); and each half carries its non-vacuity control (BD: 16384/16384 filtered and
the render differs at `airAmount = 0.35` outside). **This is not "bit-transparency ✓"** — there is no
filter-absent build to render against, and fabricating one would mean a second arithmetic path
selected by a compile flag, which §3.4.3 forbids. The `ScopedNoDenormals` precondition remains stated
rather than implicit.

---

## Deviations from `PLAN-2.3.md` — all three reviewed and accepted

| # | Deviation | Verdict at verify |
|---|---|---|
| 1 | **P27** — `reset(x)` at the air edge, beyond §3.5.2's two named policies | **Accepted.** It satisfies D2's own stated rationale (don't discard continuity) better than D2's own choice, and NC1 shows the alternative is measurably worse across three probes and two phases. No checksum moved — this is an implementation choice inside §3.5.2's intent |
| 2 | **P29's position guard** — a pre-existing 2.2 hazard folded into a 2.3 site | **Accepted as a recorded scope addition.** Same loop, same site, no extra cost; NC2 confirms the trim half is this phase's and the NaN-coordinate half is genuinely pre-existing. Recorded rather than slipped, which is the standard this stage has held three times |
| 3 | **P25** — `HullProcessor.h` header-only against ROADMAP's `.{h,cpp}` | **Accepted.** The P14 `VenueGeometry.h` precedent, second use. Gate 11 confirms the consequence it was chosen for: zero CMake churn and no `juce_dsp` on the fast target's link line |

---

## Issues Found at verify

None is a defect in delivered code.

1. **`SUMMARY-2.3.md` attributes QUAL-04/3 to AZ alone.** NC6 shows AZ passes with `width` wired to
   nothing. The criterion is written here as **AY + AZ**. *(Corrected in `REQUIREMENTS.md`.)*
2. **`SUMMARY-2.3.md` over-reads a green probe BE.** Half 2's pass is over-determined in the shipping
   build — P31's guard and P27's seed independently close the same hole, and only NC8 separates them.
   *(Recorded here and in `REQUIREMENTS.md`'s DSP-07 note.)*
3. **Probe AS's position figure moved, and it is a real behavioural change, not drift.**
   VERIFICATION-2.2 recorded **0.0008846**; verify measures **0.0008203** (negative control
   0.0564730 → 0.0524102). The weights half is unchanged to the last digit (0.0016529 / 0.1057403).
   The cause is 2.3 going live: `hullAtten` defaults to **1.0 dB/m**, so a full-range position sweep
   now crosses the hull and picks up the trim and the air stage. NC1 corroborates it — the same probe
   reads 0.3097257 when the seed is removed. **QUAL-04 criteria 1 and 2 are still met, at 20 % of the
   bound.**
4. **Gate 1's cold-configure warning count is 40 again** — 39 repo-wide `JUCE_BUNDLE_ID` messages
   (none O-Octagon's) plus 1 concurrentqueue deprecation, 0 compiler diagnostics. Unchanged from 2.2
   issue 1; re-recorded so it is not read as new.

---

## Residual — open beyond 2.3

1. **The D5 manual Logic gate — OPEN, carried to Stage 3 discuss.** ~15 minutes, folding in 2.2's
   Task 12. Fresh VST3 + AU installed at verify and `auval`-clean. Corroboration for width / air /
   trim / lockstep / `w3 = 0`; **the H2 HF-rich hull-crossing item is the one claim that only
   listening can settle** (QUAL-01/2's audibility clause). Everything else it covers already has a
   non-vacuous probe.
2. **CI gap** — unchanged since 2.1. All 62 probes fire only under `-DOUARICON_BUILD_TESTS=ON`
   locally; no test target in this repo has ever run in CI. Stage 4.
3. **`COMPAT-04`** — retroactive acceptance criteria owed at Stage 4 (found at 2.2 verify: it is
   ticked `complete` at stage-1 against no criteria at all).
4. **`FUNC-06` and `UI-02..05`** — summary rows with no acceptance criteria, owed at **Stage 3
   discuss, before Stage 3 plan**. This stage has now repaired the same defect three times (PERF-02
   and QUAL-04 at the 2.2 boundary; FUNC-07, DSP-06, DSP-07 and DSP-08 at the 2.3 boundary).

---

## Why the verdict is VERIFIED and not PARTIAL

A partial would be right if a requirement on this phase's line were unmet, or if a probe carrying one
were vacuous. Neither holds:

- **All six requirements close ✅**, and the two probes that *could* have carried a requirement
  vacuously were caught by negative controls at verify and their attributions corrected before the
  verdict, not after.
- **Every load-bearing claim has a measured failure path.** Eight controls, eight loud diagnostic
  failures, and the tree proved byte-identical afterwards.
- **The one genuinely open item is a listening session**, and its only unique coverage is a clause
  that measurement is structurally incapable of concluding. It is bounded (one sample, on a
  deliberate gesture), its magnitude is measured to 0.000 % of prediction, and its remedy has a named
  owner at a Stage-3 discuss boundary. Blocking Stage 3 on it would gate a UI phase on a DSP-tuning
  question that a Stage-3 discuss is the correct place to raise.

---

## Next

**Stage 2 is complete.** The stage-level `VERIFICATION.md` in this directory closes it.

**Stage 3 (GUI)** — carry into its discuss phase: the D5 session, the `FUNC-06` / `UI-02..05` criteria
debt, `pattern_render_harness_breaks_on_webview_editor` (the `createEditor` guard is already present
and gate 9 keeps it), and `critical_juce_string_char_ctor_is_ascii_only` for every UI string.
