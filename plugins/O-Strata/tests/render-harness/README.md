# O-Strata offline render harness (`O-Strata-render-test`)

The Stage 2 DSP gate (ARCHITECTURE "Harness design"; `stages/2-dsp/round-a/PLAN.md`,
`stages/2-dsp/PLAN.md`). A console target built by `ouaricon_add_processor_console` from
`plugins/O-Strata/CMakeLists.txt` (`JUCE_WEB_BROWSER=0`, no editor TU, no UIResources,
`JUCE_MODAL_LOOPS_PERMITTED=1` for the pump). It drives the processor directly —
`createPluginFilter()`, `prepareToPlay`, `processBlock`, `setValueNotifyingHost` by ID,
`getTuningEngine()`, `getTerrainScheduler().runOnceSynchronously()`.

**The `pump` rule (Round B):** the message loop is pumped by exactly one call site,
`pump (ms)`, and `grep -c runDispatchLoopUntil main.cpp` = 1. Only the gates marked
*pumps* below call it (`storm`, the async rows of `scheduler` and `import`); every other
gate is loop-free and publishes Bandlimited sets / images deterministically through
`Instance::syncScheduler()`, which `render()` calls after its warm-up block unless
`RenderSpec::noSync`. Pumping happens only between blocks, never while the allocation
counter is armed.

Exit code = number of failed checks. Every check prints its measured value and its
threshold on one line, e.g. `[H2] SineProduct x Ellipse h1-max worst = -0.0 dB, 100 % windows PASS`.
Negative controls print `FAIL-as-expected` inside a `PASS` line; if a control passes its
inner measurement the line reads `FAIL … (control PASSED — the probe is vacuous)`.

## Build

```bash
cmake --build /Users/taylorbrook/Dev/VST-development/build --target O-Strata-render-test
```

Artefact: `/Users/taylorbrook/Dev/VST-development/build/plugins/O-Strata/O-Strata-render-test_artefacts/Release/O-Strata-render-test`
(the configured tree has `OUARICON_BUILD_TESTS=ON`).

## Run — every command below runs identically from any directory

Fixtures resolve from the compile-time `STRATA_FIXTURES_DIR`
(`plugins/O-Strata/tests/render-harness/fixtures`), never from cwd; `--fixtures DIR`
overrides it for ad-hoc runs.

```bash
H=/Users/taylorbrook/Dev/VST-development/build/plugins/O-Strata/O-Strata-render-test_artefacts/Release/O-Strata-render-test
$H --gate all                 # every gate H1–H11 + the named ones in one run (≤ 3 min budget; `export` excluded)
$H --gate smoke               # Stage 1 checks [1]–[6] on the terrain oscillator ([4] inverted; [5] needs --with-disk)
$H --gate smoke --with-disk   # + the on-disk factory bank check (touches ~/Library/O-Strata/Presets)
$H --gate tuning              # FUNC-09 / FUNC-10: Scala + 31-EDO, 5 keys, ≤ 0.5 cent
$H --gate H1 --gate H2        # any subset
$H --print-only --terrain 3 --orbit 5 --seconds 2 --set oscAOrbFeedback=0.5   # ad-hoc render report
```

Options: `--gate G` (repeatable; `H1..H11 | tuning | smoke | centroids | saturation | decimator | crossfade | latency | clenshaw | scheduler | storm | import | export | all` — `export` is never part of `all`), `--note N`,
`--velocity V`, `--seconds S`, `--terrain I`, `--orbit I`, `--quality I`, `--set id=norm`
(repeatable, normalised 0..1), `--fs F`, `--block B`, `--seed S` (harness phase seed,
default `0x5EED0001`), `--fixtures DIR`, `--export NAME`, `--png PATH` (imports a PNG into
oscillator A for `--print-only`), `--print-only`, `--with-disk`.

## Gates (Round A rows, with their Round B extensions)

| Gate | Requirement | Method | Threshold |
|---|---|---|---|
| `smoke` | Stage 1 carry-over | checks [1]–[6]; [4] **inverted**: LFO1 → OscA Terrain Freq (31) must CHANGE the render, unrouted render = control, LFO1 → Pitch (23) = positive control | [4] max\|Δ\| > 1e−3 |
| `tuning` | FUNC-09, FUNC-10 | `loadScalaFile (just-major.scl)` and `ScaleGenerator::generateEDO (31)` → `setCustomIntervals`; keys 48/55/60/64/67 rendered 1.5 s, fundamental by Hann-DFT phase progression vs `getFrequency (key)` | \|Δ\| ≤ 0.5 cent |
| H1 | FUNC-01 | Identity-X terrain + centred unit circle (Aspect 1, Rot 0, Centre 0, Size 1), Quality Bandlimited (kind 100 never matches a published set → analytic 1×), pre-filter tap, Phase 0.25; rows Off / Sync 0.5 / Bend 0.5 / FM 0.5 (B as modulator) / Window 0.5 × unison {1, 4}; `20·log10 (RMS (y − g·ref) / RMS (ref))`, g least-squares, reference = `reference/theta_reference.h` with the 5 Hz DC blocker; pitch at C4. **Round B row:** a hand-built set c = δ₁,₀ (f = x) published through `publishForHarness` → the real Clenshaw path renders cos θ | ≤ −80 dB; ± 1 cent; Bend 0.6 control fails; Chebyshev identity ≤ −80 dB |
| H2 | DSP-06 | 6 × 11 grid at defaults + `FactoryPresets::build` (prints `presets: 1 (Init)`), C4, 1 s, Hann 8192 / hop 100 ms, h1 ≥ max − 6 dB; on failure prints h1 at the eight neighbouring centres ± 0.1. **Round B row:** the same grid in Bandlimited (Chebyshev sets via the sync path) | ≥ 95 % of windows on 66 / 66 (2×) and 66 / 66 (Bandlimited); centred-circle control fails |
| `centroids` | FUNC-02, FUNC-03 | Mod X / Mod Y per terrain and Orbit Mod per non-Ellipse orbit over {0, .25, .5, .75, 1} (endpoints alone are degenerate for phase-type inputs): centroid delta and the largest level move of any partial h1..h12 above −40 dB; Orbit Size 0.05 → 1.0 in 8 steps | centroid ≥ 5 % **or** a partial moves ≥ 3 dB; monotone non-decreasing (0.5 % / step) |
| H5 | QUAL-02, FUNC-05 | 22 base parameters stepped 10 % every 100 ms (block 480) on the real output at unity gain (A or B alone); zipper = the max sample step in the 10 ms after each instant vs the larger adjacent 100 ms plateau (the absolute 0.1 literal is printed: at unity gain a Terrain Freq of 8 has natural steps of 0.30, and at the default gain staging a raw 0.6 zipper shrinks below 0.1 — the control was vacuous either way); ModWheel route CC1 stepped by 16; `harnessRampSeconds = 0` control; 40 Hz LFO (10 Hz × 2^2 via Velocity → LFO1 Rate) → OscA Orbit CX sidebands | excess ratio ≤ 1.5; control fails; sidebands ≥ −40 dB at h1, h2 |
| H8 | DSP-05, PERF-01 | operator-new family armed around every `processBlock`, terrain / orbit changed every 50 ms under 8 held notes for 3 s. **Round B rows:** the `storm` (Bandlimited set swaps) and the `import` image publish under held notes, armed around every block. Under ASan the counter is compiled out and the rows print `skipped under ASan` | 0 counted; foreign tally printed |
| H9 | block-size invariance | 10 s default patch, Phase 0.25 + seed, blocks 64 / 256 / 1024, plus note-on at sample 37. **Round B row:** both oscillators Bandlimited through the sync path (no swaps during the render) | pairwise max\|Δ\| ≤ 1e−5 |
| H3 | FUNC-04, DSP-04 | Feedback {.25 .5 .75 1} × Damp {0 .5 1} × 66 pairs × {C2 C4 C6}: finite; \|y\| ≤ 1 on the pre-blocker scan sum (`harnessDcBlockerBypass`, 0.5 s); \|DC\| (Hann-windowed) over the last 250 ms of 1 s; Feedback 0 vs `harnessFeedbackPathEnabled = false` memcmp on 66 pairs; Nyquist control at 1× (single-sample form must raise the fs/2 peak ≥ 20 dB somewhere in a terrain × F × note sweep; the decimator removes the oversampled hunt at 2× / 4×) | finite; ≤ 1; < 1e−3; 66 / 66; ≥ 20 dB |
| H4 | DSP-01 (as amended 2026-09-11, Round A verify) | Track 1: partials > −40 dB at C2 / C4 / C6 at F = 1 and F = 4 (Track 0 / 0.5 rows, h1..h6 levels; the original C2 / C4 / C6 spread is printed, not gated — unsatisfiable at F = 4 under ARCH Decision 2); control Track 0 at F = 8 over {Ellipse, Epitrochoid 7} × {C6, C8} | \|C2 − C4\| ≤ 2; C6 ≤ C4; C6 (Track 1) < C6 (Track 0); control rise ≥ 20 dB |
| `saturation` | DSP-07 | Sat 0 vs `harnessSaturationBypass` memcmp; Sat 1 partial count | identical; more partials |
| `decimator` | DSP-03 | designed coefficients + latencies; 1 kHz + 30 kHz at 96 k through `HalfbandStage2` | 1 < L2 < L4 ≤ 2; alias ≤ −68 dB |
| H6 | QUAL-01 | 66 × {A2, A4, A6} exact-cycle (150 / 600 / 2400 cycles in 65536 at fs = 440·65536/600), pre-filter tap, Track 1, 2× gated, 4× and 1× (`harnessChebyshevBypass`) reported; rate rows at 440·65536/652 and /300 | nonharm/max ≤ −60 dB; within 3 dB |
| H6 (Bandlimited) | DSP-02, FUNC-06, QUAL-01 | the same exact-cycle method through the real Chebyshev path (set published by the sync call): **8 exact orbits × 6 terrains × {A2, A4, A6} = 144 rows**; per row nonharm/max and the highest harmonic above −100 dB vs D_max·K from `chebDiagonalCutoff` / `chebDMax` (shared with the oscillator — no fixture mirror); Superellipse / Butterfly / Squarcle reported | ≤ −90 dB; h_max ≤ D_max·K (equality printed) |
| H7 | PERF-02 | 16 voices × 2 osc, unison 1, 2×, default patch, 48 kHz, block 512, 10 s, best of 3, total vs `harnessTerrainKernelBypass`; rows unison 4 / 4× / **Bandlimited (Chebyshev — sets published before timing; reported, Decision 25 fallback only if > 12 %)** | delta ≤ 12 % |
| H8 (Quality) | DSP-05, PERF-01 | Bandlimited → 2× → 4× every 50 ms + terrain / orbit changes under 16 held notes, 5 s, armed | 0 counted |
| `crossfade` | FUNC-06 | 2× → 4× at t = 1 s mid-note (unity gain, tap): max step inside / outside the 64-sample window vs plateau | ≤ 1.5 × plateau |
| `latency` | DSP-03 | `getLatencySamples()` after `prepareToPlay` for 9 Quality pairs × distortion bypass | bypassed = 1, on = distortion + 1 |
| `export` | (listening pass) | 6 × 11 × {C2 C4 C6}, 2 s, 24-bit WAV → `tests/exports/`, checksums → `golden/round-a-grid.sha256` (not part of `--all`; writes files) | 198 files |

## Gates (Round B)

| Gate | Requirement | Method | Threshold |
|---|---|---|---|
| `clenshaw` | (Decision 25) | 1e6 dependency-carried `clenshaw2D` evaluations on a random 153-float set | ns per evaluation reported |
| `scheduler` | DSP-02, PERF-01 | (a) fit % per terrain at F = 1 / 2 vs the Task 3 scratch table and one-sided vs ARCH; (b) Terrain Freq 4 → key F = 2, coefficients memcmp-equal to the F = 2 set; (c) *pumps*: ModWheel → OscA Terrain Freq (amount 0.5), CC1 0 → 127 through `processBlock`, pumped in 10 ms slices until `chebGeneration[0]` advances; (d) LFO1 → OscA Terrain Mod X renders bit-identical in Bandlimited and differs in 2×; (e) `chebApproximate` for Ellipse / Feedback 0.3 / Superellipse / Feedback routed; (f) *pumps*: `prepareToPlay` ×2 + `setStateInformation` at the default patch; (g) *pumps*: the timer's set vs the sync set; (h) `chebPartialsAtC4` Ellipse / Epitrochoid 7 | scratch ± 0.5 %, ≥ ARCH − 2 %; F = 2 + identical; ≤ 120 ms; identical / > 1e−3; 0 / 1 / 1 / 1; `publishCount` 0; memcmp-equal; 16 / 80 |
| `storm` | PERF-01, DSP-05, ROADMAP 2.4 | *pumps*: Bandlimited (both oscillators), 16 held notes, 5 s, block 480, unity gain, tap; `oscATerModX` stepped 0.05 every 20 ms from 0.5 s, `pump (10)` between blocks (real-time pacing so the 50 ms cadence is exercised), armed around every `processBlock`; `chebGeneration[0]` read before every block; then 40 more blocks with `pump (50)` and `pump (700)` ×2 so the reaper runs. A second storm on a single C4 note carries the click metric (a one-voice swap click would hide under a 16-voice sum): max step outside the 64-sample windows after each swap over [0.5, 5) s vs the plateau max step over [0.1, 0.5) s | 0 allocations; finite; ≥ 20 swaps; every free ≥ 2 block generations after its retire (FIFO); `liveCount` = 2 and `retired` = 0 after the drain; ≤ 1 non-cancelled job in flight; superseded keys + cancelled jobs > 0; click ratio ≤ 1.5 |

## Allocation counter coverage (H8)

Measured via the replaced `operator new` family (copied from O-Octagon's probe AO with its
thread filter): **covered** = every new-expression on the audio thread; **not covered** =
HeapBlock / malloc paths (`AudioBuffer`, `Array`, `MidiBuffer`, `ReferenceCountedArray`,
`MemoryBlock`) and the `AsyncUpdater` post path — grep + inspection. `MidiBuffer::ensureSize (256)`
runs before arming and one warm-up `processBlock` absorbs the first-block tuning async post;
the five tuning parameters are never touched while armed.

## Harness-only switches (processor atomics, never parameters)

`harnessPhaseSeed`, `harnessFeedbackPathEnabled`, `harnessTerrainKernelBypass`,
`harnessSingleSampleFeedback`, `harnessPreFilterTap`, `harnessSaturationBypass`,
`harnessDcBlockerBypass`, `harnessRampSeconds`, `harnessTerrainOverride[2]` — see
`PluginProcessor.h`. Diagnostics: `STRATA_H1_DUMP=1` prints the H1 error envelope,
`STRATA_H3_DUMP=1` a 2 s DC trace for every H3 DC failure.

## Files

`main.cpp` (single TU), `reference/theta_reference.h` (θ pipeline copied from the deleted
wavetable oscillator at `efae0bfc`), `reference/spectrum.h` (FFT / exact-cycle / windowed
helpers adapted from the terrain bench's own code), `fixtures/test-tunings/just-major.scl`,
`golden/*.sha256` (tracked; WAVs and JSON are gitignored via `tests/.gitignore`),
`tests/exports/` (gitignored).
