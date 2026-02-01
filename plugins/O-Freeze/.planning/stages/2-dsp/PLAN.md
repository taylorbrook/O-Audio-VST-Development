# Stage 2: DSP - Execution Plan

## Goal

Implement the granular freeze engine for O-Freeze using a phased approach: buffer loop first (validate mechanics), then upgrade to 8-grain granular synthesis with Hann windowing, threshold gate, and dry/wet mixing.

## Implementation Phases

### Phase A: Simple Buffer Loop (Tasks 1-5)
Validates: Buffer mechanics, freeze trigger, crossfade, mixing

### Phase B: Threshold Gate (Tasks 6-7)
Validates: Automatic freeze triggering with hysteresis

### Phase C: Granular Engine (Tasks 8-11)
Validates: Smooth freeze texture, no artifacts

---

## Tasks

### Phase A: Simple Buffer Loop

#### 1. [ ] Add DSP member variables to PluginProcessor.h
- **Files:** `Source/PluginProcessor.h`
- **Changes:**
  - Add `juce::AudioBuffer<float> freezeBuffer` (2-second circular buffer)
  - Add `int writePosition = 0` (circular write head)
  - Add `int readPosition = 0` (playback head)
  - Add `bool bufferFrozen = false` (freeze state flag)
  - Add `double currentSampleRate = 44100.0`
  - Add `juce::LinearSmoothedValue<float> freezeGain` (crossfade envelope)
  - Add `juce::dsp::DryWetMixer<float> dryWetMixer`
  - Add `#include <juce_dsp/juce_dsp.h>` to header
- **Depends on:** None

#### 2. [ ] Initialize buffers in prepareToPlay()
- **Files:** `Source/PluginProcessor.cpp`
- **Changes:**
  - Pre-allocate freezeBuffer for 2 seconds at max sample rate (192kHz = 384,000 samples stereo)
  - Store currentSampleRate
  - Reset writePosition and readPosition to 0
  - Initialize freezeGain smoother (50ms fade-in, 100ms fade-out ramp time)
  - Prepare dryWetMixer with ProcessSpec
  - Clear freezeBuffer on prepare
- **Depends on:** Task 1

#### 3. [ ] Implement circular buffer write in processBlock()
- **Files:** `Source/PluginProcessor.cpp`
- **Changes:**
  - Read FREEZE parameter value
  - If not frozen: write input to freezeBuffer at writePosition, advance with wraparound
  - If frozen: lock writePosition (stop writing)
  - Use modulo arithmetic for circular indexing
- **Depends on:** Task 2

#### 4. [ ] Implement simple buffer loop playback
- **Files:** `Source/PluginProcessor.cpp`
- **Changes:**
  - When frozen: read from freezeBuffer at readPosition, advance with wraparound
  - Loop through entire frozen region (2 seconds)
  - Apply freezeGain crossfade envelope to frozen output
  - Blend: `output = dry * (1 - freezeGain) + frozen * freezeGain`
- **Depends on:** Task 3

#### 5. [ ] Integrate DryWetMixer
- **Files:** `Source/PluginProcessor.cpp`
- **Changes:**
  - Push dry input to dryWetMixer before freeze processing
  - Read MIX parameter, set mixer ratio (0-1 range from 0-100%)
  - Apply mixer to output after crossfade blending
  - Process through dryWetMixer.mixWetSamples()
- **Depends on:** Task 4

---

### Phase B: Threshold Gate

#### 6. [ ] Add threshold gate state machine
- **Files:** `Source/PluginProcessor.h`, `Source/PluginProcessor.cpp`
- **Changes (header):**
  - Add `enum class GateState { Idle, Frozen }` (simplified state machine)
  - Add `GateState gateState = GateState::Idle`
  - Add `float rmsLevel = 0.0f` (current RMS)
  - Add `std::vector<float> rmsBuffer` (20ms rolling window)
  - Add `int rmsWriteIndex = 0`
  - Add `int rmsSamplesPerWindow` (calculated from sample rate)
- **Changes (cpp - prepareToPlay):**
  - Calculate rmsSamplesPerWindow = sampleRate * 0.020 (20ms)
  - Resize rmsBuffer to rmsSamplesPerWindow
  - Clear rmsBuffer
- **Depends on:** Task 5

#### 7. [ ] Implement RMS detection and threshold triggering
- **Files:** `Source/PluginProcessor.cpp`
- **Changes:**
  - Read MODE parameter (0 = Manual, 1 = Threshold)
  - Read THRESHOLD parameter (dB)
  - If MODE == Manual: use FREEZE button directly
  - If MODE == Threshold:
    - Calculate RMS: write sample² to rmsBuffer, compute rolling average
    - Convert to dB: `20 * log10(sqrt(rmsSum / rmsSamplesPerWindow) + 1e-6f)`
    - Engage freeze when RMS < threshold
    - Release freeze when RMS > threshold + 3dB (hysteresis)
  - Update gateState based on detection
- **Depends on:** Task 6

---

### Phase C: Granular Engine

#### 8. [ ] Add grain structure and granular state
- **Files:** `Source/PluginProcessor.h`
- **Changes:**
  - Add grain struct: `struct Grain { int startSample; int position; bool active; }`
  - Add `std::array<Grain, 8> grains` (8 grains for 87.5% overlap)
  - Add `std::vector<float> hannWindow` (pre-computed window table)
  - Add `int grainSize` (50ms in samples, scales with sample rate)
  - Add `int grainTriggerInterval` (grainSize / 8 for 8 grains)
  - Add `int grainTriggerCounter = 0`
  - Add `int nextGrainIndex = 0` (round-robin grain allocation)
  - Add `juce::Random random` (for drift randomization)
- **Depends on:** Task 7

#### 9. [ ] Pre-compute Hann window in prepareToPlay()
- **Files:** `Source/PluginProcessor.cpp`
- **Changes:**
  - Calculate grainSize = sampleRate * 0.200 (200ms)
  - Calculate grainTriggerInterval = grainSize / 8
  - Resize hannWindow to grainSize
  - Generate Hann: `window[i] = 0.5f * (1.0f - std::cos(2.0f * PI * i / (grainSize - 1)))`
  - Reset all grains to inactive
  - Reset grainTriggerCounter and nextGrainIndex
- **Depends on:** Task 8

#### 10. [ ] Implement grain triggering and scheduling
- **Files:** `Source/PluginProcessor.cpp`
- **Changes:**
  - Replace simple loop playback with granular engine
  - When frozen:
    - Increment grainTriggerCounter per sample
    - When counter >= grainTriggerInterval: trigger new grain
    - New grain: set startSample = 0, position = random offset (based on DRIFT), active = true
    - Use nextGrainIndex for round-robin allocation, wrap at 8
  - Read DRIFT parameter (0-100%), convert to offset range (0 to bufferLength)
- **Depends on:** Task 9

#### 11. [ ] Implement overlap-add grain synthesis
- **Files:** `Source/PluginProcessor.cpp`
- **Changes:**
  - For each sample in processBlock:
    - Sum all active grains:
      - Read from freezeBuffer at grain.position (with wraparound)
      - Multiply by hannWindow[grain.startSample]
      - Advance grain.startSample and grain.position
      - Deactivate grain when startSample >= grainSize
    - Normalize output by active grain count (prevent clipping)
  - Output granular sum through crossfade and dryWetMixer
- **Depends on:** Task 10

---

## Files Modified

| File | Changes |
|------|---------|
| `Source/PluginProcessor.h` | DSP members, grain struct, state machine enum |
| `Source/PluginProcessor.cpp` | prepareToPlay, processBlock (full DSP implementation) |

## Success Criteria

### Phase A (Buffer Loop)
- [ ] FREEZE button activates buffer lock (stops writing)
- [ ] Frozen audio loops smoothly (no gaps)
- [ ] Crossfade prevents clicks on engage/disengage (50ms in, 100ms out)
- [ ] MIX parameter blends dry/wet correctly

### Phase B (Threshold Gate)
- [ ] MODE=Threshold triggers freeze when input level drops
- [ ] THRESHOLD parameter adjusts sensitivity (-60dB to 0dB)
- [ ] Hysteresis prevents flutter (3dB gap)
- [ ] MODE=Manual ignores threshold, uses button

### Phase C (Granular Engine)
- [ ] 8 simultaneous grains produce smooth texture
- [ ] No audible clicks or pops at grain boundaries
- [ ] DRIFT=0% produces static freeze (grains read same position)
- [ ] DRIFT=100% produces evolving texture (maximum position variation)
- [ ] CPU usage acceptable (< 25% single core)

### Overall
- [ ] Plugin builds without warnings
- [ ] Plugin passes pluginval validation
- [ ] Tested in DAW with various input sources

---

## Estimated Complexity

| Phase | Tasks | Complexity |
|-------|-------|------------|
| Phase A | 1-5 | Low-Medium |
| Phase B | 6-7 | Medium |
| Phase C | 8-11 | Medium-High |

**Total:** 11 tasks across 3 phases

---

## Next Step

Run `/plugin-execute O-Freeze 2-dsp` to begin implementation.
