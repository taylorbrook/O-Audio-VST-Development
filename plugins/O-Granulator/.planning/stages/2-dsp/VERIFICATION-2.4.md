# Stage 2 — DSP · Phase 2.4 (Freeze, feedback network, output reverb) — Verification

## Verification Date

2026-11-05

I re-ran every check below myself in the verify session, on a clean plugin tree at `154f3e8c`. Before running them I touch-rebuilt `Engine.cpp` and `PluginProcessor.cpp`. The harness ran on a quiet machine, with the plugin build held until it finished so it couldn't skew C4. Then I rebuilt and reinstalled with `build-and-install.sh`. No result here is copied from SUMMARY-2.4.md.

## Goal-Backward Analysis

### Original Goals (CONTEXT-2.4.md / PLAN-2.4.md)

1. Prove the 2.4 plumbing is neutral first: the N1 out-of-tree null vs `6aed8017`, with a strict control per feature (P31).
2. Freeze (FUNC-02): a head-stop that holds the ring byte-identical, click-free on engage and release, with no buzz at spray 0 (R23–R28).
3. Feedback network (FUNC-05): tone HP/LP, D12 saturation, a 4-allpass diffuser and stereo AGC in a z^-32 loop, with the R29 bound tap law.
4. DSP-06: the loop is bounded at feedback 100 % under every setting; it decays linearly at FX gain ≤ 1 (R30 scope).
5. DSP-07: ReverbUnit output through reverb-unit 1.2.0's atomic bind (D11), non-breaking for O-Marimba.
6. D9 wet-safety knee (peak ≤ A) and the D10 LP/HP make-down, attributed exactly by the legacy hooks (P33).
7. RT-safe, block-size invariant, finite at every rate; C4 CPU measured.

### Goal Achievement

| Goal | Status | Evidence (re-run) |
|------|--------|-------------------|
| 1 N1 neutral null | ✅ Achieved | `tests/n1/run.sh --strict` rc 0: free and capped max\|Δ\| 0 (peak 0.211 / 0.065); N1c differs (568,972); N1n-MK / KN / FB / FZ / RV all DIFFER (strict) |
| 2 Freeze | ✅ Achieved | FZ1 hash held 10 s (FZ1n differs). FZ2a engage κ **1.186×** (no-defer control 23.85×). FZ2b release κ **1.147×** (old splice 112.7×). FZ3 ρ_H −0.001 (control 0.990). FZ6 0 violations; FZ7 / FZ8 PASS. Listening item 1 PASS |
| 3 Feedback network | ✅ Achieved | U7a–f chain oracle; FB1 +347 dB over fb 0; FB2a–d: HP, LP, saturation and diffusion each change the loop; FB5 tap pan / mute / solo |
| 4 DSP-06 bounded | ✅ Achieved | Z3: 60 s at fb 100 %, 4 × 200 g/s, +12 st, +6 dB, LP res 100 %, rev 30 %, at 48 / 44.1 / 192 k. Peaks 1.58 / 1.53 / 0.93 (≤ A = +6 dBFS); 0 subnormal, 0 scrubs, 0 NaN kills. FB3a 32/32 corners decay (slowest −27.9 dB/s); FB3n ARCH law grows (discriminates); U8 theorem holds |
| 5 Reverb | ✅ Achieved | RV1–RV4 PASS. reverb-unit diff vs `6aed8017` = the bind overload + version/changelog only |
| 6 D9 / D10 | ✅ Achieved | U5a–c: identity bitwise below the knee, max y == float(A). U6a–c: m == 1 to res 30.83 %, res 100 % peak +10.0 dB. Carried rows PASS; W2o +0.483 dB unchanged |
| 7 RT / block size / CPU | ✅ Achieved | M4 0 allocations (liveness 1). N7 final: 441 / 512 / 4096 / var == 32 (value compare). C4 **17.75 %** (SUMMARY 18.57 %); headroom to 30 % is 12.25 % |

## Deviation Review (SUMMARY-2.4 execution deviations 11 and 12)

Code read: `Engine.cpp` `handleFreezeEdges` / `forcedReleaseScan`, `CaptureWriter.h`, `Spawner.h` `frozenDelay`.

- **11 — Deferred head-stop.** The rising edge latches `W0 = head + freezeDeferCells·32`, and the R25 scan runs against that W0. The ring keeps pushing until `freezeStopTick`, where `jassert (writeCount == W0)` holds the invariant. Spawns inside the window clamp by `headLead`, so they never read ahead of the moving head. A release inside the deferral just cancels it. The stop lands on a tick, so it stays block-size invariant (N7). Cost: about 5 ms of engage latency. **Accepted** (23.85× → 1.19×; listening item 1 PASS).
- **12 — Slope-matched release continuation.** At k = 0, `held` is the linear extrapolation 2·x[W0−1] − x[W0−2] (no step, no slope reversal). It blends onto the DC-free R26 mirror over 2 ms, before (1 − w) has grown, and the indices clamp to the captured span. The per-sample `cos` runs only during the 10 ms fade. **Accepted** (plain mirror 6.98× → 1.15×; listening item 1 PASS).
- **Errata 1–16 and deviations 1–10 and 13–21:** accepted as documented. Erratum 16 (the PLAN U8 −0.16 dB for N < 1) and deviation 15 (FZ8 tolerance max(0.2 dB/s, 2 %)) are threshold restatements with a stated measured reason; neither one hides a failure.

## Requirements Verification

**Stage:** stage-2 (Phase 2.4 of 6). Statuses written to REQUIREMENTS.md after the listening verdict (all 7 PASS).

| Requirement | Priority | Status | Acceptance Criteria |
|-------------|----------|-----------------|---------------------|
| FUNC-02: Freeze | must | ✅ Complete | Byte-identical ring over 10 s (FZ1). Engage / release click gates vs same-motion controls (FZ2a / FZ2b) |
| FUNC-05: Feedback network | must | ✅ Complete | FB1 recirculates; FB2a–d: tone, sat and diffusion each change the loop. Freeze wins over feedback, with no push (the ARCH decision; N7 freeze + feedback form) |
| DSP-06: Feedback bounded | must | ✅ Complete | Z3 60 s at fb 100 % / max density / +12 st: peak ≤ +6 dBFS, no NaN, at three rates |
| DSP-07: Output reverb | should | ✅ Complete | RV1–RV4 |
| QUAL-01: No artifacts | must | ⚠️ Partial | Added: freeze toggle (FZ2a / b). Open: mod sweeps (2.5) |
| QUAL-02: No NaN / denormal | must | ⚠️ Partial → near complete | Added: max feedback (Z3 / Z3b / Z4). Open: matrix stress re-run (2.5 / 2.6) |
| PERF-01: RT safety | must | ⚠️ Partial | Added: freeze, feedback and reverb (M4 0 allocations). Open: matrix edits (2.5) |
| PERF-02: CPU | must | ⚠️ Partial | C4 17.75 %. The full budget gate is in 2.6 |
| PERF-03: Block-size invariance | should | ⚠️ Partial | Added: N7 with feedback + freeze + reverb. Open: modulation (2.5) |

## Automated Checks

| Check | Result | Notes |
|-------|--------|-------|
| Harness build (touch rebuild) | ✅ Pass | 0 warnings |
| Render harness | ✅ Pass | **225 PASS, 0 FAIL**, rc 0, 2:49 wall |
| N1 `--strict` | ✅ Pass | rc 0 (above) |
| N0 `--strict` (vs `c10d0f2c`) | ✅ Pass | free / capped max\|Δ\| 0 (2371 kills); N0c + N0n-F / D / C / R differ |
| R12 (vs `6508c635`) | ✅ Pass | free / capped (1168 kills) max\|Δ\| 0 |
| `build-and-install.sh O-Granulator` | ✅ Pass | rc 0, 0 warnings, `O-Granulator-dev.{vst3,component}` |
| auval `aufx OuGn OuDv` | ✅ Pass | AU VALIDATION SUCCEEDED |
| pluginval 10 VST3 (in-process) | ✅ Pass | SUCCESS |
| pluginval 10 AU (in-process) | ✅ Pass | SUCCESS |
| `StateVariableTPTFilter` / `IIR::` in `Source/` | ✅ Pass | 0 |
| Heap ops in `FeedbackChain.h` | ✅ Pass | 0 |
| `std::tan` in `FeedbackChain.h` | ✅ Pass | `tptCoeffs` only (`tanh` at :70 is the D12 law) |
| `setValueNotifyingHost` in `Source/` | ✅ Pass | 0 |
| Locks on the audio path | ✅ Pass | Unchanged: only the `copyCustomTable` try-lock (`Engine.cpp:198`); the blocking branch is prepare-only |
| Modules vs `6aed8017` | ✅ Pass | reverb-unit bind overload + module.yaml + registry only; grain-engine and pitch-collections empty |

## Human Verification — Listening Checkpoint (LISTENING-2.4.md)

| # | Item | Verdict | Fallback taken |
|---|---|---|---|
| 1 | Freeze buzz / clicks / latency (deviations 11–12) | ✅ PASS (Taylor, 2026-11-05) | none |
| 2 | Feedback character, D12 | ✅ PASS (Taylor, 2026-11-05) | none |
| 3 | Diffusion / tone | ✅ PASS (Taylor, 2026-11-05) | none |
| 4 | Reverb range, mix → 0 | ✅ PASS (Taylor, 2026-11-05) | none |
| 5 | R22 crush 100 % | ✅ PASS (Taylor, 2026-11-05) | none |
| 6 | D10 resonance | ✅ PASS (Taylor, 2026-11-05) | none |
| 7 | Drive + feedback sustain | ✅ PASS (Taylor, 2026-11-05) | none |

## Issues Found

- **None blocking** in the automated pass.
- **Carried:** W2o +0.483 dB (0.017 dB margin): widen seeds, never the threshold.
- **Carried to 2.5:** the matrix writes the reverb atomics; QUAL-01 / PERF-01 / PERF-03 re-gate under modulation.
- **Carried to 2.6:** the full PERF-02 gate (C4 17.75 % today); `getTailLengthSeconds()` with reverb + feedback.

## Phase Verdict

**Status:** ✅ VERIFIED (Phase 2.4). Automated checks all pass; listening checkpoint items 1–7 PASS with no fallbacks taken. D12 keeps `kSatSpan = 7`, the R29 tap law stays (no ripple table), and D7 / R22, D10 and R30 are unchanged.

**Ready for next phase:** Yes. Next is Phase 2.5 (see ROADMAP).

**Blockers:** none
