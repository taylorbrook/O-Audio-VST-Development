# Stage 2 — DSP · Phase 2.1 (Ring + pass engine + one stream) — Verification

## Verification Date

2026-11-04

I re-ran every check below myself in the verify session. Before running them I touch-rebuilt `Engine.cpp` and `PluginProcessor.cpp`, then rebuilt and reinstalled with `build-and-install.sh`. No result here is copied from SUMMARY-2.1.md.

## Goal-Backward Analysis

### Original Goals (CONTEXT-2.1.md / PLAN-2.1.md Goal)

1. Replace the Stage 1 pass-through with the engine skeleton:
   - a 32-sample absolute pass grid with write-before-read;
   - a 41 s `MipRing<2>` (D2);
   - `fbDelay` z⁻³² present and holding zeros.
2. Run one live stream (Free timing, telescoping jitter, R4 epoch) in an engine shaped for 4 streams.
3. Build a spawner with a reach floor (P1), double read positions, and R1 read-fraction quantisation, plus a `GrainPool<Grain,128,16>` with the R3 `grain_limit` cap.
4. Use a Hann renderer and the cap-aware incoherent output law.
5. Apply equal-power dry/wet and output gain on the control grid, using exact-arrival ramps, the R2 0 dB snap and the P4 mix-0 bypass.
6. Pass every ROADMAP 2.1 gate in the harness. Install, pass pluginval 10 and auval, and pass the D3 listening checkpoint.
7. Consume grain-engine 1.1.0 unmodified (D1).

### Deliverables (SUMMARY-2.1.md)

The engine lives in `Source/dsp/`: EngineConstants, Grain, ControlGrid, OutputLaw, StreamScheduler, CaptureWriter, Spawner and Engine.h/.cpp. `PluginProcessor` is wired to 17 params plus the s2–s4 mask. The harness adds the E, A, L, N, O, M and R3 rows and rescopes B2 to mix 0 % (P8).

### Goal Achievement

| Goal | Status | Evidence |
|------|--------|----------|
| 1 Pass grid + ring | ✅ Achieved | Both static_asserts are present at `EngineConstants.h:45` (`kCell <= kFbDelay`) and `:75` (span + reach + guard ≤ 41 s). `CaptureWriter.h:53` asserts kFbDelay == kCell. Harness N1 shows a bitwise block-size null. |
| 2 One stream, 4-shaped | ✅ Achieved | Harness E1 and N4 (R4: 0/8 start delays repeat on re-enable) |
| 3 Spawner + pool + cap | ✅ Achieved | E2: 0 reach violations. R3: maxAlive 32/64 with kills > 0. One read site: `readGrain()` at `Engine.h:167` is the only `readStereo` caller, and the R1 `0x1p28` quantisation sits at `Engine.h:170`. |
| 4 Renderer + output law | ✅ Achieved | E0 units: ∫w² error 0, continuity 0, P/μ₂ = 43.020. This is the corrected value; the §10 erratum is recorded in SUMMARY. |
| 5 Dry/wet + gains | ✅ Achieved | B2 is bitwise for mix 0 % (host-normalised path). E3 corr is −0.0023 at mix 100. E4 snaps gain to exactly 1.0f. No `juce::SmoothedValue` is used; the only mention is a comment at `ControlGrid.h:64`. |
| 6 Gates + validation + D3 | ✅ Achieved | Harness 77/77. pluginval 10 passes on VST3 and AU, and auval passes. D3 was confirmed by Taylor (see Human Verification). |
| 7 grain-engine unmodified | ✅ Achieved | `git diff --stat 3e161927 HEAD -- modules/` is empty |

## Requirements Verification

**Stage:** stage-2 (Phase 2.1 of 6). Requirements verified at stage-2 are closed only when their full acceptance criteria are covered. Rows that 2.1 covers only in part are marked partial.

| Requirement | Priority | Status | Acceptance Criteria |
|-------------|----------|--------|---------------------|
| FUNC-01: Live ring buffer | must | ✅ Complete | E1: audible at sample 0 at position 0, and the position-100 % control is silent. E2: buffer_length 30 → 1 s, 0 over the bound, 0 reach violations, and a step ratio of 1.000 against the same-motion reference (an injected step reads 30.5×). D3 (4) passed. |
| FUNC-07: Mix / gain | must | ✅ Complete | Mix 0 % is bitwise dry across 3 layouts × 6 blocks with latency 0. Mix 100 % gives corr −0.0023. Input and output gain snap to exactly 1.0f at 0 dB. D3 (5) passed. |
| DSP-02: Band-limited reads | must | ✅ Complete | A1 at +24 st, 44.1 k: tones −118.5 dB and sweep −94.5 dB, both under the −60 dB threshold. The A1n `forceLevel0` control fails that threshold (non-vacuous). |
| DSP-01: Pitch + quantisation | must | ⚠️ Partial | Covered: the ±24 st extremes are clean (A1 and D3 (2)), and the long-buffer reads hold (L1/L2). Open: scale quantisation within 1 cent, which is Phase 2.2. |
| DSP-04: Grain randomisation | must | ⚠️ Partial | Covered: reverse prob works (D3 (3)), with its 0/100 % gating in the spawner. Open: a statistical 50 % row, spray-0 determinism, and stereo spread, all due in 2.2. |
| PERF-01: RT safety | must | ⚠️ Partial | Covered: 0 allocations across 2104 spawns, 1073 steals, 903 kills, toggles and 119 buffer_length changes, with a liveness malloc count of 1. Open: freeze (2.4) and matrix edits (2.5). |
| PERF-02: Pool + CPU budget | must | ⚠️ Partial | Covered: the R3 cap is enforced, and the CPU baseline is 8.28 % of one core at 128 grains, 48 k/128, no FX. Open: the worst-case preset with FX and feedback against the ≤ 30 % budget, in 2.3 and 2.6. |
| PERF-03: Block-size invariance | should | ⚠️ Partial | Covered for one stream: N1 gives max\|Δ\| = 0 for 441/512/4096/variable vs 32. The N2 different-seed control differs and the N3n R1 negative control is non-vacuous. Open: a re-gate once 4 streams, FX and feedback land. |

All other stage-2 requirements stay pending: FUNC-02–06, FUNC-08, DSP-03, DSP-05–08 and QUAL-01–03.

**Requirements Summary (this phase):**
- ✅ Complete: 3 (FUNC-01, FUNC-07, DSP-02)
- ⚠️ Partial: 5 (DSP-01, DSP-04, PERF-01, PERF-02, PERF-03)
- ⏸️ Deferred (later phase or stage): 21
- ❌ Failed: 0

## Automated Checks

| Check | Result | Notes |
|-------|--------|-------|
| Build: VST3, AU, Standalone and render harness | ✅ Pass | Touch rebuild of `Engine.cpp` and `PluginProcessor.cpp`: 0 warnings, 0 errors |
| Install | ✅ Pass | `build-and-install.sh O-Granulator` exits 0 and installs `O-Granulator-dev.{vst3,component}`. The sweep found no alternate variant. |
| Render harness | ✅ Pass | 77 PASS, 0 FAIL, rc 0, 14 s wall |
| CPU baseline C1 (non-blocking) | ℹ️ Recorded | 8.28 % of one core. Engine bypass is 0.102 %, a ratio of 81× (SUMMARY reported 8.2–8.7 %). |
| auval `aufx OuGn OuDv` | ✅ Pass | AU VALIDATION SUCCEEDED |
| pluginval 10 VST3 | ✅ Pass | SUCCESS, rc 0 |
| pluginval 10 AU (in-process) | ✅ Pass | SUCCESS, rc 0. One warning: "Current program is -1". This is expected because there are no factory programs until Stage 4, same as Stage 1. |
| `setValueNotifyingHost` in Source | ✅ Pass | 0 occurrences |
| `ScopedNoDenormals` in processBlock | ✅ Pass | `PluginProcessor.cpp:162` |
| Heap ops in `Source/dsp/` | ✅ Pass | No `new`, `std::vector`, `push_back`, `resize(` or `malloc` |

## Human Verification

D3 listening checkpoint: Taylor confirmed all five on 2026-11-04, on live input with stream 1 and the Generic editor.

- [x] Position scans the buffer
- [x] `s1_pitch` ±24 st sounds clean
- [x] `s1_reverse_prob` produces reversed grains
- [x] `buffer_length` moves without clicks
- [x] Mix 0 / 50 / 100 behave as expected

## Issues Found

- **Gap carried to 2.2: no isolated steal-free bit-identity row.** PLAN Risks calls for one: `grain_limit` 128 with N < 128, compared against the native pool. SUMMARY deviation 8 records that it is missing. Steal-free behaviour is exercised inside N1 and M1 but never isolated. 2.2 must add this row before it changes the spawner or the pool path.
- **Erratum carried to 2.2/2.3: ARCHITECTURE §10 double-counts μ₂.** Use `M_m(x)` in the single-normalisation form (see `OutputLaw.h`). The correct worst case is P/μ₂ = 43.02 (−4.74 dB), not ≈ 61.
- **By design until 2.2:** with spray 0, jitter 0 and pitch 0, the output runs about +7 dB hot. The κ blend in 2.2 fixes this.
- **Carried to 2.6:** `getTailLengthSeconds()` is still 0 (P7). 2.6 also adds the allocation-size check for 110 MB at 192 k (D2).
- **Carry-ins already scheduled for 2.2:** sort the active pitch table, and give the audio thread its own copy of the custom scale table.

## Phase Verdict

**Status:** ✅ VERIFIED (Phase 2.1)

**Ready for next phase:** Yes. Next is Phase 2.2, which adds 4 streams, the scale quantiser, the window morph and the κ blend (see ROADMAP).

**Blockers:** none
