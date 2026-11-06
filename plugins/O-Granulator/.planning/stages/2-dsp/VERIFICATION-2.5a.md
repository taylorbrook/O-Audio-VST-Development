# Stage 2 — DSP · Phase 2.5a (Tempo sync, LFO/env/macro sources, modulation matrix) — Verification

## Verification Date

2026-11-05

Every check below was re-run in the verify session on a clean plugin tree at `839eaf60` (`Source/`, `tests/` and `modules/` unchanged since the execute commit). I touch-rebuilt `Engine.cpp` and `PluginProcessor.cpp` first. The harness ran alone (no concurrent build) so C5 is undisturbed. The nulls, install and validation ran after it, in that order. No result here is copied from SUMMARY-2.5a.md.

## Goal-Backward Analysis

### Original Goals (CONTEXT-2.5a.md / PLAN-2.5a.md success criteria 1–14)

1. 2.5a plumbing is neutral: N2 out-of-tree null vs `42d30b01`, N2p identical, every N2n differs; N1 / N0 / R12 hold (COMPAT-01).
2. TempoTracker (R39 held anchor, R42 rules, R44 bars) + plugin-local Tempo clock: synced onsets on the analytic grid (FUNC-04).
3. Sources: LFO ×4 (D15 / R50 phase model), env follower, macros (FUNC-06).
4. 16-slot matrix: every 2.5a source → every destination class, bipolar; removal without a step; sparse eval (R46), residuals (R49) (FUNC-06).
5. Zipper-free modulation of position / pitch / cutoff / mix (QUAL-03).
6. RT-safe, block-size invariant under modulation + tempo events, finite at 3 rates (PERF-01 / PERF-03 / QUAL-02).
7. Telemetry without APVTS writes; host cache intact (HA).
8. D16: the DAW tempo check against a real host transport.

### Goal Achievement

| Goal | Status | Evidence (re-run) |
|------|--------|-------------------|
| 1 Neutral null | ✅ Achieved | `tests/n2/run.sh --strict` rc 0: free + capped max\|Δ\| 0; N2c differs (568,972); **N2p IDENTICAL**; N2n-TMP / LFO / MAC / ENV all differ (strict). N1 `--strict`, N0 `--strict`, R12 (free + capped, 1168 kills) all rc 0, max\|Δ\| 0 |
| 2 Tempo | ✅ Achieved (harness) | TT0–TT5 PASS (TT1n per-block origin differs — control live). **TS1 152/152, worst \|Δ\| 0.00**, 13,158 onsets, 3 boundary slots excluded; TS1n drops slots; TS2–TS6 PASS. Real-host behaviour → D16 |
| 3 Sources | ✅ Achieved | LS1–LS5 (LS2n jumps 0.6 cycle), EV1–EV3, MC1 PASS |
| 4 Matrix | ✅ Achieved | U9a–g (range parity 7168/7168), MF1–MF7 (MF7n control fails as it must), PH1–PH3 (PH1n 0.3 cycle jump) PASS; Pitch / Onset inert (2.5b) |
| 5 QUAL-03 | ✅ Achieved (harness) | ZQ1 trajectory ≤ slope bound; ZQ2 mix κ ≈ 1.00× vs ZQ2n ≫ 2.0; latched κ within 2.0×. Audible check → D16 item 4 |
| 6 RT / blocks / finite | ✅ Achieved | M5 0 allocs (liveness 1; 15 route changes, 3 re-locks). **N8** 441 / 512 / 4096 / var == 32 max\|Δ\| 0 (4 re-anchors, 2 re-locks, 13 route changes, 533 tempo onsets); N8c, N8n differ. Z5 60 s × 48 / 44.1 / 192 k finite, 0 subnormal, wet ≤ A |
| 7 Telemetry / host | ✅ Achieved | TL1 PASS (220 params bitwise unchanged). HA on the installed VST3: HA1 69/69, HA2 152 untouched unchanged, HA3 state XML 69/69 |
| 8 D16 DAW check | ⏸️ Deferred | To Stage 3 (Taylor, 2026-11-05) — run with the WebView UI in place; DAW-2.5a.md items 1–8 unchanged |

## Deviation Review (SUMMARY-2.5a execution deviations 1–12)

I read the code in `TempoTracker.h` (`update`), `StreamScheduler.h` (`processTempo`) and `ModMatrix.h` (route / residual / routed predicate).

- **1 — Step OR trapezoid predictor.** A re-lock requires that the host miss both `prevPpq + n·prevPps` and `prevPpq + n·½(prevPps + pps)` by more than 1 ms. A tempo step and a linear ramp each fall inside one of the two predictions. A true loop or locate misses both by far more than 1 ms. **Accepted** (TT2 step 0 re-locks, TT3 ramp 0 re-locks, TT2 loop / locate 1 re-lock each).
- **2 — Bar-frame change re-anchors.** **Accepted.** One edge to watch in D16 item 6: `barMoved` re-anchors through `adoptHost(false)`, which does **not** bump `relockEpoch`, so `slotNext` is not re-derived when only `barOrigin` moves. During continuous play this happens only on a host bar-structure edit (a locate re-locks and re-derives). If a host reports the new time signature one block early, before its `ppqPositionOfLastBarStart` follows, one onset at the change can land off the downbeat before the next block corrects the origin. That is not a harness case (R56), so D16 item 6 is the gate. If it is heard, the fix is to re-derive the slot on `barMoved` / `sigChanged` (bump a separate epoch).
- **3 — Slot re-derivation only at re-lock / enable / mode / division change.** A plain re-anchor keeps ppq continuous, so keeping the slot is correct and avoids a jittered double. **Accepted** (N8, TS1).
- **4 — TS1n at 133 bpm / 1/16 T.** At 120 bpm the plan's stimulus is vacuous (integer-sample grid). **Accepted.**
- **5 — TS3 telescoping bound.** The 1e-9 mean bound cannot hold mathematically. The bound that replaces it is derived. **Accepted.**
- **6 — TS1 boundary slots (3 excluded).** A slot due inside the last sample before an event fires on the event's first sample. Host reality. **Accepted.**
- **7 — N2n-MAC at −1 %.** +1 % clamps at the range end because mix is 100 % in the recipe. **Accepted** (re-run differs, max\|Δ\| 3.9e-3).
- **8 — Z5 split peak gate.** The output carries `output_gain` +12 dB and modulated mix, so the wet ≤ float(A) and out ≤ √(1+A²)·4 bounds are the right ones. **Accepted.**
- **9 — ZQ2 latched references at the same per-grain span.** A depth-0 reference is not same-motion. **Accepted.**
- **10 — Range table from `getNormalisableRange()`.** Same source; U9c parity 7168/7168. **Accepted.**
- **11 — Commit split** (T6's harness compiles only from `10c8053a`). Bisect-only cost. **Accepted.**
- **12 — Units against the implementation.** Non-vacuity holds because all eight negative controls (TT1n, TS1n, LS2n, U9en, MF7n, PH1n, ZQ2n, N8n) fail their gates in this re-run. **Accepted.**
- **Errata 1–14:** accepted as documented; R44 / R45 / R49 / R50 were signed off at plan.

## Requirements Verification

**Stage:** stage-2 (Phase 2.5a of 7). Written to REQUIREMENTS.md at this verify. FUNC-04 and QUAL-03 close on their harness acceptance criteria; the D16 DAW confirmation is deferred to Stage 3 and reopens them if it fails.

| Requirement | Priority | Status | Acceptance Criteria |
|-------------|----------|-----------------|---------------------|
| FUNC-04: Free + sync timing | must | ✅ Complete (DAW confirmation deferred to Stage 3) | Synced onsets within 1 sample at jitter 0 (TS1 worst 0); Free density within 2 % (carried F-rows, TS6) |
| FUNC-06: Mod matrix | must | ⚠️ Partial | LFO / Env / Macro → every class, bipolar, removal without a step (MF6 / MF7). Pitch / Onset sources live in 2.5b |
| QUAL-03: Smoothing | must | ✅ Complete (DAW confirmation deferred to Stage 3) | ZQ1 / ZQ2 sweeps of position / pitch / cutoff / mix |
| QUAL-01: No artifacts | must | ⚠️ Partial | Added: mod sweeps, route removal (MF7, ZQ2). Open: onset bursts (2.5b), full sweep (2.6) |
| QUAL-02: No NaN / denormal | must | ⚠️ Partial | Added: matrix stress Z5 at 3 rates. Open: 2.6 hardening |
| PERF-01: RT safety | must | ⚠️ Partial → near complete | Added: matrix edits + transport (M5 0 allocs). Open: onset / MPM (2.5b) |
| PERF-02: CPU | must | ⚠️ Partial | C5 **16.63 %** (C4 re-run 17.92 %; equal within run noise). Budget gate 2.6 |
| PERF-03: Block-size invariance | should | ⚠️ Partial → near complete | Added: N8 under modulation + tempo. Open: 2.5b sources |

## Automated Checks

| Check | Result | Notes |
|-------|--------|-------|
| Harness build (touch rebuild) | ✅ Pass | 0 warnings |
| Render harness | ✅ Pass | **299 PASS, 0 FAIL**, rc 0, 3:40 wall. W2o +0.483 dB unchanged |
| N2 `--strict` (vs `42d30b01`) | ✅ Pass | above |
| N1 `--strict` (vs `6aed8017`) | ✅ Pass | free / capped 0; N1c + N1n-MK / KN / FB / FZ / RV differ |
| N0 `--strict` (vs `c10d0f2c`) | ✅ Pass | free / capped 0; N0c + N0n-F / D / C / R differ |
| R12 (vs `6508c635`) | ✅ Pass | free / capped max\|Δ\| 0 |
| `build-and-install.sh O-Granulator` | ✅ Pass | rc 0, 0 warnings, dual-variant sweep, caches cleared |
| auval `aufx OuGn OuDv` | ✅ Pass | AU VALIDATION SUCCEEDED |
| pluginval 10 VST3 (in-process) | ✅ Pass | SUCCESS (fuzzed params hit routes, sync, LFO rates) |
| pluginval 10 AU (in-process) | ✅ Pass | SUCCESS |
| HA host-cache gate (installed VST3) | ✅ Pass | HA1–HA3 |
| Heap ops in TempoTracker / ModSources / ModMatrix | ✅ Pass | 0 (non-comment) |
| `fmod` in TempoTracker / ModSources / StreamScheduler | ✅ Pass | 0 |
| APVTS writes in `Source/dsp` | ✅ Pass | 0 |
| Playhead call sites | ✅ Pass | one: `PluginProcessor.cpp:278` in `processBlock` |
| Locks on the audio path | ✅ Pass | Unchanged: the `copyCustomTable` try-lock only (blocking branch prepare-only) |
| Modules vs `42d30b01` | ✅ Pass | empty |

## Human Verification — D16 DAW tempo check (DAW-2.5a.md)

**Deferred to Stage 3** (Taylor, 2026-11-05; amends D16): the check runs once the WebView UI is in place, not in the generic editor. It joins the 2.5b DAW items (onset bursts, pitch follow) in one Stage 3 DAW pass. A FAIL there reopens FUNC-04 / QUAL-03.

| # | Item | Verdict | Note |
|---|---|---|---|
| 1 | Synced grains vs the click (120 / 133 bpm, 1/16, 1/8 T) | ⏸️ Stage 3 | |
| 2 | Re-timing on start / loop / locate | ⏸️ Stage 3 | |
| 3 | Tempo ramp 60 → 180 bpm | ⏸️ Stage 3 | |
| 4 | LFO sweeps for zipper (pos / pitch / cutoff / mix, 2 + 20 Hz) | ⏸️ Stage 3 | |
| 5 | Synced LFO across a loop point | ⏸️ Stage 3 | |
| 6 | 3/4 project + 4/4 change (R44) | ⏸️ Stage 3 | watch the first grain at the signature change (deviation 2 note) |
| 7 | LFO → reverb_mix through 0; 8 Hz → reverb_size (R54) | ⏸️ Stage 3 | |
| 8 | Logic count-in / pre-roll | ⏸️ Stage 3 | record which behaviour Logic shows |

## Issues Found

- **None blocking** in the automated pass.
- **Watch (D16 item 6):** a bar-origin re-anchor doesn't re-derive the slot. See deviation 2.
- **Carried:** W2o +0.483 dB (0.017 dB margin): widen seeds, never the threshold.
- **Carried to 2.5b:** onset + burst, MPM, pitch follow, Pitch / Onset sources live; N3 null vs the 2.5a tree.
- **Carried to 2.6:** the full PERF-02 gate; `getTailLengthSeconds()`; full QUAL-01 sweep.

## Phase Verdict

**Status:** ✅ VERIFIED (Phase 2.5a, automated). Every automated check passes on an independent re-run, and deviations 1–12 are accepted. The D16 DAW check is deferred to Stage 3 by decision; it is not a failed or skipped gate.

**Ready for next phase:** Yes. Next is Phase 2.5b (onset + burst, MPM, absolute pitch follow).

**Blockers:** none. **Carried to Stage 3:** DAW-2.5a.md items 1–8 (watch item 6).
