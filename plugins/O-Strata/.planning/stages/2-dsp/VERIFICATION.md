# Stage 2: DSP (live wave-terrain oscillator), Round B — Verification (stage verdict)

## Verification Date

2026-09-12

**Plugin:** O-Strata · **Stage:** 2 of 4 (DSP) · **Round:** B = Phases 2.4 + 2.5 (CONTEXT D1) — this file carries the **stage-level verdict**; the Round A gates are cited from `round-a/VERIFICATION.md` (2026-09-11, ✅ VERIFIED) and were re-run here as part of `--gate all`. · **Mode:** manual
**Inputs:** `CONTEXT.md` (D1–D4, "Round B exit"), `RESEARCH.md`, `PLAN.md` (Round B — 16 tasks, Decisions 25–46, success criteria), `SUMMARY.md` (Round B execute report, commits `6428829d` Phase 2.4 / `6f30137b` Phase 2.5), `ROADMAP.md` Phases 2.4–2.5, `REQUIREMENTS.md` v2.0.0 (DSP-01 as amended 2026-09-11)
**Method:** every SUMMARY claim was re-measured in this session from the committed tree at `6f30137b` (`git diff HEAD -- plugins/O-Strata` empty): `ninja` of the plugin, param-dump and harness targets (no work to do — the tree was already built), the built VST3 / AU binaries sha256-identical to the installed `O-Strata-dev` bundles (`9a205295…` / `437b69bd…`), `O-Strata-render-test --gate all` run twice (from `/` and from the repo root), param-dump diff, pluginval ×2, auval, the grep gates, commit scopes, contract checksums, a warning census of the Round B translation units. Nothing below is copied from SUMMARY without a fresh run. No plugin source or harness code changed in this phase; the only edits are `REQUIREMENTS.md` (statuses + one acceptance re-wording, see Issues), `STATUS.md`, `PLUGINS.md` and this file.

## Goal-Backward Analysis

### Original Goals (from CONTEXT.md "Goal" + "Round B exit", ROADMAP Phases 2.4–2.5, PLAN success criteria)

1. **Bandlimited mode** — degree-16 Chebyshev triangle evaluated at 1× by 2-D Clenshaw, truncated per voice so the highest harmonic (n + m)·K stays below Nyquist; ≤ −90 dB non-harmonic on every exact-orbit × terrain row; the 2× gate still 198 / 198 (DSP-02, FUNC-06 Bandlimited half, QUAL-01 Bandlimited half).
2. **Off-thread coefficient scheduler** — sets projected off the audio thread, swapped atomically on the message thread, retired through the reaper; fit readout; ModWheel → Terrain Freq re-publishes within 100 ms; LFO → Terrain Mod X inert in Bandlimited, live in 2×; `prepareToPlay` / `setStateInformation` publish nothing; sync ≡ async sets (DSP-02, PERF-01).
3. **Swap storm** — zero audio-thread allocations, no NaN, click-free outside the 64-sample windows, ≤ 1 job in flight per oscillator, superseded work cancelled, old sets freed ≥ 2 generations later, no leak (PERF-01, DSP-05).
4. **PNG terrain path (DSP side)** — decode ≤ 1024², pre-blur, Mirror / Window edge read, embedded Chebyshev set, import API that never touches a parameter; Blur 0 → 1 lowers the centroid monotonically; Mirror vs Window differ at r = 1; import job ≤ 100 ms at 1024², never publishing inside a `processBlock`; missing file → false (FUNC-07 DSP half, DSP-04 pre-blur / edge half).
5. **Edge gate H11** — hard-edged PNG, orbit crossing the border, no edge partial h ≥ 32 above −40 dB re h1 under Mirror and Window at Blur 0.2 and Blur 0; the periodic-tiling control fails; Bandlimited + image ≤ −90 dB with Ellipse (DSP-04, QUAL-01).
6. **Bytes determinism H10** — identical bytes ⇒ identical 1 s render SHA-256 across two instances, differing from the library fallback (FUNC-08 bytes half).
7. **Round A rows re-verified** — H1 unchanged, H2 Bandlimited grid 66 / 66, H9 Bandlimited row block-size invariant, H8 storm + image rows 0, H7 Bandlimited delta reported, `crossfade` / `latency` unchanged; `--gate all` H1–H11 green in one run ≤ 3 min from `/` and the repo root; `pump` the only message-loop call site; ASan configuration; DSP-05 grep clean; COMPAT-01 regression; docs (CHANGELOG "Known limits — Bandlimited mode", README H1–H11, SUMMARY with the full H6 table); two path-scoped commits, no tag, contracts and O-Prism untouched, params.tsv diff empty.

### Deliverables (from SUMMARY.md, confirmed on disk)

1. `Source/dsp/ChebyshevSet.h` (triangle, `ChebKey`, `clenshaw2D`, shared `chebDiagonalCutoff` / `chebDMax` / `chebTaperWeight`), `ChebyshevProjector.h/.cpp`; `TerrainOscillator` Chebyshev path with the per-oscillator tapered copy at block start, continuous raised-cosine taper and 64-sample set crossfade; `StrataVoice::setPublished` / `updateBlockRate (target, max (target, current))`.
2. `Source/dsp/TerrainScheduler.h/.cpp` (50 ms timer, 2-thread `ThreadPool`, scheduler-owned `ModulationMatrix` fold, 1/1024 key quantisation, Terrain Freq clamp ≤ 2, throttle + trailing debounce, cancellation, `callAsync` + weak token, `runOnceSynchronously`, `publishForHarness`, counters); `dsp/Retirable.h` + live-instance counters; processor pointers, readout atomics, `isInsideProcessBlockOnThisThread`, `getBlockGeneration` / `getRetiredCount`, scheduler `shutdown()` first in the destructor.
3. Harness `storm` gate (16-note and single-note storms, allocation counter armed around every block, free-gap FIFO check, counters).
4. `Source/dsp/TerrainImage.h/.cpp` (decode ≤ 1024², 3-pass mirror-padded box blur, luminance → [−1, 1], embedded set at F = 1 with M = 64 nodes, 64² view, bilinear `sample` with Mirror / Window / HarnessWrap); processor `importTerrainImage` / `importTerrainFile` / `importSlot` / `importRevision`; scheduler image side at a 150 ms cadence with `publishImage`; oscillator / voice image path.
5. Harness H11 (+ `harnessEdgeOverride` control) and the Bandlimited + image row.
6. Harness H10.
7. Harness `pump` / `syncScheduler`, H1 Chebyshev identity row, H2 / H9 / H7 / H8 Bandlimited rows, `clenshaw`, `scheduler`, `import` gates, `--png`; CMake `STRATA_HARNESS_ASAN` option + root `.gitignore` `build-asan/`; README "Gates (Round B)" + "AddressSanitizer run"; CHANGELOG Round B entry with "Known limits — Bandlimited mode"; `JUCE_MODAL_LOOPS_PERMITTED=1` on the harness target only.

### Goal Achievement

| Goal | Status | Evidence (re-measured this session; run 1 = from `/`, run 2 = from the repo root) |
|------|--------|----------|
| 1. Bandlimited mode | ✅ Achieved (17 rows muted by the contracted taper law — accepted as the documented limit, see Issues) | H6 Bandlimited: **127 / 127 sounding rows ≤ −90 dB, worst −98.4 dB** (Radial Rings × Ellipse A6); highest harmonic above −100 dB ≤ D_max · K on **127 / 127** (equality on 13); 8 quiet rows gated on absolute non-harmonic ≤ −100 dBFS, worst −124 dBFS (Cosine Wells × Epitrochoid 3 A6); 17 muted rows reported (all at A6: Epitrochoid 5 / 7 and Hypocycloid 7 on the five even terrains = 15, plus Radial Rings × Epitrochoid 3 / Hypocycloid 5 = 2 — exactly the set the law predicts, recomputed here: D_c = fs / (2·K·f) − 1 gives D_max = 1 for K ≥ 6 at A6 and D_max = 2 at a 0.10 taper weight for K = 4); largest tapped \|y\| 0.673; approximate orbits reported, worst −30.2 dB (Radial Rings × Squarcle A6, nominal K = 1). H1 Chebyshev identity row **−128.3 dB**. 2× gate still **198 / 198, worst −70.7 dB** (Mitsuhashi × Epitrochoid 7 A2); 44.1 k / 96 k rows within 3 dB. Both runs identical on every H6 figure |
| 2. Scheduler | ✅ Achieved | `scheduler`: fit % of every terrain at F = 1 / 2 within 0.5 % of the scratch table and ≥ ARCH − 2 % on the five measured rows (Mitsuhashi vs its re-measured 98.3 / 87.6 %); Terrain Freq 4 → key F = 2.00, coefficients byte-identical to the F = 2 set; ModWheel → OscA Terrain Freq re-publishes in **58 ms** (run 1) / **57 ms** (run 2), key F 1.000 → 2.000 (need ≤ 120 under pump); LFO1 → Terrain Mod X inert in Bandlimited (max\|Δ\| 0.000e+00) and live in 2× (6.104e−1); `chebApproximate` 0 / 1 / 1 / 1; `prepareToPlay` ×2 + `setStateInformation` under pump: `publishCount = 0`; async (63 ms) set == sync set (keys equal, memcmp-identical; counters completed 1 cancelled 0 superseded 0 dropped 0 inFlightMax 1); `chebPartialsAtC4` Ellipse 16, Epitrochoid 7 80. `clenshaw` 84.6 ns per dependency-carried evaluation (both runs). Publish / retire sites carry `jassert (! isInsideProcessBlockOnThisThread())` (`TerrainScheduler.cpp:54 / 92 / 363 / 446`); the reaper frees at `gen >= retiredAt + 2` (`PluginProcessor.cpp:1059`) |
| 3. Swap storm | ✅ Achieved | run 1: 117 swaps under 16 held notes, **0 allocations** (+30 foreign-thread, not counted), finite, click ratio **1.16** (plateau 0.3272 / outside-window 0.3789, need ≤ 1.5), inFlightMax A 1 B 0, superseded 27 + cancelled 0 (> 0), 104 frees every one ≥ 2 block generations after publish (min gap 5), after the drain `ChebyshevSet::liveCount = 2`, `retired = 0`, live jobs 0; single C4 note: 93 swaps, ratio **1.02**. run 2: 105 swaps, ratio 1.13; single note 96 swaps, ratio 1.02. `cancelled` = 0 by physics (a 0.3 ms job is never caught by a 50 ms poll — SUMMARY deviation 4; the in-flight cancellation path is exercised on the image side: `import` (g) two Blur changes → completed 2 cancelled 0 superseded 1) |
| 4. PNG path | ✅ Achieved | `import`: (a) 512² hard-edged bytes → true, Terrain = Imported renders rms 0.1015 (fit 99.7 %), max\|Δ\| vs Sine Product 0.322, slot carries name + SHA-256; (b) Blur 0 / .25 / .5 / .75 / 1 centroids 9183 / 416 / 262 / 262 / 262 Hz, one publish per value (imageGeneration 5); (c) Mirror vs Window at r = 1 centroids 279 / 498 Hz (Δ 44 %), max partial move 39.5 dB; (d) 1024² at Blur 1: decode 2.9 ms, blur + projection + view 23.3 ms, sync-path import 26.1 ms, **gate 26.3 ms ≤ 100**; (e) async import during a 1 s render: `publishCount` changed inside a `processBlock` on **0 of 100** blocks, between blocks once; (f) missing file → false, `imagePtr` null, no revision; undecodable bytes → false; (g) async import through the timer + pool published after 69 ms, imageGeneration 1. The API touches no parameter: params.tsv diff empty after the run |
| 5. H11 edge gate | ✅ Achieved | h ≥ 32 re h1: Mirror Blur 0.2 **−62.8 dB**, Window Blur 0.2 **−88.7 dB**, Mirror Blur 0 **−59.2 dB**, Window Blur 0 **−58.2 dB** (need ≤ −40); HarnessWrap (periodic tiling) control **−24.4 dB** at Blur 0 and 0.2, fails as required; Bandlimited + image (embedded set, Ellipse, A4 exact-cycle) nonharm/max **−117.8 dB**, h ≤ 14, fit 99.7 % = chebFit 99.7 % |
| 6. H10 bytes determinism | ✅ Achieved | A = B = `6d8a1f1b…35a297`; Sine Product fallback `f567c150…9927` differs |
| 7. Round A rows, `--all`, discipline, docs | ✅ Achieved | `--gate all`: **ALL GATES PASSED — 135 check(s), 0 failure(s), 109.1 s** from `/` and **110.0 s** from the repo root (≤ 3 min). H2 Bandlimited grid 66 / 66 (worst h1-max −2.3 dB, Radial Rings × Ellipse); H9 Bandlimited row 0.000e+00 at 64 / 256 / 1024; H8 60 changes / 8 notes 0, 100 Quality changes / 16 notes 0, storm 0, image row 0 (`TerrainImage::liveCount 2, ChebyshevSet::liveCount 2`); H7 2× delta **4.19 %** (11.26 / 7.07) and **4.38 %** (11.50 / 7.12), rows unison 4 17.14 / 17.47 %, 4× 7.38 / 7.50 %, Bandlimited **11.61 / 12.06 %** (reported — see Issues); H3 (0 non-finite, pre-blocker \|y\| 0.8905, \|DC\| 9.56e−4, fb = 0 66 / 66, Nyquist −8.3 dB), H4 (5 / 5 / 2 and 13 / 12 / 5 as amended), H5 (worst 1.08), tuning (0.0000 cent × 10), `latency` (1 / 4 on 9 pairs), `decimator`, `crossfade`, `saturation`, `centroids`, `smoke` green. `grep -c runDispatchLoopUntil main.cpp` = 1; `struct Retirable` = 1; Wavetable grep 0; DSP-05 grep hits only the `prepare-time only` / `block-rate (DSP-05 exemption …)` / inherited Bend-warp lines, none in `ChebyshevSet.h` / `TerrainImage.h`; `build-asan/` at `.gitignore:89`; `tests/.gitignore:3-4` still cover exports and golden WAVs. Commits `6428829d` (20 files, all under `plugins/O-Strata`) and `6f30137b` (11 files: `plugins/O-Strata` + `.gitignore` + `PLUGINS.md`) carry the ROADMAP messages; `git tag -l '*Strata*'` empty; O-Prism / `research/` untouched since `247cb77e`; BRIEF / spec / ARCHITECTURE / ROADMAP sha256 equal STATUS `contract_checksums`. CHANGELOG has the Round B entry + "Known limits — Bandlimited mode"; README lists Round A and Round B gates with the ASan commands; SUMMARY carries the full H6 table, H7 total / baseline / delta and the latencies |

## Requirements Verification

**Stage:** stage-2 — final (Rounds A + B). Round A's 15 completions stand (`round-a/VERIFICATION.md`); the four Round B halves close here.
**Requirements for this stage:** 21 total (FUNC-01..06, FUNC-09, FUNC-10, DSP-01..07, PERF-01, PERF-02, QUAL-01, QUAL-02 = 19 rows, plus the DSP half of FUNC-07 and the bytes half of FUNC-08) — 20 must, 1 should.

| Requirement | Priority | Status | Acceptance Criteria |
|-------------|----------|--------|---------------------|
| FUNC-01: live wave-terrain oscillator, θ parity | must | ✅ Complete (Round A) | H1 all rows ≤ −119 dB this run; Chebyshev identity row −128.3 dB |
| FUNC-02: 6 analytic terrains | must | ✅ Complete (Round A) | `centroids` green |
| FUNC-03: 11 orbits | must | ✅ Complete (Round A) | `centroids` green |
| FUNC-04: trajectory feedback bounded | must | ✅ Complete (Round A) | H3 green (0 non-finite, \|y\| ≤ 0.8905, \|DC\| < 1e−3) |
| FUNC-05: 10 new destinations per osc, per-sample | must | ✅ Complete (Round A) | H5 worst 1.08; the three terrain destinations are ignored per voice in Bandlimited by contract (CONTEXT / ARCH, `scheduler` (d)) |
| FUNC-06: per-oscillator Quality Bandlimited / 2× / 4× | must | ✅ Complete (Bandlimited half closed 2026-09-12) | H6 Bandlimited 127 / 127; analytic ↔ Chebyshev crossfade in the storm click metric (ratio ≤ 1.16); H8 Quality row 0 allocations; `latency` 1 / 4 unchanged; acceptance re-worded to the A-note exact-cycle rows and the ≤ bound (see Issues) |
| FUNC-07: PNG import (DSP half) | must | ⏸️ Stage 3 row; **DSP half verified** | `import` (a)–(g), H11; chooser / drag-and-drop / 3D view are Stage 3 |
| FUNC-08: PNG persistence (bytes half) | must | ⏸️ Stage 4 row; **bytes half verified** | H10 identical SHA-256 across two instances; state child / cap / notice are Stage 4 |
| FUNC-09: O-Prism sections carry over | must | ✅ Complete (Round A) | param-dump diff empty (205 rows); smoke green |
| FUNC-10: microtonal tuning engine | must | ✅ Complete (Round A) | tuning 0.0000 cent on 10 rows |
| DSP-01: pitch-tracked terrain frequency | must | ✅ Complete (Round A, amended) | H4 as amended, both F |
| DSP-02: Bandlimited Chebyshev mode | must | ✅ Complete | degree-16 triangle (153 coefficients), per-voice truncation by `chebDMax` with the highest harmonic ≤ D_max · K on 127 / 127 sounding rows; sets projected off the audio thread (`scheduler` (g) async == sync, `import` (e) never inside a block) and swapped atomically (`storm` 0 allocations, liveCount 2 after the reaper); PNG terrains projected at import (embedded set, H11 Bandlimited + image −117.8 dB) |
| DSP-03: halfband oversampling, base-rate voice loop | must | ✅ Complete (Round A) | `decimator`, `latency` green |
| DSP-04: clamps, pre-blur, edge, isfinite | must | ✅ Complete (pre-blur / edge half closed 2026-09-12) | H11 Mirror / Window ≤ −58 dB at Blur 0 and 0.2, control −24.4 dB fails; H3 clamps + NaN scrub (Round A) |
| DSP-05: no pow / std::function / allocation per sample | must | ✅ Complete | grep clean on the six DSP files (exemption lines only); H8 four rows 0 counted; the sample loop never dereferences a published set (per-oscillator copy at block start, `TerrainOscillator.cpp`) |
| DSP-06: symmetry rule | must | ✅ Complete (Round A) | H2 66 / 66 at 2× and 66 / 66 in Bandlimited |
| DSP-07: saturation | should | ✅ Complete (Round A) | `saturation` green |
| PERF-01: real-time safe | must | ✅ Complete (scheduler / import rows added) | H8 0 in every row; storm 0 across 117 swaps; import publish inside a block 0 / 100; pluginval strictness 10 ×2 SUCCESS |
| PERF-02: oscillator delta ≤ 12 % (2×, as amended D2) | must | ✅ Complete | 4.19 % / 4.38 %; Bandlimited row reported 11.61 / 12.06 % (not gated — see Issues) |
| QUAL-01: aliasing ≤ −60 dB at 2×; ≤ −90 dB Bandlimited | must | ✅ Complete (Bandlimited half closed 2026-09-12) | 198 / 198 at 2× (worst −70.7 dB); 127 / 127 sounding Bandlimited rows ≤ −90 dB (worst −98.4 dB); 17 muted rows are silent, not aliasing |
| QUAL-02: no zipper noise | must | ✅ Complete (Round A) | H5 worst 1.08; storm click ratio ≤ 1.16 |

**Requirements Summary:**
- ✅ Complete: 19 (every stage-2 row)
- ⚠️ Partial: 0
- ⏸️ Deferred (their Stage 2 halves verified; rows stay at stage-3 / stage-4): 2 (FUNC-07, FUNC-08)
- ❌ Failed: 0

`REQUIREMENTS.md` updated: FUNC-06, DSP-02, DSP-04, QUAL-01 → `complete`; FUNC-07 / FUNC-08 descriptions note the verified half; the FUNC-06 / DSP-02 acceptance bullet re-worded (A2 / A4 / A6 exact-cycle, ≤ D_max · K with equality where the terrain has content on the cut diagonal, the 17 muted rows named as the documented limit); the DSP-05 / PERF-01 / PERF-02 and QUAL-01 bullets ticked with the measured figures.

## Automated Checks

| Check | Result | Notes |
|-------|--------|-------|
| Build (`ninja O-Strata-render-test O-Strata-param-dump O-Strata_VST3 O-Strata_AU`) | ✅ Pass | "no work to do" — the tree at `6f30137b` was already built; VST3 / AU binaries sha256-identical to the installed `O-Strata-dev.vst3` / `.component` (`9a205295…` / `437b69bd…`), so the installed bundles are this tree |
| `O-Strata-render-test --gate all` from `/` | ✅ Pass | ALL GATES PASSED — 135 checks, 0 failures, 109.1 s |
| `O-Strata-render-test --gate all` from the repo root | ✅ Pass | ALL GATES PASSED — 135 checks, 0 failures, 110.0 s (H7 and the storm swap counts differ run to run; every gated figure identical) |
| param-dump vs `.planning/params.tsv` | ✅ Pass | `diff` empty — 205 rows, no parameter, range, list or default changed by Round B (the import API is not a parameter) |
| pluginval strictness 10 VST3 (installed bundle) | ✅ Pass | SUCCESS |
| pluginval strictness 10 AU (installed bundle) | ✅ Pass | SUCCESS |
| `auval -v aumu OuSt OuDv` | ✅ Pass | AU VALIDATION SUCCEEDED |
| DSP-05 grep (six files) | ✅ Pass | exemption-commented lines only; `ChebyshevSet.h` / `TerrainImage.h` 0 hits |
| `runDispatchLoopUntil` in `main.cpp` | ✅ Pass | exactly 1 (`pump`) |
| Wavetable removal grep | ✅ Pass | 0 |
| Ignore scope | ✅ Pass | `build-asan/` at root `.gitignore:89`; `tests/exports/`, `golden/*.wav` ignored; `golden/round-a-grid.sha256` tracked |
| Commit scope / tags / O-Prism / contracts | ✅ Pass | two path-scoped commits with the ROADMAP messages; no tag; O-Prism and `research/` untouched; four contract checksums equal STATUS |
| ASan run (`build-asan/`) | ⚠️ Not runnable on this toolchain | configuration and README commands committed; runtime hangs before `main` on macOS 26 / Xcode 26.3 (memory `critical_asan_runtime_hangs_at_init_on_macos26_xcode263` — known); leak verdict = instance counters, green in `storm` and the H8 image row |
| Warning census (touched `ChebyshevProjector.cpp`, `TerrainScheduler.cpp`, `TerrainImage.cpp`, `TerrainOscillator.cpp`, `main.cpp`; rebuilt the harness) | ✅ Pass (non-blocking) | **0 warnings in the three new Round B files**; `TerrainOscillator.cpp:175 / 495` `-Wfloat-equal`, `:321` `-Wswitch-enum` (pre-existing cache / warp comparisons, Round A); harness-only: unused `mean` / `maxStep` (`main.cpp:362 / 367`), `-Wsign-conversion` ×3, `-Wfloat-equal` ×1, deprecated `createWriterFor` (`main.cpp:1672`) |

## Human Verification

Non-blocking; the harness covers the substance. Left for the next time Logic or the Standalone is open (Stage 4 QUAL-04 is the formal listening pass):

- [ ] Logic: Quality = Bandlimited on `Init` — a held C4 sounds; a ModWheel → Terrain Freq route re-shapes the tone within a click-free swap; an LFO → Terrain Mod X route is inert in Bandlimited and live at 2×
- [ ] Logic: Bandlimited, Epitrochoid 7 over Sine Product — notes from about A6 up go silent (the documented limit); the same notes play at 2× / 4×
- [ ] Standalone / Stage 3: an imported PNG (once the chooser lands) sounds through the sync path; Blur softens, Window vs Mirror differ at r = 1
- [ ] Listening: the Round A `tests/exports/` grid remains the Stage 4 defaults material

## Issues Found

- **Finding 1 — 17 Bandlimited rows muted by truncation (accepted as the documented limit).** All 17 sit at A6 with a K ≥ 6 orbit on an even terrain (Sine Product at Mod X = 0.5 is odd × odd, so it mutes too) or with a K = 4 orbit on Radial Rings (D_max = 2 at a 0.10 taper weight, strongest harmonic < −60 dBFS); the list is exactly what the contracted law (ARCH Decision — D_c = fs / (2·K·f) − 1, lower clamp at 1, one-harmonic margin) predicts, recomputed independently in this session, so the muted tier hides no defect. Mitsuhashi sounds on every row because its `tri()` wrap has odd content on the linear diagonal. **Ruling:** the taper law is the contract (CONTEXT "Inherited contracts — NOT re-opened"); the rows are silent, not aliasing, and are recorded in CHANGELOG "Known limits — Bandlimited mode". The two remedies in SUMMARY (allow d = 2 without the margin when 2·K·f < fs/2 — insufficient for K = 8 at A6; or fall back to the analytic 2× path above the muting note) both re-open the law and are **an optional discuss item for Stage 4 or v1.1**, alongside the Round A soft-knee item. Not a blocker.
- **Finding 2 — H7 Bandlimited delta straddles 12 %.** Measured 11.61 % (run 1) and 12.06 % (run 2) here, 11.69 / 11.82 / 12.27 % in execute — mean ≈ 11.9 %, i.e. the Chebyshev mode costs about what 2× costs (4.2–4.4 % delta at 2×, 7.4–7.5 % at 4×) rather than less. PERF-02 as contracted (CONTEXT D2: the 2× row) is satisfied with 3× headroom; the Bandlimited row is reported, not gated, and Decision 25's fallback layout was an executor's option, not a requirement. **Ruling:** PERF-02 complete; the vectorised basis evaluator (three-term recurrence for T_n(x), T_m(y) then 153 independent FMAs, SUMMARY's recommended follow-up, est. 2–3×) is recorded as a **Stage 4 (Polish) performance item**, not a Stage 2 blocker. `clenshaw` at 84.6 ns per dependency-carried evaluation is inside RESEARCH's 43–91 ns band.
- **Finding 3 — ASan not runnable.** Toolchain limit already in memory; the instance-counter rows are the leak verdict (`ChebyshevSet::liveCount = 2, retired = 0` after the reaper in both storms; `TerrainImage::liveCount 2, ChebyshevSet::liveCount 2` in the H8 image row). Re-run `build-asan/` on a toolchain where the runtime initialises (Stage 4 CI or a later Xcode). Accepted.
- **Acceptance wording (ROADMAP 2.4 / REQUIREMENTS FUNC-06 bullet).** ROADMAP asked for "max harmonic index = D_max · K" at C2 / C4 / C6; the harness measures A2 / A4 / A6 exact-cycle (PLAN Decision 44) and gates ≤ with equality printed (13 / 127 equal). Equality is unsatisfiable where the terrain has no content on the cut diagonal — Sine Product × Ellipse at fit 100 % has h ≤ 8 against a bound of 16 because the terrain's own spectrum ends there. Same pattern as the Round A DSP-01 ruling (memory `pattern_acceptance_criterion_unsatisfiable_under_contracted_law`): the bound is the contract, equality was the estimate. The REQUIREMENTS bullet is re-worded to what DSP-02 promises (highest harmonic ≤ D_max · K, ≤ −90 dB non-harmonic on every sounding exact-orbit row, A-notes, the 17 muted rows named). No code change.
- **Harness warnings (non-blocking):** two unused helpers (`mean`, `maxStep`) and a deprecated `createWriterFor` in `main.cpp` — a one-line tidy for the next harness edit; nothing in the plugin's new files.
- None in the plugin.

## Stage Verdict

**Status:** ✅ VERIFIED — **Stage 2 (DSP) complete**: Round A (`round-a/VERIFICATION.md`, 2026-09-11) + Round B (this file, 2026-09-12). Every H1–H11 gate and every named gate green in one `--gate all` run from two directories; COMPAT-01 regression green on the installed build; all 19 stage-2 requirement rows complete, the FUNC-07 DSP half and FUNC-08 bytes half verified for their later stages.

**Ready for next stage:** Yes — Stage 3 (GUI: WebGL 3D terrain view, readout pushes `terrainStatus` / `terrainHeightmap` / `terrainCycle` / `terrainPlayhead`, file chooser + drag-and-drop on the import API, relays from `parameter-spec.md` v2, removal of the two native stubs) via `/plugin-discuss O-Strata 3-gui`.

**Blockers:** none

**Carried into Stage 3 / Stage 4 (from this verification):**
- Stage 3 reads the readout atomics (`chebGeneration`, fit %, `chebPartialsAtC4`, `chebApproximate`, `imageGeneration`) and the import API exactly as the harness does; the muted-note limit belongs in the UI's Bandlimited readout ("approximate" / partials count) rather than a new parameter.
- Stage 4: (i) optional discuss item — muted rows above ≈ A6 with K ≥ 6 orbits in Bandlimited (analytic fallback above the muting note, or d = 2 without the margin) together with the Round A soft-knee item; (ii) PERF item — vectorised Chebyshev basis evaluator (H7 Bandlimited ≈ 12 % → target ≈ 2× cost or below); (iii) ASan re-run when the toolchain allows; (iv) harness warning tidy; (v) `--gate export` golden re-record only if a Stage 4 defaults change touches the grid.
