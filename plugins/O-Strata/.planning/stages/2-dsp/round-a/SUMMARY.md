# Stage 2 — DSP (live wave-terrain oscillator), Round A: SUMMARY

**Plugin:** O-Strata · **Stage:** 2 of 4 (DSP) · **Round:** A = Phases 2.1 + 2.2 + 2.3 (CONTEXT D1) · **Phase:** execute ✓
**Date:** 2026-09-11 · **Mode:** manual · **Branch:** `main` (three path-scoped commits under `plugins/O-Strata` (+ `PLUGINS.md` on the third))
**Inputs:** `CONTEXT.md` (D1–D4), `RESEARCH.md` (§2.1–2.12, §3, §4, §5), `PLAN.md` (28 tasks / 7 waves, Decisions 1–24), `parameter-spec.md` v2 (locked, untouched).
**Commits:** `4640a1a7` Phase 2.1 · `45b1fb40` Phase 2.2 · (this commit) Phase 2.3. Baseline `378da673` (Source identical to `efae0bfc`).
**Template:** summary-complex (28 tasks, 20 files, harness with 85 checks).

Every number below was measured in this session with the committed harness (`tests/render-harness/main.cpp`), Release build, Apple M4 Max, Apple clang 17 (memory `pattern_review_recomputes_instead_of_measuring`). Log lines quoted verbatim where they matter.

---

## Outcome

Both oscillators are live wave-terrain oscillators end to end on the DSP side, at 2× by default. `TerrainOscillator` replaced the wavetable oscillator in place (11 orbits, 6 analytic terrains under the πF convention, θ parity with the inherited warps to −120 dB, unison capped at 4); the 20 new mod destinations plus Orbit Size are per-sample through 5 ms ramps shared across voices; pitch-tracked terrain frequency, bounded trajectory feedback, 5 Hz output DC blocker, exact-identity saturation, per-partial 2× / 4× polyphase-IIR halfband decimation with a 64-sample Quality crossfade, and the constant +1 latency report are in. Every wavetable class and table-plumbing member is deleted, the two native functions are stubbed, the reaper is type-erased. The offline render harness `O-Strata-render-test` runs H1–H9 + tuning + smoke + centroids + saturation + decimator + crossfade + latency in one `--gate all`:

```
GATES FAILED — 85 check(s), 2 failure(s), 70.8 s          (run from /, and identically from the repo root)
```

The two failures are the **H4 partial-count rows** — a design finding, not a defect, recorded below as the round's outcome for verify. **H6 needed no fallback rung** (198 / 198 rows ≤ −60 dB at 2×) and **H7 needed no ladder rung** (oscillator delta 4.4 %, limit 12 %), so neither blocking question in PLAN.md arose. COMPAT-01 regression: pluginval strictness 10 **SUCCESS** on VST3 and AU, `auval -v aumu OuSt OuDv` **AU VALIDATION SUCCEEDED**. `params.tsv` diff: **empty** (no parameter, range, list or default changed).

## H6 / H7 outcome (Round B's first input — CONTEXT D1)

| Gate | Result | Fallback applied |
|---|---|---|
| **H6** aliasing at 2× | 198 / 198 rows (66 pairs × A2 / A4 / A6) ≤ −60 dB nonharm/max; **worst −70.7 dB** (Mitsuhashi × Epitrochoid 7, A2). 4× reported: worst −72.6 dB (Ridged Cosines × Butterfly, A6). 1× analytic reported: worst −30.1 dB. Rate rows: Sine Product × Ellipse −124.1 / −114.5 / −103.0 (48 k), −123.7 / −113.9 / −102.2 (44.1 k), −124.6 / −118.8 / −108.8 (96 k); Ridged × Epitrochoid 7 −74.7 / −103.7 / −77.7, −72.5 / −103.8 / −78.0, −87.3 / −101.7 / −74.9 — within 3 dB or below −60. | **none** (no shallower lobes, no `Auto` Quality, no harmonic-budget clamp) |
| **H7** CPU | `total = 11.72 %  baseline (kernel bypass) = 7.28 %  delta = 4.44 %` (final `--all` run); standalone runs 4.37 % (shared ramps) and **4.82 % (per-voice ramps, Phase 2.2 tree + OS loop, measured before Task 21)** — the shared-row saving is ≈ 0.45 % delta / ≈ 0.9 % total. Rows: unison 4 / 2× delta 17.5 % (total 26.5), 4× delta 7.3 % (total 15.1), Bandlimited-as-1× delta 2.1 % (total 9.2). | **none** (the two planned savings — shared ramps, fb = 0 block skip — are in; float phase path, dirty-flag smoothing and the 15 % rung were not needed) |

The ARCH CPU model (≈ 15 % before savings, ≈ 11.4 % after) over-estimated by ≈ 3×: the measured per-partial cost at the default patch is ≈ 30 ns, not ≈ 100 ns.

## Deviations from the plan (all documented in code comments)

1. **Cosine Wells uses πF, not ARCH Core 3's 2πF** (`Terrains.h`). The 2πF form puts two cycles across [−1, 1] at F = 1, contradicting the stated convention ("F = 1 ⇒ one full sine cycle across [−1, 1]"), and failed H2 on 7 of 11 orbits inside the harness (all 8 neighbouring centres negative too) and on 10 of 11 in a pure-math scan; under πF h1 is the strongest partial on all 11. Radial Rings keeps 2πF·ρ (ρ ∈ [0, 1] on the half-domain is one cycle — consistent). Round B note: RESEARCH §2.10's Chebyshev quadrature figures for Wells were computed at 2πF (M ≥ 40 at F = 2); at πF the terrain is smoother, so M = 32 / 48 is conservative.
2. **Ramp note-on snap keyed on the amp envelope, then removed.** `juce::Synthesiser::startVoice` assigns `currentlyPlayingNote` before calling `startNote`, so the inherited `wasActive` is always true (O-Prism quirk, left as is: with glide Off the phase reset still fires; in Legato mode `glideIn` is therefore always true — not touched, recorded for the O-Prism CODE_REVIEW). The Phase 2.1 per-voice ramp snap used `! ampEnvelope.isActive()` instead; H1 went from −71 dB (a 5 ms Orbit Size glide 0 → 1 at every note-on) to −128 dB. Phase 2.3's processor rows made the snap moot.
3. **H5 gates on the excess step at the automation instant** (max step in the 10 ms after the step ÷ the larger adjacent 100 ms plateau ≤ 1.5), at unity gain, A or B alone. The literal "max |y[n] − y[n−1]| ≤ 0.1 on the rendered output" is printed but cannot discriminate: at the default gain staging (level 0.8 × pan 0.707 × sustain 0.7 × velocity 0.8 × master 0.8 × mix 0.5) a raw 0.6 zipper shrinks to 0.05 — the ramp-0 control PASSED (0.048) — while at unity gain Terrain Freq = 8 has natural waveform steps of 0.30. With the ratio the control fails as required (7.68) and every row sits at 1.00–1.08.
4. **Centroid gate reads a five-point sweep and accepts "centroid ≥ 5 % OR some partial above −40 dB moves ≥ 3 dB".** The plan's 0 → 1 endpoints are degenerate for the phase-type shape inputs (Sine Product Mod X: sign flip; Ridged Cosines Mod X: 0 ≡ 1; Radial Rings Mod Y spans a full turn: 0 ≡ 0.5 ≡ 1 up to sign). Ridged Cosines Mod X moves the centroid only 3.0 % across the whole range while its partials move by 24.6 dB — the centroid alone is the wrong summary for a ridge-phase control.
5. **H3 |y| ≤ 1 is measured on the pre-blocker scan sum** (new harness switch `harnessDcBlockerBypass`). The 5 Hz blocker is LTI with time-domain gain bound 2 (impulse-response L1 norm), so a post-blocker bound of 1 is not a property of the oscillator; ARCH Core 4's boundedness argument (and its "max |y| = 0.998") is about the scan value. Post-blocker peak reported: 1.40 (Radial Rings × Limaçon, fb 1, Damp 0.5, C2).
6. **H3 DC read over the last 250 ms of a 1 s render, Hann-windowed.** The plain mean over 0.25 s of a 0.5 s render was contaminated by the C2 fundamental (non-integer cycles) and by the Damp 1 attractor transient, which lasts ≈ 0.5 s through the blocker (2 s traces: DC → 0.000 after 0.5 s in every failing case; one bounded chaotic hop of 0.054 at 1.75 s in Saddle × Butterfly fb 0.75 C2 — spatial compression by design, not runaway). Bounded hops in [0.5, 0.75] s are counted and printed (0 in the final run).
7. **Nyquist-hunting control at 1× and swept.** The plan's row (Ridged × Epitrochoid 5, C4, F = 1) shows no hunt in either form (−94.5 vs −99.3 dB); the loop gain scales with the terrain slope, so the control sweeps 6 terrains × F {1, 4, 8} × {C4, C6} and finds a **103.4 dB** rise (Sine Product × Epi5, C6, F = 8) with the single-sample form at 1×, while the two-sample average keeps fs/2 below the strongest bin everywhere (worst −8.3 dB). At 2× the decimator removes the oversampled-Nyquist hunt (rise 4.2 dB) — the halfband is a second line of defence, the average is the first.
8. **H4 negative control swept** ({Ellipse, Epitrochoid 7} × {C6, C8}): the plan's Ellipse / C6 / F = 8 row never reaches Nyquist at 1× (harmonics top out ≈ 17 kHz; rise −0.9 dB) and would be vacuous at 2× as well; Ellipse / C8 rises 39.6 dB, Epitrochoid 7 / C6 31.2 dB.
9. **Bend warp `std::pow` per sample** stays (inherited O-Prism `applyWarp`, copied verbatim per ARCH Core 1); marked as a documented DSP-05 exemption inline. The DSP-05 grep on `TerrainOscillator.cpp`, `Orbits.h`, `Terrains.h`, `HalfbandDecimator.h` is otherwise clean (hits only on lines marked `prepare-time only` / `block-rate (DSP-05 exemption: …)`).
10. **`--gate export` is not part of `--all`** (it writes 198 files); `tests/render-harness/golden/round-a-grid.sha256` is tracked, the WAVs are ignored.

## Round outcome for verify — H4 (DSP-01)

```
  [H4] F = 1, Track 1: h1..h6 levels (dB re max) C2: 0 -1 -16 -24 -37 -54 C4: 0 -1 -16 -24 -36 -55 C6: 0 -1 -42 -50 -87 -107
FAIL  [H4] F = 1, Track 1: partials > -40 dB at C2 / C4 / C6 = 5 / 5 / 2 (spread 3, need <= 2)
  [H4 diag] F = 1, Track 0.0: partials > -40 dB at C2 / C4 / C6 = 5 / 5 / 5
  [H4 diag] F = 1, Track 0.5: partials > -40 dB at C2 / C4 / C6 = 5 / 5 / 4
  [H4] F = 4, Track 1: h1..h6 levels (dB re max) C2: -7 -24 0 -16 -25 -5 C4: -7 -23 -0 -16 -25 -6 C6: 0 -1 -16 -24 -36 -55
FAIL  [H4] F = 4, Track 1: partials > -40 dB at C2 / C4 / C6 = 13 / 12 / 5 (spread 8, need <= 2)
  [H4 diag] F = 4, Track 0.0: partials > -40 dB at C2 / C4 / C6 = 13 / 12 / 12
  [H4 diag] F = 4, Track 0.5: partials > -40 dB at C2 / C4 / C6 = 13 / 12 / 7
```

The law is exactly ARCH Decision 2 (F_eff = F · min(1, C4 / f_note)^Track from the glide target). Two facts follow from it: (a) C2 and C4 share F_eff (scaling is downward only), so at F = 4 they are identically rich (13 / 12) and C6 at F_eff = 1 cannot be within 2 — the F = 4 criterion is unsatisfiable under the contracted law; (b) at F = 1 C6 lands at F_eff = 0.25 where the terrain is near-linear over the orbit: h3 sits at −42 dB (vs −16 dB at C4), 2 dB under the −40 dB counting threshold — ARCH's estimate "C6 ≈ 4 → passes" measured 2. Track 0.5 gives 5 / 5 / 4 and 13 / 12 / 7. Options for verify / Round B (not taken here — `osc?TerTrack` default 1.0 is spec-locked): change the default Track, soften the knee (ARCH's own listed fallback), or re-word DSP-01's acceptance to the count *not growing* above C4 (which holds: 5 → 2, 12 → 5). Everything else the law promises is measured: the Track 0 / F = 8 control raises non-harmonic energy 39.6 dB.

## Tasks executed (PLAN.md 1–28)

| # | Task | Result |
|---|---|---|
| 1 | reference headers | `theta_reference.h` (warp / Sync / unison / LCG copied from `WavetableOscillator.cpp` at `efae0bfc`, + DC blocker, two-oscillator FM path), `spectrum.h` (FFT, `analyseExactCycle`, windowed helpers; provenance comments); `grep kernels.h` = 0 |
| 2 | `dsp/Orbits.h` | 11 kinds, `orbitK`, `OrbitScratch`, 64-point norm scan, 33 × 129 Superellipse LUT under `call_once`; scratch sweep: every orbit max r = 1.000 after the norm (Butterfly 1.001 at 256 points), LUT n = 2 max \|r − 1\| = 4.5e−4 |
| 3 | `dsp/Terrains.h` | 6 terrains + `Imported` (0) + `HarnessIdentityX` (100); scratch 64² grid: all within [−1, 1], non-constant; **Cosine Wells at πF** (deviation 1) |
| 4 | `dsp/TerrainOscillator.h/.cpp` | interface = the old one minus the table setter; `scan` = orbit → affine → feedback → clamp → terrain → clamp → saturation; DC blocker; `CycleCapture` writes (display voice, partial 0); harness switches; `setUnison` cached on (count, detune, width) |
| 5 | `StrataVoice` | member swap; 34 cached pointers; block-rate feed incl. `updateBlockRate (glide target × pitch ratio)`; per-sample `feedOscillator` mapping (Aspect × 0.9, Rotation × 180°, Terrain Freq `exp2f (log2 base + 2·off)` clamped [0.25, 8]); ModWheel / Aftertouch via rows 22 / 23; pre-filter tap; `setVoiceIndex`, capture targets, seed |
| 6 | `ModulationMatrix::isDestinationRouted` | added (cached slot state, no allocation) |
| 7 | processor | seed atomic + 8 harness switches + `harnessTerrainOverride[2]`; `CycleCapture[2]`; `lastPlayedNote`; tables / reaper types deleted; `Retirable` / `Retired` / `retire()` (message thread, asserted); `updateOscillatorAssignments()` empty; editor stubs `"{}"` / `"[]"`; `toJsonFloatArray` removed |
| 8 | static_asserts | `TerrainKind::CosineWells == 5 / Imported == 6`, `OrbitKind::Squarcle == 10`, `Quality::X4 == 2 / EdgeMode::Window == 1` beside the `choice (…)` calls; comments refreshed; params.tsv diff empty |
| 9 | CMake + ignores + fixture | `ouaricon_add_processor_console (… render-test)`, `STRATA_FIXTURES_DIR` / `STRATA_EXPORTS_DIR`, `STRATA_HARNESS_ASAN` option; `tests/.gitignore`, `smoke/.gitignore` (`strata-smoke`, `main.o` no longer in `git status`); `just-major.scl` copied; `git check-ignore -v tests/exports/x.wav` → `tests/.gitignore:3` |
| 10 | deletion | five files `git rm`; `grep -rn 'Wavetable\|WaveShape\|kNumMipmapLevels' Source/ CMakeLists.txt --exclude-dir=ui` = 0 (banners renamed to "Microtonal Wave-Terrain Synthesizer"; the untouched page keeps its `WavetableDisplay` class until Phase 3.1) |
| 11 | build / parity / pluginval (2.1) | param-dump diff empty; `build-and-install.sh` OK; pluginval 10 SUCCESS ×2 |
| 12 | harness skeleton + 2.1 gates | see the gate table; 62 / 62 at the 2.1 commit |
| 13 | pitch tracking | `rTrack = pow (min (1, 261.6256 / noteHz), track)` in `updateBlockRate`; `F_eff = terrainFreq · rTrack` |
| 14 | trajectory feedback | per partial: `avg = ½ (y1 + y2)` (single-sample under the harness switch), `d = aEff·d + (1 − aEff)·avg`, scrub, ±0.5, `p += fb·d·(cos ρ, sin ρ)`; `aEff = pow (1 − 2^(−1 − 9·damp), 48000 / (fs·OS))` |
| 15 | saturation plumbing | Decision 13 as written (Task 4); `harnessSaturationBypass` for the memcmp row |
| 16 | 2.2 gates | H3, H4, DSP-07 (see table); 75 checks / 2 failures at the 2.2 commit |
| 17 | commit 2.2 | `45b1fb40` |
| 18 | `dsp/HalfbandDecimator.h` | coefficients 2×: direct 0.074723 0.488018 0.899166, delayed 0.261946 0.702383; 4×: direct 0.098862 0.743755, delayed 0.360364 (= RESEARCH §2.2 to 6 decimals); **L2 = 1.2589, L4 = 1.7432** base samples; `--gate decimator`: 30 kHz → 18 kHz alias **−94.0 dB** re 1 kHz after 2× decimation (need ≤ −68) |
| 19 | OS loop + crossfade | `subSample` (inherited branches, `inc = phaseIncrement / OS`), `decimate` (hb4 ×2 → hb2), two decimator sets per partial, shadow old path for 64 samples, instant switch while `fresh`; kernel bypass skips scan + decimator |
| 20 | latency +1 | both sites; `--gate latency`: bypassed = **1**, distortion on = **4** (= 3 + 1) for all 9 Quality pairs; the timer follow-up uses the same expression (inspected — no message loop in Round A) |
| 21 | shared ramp rows | processor `baseRamps[24]` + `rampBuffers` (24 × samplesPerBlock) filled before `synthesiser.renderNextBlock` (`isSmoothing() ? getNextValue : FloatVectorOperations::fill`), rows 6 / 17 log2, `jassert + jmin` capacity guard; voices read `row[k][jmin (sample, cap − 1)]`; per-voice ramps, `rampTarget`, the note-on snap removed |
| 22 | fb = 0 skip | `setFeedbackActive (rowMax (row 9 / 20) > 0 ‖ isDestinationRouted (…))` per block; H3 memcmp still 66 / 66 |
| 23 | export grid | 198 WAVs (24-bit, 2 s, defaults, 2×) → `tests/exports/`; `golden/round-a-grid.sha256` written (tracked) |
| 24 | H7 | see outcome table; per-voice figure taken before Task 21 as planned |
| 25 | H6, H8 across Quality, crossfade, latency, rate rows | see gate table |
| 26 | install + validate | `build-and-install.sh O-Strata` (dual-variant sweep, no orphan); pluginval strictness 10 SUCCESS VST3 + AU; `auval -v aumu OuSt OuDv` SUCCEEDED |
| 27 | docs | CHANGELOG (Stage 2 Round A entry + latency line), banners / stale comments, PLUGINS.md row, STATUS.md, this SUMMARY, harness README |
| 28 | commit 2.3 | this commit (`git commit -- plugins/O-Strata PLUGINS.md`, no tag) |

## Gate results (final `--gate all`, from `/`, 70.8 s)

| Gate | Measured | Threshold | Verdict |
|---|---|---|---|
| index | source 1 / 7 / 9, dest 10 / 23 / 28 / 31 match the live lists | — | PASS |
| `smoke` [1]–[6] | 30 PASS lines; [4] inverted: LFO1 → Terrain Freq max\|Δ\| = 5.46e−1, Pitch control 6.48e−1, unrouted renders identical; [5] skipped (no `--with-disk`) | [4] > 1e−3 | PASS |
| `tuning` | Scala just-major and 31-EDO, keys 48 / 55 / 60 / 64 / 67: \|Δ\| = 0.0000 cent on all 10 rows | ≤ 0.5 cent | PASS |
| H1 | Off / Sync / Bend / FM / Window × unison {1, 4}: −128.3 / −120.6 / −126.5 / −121.6 / −120.3 / −126.7 / −119.8 / −125.2 / −119.7 / −119.5 dB; C4 pitch 261.6256 Hz (−0.000 cent); Bend 0.6 control −22.9 dB | ≤ −80 dB; ± 1 cent; control fails | PASS |
| H2 | **66 / 66** pairs, 100 % windows each; worst h1-max −2.3 dB (Radial Rings × Ellipse); `presets: 1 (Init)` — Init passes; centred-circle control −109.1 dB, 0 % windows | ≥ 95 % windows; control fails | PASS |
| centroids | 12 / 12 shape inputs and 10 / 10 Orbit Mods pass (Ridged Cosines Mod X: centroid 3.0 %, partial move 24.6 dB); Orbit Size centroids 263 277 308 353 410 480 566 653 Hz (monotone) | ≥ 5 % or ≥ 3 dB; monotone | PASS |
| H3 | 2376 grid points × 2 passes, 25.8 s: finite; pre-blocker \|y\| worst **0.8905**; post-blocker 1.3985 (reported); \|DC\| worst **9.56e−4**; bounded hops 0; memcmp **66 / 66**; average form fs/2 never strongest (worst −8.3 dB); single-sample rise **103.4 dB** at 1× | finite; ≤ 1; < 1e−3; 66; ≥ 20 dB | PASS |
| H4 | counts 5 / 5 / 2 (F = 1), 13 / 12 / 5 (F = 4); control rise 39.6 dB (Ellipse C8) | spread ≤ 2; ≥ 20 dB | **FAIL ×2** (design finding above); control PASS |
| `saturation` | Sat 0 byte-identical to the bypassed branch; partials 5 → 16; \|y\| max 0.4650 | identical; more | PASS |
| H5 | 22 rows excess ratio 1.00–1.08 (abs max step 0.30 at unity gain, Terrain Freq); ramp-0 control 7.68; ModWheel route 1.00; sidebands h1 +0.3 / +0.4 dB, h2 −4.4 / −4.3 dB rel. the carrier | ≤ 1.5; control fails; ≥ −40 dB | PASS |
| H8 | 0 allocations across 60 terrain / orbit changes (8 held notes, 3 s); 0 across 100 Quality / terrain / orbit changes (16 held notes, 5 s); foreign tally 0 in both | 0 | PASS |
| H9 | 64 / 256 / 1024 pairwise max\|Δ\| = **0.000e+00**; note-on at sample 37: 0.000e+00 | ≤ 1e−5 | PASS |
| `decimator` | L2 1.259, L4 1.743; alias −94.0 dB | 1 < L2 < L4 ≤ 2; ≤ −68 | PASS |
| H6 | 198 / 198 at 2×, worst −70.7 dB; rate rows within 3 dB | ≤ −60 dB; 3 dB | PASS |
| `crossfade` | max step inside the window 0.0261, outside 0.0380, plateau 0.0380 | ≤ 1.5 × plateau | PASS |
| `latency` | bypassed 1, on 4, all 9 pairs | 1 / distortion + 1 | PASS |
| H7 | delta 4.44 % (total 11.72, baseline 7.28) | ≤ 12 % | PASS |

**H8 coverage statement (plan Decision 4, verbatim):** covered = every new-expression on the audio thread; not covered = HeapBlock / malloc paths (`AudioBuffer`, `Array`, `MidiBuffer`, `ReferenceCountedArray`, `MemoryBlock`) and the `AsyncUpdater` post path — grep + inspection. `MidiBuffer::ensureSize (256)` before arming, one warm-up block, tuning parameters untouched while armed.

**Harness hygiene:** `grep -c runDispatchLoopUntil main.cpp` = 0; every README command carries the absolute artefact path and ran from `/`; `tests/exports/` and `golden/*.wav` ignored, `golden/round-a-grid.sha256` tracked; the Stage 1 `strata-smoke` binary is ignored (its `--smoke` role is folded into the harness).

## Files

**Created:** `Source/dsp/Orbits.h`, `Terrains.h`, `TerrainOscillator.h/.cpp`, `HalfbandDecimator.h`; `tests/render-harness/main.cpp`, `README.md`, `reference/theta_reference.h`, `reference/spectrum.h`, `fixtures/test-tunings/just-major.scl`, `golden/round-a-grid.sha256`; `tests/.gitignore`; `.planning/stages/1-foundation/smoke/.gitignore`; this file.
**Modified:** `Source/StrataVoice.h/.cpp`, `PluginProcessor.h/.cpp`, `PluginEditor.cpp` (stub bodies + helper removal), `dsp/ModulationMatrix.h/.cpp`, `dsp/GlideProcessor.h` (`getTargetFrequency`), 22 banner lines, `CMakeLists.txt`, `CHANGELOG.md`, `.planning/STATUS.md`, `PLUGINS.md`.
**Deleted:** `Source/dsp/WavetableOscillator.h/.cpp`, `WavetableData.h`, `WavetableGenerator.h/.cpp`.
**Untouched:** `parameter-spec.md`, `BRIEF.md`, `research/ARCHITECTURE.md`, `ROADMAP.md`, `Source/ui/public/**`, `plugins/O-Prism/**`, `research/**`.

## Corrections carried forward (RESEARCH §3 + new)

- RESEARCH §3.3–3.4 confirmed: 5 + 3 sections, L2 1.259 / L4 1.743, report +1.
- RESEARCH §3.5 amended: the shared-ramp saving measured ≈ 0.45 % delta at the H7 patch (per-voice 4.82 % → shared 4.37 %), i.e. ≈ 3 ns per partial-sample — RESEARCH's "3–4 ns" estimate was right, ARCH's "−12 ns" was the ramping figure.
- New: ARCH CPU model ≈ 3× pessimistic (default patch ≈ 30 ns per partial-sample end to end, incl. decimator and ramps).
- New: ARCH Core 3 Cosine Wells 2πF → πF (deviation 1).
- New: ARCH H3 "|y| ≤ 1 at the oscillator output" is the pre-blocker bound; the post-blocker peak can reach 1.4.
- New: ARCH H3 Nyquist control and H4 control rows as written are vacuous (deviations 7 / 8); the sweeps in the harness are the working forms.
- New: ARCH Algorithm "Pitch tracking" — "C6 (F_eff = 0.25) ≈ 4 → passes" measured 2 partials; F = 4 unsatisfiable under Decision 2 (H4 outcome).
- New: `juce::Synthesiser` sets `currentlyPlayingNote` before `startNote` → O-Prism's `wasActive` is always true (deviation 2).

## Open for verify

1. **H4 / DSP-01** — the outcome section above; verify decides between accepting the measured behaviour with a re-worded acceptance, or queueing a law / default change for Round B (spec touch).
2. The harness `--all` exits 2 until H4 is resolved one way or the other; every other check is green.
3. Round B inputs: Cosine Wells now πF (Chebyshev fit numbers to re-measure); Bandlimited in Round A is the analytic 1× path (H6 1× worst −30.1 dB shows why the real Bandlimited mode matters); the `harnessDcBlockerBypass` switch and the 1× Nyquist control are available for the Bandlimited rows.
