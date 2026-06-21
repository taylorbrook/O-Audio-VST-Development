# Stage 2 (DSP) — PLAN

**Plugin:** O-simpleFM · **Stage:** 2 DSP · **Date:** 2026-06-20 · **Mode:** express
**Goal:** Render the locked 2-operator PM architecture: a 16-voice, MIDI-playable synth with amp+mod envelopes, DX7 self-feedback, anti-aliasing (index ceiling + 2× OS), and a real-time-safe viz data path (ring + FFT/scope analyzer). After this stage the plugin makes expressive sound; only the WebView UI remains (Stage 3).

**Inputs:** ARCHITECTURE.md (immutable DSP spec), ROADMAP.md (3 DSP phases + test criteria), CONTEXT.md (this stage), RESEARCH.md (extracted suite reference code), existing 17-param APVTS in `PluginProcessor.h`.

---

## Files

| File | Action | Purpose |
|------|--------|---------|
| `Source/Operator.h` | **create** | Phase-accumulator operator + `fastSine` (`LookupTableTransform`, 1024 pts, **floor-modulo wrap**). Header-only. |
| `Source/FMVoice.h` | **create** | `FMSound : juce::SynthesiserSound`, `FMVoice : juce::SynthesiserVoice` — the PM core, feedback, amp+mod ADSR, index ceiling, per-voice `setParams` + custom JUCE-8 `prepareToPlay`. Header-only (suite norm). |
| `Source/FmVizAnalyzer.h` | **create** | Lock-free `AbstractFifo` ring (audio-thread copy-only write) + message-thread FFT (4096 / Blackman-Harris) + scope downsampler. Self-contained; consumed by editor Timer now, WebView emit in Stage 3. |
| `Source/PluginProcessor.h` | **modify** | Add `juce::Synthesiser`, `juce::dsp::Oversampling`, output `SmoothedValue`, `FmVizAnalyzer`, cached APVTS atomic pointers, `pushParamsToVoices()` + viz accessors. |
| `Source/PluginProcessor.cpp` | **modify** | `prepareToPlay` (synth SR, voice prepare, OS init, smoothers, FFT alloc, `setLatencySamples`); `processBlock` (param push, `renderNextBlock`, OS up/down around voice sum, output gain, NaN scrub, ring tap). |
| `Source/PluginEditor.h/.cpp` | **modify** | Make the existing thin editor a `juce::Timer` (30 Hz) that pumps `FmVizAnalyzer` (ring → FFT/scope) and tracks frame counts — keeps GenericAudioProcessorEditor body. Stage 3 swaps body for WebView + `emitEventIfBrowserIsVisible` using this same callback. |
| `CMakeLists.txt` | **modify (if needed)** | Add new source files to the target if not auto-globbed. |

---

## Tasks (mapped to ROADMAP DSP phases — one commit per phase)

### Task 1 — Phase 2.1: Core PM voice (FUNC-01/02/05, DSP-01/02/03)

1. `Operator.h`: `fastSine(float phase)` with `phase -= twoPi*std::floor(phase/twoPi)` then `sineTable[phase]`; static 1024-pt `LookupTableTransform` filled with `std::sin`. Phase-accumulator helper (`advance(incr)`, `reset()` — but voices never reset phase mid-note).
2. `FMVoice.h`: `FMSound::canPlaySound` → true; `FMVoice` with carrier+modulator phase accumulators, one `juce::ADSR` (amp) wired here in 2.1 (mod ADSR added in 2.2).
   - `startNote`: compute `fc` from MIDI note + pitch-wheel; set velocity→amp; `ampEnv.noteOn()`; **do not reset carrier/mod phase**; reset feedback history (added 2.2).
   - `stopNote`: `ampEnv.noteOff()` (allowTailOff) / `clearCurrentNote()` when `!allowTailOff`.
   - `renderNextBlock`: per-sample PM core in **radians** — `fm = modFixedMode ? modFixedHz : fc*ratioEff`; `modOut = fastSine(modPhase)`; `modPhase += twoPi*fm/fs` (wrap); `carOut = fastSine(carPhase + I*modOut)`; `carPhase += twoPi*fc/fs` (wrap); `sample = carOut * ampEnv.getNextSample() * velAmp`; add to both channels; `if (!ampEnv.isActive()) clearCurrentNote()`.
   - `setParams(...)`: receive block-pushed values; apply index perceptual taper `I = 20·norm^1.7` (or receive pre-tapered); `ratioEff = ratioSnap ? round(ratio) : ratio`.
   - Custom `prepareToPlay(sr, blockSize)` (NOT virtual): `setCurrentPlaybackSampleRate`, `ampEnv.setSampleRate` **before** any `setParameters`.
3. Processor: add `juce::Synthesiser`; `addVoice`×16 + `addSound`; in `prepareToPlay` call `setCurrentPlaybackSampleRate` + each voice's custom prepare; in `processBlock` cache APVTS atomics once, `pushParamsToVoices()`, `synth.renderNextBlock(buffer, midi, 0, numSamples)`.
4. Smoothing: `SmoothedValue` on index + output (20 ms); ratio via Multiplicative smoothing or snap+short-ramp at the read site.

**Phase 2.1 test criteria (ROADMAP):** loads as instrument, MIDI poly, no crash; index 0 = pure sine, raising index adds sidebands (no zipper); integer ratio harmonic / non-integer inharmonic; fixed-mode holds Hz while carrier tracks pitch / ratio-mode key-tracks; amp ADSR shapes notes, no stuck/silent voices; no note-on/off clicks; no denormal stalls.

### Task 2 — Phase 2.2: Mod env → index + feedback (FUNC-03/04, DSP-05/06)

1. Add independent mod `juce::ADSR` to `FMVoice` (prepared alongside amp ADSR). `modEnv.noteOn/noteOff` in lockstep with amp env, but **voice lifetime keys on amp env only**.
2. Multiplicative routing: `I_inst = baseIndex·((1−depth)+depth·modEnv)`, `depth = modEnvToIndex` (default 1.0). Add `velToIndex` (default 0): `baseIndex *= (1 + velToIndex*(vel−...))` per spec.
3. DX7 self-feedback in the modulator: `avg = 0.5*(fbPrev1+fbPrev2)`; `fbOut = fastSine(modPhase + feedbackCoeff*avg)`; `if(!isfinite) fbOut=0`; `fbOut = jlimit(-1,1,fbOut)`; `fbPrev2=fbPrev1; fbPrev1=fbOut`; use `fbOut` as `modOut`. Reset `fbPrev1/fbPrev2=0` on `startNote`.
4. `feedback` 0–1 → `feedbackCoeff` max ≈ π, `x^1.5` taper; `SmoothedValue` on feedback.

**Phase 2.2 test criteria (ROADMAP):** mod env sweeps timbre over a held note independent of amplitude; depth 1.0 + sustain 0 → pure-sine tail, carrier null near I≈2.405; feedback enriches mod → saw → noise smoothly, **no screech/limit-cycle, no NaN**; velocity→index only when `velToIndex`>0; no zipper on feedback/index automation.

### Task 3 — Phase 2.3: Anti-aliasing + viz tap (DSP-03, PERF-01, QUAL-01)

1. Key-tracked index ceiling per voice: `indexCeil = (0.9·Nyquist − fc)/fm − 1; effIndex = min(I_inst, max(0,indexCeil))`; crossfade/smooth so bright patches dull gracefully when played high (no zipper).
2. `juce::dsp::Oversampling<float>` (1 ch on the mono synth render or 2 ch on the stereo bus — choose per render path), factor **2×**, `filterHalfBandPolyphaseIIR`, `initProcessing(maxBlockSize)` in prepare; wrap the **summed** voice render with `processSamplesUp` → (voices already rendered into upsampled block? no — render at base SR then upsample? ) → use the standard pattern: render voices into the buffer at base rate, then OS the buffer: `processSamplesUp(block)` is for upsampling an existing signal; the correct pattern is to upsample the *block to be processed*. For a synth, render at base rate then run a 2× AA resample is NOT how OS works. **Use OS correctly:** create OS block from the output buffer, `processSamplesUp`, (no extra processing needed — voices already rendered), `processSamplesDown` — this applies the half-band AA filter pair, attenuating images. (Confirm exact synth-OS pattern in RESEARCH; if a cleaner approach is to oversample the render itself, document the deviation.) Report `setLatencySamples((int)os->getLatencyInSamples())` in `prepareToPlay`.
3. Output: dB→lin `outputLevel` via `SmoothedValue` (20 ms); after summing/OS, block-level `std::isfinite` scrub.
4. `FmVizAnalyzer.h`: pre-allocated `AbstractFifo` ring (e.g. 8192). Audio thread (post-gain): mono-sum block, `write`-region copy into ring (**no alloc, no locks**). Message thread: `pullScopeWindow` copies a window; scope = downsample (512→128, max-abs-keep-sign) → array; spectrum = copy window → Blackman-Harris `WindowingFunction` → `dsp::FFT(order 12)` `performFrequencyOnlyForwardTransform` → log-freq dB bins + rise-fast/fall-slow smoothing. **Copy scope window BEFORE FFT (in-place clobber).**
5. Editor: `OSimpleFMAudioProcessorEditor : juce::AudioProcessorEditor, juce::Timer`; `startTimerHz(30)`; `timerCallback()` pulls ring → analyzer → stores latest spectrum/scope + increments `vizFrameCount` (exposed for verification). Body remains `GenericAudioProcessorEditor`. Stage 3 reuses this callback to `emitEventIfBrowserIsVisible`.

**Phase 2.3 test criteria (ROADMAP):** no audible aliasing across index/feedback/pitch (2× OS + ceiling); latency reported; viz ring fed each block (frame counter advances), analyzer produces finite spectrum/scope without allocation or crash.

---

## Dependencies

Task 1 → Task 2 → Task 3 (strictly sequential; each builds on the prior voice state). Viz tap (Task 3) depends only on a working render (Task 1) but is grouped with AA for one commit. Build + auval after each task where practical; mandatory full build + auval at stage end.

## Build / validation (orchestrator-run after execute)

```
cmake -B build -G Ninja           # reconfigure (new sources auto-globbed if pattern-based)
ninja O-simpleFM_VST3 O-simpleFM_AU
# AU cache clear + dual-variant sweep + install per CLAUDE.md (build-and-install.sh O-simpleFM)
auval -v aumu OSiF OuDv           # must SUCCEED; renders audio (catches NaN/crash)
```

## Success criteria (stage-level, goal-backward)

- [ ] Builds clean (VST3 + AU); auval **SUCCEEDS** (render + MIDI + param ramp + reset-retention).
- [ ] Synth produces audio on MIDI note-on; silent at rest; 16-voice poly; no stuck/silent voices.
- [ ] PM core correct: index 0 = sine; index up = sidebands; integer ratio harmonic / non-integer inharmonic; fixed vs ratio modulator modes behave per spec.
- [ ] Amp + mod envelopes independent; depth 1.0 + sustain 0 → pure-sine tail.
- [ ] Feedback stable to 100% — no NaN, no Nyquist screech (two-sample avg + history clamp + scrub).
- [ ] 2× oversampling active, latency reported; no audible aliasing at extreme index/pitch/feedback.
- [ ] Viz data path real-time-safe: ring written copy-only on audio thread; FFT/scope on 30 Hz editor Timer; frame counter advances under render.
- [ ] No new APVTS params; 17-param contract intact; state persistence still passes.

## Out of scope

Non-sine operators, 4× OS, fineCents/masterTune (v1.1). WebView UI + actual on-screen drawing (Stage 3). Presets/preset-tour (Stage 4).
