# Stage 2 (DSP) — SUMMARY

**Plugin:** O-simpleGrain · **Stage:** 2 of 4 (DSP) · **Run mode:** express
**Phases:** 2.1 ✅ · 2.2 ⏳ · 2.3 ⏳ (this file appends per sub-phase)

---

## Phase 2.1 — Core grain engine + overlap-add + window LUTs + amp ADSR + key resample ✅

**Execute agent:** dsp-agent · **Build:** clean (VST3+AU+Standalone) · **auval:** SUCCEEDED · **Date:** 2026-06-24

### Files created
- `Source/dsp/LagrangeInterpolation.h` — 4-pt stateless `lagrangeInterpolate` (copied verbatim from O-GrainScatter).
- `Source/dsp/WindowLuts.h` — `WindowLuts`: five precomputed 2048-pt envelope tables (rect/tri/Welch/Gauss/Hann), `read(shape,phase)` linear-interp; no per-sample transcendental.
- `Source/dsp/Grain.h` — `Grain` POD (forward-phase model: active/readPos/rate/phase/phaseInc/lengthSamples/pan/shape/age/aaState).
- `Source/GrainSound.h` — `GrainSound : juce::SynthesiserSound`.
- `Source/GrainVoice.h` — preallocated `std::array<Grain,24>` pool + steal-oldest spawn + per-sample density scheduler + overlap-add render loop + key-tracked resample + amp ADSR + non-virtual `prepareToPlay` + `GrainVoiceParams` block-push struct.

### Files modified
- `Source/PluginProcessor.h` — `WindowLuts`, `juce::Synthesiser synth`, atomic `shared_ptr<AudioBuffer<float>> currentSource` (+atomicLoad/Store helpers), `outputGain` SmoothedValue, `positionAbsolute`, `loadDefaultSource()` decl.
- `Source/PluginProcessor.cpp` — ctor adds 8 voices + 1 sound + note-stealing, hands each voice the LUT; `prepareToPlay` dispatches voice prepare via `dynamic_cast`, resets gain, decodes default `fire.wav` (resampled to engine rate, capped 10 s, atomic-published); full `processBlock` (snapshot source, push params/source/playhead, `synth.renderNextBlock`, output gain + fixed 0.5 headroom, `isfinite` scrub). `setLatencySamples(0)` kept.
- `CMakeLists.txt` — 5 new headers added to `target_sources` (no `juce_add_binary_data` yet — Phase 2.3 TODO intact).

### Verification
- **Build:** VST3 + AU + Standalone link clean, no errors/warnings (beyond benign repo-wide BUNDLE_ID-spaces note).
- **auval:** `aumu OsGr OuDv` → **AU VALIDATION SUCCEEDED** (render tests at 22050/44100/48000/96000/192000 Hz × frame sizes 64–4096; 1-channel; bad-max-frames; parameter set/ramp; **Test MIDI PASS**).
- **RT-safety:** allocation/lock/IO-free render path (preallocated pool, steal-oldest, LUTs at construction, lock-free atomic source snapshot, ScopedNoDenormals + isfinite, per-voice `juce::Random`). Zero added latency.
- **Manual-listen (deferred to stage-end DAW pass):** separated→fused grains, buzz↔fragments, rect-click, MIDI transpose.

### Seams pre-wired for 2.2/2.3 (no hot-loop re-touch)
1. Atomic `currentSource` publish (2.3 swaps byte source only). 2. `setParams(GrainVoiceParams)` struct (2.2 adds spray/scatter fields). 3. Abstract per-block playhead value the voice reads at spawn (2.2 promotes to moving/freezable). 4. `aaOnePole` no-op pass-through at the call site (2.2 fills body). 5. Per-voice `juce::Random rng` member present.

### Deviations / notes
- `WindowLuts` uses a `std::vector<float>` sized once in its ctor (runtime `kWindowLutSize`) instead of `std::array<float,2048>` — functionally identical, allocation-free on the audio thread.
- Mono source read in 2.1 (default `fire.wav` is mono); stereo handling lands with 2.3 multi-source.
- Headroom is a fixed 0.5 factor (ARCHITECTURE's "fixed headroom factor" option), not overlap-aware normalization (acceptable for v1.0).
- `loadDefaultSource` locates `fire.wav` via a `__FILE__`-relative path (throwaway 2.1 path; replaced by `BinaryData` embed in 2.3). Resolves in local CMake dev builds.
- `std::atomic_load/store` on `shared_ptr` are C++20-deprecated (compile fine) — established O-MicrotonalSampler pattern.
