# Stage 2: DSP - Research

**Date:** 2026-08-20
**Scope:** Implementation-level research for the four DSP phases (2.1–2.4). ARCHITECTURE.md is the immutable contract; this document verifies APIs against local JUCE 8.0.14 source, identifies house patterns/files to reuse, and collects pitfalls. It does not re-derive algorithms already specified in ARCHITECTURE.md.

**Sequencing note (carried from CONTEXT.md):** research ran ahead of sequence — Stage 1 has no artifacts yet. Stage 2 execution must not start until Stage 1 is complete and verified.

---

## 1. JUCE API Verification (read from /Users/taylorbrook/JUCE, 8.0.14)

### 1.1 `juce::Interpolators` (downsampler workhorse)

`juce_audio_basics/utilities/juce_Interpolators.h` + `juce_GenericInterpolator.h`:

- `Interpolators::Lagrange` = `GenericInterpolator<LagrangeTraits, 5>` (4 taps used); `Interpolators::WindowedSinc` = 200-point (base latency 100 samples — too heavy; use Lagrange as planned).
- `int process(double speedRatio, const float* in, float* out, int numOutputSamplesToProduce)` — `speedRatio` = **input samples consumed per output sample** (>1 = decimation, i.e. hostRate/consoleRate for the downsample). **Return value = input samples consumed** — the FIFO advance must use this return, not a precomputed count.
- ⚠ The wrap-around overload's return value is `(consumed + wrap) % wrap` — a modulo position, NOT a consumed count. Use the non-wrap overload with a linear scratch buffer.
- ⚠ Caller must guarantee `in` holds ≥ `speedRatio * numOut` samples; house precedent (O-MicrotonalSampler `SampleLoader.cpp:185-188`) pads a cleared guard sample because the interpolator can over-read ~1 sample.
- It is a **stream** resampler: fixed-size internal history array, state carries across calls, no allocation, RT-safe. One instance per channel (non-copyable). `reset()` zeroes history and sets `subSamplePos = 1.0`. Never reset per block (phase continuity = PERF-02).
- `getBaseLatency()`: Lagrange = 2.0 samples (input domain). Budget into the latency figure.

### 1.2 `juce::dsp::DryWetMixer` — **critical constructor gotcha**

`juce_dsp/processors/juce_DryWetMixer.h/.cpp`:

- **MUST construct with `DryWetMixer<float> mixer { maxWetLatencySamples }`.** A default-constructed mixer sizes its internal Thiran `DelayLine` to 0 and `setWetLatency()` becomes a **silent no-op** (`pushDrySamples` takes the plain-copy branch). O-Prism has this exact latent bug (`DistortionProcessor.h:52`); O-Bitrot does it right: `static constexpr int kMaxWetLatencySamples = 8192;` + `dryWetMixer { kMaxWetLatencySamples }` (`O-Bitrot/Source/PluginProcessor.h:201,226`).
- Call order in `prepareToPlay` (proven in O-Bitrot `PluginProcessor.cpp:1263-1310`): compute latency → `jassert(latency <= kMax)` → `setLatencySamples(latency)` → prepare all DSP stages (hand the same integer to the stage that creates the delay) → `dryWetMixer.prepare(spec)` → `setMixingRule(linear)` → `setWetLatency((float) latency)` **after** prepare → `setWetMixProportion(...)`.
- `setWetLatency` takes float; `pushDrySamples`/`mixWetSamples` must be 1:1 per block, channel count ≤ prepared (jasserts).
- Gain smoothers use a fixed **0.05 s** ramp set in `reset()` — bit-exact null tests need a >0.05 s warm-up skip (dsp-agent memory L101). Linear rule IS bit-transparent at settled proportion.
- ⚠ **NaN poisons the dry path forever**: the Thiran delay computes `alpha*(x−v)`; at integer delay alpha==0 but `0*NaN == NaN` (dsp-agent memory L118). Mitigation (implemented in O-Bitrot `PluginProcessor.cpp:1364-1379`): scrub non-finite *input* samples to 0 at the `processBlock` boundary; finite samples untouched so bit-exactness holds.
- `setLatencySamples()` triggers `updateHostDisplay()` — **not RT-safe from the audio callback**; only call it in `prepareToPlay` (O-AnalogSaturation `PluginProcessor.h:168-174` documents the atomic+AsyncUpdater workaround, which O-Emulator doesn't need since latency is constant).

### 1.3 AA filter: 8th-order Butterworth from `ArrayCoefficients`

`juce_dsp/processors/juce_IIRFilter.h`:

- `ArrayCoefficients<float>::makeLowPass(sampleRate, freq)` and **`makeLowPass(sampleRate, freq, Q)`** both exist, returning raw 6-element `{b0,b1,b2,a0,a1,a2}` arrays — no allocation (the `Coefficients::makeXXX` Ptr versions allocate; per house memory use ArrayCoefficients only). Assigning the array to a pre-allocated `IIR::Filter`'s coefficients normalises to the 5-element stored form.
- JUCE has **no 8th-order factory** usable RT-safe. `FilterDesign::designIIRLowpassHighOrderButterworthMethod` exists (`juce_dsp/filter_design/juce_FilterDesign.h:167,183`) but allocates (`ReferenceCountedArray` of `new Coefficients`) — prepare-time only.
- Simplest robust approach: 4 cascaded biquads at the same cutoff with the Butterworth pole Qs, reproduced allocation-free from `juce_FilterDesign.cpp:571-579`:
  `Q_i = 1 / (2·cos((2i+1)·π/16))` for i = 0..3 → **Q ≈ 0.50980, 0.60134, 0.89998, 2.56292**.
  Feed each to `ArrayCoefficients<float>::makeLowPass(sr, 0.45·consoleRate, Q_i)` in `prepareToPlay` (and at the crush AA-open breakpoints — precompute the small coefficient set, as ARCHITECTURE.md specifies).
- Crush ≥ 80% "AA opens": precompute coefficient sets at a few cutoffs and crossfade/step between them — never call any `make*` returning a Ptr on the audio thread.

### 1.4 Output stage: `juce::dsp::FirstOrderTPTFilter`

`juce_dsp/processors/juce_FirstOrderTPTFilter.h`: `setType(lowpass/highpass/allpass)`, `setCutoffFrequency(Hz)` (safe to modulate — TPT structure), `prepare(spec)`, `reset()`, `processSample(channel, x)` (state vector sized by prepare — channel must be < prepared channels), `snapToZero()` must be called manually when doing per-sample processing (denormal purge). Good fit for per-console output LP with Age dulling (corner = per-console × ageScale, updated per control chunk via `setCutoffFrequency`).

### 1.5 `juce::SmoothedValue`

`juce_audio_basics/utilities/juce_SmoothedValue.h`: `reset(sampleRate, rampSeconds)` (also snaps current to target), `setTargetValue`, `getNextValue`, `skip(n)`, `isSmoothing`. Settles **bit-exactly** on target when the countdown ends. ⚠ House gotcha: `skip(numSamples - 1)` underflows when `numSamples == 1` — guard with `jmax(0, …)` (dsp-agent memory L85).

---

## 2. House Patterns and Files to Reuse

### 2.1 Render harness — build in Phase 2.1 (CONTEXT.md decision)

**Primary template: O-Bitrot** (`plugins/O-Bitrot/tests/render-harness/main.cpp`, 6714 lines) — the closest structural match: multi-stage effect with a fixed-frame internal codec (GSM 160-slot ≈ our BRR 16 / SPU-ADPCM 28), constant reported latency mirrored into `setWetLatency`, per-phase invariance re-runs, FNV-1a digest anchors, pathological timeline, FFT probes.

Canonical layout: `plugins/O-Emulator/tests/render-harness/{CMakeLists.txt, main.cpp}`, `juce_add_console_app`, gated by `option(OUARICON_BUILD_TESTS ... OFF)`. Exit code is the gate. No test framework.

**Files to copy/adapt, in order:**

1. **CMakeLists.txt** — copy `plugins/O-Tapestop/tests/render-harness/CMakeLists.txt` (cleanest, carries the EXISTS-guard + `OUARICON_RENDER_HARNESS=1` instrumentation idiom). Key properties baked in: compiles the plugin's `Source/PluginProcessor.cpp` directly (never the WebView editor TU — `pattern_render_harness_breaks_on_webview_editor`); derives version via `get_target_property(... JUCE_VERSION)` never a mirrored literal (`pattern_test_fixture_mirrors_drift_silently`); `JUCE_WEB_BROWSER=0`. ⚠ Use O-Emulator's real CMake **target** name from its Stage-1 CMakeLists, not the folder name (`build_script_target_name_vs_folder`). Mirror the option block from `plugins/O-Tapestop/CMakeLists.txt:105-111` into the plugin CMakeLists.
2. **Scaffold** — O-Bitrot `main.cpp:1-460` (check/bitExact, `noiseAt` position-hashed excitation, `setParam`/`setBaseline`, `renderInto`, `bitIdentical`, `firstDifference`), plus `:735-745` `pathologicalStereo`, `:805-832` `bandEnergy`, `:910-927`/`:1064` `makeProc`/`makeProcAtRate`, `:975-997` `renderChecksum` (FNV-1a), `:1121-1141` `toneAmplitude` (Goertzel — exact-bin lengths only).
3. **Probe bodies to clone:** `F`/`G` (`:1443-1483`) block-size invariance; `B FUNC-02 null` (`:1229`) mix-0% bit-transparent minus latency; `A`/`Z` latency-reported + constant-across-modes; `E` determinism (two fresh instances, same seed); `U QUAL-01` pathological timeline; `R4` (`:6541-6577`) three-way block-size identity with liveness clause.
4. **64/512/4096 sweep shape** — O-Octagon `main.cpp:1643-1685` (probe `AN`): `sizeSets = { {64}, {512}, {4096}, {1,7,64,333,4096} }` each vs a fixed reference, first failing size named in the detail string. No existing harness runs the literal 64/512/4096 triple — adapt this loop. Also O-Octagon `:1990-2025` (probe `AR`) for the explicit **denormal case (1.0e-40f)** and NaN-does-not-latch structure (QUAL-01's denormal clause; Bitrot's timeline lacks it).
5. **Spectral probes** — take `Spectrum` **with `flatness()`** from `plugins/O-simpleSampler/tests/render-harness/main.cpp:236-306` (geometric/arithmetic mean ratio). Flatness is the natural DSP-02 metric: quantization noise + S&H aliasing fill inter-harmonic valleys → flatness rises. `centroid()` for Gaussian-rolloff darkening; `bandEnergy` **ratios against a control render, never absolute magnitudes** (Bitrot probe `W` model).

**Harness conventions (non-negotiable, from the Bitrot file header):**
- `setBaseline()` at the top of every probe — resets ALL params to spec defaults (⚠ neutral ≠ minimum: our `mix` default 100, `crush` 50).
- `setValueNotifyingHost(convertTo0to1(engineeringValue))` only — bare `setValue()` leaves cached atomics stale and tests defaults while reporting green.
- Excitation is a **pure function of absolute sample index** (hash-based `noiseAt(n)`, position-computed sine) — a sequential RNG would give different signals at different block sizes (`pattern_rng_stream_interleave_blocksize`).
- Every potentially-vacuous probe carries a **liveness clause** (`pattern_zipper_sweep_probe_needs_liveness_gate`): magnitude > 1e-4, min-vs-max A/B diff, or deviation-from-dry. A failed liveness gate fails the probe (never skips).
- No wall-clock inside any verdict (`pattern_wallclock_inside_a_stability_verdict`); the only sanctioned wall-clock uses are CPU-ratio probes (Bitrot `P1`, ratio ≤ 0.15).
- Fixed settle length across compared renders (O-Tapestop `prepareAndSettle`, `main.cpp:216-229`) — an absolute-double read phase rounds differently otherwise (a 512-vs-4096 settle pair once diverged by 3e-8).
- Mid-render console-switch probes: use O-Tapestop's `Event`/`renderTimeline` (`main.cpp:237-421`) — chunks clamped so **no `processBlock` call spans an event**, which is what makes automation-carrying invariance probes legal at arbitrary offsets.
- Goldens: prefer **in-source `constexpr uint64` FNV-1a anchors** (no binaries; retired anchors kept and asserted `moved` — `pattern_reanchor_cross_version_digest_probe`). If persisted WAVs are ever wanted, sha256-only per `.gitignore:50-68` policy.
- `#if OUARICON_RENDER_HARNESS` instrumentation accessors (O-Tapestop `PluginProcessor.h:160-175`) to expose FIFO fill / chunk phase / computed latency to probes without shipping it.

### 2.2 Fixed-chunk FIFO / block-size invariance

No shared module exists; three in-repo implementations to model from:

- **Control-grid discipline** — O-Octagon `Source/DSP/GainStage.h:127-133` + `GainStage.cpp:136-153`: `kControlBlock = 64` power-of-two with `static_assert`, `std::uint64_t absoluteSampleCounter` reset only in `prepare()`, walk `phase = counter & (N−1)`, chunk clamped to grid boundary. This is the exact mechanism ARCHITECTURE.md's "fixed-chunk feeder" needs (its header comment names `pattern_block_rate_envelope_breaks_blocksize_invariance` verbatim).
- **Input/output FIFO with FIFO-owned latency** — O-Texture `Source/DSP/OverlapAddProcessor.h` (130 lines, header-only): boundary-clamped inner `while`, `getSamplesUntilNextHop()`, and `static constexpr int getLatencySamples()` — the FIFO owns its latency figure, which composes cleanly into the single computed total. Also O-SpectralShaper `STFTProcessor.h/.cpp` (`processSample` push/read + hop-boundary shift; latency = FIFO size, `PluginProcessor.cpp:676`).
- **Fixed-frame codec chain + constant-latency contract** — O-Bitrot `Source/dsp/CodecStage.h` (`kFrameLen = 160` slots, `frameSlot` counter, header comment `:82-93` states the constant-latency-in-every-codec-state contract). This is the direct model for the BRR 16-sample / SPU-ADPCM 28-sample block fill counters and for holding latency constant across console modes.

### 2.3 Prior art relevant to specific components

- **Gaussian 4-tap interpolator / BRR / ADPCM: zero prior art in the repo** — all net-new code (module registry confirmed: no DSP-primitive modules exist; the 11 modules are UI/persistence/tuning/instrument-unit scoped).
- Hand-rolled 4-point Lagrange (stateless, Horner form): `plugins/O-GrainScatter/Source/dsp/LagrangeInterpolation.h:22-38` — reference if a random-access interpolator is ever needed; note O-Tapestop `VarispeedVoice.h:38-49` documents that `juce::Interpolators` are *stream* resamplers and wrong for random access. O-Emulator's use IS streaming, so `Interpolators::Lagrange` is correct as planned.
- Reverb topology background: `research/reverb-comprehensive-research.md` (comb/allpass architectures, feedback stability); `research/delay-effects-comprehensive-guide.md` (circular buffers, feedback networks).
- Output-stage nonlinearity: `troubleshooting/dsp-issues/analog-effects-modeling-complete-guide.md` + `research/circuit-modeling-fundamentals.md`.
- Crossfade/click prevention: `research/dsp-click-prevention-debugging.md` (FUNC-04).
- Degradation DSP survey: `research/glitch-effects/degradation-dsp-deep-dive.md` (bit-depth reduction, SR reduction with fractional hold, ZOH sinc droop, zipper-free sweeping rules).

---

## 3. Pitfalls (troubleshooting KB + agent memory)

Highest-value file: **`.claude/agent-memory/dsp-agent.md`** — the O-Bitrot entries (L101–121) are the same problem shape (codec round-trip + dry/wet latency + invariance). Load-bearing entries beyond those already cited:

- **L119 — constant-latency alignment across sub-paths:** every blend input needs its own alignment ring **before** blending; you cannot blend first and delay after when structural delays differ. Applies to the reverb return (its 22.05 kHz round-trip has its own delay) joining the direct path — align before summing, and to the console crossfader (both engines must present identical latency — they do by construction, worst-case constant).
- **L116 — RNG invariance:** per-sample RNG is block-size-safe only as unconditional consumption on a dedicated per-subsystem stream (pure function of consumed-sample count). Age noise / hum / drift each get their own stream, consumed every sample regardless of age value.
- **L110 — phased-stage determinism:** new stochastic stages (Phase 2.4's age bed) run their RNG unconditionally from Phase 2.1's skeleton onward if digest anchors are recorded early — or gate audibility only via exact 0.0/1.0 gain rails so earlier-phase anchors stay bit-identical. Decide when recording the first anchor.
- **L114 — zipper probes on quantize/hold stages:** can't bound raw sample deltas (the crush IS a stepper) — measure the control trajectory instead. Shapes the Phase 2.4 smoothing-audit probes.
- **L120 — latency xcorr probes** must budget IIR passband group delay (~12–15 samples for cascaded Butterworths @48k) on top of grid jitter — relevant to the mix-null and latency probes since our AA filter is an 8th-order IIR.
- **L49 — denormals:** constant `−1e-20` leak injected into filter recurrences beats post-hoc flushing; `ScopedNoDenormals` in `processBlock` regardless. TPT filters: call `snapToZero()`.
- Sticky-NaN guard (memory `pattern_biquad_nan_guard_sticky_silence` + `pattern_envelope_follower_state_sticky_nan`): reverb isfinite guard resets **state only, keeps coefficients** (non-sticky); probe with pathological INPUT (the U-probe timeline), not just pathological params.
- Filter-coefficient staleness (`troubleshooting/dsp-issues/dsp-filters-not-responding-to-parameters-MicroGlitch-20251106.md`): re-derive coefficients when the controlling value changes (age dulling → output LP corner) — cached-value guards must not gate the enabled flag (`pattern_conditional_coeff_update_leaks_enabled_flag`).
- `juce8-critical-patterns.md` primary for Stage 2: **#4** bus config (stereo effect — `BusesProperties` in constructor), **#5** threading (no locks/alloc on audio thread), **#17** modern `prepare`/`process` idiom for every `juce::dsp` object. Negative: **#22** `IS_SYNTH` must NOT be set. `stage-2-patterns.md` is the condensed read-first subset.
- Offline-render divergence (`troubleshooting/dsp-issues/offline-export-cc-dynamics-jumps-stutter-*`): nothing control-rate may live on an Async/message-thread hop; all modulation (drift, smoothing) integrates on the audio thread per-sample or per-fixed-chunk.
- Delay/read ordering (`pattern_grain_read_before_capture_write_blocksize`): FIFO reads latched before the block's writes break at blockSize ≥ D — the fixed-chunk feeder must write-then-consume within the chunk walk.
- Equal-power crossfade for the console switch (`pattern_hann_pair_is_equal_gain_not_equal_power`) — already in ARCHITECTURE.md; the probe should measure the fade-region RMS dip.
- Noise-bed level rate-invariance (`pattern_noise_bed_level_is_rate_dependent`): normalize only the white-fed stage; compute in host-rate domain (already specified); Phase 2.4 has a 44.1/48/96/192 kHz invariance criterion — use `makeProcAtRate`.

---

## 4. Module Opportunities

**None for Stage 2.** Registry (11 modules) has no codec/resampler/reverb/DC-blocker/crossfader/RNG primitives — every DSP component is net-new. Reusable modules (`preset-manager` v1.0.6, `webview-relay-manager`, `resource-provider`, possibly `vu-meter`) are Stage 3/4 concerns.

---

## 5. Requirements Coverage Check

Stage-2 requirements (REQUIREMENTS.md traceability): FUNC-01..04, DSP-01..05, PERF-01..02, QUAL-01. Every one maps to a researched implementation path and at least one probe shape from §2.1:

| Req | Research anchor |
|-----|----------------|
| FUNC-01 (5 distinct modes) | Per-console configs (ARCHITECTURE table) + spectral flatness/centroid probes per mode |
| FUNC-02 (macros live; mix-0% transparent) | Min-vs-max A/B liveness probes; Bitrot probe `B` null shape + 0.05 s warm-up skip + DryWetMixer ctor sizing (§1.2) |
| FUNC-03/04 (reverb everywhere; click-safe switch) | Send-tap architecture; `renderTimeline` event probes; equal-power fade RMS probe |
| DSP-01 (codec round-trips) | Net-new per spec; CodecStage.h frame-fill model; closed-loop = encode against decoded history |
| DSP-02 (fixed rates + authentic interpolation) | `Interpolators::Lagrange` verified (§1.1); Gaussian 4-tap net-new from S-DSP table; flatness/alias-signature probes |
| DSP-03 (output stages) | FirstOrderTPTFilter verified (§1.4); per-mode corner probes via bandEnergy ratios |
| DSP-04 (SPU reverb) | Register model from psx-spx (ARCHITECTURE); isfinite non-sticky guard; impulse/decay probes |
| DSP-05 (age model) | Per-stream RNG (L116), host-rate noise bed, `makeProcAtRate` rate-invariance probe |
| PERF-01 (RT-safe) | ArrayCoefficients only; prepare-time FilterDesign; no Ptr factories on audio thread; code audit |
| PERF-02 (block-size invariant) | Absolute-counter fixed-chunk walk (GainStage model); probe `AN` shape at {64}/{512}/{4096}/ragged |
| QUAL-01 (bounded finite always) | Pathological timeline (probe `U`) + denormal case from Octagon `AR`; input NaN scrub (§1.2) |

---

## 6. Open Items for the Plan Phase

1. **Latency constant:** computable only at implementation time (AA group delay + FIFO chunk + codec block + interpolator history ≈ 100–130 samples @ 48 kHz). Plan must schedule: compute in `prepareToPlay`, one figure, `setLatencySamples` + `setWetLatency` + `kMaxWetLatencySamples` headroom constant sized for 192 kHz.
2. **Digest-anchor timing vs. phased stages (L110):** decide in the plan whether Phase 2.1 records digest anchors (then Phases 2.2–2.4 must add stages behind exact-0.0 gain rails / unconditional RNG), or whether anchors wait until Phase 2.4. Recommend: per-phase anchors with the re-anchor discipline (`pattern_reanchor_cross_version_digest_probe`) since ROADMAP gates each phase on the harness.
3. **Butterworth Q constants** (§1.3) go in a named table with the derivation comment.
4. **Harness target name:** depends on Stage 1's `juce_add_plugin` target name — resolve at plan time from the actual CMakeLists.
5. Crush integer-step micro-fades (5 ms) + control-trajectory zipper probes (L114) — Phase 2.4 detail.

---

## 7. Sources

- Local JUCE 8.0.14 source (all §1 signatures read from headers/impl, paths cited inline)
- `plugins/O-Bitrot/` — harness + codec-stage + DryWetMixer reference implementation
- `plugins/O-Tapestop/`, `plugins/O-Octagon/`, `plugins/O-simpleSampler/`, `plugins/O-Texture/`, `plugins/O-SpectralShaper/` — harness/FIFO patterns
- `.claude/agent-memory/dsp-agent.md` — L11, L49, L85, L101, L110, L114, L116, L118, L119, L120
- `troubleshooting/patterns/juce8-critical-patterns.md` (#4, #5, #17, #22), `stage-2-patterns.md`, `dsp-issues/*`
- `modules/registry.yaml` v1.0.3 — module inventory
- `research/reverb-comprehensive-research.md`, `research/glitch-effects/degradation-dsp-deep-dive.md`, `research/dsp-click-prevention-debugging.md`, `research/circuit-modeling-fundamentals.md`
