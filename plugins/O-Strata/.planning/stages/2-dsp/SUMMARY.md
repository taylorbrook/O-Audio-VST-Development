# Stage 2 — DSP (live wave-terrain oscillator), Round B: SUMMARY

**Plugin:** O-Strata · **Stage:** 2 of 4 (DSP) · **Round:** B = Phases 2.4 + 2.5 (CONTEXT D1) · **Phase:** execute ✓
**Date:** 2026-09-12 · **Mode:** manual · **Branch:** `main` (two path-scoped commits under `plugins/O-Strata`, + `PLUGINS.md` and the root `.gitignore` on the second)
**Inputs:** `CONTEXT.md` (D1–D4), `RESEARCH.md`, `PLAN.md` (Round B — 16 tasks / 7 waves, Decisions 25–46), `round-a/{SUMMARY,VERIFICATION}.md` ("Round B inputs"), `parameter-spec.md` v2 (locked, untouched).
**Commits:** Phase 2.4 `feat(O-Strata): Phase 2.4 — Chebyshev bandlimited mode, coefficient scheduler, atomic swap` · Phase 2.5 `feat(O-Strata): Phase 2.5 — PNG terrain path, edge gates, harness consolidated` (this commit). Baseline `247cb77e`.
**Template:** summary-complex (16 tasks, 24 files, harness with 135 checks).

Every number below was measured in this session with the committed harness (`tests/render-harness/main.cpp`), Release build (`-O3`), Apple M4 Max, Apple clang 17, macOS 26 / Darwin 25.6 (memory `pattern_review_recomputes_instead_of_measuring`). Log lines quoted verbatim where they matter. `--gate all` = every gate H1–H11 + `tuning smoke centroids saturation decimator crossfade latency clenshaw scheduler storm import` (`export` excluded).

---

## Outcome

Both oscillators have the two remaining terrain sources on the DSP side. **Bandlimited mode** evaluates a degree-16 Chebyshev triangle (153 coefficients, 2-D Clenshaw) at 1×, truncated per voice by a continuous raised-cosine taper on the diagonal cut-off D_c = fs / (2·K·f) − 1 (K = the orbit's trigonometric degree, nominal K for the three approximate orbits), rebuilt as a per-oscillator tapered copy at block start — the sample loop never dereferences a published set — and crossfaded on arrival (64 base samples, equal-gain; analytic ↔ Chebyshev too). Sets are projected off the audio thread by `TerrainScheduler` (50 ms message-thread poll, 2-thread `juce::ThreadPool`, ModWheel / Aftertouch folded through its own `ModulationMatrix`, keys quantised to 1/1024, Terrain Freq clamped ≤ 2, throttle + trailing debounce, cancellation, `callAsync` + weak `AliveToken`, type-erased retire) with readout atomics for Stage 3. **The PNG path** decodes to ≤ 1024² luminance, pre-blurs (3-pass box, mirror-padded), embeds a Chebyshev set projected at F = 1, and is read bilinearly with Mirror / Window edge handling through a processor import API (bytes or file) that never touches a parameter; Blur / Edge re-run at a 150 ms cadence.

```
ALL GATES PASSED — 135 check(s), 0 failure(s), 112.7 s          (--gate all from /)
ALL GATES PASSED — 135 check(s), 0 failure(s), 112.7 s          (--gate all from the repo root)
```

Phase 2.4 was committed at `113 check(s), 0 failure(s), 108.2 s` (from `/`) with `--gate storm --gate scheduler` green from the repo root (`20 check(s), 0 failure(s), 23.1 s`). COMPAT-01 regression on the installed Phase 2.5 build: pluginval strictness 10 **SUCCESS** (VST3) / **SUCCESS** (AU), `auval -v aumu OuSt OuDv` **AU VALIDATION SUCCEEDED**. `params.tsv` diff: **empty** after both phases (no parameter, range, list or default changed). `git status --short plugins/O-Prism`: empty.

## Round B findings for verify (design, not defects)

1. **17 of the 144 Bandlimited H6 rows are muted by truncation.** At A6 with a degree-6 / -8 orbit (Epitrochoid 5 / 7, Hypocycloid 7; Epitrochoid 3 / Hypocycloid 5 for Radial Rings) the law leaves D_max = 1 — only the linear diagonal survives, and it is zero for an even terrain (Radial Rings, Saddle, Ridged Cosines, Cosine Wells; Sine Product at Mod X = 0.5 is odd × odd, so it mutes too). Under the contracted taper (ARCH's lower clamp "d ≤ 1 never mutes" + the one-harmonic margin) these notes are silent in Bandlimited mode; 2× / 4× play them. Recorded in the CHANGELOG "Known limits — Bandlimited mode". Options if the contract is re-opened: allow d = 2 without the margin when 2·K·f < fs/2 (not enough for K = 8 at A6), or fall back to the analytic 2× path above the muting note. Muted rows are reported and excluded from the −90 dB gate; 8 further "quiet" rows (strongest harmonic between −60 and −40 dBFS, D_max = 2 at a 10 % taper weight) are gated on the absolute non-harmonic level ≤ −100 dBFS (worst −124 dBFS) because −90 dB relative to a −40 dBFS tone is under the float floor of the path.
2. **H7 Bandlimited (Chebyshev) delta straddles the Decision 25 line:** 11.69 % / 11.82 % / **12.27 %** across three `--all` runs (`total 19.37 %  baseline 7.10 %`), vs 4.47 % for the gated 2× row. The row is reported, not gated (CONTEXT D2), so no fallback was applied. Cause: the 2-D Clenshaw is a dependency chain (`clenshaw` gate: 80–104 ns per evaluation, RESEARCH measured 43–91) — evaluating the basis T_n(x), T_m(y) by the three-term recurrence and summing 153 independent FMAs would vectorise (est. 2–3× faster) at the same numerical cost for |x| ≤ 1; this, not the tensor + corner layout, is the recommended follow-up if verify wants Bandlimited cheaper than 2×.
3. **ASan is not runnable on this toolchain.** `build-asan/` configures and builds (102 steps, 0 errors) but the binary never reaches `main`: `sample` shows the ASan runtime spinning in `__asan::InitializeShadowMemory → __sanitizer::MemoryMappingLayout::Next → dyld_shared_cache_iterate_text_swift` (> 10 min, with and without `MallocNanoZone=0`) on macOS 26 / Xcode 26.3 clang 17. The configuration, the `STRATA_HARNESS_ASAN` compile-out of the operator-new family and the README commands are committed for a toolchain where the runtime initialises; the leak verdict is the instance counters: `storm` `ChebyshevSet::liveCount = 2, retired = 0` after the reaper, H8 image row `TerrainImage::liveCount 2, ChebyshevSet::liveCount 2`.

## H6 — full table (fs = 440·65536/600, exact-cycle A2 / A4 / A6, nonharm/max dB; pre-filter tap, Track 1, defaults)

Round A columns re-measured this run (2× gated ≤ −60 dB: **198 / 198**, worst −70.7 dB Mitsuhashi × Epitrochoid 7 A2; 4× reported worst −72.6 dB Ridged Cosines × Butterfly A6; 1× analytic worst −30.1 dB Saddle × Epitrochoid 7 A6). Rate rows: Sine Product × Ellipse 48 k −124.1 / −114.5 / −103.0, 44.1 k −123.7 / −113.9 / −102.2, 96 k −124.6 / −118.8 / −108.8; Ridged × Epitrochoid 7 −74.7 / −103.7 / −77.7, −72.5 / −103.8 / −78.0, −87.3 / −101.7 / −74.9 — within 3 dB or below −60.

**Bandlimited (Chebyshev) rows** — 8 exact orbits × 6 terrains × 3 notes = 144; **127 sounding rows ≤ −90 dB (worst −98.4 dB, Radial Rings × Ellipse A6)**, highest harmonic above −100 dB ≤ D_max·K on 127 / 127 (equality on 13), 17 muted (finding 1), largest |y| at the tap 0.673 (Radial Rings × Hypocycloid 5 A2 — the Chebyshev value is not clamped, deviation 1). Approximate orbits reported: worst −30.2 dB (Radial Rings × Squarcle A6 — Squarcle's tanh orbit is not a trig polynomial, so its nominal K = 1 lets every diagonal through; by design, readout flags it).

| Terrain | Ellipse | Limaçon | Epi 3 | Epi 5 | Epi 7 | Hypo 3 | Hypo 5 | Hypo 7 |
|---|---|---|---|---|---|---|---|---|
| Sine Product | -123.8 / -114.2 / -102.3 | -112.2 / -110.5 / -102.0 | -119.5 / -110.8 / -98.4q | -120.0 / -110.7 / M | -120.4 / -110.4 / M | -118.6 / -113.8 / -103.7 | -121.4 / -113.0 / -101.6q | -121.8 / -113.2 / M |
| Radial Rings | -110.5 / -107.6 / -98.4 | -118.3 / -112.4 / -104.6 | -115.4 / -109.6 / M | -112.8 / -100.9 / M | -112.1 / -107.8 / M | -119.8 / -112.4 / -100.7 | -112.3 / -107.6 / M | -111.9 / -102.3 / M |
| Saddle | -121.8 / -113.2 / -101.4 | -121.7 / -114.5 / -102.5 | -120.2 / -109.2 / -97.0q | -119.7 / -109.3 / M | -119.9 / -108.6 / M | -126.2 / -115.6 / -103.2 | -123.3 / -112.8 / -100.2q | -121.8 / -112.1 / M |
| Ridged Cosines | -120.3 / -112.3 / -101.2 | -115.3 / -112.1 / -103.6 | -116.5 / -110.3 / -76.4q | -116.9 / -110.2 / M | -117.7 / -106.4 / M | -112.5 / -109.7 / -100.7 | -116.6 / -111.3 / -77.2q | -118.8 / -111.1 / M |
| Mitsuhashi | -124.1 / -113.3 / -102.9 | -124.6 / -114.0 / -105.3 | -119.6 / -110.6 / -103.2 | -120.9 / -114.6 / -103.2 | -120.8 / -114.9 / -103.2 | -119.9 / -112.1 / -105.4 | -123.5 / -111.8 / -105.3 | -121.4 / -116.5 / -105.3 |
| Cosine Wells | -111.1 / -109.5 / -103.2 | -120.0 / -115.6 / -103.5 | -117.1 / -111.3 / -79.2q | -114.7 / -109.6 / M | -114.1 / -103.8 / M | -120.6 / -113.7 / -100.1 | -114.2 / -110.8 / -80.0q | -114.0 / -109.7 / M |

M = muted by truncation (reported); q = quiet row (strongest harmonic −60…−40 dBFS; gated on absolute non-harmonic ≤ −100 dBFS, all −124…−159 dBFS). Cells are A2 / A4 / A6 nonharm/max in dB, generated from the `[H6 BL]` lines of the `--gate all` run from `/` (the per-row lines also carry h_max, D_max·K, fit % and peak).

## H7 — CPU (16 voices × 2 osc, 48 kHz, block 512, 10 s, best of 3, `harnessTerrainKernelBypass` baseline)

```
[H7] total = 11.72 %  baseline (kernel bypass) = 7.25 %  delta = 4.47 %            (gated: <= 12)
[H7 rows] unison 4, 2x: total 26.47 % baseline 8.75 % delta 17.72 %
[H7 rows] unison 1, 4x: total 15.12 % baseline 7.59 % delta 7.54 %
[H7 rows] unison 1, Bandlimited (Chebyshev): total 19.37 % baseline 7.10 % delta 12.27 %   (reported; finding 2)
```

## Latencies

`decimator`: L2 = 1.2589, L4 = 1.7432 base samples; `latency`: `getLatencySamples()` = 1 with distortion bypassed, 4 with distortion on (= distortion 3 + 1) for all 9 Quality pairs — unchanged from Round A.

## Fit table (`--gate scheduler` (a); mx = my = 0.5; %)

| Terrain | F = 1 | F = 2 | ARCH table | Note |
|---|---|---|---|---|
| Sine Product | 100.00 | 100.00 | 100 / 100 | |
| Radial Rings | 100.00 | 96.28 | 100 / 94 | better than ARCH at F = 2 |
| Saddle | 100.00 | 99.85 | ≥ 99 | |
| Ridged Cosines | 99.71 | 98.55 | 99.5 / 96.8 | |
| Mitsuhashi | 98.32 | 87.58 | "≈ 100" (estimate) | `tri()` is piecewise-linear (kinks at ± 0.5 even at F = 1) — deviation 2 |
| Cosine Wells | 99.99 | 98.63 | 98 / 76 (2πF) | re-measured at πF (Round A deviation 1) |

Cosine Wells M = 32 vs M = 512 coefficient contamination at F = 1: −144.6 dB rms, max |Δc| 3.0e−8 (RESEARCH §2.10 expected ≤ −40 dB). Projection time (scratch, −O3): 0.3 ms at M = 32, 0.7–0.9 ms at M = 48.

## Scheduler rows (`--gate scheduler` (b)–(h))

- (b) Terrain Freq 4 → published key F = 2.00, coefficients memcmp-identical to the F = 2 set.
- (c) ModWheel → OscA Terrain Freq (amount 0.5), CC1 0 → 127: `generation 1 -> 2 after 59 ms` (57–59 ms across runs; poll 50 ms + a 0.3 ms job), key F 1.000 → 2.000.
- (d) LFO1 → OscA Terrain Mod X: Bandlimited renders bit-identical (max|Δ| 0); the same route in 2× moves the render by 6.1e−1.
- (e) `chebApproximate`: Ellipse / fb 0 = 0, Feedback 0.3 = 1, Superellipse = 1, Feedback routed = 1.
- (f) `prepareToPlay` ×2 + `setStateInformation` under pump: `publishCount = 0`.
- (g) async (timer + pool, 67 ms) set == sync set: keys equal, coefficients memcmp-identical; counters `completed 1 cancelled 0 superseded 0 dropped 0 inFlightMax 1`.
- (h) `chebPartialsAtC4`: Ellipse 16, Epitrochoid 7 80 (= D_max 10 × K 8 at 48 k).
- `clenshaw`: 80.5 / 84.8 / 90.1 / 104.1 ns per dependency-carried evaluation across runs.

## Storm (`--gate storm`; Bandlimited both oscillators, block 480, unity gain, tap, `pump (10)` between blocks)

```
[storm] 16 notes: 127 swaps seen, 225 ModX steps, allocations 0 [+5 foreign-thread, NOT counted], plateau step 0.3272, outside-window step 0.3778 (ratio 1.15); after drain: liveCount 2, retired 0, live jobs 0
[storm] scheduler counters: completed 128 cancelled 0 superseded 26 dropped 0 inFlightMax A 1 B 0
[storm] 114 frees observed, every one >= 2 block generations after its publish (min gap 7, need >= 2)
[storm] 1 notes: 97 swaps seen, ... plateau step 0.0380, outside-window step 0.0388 (ratio 1.02)
```

The click metric runs on a single C4 note as well because a one-voice swap click would hide under a 16-voice sum (deviation 5). `cancelled` is 0 by physics — a 0.3 ms Chebyshev job is never caught in flight by a 50 ms poll — so the plan's "`jobsCancelled > 0`" is gated as superseded keys + cancelled jobs (26 / 5), the quantity the 50 ms cadence actually produces (deviation 4); the in-flight cancellation path is exercised by the image side (`import` (g): a Blur change during a job).

## Import (`--gate import`), H10, H11, H8 rows

- (a) 512² hard-edged bytes: `returns true`, Terrain = Imported renders rms 0.1015 (fit 99.7 %), differs from Sine Product (max|Δ| 0.32); slot carries name + SHA-256.
- (b) Blur 0 / .25 / .5 / .75 / 1 on the 1024² checker: centroids 9183 / 416 / 262 / 262 / 262 Hz, one publish per value (imageGeneration 5).
- (c) Mirror vs Window at r = 1: centroids 279 / 498 Hz (Δ 44 %), max partial move 39.5 dB.
- (d) **Timing, 1024² at Blur 1 (box width 65 px per pass): decode 3.0 ms, blur + projection + view 23.5 ms, sync-path import 24.7 ms (best of 3) — 26.5 ms total vs the 100 ms budget** (ARCH's "≈ 10–20 ms" → measured).
- (e) async import pending during a 1 s render with `pump (5)` between blocks: `publishCount` changed inside a `processBlock` on 0 of 100 blocks, between blocks once.
- (f) `importTerrainFile (missing)` → false, `imagePtr` null, no revision; 64 bytes of 0x5A → false.
- (g) async import through the timer + pool published after 67 ms; two Blur changes: `completed 2 cancelled 0 superseded 1`.
- **H11:** h ≥ 32 re h1 — Mirror Blur 0.2 **−62.8 dB**, Window Blur 0.2 **−88.7 dB**, Mirror Blur 0 **−59.2 dB**, Window Blur 0 **−58.2 dB**; HarnessWrap (periodic tiling) control **−24.4 dB** at Blur 0 and 0.2 (fails as required). Bandlimited + image (embedded set, Ellipse, A4 exact-cycle): nonharm/max **−117.8 dB**, h ≤ 14, fit 99.7 % (`chebFit` mirrors `imageFit`).
- **H10:** A = B = `6d8a1f1be82ba711a1ce1016ed63fb0117f4170cbae34d99c881fd43da35a297`; Sine Product `f567c150…9927` differs.
- **H8 rows:** terrain / orbit 0 allocations (60 changes, 8 notes); Quality 0 (100 changes, 16 notes); storm 0 across 127 swaps; image row 0 with two imports published under 16 held notes (+37 foreign-thread: the pool + `callAsync`), `TerrainImage::liveCount 2, ChebyshevSet::liveCount 2`.

## Round A rows re-verified (Decision 45)

H1 unchanged (every row ≤ −119 dB) + the Chebyshev identity row **−128.3 dB**; H2 66 / 66 at 2× **and 66 / 66 in Bandlimited** (worst h1-max −2.3 dB, Radial Rings × Ellipse); H9 64 / 256 / 1024 identical (0.0) and the Bandlimited row **0.0**; H3 2376 grid points, worst pre-blocker |y| 0.8905, worst |DC| 9.6e−4; H4 5 / 5 / 2 and 13 / 12 / 5 as amended; H5 worst ratio 1.08; `crossfade` 0.0380 / 0.0261 vs plateau 0.0380 (byte-identical Quality path); `latency` unchanged; `saturation`, `decimator` (−94.0 dB), `tuning`, `smoke` green.

## Deviations from the plan (all documented in code comments)

1. **The Chebyshev value is not clamped to [−1, 1]** (`TerrainOscillator::scan`). The first Bandlimited run clamped it and the H6 rows for Radial Rings / Ridged Cosines at A4–A6 came in at −48 to −62 dB with harmonics above D_max·K: a truncated series overshoots slightly and a clamp is a hard nonlinearity that re-creates exactly what the taper removed. Downstream stays bounded (feedback ± 0.5, saturation tanh, DC blocker linear); the largest tapped |y| over the 144 rows is 0.673.
2. **Mitsuhashi fits 98.3 / 87.6 %, not "≈ 100 %"** — the `tri()` wrap is piecewise-linear, so a degree-16 polynomial cannot fit it; the scheduler gate compares against the re-measured figures (recorded), the other five terrains against ARCH one-sided (≥ ARCH − 2 %).
3. **`TerrainImage.h/.cpp` landed in the Phase 2.4 commit**, not 2.5: the oscillator's `Imported` branch needs the complete type. The import API, the scheduler's image side and the gates are the Phase 2.5 commit.
4. **Storm cancellation counted as superseded keys + cancelled jobs** (see Storm) and the click metric measured on a single note as well as under 16 held notes (5).
5. (see 4)
6. **Scheduler debounce is throttle + trailing** (submit on change at most once per period, a differing key cancels an in-flight job, intermediate keys superseded), not a pure trailing "stable for 50 ms": a pure trailing debounce never publishes under a continuous 20 ms sweep, which contradicts CONTEXT D4's "20 Hz swap cadence".
7. **Root `.gitignore` gained `build-asan/`** (Decision 43's precondition); included in the Phase 2.5 commit pathspec.
8. **`JUCE_MODAL_LOOPS_PERMITTED=1`** on the harness target: JUCE 8 compiles `runDispatchLoopUntil` out without it (the plugin target is untouched).
9. **H6 Bandlimited quiet / muted tiers** (finding 1): three tiers by the strongest harmonic's amplitude rather than one relative threshold.
10. **ASan not runnable here** (finding 3).
11. **`--png PATH`** is honoured only with `--print-only` (selects `Imported` on oscillator A after the import).

## Corrections carried (Decision 46 + new)

"Zero leaks under ASan" → instance counters (and ASan itself unavailable on this toolchain); "one-block crossfade" → 64 samples; `ChebyshevScheduler` → `TerrainScheduler`; H6 Bandlimited on A-notes; Cosine Wells fit at πF; nominal K for approximate orbits; separable Gaussian → 3-pass box; "≈ 10–20 ms" import → 26.5 ms at 1024² / Blur 1 (3 ms decode); `--all` includes every Round B gate except `export`; **new:** Mitsuhashi's fit; the Chebyshev output is unclamped; Bandlimited mutes even terrains at A6 with K ≥ 6 (and K = 4 for Radial Rings); H7 Bandlimited ≈ 12 % (≈ the 2× cost, not less).

## Tasks executed (PLAN.md 1–16)

| # | Task | Result |
|---|---|---|
| 1 | `Retirable.h`, `orbitKNominal`, processor include | `grep -rn 'struct Retirable' Source/` = 1 |
| 2 | `ChebyshevSet.h` | triangle, `ChebKey` (1/1024 quantised, exact compare), `clenshaw2D`, shared `chebDiagonalCutoff` / `chebDMax` / `chebTaperWeight`; scratch δ check worst 2.5e−7 |
| 3 | `ChebyshevProjector.h/.cpp` | nodes 32 / 48, separable contraction, fit %; scratch fit table above |
| 4 | `TerrainScheduler.h/.cpp` | one class, both job kinds, `runOnceSynchronously`, `publishForHarness`, counters |
| 5 | processor | pointers, readouts, `harnessChebyshevBypass`, `harnessEdgeOverride`, scheduler as the last member + `shutdown()` first in the dtor, `isInsideProcessBlockOnThisThread`, `updateOscillatorAssignments` publishes, `getBlockGeneration` / `getRetiredCount` |
| 6 | oscillator | `chebW[2]`, taper rebuild, Set crossfade (shadow evaluator + copy), image branch; DSP-05 grep clean |
| 7 | voice | `setPublished`, `updateBlockRate (target, max (target, current))`, edge override |
| 8 | harness 2.4 | pump / sync, H6 BL, `scheduler`, `storm`, `clenshaw`, H1 / H2 / H9 / H7 / H8 rows; `grep -c runDispatchLoopUntil main.cpp` = 1 |
| 9 | 2.4 build / parity / pluginval / commit | params diff empty; pluginval ×2 SUCCESS; commit path-scoped |
| 10 | `TerrainImage.h/.cpp` | decode ≤ 1024², 3-pass box blur, embedded set (M = 64, 4 × 4 node box), 64² view, `sample` |
| 11 | import API + scheduler image side | `importTerrainImage` / `importTerrainFile` / `importSlot` / `importRevision`, 150 ms throttle, `publishImage` (readout mirror) |
| 12 | oscillator / voice image path | `setImage`, `Imported` → `image->sample`, embedded set in Bandlimited |
| 13 | harness 2.5 + ASan + README | `import`, H11, H10, H8 image row, `--png`; ASan build ok / run hangs (finding 3); README H1–H11 |
| 14 | install + validate | see Outcome |
| 15 | docs | CHANGELOG (Round B + Known limits + ASan status), comments refreshed, PLUGINS.md, STATUS, this file |
| 16 | commit | Phase 2.5, path-scoped, no tag |

## Open for verify

1. Finding 1 (muted rows) — accept as the CHANGELOG limit or re-open the taper contract.
2. Finding 2 (H7 Bandlimited ≈ 12 %) — accept as reported or ask for the vectorised-basis evaluator.
3. Finding 3 (ASan) — accept the instance-counter verdict; re-run ASan on a toolchain where the runtime initialises.
4. The soft-knee / default-Track question from Round A stays an optional discuss item.
