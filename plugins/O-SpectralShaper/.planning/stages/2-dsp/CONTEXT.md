# Stage 2: DSP Implementation - Context

**Date:** 2026-02-03
**Participants:** User, Claude
**Plugin:** O-SpectralShaper
**Stage:** 2-dsp (Spectral Transient Shaping)

---

## Discussion Summary

Stage 2 implements the core spectral transient shaping DSP across 3 phases:
- **Phase 2.1:** Core STFT engine with overlap-add and perfect reconstruction
- **Phase 2.2:** Per-band transient detection (32 logarithmic bands, spectral flux)
- **Phase 2.3:** Envelope shaping with attack/sustain curves + lookahead

This is a COMPLEX stage (complexity score 5.0) requiring careful validation at each phase.

---

## Requirements Confirmed

### Phase 2.1: Core STFT Engine

| Requirement | Decision | Rationale |
|-------------|----------|-----------|
| FFT size | Fixed 512 samples | Simpler implementation; 5.3ms @ 96kHz still acceptable |
| Overlap | 50% (256-sample hop) | COLA with Hann window for perfect reconstruction |
| Window function | Hann | Industry standard, good spectral leakage suppression |
| Bypass mode | Yes - include null-test | Verify perfect reconstruction before adding transient logic |
| Sample-by-sample interface | Yes | Block-size independent processing via FIFO |

### Phase 2.2: Per-Band Transient Detection

| Requirement | Decision | Rationale |
|-------------|----------|-----------|
| Band count | 32 logarithmic bands | 20Hz-20kHz, ~3 bands/octave |
| Detection algorithm | Spectral flux + dual envelopes | Fast (1ms) and slow (15ms) envelope followers |
| Positive-only flux | Yes | Energy increases = transients, decreases = decays |
| Debug output | Console logging (DBG) | Log transient activities periodically for validation |
| Per-channel | Independent L/R | Not mid/side - preserves stereo transients |

### Phase 2.3: Envelope Shaping

| Requirement | Decision | Rationale |
|-------------|----------|-----------|
| Curve storage | 2x std::array<float, 32> | Attack and sustain curves, -1.0 to +1.0 per band |
| Thread sync | Double-buffering with atomic swap | Lock-free, no audio glitches during curve drawing |
| Lookahead | Implement with LOOKAHEAD_ENABLED toggle | Per ARCHITECTURE.md, 0-10ms configurable |
| Dry path | Latency-matched (512 samples) | Prevents comb filtering at partial mix settings |
| Gain smoothing | SmoothedValue per band | 50ms ramp to prevent zipper noise |

---

## Constraints Identified

1. **Real-time safety:** No allocation, locking, or blocking in processBlock()
2. **CPU budget:** Target <50% single core @ 44.1kHz stereo
3. **Latency:** Fixed 512 samples (already reported in Stage 1)
4. **Phase coherence:** Magnitude-only processing, preserve phase
5. **Denormal handling:** ScopedNoDenormals in processBlock()

---

## Approach Decisions

### STFT Processor Class Design

```cpp
class STFTProcessor {
    juce::dsp::FFT forwardFFT { FFT_ORDER };  // 512-point
    juce::dsp::FFT inverseFFT { FFT_ORDER };
    juce::dsp::WindowingFunction<float> window { FFT_SIZE, hann };

    std::array<float, FFT_SIZE> inputFIFO;
    std::array<float, FFT_SIZE> outputFIFO;
    std::array<float, FFT_SIZE * 2> fftData;  // Interleaved real/imag
    int fifoIndex = 0;

    float processSample(float input);  // Sample-by-sample interface
    void processFrame();               // Called every HOP_SIZE samples
};
```

### Band Structure Design

```cpp
struct Band {
    float prevMagnitude = 0.0f;      // Previous frame magnitude
    float fastEnvelope = 0.0f;       // 1ms attack envelope
    float slowEnvelope = 0.0f;       // 15ms attack envelope
    float transientActivity = 0.0f;  // 0.0-1.0 strength
};

std::array<Band, 32> bands;
std::array<int, 33> bandBoundaries;  // Logarithmic bin mapping
```

### Curve Double-Buffering Design

```cpp
std::array<float, NUM_BANDS> attackCurve[2];   // Ping-pong buffers
std::array<float, NUM_BANDS> sustainCurve[2];
std::atomic<int> activeBufferIndex { 0 };

// GUI thread: write to inactive buffer, then atomic swap
// Audio thread: read from active buffer
```

### Lookahead Buffer Design

```cpp
juce::AudioBuffer<float> lookaheadBuffer;
int lookaheadWritePos = 0;
int lookaheadSamples = 0;  // Calculated from LOOKAHEAD_TIME parameter

// Only active when LOOKAHEAD_ENABLED is true
// Adds delay before STFT input for clean attack capture
```

---

## Test Criteria (from ROADMAP.md)

### Phase 2.1 - STFT Engine
- [ ] Audio passes through without artifacts
- [ ] Null test: Input - Output = silence (perfect reconstruction)
- [ ] No phase distortion (mono sum test)
- [ ] Latency compensation works in DAW

### Phase 2.2 - Transient Detection
- [ ] Impulse input triggers transient detection in all bands
- [ ] Sine wave shows low transient activity (sustain only)
- [ ] Drum loop shows high activity at hits, low between
- [ ] Sensitivity parameter modulates threshold

### Phase 2.3 - Envelope Shaping
- [ ] attackCurve[all] = +1.0 → Transients boosted
- [ ] sustainCurve[all] = -1.0 → Tails reduced
- [ ] Mix parameter blends smoothly (no clicks)
- [ ] Lookahead reduces pre-ringing on sharp transients

---

## Open Questions

None - all key decisions resolved in discussion.

---

## JUCE Dependencies

| Class | Module | Purpose |
|-------|--------|---------|
| `juce::dsp::FFT` | juce_dsp | Forward/inverse FFT |
| `juce::dsp::WindowingFunction<float>` | juce_dsp | Hann window |
| `juce::dsp::DryWetMixer<float>` | juce_dsp | Mix control with latency matching |
| `juce::SmoothedValue<float>` | juce_core | Gain ramping |
| `juce::Decibels` | juce_audio_basics | dB↔linear conversion |
| `juce::FloatVectorOperations` | juce_audio_basics | SIMD overlap-add |

---

## Files to Create/Modify

### New Files
- `Source/STFTProcessor.h` - Overlap-add STFT class
- `Source/STFTProcessor.cpp` - Implementation
- `Source/TransientDetector.h` - Per-band detection (optional, may inline)

### Modified Files
- `Source/PluginProcessor.h` - Add STFT instances, curve arrays
- `Source/PluginProcessor.cpp` - Implement processBlock() with full DSP chain

---

## Next Phase

**Ready for:** RESEARCH phase

Run `/plugin-research O-SpectralShaper 2-dsp` to investigate JUCE FFT APIs and gather implementation patterns.

---

*Context captured: 2026-02-03*
