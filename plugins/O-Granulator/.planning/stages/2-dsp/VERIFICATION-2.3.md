# Stage 2 — DSP · Phase 2.3 (Per-grain FX + CPU budget) — Verification

## Verification Date

2026-11-05

I re-ran every check below myself in the verify session, on a clean plugin tree at `5b2ea3c6`. Before running them I touch-rebuilt `Engine.cpp` and `PluginProcessor.cpp`, then rebuilt and reinstalled with `build-and-install.sh`. No result here is copied from SUMMARY-2.3.md.

## Goal-Backward Analysis

### Original Goals (CONTEXT-2.3.md / PLAN-2.3.md)

1. Prove the FX plumbing is bitwise-neutral first: N0 out-of-tree null vs `c10d0f2c`, committed before any FX code (P17).
2. Run a latched per-grain chain in ARCH §9 order (read → SVF → drive → crush → ring → window·gain·pan) through one gather → FX → scatter kernel, every FX op per sample (R13, P18).
3. Use the locked level laws: D6 drive (peak-unity at −12 dBFS, continuous from 0), D7 mid-tread crush, D8 BP = k·v1.
4. Make every FX audibly per-grain and reset per-grain state at spawn (no slot bleed), incl. steals.
5. Stay finite under extreme settings; kill a grain on a non-finite SVF state (P25).
6. Stay RT-safe, block-size invariant and click-free on FX sweeps.
7. Meet the CPU phase gate: C3w ≤ 30 %, C3t ≤ 10 % of one core at 48 k/128 (P26).

### Deliverables (SUMMARY-2.3.md)

- New: `Source/dsp/GrainFx.h`, `tests/n0/`, `engine_gates_23.h` / `23b.h`.
- Extended: `EngineConstants.h`, `Grain.h` (`driveNorm`, `crushHoldPhase` default 1), `ControlGrid.h`, `Spawner.h` (`planFx`, `assignFxLatch`), `Engine.h/.cpp` (`renderSegment` kernel + P25 kill), `PluginProcessor.cpp` (4 × 10 FX pointers).
- No new params, no state-codec change. `git diff --stat c10d0f2c HEAD -- modules/` is empty.

### Goal Achievement

| Goal | Status | Evidence (re-run) |
|------|--------|----------|
| 1 N0 neutral null first | ✅ Achieved | `tests/n0/run.sh --strict` rc 0: free and capped max\|Δ\| 0; N0c differs; N0n-F/D/C/R all DIFFER (D only by ≤ 2.4e-7 — drive 1 % → d = 0.0016, just above the guard, which is the D6 continuity the row is meant to show). `b342cb60` contains no `Source/` change. |
| 2 Chain + single kernel | ✅ Achieved | Code read of `Engine.cpp:498–` and `GrainFx.h`: gather → per-stage loops gated by the latched `fxActive` bits → scatter; `fxActive == 0` keeps the 2.2 statements verbatim (N0 0 diffs). |
| 3 D6 / D7 / D8 laws | ✅ Achieved | Code matches: `norm·tanh(d·x)`, norm through the same shaper, `d = 16·drive²`, guard 1e-4; `nearbyint(x·s)/s`; BP `k·v1`. U1a worst −0.0025 dB, U2b −0.000001 dB, U3a 600/600 silence → 0, U3n control fails as designed. |
| 4 Per-grain + reset | ✅ Achieved | V1a–d censuses, V2 ρ 0.9974 (V2n −0.34), V3 58/58 (V3n 0/58), V4 78/78 (V4n 34/78). H1 B peak 0 (H1n 6.8e-4), H2 0/673 steal mismatches, H3 0 non-default FX state over 1602 + 1441 spawns. |
| 5 Finite / NaN kill | ✅ Achieved | Z1a 5/5 finite, 0 subnormal, nanKills 0; Z1b finite; Z2 5/5 (each integrator + fade slot) retired with max\|Δ\| 0 vs `forceRetire`. |
| 6 RT / block size / clicks | ✅ Achieved | M3 0 allocs (liveness 1) incl. NaN kill, steals, kills, rebuilds. N6 0 diffs at 441/512/4096/var (N6c differs). K2a 1.170, K2b 0.800, K2c 1.615; K2n live-coeff control 105.5×. U4a ring drift −141 dB (U4n per-32 control −115.6 dB). |
| 7 CPU gate | ✅ Achieved | C3w **17.08 %** (≤ 30), C3t **7.14 %** (≤ 10), C3b 0.12 %, C2′ 8.99 % (P18 lever threshold 9.8 % not crossed). |

## Requirements Verification

**Stage:** stage-2 (Phase 2.3 of 6)

| Requirement | Priority | Status | Acceptance Criteria |
|-------------|----------|--------|---------------------|
| DSP-05: Per-grain FX | must | ✅ Complete | Filter, drive, crush and ring are each per-grain: randomised values differ between grains and the audio follows the latch (V1–V4, each with a shared-latch control that fails). State resets on spawn with no slot bleed (H1–H3). |
| QUAL-02: No NaN/denormal | must | ⚠️ Partial | Covered: max resonance + max density + all FX at 44.1/48/192 k (Z1a/b), NaN injection (Z2). Open: max feedback (2.4 / 2.6). The resonant wet peak is INFO (+12.4 dBFS) until 2.4 wet safety. |
| QUAL-01: No artifacts | must | ⚠️ Partial | Added: cutoff and ring sweeps click-free (K2a/b, non-vacuous via K2n). Open: freeze toggle (2.4). |
| PERF-01: RT safety | must | ⚠️ Partial | Added: 0 allocations with all FX, FX sweeps and the NaN kill (M3). Open: freeze (2.4), matrix edits (2.5). |
| PERF-02: Pool + CPU | must | ⚠️ Partial | Added: worst-case FX config 17.08 % within 30 %. Open: full worst case with feedback + reverb (2.6). |
| PERF-03: Block-size invariance | should | ⚠️ Partial | Added: all FX on (N6, 0 diffs). Open: feedback and modulation re-gates. |

Still pending at stage-2: FUNC-02, FUNC-04–06, FUNC-08, DSP-06–08, QUAL-03.

**Requirements Summary (this phase):**
- ✅ Complete: 1 (DSP-05). Running stage-2 total: 8.
- ⚠️ Partial: 5 (QUAL-01, QUAL-02, PERF-01, PERF-02, PERF-03)
- ⏸️ Deferred (later phase or stage): 9 in stage 2, plus 8 in stages 3 and 4
- ❌ Failed: 0

## Automated Checks

| Check | Result | Notes |
|-------|--------|-------|
| Build: harness, VST3, AU, Standalone | ✅ Pass | Touch rebuild: 0 warnings, 0 errors |
| Install | ✅ Pass | `build-and-install.sh O-Granulator` rc 0, `O-Granulator-dev.{vst3,component}` |
| Render harness | ✅ Pass | **175 PASS, 0 FAIL**, rc 0, 2:05 wall |
| Carried rows | ✅ Pass | S0 0 diffs; W2o +0.483 dB (unchanged, still 0.017 dB margin); C2 9.10 % |
| N0 `tests/n0/run.sh --strict` | ✅ Pass | free + capped max\|Δ\| 0; N0c + N0n-F/D/C/R differ; rc 0 |
| R12 `tests/r12/run.sh` | ✅ Pass | free max\|Δ\| 0; capped (1168 kills) max\|Δ\| 0; rc 0 |
| CPU C3 (phase gate) | ✅ Pass | C3w 17.08 %, C3t 7.14 % (SUMMARY: 17.33 / 7.31) |
| auval `aufx OuGn OuDv` | ✅ Pass | AU VALIDATION SUCCEEDED |
| pluginval 10 VST3 (in-process) | ✅ Pass | SUCCESS, rc 0 |
| pluginval 10 AU (in-process) | ✅ Pass | SUCCESS, rc 0; "Current program is -1" warning expected (no factory programs until Stage 4) |
| `StateVariableTPTFilter` / `IIR::` in `Source/` | ✅ Pass | 0 |
| Heap ops in `GrainFx.h` | ✅ Pass | 0 |
| Transcendentals in `GrainFx.h` | ✅ Pass | Only in `randOct`, `svfLatch`, `crushLatch`, `ringLatch` (spawn-time latch); `tanh` in the drive stage is the law itself |
| `setValueNotifyingHost` in `Source/` | ✅ Pass | 0 |
| Locks on the audio path | ✅ Pass | Unchanged from 2.2: only the `ScopedTryLockType` at `Engine.cpp:126` |
| Modules unmodified | ✅ Pass | `git diff --stat c10d0f2c HEAD -- modules/` empty |

## Errata Review (SUMMARY errata 1–14, execution deviations 1–11)

- **Errata 1–4 (D6, D7, D8, R14 per-sample renorm):** accepted. These are the CONTEXT/RESEARCH decisions, and the code and U rows match them. ARCHITECTURE §9 and ROADMAP 2.3 still carry the old laws (not edited because of checksums); this file and SUMMARY-2.3 are the record.
- **Errata 5–14:** accepted as documented.
- **Deviation 1 (N0 "landed + identical raw checksum"):** accepted. The re-run shows identical raw FNV checksums on both sides per case; the skewed-range inexactness is the known normalised-path limit (memory: stepped/skewed ranges).
- **Deviations 2–3 (U3b/U3c gate forms):** accepted. Both are float-rounding reformulations; the mean-run (1.8e-5) and index-recovery forms are strictly tighter than the plan's intent.
- **Deviation 4 (B2 rows excluded from the carried diff):** accepted, with a flag. The unseeded `getSystemRandom()` processor is a pre-existing test weakness, not a 2.3 regression. Seed it when the B2 rows are next touched.
- **Deviations 5, 7, 9 (lengthened renders, Z1a steals not kills, Z1b peak):** accepted; kills under full FX are covered by H3 and N6.
- **Deviations 6, 8, 10, 11:** accepted.

## Human Verification

None required for 2.3 (CONTEXT: no listening checkpoint; per-grain audibility is gated objectively by V2–V4). Optional ear-check in the Generic editor — **keep monitors down**: resonant LP/HP can exceed 0 dBFS until 2.4 wet safety.

## Issues Found

- **None blocking.**
- **Carried to 2.4 discuss:** R19 resonant LP/HP headroom vs wet safety (option a: ARCH §14 tanh safety; option b: latched per-stream make-down, amends D8). R22 crush silence threshold (−12 dBFS at crush 100 %) — hear at the 2.4 listening checkpoint.
- **Carried:** W2o 0.017 dB margin — widen seeds, never the threshold.
- **Carried:** seed the B2 "defaults" processor so it can rejoin the carried-row diff.
- **Carried to 2.6:** full PERF-02 worst case with feedback + reverb; re-measure C2 (now 9.0–9.2 %); `getTailLengthSeconds()`; the 192 k allocation-size check.

## Phase Verdict

**Status:** ✅ VERIFIED (Phase 2.3)

**Ready for next phase:** Yes. Next is Phase 2.4 (see ROADMAP).

**Blockers:** none
