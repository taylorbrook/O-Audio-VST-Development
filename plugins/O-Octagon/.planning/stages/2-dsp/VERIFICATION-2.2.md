# Stage 2 — DSP · Phase 2.2 (DBAP Solve and Gain Application) — Verification

**Plugin:** O-Octagon
**Stage:** 2 of 4 — DSP
**Phase:** 2.2 of 3 — DBAP Solve and Gain Application
**GSD phase:** verify
**Date:** 2026-08-11
**Branch:** `feat/o-octagon`
**Verifies:** FUNC-01, FUNC-03/3, DSP-01, DSP-02, DSP-04/3, DSP-05, PERF-01, PERF-02, QUAL-02,
QUAL-03, QUAL-04

---

## Verdict

**✅ VERIFIED.** Ready for Phase 2.3: **yes**, with no blockers.

Every automated gate was **re-run from scratch at verify** against a forced full recompile, not read
out of `SUMMARY-2.2.md`. All ten passed; 46 probes, 0 failures; none of A–U regressed. Ten
requirements close ✅ and one closes ⚠️ partial — the partial **declared at discuss**, not discovered
here.

**Four negative controls were run as new work at verify.** Execute *asserted* that its load-bearing
gates work. Verify **measured** it: each of the four claims that carry a requirement on their own was
broken deliberately, and each gate failed loudly and diagnostically. One of the four also corrected a
piece of evidence attribution — see NC3.

**The Task 12 manual Logic gate (D4) remains OPEN and is carried to 2.3 verify**, by decision at this
boundary. It is corroboration, not the gate for FUNC-03/3 — NC3 below establishes that probe AJ is
that gate and that it is non-vacuous. This mirrors Stage 1's gate, which closed at 2.1 verify.

---

## Entry check — contract checksums

Recomputed at this boundary, not read out of a prior document
(`pattern_promotion_checksum_pins_replaced_file`). Obligation **C6**.

| Contract | Recomputed SHA-256 | vs `STATUS.md` |
|---|---|---|
| `BRIEF.md` | `697a4f32890d7420cdef85bafbf8fe45775bf805cf1ff7b449ed2c14f6b9fbd6` | ✅ byte-exact |
| `parameter-spec.md` | `b45f88dc5017ec2c1a9da49ba35242d01903000a4ff199d16758e1b6cbb9e02f` | ✅ byte-exact |
| `research/ARCHITECTURE.md` | `cd881a10e16fc5600845bdc9569cfdca21003bfe7202823162b0d8084b10861b` | ✅ byte-exact (D2 re-pin) |
| `ROADMAP.md` | `aec7d0ce0db9ad6c78cb1c9e9574a0a2f8ddb1cf258e6e4b701f2e2e0137ee29` | ✅ byte-exact |

No drift. The D2 re-pin issued at the 2.2 discuss boundary is the value in force, and the superseded
`bff8a83b…` is recorded in frontmatter. `BRIEF.md`, `parameter-spec.md` and `ROADMAP.md` have never
been edited.

---

## Goal-Backward Analysis

### Original goals (from `CONTEXT-2.2.md`)

1. **Make audio spatialise.** Implement the DBAP solve per the 2011-04-14 revised equations, in 3D,
   and apply the resulting eight gains through the Phase 2.1 channel map.
2. **Close the two criteria 2.1 staged forward** — DSP-04/3 (`rakeRear` moves the gain vector) and
   FUNC-03/3 (a label change moves audio to the corresponding physical output).
3. **Write the general path in its degenerate configuration, not a stub** (D1) — two sub-point slots
   and the full 17-smoother inner loop from day one, with `wEff` forced to 0.
4. **Establish an independent DBAP oracle** (D3/D4 carried from 2.1) — a Python reference emitting a
   self-contained committed fixture, so no mirrored coordinate or constant table can drift.
5. **Declare the one requirement that cannot fully close, at discuss** — QUAL-04/3 (`width`), with
   its destination named.

### Deliverables (from `SUMMARY-2.2.md`, confirmed by code inspection at verify)

1. `Source/DSP/DbapSolver.{h,cpp}` — §3.3 verbatim, raw inputs, four instrumentation counters;
   `Source/DSP/GainStage.{h,cpp}` — control grid, 17 smoothers, §3.6.4 inner loop;
   `Source/DSP/SourceShaper.{h,cpp}` — §3.4.1 steps 1–6 in full;
   `Source/Data/VenueGeometry.h` — free `earHeight`/`normToMetres` and the shared `kMinSpan`.
2. Probes AK (DSP-04/3, both halves) and AJ (FUNC-03/3, Layer 3).
3. `widthMetres` is a literal `0.0f` at `GainStage.cpp:147`, at a single `PHASE-2.3-WIDTH` marker;
   the solver, shaper and smoother path are the general ones.
4. `tests/tools/gen_dbap_reference.py` + committed `tests/fixtures/DbapReferenceFixture.h`, 102 cases.
5. QUAL-04/3 declared partial at discuss, before execute began.

### Goal achievement

| Goal | Status | Evidence at verify |
|---|---|---|
| 1 — audio spatialises | ✅ Achieved | Probes Y (1.0236e-7 vs oracle), AA (3.259e-7 over 7686 solves), AI (0 duplicate pairs of 28); **NC1** proves Y has teeth |
| 2 — close DSP-04/3, FUNC-03/3 | ✅ Achieved | AK (rear delta 0.00522166, front exactly 0.0); AJ (8 renders, exact dominant bins); **NC3** proves AJ has teeth |
| 3 — general path, not a stub | ✅ Achieved | `GainStage.cpp:147` read at verify — the degeneracy is one caller-side literal, the solver/shaper/smoothers are general |
| 4 — independent oracle | ✅ Achieved | Generator read at verify: `exp(−a·log d)` not `pow`, explicit Σv² not `k=1/√denom`, double precision, constants restated. **NC2** proves the fixture is load-bearing |
| 5 — declare the partial at discuss | ✅ Achieved | QUAL-04/3 named in `CONTEXT-2.2.md` with destination 2.3; verify discovered nothing the plan did not predict |

---

## Automated Checks — all ten re-run at verify

Build: `rm -rf build/plugins/O-Octagon` then a full 119-step recompile of VST3 + AU + Standalone +
both test targets.

| # | Gate | Verify-phase result |
|---|---|---|
| 1 | Clean 3-format build + both test targets, forced TU recompile | ✅ exit 0, **zero compiler diagnostics** — see the note below |
| 2 | Hardcoded output channel indices outside `ChannelMap` | ✅ 2 hits, **both INPUT reads** bounded by `numIn` (§3.4.3); the one output write is `speakerToBuffer[i]` at `GainStage.cpp:227` |
| 3 | `PHASE-2.3-WIDTH` / `-AIR` / `-TRIM` each once; `PHASE-2.2-REPLACE` zero | ✅ **1 / 1 / 1 / 0** |
| 4 | `auval -a` then `auval -v aufx OuOc OuDv` | ✅ **AU VALIDATION SUCCEEDED**, exit 0, 17 × `-parameter PASS` |
| 5 | pluginval strictness 10, VST3 ×3 and AU ×3 | ✅ **SUCCESS on all six**, exit 0 each, **zero `FAILED`** |
| 6 | Both test targets, `-DOUARICON_BUILD_TESTS=ON` | ✅ **29 + 17 = 46 probes, 0 failures**, exit 0 / exit 0 |
| 7 | `gen_dbap_reference.py --check` | ✅ exit 0 — 102 cases; the committed fixture matches its generator |
| 8 | 17 parameters vs `parameter-spec.md` | ✅ **17/17 across three independent sides** — see below |
| 9 | `setLatencySamples` call / `switch` on `ChannelType` / `createEditor` guard | ✅ absent / absent / `#if JUCE_WEB_BROWSER` present, both arms identical |
| 10 | `OOCTAGON_INSTRUMENT` never defined by the plugin target | ✅ **0** in the plugin `CMakeLists.txt`; 1 each in the two test targets |

### Gate 1, stated precisely — a correction to SUMMARY-2.2

`SUMMARY-2.2.md` records *"0 warning/error matches in the entire log."* At verify the same
case-insensitive grep returns **40 matches**. All 40 are CMake **configure**-stage output, and none
belongs to O-Octagon:

- 39 × `CMake Warning at JUCEUtils.cmake:1768` — `JUCE_BUNDLE_ID … contains spaces`, emitted once per
  plugin target across the whole repository (`OuariconAnalogEQ`, etc.);
- 1 × `CMake Deprecation Warning` from `_deps/concurrentqueue-src`.

Filtering to compiler diagnostics — `warning:`, `error:`, `FAILED` — returns **zero**. The claim is
true of the compile and link phases and was measured on a log that did not include a reconfigure.
Recorded so a future reader who reruns the gate on a cold build directory does not read 40 matches as
a regression. **No defect; the gate itself passes.**

### Gate 8, stated precisely

The comparison is three-sided and no side is hand-transcribed: the markdown tables in
`parameter-spec.md` are parsed with a regex; `createParameterLayout()` is parsed out of
`PluginProcessor.cpp`, including the `w1..w8` construction loop; and the **runtime** names are read
from the `auval` dump of the installed AU. All three agree on 17 parameters, and spec vs
implementation agree on id, display name, range and default for every one.

### Gate 5 — one benign warning, already on record

pluginval AU emits `!!! WARNING: Current program is -1... Is this correct?` on every run. JUCE
AU-wrapper reporting, present repo-wide, recorded benign at Stage 1 and at 2.1 verify. All six runs
still return `SUCCESS` with exit 0 and no line containing `FAILED`.

---

## Negative controls — new work at verify

Execute asserted these gates work. A gate never observed to fail is a gate whose failure path is
untested. Each defect was injected, the affected target rebuilt and run, then reverted and the tree
proved byte-identical to its pre-injection state; the final green run is 46 probes / 0 failures.

### NC1 — the DBAP oracle catches a real DSP-01 defect

`DbapSolver.cpp:60`, `const float dz = spk[i].z − src.z` → `0.0f`. This is exactly the defect DSP-01
criterion 2 exists to catch, and it is invisible on a flat rig.

| Probe | Green | With the z term dropped |
|---|---|---|
| **Y** — vs the Python oracle | 1.0236e-7 | **0.39153065** (`hull-vertex-spk5`) — FAIL |
| **Z** — `srcZ` moves the vector | 8/8 lanes moved | **0/8 lanes, max delta 0.000000** — FAIL |
| **AA** — `Σ v² = 1` | 3.2590e-7 | 3.5154e-7 — **still PASSES** |

Unit target exit 1. **The AA row is the finding.** `Σ v_i² = 1` is a normalisation invariant: it holds
whatever the distances are, because `k = 1/√denom` is computed from the same distances it normalises.
DSP-02's gate is therefore structurally incapable of catching a DSP-01 defect, and the two
requirements are not redundant coverage of each other. DSP-01 rests on Y and Z alone.

### NC2 — the committed fixture is load-bearing, and the gate is tight

One gain in `DbapReferenceFixture.h` perturbed by 1e-4 (`0.30819058…` → `0.30829058…`).

- `gen_dbap_reference.py --check` → **exit 1**, `FAILED --check … was hand-edited`.
- Probe **Y** → **FAIL at exactly `1.0000e-4`** (`inside-centre`).

So probe Y genuinely reads the committed fixture rather than recomputing an expectation
(`pattern_review_recomputes_instead_of_measuring`), and the 1e-5 hard gate catches an error four
orders of magnitude below the NC1 defect. Both directions of `pattern_test_fixture_mirrors_drift_silently`
are covered: the generator catches a hand-edited fixture, and the fixture catches a changed solver.

### NC3 — probe AJ is FUNC-03/3's gate, and probe AI is not

`GainStage.cpp:227`, `buffer.getWritePointer (snapshot.speakerToBuffer[i])` → `getWritePointer (i)`.
This is R1 itself: the map bypassed, silent, audible only in the hall.

| Probe | Green | With the map bypassed |
|---|---|---|
| **AJ** — Layer 3 | 8/8 tones in their own lane, exact bins | **FAIL** — all 8 misrouted, every dominant bin off by one position (`lane1 bin 128 != 64` …) |
| **Q′** — unity through the addressed lane | 6.0e-8 | **FAIL at 0.499429643**, loudest other lane 0.499429703 |
| **AI** — eight independent lanes | 0 duplicate pairs, 0.00398982 | **PASSES, bit-identically** |

Render target exit 1. **AI is invariant under a channel-map permutation** — it asks only whether the
eight lanes carry distinct signal, and permuting distinct signals leaves them distinct. AI is
therefore correct evidence for FUNC-01/3 (*independent, non-duplicated*) and **no evidence at all**
for FUNC-03/3 (*audio moves to the corresponding physical output*).

`SUMMARY-2.2.md` attributes them correctly — FUNC-01 to "AI + AJ", FUNC-03/3 to "AJ (Layer 3,
mandatory)". This control confirms that attribution rather than correcting it, and it is recorded
because the two probes look interchangeable in a results table and are not.

### NC4 — the H1 generation stamp is measured, not merely argued

`GainStage.cpp:138`, the `snapshot.generation == lastSolvedGeneration` term removed from the dirty
check — the H1 failure mode the P16 deviation exists to prevent.

- **AQ** → **FAIL**, `gain vector moved by 0.00000000 — THE DIRTY CHECK IS STALE (H1 has regressed)`.
- **AP** → **FAIL**, venue-edit solves **1 → 0**.

Render target exit 1. The one deviation whose justification is a hazard argument rather than an
observation now has a measured failure path on both of its probes.

---

## Requirements Verification

**Stage:** 2 (DSP), phase 2.2. Eleven requirements on this phase's traceability line.

| Requirement | Priority | Status | Evidence |
|---|---|---|---|
| **FUNC-01** — 8-channel transport, independent feeds | must | ✅ **Complete** | Criterion 3 closes: AI (0 duplicate pairs of 28, closest lanes 0.00398982) + AJ. Criteria 1–2 met at stage-1 |
| **FUNC-03** — label map | must | ✅ **Complete** | Criterion 3 closes: AJ, 8 renders on a non-identity map, each tone only in its own lane, all 8 dominant bins exact. **NC3** |
| **DSP-01** — DBAP, 3D, revised eqs | must | ✅ Complete | X (α at R=3/4/6, err ≤ 2.65e-8); Z (8/8 lanes move on `srcZ`); **Y — 1.0236e-7 over 102 cases, meeting DSP-01's own 1e-6**. **NC1, NC2** |
| **DSP-02** — constant intensity | must | ✅ Complete | AA — max \|Σv²−1\| = 3.2590e-7 over **7686** solves across rolloff × blur, inside and outside; AB (subsets), AD (degenerate rigs) |
| **DSP-04** — sloped audience plane | must | ✅ **Complete** | Criterion 3 closes: AK, **both halves** — rear source delta 0.00522166, front source exactly 0.0000000000 |
| **DSP-05** — speaker weights | must | ✅ Complete | AB (muted lane exactly `0.0f`; 2-speaker subset Σv² = 0.9999999754); AC (all-zero → exactly 0.0f ×8, not NaN, not full scale) |
| **PERF-01** — real-time safe | must | ✅ Complete | AO — **0 allocations across 66 `processBlock` calls**, measured; AP — inside projections 0. **Read the method note below** |
| **PERF-02** — solve scheduling and smoothing | should | ✅ Complete | AP (idle solves 0 / pow 0; param-change 1 / pow 16); AE (**pow == 16**, not merely ≤ 32); AT (`sampleAdvances == totalSamples`, both modes) |
| **QUAL-02** — no NaN or Inf | must | ✅ Complete | AC, AD, AR — **parameter** NaN as well as input NaN, neither latches |
| **QUAL-03** — block-size invariance | must | ✅ Complete | AL (512 vs 4096, mandated), AM (ragged `1,7,64,333,4096` — the stronger gate), AN (6 regimes). **`memcmp`, never a tolerance** |
| **QUAL-04** — no zipper noise | should | ⚠️ **Partial (2/3)** | AS closes criteria 1–2: bound 0.0041677, position 0.0008846, weights 0.0016529; negative control 0.0564730 / 0.1057403, **both over**. Criterion 3 (`width`) → 2.3 |

**Summary:** ✅ Complete **10** · ⚠️ Partial **1** · ❌ Failed **0**

**DSP-08 is implemented at 2.2 but is NOT ticked here.** Probe AG (blur 0.50 → 1.9829 m against
§3.3.2's 1.98, scaling invariance, 8 m cap) is supporting evidence only; the traceability table
assigns DSP-08 to 2.3 and it stays `pending`.

### QUAL-04's partial is real, and verified as such

`GainStage.cpp:147` was read at verify: `const float widthMetres = 0.0f;`. `p[params::width]` is not
read anywhere in the solve path. A width sweep at 2.2 would therefore produce a flat-zero output
delta and **pass vacuously** — it would report green while testing nothing. Deferring criterion 3 to
2.3 is the correct call, it was made at discuss with the destination named, and this document
confirms the premise rather than accepting it on assertion.

### PERF-01 criterion 1 — the method, not just the verdict (H8)

Reproduced from `SUMMARY-2.2.md` because it must not be compressed into "RT-safety harness pass":

| Sub-claim | How established |
|---|---|
| No **allocation** in `processBlock` | **MEASURED.** Probe AO replaces the entire global `operator new` family — plain, `[]`, both `std::align_val_t` overloads and every matching `operator delete` — with counting `malloc`/`posix_memalign` wrappers, warmed up with one un-counted block first. **0 allocations over 66 calls**, including an 8192-sample over-size block and a mid-stream venue edit |
| No **lock**, no **file I/O** | **GREP + INSPECTION ONLY.** Not measured |
| `-fsanitize=realtime` | **UNAVAILABLE** — unsupported by Apple clang 17.0.0, verified by running it |

The criterion's own wording says *"verified by inspection and an RT-safety harness pass"*. Allocation
meets the harness standard; locks and file I/O meet the inspection standard only. Ticked on that
basis, with the gap stated rather than papered over.

---

## Deviations from `PLAN-2.2.md` — all three reviewed and accepted

1. **`VenueSnapshot::generation` moved inside the payload** *(planned, P16)* — beyond ARCHITECTURE
   §3.6.6's field list. Same class as 2.1's `hullEpsCross`. Accepted: two separate acquires let a
   `publish()` landing between them pair new geometry with the old generation, which is then stored
   as `lastSolvedGeneration` so every later block compares equal — the venue edit is present in the
   snapshot and **the solve never runs against it, permanently**. **Measured by AQ, and NC4 proves
   AQ fires.**
2. **`SourceShaper` created at 2.2** *(planned, P15)* — ROADMAP places it at 2.3, but 2.2 cannot solve
   without §5 step 2. §3.4.1 implemented in full; the degeneracy is one caller-side literal at the
   `PHASE-2.3-WIDTH` marker. Accepted.
3. **`OOctagonProcessor::getAPVTS()` added** *(NOT in the plan)* — the harness needs the synchronous
   `setValueNotifyingHost` write path (RESEARCH Q1) and `apvts` was private. Accepted: it widens the
   public API, but it is the accessor name used by 12+ sibling plugins and Stage 3.1's WebView relays
   need it regardless. No behaviour changed.

None touches a contract checksum or an acceptance criterion.

---

## Findings carried from execute — re-confirmed at verify

- **F1 — `juce::String (const char*)` is ASCII-only.** Confirmed against `juce_String.h:88-94`;
  `operator+=` appends through `CharPointer_UTF8` (`juce_String.cpp:773`). `detail = "… — …"` renders
  as `â`; `detail << "… — …"` is correct, and there is **no compiler warning**. Already a standing
  memory (`critical_juce_string_char_ctor_is_ascii_only`); it will matter again when Stage 3 puts µ,
  ×, Σ or ° into UI strings.
- **F2 — the §OQ4 rig is not exactly mirror-symmetric in float32.** Independently confirmed at verify
  by reading the committed fixture: the `inside-centre` case gives speakers 5 and 6
  `0.34508078669450237` and `0.34508078897598921` — a real asymmetry in double, visible in the 9th
  significant figure, currently below one float ULP. Probe AF correctly holds bit-identity only for
  the three genuinely exact pairs, `0.5f`/`12.5f`; pinning (5,6) to bit-identity would pin a rounding
  accident.
- **F3 — the puck cannot leave the speaker bbox, and `(0,0)` is a hull VERTEX** which `isInside`
  (inside-*or-on*) accepts, so no projection fires there. AP searches for an outside position and
  asserts it found one before relying on it.
- **F4 — a helper leaving only its last block must return the absolute offset.** Q′ initially failed
  at 0.96 with routing provably perfect, purely from a head-vs-tail window mismatch.
- **F5 — `posix_memalign`, not `std::aligned_alloc`** — libc++ gates the latter behind a feature
  macro, and a probe that fails to *build* silently stops running.

---

## Issues Found at verify

1. **`SUMMARY-2.2.md` Gate 1 overstates its scope** — "0 warning/error matches in the entire log" is
   true of compile and link, but a cold-configure log carries 40 CMake configure-stage matches, none
   of them O-Octagon's. **Resolution:** stated precisely above; no code change. The gate passes.
2. **AI and AJ are not interchangeable evidence** (NC3). AI survives a full channel-map bypass
   unchanged. **Resolution:** no change needed — `SUMMARY-2.2.md` already attributes FUNC-03/3 to AJ
   alone. Recorded so no future summary collapses them into "the routing probes".
3. **DSP-02 cannot backstop DSP-01** (NC1). `Σ v² = 1` holds under a dropped z term. **Resolution:**
   no change needed; recorded so the 7686-solve figure is not read as broad correctness coverage.
4. **`COMPAT-04` is ticked `complete` with no acceptance criteria** — found by a criteria audit at
   this boundary. **Resolution:** not re-opened at 2.2 (it is Stage 1's requirement and Stage 1's
   `VERIFICATION.md` carries the observation); logged as residual item 5 for Stage 4.

None is a defect in the delivered code.

---

## Residual — open beyond 2.2, deliberately

1. **Task 12 — the manual Logic gate (D4). Still OPEN; carried to 2.3 verify.** Two observations to
   record verbatim, positive or negative: (a) automate `srcX` and confirm the 8 surround-meter lanes
   **no longer move in lockstep** — the direct contrast with 2.1; (b) set `w3 = 0` and confirm that
   lane goes **silent** while the others compensate. Freshly built VST3 and AU are installed and
   `auval`-clean. Not a blocker: FUNC-03/3's gate is probe AJ, shown non-vacuous at NC3.
2. **QUAL-04 criterion 3** (`width`) → 2.3, declared at discuss and confirmed non-testable here.
3. **CI gap** — unchanged from 2.1. No test target in this repo has ever run in CI, so all 46 probes
   and the build-failing Layer-2 checksum fire only for whoever configures with
   `-DOUARICON_BUILD_TESTS=ON`. Logged at
   `.planning/todos/pending/2026-08-11-secrets-free-test-ci-workflow.md`; belongs to Stage 4.
4. **Owed at 2.3 discuss, before 2.3 plan** — FUNC-07, DSP-06, DSP-07 and DSP-08 have summary rows in
   `REQUIREMENTS.md` and **no acceptance criteria**. 2.3 verifies four requirements; written now, or
   it verifies against nothing. This is the defect discuss found for PERF-02 and QUAL-04 at this
   boundary, and it is still live for 2.3's own set. FUNC-06 and UI-02..05 are in the same state for
   Stage 3.
5. **New at verify — `COMPAT-04` is marked `complete` and has no acceptance-criteria section.**
   A criteria audit at this boundary found nine requirements with a summary row and no `###` section:
   FUNC-06, FUNC-07, DSP-06, DSP-07, DSP-08, UI-02..05 — all `pending`, all covered by item 4 — **and
   COMPAT-04, which is already ticked `complete` at stage-1.** That is the PERF-02/QUAL-04 defect one
   step further along: it did not merely risk verifying against nothing, it *did*. Not re-opened here
   — COMPAT-04 is Stage 1's stereo-fallback policy and Stage 1's `VERIFICATION.md` records the
   observation that closed it — but criteria must be written retroactively at Stage 4, when COMPAT-02
   and "all remaining" are verified, so the tick has a standard behind it.

None of the five blocks Phase 2.3.

---

## Why the verdict is VERIFIED and not PARTIAL

Stage 1 closed PARTIAL because a requirement was found **at verify** to have been mis-staged. 2.1
corrected that by declaring its partials in the plan. 2.2 went one step further and declared
QUAL-04's partial at **discuss**, before research or planning — and verify found exactly that
partial, no more and no fewer.

Every gate passed on a forced full recompile. The one item genuinely open at execute is a human
corroboration step whose automated counterpart was not merely run but *broken on purpose and observed
to fail*. Three of the four negative controls confirm existing evidence; the fourth sharpened how two
probes should be cited without changing any result.

The phase delivered what it set out to deliver: **audio spatialises, through the map, provably.**

---

## Next

**Phase 2.3 — Source Shaping + Outside-Hull.** Verifies FUNC-07, DSP-06, DSP-07, DSP-08, QUAL-01, and
closes QUAL-04/3.

Inherits, and must not re-derive:

- **The four acceptance-criteria gaps** — FUNC-07, DSP-06, DSP-07, DSP-08 must have criteria written
  at 2.3 discuss, before 2.3 plan.
- **`PHASE-2.3-WIDTH` / `-AIR` / `-TRIM`** — each exactly once today; 2.3's gate is **zero
  occurrences**, and prose must not quote the literal tokens or the retirement gate never reaches zero.
- **`SourceShaper` already implements §3.4.1 in full.** 2.3 makes `wEff` live by replacing one
  caller-side `0.0f`; it does not write the shaper.
- **NC1's lesson** — `Σ v² = 1` is a normalisation invariant and cannot backstop a distance-math
  defect. Any new solve path needs its own oracle comparison, not an AA extension.
- **The Task 12 Logic gate**, carried forward to 2.3 verify.

The stage-level `VERIFICATION.md` is written only at the close of 2.3.
