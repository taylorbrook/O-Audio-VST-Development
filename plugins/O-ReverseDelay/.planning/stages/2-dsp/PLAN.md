# Stage 2: DSP — PLAN

**Plugin:** O-ReverseDelay
**Stage:** 2-dsp
**Phase:** plan — complete
**Date:** 2026-07-23
**Inputs:** ARCHITECTURE.md (immutable contract), CONTEXT.md (D4–D6), RESEARCH.md, ROADMAP.md phases 2.1–2.3

## Goal

Implement the complete DSP engine for O-ReverseDelay: granular reverse smear over a
3.5 s capture ring (reverse read law D+2n), damped tanh-stable feedback through the
shared capture buffer, tempo sync, per-grain width spread, and equal-power mix.
Every acceptance criterion is a hard pass/fail assertion in an offline render
harness (D5); phase advancement requires harness green; git commit per phase.

Three roadmapped phases: **2.1** core reverse wet path (feedback OFF) →
**2.2** feedback + damping + stability → **2.3** sync + width + parameter polish.

## Locked Decisions (do not relitigate in execute)

- Grain source is **mono-sum** `0.5·(L+R)` from the capture ring; equal-power pan places the mono grain (D4)
- Capture ring uses **absolute-position bookkeeping** (`int64 totalWritten`; grain latches `readStartAbs = totalWritten − D`, reads stepping −1) — no relative-delay drift math, integer reads only (RESEARCH §3.1)
- Scheduler always runs the free-countdown; sync only changes the **value of D** per block (no conditional routing)
- Overlap `= 1 + (density/100)·7`; `1/sqrt(overlap)` compensation latched per grain, applied **before** the feedback tap
- Feedback tap is post grain-sum, **pre width/mix** — width/mix never enter the loop
- Smoothed (~20 ms): feedback, mix, lowCut, highCut. **Latched per grain, never smoothed:** delayTime/D, grainSize, density, width
- Harness gate: hard exit codes; tuning constants (overlap compensation, pan bias) are tuned until assertions pass, then frozen in the assertion (D5)
- One Standalone audition after Phase 2.3, before verify (D6)

---

## Phase 2.1 — Core Reverse Wet Path (feedback OFF)

### Task 1. Render harness scaffold (FIRST deliverable)
- Files: `tests/render-harness/CMakeLists.txt`, `tests/render-harness/main.cpp`; edit `plugins/O-ReverseDelay/CMakeLists.txt` (add `OUARICON_BUILD_TESTS` option + `add_subdirectory`)
- Template: `plugins/O-simpleGrain/tests/render-harness/` with synth parts stripped (RESEARCH §1 table): no BinaryData targets, only `Source/PluginProcessor.cpp`, `JUCE_WEB_BROWSER=0`, `JucePlugin_IsSynth=0`, `WantsMidiInput=0`, `IsMidiEffect=0`
- **All target references use `OuariconReverseDelay`**, not the folder name
- `JucePlugin_*` macros per RESEARCH §1 (code `0x4f527644` 'ORvD', VersionCode `0x10000`); include dirs borrowed via `$<TARGET_PROPERTY:OuariconReverseDelay,INCLUDE_DIRECTORIES>`
- Lift helpers from O-simpleGrain main.cpp: `setParam`, `rms`, `peakAbs`, `allFinite`, `continuityFraction`, `Spectrum` (skip `autocorrPitchHz`)
- Add `MockPlayHead` (RESEARCH §1 snippet) — unused until 2.3 but scaffolded now
- Effect-driving render loop: fill input buffer per block, empty MidiBuffer
- Gate: harness builds, renders silence through the Stage-1 passthrough, exits 0
- Depends on: none

### Task 2. DSP component headers (copy-and-simplify, no shared modules)
- Files: `Source/dsp/CaptureBuffer.h`, `Source/dsp/WindowLut.h`, `Source/dsp/ReverseGrain.h`
- `CaptureBuffer`: from O-GrainScatter `DelayBuffer.h` — keep `prepare(fs, 3.5f)` sizing + alloc-free `clear()` + `pushSample(L,R)`; **drop Lagrange interpolation**; add `int64 totalWritten` and `readAbs(ch, absIndex)` double-mod wrap (RESEARCH §3.1 snippet); `monoSum(absIndex)` helper for D4
- `WindowLut`: O-simpleGrain `WindowLuts.h` trimmed to Hann-only, single 2048-entry table, clamp+lerp `read(phase)` — do NOT carry the 5-shape UI contract
- `ReverseGrain` POD per RESEARCH §3.2 (readAbs, n, G, invG, gain, gL/gR, age); pool `std::array<ReverseGrain, 32>` with GrainPool find-inactive round-robin + steal-oldest
- Depends on: none (parallel with Task 1)

### Task 3. Scheduler + reverse grain render
- Files: `Source/dsp/GrainScheduler.h` (new, from O-GrainScatter free-mode shape), `Source/PluginProcessor.h/.cpp`
- Per-sample countdown emitting spawns; `intervalSamples = max(1, (int)(G/overlap))`; spawn cap 32/block; no probability/Euclidean
- Latch at spawn: D (free-mode only this phase), G, gain `1/sqrt(overlap)`, pan center (width lands in 2.3 — latch 0.7071/0.7071)
- Per-sample grain render into `wetScratch`: `s = monoSum(readAbs); e = lut.read(n·invG); wetL += s·e·gain·gL; wetR += ...; --readAbs; ++n` — branch-free inner loop, per-grain constants precomputed at spawn
- Depends on: Task 2

### Task 4. processBlock wiring (Phase 2.1 subset of the REQUIRED order)
- Files: `Source/PluginProcessor.h/.cpp`
- Preallocate in `prepareToPlay`: capture ring, `wetScratch`, `fbScratch` (2 × maxBlock each)
- Block order: (1) resolve D (free) → (2) smoothers → (3) schedule → (4) render grains → wetScratch → (5) *feedback stage stub: fb = 0* → (6) `pushSample(in + 0)` → (7) equal-power mix wetScratch with untouched input → output. Wet never in-place over input
- `ScopedNoDenormals`; param reads once per block via `getRawParameterValue()->load()`; mono→stereo input handling (capture L=R=in)
- `getTailLengthSeconds()`: return a real tail (conservative 10.0 s) — RESEARCH pitfall 11, hosts truncate the reverse tail on bounce otherwise
- Depends on: Task 3

### Task 5. Phase 2.1 probes green → commit
- Files: `tests/render-harness/main.cpp`
- Probe A — single-grain reversed ramp (FUNC-01): mix=100, feedback=0, density=0, width=0, free 500 ms, G=200 ms; linear ramp input; de-windowed slope (guard hann > 0.1) **negative AND |slope| > threshold** (catches frozen-read D+n bug); reverse-vs-forward correlation cross-check factor ≥ 5 (RESEARCH §2.1)
- Probe B — impulse reverse bloom (FUNC-01): env peak P in `[T+D−G, T+D+G]`; `RMS(second half pre-peak) > 2× RMS(first half)`; `env(firstFrame) < 0.25× env(peakFrame)` (RESEARCH §2.2)
- Probe C — click detector (DSP-01): 220 Hz sine @ −12 dBFS; `maxStep < kSineStepBound + 10^(−60/20)`; run at defaults + density 0→100→0 sweep; `allFinite` every render (RESEARCH §2.3)
- Probe D — density flatness (DSP-01): density ∈ {0,25,50,75,100}, wet RMS `max/min ≤ 10^(1/20)` (±1 dB); tune compensation constant until green, then frozen (RESEARCH §2.4; pitfall 7: RMS stays flat even with density-0 ripple; shaped curve allowed if flat constant can't reach ±1 dB)
- Probe E — feedback=0 single generation (FUNC-03 precondition): wet energy outside `[T+D−G, T+D+G+margin]` < −80 dBFS relative (no echo at T+2D)
- Code review: no alloc/locks/logging in processBlock (PERF-01)
- **git commit** on green
- Depends on: Tasks 1, 4

---

## Phase 2.2 — Feedback Loop + Damping + Stability

### Task 6. Feedback return path
- Files: `Source/PluginProcessor.h/.cpp` (or `Source/dsp/FeedbackPath.h`)
- Fill step (5): `fbScratch = wetScratch; × smoothed fbGain; HP (lowCut) → LP (highCut) per channel; tanh; non-finite guard; then pushSample(in + fb)`
- Filters: `juce::dsp::IIR::Filter<float>` × 4; coeffs via `ArrayCoefficients<float>::makeHighPass/makeLowPass` **assigned in place** (`*state = arrayCoeffs`) — never `Coefficients::makeXXX` on audio thread, never memcpy 6 raw over 5 normalised
- Cached-cutoff guard gates **only the recompute** — no enabled flag exists; keep it that way (O-MultiBandCompressor v1.6.0 lesson)
- highCut clamp `jlimit(500, 0.49·fs)` per block after smoothing
- Non-finite guard: `reset()` BOTH filters AND zero the feedback block; keep last-known-good coefficients (sticky-silence pattern)
- Depends on: Task 5

### Task 7. Phase 2.2 probes green → commit
- Files: `tests/render-harness/main.cpp`
- Probe F — damping loss per generation (FUNC-03): impulse, feedback=60, highCut=4000, lowCut=200; FFT echo windows near T+kD with `Spectrum`; `centroid(gen k+1) < centroid(gen k)` + 20–150 Hz energy fraction falls; two generations suffice; **no direction assertions past generation 1** (alternating-direction regens are intended)
- Probe G — 60 s stability (DSP-03): feedback=100, default damping, 2 s excitation then silence; every sample finite, `peakAbs < 1.0`, last-10 s bounded
- Probe H — cutoff sweeps during playback: no zipper/clicks via Probe C detector (QUAL-01 partial)
- Code review: DSP-02 (ArrayCoefficients in-place, zero audio-thread allocation)
- **git commit** on green
- Depends on: Task 6

---

## Phase 2.3 — Tempo Sync + Width + Parameter Polish

### Task 8. Tempo sync D resolution
- Files: `Source/PluginProcessor.cpp`
- Per-block `getPlayHead()->getPosition()`; 13-entry noteDivision→beats table (contract order incl. dotted/triplet); `delayMs = beats·60000/bpm` clamped [50, 2000]; `jmax(1.0, bpm)` divide hardening
- Fallback to free delayTime when playhead null OR `getBpm()` empty (COMPAT-02)
- Sync↔Free switch changes only next-spawn D — existing grains finish on latched values (click-free mechanism)
- Depends on: Task 7

### Task 9. Width spread
- Files: `Source/PluginProcessor.cpp` / scheduler latch site
- Per-grain equal-power pan from width: alternating-sign random bias (bias amount = harness-tuned constant, then frozen); latched at spawn, never smoothed
- Depends on: Task 7 (parallel with Task 8)

### Task 10. Phase 2.3 probes + full sweep green
- Files: `tests/render-harness/main.cpp`
- Probe I — sync spacing (FUNC-02): MockPlayHead 120 BPM, Sync 1/4 → first-echo latency `|latency − 0.5s·fs| ≤ blockSize`; free-mode variant at delayTime ∈ {150, 500, 1200} ms
- Probe J — no-playhead pass (COMPAT-02): never call `setPlayHead`; Sync mode falls back to delayTime, no crash/silence; bonus `setBpm({})` case
- Probe K — width (FUNC-04): width=0 → `|RMS_L−RMS_R|/RMS < 0.01`, L/R corr ≈ 1; width=100 → wet-tail corr < ~0.9 + frame-level |L−R| energy well above width-0
- Probe L — mono→stereo identity (CONTEXT open item): 1-in/2-out pass, L=R=input; wet level matches stereo-input width-0 run within 0.5 dB (no ±6 dB surprise)
- Probe M — all-parameter sweep (QUAL-01): each of the 10 params ramped full-range over ~2 s, one at a time, others at defaults; click detector + allFinite on every render; include Sync↔Free mode switch mid-playback
- Depends on: Tasks 8, 9

### Task 11. Validation + audition → commit
- `ninja OuariconReverseDelay_VST3 OuariconReverseDelay_AU` → `./scripts/build-and-install.sh O-ReverseDelay`
- pluginval strictness 10 **×3 runs each** on installed VST3 + AU (latent feedback-NaN class)
- **Standalone audition (D6):** smear quality, feedback wash, width — character check before verify
- **git commit** on green
- Depends on: Task 10

---

## Success Criteria (verify-phase gates — all harness exit-code enforced except code review)

- [ ] FUNC-01: reversed-ramp probe (slope sign + magnitude) and impulse bloom probe pass
- [ ] FUNC-02: 120 BPM 1/4 → 500 ms ±1 block; free mode continuous at 150/500/1200 ms
- [ ] FUNC-03: feedback=0 exactly one generation; damped regen shows HF + LF loss per generation
- [ ] FUNC-04: width 0 centered dual-mono; width 100 decorrelated spread; no width-automation clicks
- [ ] DSP-01: click detector < −60 dBFS margin at defaults + density sweep; density flatness ±1 dB
- [ ] DSP-02: ArrayCoefficients in-place, no audio-thread allocation (code review)
- [ ] DSP-03: 60 s @ 100% feedback — peak < 1.0, zero NaN/Inf
- [ ] DSP-04: no change needed — log-skew ranges shipped in Stage 1 (confirm untouched)
- [ ] PERF-01: no alloc/locks/logging in processBlock (code review); pluginval-10 ×3 both formats
- [ ] COMPAT-02: no-playhead harness pass green
- [ ] QUAL-01: all-parameter sweep + mode-switch pass, zero clicks/zipper/NaN/Inf
- [ ] Mono→stereo identity within 0.5 dB (D4 open item closed)
- [ ] 3 git commits (one per phase); Standalone audition done (D6)

## Out of Scope

- WebView UI, mockup, editor changes — Stage 3 (keep PluginProcessor editor-include-free)
- Presets / OuariconPresetManager — Stage 4
- Lagrange/fractional grain reads, pitch shifting — not in v1.0 contract
- Windows CI

## Pitfalls Carried From Research (execute-phase checklist)

1. Reverse read off-by-one → probe A asserts slope sign AND magnitude
2. Harness references use target `OuariconReverseDelay`, never the folder name
3. `ScopedNoDenormals` mandatory — feedback decays sweep denormal range
4. Overlap compensation BEFORE the feedback tap or density multiplies loop gain
5. Smoothing vs latching split per Locked Decisions — smoothing D/G/density/width would be wrong
6. Non-finite guard resets filters AND zeroes feedback source, keeps coeffs
7. Density-0 amplitude ripple is intended character; RMS-based flatness probe handles it
8. pluginval-10 ×3 locally before phase 2.3 sign-off
