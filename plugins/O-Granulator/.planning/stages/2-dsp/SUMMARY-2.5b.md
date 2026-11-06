# Stage 2 — DSP · Phase 2.5b (Onset detector + burst, MPM pitch analysis, f0 history ring, absolute pitch follow) — Summary

**Plugin:** O-Granulator · **GSD phase:** execute · **Date:** 2026-11-05 · **Branch:** `main`
**Inputs:** PLAN-2.5b.md (T1–T13, P60–P78) · RESEARCH-2.5b.md (R59–R78) · CONTEXT-2.5b.md (D17–D20) · sign-offs R60 / R66 / R64 / R71 (Taylor, at plan)

**Gate:** intra-stage (2.5a → 2.5b); 2.5a verified at `2cf30120`. Entry check: the six contract checksums matched the STATUS frontmatter (brief `f9c748a8…`, requirements `f0b15388…`, param-spec draft `d0b1457e…`, param-spec `f5ce3595…`, architecture `f0cf753b…`, roadmap `1060dae0…`); `git diff 2cf30120 HEAD` empty for `Source/`, `tests/`, `CMakeLists.txt`; `modules` + `scripts/param-dump` clean. No module changed (`git diff --stat 2cf30120 -- modules scripts/param-dump` empty at the end).

**Commits:**
- `5faf025b` T1 — the N3 null (`tests/n3/`), baseline A 299/299 (two runs identical). No `Source/` change.
- `c2546e22` T2 — plumbing: constants, 10 pointers, `Analysis.h`, `AbsSet`, onset-list capacity, `Spawner::plan` follow parameter, Diag / TestHooks / records, harness scaffolding + FR0.
- `30158b99` T3–T4 — detector live, Free re-phase, Tempo extra grain, Onset source; the MF6 rewrite (baseline A′); N3 stimulus scaled.
- `1129c542` T5–T6 — MPM committed at the tick (R59), D20 guard, Pitch source + telemetry; FR1–FR4 rows.
- `4b39278e` T7–T8 — absolute follow (two-pass plan, content-midpoint lookup), `FollowPitch`, follow records + hooks.
- `7e5ac1f4` T9–T10 — MX / QB / N9 / M6 / Z6 / C6 rows.
- this commit — T11–T13: `tests/hostcache/run.py` (+10 params), DAW-2.5b.md, this summary, STATUS.

## What was built

| File | Role |
|---|---|
| `Source/dsp/Analysis.h` **(new)** | `AnalysisDims` (D, Hs, N, cell-log length, τ range, hold, R69 lags); `OnsetDetector` (TPT HP 150 Hz, fast / slow envelopes, R60 edge + 3 dB re-arm, linear compare R75, per-sample refractory); `Decimator` (64-tap Blackman sinc, Σh = 1, polyphase at abs % D == 0); `Mpm` (FFT 2048 r(τ), m(τ) to τ_max + 1, NSDF, key maxima, parabolic, clarity / −50 dBFS gate; the only `juce::dsp::FFT`); `OctaveGuard` (R66: median-of-3, asymmetric harmonic-class lookahead N = 3, acc expiry); `PitchTracker` (live note, 250 ms hold, 20 ms slew from 0); `CellLog`; `F0Ring` (tagged entries, R62 newest rule); `Analysis` (tap + sanitiser, per-piece event, pending hop → commit at the next tick, Onset / Pitch sources, hop / event logs in test builds) |
| `Source/dsp/ScaleQuantiser.h` | `AbsSet` (P71: members `c_d + k·P` over [−2400 − P, 2400 + 2P] in double, key without the span, uniform pick / nearest ties-up, EmptySet / NoMember) |
| `Source/dsp/Spawner.h` | `FollowPitch`; `plan (…, const FollowPitch* follow = nullptr)` (P72) — nullptr is the 2.5a code verbatim |
| `Source/dsp/StreamScheduler.h` | `StreamOnset::burst`, `kOnsetCap = kCell + 1`, `mergeOnsets` extent, `rephase()` |
| `Source/dsp/Engine.h/.cpp` | analysis per piece after the env scrub; analysis tick after the freeze edges (cell log, thrLin, scrubs, R59 commit); `scheduleStreams (len, eventOffset)`: Free split + re-phase, Tempo insertion `2^63 \| n`; follow sets in `updateScaleState`; lookup + pass 2 in `spawn`; Pitch / Onset in `tickSources`; `pitchNote` / `pitchConfident`; Diag / TestHooks / SpawnRecord fields |
| `Source/dsp/ControlGrid.h`, `EngineConstants.h`, `ModSources.h` | P73 fields; 2.5b constants (banner 2.1–2.5b); stale comments |
| `Source/PluginProcessor.h/.cpp` | 10 pointers; `getPitchNote()`, `getPitchConfident()` |
| `tests/n3/` **(new)** | N3 null vs `2cf30120` + N3c, N3p-THR / N3p-Z, N3n-TRG / ONS / PIT / FOL, analysis liveness via C++20 `requires` probes |
| `tests/render-harness/engine_gates_25b.h`, `engine_gates_25bb.h` **(new)**, `main.cpp`, `engine_gates_25ab.h` (MF6 block only) | `--only25b`; 89 new rows (+ 11 INFO lines); the MF6 rewrite |
| `tests/hostcache/run.py` | + scale_root, onset_threshold, sN_onset_trigger ×4, pitch_follow_sN ×4 |

## Gate results

**Harness: 388 PASS, 0 FAIL** (299 carried + 89 new). Carried-row diff: vs baseline A **empty through T3**; vs A′ (A with the one MF6 line, P74) **empty at T4, T6, T8 and T12**. A′ was captured twice at T4 (identical). W2o unchanged (+0.483 dB). 0 warnings (harness, plugin, install log).

### Out of tree

| Row | Result | Measured |
|---|---|---|
| N3 free / capped | PASS | max\|Δ\| 0 both; landed / raw checksums equal; below knee; peak in 0.44 ≤ 0.5 |
| N3c seed + 1 | PASS (differs) | 568,972 diffs |
| N3p-THR / N3p-Z | **IDENTICAL** at every task | threshold swept −60 → 0 per block; Pitch / Onset depth-0 routes (R46) |
| N3n-TRG / ONS / PIT / FOL | each flipped at its task (T3 / T4 / T6 / T8); `--strict` PASS from T8 | 527,901 / 527,870 / 564,733 / 563,910 diffs |
| N3 liveness (strict) | PASS | onsets 22, hops 555, confident 372, f0writes 552 |
| N2 / N1 / N0 `--strict`, R12 | PASS | at T1, T4, T8 and T12 |

### New rows (all PASS)

| Row | Measured |
|---|---|
| FR0 | caps at 15 rates 8–768 k (max N 5127 ≤ 8192, log 1795 ≤ 2048, τ_max 401); R69 lags == RESEARCH §3 (48 k: N 3846, hold 24) |
| OD0 / OD0n | analysisOff == on bitwise with 38 events firing; trigger on 911,920 diffs |
| OD1 / OD1n | exactly 1 event per sustained / gated tone, 4 rates (lag 1.81 / 0.39 ms); levelTrigger fires at 1.8 / 51.8 / 101.8 ms |
| OD2 (+OD2i) | 1 ms 2 kHz −6 dBFS detected at all 4 rates; impulse 1 / 1 / 0 / 0 (R61) |
| OD3 | min gap 2426 samples ≥ round (0.05 fs); 55 ms pair → 2; 30 / 45 ms pairs → 1; refractory-edge fires observed: 0 |
| OD4 / OD4b | −35 dBFS 0 events, −25 dBFS 20/20; threshold latch at the next tick (48050 → 48064); drum loop 78 events == the dB reference, 1 sample within 1e-6 dB |
| OD5 | feedback 100 % after the input stops: 0 events (loop at −51.8 dBFS); input_gain −24 dB removes all 20; freeze on: 20/20 |
| OD6 | NaN at the tap: 1 scrub, events unchanged, audio bitwise |
| OD7 | click trains IOI 60 / 125 / 250 ms × 4 rates: 3400/3400 bursts, 0 doubles; −36 dBFS: 0 |
| OB1 | Free: 216/216 re-phases at the event (Δ 0, frac 0, index prev + 1); next grid at event + 6000 exactly; s3 grid intact, s4 draws nothing |
| OB2 / OB2b | Tempo grid records bitwise == trigger off (482); bursts at `2^63 \| n`; slot / epoch traces identical; item 6 watch: TS1 × 4 divisions × 8 transports worst \|Δ\| **0** with 1408 bursts |
| OB3–OB5 | coincidence: Tempo grid + burst, Free 1 grain; frozen burst on W0 / heldSpan; muted fires; edge + event 2 grains; event at i = 0 after an edge: 1 grain, epoch unchanged; maxEventsPerPiece 1 |
| PS1 | Onset source bitwise exp law over 105,000 ticks; exactly 0 beyond 16 τ, 0 subnormal over 67 s |
| MP0 | 100 windows: max\|Δr\|/r(0) 2.6e-7, τ 2.0e-4, clarity 2.5e-7, decisions identical |
| MP1 (+MP1i, MP1h) | worst **0.36 ¢** (55–1000 Hz × 44.1 / 48 / 96 k, all hops confident); 1500 Hz @ 44.1 k 2.78 ¢ (= the R68 model); **192 k / 176.4 k: all hops confident, worst 0.76 ¢ (saw 220 Hz 0.62 ¢) — no wrong pitch, watch item clear** |
| MP2 | silence / white −20 / −60 / brown −20 / sine −55: 0 confident (max clarity 0.37); sine −45: 85/85 |
| MP3 | live confidence true for exactly 24 decided hops, false at +25; source holds 0.39311 (target 0.39311); silence ring entries NaN 264/264 |
| MP4 | 274 hops at m·D, committed at the next tick, centres exact; hopLog identical at 32 / 441 / 4096 |
| OG1 | L ≤ 2 flips → 0 wrong (40/40); leaps exact; returns / non-harmonic immediate; 2-of-3 / 1-of-3; expiry; index = raw − 3 |
| OG2 (+OG3) | 0 short runs (guardOff 34); OG3: H2/H1 8 → +12 on 271/271, H3/H1 4 → +19.02 on 271/271, breathy 0 wrong |
| PS2 / PS3 (+PS4) | Pitch source bitwise over 6000 ticks; telemetry == engine; PS4 50 % of a 5-st step at **145 ms** (deviation 14) |
| FR1–FR4 | tags, newest rule, freeze edge; B-region centre error +0.31 Hs (FR4n +3.31 Hs) |
| FR5–FR9 (+n) | content f0 12/12 A at 1 s behind (liveF0Lookup → B); frozen 3/3 positions read A; gap → miss 1 (16/16; ringHold → A); position 0 56/56 (strictNewest 0); midpoint B / start A |
| FR10 | 360 grains, 254 hits: non-pitch fields bitwise |
| AQ1–AQ7 (+AQ8) | D19 18/18 cases worst 0 ¢; masked Major / 19-EDO / BP / Custom: members 1e-6 ¢, nearest 4/4, χ² 4/4; ±24 480/480 + engine min −23.999 st; nearest ties up; fallbacks 3 / 4 / 1; BP register +500 ¢; rebuilds 4 == key changes; AQ8 isolated grains 0.06 ¢ |
| MX1 / MX2 / MX2n / MX2a | 20/20 class × sign; removal 8/8 within the P56 bound; MX2n 0.0087 vs 0.00095; removal κ 0.75× |
| QB1 / QB1n (+QB2) | Free 1.05×, Tempo 1.20× the same-grain-count reference; window 1: 2.68×; QB2 +4.52 dB (R78 ≤ +5.4) |
| N9-F / N9-T / N9c / N9n | block null 441 / 512 / 4096 / var vs 32: max\|Δ\| 0, events / hops / f0 ring bitwise (64 events, 256 bursts, 1066 confident, 1248 / 454 hits, 291 / 90 misses, 124 skips, 6 routed, T: 3 re-anchors / 2 re-locks); seed 573,800 / 567,819 diffs; commitHopImmediately 6902 diffs (0 without) |
| M6 | 0 allocations (Free + Tempo; 743 hops, 942 / 598 lookups, 7 rebuilds, 164 bursts); liveness 1 |
| Z6a / Z6b | 60 s × 48 / 44.1 / 192 k: finite, 0 subnormal (analysis state incl. FIR history, window, NSDF, guard, slew), wet ≤ A (max 1.60), out ≤ 8.885 (max 8.31), 0 scrubs / kills, 438 events, ≥ 15k hits; NaN / +Inf / 1e30: 1 scrub each, events + audio bitwise |
| C6 / C6s (INFO) | **C6 15.77 %** of one core (+1.06 vs a C5 re-run on the same tonal + burst input, 14.72 %; carried C5 16.73 %) — under the 25 % ceiling and the 20 % flag. C6s: hop blocks median 116.0 µs vs 108.3 µs (excess 7.6 µs = 1.1 % of the 666.7 µs budget) |

### Validation (T11 / T12)
- **Static (T11):** no `std::vector|new|malloc|make_unique|std::function|std::thread|std::async` in `Analysis.h` (0); no increase in `ScaleQuantiser.h` / `StreamScheduler.h` / `Spawner.h` (code lines); no `fmod` in `Analysis.h` / `ScaleQuantiser.h`; no `log10` / `pow` in `OnsetDetector::step`; one `juce::dsp::FFT` (the `Mpm` member of the engine's `Analysis`), `perform*Transform` only in `Mpm::analyse`; no APVTS write in `Source/dsp`; one playhead call site (`processBlock`); `setLatencySamples (0)` untouched; modules unchanged.
- `build-and-install.sh O-Granulator`: 0 warnings, dual-variant sweep, caches cleared. `auval -v aufx OuGn OuDv` PASS. pluginval strictness 10 VST3 + AU PASS.
- **HA** (installed VST3, pedalboard via `uv`): HA1 79/79, HA2 142 untouched unchanged, HA3 79/79 (220 PARAM) — PASS.
- `DAW-2.5b.md` written (items 1–5); joins DAW-2.5a.md in the deferred Stage 3 DAW pass.

## Errata (ARCHITECTURE / ROADMAP / CONTEXT not edited — checksums)
1. **R59** hop commit at the next gridTick (ARCH §15 "runs at hop boundaries": computed there, committed at the tick). N9n proves the class (6902 diffs).
2. **R60** (signed off) edge-triggered detector, +3 dB re-arm (ARCH §8 / §15 level trigger re-fires at 0 / 50 / 100 ms — OD1n).
3. **R61** detection stimuli ≥ 1 ms bursts; impulses rate dependent (OD2i 1 / 1 / 0 / 0); offsets measured against the reported event.
4. **R62** a read point newer than the newest entry uses the newest entry (FR8 / FR8n).
5. **R63** ring-domain f0 ring via the per-cell log, tagged entries; N = 3846 entries at 48 k (41 s ring), not CONTEXT's ≈ 2.8 k.
6. **R64** (signed off) content-midpoint lookup through a two-pass plan (FR9 / FR9n).
7. **R65** no hold in the ring (MP3, FR7 / FR7n).
8. **R66** (signed off) asymmetric harmonic-class lookahead N = 3; uniform 3-hop decision lag; P67's acc expiry after the hold is a plan refinement (OG1).
9. **R67** D20 rows = injected sequences + near-boundary audio with a guard-off control (OG2n 34 short runs); breathy / persistent H2 INFO (OG3).
10. **R68** m(τ) to τ_max only (+ the local-maximum rule, deviation 3). **R69** lag constants (FR0, FR4).
11. **R70** absolute set 1/1 at C4 + root; built over the maximum span (key without the span; AQ7).
12. **R71** (signed off) nearest member at pitch_rand 0, ties up (AQ2 / AQ4); empty set / no member → interval mode (AQ5).
13. **R72** D19 and the quantisation rows gate the recorded target; sounding pitch INFO (AQ8); the MIDI 100 clamp case is a unit row (AQ3).
14. **R73** Free re-phase via `restart()`; Tempo bursts on `2^63 | n` (OB1, OB2).
15. **R74** conditioned input kept (OD5).
16. **R75** linear compare (OD4b); analytic Onset source, 0 after 16 τ (PS1).
17. **R76** no-heap FFT (M6 on macOS); f0 not bitwise across platforms.
18. **R77** hop not spread (C6s excess 1.1 % of a 32-sample block).
19. **R78** Tempo bursts outside the R5 law (QB2 +4.52 dB); follow does not feed the law (P72).
20. **P65** 64-tap decimator kept at every rate; MP1h 176.4 / 192 k all confident, worst 0.76 ¢ — no 16·D fallback needed.
21. **P68** Pitch source 0 until the first confident hop, slews from 0.
22. **P74** the MF6 inert carried row rewritten (A′).
23. Final harness count: **388** (plan ≈ 370).

## Execution deviations (for review at verify)
1. **N3 stimulus scaled (T3)**, both sides: noise × 0.05, sine 0.05, 5 ms hard-gated 1 kHz bursts at 0.4 (plan: × 0.1, 0.1, 2 ms Hann at 0.35). With input_gain −12 dB the plan's bursts never crossed the −24 dB threshold (0 events: the liveness precondition failed). The check is unchanged; the harness `compositeStim` matches.
2. **Tap sanitiser bound**: `! (|a| <= 1e4)` → 0 + scrub (non-finite AND beyond +80 dBFS). P61 names non-finite only; with that, Z6b's 1e30 sample fires an event and pins the slow envelope for ≈ 6 s. Z6b: 1 scrub each for NaN / +Inf / 1e30.
3. **MPM key maxima must be local maxima** (`n(p + 1) <= n(p)`): with m(τ) only to τ_max + 1 (R68), a positive region unterminated at the horizon offered its rising edge, refined on a slope (MP0 caught τ 744 > τ_max, clarity 1.49, on a 42 Hz two-tone). The model scans to W − 1. Same rule in the MP0 reference; affects only content below ≈ 40 Hz.
4. **AbsSet key**: the custom version keys the Custom choice only, so a custom apply never rebuilds a built-in follow set (ActiveSet keeps its 2.2 key). AQ7: 4 rebuilds = 2 initial + 1 mask + 1 custom apply.
5. **OB1 density 8 g/s** (plan 4): at 4 g/s the 250 ms interval exceeds the 180 ms IOI, so no grid onset ever falls between two events and "next grid onset at event + fs/density" is unobservable.
6. **OD4 bursts 5 ms hard-gated** (the first try, 2 ms Hann at −25 dBFS, peaks the fast envelope at ≈ −30 dB: on the threshold, not above it).
7. **Z6a stimulus re-derived**: continuous 0 dBFS noise masks every burst (1 event in 60 s) and gives the tracker nothing confident. Per 250 ms: 60 ms 0 dBFS noise, 150 ms sine (110 / 220 / 440 / 880 Hz), 40 ms silence, + 2 ms 0 dBFS bursts every 60 ms → 438 events, ≥ 15k follow hits per rate.
8. **MF6 rewritten detail is value-free** ("differs, routedDests >= 1: yes") so the A′ line does not move when the Pitch source goes live at T6 (Pitch → mix at base mix 100 % clamps; Onset → s1_pitch carries the difference). A′ = A with that line replaced, confirmed by the T6 / T8 / T12 runs.
9. **Analysis.h and AbsSet landed whole at T2** (prepare + per-sample code), wired per task. The N3 liveness counters (hops / confident / f0writes) were therefore live from T3; each N3n still flipped at its own task.
10. **Commit grouping**: T7 needed no `Source/` change (the cell log and the ring write are part of the analysis tick / commit wired at T3 / T5); FR1–FR4 landed in the T5–T6 commit, the MP rows in T3–T4, the Mpm local-maximum fix in T3–T4. The test accessors (`getMpm`, `getGuard`, `medians`) landed in the T7–T8 commit (the T9–T10 message says otherwise).
11. **AQ2 Custom mask {0, 2, 4, 5, 7}** of `edo 9 1800`: with {0, 2, 5} the ±5 st window holds one member (χ² dof 0, vacuous).
12. **AQ8 on isolated grains** (2 g/s, 400 ms): windows across grain crossfades read 57.6–57.7 for a 58.00 target (grain-to-grain phase resets pull a period estimate toward the input); isolated grains 0.06 ¢.
13. **FR7 density 100 g/s**, FR10 6 s, AQ2 pitch_rand 5 at 200 g/s × 12 s (grain counts for the gates).
14. **PS4 145 ms** to 50 % of a 5-st step (R69 projected ≈ 95 ms): MPM windows straddling a note change are unconfident, so the first new decision waits for a full new window, the 2-of-3 median and the 3-hop lag. INFO; the Stage 3 UI copy should say ≈ 150 ms.
15. Units were written against the implementation (as 2.5a deviation 12); non-vacuity from the negative controls OD0n, OD1n, OG2n, FR4n–FR9n, MX2n, QB1n, N9n, N3n (each fails its gate).

## Carried
- **Verify:** independent re-run; review deviations 1–15.
- **2.6:** full PERF-02 budget gate; `getTailLengthSeconds()`; full QUAL-01 click sweep incl. route edits under all sources and the burst-steal click in a capped pool; full-feature PERF-03 null incl. the oversized host block; 96 / 192 k smoke renders. Decimator taps above 96 k: not needed (MP1h).
- **Stage 3 DAW pass:** DAW-2.5a.md items 1–8 (watch item 6) + DAW-2.5b.md items 1–5. A 2.5b FAIL reopens FUNC-08 / DSP-08.
- **3.x UI copy:** Pitch source latency ≈ 75 ms decided-entry lag, ≈ 145 ms to swing halfway on a note change (PS4); 40–1500 Hz covers 0.04–0.79 of the Pitch source; follow "harmonises against the source note" (R74); the R45 size-sync tooltip. `getPitchNote` / `getPitchConfident` and the telemetry accessors feed the UI.
- **Out of scope:** O-GrainScatter stopped-transport re-fire (R43).
- W2o rule unchanged: widen seeds, never the threshold.
