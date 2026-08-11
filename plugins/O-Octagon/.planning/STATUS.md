---
plugin: O-Octagon
stage: 2
stage_phase: "2.1"
stage_phase_name: geometry-core
stage_phase_total: 3
artifact_suffix: "-2.1"
phase: verify
status: phase_complete
last_updated: 2026-08-11
branch: feat/o-octagon
complexity_tier: 6
complexity_score: 5.0
research_depth: DEEP
staged_implementation: true
orchestration_mode: true
next_action: discuss_stage_2_phase_2.2
next_stage: 2
ready_for_implementation: true
stage_1_open_manual_gate: CLOSED
stage_1_gate_closed_at: stage_2_phase_2.1_verify
logic_negotiated_container: create7point1
build_target: OuariconOctagon
plugin_code: OuOc
musical_parameter_count: 17
venue_value_count: 42
contract_checksums:
  brief: sha256:697a4f32890d7420cdef85bafbf8fe45775bf805cf1ff7b449ed2c14f6b9fbd6
  parameter_spec: sha256:b45f88dc5017ec2c1a9da49ba35242d01903000a4ff199d16758e1b6cbb9e02f
  architecture: sha256:bff8a83b379113ac8b1e2a8915d6f1edc7183558b992bdc3808877f86c406cfe
  roadmap: sha256:aec7d0ce0db9ad6c78cb1c9e9574a0a2f8ddb1cf258e6e4b701f2e2e0137ee29
---

# O-Octagon Status

## Current Position

Stage: 2 of 4 (DSP) — **Phase 2.1 of 3 (Geometry Core) ✅ VERIFIED**
Status: Phase 2.1 closed ✅ VERIFIED, no blockers. **Stage 1's open manual gate is now CLOSED** —
Task 1 (Logic 8-channel negotiation) ran and all four observations are recorded. Stage 2 runs one
full GSD cycle per roadmap phase; artifacts are phase-suffixed (`-2.1`, `-2.2`, `-2.3`).
Progress: `[##########..........]` 45%
Branch: `feat/o-octagon` (cut from `docs/logic-multichannel-dbap-research` @ 12ae50dd)

## Stage 2 Cycle Structure (decided at 2.1 discuss)

**One full discuss→research→plan→execute→verify cycle per phase**, not one pass over the stage. The
channel map is R1 (CRITICAL, silent failure, audible only in the hall) and gets its own verify
before any gain math exists to confuse a diagnosis.

| Phase | Name | Verifies | Status |
|-------|------|----------|--------|
| 2.1 | Geometry Core — `VenueModel`, `ConvexHull2D`, `ChannelMap`, `VenueSnapshot`, tests | COMPAT-03 ✅, DSP-03 ✅, DSP-04 ⚠️, FUNC-03 ⚠️ | **✅ VERIFIED** |
| 2.2 | DBAP Solve + Gain Application | FUNC-01, DSP-01/02/05, PERF-01/02, QUAL-02/03/04 | pending |
| 2.3 | Source Shaping + Outside-Hull | FUNC-07, DSP-06/07/08, QUAL-01 | pending |

Artifacts live in `stages/2-dsp/` with a phase suffix — `CONTEXT-2.1.md`, `RESEARCH-2.1.md`,
`PLAN-2.1.md`, `SUMMARY-2.1.md`, `VERIFICATION-2.1.md` — so the stage-level commands still resolve
`2-dsp`. A stage-level `VERIFICATION.md` is written only at the close of 2.3.

## Stage 2 Phase 2.1 Discuss Results (2026-08-11) — `stages/2-dsp/CONTEXT-2.1.md`

**Contract checksums re-verified at the stage boundary — all four byte-exact.** This was the
explicit carry-forward from Stage 1 issue 7 (`pattern_promotion_checksum_pins_replaced_file`).
No drift; 2.1 plans against these exact documents.

Four decisions settled; the architecture itself was not re-opened (ARCHITECTURE.md already resolves
OQ1–OQ5, both design defects, and every numeric default).

| # | Decision | Choice |
|---|----------|--------|
| D1 | Cycle granularity | One full GSD cycle per phase (2.1 → 2.2 → 2.3) |
| D2 | Task 13 (Logic 8-ch negotiation) | Run **before 2.1 execute** — discuss/research/plan do not depend on it |
| D3 | 2.1 test vehicle | Stand up **both** the `tests/` unit target **and** `tests/render-harness/` now (harness pulled forward from 2.2) |
| D4 | DBAP reference for 2.2's DSP-01 gate | **Python** script in `tests/tools/` emitting a committed fixture — a C++ reference beside the implementation mirrors the same misreading (`pattern_test_fixture_mirrors_drift_silently`) |

**D3 closes two Stage 1 issues a phase early:** issue 3 (unity gain confirmed by inspection only)
and issue 4 (the F3 3–7-channel hazard reasoned, not measured — the harness constructs those bus
layouts programmatically, no hardware needed).

**Five open questions handed to research:** what replaces the `PHASE-2.2-REPLACE` block at 2.1 with
no solver yet; how `gen_juce_channel_order.py` locates JUCE portably (CI does not use the local
tree); whether the unit target runs under CI; where hull classification surfaces before the Venue
screen exists; which unit-test framework.

## Phase Progress

### Stage 1: Foundation
| Phase | Status | Date |
|-------|--------|------|
| discuss | ✓ | 2026-08-11 |
| research | ✓ | 2026-08-11 |
| plan | ✓ | 2026-08-11 |
| execute | ✓ | 2026-08-11 |
| verify | ⚠️ PARTIAL | 2026-08-11 |

### Stage 2: DSP — Phase 2.1 (Geometry Core)
| Phase | Status | Date |
|-------|--------|------|
| discuss | ✓ | 2026-08-11 |
| research | ✓ | 2026-08-11 |
| plan | ✓ | 2026-08-11 |
| execute | ✓ | 2026-08-11 |
| verify | ✅ VERIFIED | 2026-08-11 |

## Stage 2 Phase 2.1 Verify Results (2026-08-11) — `stages/2-dsp/VERIFICATION-2.1.md`

**Verdict: ✅ VERIFIED. Ready for Phase 2.2: yes, no blockers.**

Every automated gate was **re-run from scratch at verify**, not read out of SUMMARY-2.1.md. All 12
passed; none regressed. Contract checksums recomputed — all four byte-exact (C6).

| Gate | Verify-phase result |
|---|---|
| Clean 3-format build + both test targets, forced TU recompile | ✓ exit 0, **zero warning/error matches in the whole log** |
| Hardcoded output indices outside `ChannelMap` | ✓ 3 hits, all doc prose; `processBlock` read line-by-line |
| `PHASE-2.2-REPLACE` | ✓ **0** |
| `auval -v aufx OuOc OuDv` | ✓ **AU VALIDATION SUCCEEDED**, 17 × `-parameter PASS` |
| pluginval s10, VST3 ×3 / AU ×3 | ✓ SUCCESS on all six, exit 0 each |
| Both test targets | ✓ 16 probes / 5 probes, **0 failures**, exit 0 / exit 0 |
| 17 params vs `parameter-spec.md` | ✓ **17/17** name, range, default AND group — compared programmatically, neither side hand-transcribed |
| `setLatencySamples` / `switch` on ChannelType / non-goal symbols | ✓ all absent |
| `createEditor` `#if JUCE_WEB_BROWSER` guard (G8) | ✓ present, provably inert |
| Test dirs gated behind `OUARICON_BUILD_TESTS` (default OFF) | ✓ |

### Negative controls — new work at verify, not run at execute

Execute *asserted* the Layer-2 gate works. Verify **measured** it. A gate never observed to fail is
a gate whose failure path is untested.

- **NC1 — the golden genuinely tracks parsed JUCE source.** Mutating `leftSurroundRear = 20 → 90` in
  a copied JUCE tree moved the generated SHA `5cd774cb…` → `39298098…`. The baseline is byte-identical
  to the committed constant. **Not a mirrored fixture.**
- **NC2 — divergence FAILS THE BUILD.** Substituting the mutated SHA produced
  `error: static assertion failed`, `BUILD_EXIT=1`. ROADMAP:131 requires the *build* to fail, not a
  test run. Reverted and rebuilt green before proceeding.
- **NC3 — the generator refuses a vacuous golden.** Bad `--juce-modules` → exit 1, no header; a JUCE
  tree parsing to zero enum entries → exit 1, no header; two runs byte-identical.
- **NC4 — `rigScale` recomputed independently** from §OQ4's own table: **7.93165 m**; centroid
  `(6.5000, 12.4625, 4.9250)` and bbox exact. F1 confirmed — the spec prose is wrong, not the code.

### New at verify

- **V1 — Logic negotiated `7.1` (`create7point1()`), NOT 7.1-SDDS. The Stage-0 R2 prediction is
  contradicted and is now retired.** R2 reasoned from `kAudioChannelLayoutTag_Emagic_Default_7_1`.
  Impact nil — all three containers are accepted, the map is keyed on `ChannelType` not on container,
  and nothing branches on the choice. But the prediction was being carried forward as fact into
  COMPAT-02 at Stage 4. Bonus: the container Logic actually uses is the **primary** one, not the
  least-exercised `create5point1point2()` VST3 path (F5).
- **V2 (benign)** — `outputGain` reads `Default = 0.0000007` in the auval dump. AU
  normalise→denormalise float round-trip of `0.0 dB` over `−24…+12`; within the 1e-6 tolerance.
  Recorded so nobody chases it.

### Human verification — Task 1 CLOSED, all four observations positive

- ✅ **All 8 surround-meter lanes moved** — proves negotiation and writability. **Not** independence:
  all 8 lanes carry identical signal at this phase by design. Independence is FUNC-01/3 at 2.2.
- ✅ **Container: `7.1`** (see V1)
- ✅ **`outputGain` survived save → close → reopen** — FUNC-05 slice confirmed in a real host
- ✅ **17 parameters under 5 groups** in the automation menu, matching the auval clump dump

### Requirements

| Req | Status |
|---|---|
| COMPAT-03 | ✅ **complete** — 3/3 criteria |
| DSP-03 | ✅ **complete** — 4/4 criteria |
| DSP-04 | ⚠️ **partial** — 2/3; criterion 3 (`rakeRear` changes the **gain vector**) → 2.2 |
| FUNC-03 | ⚠️ **partial** — 2/3; criterion 3 (a row edit moves audio) → 2.2 |

### Why VERIFIED and not PARTIAL

Stage 1 closed PARTIAL because a mis-staged requirement was discovered **at verify**. 2.1 is the
direct correction: both partials were declared in `PLAN-2.1.md` **before execute began**, with reason
and destination phase named. Nothing was discovered at verify the plan did not predict, no gate
failed, and the one genuinely open item — the Logic manual gate — is closed by this verify.

### Residual — open beyond 2.1, none blocking 2.2

1. **CI gap.** No test target in this repo has ever run in CI, so the Layer-2 build-failing checksum
   only fires for whoever configures with `-DOUARICON_BUILD_TESTS=ON`. **A JUCE bump performed
   without building the test target ships silently.** Logged at
   `.planning/todos/pending/2026-08-11-secrets-free-test-ci-workflow.md`; belongs to Stage 4.
2. **`ARCHITECTURE §OQ4` `rigScale ≈ 7.95`** → correct to `7.93` at the next contract revision. Not
   edited mid-phase: it is a checksummed contract and editing it invalidates the pins 2.2 re-verifies.
3. **DSP-04/3 and FUNC-03/3** staged to 2.2.

## Stage 2 Phase 2.1 Execute Results (2026-08-11) — `stages/2-dsp/SUMMARY-2.1.md`

Contract checksums re-verified at the boundary — all four byte-exact. **13 of 14 tasks complete;
Task 1 (manual Logic check) reported done, its four observations still to be recorded.**

**21 probes across two test targets, 0 failures. Every gate re-run, none read out of a prior document.**

| Gate | Result |
|---|---|
| Clean 3-format build + both test targets, forced TU recompile | ✓ **0 warnings, 0 errors** in the entire log |
| Hardcoded output channel indices outside `ChannelMap` | ✓ **zero** (4 remaining `8` hits are doc prose) |
| `PHASE-2.2-REPLACE` | ✓ **0** — retired, not grandfathered |
| `auval -v aufx OuOc OuDv` | ✓ **AU VALIDATION SUCCEEDED**, 17 × `parameter PASS` |
| pluginval s10, VST3 ×3 / AU ×3 | ✓ SUCCESS on all six |
| Both test targets | ✓ exit 0 / exit 0 |
| 17 params vs `parameter-spec.md` | ✓ **17/17** on name, range, default **and group** |
| `setLatencySamples` | ✓ absent |

**Two Stage-1 carry-overs closed a phase early, as D3 predicted.** Issue 3 (unity gain confirmed by
inspection only) → probe Q **measures** it, `max |out − in| = 0.000000000` across all 8 lanes.
Issue 4 (the F3 3–7-channel hazard reasoned, not measured) → probe S constructs those layouts
programmatically, no hardware, and asserts *both* that the hazard state is genuinely reproduced
(`getTotalNumOutputChannels() == 8` while the buffer is narrower) **and** that nothing writes OOB.

### Findings worth carrying

- **`ARCHITECTURE §OQ4`'s `rigScale ≈ 7.95 m` is wrong — the true value is 7.9317 m.** Recomputing
  by hand from §OQ4's own coordinate table gives 7.93165, so this is a hand-calc slip in the spec,
  not an implementation error; centroid and bbox in the same section match exactly. Impact nil today
  (`rigScale` is consumed by the blur mapping at 2.2; the error is 0.23%). **Not corrected in
  ARCHITECTURE** — it is a checksummed contract and editing it mid-phase would invalidate the pins
  2.2 re-verifies against. Correct it at the next contract revision.
- **This is exactly why probe O asserts the scaling INVARIANT, not the constant.** A bare `≈ 7.95`
  assertion would have been "fixed" by tuning the code until it produced 7.95
  (`pattern_test_fixture_mirrors_drift_silently`).
- **Probe H does not test what it looks like it tests** — confirmed empirically. The §OQ4 walls are
  dead straight, so speakers 3 and 8 have *exactly* zero cross product and pop for any non-negative
  epsilon; H would pass with `EPS_CROSS = 0`. Probe I supplies the coverage: 1 µm popped, 1 mm kept,
  against a measured `EPS_CROSS` of 1.0e-4.
- **G4 confirmed:** a label of `"7"` resolves to type **134** (`discreteChannel0 + 6`) — a plausible
  discrete channel, **not** `unknown` — and is rejected by the *permutation check*, not by a parse
  error. The mechanism keeping this safe is not the one a reader would assume.
- **Probe K's oracle is genuinely independent:** ternary search on a convex 1-D function in double
  precision, not the closed-form dot-product projection the implementation uses. Measured worst
  deviation **8.67e-7 m**, which meets DSP-03 criterion 3's 1e-6; the asserted bound is set at 1e-4
  because one float ULP at the 27 m sampling extent is already ~2e-6.
- **`juce::var` double serialisation uses `serialiseDouble()` (15–20 dp), not `ostream`'s 6-sig-fig
  default** — so float → XML → float is exact and FUNC-02's exactness is not at risk. Probe T
  asserts all 42 values bit-exactly via `memcmp`.
- **Generator failure paths tested, not asserted in prose:** bad `--juce-modules` → exit 1; a JUCE
  tree parsing to zero enum entries → exit 1 **and no header written**; two runs byte-identical.

### Deviations from PLAN-2.1 (both deliberate)

1. `VenueSnapshot` carries one field beyond §3.6.6's list — `hullEpsCross`. `hull::isInside()` needs
   the room-scaled tolerance, and recomputing it on the audio thread would be a second derivation of
   a message-thread quantity, which is the drift the snapshot exists to prevent.
2. `ConvexHull2D::classify()` also returns `VERTEX` by coordinate, not only by index. §3.1.3's
   index-only definition leaves the STEP-0 dedup case ambiguous; a duplicate sitting exactly on a
   corner is a corner.

### Requirement outcomes (planned at plan, not discovered at verify)

COMPAT-03 ✅ complete · DSP-03 ✅ complete · DSP-04 ⚠️ partial (criterion 3 → 2.2) ·
FUNC-03 ⚠️ partial (criterion 3 → 2.2). `REQUIREMENTS.md` is updated at **verify**.

## Stage 2 Phase 2.1 Plan Results (2026-08-11) — `stages/2-dsp/PLAN-2.1.md`

Contract checksums re-verified — all four byte-exact. **14 tasks**, the first of which is the manual
Logic gate (Task 13 from Stage 1) and blocks every other.

**Requirement staging corrected at plan, not at verify** (the direct lesson of Stage 1's FUNC-01
mis-staging). Two of the four requirements this phase "verifies" cannot fully close here:

| Req | Outcome expected at 2.1 verify |
|---|---|
| COMPAT-03 | ✅ complete — all 3 criteria |
| DSP-03 | ✅ complete — all 4 criteria |
| DSP-04 | ⚠️ **partial** — criterion 3 ("changing `rakeRear` alone changes the **gain vector**") needs `DbapSolver` → 2.2 |
| FUNC-03 | ⚠️ **partial** — criterion 3 ("changing a row moves audio to that output") is unobservable while all 8 lanes carry identical signal → 2.2 Layer 3 |

The five RESEARCH open items are resolved as P5–P13:

- **P5** — targets `O-Octagon-geometry-test` / `O-Octagon-render-test`; both reference the *plugin
  target* `OuariconOctagon`, never the folder name
- **P6** — the SAFE/REAL branch is a **named helper** `mappedOutputAvailable(int)`, so G1 is stated
  once and 2.2's `GainStage` inherits it rather than re-deriving it
- **P7** — `ChannelMap` is **free functions in `ochan`** with no processor reference (so the unit
  target can link it without `PluginProcessor.cpp`); `rebuildChannelMap()` remains the single
  construction site
- **P8** — Layer 1 is **one function with two callers**: a Debug `jassert` in `rebuildChannelMap()`
  and a unit probe. G2's two fixes fold in — scan bound 256 **and** `expected.size() == set.size()`
- **P9** — 200-point hull fixture **generated in-test from a pinned seed**; oracle in the same TU
- **P10** — Layer 2's checksum gate is a **`static_assert`** (ROADMAP says *fails the build*), and the
  SHA covers **parsed data**, not emitted file text
- **P11** — hull classification is a first-class `ConvexHull2D` return value (Q4)
- **P12** — two supporting headers: `Source/DSP/Vec.h`, `Source/Data/VenueSnapshot.h`
- **P13** — the CI gap goes to `.planning/todos/pending/` as a **repo-level** todo;
  `build-and-release.yml` is not touched

**Probe I exists because probe H does not test what it looks like it tests:** the §OQ4 walls are dead
straight, so speakers 3 and 8 have *exactly* zero cross product and are popped regardless of
`EPS_CROSS`. A separate near-collinear case exercises the epsilon.

**Probe O asserts a scaling invariant, not just `rigScale ≈ 7.95`** — a bare constant is a mirrored
fixture (`pattern_test_fixture_mirrors_drift_silently`).

## Stage 2 Phase 2.1 Research Results (2026-08-11) — `stages/2-dsp/RESEARCH-2.1.md`

Contract checksums re-verified — all four byte-exact. Q1–Q5 answered; 9 findings (G1–G9), three of
which are defects in the *specification* found by checking it against JUCE 8.0.14 source.

| Q | Answer |
|---|---|
| Q1 — replaces `PHASE-2.2-REPLACE` | Route the mono sum through `speakerToBuffer` — **but bound the indexing by `buffer.getNumChannels()` independently of `mapInvalid`** (see G1) |
| Q2 — JUCE path for the generator | `JUCE_MODULES_DIR` (`JUCE/CMakeLists.txt:41`, `CACHE INTERNAL`), passed as a script argument. Portable: CI sets `JUCE_DIR` at `build-and-release.yml:131` and the root resolves it identically. `FATAL_ERROR` if empty |
| Q3 — CI | **Local-only at 2.1.** CI is tag-triggered release-only, secrets-bearing, and has *never* set `OUARICON_BUILD_TESTS` — no harness in the repo has ever run in CI. Residual gap logged for Stage 4, not hidden |
| Q4 — hull classification sink | A first-class `ConvexHull2D` return value, not a processor accessor. The 3.2 Venue screen reads the same call — drift becomes structurally impossible |
| Q5 — test framework | **None.** Zero Catch2/GTest/doctest/`juce::UnitTest` anywhere in plugin source or CMake. Use `juce_add_console_app` + `check()`/exit-code, matching all 12 existing harnesses. The Catch2 references in `docs/codebase/TESTING.md` describe an intent never implemented — do not follow them |

### Findings worth carrying

- **G1 (HIGH)** — a valid channel map is **not** evidence of an 8-channel buffer. Under F3 the layout
  reports 7.1 while the buffer holds `n < 8`, so `mapInvalid` stays false and `speakerToBuffer` holds
  indices up to 7. The map is derived from the accessor that lies. Carries into 2.2's `GainStage`.
- **G2 (HIGH)** — ARCHITECTURE §3.2.5 Layer 1's `bit < 64` scan is too small: named types run to 99,
  `discreteChannel0 = 128`, `channels` is a `BigInteger`. Worse, it fails *silently* — a truncated
  list is still strictly increasing. Assert `expected.size() == set.size()`.
- **G3 (HIGH)** — the enum's declaration order ≠ value order (`topSideLeft/Right = 28/29` are declared
  before `ambisonicACN0..3 = 24..27`) and it contains an alias (`surround = centreSurround`). The
  Layer 2 parser must read `= <int>` pairs and assert every referenced name resolved.
- **G5 (HIGH)** — the shipped default label map is the identity map, so a test using it is vacuous
  (C1). Two free non-vacuous cases: a permuted label set under 7.1, and 7.1-types-vs-SDDS (4 of 8
  types absent → the ROADMAP:131 missing-label test from real JUCE sets).
- **G4** — `getChannelTypeFromAbbreviation` already exists (`.h:553`); use it instead of a hand-rolled
  table. Sharp edge: an unvalidated numeric branch (`.cpp:283-285`) turns a label of `"7"` into a
  plausible-looking discrete type rather than `unknown`. Fails safe via the permutation check — test it.
- **G7/G8** — derive `JucePlugin_VersionString` from the target's `JUCE_VERSION`; guard `createEditor`
  with `#if JUCE_WEB_BROWSER` now, while it is inert.
- **D3 resolves to two console apps, not two frameworks:** `tests/unit/` links only the three geometry
  TUs (no `PluginProcessor.cpp`, so no `JucePlugin_*` block and immune to the Stage-3 WebView swap);
  `tests/render-harness/` links the processor.
- **`stages/2-dsp/` is untracked — do NOT execute 2.1 in an isolated worktree**
  (`pattern_worktree_isolation_wrong_for_untracked_scope`).

## Stage 1 Verify Results (2026-08-11) — `stages/1-foundation/VERIFICATION.md`

**Verdict: ⚠️ PARTIAL. Ready for Stage 2: yes, with one caveat.**

Every automated gate below was **re-run from scratch at verify**, not read out of SUMMARY.md.
All passed; none regressed.

| Gate | Verify-phase result |
|---|---|
| Clean rebuild, 3 formats (forced TU recompile) | ✓ **0 warnings, 0 errors** in the entire log |
| `auval -a` | ✓ `aufx OuOc OuDv` |
| `auval -v aufx OuOc OuDv` | ✓ **AU VALIDATION SUCCEEDED** |
| AU channel config set | ✓ `[1,1] [1,2] [1,8] [2,1] [2,2] [2,8]` — exactly RESEARCH F2 |
| pluginval s10 VST3 / AU | ✓ SUCCESS ×3 each |
| State round-trip | ✓ pluginval *Plugin state* + *Plugin state restoration*, ×3 |
| 17 params vs `parameter-spec.md` | ✓ 17/17 on name, range, default **and** group |
| Standalone on a 2-ch device (COMPAT-04) | ✓ launched, stayed running |
| `PHASE-2.2-REPLACE` uniqueness | ✓ exactly 1 occurrence |
| Forbidden CMake keywords, `setLatencySamples`, non-goals | ✓ all clean |

### Requirements

| Req | Status |
|---|---|
| COMPAT-01 (pluginval VST3+AU s10) | ✅ **complete** |
| COMPAT-04 (defined behaviour on stereo) | ✅ **complete** |
| FUNC-01 (8 discrete feeds) | ⚠️ **partial → re-mapped to stage-2** |

**Verify finding — FUNC-01 was mis-staged.** Its third acceptance criterion (*"all 8 output channels
carry independent, non-duplicated signal"*) cannot be met by a shell whose D1 placeholder writes the
same mono sum to all 8 **by design**. Independence requires the DBAP solve (DSP-01/DSP-05). The first
two criteria were met at Stage 1 and are recorded as such in REQUIREMENTS.md so Stage 2 does not
re-derive them. `verifiedAt` moved `stage-1` → `stage-2`.

Corollary: Task 13's "all 8 lanes move" proves **negotiation and writability**, not independence —
with the placeholder in place all 8 lanes carry identical signal. Do not over-read it.

### Why PARTIAL, and why Stage 2 is not blocked

Nothing in Stage 2 depends on the Logic result: Phase 2.1 builds `ChannelMap` and the `VENUE` tree,
and the plugin accepts all three 8-channel containers regardless of which one Logic picks. Running
Task 13 first is nonetheless the cheaper order — if Logic fails to negotiate 8 channels the fault is
in the bus predicate, and unpicking that after a DBAP solver exists costs materially more.

## Stage 1 Execute Results (2026-08-11)

**Files created:** `CMakeLists.txt`, `Source/PluginProcessor.h`, `Source/PluginProcessor.cpp`,
`.planning/parameter-spec.md` (promoted; draft banner-marked superseded).

| Gate | Result |
|---|---|
| Build — VST3 + AU + Standalone | ✓ clean, **zero warnings** from O-Octagon's own TU |
| `auval -a` | ✓ `aufx OuOc OuDv — Ouaricon Audio Development: O-Octagon-dev` |
| `auval -v aufx OuOc OuDv` | ✓ **AU VALIDATION SUCCEEDED**, 17 × `-parameter PASS` |
| AU channel configs | ✓ `[1,1] [1,2] [1,8] [2,1] [2,2] [2,8]` — **exactly RESEARCH F2's prediction** |
| pluginval strictness 10, VST3 | ✓ SUCCESS × 3 runs |
| pluginval strictness 10, AU | ✓ SUCCESS × 3 runs |
| 17 parameters vs `parameter-spec.md` | ✓ verified **programmatically** from the auval dump — all 17 match name, range, default *and* group |
| Standalone on 2-ch device (COMPAT-04) | ✓ opens, no error, generic editor renders 5 groups + units (`m`, `dB/2x`) |

**F2 confirmed empirically.** The AU config set is derived from `isBusesLayoutSupported()` exactly
as researched — this is the first hard evidence that the predicate is the sole authority.

### Outstanding at Stage 1 — carry into verify

1. **Task 13 — Logic 8-channel negotiation is NOT yet done.** Requires Logic and a surround track;
   cannot be automated. It is the strongest available evidence for **FUNC-01 / COMPAT-01**.
   Must confirm: plugin instantiates on a surround track; **all 8 surround-meter lanes move**;
   `outputGain` survives save/close/reopen (FUNC-05 slice); automation menu lists 17 under 5 groups.
   **Record which container Logic actually negotiated** — R2 predicts 7.1-SDDS. Observation, not a
   gate (all three are accepted); it feeds COMPAT-02 at Stage 4.
2. **Task 12 item 3** — audio reaching outputs at unity in Standalone was not confirmed: JUCE's
   Standalone mutes input by default ("Audio input is muted to avoid feedback loop"). Needs a
   manual unmute, or is subsumed by Task 13's meter check.
3. **Gate bypass on record.** The `0-ideation → 1-foundation` gate was run with `--force`: its build
   check is unconditional on stage and cannot pass before `CMakeLists.txt` exists. Logged to
   `.planning/gate-bypasses.log`.
4. **Benign:** pluginval AU emits `WARNING: Current program is -1` — JUCE AU-wrapper reporting,
   present across the repo, not a failure (run still returns SUCCESS).

## Completed So Far

**Ideation:** ✓ Complete
- Core concept defined (Logic-native 8-channel DBAP spatializer for an irregular, non-flat concert array)
- Architecture inherited as locked constraints from `research/logic-pro-multichannel-octaphonic-dbap.md`
- **17** musical parameters + 42 venue values specified *(corrected from 18 at Stage 0 — see ARCHITECTURE §11)*
- Preset strategy settled (two separate stores)
- Signal flow, UI concept, use cases captured
- 30 requirements extracted with acceptance criteria
- v1.1+ deferrals recorded explicitly

**Stage 0:** ✓ Complete
- Complexity tier 6 (DEEP research); complexity score **5.0** (capped; raw 13.0)
- 9 features researched: bus transport, channel map, venue model, convex hull, DBAP solver,
  source shaping, outside-hull processing, gain stage, verify-ping
- All JUCE APIs verified **directly against the local JUCE 8.0.14 source tree** with file:line
  references (Context7 doc-fetch was unavailable; local source is the stronger authority)
- 8 core DSP components specified with full algorithms
- 3-layer channel-map test strategy designed (runtime invariant → source-parsed golden with a
  committed SHA → offline tone-per-speaker render)
- All 5 open questions resolved with concrete defaults
- Parameter-count discrepancy resolved: **17**, arithmetic slip demonstrated
- 8 risks registered; 2 design defects found and fixed before code exists
- Strategy: **staged implementation** — Stage 2 in 3 phases, Stage 3 in 3 phases
- ARCHITECTURE.md and ROADMAP.md documented

## Stage 0 Findings Worth Carrying

1. **`kAudioChannelLayoutTag_Emagic_Default_7_1`** (`juce_CoreAudioLayouts_mac.h:117`) shows Logic's
   native 7.1 ordering corresponds to JUCE's `create7point1SDDS()` membership, not `create7point1()`.
   `isBusesLayoutSupported()` therefore accepts all three 8-channel containers, and the label map is
   keyed on `ChannelType`.
   > ⚠️ **The R2 corollary drawn from this — "Logic will negotiate 7.1-SDDS" — is CONTRADICTED by
   > observation (2.1 verify, V1). Logic negotiated `create7point1()`.** The layout-tag reading
   > itself still stands; the prediction built on it does not. Impact nil (all three are accepted,
   > nothing branches on the choice), but do not carry the SDDS prediction forward as fact.
   > COMPAT-02 at Stage 4 tests against **7.1** as observed, with SDDS and 5.1.2 as alternates.
2. **For 7.1 the enum-bit order coincidentally equals the initializer-list order** — a hardcoded
   0..7 map would appear correct today. The locked constraint is *more* important because of this,
   not less. **Amplified at Stage 1 research (F1): this holds for all THREE accepted 8-channel
   containers**, so no container choice can discriminate a hardcoded map. Only a non-identity
   `map1..map8` label assignment permutes the buffer — the Phase 2.1 test must drive those.
3. **PERF-02 and QUAL-03 are incompatible under a per-block solve.** Resolved with a fixed
   64-sample absolute-sample-aligned control grid.
4. **Centre-crossing L/R flip** in the stereo sub-point geometry found at design time; fixed with an
   `rFade` spread collapse.

## Stage 1 Research Findings Worth Carrying

- **F1** — all three accepted 8-channel containers have initializer order == enum-bit order (see above)
- **F2** — AU channel configs are *derived* from `isBusesLayoutSupported()`:
  `AUChannelInfo = {(1,1),(1,2),(1,8),(2,1),(2,2),(2,8)}`. `auval` exercises all six, so the Stage 1
  placeholder must be correct at 1 and 2 output channels, not only 8. SAFE mode is load-bearing for
  AU, not just Standalone. Confirms `PLUGIN_CHANNEL_CONFIGURATIONS` is redundant as well as harmful.
- **F3 (hazard)** — Standalone on a 3–7 output device: `canonicalChannelSet(n)` yields LCR/quad/5.0/
  5.1/7.0, all rejected; Debug asserts, **Release keeps the 7.1 layout while the buffer has n
  channels**. Bound every output loop by `buffer.getNumChannels()` — never by `8`, never by
  `getTotalNumOutputChannels()`, which is the accessor that lies in exactly this state.
- **F4** — `canonicalChannelSet(8) == create7point1()`, so Standalone on an 8-out interface
  negotiates REAL mode with no host. Free Stage 2 listening rig.
- **F5** — `create5point1point2()` has no VST3 layout-table entry; it resolves via the generic
  bit-order fallback. Works, least-exercised of the three; keep 7.1 primary.
- **F6** — JUCE hoists `Fx` to index 0 of `VST3_CATEGORIES`; the emitted string is always
  `Fx|Spatial`. Declare `"Fx" "Spatial"` so the source matches what ships.
- **F7** — `AU_MAIN_TYPE kAudioUnitType_Effect` is already JUCE's default. Both keywords are new to
  this repo — no sibling CMakeLists sets either.
- **State root is `OOctagon`** (not the sibling's `OOrbitParams` idiom) and must never change —
  Phase 2.1 attaches `VENUE` to that node. Stage 1 sessions carry **no** `VENUE` child, so
  `readVenueFromState()` must treat a missing/partial node as "use defaults".
- **`-Wswitch-enum` bans switching on `AudioChannelSet::ChannelType`** (~60 enumerators; warns even
  with a `default:`). The Phase 2.1 label map must be a table or `if`-chain.

## Stage 1 Plan Decisions (plan phase)

The four open items RESEARCH §9 handed to the plan phase are resolved in PLAN.md:

- **P1** — five parameter groups: Position / Solve / Weights / Space / Output. The headline
  gesture is automating eight weights; a flat 17-entry menu buries them. Reversible.
- **P2** — the venue-store member slot is claimed **above `apvts`** in `PluginProcessor.h` as a
  comment marker. Declaration order is fixed at Stage 1 and annoying to change once 2.1 depends
  on it. No member exists yet.
- **P3** — the D1 placeholder loop carries the greppable token `// PHASE-2.2-REPLACE:` so Phase
  2.1's "zero hardcoded output indices" gate retires it rather than grandfathering it.
- **P4** — `parameter-spec.md` is promoted from the draft as Task 1. The executor reads the
  promoted file, **never** `parameter-spec-draft.md` (which still marks OQ3/4/5 and the
  17-vs-18 count as open, all four resolved at Stage 0).

Also settled at plan: `srcX`/`srcY` display **normalised** in the host lane (metres are a Stage
3.1 UI-side conversion — a value→text lambda is captured at construction and cannot read a live
venue); **no `PluginEditor.{h,cpp}`** at Stage 1 (`GenericAudioProcessorEditor` is five lines and
is itself the 17-parameter exit criterion).

## Next Steps

1. **Phase 2.2 discuss** — `/plugin-discuss O-Octagon 2-dsp`. DBAP Solve + Gain Application.
   Verifies FUNC-01, DSP-01/02/05, PERF-01/02, QUAL-02/03/04, and closes DSP-04/3 and FUNC-03/3.
   Inherits verbatim:
   - **G1** — `mappedOutputAvailable(int)`. A valid map is **not** evidence of an 8-channel buffer;
     `GainStage` calls the helper, it does not re-derive the condition.
   - **C1** — Layer 3 must drive **non-identity** label maps. All three accepted containers have
     initializer order == enum-bit order, so a container-only test is vacuous.
   - **D4** — the DBAP reference for DSP-01 is a **Python** script in `tests/tools/` emitting a
     committed fixture; a C++ reference beside the implementation mirrors the same misreading.
   - **DBAP per the 2011-04-14 revised equations** — the original's eqs 3–6 and 9–10 are wrong.
2. UI mockup — two screens, Room + Venue. Due before Stage 3.1; not a Stage 2 blocker.
3. Measure Roy Barnett Recital Hall — 8 × (x, y, z) metres + front/rear ear heights
4. **Stage 4** — pick up the CI-gap todo and the `ARCHITECTURE §OQ4` `rigScale` correction.

## Context to Preserve

**Build constraints for Stage 1:**
- Target `OuariconOctagon`, folder `plugins/O-Octagon`, `PRODUCT_NAME "O-Octagon${OUARICON_DEV_SUFFIX}"`
- `PLUGIN_CODE OuOc` (verified unused across all 39 existing plugins)
- **`VERSION 1.0.0`**, never `PLUGIN_VERSION`
- **No `PLUGIN_CHANNEL_CONFIGURATIONS`**
- `juce::juce_dsp` linked; `BusesProperties` in the constructor init list
- **Must not link SAF** (unlike sibling O-Orbit)

**Locked by prior research — do NOT re-litigate:**
- mono/stereo in → `AudioChannelSet::create7point1()` out; 7.1 is only an 8-channel carrier
- Never `octagonal()` or `discreteChannels(8)` — Logic ignores both
- Not multi-output (`aumu` only; `aufx` gets one bus)
- Speaker→buffer map built ONCE in `prepareToPlay()` via `getChannelIndexForType()` — a wrong map
  is SILENT and passes every automated gate
- DBAP per the **2011-04-14 revised** equations

**Deferred to v1.1+ — do not plan work for these:**
VBAP A/B mode; binaural/stereo fold-down; quadraphonic variant; internal diffuse reverb; motion
engine; multiple simultaneous sources.

**Highest risk:** the speaker→buffer channel map (R1, CRITICAL, silent failure).

## Files Created

- `plugins/O-Octagon/.planning/BRIEF.md` *(Ideation; parameter count corrected at Stage 0)*
- `plugins/O-Octagon/.planning/REQUIREMENTS.md` *(Ideation)*
- `plugins/O-Octagon/.planning/parameter-spec-draft.md` *(Ideation)*
- `plugins/O-Octagon/.planning/research/ARCHITECTURE.md` *(Stage 0)*
- `plugins/O-Octagon/.planning/ROADMAP.md` *(Stage 0)*
- `plugins/O-Octagon/.planning/stages/0-ideation/CONTEXT.md` *(Stage 0)*
- `plugins/O-Octagon/.planning/stages/1-foundation/CONTEXT.md` *(Stage 1 discuss)*
- `plugins/O-Octagon/.planning/stages/1-foundation/RESEARCH.md` *(Stage 1 research)*
- `plugins/O-Octagon/.planning/stages/1-foundation/PLAN.md` *(Stage 1 plan — 14 tasks)*
- `plugins/O-Octagon/.planning/stages/1-foundation/SUMMARY.md` *(Stage 1 execute)*
- `plugins/O-Octagon/.planning/parameter-spec.md` *(Stage 1 execute — promoted, supersedes the draft)*
- `plugins/O-Octagon/.planning/stages/2-dsp/CONTEXT-2.1.md` *(Stage 2 Phase 2.1 discuss)*
- `plugins/O-Octagon/.planning/stages/2-dsp/RESEARCH-2.1.md` *(Stage 2 Phase 2.1 research)*
- `plugins/O-Octagon/.planning/stages/2-dsp/PLAN-2.1.md` *(Stage 2 Phase 2.1 plan — 14 tasks)*
- `plugins/O-Octagon/.planning/stages/2-dsp/SUMMARY-2.1.md` *(Stage 2 Phase 2.1 execute)*
- `plugins/O-Octagon/.planning/stages/2-dsp/VERIFICATION-2.1.md` *(Stage 2 Phase 2.1 verify)*
- `plugins/O-Octagon/CMakeLists.txt` *(Stage 1 execute; 2.1 adds 3 TUs + the test-target option)*
- `plugins/O-Octagon/Source/PluginProcessor.h` *(Stage 1 execute; rewritten at 2.1)*
- `plugins/O-Octagon/Source/PluginProcessor.cpp` *(Stage 1 execute; rewritten at 2.1)*
- `plugins/O-Octagon/Source/DSP/Vec.h` *(2.1)*
- `plugins/O-Octagon/Source/DSP/ConvexHull2D.{h,cpp}` *(2.1)*
- `plugins/O-Octagon/Source/DSP/ChannelMap.{h,cpp}` *(2.1)*
- `plugins/O-Octagon/Source/Data/VenueModel.{h,cpp}` *(2.1)*
- `plugins/O-Octagon/Source/Data/VenueSnapshot.h` *(2.1)*
- `plugins/O-Octagon/tests/tools/gen_juce_channel_order.py` *(2.1 — Layer 2 generator)*
- `plugins/O-Octagon/tests/unit/{CMakeLists.txt,main.cpp}` *(2.1 — 16 probes A–P)*
- `plugins/O-Octagon/tests/render-harness/{CMakeLists.txt,main.cpp}` *(2.1 — 5 probes Q–U)*
- `.planning/todos/pending/2026-08-11-secrets-free-test-ci-workflow.md` *(2.1 — repo-level, P13)*
- `plugins/O-Octagon/.planning/STATUS.md`
