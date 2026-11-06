# Stage 2 — DSP · Phase 2.6 (Hardening: QUAL-01 / QUAL-02 / PERF-01 / PERF-02 / PERF-03, infinite tail, 96 / 192 k smoke) — Summary

**Plugin:** O-Granulator · **GSD phase:** execute · **Date:** 2026-11-06 · **Branch:** `main`
**Inputs:** PLAN-2.6.md (T0–T13, P79–P96) · RESEARCH-2.6.md (R79–R103) · CONTEXT-2.6.md (D21–D25) · sign-offs 1–6 (Taylor, at plan) · **one sign-off at execute** (Taylor, 2026-11-06: keep the cap-convergence fix, adapt carried F7 — see deviation 1)

**Entry check:** the six contract checksums matched the STATUS frontmatter (brief `f9c748a8…`, requirements `5f481caa…`, param-spec draft `d0b1457e…`, param-spec `f5ce3595…`, architecture `f0cf753b…`, roadmap `1060dae0…`); `git diff --stat 454c8a31 HEAD -- Source tests CMakeLists.txt` empty; `modules` and `scripts/param-dump` clean.

**Commits:**
- `e66159d2` T1 — D21 ∞ tail; TA1; harness `--only26` / `--long` / rc 2. Validated on the installed binaries before any other Source task (R80 checkpoint: ∞ kept).
- `894d028c` T2–T3 — hooks, accessors, census + Z7s; R96 ring-write + wet guards; Z7h1–5 / Z7hn / Z7hr (harness exactly as validated at T3, 397/397).
- this commit — T4–T13: `engine_gates_26b.h` (C7, GL, MX3, MF8, EN1, FZ9, K3, QB3, M7, N10, O2), Z7a / Z7m / Z7f / Z7d / HR in `engine_gates_26.h`, the three D25 fixes (deviations 1–3), F7 policy flag (signed off), DAW-2.6.md, this summary, STATUS.

## What was built

| File | Change |
|---|---|
| `Source/PluginProcessor.h` | D21: `#include <limits>`; `getTailLengthSeconds()` = `+∞`; banner 2.1–2.6 |
| `Source/dsp/CaptureWriter.h` | R96 ring-write guard: per channel `x = ! (|x·g| <= kAnalysisTapMax) ? 0 : x` in its own statement (the predicate product separate), push arithmetic verbatim (`x·g + fb` stays one contracted expression) → bitwise ≤ +80 dBFS; `write()` returns the guarded count, trailing `sanitise` |
| `Source/dsp/Engine.cpp` | wet guard (`! isfinite` → 0) before `reverb.process`, always on; `hostScrubs` / `wetScrubs`; test-build fade arguments for `pool.prepare` / `spawner.prepare`; `noDepthRamp` copy; capKill counting; `deferFloorR` hook |
| `Source/dsp/Spawner.h` | `prepare (fs, killMs)`; **D25: over-cap convergence kill** in `acquire` (dev. 1); **D25: deferred-stop floor `max (r, 1)·(headLead + 1)`** in `frozenDelay` (dev. 2); `AcquirePolicy::noConverge`, `FrozenGeom::deferFloorR`, `Acquired::capKill` |
| `Source/dsp/ScaleQuantiser.h` | **D25: ActiveSet truncation flag** — loops on `! truncated` (the AbsSet form) (dev. 3) |
| `Source/dsp/ModMatrix.h` | `noDepthRamp` member + branch (else-branch verbatim) |
| `Source/dsp/Engine.h` | TestHooks `noDepthRamp`, `stealFadeMs`, `killFadeMs`, `hostGuardOff`, `wetGuardOff`, `capNoConverge`, `deferFloorR`; Diag `hostScrubs`, `wetScrubs`, `capKills`; `getFbDelayForTest()`, `getReverbForTest()` (all in `#if OGRAN_TEST_HOOKS`); banner |
| `Source/dsp/EngineConstants.h` | `kAnalysisTapMax` comment (host-input sanitiser: tap + ring); banner |
| `tests/render-harness/engine_gates_26.h`, `engine_gates_26b.h` **(new)** | census (P83, reverb-unit privates via the explicit-instantiation idiom — no module change), FTZ-off path, C7 / Q builders, 75 default rows + 2 `--long` |
| `tests/render-harness/main.cpp` | `--only26`, `--long`, rc 2; iteration aids `--no-c7`, `--sect=` (dev. 10); banner 2.1–2.6 |
| `tests/render-harness/engine_gates_22.h` | **F7 only:** `policy.noConverge = true` (signed off at execute) |
| `stages/2-dsp/DAW-2.6.md` **(new)** | P96 items 1–3 |

No change: `modules/`, `scripts/param-dump`, `GranulatorParamIds.h`, `tests/golden`, `tests/hostcache`, `tests/n0–n3`, `tests/r12`. Latency 0.

## Gate results

**Harness (default): 463 PASS, 0 FAIL** (388 carried + 75 new), 7 min 26 s. **`--long`: 465 rows, 464 PASS + C7 contended (see below)**; Z7d / Z7dn PASS. 0 warnings (harness, plugin build, install log).

**Carried rows vs A″** (A″ = two identical 388/388 runs on the `454c8a31` tree, T0): **empty at T1, T2, T3** (397/397 — the R96 guard is bitwise-neutral on every carried stimulus). Final: **every carried verdict identical; F7 PASS** with its signed-off policy flag; detail-only moves, all from the cap-convergence fix (deviation 1), recorded as **A‴**:
- M1 / M2 / M3 / M4 (allocation rows, 0 allocs unchanged): steal / kill split moves where those scripts lower `grain_limit` mid-render (e.g. M1 steals 1073 → 474, kills 903 → 1566; M4 forced releases 127 → 80).
- N8n (control): diff count 49 019 → 47 520 (still breaks the null).
- W2o unchanged (+0.483 dB).

**N-suite set** (`n3`, `n2`, `n1`, `n0` `--strict`, `r12`): rc 0 at T0, T1, T2, T3 and final.

### New rows (final default run unless noted)

| Row | Result | Measured |
|---|---|---|
| TA1 | PASS | tail +∞ / +∞ (before / after prepare), latency 0 |
| Z7s | PASS | planted 1e-40f / NaN seen 1 / 1; all 16 states scanned at their declared size (diffuser 33 336, ring 168 000, fbDelay 64, tank 26 624, …); 0 / 0 |
| Z7h1 / 2 / 3 | PASS 12/12 each | NaN / +Inf / 1e30 × {L, L+R, mono} × 4 rates: bitwise == zero-sample reference after `at`, census non-finite 0, hostScrubs exact |
| Z7h3 INFO | — | Env source pinned ≥ 0.999 for ≈ 11 s after one 1e30 sample (3791 / 4125 / 8250 / 16 500 cells of 128 at 44.1 / 48 / 96 / 192 k, vs 0) — **decision at verify** |
| Z7h4 | PASS | NaN inside the release crossfade (`xfading` 1): 0 diffs, host 1 |
| Z7h5 | PASS 4/4 | C7, 3 injections: finite from each `at + 1`, hostScrubs 4 (the injected samples reach the output on the dry path — out of scope, P81) |
| Z7hn | PASS (reproduces R96) | 500 621 non-finite out, last block bad, ring 160 / tank 26 624 (all) poisoned |
| Z7hr | PASS | ring poisoned (160), wetScrubs 128, tank 0, output finite |
| C7 | **PASS** | **median 17.08 %** (16.94–17.15, N 7, 0 contended, duty ≥ 1.000); per block p50 455 µs, p99 501 µs (budget 2667 µs). Re-run alone: 16.37 % (15.99–16.58) |
| C7L | PASS | events 19.2 /s, confident 6563 / 6563, steals 834 /s, maxAlive 128, 131 active |
| C7 `--long` INFO | — | literal all-max 17.36 % (1 contended run); 44.1 k 14.57 %; 96 k 32.83 %; **192 k 63.41 %** (p99 475 µs of 667 µs); caps 32 / 64 / 96: 4.81 / 8.36 / 12.03 % |
| GL1 / GL2 / GL2c | PASS | maxAlive = scan = cap at 32 / 64 / 96 / 128; kills ≈ 8250 below 128, steals 8197 at 128; same seed bitwise (logs + audio) 4/4; seed + 1 differs 4/4 |
| GL3 / GL3n (new, dev. 1) | PASS | cap lowered 128 → 32 at 3 s under C7: 32 live at 3.25 s and after (195 capKills); control stays at 128 |
| MX3a–g | PASS 77/77 each | 11 × 112 × 7 kinds; worst 1.019× of the analytic bound; 0 cross-talk |
| MX3nR / MX3nD | PASS | every source 13.9–29.8× (noResidual); add 13.9–29.8×, step 13.9–25.0× (noDepthRamp) |
| MF8a–f | PASS 20/20 each | worst 1.444× (LFO step); plain κ INFO worst 2.13× (dev. 6) |
| MF8n | PASS | mix 2.39×, s1_level 2.09× |
| EN1a–c / EN1n | PASS | worst 1.153×; s2 spawns within 20 ms of on: 5; hard-switch model 12.5–73× |
| FZ9a / b / an / bn | PASS | engage 1.138× (31 forced releases), release 1.005×; NCs 4.54× / 2.18× |
| K3a / b / an / bn | PASS | kills 1.636× (321 in window), steals 1.208× (797); hard cuts 48.3× / 19.7× |
| QB3a / b / an / bn | PASS | burst kills 1.351× (128 `burst && killed`, 56 events), burst steals 0.860× (128); hard cuts 48.4× / 16.4× |
| Z7a | PASS ×4 rates | 90 s: wet ≤ 1.178 (≤ A), out ≤ 4.69 (≤ bound), census 0 / 0 over 90 scans, 0 scrubs / reach, 73 750 steals, 1201 events, 3 engages |
| Z7m1–8 | PASS | 48 + 192 k; Z7m3 reach clamp 461 / 186 grains, 0 violations; Z7m4 kills 15 541 (cap 32) and steals 15 415 (cap 128); Z7m8 truncation seen (dev. 3) |
| Z7m9 / Z7m9n (new, dev. 2) | PASS | freeze deferral with 5 ms grains at −18 st: reach 0 (block 0 + mid-run, 48 / 192 k); old floor: 960 / 3540 (block 0), 3108 (mid-run 192 k) |
| Z7f ×4 | PASS 64/64 each | after dev. 2 (before it: 61/64 at every rate — states 13 / 22 / 60, reach violations) |
| Z7d (`--long`) | PASS | last non-zero output 295.46 s (R81: 295.63 s); 340 scans, 0 subnormal, 0 non-finite |
| Z7dn (`--long`) | PASS | FTZ-off probe 1; first subnormal scan at 11 s (ring 8.9 M, tank 1.07 M, …) |
| M7L / M7F / M7T / M7c | PASS | 1; 0 allocations in all four renders (Free / Tempo × var / fresh 8192), armed from block 0; every counter ≥ 1 |
| N10F / N10T | PASS 6/6 each | max\|Δ\| 0 at 441 / 512 / 4096 / var / 8192 (prep 512) / 8192 (prep 1); events, hops, f0 ring identical; reverb running → idle → re-enable seen |
| N10c / N10n | PASS | seed + 1 536 381 / 449 096 diffs; perBlockOrigin 277 239 diffs |
| O2 | PASS | finite, host guards, canaries, reach 0, 2463 spawns |
| HR1 | PASS | 555 hops bitwise at 96 and 192 k (93.2 % confident, 48 events) |
| HR2 | PASS | no fcHz / ringHz above 0.45 fs at any rate; 44.1 k fc max 19 845.0 (binds); fb LP effective fc = 19 845 / 20 000 (dev. 7) |
| HR3 | PASS | 25 315 320 / 27 553 920 / 55 105 920 / 110 209 920 bytes; prepare 0.2–0.9 ms |

**C7 under `--long`:** the `--long` run's C7 read **38.0 %** (30.2–52.0): every block 2.3× slower (p50 1035 µs vs 455 µs), whole run 59 min at 63 % CPU — Dorico's VSTAudioEngine and other apps were active at the time. The duty witness (thread CPU / wall ≥ 0.9) cannot see a slowed or shared core, so it passed 6 of 7 runs. Re-run alone: 16.37 % PASS. **Finding for verify:** the witness needs a machine-speed check (a fixed-work calibration loop timed before and after the run) to turn this case into `[NOVERDICT]` instead of FAIL.

### T11 static checks — PASS
`ring.push` ×2, both in `CaptureWriter.h` on `xl` / `xr`; `reverb.process` ×1, preceded by the wet guard; `kAnalysisTapMax` = definition + tap + ring guard; `getTailLengthSeconds` returns `numeric_limits<double>::infinity()`, `<limits>` included; `ScopedNoDenormals` is `processBlock`'s first statement; heap / thread tokens in `Source/dsp`: no real increase (the one new hit is the word "new" in a comment); APVTS writes in `Source/dsp`: 0; one playhead site; `setLatencySamples (0)` untouched; new hook / Diag fields and accessors in `#if OGRAN_TEST_HOOKS` (the policy / geometry flags sit beside their pre-existing siblings `noResidual`, `noMinSpray`); modules / param-dump / ParamIds / golden: no diff vs `454c8a31`.

### T1 / T12 validation (installed binaries)

| Check | T1 (∞ tail tracer) | T12 (final) |
|---|---|---|
| `build-and-install.sh` | ✓ (dual-variant sweep, caches cleared) | ✓ (0 warnings in the install log) |
| `auval -v aufx OuGn OuDv` | AU VALIDATION SUCCEEDED; `VERIFYING PROPERTY: Tail Time` → PASS | AU VALIDATION SUCCEEDED; Tail Time → PASS |
| pluginval 10 VST3 (in-process) | SUCCESS, `Reported taillength: inf` | SUCCESS, `Reported taillength: inf` |
| pluginval 10 AU (in-process) | SUCCESS, `Reported taillength: inf` | SUCCESS, `Reported taillength: inf` |
| HA (pedalboard, installed VST3) | HA1 79/79, HA2 142 untouched, HA3 79/79 | HA1 79/79, HA2 142 untouched, HA3 79/79 |

auval prints (T1 and T12) 4 pre-existing `WARNING: … Parameter did not retain default value` lines (LFO 1–4 Rate, skewed range, default 0.5 Hz → 0.328712 normalised; each `-parameter PASS`) — parameter-level, not new in 2.6 (no parameter changed). **R80 fallback (b) not needed: ∞ kept.**

## Execution deviations (for review at verify)

1. **D25 defect: `grain_limit` lowered live never took effect** (found by the M7 event script; every cap row started at its cap). `Spawner::acquire` holds `alive` constant once alive ≥ cap (R3: kill one + add one; full pool: native steal one-for-one); under sustained over-demand, steal-oldest replaces grains before any expires, so a 128 → 32 drop kept 128 grains (and the CPU) indefinitely. Fix: while alive > cap, each acquire also gives one more over-cap victim (same priority order) the R3 kill fade → the pool reaches the cap at the spawn rate (≈ 0.1 s under C7). Never active when alive ≤ cap (every start-at-cap render, every cap-128 render). Gates GL3 / GL3n. **Carried F7** (asserts acquire == the 2.1 reference on random over-cap pools) now runs with `AcquirePolicy::noConverge` — **signed off by Taylor at execute**. A‴ = A″ with the M1–M4 / N8n detail counts above.
2. **D25 defect: deferred-freeze floor derived for r ≥ 1 only** (found by Z7f fuzz, states 13 / 22 / 60 at every rate). Inside the 9-cell deferral, a forward grain's gap to the moving head is `D − headLead + a(1 − r)`; for r < 1 the minimum is at age 0, so the R24 floor `margin + r·(headLead + 1) + 1` let short sub-unity-rate grains start ahead of the moving head (reading unwritten ring: zeros at session start, 41 s-old content mid-run). Fix: `max (r, 1)`; identical for r ≥ 1 (no carried row moved). Gates Z7m9 / Z7m9n (minimal repro: freeze on + one stream at 5–11 ms with pitch ≠ 0).
3. **D25 diagnostic fix: `ActiveSet::wasTruncated()`** missed a cap landing exactly on a period boundary (512 = 8 × 64 degrees: the outer loops stopped on `count < kMaxActive` before the inner check flagged it). Loops now run on `! truncated` (the AbsSet form) — same members, flag correct. Found by Z7m8's liveness.
4. **Q set re-derived twice (P86)** — pre-authorised NC-failure re-derivation. (a) C7's position 100 % + spray 100 % over a 30 s buffer reads 15–30 s back, so every Q-set render was silent (κ 0/0): position 0, buffer 2 s. (b) Under that set every κ NC was vacuous (K3bn == K3b 0.947×, FZ9an 1.13×, EN1n 1.1×): window 1 + skew +100, ring mod, ±24 st pitch + rand, res 100 % at random cutoffs swamp a click → Hann, ring off, pitch 0, res / filter_rand 0; every other C7 element (4 streams, spray / jitter / size_rand / reverse / spread / gain_rand, drive, LP filter, triggers + follow, 16 routes at 2 Hz ±30 %, feedback 100 % + diffusion + saturation) stays.
5. **FZ9** additionally uses FZ2's own grain geometry (engage spray 0 / position 0 — the +24 st fallback was taken: 0 st NC read 0.997; release spray 50 %; forward, 20 g/s, 200 ms, jitter 20 %), route 2 (LFO2 → s2 pitch, ±14 st latched per grain) retargeted to s2 spray (bisected: it alone held the engage NC at 1.99×), and 3 s engage renders (the never-frozen control's fb-100 % loop builds to wet 0.90 after the window).
6. **MF8 measure**: base mix 50 % (at 100 % a Mix add clamps to a no-op: MF8n read 1.00×); references held at the pre- **and** post-edit state (add / remove: 100 % and no route; step: +60 and −60) and the ratio is max curvature vs the larger reference — per-render κ = maxCurv / RMS is biased by a deliberate level change inside its own window (a 34 dB input-gain drop, MF8c plain κ 2.18× / 2.13×). Plain κ stays printed as INFO. Threshold unchanged (2.0×).
7. **HR2**: the fb LP check recovers the coefficient's effective cutoff (`atan (a2 / a1)·fs / π`) within 1e-4 relative — the chain's cutoff comes through `exp2 (log2 ramp)` in float and lands a few ulps off 20 000 Hz, so the planned bitwise `tptCoeffs` compare was the wrong form.
8. **MX3**: one composite input drives every source (233 → 311 Hz steps with +12 dB swells and 2 ms bursts); the edit cell per source is the largest |src| in 0.4–0.75 s whose ±3-cell neighbourhood is smooth (from a route-free probe) — at a source's own attack / step the bound is wide and an NC would be vacuous. Bound ramp terms use |src| ≤ 1 (conservative); 1e-6 absolute float slack.
9. **Z7m4** split into three renders (spawn-bound 5 ms; demand 400 at cap 32 → kills; demand 400 at cap 128 → steals): at cap 32 the over-cap path is the R3 kill, so steals can't occur there. **N10T** adds the R82 "MAX Tempo 1/64T" sync (N9's 1/16 grid never over-demands: steals 0 → vacuous). **M7**'s cap drop moved to 6.25 s (inside the scripted 7–8 s Tempo stop nothing spawns).
10. Iteration aids not in the plan: `--no-c7`, `--sect=a,b` (acceptance runs never pass them). Harness-only tasks T4–T10 were accepted on the consolidated final runs (the 2.6 sections run after every carried section, so carried rows can't see them); the Source tasks T1–T3 had their own full runs.
11. **Observation (carried row, not edited):** Z6a's freeze toggles never fire — `eg22b::renderPool` → `eg22::runLoop` ignores `Job::events` (the `jassert (events.empty())` compiles out in Release). The 2.6 rows use an event-aware pool. Flag for verify (fix in a later harness pass or leave).
12. Counts: 75 new default rows (plan ≈ 74: + GL3, GL3n, Z7m9, Z7m9n; N10c / N10n share rows per form) → 463 default / 465 `--long`.

## Errata (ARCHITECTURE / ROADMAP / CONTEXT not edited — checksums)

1. **D21 / R79 / R80**: ∞ tail on the installed AU and VST3 (auval Tail Time PASS, pluginval `taillength: inf` ×2); fallback (b) not taken.
2. **R83** (signed off): C7 = D22 with crush 1 %; literal all-max (crush 100 %) is INFO (17.36 %).
3. **R84** (signed off): no `input_gain` route in C7 (route 13 → s1_level); the R84 stimulus.
4. **R86**: C3w / C3t are worst / typical configs, not clocks.
5. **R88**: max reverb = reverb_mix / size / damp (shimmer / mod fixed internally).
6. **R93** (signed off): "no subnormal" = FTZ-path census 0 + the FTZ-off control (Z7dn: subnormals from 11 s).
7. **R96** (signed off): ring-write + wet guards; QUAL-02 extended to host input; P81's contraction-safe form.
8. **P87**: MX3 has 7 edit kinds. **P89**: QB3 kill / steal arms + `killFadeMs`; FZ8 → FZ9. **P90**: `edo 64 1.001`. **P92**: N10 keeps crush. **P83**: census includes `preDelayR`.
9. **R24 / PLAN-2.4 P38** deferred-stop floor: `max (r, 1)·(headLead + 1)`, not `r·(headLead + 1)` (deviation 2).
10. **RESEARCH-2.1 R3** cap semantics: an over-cap pool (cap lowered live) converges by one extra R3 kill per spawn (deviation 1).

## Carried

- **Stage 3 DAW pass:** DAW-2.5a items 1–8 (watch 6) + DAW-2.5b items 1–5 + **DAW-2.6 items 1–3** (∞ tail in Logic / Live / Reaper; drums over a sustained pad — R85; 192 k CPU).
- **Decisions at verify:** (a) the Env 1e30 pin (≈ 11 s at default release; same predicate at the env input, bitwise ≤ +80 dBFS, or leave); (b) A‴ (deviation 1's detail moves); (c) the C7 witness machine-speed check; (d) Z6a's dropped events.
- **3.x UI copy:** "at 192 kHz the worst case uses about ⅔ of a core" (C7 192 k 63.4 %); the 2.5b items.
- **After the Stage 3 DAW pass:** R60 re-arm under sustained tones (R85).
- W2o: widen seeds, never the threshold.
