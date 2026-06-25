# Stage 2 (DSP) — SUMMARY

**Plugin:** O-simpleGrain · **Stage:** 2 of 4 (DSP) · **Run mode:** express
**Phases:** 2.1 ✅ · 2.2 ✅ · 2.3 ⏳ (this file appends per sub-phase)

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

---

## Phase 2.2 — Read head (scan / time-stretch / freeze) + spray & scatter + anti-aliasing + velToDensity ✅

**Execute agent:** dsp-agent · **Build:** pending orchestrator · **Date:** 2026-06-24
**Tasks:** T5 (global read head) · T6 (freeze) · T7 (spray/scatter + velToDensity) · T8 (AA one-pole). Deps held: `T5 → {T6,T7}; T7 → T8`. No new files. Phase 2.1 code preserved (only extended).

### Files modified
- `Source/PluginProcessor.h` — promoted the static `positionAbsolute` resting point to a **moving/freezable global read head**: `double playheadPos` + three `SmoothedValue<float>` (`scanSmoothed`, `positionSmoothed`, `playheadVelocity`). Removed the dead `positionAbsolute` member.
- `Source/PluginProcessor.cpp` — `prepareToPlay`: reset the 3 new SmoothedValues (~20 ms) and seed `playheadPos` at `position% × srcLen` after the source decodes. `processBlock`: read `scan`/`freeze`/`positionSpray`/`pitchSpray`/`scatter`/`panSpray`/`velToDensity` atomics into `GrainVoiceParams`; advance the global playhead **per sample** (freeze-pinnable velocity, rest-ease toward the resting point when scan≈0, bidirectional wrap); push the **block-start** playhead to voices via the existing `setPlayhead` seam (voice spawn signature unchanged).
- `Source/GrainVoice.h` — extended `GrainVoiceParams` with the 5 spray/scatter/velToDensity fields; `velToDensity` folded into `effectiveDensity`; `scatter` jitters the scheduler period per spawn (RNG); `spawnGrain` adds position spray (read-start), pitch spray (rate), pan spray (equal-power pan); filled the `aaOnePole` body.

### What each task added
- **T5 — global read head.** Processor advances `playheadPos += playheadVelocity.getNextValue()` per sample; velocity = `scan/100` (realtime-relative; ±200% range → ±2 samples/sample). `position` sets the resting point the playhead eases toward when `|velocity| < 1e-4` (Position knob stays live). Bidirectional `[0,srcLen)` wrap (negative scan = reverse). `SmoothedValue` (~20 ms) on scan/position/velocity, reset in `prepareToPlay`. Per ARCHITECTURE §Read head / RESEARCH §4.1.
- **T6 — freeze.** `freeze = freezeParam->load() > 0.5f`; engage targets `playheadVelocity → 0`, disengage ramps back to the scan velocity — **via the velocity `SmoothedValue` ramp** (RESEARCH §4.2 "simplest robust approach"), so engage/disengage is click-free and the playhead is **never hard-jumped** (QUAL-01). Held note sustains the frozen instant (voice alive in amp sustain; grains keep reading the pinned region ± position spray → a frozen pad). Freeze overrides scan (pins) per the freeze×scan interaction.
- **T7 — spray & scatter + velToDensity.** All via the per-voice `juce::Random rng` (no alloc/lock). Position spray: `readPos = playheadPos + U(−1,1)·positionSpray%·srcLen`. Pitch spray: `rate = voiceRate·2^((grainPitch + U(−1,1)·pitchSpray)/12)`. Pan spray: `pan = clamp(0.5 + U(−1,1)·panSpray%·0.5, 0,1)`. Scatter: `nextInterval = max(1, (int)(baseInterval + scatter%·baseInterval·U(−1,1)))`. velToDensity: `effectiveDensity = clamp(density·(1 + velToDensity·(velLevel−0.5)·2), 1, 200)` → `baseInterval = fs/effectiveDensity`.
- **T8 — AA one-pole.** Filled the 2.1 no-op: bypass at `rate ≤ 1` (keeps `state = x` coherent for the bypass→engage edge); else `fc = 0.5·fs/rate`, `g = 1 − exp(−2π·fc/fs)`, `state += g·(x − state)`. Inserted between `readSourceLagrange` and the envelope multiply (existing call site). `aaState` primed to the **first read sample** on spawn (settled start, no attack transient).

### Deviations / notes
- **Freeze mechanic:** used the `SmoothedValue<float>` velocity ramp (RESEARCH §4.2's explicitly-recommended "simplest robust approach") rather than the FreezeManager crossfade-gain — fewer moving parts, same click-free guarantee for a static-source playhead. No crossfade buffer needed.
- **Position + scan both live:** added a gentle per-sample ease (×0.0008/sample) toward the smoothed resting point when scan≈0 and not frozen, so the Position knob remains responsive between note re-triggers without a hard jump. (ARCHITECTURE: "`position` sets the playhead's resting point; `scan` is its velocity.")
- **Block-start playhead snapshot:** voices read a single per-block `setPlayhead` value (Sequencing Note 3 / PLAN T5 "push per block"); the processor still advances the true playhead per sample for read-head coherence + the Stage-3 viz playhead line. Block-granular spawn position is musically correct for granular (short, position-sprayed grains).
- **`aaState` primed to first sample** (not 0) — RESEARCH §5 permits either; first-sample priming avoids a spurious transient on up-transposed grains.
- No new files; `CMakeLists.txt` unchanged (Phase 2.3 `juce_add_binary_data` TODO intact).

### Success criteria: code-complete vs. manual-listen
- **Code-complete (this phase):** scan velocity + position resting point + bidirectional wrap (T5); freeze pin + click-free SmoothedValue ramp + held-note sustain (T6); per-voice position/pitch/pan spray + scatter period jitter + velToDensity (T7); rate-tracking AA one-pole, bypass at rate≤1 (T8); `SmoothedValue` on scan/position/velocity, no hard playhead jump, per-voice `juce::Random` only (no alloc/lock), `setLatencySamples(0)` retained.
- **Manual-listen (verify phase / DAW):** scan fwd/slow/reverse/×2; freeze pins click-free (DSP-06/FUNC-03/QUAL-01); pitch + position spray shimmer, no two grains identical (DSP-04); scatter 0%→sync (sidebands) vs high→async (noise) (DSP-05); high pitch-spray grains stay clean, no buzz (DSP-08); no zipper on scan/position automation (QUAL-01).
- **AUTO build gate (orchestrator):** clean VST3+AU+Standalone; AU registers (`auval`); pluginval ≥8; `setLatencySamples(0)`.
