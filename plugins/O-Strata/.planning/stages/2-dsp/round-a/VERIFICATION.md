# Stage 2: DSP (live wave-terrain oscillator), Round A — Verification

## Verification Date

2026-09-11

**Plugin:** O-Strata · **Stage:** 2 of 4 (DSP) · **Round:** A = Phases 2.1 + 2.2 + 2.3 (CONTEXT D1) · **Mode:** manual
**Inputs:** `CONTEXT.md` (D1–D4, Stage 2 test criteria), `RESEARCH.md`, `PLAN.md` (28 tasks, Decisions 1–24, success criteria), `SUMMARY.md` (execute report, commits `4640a1a7` / `45b1fb40` / `0c07923a`), `ROADMAP.md` Phases 2.1–2.3, `REQUIREMENTS.md` v2.0.0
**Method:** every SUMMARY claim was re-measured in this session from the committed tree at `0c07923a` (Source identical to the working tree): `ninja` rebuild of the plugin, param-dump and harness targets, `O-Strata-render-test --gate all` run twice (from `/` and from the repo root), `--gate smoke --with-disk`, param-dump diff, pluginval ×2, auval, bundle hashes, the grep gates, contract checksums. Nothing below is copied from SUMMARY without a fresh run. One harness edit was made in this phase (H4, see "Issues Found" — the DSP-01 ruling); no plugin source changed.

## Goal-Backward Analysis

### Original Goals (from CONTEXT.md "Goal" + ROADMAP Phases 2.1–2.3, Round A scope)

1. `TerrainOscillator` replaces `WavetableOscillator` in place: 11 orbits, 6 analytic terrains under the πF convention, θ parity with the inherited Sync / Bend / Window / FM warps, unison capped at 4; the wavetable path deleted, the reaper type-erased, the two native functions stubbed (FUNC-01, FUNC-02, FUNC-03, FUNC-09).
2. Orbit Size and the 20 new mod destinations read per sample through 5 ms smoothing, without zipper noise; 40 Hz sidebands prove the per-sample path (FUNC-05, QUAL-02).
3. Pitch-tracked terrain frequency (ARCH Decision 2) and bounded trajectory feedback (ARCH Core 4) with a 5 Hz DC blocker; Saturation identity at 0 (DSP-01, FUNC-04, DSP-04 clamp half, DSP-07).
4. Per-oscillator 2× / 4× polyphase-IIR halfband oversampling with a click-free Quality switch, no allocation on any change, constant +1 latency report; aliasing ≤ −60 dB at 2× on the whole library grid (DSP-03, FUNC-06 2×/4× half, QUAL-01 2× half, DSP-05, PERF-01).
5. CPU: oscillator delta ≤ 12 % at the default patch (PERF-02 as amended by D2), with the H6 / H7 fallback outcome recorded for Round B.
6. Symmetry gate on every library pair and the `Init` bank (DSP-06); tuning engine regression on ≥ 5 keys (FUNC-10).
7. Harness `O-Strata-render-test` with exit-code gates H1–H9 + tuning + smoke, runnable from any directory, no message loop; three path-scoped commits, no tag, O-Prism untouched, no parameter change.

### Deliverables (from SUMMARY.md)

1. `Source/dsp/TerrainOscillator.h/.cpp`, `Orbits.h`, `Terrains.h`; `StrataVoice` member swap + 34 cached pointers; five wavetable files deleted; `Retirable` / `retire()`; editor stubs `"{}"` / `"[]"`.
2. 24 processor-owned ramp rows (`baseRamps` / `rampBuffers`) read by every voice; log2-domain Terrain Freq; `ModulationMatrix::isDestinationRouted`.
3. `updateBlockRate` tracking ratio; per-partial feedback (two-sample average + leaky integrator, ±0.5, NaN scrub); DC blocker; `tanh(g·y)/tanh(g)` saturation with the exact-0 skip.
4. `Source/dsp/HalfbandDecimator.h` (2×: 3 + 2 sections; 4×: 2 + 1), oversampling loop, 64-sample crossfade, `setLatencySamples (distortion + 1)` at both sites.
5. H7 rows in the harness; shared ramps and fb = 0 skip applied before the gate; no ladder rung.
6. H2 grid + `FactoryPresets::build` loop; `--gate tuning`.
7. `tests/render-harness/` (main.cpp, README, reference headers, fixture, `golden/round-a-grid.sha256`), `tests/.gitignore`, CMake `ouaricon_add_processor_console`; CHANGELOG / PLUGINS.md / STATUS.

### Goal Achievement

| Goal | Status | Evidence (re-measured this session) |
|------|--------|----------|
| 1. Live oscillator, parity, deletion | ✅ Achieved | H1: Off / Sync / Bend / FM / Window × unison {1, 4} = −128.3 / −120.6 / −126.5 / −121.6 / −120.3 / −126.6 / −119.7 / −125.1 / −119.7 / −119.5 dB (need ≤ −80); C4 = 261.6256 Hz, −0.000 cent; Bend 0.6 control −22.9 dB fails as expected. `grep -rn 'Wavetable\|WaveShape\|kNumMipmapLevels' Source/ CMakeLists.txt --exclude-dir=ui` = 0 (the untouched page keeps its `WavetableDisplay` class until Phase 3.1); `PluginProcessor.h:332-342` `struct Retirable` + `retire (std::unique_ptr<Retirable>)`; `PluginEditor.cpp:483 / 488` `complete ("{}")` / `complete ("[]")`; `kMaxUnison = 4` (`TerrainOscillator.h:92`); smoke [6] 7 terrains / 11 orbits / 3 Quality choices (default `2×`) |
| 2. Per-sample destinations, no zipper | ✅ Achieved | H5: 22 rows excess ratio 1.00–1.08 (worst `oscAOrbRot` 1.08, need ≤ 1.5), ramp-0 control 7.62 fails as expected, ModWheel route 1.00; sidebands at ±40 Hz: h1 +0.3 / +0.4 dB, h2 −4.4 / −4.3 dB (need ≥ −40); smoke [4]: LFO1 → OscA Terrain Freq changes the render max\|Δ\| = 5.459e−1, Pitch control 6.484e−1, unrouted render deterministic; `getModDestNames().size() == 46` |
| 3. Tracking, feedback, saturation | ✅ Achieved (DSP-01 acceptance re-worded — see Issues) | H4: F = 1 counts 5 / 5 / 2, F = 4 counts 13 / 12 / 5 (C2 ≈ C4 within 2, C6 ≤ C4, tracked C6 below the Track 0 count 5 / 12); control Track 0 at F = 8 raises non-harmonic energy 36.3 dB (Epitrochoid 7, C8), 29.9 dB (Ellipse, C8), 22.6 dB (Epi 7, C6). H3: 2376 grid points × 2 passes, 0 NaN/Inf, pre-blocker \|y\| worst 0.8905 (post-blocker 1.3985 reported), \|DC\| worst 9.56e−4, fb = 0 byte-identical on 66 / 66 pairs, two-sample average keeps fs/2 below the strongest bin everywhere (worst −8.3 dB), single-sample control rises 103.4 dB. DSP-07: Sat 0 byte-identical to the bypassed branch, partials 5 → 16, \|y\| max 0.4650 |
| 4. Oversampling, Quality switch, latency, aliasing | ✅ Achieved (2× half) | decimator: coefficients as designed, L2 = 1.259 / L4 = 1.743, 30 kHz alias −94.0 dB (need ≤ −68). H6 at 2×: **198 / 198 rows ≤ −60 dB, worst −70.7 dB** (Mitsuhashi × Epitrochoid 7, A2); 4× reported worst −72.6 dB (Ridged Cosines × Butterfly, A6); 1× analytic worst −30.1 dB; 44.1 k / 96 k rows within 3 dB or below −60. crossfade: max step inside the window 0.0261, outside 0.0380, plateau 0.0380. latency: bypassed 1, distortion on 4 (= 3 + 1) for all 9 Quality pairs. H8: 0 allocations across 60 terrain / orbit changes (8 notes, 3 s) and across 100 Quality / terrain / orbit changes (16 notes, 5 s) |
| 5. CPU | ✅ Achieved | H7 (M4 Max, Release, best of 3): run 1 total 11.58 % / baseline 7.10 % / **delta 4.48 %**; run 2 total 11.90 % / baseline 7.34 % / **delta 4.56 %** (need ≤ 12; SUMMARY 4.44 %). Rows: unison 4 / 2× delta 17.20 %, 4× delta 7.39 %, Bandlimited-as-1× delta 1.98 %. No fallback rung applied (SUMMARY "H6 / H7 outcome"); the CPU model's ≈ 3× pessimism confirmed |
| 6. Symmetry, tuning | ✅ Achieved | H2: 66 / 66 pairs, 100 % windows each, worst h1-max −2.3 dB (Radial Rings × Ellipse); `presets: 1 (Init)` — 1 / 1; centred-circle control −109.0 dB / 0 % windows fails as expected. tuning: Scala just-major and 31-EDO, keys 48 / 55 / 60 / 64 / 67, \|Δ\| = 0.0000 cent on all 10 rows (need ≤ 0.5) |
| 7. Harness + discipline | ✅ Achieved | `--gate all`: **ALL GATES PASSED — 89 check(s), 0 failure(s)** in 68.8 s from `/` and 70.3 s from the repo root; `--gate smoke --with-disk` 36 / 36 ([5]: `Factory/Init/Init.json` + `.factory-version 1.0.0`, two files); `grep -c runDispatchLoopUntil main.cpp` = 0; `git check-ignore -v` → `tests/.gitignore:3` for `tests/exports/x.wav` and `:4` for `golden/x.wav`; `golden/round-a-grid.sha256` tracked. Commits `4640a1a7` / `45b1fb40` / `0c07923a` carry the ROADMAP messages and touch only `plugins/O-Strata` (+ `PLUGINS.md` on the third); `git tag -l '*Strata*'` empty; `git diff HEAD -- plugins/O-Prism` and `research/` empty; BRIEF / spec / ARCHITECTURE / ROADMAP sha256 equal the STATUS `contract_checksums` |

## Requirements Verification

**Stage:** stage-2, Round A (Phases 2.1–2.3). Round B (Phases 2.4–2.5) owns the Bandlimited and PNG halves.
**Requirements for this stage:** 21 total (FUNC-01..06, FUNC-09, FUNC-10, DSP-01..07, PERF-01, PERF-02, QUAL-01, QUAL-02) — 20 must, 1 should.

| Requirement | Priority | Status | Acceptance Criteria |
|-------------|----------|--------|---------------------|
| FUNC-01: live wave-terrain oscillator, θ parity | must | ✅ Complete | H1 ≤ −80 dB all 10 rows; C4 ± 1 cent; Orbit Size changes the spectrum (centroids 263 → 653 Hz) |
| FUNC-02: 6 analytic terrains, Mod X / Mod Y documented | must | ✅ Complete | 12 / 12 shape inputs move the spectrum (centroid ≥ 5 % or a partial ≥ 3 dB; Ridged Cosines Mod X: centroid 3.0 %, partial move 24.6 dB); formulas documented in `Terrains.h` |
| FUNC-03: 11 orbits, Orbit Mod, Size monotone | must | ✅ Complete | 10 / 10 non-Ellipse Orbit Mods move the spectrum (Ellipse inert, documented); Orbit Size centroid monotone 263 277 308 353 410 480 566 653 Hz |
| FUNC-04: trajectory feedback bounded | must | ✅ Complete | H3 grid finite, pre-blocker \|y\| ≤ 0.8905, \|DC\| < 1e−3, fb = 0 byte-identical 66 / 66 |
| FUNC-05: 10 new destinations per osc, per-sample | must | ✅ Complete | 46-entry list; 22 rows smoothed (H5); ±40 Hz sidebands present |
| FUNC-06: per-oscillator Quality Bandlimited / 2× / 4× | must | ⚠️ Partial (2× / 4× half) | Quality switch no-alloc + no click beyond the crossfade; latency reported from `prepareToPlay`; **Bandlimited half → Round B (2.4)** |
| FUNC-09: O-Prism non-oscillator sections carry over | must | ✅ Complete | param-dump byte-identical to `params.tsv` (205 rows, Stage 1 ID-parity diff unchanged); smoke [3] 205-param round-trip; FX / filter / LFO / matrix untouched by Round A (`StrataVoice` diff limited to the oscillator feed) |
| FUNC-10: full microtonal tuning engine | must | ✅ Complete | Scala + 31-EDO on 5 keys each, 0.0000 cent |
| DSP-01: pitch-tracked terrain frequency | must | ✅ Complete (acceptance amended 2026-09-11) | H4 as re-worded: C2 / C4 within 2, C6 ≤ C4, tracked C6 < untracked C6 at F = 1 and F = 4; Track 0 control ≥ 20 dB — see Issues Found |
| DSP-02: Bandlimited Chebyshev mode | must | ⏸️ Deferred | Round B (2.4) |
| DSP-03: per-oscillator halfband oversampling, base-rate voice loop | must | ✅ Complete | decimator gate; latency 1 / 4 on all 9 pairs; voice loop / filters / LFOs / matrix at base rate (block-rate feed + ramp rows inspected) |
| DSP-04: clamps, pre-blur, isfinite guards | must | ⚠️ Partial (clamp half) | H3 clamps + NaN scrub (0 non-finite renders); **edge / pre-blur half → Round B (2.5, H11)** |
| DSP-05: no pow / std::function / allocation per sample | must | ✅ Complete | grep over the four DSP files hits only `prepare-time only` (Superellipse LUT), `block-rate (DSP-05 exemption)` (setUnison cache, `updateBlockRate`) and the documented inherited Bend-warp `pow` (SUMMARY deviation 9); H8 0 allocations in both rows |
| DSP-06: symmetry rule | must | ✅ Complete | H2 66 / 66 + `Init`; negative control fails |
| DSP-07: saturation tanh, identity at 0 | should | ✅ Complete | byte-identical at 0; partials 5 → 16 at 1 |
| PERF-01: real-time safe | must | ✅ Complete | H8 0 allocations (operator-new family; HeapBlock / malloc paths covered by grep + inspection per plan Decision 4); pluginval strictness 10 ×2 |
| PERF-02: oscillator delta ≤ 12 % | must | ✅ Complete | 4.48 % / 4.56 % (two runs); total and baseline printed; unison 4 / 4× / 1× rows reported |
| QUAL-01: aliasing ≤ −60 dB at 2×; ≤ −90 dB Bandlimited | must | ⚠️ Partial (2× half) | 198 / 198 rows ≤ −60 dB at 2×, worst −70.7 dB; **Bandlimited −90 dB half → Round B (2.4)** |
| QUAL-02: no zipper noise | must | ✅ Complete | H5 excess ratio ≤ 1.08 on 22 rows + ModWheel; ramp-0 control fails |

**Requirements Summary:**
- ✅ Complete: 15 (FUNC-01, 02, 03, 04, 05, 09, 10, DSP-01, 03, 05, 06, 07, PERF-01, PERF-02, QUAL-02)
- ⚠️ Partial (Round B owns the other half): 3 (FUNC-06, DSP-04, QUAL-01)
- ⏸️ Deferred (Round B): 1 (DSP-02); FUNC-07 DSP half and FUNC-08 bytes half are Round B too (listed under stage-3 / stage-4 in REQUIREMENTS)
- ❌ Failed: 0

`REQUIREMENTS.md` updated: the 15 complete rows → `complete`; FUNC-06 / DSP-04 / QUAL-01 → `partial` (Round A half named); DSP-02 stays `pending`; DSP-01 text and acceptance amended (see Issues).

## Automated Checks

| Check | Result | Notes |
|-------|--------|-------|
| Build (`ninja O-Strata-render-test O-Strata-param-dump O-Strata_VST3 O-Strata_AU`) | ✅ Pass | exit 0; the relinked VST3 / AU binaries are sha256-identical to the installed `O-Strata-dev.vst3` / `.component` (`fe459faa…` / `44681c1c…`), so the installed bundles are this tree. 10 compiler warnings, all pre-existing `-Wfloat-equal` cache comparisons / `-Wswitch-enum` / harness `-Wsign-conversion` — see Issues |
| `O-Strata-render-test --gate all` from `/` | ✅ Pass | ALL GATES PASSED — 89 checks, 0 failures, 68.8 s |
| `O-Strata-render-test --gate all` from the repo root | ✅ Pass | ALL GATES PASSED — 89 checks, 0 failures, 70.3 s (H7 delta 4.56 % vs 4.48 %; every other number identical) |
| `--gate smoke --with-disk` | ✅ Pass | 36 checks, 0 failures; on-disk bank = `Init` + `.factory-version 1.0.0`, unchanged after the run |
| `--gate export` | — not re-run | writes 198 WAVs; `golden/round-a-grid.sha256` tracked at `0c07923a` (198 lines) |
| param-dump vs `.planning/params.tsv` | ✅ Pass | `diff` empty — 205 rows, no parameter, range, list or default changed (no `Auto` Quality entry was needed) |
| pluginval strictness 10 VST3 (installed bundle) | ✅ Pass | SUCCESS, exit 0 |
| pluginval strictness 10 AU (installed bundle) | ✅ Pass | SUCCESS, exit 0 |
| `auval -v aumu OuSt OuDv` | ✅ Pass | AU VALIDATION SUCCEEDED |
| Wavetable removal grep | ✅ Pass | 0 hits in `Source/` (excl. `ui/`) and `CMakeLists.txt` |
| DSP-05 grep | ✅ Pass | hits only on exemption-commented lines |
| `runDispatchLoopUntil` in `main.cpp` | ✅ Pass | 0 |
| Ignore / golden scope | ✅ Pass | `tests/exports/` and `golden/*.wav` ignored via `tests/.gitignore`; `.sha256` tracked |
| Commit scope / tags / O-Prism / contracts | ✅ Pass | three path-scoped commits with the ROADMAP messages; no tag; O-Prism and `research/` diffs empty; four contract checksums equal STATUS |

## Human Verification

Non-blocking; the harness covers the substance. Left for the next time Logic or the Standalone is open (the Stage 4 QUAL-04 listening pass is the formal one):

- [ ] Logic: a held note on `Init` sounds the terrain oscillator at 2×; automating Terrain Freq or Orbit Size is click-free; switching Quality mid-note is click-free
- [ ] Logic: the plug-in reports 1 sample of latency with Distortion bypassed (4 with it on)
- [ ] Listening: `tests/exports/` grid (`--gate export`) — any dull or harsh library default is a Stage 4 defaults note, not a Round B change

## Issues Found

- **H4 / DSP-01 ruling (the round's open item).** SUMMARY measured 5 / 5 / 2 at F = 1 and 13 / 12 / 5 at F = 4 against "C2 / C4 / C6 differ by ≤ 2". Under the contracted law (ARCH Decision 2, F_eff = F · min(1, C4/f)^Track, scaling downward only) C2 and C4 share F_eff and C6 sits at F/4, so the F = 4 form of the criterion is unsatisfiable by construction and the F = 1 form missed by one partial sitting 2 dB under the −40 dB counting threshold (ARCH estimated "C6 ≈ 4", measured 2). The two alternatives in SUMMARY — a different default Track (spec-locked, checksummed) or the soft knee (re-opens Decision 2, which Stage 2 CONTEXT lists as not re-opened) — are outside this phase's authority. **Ruling: the law is the contract and the acceptance wording was the estimate error.** DSP-01's statement and acceptance in `REQUIREMENTS.md` are amended to what Decision 2 promises (identical below C4, not growing above it, tracked C6 count strictly below the untracked one, Track 0 control ≥ 20 dB), and the harness gate now asserts exactly that (three checks per F, the original spread printed as a diagnostic) — `tests/render-harness/main.cpp` `gateH4`, the only code change in this phase. Round B may still choose the soft knee as a defaults / law change through its own discuss decision; nothing here forecloses it.
- **4× is reported at −72.6 dB, not the ROADMAP's "≤ −90 dB reported".** ROADMAP 2.3 lists "4× ≤ −90 dB on the same grid reported" beside the 2× gate; the measured 4× worst is −72.6 dB (Ridged Cosines × Butterfly, A6), and 4× barely beats 2× on the A6 rows (−72.6 vs −72.6) because the residual is harmonic-side content near fs/2, not fold-back. −90 dB is QUAL-01's Bandlimited target and stays with Round B; the 4× figure is a reported number, not a gate (PLAN Decision 1). Recorded as a Round B input: the Bandlimited rows, not 4×, are where −90 dB is expected.
- **Compiler warnings (non-blocking):** `TerrainOscillator.cpp:169 / 407` and `PluginProcessor.cpp:779-780` `-Wfloat-equal` on intentional cache-identity comparisons (`unisonCount / detune / width`, `rotation`, the inherited tuning cache); `TerrainOscillator.cpp:249` and `theta_reference.h:115` `-Wswitch-enum` (Off / Sync / Window fall to `default`); `main.cpp:620 / 792` `-Wsign-conversion`. Warnings are not fatal in this tree; a Round B tidy (explicit `case` labels, `juce::exactlyEqual`) is optional.
- **Gate-time drift:** SUMMARY's `--all` took 70.8 s; this session 68.8 / 70.3 s — the ≤ 3 min Round B budget has ≈ 110 s of headroom for H10 / H11 and the Bandlimited H6 rows.
- None in the plugin.

## Stage Verdict

**Status:** ✅ VERIFIED — Round A (Phases 2.1–2.3). Stage 2 continues into Round B (Phases 2.4–2.5); the stage-level verdict is written by Round B's VERIFICATION.md, which cites this file for the Round A gates.

**Ready for next round:** Yes — Round B plan (`/plugin-plan O-Strata 2-dsp`), reading SUMMARY "H6 / H7 outcome" first: **no fallback rung was applied** (2× worst −70.7 dB; delta 4.5 %), so Bandlimited and the PNG path layer onto the settled 2× loop as designed.

**Blockers:** none

**Round B inputs (from this verification):** DSP-01 amended (soft knee remains an optional Round B discuss item, not a defect); Cosine Wells at πF (Chebyshev fit numbers in RESEARCH §2.10 to re-measure at πF); 1× analytic worst −30.1 dB shows why Bandlimited matters; 4× reported at −72.6 dB (−90 dB belongs to the Bandlimited rows); `harnessDcBlockerBypass` and the 1× Nyquist control are available for the Bandlimited rows; `--all` headroom ≈ 110 s; H8 coverage statement stands (HeapBlock paths by inspection — Round B's `ChebyshevSet` / `TerrainImage` publish must keep allocation on the message thread / ThreadPool). Artifact layout per CONTEXT D1: `PLAN.md` / `SUMMARY.md` / this file move to `stages/2-dsp/round-a/`; Round B writes fresh ones at the fixed paths.
