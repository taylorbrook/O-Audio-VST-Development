# Stage 2: DSP Implementation - Execution Plan

**Date:** 2026-02-07
**Plugin:** O-GrainScatter
**Goal:** Implement the complete granular stutter/scatter DSP engine — circular delay buffer, 64-voice grain pool with Hann windowing and Lagrange interpolation, beat-synced and free-mode scheduling, pitch quantization with ladder modes, Euclidean rhythm gating, freeze buffer, feedback with soft clip, and dry/wet mixing.

---

## Pre-Implementation Fix

### Task 0: Rename `texture` → `spread` and add `stutter_gate` parameter
- **Files:** `Source/PluginProcessor.h`, `Source/PluginProcessor.cpp`, `Source/PluginEditor.h`, `Source/PluginEditor.cpp`
- **Depends on:** none
- **Details:**
  - Rename parameter ID `texture` → `spread`, display name "Texture" → "Spread"
  - Rename parameter group `texture` / "Texture & Pitch" → `spread` / "Spread & Pitch"
  - Rename all cached pointers: `textureParam` → `spreadParam`, `textureRelay` → `spreadRelay`, `textureAttachment` → `spreadAttachment`
  - Add `stutter_gate` `AudioParameterBool` (default false) to the Sync parameter group
  - Add `stutterGateParam` cached pointer + relay + attachment in editor
  - Total parameters: 18
- **Verification:** Builds clean, standalone launches, all 18 params visible in host

---

## Layer 1: Core Grain Engine

### Task 1: Create `DelayBuffer.h`
- **Files:** `Source/dsp/DelayBuffer.h` (new)
- **Depends on:** none
- **Details:**
  - Header-only in `Source/dsp/`
  - Stereo circular buffer using `juce::AudioBuffer<float>`
  - `prepare(sampleRate, maxDelaySec=2.0)` — allocates buffer
  - `pushSample(float L, float R)` — writes at writePosition, advances
  - `readSample(int channel, float delaySamples)` — Lagrange 3rd-order interpolated read
  - `copyRegion(AudioBuffer<float>& dest, int startOffset, int length)` — for freeze capture
  - `getWritePosition()`, `getBufferSize()` accessors
  - All 4 Lagrange sample indices modulo-wrapped independently (Pitfall #1)

### Task 2: Create `GrainPool.h` (GrainVoice + GrainPool)
- **Files:** `Source/dsp/GrainPool.h` (new)
- **Depends on:** Task 1
- **Details:**
  - `GrainVoice` struct: active, readPosition, playbackRate, panPosition, samplesRemaining, grainLengthSamples, reverse, readFromFrozen
  - `GrainPool` class with `std::array<GrainVoice, 64>`
  - `GrainParams` struct for spawn parameters (position, rate, pan, length, reverse, fromFrozen)
  - `spawnGrain(const GrainParams&)` — round-robin oldest-steal allocation
  - `processSample(const DelayBuffer&, const FreezeManager&, float& outL, float& outR)` — iterate all voices, Hann window, Lagrange read, pan, accumulate
  - Hann window: `0.5f * (1.0f - std::cos(twoPi * phase))`, phase = elapsed/total (Pitfall #6)
  - `getActiveCount()` accessor

### Task 3: Create `GrainScheduler.h` — Free mode only
- **Files:** `Source/dsp/GrainScheduler.h` (new)
- **Depends on:** none
- **Details:**
  - `SpawnRequest` struct: `int sampleOffset`
  - Free mode: density (1-100%) → inter-grain interval. 100% = ~10ms, 1% = ~1000ms
  - `prepare(sampleRate)`, `processBlockFree(numSamples, density, probability, outRequests)`
  - `juce::Random` RNG member (real-time safe, per-instance)
  - Probability gate: `rng.nextFloat() < probability/100.0f`

### Task 4: Integrate core processBlock — delay + scheduler + pool + mix
- **Files:** `Source/PluginProcessor.h`, `Source/PluginProcessor.cpp`
- **Depends on:** Tasks 0, 1, 2, 3
- **Details:**
  - Add `#include` for all DSP headers
  - Add member instances: `DelayBuffer delayBuffer`, `GrainPool grainPool`, `GrainScheduler scheduler`
  - Add `SmoothedValue<float>` for dryWet and feedback
  - `prepareToPlay()`: init all components with sampleRate, reset SmoothedValues (rampLengthSeconds=0.02)
  - `processBlock()` implementation:
    1. Read params atomically
    2. `dryWetSmoothed.setTargetValue()`, `feedbackSmoothed.setTargetValue()` (Pitfall #5: NO reset() here)
    3. Get spawn requests from scheduler (free mode only for now)
    4. Per-sample loop: push input + feedback → check spawn → process pool → feedback calc → dry/wet mix
  - Feedback soft clip: `std::tanh(x * 3.0f) * 1.00497f * 0.95f` (Pitfall #4: clip BEFORE writing back)
  - Dry/wet: `out = dry * (1 - mix) + wet * mix` using SmoothedValue
  - `getTailLengthSeconds()` returns 2.0 (delay buffer length)
- **Verification:** Free-mode grain engine works — audio input gets scattered into grains with density control, feedback, dry/wet mixing

---

## Layer 2: Beat Sync

### Task 5: Create `TempoTracker.h`
- **Files:** `Source/dsp/TempoTracker.h` (new)
- **Depends on:** none
- **Details:**
  - `SyncInfo` struct: bpm, ppqPosition, ppqPerSample, isPlaying
  - `prepare(sampleRate)`
  - `update(AudioPlayHead*, numSamples)` → returns SyncInfo
  - Reads `getPosition()` from AudioPlayHead (JUCE 8.0.4 API)
  - Falls back to manual BPM=120 counter in standalone (`manualPpq += ppqPerSample`)
  - PPQ per sample: `bpm / (60.0 * sampleRate)`
  - Guard against PPQ backward jump on DAW loop (Pitfall #2)

### Task 6: Add sync mode to GrainScheduler
- **Files:** `Source/dsp/GrainScheduler.h` (modify)
- **Depends on:** Tasks 3, 5
- **Details:**
  - Add `processBlockSync(numSamples, SyncInfo, subdivIndex, probability, euclideanPattern, euclideanLength, outRequests)`
  - Subdivision PPQ values: {N/A, 1.0, 0.5, 0.25, 0.125, 1.0/3.0, 1.0/6.0}
  - Crossing detection: `floor(newPpq / subdiv) > floor(oldPpq / subdiv)`
  - Per-sample PPQ interpolation: `ppqAtSample = syncInfo.ppqPosition + i * syncInfo.ppqPerSample`
  - Euclidean gate check: `pattern[euclideanStep % length]`, advance step on crossing
  - Euclidean step counter: `euclideanStep %= newSteps` on steps param change (Pitfall #10)
  - Repeat logic: on trigger, schedule `repeats` grains at subdivision intervals
  - Stutter Gate: when on, mute dry signal between repeat bursts

### Task 7: Integrate sync mode into processBlock
- **Files:** `Source/PluginProcessor.cpp` (modify)
- **Depends on:** Tasks 5, 6
- **Details:**
  - Add `TempoTracker tempoTracker` member
  - Call `tempoTracker.update()` at block start
  - Route to `processBlockFree()` or `processBlockSync()` based on `syncModeParam`
  - Pass SyncInfo, subdivision index, probability, euclidean pattern to scheduler
  - Stutter gate: if active and between repeat bursts, zero the dry contribution to output
- **Verification:** Grains trigger in time with DAW transport. Subdivision changes heard correctly. Standalone falls back to 120 BPM.

---

## Layer 3: Pitch & Scale

### Task 8: Create `ScaleQuantizer.h`
- **Files:** `Source/dsp/ScaleQuantizer.h` (new)
- **Depends on:** none
- **Details:**
  - Precomputed `constexpr` 12-element lookup tables for all 5 scales (from RESEARCH.md)
  - `quantizeToScale(int semitones, int scaleIndex, int rootNote)` — O(1) lookup
  - Negative modulo guard: `((x % 12) + 12) % 12` (Pitfall #7)
  - `getNextPitch(pitchMode, scaleIndex, rootNote, pitchRandom, Random&)` → returns playback rate
  - Pitch modes:
    - Random: `rng.nextInt(25) - 12` scaled by `pitchRandom/100`, quantized
    - Ladder Up: step through scale degrees ascending, wrap at octave
    - Ladder Down: step descending, wrap
    - Pendulum: ascend then descend, skip boundary on reversal (Pitfall #8)
  - `pitchRandom` in ladder modes = depth scaler: `finalSemitones = ladderSemitones * (pitchRandom / 100.0f)`
  - Output: `std::pow(2.0f, semitones / 12.0f)`
  - `resetLadder()` — called on scale/pitchMode change (Pitfall #9)
  - Ladder state persists across freeze

### Task 9: Integrate pitch into grain spawning
- **Files:** `Source/PluginProcessor.cpp` (modify)
- **Depends on:** Tasks 4, 8
- **Details:**
  - Add `ScaleQuantizer scaleQuantizer` member
  - On grain spawn: call `scaleQuantizer.getNextPitch(...)` → set `GrainParams.playbackRate`
  - Detect scale/pitchMode param changes → call `resetLadder()`
  - Variable-rate grain playback: `readPosition += playbackRate` per sample in GrainPool
- **Verification:** Pitch modes produce musically quantized pitch shifts. Scale changes reset ladder.

---

## Layer 4: Features

### Task 10: Create `EuclideanGenerator.h`
- **Files:** `Source/dsp/EuclideanGenerator.h` (new)
- **Depends on:** none
- **Details:**
  - Namespace with `generate(int steps, int pulses)` → `std::array<bool, 16>`
  - One-liner algorithm: `(i * pulses) % steps < pulses` (verified Bjorklund equivalent)
  - Returns `std::array<bool, 16>` + actual length (no heap allocation)
  - Called on parameter change only, cached in processor

### Task 11: Create `FreezeManager.h`
- **Files:** `Source/dsp/FreezeManager.h` (new)
- **Depends on:** Task 1
- **Details:**
  - `prepare(sampleRate, maxLengthSamples)` — pre-allocate freeze buffer to 2s
  - `engage(const DelayBuffer&, int grainSizeSamples)` — copy `4 * grainSizeSamples` from delay buffer
  - `release()` — deactivate
  - `isActive()` accessor
  - `readSample(int channel, float position)` — Lagrange 3rd-order interpolated read (wraps within capture length)
  - `getCaptureLength()` accessor
  - Linear crossfade on engage: ~5ms / `(int)(sampleRate * 0.005)` samples (Pitfall #3)
  - Active grains continue from delay buffer; only NEW grains read from frozen buffer

### Task 12: Integrate all remaining features into processBlock
- **Files:** `Source/PluginProcessor.h`, `Source/PluginProcessor.cpp`
- **Depends on:** Tasks 7, 9, 10, 11
- **Details:**
  - Add `FreezeManager freezeManager` member
  - Add Euclidean pattern cache: `std::array<bool, 16> euclideanPattern`, `std::atomic<int> euclideanLength`
  - **Euclidean integration:** Detect euclidean param changes → regenerate pattern, pass to scheduler
  - **Freeze integration:** Detect freeze param toggle → `freezeManager.engage()` / `release()`. New grains set `readFromFrozen = true` when frozen.
  - **Spread control:** On grain spawn, offset read position by `spread * random(-1, 1) * grainSizeSamples`
  - **Pan randomization:** On grain spawn, `pan = 0.5f + panRandom/100.0f * random(-0.5, 0.5)`
  - **Reverse grains:** On grain spawn, `reverse = rng.nextFloat() < reverseParam/100.0f`
  - **Repeat count:** In sync mode, scheduler spawns `repeats` grains per trigger at subdivision intervals
  - **Stutter gate:** When active in sync mode, zero dry signal between repeat bursts
- **Verification:** All 18 parameters functional. Freeze captures and loops. Euclidean patterns gate correctly. Spread/pan/reverse working.

---

## Success Criteria

- [ ] All 18 parameters connected and functional
- [ ] Free mode: density-driven grain cloud with smooth feedback and dry/wet
- [ ] Sync mode: beat-locked stutters at all 6 subdivisions
- [ ] Euclidean rhythms gate grain triggers correctly (verified for E(3,8), E(5,8))
- [ ] Pitch modes: Random produces scale-quantized pitches; Ladder Up/Down/Pendulum step through degrees
- [ ] All 5 scales quantize correctly (verified against lookup tables)
- [ ] Freeze captures audio and grains loop from frozen buffer
- [ ] Feedback soft-clips at 0.95 — no runaway gain
- [ ] Spread scatters grain positions, pan randomizes stereo field
- [ ] Reverse grains play backwards at correct probability
- [ ] Stutter Gate mutes dry signal between repeat bursts when active
- [ ] No clicks or glitches on grain boundaries (Hann window envelope)
- [ ] No clicks on freeze engage/release (crossfade)
- [ ] No audio artifacts on DAW transport loop (PPQ backward jump guard)
- [ ] Zero allocations in processBlock (all buffers pre-allocated in prepareToPlay)
- [ ] Builds clean on macOS (VST3 + AU)
- [ ] Passes pluginval at strictness level 5

---

## File Summary

| File | Action | Task |
|------|--------|------|
| `Source/PluginProcessor.h` | Modify | 0, 4, 7, 9, 12 |
| `Source/PluginProcessor.cpp` | Modify | 0, 4, 7, 9, 12 |
| `Source/PluginEditor.h` | Modify | 0 |
| `Source/PluginEditor.cpp` | Modify | 0 |
| `Source/dsp/DelayBuffer.h` | Create | 1 |
| `Source/dsp/GrainPool.h` | Create | 2 |
| `Source/dsp/GrainScheduler.h` | Create | 3, 6 |
| `Source/dsp/TempoTracker.h` | Create | 5 |
| `Source/dsp/ScaleQuantizer.h` | Create | 8 |
| `Source/dsp/EuclideanGenerator.h` | Create | 10 |
| `Source/dsp/FreezeManager.h` | Create | 11 |

**Total:** 7 new files, 4 modified files, 13 tasks
