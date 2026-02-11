# Stage 2: DSP — Execution Plan

> **Plugin:** O-Orbit
> **Stage:** 2 (DSP)
> **Date:** 2026-02-10
> **Source:** CONTEXT.md, RESEARCH.md, ARCHITECTURE.md
> **Phases:** 3 (2.1 Motion + Distance + Stereo VBAP, 2.2 Full 2D VBAP + Multi-Channel, 2.3 3D VBAP + Auto-Downmix)

---

## Goal

Implement the complete DSP engine for O-Orbit: motion path generation (Orbit, Pendulum, Linear, Drift), distance modeling (attenuation + air absorption), and VBAP rendering from stereo through 24-channel output. All 17 APVTS parameters functional and producing audible spatial motion.

---

## Phase 2.1: Motion Engine + Distance Model + Stereo VBAP

### Task 1: Create PerlinNoise header
- **Files:** `Source/DSP/PerlinNoise.h` (new)
- **Depends on:** none
- **Details:**
  - Custom 1D Perlin noise + fBm, ~50 lines, header-only
  - `seed(uint32_t)` — Fisher-Yates shuffle of permutation table
  - `noise(float x)` — 1D Perlin with quintic fade
  - `fbm(float x, int octaves, float lacunarity, float persistence)` — fractal Brownian motion
  - Output range normalized to [-1, 1]
  - No external dependencies (`<cmath>`, `<cstdint>` only)

### Task 2: Implement MotionEngine path algorithms
- **Files:** `Source/DSP/MotionEngine.h` (modify), `Source/DSP/MotionEngine.cpp` (modify)
- **Depends on:** Task 1
- **Details:**
  - Add `#include "PerlinNoise.h"` and `PerlinNoise perlin` member
  - Add `float noiseTime = 0.0f` member for Drift path
  - Add `MotionState getCurrentState() const` accessor (returns last computed state)
  - Change `process(int numSamples)` to compute actual positions:
    - **Orbit:** `az = (width/2) * cos(phase_acc + phase_offset)`, elevation via `sin()`, distance modulated by depth
    - **Pendulum:** `az = (width/2) * sin(phase_acc)`, no elevation, constant distance
    - **Linear:** `az = width * (phase_acc / 2pi) - width/2`, wraps via modulo
    - **Drift:** `az = perlin.fbm(noiseTime, 4) * (width/2)`, decorrelated el/dist noise
  - Tempo sync: read hostBpm, convert division index to Hz via `tempoMultipliers[]` lookup
  - Effective speed = tempoSync > 0 ? bpm-derived Hz : speed parameter
  - Phase accumulator wraps at 2pi (Orbit/Pendulum/Linear) or increments continuously (Drift)
  - Store last computed `MotionState` for `getCurrentState()`

### Task 3: Implement DistanceModel with attenuation + air absorption LPF
- **Files:** `Source/DSP/DistanceModel.h` (modify), `Source/DSP/DistanceModel.cpp` (modify)
- **Depends on:** none
- **Details:**
  - Add `juce::dsp::IIR::Filter<float> lpf` member
  - Add `float currentGain = 1.0f` member
  - `prepare(double sampleRate)`: initialize LPF via `juce::dsp::ProcessSpec`
  - `updateDistance(float distance, float airAbsorption, int attenuationCurve)`:
    - Compute gain: Linear = `clamp(1 - (d-0.1)/29.9, 0, 1)`, Inverse = `1/max(d, 0.1)`, InvSquare = `1/max(d*d, 0.01)`
    - Compute LPF cutoff: `20000 / (1 + airAbsorption*0.01 * (d/10))`, clamp [100, 20000]
    - Update LPF coefficients via `IIR::Coefficients::makeLowPass()`
  - `processSample(float sample)`: apply `currentGain * lpf.processSample(sample)`
  - Add `reset()` method to clear LPF state

### Task 4: Implement 2-speaker equal-power panning in VBAPRenderer
- **Files:** `Source/DSP/VBAPRenderer.h` (modify), `Source/DSP/VBAPRenderer.cpp` (modify)
- **Depends on:** none
- **Details:**
  - Add `bool useSAF = false` member
  - Add `std::vector<float> speakerAzimuths` member (cached from layout)
  - `prepare(const SpeakerLayout& layout)`:
    - If numSpeakers <= 3: `useSAF = false`, cache speaker azimuths
    - If numSpeakers >= 4: `useSAF = true` (SAF path, stub for now — implemented in Phase 2.2)
  - `computeGains(float azimuth, float elevation, float* gains, int numSpeakers)`:
    - For 2-speaker (stereo): equal-power panning from azimuth
      - `panAngle = clamp((az + 90) / 180, 0, 1)` where speakers are at -30/+30
      - `gainL = cos(panAngle * pi/2)`, `gainR = sin(panAngle * pi/2)`
    - For 3-speaker: find nearest pair, pair-wise panning
    - For 4+: fall through to SAF (Phase 2.2)

### Task 5: Wire DSP into processBlock
- **Files:** `Source/PluginProcessor.h` (modify), `Source/PluginProcessor.cpp` (modify)
- **Depends on:** Tasks 2, 3, 4
- **Details:**
  - Add `DistanceModel distanceModelR` second instance (for L+R Split mode, R channel)
  - Add `juce::AudioBuffer<float> dryBuffer` for dry/wet mix
  - In `prepareToPlay()`: call `distanceModelR.prepare()`, allocate dryBuffer
  - In `processBlock()`:
    1. Copy input to dryBuffer for dry/wet mix
    2. Read all parameters → set on MotionEngine, DistanceModel
    3. Read host BPM via `getPlayHead()->getPosition()->getBpm()`
    4. Get start MotionState, call `motionEngine.process(numSamples)`, get end MotionState
    5. Per-sample loop with linear interpolation of az/el/dist:
       - `t = sample / numSamples`
       - Interpolated az, el, dist
       - Shortest-arc interpolation for azimuth wrapping at +-180
       - Source mode: Mono → sum L+R, apply distance model, compute VBAP gains, write to output channels
       - Source mode: L+R Split → process L at azimuth, R at azimuth+offset, each through own distance model
       - Accumulate source contributions to output channels via VBAP gains
    6. Apply dry/wet mix: `output = mix * wet + (1-mix) * dry`
  - Add `shortestArc(float from, float to)` utility for azimuth wrapping

### Task 6: Add new source files to CMakeLists.txt
- **Files:** `CMakeLists.txt` (modify)
- **Depends on:** Task 1
- **Details:**
  - Add `Source/DSP/PerlinNoise.h` to target_sources (header-only, but good for IDE visibility)
  - No other CMake changes needed for Phase 2.1

---

## Phase 2.2: Full 2D VBAP + Multi-Channel Output

### Task 7: Implement SAF VBAP gain table generation
- **Files:** `Source/DSP/VBAPRenderer.h` (modify), `Source/DSP/VBAPRenderer.cpp` (modify)
- **Depends on:** Task 4
- **Details:**
  - Add `extern "C" { #include "saf.h" }` include
  - Add members: `std::vector<float> gainTable`, `int gainTableSize`, `int aziRes = 1`
  - In `prepare()` for 4+ speakers (2D, no elevation):
    - Build `float ls_dirs_deg[]` flat array from speaker azimuths (elevation=0 for all)
    - Exclude LFE speakers from VBAP calculation (check label == "LFE")
    - Call `generateVBAPgainTable3D(ls_dirs, L, 1, 1, 1, 1, 0.0f, &gtable, &N, &nTri)`
    - Copy `gtable` to `std::vector<float>`, `free(gtable)`
    - Store mapping from VBAP speaker index to actual output channel index (to handle LFE exclusion)
  - In `computeGains()` for 4+ speakers:
    - 2D lookup: `idx = (int)(fmod(az + 180, 360) / aziRes + 0.5)`
    - Copy gains from `gainTable[idx * numVBAPSpeakers]` to output `gains[]`
    - Set LFE channel gain to 0

### Task 8: Implement VBAPDataExchange for thread-safe gain table swap
- **Files:** `Source/DSP/VBAPDataExchange.h` (new), `Source/DSP/VBAPDataExchange.cpp` (new)
- **Depends on:** Task 7
- **Details:**
  - `VBAPData` struct: gain table vector, speaker count, azi resolution, speaker-to-channel mapping
  - `VBAPDataExchange` class (models JUCE RenderSequenceExchange):
    - `juce::SpinLock lock`
    - `std::unique_ptr<VBAPData> pending`, `std::unique_ptr<VBAPData> active`, `std::unique_ptr<VBAPData> old`
    - `setNewData(unique_ptr<VBAPData>)` — writer (background/message thread), takes SpinLock
    - `updateAudioThreadData()` — reader (audio thread), ScopedTryLock — swaps pending→active if lock acquired
    - `getActiveData()` — returns raw pointer to active VBAPData
  - `VBAPComputeThread` class (juce::Thread):
    - `requestRecomputation(SpeakerLayout layout)` — stores layout, signals thread via `notify()`
    - `run()` — waits for signal, generates gain table, calls `exchange.setNewData()`
    - Clean shutdown via `signalThreadShouldExit()` + `notify()`

### Task 9: Integrate VBAPDataExchange into processor
- **Files:** `Source/PluginProcessor.h` (modify), `Source/PluginProcessor.cpp` (modify)
- **Depends on:** Task 8
- **Details:**
  - Add `VBAPDataExchange vbapExchange` and `VBAPComputeThread vbapThread` members
  - Add `int lastSpeakerLayoutIndex = -1` to detect layout changes
  - In `prepareToPlay()`: start `vbapThread`, trigger initial computation
  - In `processBlock()`:
    - Call `vbapExchange.updateAudioThreadData()` at top
    - Check speaker layout param; if changed from `lastSpeakerLayoutIndex`, trigger async recomputation via `AsyncUpdater`
    - Use `vbapExchange.getActiveData()` for gain lookups
  - In destructor: stop `vbapThread`
  - Update VBAPRenderer to accept `VBAPData*` for gain lookup instead of internal table

### Task 10: Per-speaker gain smoothing
- **Files:** `Source/PluginProcessor.cpp` (modify), `Source/PluginProcessor.h` (modify)
- **Depends on:** Task 9
- **Details:**
  - Add `std::array<float, 24> previousGains{}` and `std::array<float, 24> currentGains{}` members (max 24 speakers)
  - At start of processBlock: copy currentGains → previousGains, compute new currentGains from VBAP
  - Per-sample: `smoothedGain[s] = previousGains[s] + t * (currentGains[s] - previousGains[s])` where `t = sample / numSamples`
  - Apply smoothed gains to speaker feed accumulation
  - For L+R Split: maintain separate gain arrays for L and R sources

### Task 11: Center diverge parameter
- **Files:** `Source/DSP/VBAPRenderer.h` (modify), `Source/DSP/VBAPRenderer.cpp` (modify)
- **Depends on:** Task 7
- **Details:**
  - Add `void setCenterDiverge(float amount)` — stores 0-1 value
  - After VBAP gain lookup, spread gain to adjacent speakers:
    - For each speaker with gain > 0, add fraction to neighbor speakers
    - `neighborGain = diverge * angularProximity * originalGain`
    - Re-normalize all gains to preserve energy: `gains /= sqrt(sum(gains^2))`

### Task 12: Add new source files to CMakeLists.txt (Phase 2.2)
- **Files:** `CMakeLists.txt` (modify)
- **Depends on:** Task 8
- **Details:**
  - Add `Source/DSP/VBAPDataExchange.cpp` to target_sources
  - Add `Source/DSP/VBAPDataExchange.h` for IDE visibility

---

## Phase 2.3: 3D VBAP + Custom Layouts + Auto-Downmix

### Task 13: Enable 3D VBAP with elevation
- **Files:** `Source/DSP/VBAPRenderer.cpp` (modify)
- **Depends on:** Task 7
- **Details:**
  - In `prepare()`: detect if layout.is3D (any speaker has elevation != 0)
  - For 3D layouts: use SAF `generateVBAPgainTable3D()` with elevation resolution = 1 degree
  - In `computeGains()`: use 3D gain table lookup
    - `N_azi = 361`, `aziIndex = (int)(fmod(az+180, 360) + 0.5)`
    - `elevIndex = (int)(el + 90 + 0.5)` (clamp [0, 180])
    - `idx = elevIndex * N_azi + aziIndex`
    - Read gains from `gainTable[idx * numVBAPSpeakers]`

### Task 14: Implement auto-downmix
- **Files:** `Source/DSP/DownmixEngine.h` (new), `Source/DSP/DownmixEngine.cpp` (new)
- **Depends on:** none
- **Details:**
  - `DownmixEngine` class:
    - `prepare(const SpeakerLayout& layout, int availableChannels)` — generate downmix matrix
    - `isActive()` — returns true if downmix needed
    - `process(juce::AudioBuffer<float>& buffer, int layoutChannels)` — fold down in-place
  - Stereo downmix: for each speaker feed, pan to L/R based on speaker azimuth
    - `panAngle = (az + 90) / 180`, `gainL = cos(pan * pi/2)`, `gainR = sin(pan * pi/2)`
    - Energy-preserving: normalize by `sqrt(sum of squared gains)`
  - N-channel downmix (e.g., 7.1.4 → 5.1): fold height to nearest ear-level speaker
  - Store `std::vector<std::vector<float>> downmixMatrix` (target x source)

### Task 15: Integrate auto-downmix into processor
- **Files:** `Source/PluginProcessor.h` (modify), `Source/PluginProcessor.cpp` (modify)
- **Depends on:** Task 14
- **Details:**
  - Add `DownmixEngine downmixEngine` member
  - Add `bool downmixActive = false` and `juce::String downmixWarning` members
  - In `prepareToPlay()`:
    - Compare `getTotalNumOutputChannels()` with current speaker layout channel count
    - If mismatch: `downmixEngine.prepare(layout, outputChannels)`, set `downmixActive = true`
    - Build warning string: "Layout: 7.1.4 → DAW: Stereo"
  - In `processBlock()`:
    - Process all VBAP speaker feeds into internal buffer (up to layout channel count)
    - If downmixActive: call `downmixEngine.process()` to fold to output channel count
  - Add `juce::AudioBuffer<float> spatialBuffer` for intermediate multi-channel processing
    - Allocated to max(layoutChannels, outputChannels) in prepareToPlay

### Task 16: LFE channel handling
- **Files:** `Source/Data/SpeakerLayout.h` (modify), `Source/DSP/VBAPRenderer.cpp` (modify)
- **Depends on:** Task 7
- **Details:**
  - Add `bool isLFE = false` flag to `Speaker` struct
  - Mark LFE speakers in 5.1, 7.1, 5.1.4, 7.1.4 presets (`SpeakerPresets.h`)
  - VBAPRenderer: exclude LFE-flagged speakers from VBAP gain table generation
  - Maintain channel index mapping: VBAP speaker index → output channel index
  - LFE output channel gets zero gain from VBAP (future: optional bass-filtered send)

### Task 17: Add new source files to CMakeLists.txt (Phase 2.3)
- **Files:** `CMakeLists.txt` (modify)
- **Depends on:** Task 14
- **Details:**
  - Add `Source/DSP/DownmixEngine.cpp` and `Source/DSP/DownmixEngine.h`

### Task 18: Build verification and smoke test
- **Files:** none (verification only)
- **Depends on:** all previous tasks
- **Details:**
  - Build all 3 targets: `ninja OuariconOrbit_VST3 OuariconOrbit_AU OuariconOrbit_Standalone`
  - Zero warnings from O-Orbit source files
  - Standalone launches without crash
  - Audio input produces spatially moving output in stereo
  - Speaker layout parameter switches without crash
  - Verify: motion is audible (Orbit path, medium speed, full width)

---

## Success Criteria

- [ ] All 4 motion paths produce distinct, audible spatial movement (Orbit, Pendulum, Linear, Drift)
- [ ] Tempo sync locks speed to host BPM correctly
- [ ] Distance parameter affects level (attenuation curve) and brightness (air absorption LPF)
- [ ] Stereo panning works for 2-speaker layout (equal-power from azimuth)
- [ ] SAF VBAP produces correct gains for 4+ speaker layouts (Quad, 5.1, 7.1)
- [ ] 3D VBAP works for layouts with elevation (5.1.4, 7.1.4)
- [ ] Speaker layout switching at runtime triggers background recomputation without audio dropout
- [ ] Per-sample gain smoothing — no clicks or zipper noise during motion
- [ ] L+R Split mode produces two independently orbiting sources with phase offset
- [ ] Auto-downmix folds multi-channel to stereo when DAW provides 2 channels
- [ ] LFE channels excluded from VBAP (no phantom image on subwoofer)
- [ ] Center diverge spreads signal to adjacent speakers
- [ ] Mix parameter blends dry (input) with wet (spatialized)
- [ ] All 3 targets build (VST3, AU, Standalone) with zero warnings
- [ ] Standalone runs without crash

---

## File Summary

### New Files
| File | Phase | Description |
|------|-------|-------------|
| `Source/DSP/PerlinNoise.h` | 2.1 | 1D Perlin noise + fBm, header-only |
| `Source/DSP/VBAPDataExchange.h` | 2.2 | Thread-safe VBAP data swap (SpinLock pattern) |
| `Source/DSP/VBAPDataExchange.cpp` | 2.2 | VBAPDataExchange + VBAPComputeThread implementation |
| `Source/DSP/DownmixEngine.h` | 2.3 | Auto-downmix engine |
| `Source/DSP/DownmixEngine.cpp` | 2.3 | Downmix matrix generation + processing |

### Modified Files
| File | Phases | Changes |
|------|--------|---------|
| `Source/DSP/MotionEngine.h` | 2.1 | Add PerlinNoise member, getCurrentState(), noiseTime |
| `Source/DSP/MotionEngine.cpp` | 2.1 | Implement 4 path algorithms, tempo sync |
| `Source/DSP/DistanceModel.h` | 2.1 | Add IIR::Filter, currentGain, reset() |
| `Source/DSP/DistanceModel.cpp` | 2.1 | Implement attenuation curves + LPF |
| `Source/DSP/VBAPRenderer.h` | 2.1, 2.2, 2.3 | Add SAF integration, gain table, LFE exclusion |
| `Source/DSP/VBAPRenderer.cpp` | 2.1, 2.2, 2.3 | Implement stereo panning, SAF VBAP, 3D lookup |
| `Source/PluginProcessor.h` | 2.1, 2.2, 2.3 | Add buffers, VBAPDataExchange, DownmixEngine |
| `Source/PluginProcessor.cpp` | 2.1, 2.2, 2.3 | Wire full DSP chain in processBlock |
| `Source/Data/SpeakerLayout.h` | 2.3 | Add isLFE flag to Speaker struct |
| `Source/Data/SpeakerPresets.h` | 2.3 | Mark LFE speakers in presets |
| `CMakeLists.txt` | 2.1, 2.2, 2.3 | Add new source files |

---

## Dependencies Graph

```
Task 1 (PerlinNoise) ──→ Task 2 (MotionEngine)
                                              ╲
Task 3 (DistanceModel) ─────────────────────→ Task 5 (processBlock wiring)
                                              ╱
Task 4 (Stereo panning) ──→ Task 5
                           ╲
Task 6 (CMake 2.1) ────────╴

Task 4 ──→ Task 7 (SAF VBAP) ──→ Task 8 (VBAPDataExchange) ──→ Task 9 (Integrate exchange)
                                                                        ╲
Task 7 ──→ Task 11 (Center diverge)                              Task 10 (Gain smoothing)
Task 7 ──→ Task 13 (3D VBAP)
                                                                 Task 12 (CMake 2.2)

Task 14 (DownmixEngine) ──→ Task 15 (Integrate downmix)
Task 7 ──→ Task 16 (LFE handling)
Task 14 ──→ Task 17 (CMake 2.3)

All tasks ──→ Task 18 (Build verification)
```
