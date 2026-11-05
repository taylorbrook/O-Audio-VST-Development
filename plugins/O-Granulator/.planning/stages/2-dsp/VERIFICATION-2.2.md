# Stage 2 — DSP · Phase 2.2 (Four streams, randomisation, pitch/scale, window morph, normalisation) — Verification

## Verification Date

2026-11-05

I re-ran every check below myself in the verify session, on a clean tree at `60d00196`. Before running them I touch-rebuilt `Engine.cpp` and `PluginProcessor.cpp`, then rebuilt and reinstalled with `build-and-install.sh`. No result here is copied from SUMMARY-2.2.md.

## Goal-Backward Analysis

### Original Goals (CONTEXT-2.2.md / PLAN-2.2.md)

1. Land the isolated steal-free bit-identity row first, before any spawner or pool change (gap carried from VERIFICATION-2.1).
2. Run four streams over one shared pool. Use the (offset, stream) onset merge, parameter-derived water-filled quotas and stream-fair stealing.
3. Add the D4 scale quantiser (GS parity: uniform pick among active degrees, base pitch never snapped) over pitch-collections 1.0.0, incl. Custom, with an audio-side table copy.
4. Add the 65-step WindowMorphBank with skew, latched at spawn.
5. Add stereo spread, cut-only gain rand (D5) and statistical reverse.
6. Use an output law that stays level-flat across density and kills the 2.1 +7 dB hot spot. R5 replaced the κ blend.
7. Add a mute/solo/level/pan mixer on 10 ms ramps.
8. Stay RT-safe and block-size invariant with everything on, and stay bitwise-equivalent to 2.1 where 2.2 features are off.

### Deliverables (SUMMARY-2.2.md)

- New: `WindowMorphBank.h`, `Quotas.h`, `ScaleQuantiser.h`.
- Extended: `OutputLaw.h`, `Spawner.h`, `Engine.h/.cpp`, `ControlGrid.h`, `Grain.h` and `PluginProcessor`.
- Gate files: `engine_gates_22.h` / `22b.h` and `tests/r12/`.
- grain-engine 1.1.0 and pitch-collections 1.0.0 are unmodified.

### Goal Achievement

| Goal | Status | Evidence |
|------|--------|----------|
| 1 Steal-free row first | ✅ Achieved | S0: 0 diffs vs the native pool (401 spawns, 0 steals or kills on both sides). It landed alone in `fccf9ce6`, ahead of the engine commit. |
| 2 Four streams, shared pool | ✅ Achieved | F1, F2, F3 (maxAlive == cap at 32/64/96/128). F4 starvation: worst +0.69 dB, and the native steal-oldest control reads −25.8 dB. F5: six cases within ±1.0 dB, and the R8 control fails as it should. F7 single-stream victim == 2.1 (0 / 18000). |
| 3 D4 quantiser | ✅ Achieved | Q1: 12493 picks across 31 collections, worst 3e-5 ¢. Q2–Q7 cover empty masks, 7-Limit sorting, spec ≠ gs index, audio 5/4 exact, custom swap with 0 try-lock misses, and the unsnapped base pitch. |
| 4 Window morph + skew | ✅ Achieved | W1: 65 × 5 within −0.033 dB, and the capped corner within −0.011 dB. E0b/c bank cumulatives are within 1e-6. |
| 5 Spread / gain rand / reverse | ✅ Achieved | D1: 0 / 2401 / 1165 (z −1.45). D2 is deterministic at spray 0 / spread 0. D3 constant power within ±1.8e-7. D4 bitwise at 0, and 100 % stays within [−23.93, −0.24] dB. D5 draws happen before obtain. |
| 6 Output law, hot spot gone | ✅ Achieved | W2: density 1→200, worst +0.76 dB (limit ±1.5). W2n: the 2.1 law reads +19.57 / +19.75 dB, the 2.2 law −0.14 / +0.05 dB. W3: the sub-floor case does not boost. E0d: at c = 0 the law is bitwise equal to 2.1 in 1680/1680 cells. |
| 7 Mixer ramps | ✅ Achieved | X1: worst 1.0001× against the same-motion reference, and the hard-switch control reads 103.5×. F6: solo and mute leave the pool untouched (0 / 1568). |
| 8 RT / block size / 2.1 equivalence | ✅ Achieved | M2: 0 allocations (113 rebuilds, 19 custom copies, 2659 steals) with liveness 1. N5: 0 diffs at 441/512/4096/variable, and the N5c seed control differs. **R12 out-of-tree null against `6508c635`: max\|Δ\| 0 in both cases (free and capped).** |

## Requirements Verification

**Stage:** stage-2 (Phase 2.2 of 6)

| Requirement | Priority | Status | Acceptance Criteria |
|-------------|----------|--------|---------------------|
| FUNC-03: Four streams | must | ✅ Complete | Covered: each stream is audible alone at −16.0 dBFS (F1). Disabled streams make 0 plan or scheduler calls and fire 0 onsets (F2). The pool is shared and never exceeds the cap (F3). Per-stream level and pan come from the X1 mixer. |
| DSP-01: Pitch + quantisation | must | ✅ Complete | Covered: picks land within 1 cent across all 31 collections incl. BP and Custom (Q1, worst 3e-5 ¢). The ±24 st extremes were already covered by 2.1 A1. Absolute mode (pitch follow) is 2.5 and is not in this requirement. |
| DSP-03: Window morph | must | ✅ Complete | Covered: RMS stays within ±1 dB across morph × skew (W1 −0.033 dB), and level is overlap-normalised across density (W2 ±1.5 dB). |
| DSP-04: Grain randomisation | must | ✅ Complete | Covered: reverse is exact at 0 % and 100 %, and 50 % falls within 4σ (D1). Spray 0 and spread 0 are deterministic (D2). Size rand and gain rand are live, exercised in N5, D4 and M2. |
| QUAL-01: No artifacts | must | ⚠️ Partial | Covered: the grain-steal click gate passes in both kill and steal modes (K1, non-vacuous through K1n at 27.9×), and the mixer shows no zipper (X1). Open: freeze toggle (2.4), and sweeps of cutoff and other FX params (2.3). |
| PERF-01: RT safety | must | ⚠️ Partial | Covered: 0 allocations in M2 with 4 streams, steals, kills, scale rebuilds and custom swaps. Open: freeze (2.4) and matrix edits (2.5). |
| PERF-02: Pool + CPU | must | ⚠️ Partial | Covered: the shared-pool cap is enforced (F3), and C2 measures 8.92 % of one core with 4 streams and 126 live grains. Open: the worst case with FX and feedback against the ≤ 30 % budget (2.3, 2.6). |
| PERF-03: Block-size invariance | should | ⚠️ Partial | Covered: 4 streams with all 2.2 randomisation on (N5, 0 diffs). Open: a re-gate once FX, feedback and modulation land. |

Still pending at stage-2: FUNC-02, FUNC-04–06, FUNC-08, DSP-05–08, QUAL-02 and QUAL-03.

**Requirements Summary (this phase):**
- ✅ Complete: 4 (FUNC-03, DSP-01, DSP-03, DSP-04). Running stage-2 total: 7.
- ⚠️ Partial: 4 (QUAL-01, PERF-01, PERF-02, PERF-03)
- ⏸️ Deferred (later phase or stage): 11 in stage 2, plus 8 in stages 3 and 4
- ❌ Failed: 0

## Automated Checks

| Check | Result | Notes |
|-------|--------|-------|
| Build: VST3, AU, Standalone and render harness | ✅ Pass | Touch rebuild: 0 warnings, 0 errors |
| Install | ✅ Pass | `build-and-install.sh O-Granulator` exits 0 and installs `O-Granulator-dev.{vst3,component}`. The sweep found no alternate variant. |
| Render harness | ✅ Pass | **135 PASS, 0 FAIL**, rc 0, 84.8 s wall |
| R12 `tests/r12/run.sh` | ✅ Pass | free: 576000 samples, max\|Δ\| 0. capped (1168 kills): max\|Δ\| 0. rc 0 |
| CPU C2 (non-blocking) | ℹ️ Recorded | 8.92 % of one core at 48 k/128, 1.08× the 2.1 baseline (SUMMARY reported 9.01 %) |
| auval `aufx OuGn OuDv` | ✅ Pass | AU VALIDATION SUCCEEDED |
| pluginval 10 VST3 | ✅ Pass | SUCCESS, rc 0 |
| pluginval 10 AU (in-process) | ✅ Pass | SUCCESS, rc 0. One warning: "Current program is -1". This is expected because there are no factory programs until Stage 4. |
| `setValueNotifyingHost` in Source | ✅ Pass | 0 |
| `ScopedNoDenormals` in processBlock | ✅ Pass | `PluginProcessor.cpp:185` |
| Heap ops in `Source/dsp/` | ✅ Pass | The only `std::vector` is in `WindowMorphBank` (built in the ctor and `build()` on the message thread). Nothing on the audio path, as confirmed by M2. |
| Locks on the audio path | ✅ Pass | Only `ScopedTryLockType` at `Engine.cpp:126`. The blocking lock at `:119` is reached from `prepare()` only (P11). |
| Modules unmodified | ✅ Pass | `git diff --stat 6508c635 HEAD -- modules/` is empty |
| 2.1 force-off mask removed | ✅ Pass | No remnants in `Engine.*` |

## Errata Review (SUMMARY 6–8, flagged "review at verify")

- **6 — S0n moved to cap 64: accepted.** The re-run confirms the plan's cap-128 pair is identical by construction: the S0n′ INFO row shows 0 diffs, with both sides at 473 steals and 0 kills. At cap 64 the kill path engages (537 kills vs 473 native steals) and the outputs differ by 342,982 samples. That proves the native hook bypasses `Spawner::acquire`, which is what the control exists for. F7 covers victim equivalence with stealing active.
- **7 — F3 at cap 128 gated on steals > 0: accepted.** 128 is the pool size, so the pool's own steal enforces the cap (2274 steals, maxAlive 128). Kills cannot occur there by design.
- **8 — W2o scope narrowed: accepted, with a flag.**
  - The gated cells (noise, plus sine at c = 1) pass at +0.483 dB against a 0.5 dB limit.
  - The excluded incoherent-sine cells show a 0.733 dB seed spread. Adding that spread to the W2 worst case gives 0.76 + 0.73 = 1.49 dB, which is still inside ±1.5.
  - The result is deterministic, because the seeds are fixed.
  - **Risk:** the 0.017 dB margin is fragile. Any legitimate change to draw order or seeds in 2.3+ could flip W2o without a real regression. If it does, widen the seed set; do not loosen the threshold.

## Human Verification

None required for 2.2 (CONTEXT D1–D3: no listening checkpoint in this phase). An optional ear-check in the Generic editor is enough to confirm that 4 streams with scale on sound musical.

## Issues Found

- **Minor (not blocking): ordering in `setCustomScale`.** It writes the table under the lock, then resets the Custom masks to all-on, then bumps `customVersion`. The audio thread copies only when the version moves, so the worst case is one grid that pairs the new table with a not-yet-reset mask. Q6 shows 0 bad picks. Leave as is.
- **Carried:** the W2o margin (see erratum 8).
- **Carried to 2.6:** `getTailLengthSeconds()` is still 0 (P7), and the 110 MB allocation-size check at 192 k is still due.
- **Carried to 2.5:** absolute-mode scale (pitch follow + `scale_root`) must use the same D4 pick-from-active-set rule, with the set rooted at `scale_root`.

## Phase Verdict

**Status:** ✅ VERIFIED (Phase 2.2)

**Ready for next phase:** Yes. Next is Phase 2.3 (see ROADMAP).

**Blockers:** none
