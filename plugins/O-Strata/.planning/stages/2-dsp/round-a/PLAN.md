# Stage 2 — DSP (live wave-terrain oscillator), Round A: PLAN

**Plugin:** O-Strata · **Stage:** 2 of 4 (DSP) · **Round:** A = ROADMAP Phases 2.1 + 2.2 + 2.3 (CONTEXT D1) · **Phase:** plan
**Date:** 2026-09-11 · **Mode:** manual · **Branch:** `main` (trunk-based; path-scoped commits under `plugins/O-Strata`)
**Inputs:** `stages/2-dsp/CONTEXT.md` (D1–D4, findings 1–9, constraints, amended criteria), `stages/2-dsp/RESEARCH.md` (§2.1–2.12 resolutions, §3 corrections 1–16, §4 decisions 1–10, §5 harness skeleton), `research/ARCHITECTURE.md` v2 (Core 1–5, 9, 10; Algorithms; Risks; Decisions 1–12; Harness H1–H9), `ROADMAP.md` v2 Phases 2.1–2.3, `REQUIREMENTS.md` v2.0.0 (PERF-02 as amended), `parameter-spec.md` v2 (locked — **the spec wins**; no parameter changes in this round).
**Baseline commit:** `efae0bfc` (docs-only on top of `e9686d4d`; `Source/` unchanged since the Stage 1 second pass verify).
**Line numbers** below are the fork's at `efae0bfc` and drift as edits land — always re-locate by the quoted token (memory `pattern_deferred_item_line_number_drifts_record_a_token`).
**Round B** (Phases 2.4 + 2.5: Bandlimited + scheduler, PNG path + consolidation) gets its own PLAN.md after this round's verify; this file moves to `stages/2-dsp/round-a/` at that point.

---

## Goal

Both oscillators become live wave-terrain oscillators end to end on the DSP side, at 2× by default: `TerrainOscillator` replaces `WavetableOscillator` in place (11 orbits, 6 analytic terrains under the πF convention, θ parity with the inherited Sync / Bend / Window / FM warps, unison capped at 4); the 20 new mod destinations plus Orbit Size are read per sample through 5 ms base-value ramps; pitch-tracked terrain frequency (F_eff = F_mod · min(1, C4/f_note)^track); bounded trajectory feedback (two-sample average + leaky integrator, clamps, NaN scrub, 5 Hz output DC blocker); per-oscillator per-partial 2× / 4× polyphase-IIR halfband decimation with the Quality switch crossfade and the constant +1 latency report; every wavetable class and table-plumbing member deleted; the two native functions stubbed; the reaper type-erased; and the offline render harness `O-Strata-render-test` running gates **H1–H9** plus `--smoke` exit-code green in one `--all` run.

**Requirements verified in Round A:** FUNC-01, FUNC-02, FUNC-03, FUNC-04, FUNC-05, FUNC-09, FUNC-10, DSP-01, DSP-03 (2× / 4× half), DSP-04 (clamp half), DSP-05, DSP-06, DSP-07, PERF-01, PERF-02 (D2: oscillator delta ≤ 12 %), QUAL-01 (2× half), QUAL-02; FUNC-06 (2× / 4× half). Bandlimited (DSP-02, the other halves of FUNC-06 / QUAL-01) and images (FUNC-07 / FUNC-08 / DSP-04 edge half) are Round B.

**Round A's verify is the decision point** (D1): the H6 / H7 outcomes — which fallbacks were applied, if any — are recorded in SUMMARY.md and are inputs to Round B's plan.

---

## Decisions resolved for the executor

These close RESEARCH §4's ten flags and the choices ARCHITECTURE / RESEARCH left open. The executor applies them without asking.

| # | Question | Decision |
|---|---|---|
| 1 | Latency report +1 or +2 (RESEARCH §4.1, §2.2: measured 1.26 at 2×, 1.74 at 4×) | **Constant +1**, as ARCH Decision 5 contracts. `HalfbandDecimator::prepare` computes L2 / L4 from the designed coefficients with the §2.2 formula and `jassert (L2 <= 2.0 && L4 <= 2.0)`; the harness `--print-only` prints both and SUMMARY records them. Both report sites (`setLatencySamples` in `prepareToPlay`, token `IN-04: distortion (oversampler) is the only latency source`, and the timer follow-up, token `wantedLatency`) become `distortionLatency + 1` — **unconditionally**, every Quality combination, Bandlimited included (a per-path report is impossible with automatable per-oscillator Quality). The `IN-04` comments are rewritten to say so. |
| 2 | H6 pitch rows (RESEARCH §4.2, §2.6) | **A2 / A4 / A6 (MIDI 45 / 69 / 93) at fs = 440·65536/600 = 48059.7333 Hz, N = 65536** (Option C1) → 150 / 600 / 2400 exact cycles. Rate-invariance rows: 44.1 k → fs = 440·65536/652, 96 k → 440·65536/300. Default tuning only (a4 440, stretch 1, 12-TET, Coarse / Fine 0, Unison 1, glide Off, no routes, no bend). ARCH / ROADMAP "C2 / C4 / C6" for H6 is a recorded correction (RESEARCH §3.9); **H4's** partial counts are windowed and stay on C2 / C4 / C6; **H1's** pitch check is ± 1 cent at C4 (windowed peak interpolation, not exact-cycle). The harness prints nonharm/fund **and** nonharm/max and gates on nonharm/max (ARCH H6 convention). |
| 3 | H1 / H6 signal tap (RESEARCH §4.3) | **Harness-only pre-filter oscillator tap**: a processor atomic `harnessPreFilterTap` (default false); when set, `StrataVoice::renderNextBlock` writes the summed oscillator L/R (after level / pan / mix, before the sub / noise add and the filters) instead of the filtered voice output. The tap is placed **after** the per-oscillator DC blocker (the blocker is part of the oscillator). H1's analytic reference therefore applies the same one-pole DC blocker (R = 1 − 2π·5/fs) to cos θ before comparison (RESEARCH §2.6 blocker i). Everything downstream (filters LP24 @ 20 kHz, FX at mix 0) is unchanged in production. |
| 4 | H8 coverage (RESEARCH §4.4, §2.5) | **Operator-new family + foreign-thread tally + stated HeapBlock gap** (O-Octagon precedent `plugins/O-Octagon/tests/render-harness/main.cpp:110-241`, copied verbatim with the provenance comment). `MidiBuffer::ensureSize (256)` before arming; one warm-up `processBlock` before the first armed block (absorbs the first-block `triggerAsyncUpdate` post — the tuning sentinels are −1e9); the five tuning parameters are never touched while armed. SUMMARY carries the coverage statement verbatim: "covered = every new-expression on the audio thread; not covered = HeapBlock / malloc paths (`AudioBuffer`, `Array`, `MidiBuffer`, `ReferenceCountedArray`, `MemoryBlock`) and the `AsyncUpdater` post path — grep + inspection". No `malloc_zone_t` hook in Round A. |
| 5 | Shared ramps in 2.3 (RESEARCH §4.5, §2.4) | **Processor-owned per-block ramp buffers indexed by absolute sample position.** `std::array<juce::SmoothedValue<float>, 24> baseRamps` (22 base values + ModWheel + Aftertouch) + `juce::AudioBuffer<float> rampBuffers` (24 rows × `samplesPerBlock`, sized in `prepareToPlay`) filled once per `processBlock` **before** `synthesiser.renderNextBlock` (token `// Render synth voices`): `getNextValue()` per sample only while `isSmoothing()`, else `FloatVectorOperations::fill`. Terrain Freq rows hold **log2 F_base**. Voices cache the 24 read pointers via `processor->getRampRow (i)` at the top of `renderNextBlock` and index `row[sample]` with the loop's absolute `sample` (the `juce::Synthesiser` sub-block split is then free). **Capacity guard:** `jassert (numSamples <= rampCapacity)` + `jmin` clamp of the index — never `setSize` on the audio thread; a host that exceeds `samplesPerBlock` reads the last ramp sample (documented in SUMMARY). The Phase 2.1 per-voice ramps (Decision 6 below) are **replaced**, not kept alongside; H7 measures the per-voice → shared delta by running H7 at the Phase 2.2 commit (per-voice) and at the 2.3 working tree (shared), both best of 3, and prints both. |
| 6 | Phase 2.1 per-voice ramps (RESEARCH §2.4) | Exactly ARCH Core 9: `std::array<juce::SmoothedValue<float>, 24> ramps` in `StrataVoice`, `reset (fs, 0.005)` in `prepare`, `setTargetValue` once per `renderNextBlock`, `getNextValue()` per sample, **plus** `setCurrentAndTargetValue` on all 24 in `startNote` when `! wasActive` (idle voices never advance their ramps — the note-on reset is the line the CONTEXT lacked). Terrain Freq ramps carry log2 F_base. Row order (index → base value) is fixed here and reused by the 2.3 processor buffers: 0 `oscAPos`, 1 `oscAOrbAspect`, 2 `oscAOrbRot`, 3 `oscAOrbCX`, 4 `oscAOrbCY`, 5 `oscAOrbMod`, 6 `oscATerFreq` (log2), 7 `oscATerModX`, 8 `oscATerModY`, 9 `oscAOrbFeedback`, 10 `oscATerSat`, 11–21 the same for `oscB`, 22 ModWheel, 23 Aftertouch. |
| 7 | Native stubs (RESEARCH §4.6, §2.7) | `getActiveOscFrame` → `complete ("[]")`; `getActiveOscInfo` → `complete ("{}")`. Both keep their `withNativeFunction` registration in `PluginEditor.cpp` (page untouched, Stage 1 D1); the `toJsonFloatArray` helper stays if still used elsewhere, else goes. Both stubs carry a `// Phase 3.1 removes this stub` comment. Removed in Phase 3.1. |
| 8 | Phase seed (RESEARCH §4.7, §2.8) | `std::atomic<uint32_t> harnessPhaseSeed { 0 }` on the processor + public `setHarnessPhaseSeed` / `getHarnessPhaseSeed`; `StrataVoice::setVoiceIndex (int)` called in the ctor loop (token `auto* voice = new StrataVoice();`); `startNote` reads the seed relaxed and calls `resetWithRandomPhases (seed == 0 ? 0 : seed ^ (voiceIndex · 0x9E3779B9u) ^ (oscIndex << 16))`. `TerrainOscillator::resetWithRandomPhases (uint32_t seed = 0)`: `seed == 0` → the existing `this ^ 0x12345678` address hash (production unchanged), else the LCG runs from `seed`. H9 / H10 renders pin `osc?Phase = 0.25` (single fixed phase, as the Stage 1 smoke) **and** set the seed, so unison-4 rows are deterministic too. |
| 9 | `theta_reference.h` (RESEARCH §4.10) | **Task 1 of this plan**, before any deletion: the warp / unison formulas are copied from `WavetableOscillator.cpp` into `tests/render-harness/reference/theta_reference.h` with a provenance comment naming the source file and commit `efae0bfc`. Nothing from `research/…/terrain-bench/kernels.h` terrains is copied (GPL transcription); only the neutral FFT / `analyse()` / Kaiser pieces of `alias.cpp` are adapted into `reference/spectrum.h` with a provenance comment. No cross-tree `#include` from `research/`. |
| 10 | `setUnison` `pow` (RESEARCH §3.15) | The per-partial `std::pow (2.0, normalizedPos · detune · 50 / 1200)` is **cached on (count, detune, width) change** inside `TerrainOscillator::setUnison` (early-out when all three are equal to the stored values, `detune` / `width` compared exactly). `setUnison` is called every block from `renderNextBlock` (token `// OscA/OscB Detune mod destinations (WR-02)`) and stays so; with the cache the DSP-05 grep gate needs no `updateBlockRate` exemption for it. |
| 11 | Damp law coefficient and `a_eff` (ARCH Core 4) | Computed in `updateBlockRate` once per oscillator per block from `fs`, `OS` and the block-rate Damp (Feedback Damp is **not** a mod destination — it is one of the 7 non-modulated new params): `a = 1 − exp2 (−1 − 9·damp)`, `a_eff = pow (a, 48000 / (fs · OS))`. Block-rate `pow` / `exp2` allowed here and only here (plus `r_track`). |
| 12 | Where the exp2 for Terrain Freq runs | **Once per oscillator per sample in the voice**, not per partial: `F = exp2f (log2Base + 2·offset)` (ramp row 6 / 17 in log2 + the raw mod offset × 2), then `jlimit (0.25f, 8.0f, F)`, then `oscA.setTerrainFreq (F)`. The oscillator multiplies by `r_track` (block-rate) internally. `std::exp2f`, not a polynomial (RESEARCH §2.3). |
| 13 | Saturation | `TerrainOscillator::scan` applies `y = FastMathApproximations::tanh (jlimit (−3.f, 3.f, 1.313f · sat · … ))` **only when `sat > 0.0f` exactly** — the branch is skipped at 0 so DSP-07 identity is bit-exact. Law: `y' = tanh (g · y) / tanh (g)`, g = 1 + 4·sat (unity-normalised, no level drop). Argument clamped to ± 3 before the Padé (RESEARCH §2.3). |
| 14 | Superellipse `pow` | `Orbits.h` reads r(θ, n) from a `33 × 129` float LUT (n over [0.5, 6], θ over one quadrant, symmetric) built in `TerrainOscillator::prepare` — the LUT lives as a `static` shared table built once under a `std::call_once` in `prepare` (16 voices × 2 oscillators must not each own 17 KB). `prepare` is not RT; `call_once` is allowed there. |
| 15 | Orbit re-normalisation per block (ARCH Core 2) | `updateBlockRate` re-scans 64 θ-points to find max radius when `orbitMod` differs from the last-normalised value by > 1e−3 (Superellipse, Limaçon, Butterfly, Squarcle, epitrochoids, hypocycloids — all except Ellipse); the norm factor is a block constant. Computed from constants and the block-start Orbit Mod, never from block length (block-size invariance). |
| 16 | Feedback `û` | Rotation vector (cos ρ, sin ρ) of the **base** rotation (ramp + offset × 180°) at the sample, i.e. the same ρ used for the affine — one `sincos` per oscillator per sample already needed for the rotation; no second trig call. |
| 17 | `fb = 0` block skip (2.3 saving) | `updateBlockRate` receives the block's **maximum** |Feedback| over the ramp row + the routed offsets is unknown per block, so the skip is gated on `feedbackRow` being at 0 for the whole block **and** no mod slot routed to that oscillator's Feedback destination (the voice checks `modMatrix.isDestinationRouted (ModDest::OscAOrbFeedback)` — add this const query to `ModulationMatrix`, block-rate, no allocation). When skipped, `d`, `y1`, `y2` are held (not reset); the H3 `memcmp` row compares `fb = 0` against `feedbackPathEnabled = false` on the **same** code path, so the skip must produce bit-identical output to the unskipped `0·finite` path — it does (both add 0). |
| 18 | Quality switch crossfade (ARCH Core 5) | On `setQuality (q)` with `q != current` mid-note: the oscillator marks `xfadeRemaining = 64`, resets the **new** path's decimator state, and for 64 base samples runs both paths and blends linearly (equal-gain). Phase state is shared (one θ per partial), so both paths read the same θ; the old path's sub-sample θ advance is dropped after the fade. `setQuality` from `startNote` (voice inactive) switches instantly, no fade. |
| 19 | H2 preset loop (RESEARCH §2.11) | H2 iterates the **6 × 11 terrain × orbit grid** as the real coverage and, separately, `FactoryPresets::build (apvts)` through the disk-free apply loop of RESEARCH §2.11, printing `presets: 1 (Init)` so the vacuity is visible. Phase 4.1 re-runs the same loop over the full bank. |
| 20 | `.gitignore` placement (RESEARCH §2.12) | `plugins/O-Strata/tests/.gitignore` with `exports/`, `render-harness/golden/*.wav`, `render-harness/golden/*.json`, `!render-harness/golden/*.sha256`, `*.png` (ad-hoc probe images), and `plugins/O-Strata/.planning/stages/1-foundation/smoke/.gitignore` with `strata-smoke` + `main.o` (the untracked binary stays untracked and stops showing in `git status`). Root `.gitignore` untouched. |
| 21 | Smoke fold-in (CONTEXT correction 4) | `--smoke` reproduces Stage 1 checks [1]–[6] from `stages/1-foundation/smoke/main.cpp` with: [1] on the terrain oscillator; [2] Scala from `STRATA_FIXTURES_DIR/test-tunings/just-major.scl` (copied in); [3] unchanged; [4] **inverted** — LFO1 → `OscA Terrain Freq` (index 31) **changes** the render (`maxAbsDiff > 1e−3`), the unrouted render is the control, `Pitch` (23) stays as a second positive control; [5] unchanged but **skipped unless `--with-disk`** (the constraints forbid `~/Library` in the harness by default; the Stage 1 D2 assertion is already verified); [6] unchanged. Literals asserted alongside live values as in Stage 1 Decision 6. |
| 22 | FUNC-09 / FUNC-10 formal gate (`--gate tuning`) | Through `getTuningEngine()` directly (no message loop): (a) `loadScalaFile (just-major.scl)`, (b) a 31-EDO generator via the `ScaleGenerator` / `TuningEngine` API the tuning tab uses (executor reads `TuningEngine.h` for the EDO entry point — token `EDO`), then render 1 s at each of ≥ 5 keys (48, 55, 60, 64, 67) with Sine Product + Ellipse at defaults, Unison 1, Phase 0.25; fundamental by windowed parabolic peak interpolation vs `getTuningEngine()->getFrequency (key)`: |Δ| ≤ 0.5 cent. Plus the param-dump ID parity already on file (params.tsv). |
| 23 | Ellipse Orbit Mod (FUNC-03 criterion) | Inert by design (ARCH Core 2). The FUNC-03 centroid gate **skips** Ellipse and prints `Ellipse: Orbit Mod inert (documented)`; the 10 other orbits must each show ≥ 5 % centroid delta across Orbit Mod 0 → 1. |
| 24 | ROADMAP items moved by CONTEXT / RESEARCH | `CycleCapture` ring (Core 10) is **data only** in 2.1 (written, never read); H6 "C-notes" → A-notes; H8 "operator new" → family + gap statement; latency "1.15 / 1.4" → "1.26 / 1.74, report +1"; "22 ramps per oscillator" → 24 per voice (2.1) → 24 processor rows (2.3). ROADMAP / ARCHITECTURE are checksummed and **not edited**; corrections live in CONTEXT, RESEARCH §3 and SUMMARY. |

---

## Tasks

Three commit groups (one per ROADMAP phase), each ending with its gates green and a path-scoped commit. Tasks 1–12 = Phase 2.1; 13–17 = Phase 2.2; 18–25 = Phase 2.3; 26–28 close the round. Each task ends with the check the executor runs before moving on.

### ━━━ Phase 2.1 — TerrainOscillator core at 1×, libraries, mod wiring, wavetable deletion, harness skeleton ━━━

### Task 1 — Reference copies before the deletion (Decision 9)
- **Create:** `tests/render-harness/reference/theta_reference.h`, `tests/render-harness/reference/spectrum.h`
- **`theta_reference.h`:** header-only analytic θ pipeline reproducing `WavetableOscillator.cpp` (`efae0bfc`) exactly — `applyWarp` (Bend `pow (phase, 1 + 3a)`, FM `frac (phase + fmIn·a)`), the Sync / Window branch (`syncRatio = 1 + 3a`, master re-seed on wrap, `× sin (π·masterPhase)`), unison laws (`gain = 1/√n`, `centerIndex`, `detuneFactor = 2^(pos·detune·50/1200)`, pan `cos / sin (panNorm·π/2)`), LCG `resetWithRandomPhases` with an explicit seed, `resetWithPhase`; a function `renderCosReference (params, N, fs, out)` producing `y[n] = Σ_partials gain·pan·cos (2π·θ_i[n])` with the **5 Hz one-pole DC blocker** applied (Decision 3). Provenance comment: `// Copied from Source/dsp/WavetableOscillator.cpp at efae0bfc before its deletion in Phase 2.1 — the H1 θ-parity reference.`
- **`spectrum.h`:** radix-2 FFT (`std::complex<double>`), `analyseExactCycle (y, k, bandHz)` → `{ fundPow, maxHarmPow, nonHarmPow, peakNonHarmBin, maxHarmIdxAboveMinus100dB }` printing nonharm/fund and nonharm/max; `windowedSpectrum (y, fs, hann 8192, hop)`; `partialsAbove (spec, −40 dB)` count; `spectralCentroid (spec)`; `peakFrequencyInterpolated (spec)` (parabolic). Provenance comment: `// FFT, analyse() and the Kaiser-sinc decimator adapted from research/wavetable-synthesis-3d-geometry-prototypes/terrain-bench/alias.cpp (own code, no terrain formulas).`
- **Depends on:** none
- **Verify:** both headers compile in isolation (`clang++ -std=c++17 -fsyntax-only -I…` with a stub `JuceHeader.h` is not needed — they are JUCE-free); `grep -c 'kernels.h' tests/render-harness/reference/*` = 0.

### Task 2 — `dsp/Orbits.h` (ARCH Core 2, Decision 10)
- **Create:** `Source/dsp/Orbits.h` — `enum class OrbitKind { Ellipse = 0, Superellipse, Limacon, Epitrochoid3, Epitrochoid5, Epitrochoid7, Hypocycloid3, Hypocycloid5, Hypocycloid7, Butterfly, Squarcle }` (order = the `osc?Orbit` choice list, asserted in Task 8), `struct OrbitPoint { float x, y; }`, `inline OrbitPoint baseOrbit (OrbitKind, float theta, float m, const OrbitScratch&)` with the ARCH Core 2 table formulas verbatim: Ellipse `(cos θ, sin θ)`; Superellipse via the LUT `r (θ, n)`, n = 0.5 + 5.5m; Limaçon r = 1 + 2m·cos θ; Epitrochoid p `((p+1)cos θ − d cos ((p+1)θ), (p+1)sin θ − d sin ((p+1)θ))`, d = 0.1 + 2.9m; Hypocycloid p `((p−1)cos θ + d cos ((p−1)θ), (p−1)sin θ − d sin ((p−1)θ))`, d = 0.1 + 1.9m; Butterfly r = e^{cos θ} − 2cos 4θ + 2m·sin⁵θ (sin⁵ = s²·s²·s; `std::exp`); Squarcle `(tanh (k cos θ)/tanh k, tanh (k sin θ)/tanh k)`, k = 0.3 + 6m, `FastMathApproximations::tanh` on the ± 3-clamped argument, `1/tanh k` from `OrbitScratch` (block cache). `constexpr int orbitK (OrbitKind)` = {1, 0, 2, 4, 6, 8, 2, 4, 6, 0, 0} (0 = not a trig polynomial; Round B uses it). `struct OrbitScratch { float normFactor = 1; float invTanhK = 1; }` filled by `TerrainOscillator::updateBlockRate` (Decision 15). `struct SuperellipseLUT { static const float* get(); }` — 33 × 129 floats built under `std::call_once` (Decision 14), bilinear read `lookup (theta, n)` with quadrant symmetry.
- **Depends on:** none
- **Verify:** `grep -n 'std::pow\|powf\|std::function\|new \|make_unique' Source/dsp/Orbits.h` hits only inside `SuperellipseLUT::build` (the `prepare`-time builder, commented `// prepare-time only`). A throwaway scratch program (scratchpad, not committed) sweeps θ over 256 points at m = 0.5 for every orbit and prints max radius — all ≈ 1 after the norm scan (the executor's sanity check that the formulas close and are bounded).

### Task 3 — `dsp/Terrains.h` (ARCH Core 3, Decision 10)
- **Create:** `Source/dsp/Terrains.h` — `enum class TerrainKind { SineProduct = 0, RadialRings, Saddle, RidgedCosines, Mitsuhashi, CosineWells, Imported, HarnessIdentityX = 100 }` (0–6 = the `osc?Terrain` choice list, asserted in Task 8; `HarnessIdentityX` is harness-only, never reachable from the parameter — the voice maps choice index → kind through a `static_cast` guarded by `jlimit (0, 6, …)`), `inline float terrain (TerrainKind, float x, float y, float F, float mx, float my)` with the ARCH Core 3 formulas verbatim under the **πF convention**: Sine Product `sin (πF·x + π (mx − ½)) · sin (πF·(½ + my)·y)`; Radial Rings `cos (2πF·(½ + mx)·ρ + 2π (my − ½))`, ρ = √(x² + y²); Saddle `sin (πF·(x² − y² + 2 (mx − ½)·xy) + π (my − ½))`; Ridged Cosines `1 − |cos (πF·x + π (mx − ½))| − |cos (πF·(½ + my)·y)|`; Mitsuhashi `1.747·(u cos α − v sin α)(u² − 1)(v² − 1)`, u = tri (F·x), v = tri (F·(½ + my)·y), α = mx·π/2, `tri` = triangle wrap into [−1, 1]; Cosine Wells `1 − 2·hᵖ`, h = ½ (1 + cos (2πF·x)·cos (2πF·(½ + mx)·y)), p = 1 + 6·my via lerp between integer powers ⌊p⌋ and ⌊p⌋ + 1 (integer powers by repeated multiplication, ≤ 7); `Imported` returns 0 in Round A (`jassertfalse` off — the choice is legal, the path lands in 2.5); `HarnessIdentityX` returns `x`. Every branch ends `jlimit (−1.f, 1.f, …)`. Trig via `std::sinf / cosf` (RESEARCH §2.3: no fast-sin needed).
- **Depends on:** none
- **Verify:** DSP-05 grep as Task 2 → zero hits. Scratch check: each terrain at F = 1, mx = my = 0.5 sampled on a 64² grid stays within [−1, 1] and is non-constant.

### Task 4 — `dsp/TerrainOscillator.h/.cpp` (ARCH Core 1, 4 skeleton, 10; Decisions 8, 10, 13–16)
- **Create:** `Source/dsp/TerrainOscillator.h`, `Source/dsp/TerrainOscillator.cpp`. **Delete:** nothing yet (Task 10).
- **Public interface = `WavetableOscillator`'s minus `setWavetable`** (`WavetableOscillator.h:45-63`): `prepare (double)`, `setFrequency (double)`, `setPosition (float)`, `reset()`, `resetWithPhase (double)`, `resetWithRandomPhases (uint32_t seed = 0)` (Decision 8), `getNextSampleStereo (double&, double&)`, `setUnison (int, float, float)` (cached per Decision 10, clamps to `kMaxUnison = 4`), `setWarpType`, `setWarpAmount`, `setFMInput`, `getFrequency`. `enum class WarpType` **moves** into this header unchanged (it was in `WavetableOscillator.h:34-41`; `StrataVoice.h` includes the new header).
- **New block-rate setters:** `setTerrain (TerrainKind)`, `setOrbit (OrbitKind)`, `setQuality (Quality)` (enum `{ Bandlimited = 0, X2, X4 }`; in Round A `Bandlimited` runs the analytic path at 1× — "chebPtr == nullptr fallback"), `setPitchTrack (float)`, `setFeedbackDamp (float)`, `setEdgeMode (EdgeMode)` (stored, unused until 2.5), `updateBlockRate (double noteHz)` (Decisions 11, 15; in 2.1 computes only the orbit norm and the Squarcle `invTanhK`; tracking / damp arrive in Task 13 / 14). **Per-sample setters** (store a float, no computation): `setAspect`, `setRotation` (radians), `setCentre (x, y)`, `setOrbitMod`, `setTerrainFreq` (already exp2'd and clamped by the voice, Decision 12), `setTerrainMod (x, y)`, `setFeedback`, `setSaturation`.
- **Per-partial state** (`kMaxUnison = 4`): `double phaseAccumulators[4]`, `masterPhases[4]`, `unisonDetuneFactors[4]`, `unisonPanL/R[4]` (copied from `WavetableOscillator.h:76-88` minus the wavetable pointer), `float fbD[4], fbY1[4], fbY2[4]` (Task 14 uses them; zeroed in `reset*`), decimator states (Task 18). Per-oscillator: DC blocker `x1, y1` for L and R, `OrbitScratch`, cached `sinRot / cosRot`.
- **`getNextSampleStereo`:** the Sync / Window branch and the Bend / FM branch copied **verbatim** from `WavetableOscillator.cpp:183-268` with `readSample (phase)` → `scan (phase, i)`; `isfinite` guard kept. `scan (phase, i)`: θ = 2π·phase → `baseOrbit (kind, θ, m, scratch)` → affine (aspect on y, rotation, `r = 0.05 + 0.95·pos`, centre) → **(feedback displacement, Task 14: `+ fb·d[i]·û`)** → `p` clamped to [−1, 1]² → `terrain (kind, p.x, p.y, F_eff, mx, my)` → clamp → saturation (Decision 13, skipped at 0). After the unison sum: DC blocker on L and R (`y = x − x1 + R·y1`, R = 1 − 2π·5/fs, computed in `prepare`). Round A Phase 2.1 runs OS = 1 only; Task 19 adds the OS loop.
- **`CycleCapture` (Core 10, data only):** `struct CycleCapture { std::array<float, 2048 * 4> ring; std::atomic<uint32_t> writeIndex; }` owned by the processor (one per oscillator); `TerrainOscillator::setCaptureTarget (CycleCapture*)`; partial 0 writes (θ, p.x, p.y, y) per base sample when the target is non-null. The processor points only the **display voice's** oscillators at the rings (most recently started voice — `setLastPlayedFrequency`'s caller); Round A never reads them. Cost: one predicted branch + 4 stores when active.
- **Harness switches read here:** `terrainKernelBypass` (scan returns 0 and skips the decimator; everything else runs — Task 24) via a `bool` the voice copies from the processor atomic at block start into `setKernelBypass (bool)`.
- **Depends on:** Tasks 2, 3
- **Verify:** compiles with the voice still on `WavetableOscillator` (both headers coexist until Task 10); `grep -n 'std::pow\|powf\|std::function\|new \|make_unique\|malloc' Source/dsp/TerrainOscillator.cpp` hits only lines inside `prepare` / `setUnison` (the cached detune `pow`, Decision 10) / `updateBlockRate` — each hit commented `// block-rate (DSP-05 exemption: …)`.

### Task 5 — `StrataVoice`: member swap, ramps, mod-destination reads, block-rate feed (ARCH Core 9; Decisions 6, 8, 12)
- **Modify:** `Source/StrataVoice.h`, `Source/StrataVoice.cpp`
- **`.h`:** `#include "dsp/TerrainOscillator.h"` replaces the `WavetableOscillator.h` include; `TerrainOscillator oscA, oscB;` (token `WavetableOscillator oscA;`); delete `setWavetableA/B` declarations and the `struct WavetableData;` forward declaration; add `void setVoiceIndex (int i) { voiceIndex = i; }` + `int voiceIndex = 0;`; add `std::array<juce::SmoothedValue<float>, 24> ramps;` (Decision 6 row order) and the 20 new cached APVTS pointers (`pOscATerrain`, `pOscATerFreq`, `pOscATerModX`, `pOscATerModY`, `pOscATerTrack`, `pOscATerSat`, `pOscATerBlur`, `pOscATerEdge`, `pOscAOrbit`, `pOscAOrbAspect`, `pOscAOrbRot`, `pOscAOrbCX`, `pOscAOrbCY`, `pOscAOrbMod`, `pOscAOrbFeedback`, `pOscAOrbFbDamp`, `pOscAQuality` and the B set — 34 total, all 17 per oscillator cached even where Round A does not read them yet).
- **`.cpp` `setAPVTS`:** cache the 34 pointers (`getRawParameterValue` by ID; IDs from `StrataParamIds.h` suffixes).
- **`prepare`:** `ramps[i].reset (sampleRate, 0.005)` for all 24; `oscA/B.prepare (sampleRate)` unchanged.
- **`startNote`:** after the existing unison / phase block, `if (! wasActive)` → `setCurrentAndTargetValue` on all 24 ramps from the current APVTS values (log2 for rows 6 / 17); the phase reset becomes `resetWithRandomPhases (seedFor (oscIndex))` with `seedFor` per Decision 8 reading `processor->getHarnessPhaseSeed()`; delete `setWavetableA/B` definitions (token `void StrataVoice::setWavetableA`).
- **`renderNextBlock` block section** (token `// ─── Read per-block parameters`): read the 34 new params; `oscA.setTerrain (static_cast<TerrainKind> (jlimit (0, 6, int (pOscATerrain->load()))))`, `setOrbit`, `setQuality`, `setPitchTrack`, `setFeedbackDamp`, `setEdgeMode`; `oscA.setKernelBypass (processor->harnessTerrainKernelBypass.load())`; **`updateBlockRate (glide target frequency × pitchRatioA)`** — the glide *target* (add `GlideProcessor::getTarget()` const accessor if absent; token `setTarget`), not the per-sample glided value (ARCH Algorithm "Pitch tracking"); `ramps[i].setTargetValue (…)` for all 24 (rows 6 / 17 = `std::log2 (pOscATerFreq->load())`; rows 22 / 23 = `modWheelVal` / `aftertouchVal`).
- **`renderNextBlock` per-sample section** (token `// ─── Apply modulation offsets to parameters`): replace the `modulatedPosA/B` lines with the full mapping — for each oscillator, base = `ramps[row].getNextValue()`, offset = `modMatrix.getModOffset (ModDest::…)`: Size `jlimit (0, 1, base + off)`; Aspect `jlimit (0.1, 1, base + 0.9·off)`; Rotation `(base + 180·off)·π/180` (no clamp, periodic); Centre X / Y `jlimit (−1, 1, base + off)`; Orbit Mod `jlimit (0, 1, base + off)`; Terrain Freq `jlimit (0.25, 8, exp2f (base + 2·off))` (Decision 12); Mod X / Y `jlimit (0, 1, …)`; Feedback `jlimit (0, 1, …)`; Saturation `jlimit (0, 1, …)` → the nine per-sample setters. ModWheel / Aftertouch: `modMatrix.setSourceValue (ModSource::ModWheel, ramps[22].getNextValue())` replaces the block-constant `modWheelVal` at the token `modMatrix.setSourceValue (ModSource::ModWheel, modWheelVal)` (same for Aftertouch).
- **Harness tap** (Decision 3): after the `// Apply level and pan` block, `if (preFilterTap) { leftChannel[sample] += float (oscAL + oscBL) (mix-weighted as the existing mix code); rightChannel[…]; continue-to-envelope-only path }` — implement as: compute the mixed oscillator L/R exactly as the existing mix code does, then when the tap is set write `mixL · envVal`, `mixR · envVal` to the output and skip sub / noise / filters / the rest of the sample body. The tap flag is a `bool` copied once per block from `processor->harnessPreFilterTap`.
- **Depends on:** Task 4
- **Verify:** builds; `grep -n 'Wavetable' Source/StrataVoice.*` = 0; the 20 new `ModDest` enumerators each appear exactly once in `StrataVoice.cpp` (`grep -c 'ModDest::OscAOrbAspect' …` = 1, etc.).

### Task 6 — `ModulationMatrix`: routed-destination query (Decision 17)
- **Modify:** `Source/dsp/ModulationMatrix.h/.cpp` — add `bool isDestinationRouted (ModDest d) const` (true if any enabled slot with a non-zero amount targets `d`; evaluated from the cached slot state `updateFromAPVTS` already builds — no APVTS reads, no allocation). Used by Task 22's fb = 0 skip and by the harness (`--smoke` [4] positive control asserts it flips).
- **Depends on:** none
- **Verify:** `grep -n 'isDestinationRouted' Source/dsp/ModulationMatrix.h` = 1 declaration; no `juce::String` in the body.

### Task 7 — Processor: seed, harness switches, `setVoiceIndex`, `CycleCapture`, reaper type-erasure, `updateOscillatorAssignments` stub, native stubs (Decisions 4, 7, 8; ARCH Decision 6)
- **Modify:** `Source/PluginProcessor.h`, `Source/PluginProcessor.cpp`, `Source/PluginEditor.cpp`
- **`.h` public:** `std::atomic<uint32_t> harnessPhaseSeed { 0 }` + `set/getHarnessPhaseSeed`; `std::atomic<bool> harnessFeedbackPathEnabled { true }`, `harnessTerrainKernelBypass { false }`, `harnessSingleSampleFeedback { false }`, `harnessPreFilterTap { false }` — all documented `// Harness-only switch (ARCH "Harness design"); never a parameter, never persisted.`; `CycleCapture cycleCapture[2]` + `const CycleCapture& getCycleCapture (int)`. **Delete:** `getActiveOscTable`, `placeholderTable`, `oscTablePtr[2]`, `lastAssignedTable[2]`, `updateWavetableAssignments`, `struct RetiredTable`, `retireTable`, `std::vector<RetiredTable> retiredTables` (tokens: `getActiveOscTable`, `placeholderTable`, `oscTablePtr`, `lastAssignedTable`, `updateWavetableAssignments`, `RetiredTable`, `retireTable`). **Add** the type-erased reaper: `struct Retirable { virtual ~Retirable() = default; };`, `struct Retired { std::unique_ptr<Retirable> object; uint64_t retiredAt; }`, `std::vector<Retired> retired;  // message thread only`, `void retire (std::unique_ptr<Retirable>)` (message thread; `JUCE_ASSERT_MESSAGE_THREAD`), `void updateOscillatorAssignments()` (Round A: empty body with the comment `// Round B: publishes chebPtr[] / imagePtr[] into the voices` — called from `processBlock` where `updateWavetableAssignments()` was, token `// Update wavetable assignments if table selection changed`). The reaper comment block (`// ─── Retired-table reaper ───`) is rewritten for `Retirable`; `blockGeneration` and its `fetch_add` (token `// Publish "this block is done reading wavetables"`) stay, comment updated.
- **`.cpp` ctor:** delete the placeholder-table block (token `// Stage 1 placeholder: one sine frame`), `setWavetableA/B` calls, `lastAssignedTable[0] = …`; add `voice->setVoiceIndex (i)` and `voice->setCaptureTargets (&cycleCapture[0], &cycleCapture[1])` (the voice forwards to its oscillators only when it is the display voice — simplest Round A rule: **every** voice points at the rings and only the voice whose `currentMidiNote` equals `processor->getLastPlayedNote()` writes; add `std::atomic<int> lastPlayedNote` set beside `setLastPlayedFrequency`). `timerCallback` reaper body → `retired` / `Retirable`. `getActiveOscTable` definition deleted.
- **`PluginEditor.cpp`** (tokens `withNativeFunction ("getActiveOscInfo"`, `withNativeFunction ("getActiveOscFrame"`): bodies → `complete ("{}")` / `complete ("[]")` with the Phase 3.1 comment (Decision 7); `#include "dsp/WavetableData.h"` and `toJsonFloatArray` removed if unused (grep first).
- **Depends on:** Tasks 4, 5
- **Verify:** builds (the plugin target with `JUCE_WEB_BROWSER=1`); `grep -rn 'Wavetable\|WaveShape\|oscTablePtr\|placeholderTable\|retireTable\|RetiredTable' Source/` → only the two files deleted in Task 10 (until then) — after Task 10, **zero**.

### Task 8 — Enum ↔ choice-list assertions and `StrataParamIds` comment refresh
- **Modify:** `Source/PluginProcessor.cpp` (`createOscParameters`, token `choice ("Terrain",`), `Source/StrataParamIds.h` (comments only)
- Add `static_assert (static_cast<int> (TerrainKind::CosineWells) == 5 && static_cast<int> (TerrainKind::Imported) == 6, "osc?Terrain choice order")` and `static_assert (static_cast<int> (OrbitKind::Squarcle) == 10, "osc?Orbit choice order")` next to the `choice (…)` calls (include the two headers); refresh the `// live oscillator lands in Phase 2.1` / `Plain APVTS parameters; the voice does not read them yet` comments to say the voice reads them. **No parameter, range, list or default changes** (spec v2 locked).
- **Depends on:** Tasks 2, 3
- **Verify:** param-dump rebuild + `diff` against `.planning/params.tsv` is **empty** (Task 11).

### Task 9 — CMake: harness target, defines, `.gitignore`s (RESEARCH §2.1, §2.12; Decision 20)
- **Modify:** `plugins/O-Strata/CMakeLists.txt` — inside the existing `if(OUARICON_BUILD_TESTS)` after `ouaricon_add_param_dump`, the RESEARCH §2.1 block verbatim: `ouaricon_add_processor_console(O-Strata ${CMAKE_CURRENT_SOURCE_DIR}/Source ${CMAKE_CURRENT_SOURCE_DIR}/tests/render-harness/main.cpp render-test)`, `target_compile_definitions(O-Strata-render-test PRIVATE STRATA_FIXTURES_DIR="…/tests/render-harness/fixtures" STRATA_EXPORTS_DIR="…/tests/exports")`, the `STRATA_HARNESS_ASAN` option block (OFF; Round B uses it). Replace the `target_sources` lines `Source/dsp/WavetableGenerator.cpp` / `WavetableOscillator.cpp` with `Source/dsp/TerrainOscillator.cpp` (token `Source/dsp/WavetableGenerator.cpp`). Refresh the test-block comment (`Expect 205 rows` stays true).
- **Create:** `plugins/O-Strata/tests/.gitignore` and `.planning/stages/1-foundation/smoke/.gitignore` per Decision 20; `tests/render-harness/fixtures/test-tunings/just-major.scl` (copied from the smoke's fixture — find it with `find . -name just-major.scl`; commit the copy); `tests/render-harness/main.cpp` **must exist before configuring** (the builder FATALs otherwise) — Task 12 writes it; the executor configures only after Task 12.
- **Depends on:** Task 12 for the configure step
- **Verify:** `cmake --build build --target O-Strata-render-test` (background) links `build/plugins/O-Strata/O-Strata-render-test_artefacts/Release/O-Strata-render-test`; `git status --short` shows no `strata-smoke`, no `main.o`, no `tests/exports/`; `git check-ignore -v plugins/O-Strata/tests/exports/x.wav` names `tests/.gitignore`.

### Task 10 — Delete the wavetable path (ARCH Decision 6)
- **Delete:** `Source/dsp/WavetableOscillator.h/.cpp`, `Source/dsp/WavetableData.h`, `Source/dsp/WavetableGenerator.h/.cpp` (`git rm`).
- **Depends on:** Tasks 1 (reference copied), 4, 5, 7, 9
- **Verify:** `grep -rn 'Wavetable\|WaveShape\|kNumMipmapLevels' Source/ CMakeLists.txt` = 0; full plugin rebuild green (`ninja O-Strata_VST3 O-Strata_AU O-Strata-param-dump`, background); `ls Source/dsp | grep -c Wavetable` = 0.

### Task 11 — Build, param-dump parity, pluginval (Phase 2.1 mid-check)
- Background: `cmake --build build --target O-Strata_VST3 O-Strata_AU O-Strata-param-dump O-Strata-render-test`. Run the param-dump, `diff` against `.planning/params.tsv` → **empty** (no parameter moved). `./scripts/build-and-install.sh O-Strata` then `pluginval --strictness-level 10` on the VST3 and the AU (background, per the CLAUDE.md cache-clearing sequence the script already performs). auval at the round's end (Task 26) is enough; here pluginval only.
- **Depends on:** Task 10
- **Verify:** `diff` empty; pluginval ×2 SUCCESS; a held note sounds (the harness `--smoke` [1] in Task 12 is the measured form).

### Task 12 — Harness skeleton + Phase 2.1 gates: `--smoke`, `--gate tuning`, H1, H2, H5, H8, H9, FUNC-02/03 centroids (RESEARCH §5; Decisions 2–4, 8, 19, 21–23)
- **Create:** `tests/render-harness/main.cpp` (single TU; `reference/*.h` included), `tests/render-harness/README.md`
- **Skeleton:** `ScopedJuceInitialiser_GUI` first; `createPluginFilter()` → `dynamic_cast<OStrataAudioProcessor&>`; CLI per RESEARCH §5 (`--gate H1..H9|tuning|smoke|all`, `--note`, `--velocity`, `--seconds`, `--terrain`, `--orbit`, `--quality`, `--set id=norm` repeatable, `--fs`, `--block`, `--seed`, `--fixtures` (default `STRATA_FIXTURES_DIR`), `--export name` (into `STRATA_EXPORTS_DIR`, created on demand), `--print-only`, `--with-disk`); exit code = number of failed checks; every check prints measured value and threshold on one line (`[H2] SineProduct×Ellipse h1-max = -0.0 dB (need >= -6.0) PASS`); a `Render` helper: `prepareToPlay (fs, block)`, pre-sized `AudioBuffer`, `MidiBuffer` with `ensureSize (256)`, one warm-up block, note-on at sample 0, note-off at `seconds − release`, discards the first 0.35 s unless `ampSustain` is pinned to 1.0 (the helper pins `ampSustain = 1`, `ampAttack = 0`, `oscBLevel = 0` for spectral gates — the "clean patch" of RESEARCH §2.6 — and prints the pinned set once). **No `runDispatchLoopUntil` anywhere in Round A** (assert by grep in the README).
- **Allocation counter** (Decision 4): O-Octagon's family copied verbatim into `main.cpp` with provenance; `armed` thread-local; foreign tally printed beside every H8 verdict.
- **Gates implemented here:**
  - **`--smoke`** (Decision 21): [1]–[6], [4] inverted, [5] behind `--with-disk`.
  - **`--gate tuning`** (Decision 22): Scala + 31-EDO, ≥ 5 keys, ≤ 0.5 cent.
  - **H1 θ parity** (Decision 3): `TerrainKind::HarnessIdentityX` (set through a harness-only processor method `setHarnessTerrainOverride (osc, TerrainKind)` the voice honours over the parameter), Ellipse, Aspect 1, Rotation 0, Centre (0, 0), Orbit Size → r = 1 (`osc?Pos = 1.0`), Quality **Bandlimited** (= analytic at 1× in Round A), `preFilterTap`, Phase 0.25, seed pinned; rows: Warp Off, Sync 0.5, Bend 0.5, Window 0.5, FM 0.5 (FM from B: `oscBLevel` stays 0 but B still runs — the reference gets B's cos as `fmIn`; if this couples awkwardly, the FM row uses `oscMix = 0` and B at Sine Product defaults with the reference computing B's exact cos θ — executor's call, documented), × unison {1, 4} with detune 0.2 / width 0.5; 1 s at C4; metric = `20·log10 (RMS (y − g·ref) / RMS (ref))` with least-squares g over the steady window: **≤ −80 dB** each row. Negative control: the reference run with Bend 0.5 against a Bend 0.6 render **must fail** (prints its dB). Pitch: windowed interpolated peak at C4 within ± 1 cent of 261.6256 Hz.
  - **H2 symmetry** (Decision 19): 6 × 11 grid at defaults, C4, 1 s, Hann 8192 hop 100 ms, h1 ≥ max − 6 dB in ≥ 95 % of windows; `Init` via the disk-free loop; negative control Sine Product + Ellipse, Centre (0, 0), Aspect 1 → must fail. On any grid failure print h1 at the eight neighbouring centres ± 0.1 (ARCH "Symmetry rule").
  - **H5 zipper:** each of the 22 destinations (+ ModWheel route) stepped 10 % every 100 ms at C4 via `setValueNotifyingHost`, 2 s, `preFilterTap` off (the real output): max |y[n] − y[n−1]| ≤ 0.1; negative control: `--set` a harness-only `harnessRampSeconds = 0` (a processor atomic the voice's `prepare` reads; default 0.005) → **must fail** on at least Centre X. Companion: LFO1 40 Hz → `OscA Orbit CX`, amount 0.5: spectrum shows peaks at h·f0 ± 40 Hz ≥ −40 dB relative to h·f0 for h = 1, 2 (the FUNC-05 sideband proof).
  - **H8 allocation:** arm around each `processBlock` while terrain / orbit change every 50 ms under 8 held notes for 3 s, plus Quality flips (Task 25 extends): **zero** counted; foreign tally printed.
  - **H9 block-size invariance:** 10 s default patch, Phase 0.25 + seed, at block 64 / 256 / 1024: pairwise max |Δ| ≤ 1e−5 (−100 dB) — per-voice ramps in 2.1 are time-based so this must already hold; Task 23 re-runs it on the shared rows.
  - **FUNC-02/03 centroids:** for each terrain, Mod X 0 → 1 and Mod Y 0 → 1 (Sine Product + Ellipse otherwise defaults): centroid delta ≥ 5 %; for each orbit ≠ Ellipse, Orbit Mod 0 → 1: ≥ 5 % (Decision 23); Orbit Size 0.05 → 1.0 in 8 steps over Sine Product: centroid monotone non-decreasing (tolerance 0.5 % per step for numerical noise, printed).
- **README:** every command executable as written from any directory, using the absolute artefact path; the H-table with thresholds; the coverage statement of Decision 4; the `--with-disk` note.
- **Depends on:** Tasks 1–11
- **Verify:** `O-Strata-render-test --gate all` (Round A subset so far) exits 0 from the repo root **and** from `/`; the three negative controls print FAIL as expected inside a PASS verdict line (`[H1 neg] … FAIL-as-expected`).

### Phase 2.1 commit
```bash
git branch --show-current && git status --short            # immediately before
git add plugins/O-Strata/tests plugins/O-Strata/.planning/stages/1-foundation/smoke/.gitignore   # new files
git commit -- plugins/O-Strata -m "feat(O-Strata): Phase 2.1 — live terrain oscillator at 1x, orbit/terrain libraries, mod wiring, wavetable path removed, harness"
```
Pre-commit: `git status --short plugins/O-Prism` empty; `grep -rn Wavetable plugins/O-Strata/Source` empty.

### ━━━ Phase 2.2 — Pitch tracking, trajectory feedback, saturation gates ━━━

### Task 13 — Pitch tracking (ARCH Algorithm "Pitch-tracked terrain frequency", Decision 2; plan Decision 11)
- **Modify:** `Source/dsp/TerrainOscillator.cpp` `updateBlockRate (noteHz)`: `rTrack = pow (jmin (1.0, 261.6256 / noteHz), pitchTrack)` (one block-rate `pow`, commented DSP-05 exemption); `scan` uses `F_eff = terrainFreq · rTrack` (`terrainFreq` is the per-sample setter value, already clamped [0.25, 8] by the voice). In Round A `Quality::Bandlimited` also applies `rTrack` (the analytic fallback); Round B makes it inert in that mode.
- **Depends on:** Task 12 (Phase 2.1 committed)
- **Verify:** H4 (Task 16).

### Task 14 — Trajectory feedback (ARCH Core 4, Decision 1; plan Decisions 11, 16)
- **Modify:** `Source/dsp/TerrainOscillator.h/.cpp`
- `updateBlockRate`: `a = 1 − exp2 (−1 − 9·feedbackDamp)`, `aEff = pow (a, 48000 / (fs · OS))` (OS = 1 in 2.2; Task 19 feeds the real OS).
- `scan (phase, i)`: after the affine, `if (feedbackEnabled)` (the harness switch, default true) `{ avg = 0.5·(fbY1[i] + fbY2[i]); d = aEff·fbD[i] + (1 − aEff)·avg; if (! isfinite (d)) d = 0; d = jlimit (−0.5, 0.5, d); fbD[i] = d; p += fb·d·û; }` where `harnessSingleSampleFeedback` replaces `avg` with `fbY1[i]` (the negative-control branch). After the terrain clamp: `fbY2[i] = fbY1[i]; fbY1[i] = y` (the **pre-saturation, pre-blocker** scan value — ARCH Core 1 "the feedback tap reads the pre-blocker scan value"). `reset*` zero `fbD / fbY1 / fbY2`. Feedback 0: `p += 0·d·û` is bit-identical to the branch removed (0·finite = 0; `d` stays finite by the scrub).
- **`CycleCapture`** now writes the **displaced** `p` (already the case if Task 4 wrote `p` after displacement — confirm).
- **Depends on:** Task 13
- **Verify:** H3 (Task 16).

### Task 15 — Saturation gate plumbing
- No new code beyond Task 4's Decision 13; add the harness rows in Task 16. If the Padé argument clamp or the `tanh (g)` normalisation was deferred in Task 4, land it here.
- **Depends on:** Task 4

### Task 16 — Phase 2.2 gates: H3, H4, DSP-07, Nyquist-hunting control
- **Modify:** `tests/render-harness/main.cpp`, `README.md`
- **H4:** Track 1: partials above −40 dB (windowed, `preFilterTap`) at C2 / C4 / C6 differ by ≤ 2, at F = 1 **and** F = 4 (Sine Product + Ellipse defaults); negative control: Track 0, **F = 8**, C6, 2× (in 2.2 the harness runs this row at 1× and re-runs at 2× after Task 19 — the README says which): non-harmonic energy ≥ 20 dB above the Track 1 run. Prints all six counts.
- **H3 grid:** Feedback {0.25, 0.5, 0.75, 1} × Damp {0, 0.5, 1} × 6 terrains × 11 orbits × {C2, C4, C6}, 0.5 s each (≈ 2376 renders; budget ≤ 90 s — if over, drop C2 to 0.25 s and say so): every sample finite, |y| ≤ 1 at the tap, |DC| of the post-blocker output < 1e−3 over the last 250 ms. **`memcmp` row:** Feedback 0 vs `harnessFeedbackPathEnabled = false` on the 66 pairs, 1 s, seed pinned: byte-identical.
- **Nyquist-hunting control:** Damp 0, Feedback 1, Ridged Cosines + Epitrochoid 5, C4: the bin at fs/2 (and the two beside it) is not the strongest bin; with `harnessSingleSampleFeedback = true` the Nyquist-region peak is ≥ 20 dB higher than with the average → prints both, the control must show the rise.
- **DSP-07:** Saturation 0 render `memcmp`-identical to a render where the saturation branch is compiled out of the path — a harness-only processor flag `harnessSaturationBypass` (default false) that makes `scan` skip the branch unconditionally; the two renders must be byte-identical (the `sat > 0.0f` exact-zero skip is what makes them so). Saturation 1 raises the partial count above −40 dB vs 0 (Sine Product + Ellipse, C4).
- **H2, H5 re-run** green.
- **Depends on:** Tasks 13–15
- **Verify:** `--gate all` exit 0 from `/`; runtime printed (≤ 3 min total budget for the round's `--all`, H3 dominant).

### Task 17 — Phase 2.2 commit
```bash
git branch --show-current && git status --short
git commit -- plugins/O-Strata -m "feat(O-Strata): Phase 2.2 — pitch-tracked terrain frequency, trajectory feedback, gates"
```

### ━━━ Phase 2.3 — Per-oscillator 2× / 4× oversampling, CPU and aliasing gates ━━━

### Task 18 — `dsp/HalfbandDecimator.h` (ARCH Core 5, Decision 5; RESEARCH §2.2; plan Decision 1)
- **Create:** `Source/dsp/HalfbandDecimator.h` — `struct HalfbandCoeffs { std::array<float, 3> direct2; std::array<float, 2> delayed2; std::array<float, 2> direct4; std::array<float, 1> delayed4; double latency2, latency4; }` designed **once per processor** in `TerrainOscillator::prepare` via `FilterDesign<float>::designIIRLowpassHalfBandPolyphaseAllpassMethod (0.06f, −70.f)` and `(0.15f, −60.f)` with the RESEARCH §2.2 extraction (`directPath[i]->coefficients[0]`, `delayedPath[i + 1]->coefficients[0]` — `i = 1` skips the delay element), `jassert` on the section counts (5 = 3 + 2; 3 = 2 + 1), latency from the DC group-delay sums (`L2 ≈ 1.259`, `L4 ≈ 1.743`), `jassert (L2 <= 2.0 && L4 <= 2.0)`. The design allocates (`juce::Array`) — `prepare` only; the coefficients are shared as a `static` built under `std::call_once` keyed on nothing (they are rate-independent halfbands). `struct HalfbandStage2 { float vDirect[3], vDelayed[2], prevOdd; float down (float even, float odd, const HalfbandCoeffs&); void reset(); }` — one-state transposed allpass per section at the decimated rate (`out = a·in + v; v = in − a·out`), `y = 0.5·(directOut + prevOdd)`, `prevOdd = delayedOut` (mirrors `Oversampling2TimesPolyphaseIIR::processSamplesDown`). `struct HalfbandStage4 { HalfbandStage2-shaped 2 + 1 sections; }` chained: 4 sub-samples → stage4 twice → 2 samples → stage2 once.
- **Depends on:** Phase 2.2 committed
- **Verify:** a scratch check (scratchpad) feeds a 1 kHz + 30 kHz mix at 96 k into `down` and shows the 30 kHz component ≤ −68 dB after decimation to 48 k; DSP-05 grep on the header hits only `prepare`-marked lines.

### Task 19 — Oversampling loop in `getNextSampleStereo` + Quality switch (ARCH Core 1, 5; plan Decision 18)
- **Modify:** `Source/dsp/TerrainOscillator.h/.cpp` — per partial: `HalfbandStage2 hb2; HalfbandStage4 hb4;`; `OS = quality == X4 ? 4 : quality == X2 ? 2 : 1`; per base sample, per partial: run `scan` OS times with phase increment `phaseIncrement / OS` (the Sync / Window master phases advance per sub-sample with `masterInc / OS`), **all setter values held** over the sub-samples, feedback per sub-sample with `aEff` (Task 14's OS feed), feed the sub-samples to `hb4`/`hb2`, take one output. `kernelBypass` → scan returns 0 and the decimator is skipped (H7 baseline). Quality change mid-note: Decision 18's 64-sample equal-gain crossfade, new path's stage states reset. `updateBlockRate` recomputes `aEff` with the current OS.
- **Depends on:** Task 18
- **Verify:** H6 / H8 / crossfade gate (Task 25); H1 still ≤ −80 dB at Bandlimited (1×); H9 at 2×.

### Task 20 — Latency report +1 (plan Decision 1)
- **Modify:** `Source/PluginProcessor.cpp` — both sites (`setLatencySamples (pDistBypass->load() > 0.5f ? 0 : …)` in `prepareToPlay`, and `wantedLatency` in `timerCallback`) → `(distortion bypassed ? 0 : distortionLatency) + 1`; comments rewritten: `// +1: the terrain oscillators' halfband decimators (1.26 samples at 2×, 1.74 at 4×, 0 Bandlimited — ARCH Decision 5, RESEARCH §2.2); constant because Quality is per-oscillator and automatable.`
- **Depends on:** none
- **Verify:** harness `--gate latency` (Task 25) reads `getLatencySamples()` after `prepareToPlay` for every Quality pair {Bandlimited, 2×, 4×}² × distortion {bypassed, on}: equals `distortionLatency + 1`.

### Task 21 — Shared ramp rows in the processor (plan Decision 5; RESEARCH §2.4)
- **Modify:** `Source/PluginProcessor.h/.cpp`, `Source/StrataVoice.h/.cpp`
- Processor: `std::array<juce::SmoothedValue<float>, 24> baseRamps; juce::AudioBuffer<float> rampBuffers; int rampCapacity = 0;` (+ cached pointers to the 22 base params); `prepareToPlay`: `reset (fs, harnessRampSeconds)` ×24, `rampBuffers.setSize (24, samplesPerBlock)`, `rampCapacity = samplesPerBlock`; `processBlock` before `synthesiser.renderNextBlock`: `fillRampRows (numSamples)` — `jassert (numSamples <= rampCapacity)`, `n = jmin (numSamples, rampCapacity)`, per row `setTargetValue` (rows 6 / 17 log2; 22 / 23 the CC atomics), then `isSmoothing() ? per-sample getNextValue : FloatVectorOperations::fill`; `const float* getRampRow (int i) const`.
- Voice: delete `ramps` and its `prepare` / `startNote` / `setTargetValue` lines (Task 5); at the top of `renderNextBlock` cache `const float* row[24]` from the processor; per sample read `row[k][jmin (sample, rampCapacity − 1)]` in place of `ramps[k].getNextValue()`; the note-on `setCurrentAndTargetValue` reset is gone (rows are always current).
- Both `harnessRampSeconds` (Task 12's H5 negative control) and the H5 gate itself now act on the processor ramps.
- **Depends on:** Task 12's H5 in place (so the swap is gated)
- **Verify:** H5 green (both arms); H9 at 64 / 256 / 1024 identical (the absolute-index rule is what this checks — sub-block splits at MIDI events are exercised by placing the note-on at sample 37 of block 0 in one H9 row).

### Task 22 — fb = 0 block skip (plan Decision 17)
- **Modify:** `Source/StrataVoice.cpp` (`updateBlockRate` feed: `oscA.setFeedbackActive (rowMax (row 9) > 0 || modMatrix.isDestinationRouted (ModDest::OscAOrbFeedback))`), `Source/dsp/TerrainOscillator.cpp` (skip the displacement arithmetic when inactive; state held).
- **Depends on:** Tasks 6, 21
- **Verify:** H3 `memcmp` row still byte-identical (the skipped and the `0·finite` paths agree); H7 records the saving.

### Task 23 — WAV export grid for the listening pass (CONTEXT D3)
- **Modify:** `tests/render-harness/main.cpp` — `--gate export`: 6 terrains × 11 orbits × {C2, C4, C6}, 2 s each, defaults, 2×, 24-bit WAV via `juce::WavAudioFormat` into `STRATA_EXPORTS_DIR/<terrain>-<orbit>-<note>.wav`; writes `golden/round-a-grid.sha256` (one line per file); the `.sha256` is tracked, the WAVs are not (Decision 20). Listening happens in Stage 4 (QUAL-04).
- **Depends on:** Task 19
- **Verify:** `ls tests/exports | wc -l` = 198; `git status --short tests/` shows only `golden/round-a-grid.sha256`.

### Task 24 — H7 CPU gate (PERF-02 as amended, D2; RESEARCH §2.4 / §2.5; ARCH H7)
- **Modify:** `tests/render-harness/main.cpp` — `--gate H7`: default patch, 16 voices held (16 note-ons at sample 0 — the `Init` patch with `ampSustain` at its default is fine here; the gate is CPU, not spectrum), unison 1, Quality 2×, 48 kHz, block 512, 10 s of `processBlock` under `ScopedNoDenormals` (the plugin's own), wall-clock via `std::chrono::steady_clock`, **best of 3**; the same with `harnessTerrainKernelBypass = true`; prints `total = x %`, `baseline = y %`, **`delta = x − y %`**; gate **delta ≤ 12 %**. Reported rows (not gated): unison 4, 4×, Bandlimited (= 1× analytic in Round A; the true Bandlimited row is Round B), and — per Decision 5 — the per-voice-ramp figure taken from the Phase 2.2 commit (the executor checks out `plugins/O-Strata/Source` at the 2.2 commit into a scratch worktree **only for the measurement**, or simply records the 2.2-tree H7 number **before** Task 21 lands — the latter is the plan: **run H7 once at the end of Task 20, record it, then land 21–22**). Release build, machine otherwise idle, printed with the CPU name.
- **Fallback ladder** (CONTEXT D2, in order, each re-measured and recorded): the two savings are already in (Tasks 21, 22) → float terrain phase path (**kernel input only**; the accumulator stays double — RESEARCH §2.6 accumulator precision) → dirty-flag smoothing (fill only rows that are routed or whose target moved) → 15 % with Taylor's sign-off (**stop and ask** — the only blocking question in this plan).
- **Depends on:** Tasks 19–22
- **Verify:** delta ≤ 12 % printed; the outcome line goes into SUMMARY verbatim.

### Task 25 — H6 aliasing, H8 across Quality, crossfade click, latency, rate invariance (QUAL-01, FUNC-06, DSP-03, DSP-05)
- **Modify:** `tests/render-harness/main.cpp`, `README.md`
- **H6** (Decision 2): `prepareToPlay (48059.7333, 512)`, N = 65536 after warm-up + 0.35 s discard, `preFilterTap`, `ampSustain` 1, Track 1, defaults, seed pinned, Phase 0.25; every terrain × orbit (66) × {A2, A4, A6} at **2×**: nonharm/max ≤ −60 dB; the same grid at **4×** reported (≤ −90 dB expected, not gated in Round A — ROADMAP says "reported"); band 0–22 kHz; prints nonharm/fund alongside and the highest harmonic index above −100 dB. Rate rows: Sine Product + Ellipse and Epitrochoid 7 + Ridged Cosines at fs = 440·65536/652 and 440·65536/300: within 3 dB of the 48 k row. **If any 2× row fails:** apply ARCH Risks "Aliasing at 2×" fallbacks in order — (1) shallower default lobes (d-range) — **not allowed** without a spec change (defaults are locked): skip to (2) `Auto` Quality appended at index 3 (the only sanctioned list change, CONTEXT) selecting 4× for K ≥ 6, then (3) the F_eff harmonic-budget clamp; record which rung in SUMMARY. Executor stops and reports if (2) is reached (a list change is a spec touch even when sanctioned — Taylor confirms).
- **H8 across Quality:** arm around every block while Quality toggles Bandlimited → 2× → 4× every 50 ms under 16 held notes for 5 s, plus terrain / orbit changes: zero; foreign tally printed.
- **Crossfade click:** Quality 2× → 4× at t = 1 s mid-note, default patch: no |Δy| > 0.1 outside the 64-sample window after the switch; prints the max step inside and outside.
- **Latency** (Task 20's gate).
- **H9 at 2×** and **H1 at Bandlimited** re-run.
- **Depends on:** Tasks 18–24
- **Verify:** `--gate all` (H1–H9 + tuning + smoke + latency + crossfade) exit 0 from `/`, runtime printed ≤ 3 min.

### ━━━ Round A close ━━━

### Task 26 — Install + validate (COMPAT-01 regression)
- `./scripts/build-and-install.sh O-Strata` (background; Phase 4 sweeps both variants), `pluginval --strictness-level 10` on VST3 + AU, `auval -v aumu OuSt <manu>` (background, 2-minute budget — cold auval rescans; memory `pattern_cold_auval_after_install_rescans_registry`); `git status --short plugins/O-Prism` empty.
- **Depends on:** Task 25
- **Verify:** pluginval ×2 SUCCESS, auval SUCCEEDED (or the rescan time noted and re-run).

### Task 27 — Docs: CHANGELOG, comments, PLUGINS.md, `SUMMARY.md`
- **Modify:** `plugins/O-Strata/CHANGELOG.md` (v1.0.0 Unreleased: "Live wave-terrain oscillator (Phases 2.1–2.3): …" + a "Latency: +1 sample constant" line), stale comments naming the wavetable path (`grep -rn 'wavetable\|Wavetable\|GeometryBakeScheduler\|baked' Source/ CMakeLists.txt` → refresh each hit's comment; the `PluginProcessor.h` "Placeholder table (Stage 1)" block is gone with Task 7), `PLUGINS.md` row (🚧 Stage 2, date), `.planning/STATUS.md` (execute ✓ Round A, H6 / H7 outcome line).
- **`stages/2-dsp/SUMMARY.md`** (complex template): every measured number — H1 dB per row, H2 grid pass count + `presets: 1 (Init)`, H3 grid counts + runtime, H4 six partial counts, H5 max steps both arms + sideband dB, **H6 table (66 × 3 at 2× and 4×, rate rows)**, **H7 total / baseline / delta, unison 4 / 4× / 1×-analytic rows, per-voice vs shared ramp figure, ladder rung applied**, H8 counts + foreign tally + the Decision 4 coverage statement, H9 max |Δ|, latencies L2 / L4 from `--print-only`, tuning ≤ 0.5 cent per key, smoke 6/6, `--all` runtime; the **H6 / H7 outcome section** Round B's plan reads first; corrections carried (RESEARCH §3 + any new).
- **Depends on:** Task 26

### Task 28 — Phase 2.3 commit (path-scoped, no tag)
```bash
git branch --show-current && git status --short
git add plugins/O-Strata/tests/render-harness/golden/round-a-grid.sha256   # if new
git commit -- plugins/O-Strata PLUGINS.md -m "feat(O-Strata): Phase 2.3 — per-oscillator 2x/4x halfband oversampling, aliasing + CPU gates"
```
`git diff --stat HEAD -- plugins/O-Prism` empty. **No tag** (memory `feedback_never_tag_unless_publish`).

---

## Parallelism

| Wave | Tasks | Notes |
|---|---|---|
| 1 | 1, 2, 3, 6 | independent files; 1 must precede any deletion |
| 2 | 4 | needs 2, 3 |
| 3 | 5, 7, 8 | need 4; 7 needs 5 for `setVoiceIndex` / capture plumbing |
| 4 | 9 → 12 → 10 → 11 | `main.cpp` before configure; delete after the harness compiles against the new class; then build + parity + pluginval; **commit 2.1** |
| 5 | 13, 14, 15 → 16 → 17 | **commit 2.2** |
| 6 | 18 → 19, 20 → **H7 per-voice measurement** → 21, 22 → 23, 24, 25 | order inside the wave matters (Decision 5's two H7 numbers) |
| 7 | 26 → 27 → 28 | **commit 2.3** |

Builds and every `--all` run go to the background (executor 600 s watchdog); the executor polls, never blocks.

---

## Files

**Create:** `Source/dsp/Orbits.h`, `Source/dsp/Terrains.h`, `Source/dsp/TerrainOscillator.h`, `Source/dsp/TerrainOscillator.cpp`, `Source/dsp/HalfbandDecimator.h`, `tests/render-harness/main.cpp`, `tests/render-harness/README.md`, `tests/render-harness/reference/theta_reference.h`, `tests/render-harness/reference/spectrum.h`, `tests/render-harness/fixtures/test-tunings/just-major.scl`, `tests/render-harness/golden/round-a-grid.sha256`, `tests/.gitignore`, `.planning/stages/1-foundation/smoke/.gitignore`, `.planning/stages/2-dsp/SUMMARY.md`
**Modify:** `Source/StrataVoice.h/.cpp`, `Source/PluginProcessor.h/.cpp`, `Source/PluginEditor.cpp` (two stub bodies only), `Source/dsp/ModulationMatrix.h/.cpp`, `CMakeLists.txt`, `CHANGELOG.md`, `.planning/STATUS.md`, `PLUGINS.md`
**Delete:** `Source/dsp/WavetableOscillator.h/.cpp`, `Source/dsp/WavetableData.h`, `Source/dsp/WavetableGenerator.h/.cpp`
**Never touched:** `parameter-spec.md`, `BRIEF.md`, `research/ARCHITECTURE.md`, `ROADMAP.md`, `Source/ui/public/**`, `plugins/O-Prism/**`, `research/**` (the bench stays where it is).

---

## Success criteria (CONTEXT "Stage 2 test criteria" Round A rows + ROADMAP 2.1–2.3 as amended by Decisions 1–24)

- [ ] **H1:** Identity-X + centred unit circle vs the DC-blocked analytic cosine ≤ −80 dB for Off / Sync 0.5 / Bend 0.5 / Window 0.5 / FM 0.5 × unison {1, 4}, Phase 0.25, at 1×; Bend 0.6 control fails; C4 ± 1 cent (FUNC-01)
- [ ] **FUNC-02 / 03:** every terrain's Mod X / Mod Y and every non-Ellipse orbit's Orbit Mod: centroid delta ≥ 5 % (Ellipse printed as inert); Orbit Size 0.05 → 1.0 centroid monotone non-decreasing
- [ ] **H2:** 66 terrain × orbit pairs at defaults pass (h1 ≥ max − 6 dB in ≥ 95 % of windows); `Init` passes; `presets: 1 (Init)` printed; Centre (0, 0) / Aspect 1 / Sine Product control fails (DSP-06)
- [ ] **H5:** 22 destinations + ModWheel route stepped 10 % / 100 ms → max step ≤ 0.1; 0 s ramp control fails; 40 Hz LFO → Centre X shows ± 40 Hz sidebands (FUNC-05, QUAL-02)
- [ ] **H8:** zero counted allocations across terrain / orbit changes and Bandlimited → 2× → 4× flips under held notes; foreign tally printed; coverage statement in SUMMARY (DSP-05, PERF-01)
- [ ] **H9:** 64 / 256 / 1024 identical to ≤ 1e−5 with the seeded phase (incl. the note-on-at-sample-37 row); unseeded path documented as non-deterministic
- [ ] **H4:** Track 1 partial counts at C2 / C4 / C6 differ ≤ 2 at F = 1 and F = 4; Track 0 / F = 8 / C6 control raises non-harmonic energy ≥ 20 dB (DSP-01)
- [ ] **H3:** full grid finite, |y| ≤ 1, |DC| < 1e−3; fb = 0 `memcmp`-identical to `feedbackPathEnabled = false` on 66 pairs; Damp 0 / Feedback 1 no Nyquist dominance and the `singleSampleFeedback` control shows ≥ 20 dB more (FUNC-04, DSP-04 clamp half)
- [ ] **DSP-07:** Saturation 0 `memcmp`-identical to the setter never called; Saturation 1 raises the partial count
- [ ] **H6:** 66 × {A2, A4, A6} at 2× nonharm/max ≤ −60 dB at fs = 48059.73 Hz; 4× grid reported; 44.1 k / 96 k rows within 3 dB (QUAL-01 2× half)
- [ ] **H7:** oscillator delta ≤ 12 % (16 v × 2 osc, unison 1, 2×, 48 kHz, 10 s, Release, best of 3); total, baseline, delta, unison 4 / 4× / 1× rows and the per-voice vs shared ramp delta printed; ladder rung recorded (PERF-02 as amended)
- [ ] **Quality switch:** no step > 0.1 outside the 64-sample crossfade; `getLatencySamples()` = distortion latency + 1 in every combination, from `prepareToPlay` and the timer (FUNC-06, DSP-03)
- [ ] **Tuning:** Scala + 31-EDO, ≥ 5 keys, ≤ 0.5 cent; params.tsv diff empty (FUNC-09, FUNC-10)
- [ ] **`--smoke`:** [1]–[6] pass with [4] inverted ([5] under `--with-disk`)
- [ ] **Harness hygiene:** every README command runs identically from the repo root and from `/`; no `runDispatchLoopUntil` in `main.cpp`; `tests/exports/` and WAV goldens ignored, `.sha256` tracked
- [ ] **Deletion:** `grep -rn 'Wavetable\|WaveShape' Source/ CMakeLists.txt` empty; `retire (std::unique_ptr<Retirable>)` in place; the two native stubs answer `"[]"` / `"{}"`
- [ ] **DSP-05 grep:** no `pow` / `std::function` / `new` / `make_unique` in `TerrainOscillator.cpp`, `Orbits.h`, `Terrains.h`, `HalfbandDecimator.h` outside lines commented as `prepare` / `setUnison` cache / `updateBlockRate`
- [ ] **Round A exit:** H1–H9 + tuning + smoke + latency + crossfade green in one `--all` run ≤ 3 min; pluginval strictness 10 VST3 + AU SUCCESS and auval SUCCEEDED on the 2× build; three path-scoped commits with the ROADMAP messages; O-Prism untouched; H6 / H7 outcome section in SUMMARY.md
- [ ] **No parameter, range, list or default changed** — unless the H6 rung (2) `Auto` Quality entry was appended at index 3 with Taylor's confirmation (then the params.tsv diff is exactly the two `osc?Quality` rows)

---

## Out of scope (do not do in this round)

- Bandlimited mode proper (`ChebyshevSet`, projector, scheduler, D_max taper, readout atomics) — Round B 2.4; in Round A `Quality::Bandlimited` is the analytic path at 1×
- PNG path (`TerrainImage`, blur, edge, import API), H10 / H11, ASan runs, the message-loop `pump` helper — Round B 2.5
- `CycleCapture` readers, `terrainCycle` / `terrainPlayhead` pushes, ≋ view — Stage 3 (Round A writes the ring only)
- Page edits, i18n keys, removal of the two native stubs — Phase 3.1
- `terrainImports` child content, presets beyond `Init`, CI workflow edit for the harness target — Stage 4
- Any ARCHITECTURE / ROADMAP / spec edit; any parameter change beyond the sanctioned `Auto` fallback
- The *Terrain* listening pass and the raw-feedback damp-law blend (D3) — Stage 4 if QUAL-04 asks

---

## Requirements traceability (Round A)

| ID | Task(s) | Gate |
|---|---|---|
| FUNC-01 | 1, 4, 12 | H1 |
| FUNC-02, FUNC-03 | 2, 3, 12 | centroid gates |
| FUNC-04 | 14, 16 | H3 + Nyquist control |
| FUNC-05, QUAL-02 | 5, 12, 21 | H5 + sidebands |
| FUNC-06 (2× / 4×), DSP-03 | 18–20, 25 | crossfade, latency, H8 Quality |
| FUNC-09, FUNC-10 | 11, 12 | params.tsv diff, `--gate tuning` |
| DSP-01 | 13, 16 | H4 |
| DSP-04 (clamp half) | 14, 16 | H3 |
| DSP-05, PERF-01 | 4, 12, 25 | grep gate, H8 |
| DSP-06 | 12 | H2 |
| DSP-07 | 4, 16 | Saturation rows |
| PERF-02 (D2) | 21, 22, 24 | H7 delta |
| QUAL-01 (2× half) | 19, 25 | H6 |
| COMPAT-01 (regression) | 11, 26 | pluginval ×2, auval |
