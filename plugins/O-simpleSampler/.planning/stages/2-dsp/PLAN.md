# Stage 2 (DSP) — PLAN

**Plugin:** O-simpleSampler
**Stage:** 2 of 4 — DSP
**Phase:** plan → complete
**Date:** 2026-06-25
**Inputs:** stages/2-dsp/CONTEXT.md (D1 piano-only, D2 checkpoint-after-2.1), stages/2-dsp/RESEARCH.md (6 open items resolved + verbatim reuse map), research/ARCHITECTURE.md, ROADMAP.md, parameter-spec.md, Stage-1 PluginProcessor.{h,cpp}.

---

## Goal

Turn the silent 16-voice Stage-1 shell into a playable, professional-sounding educational
sampler. Stage 2 is internally **3 phases**; per **CONTEXT D2 the execute phase implements
Phase 2.1 only, then STOPS for a DAW play-test** before committing to 2.2/2.3.

- **2.1 (THIS execute target):** Repitch fractional-read varispeed + start/end region + amp
  ADSR + `velToAmp` + built-in `piano.wav` decode/resample/atomic-publish → **first audio**.
- **2.2 (next execute pass):** loop (fwd/ping-pong + equal-power crossfade) + reverse + Stretch
  (synchronous-granular SOLA) + Vintage (S&H + bit-crush) + resonant LP filter.
- **2.3 (final pass):** AA hardening + lock-free viz taps + voice-stealing audit + RT-safety +
  offline render-harness (the Stage-2 correctness gate).

## Frozen constraints (carry-forward from Stage 1)

- **21-param APVTS is frozen** — read existing IDs only; no rename/add (breaks state checksums).
  String IDs `"start"`/`"end"` → C++ `regionStart`/`regionEnd` (bare `end` shadows `juce::end`).
- Constants already in `PluginProcessor.h`: `kMaxVoices=16`, `kMaxGrainsPerVoice=4`, `kRootNote=60`,
  `kMaxSourceSeconds=30`, `kStretchGrainMs=60`, `kNumBuiltIns=4`, `kBuiltInNames={piano,vocal,flute,vinyl}`.
- `setLatencySamples(0)` — `getLatencySamples()` is non-virtual in JUCE 8; never override.
- Dual binary-data NAMESPACE: samples → `NAMESPACE BinaryData`, UI (Stage 3) → `NAMESPACE UIBinaryData`
  — distinct NAMESPACE **and** HEADER_NAME (O-simpleGrain Stage-3.1 duplicate-symbol lesson).
- State persistence (APVTS tree + custom `SOURCE/identity`, default `embedded:piano`) already wired.

---

# Phase 2.1 — Core Playable Sampler (IMMEDIATE EXECUTE TARGET)

**Goal:** A polyphonic, MIDI-playable sampler — `piano.wav` read through the Repitch read head,
isolated by start/end, AA-filtered, shaped by amp ADSR + VCA + `velToAmp`, tuned relative to Root Key.

## Tasks

1. [ ] **Embed `piano.wav` + 2nd `juce_add_binary_data` target**
   - `cp plugins/O-simpleGrain/Source/samples/piano.wav plugins/O-simpleSampler/Source/samples/piano.wav`
   - Add `juce_add_binary_data(O-simpleSampler_Samples NAMESPACE BinaryData HEADER_NAME BinaryData.h SOURCES Source/samples/piano.wav)` **after** `juce_generate_juce_header(...)`; `target_link_libraries(O-simpleSampler PRIVATE O-simpleSampler_Samples)`. Flip the Stage-1 binary-data TODO; leave the UI/`UIBinaryData` target as a Stage-3 TODO comment.
   - Confirm `juce_add_plugin` flags already correct (`IS_SYNTH/NEEDS_MIDI_INPUT/NEEDS_WEB_BROWSER/NEEDS_WEBVIEW2`) + defs (`JUCE_WEB_BROWSER=1`, `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`, `JUCE_USE_CURL=0`).
   - Files: `CMakeLists.txt`, `Source/samples/piano.wav` (new)
   - Depends on: none
   - Ref: RESEARCH §7, §16

2. [ ] **Port anti-alias read helpers (verbatim)**
   - Copy `dsp/LagrangeInterpolation.h` from O-simpleGrain verbatim (global `lagrangeInterpolate`).
   - Add `readSourceLagrange(const float* src,int len,float pos)` (jlimit-clamped 4-tap) and `aaOnePole(float x,float coeff,bool engaged,float& state)` statics (from `GrainVoice.h:390,412`) — place in `SamplerVoice.h` or a small shared header.
   - Files: `Source/dsp/LagrangeInterpolation.h` (new), `Source/SamplerVoice.h` (new, helpers)
   - Depends on: none
   - Ref: RESEARCH §2

3. [ ] **`SamplerSound` + `SamplerVoice` (Repitch read head + AA + amp ADSR + `velToAmp`)**
   - `SamplerSound : juce::SynthesiserSound` (trivial `appliesToNote/appliesToChannel` → true).
   - `SamplerVoice : juce::SynthesiserVoice` (custom — NOT `juce::SamplerVoice`). `canPlaySound` → `dynamic_cast<SamplerSound*>`. Overrides `startNote/stopNote/pitchWheelMoved/controllerMoved/renderNextBlock`.
   - **Non-virtual** `prepareToPlay(double sr,int blockSize)` (SynthesiserVoice has none): `setCurrentPlaybackSampleRate(sr)`, `ampEnv.setSampleRate(sr)` **then** `setParameters` (JUCE-8 order gate).
   - Voice holds **raw `const float* sourcePtr` + `int sourceLen`** via `setSource(const float*,int)` — never owns an `AudioBuffer`. Null source → silence.
   - `startNote`: `voiceRate = 2^((note − rootKey + tune + fine·0.01)/12)` (live APVTS values, NOT `kRootNote`); `readPos = startSamp`; `velLevel = (1−v) + v·velocity`, `v = velToAmp·0.01`; compute AA coeff from `voiceRate` (engaged when `>1`); `ampEnv.noteOn()`.
   - `renderNextBlock`: early-out if `!ampEnv.isActive()`; per-sample `readSourceLagrange` → `aaOnePole` → `ampVal = ampEnv.getNextSample()·velLevel`; `addSample` L/R (mono dup), confined to `[startSamp,endSamp)`; `clearCurrentNote()` when env finishes.
   - `SamplerVoiceParams` push struct (rootKey, tune, fine, region start/end samples, `velToAmp`, `juce::ADSR::Parameters amp`).
   - Files: `Source/SamplerSound.h` (new), `Source/SamplerVoice.h` (new)
   - Depends on: Task 2
   - Ref: RESEARCH §3, §8

4. [ ] **Source decode → resample → atomic publish (processor)**
   - `std::shared_ptr<juce::AudioBuffer<float>> currentSource` + `atomicLoad/atomicStore` (`std::atomic_load/store`).
   - `decodeAndPublish(const char* data,size_t bytes,double engineRate,String identity)`: `AudioFormatManager` + `createReaderFor(MemoryInputStream)` → `reader->read` → `resampleToEngineRate` (offline `juce::LagrangeInterpolator`, per-channel, capped at `kMaxSourceSeconds`) → `atomicStore`. Invalid reader → keep previous source.
   - `loadBuiltInSource(int idx,double rate)` + `builtInBlob(idx)` switch (index 0 → `BinaryData::piano_wav`; indices 1–3 fall back to piano blob, documented TODO so no selection is silent).
   - `#include "BinaryData.h"`.
   - Files: `Source/PluginProcessor.{h,cpp}`
   - Depends on: Task 1
   - Ref: RESEARCH §4, §5

5. [ ] **`sourceSample` listener → AsyncUpdater + per-source root seed**
   - Make processor a `juce::AudioProcessorValueTreeState::Listener` + `juce::AsyncUpdater`; ctor `addParameterListener(sourceSample,this)`.
   - `parameterChanged` → `pendingBuiltInIndex.store; triggerAsyncUpdate()`; `handleAsyncUpdate` → `loadBuiltInSource(idx)` + `seedRootForSource(idx)`.
   - **Per-source root table** `kBuiltInRoot[kNumBuiltIns] = {48,69,72,48}` — **piano = 48** (probed f0 ≈131.25 Hz; root 60 plays an octave flat). APVTS `rootKey` *default* stays 60 (frozen); seed the **live** value.
   - **Seeding mechanism:** prefer the prepare-time guarded seed — seed the default source's root once on first `prepareToPlay`, guarded by a "state-was-not-restored" flag, so a restored session keeps its `rootKey` and a fresh instance gets 48. Also seed on explicit user `sourceSample` change.
   - Files: `Source/PluginProcessor.{h,cpp}`
   - Depends on: Task 4
   - Ref: RESEARCH §5, §6 (seed caveat)

6. [ ] **Wire the 16-voice synth + per-block param push**
   - Build `juce::Synthesiser`: 16 `SamplerVoice` + 1 `SamplerSound`; `setNoteStealingEnabled(true)`.
   - `prepareToPlay`: `synth.setCurrentPlaybackSampleRate(sr)`; dispatch each voice's non-virtual `prepareToPlay` via `dynamic_cast`; `setLatencySamples(0)`; output `SmoothedValue.reset(sr,0.02)`; decode the default/restored source.
   - `processBlock`: `juce::ScopedNoDenormals`; read 21 cached APVTS atomics once → build `SamplerVoiceParams` + compute `startSamp/endSamp` from `start`/`end` % of source length → `voice->setParams(...)` + `voice->setSource(srcPtr,srcLen)` (snapshot `atomicLoad` once/block) → `synth.renderNextBlock`; output gain `applyGainRamp` (`decibelsToGain(outputLevel,-60)`); final `std::isfinite` scrub.
   - Files: `Source/PluginProcessor.{h,cpp}`
   - Depends on: Tasks 3, 4, 5
   - Ref: RESEARCH §3, §4, §9.3 (output)

7. [ ] **Restore-aware `setStateInformation`**
   - After `replaceState`, re-decode the restored `SOURCE/identity` source and `cancelPendingUpdate()` so the choice-rebuild doesn't clobber a restored file; set the "state-was-restored" flag so the prepare-time root seed (Task 5) is skipped.
   - Files: `Source/PluginProcessor.cpp`
   - Depends on: Tasks 4, 5
   - Ref: RESEARCH §5

8. [ ] **Build, validate, STOP for DAW play-test**
   - `ninja O-simpleSampler_VST3 O-simpleSampler_AU O-simpleSampler_Standalone` clean.
   - `auval -v aumu OsSm OuDv` → SUCCEEDED (21 Global Scope Parameters); `pluginval` strictness-5 → SUCCESS.
   - Install (build-and-install dual-variant sweep) + **STOP** — DAW play-test gate per CONTEXT D2 before 2.2.
   - Files: none (build/validation)
   - Depends on: Tasks 1–7

## Phase 2.1 Success Criteria

- [ ] Loads in a DAW as an **instrument**; MIDI routes; 16-voice polyphonic, no crash.
- [ ] Root Key (48 for piano) plays at original pitch (~131 Hz); notes above/below transpose by varispeed (Repitch).
- [ ] Start/End change the played region; playback begins at Start, ends at End.
- [ ] `tune`/`fine` transpose independent of the keyboard.
- [ ] Built-in piano selects, decodes, plays; selecting it seeds root = 48; fresh instance is in standard tune.
- [ ] No obvious aliasing/buzz at high notes (formal probe deferred to 2.3).
- [ ] No clicks on note-on/off (5 ms ADSR attack/release); no denormal stalls on long releases.
- [ ] Build clean (VST3+AU+Standalone); `auval` SUCCEEDED; pluginval@5 SUCCESS.

---

# Phase 2.2 — Full Tone Chain (forward scope; next execute pass)

> Implement after the 2.1 DAW play-test passes. Captured here for plan completeness; not part of this execute.

## Tasks (2.2)

9. [ ] **Loop engine** — `loopMode` off/forward/ping-pong; `loopStart`/`loopEnd` (% of region); equal-power (cos/sin) crossfade over `loopCrossfade` ms via a 2nd read head; zero-cross marker snap (off-thread, publish int indices); `reverse` negates the base increment. (`Source/SamplerVoice.h`, `PluginProcessor.cpp`) — RESEARCH §10
10. [ ] **Stretch (synchronous-granular SOLA, HEADLINE)** — copy `dsp/Grain.h`, `dsp/WindowLuts.h` from O-simpleGrain; per-voice `std::array<Grain,4>`; fixed `grainHop = grainSize/2` (~60 ms, 2× Hann); time-axis `timePos += 1.0`, each grain `g.rate = keyRatio`; `pitchMode` toggles Repitch↔Stretch. (`Source/SamplerVoice.h`, new `Source/dsp/*`) — RESEARCH §11
11. [ ] **Vintage (S&H + bit-crush)** — per-voice, before the filter; `fsEff=lerp(fs,3000,vintage)` decimation + `bits=lerp(24,8,vintage)` quantize; **full bypass at `vintage==0`** (bit-for-bit). (`Source/SamplerVoice.h`) — RESEARCH §9.1
12. [ ] **Resonant LP filter** — per-voice `juce::dsp::StateVariableTPTFilter<float>` LP; `setCutoffFrequency(jlimit(20,0.45·fs,filterCutoff))`; `Q = jmap(filterResonance,0,100,0.707,12)`; `SmoothedValue` (20 ms) on cutoff/res. (`Source/SamplerVoice.h`, `PluginProcessor.cpp`) — RESEARCH §9.2

## Success Criteria (2.2)

- [ ] Loop forward sustains without dropout; **no click at the seam** with crossfade; ping-pong + reverse correct.
- [ ] **Repitch vs Stretch obvious** on a sustained note (Repitch couples pitch+time; Stretch holds duration, pitch tracks key).
- [ ] Vintage 0% bit-for-bit clean; increasing adds grit; no NaNs across range.
- [ ] Filter cutoff/resonance audibly shape tone; LP open at default; no zipper.

---

# Phase 2.3 — Hardening + Viz + Render-Harness (forward scope; final pass)

> The Stage-2 correctness gate. Captured for completeness; not part of this execute.

## Tasks (2.3)

13. [ ] **Lead-voice display atomics** — loudest-active voice publishes `displayPlayhead`, `displayCutoffHz`, `displayK` (=1/Q) once/block (O-simpleSubtractive pattern; flag the ARCHITECTURE "most-recently-triggered" divergence). — RESEARCH §9.3
14. [ ] **Viz tap** — copy `O-simpleGrain/Source/VizAnalyzer.h` verbatim → `SamplerVizAnalyzer.h`; `VizRing` write at tail of `processBlock` (≤4096 stack chunks, post-gain); FFT on editor Timer (Stage 3 consumes). No alloc/FFT/locks on audio thread. — RESEARCH §12
15. [ ] **Voice-stealing audit + AA hardening + RT-safety** — confirm 16-voice stealing, no stuck notes; verify high-key + extreme-Stretch AA budget; denormal/`isfinite` audit; `setLatencySamples(0)`. — RESEARCH §14
16. [ ] **Offline render-harness (correctness gate)** — port `O-simpleGrain/tests/render-harness/{main.cpp,CMakeLists.txt}`; `target_sources` plugin `PluginProcessor.cpp` + `PluginEditor.cpp`; `add_dependencies` on plugin + both binary-data targets; `JUCE_MODAL_LOOPS_PERMITTED=1` (pump AsyncUpdater decode); gate `option(OUARICON_BUILD_TESTS OFF)`. — RESEARCH §13

## Success Criteria (2.3) — Stage-2 correctness gate

- [ ] `processBlock` allocation-free; sample load does not glitch/block the audio thread.
- [ ] 16 voices, graceful voice-stealing, no stuck notes; latency reported 0.
- [ ] Render-harness asserts: **Repitch tuning** (f0 at root = 131 Hz, octave = 2×, `autocorrPitchHz`); **Stretch pitch/time independence** (single-grain autocorr probe — spectral bins confounded by grain comb, project memory); **loop-seam click absence** (`continuityFraction`); **Vintage clean-at-zero** (bit-for-bit); **anti-alias budget** on high keys.

---

## Files (Stage 2)

**New (2.1):** `Source/samples/piano.wav`, `Source/dsp/LagrangeInterpolation.h`, `Source/SamplerSound.h`, `Source/SamplerVoice.h`
**Modified (2.1):** `CMakeLists.txt`, `Source/PluginProcessor.{h,cpp}`
**New (2.2):** `Source/dsp/Grain.h`, `Source/dsp/WindowLuts.h`
**New (2.3):** `Source/SamplerVizAnalyzer.h`, `tests/render-harness/{main.cpp,CMakeLists.txt}`

## Reuse map (copy paths)

| File | From | Phase |
|------|------|-------|
| `dsp/LagrangeInterpolation.h` | O-simpleGrain | 2.1 (verbatim) |
| `samples/piano.wav` | O-simpleGrain | 2.1 |
| decode/resample/atomic-publish | O-simpleGrain `PluginProcessor.cpp:336–405` | 2.1 |
| amp ADSR + VCA + lifetime | O-simpleSubtractive `SubVoice.h:62–282` | 2.1 |
| `Grain.h`, `WindowLuts.h` | O-simpleGrain `dsp/` | 2.2 |
| `StateVariableTPTFilter` curve (`k=1/Q`) | O-simpleSubtractive `SubVizAnalyzer.h:71` | 2.2 |
| `VizAnalyzer.h` → `SamplerVizAnalyzer.h` | O-simpleGrain | 2.3 |
| `tests/render-harness/` | O-simpleGrain | 2.3 |

## Key risks (top 3)

- **Repitch tuning wrong (root mis-seed)** → seed piano root = **48** (not 60); 2.3 harness asserts f0 at root + octave.
- **Upward-transposition aliasing** → `aaOnePole` `fc=0.5·fs/rate` engaged on `rate>1` (proven O-simpleGrain decision).
- **Dual binary-data symbol collision** → distinct `NAMESPACE` **and** `HEADER_NAME` (O-simpleGrain Stage-3.1 lesson).

---
*Plan complete 2026-06-25. Execute target: Phase 2.1 only, then STOP for DAW play-test (CONTEXT D2). Next: execute phase.*
