# Stage 2 — DSP · Phase 2.6 (Hardening: QUAL-01 / QUAL-02 / PERF-01 / PERF-02 / PERF-03, infinite tail, 96 / 192 k smoke) — Verification

## Verification Date

2026-11-06

Every check below was re-run in this verify session on a clean tree at `a8fdd831`. `git diff a8fdd831 HEAD` on `Source/`, `tests/`, `CMakeLists.txt` and `modules/` was empty, and all six contract checksums matched the STATUS frontmatter. I touch-rebuilt `Engine.cpp` and `PluginProcessor.cpp` first (0 warnings). The default harness ran alone with nothing else building, so C7 was undisturbed. The N-suites, install, validation, HA and the `--long` Z7d section ran after it, one at a time. No result here is copied from SUMMARY-2.6.md. Logs are in the session scratchpad `v26/`.

## Goal-Backward Analysis

### Original Goals (CONTEXT-2.6.md D21–D25 / PLAN-2.6.md success criteria 1–12)

1. Carried rows identical to A″ (A‴ for the attributed detail moves). N3 / N2 / N1 / N0 `--strict` + R12 PASS. W2o unchanged (D24).
2. ∞ tail through the installed AU and VST3 (D21).
3. Route edits are click-free under every source, on every destination and application path (QUAL-01).
4. Enable / mute / solo, freeze, steal / kill and burst steal / kill are click-free under the full feature set (QUAL-01).
5. No NaN / Inf / subnormal under extreme, min-side and fuzzed settings at 4 rates. The census reaches every persistent state, and FTZ is shown to be load-bearing (QUAL-02).
6. A non-finite or absurd host sample can't poison the plugin (R96, QUAL-02).
7. 0 allocations over the whole feature, armed from block 0, with variable blocks and a fresh 8192 block (PERF-01).
8. Worst case ≤ 30 % of one core at 48 k / 128. `grain_limit` caps the pool and is deterministic (PERF-02, D22).
9. Full-feature block null, including the oversized host block on a fresh instance (PERF-03).
10. 96 / 192 k smoke: analysis is load-independent, coefficients stay ≤ 0.45·fs, ring bytes match the documented figure.
11. Static: guards at every ring push and before the reverb; no new heap use or APVTS write; one playhead site; latency 0; modules / params / golden unchanged.
12. Install, auval, pluginval 10 VST3 + AU, HA; 0 new warnings; DAW-2.6.md ready.

### Goal Achievement

| Goal | Status | Evidence (re-run) |
|------|--------|-------------------|
| 1 Regression | ✅ Achieved | Harness **463 PASS, 0 FAIL**, rc 0, 440 s. Every carried verdict PASS. F7 PASS under its signed-off `noConverge` flag. W2o **+0.483 dB** unchanged. N3 / N2 / N1 / N0 `--strict` rc 0 (free + capped max\|Δ\| 0; N3p-THR / N3p-Z / N2p identical; every Nn control differs). R12 free + capped max\|Δ\| 0 |
| 2 ∞ tail | ✅ Achieved | TA1 +∞ / +∞ before / after prepare, latency 0. auval `VERIFYING PROPERTY: Tail Time` → PASS. pluginval 10 VST3 + AU `Reported taillength: inf`, SUCCESS. Fallback (b) not needed |
| 3 Route edits | ✅ Achieved | MX3a–g 77/77 each, worst 1.000× of the analytic bound, cross-talk 0. MX3nR / MX3nD > 2× for every source. MF8a–f 20/20 each, worst gated 1.444× (MF8c; the 2.130× printed beside it is the INFO plain κ, per deviation 6). MF8n 2.09× |
| 4 Toggles / steals | ✅ Achieved | EN1a–c worst 0.732×, EN1n 36×. FZ9a 1.138× / FZ9b PASS, NCs 4.54× / 2.18×. K3a 1.636× (321 kills), K3b 1.208× (797 steals), hard cuts 48.3× / 19.7×. QB3a 1.351× (128 burst kills), QB3b 0.860×, NCs 48.4× / 16.4× |
| 5 Extreme / census | ✅ Achieved | Z7s planted 1e-40f / NaN seen 1 / 1, every state scanned at its declared size. Z7a ×4 rates PASS. Z7m1–8 PASS. Z7m9 reach 0 / Z7m9n 3108 violations (old floor). Z7f 64/64 at 44.1 / 48 / 96 / 192 k. **Z7d** exact zero from 295.91 s, 0 subnormal over 340 scans; **Z7dn** sees subnormals at 11 s with FTZ off |
| 6 Host input | ✅ Achieved | Z7h1 / h2 / h3 12/12 each (bitwise == zero-sample reference after the bad sample). Z7h4 and Z7h5 4/4 PASS. **Z7hn reproduces R96** with both guards off. Z7hr: ring poisoned, wet guard holds the output finite |
| 7 Allocations | ✅ Achieved | M7L liveness 1. **M7F / M7T 0 allocs** (Free / Tempo × var / fresh 8192), armed from block 0. M7c: every feature counter ≥ 1 |
| 8 CPU + cap | ✅ Achieved | **C7 median 17.29 %** (17.16–17.43, N 7, 0 contended, duty ≥ 0.998; p99 498 µs of 2667 µs). C7L: 19.2 events/s, MPM 100 % confident, 834 steals/s, maxAlive 128. GL1, GL2 4/4 bitwise, GL2c 4/4 differ. GL3: 32 live from 3.25 s; GL3n stays at 128 |
| 9 Block null | ✅ Achieved | **N10F / N10T** 441 / 512 / 4096 / var / 8192 (prep 512, prep 1) vs 32: max\|Δ\| 0; events, hops and f0 ring identical; canaries 6/6. N10c / N10n differ. O2 PASS (2463 spawns) |
| 10 High rate | ✅ Achieved | HR1 hop log bitwise under C7 load at 96 / 192 k. HR2 every fcHz / ringHz ≤ 0.45·fs (44.1 k binds at 19 845). HR3 ring bytes 25 315 320 / 27 553 920 / 55 105 920 / 110 209 920 |
| 11 Static | ✅ Achieved | Automated Checks table below |
| 12 Install / validation | ✅ Achieved | Automated Checks table below. DAW-2.6.md items 1–3 are ready for the Stage 3 pass |

## Deviation Review (SUMMARY-2.6 execution deviations 1–12)

I read the `Spawner.h` and `ScaleQuantiser.h` diffs against `454c8a31`, `CaptureWriter.h` (both push sites), the wet guard in `Engine.cpp`, and `ModSources.h` (the env follower).

- **1 — Over-cap convergence kill in `Spawner::acquire`.** The branch runs only when `alive > grainLimit && ! noConverge`. It skips the new slot and grains that are already killing, picks the victim in the same priority / progress order as the R3 path, and is bounded at one extra kill per acquire. It is O(kPoolLive) and only while over the cap. A start-at-cap render never enters it. The F7 `noConverge` flag was signed off at execute. **Accepted.** A‴ accepted by Taylor at this verify (decision b).
- **2 — Deferred-freeze floor `max(r, 1)`.** The comment's derivation is correct: gap = D − headLead + a(1 − r), minimised at a = 0 when r < 1. The result is identical for r ≥ 1, so no carried row moved. Z7m9 / Z7m9n reproduce and fix the case. **Accepted.** Erratum to RESEARCH-2.4 R24 / PLAN-2.4 P38.
- **3 — `ActiveSet` loops on `! truncated`.** This is the AbsSet form: same members, correct flag. **Accepted.**
- **4–9 — Q-set re-derivation, FZ9 geometry, MF8 measure, HR2 effective-fc form, MX3 composite input, Z7m4 / N10T / M7 script splits.** Each is needed to make its gate observable and non-vacuous. No threshold was loosened (MF8 stays 2.0×; κ NCs are live at 2.09–48×). **Accepted.**
- **10 — Iteration aids `--no-c7` / `--sect=`.** Acceptance runs don't use them. This verify used `--sect=z7dRows` only to run the `--long` decay section alone. **Accepted.**
- **11 — Z6a's freeze events are dropped** (`renderPool` ignores `Job::events`). Z6a's freeze arm is vacuous, but freeze toggling under stress is covered by Z7a / Z7m / FZ9 through the event-aware pool. **Carried as a harness item** (Taylor, decision d).
- **12 — Row counts** (463 default / 465 `--long`). **Accepted.**
- **Errata 1–10:** accepted as documented. R83 / R84 / R93 / R96 / R80 / R85 were signed off at plan.

## Decisions at verify (Taylor, 2026-11-06)

| # | Item | Decision |
|---|---|---|
| a | Env 1e30 pin: Env source ≥ 0.999 for ≈ 11 s after one finite 1e30 host sample (Z7h3 INFO: 3791 / 4125 / 8250 / 16 500 cells) | **Carry to Stage 4.** Add the `\|x\| ≤ kAnalysisTapMax` predicate at the env input (`ModSources.h` `process`), which is bitwise-neutral ≤ +80 dBFS. Finite and self-recovering, so not blocking |
| b | A‴ (M1–M4 / N8n detail counts moved by deviation 1) | **Accepted** as the baseline |
| c | C7 duty witness can't see a slowed or shared core | **Carried as a harness item:** add a fixed-work calibration loop timed before and after the run, so a contended run reads `[NOVERDICT]`. C7 is accepted on the quiet runs (execute 17.08 % / 16.37 %, this verify 17.29 %) |
| d | Z6a dropped events | **Carried as a harness item:** make `eg22b::renderPool` event-aware. Coverage is provided by Z7a / Z7m / FZ9 meanwhile |

## Requirements Verification

**Stage:** stage-2 (Phase 2.6 of 7, the last). Written to REQUIREMENTS.md at this verify. Every stage-2 requirement is now complete.

| Requirement | Priority | Status | Acceptance Criteria |
|-------------|----------|--------|---------------------|
| QUAL-01: No artifacts | must | ✅ Complete (partial → complete) | Click gates pass on freeze toggle (FZ2, FZ9), grain steal / kill and burst steal / kill (K1, K3, QB1, QB3), enable / mute / solo (X1, EN1), route edits under every source (MX2, MX3, MF7, MF8). Each is taken against a same-motion reference, with a live NC |
| QUAL-02: No NaN / denormal | must | ✅ Complete (partial → complete) | Extreme / min-side / fuzz at 4 rates (Z7a, Z7m1–9, Z7f); census reaches every state (Z7s); FTZ shown load-bearing (Z7dn); host NaN / Inf / 1e30 can't poison the plugin (Z7h, R96 guards) |
| PERF-01: RT safety | must | ✅ Complete (partial → complete) | 0 allocations over the whole feature + event script, var + fresh 8192, armed from block 0 (M7); static heap / APVTS / playhead greps clean |
| PERF-02: CPU + cap | must | ✅ Complete (partial → complete) | D22 worst case **17.29 %** ≤ 30 % at 48 k / 128; the cap engages, is deterministic (GL1 / GL2) and converges when lowered live (GL3) |
| PERF-03: Block-size invariance | should | ✅ Complete (partial → complete) | Full-feature null 441 / 512 / 4096 / var / 8192-after-512 / 8192-after-1 vs 32, max\|Δ\| 0 (N10F / N10T); O2 |

Stage-2 roll-up: FUNC-01–08, DSP-01–08, PERF-01–03 and QUAL-01–03 are complete. The Stage 3 DAW pass is a confirmation pass: a FAIL there reopens the requirement its checklist names (DAW-2.5a → FUNC-04 / QUAL-03; DAW-2.5b → FUNC-08 / DSP-08; DAW-2.6 item 1 → D21 fallback 600 s).

## Automated Checks

| Check | Result | Notes |
|-------|--------|-------|
| Contract checksums | ✅ Pass | all six match (brief `f9c748a8`, requirements `5f481caa`, draft `d0b1457e`, param-spec `f5ce3595`, architecture `f0cf753b`, roadmap `1060dae0`) |
| Harness build (touch rebuild) | ✅ Pass | 0 warnings |
| Render harness (default) | ✅ Pass | **463 PASS, 0 FAIL**, rc 0, 440 s wall |
| Render harness `--long`, Z7d section (`--only26 --long --sect=z7dRows`) | ✅ Pass | 2/2, rc 0, 41 s. **Z7d** (FTZ on): last non-zero output 295.91 s (R81 295.63 s; execute 295.46 s — section-only run vs full `--long`, both well inside the 330 s gate), 340 scans, 0 subnormal, 0 non-finite. **Z7dn** (FTZ off): first subnormal scan at 11 s (ring 9.0 M, tank 1.07 M) → census non-vacuous, FTZ load-bearing |
| N3 `--strict` (vs `2cf30120`) | ✅ Pass | free / capped 0; N3p ×2 identical; N3n ×4 differ; liveness: onsets 22, hops 555 |
| N2 `--strict` (vs `42d30b01`) | ✅ Pass | free / capped 0; N2p identical; N2n differ |
| N1 `--strict` (vs `6aed8017`) | ✅ Pass | free / capped 0; controls differ |
| N0 `--strict` (vs `c10d0f2c`) | ✅ Pass | free / capped 0; controls differ |
| R12 (vs `6508c635`) | ✅ Pass | free / capped max\|Δ\| 0 (1168 kills) |
| `build-and-install.sh O-Granulator` | ✅ Pass | rc 0, 0 warnings, caches cleared |
| auval `aufx OuGn OuDv` | ✅ Pass | AU VALIDATION SUCCEEDED; **Tail Time PASS**; 4 pre-existing LFO 1–4 Rate "did not retain default value" warnings (skewed range, value identical, each parameter PASS) — unchanged since Stage 1 |
| pluginval 10 VST3 (in-process) | ✅ Pass | SUCCESS, `Reported taillength: inf` |
| pluginval 10 AU (in-process) | ✅ Pass | SUCCESS, `Reported taillength: inf` |
| HA host-cache gate (installed VST3) | ✅ Pass | HA1 79/79, HA2 142 untouched, HA3 79/79 (220 PARAM) |
| `ring.push` sites | ✅ Pass | 2, both in `CaptureWriter.h` (`:144`, `:155`), on guarded `xl` / `xr` |
| Wet guard before `reverb.process` | ✅ Pass | `Engine.cpp` step 7, always on outside test hooks |
| `getTailLengthSeconds` | ✅ Pass | `numeric_limits<double>::infinity()`, `<limits>` included |
| `ScopedNoDenormals` | ✅ Pass | first statement of `processBlock` (`PluginProcessor.cpp:253`) |
| Heap / thread tokens in `Source/dsp` | ✅ Pass | 2 hits, both comments (`ModMatrix.h:211`, `StreamScheduler.h:223`) |
| APVTS writes in `Source/dsp` | ✅ Pass | 0 |
| Playhead sites / latency | ✅ Pass | one (`PluginProcessor.cpp:284`); `setLatencySamples (0)` |
| modules / param-dump / ParamIds / golden / hostcache vs `454c8a31` | ✅ Pass | empty |

## Human Verification — DAW pass (DAW-2.6.md)

**Deferred to Stage 3** (D23: 2.6 verify is fully automated). These items join DAW-2.5a.md items 1–8 and DAW-2.5b.md items 1–5 in one pass with the WebView UI.

| # | Item | Verdict |
|---|---|---|
| 1 | ∞ tail in Logic / Live / Reaper (bounce, playback past the region, stopped transport) | ⏸️ Stage 3. A FAIL triggers fallback (b), 600 s |
| 2 | Drums over a sustained pad (R85 re-arm watch) | ⏸️ Stage 3, recorded behaviour |
| 3 | 192 kHz CPU | ⏸️ Stage 3 |

## Issues Found

- **None blocking.**
- **Carried to Stage 4:** the Env 1e30 pin (decision a).
- **Carried harness items:** C7 machine-speed calibration (decision c); event-aware `renderPool` for Z6a (decision d).
- **Carried rule:** W2o +0.483 dB (0.017 dB margin). Widen the seeds, never the threshold.
- **Carried to 3.x UI copy:** "at 192 kHz the worst case uses about ⅔ of a core" (C7 192 k 63.4 %, execute `--long`). Pitch source ≈ 75 ms decided-entry lag, ≈ 150 ms to swing halfway; tracking range 40–1500 Hz.
- **After the Stage 3 DAW pass:** decide whether to revisit R60 re-arm under sustained tones (R85).

## Phase Verdict

**Status:** ✅ VERIFIED (Phase 2.6, automated). Every automated check passes on an independent re-run. Deviations 1–12 are accepted, and the four verify decisions are recorded.

## Stage 2 Verdict

**Status:** ✅ VERIFIED. Phases 2.1, 2.2, 2.3, 2.4, 2.5a, 2.5b and 2.6 are each verified (VERIFICATION-2.1 … 2.6), and every stage-2 requirement is complete.

**Ready for next stage:** Yes. Next is Stage 3 (GUI), Phase 3.1.

**Blockers:** none. **Carried to Stage 3:** the DAW pass (DAW-2.5a 1–8 with watch item 6, DAW-2.5b 1–5, DAW-2.6 1–3) and `mockups/v2-integration-checklist.md`.
