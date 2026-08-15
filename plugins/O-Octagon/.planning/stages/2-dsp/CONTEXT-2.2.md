# Stage 2 — DSP · Phase 2.2 (DBAP Solve and Gain Application) — Context

**Plugin:** O-Octagon
**Stage:** 2 of 4 — DSP
**Phase:** 2.2 of 3 — DBAP Solve and Gain Application
**GSD phase:** discuss
**Date:** 2026-08-11
**Branch:** `feat/o-octagon` @ `a47cef88`
**Participants:** Taylor Brook, Claude

---

## Entry Check — carried obligations from Stage 1 and Phase 2.1

The standing obligation at every boundary: *"Re-verify all four checksums — a checksum that silently
points at the wrong file is worse than no checksum, because it reports green."*
(`pattern_promotion_checksum_pins_replaced_file`)

**Re-run at this boundary, before anything else. All four byte-exact on arrival:**

| Contract | SHA-256 on arrival | Result |
|---|---|---|
| `BRIEF.md` | `697a4f32…9fbd6` | ✅ matches STATUS frontmatter |
| `parameter-spec.md` | `b45f88dc…b9e02f` | ✅ matches |
| `research/ARCHITECTURE.md` | `bff8a83b…06cfe` | ✅ matches |
| `ROADMAP.md` | `aec7d0ce…7ee29` | ✅ matches |

**No drift on arrival.** `ARCHITECTURE.md` was then deliberately amended by decision **D2** below and
its pin re-issued; see *Contract corrections*. The other three are untouched and 2.2 plans against
them exactly as 2.1 did.

---

## Discussion Summary

Phase 2.2 is the phase where audio actually spatialises. Like 2.1 it inherits an unusually closed
architecture: §3.3 fixes every constant, the blur mapping, the distance floor and the all-zero-weight
guard; §3.6 fixes the control grid, the smoothing time and the per-sample inner loop line by line;
§5 fixes the processing order. Discuss did **not** re-open any of that.

What it settled is the seam between 2.2 and 2.3, and three things the contracts leave to execution.
It also **found and repaired two defects in the planning documents themselves** — one in a
checksummed contract, one in `REQUIREMENTS.md` — both of which would have produced a green verify
against a wrong or absent standard.

---

## Requirements Confirmed

**Phase 2.2 verifies** *(per ROADMAP Stage 2 → Phase 2.2 and the REQUIREMENTS traceability table)*:

| Req | What closes here |
|---|---|
| **FUNC-01** | criterion 3 — all 8 outputs carry **independent, non-duplicated** signal for an off-centre source. The re-mapped criterion from Stage 1; this is the phase it was re-mapped *to* |
| **DSP-01** | DBAP per the 2011-04-14 **revised** equations, in 3D: `a = R/(20·log10 2)`, `d_i` carrying the `(z_i − z_s)²` term, gains matching an independent reference to 1e-6 |
| **DSP-02** | `Σ v_i² = 1 ± 1e-6` — inside the hull, outside it, at hull vertices, at exact speaker positions, across the full rolloff × blur product |
| **DSP-05** | `w_i = 0` → exactly zero at speaker *i*; a 2-speaker subset still `Σ v_i² = 1` at unchanged output level; **all-zero weights → silence, not NaN, not full-scale** |
| **PERF-01** | no allocation, lock or file I/O in `processBlock`; `pow` on parameter change, not per sample; hull projection only when outside |
| **PERF-02** | the 64-sample grid, the skip-when-unchanged dirty check, the ≤ 32 `pow`/block budget, the exactly-once `getNextValue()` invariant |
| **QUAL-02** | finite output at every speaker coordinate with `blur = 0`, at both rolloff ends, on degenerate venues, and under pathological input with **no sticky NaN** |
| **QUAL-03** | block-size invariance under the §3.6.3 protocol — `memcmp`, not a tolerance |
| **DSP-04 /3** | changing `rakeRear` alone changes the **gain vector** for a rear source *(staged here at 2.1 plan)* |
| **FUNC-03 /3** | changing a label row moves audio to the corresponding physical output *(staged here at 2.1 plan)* |

### Declared partial AT DISCUSS — not to be discovered at verify

This is the discipline 2.1 established after Stage 1's FUNC-01 mis-staging was found at verify. One
requirement cannot fully close at 2.2, and it is named now with its destination:

| Req | Outcome expected at 2.2 verify | Why |
|---|---|---|
| **QUAL-04** | ⚠️ **partial** — criteria 1–2 (position, weights) close here; criterion 3 (**`width`**) → **2.3** | `width` is not read at 2.2 by decision D1; `wEff` is forced to 0. A width sweep at 2.2 would be a test of a parameter that is wired to nothing — it would pass vacuously |

`REQUIREMENTS.md`'s traceability table has been updated to carry `QUAL-04/3` on the 2.3 row.

### Not verified in this phase

FUNC-02, FUNC-04, FUNC-05, FUNC-06 (stage 3) · **FUNC-07, DSP-06, DSP-07, DSP-08, QUAL-01** (2.3) ·
UI-01..05 (stage 3) · COMPAT-02 (stage 4). COMPAT-01, COMPAT-03, COMPAT-04, DSP-03 are already
complete and are re-run at 2.2 as **regression** gates only, not as new evidence.

---

## Approach Decisions

| # | Decision | Choice | Rationale |
|---|---|---|---|
| **D1** | Gain-stage shape at 2.2, given that §3.6.4's inner loop assumes two sub-points but SourceShaper is 2.3 | **Two sub-point slots from day one, `wEff` forced to 0.** The full 17-smoother inner loop is written verbatim from §3.6.4; the solver runs twice on coincident points; the `width` parameter is not read | §3.4.3 proves this is not a stub: at `width = 0` the two sub-points *coincide*, `v_L ≡ v_R` bit-for-bit, and the sum degenerates to `v_i · 0.5·(L+R)` — the mono-sum case, reached with **no branch**. So 2.2 ships the general path in its degenerate configuration, and 2.3 only has to make `wEff` live. The inner loop and its exactly-once `getNextValue()` invariant are written once and never rewritten. The alternative (9 smoothers now, 17 at 2.3) re-derives that invariant at 2.3 — and §3.6.4 is explicit that a desynchronised smoother produces a slow, position-dependent gain error **no single-parameter test will find** |
| **D2** | `ARCHITECTURE §OQ4`'s `rigScale ≈ 7.95 m`, known wrong since 2.1 verify (true value 7.93165 m) | **Correct it now, at this boundary, and re-pin the checksum** | 2.2 is the phase that first *consumes* `rigScale` — via the blur mapping `r_s = blur · 0.5 · rigScale`. Leaving a number known to be wrong inside the contract 2.2 tests against is the mirrored-fixture trap in its purest form (`pattern_test_fixture_mirrors_drift_silently`). 2.1 deferred it for a good reason — editing a checksummed contract mid-phase invalidates pins that phase re-verifies — but a phase boundary is precisely where that objection does not apply. Cost: one re-pin in STATUS frontmatter |
| **D3** | What geometry the D4 Python DBAP reference solves against | **A self-contained fixture.** The generator writes the speaker array, weights, rolloff, blur and source positions it used *into* the fixture; the C++ test constructs a `VenueModel` **from the fixture** and then compares gains | Zero coupling to `VenueModel`'s defaults, so venue-default drift cannot silently invalidate the DBAP reference — there is no mirrored coordinate table anywhere to drift. The §OQ4 defaults remain pinned independently by the existing 2.1 probes (M, N, O), which is where that assertion belongs. Rejected: parsing `VenueModel.cpp` (a second source-parsing script to maintain, and it couples the DBAP fixture to venue edits) and hardcoding §OQ4 (two copies of the table, with an assertion standing between them and silence) |
| **D4** | Human gate before 2.2 verify closes | **Yes — a Logic surround check, ~10 min**, run before verify closes, as Task 13 was for Stage 1 | 2.2 produces the first genuinely spatialised audio, and the strongest evidence for FUNC-01/3 is a human watching 8 meters move *differently*. Two observations: (a) automate `srcX` across the room and confirm the 8 surround-meter lanes **no longer move in lockstep** — the direct contrast with 2.1, where identical signal on all 8 was correct by design; (b) set `w3 = 0` and confirm that speaker's lane goes silent while the others compensate (DSP-05, and the audible half of `Σ v_i² = 1`) |

### D1 consequence — what `width` does at 2.2

`width` remains an APVTS parameter (it is one of the 17 and must stay, or the parameter-spec gate
fails) but nothing reads it. This is stated here so that neither research nor plan treats "the
`width` knob does nothing" as a defect to fix. It is DSP-06, at 2.3.

### D4 consequence — FUNC-03/3 is closed by the automated test, not by the human gate

FUNC-03 criterion 3 reads *"confirmed by verify-ping"* — but verify-ping is FUNC-04, at Stage 3. At
2.2 the evidence is **channel-map Layer 3**: render with `w_j = δ_ij` and a unique tone per speaker,
then assert each output channel's dominant FFT bin is exactly its speaker's frequency. The ROADMAP
calls Layer 3 *mandatory — the layer that catches a reintroduced hardcoded index*. The Logic check
is corroboration, not the gate.

---

## Contract corrections made at this boundary

Both were found during discuss by reading the contracts against each other and against the code.
Both would have produced a green verify against a wrong or absent standard.

### 1. `ARCHITECTURE.md` — `rigScale` (decision D2) — **checksum re-pinned**

- §OQ4 `rigScale ≈ 7.95 m` → **`rigScale = 7.93165 m`**, annotated with an inline correction note
  recording the prior value, the reason, and the superseded checksum
- §3.3.2's header line, and its blur table entry at `blur = 0.50`: **1.99 m → 1.98 m**
  (`0.5 · 0.5 · 7.93165 = 1.98291`). The `0.40` and `3.97` entries are unchanged — and `3.97` is now
  *correct*, where against 7.95 it should have read 3.98
- Centroid and bbox in §OQ4 were already exact; only their displayed precision increased, to
  `(6.5000, 12.4625, 4.9250)`

Recomputed independently at discuss from §OQ4's own coordinate table — RMS **3-D** radius from the
centroid — reproducing 7.93165 and the centroid to all printed digits. This is the third independent
derivation of the figure (execute, verify NC4, discuss) and the first time the contract agrees.

| | Before | After |
|---|---|---|
| `contract_checksums.architecture` | `sha256:bff8a83b…06cfe` | `sha256:cd881a10e16fc5600845bdc9569cfdca21003bfe7202823162b0d8084b10861b` |

`grep` confirms no `7.95` survives in any of the four contracts except inside the correction note
itself. **`BRIEF.md`, `parameter-spec.md` and `ROADMAP.md` are unmodified** — their pins stand.

### 2. `REQUIREMENTS.md` — two defects, not checksummed, repaired in place

- **`### DSP-04` appeared twice.** The first copy carried the 2.1 verify results (2 of 3 criteria
  ticked, criterion 3 staged to 2.2); the second was the original, all three unticked. A verify pass
  reading the file top-down could land on either. The stale duplicate is **deleted**; heading
  uniqueness is now asserted (`sort | uniq -d` → empty)
- **PERF-02 and QUAL-04 had summary-table rows but no acceptance criteria at all.** Both are on
  2.2's traceability line — they would have been "verified" against nothing. Criteria are now
  written, **derived from `ROADMAP.md` Phase 2.2 and `ARCHITECTURE.md` §3.6.2 / §3.6.4 / §3.6.5 /
  §3.3.5**, not invented, and each section is marked with when and why it was added
- QUAL-03's section gained a pointer to the §3.6.3 protocol, so a reader cannot apply the
  unqualified wording that §3.6.3 explains is untestable

> **Still absent, and owed at 2.3 discuss:** FUNC-07, DSP-06, DSP-07 and DSP-08 have summary rows and
> no acceptance-criteria sections. They are 2.3's requirements, so they are 2.3's obligation — but
> they must be written **before** 2.3 plan, for exactly the reason above. UI-02..05 and COMPAT-04
> are in the same state and belong to Stage 3 / Stage 4.

---

## Constraints Identified

### Inherited verbatim — do not re-derive

- **G1 — `mappedOutputAvailable(int)`.** A valid channel map is **not** evidence of an 8-channel
  buffer. Under the F3 hazard the layout reports 7.1 while the buffer holds `n < 8`; `mapInvalid`
  stays false and `speakerToBuffer` still holds indices up to 7. `GainStage` **calls the existing
  helper** — it does not re-derive the condition, and it does not bound by
  `getTotalNumOutputChannels()`, which is the accessor that lies in exactly that state
- **C1 — Layer 3 must drive non-identity label maps.** All three accepted 8-channel containers have
  initializer order == enum-bit order, so a container-swap test is vacuous
  (`critical_audiochannelset_is_a_bitset_not_an_order`)
- **DBAP per the 2011-04-14 revised equations.** The original paper's eqs 3–6 and 9–10 are wrong
  (`pattern_dbap_not_vbap_for_irregular_arrays`)
- **The exactly-once invariant (§3.6.4).** No `continue`, no early exit, no `if (w[i] == 0) skip` in
  the per-sample loop, ever
- **Do not add a width-dependent branch to elide the second solve** (§3.4.3) — a branch that changes
  the arithmetic path between block boundary A and block boundary B is the exact class of bug QUAL-03
  exists to catch. Under D1 this is doubly binding: the second solve is *already* redundant at 2.2,
  and it still must not be optimised away
- **`stages/2-dsp/` work is untracked-file-heavy — do NOT execute 2.2 in an isolated worktree**
  (`pattern_worktree_isolation_wrong_for_untracked_scope`)

### Available from 2.1 — already built, do not rebuild

- `hull::isInside (pts, count, p, epsCross)` and `hull::project (pts, count, p)` are **free functions
  over raw storage**, callable directly against `snapshot.hullPts` / `hullCount` / `hullEpsCross`
  from the audio thread. §5 step 4 needs precisely this, and 2.1's `hullEpsCross` snapshot deviation
  exists to make it possible without a second derivation
- `VenueSnapshotPublisher` carries `spk`, `trimLin`, `speakerToBuffer`, `centroid`, `rigScale`,
  `bbMinX/MaxX/MinY/MaxY`, `rakeFront/rakeRear` and `getGeneration()` — the dirty check's venue half
  is already published
- The 17 raw parameter atomics are already cached in `PluginProcessor.h` as the control-grid snapshot
  source. They are the reason that constructor is "complete" at Stage 1

### Environment

- **No unit-test framework in this repo** — `juce_add_console_app` + `check()` + exit codes. The
  Catch2 references in `docs/codebase/TESTING.md` describe an intent never implemented
  (`project_no_unit_test_framework_ci_never_runs_tests`)
- **No test target in this repo has ever run in CI.** 2.2's gates are local-only, same as 2.1's. The
  gap is logged at `.planning/todos/pending/2026-08-11-secrets-free-test-ci-workflow.md` and belongs
  to Stage 4 — it is not re-litigated here
- Test dirs stay gated behind `OUARICON_BUILD_TESTS` (default **OFF**)

---

## Open Questions — handed to research

1. **How does the render harness write a parameter at an exact absolute sample offset?** §3.6.3's
   protocol requires programmatic writes at control-grid-aligned offsets between `processBlock`
   calls. Determine which API actually updates the cached `std::atomic<float>*` **synchronously**, in
   a console app with no message loop being pumped — `setValueNotifyingHost`, `AudioProcessorParameter
   ::setValue`, or writing through `apvts.getParameter()`. If any path defers through a listener, the
   protocol silently becomes non-deterministic and QUAL-03's `memcmp` gate turns flaky rather than
   failing honestly.
2. **`earHeight()` on the audio thread.** §5 step 3 needs `z = earHeight(P.y) + srcZ` at control
   rate, but `earHeight()` is a message-thread `VenueModel` method and the snapshot carries
   `rakeFront/rakeRear/bbMinY/bbMaxY` rather than the *coefficients* §5's message-thread box implies.
   Recomputing the line in the solver is a **second derivation of a message-thread quantity** — the
   drift the snapshot exists to prevent, and the exact argument that justified 2.1's `hullEpsCross`
   deviation. Decide: shared free function over the four published values, or precomputed slope /
   intercept in the snapshot. The `kMinSpan` zero-span guard must survive either way.
3. **`absoluteSampleCounter` lifecycle.** Where is it reset (`prepareToPlay`?), what happens on a
   host locate or loop, and does it overflow? QUAL-03's bit-identity requires both renders to start
   from an identical counter state — if the reset point differs between the 512 and 4096 runs, the
   grid dephases and the gate fails for a reason that has nothing to do with the DSP.
4. **The dirty check's comparison.** Exact bitwise compare of the 17-float snapshot is right for a
   skip gate, but confirm the behaviour when a host writes a NaN or a denormal into a parameter
   (`x != x` never compares equal, so the solve would run every block — degraded, not incorrect, but
   worth knowing). Confirm the venue generation counter is part of the same comparison.
5. **SAFE mode at 2.2.** §5 says steps 1–7 still execute in full in SAFE mode "so the UI stays live",
   but there is no UI until Stage 3. Decide whether the solve runs or is skipped when
   `mappedOutputAvailable()` is false — this path is exercised by `auval` at the (1,1), (1,2), (2,1)
   and (2,2) configs (RESEARCH F2), so it is load-bearing for COMPAT-01, not hypothetical.
6. **`pow` instrumentation without polluting the shipping binary.** PERF-02 requires a *measured*
   `pow`-per-block count. Determine where the counter lives and how it compiles to nothing in
   Release — the natural candidate is a compile-time switch set only by the test targets.
7. **FFT for channel-map Layer 3.** Does `tests/render-harness/` link `juce_dsp`, and what tone set /
   FFT length makes "the dominant bin is *exactly* its speaker's frequency" a true statement rather
   than a leakage artefact? Bin-centred frequencies are the obvious answer; confirm against the
   harness's sample rate and block length.
8. **Fixture format and dimensions for D3.** What the self-contained fixture carries, how positions
   are sampled (a pinned grid, per `pattern_forward_grain_read_is_coherent`-style determinism), and
   whether the Python generator is re-run at build time or the committed artifact is the source of
   truth. Given finding 3 above and the CI gap, the committed fixture is expected to win — confirm.

---

## Next Phase

Ready for: **research** — `/plugin-research O-Octagon 2-dsp`

Research inherits eight questions, four settled decisions, one amended contract with a new pin, and
a `REQUIREMENTS.md` that now has something to verify PERF-02 and QUAL-04 against.
