# O-FreqPulse - Architecture Specification

---
**Contract Status:** RESEARCH COMPLETE
**Plugin Type:** Audio Effect (Spectral Sequencer)
**Generated:** 2026-02-03
**Research Depth:** DEEP (Tier 6 - Real-time spectral analysis)
**Complexity Score:** 5.0 (see ROADMAP.md)
---

## Executive Summary

O-FreqPulse is a rhythmic spectral gate that combines FFT-based frequency-domain processing with multi-band step sequencing and Euclidean rhythm generation. The plugin splits audio into 4 configurable frequency bands using STFT processing, applies independent rhythmic gating patterns per band, and reconstructs the output via overlap-add synthesis.

**Core Innovation:** Real-time spectral processing meets polyrhythmic sequencing through a visual 2D frequency × time grid interface.

**Complexity Assessment:**
- Tier 6 plugin (real-time FFT processing + visualization)
- Deep research protocol executed (30 minutes)
- High algorithmic complexity but proven JUCE DSP foundation

---

## 1. Core Components

### 1.1 FFT Spectral Processor

**Purpose:** Convert time-domain audio to frequency domain for per-band processing.

**JUCE Classes:**
- `juce::dsp::FFT` - Forward/inverse FFT transforms
- `juce::dsp::WindowingFunction<float>` - Hann window for STFT
- Module: `juce_dsp`

**Algorithm:**
1. Input signal → overlapping frames (75% overlap)
2. Apply Hann window to each frame
3. Forward FFT (time → frequency domain)
4. Process frequency bins (apply band gains)
5. Inverse FFT (frequency → time domain)
6. Apply synthesis window
7. Overlap-add reconstruction

**Parameters:**
- FFT Size: 2048 samples (fixed for v1.0, ~46ms latency at 44.1kHz)
- Hop Size: 512 samples (FFT size / 4, 75% overlap)
- Window: Hann (satisfies COLA constraint for perfect reconstruction)

**Implementation Notes:**
- Use `juce::dsp::FFT::performRealOnlyForwardTransform()` for forward
- Use `juce::dsp::FFT::performRealOnlyInverseTransform()` for inverse
- Real-only FFT returns N/2 + 1 complex bins for N-point FFT
- Minimum latency = FFT size samples (report to host via `setLatencySamples()`)

**References:**
- [Overlap-Add STFT Processing](https://www.dsprelated.com/freebooks/sasp/Overlap_Add_OLA_STFT_Processing.html)
- [FFT Processing in JUCE](https://audiodev.blog/fft-processing/)

---

### 1.2 Band Frequency Splitter

**Purpose:** Map FFT bins to 4 frequency bands for independent processing.

**Algorithm:**
```
For each FFT bin:
  1. Convert bin index to frequency: freq = (bin * sampleRate) / fftSize
  2. Determine which band(s) the frequency belongs to
  3. Apply band gain to bin magnitude
  4. Preserve phase information
```

**Default Band Configuration:**
| Band | Name | Low Freq | High Freq | Bin Range (44.1kHz, 2048 FFT) |
|------|------|----------|-----------|-------------------------------|
| 1    | Sub  | 20 Hz    | 120 Hz    | ~1-5 bins                     |
| 2    | Low  | 120 Hz   | 500 Hz    | ~5-23 bins                    |
| 3    | Mid  | 500 Hz   | 4000 Hz   | ~23-186 bins                  |
| 4    | High | 4000 Hz  | 20000 Hz  | ~186-929 bins                 |

**Crossover Type (v1.0):**
- Hard cutoff (bins belong to exactly one band)
- No crossfade/blending between bands
- Simpler implementation, acceptable for rhythmic effects

**Future Enhancement (v1.1+):**
- Smooth crossfade at band boundaries (overlapping gain curves)
- Reduces spectral artifacts at crossover points

**Implementation:**
- No specific JUCE class needed (custom bin-to-band mapping)
- Store band gain per bin in lookup table for efficiency
- Recalculate bin mapping when band frequencies change

---

### 1.3 Step Sequencer Engine

**Purpose:** Generate rhythmic on/off patterns synchronized to host tempo.

**JUCE Classes:**
- `juce::AudioPlayHead` - Host tempo/position information
- `juce::AudioPlayHead::PositionInfo` - BPM, PPQ position
- Module: `juce_audio_processors`

**Tempo Sync Algorithm:**
```cpp
// Get host BPM and PPQ position
auto posInfo = getPlayHead()->getPosition();
double bpm = *posInfo->getBpm();
double ppq = *posInfo->getPpqPosition();

// Calculate step length in PPQ (pulses per quarter note)
double stepLengthPPQ = getStepLengthFromRate(rate);  // e.g., 0.25 for 1/16

// Calculate current step
int currentStep = (int)(ppq / stepLengthPPQ) % numSteps;
```

**Rate Subdivision Table:**
| Rate     | PPQ Length | Steps per Bar (4/4) |
|----------|-----------|---------------------|
| 1/1      | 4.0       | 1                   |
| 1/2      | 2.0       | 2                   |
| 1/4      | 1.0       | 4                   |
| 1/8      | 0.5       | 8                   |
| 1/16     | 0.25      | 16                  |
| 1/32     | 0.125     | 32                  |
| 1/8T     | 0.333...  | 12 (triplet)        |
| 1/16T    | 0.166...  | 24 (triplet)        |

**Per-Band Pattern Storage:**
- 4 bands × 32 steps max = 128 boolean states
- Stored as `std::array<std::array<bool, 32>, 4>` in processor
- Indexed as `stepPattern[bandIndex][stepIndex]`

**Swing Implementation:**
```cpp
// Swing delays every other step
double swingAmount = swingParameter.get();  // 0-100%
bool isOffBeat = (currentStep % 2 == 1);
double swingOffset = isOffBeat ? (swingAmount / 100.0) * 0.5 * stepLengthPPQ : 0.0;
double adjustedPPQ = ppq + swingOffset;
```

---

### 1.4 Euclidean Rhythm Generator

**Purpose:** Algorithmically generate rhythmic patterns per band.

**Algorithm (Bresenham/Bucket Fill):**
```cpp
std::array<bool, 32> generateEuclidean(int steps, int pulses, int offset) {
    std::array<bool, 32> pattern;
    pattern.fill(false);

    int bucket = 0;
    for (int i = 0; i < steps; i++) {
        bucket += pulses;
        if (bucket >= steps) {
            bucket -= steps;
            pattern[i] = true;
        }
    }

    // Apply rotation offset
    std::rotate(pattern.begin(), pattern.begin() + offset, pattern.begin() + steps);
    return pattern;
}
```

**Pattern Examples:**
- (16, 8, 0) → [X..X..X..X..X..X..X..X] - Every other step (classic 8th notes)
- (16, 5, 0) → [X...X..X...X..X.....] - Bjorklund(5,16)
- (16, 7, 0) → [X..X.X..X.X..X.X....] - Bjorklund(7,16)
- (8, 3, 0) → [X..X..X.] - West African bell pattern

**JUCE Classes:**
- None (pure algorithm, no JUCE dependencies)
- Implemented as standalone function in processor

**UI Integration:**
- Toggle per band: Manual mode vs Euclidean mode
- When Euclidean enabled, pattern overwrites manual step states
- Parameters: Steps (1-32), Pulses (1-32), Offset (0-31)

**References:**
- [Euclidean Rhythms - LANDR Blog](https://blog.landr.com/euclidean-rhythms/)
- [Medium: Euclidean Rhythms](https://medium.com/code-music-noise/euclidean-rhythms-391d879494df)

---

### 1.5 Gain Smoothing / Envelope

**Purpose:** Prevent clicks and artifacts when gate transitions occur.

**JUCE Classes:**
- `juce::SmoothedValue<float>` - One-pole lowpass smoothing
- Module: `juce_audio_basics`

**Smoothing Strategies:**

**A. Per-Bin Temporal Smoothing (Primary):**
```cpp
// One SmoothedValue per FFT bin (or per band)
juce::SmoothedValue<float> binGainSmoother[numBins];

// In prepareToPlay:
for (auto& smoother : binGainSmoother) {
    smoother.reset(sampleRate, smoothingTimeMs / 1000.0);
}

// In processBlock:
float targetGain = getStepGain(currentStep, binIndex);
float smoothedGain = binGainSmoother[binIndex].getNextValue(targetGain);
```

**B. Attack/Release Envelope (Alternative):**
```cpp
// Asymmetric smoothing (fast attack, slow release)
float attackTimeMs = 5.0f;
float releaseTimeMs = smoothingParameter.get();  // 0-100ms

if (targetGain > currentGain) {
    // Attack
    currentGain += (targetGain - currentGain) * attackCoeff;
} else {
    // Release
    currentGain += (targetGain - currentGain) * releaseCoeff;
}
```

**Trade-offs:**
- Temporal smoothing: Simpler, uniform behavior across all transitions
- Attack/Release: More "analog" feel, matches traditional gate behavior
- **Recommendation for v1.0:** Use juce::SmoothedValue with single smoothing parameter

**Parameter:**
- Smoothing: 0-100ms (global, applies to all bands)
- Default: 5ms (prevents clicks without excessive smearing)

---

### 1.6 Dry/Wet Mixer

**Purpose:** Blend processed and unprocessed signals.

**JUCE Classes:**
- `juce::dsp::DryWetMixer<float>` - Built-in dry/wet mixing
- Module: `juce_dsp`

**Usage:**
```cpp
juce::dsp::DryWetMixer<float> dryWetMixer;

void prepareToPlay(double sampleRate, int samplesPerBlock) {
    dryWetMixer.prepare({sampleRate, (uint32)samplesPerBlock, 2});
}

void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    dryWetMixer.pushDrySamples(buffer);

    // ... FFT processing ...

    dryWetMixer.mixWetSamples(buffer);
}
```

**Parameter:**
- Mix: 0-100% (0% = fully dry, 100% = fully wet)
- Default: 100% (fully processed)

---

## 2. Processing Chain

```
                    ┌─────────────────────────────────────────┐
                    │         Input Audio (Stereo)            │
                    └────────────────┬────────────────────────┘
                                     │
                    ┌────────────────▼────────────────────────┐
                    │     Dry/Wet Mixer - Push Dry Samples    │
                    └────────────────┬────────────────────────┘
                                     │
                    ┌────────────────▼────────────────────────┐
                    │      STFT Input Buffer (Overlap 75%)    │
                    │    Accumulate samples until frame full  │
                    └────────────────┬────────────────────────┘
                                     │
                                     │ Frame Ready?
                                     ▼
                    ┌─────────────────────────────────────────┐
                    │    Apply Hann Window (Analysis)         │
                    │   juce::dsp::WindowingFunction           │
                    └────────────────┬────────────────────────┘
                                     │
                    ┌────────────────▼────────────────────────┐
                    │      Forward FFT (Time → Frequency)      │
                    │   juce::dsp::FFT::performRealOnly...     │
                    │   Output: N/2+1 complex bins            │
                    └────────────────┬────────────────────────┘
                                     │
              ┌──────────────────────┼──────────────────────┐
              │                      │                      │
    ┌─────────▼─────────┐  ┌────────▼────────┐  ┌─────────▼─────────┐
    │   Band 1 (Sub)    │  │  Band 2 (Low)   │  │  Band 3 (Mid)     │
    │  Bins 1-5         │  │  Bins 5-23      │  │  Bins 23-186      │
    └─────────┬─────────┘  └────────┬────────┘  └─────────┬─────────┘
              │                      │                      │
    ┌─────────▼─────────┐  ┌────────▼────────┐  ┌─────────▼─────────┐
    │ Step Sequencer 1  │  │ Step Sequencer 2│  │ Step Sequencer 3  │
    │  Get Current Step │  │ Get Current Step│  │ Get Current Step  │
    │  Check On/Off     │  │ Check On/Off    │  │ Check On/Off      │
    └─────────┬─────────┘  └────────┬────────┘  └─────────┬─────────┘
              │                      │                      │
    ┌─────────▼─────────┐  ┌────────▼────────┐  ┌─────────▼─────────┐
    │ Gain Smoothing 1  │  │ Gain Smoothing 2│  │ Gain Smoothing 3  │
    │ SmoothedValue     │  │ SmoothedValue   │  │ SmoothedValue     │
    └─────────┬─────────┘  └────────┬────────┘  └─────────┬─────────┘
              │                      │                      │
    ┌─────────▼─────────┐  ┌────────▼────────┐  ┌─────────▼─────────┐
    │ Apply Gain to     │  │ Apply Gain to   │  │ Apply Gain to     │
    │ Bin Magnitudes    │  │ Bin Magnitudes  │  │ Bin Magnitudes    │
    │ (Preserve Phase)  │  │ (Preserve Phase)│  │ (Preserve Phase)  │
    └─────────┬─────────┘  └────────┬────────┘  └─────────┬─────────┘
              │                      │                      │
              └──────────────────────┼──────────────────────┘
                                     │
                    ┌────────────────▼────────────────────────┐
                    │   Inverse FFT (Frequency → Time)        │
                    │   juce::dsp::FFT::performRealOnly...     │
                    └────────────────┬────────────────────────┘
                                     │
                    ┌────────────────▼────────────────────────┐
                    │   Apply Hann Window (Synthesis)         │
                    └────────────────┬────────────────────────┘
                                     │
                    ┌────────────────▼────────────────────────┐
                    │   Overlap-Add Reconstruction             │
                    │   (75% overlap, hop size = 512)         │
                    └────────────────┬────────────────────────┘
                                     │
                    ┌────────────────▼────────────────────────┐
                    │    Dry/Wet Mixer - Mix Wet Samples      │
                    └────────────────┬────────────────────────┘
                                     │
                    ┌────────────────▼────────────────────────┐
                    │        Output Audio (Stereo)             │
                    └─────────────────────────────────────────┘
```

**Processing Order Notes:**
1. Dry/wet mixer pushes dry samples BEFORE any processing
2. STFT buffers accumulate input samples until frame is full (512 samples)
3. FFT processing is frame-based, not sample-by-sample
4. Each band's gain is calculated independently per step
5. Smoothing prevents clicks between step transitions
6. Phase information is preserved throughout (critical for sound quality)
7. Overlap-add ensures COLA (Constant Overlap-Add) for perfect reconstruction

---

## 3. System Architecture

### 3.1 Thread Boundaries

**Audio Thread (Real-time):**
- FFT processing
- Bin magnitude modification
- Overlap-add reconstruction
- Tempo sync state reading
- Smoothing calculations

**UI Thread (Non-real-time):**
- Parameter changes via APVTS
- Step grid editing
- Euclidean pattern generation
- Visual updates (playhead, grid state)

**Thread-Safe Communication:**
- Use `juce::AudioProcessorValueTreeState` (APVTS) for all parameters
- Atomic flags for mode switches (Manual ↔ Euclidean)
- No direct UI → processBlock calls
- Pattern updates: Generate in UI thread, swap pointer atomically

---

### 3.2 State Persistence

**Plugin State (Preset System):**
- All parameters via APVTS (auto-serialized)
- Step grid states: 4 × 32 booleans (128 bits, compact)
- Euclidean settings: 3 integers per band (steps, pulses, offset)
- Band frequencies: 8 floats (low/high per 4 bands)

**Serialization Format:**
```xml
<PluginState>
  <Parameters>
    <Parameter id="mix" value="1.0"/>
    <Parameter id="steps" value="16"/>
    ...
  </Parameters>
  <StepGrid>
    <Band index="0" pattern="1010101010101010"/>  <!-- Binary string -->
    <Band index="1" pattern="1100110011001100"/>
    ...
  </StepGrid>
</PluginState>
```

---

### 3.3 UI Architecture (WebView-Based)

**Technology Stack:**
- JUCE WebBrowserComponent (WebView container)
- HTML/CSS/JavaScript (UI implementation)
- JUCE's JavaScript bridge (C++ ↔ JS communication)

**UI Components:**
1. **2D Step Grid:**
   - Y-axis: 4 frequency bands (Sub, Low, Mid, High)
   - X-axis: 4/8/16/32 steps (configurable)
   - Cells: Clickable buttons (toggle on/off)
   - Current step: Visual playhead indicator

2. **Band Configuration:**
   - Per-band enable/disable toggle
   - Frequency range sliders (low/high cutoff)
   - Depth control (gate attenuation amount)

3. **Euclidean Controls:**
   - Per-band toggle: Manual ↔ Euclidean
   - Steps slider (1-32)
   - Pulses slider (1-32)
   - Offset slider (0-31)

4. **Global Controls:**
   - Mix knob (dry/wet)
   - Rate dropdown (1/1 to 1/32, triplets, dotted)
   - Swing slider (0-100%)
   - Smoothing slider (0-100ms)

**Parameter Binding:**
- Use `juce::WebSliderParameterAttachment` for knobs/sliders
- Use `juce::WebToggleButtonParameterAttachment` for buttons
- Custom JavaScript callbacks for step grid editing

**Critical Pattern (from juce8-critical-patterns.md):**
- MUST include `type="module"` in script tags for ES6 imports
- MUST use `import { getSliderState } from './js/juce/index.js'`
- MUST use relative drag for knobs (frame-delta pattern)
- MUST use `requestAnimationFrame` for playhead animation

---

## 4. Parameter Mapping

### 4.1 Global Parameters (5 total)

| Parameter ID | Name      | Type  | Range       | Default | Skew | Unit |
|-------------|-----------|-------|-------------|---------|------|------|
| mix         | Mix       | Float | 0.0 - 1.0   | 1.0     | 1.0  | %    |
| steps       | Steps     | Int   | 4/8/16/32   | 16      | N/A  | -    |
| rate        | Rate      | Int   | 0-9 (enum)  | 4 (1/16)| N/A  | -    |
| swing       | Swing     | Float | 0.0 - 1.0   | 0.0     | 1.0  | %    |
| smoothing   | Smoothing | Float | 0.0 - 100.0 | 5.0     | 1.0  | ms   |

**Rate Enum Mapping:**
```
0 = 1/1 (whole)
1 = 1/2 (half)
2 = 1/4 (quarter)
3 = 1/8
4 = 1/16
5 = 1/32
6 = 1/8T (triplet)
7 = 1/16T (triplet)
8 = 1/4D (dotted)
9 = 1/8D (dotted)
```

---

### 4.2 Per-Band Parameters (32 total: 8 × 4 bands)

| Parameter ID Pattern | Name            | Type  | Range       | Default (Band-specific) |
|--------------------|-----------------|-------|-------------|------------------------|
| band{N}_enable     | Band N Enable   | Bool  | On/Off      | On                     |
| band{N}_low        | Band N Low Freq | Float | 20 - 20000  | [20,120,500,4000]      |
| band{N}_high       | Band N High Freq| Float | 20 - 20000  | [120,500,4000,20000]   |
| band{N}_depth      | Band N Depth    | Float | 0.0 - 1.0   | 1.0                    |
| band{N}_euc_on     | Band N Euclidean| Bool  | On/Off      | Off                    |
| band{N}_euc_steps  | Band N Euc Steps| Int   | 1 - 32      | 16                     |
| band{N}_euc_pulses | Band N Euc Pulses|Int   | 1 - 32      | 8                      |
| band{N}_euc_offset | Band N Euc Offset|Int   | 0 - 31      | 0                      |

**{N}** = 0, 1, 2, 3 (band indices)

**Frequency Skew:**
- Use logarithmic skew for frequency parameters (better control across range)
- Skew factor: 0.3 (standard for audio frequencies)

---

### 4.3 Step Grid Parameters (128 total: 32 steps × 4 bands)

| Parameter ID Pattern | Name             | Type  | Range  | Default |
|---------------------|------------------|-------|--------|---------|
| step_b{N}_s{M}      | Band N Step M    | Bool  | On/Off | Off     |

**{N}** = 0, 1, 2, 3 (band index)
**{M}** = 0-31 (step index)

**Storage Optimization:**
- Store as bitfield internally (128 bits = 16 bytes)
- Expose as individual bool parameters for UI binding
- Only serialize non-default values in presets

---

### 4.4 Parameter Interactions

**Dependency: Steps Count ↔ Step Grid**
- When `steps` parameter changes (4/8/16/32), UI displays only active steps
- Inactive steps retain state but don't participate in sequencing

**Dependency: Euclidean Mode ↔ Manual Steps**
- When `band{N}_euc_on` = true, generated pattern overrides manual steps
- Manual step states preserved (user can toggle back without losing pattern)

**Dependency: Band Frequency Ranges**
- Validate: `band{N}_low` < `band{N}_high`
- Prevent overlap: Band N high ≤ Band N+1 low (optional enforcement)

---

## 5. Algorithm Details

### 5.1 FFT Bin-to-Frequency Mapping

```cpp
float binToFrequency(int binIndex, double sampleRate, int fftSize) {
    return (binIndex * sampleRate) / fftSize;
}

int frequencyToBin(float frequency, double sampleRate, int fftSize) {
    return (int)std::round((frequency * fftSize) / sampleRate);
}
```

**Example (44.1kHz, 2048 FFT):**
- Bin 0: 0 Hz (DC)
- Bin 1: 21.53 Hz
- Bin 10: 215.3 Hz
- Bin 100: 2153 Hz
- Bin 1024: 22050 Hz (Nyquist)

**Frequency Resolution:**
- Δf = sampleRate / fftSize
- At 44.1kHz with 2048 FFT: Δf = 21.53 Hz per bin
- At 48kHz with 2048 FFT: Δf = 23.44 Hz per bin

---

### 5.2 Complex Magnitude Scaling

```cpp
// FFT output format: [real0, imag0, real1, imag1, ..., realN/2, imagN/2]
// Note: bin 0 (DC) and bin N/2 (Nyquist) are real-only

void applyGainToBin(float* fftData, int binIndex, float gain, int fftSize) {
    int realIndex = binIndex * 2;
    int imagIndex = realIndex + 1;

    // Scale both real and imaginary parts (preserves phase)
    fftData[realIndex] *= gain;
    fftData[imagIndex] *= gain;
}

float getBinMagnitude(const float* fftData, int binIndex) {
    int realIndex = binIndex * 2;
    int imagIndex = realIndex + 1;

    float real = fftData[realIndex];
    float imag = fftData[imagIndex];

    return std::sqrt(real * real + imag * imag);
}
```

**Phase Preservation:**
- Multiply both real and imaginary components by same gain
- Magnitude changes, phase angle remains constant
- Critical for avoiding "phasiness" artifacts

---

### 5.3 COLA (Constant Overlap-Add) Window Scaling

```cpp
// Window must satisfy COLA constraint for perfect reconstruction
// Hann window with 75% overlap satisfies this

void applyWindowAndScale(float* data, int fftSize, float overlapFactor) {
    // JUCE's WindowingFunction already includes COLA normalization
    juce::dsp::WindowingFunction<float> window(
        fftSize,
        juce::dsp::WindowingFunction<float>::hann,
        true  // Enable COLA normalization
    );

    window.multiplyWithWindowingTable(data, fftSize);
}
```

---

### 5.4 Tempo Sync PPQ Calculation

```cpp
double getStepLengthPPQ(int rateEnum) {
    const double ppqValues[] = {
        4.0,       // 1/1 (whole note)
        2.0,       // 1/2 (half note)
        1.0,       // 1/4 (quarter note)
        0.5,       // 1/8
        0.25,      // 1/16
        0.125,     // 1/32
        1.0/3.0,   // 1/8T (triplet)
        1.0/6.0,   // 1/16T (triplet)
        1.5,       // 1/4D (dotted)
        0.75       // 1/8D (dotted)
    };
    return ppqValues[rateEnum];
}

int getCurrentStep(double ppq, int numSteps, double stepLengthPPQ) {
    return (int)(ppq / stepLengthPPQ) % numSteps;
}
```

---

## 6. Integration Points

### 6.1 Dependencies

**Module Dependencies (CMakeLists.txt):**
```cmake
target_link_libraries(O-FreqPulse
    PRIVATE
        juce::juce_audio_processors   # AudioProcessor, AudioPlayHead
        juce::juce_dsp                # FFT, WindowingFunction, DryWetMixer
        juce::juce_gui_extra          # WebBrowserComponent
)
```

**Component Interaction:**
```
AudioPlayHead ─(BPM, PPQ)──> StepSequencer ─(Current Step)──> BandGate
                                                                    │
FFTProcessor ─(Bin Data)──> BandSplitter ─(Band Assignments)────>  │
                                                                    │
EuclideanGen ─(Pattern)────> StepSequencer                         │
                                                                    │
Parameters ───(Smoothing)──> SmoothedValue ──────────────────────> │
                                                                    │
                            ┌────────────────────────────────────<─┘
                            │
                            ▼
                    Apply Gain to Bins
```

---

### 6.2 Parameter Processing Order

**In prepareToPlay():**
1. Initialize FFT object (juce::dsp::FFT)
2. Allocate FFT buffers (input, output, window)
3. Setup DryWetMixer
4. Reset SmoothedValue objects with sample rate
5. Clear STFT overlap buffers

**In processBlock():**
1. Push dry samples to DryWetMixer
2. Get tempo/position from AudioPlayHead
3. Calculate current step per band
4. FOR each audio sample:
   - Buffer into STFT input frame
   - IF frame full:
     - Apply analysis window
     - Forward FFT
     - FOR each bin:
       - Determine band
       - Get step state (on/off)
       - Calculate target gain (with depth)
       - Smooth gain transition
       - Apply gain to bin
     - Inverse FFT
     - Apply synthesis window
     - Overlap-add to output buffer
5. Mix wet samples with DryWetMixer
6. Report latency to host (if not already set)

---

### 6.3 Thread Communication

**Lock-Free Pattern for Step Grid Updates:**

```cpp
// In Processor
std::atomic<StepPattern*> activePattern{nullptr};
StepPattern patternA, patternB;

// UI thread generates new pattern
StepPattern* newPattern = (activePattern == &patternA) ? &patternB : &patternA;
generateEuclideanPattern(newPattern, steps, pulses, offset);

// Atomic swap (no locks, safe)
activePattern.store(newPattern, std::memory_order_release);

// Audio thread reads
StepPattern* pattern = activePattern.load(std::memory_order_acquire);
bool stepOn = (*pattern)[bandIndex][stepIndex];
```

---

## 7. Implementation Risks

### 7.1 HIGH Risk: FFT Processing Artifacts

**Problem:**
- "Musical noise" (chirpy/watery artifacts) from abrupt spectral changes
- Phase discontinuities causing "phasey" sound
- Spectral leakage at band boundaries

**Mitigation:**
1. **Temporal Smoothing:** Use juce::SmoothedValue per band (not per bin for efficiency)
2. **Adequate Overlap:** 75% overlap (4×) prevents most artifacts
3. **Proper COLA:** Hann window with COLA normalization (JUCE handles this)
4. **Phase Preservation:** Multiply magnitude, don't touch phase

**Fallback:**
- If artifacts persist, add crossfade zones at band boundaries
- Increase smoothing time parameter default
- Consider 8× overlap (87.5%) for critical applications (higher CPU cost)

**Testing:**
- Test with sine sweeps (reveal phase issues)
- Test with white noise (reveal temporal smearing)
- Test with drum transients (reveal attack distortion)

---

### 7.2 MEDIUM Risk: Latency Perception

**Problem:**
- 2048 FFT at 44.1kHz = 46ms latency (audible delay)
- Users may perceive plugin as "sluggish" in real-time tracking

**Mitigation:**
1. **Report Latency:** Use `setLatencySamples(2048)` for DAW compensation
2. **Bypass Zero-Latency:** Implement true bypass (no FFT) when Mix = 0%
3. **Documentation:** Clearly state latency in manual, explain PDC (Plugin Delay Compensation)

**Fallback (v1.1+):**
- Add "Low Latency Mode" parameter (1024 FFT, ~23ms latency)
- Trade-off: Lower frequency resolution (43 Hz bins vs 21 Hz)
- Let user choose: Latency vs Resolution

---

### 7.3 MEDIUM Risk: CPU Performance

**Problem:**
- FFT processing is CPU-intensive (especially at high sample rates)
- 4 bands × 32 steps = complex per-frame calculations
- Target: <5% CPU on Apple Silicon

**Mitigation:**
1. **SIMD Optimization:** JUCE's FFT already uses SIMD (vDSP on macOS)
2. **Process Only Active Bands:** Skip FFT bins for disabled bands
3. **Efficient Smoothing:** One SmoothedValue per band, not per bin
4. **Avoid Allocations:** Pre-allocate all buffers in prepareToPlay()

**Profiling Strategy:**
- Use Xcode Instruments (Time Profiler) to identify bottlenecks
- Measure FFT vs bin processing vs smoothing time
- Optimize hottest code paths first

**Fallback:**
- If CPU too high, reduce overlap factor (4× → 2×, trades quality for performance)
- Add "Quality Mode" parameter (high/balanced/low CPU)

---

### 7.4 LOW Risk: UI Performance (Grid Rendering)

**Problem:**
- 4 bands × 32 steps = 128 clickable cells
- Playhead updates at ~60fps
- WebView rendering overhead

**Mitigation:**
1. **Batch DOM Updates:** Use single `innerHTML` update per frame, not per cell
2. **CSS Transforms:** Use `transform: translateX()` for playhead (GPU-accelerated)
3. **RequestAnimationFrame:** Throttle updates to display refresh rate
4. **Dirty Rectangle:** Only redraw changed cells, not entire grid

**Fallback:**
- If WebView slow, consider native JUCE UI (juce::Grid, juce::Button)
- Benchmark: O-Polystutter's multi-lane UI is proven to perform well

---

## 8. Architecture Decisions

### 8.1 Why FFT-Based (vs Filter Banks)?

**Decision:** Use STFT (Short-Time Fourier Transform) instead of crossover filters.

**Alternatives Considered:**
1. **Linkwitz-Riley Crossover Filters** (like Sinevibes Array)
   - Pros: Lower latency, simpler CPU profile
   - Cons: Fixed band count, less flexible frequency ranges

2. **IIR Band-Pass Filter Bank**
   - Pros: Zero latency (recursive filters)
   - Cons: Phase distortion, harder to sync across bands

**Why STFT Won:**
- Flexible band configuration (any frequency range)
- True spectral resolution (not limited to 4-8 bands)
- Foundation for future features (spectral painting, freeze)
- Clean separation without phase issues (linear phase)
- Proven architecture (SpecOps, Harrison Spectral Gate)

**Trade-off Accepted:**
- Latency (~46ms) is acceptable for mixing/production use
- Not suitable for live performance tracking (but DAW PDC handles it)

---

### 8.2 Why Hard Cutoff Bands (vs Crossfade)?

**Decision:** Use hard cutoff between bands for v1.0.

**Alternatives Considered:**
1. **Overlapping Crossfade** (e.g., 200-400Hz overlap zone)
   - Pros: Smoother spectral transitions, fewer artifacts at boundaries
   - Cons: More complex bin assignment, higher CPU (process bins twice)

**Why Hard Cutoff Won:**
- Simpler implementation (single band per bin)
- Acceptable for rhythmic gating (artifacts masked by transients)
- Can add crossfade in v1.1 if users report issues

**When to Revisit:**
- If users report "spectral holes" at band boundaries
- If A/B testing shows audible artifacts on sustained tones

---

### 8.3 Why WebView UI (vs Native JUCE)?

**Decision:** Use WebView-based UI (HTML/CSS/JavaScript).

**Alternatives Considered:**
1. **Native JUCE Components** (juce::Grid + juce::Button)
   - Pros: Proven performance, no browser dependencies
   - Cons: More C++ code, harder to iterate on design

2. **OpenGL Custom Renderer**
   - Pros: Maximum performance, ultimate flexibility
   - Cons: Complex code, overkill for 2D grid

**Why WebView Won:**
- Rapid iteration (edit HTML/CSS without recompiling)
- Proven pattern from O-series plugins (O-Detune, O-Tremolo, etc.)
- Rich visual design capabilities (CSS animations, gradients)
- Established JUCE JavaScript bridge pattern

**Risk Mitigation:**
- Follow juce8-critical-patterns.md exactly (ES6 modules, relative drag, RAF loop)
- Fallback: Native UI prototype ready if WebView performance insufficient

---

### 8.4 Why Euclidean Generation (vs Full MIDI Sequencer)?

**Decision:** Include Euclidean rhythm generator per band.

**Alternatives Considered:**
1. **Manual Step Editing Only**
   - Pros: Simpler UI, no algorithm complexity
   - Cons: Misses algorithmic pattern discovery

2. **MIDI Trigger Input**
   - Pros: Maximum flexibility, external sequencer control
   - Cons: Scope creep, adds MIDI handling complexity

**Why Euclidean Won:**
- Unique selling point (no competitor has per-band Euclidean)
- Proven algorithm (simple Bresenham implementation)
- Encourages experimentation (polyrhythmic patterns)
- Complements manual editing (hybrid workflow)

**Scope:**
- v1.0: Euclidean generation only (no MIDI input)
- v1.2+: Add MIDI trigger support if requested

---

## 9. Special Considerations

### 9.1 Thread Safety

**Audio Thread Guarantees:**
- No allocations in processBlock()
- No locks (use atomic operations)
- Pre-allocated FFT buffers (size = max possible FFT size)

**Parameter Thread Safety:**
- APVTS handles all parameter synchronization
- Atomic read for step pattern pointer
- Avoid reading UI state directly in processBlock()

---

### 9.2 Performance Optimization

**Critical Hot Paths:**
1. FFT forward/inverse transforms (vectorized by JUCE)
2. Bin magnitude scaling loop (consider SIMD for large band ranges)
3. Overlap-add accumulation (sequential, hard to vectorize)

**Optimization Checklist:**
- [ ] Pre-calculate bin-to-band mapping table (don't recalc every frame)
- [ ] Use `juce::FloatVectorOperations` for bulk gain application
- [ ] Profile with Release build (Debug is misleading for DSP)
- [ ] Test at 96kHz (2× workload vs 48kHz)

---

### 9.3 Denormal Prevention

**Problem:** Denormalized floats (near-zero values) cause CPU spikes.

**Solution:**
```cpp
// In prepareToPlay()
juce::FloatVectorOperations::disableDenormalisedNumberSupport();

// After FFT processing
juce::FloatVectorOperations::clip(fftData, fftData, -1.0f, 1.0f, fftSize);
```

---

### 9.4 Sample Rate Handling

**Supported Sample Rates:**
- 44.1 kHz (standard)
- 48 kHz (video standard)
- 88.2 kHz (high-res)
- 96 kHz (high-res)

**Sample Rate Dependencies:**
- Bin-to-frequency mapping (recalculate on rate change)
- Smoothing time (recalculate coefficients)
- FFT size remains constant (2048 samples = varying time duration)

**Consideration:**
- At 96kHz, 2048 FFT = 21ms latency (lower than 44.1kHz's 46ms)
- Frequency resolution stays constant (bins spread across wider spectrum)

---

### 9.5 Latency Reporting

**Implementation:**
```cpp
int PluginProcessor::getLatencySamples() const override {
    return fftSize;  // 2048 samples
}

void PluginProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    setLatencySamples(fftSize);
    // ... rest of preparation
}
```

**DAW Behavior:**
- DAW shifts plugin's output back by latency samples
- Aligned with other tracks for zero-latency monitoring
- Critical for maintaining groove/timing

---

## 10. Research References

### Professional Plugins Analyzed

1. **Sinevibes Array v4** - [Spectral Sequencer](https://www.sinevibes.com/array/)
   - 8-band octave-separated filter with step sequencer
   - Clean -24 dB/octave crossover filters
   - Two-pole lag filters for gate transitions
   - Polymetric/polyrhythmic timing

2. **Unfiltered Audio SpecOps** - [Spectral Effects](https://www.plugin-alliance.com/products/specops)
   - FFT-based processing (128-32768 bins)
   - 16-step sequencer for modulation
   - 36 spectral effects library
   - Patchable modulation system

3. **Harrison Spectral Gate** - [Frequency-Dependent Gate](https://store.harrisonaudio.com/all-products/spectral-gate)
   - Spectral fingerprint learning
   - Noise reduction focus (not rhythmic)

4. **SoundHack ++spectralgate** - [Multiband Spectral Gate](https://www.soundhack.com/spectralgate-manual/)
   - 8-8192 bands configurable
   - Drawable per-frequency thresholds

5. **Kilohearts Trance Gate** - [Traditional Trance Gate](https://kilohearts.com/products/trance_gate)
   - Wideband gating (no spectral processing)
   - Step sequencer with ADSR envelope

### Technical References

1. **Overlap-Add STFT Processing** - [CCRMA Stanford](https://www.dsprelated.com/freebooks/sasp/Overlap_Add_OLA_STFT_Processing.html)
   - COLA constraint explanation
   - Window function trade-offs

2. **FFT Processing in JUCE** - [Audio Dev Blog](https://audiodev.blog/fft-processing/)
   - Practical JUCE FFT implementation
   - Real-time processing patterns

3. **Euclidean Rhythms** - [LANDR Blog](https://blog.landr.com/euclidean-rhythms/)
   - Algorithm explanation (Bresenham method)
   - Musical applications

4. **Euclidean Rhythms (Medium)** - [Code Music Noise](https://medium.com/code-music-noise/euclidean-rhythms-391d879494df)
   - Implementation details
   - VST plugin examples

### JUCE Documentation

1. **juce::dsp::FFT** - [JUCE Docs](https://docs.juce.com/master/classjuce_1_1dsp_1_1FFT.html)
   - Forward/inverse transform methods
   - Real-only optimization

2. **juce::dsp::WindowingFunction** - [JUCE Docs](https://docs.juce.com/master/classjuce_1_1dsp_1_1WindowingFunction.html)
   - Window types (Hann, Blackman, etc.)
   - COLA normalization

3. **juce::AudioPlayHead** - [JUCE Docs](https://docs.juce.com/master/classAudioPlayHead.html)
   - BPM and PPQ position retrieval
   - Tempo sync patterns

4. **juce::SmoothedValue** - [JUCE Docs](https://docs.juce.com/master/classjuce_1_1SmoothedValue.html)
   - One-pole lowpass smoothing
   - Sample-accurate parameter ramping

### Algorithm Research

- **Pre-Research Document:** `/research/spectral-sequencer-research.md`
  - Comprehensive market analysis
  - Algorithm deep-dive
  - Use cases and parameter design

---

## 11. Validation Checklist

**Architecture Completeness:**
- [X] All 9 features from meta-research documented
- [X] Every JUCE class has module dependency listed
- [X] Processing chain shows complete signal flow
- [X] Integration analysis covers dependencies, interactions, order, threads
- [X] All HIGH risk features have fallback architectures
- [X] Parameter mapping complete (165 parameters)
- [X] Professional plugin research cited (5 plugins)

**Implementation Readiness:**
- [X] FFT processing algorithm specified
- [X] Band splitting algorithm specified
- [X] Euclidean generation algorithm specified
- [X] Tempo sync pattern documented
- [X] Smoothing strategy chosen
- [X] Thread boundaries defined
- [X] State persistence format designed

**Risk Assessment:**
- [X] 4 risks identified (1 HIGH, 2 MEDIUM, 1 LOW)
- [X] All risks have mitigation strategies
- [X] Fallback architectures documented for MEDIUM+ risks
- [X] Performance profiling strategy outlined

---

**Generated:** 2026-02-03 by research-planning-agent
**Status:** ARCHITECTURE COMPLETE - Ready for ROADMAP.md generation
