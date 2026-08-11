# Stage 2 — DSP · Phase 2.1 (Geometry Core) — Verification

**Plugin:** O-Octagon
**Stage:** 2 of 4 — DSP
**Phase:** 2.1 of 3 — Geometry Core
**GSD phase:** verify
**Date:** 2026-08-11
**Branch:** `feat/o-octagon`
**Verifies:** COMPAT-03, DSP-03, DSP-04, FUNC-03

---

## Verdict

**✅ VERIFIED.** Ready for Phase 2.2: **yes**, with no blockers.

Every automated gate was **re-run from scratch at verify**, not read out of `SUMMARY-2.1.md`.
All passed; none regressed. Two requirements close as **partial**, both **by plan rather than by
shortfall** — their remaining criteria are unobservable until a solver exists and were staged to 2.2
at the plan phase, not discovered here.

**Stage 1's open manual gate is now CLOSED.** Task 1 (Logic 8-channel negotiation) was outstanding
at execute with its four observations unrecorded; all four are recorded below and all four are
positive. `stage_1_open_manual_gate` is retired.

---

## Entry check — contract checksums

Re-verified at this boundary by recomputation, not read out of a prior document
(`pattern_promotion_checksum_pins_replaced_file`). This is obligation **C6**.

| Contract | Recomputed SHA-256 | vs `STATUS.md` |
|---|---|---|
| `BRIEF.md` | `697a4f32890d7420cdef85bafbf8fe45775bf805cf1ff7b449ed2c14f6b9fbd6` | ✅ byte-exact |
| `parameter-spec.md` | `b45f88dc5017ec2c1a9da49ba35242d01903000a4ff199d16758e1b6cbb9e02f` | ✅ byte-exact |
| `research/ARCHITECTURE.md` | `bff8a83b379113ac8b1e2a8915d6f1edc7183558b992bdc3808877f86c406cfe` | ✅ byte-exact |
| `ROADMAP.md` | `aec7d0ce0db9ad6c78cb1c9e9574a0a2f8ddb1cf258e6e4b701f2e2e0137ee29` | ✅ byte-exact |

No drift. 2.1 was planned, executed and verified against these exact four documents.

---

## Goal-Backward Analysis

### Original goals (from `CONTEXT-2.1.md`)

1. **Own the room.** Stand up `VenueModel`, `ConvexHull2D`, `ChannelMap` and `VenueSnapshot` — the
   geometry the DBAP solver will consume — with no gain math present to confuse a diagnosis.
2. **Front-load R1.** The speaker→buffer channel map is the plugin's CRITICAL risk: a wrong map is
   silent, passes every automated gate, and is audible only in the hall. Give it its own verify.
3. **Route through the map.** Retire the Stage-1 `PHASE-2.2-REPLACE` placeholder — the plugin's one
   hardcoded output index — rather than grandfathering it (C2).
4. **Close the two Stage-1 carry-overs a phase early** (D3): unity gain confirmed by inspection only
   (C4), and the F3 3–7-channel hazard reasoned but never measured (C5).
5. **Close the Stage-1 manual gate** (D2): Logic 8-channel negotiation, before 2.1 execute.

### Deliverables (from `SUMMARY-2.1.md`, confirmed by code inspection at verify)

1. Five new source files — `Vec.h`, `VenueModel.{h,cpp}`, `ConvexHull2D.{h,cpp}`,
   `ChannelMap.{h,cpp}`, `VenueSnapshot.h` — plus the processor integration.
2. Three-layer test scaffolding: Layer 1 (runtime invariant, two callers), Layer 2 (source-parsed
   golden with a build-failing checksum), Layer 3 staged to 2.2. 21 probes across two console
   targets.
3. `processBlock` rewritten to write **through** `snapshot.speakerToBuffer`; the marker token is
   gone from the source entirely.
4. Probes Q (unity gain) and S (F3 hazard) — both **measured**, not inspected or reasoned.
5. Task 1 run; observations recorded at this verify.

### Goal achievement

| Goal | Status | Evidence gathered at verify |
|---|---|---|
| 1 — Own the room | ✅ Achieved | 16/16 geometry probes pass; all five files present, `VenueSnapshot` `static_assert`ed trivially copyable; publisher is a lock-free double buffer with no `shared_ptr` and no `free` on the audio thread |
| 2 — Front-load R1 | ✅ Achieved | Layer 1 + Layer 2 both standing and **both proven by negative control** (below) — not merely asserted |
| 3 — Route through the map | ✅ Achieved | `grep -c "PHASE-2.2-REPLACE"` → **0**; `processBlock` inspected line-by-line — the only output index is `snapshot.speakerToBuffer[i]`, guarded by `mappedOutputAvailable()` |
| 4 — Close C4 / C5 | ✅ Achieved | Probe Q: `max \|out − in\| = 0.000000000` across all 8 lanes. Probe S: hazard state confirmed reproduced (`getTotalNumOutputChannels() == 8` while the buffer is narrower) **and** no OOB write |
| 5 — Close the Logic gate | ✅ Achieved | All four observations recorded below; all four positive |

---

## Requirements Verification

**Requirements for this phase:** 4 total (4 must, 0 should, 0 nice)

| Requirement | Priority | Status | Acceptance criteria |
|---|---|---|---|
| **COMPAT-03** — channel map built once via `getChannelIndexForType()`, zero hardcoded indices | must | ✅ **Complete** | 3 / 3 met |
| **DSP-03** — convex hull, explicit collinearity, outside sources projected | must | ✅ **Complete** | 4 / 4 met |
| **DSP-04** — sloped audience plane, `srcZ = 0` tracks ear level | must | ⚠️ **Partial** | 2 / 3 met — criterion 3 → **2.2** |
| **FUNC-03** — 8-row label map, duplicate/missing surfaced not silently routed | must | ⚠️ **Partial** | 2 / 3 met — criterion 3 → **2.2** |

### COMPAT-03 — criterion by criterion

| # | Criterion | Verdict | Evidence at verify |
|---|---|---|---|
| 1 | Grep confirms zero hardcoded channel indices in the output path | ✅ | `grep -rnE "\[[0-7]\]\|== 8\|< 8\|, 8\)"` over `PluginProcessor.{h,cpp}` returns **3 hits, all doc-comment prose**. Line-by-line read of `processBlock` confirms the single index is `snapshot.speakerToBuffer[i]` |
| 2 | Unit test asserts the map built by `getChannelIndexForType()` against known JUCE enum-bit order for 7.1 | ✅ | Probe A (all 3 accepted 8-channel sets, scan bound 256 + `expected.size() == set.size()`), probe B (`7.1 {1,2,3,4,10,11,20,21}`) |
| 3 | Test fails loudly if JUCE's enum-bit order changes — **asserted against parsed source, not a mirrored constant** | ✅ | **Proven by negative control, not by claim.** See below |

### DSP-03 — criterion by criterion

| # | Criterion | Verdict | Evidence at verify |
|---|---|---|---|
| 1 | Hull yields vertices 1, 2, 4, 5, 6, 7; speakers 3 and 8 `ON_EDGE` | ✅ | Probe H: `m=6 vertices {1,2,4,5,6,7}, spk3 ON_EDGE, spk8 ON_EDGE`. **Probe I is the criterion's real coverage** — see F3 carried forward |
| 2 | A source at a physical rear corner classifies as outside | ✅ | Probe J: `outside, d_hull = 4.0608 m` |
| 3 | Projection matches brute-force nearest-point-on-segment to 1e-6 over a fixture set | ✅ | Probe K: `max \|impl − oracle\| = 8.67e-7 m over 200 pts, 147 outside`. Meets 1e-6; asserted bound deliberately 1e-4 (F2) |
| 4 | Degenerate venues do not crash or produce NaN | ✅ | Probe L: collinear `m=2`; coincident `m=1`; zero-rake-span → `rakeFront`; degenerate bbox finite |

### DSP-04 — criterion by criterion

| # | Criterion | Verdict | Evidence at verify |
|---|---|---|---|
| 1 | `srcZ = 0` height varies linearly `rakeFront` → `rakeRear` | ✅ | Probe M: `front 1.100, mid 3.550, rear 6.000`; `rakeRear` moves the rear, front unchanged |
| 2 | `srcZ = 0` at the rear of a steeply raked room is never below rear-row ear height | ✅ | Probe N: `rear height 8.0000 m >= rakeRear 8.0000 m, worst deficit 0.000000` |
| 3 | Changing `rakeRear` alone changes **the gain vector** for a rear source | ⏸️ **→ 2.2** | Requires `DbapSolver`, an explicit 2.1 non-goal. Staged at **plan**, not discovered here |

### FUNC-03 — criterion by criterion

| # | Criterion | Verdict | Evidence at verify |
|---|---|---|---|
| 1 | Each of the 8 rows can be assigned any of the 8 labels | ✅ | Probe D drives a **non-identity** map: built `{1,2,3,4,5,6,7,0}`, expected `{1,2,3,4,5,6,7,0}` — C1 satisfied, a hardcoded 0..7 fails immediately |
| 2 | Duplicate or missing assignment detected and surfaced, not silently routed | ✅ | Probe E (cross-container missing label → rejected, output `{9,…}` completely untouched), probe F (duplicate → rejected, **last valid map retained**), probe G (numeric-label trap) |
| 3 | Changing a row moves audio to the corresponding physical output, confirmed by verify-ping | ⏸️ **→ 2.2** | Unobservable while all 8 lanes carry identical signal; verify-ping itself is FUNC-04 at Stage 3. Staged at **plan** |

**Requirements summary:** ✅ Complete 2 · ⚠️ Partial 2 (both by plan) · ⏸️ Deferred criteria 2 · ❌ Failed 0

---

## Automated Checks — all re-run at verify

| # | Check | Result | Detail |
|---|---|---|---|
| 1 | Clean 3-format build + both test targets, forced TU recompile | ✅ Pass | All 8 source files and both test `main.cpp` touched; 21-step ninja run, **exit 0, zero `warning`/`error` matches in the entire 31-line log** |
| 2 | Hardcoded output channel indices outside `ChannelMap` | ✅ Pass | 3 hits, **all doc-comment prose**; no code |
| 3 | `grep -c "PHASE-2.2-REPLACE"` | ✅ Pass | **0** — retired, not grandfathered |
| 4 | `auval -a` → `aufx OuOc OuDv`; `auval -v` | ✅ Pass | `AU VALIDATION SUCCEEDED`; **17 × `-parameter PASS`**; `# # # 17 Global Scope Parameters` |
| 5 | pluginval strictness 10, VST3 ×3 and AU ×3 | ✅ Pass | **SUCCESS on all six runs**, exit 0 each (`pattern_ci_pluginval10_catches_latent_nan`) |
| 6 | Both test targets, `-DOUARICON_BUILD_TESTS=ON` | ✅ Pass | `geometry-test` **16 probes, 0 failures, exit 0**; `render-test` **5 probes, 0 failures, exit 0** |
| 7 | 17 parameters vs `parameter-spec.md` | ✅ Pass | **17/17 on name, range, default AND group**, compared programmatically by parsing both `parameter-spec.md` and the `auval` dump — neither side hand-transcribed |
| 8 | `setLatencySamples` | ✅ Pass | Absent; one comment saying never to call it |
| 9 | `switch` on `AudioChannelSet::ChannelType` (`-Wswitch-enum`) | ✅ Pass | Zero `switch` statements anywhere in `Source/` |
| 10 | Non-goal symbols absent | ✅ Pass | All 6 hits on `DbapSolver`/`GainStage`/`SourceShaper`/`HullProcessor`/`VerifyPing`/`juce_add_binary_data` are prose comments naming what is deliberately absent |
| 11 | `createEditor` guarded with `#if JUCE_WEB_BROWSER` (G8) | ✅ Pass | Present and provably inert today — both arms identical (`pattern_render_harness_breaks_on_webview_editor`) |
| 12 | Test targets gated behind `OUARICON_BUILD_TESTS` (default OFF) | ✅ Pass | `option(... OFF)`; both `add_subdirectory` calls inside the `if` |

Benign, unchanged since Stage 1: pluginval AU emits `!!! WARNING: Current program is -1` — a JUCE
AU-wrapper report present across the repo; the run still returns SUCCESS.

---

## Negative Controls — run at verify, not at execute

`SUMMARY-2.1.md` asserted the Layer-2 gate works. Verify **measured** it. Both controls below are
new work at this phase; a gate that has never been observed to fail is a gate whose failure path is
untested (`pattern_check_for_generator_before_declaring_provenance_unknown`).

### NC1 — the golden genuinely tracks parsed JUCE source

`juce_AudioChannelSet.{h,cpp}` were copied to a scratch tree and `leftSurroundRear = 20` mutated to
`90` — one enum value, in one of the two files the generator parses.

| Tree | Generated SHA-256 |
|---|---|
| Real JUCE 8.0.14 | `5cd774cb228888a28f1526d127ce4ee1d076386b26a98ef8315b35a4abdd7b00` |
| Mutated copy | `39298098e3efd38cf28ec2f375e1e6d90fa830a3e9a095ab8c0a971fbceef1cf` |

The baseline is byte-identical to the constant committed at `tests/unit/main.cpp:84`. **The SHA
moves when JUCE moves** — it is not a mirrored constant (`pattern_test_fixture_mirrors_drift_silently`).

### NC2 — the checksum divergence genuinely FAILS THE BUILD

The committed SHA was temporarily replaced with NC1's mutated value and `O-Octagon-geometry-test`
rebuilt:

```
BUILD_EXIT=1
main.cpp:86:16: error: static assertion failed due to requirement
  'std::string_view(juce_golden::kGeneratedChannelOrderSha256)
   == std::string_view(kCommittedChannelOrderSha256)':
  JUCE's ChannelType enum values or 8-channel set membership have CHANGED. …
```

ROADMAP:131 requires Layer 2 to **fail the build**, not merely fail a test run. Confirmed. The file
was reverted and the target rebuilt green (16 probes, 0 failures) before proceeding.

### NC3 — the generator refuses to emit a vacuous golden

| Case | Result |
|---|---|
| `--juce-modules` pointing at a non-existent directory | exit **1**, **no header written** |
| A JUCE tree whose `enum ChannelType` block parses to zero `= <int>` pairs | exit **1**, **no header written** |
| Two consecutive runs against the same JUCE tree | **byte-identical** |

The second case is the load-bearing one: a vacuous golden reports green, which is worse than no
golden at all.

### NC4 — `ARCHITECTURE §OQ4`'s `rigScale` recomputed independently

Recomputed from §OQ4's own 8-row coordinate table in a standalone script, not by re-running the
implementation:

| Quantity | Independent recomputation | Implementation (probe O) | §OQ4 prose |
|---|---|---|---|
| centroid | `(6.5000, 12.4625, 4.9250)` | — | `(6.50, 12.4625, 4.925)` ✅ |
| bbox | `x[0.50, 12.50] y[4.50, 19.50]` | — | identical ✅ |
| `rigScale` (3D RMS radius) | **7.93165 m** | **7.9317 m** | `≈ 7.95 m` ❌ |

F1 confirmed independently: **§OQ4's `≈ 7.95` is a hand-calculation that does not follow from the
coordinates listed immediately above it.** The implementation is right; the spec prose is wrong by
0.231%.

---

## Human Verification — Task 1 (Logic 8-channel negotiation)

The Stage-1 manual gate, run before 2.1 execute per **D2**. Reported done at execute with
observations unrecorded; **recorded here, which closes it.**

- [x] **All 8 surround-meter lanes moved.** Confirms **negotiation and writability**.
      *Not* independence — all 8 lanes carry identical signal at this phase by design. Independence
      is FUNC-01 criterion 3 at 2.2.
- [x] **Container negotiated: `7.1`** — JUCE's `create7point1()`, the primary container. **See V1.**
- [x] **`outputGain` survived save → close → reopen.** FUNC-05 slice confirmed in a real host,
      beyond pluginval's synthetic state round-trip.
- [x] **Automation menu listed 17 parameters under 5 groups** (Position / Solve / Weights / Space /
      Output) — matching the `auval` clump dump exactly.

---

## Issues Found

### V1 — Logic negotiated **7.1**, not 7.1-SDDS. R2's Stage-0 prediction is wrong. *(new at verify)*

Stage 0 finding 1 predicted 7.1-SDDS, reasoning from
`kAudioChannelLayoutTag_Emagic_Default_7_1` (`juce_CoreAudioLayouts_mac.h:117`), whose membership
corresponds to `create7point1SDDS()` rather than `create7point1()`. The observation contradicts it.

**Impact: nil.** All three containers are accepted, the label map is keyed on `ChannelType` rather
than on container, and probe A exercises all three. Nothing in the code branches on which one Logic
picks. But the prediction was being carried forward as fact into COMPAT-02 at Stage 4, and it is now
retired in favour of an observation. It also means the **least**-exercised VST3 path
(`create5point1point2()`, RESEARCH F5) is not the one Logic uses, and the **primary** container is —
a mildly better position than planned for.

**Action:** none at 2.1. COMPAT-02 at Stage 4 tests against 7.1 as observed, with SDDS and 5.1.2 as
the alternates that must still work.

### V2 — `Output` parameter default reads `7e-07` in the `auval` dump, not `0.0` *(new at verify, benign)*

`outputGain` is specified as default `0.0` over `−24.0 … +12.0 dB`. `auval` reports
`Default = 0.0000007`. This is AU's normalise → denormalise float round-trip: `0.0 dB` normalises to
`24/36 = 0.666…`, which does not have an exact float representation, and denormalises back a fraction
of a ULP high. Well within the 1e-6 comparison tolerance and not a defect. Recorded so a future
reader comparing the dump to `parameter-spec.md` does not chase it.

### Carried forward from execute — re-confirmed at verify, not re-derived

- **F1 — `ARCHITECTURE §OQ4`'s `rigScale ≈ 7.95 m` is wrong; the true value is 7.9317 m.**
  Independently reproduced at NC4. **Not corrected here** — ARCHITECTURE is a checksummed contract
  and editing it mid-phase invalidates the pins 2.2 re-verifies against. Correct at the next contract
  revision. Impact today is nil (`rigScale` is consumed by the blur mapping at 2.2; 0.231% error).
- **F3 — probe H does not test what it appears to test.** The §OQ4 side walls are dead straight, so
  speakers 3 and 8 have *exactly* zero cross product and are popped for any non-negative epsilon —
  **H would pass with `EPS_CROSS = 0`.** Probe I supplies the real coverage: 1 µm popped, 1 mm kept,
  against a measured `EPS_CROSS` of 1.0e-4. DSP-03 criterion 1 is met **by H and I together**, not
  by H.
- **F4 — the numeric-label trap.** A label of `"7"` resolves to type **134** (`discreteChannel0 + 6`)
  — a plausible-looking discrete channel, **not** `unknown` — and is rejected by the *permutation
  check*, not by a parse error. The mechanism keeping this safe is not the one a reader would assume.

### Deviations from `PLAN-2.1.md` — both reviewed and accepted

1. **`VenueSnapshot` carries `hullEpsCross`, one field beyond ARCHITECTURE §3.6.6's list.**
   Accepted: `hull::isInside()` needs the room-scaled area tolerance, and recomputing it on the audio
   thread from the bbox would be a second derivation of a message-thread quantity — the exact drift
   the snapshot exists to prevent. Documented in the header at the field.
2. **`ConvexHull2D::classify()` returns `VERTEX` by coordinate as well as by index.** Accepted:
   §3.1.3's index-only definition leaves the STEP-0 dedup case ambiguous, and a duplicate sitting
   exactly on a corner *is* a corner. Reporting `ON_EDGE` there would mislead a user who has just
   mistyped two identical coordinates. Documented at the call site.

Neither deviation touches a contract checksum or a requirement's acceptance criteria.

---

## Residual — open beyond 2.1, deliberately

1. **CI gap.** No test target in this repo has ever run in CI (`build-and-release.yml` is
   tag-triggered, release-only, secrets-bearing, and has never set `OUARICON_BUILD_TESTS`). The
   Layer-2 build-failing checksum therefore only fires for whoever configures with
   `-DOUARICON_BUILD_TESTS=ON`. **A JUCE bump performed without building the test target ships
   silently.** Logged at `.planning/todos/pending/2026-08-11-secrets-free-test-ci-workflow.md`.
   Not closed at 2.1; belongs to Stage 4. Verified present at verify.
2. **`ARCHITECTURE §OQ4` `rigScale`** — correct `≈ 7.95` to `7.93` at the next contract revision.
3. **Two acceptance criteria staged to 2.2** — DSP-04/3 and FUNC-03/3, per the table above.

None of the three blocks Phase 2.2.

---

## Why the verdict is VERIFIED and not PARTIAL

Stage 1 closed PARTIAL because a requirement was discovered to be mis-staged **at verify** —
FUNC-01's third criterion could never have been met by a shell whose placeholder wrote the same mono
sum to all 8 outputs by design.

2.1 is the direct correction of that. Both partial requirements here were declared partial **in
`PLAN-2.1.md`, before execute began**, with the reason and the destination phase named. Nothing was
discovered at verify that the plan did not already predict, no gate failed, and the one item that
*was* genuinely open at execute — the Logic manual gate — is closed by this document with four
positive observations.

The phase delivered what it set out to deliver.

---

## Next

**Phase 2.2 — DBAP Solve + Gain Application.** Verifies FUNC-01, DSP-01/02/05, PERF-01/02,
QUAL-02/03/04, and closes DSP-04/3 and FUNC-03/3.

Inherits verbatim:
- **G1** — `mappedOutputAvailable(int)`. A valid channel map is **not** evidence of an 8-channel
  buffer. 2.2's `GainStage` calls the helper; it does not re-derive the condition.
- **C1** — Layer 3 must drive **non-identity** label maps. All three accepted containers have
  initializer order == enum-bit order, so a container-only test is vacuous
  (`critical_audiochannelset_is_a_bitset_not_an_order`).
- **D4** — the DBAP reference for DSP-01 is a **Python** script in `tests/tools/` emitting a
  committed fixture. A C++ reference beside the implementation would mirror the same misreading.
- **DBAP per the 2011-04-14 revised equations** — the original paper's eqs 3–6 and 9–10 are wrong
  (`pattern_dbap_not_vbap_for_irregular_arrays`).

The stage-level `VERIFICATION.md` is written only at the close of 2.3.
