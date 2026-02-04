# Stage 2: DSP - Research Findings

**Plugin:** O-FreqPulse
**Stage:** 2 (DSP Implementation)
**Date:** 2026-02-03
**Status:** COMPLETE

---

## 1. JUCE API Research

### 1.1 FFT Processing (`juce::dsp::FFT`)

**Module:** `juce_dsp`

**Construction:**
```cpp
// FFT order determines size: 2^11 = 2048 samples
static constexpr int fftOrder = 11;
static constexpr int fftSize = 1 << fftOrder;  // 2048
static constexpr int numBins = fftSize / 2 + 1;  // 1025 complex bins

juce::dsp::FFT fft { fftOrder };
```

**Forward Transform:**
```cpp
// Buffer must be 2 * fftSize (4096 floats for 2048-point FFT)
std::array<float, fftSize * 2> fftData;

// Copy audio into first half
std::copy(inputFrame.begin(), inputFrame.end(), fftData.begin());

// Transform - second parameter: true = positive frequencies only
fft.performRealOnlyForwardTransform(fftData.data(), true);
```

**Data Format After Forward FFT:**
- Complex interleaved: `[real0, imag0, real1, imag1, ...]`
- First `numBins` (1025) complex numbers contain valid frequency data
- Bin 0 = DC (0 Hz), Bin 512 = Nyquist
- Magnitudes are scaled by `fftSize` (divide by 2048 after processing)

**Inverse Transform:**
```cpp
fft.performRealOnlyInverseTransform(fftData.data());
// Automatically divides by fftSize internally
// Output in first fftSize samples
```

**Reference:** [JUCE FFT Documentation](https://docs.juce.com/master/classjuce_1_1dsp_1_1FFT.html), [audiodev.blog FFT Processing](https://audiodev.blog/fft-processing/)

---

### 1.2 Windowing Function (`juce::dsp::WindowingFunction`)

**Module:** `juce_dsp`

**Setup:**
```cpp
// NOTE: Size = fftSize + 1 for periodic (not symmetric) window
juce::dsp::WindowingFunction<float> window {
    static_cast<size_t>(fftSize + 1),
    juce::dsp::WindowingFunction<float>::WindowingMethod::hann,
    false  // Don't normalize - we handle gain manually
};
```

**Application:**
```cpp
// Apply window before FFT (analysis window)
window.multiplyWithWindowingTable(fftData.data(), fftSize);

// ... FFT, spectral processing, IFFT ...

// Apply window after IFFT (synthesis window)
window.multiplyWithWindowingTable(fftData.data(), fftSize);
```

**COLA Gain Correction:**
For Hann window with 75% overlap (4× overlap factor):
```cpp
const float windowCorrection = 2.0f / 3.0f;  // Compensates for 1.5× amplification
for (int i = 0; i < fftSize; ++i) {
    fftData[i] *= windowCorrection;
}
```

**Reference:** [audiodev.blog STFT Processing](https://audiodev.blog/fft-processing/)

---

### 1.3 Tempo Sync (`juce::AudioPlayHead`)

**Module:** `juce_audio_processors`

**Getting Position Info (JUCE 8 API):**
```cpp
void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    auto playHead = getPlayHead();
    if (playHead != nullptr) {
        auto posInfo = playHead->getPosition();

        if (posInfo.hasValue()) {
            // BPM
            if (auto bpmOpt = posInfo->getBpm()) {
                double bpm = *bpmOpt;
            }

            // PPQ Position (pulses per quarter note)
            if (auto ppqOpt = posInfo->getPpqPosition()) {
                double ppq = *ppqOpt;
            }

            // Transport state
            if (auto playing = posInfo->getIsPlaying()) {
                bool isPlaying = *playing;
            }
        }
    }
}
```

**Step Calculation from PPQ:**
```cpp
// Rate enum to PPQ length mapping
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

int getCurrentStep(double ppq, int numSteps, int rateIndex) {
    double stepLengthPPQ = ppqValues[rateIndex];
    return static_cast<int>(ppq / stepLengthPPQ) % numSteps;
}
```

**Reference:** [JUCE AudioPlayHead](https://docs.juce.com/master/classAudioPlayHead.html)

---

### 1.4 Dry/Wet Mixing (`juce::dsp::DryWetMixer`)

**Module:** `juce_dsp`

**Setup:**
```cpp
juce::dsp::DryWetMixer<float> dryWetMixer;

void prepareToPlay(double sampleRate, int samplesPerBlock) {
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = 2;

    dryWetMixer.prepare(spec);
    dryWetMixer.reset();
}
```

**Usage in processBlock:**
```cpp
// Push dry samples FIRST (before any processing)
dryWetMixer.pushDrySamples(juce::dsp::AudioBlock<float>(buffer));

// ... all spectral processing here ...

// Mix wet samples at END
dryWetMixer.setWetMixProportion(mixParam->load());  // 0.0 - 1.0
dryWetMixer.mixWetSamples(juce::dsp::AudioBlock<float>(buffer));
```

**Reference:** [juce::dsp::DryWetMixer](https://docs.juce.com/master/classjuce_1_1dsp_1_1DryWetMixer.html)

---

### 1.5 Gain Smoothing (`juce::SmoothedValue`)

**Module:** `juce_audio_basics`

**Setup (per band):**
```cpp
std::array<juce::SmoothedValue<float>, 4> bandGainSmooth;

void prepareToPlay(double sampleRate, int) {
    for (auto& smoother : bandGainSmooth) {
        smoother.reset(sampleRate, 0.005);  // 5ms smoothing time
    }
}
```

**Usage:**
```cpp
// Set target gain (0.0 = muted, 1.0 = full)
bandGainSmooth[bandIdx].setTargetValue(stepIsOn ? 1.0f : (1.0f - depth));

// Get smoothed value per sample or per frame
float currentGain = bandGainSmooth[bandIdx].getNextValue();
```

---

## 2. Algorithm Research

### 2.1 STFT Overlap-Add Processing

**Configuration for O-FreqPulse:**
- FFT Size: 2048 samples
- Hop Size: 512 samples (fftSize / 4)
- Overlap: 75% (4× overlap factor)
- Window: Hann (satisfies COLA constraint)
- Latency: 2048 samples (~46ms at 44.1kHz)

**Processing Flow:**
```
1. Accumulate input samples into ring buffer
2. When hopSize samples accumulated:
   a. Copy fftSize samples to FFT buffer
   b. Apply analysis window (Hann)
   c. Forward FFT
   d. Process bins (apply band gains)
   e. Inverse FFT
   f. Apply synthesis window (Hann)
   g. Apply COLA gain correction (2/3)
   h. Overlap-add to output buffer
3. Output from output buffer (oldest samples)
```

**Circular Buffer Strategy:**
```cpp
// Input FIFO - accumulates incoming samples
std::vector<float> inputFifo;    // Size: fftSize per channel
int inputWritePos = 0;

// Output FIFO - overlap-add output buffer
std::vector<float> outputFifo;   // Size: fftSize per channel
int outputReadPos = 0;

// FFT buffers
std::vector<float> fftData;      // Size: fftSize * 2 (complex interleaved)
```

**Reference:** [CCRMA Overlap-Add](https://www.dsprelated.com/freebooks/sasp/Overlap_Add_OLA_STFT_Processing.html)

---

### 2.2 Euclidean Rhythm Algorithm

**Bresenham/Bucket-Fill Method:**
```cpp
std::array<bool, 32> generateEuclidean(int steps, int pulses, int offset) {
    std::array<bool, 32> pattern;
    pattern.fill(false);

    if (pulses > steps) pulses = steps;  // Clamp
    if (pulses == 0) return pattern;      // All silent

    int bucket = 0;
    for (int i = 0; i < steps; i++) {
        bucket += pulses;
        if (bucket >= steps) {
            bucket -= steps;
            pattern[i] = true;
        }
    }

    // Apply rotation offset
    if (offset > 0 && offset < steps) {
        std::rotate(pattern.begin(),
                    pattern.begin() + offset,
                    pattern.begin() + steps);
    }

    return pattern;
}
```

**Common Euclidean Patterns:**
- (8, 3, 0) = `[X..X..X.]` - West African bell
- (8, 5, 0) = `[X.X.X.XX]` - Cuban cinquillo
- (16, 5, 0) = `[X...X..X...X..X.]` - Bossa nova
- (16, 7, 0) = `[X..X.X..X.X..X.X]` - Afro-Cuban

**Reference:** [Euclidean Rhythms - LANDR](https://blog.landr.com/euclidean-rhythms/)

---

### 2.3 Bin-to-Band Mapping

**Frequency-to-Bin Conversion:**
```cpp
int frequencyToBin(float freq, double sampleRate, int fftSize) {
    return static_cast<int>(std::round(freq * fftSize / sampleRate));
}

float binToFrequency(int bin, double sampleRate, int fftSize) {
    return static_cast<float>(bin * sampleRate / fftSize);
}
```

**Pre-calculated Lookup Table:**
```cpp
// Array maps each bin to a band index (0-3, or -1 for passthrough)
std::array<int, 1025> bandForBin;

void recalculateBinMapping(double sampleRate) {
    float bandLow[4], bandHigh[4];
    // ... read from parameters ...

    for (int bin = 0; bin < 1025; ++bin) {
        float freq = binToFrequency(bin, sampleRate, 2048);
        bandForBin[bin] = -1;  // Default: passthrough

        for (int b = 0; b < 4; ++b) {
            if (freq >= bandLow[b] && freq < bandHigh[b]) {
                bandForBin[bin] = b;
                break;
            }
        }
    }
}
```

---

### 2.4 Complex Magnitude Scaling (Phase Preservation)

**Applying Gain to FFT Bins:**
```cpp
void applyGainToBin(float* fftData, int bin, float gain) {
    int realIdx = bin * 2;
    int imagIdx = realIdx + 1;

    // Scale both components by same factor - preserves phase
    fftData[realIdx] *= gain;
    fftData[imagIdx] *= gain;
}
```

**Critical:** Multiply both real and imaginary parts by the same gain value. This scales magnitude while preserving phase angle.

---

## 3. Module Reuse Assessment

### 3.1 Existing Modules Review

| Module | Relevance | Decision |
|--------|-----------|----------|
| `webview-relay-manager` | Stage 3 (GUI) | Will use for WebView parameter binding |
| `vu-meter` | Not needed | No metering in v1.0 |
| `preset-manager` | Stage 4 | Consider for presets |
| `scala-tuning-engine` | Not relevant | Tuning not needed |

### 3.2 New Code Required

All DSP components are **custom implementations**:
1. STFT processor (ring buffers, FFT, overlap-add)
2. Band processor (bin mapping, gain application)
3. Step sequencer engine (tempo sync, step calculation)
4. Euclidean generator (pattern algorithm)
5. Gain smoothing array (per-band SmoothedValue)

No existing module provides FFT-based spectral processing - this will be the first spectral effect in the O-series.

---

## 4. Pitfalls from Knowledge Base

### 4.1 Real-Time Safety (from `stage-2-patterns.md`)

**Mandatory Rules:**
```cpp
void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override {
    juce::ScopedNoDenormals noDenormals;  // ALWAYS first line

    // NO allocations: new, malloc, std::vector resize
    // NO string operations
    // NO locks/mutexes

    // Use pre-allocated buffers from prepareToPlay
}
```

**Pre-allocation in prepareToPlay:**
```cpp
void prepareToPlay(double sampleRate, int samplesPerBlock) override {
    // Allocate ALL buffers here
    inputFifo.resize(fftSize * numChannels);
    outputFifo.resize(fftSize * numChannels);
    fftData.resize(fftSize * 2);  // Complex interleaved
    hannWindow.resize(fftSize + 1);
    bandForBin.fill(-1);

    // Pre-compute Hann window
    for (int i = 0; i <= fftSize; ++i) {
        hannWindow[i] = 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * i / fftSize));
    }
}
```

### 4.2 Modern DSP API Pattern (from `juce8-critical-patterns.md`)

**Correct ProcessSpec Usage:**
```cpp
void prepareToPlay(double sampleRate, int samplesPerBlock) override {
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(getTotalNumOutputChannels());

    dryWetMixer.prepare(spec);
    // All juce::dsp components use this pattern
}
```

### 4.3 Threading (from `juce8-critical-patterns.md`)

**Audio Thread Constraints:**
- Parameter reads: `parameters.getRawParameterValue("id")->load()` (atomic)
- Never call UI methods from processBlock
- Use `std::atomic` for flags shared between threads

### 4.4 Latency Reporting

**Must call `setLatencySamples()` in prepareToPlay:**
```cpp
void prepareToPlay(double sampleRate, int samplesPerBlock) override {
    setLatencySamples(fftSize);  // 2048 samples
    // ... rest of initialization
}
```

---

## 5. Reference Implementation Patterns

### 5.1 O-Freeze COLA Implementation (Proven Pattern)

From `plugins/O-Freeze/Source/PluginProcessor.cpp`:
```cpp
// True Hann window scaled for COLA
// With 8 grains at 87.5% overlap (hop = N/8), Hann windows sum to 4.0
// Scale by 0.25 so overlapping grains sum to 1.0
const float colaScale = 0.25f;

for (int i = 0; i < grainSize; ++i) {
    double phase = static_cast<double>(i) / static_cast<double>(grainSize);
    float hannValue = static_cast<float>(0.5 * (1.0 - std::cos(2.0 * PI * phase)));
    hannWindow[i] = hannValue * colaScale;
}
```

**For O-FreqPulse (4× overlap):**
- Hann windows at 75% overlap sum to ~1.5
- COLA correction factor: `2.0f / 3.0f` (≈0.667)

### 5.2 audiodev.blog STFT Pattern (Proven)

**Frame Processing Structure:**
```cpp
void processFrame() {
    // 1. Copy input to FFT buffer
    for (int i = 0; i < fftSize; ++i) {
        int idx = (inputWritePos + i) % fftSize;
        fftData[i] = inputFifo[idx];
    }

    // 2. Apply analysis window
    window.multiplyWithWindowingTable(fftData.data(), fftSize);

    // 3. Forward FFT
    fft.performRealOnlyForwardTransform(fftData.data(), true);

    // 4. Spectral processing (band gains)
    for (int bin = 0; bin < numBins; ++bin) {
        float gain = calculateBinGain(bin);
        applyGainToBin(fftData.data(), bin, gain);
    }

    // 5. Inverse FFT
    fft.performRealOnlyInverseTransform(fftData.data());

    // 6. Apply synthesis window + COLA correction
    window.multiplyWithWindowingTable(fftData.data(), fftSize);
    for (int i = 0; i < fftSize; ++i) {
        fftData[i] *= windowCorrection;
    }

    // 7. Overlap-add to output buffer
    for (int i = 0; i < fftSize; ++i) {
        int idx = (outputReadPos + i) % fftSize;
        outputFifo[idx] += fftData[i];
    }
}
```

---

## 6. Implementation Checklist

### 6.1 Required DSP Components

- [ ] **STFT Processor Class** (or inline)
  - [ ] Input ring buffer (fftSize × numChannels)
  - [ ] Output ring buffer (fftSize × numChannels)
  - [ ] FFT working buffer (fftSize × 2)
  - [ ] Hann window lookup table (fftSize + 1)
  - [ ] Hop counter (trigger every 512 samples)

- [ ] **Band Processor**
  - [ ] Bin-to-band mapping array (1025 entries)
  - [ ] Band frequency parameter listener (recalculate on change)
  - [ ] Per-band gain smoothers (4× SmoothedValue)

- [ ] **Step Sequencer Engine**
  - [ ] Current step index tracking
  - [ ] PPQ to step conversion
  - [ ] Swing offset calculation
  - [ ] Transport stopped handling

- [ ] **Euclidean Generator**
  - [ ] Pattern generation function
  - [ ] Per-band pattern cache (4 × 32 bools)
  - [ ] Regenerate on parameter change

- [ ] **Output Stage**
  - [ ] DryWetMixer integration
  - [ ] Latency reporting

### 6.2 Testing Strategy

| Test | Method | Expected Result |
|------|--------|-----------------|
| FFT null test | Bypass processing, compare input/output | Silence (perfect reconstruction) |
| Phase preservation | Sine sweep through all bands | No phase artifacts |
| COLA validation | Constant tone, vary step pattern | No amplitude modulation artifacts |
| Tempo sync | Compare playhead to DAW grid | Steps align with beat markers |
| Euclidean (8,3,0) | Visual inspection | Pattern: X..X..X. |
| Smoothing | Rapid step changes | No clicks or pops |
| Latency | DAW PDC | 2048 samples reported |

---

## 7. Risk Mitigations Confirmed

### 7.1 FFT Artifacts (HIGH Risk)

**Mitigation Applied:**
- Hann window with COLA normalization
- 75% overlap (4× factor)
- Phase preservation (scale real+imag together)
- Per-band smoothing (not per-bin, for efficiency)

**Fallback:** If artifacts persist, increase to 8× overlap (87.5%)

### 7.2 CPU Performance (MEDIUM Risk)

**Optimizations Planned:**
- Pre-computed window and bin mapping
- `juce::FloatVectorOperations` for bulk gain application
- Skip inactive bands
- Single FFT for both channels (process together)

**Profile At:** 96kHz stereo (worst case)

### 7.3 Latency (MEDIUM Risk)

**Mitigation:**
- Report 2048 samples to DAW via `setLatencySamples()`
- Document in user manual
- Consider 1024-FFT "low latency mode" for v1.1

---

## 8. Sources

- [JUCE FFT Documentation](https://docs.juce.com/master/classjuce_1_1dsp_1_1FFT.html)
- [audiodev.blog FFT Processing Tutorial](https://audiodev.blog/fft-processing/)
- [JUCE AudioPlayHead](https://docs.juce.com/master/classAudioPlayHead.html)
- [CCRMA STFT Processing](https://www.dsprelated.com/freebooks/sasp/Overlap_Add_OLA_STFT_Processing.html)
- [Euclidean Rhythms - LANDR](https://blog.landr.com/euclidean-rhythms/)
- O-Freeze implementation: `plugins/O-Freeze/Source/PluginProcessor.cpp` (COLA pattern)
- Troubleshooting patterns: `troubleshooting/patterns/stage-2-patterns.md`

---

*Generated: 2026-02-03 via /plugin-research*
