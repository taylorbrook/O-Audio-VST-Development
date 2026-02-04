# Stage 2: DSP Implementation - Execution Plan

**Date:** 2026-02-03
**Plugin:** O-SpectralShaper
**Stage:** 2-dsp (Spectral Transient Shaping)
**Complexity:** COMPLEX (3 phases, sequential execution)

---

## Goal

Implement the complete spectral transient shaping DSP engine: overlap-add STFT processing with perfect reconstruction, 32-band logarithmic transient detection using spectral flux with dual envelopes, and per-band envelope shaping with attack/sustain curves.

---

## Phase Breakdown

### Phase 2.1: Core STFT Engine

**Objective:** Sample-by-sample overlap-add FFT processing with perfect reconstruction

### Phase 2.2: Per-Band Transient Detection

**Objective:** Detect transients independently in 32 logarithmic frequency bands

### Phase 2.3: Envelope Shaping & Parameters

**Objective:** Apply attack/sustain curves to shape transients per band

---

## Task Breakdown

### Phase 2.1 Tasks (Core STFT Engine)

| # | Task | Files | Depends On |
|---|------|-------|------------|
| 1 | Create STFTProcessor class header | Source/STFTProcessor.h | - |
| 2 | Implement STFTProcessor with FFT setup | Source/STFTProcessor.cpp | Task 1 |
| 3 | Implement input/output FIFOs and sample-by-sample interface | Source/STFTProcessor.cpp | Task 2 |
| 4 | Implement processFrame() with overlap-add | Source/STFTProcessor.cpp | Task 3 |
| 5 | Add spectral processing callback placeholder | Source/STFTProcessor.cpp | Task 4 |
| 6 | Add STFTProcessor instances to PluginProcessor | Source/PluginProcessor.h | Task 1 |
| 7 | Initialize STFTProcessor in prepareToPlay() | Source/PluginProcessor.cpp | Task 6 |
| 8 | Integrate STFT into processBlock() | Source/PluginProcessor.cpp | Tasks 4, 7 |
| 9 | Add bypass mode for null-test verification | Source/PluginProcessor.cpp | Task 8 |

**Phase 2.1 Deliverables:**
- `Source/STFTProcessor.h` - Class declaration
- `Source/STFTProcessor.cpp` - Implementation with perfect reconstruction
- Modified `PluginProcessor.h/cpp` - STFT integration

---

### Phase 2.2 Tasks (Per-Band Transient Detection)

| # | Task | Files | Depends On |
|---|------|-------|------------|
| 10 | Add Band struct and band boundary arrays | Source/STFTProcessor.h | Task 9 |
| 11 | Implement setupBandBoundaries() for 32 log bands | Source/STFTProcessor.cpp | Task 10 |
| 12 | Implement calculateBandMagnitudes() | Source/STFTProcessor.cpp | Task 11 |
| 13 | Implement spectral flux detection (positive-only) | Source/STFTProcessor.cpp | Task 12 |
| 14 | Implement dual envelope followers (fast 1ms, slow 15ms) | Source/STFTProcessor.cpp | Task 13 |
| 15 | Calculate transient activity per band | Source/STFTProcessor.cpp | Task 14 |
| 16 | Add sensitivity parameter integration | Source/PluginProcessor.cpp | Task 15 |
| 17 | Add debug logging for transient detection | Source/STFTProcessor.cpp | Task 15 |

**Phase 2.2 Deliverables:**
- 32-band logarithmic frequency splitting
- Per-band transient detection with spectral flux
- Dual envelope followers for attack/sustain separation
- Sensitivity parameter modulation

---

### Phase 2.3 Tasks (Envelope Shaping & Parameters)

| # | Task | Files | Depends On |
|---|------|-------|------------|
| 18 | Add attack/sustain curve arrays with double-buffering | Source/PluginProcessor.h | Task 17 |
| 19 | Implement thread-safe curve setters/getters | Source/PluginProcessor.cpp | Task 18 |
| 20 | Implement per-band gain calculation | Source/STFTProcessor.cpp | Tasks 15, 19 |
| 21 | Apply gain to FFT bins (magnitude-only, preserve phase) | Source/STFTProcessor.cpp | Task 20 |
| 22 | Add SmoothedValue for gain ramping (50ms) | Source/STFTProcessor.cpp | Task 21 |
| 23 | Add dry delay buffer for latency matching | Source/PluginProcessor.h/cpp | Task 18 |
| 24 | Implement dry/wet mixing with matched latency | Source/PluginProcessor.cpp | Tasks 8, 23 |
| 25 | Add output gain with smoothing | Source/PluginProcessor.cpp | Task 24 |
| 26 | Implement lookahead buffer (optional toggle) | Source/PluginProcessor.cpp | Task 24 |
| 27 | Connect Attack Time, Sustain Time parameters | Source/PluginProcessor.cpp | Task 20 |
| 28 | Update state save/load for curve arrays | Source/PluginProcessor.cpp | Task 18 |

**Phase 2.3 Deliverables:**
- Attack/sustain curve arrays with thread-safe sync
- Per-band envelope shaping
- Latency-matched dry/wet mixing
- Lookahead buffer (toggleable)
- Full parameter integration

---

## File Structure

### New Files

```
Source/
├── STFTProcessor.h      # STFT class declaration
└── STFTProcessor.cpp    # STFT implementation
```

### Modified Files

```
Source/
├── PluginProcessor.h    # Add STFT instances, curves, buffers
└── PluginProcessor.cpp  # Implement DSP chain in processBlock()
```

---

## Detailed Task Specifications

### Task 1: Create STFTProcessor Header

```cpp
// Source/STFTProcessor.h
#pragma once
#include <juce_dsp/juce_dsp.h>
#include <array>

class STFTProcessor {
public:
    static constexpr int FFT_ORDER = 9;
    static constexpr int FFT_SIZE = 512;
    static constexpr int HOP_SIZE = 256;
    static constexpr int NUM_BINS = FFT_SIZE / 2 + 1;  // 257
    static constexpr int NUM_BANDS = 32;

    void prepare(double sampleRate);
    void reset();
    float processSample(float input);

    // Transient detection output (read by processor)
    const std::array<float, NUM_BANDS>& getTransientActivity() const;

    // Curve input (set by processor)
    void setAttackCurve(const std::array<float, NUM_BANDS>& curve);
    void setSustainCurve(const std::array<float, NUM_BANDS>& curve);
    void setAttackTime(float ms);
    void setSustainTime(float ms);
    void setSensitivity(float value);

private:
    // FFT engine
    juce::dsp::FFT forwardFFT { FFT_ORDER };
    juce::dsp::FFT inverseFFT { FFT_ORDER };
    juce::dsp::WindowingFunction<float> window {
        FFT_SIZE, juce::dsp::WindowingFunction<float>::hann, false
    };

    // FIFOs
    std::array<float, FFT_SIZE> inputFIFO {};
    std::array<float, FFT_SIZE> outputFIFO {};
    std::array<float, FFT_SIZE * 2> fftData {};
    int fifoIndex = 0;

    // Band structure
    struct BandBoundary { int startBin; int endBin; };
    std::array<BandBoundary, NUM_BANDS> bandBoundaries;

    struct Band {
        float prevMagnitude = 0.0f;
        float fastEnvelope = 0.0f;
        float slowEnvelope = 0.0f;
        float transientActivity = 0.0f;
        juce::SmoothedValue<float> gainSmoothed;
    };
    std::array<Band, NUM_BANDS> bands;

    // Parameters
    float sensitivity = 0.5f;
    float attackTimeMs = 10.0f;
    float sustainTimeMs = 100.0f;
    float hopTime = 0.0f;  // HOP_SIZE / sampleRate
    float fastCoeff = 0.0f, slowCoeff = 0.0f, releaseCoeff = 0.0f;

    // Curves (double-buffered, atomic swap)
    std::array<float, NUM_BANDS> attackCurve[2] {};
    std::array<float, NUM_BANDS> sustainCurve[2] {};
    std::atomic<int> activeCurveBuffer { 0 };

    // Processing methods
    void processFrame();
    void setupBandBoundaries(double sampleRate);
    void detectTransients();
    void applyEnvelopeShaping();
};
```

### Task 8: processBlock Integration

```cpp
void OSpectralShaperAudioProcessor::processBlock(AudioBuffer<float>& buffer, MidiBuffer&)
{
    ScopedNoDenormals noDenormals;

    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    // Read parameters
    float mixValue = parameters.getRawParameterValue("MIX")->load();
    float sensitivity = parameters.getRawParameterValue("SENSITIVITY")->load();
    float attackTime = parameters.getRawParameterValue("ATTACK_TIME")->load();
    float sustainTime = parameters.getRawParameterValue("SUSTAIN_TIME")->load();
    float outputGainDB = parameters.getRawParameterValue("OUTPUT_GAIN")->load();
    float outputGain = Decibels::decibelsToGain(outputGainDB);

    // Update STFT parameters
    for (int ch = 0; ch < numChannels; ++ch) {
        stftProcessor[ch].setSensitivity(sensitivity);
        stftProcessor[ch].setAttackTime(attackTime);
        stftProcessor[ch].setSustainTime(sustainTime);
        stftProcessor[ch].setAttackCurve(getAttackCurve());
        stftProcessor[ch].setSustainCurve(getSustainCurve());
    }

    // Process sample-by-sample
    for (int sample = 0; sample < numSamples; ++sample) {
        for (int ch = 0; ch < numChannels; ++ch) {
            float input = buffer.getSample(ch, sample);

            // Dry path (latency-matched)
            float dry = getDryDelayedSample(ch, input);

            // Wet path (STFT processing)
            float wet = stftProcessor[ch].processSample(input);

            // Mix and output gain
            float output = (dry * (1.0f - mixValue) + wet * mixValue) * outputGain;
            buffer.setSample(ch, sample, output);
        }
        advanceDryDelay();
    }
}
```

### Task 20: Per-Band Gain Calculation

```cpp
void STFTProcessor::applyEnvelopeShaping()
{
    const auto& attackCurveData = attackCurve[activeCurveBuffer.load()];
    const auto& sustainCurveData = sustainCurve[activeCurveBuffer.load()];

    for (int band = 0; band < NUM_BANDS; ++band) {
        float transient = bands[band].transientActivity;  // 0.0-1.0

        // Attack gain: boost/cut transients
        float attackDB = attackCurveData[band] * attackTimeMs * 0.1f;
        float attackGain = juce::Decibels::decibelsToGain(attackDB * transient);

        // Sustain gain: boost/cut non-transient portions
        float sustainDB = sustainCurveData[band] * sustainTimeMs * 0.01f;
        float sustainGain = juce::Decibels::decibelsToGain(sustainDB * (1.0f - transient));

        // Combined target gain
        float targetGain = attackGain * sustainGain;

        // Smooth gain changes (50ms ramp)
        bands[band].gainSmoothed.setTargetValue(targetGain);
        float smoothedGain = bands[band].gainSmoothed.getNextValue();

        // Apply to FFT bins in this band (preserve phase)
        for (int bin = bandBoundaries[band].startBin; bin < bandBoundaries[band].endBin; ++bin) {
            fftData[bin * 2] *= smoothedGain;      // Real
            fftData[bin * 2 + 1] *= smoothedGain;  // Imaginary
        }
    }
}
```

---

## Success Criteria

### Phase 2.1 - Core STFT Engine
- [ ] Audio passes through without artifacts
- [ ] Null test: Input - Output = silence (perfect reconstruction)
- [ ] No phase distortion (mono sum test)
- [ ] Latency compensation works in DAW (512 samples reported)

### Phase 2.2 - Transient Detection
- [ ] Impulse input triggers transient detection in all bands
- [ ] Sine wave shows low transient activity (sustain only)
- [ ] Drum loop shows high activity at hits, low between
- [ ] Sensitivity parameter modulates threshold
- [ ] Debug output shows transient activities

### Phase 2.3 - Envelope Shaping
- [ ] attackCurve[all] = +1.0 → Transients audibly boosted
- [ ] sustainCurve[all] = -1.0 → Tails audibly reduced
- [ ] Mix parameter blends smoothly (no clicks)
- [ ] Lookahead reduces pre-ringing on sharp transients
- [ ] Output gain compensates for level changes
- [ ] Curves persist across preset save/load
- [ ] No audio glitches during curve updates

---

## Git Commits

| Phase | Commit Message |
|-------|----------------|
| 2.1 | `feat(O-SpectralShaper): Phase 2.1 - Core STFT engine with perfect reconstruction` |
| 2.2 | `feat(O-SpectralShaper): Phase 2.2 - Per-band transient detection with spectral flux` |
| 2.3 | `feat(O-SpectralShaper): Phase 2.3 - Envelope shaping with attack/sustain curves` |

---

## Dependencies

### JUCE Classes Required

| Class | Module | Purpose |
|-------|--------|---------|
| `juce::dsp::FFT` | juce_dsp | Forward/inverse transforms |
| `juce::dsp::WindowingFunction<float>` | juce_dsp | Hann window |
| `juce::SmoothedValue<float>` | juce_core | Gain smoothing |
| `juce::Decibels` | juce_audio_basics | dB ↔ linear |
| `juce::FloatVectorOperations` | juce_audio_basics | SIMD overlap-add |
| `juce::ScopedNoDenormals` | juce_audio_basics | CPU protection |

### Existing Code (Stage 1)

- `PluginProcessor.h/cpp` - 7 APVTS parameters already defined
- `PluginEditor.h/cpp` - WebView with parameter relays
- 512-sample latency already reported

---

## Constraints

1. **Real-time safety:** No allocation, locking, or blocking in processBlock()
2. **CPU budget:** Target <50% single core @ 44.1kHz stereo
3. **Latency:** Fixed 512 samples (already reported)
4. **Phase coherence:** Magnitude-only processing, preserve phase
5. **Thread safety:** Double-buffering for curve updates
6. **Denormal handling:** ScopedNoDenormals in processBlock()

---

## Estimated Effort

| Phase | Tasks | Complexity | Estimate |
|-------|-------|------------|----------|
| 2.1 | 9 | Medium | Core STFT |
| 2.2 | 8 | Medium | Detection |
| 2.3 | 11 | High | Full integration |
| **Total** | **28** | **High** | **3 phases** |

---

## Next Steps

Run `/plugin-execute O-SpectralShaper 2-dsp` to begin implementation.

The executor will:
1. Complete Phase 2.1 tasks sequentially
2. Validate null-test passes
3. Git commit Phase 2.1
4. Complete Phase 2.2 tasks
5. Validate transient detection
6. Git commit Phase 2.2
7. Complete Phase 2.3 tasks
8. Validate shaping works
9. Git commit Phase 2.3

---

*Plan created: 2026-02-03*
