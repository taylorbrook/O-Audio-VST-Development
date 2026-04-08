# Stage 2: DSP Phase 3.5 - Execution Plan

## Goal

Add per-voice 2x/4x oversampling, integrate TuningEngine for microtonal tuning, complete MPE frequency handling with tuning-aware pitchbend, and profile CPU performance. After this phase, 33 of 35 parameters are active (instrumentPreset deferred to GUI morph).

---

## Scope Adjustments (from Discuss Phase)

| ROADMAP Item | Decision |
|---|---|
| Newton-Raphson solver for 4x | **Excluded** — same polynomial solver at higher OS rate |
| Instrument morph system | **Deferred to Stage 3** — DSP already responds to smooth param changes |

---

## Tasks

### 1. [ ] Add per-voice Oversampling members and voiceBuffer to ReedWindVoice

**Files:** `Source/ReedWindVoice.h`

**Changes:**
- Add `#include <juce_dsp/juce_dsp.h>` (already present)
- Add two `juce::dsp::Oversampling<float>` members: `oversampling2x { 1, 1, filterHalfBandPolyphaseIIR }` and `oversampling4x { 1, 2, filterHalfBandPolyphaseIIR }`
- Add `juce::AudioBuffer<float> voiceBuffer`
- Add `int currentOsFactor = 1` (1=2x, 2=4x)
- Add `TuningEngine* tuningEngine = nullptr` + `void setTuningEngine(TuningEngine* engine)`
- Add `float getBaseFrequencyFromTuning(int midiNote) const` helper declaration

**Depends on:** none

---

### 2. [ ] Initialize Oversampling and TuningEngine in prepare() and add helper methods

**Files:** `Source/ReedWindVoice.cpp`

**Changes in `prepare()`:**
- Call `oversampling2x.initProcessing(maxBlockSize)` and `oversampling4x.initProcessing(maxBlockSize)`
- Allocate `voiceBuffer.setSize(1, maxBlockSize)`
- Set `currentOsFactor = 1` (default 2x)
- Prepare all DSP components at 2x rate (default): `sampleRate * 2.0` for reedModel, bore, bore2, breathEnv, breathNoise, chamber
- Reset oversampling state

**Add methods:**
- `setTuningEngine()` — store pointer
- `getBaseFrequencyFromTuning(int midiNote)` — returns `tuningEngine->getFrequency(midiNote)` cast to float, with 12-TET fallback if nullptr
- `getActiveOversampling()` — returns ref to 2x or 4x based on `currentOsFactor`

**Depends on:** Task 1

---

### 3. [ ] Wire TuningEngine in PluginProcessor constructor and processBlock

**Files:** `Source/PluginProcessor.cpp`

**Changes in constructor (voice creation loop):**
- Add `voice->setTuningEngine(&tuningEngine)` after `voice->setAPVTS(&parameters)`

**Changes in `prepareToPlay()`:**
- After voice prepare loop, get voice 0's oversampling latency and call `setLatencySamples()`
- Pattern: `dynamic_cast<ReedWindVoice*>(synthesiser.getVoice(0))` → get latency from `oversampling2x.getLatencyInSamples()`

**Changes in `processBlock()` (before `synthesiser.renderNextBlock()`):**
- Read `referencePitch` and `tuningSystem` from APVTS
- Call `tuningEngine.setMasterTune(refPitch)`
- Switch on tuningSystem index: 0→Scala, 1→MTS-ESP, 2→TwelveTET → call `tuningEngine.setMode()`

**Depends on:** Task 2

---

### 4. [ ] Replace all getFrequencyInHertz() with tuning-aware frequency + pitchbend

**Files:** `Source/ReedWindVoice.cpp`

**Three replacement sites:**

1. **`noteStarted()` legato path (line ~107):**
   ```cpp
   // OLD: float frequency = static_cast<float>(note.getFrequencyInHertz());
   // NEW:
   float frequency = getBaseFrequencyFromTuning(note.initialNote);
   float bendSemitones = static_cast<float>(note.totalPitchbendInSemitones);
   if (std::abs(bendSemitones) > 0.001f)
       frequency *= std::pow(2.0f, bendSemitones / 12.0f);
   ```

2. **`noteStarted()` normal onset (line ~163):** Same pattern.

3. **`renderNextBlock()` per-block frequency update (line ~327):** Same pattern.

**Depends on:** Task 3

---

### 5. [ ] Wrap renderNextBlock per-sample loop with oversampling

**Files:** `Source/ReedWindVoice.cpp`

**Changes in `renderNextBlock()`:**

1. **Read oversampling param:** `int osChoice = static_cast<int>(pOversampling->load())` — 0=2x, 1=4x
2. **Detect factor change:** If `osChoice + 1 != currentOsFactor`, update `currentOsFactor`, re-prepare DSP at new oversampled rate, reset active oversampling
3. **Clear voiceBuffer** subblock of `numSamples`
4. **Create AudioBlock** from voiceBuffer subblock
5. **processSamplesUp(block)** → get oversampled block
6. **Move existing per-sample loop** to iterate over oversampled block length, writing to oversampled data pointer
7. **Update `sr` reference** for LFO phase increments to use oversampled rate inside the loop
8. **processSamplesDown(block)** → downsampled audio back in voiceBuffer
9. **Mix voiceBuffer** into outputBuffer (replace direct addSample)
10. **Post-block:** bore snapFiltersToZero + voice cleanup remain after mix

**Key detail:** All LFO phase increments (`vibratoPhase`, `growlPhase`, `flutterPhase`) use `sr` which must be the oversampled rate inside the oversampled loop. Store `float osRate = sr * float(1 << currentOsFactor)` and use that for LFO increments.

**Depends on:** Task 4

---

### 6. [ ] Report oversampling latency in prepareToPlay

**Files:** `Source/PluginProcessor.cpp`, `Source/ReedWindVoice.h`

**Changes:**
- Add public `float getOversamplingLatency() const` to ReedWindVoice — returns `oversampling2x.getLatencyInSamples()` (default, since prepare uses 2x)
- In `prepareToPlay()`, after voice prepare loop: cast voice 0 and call `setLatencySamples(static_cast<int>(std::ceil(voice->getOversamplingLatency())))`

**Depends on:** Task 2

---

### 7. [ ] Build and regression test

**Files:** none (build + test)

**Steps:**
1. `ninja O-Reed_VST3 O-Reed_AU` — zero errors, zero warnings
2. Install to system plugin folders
3. `auval -v aumu ORed Ouar` — must pass
4. Load in DAW with 12-TET default — verify sound matches Phase 3.4 output (oversampling wraps existing chain, no algorithmic change)
5. Test 4x mode — verify no clicks or artifacts on switch
6. Test Scala tuning — verify pitch shift with reference pitch change

**Depends on:** Tasks 1-6

---

### 8. [ ] CPU profiling and documentation

**Files:** `Source/ReedWindVoice.cpp` (temporary profiling code, document results in SUMMARY)

**Steps:**
1. Add `std::chrono::high_resolution_clock` measurement around voice render in processBlock
2. Measure mono 2x, mono 4x, 8-voice poly 2x at 48kHz
3. Document results in SUMMARY-3.5.md
4. Remove profiling code after measurement (or gate behind `#if 0`)
5. Fix any hotspot exceeding 2% CPU mono at 2x

**CPU targets from ROADMAP:**
- Mono 2x: < 2%
- Mono 4x: < 4%
- 8-voice poly 2x: < 16%

**Depends on:** Task 7

---

## Files Modified

| File | Changes |
|------|---------|
| `Source/ReedWindVoice.h` | +Oversampling members, voiceBuffer, TuningEngine ptr, helpers |
| `Source/ReedWindVoice.cpp` | Oversampling wrapper, tuning-aware freq, prepare changes, factor switching |
| `Source/PluginProcessor.cpp` | TuningEngine wiring (constructor + processBlock), latency reporting |

**No new files.** Oversampling wraps existing DSP chain transparently.

---

## Success Criteria

- [ ] 2x oversampling active by default — audible improvement over native rate
- [ ] 4x oversampling selectable — no clicks on factor switch
- [ ] Latency correctly reported to host via setLatencySamples()
- [ ] TuningEngine integration: reference pitch shifts all notes, tuning mode switches work
- [ ] 12-TET default produces identical pitch to Phase 3.4
- [ ] MPE pitchbend works with tuning-aware frequency (smooth per-note bend)
- [ ] All LFOs run at correct rates inside oversampled loop
- [ ] No regression: sound quality matches Phase 3.4 at 2x/12-TET
- [ ] CPU targets met (mono 2x < 2%, mono 4x < 4%, 8-voice poly < 16%)
- [ ] Zero build errors, auval pass
- [ ] 33 of 35 parameters active (instrumentPreset deferred)

---

## Parameters Activated

| Parameter | Wiring |
|-----------|--------|
| referencePitch | → `tuningEngine.setMasterTune()` in processBlock |
| tuningSystem | → `tuningEngine.setMode()` in processBlock |
| oversampling | → per-voice factor selection (0=2x, 1=4x) |
| instrumentPreset | NOT wired (deferred to GUI morph) |
