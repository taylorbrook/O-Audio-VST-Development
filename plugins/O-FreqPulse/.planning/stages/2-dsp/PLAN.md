# Stage 2: DSP - Execution Plan

**Plugin:** O-FreqPulse
**Stage:** 2 (DSP Implementation)
**Created:** 2026-02-03
**Status:** READY FOR EXECUTION

---

## Goal

Implement the complete spectral gating DSP engine: FFT-based STFT processing with 4-band frequency isolation, tempo-synced step sequencing, Euclidean rhythm generation, and smooth gain transitions.

---

## Tasks

### Phase 1: STFT Infrastructure

#### Task 1: Add DSP member variables to PluginProcessor.h
- **Files:** `Source/PluginProcessor.h`
- **Depends on:** None
- **Description:**
  - Add FFT object: `juce::dsp::FFT fft { 11 };` (2^11 = 2048)
  - Add windowing: `juce::dsp::WindowingFunction<float> window`
  - Add STFT buffers per channel:
    - `std::array<std::vector<float>, 2> inputFifo`
    - `std::array<std::vector<float>, 2> outputFifo`
    - `std::array<std::vector<float>, 2> fftData`
  - Add buffer indices: `int inputWritePos`, `int outputReadPos`, `int hopCounter`
  - Add DryWetMixer: `juce::dsp::DryWetMixer<float> dryWetMixer`
  - Add gain smoothers: `std::array<juce::SmoothedValue<float>, 4> bandGainSmooth`
  - Add bin mapping: `std::array<int, 1025> bandForBin`
  - Add Euclidean pattern cache: `std::array<std::array<bool, 32>, 4> euclideanPatterns`
  - Add step tracking: `int currentStep`, `double lastPpqPosition`
  - Add constants: `fftOrder`, `fftSize`, `hopSize`, `numBins`, `windowCorrection`

#### Task 2: Initialize STFT in prepareToPlay()
- **Files:** `Source/PluginProcessor.cpp`
- **Depends on:** Task 1
- **Description:**
  - Resize all STFT buffers to correct sizes (fftSize, fftSize*2)
  - Clear/zero all buffers
  - Initialize Hann window (fftSize + 1 samples, periodic)
  - Reset hop counter to 0
  - Set latency: `setLatencySamples(fftSize)` → 2048 samples
  - Initialize DryWetMixer with ProcessSpec
  - Configure gain smoothers with sample rate and smoothing time
  - Calculate initial bin-to-band mapping

### Phase 2: Band Processing

#### Task 3: Implement bin-to-band mapping function
- **Files:** `Source/PluginProcessor.cpp`
- **Depends on:** Task 1
- **Description:**
  - Create `void recalculateBinMapping()` method
  - Read band low/high frequencies from parameters
  - For each bin (0-1024), calculate frequency: `bin * sampleRate / fftSize`
  - Assign bin to band index (0-3) or -1 for passthrough
  - Store in `bandForBin` array
  - Handle gaps between bands (passthrough at 100%)
  - Call in prepareToPlay and when band frequencies change

#### Task 4: Implement Euclidean pattern generator
- **Files:** `Source/PluginProcessor.cpp`
- **Depends on:** Task 1
- **Description:**
  - Create `std::array<bool, 32> generateEuclidean(int steps, int pulses, int offset)` method
  - Implement Bresenham bucket-fill algorithm
  - Apply rotation offset via std::rotate
  - Clamp pulses to steps
  - Create `void updateEuclideanPatterns()` to regenerate all 4 band patterns
  - Call when euclidean parameters change

### Phase 3: Step Sequencer

#### Task 5: Implement tempo sync step calculation
- **Files:** `Source/PluginProcessor.cpp`
- **Depends on:** Task 1
- **Description:**
  - Create PPQ values array for rate options (1/1 through 1/8D)
  - Create `int calculateCurrentStep(double ppq, int numSteps, int rateIndex, float swing)` method
  - Handle swing: delay odd steps by swing amount
  - Return step index (0 to numSteps-1)
  - Handle transport stopped: freeze on current step

#### Task 6: Implement step/gain lookup for current position
- **Files:** `Source/PluginProcessor.cpp`
- **Depends on:** Tasks 4, 5
- **Description:**
  - Create `float getTargetGainForBand(int bandIndex, int currentStep)` method
  - Check if band is enabled
  - Check if Euclidean mode active → use pattern cache, else use step parameter
  - Return 1.0f if step is ON, else (1.0f - depth) if step is OFF
  - Handle band disable → return 1.0f (passthrough)

### Phase 4: STFT Processing Core

#### Task 7: Implement processFrame() for single FFT frame
- **Files:** `Source/PluginProcessor.cpp`
- **Depends on:** Tasks 2, 3, 6
- **Description:**
  - Create `void processFrame(int channel)` method
  - Copy fftSize samples from inputFifo to fftData buffer
  - Apply analysis window (Hann)
  - Forward FFT: `fft.performRealOnlyForwardTransform()`
  - Apply band gains to bins:
    - For each bin, lookup band from `bandForBin`
    - Get smoothed gain from `bandGainSmooth[band].getNextValue()`
    - Scale real and imaginary parts by gain (phase preservation)
  - Inverse FFT: `fft.performRealOnlyInverseTransform()`
  - Apply synthesis window (Hann)
  - Apply COLA correction (2/3 for 4× overlap)
  - Overlap-add to outputFifo

#### Task 8: Implement full processBlock() with STFT
- **Files:** `Source/PluginProcessor.cpp`
- **Depends on:** Task 7
- **Description:**
  - Push dry samples to DryWetMixer first
  - Get playhead position, calculate current step
  - Update band target gains from step sequencer
  - Process each sample:
    - Push to inputFifo
    - Increment hopCounter
    - When hopCounter >= hopSize:
      - Call processFrame() for each channel
      - Reset hopCounter
      - Advance output read position
    - Read output from outputFifo
  - Mix wet samples with DryWetMixer
  - Update lastPpqPosition for next block

### Phase 5: Integration & Polish

#### Task 9: Implement releaseResources() cleanup
- **Files:** `Source/PluginProcessor.cpp`
- **Depends on:** Task 2
- **Description:**
  - Reset DryWetMixer
  - Clear all STFT buffers
  - Reset step tracking state

#### Task 10: Add parameter change handling
- **Files:** `Source/PluginProcessor.cpp`
- **Depends on:** Tasks 3, 4
- **Description:**
  - Track last-known band frequency values
  - In processBlock, detect frequency changes → call recalculateBinMapping()
  - Track last-known Euclidean parameters
  - Detect Euclidean param changes → call updateEuclideanPatterns()
  - Update smoothing time when smoothing parameter changes

#### Task 11: Build, install, and verify in DAW
- **Files:** None (build system)
- **Depends on:** Tasks 1-10
- **Description:**
  - Build VST3 and AU: `ninja O-FreqPulse_VST3 O-FreqPulse_AU`
  - Clear AU cache and install to system folders
  - Verify auval passes
  - Test in DAW with audio material
  - Verify latency reported correctly (2048 samples)
  - Verify step sequencer syncs to tempo

---

## Success Criteria

- [ ] FFT processing produces audible spectral gating
- [ ] 4 bands independently controllable
- [ ] Tempo-synced step sequencing works (verified with playhead)
- [ ] Euclidean patterns generate correctly (test: 8 steps, 3 pulses = X..X..X.)
- [ ] No clicks or pops during gate transitions (smoothing working)
- [ ] Latency reported to DAW (2048 samples, ~46ms at 44.1kHz)
- [ ] Bypass/null test: mix=0% passes audio unchanged
- [ ] CPU usage <5% on Apple Silicon at 44.1kHz stereo

---

## File Modifications Summary

| File | Action | Changes |
|------|--------|---------|
| `Source/PluginProcessor.h` | Modify | Add ~50 lines (STFT members, constants, helper method declarations) |
| `Source/PluginProcessor.cpp` | Modify | Add ~250 lines (STFT implementation, step sequencer, Euclidean generator) |

---

## Testing Strategy

| Test | Method | Expected Result |
|------|--------|-----------------|
| FFT reconstruction | Set all steps ON, compare to dry | Minimal difference (COLA working) |
| Band isolation | Solo each band | Only that frequency range affected |
| Step timing | Visual playhead alignment | Steps change on beat grid |
| Euclidean (8,3) | Enable Euclidean, 8 steps, 3 pulses | Pattern: X..X..X. audible |
| Smoothing | Fast step changes at 0ms vs 10ms | 0ms has clicks, 10ms is smooth |
| Latency | Check DAW PDC | 2048 samples reported |

---

## Risk Mitigations

### FFT Artifacts
- Use Hann window with COLA normalization (2/3 factor)
- 75% overlap (4× factor)
- Phase preservation (scale real+imag together)
- **Fallback:** Increase to 8× overlap if artifacts persist

### CPU Performance
- Pre-computed window and bin mapping
- Skip processing for disabled bands
- `juce::FloatVectorOperations` for bulk operations
- **Profile at:** 96kHz stereo (worst case)

### Timing Issues
- Per-band SmoothedValue for glitch-free transitions
- Handle transport stopped (freeze current step)
- Track PPQ across blocks for continuity

---

**Next Command:** `/plugin-execute O-FreqPulse 2-dsp`
