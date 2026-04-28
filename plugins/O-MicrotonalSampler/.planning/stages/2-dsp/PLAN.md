---
title: "O-MicrotonalSampler Stage 2 (DSP) — Execution Plan"
created: 2026-04-27
stage: 2-dsp
phase: plan
status: ready_for_execute
inputs:
  - .planning/stages/2-dsp/CONTEXT.md
  - .planning/stages/2-dsp/RESEARCH.md
  - .planning/stages/1-foundation/SUMMARY.md
  - .planning/stages/1-foundation/VERIFICATION.md
  - .planning/REQUIREMENTS.md
  - .planning/BRIEF.md
---

# Stage 2 (DSP) — Execution Plan

## Goal

Convert the Stage 1 silent shell into a fully working sample-playback engine: 16-voice polyphonic, varispeed cubic-Hermite interpolation, ADSR, equal-power velocity-layer crossfade, voice-stealing with 5-ms tail ramp, and auto-detected sustain loops. Sample loading is asynchronous on a background thread with tolerant filename parsing and one-time SR conversion. Microtonal pitch comes from `TuningEngine::getFrequency` plus VST3 Note Expression baked at `startNote`.

After Stage 2, the plugin produces correct, RT-safe audio for any folder of named samples drag-dropped into it; Stage 3 wraps a real UI around it.

## Sub-stage strategy (locked by D2-6)

Five sequential phases, each ending in an atomic commit on a passing gate. **No concurrent file edits across phases** — the wave boundaries are temporal, not parallel.

| Phase | Scope | Gate |
|---|---|---|
| 2.1 | Voice DSP (cubic-Hermite, ADSR, NE, in-memory test fixture) | First audio + aliasing measurement |
| 2.2 | Sample loader + filename parser + SR conversion + `findSlot` body | Drag-drop folder → 4-layer mapping plays |
| 2.3 | Velocity-layer equal-power crossfade | Smooth blend across vel boundaries |
| 2.4 | Voice-stealing with 5-ms ramp | 17th note clean handoff, no clicks |
| 2.5 | Loop auto-detect + 8-sample loop crossfade | Sustained material loops seamlessly |

Each gate runs the full triple-build (`VST3 + AU + Standalone`) plus `pluginval --strictness 5` plus the gate-specific functional check (RESEARCH §Sub-stage decomposition).

---

## Tasks

### Phase 2.1 — Voice DSP + first audio (Gate 1)

1. [ ] **Add per-voice DSP state to `MicrotonalSamplerVoice.h`**
   - Files: `Source/MicrotonalSamplerVoice.h`
   - Adds: `juce::ADSR adsr;`, `double pos = 0.0;`, `double playRate = 1.0;`, `double currentFrequency = 0.0;`, `int currentMidiNote = -1;`, `const SampleSlot* currentSlot = nullptr;`, `std::shared_ptr<SampleMap> currentMap;` (lifetime owner snapshot), public `void prepareToPlay(double sampleRate, int samplesPerBlock);`
   - **No** APVTS-source / loop / steal members yet — those land in 2.3 / 2.4 / 2.5.
   - Depends on: none

2. [ ] **Implement `MicrotonalSamplerVoice::prepareToPlay` and `setCurrentPlaybackSampleRate`**
   - Files: `Source/MicrotonalSamplerVoice.cpp`
   - `prepareToPlay`: `adsr.setSampleRate(sampleRate)` (RESEARCH pitfall #1), then default ADSR params so first-build defaults make sound: `adsr.setParameters({0.005f, 0.1f, 1.0f, 0.3f})`. Override with APVTS values inside `startNote`. Override `setCurrentPlaybackSampleRate` to also call `adsr.setSampleRate(...)` so JUCE's reset path keeps ADSR in sync.
   - Depends on: Task 1

3. [ ] **Inline `cubicInterp` helper (anonymous namespace, top of `MicrotonalSamplerVoice.cpp`)**
   - Files: `Source/MicrotonalSamplerVoice.cpp`
   - Signature: `static inline float cubicInterp(const float* buf, int N, double pos, int loopStart, int loopEnd) noexcept;`
   - Body: Catmull-Rom Horner expansion exactly as `juce_Interpolators.h:118-131` (RESEARCH RQ-1, R2). Wrap helper: if `loopEnd > 0` then `wrap(idx) = loopStart + ((idx - loopStart) mod (loopEnd - loopStart))`; else `clamp(idx, 0, N-1)`. Loop wrap path is dormant in 2.1 (we set `loopEnd = 0` for the test fixture); active in 2.5.
   - Depends on: none

4. [ ] **Implement `startNote` (Phase 2.1 baseline — no steal ramp yet, no loop, single-layer)**
   - Files: `Source/MicrotonalSamplerVoice.cpp`
   - Behavior:
     - Snapshot map: `currentMap = sampleMapSource ? *sampleMapSource : nullptr;` (RESEARCH pitfall #4 — `shared_ptr` copy is the only mandatory atomic op).
     - If `currentMap == nullptr` → `clearCurrentNote(); return;` (EC-2).
     - Velocity → layer index: `int vel = juce::jlimit(1, 127, (int)std::round(velocity * 127.0f));`, `layerWidth = 128 / max(currentMap->numVelocityLayers, 1)`, `int layerIdx = juce::jlimit(0, currentMap->numVelocityLayers - 1, (vel - 1) / layerWidth);`. (Phase 2.3 replaces this with two-layer crossfade.)
     - `currentSlot = currentMap->findSlot(midiNoteNumber, layerIdx);` — in Phase 2.1 this returns `nullptr` (Stage-1 stub) for any real folder; the hardcoded test fixture (Task 6) supplies a real slot. If `nullptr` → `clearCurrentNote(); return;` (EC-1).
     - `currentMidiNote = midiNoteNumber;`
     - Base frequency: `currentFrequency = (tuningEngine != nullptr) ? tuningEngine->getFrequency(midiNoteNumber) : (440.0 * std::pow(2.0, (midiNoteNumber - 69) / 12.0));` — fallback to ET keeps Standalone usable when host doesn't provide a tuning table.
     - Apply NE (D2-12, R6): `if (pendingTuningSource != nullptr) Ouaricon::NoteExpression::applyPendingTuning(*pendingTuningSource, midiNoteNumber, currentFrequency);` (mutates `currentFrequency` in place per the helper's signature; verify against the actual API in `modules/tuning/note-expression/cpp/NoteExpression.h` and adapt the call site if the helper returns instead).
     - `playRate = (currentFrequency / referenceFrequency(currentMidiNote)) * (currentSlot->sourceSampleRate / getSampleRate());` — where `referenceFrequency` uses ET semitone math relative to the slot's MIDI root (the slot is recorded at MIDI `currentSlot->midiNote`; in 2.1 the slot equals the played note so `referenceFrequency == 440 * 2^((slotNote-69)/12)`). For the test fixture we record at the played note → `playRate = currentFrequency / ET-of-played-note × srRatio`. Document the formula in the cpp comment header.
     - Read APVTS ADSR values **once**: `adsr.setParameters({attack, decay, sustain, release});` — no per-block reads (RESEARCH pitfall #2).
     - `pos = 0.0; adsr.reset(); adsr.noteOn();`
   - Depends on: Tasks 1, 2, 3

5. [ ] **Implement `stopNote` and `renderNextBlock` (Phase 2.1 baseline)**
   - Files: `Source/MicrotonalSamplerVoice.cpp`
   - `stopNote(velocity, allowTailOff)`: if `allowTailOff` → `adsr.noteOff();` else `adsr.reset(); clearCurrentNote();`. (Steal ramp path lands in 2.4.)
   - `renderNextBlock(out, startSample, numSamples)`:
     - Early-out if `currentSlot == nullptr || !adsr.isActive()` → call `clearCurrentNote()` if ADSR finished and return.
     - Per-sample loop: `float interp = cubicInterp(currentSlot->audio.getReadPointer(0), currentSlot->audio.getNumSamples(), pos, currentSlot->loopStart, currentSlot->loopEnd);` (mono-from-stereo for 2.1; 2.2 promotes to per-channel by reading both pointers — slots are guaranteed stereo per D2-10). Apply ADSR: `float env = adsr.getNextSample();` and write `out.addSample(0, startSample + i, interp * env); out.addSample(1, startSample + i, interp * env);`.
     - Advance: `pos += playRate;`
     - End-of-sample (`pos >= currentSlot->audio.getNumSamples() - 4`) and `loopEnd == 0` → hold last sample value × env until ADSR completes (EC-4).
     - Allocation-free, lock-free, deterministic. Verify under pluginval allocation guard.
   - Depends on: Task 4

6. [ ] **Add Phase-2.1 in-memory test fixture (gated by macro)**
   - Files: `Source/PluginProcessor.cpp`, `plugins/O-MicrotonalSampler/CMakeLists.txt`
   - In `prepareToPlay`, behind `#ifdef O_MICROTONAL_SAMPLER_PHASE_2_1_TEST_FIXTURE`: build a `std::shared_ptr<SampleMap>` containing one `SampleSlot` per MIDI note 21..108 (for FUNC-04 range), 1 layer each, populated with a 0.25 s × `sampleRate` 440 Hz sine burst (or recorded at the slot's MIDI note's frequency to make the varispeed math identity-correct), `sourceSampleRate = sampleRate`, `loopStart/End = 0`. Atomic-store into `currentSampleMap`.
   - In `CMakeLists.txt`, add a CMake option `OMTS_PHASE_2_1_TEST_FIXTURE` defaulting `ON` for Phase 2.1, that defines `O_MICROTONAL_SAMPLER_PHASE_2_1_TEST_FIXTURE=1` via `target_compile_definitions(O-MicrotonalSampler PRIVATE ...)`. Phase 2.2 flips it OFF.
   - **Important:** Stage 1's `SampleMap::findSlot` returns `nullptr` unconditionally. The test fixture cannot work until Task 7 fills the body. Either (a) fill `findSlot` in this task and order Task 7 before Task 6, or (b) keep this task simple by hand-storing the slot pointer into a 2.1-only `voice->setTestSlot(...)` setter that bypasses `findSlot`. **Choose (a)**: do the trivial `findSlot` linear scan now (it's needed for 2.2 anyway), and `clearCurrentNote()` on miss exactly per EC-1.
   - Depends on: Task 7 (re-ordered: 7 lands first; this task lights up the fixture)

7. [ ] **Fill `SampleMap::findSlot` body (linear scan)**
   - Files: `Source/SampleMap.h` (inline) — or split body into new `Source/SampleMap.cpp` if header bloat warrants.
   - Linear scan over `slots`: return first slot where `slot.midiNote == midiNote && slot.velocityLayer == velocityLayer`; else `nullptr`. ~10 ns over 352 entries (88 × 4); trivial. `velocity` parameter is the `velocityLayer` index, not a 0-127 value (clarifies the call site).
   - Update inline doc to remove "Stage 2.2 implements" qualifier.
   - Depends on: none — but order before Task 6.

8. [ ] **Wire `voice->prepareToPlay` from `OMicrotonalSamplerAudioProcessor::prepareToPlay`**
   - Files: `Source/PluginProcessor.cpp`
   - Loop over `synthesiser.getNumVoices()` and call `static_cast<MicrotonalSamplerVoice*>(voice)->prepareToPlay(sampleRate, samplesPerBlock);` per voice.
   - Add `juce::SmoothedValue<float> outputGainSmoother;` (RESEARCH R7, pitfall #8) initialized in `prepareToPlay`: `outputGainSmoother.reset(sampleRate, 0.01);` and seeded with current dB-to-linear value. Apply post-render in `processBlock` via `buffer.applyGainRamp(...)` per channel using start/end snapshots (RESEARCH pitfall #9).
   - Depends on: Tasks 2, 5

9. [ ] **Phase 2.1 Gate 1 verification**
   - Build: `ninja O-MicrotonalSampler_VST3 O-MicrotonalSampler_AU O-MicrotonalSampler_Standalone` — all green.
   - Install fresh per CLAUDE.md cache-clearing protocol.
   - `auval -v aumu OMtS OuDv` — pass.
   - `pluginval --strictness 5 --validate ~/Library/Audio/Plug-Ins/VST3/O-MicrotonalSampler.vst3` — pass, **including allocation guard** (no allocs in `processBlock`).
   - Standalone smoke: hit MIDI key → fixture sine burst plays cleanly with applied ADSR; varispeed audibly tracks NE pitch shift if a Dorico-style pitch-bend or a manual NE event is injected.
   - **RQ-1 sine-sweep null test (codified):**
     - Build a temporary unit-test driver (`Source/tests/aliasing_check.cpp`, NOT part of the shipped plugin — `add_executable` with `EXCLUDE_FROM_ALL` in CMakeLists, manual run only).
     - Generate a 5 s exponential 20 Hz → Nyquist sine sweep at host SR; load as `SampleSlot`.
     - Render once at +0c (`playRate = 1.0`), once at +50c (`playRate ≈ 1.02930`).
     - FFT both outputs (e.g., `juce::dsp::FFT`) and compute the +50c residual after image-bin matching. **Threshold:** any alias bin > −60 dBFS above 8 kHz → pre-filter required.
     - **If pre-filter required:** add `juce::dsp::IIR::Filter<float> antiAlias;` to voice (per channel — pitfall #10), `juce::dsp::IIR::Coefficients<float>::makeLowPass(hostSR, 22000.0f)` in `prepareToPlay`, run after `cubicInterp` and before envelope multiply. Re-run aliasing test, document outcome in `RESEARCH.md` Decision Refinements as R8.
     - **If clean:** document negative result in commit message; no code change.
   - **Commit:** `feat(O-MicrotonalSampler): cubic-Hermite varispeed voice + ADSR + NE - Phase 2.1 Gate 1 PASS`
   - Depends on: Tasks 1–8

---

### Phase 2.2 — Sample loader + filename parser (Gate 2)

10. [ ] **Create `Source/FilenameParser.h` + `.cpp` (RQ-5 / D2-7)**
    - Files: `Source/FilenameParser.h`, `Source/FilenameParser.cpp`
    - Pure function `std::optional<ParsedName> FilenameParser::parse(const juce::String& filenameNoExtension);` where `struct ParsedName { int midiNote; int velLayer; };`.
    - Tokenizer: split on `[_\-\s.]+` via `juce::StringArray::addTokens(...)`.
    - Note regex set per RESEARCH RQ-5 table (case-insensitive note letter + optional `#`/`b` + signed octave; `MIDI60`/`midi72`; bare number `[0-127]` only when no other token parses as a note).
    - Velocity regex set: `v[1-4]`, `vel[1-4]`, `p`/`mp`/`mf`/`f`, `layer[N]`/`L[N]`/`Lyr[N]`. Default `velLayer = 0` if no velocity token.
    - Note letter → semitone offset table: C=0, D=2, E=4, F=5, G=7, A=9, B=11. Sharps add 1, flats subtract 1. MIDI = `(octave + 1) * 12 + semitoneOffset` (so `C4 = 60`, `A-1 = 9`).
    - Unit tests inline (gated `#ifdef OMTS_UNIT_TESTS`) per RESEARCH RQ-5 fixture table.
    - Depends on: none

11. [ ] **Implement `SampleLoader::run()` body**
    - Files: `Source/SampleLoader.cpp`, `Source/SampleLoader.h`
    - Inside `run()`:
      1. Construct `juce::AudioFormatManager formatManager; formatManager.registerBasicFormats();` (LOCAL, not member — pitfall #9 carry-over).
      2. Iterate `pendingFolder` via `juce::DirectoryIterator` (or `RangedDirectoryIterator`, JUCE 8 preferred), filter by `*.wav;*.aif;*.aiff;*.flac`.
      3. For each file: check `threadShouldExit()` (RESEARCH pitfall #11); parse filename via `FilenameParser::parse(file.getFileNameWithoutExtension())`. Skip + log on `nullopt`.
      4. Open reader: `std::unique_ptr<juce::AudioFormatReader> reader { formatManager.createReaderFor(file) };` (pitfall #14). Skip + log if null.
      5. Allocate temp buffer `juce::AudioBuffer<float> sourceBuf((int)reader->numChannels, (int)reader->lengthInSamples);` and `reader->read(&sourceBuf, 0, (int)reader->lengthInSamples, 0, true, true)`.
      6. **SR conversion (D2-9, RQ-6):** if `std::abs(reader->sampleRate - targetSampleRate) > 0.1`:
         - `double srcRatio = reader->sampleRate / targetSampleRate;`
         - `int outNumSamples = (int)std::ceil(sourceBuf.getNumSamples() / srcRatio);`
         - Allocate `juce::AudioBuffer<float> resampled(reader->numChannels, outNumSamples);`
         - **Per channel:** `juce::LagrangeInterpolator interp; interp.reset(); interp.process(srcRatio, sourceBuf.getReadPointer(ch), resampled.getWritePointer(ch), outNumSamples);` — separate instance per channel (pitfall #10).
         - Log INFO if `srcRatio ∈ [0.25, 4.0]`; WARN if outside (RQ-6 fallback policy).
      7. **Mono → stereo (D2-10):** allocate the slot's `audio` buffer at 2 channels × output samples. If `reader->numChannels == 1`: copy mono to both L and R at unity gain. If 2: copy straight through (pitfall #15).
      8. Populate `SampleSlot` fields: `audio` (the stereo buffer), `sourceSampleRate = targetSampleRate` (it's now host-SR), `midiNote`, `velocityLayer`, `loopStart = 0`, `loopEnd = 0` (Phase 2.5 fills loop fields).
      9. Push into `std::vector<SampleSlot>`. After loop, build the `SampleMap`: scan slots → derive `lowestNote`, `highestNote`, `numVelocityLayers = max(velLayer)+1` clamped to [1,4].
      10. Wrap in `auto map = std::make_shared<SampleMap>(std::move(builtMap));` and call `juce::MessageManager::callAsync([cb = completionCallback, map]() { if (cb) cb(map); });` (pitfall #12).
      11. On any abort: dispatch via `failureCallback` with a descriptive `juce::String`.
    - Add `juce::StringArray skippedFiles;` field on `SampleLoader` (member is fine — touched only by the loader thread, then captured by the message-thread callback). Decision: surface skipped list to processor by extending `CompletionCallback` to `void(std::shared_ptr<SampleMap>, juce::StringArray skipped)` — Stage 3 UI consumes; Phase 2.2 just stores in processor for now.
    - **`loadFolder` body update**: store `pendingFolder`, `targetSampleRate`, `completionCallback`, `failureCallback`; `startThread();` (Stage-1 stub had no startThread — that's the change). Drop the Stage-1 unconditional failure-callback fast-path.
    - Depends on: Task 10

12. [ ] **Update `OMicrotonalSamplerAudioProcessor` to consume loader completion**
    - Files: `Source/PluginProcessor.h`, `Source/PluginProcessor.cpp`
    - Add `juce::StringArray lastSkippedFiles;` member (used by Stage-3 UI; populated on completion).
    - In ctor, configure `sampleLoader->loadFolder(...)` callbacks to atomic-store the new `shared_ptr<SampleMap>` into `currentSampleMap` on completion: `std::atomic_store(&currentSampleMap, newMap);` (or use `std::shared_ptr` `std::atomic_*` if compiler still supports it; otherwise wrap in `std::atomic<std::shared_ptr<SampleMap>>` if C++20 available — confirm in CMake `CXX_STANDARD`). Note: per RESEARCH pitfall #4 the **voice** uses `std::atomic_load(sampleMapSource)` to acquire; processor side must match.
    - Add a public method `void loadSampleFolder(const juce::File& folder)` that the editor's Phase-2.2 button calls.
    - Remove (or `#ifndef OMTS_PHASE_2_1_TEST_FIXTURE`-guard) the in-memory test fixture from `prepareToPlay` — Phase 2.2 obsoletes it. CMake option flips OFF.
    - Depends on: Task 11, Tasks 6, 8

13. [ ] **Add minimal "Load Folder…" button to `PluginEditor`**
    - Files: `Source/PluginEditor.h`, `Source/PluginEditor.cpp`
    - Single `juce::TextButton loadFolderButton { "Load Folder..." };` member. On click, open `juce::FileChooser` (modal-async per JUCE 8 idiom) and call `processor.loadSampleFolder(chosenFolder)`.
    - **Stage 3 will replace this entirely**; keep it ugly and minimal — just enough to test 2.2 in DAW + Standalone.
    - Depends on: Task 12

14. [ ] **CMake: register new sources**
    - Files: `plugins/O-MicrotonalSampler/CMakeLists.txt`
    - Add `Source/FilenameParser.cpp` (and `.h` listing for IDE) to the source list. (`SampleMap.cpp` only added if Task 7 split it out.)
    - Flip `OMTS_PHASE_2_1_TEST_FIXTURE` default to `OFF`.
    - Depends on: Tasks 7, 10

15. [ ] **Phase 2.2 Gate 2 verification**
    - Build triple + install per CLAUDE.md.
    - Smoke test: drop `tests/fixtures/4-sample-set/` (build if absent: `C4_v1.wav`, `C4_v2.wav`, `D4_v1.wav`, `D4_v2.wav` synthesized sines) → all 4 audibly map and play correctly.
    - Malformed-name test: `tests/fixtures/mixed-names/` containing `weird-name.wav` + a parseable file → unparseable silently skipped, parseable plays. `lastSkippedFiles` populated.
    - Mixed-SR test: `tests/fixtures/mixed-sr/` containing one 44.1k file + one 96k file → both play **in tune** (varispeed + LagrangeInterpolator combine correctly). Subjective test: hit both notes on the same chord, verify no detuning relative to each other.
    - Concurrent reload: hold a sustained note → drop a different folder → active note keeps playing the OLD map, next note picks up new map (EC-3).
    - `pluginval --strictness 5` clean.
    - **Commit:** `feat(O-MicrotonalSampler): background sample loader + filename parser + SR conversion - Phase 2.2 Gate 2 PASS`
    - Depends on: Tasks 10–14

---

### Phase 2.3 — Velocity-layer crossfade (Gate 3)

16. [ ] **Add per-voice layer crossfade state**
    - Files: `Source/MicrotonalSamplerVoice.h`
    - Adds: `const SampleSlot* slotLow = nullptr;`, `const SampleSlot* slotHigh = nullptr;`, `float layerWeightLow = 1.0f;`, `float layerWeightHigh = 0.0f;`, `double posHigh = 0.0;`, `double playRateHigh = 1.0;` (high slot may be at a different `sourceSampleRate` if the user mixed SRs across velocity layers — usually identical, but defensive).
    - Replace the single-`currentSlot` member with the dual-slot members. `currentSlot` becomes `slotLow` for clarity.
    - Depends on: Task 1

17. [ ] **Update `startNote` to compute equal-power layer weights (RQ-7 Site 1)**
    - Files: `Source/MicrotonalSamplerVoice.cpp`
    - Read `velocity_crossfade` once from APVTS at `startNote` (no SmoothedValue — value is consumed once per note, RESEARCH §RQ-7).
    - Apply geometry from RESEARCH RQ-7 Site 1 pseudocode: pick `layerIdx`, `layerCenter`, `distanceCenter`, `fadeWidthSamples = velocity_crossfade * (layerWidth / 2)`. If within fade region AND adjacent layer exists in the map → look up `slotHigh = currentMap->findSlot(midi, layerIdx ± 1)`; compute `equalPowerWeights(x)`; assign `wLow / wHigh`. Else `slotHigh = nullptr; wLow = 1; wHigh = 0;`.
    - Compute per-slot `playRateLow` / `playRateHigh` independently (their `sourceSampleRate` could differ).
    - `posHigh = 0.0;`
    - **EC-5 (vel exactly at boundary):** the geometry naturally returns `wLow = wHigh = 0.707` — verify by unit test below.
    - Depends on: Tasks 4, 16

18. [ ] **Add inline `equalPowerWeights` helper (anonymous namespace)**
    - Files: `Source/MicrotonalSamplerVoice.cpp`
    - `static inline std::pair<float, float> equalPowerWeights(float x) noexcept { const float t = juce::jlimit(0.0f, 1.0f, x) * juce::MathConstants<float>::halfPi; return { std::cos(t), std::sin(t) }; }` — inlined trig is cheap at note-rate (only called in `startNote`); no LUT needed at this site (LUT is for the per-sample loop crossfade in 2.5).
    - Depends on: none

19. [ ] **Update `renderNextBlock` to mix two slots when `slotHigh != nullptr`**
    - Files: `Source/MicrotonalSamplerVoice.cpp`
    - Per-sample: read `lowSample = cubicInterp(slotLow, posLow, ...)`, advance `posLow += playRateLow`. If `slotHigh`, read `highSample = cubicInterp(slotHigh, posHigh, ...)`, advance `posHigh += playRateHigh`. Output `(lowSample * layerWeightLow + highSample * layerWeightHigh) * env` (mono-source path); for stereo, channel-by-channel.
    - End-of-sample EC-4 path applies independently to each slot — once both have ended, ADSR continues but output is silent.
    - Depends on: Tasks 17, 18

20. [ ] **Phase 2.3 Gate 3 verification**
    - Build a 4-layer fixture: `tests/fixtures/4-layer/{C4_v1.wav,C4_v2.wav,C4_v3.wav,C4_v4.wav}` synthesized as the same sine but at amplitudes 0.25 / 0.5 / 0.75 / 1.0 (so layer transitions are audible).
    - Drag-load fixture; sweep MIDI velocity 1 → 127 → audible crossfade between layers, no clicks at boundaries.
    - `velocity_crossfade = 0.0` → hard switch at boundaries 32 / 64 / 96.
    - `velocity_crossfade = 1.0` → maximum overlap (50 % into each adjacent layer).
    - Velocity exactly at 64 → both adjacent layers contribute (verify by amplitude — a v2 (amp 0.5) and v3 (amp 0.75) at 0.707 each gives ≈ 0.884 RMS).
    - `pluginval --strictness 5` clean.
    - **Commit:** `feat(O-MicrotonalSampler): equal-power velocity-layer crossfade - Phase 2.3 Gate 3 PASS`
    - Depends on: Tasks 16–19

---

### Phase 2.4 — Voice-stealing with 5-ms ramp (Gate 4)

21. [ ] **Add steal-tail buffers to `MicrotonalSamplerVoice`**
    - Files: `Source/MicrotonalSamplerVoice.h`
    - Adds: `std::vector<float> stealTailBufferL;`, `std::vector<float> stealTailBufferR;`, `int stealTailSamplesRemaining = 0;`, `int kMaxStealRamp = 0;` (computed in `prepareToPlay`).
    - Depends on: Task 1

22. [ ] **Allocate steal-tail buffers in `prepareToPlay` (message-thread allocation, pitfall #6)**
    - Files: `Source/MicrotonalSamplerVoice.cpp`
    - `kMaxStealRamp = (int)std::ceil(0.005 * sampleRate);` plus a small safety margin (`+ 16`); resize both buffers to `kMaxStealRamp`.
    - Depends on: Task 21, Task 2

23. [ ] **Implement `renderTailRamp(int rampSamples)` private helper**
    - Files: `Source/MicrotonalSamplerVoice.cpp`
    - Renders the OLD note for `rampSamples` more samples into the steal-tail buffers, multiplied by a linear ramp from `currentEnvValue → 0`. Reads the same `cubicInterp` + dual-layer + ADSR-snapshot path used by `renderNextBlock`, but writes to the local buffers.
    - **Single-shot use:** called from inside `startNote` BEFORE state reset. After this returns, the OLD note's sample state is captured in the buffers; we proceed to wipe state and start the new note.
    - Snapshot envelope value: rather than peeking ADSR (no public peek), capture `float lastEnv = adsr.getNextSample();` at function entry — cheap, advances ADSR by one sample, but ADSR is about to be reset anyway. Linear-ramp from `lastEnv` to `0` over `rampSamples`.
    - The two-slot path (Phase 2.3 layer crossfade) is preserved during tail render — both slots contribute weighted by their layer weights, advancing their own `pos` cursors.
    - Depends on: Tasks 19, 22

24. [ ] **Wire steal detection into `startNote`**
    - Files: `Source/MicrotonalSamplerVoice.cpp`
    - At top of `startNote(...)`, BEFORE any state reset:
      ```
      if (adsr.isActive() && (slotLow != nullptr || slotHigh != nullptr))
      {
          const int rampSamples = juce::jmin(kMaxStealRamp, (int)stealTailBufferL.size());
          renderTailRamp(rampSamples);
          stealTailSamplesRemaining = rampSamples;
      }
      ```
    - Continue with normal `startNote` body (state reset, new ADSR onset, etc.).
    - Depends on: Task 23

25. [ ] **Mix steal tail into `renderNextBlock`**
    - Files: `Source/MicrotonalSamplerVoice.cpp`
    - At top of `renderNextBlock`, BEFORE the main per-sample loop:
      ```
      if (stealTailSamplesRemaining > 0)
      {
          const int n = juce::jmin(stealTailSamplesRemaining, numSamples);
          const int offset = kMaxStealRamp - stealTailSamplesRemaining;
          out.addFrom(0, startSample, stealTailBufferL.data() + offset, n);
          out.addFrom(1, startSample, stealTailBufferR.data() + offset, n);
          stealTailSamplesRemaining -= n;
      }
      ```
    - Then run the normal new-note render path on top (additive — `addSample`, not `setSample`).
    - Depends on: Task 24

26. [ ] **Verify `setNoteStealingEnabled(true)` is set explicitly**
    - Files: `Source/PluginProcessor.cpp`
    - In ctor (or `prepareToPlay`): `synthesiser.setNoteStealingEnabled(true);` — JUCE default is true, but make it explicit per RESEARCH §RQ-3+RQ-4 final design.
    - Confirm RESEARCH R1: **no `findVoiceToSteal` override** — JUCE default already implements oldest-released → oldest-keyup → oldest-non-protected. D2-2 satisfied by stock JUCE.
    - Depends on: none

27. [ ] **Stop-note path: graceful release when ADSR-release > 0**
    - Files: `Source/MicrotonalSamplerVoice.cpp`
    - `stopNote(velocity, allowTailOff)`: if `allowTailOff` → `adsr.noteOff();` (ADSR plays out the release tail naturally). If NOT `allowTailOff` → JUCE only calls this with `allowTailOff=false` for hard-kill (rare; not the steal path). Behavior: `adsr.reset(); slotLow = slotHigh = nullptr; clearCurrentNote();` — but this never fires in practice because steal flows through `startNote` directly per RESEARCH §RQ-3.
    - Polyphony reduction (EC-9): JUCE handles this — when user reduces `polyphony` parameter, the lower-numbered voices remain active until their ADSR completes; new notes steal-pick from the freed pool. **No code change needed**; verify behaviorally at gate.
    - Depends on: Task 5

28. [ ] **Phase 2.4 Gate 4 verification**
    - Build triple + install.
    - Hold 16 sustained notes (use a sustain-pedal-friendly test pad with ADSR sustain=1, release=0.3) → trigger 17th note → 17th plays cleanly, one of the 16 fades over ~5 ms (audible only as a soft drop-out, no click).
    - Set ADSR `release = 0.0` → still no click on steal (the 5-ms forced ramp covers it).
    - Set ADSR `release = 2.0` → released voices have natural tails AND become preferred steal targets (JUCE default behavior). Steal of an already-released voice → ramp still applied to current envelope value (likely small), inaudible.
    - Reduce `polyphony` 16 → 8 mid-performance with all 16 held → next note-on triggers steal cycle, excess voices steal-released cleanly (EC-9). No clicks.
    - `pluginval --strictness 5` clean.
    - **Commit:** `feat(O-MicrotonalSampler): voice-steal 5-ms tail ramp - Phase 2.4 Gate 4 PASS`
    - Depends on: Tasks 21–27

---

### Phase 2.5 — Loop auto-detect + crossfade (Gate 5)

29. [ ] **Create `Source/LoopDetector.h` + `.cpp` (RQ-2 / D2-4)**
    - Files: `Source/LoopDetector.h`, `Source/LoopDetector.cpp`
    - Pure function `struct LoopRegion { int loopStart; int loopEnd; bool valid; }; LoopRegion detectLoop(const juce::AudioBuffer<float>& buf, double sampleRate);`.
    - Body per RESEARCH RQ-2:
      1. Read channel 0 (mono analysis); for stereo, `(L+R)*0.5` mixdown into a `std::vector<float>` working buffer.
      2. `windowSize = 1024; searchStart = (int)(N * 0.40); searchEnd = N - windowSize - 64; stride = 256;`.
      3. For `i in [searchStart..searchEnd] step stride`: `rms[i] = sqrt(mean(buf[i..i+windowSize-1]^2))`. Track `minIdx = argmin(rms)`.
      4. **Variance gate:** `meanRms = mean(rms);` if `rms[minIdx] / meanRms > 0.7` → return `{0, 0, false}` (one-shot fallback, EC-7).
      5. **Zero-crossing snap:** scan `[minIdx - 64, minIdx + 64]` for first sign change with falling-edge preference; pick that as `loopStart`. If no zc found, use `minIdx` as-is.
      6. **Target loop length:** `targetLen = std::max(2048, (int)(sampleRate / 50.0));`. Scan `[loopStart + targetLen - 64, loopStart + targetLen + 64]` for same-direction zc; pick as `loopEnd`. If `loopEnd >= N - 16` (no headroom for crossfade), shrink `targetLen` to `N - loopStart - 64` and re-scan; if STILL no fit, return `{0, 0, false}`.
      7. **Defensive guard (RESEARCH Phase-2.5 Gate 5 item 4):** if `loopEnd - loopStart < 16` → return `{0, 0, false}` (8-sample crossfade would consume half the loop region).
      8. Else return `{loopStart, loopEnd, true}`.
    - Depends on: none

30. [ ] **Wire `LoopDetector` into `SampleLoader::run`**
    - Files: `Source/SampleLoader.cpp`, `plugins/O-MicrotonalSampler/CMakeLists.txt`
    - After loading + SR-converting + stereo-promoting each slot, call `auto region = LoopDetector::detectLoop(slot.audio, targetSampleRate);` and populate `slot.loopStart`, `slot.loopEnd` (set to 0,0 if `!region.valid`).
    - CMake: add `Source/LoopDetector.cpp` to source list.
    - Depends on: Task 29, Task 11

31. [ ] **Add 8-sample equal-power crossfade LUT (anonymous namespace)**
    - Files: `Source/MicrotonalSamplerVoice.cpp`
    - At voice ctor / static init: pre-compute `static constexpr std::array<std::pair<float,float>, 8> kLoopXfadeLUT = ...` populated from `equalPowerWeights(i / 8.0f)` for `i ∈ [0..7]`. Use a `constexpr`-friendly approximation if `std::cos` isn't constexpr in the toolchain (most modern compilers support it; otherwise compute once at static-init time in an anonymous-namespace lambda).
    - Depends on: Task 18

32. [ ] **Implement loop-wrap + 8-sample crossfade in `cubicInterp` callers**
    - Files: `Source/MicrotonalSamplerVoice.cpp`
    - In `renderNextBlock` per-sample loop, when `slot.loopEnd > 0`:
      - If `pos < loopEnd - 8`: standard `cubicInterp` read (with wrap inside the helper for the 4-tap context).
      - If `loopEnd - 8 <= pos < loopEnd`: `int xIdx = (int)(pos - (loopEnd - 8)); auto [wOut, wIn] = kLoopXfadeLUT[xIdx];`. Read `outSample = cubicInterp(buf, N, pos, /*no-wrap*/ 0, 0); inSample = cubicInterp(buf, N, pos - (loopEnd - loopStart), loopStart, loopEnd);`. Output = `outSample*wOut + inSample*wIn`.
      - If `pos >= loopEnd`: `pos -= (loopEnd - loopStart);` (wrap).
    - Apply this logic for BOTH `slotLow` and `slotHigh` cursors independently (each may have different loop regions).
    - Depends on: Tasks 19, 30, 31

33. [ ] **Phase 2.5 Gate 5 verification**
    - Build triple + install.
    - **Sustained sine (5 s, 440 Hz):** loops at the same level forever, no audible crossfade artifact. Loop detector should pick a steady region in the 40-100 % zone.
    - **Vibrato cello sample** (synthesize: 440 Hz with 5 Hz vibrato, 5 s): loops without pitch jump or click.
    - **Transient percussion** (synthesize: kick-drum impulse): falls back to one-shot (`region.valid == false`) → ADSR release ends voice (EC-7).
    - **Short loop region edge case:** synthesize a sample where the only low-RMS region is < 16 samples → defensive guard fires, falls back to one-shot.
    - Acceptance suite (RESEARCH Phase 2 Final §1): re-run all previous gate fixtures (4-sample-set, 4-layer, 16-voice steal) — no regressions.
    - **CPU benchmark:** Apple Silicon, 48 kHz / 256 buffer, 16 sustained voices at full polyphony with looping samples → measure CPU% via Logic Pro CPU meter or `pluginval --benchmark-voices`. Target: ≤ 5% (PERF-02). If above 5%, investigate cubic-Hermite hot path; consider if conditional pre-filter from Gate 1 was overzealous.
    - **Allocation audit:** `pluginval --strictness 5` reports zero allocs in `processBlock` (PERF-01).
    - **Latency audit:** `getLatencySamples()` returns 0 (PERF-04). Per project memory: the getter is non-virtual in JUCE 8; default value is 0; no override needed.
    - **Full requirement traceability:** verify FUNC-01..04, FUNC-07, DSP-01..05, DSP-07, DSP-08, PERF-01..04, COMPAT-02, QUAL-01 all satisfied by walking REQUIREMENTS.md acceptance criteria one by one. Update `STATUS.md` with a green check per requirement.
    - **Commit:** `feat(O-MicrotonalSampler): loop auto-detect + 8-sample equal-power crossfade - Phase 2.5 Gate 5 PASS`
    - Depends on: Tasks 29–32

---

## Files to Create or Modify

| File | Action | Phase |
|---|---|---|
| `Source/MicrotonalSamplerVoice.h` | Modify (add ADSR, pos, playRate, currentSlot, currentMap, layer-crossfade, steal buffers, prepareToPlay decl) | 2.1, 2.3, 2.4 |
| `Source/MicrotonalSamplerVoice.cpp` | Modify (startNote, stopNote, renderNextBlock, cubicInterp, equalPowerWeights, renderTailRamp, loop crossfade) | 2.1, 2.3, 2.4, 2.5 |
| `Source/SampleMap.h` | Modify (fill `findSlot` body) | 2.1 |
| `Source/SampleLoader.h` | Modify (extend completion callback signature with `StringArray skipped`) | 2.2 |
| `Source/SampleLoader.cpp` | Modify (implement `run()`: AudioFormatManager + FilenameParser + LagrangeInterpolator + stereo + LoopDetector wiring) | 2.2, 2.5 |
| `Source/FilenameParser.h` | Create (pure-function tolerant parser) | 2.2 |
| `Source/FilenameParser.cpp` | Create | 2.2 |
| `Source/LoopDetector.h` | Create (pure-function RMS + zc-snap + variance gate) | 2.5 |
| `Source/LoopDetector.cpp` | Create | 2.5 |
| `Source/PluginProcessor.h` | Modify (outputGainSmoother member, lastSkippedFiles, loadSampleFolder method) | 2.1, 2.2 |
| `Source/PluginProcessor.cpp` | Modify (voice prepareToPlay loop, output-gain smoothing, loader callback wiring, atomic_store of map, optional 2.1 test fixture, explicit setNoteStealingEnabled) | 2.1, 2.2, 2.4 |
| `Source/PluginEditor.h` | Modify (loadFolderButton member) | 2.2 |
| `Source/PluginEditor.cpp` | Modify (button callback → FileChooser → processor.loadSampleFolder) | 2.2 |
| `plugins/O-MicrotonalSampler/CMakeLists.txt` | Modify (add FilenameParser, LoopDetector, optionally SampleMap.cpp; add OMTS_PHASE_2_1_TEST_FIXTURE option) | 2.1, 2.2, 2.5 |
| `Source/tests/aliasing_check.cpp` | Create (manual aliasing measurement driver, EXCLUDE_FROM_ALL) | 2.1 |

**No new module dependencies.** `juce::juce_audio_formats` and `juce::juce_dsp` already linked from Stage 1; both shared modules (`note-expression`, `scala-tuning-engine`) already wired headless.

---

## Dependency Graph (Wave Boundaries)

```
Phase 2.1: 1 → 2 → 3 → 7 → 6 → 4 → 5 → 8 → 9(Gate 1)
Phase 2.2: 10, 14 (parallel) → 11 → 12 → 13 → 15(Gate 2)
Phase 2.3: 16 → 18 → 17 → 19 → 20(Gate 3)
Phase 2.4: 21 → 22 → 23 → 24 → 25 → 26, 27 (parallel) → 28(Gate 4)
Phase 2.5: 29 → 30, 31 (parallel) → 32 → 33(Gate 5)
```

Gate transitions are atomic commits. **No sub-stage starts before the prior gate passes** (D2-6).

---

## Success Criteria (Phase 2 Final)

- [ ] All 5 sub-stage gates pass with green builds and clean pluginval `--strictness 5`.
- [ ] Drag-drop a typical sample library (88 notes × 4 layers) → all notes audibly play, mapped correctly.
- [ ] MIDI velocity sweep across layer boundaries → equal-power crossfade, no clicks, no power dips.
- [ ] 17-note polyphony test with 16-voice limit → clean steal, no clicks (ADSR release = 0 worst case).
- [ ] Sustained-tone samples loop seamlessly; transient samples one-shot cleanly.
- [ ] Source SR ≠ host SR (e.g., 96 k → 48 k) → in-tune playback.
- [ ] +50c retune via VST3 NE → audibly correct pitch shift, no audible aliasing on suite-target material (or pre-filter activated and verified).
- [ ] CPU ≤ 5 % @ 16 voices on Apple Silicon, 48 kHz, 256 buffer (PERF-02).
- [ ] Zero allocations in `processBlock` (PERF-01, allocation guard).
- [ ] Zero added latency (PERF-04, `getLatencySamples() == 0`).
- [ ] All 15 Stage-2 requirements (FUNC-01..04, FUNC-07, DSP-01..05, DSP-07, DSP-08, PERF-01..04, COMPAT-02, QUAL-01) traceably verified.

---

## Out of Scope (Stage 2)

- **FUNC-05** Drag-drop UI surface (Stage 3 GUI)
- **FUNC-06** Manual cell assignment / re-mapping (Stage 3 GUI)
- **DSP-06** Manual loop-region override (Stage 3 GUI control over `slot.loopStart` / `loopEnd`)
- Adaptive loop length via autocorrelation pitch detection (deferred v1.1)
- Anti-alias prefilter for >4× downsample ratios (deferred v1.1)
- Promotion of `SampleLoader/SampleMap/FilenameParser/LoopDetector` to a shared module (deferred — extract on second use)

---

## Next Phase

Ready for **execute** phase: `/plugin-execute O-MicrotonalSampler 2-dsp`

Execute orchestrator should run the 5 sub-stages sequentially, checkpointing after each gate commit. Phase 2.1 spawns `dsp-agent` with this PLAN.md + RESEARCH.md + CONTEXT.md attached and the Phase-2.1 in-memory test fixture macro defaulted ON. Each subsequent sub-stage agent inherits the prior sub-stage's commit as its base.
