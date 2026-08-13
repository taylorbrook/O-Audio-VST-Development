# Stage 2 — DSP · Phase 2.3 (Source Shaping and Outside-Hull Processing) — Context

**Plugin:** O-Octagon
**Stage:** 2 of 4 — DSP
**Phase:** 2.3 of 3 — Source Shaping and Outside-Hull Processing
**GSD phase:** discuss
**Date:** 2026-08-11
**Branch:** `feat/o-octagon` @ `a47cef88` (2.2 work uncommitted)
**Participants:** Taylor Brook, Claude

---

## Entry Check — carried obligations from Phases 2.1 and 2.2

The standing obligation at every boundary: *"Re-verify all four checksums — a checksum that silently
points at the wrong file is worse than no checksum, because it reports green."*
(`pattern_promotion_checksum_pins_replaced_file`)

**Re-run at this boundary, before anything else. All four byte-exact on arrival:**

| Contract | SHA-256 on arrival | Result |
|---|---|---|
| `BRIEF.md` | `697a4f32…9fbd6` | ✅ matches STATUS frontmatter |
| `parameter-spec.md` | `b45f88dc…b9e02f` | ✅ matches |
| `research/ARCHITECTURE.md` | `cd881a10…10861b` | ✅ matches (the D2 re-pin issued at 2.2 discuss) |
| `ROADMAP.md` | `aec7d0ce…7ee29` | ✅ matches |

**No drift on arrival.** `ARCHITECTURE.md` was then deliberately amended by decision **D2** below and
its pin re-issued — `cd881a10…` → **`a8a358f4be0ea1834da1540c01550c438d4c445dfbfe3d79478df0429b6d4408`**.
The other three are untouched. This is the second use of the mechanism (2.2 discuss corrected
`rigScale`); a phase boundary is exactly where the "don't edit a checksummed contract mid-phase"
objection does not apply.

---

## Discussion Summary

2.3 is the last DSP phase. It completes the chain: `width` goes live, the hull distance that
`solveSubPoint` already computes and discards starts driving a trim and a filter, and the per-speaker
trims that have been riding in the snapshot since 2.1 finally get multiplied in.

**The phase is unusually small in code and unusually sharp in measurement.** 2.2 deliberately shipped
the general path in its degenerate configuration (decision 2.2/D1), so the functional diff is close to
three lines plus a filter pair:

| Marker (each appears exactly once in `Source/`, asserted at 2.2) | What 2.3 does there |
|---|---|
| `GainStage.cpp:147` `PHASE-2.3-WIDTH` | `const float widthMetres = 0.0f;` → `p[params::width]` |
| `GainStage.cpp:174` `PHASE-2.3-AIR` | consume `d_hull`, apply the trim, set the two cutoffs |
| `GainStage.cpp:182` `PHASE-2.3-TRIM` | `* snapshot.trimLin[i]` on both target lines |

All three must read **zero** at 2.3 — the `PHASE-2.2-REPLACE` mechanism from Stage 1, which worked,
and which needed two rounds at 2.2 because prose comments quoting the literal token would have
survived the real marker's retirement.

**`SourceShaper` is already complete** (2.2/P15): §3.4.1 steps 1–6 written in full, the degeneracy
living in the caller's literal `0.0f`. Probe AH already drives `width > 0` directly in the unit target
as coverage for code shipped inert — explicitly **not** a DSP-06 claim. So DSP-06 closes at 2.3 mostly
by *measuring* code that has been there and correct for a phase.

The architecture was re-opened in exactly one place, §3.5.2, and the amendment is an extension of that
section's own reasoning rather than a reversal of it (D2).

---

## Requirements Confirmed

`ROADMAP.md` Phase 2.3 assigns **FUNC-07, DSP-06, DSP-07, DSP-08, QUAL-01**, plus **QUAL-04
criterion 3** carried from 2.2. That closes Stage 2 with every DSP requirement ticked.

### The criteria debt owed at this boundary is CLEARED

STATUS carried an explicit obligation: *"Criteria owed at 2.3 discuss, before 2.3 plan — FUNC-07,
DSP-06, DSP-07, DSP-08."* All four had a summary-table row and **no acceptance criteria at all** —
the same defect repaired for PERF-02 and QUAL-04 at the 2.2 boundary, and the same defect that
COMPAT-04 demonstrates in its terminal form (ticked `complete` at Stage 1 against nothing).

**Written now, before plan, in `REQUIREMENTS.md`:**

| Req | Criteria | Derived from |
|---|---|---|
| FUNC-07 | 4 | ROADMAP 2.3, ARCHITECTURE §5 step 7, §6.2, the `PHASE-2.3-TRIM` marker |
| DSP-06 | 5 | ROADMAP 2.3, ARCHITECTURE §3.4.1–§3.4.3 |
| DSP-07 | 8 | ROADMAP 2.3, ARCHITECTURE §3.5.1–§3.5.2 **as amended by D2** |
| DSP-08 | 4 | ROADMAP 2.3, ARCHITECTURE §3.3.2, §OQ4 |

Coverage re-checked programmatically: **30 summary rows, 24 sections, zero sections without a row.**
The six rows still lacking criteria are exactly the known, dated debt — `FUNC-06` and `UI-02..05`
(Stage 3) and `COMPAT-04` (retroactive, Stage 4). **No new gap, and the `### DSP-04` duplicate-heading
defect repaired at 2.2 stays repaired** (heading uniqueness re-asserted: 30 headings, 0 duplicates).

### Two criteria are worth calling out because they are non-obvious

- **DSP-08's room-size independence is an INVARIANT, not a constant.** Scaling every speaker
  coordinate about the centroid by λ must scale `r_s` by λ at fixed `blur`, leaving the gain vector
  unchanged. A bare `rigScale ≈ 7.93` assertion is a mirrored fixture
  (`pattern_test_fixture_mirrors_drift_silently`) — and this specific number is the one the
  architecture got wrong twice, so a constant-valued probe here would have been "fixed" by tuning the
  code to reproduce 7.95.
- **DSP-06 criterion 1 is a regression gate, not a new feature gate.** `width = 0` must remain
  bit-identical to the 2.2 shipping behaviour once `width` goes live. It is the criterion most likely
  to break and the least likely to be noticed.

---

## Approach Decisions

| # | Decision | Choice | Rationale |
|---|---|---|---|
| **D1** | `HullProcessor` file shape | **JUCE-free free functions `hullproc::hullTrimGain()` / `hullproc::airCutoffHz()`; the two `FirstOrderTPTFilter`s live in `GainStage` beside the smoothers** | The `hull::` / `dbap::` / `shaper::` precedent from 2.1 and 2.2 — free functions over raw values, no JUCE, so the narrow `tests/unit/` link line survives. DSP-07's cutoff-curve table gate then runs in the **fast** unit target; only genuine filter behaviour needs the render harness. A `HullProcessor` class owning the filters would drag `juce_dsp` in and push even the pure arithmetic into the slow target |
| **D2** | `ARCHITECTURE §3.5.2` air-filter skip condition | **Amend to `airAmount · d_hull == 0`, and re-pin** | §3.5.2 already argues that `fc = 20 kHz` is not a transparent bypass — it just applied that argument to one of the two axes that can zero the effect. `d_hull = 0` for every source **inside** the hull, which is the common case and the default patch; the TPT one-pole's zero at Nyquist puts ≈ −3 dB at 20 kHz and ≈ −0.7 dB at 10 kHz there **with no distance to justify it**. Under the original wording the plugin is bit-transparent only at `airAmount = 0` — never on the shipping default of 0.35 |
| **D3** | How QUAL-01 measures `airAmount` | **Differential 1 kHz sine probe against a held-parameter reference, with a negative control** | QUAL-04's DC method is **exactly blind** here, not merely weak: a one-pole passes DC unchanged in steady state, so an air sweep moves a DC output by literally zero and the probe passes vacuously. Compare `max abs(out[n] − out[n−1])` under a full-speed sweep against the same measurement with `airAmount` held — self-calibrating against the signal's own slew. Negative control: update `fc` only every 4096 samples, which must exceed the bound. Structurally identical to probe AS, which fires at 13–25× |
| **D4** | QUAL-01 probe scope | **17 musical parameters + the hull crossing + a FUNC-07 trim step + one representative live venue edit during playback** | QUAL-01 is a statement about automation and only APVTS parameters ride a lane. Venue values are message-thread edits, and everything they touch lands on the same smoothed targets — one live-edit probe covers the mechanism. Sweeping all 42 at speed stresses a path no user can drive |
| **D5** | Manual Logic gate | **One combined session at 2.3 verify** — 2.2's carried Task 12 (a: automate `srcX`, 8 lanes no longer in lockstep; b: `w3 = 0` → that lane silent, others compensate) **plus** 2.3's items (width audibly spreads; air audibly dulls outside the hull; a per-speaker trim moves one lane only) | ~15 minutes once, at the point where the whole chain exists. 2.2's items are corroboration, not the requirement's gate — FUNC-03/3's gate is probe AJ, shown non-vacuous by VERIFICATION-2.2 NC3 |

### D2 in full — what was traded for what

```
// §3.5.2 as originally written
airActive = (airAmount > 0);          // inside hull → fc = 20 kHz → permanent top-octave tilt

// as amended
airActive = (airAmount > 0 && d_hull > 0);
// reset() fires ONLY on the airAmount → 0 transition, never on a d_hull == 0 block
```

**Gained:** the default patch is bit-transparent. The stage is also free inside the hull, which is
where the source spends most of its life.

**Paid, and named rather than hidden:** crossing the hull boundary now **steps** the transfer
function instead of sliding it — bounded at 3 dB @ 20 kHz, 0.7 dB @ 10 kHz, and **0 dB at DC**. That
last figure is the important one: it means QUAL-01 criterion 2 (*"rapid puck movement across the hull
boundary produces no audible discontinuity"*) **cannot be measured with a DC probe** — the one
discontinuity this amendment introduces is invisible to the method 2.2 established. Criterion 2 is
therefore measured on both excitations: DC for the gain vector, 1 kHz sine for the filter. This is a
measurement obligation accepted deliberately, recorded here so plan cannot skip it and verify cannot
discover it.

**Not chosen:** resetting on every skip. A puck oscillating across the hull edge would re-zero the
filter state repeatedly — a self-inflicted QUAL-01/2 hazard, and the re-entry corner is 20 kHz so the
resident state is stale by at most one control block and by an inaudible amount.

---

## Constraints Identified

1. **The three `PHASE-2.3-*` markers must go to zero, and prose must not resurrect them.** At 2.2 the
   first pass had each token appearing twice because comments quoted it. The retirement gate counts
   occurrences in `Source/`; a mention in a comment keeps it non-zero.
2. **`d_hull` currently escapes.** `solveSubPoint` computes `projection.distance` inside the
   outside-hull branch and discards it, by design and with a comment saying so. 2.3 must return it —
   and must return **0** on the inside path, not leave it uninitialised.
3. **`hullproc` must stay JUCE-free** (D1) or the fast unit target loses the DSP-07 curve gate.
4. **Nothing may be elided under a `width == 0` branch.** §3.4.3 forbids it explicitly: a branch that
   changes the arithmetic path between control boundary A and boundary B is the class of bug QUAL-03
   exists to catch. The second solve stays unconditional. `powCalls == 16` must still hold exactly.
5. **The air filter is per **sub-point**, not per speaker** — 2 instances, applied to the source
   signal before the gain matrix. Per-speaker would be 4× the cost and wrong (§3.5.2).
6. **The per-sample invariant survives:** all 17 `getNextValue()` called exactly once,
   unconditionally, in both SAFE and REAL modes. The air filter's skip branch sits **outside** that
   rule — it gates `airL.processSample` / `airR.processSample`, not any smoother advance.
7. **H7 input aliasing is unchanged and still load-bearing.** `out[0]` and `in[0]` are the same
   memory; `s_L`/`s_R` are read at the top of each sample's iteration before any output write. The air
   filter inserts *after* that read and before the gain matrix, so it does not relax the rule.
8. **No new parameters.** `width`, `hullAtten`, `airAmount` are already in the 17 (`parameter-spec.md`
   #4, #15, #16); trims are venue values. The 17/17 programmatic parameter gate must still pass.
9. **`juce::juce_dsp` must be added to `target_link_libraries`** — the only DSP module dependency in
   the plugin (ARCHITECTURE §F7). It is not linked today.
10. **`stages/2-dsp/` is still untracked — do NOT execute 2.3 in an isolated worktree**
    (`pattern_worktree_isolation_wrong_for_untracked_scope`).

---

## Open Questions for Research

1. **`FirstOrderTPTFilter` state inspection.** DSP-07's sticky-NaN criterion needs a per-block
   `std::isfinite` check on *state*, but JUCE 8.0.14 exposes `s1` privately. Is the check written
   against the filter's **output** instead, and is that equivalent? (`processSample` returns the
   output; a NaN in `s1` surfaces on the next sample regardless.) Answer against source, with
   file:line.
2. **`setCutoffFrequency` cost per control block.** It calls `std::tan`. Two calls per control block
   is ~1500 tan/second — confirm, and confirm it is not called per sample. Does it need a
   counter alongside `powCalls` so PERF-01 stays a number rather than an argument?
3. **Does `setCutoffFrequency` reset or perturb state?** If it touches `s1`, the D2 "skip without
   reset" design needs to know, and a cutoff change mid-stream may itself be the discontinuity D3 is
   trying to measure.
4. **`prepare()` on the two filters vs. `GainStage::prepare`'s "one reset site, ever" rule (P23).**
   Where does `juce::dsp::ProcessSpec` come from, and does adding filter preparation violate the
   single-reset-site discipline or extend it?
5. **The exact bit-transparency claim.** DSP-07 asserts `airAmount = 0` and `d_hull = 0` are each
   bit-identical to the filter absent. With the skip in place that is true by construction — but
   confirm nothing else on the path (e.g. a `processSample` call with a unity coefficient, or a
   denormal flush) breaks bit-identity, and confirm `dbToGain(-0.0f) == 1.0f` exactly on this
   toolchain rather than assuming it.
6. **Differential sine probe tolerance (D3).** What is the right `tol`, derived rather than tuned?
   The held reference and the swept render differ by the filter's own response change; the bound must
   be tight enough that the 4096-sample negative control fails it. Establish the numbers before plan.
7. **QUAL-01 criterion 2's sine-excitation form.** A hull crossing at 1 kHz — the filter's response at
   1 kHz barely moves between fc = 20 kHz and fc = 13 kHz, so is 1 kHz sensitive enough to *see* the
   D2 step, or does criterion 2 need a higher probe tone (8 kHz) while criterion 1 keeps 1 kHz?
   This decides whether D3's method transfers or needs a second form.
8. **`trimLin` provenance.** It has been carried in the snapshot since 2.1 and applied nowhere. Is it
   populated from the venue store today, or is it currently all-ones and FUNC-07 needs the
   `VenueModel` → snapshot plumbing as well as the multiply?
9. **Sub-points straddling the hull boundary** (ROADMAP criterion). One in, one out means one filter
   active and one skipped under D2, with different gains. Is there a continuity hazard at the moment
   one sub-point crosses while the other does not — and does `wEff`'s centroid collapse interact,
   since a wide source near the hull edge is exactly the configuration that produces it?
10. **Venue live-edit probe mechanics (D4).** 2.2's probe AQ publishes a venue edit *between* two
    `processBlock` calls to measure the generation stamp. Can the same rig drive a continuity
    measurement, or does QUAL-01's live-edit probe need edits landing at control-grid-aligned offsets
    the way QUAL-03's protocol does?

---

## Confirmed available — do not rebuild

- **`shaper::shape()` is complete and correct**, §3.4.1 steps 1–6, already exercised at `width > 0` by
  probe AH. 2.3 changes its **caller**, not it.
- **`hull::project()` already returns the distance** — `projection.distance` **is** `d_hull`. Nothing
  new to compute; it is thrown away today at a documented line.
- **`VenueSnapshot::trimLin[8]`** exists (`VenueSnapshot.h:42`) and rides the same publication path as
  everything else. Subject to open question 8.
- **`instr::` counters** (`powCalls`, `solveRuns`, `hullProjections`, `sampleAdvances`) are live under
  `OOCTAGON_INSTRUMENT`, defined only by the two test targets — verified 0 in the plugin target at
  2.2 verify. A fifth counter costs nothing.
- **Probe AS's differential/negative-control structure** is the template for D3. It is proven to fire.
- **46 probes (A–AT) must all still pass.** Q′ and AI–AT are the ones `width` going live can break.

---

## Predicted Outcomes — declared here, not to be discovered at verify

The 2.1 discipline, applied a third time.

| Req | Expected at 2.3 verify |
|---|---|
| FUNC-07 | ✅ complete — pending open question 8; if `trimLin` is unpopulated the plumbing is in scope, not a deferral |
| DSP-06 | ✅ complete |
| DSP-07 | ✅ complete, **under §3.5.2 as amended** |
| DSP-08 | ✅ complete — implemented at 2.2, ticks here per the traceability table |
| QUAL-01 | ✅ complete under the D3/D4 scope and method now written into `REQUIREMENTS.md` |
| QUAL-04 | ✅ **completes** — criterion 3 (`width`) closes, clearing 2.2's partial |

**Stage 2 then closes with a stage-level `VERIFICATION.md`** (2.1 and 2.2 wrote phase-suffixed ones
only). No requirement is expected to carry past 2.3 except the three known residuals below.

### Residuals that 2.3 does not close, stated now

1. **CI gap** — all probes fire only under `-DOUARICON_BUILD_TESTS=ON`; no test target in this repo
   has ever run in CI. Logged at `.planning/todos/pending/`; Stage 4.
2. **`COMPAT-04` criteria** — owed retroactively at Stage 4.
3. **`FUNC-06`, `UI-02..05` criteria** — owed at Stage 3 discuss, before Stage 3 plan.

---

## Contract Corrections at This Boundary

**One contract amended, one document repaired.**

1. **`ARCHITECTURE.md` §3.5.2 — the air-filter skip condition** (D2). Amended in place with an
   explicit dated callout naming what was gained, what was paid, and what was rejected; §5 step 6's
   diagram gains the `airActive` line so the processing chain and the prose cannot disagree.
   **Checksum re-pinned `cd881a10…` → `a8a358f4…`**; the superseded value goes to STATUS frontmatter
   alongside 2.2's. Every artifact dated 2.2 or earlier was verified against `cd881a10…` and that
   remains correct; 2.3 onward verifies against `a8a358f4…`.
2. **`REQUIREMENTS.md` — the four missing criteria sections** (FUNC-07, DSP-06, DSP-07, DSP-08),
   plus a scope-and-method note under QUAL-01 recording D3 and D4. `REQUIREMENTS.md` is not a
   checksummed contract; no pin moves.

`BRIEF.md`, `parameter-spec.md` and `ROADMAP.md` are **unmodified** — their pins have never changed
across three phases.

---

## Next Phase

Ready for: **research**. Ten open questions, all answerable against JUCE 8.0.14 source and by running
the toolchain. Questions 1–5 gate the implementation shape; 6–7 gate whether QUAL-01 can be measured
at all; 8 may put FUNC-07 plumbing in scope.
