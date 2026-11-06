# Stage 2 — DSP · Phase 2.5b (Onset detector + burst, MPM pitch analysis, f0 history ring, absolute pitch follow) — Verification

## Verification Date

2026-11-06

Every check below was re-run in this verify session on a clean plugin tree at `7f2ac5b6`. `Source/`, `tests/`, `CMakeLists.txt` and `modules/` were unchanged since the execute commit. I touch-rebuilt `Engine.cpp` and `PluginProcessor.cpp` first (0 warnings). The harness ran alone, with no concurrent build, so C6 is undisturbed. The nulls, install, validation and HA ran after it, in that order. No result here is copied from SUMMARY-2.5b.md. Logs are in the session scratchpad `v25b/`.

## Goal-Backward Analysis

### Original Goals (CONTEXT-2.5b.md D17–D20 / PLAN-2.5b.md success criteria / ROADMAP 2.5b test criteria)

1. 2.5b plumbing is neutral: N3 out-of-tree null vs `2cf30120`, N3p identical, each N3n differs, analysis live. N2 / N1 / N0 / R12 hold.
2. Onset detector (R60 edge trigger, +3 dB re-arm, refractory) fires once per hit at the detected sample. Bursts: Free re-phase (D14), Tempo extra grain with the grid untouched (D18) (FUNC-08).
3. MPM pitch follower: sine 55–1000 Hz within 10 ¢; clarity hold on silence / noise; D20 octave guard (DSP-08).
4. f0 history ring indexed by write position (D17): grains harmonise against the content they read; a frozen span holds its f0.
5. Absolute pitch follow with `scale_root`; D19 scale Off → chromatic (DSP-01 re-gated on follow).
6. The Pitch and Onset sources modulate any destination; removing a route leaves no step (FUNC-06 completion).
7. RT-safe, block-size invariant with every 2.5b element live, finite at 3 rates, CPU measured (PERF-01/02/03, QUAL-01/02).
8. DAW check: onset bursts on drums, follow on real material (D16 → Stage 3).

### Goal Achievement

| Goal | Status | Evidence (re-run) |
|------|--------|-------------------|
| 1 Neutral null | ✅ Achieved | `tests/n3/run.sh --strict` rc 0: free + capped max\|Δ\| 0. N3c differs (568,972). **N3p-THR / N3p-Z IDENTICAL**. N3n-TRG / ONS / PIT / FOL differ (527,901 / 527,870 / 564,733 / 563,910). Liveness: onsets 22, hops 555, confident 372, f0writes 552. N2 / N1 / N0 `--strict` rc 0, R12 free + capped max\|Δ\| 0 (1168 kills) |
| 2 Onset + burst | ✅ Achieved (harness) | OD0 analysisOff == on bitwise with 38 events (OD0n 911,920 diffs). OD1 exactly 1 event × 8, 4 rates. OD3 min gap 2426, 0 fires at the refractory edge. OD4–OD7 PASS. **OB1** 216/216 re-phases at Δ 0, next grid at event + 6000 exactly, s3 grid intact. **OB2b** TS1 with bursts, worst \|Δ\| 0 (1408 bursts) |
| 3 MPM + guard | ✅ Achieved | **MP1 worst 0.36 ¢** (21/21, all hops confident); MP1i 1500 Hz @ 44.1 k 2.78 ¢ (the R68 model); **MP1h 176.4 / 192 k all confident, worst 0.76 ¢** (P65 watch clear). MP2 0 confident on silence / noise / −55 dBFS sine (max clarity 0.37). MP3 hold exactly 24 hops. **OG2 guard 0 short runs vs guardOff 34**. OG1, OG3 PASS |
| 4 f0 ring (D17) | ✅ Achieved | FR1–FR10 PASS: content f0 at 1 s behind; frozen positions read the frozen f0; R62 newest rule; FR9 midpoint reads B (62.99) vs FR9n start A (57.99); FR4n / FR5n–FR9n controls all fail their gates |
| 5 Absolute follow | ✅ Achieved | AQ1 D19 18/18, worst 0 ¢; AQ2–AQ7 PASS (masked Major / 19-EDO / BP / Custom, nearest ties up, fallbacks, rebuild keys). AQ8 isolated grains 0.06 ¢ (INFO) |
| 6 Pitch / Onset sources | ✅ Achieved | MX1 20/20 class × sign. MX2 removal 8/8 within the P56 bound (MX2n 0.0087 vs bound 0.00095). PS1 Onset source bitwise over 105,000 ticks, 0 beyond 16 τ, 0 subnormal. PS2 / PS3 PASS |
| 7 RT / blocks / finite / CPU | ✅ Achieved | **M6 0 allocations** (Free + Tempo: 743 hops, 942 / 598 lookups, 7 rebuilds, 164 bursts; liveness 1). **N9-F / N9-T** 441 / 512 / 4096 / var == 32 max\|Δ\| 0, events / hops / f0 ring bitwise; **N9n** commitHopImmediately 6902 diffs (0 without). Z6a 60 s × 48 / 44.1 / 192 k finite, 0 subnormal, wet ≤ 1.60, out ≤ 8.31. Z6b NaN / +Inf / 1e30: 1 scrub each, audio bitwise. **C6 15.30 %** (+1.07 vs the C5 re-run of 14.23 %; ceiling 25 %). C6s hop excess 7.29 µs = 1.1 % of the block budget |
| 8 DAW check | ⏸️ Deferred | To Stage 3 with DAW-2.5a.md (Taylor's 2.5a decision; CONTEXT-2.5b). DAW-2.5b.md items 1–5 |

## Deviation Review (SUMMARY-2.5b execution deviations 1–15)

I read `Analysis.h` (tap sanitiser in `processPiece`, `Mpm::analyse` peak pick), `ScaleQuantiser.h` (`AbsSet::rebuildIfNeeded`) and `Engine.cpp` (`updateScaleState` follow-set key).

- **1 — N3 stimulus scaled on both sides.** The plan's bursts never crossed the threshold, so the liveness precondition failed. The comparison is unchanged and is old-vs-new on the same stimulus. **Accepted** (liveness 22 onsets in the re-run).
- **2 — Tap sanitiser `!(|a| <= kAnalysisTapMax)`.** This catches non-finite input and absurd finite input (1e30) in one branch. 1e4 is +80 dBFS, far beyond anything a conditioned input reaches. Without the bound, one 1e30 sample pins the slow envelope for about 6 s. **Accepted** (Z6b 1 scrub each, audio bitwise).
- **3 — MPM key maxima must be local maxima.** `nsdf` is filled for `t ≤ H`, and the guard `best < H` keeps `best + 1` in range; `best > 0` covers `p − 1`. This rejects the rising edge of a region still positive at the horizon. It only affects content below about 40 Hz, which is outside DSP-08's range. **Accepted** (MP0 PASS against the reference with the same rule; MP1i 40 Hz 0.00–0.01 ¢).
- **4 — AbsSet custom-version key on Custom only.** `Engine.cpp:308` passes `customCopyVersion` only when the choice resolves to `PitchCollections::Custom`, else 0. Built-in follow sets never rebuild on a custom apply. **Accepted** (AQ7 4 rebuilds).
- **5–7, 11–13 — Stimulus / density re-derivations** (OB1 8 g/s, OD4 5 ms bursts, Z6a structured stimulus, AQ2 mask, AQ8 isolated grains, FR7 / FR10 / AQ2 counts). Each is needed to make its gate observable and non-vacuous; none loosens a threshold. **Accepted.**
- **8 — MF6 detail value-free (baseline A′).** The line no longer embeds a value that legitimately moves when the Pitch source goes live. The gate itself (differs, routedDests ≥ 1) is unchanged. **Accepted.**
- **9, 10 — Landing / commit grouping.** Bisect-only cost; each N3n still flipped at its own task. **Accepted.**
- **14 — PS4 145 ms** to half a 5-st step (R69 projected ≈ 95 ms). This is an INFO row. The extra lag comes from unconfident straddling windows, and that is correct behaviour. **Accepted.** The Stage 3 UI copy should say ≈ 150 ms.
- **15 — Units against the implementation.** Non-vacuity holds because every negative control (OD0n, OD1n, OG2n, FR4n–FR9n, MX2n, QB1n, N9n, N3n ×4) fails its gate in this re-run. **Accepted.**
- **Errata 1–23:** accepted as documented. R60 / R64 / R66 / R71 were signed off at plan.

## Requirements Verification

**Stage:** stage-2 (Phase 2.5b of 7). Written to REQUIREMENTS.md at this verify. FUNC-06, FUNC-08 and DSP-08 close on their harness acceptance criteria. The real-material DAW confirmation is deferred to Stage 3; a FAIL there reopens FUNC-08 / DSP-08.

| Requirement | Priority | Status | Acceptance Criteria |
|-------------|----------|--------|---------------------|
| FUNC-06: Mod matrix | must | ✅ Complete (partial → complete) | Every source, including Pitch and Onset (MX1), moves every class bipolar; removal without a step (MX2, MF7) |
| FUNC-08: Onset bursts | should | ✅ Complete (DAW confirmation deferred to Stage 3) | Burst at the detected sample (OB1 Δ 0); Tempo grid untouched (OB2 / OB2b); one event per hit (OD1 / OD3 / OD7) |
| DSP-08: Pitch follower | should | ✅ Complete (DAW confirmation deferred to Stage 3) | Sines 55–1000 Hz ≤ 10 ¢ (worst 0.36 ¢); clarity hold on silence / noise (MP2 / MP3); usable as source (PS2, MX1) |
| DSP-01: Pitch + quantisation | must | ✅ Complete (re-gated) | Follow targets on degrees within 0.001 ¢ (AQ1–AQ4), ±24 st (AQ4) |
| QUAL-01: No artifacts | must | ⚠️ Partial | Added: burst click QB1 1.05× / 1.20× vs the reference (QB1n 2.68×). Open: the full 2.6 sweep |
| QUAL-02: No NaN / denormal | must | ⚠️ Partial | Added: Z6a / Z6b analysis stress at 3 rates. Open: 2.6 extreme render |
| PERF-01: RT safety | must | ⚠️ Partial → near complete | Added: M6 0 allocs over every 2.5b path; no heap / fmod in Analysis.h. Open: 2.6 whole-feature gate |
| PERF-02: CPU | must | ⚠️ Partial | C6 **15.30 %**. Budget gate in 2.6 |
| PERF-03: Block-size invariance | should | ⚠️ Partial → near complete | Added: N9 with every 2.5b element. Open: 2.6 oversized host block |

## Automated Checks

| Check | Result | Notes |
|-------|--------|-------|
| Harness build (touch rebuild) | ✅ Pass | 0 warnings |
| Render harness | ✅ Pass | **388 PASS, 0 FAIL**, rc 0, 267 s wall. W2o +0.483 dB unchanged |
| N3 `--strict` (vs `2cf30120`) | ✅ Pass | above |
| N2 `--strict` (vs `42d30b01`) | ✅ Pass | free / capped 0; N2c, N2n ×4 differ, N2p identical |
| N1 `--strict` (vs `6aed8017`) | ✅ Pass | free / capped 0; controls differ |
| N0 `--strict` (vs `c10d0f2c`) | ✅ Pass | free / capped 0; controls differ |
| R12 (vs `6508c635`) | ✅ Pass | free / capped max\|Δ\| 0 |
| `build-and-install.sh O-Granulator` | ✅ Pass | rc 0, 0 warnings, caches cleared |
| auval `aufx OuGn OuDv` | ✅ Pass | AU VALIDATION SUCCEEDED |
| pluginval 10 VST3 (in-process) | ✅ Pass | SUCCESS |
| pluginval 10 AU (in-process) | ✅ Pass | SUCCESS |
| HA host-cache gate (installed VST3) | ✅ Pass | HA1 79/79, HA2 142 untouched unchanged, HA3 79/79 (220 PARAM). Run with the script's documented `uv run --no-project --python 3.12 --with pedalboard --with numpy` line |
| Heap ops / `fmod` in `Analysis.h` | ✅ Pass | 0 / 0 |
| FFT | ✅ Pass | One `juce::dsp::FFT` (the `Mpm` member, `Analysis.h:399`); transforms only in `Mpm::analyse` |
| APVTS writes in `Source/dsp` | ✅ Pass | 0 |
| Playhead call sites / latency | ✅ Pass | one (`PluginProcessor.cpp:284`, `processBlock`); `setLatencySamples (0)` unchanged |
| Modules + `scripts/param-dump` vs `2cf30120` | ✅ Pass | empty |

## Human Verification — DAW pass (DAW-2.5b.md)

**Deferred to Stage 3.** These items join DAW-2.5a.md items 1–8 in one pass with the WebView UI in place.

| # | Item | Verdict |
|---|---|---|
| 1 | Onset bursts on drums, Free and Tempo | ⏸️ Stage 3 |
| 2 | Follow on a monophonic line, position 0 and 2 s | ⏸️ Stage 3 |
| 3 | Frozen-chord harmonisation | ⏸️ Stage 3 |
| 4 | Feedback-heavy follow (R74) | ⏸️ Stage 3 |
| 5 | Tempo burst level (R78) | ⏸️ Stage 3 |

## Issues Found

- **None blocking.**
- **Note (tooling):** `tests/hostcache/run.py` fails under a plain `uv run` / `python3` (no pedalboard). Use the invocation in its header.
- **Carried:** W2o +0.483 dB (0.017 dB margin). The rule is to widen the seeds, never the threshold.
- **Carried to 2.6:** full PERF-02 budget gate; `getTailLengthSeconds()`; full QUAL-01 click sweep (route edits under all sources, burst steal in a capped pool); full-feature PERF-03 null incl. the oversized host block; 96 / 192 k smoke renders.
- **Carried to 3.x UI copy:** Pitch source ≈ 75 ms decided-entry lag, ≈ 150 ms to swing halfway (PS4); tracking range 40–1500 Hz.

## Phase Verdict

**Status:** ✅ VERIFIED (Phase 2.5b, automated). Every automated check passes on an independent re-run, and deviations 1–15 are accepted. The DAW pass is deferred to Stage 3 by decision; it is not a failed or skipped gate.

**Ready for next phase:** Yes. Next is Phase 2.6 (Hardening).

**Blockers:** none. **Carried to Stage 3:** DAW-2.5a.md items 1–8 (watch item 6) + DAW-2.5b.md items 1–5.
