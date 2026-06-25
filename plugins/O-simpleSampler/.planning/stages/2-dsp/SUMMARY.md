# Stage 2 (DSP) — SUMMARY (Phase 2.1)

**Plugin:** O-simpleSampler
**Stage:** 2 of 4 — DSP
**Phase executed:** **2.1 only** (Core Playable Sampler) — STOP for DAW play-test per CONTEXT D2
**Date:** 2026-06-25
**Result:** ✅ First audio. Build clean (VST3+AU+Standalone), auval SUCCEEDED (21 params), pluginval@5 SUCCESS, installed.

---

## What was built

Turned the silent Stage-1 16-voice shell into a polyphonic, MIDI-playable sampler: the embedded
`piano.wav` is decoded/resampled off the audio thread and played through a per-voice fractional-read
varispeed ("Repitch") head, isolated by the start/end region, anti-aliased on up-transposition, and
shaped by a per-voice amp ADSR + VCA + velocity-sensitivity blend, tuned relative to the live Root Key.

### Tasks completed (Phase 2.1 = Tasks 1–8)

1. **Embed `piano.wav` + 2nd binary-data target** — `Source/samples/piano.wav` copied from O-simpleGrain;
   `juce_add_binary_data(O-simpleSampler_Samples NAMESPACE BinaryData HEADER_NAME BinaryData.h)` added
   AFTER `juce_generate_juce_header` and linked. UI/`UIBinaryData` target left as a Stage-3 TODO with the
   distinct-NAMESPACE collision note preserved.
2. **Anti-alias read helpers (verbatim)** — `dsp/LagrangeInterpolation.h` copied verbatim;
   `readSourceLagrange` (4-tap, jlimit-clamped) + `aaOnePole` (precomputed-coeff one-pole) ported into
   `SampleVoice.h` as private statics.
3. **`SampleSound` + `SampleVoice`** — custom `juce::SynthesiserVoice` (NOT `juce::SamplerVoice`): Repitch
   read head (`voiceRate = 2^((note − rootKey + tune + fine·0.01)/12)`, live APVTS values), region clamp
   `[startSamp,endSamp)`, AA one-pole engaged on `rate>1` (coeff `1−exp(−2π·(0.5·fs/rate)/fs)` computed once
   on note-on, state primed), amp ADSR via `juce::ADSR`, `velLevel=(1−v)+v·velocity` (`v=velToAmp·0.01`),
   non-virtual `prepareToPlay` (setSampleRate **before** setParameters — JUCE-8 order gate), lifetime keyed
   on the amp env, `SamplerVoiceParams` push struct.
4. **Source decode → resample → atomic publish** — `std::shared_ptr<AudioBuffer> currentSource` with
   `std::atomic_load/store` helpers; `decodeAndPublish` (`AudioFormatManager` + `createReaderFor(MemoryInputStream)`
   → `resampleToEngineRate` offline `juce::LagrangeInterpolator`, capped at `kMaxSourceSeconds`) → atomic swap;
   `loadBuiltInSource` + `builtInBlob` switch (index 0 → `BinaryData::piano_wav`; 1–3 fall back to the piano
   blob, documented TODO — no selection is silent).
5. **`sourceSample` listener → AsyncUpdater + per-source root seed** — processor is now an
   `APVTS::Listener` + `AsyncUpdater`; `parameterChanged`→`triggerAsyncUpdate`→`handleAsyncUpdate` does
   `loadBuiltInSource` + `seedRootForSource`. Per-source root table `kBuiltInRoot={48,69,72,48}` — **piano=48**
   (probed f0 ≈131.25 Hz; root 60 plays an octave flat). APVTS `rootKey` *default* stays 60 (frozen); the
   **live** value is seeded. Prepare-time guarded seed (`rootSeeded`/`stateWasRestored`) seeds a fresh
   instance once and leaves a restored session's saved `rootKey` intact.
6. **16-voice synth + per-block param push** — `juce::Synthesiser` with 16 `SampleVoice` + 1 `SampleSound`,
   note-stealing on; `prepareToPlay` dispatches each voice's non-virtual prepare via `dynamic_cast`, decodes
   the active/restored source, resets `outputGain`; `processBlock` (`ScopedNoDenormals`) snapshots the source
   `shared_ptr` once, reads the 21 cached atomics, builds `SamplerVoiceParams` + region samples, pushes to
   every voice, `synth.renderNextBlock`, smoothed output trim (`decibelsToGain(outputLevel,−60)` ramp), final
   `std::isfinite` scrub. Allocation-free.
7. **Restore-aware `setStateInformation`** — after `replaceState`, re-decodes the restored `SOURCE/identity`
   source at the current engine rate, sets `stateWasRestored` (skips the prepare-time root seed), and
   `cancelPendingUpdate()` + clears `pendingBuiltInIndex` so the listener-queued rebuild can't clobber it.
8. **Build / validate / install** — see Validation below.

---

## Files

**New:** `Source/samples/piano.wav`, `Source/dsp/LagrangeInterpolation.h`, `Source/SampleSound.h`, `Source/SampleVoice.h`
**Modified:** `CMakeLists.txt`, `Source/PluginProcessor.{h,cpp}`

---

## Deviation — class naming collision (NEW gotcha)

`SamplerSound` / `SamplerVoice` (the names in PLAN Task 3) **collide with `juce::SamplerSound` /
`juce::SamplerVoice`** (`juce_audio_formats/sampler/juce_Sampler.h`). The generated `JuceHeader.h` does
`using namespace juce;`, so the unqualified `Sampler`-prefixed names are **ambiguous at every use** —
the build failed with `reference to 'SamplerVoice' is ambiguous`. This is the same class of bug as the
documented `regionStart`/`regionEnd` vs `juce::end` collision.

**Fix:** classes renamed to `SampleSound` / `SampleVoice` (drop the "r" — `Sampler`-free, collision-checked
against JUCE), files renamed to match, with a header comment in each documenting why. The `SamplerVoiceParams`
struct keeps its name (no `juce::SamplerVoiceParams` exists). Rebuild then linked all three formats.

---

## Validation (Task 8)

| Check | Result |
|-------|--------|
| `ninja` VST3 + AU + Standalone | ✅ clean (after the naming-collision fix) |
| `auval -v aumu OsSm OuDv` | ✅ **AU VALIDATION SUCCEEDED** (render + 1-channel + MIDI + ramped-param) |
| Parameter count | ✅ **21 Global Scope Parameters** (frozen, unchanged from Stage 1) |
| `pluginval --strictness-level 5` (VST3) | ✅ **SUCCESS** |
| Bus layout | ✅ 0 input / 2 output (instrument); Mono+Stereo output layouts |
| Install (build-and-install.sh dual-variant sweep) | ✅ single variant installed, no orphan shadow |

### Phase 2.1 success criteria
- ✅ Loads as an instrument; MIDI routes; 16-voice; no crash (auval MIDI + pluginval).
- ✅ Build clean (3 formats); auval SUCCEEDED; pluginval@5 SUCCESS.
- ⏳ **DAW play-test gate (CONTEXT D2)** — the audible criteria below need a DAW and are the explicit STOP:
  - Root Key (48 for piano) plays at original pitch (~131 Hz); notes transpose by varispeed.
  - Start/End change the played region; tune/fine transpose independent of the keyboard.
  - Piano selects/decodes/plays; selecting it seeds root=48; fresh instance in standard tune.
  - No clicks on note-on/off (5 ms ADSR); no obvious aliasing at high notes (formal probe deferred to 2.3).

---

## Forward scope (NOT in this execute)

- **Phase 2.2:** loop (fwd/ping-pong + equal-power crossfade) + reverse + Stretch (synchronous-granular SOLA)
  + Vintage (S&H + bit-crush) + resonant LP filter.
- **Phase 2.3:** AA hardening + lock-free viz taps + voice-stealing audit + RT-safety + offline render-harness
  (the Stage-2 correctness gate).

*Phase 2.1 complete 2026-06-25. STOP for DAW play-test per CONTEXT D2 before 2.2.*
