---
title: "DSP Click Prevention and Debugging Guide"
created: 2026-02-04
last_verified: 2026-02-06
juce_version: "8.0.4"
summary: "Reference for understanding, preventing, and debugging audio clicks and pops in JUCE audio plugins, covering signal discontinuities, buffer boundary issues, parameter smoothing, and JUCE-specific patterns with diagnostic techniques."
domain: dsp
type: research
keywords:
  - clicks
  - pops
  - audio-artifacts
  - parameter-smoothing
  - buffer-boundaries
  - debugging
  - discontinuities
  - juce-dsp
stages: [1, 2, 3, 4]
agents: [dsp, build]
---

# DSP Click Prevention and Debugging Guide

A comprehensive reference for understanding, preventing, and debugging audio clicks and pops in JUCE audio plugins.

**Created:** 2026-02-04
**Version:** 1.0
**Purpose:** Reference document for all plugin development agents

---

## Table of Contents

1. [Root Causes of Clicks](#root-causes-of-clicks)
2. [Prevention Patterns](#prevention-patterns)
3. [JUCE-Specific Patterns](#juce-specific-patterns)
4. [Debugging Clicks](#debugging-clicks)
5. [Code Examples](#code-examples)
6. [Quick Reference](#quick-reference)

---

## Root Causes of Clicks

### 1. Signal Discontinuities

**The Fundamental Principle:** A click is an abrupt step change in signal level. Even stopping at a zero crossing can click if the transition is instantaneous because the derivative (rate of change) is discontinuous.

```
Continuous signal:    ~~~~∿~~~~
                           ↓
Discontinuity:        ~~~~|____  ← Click!

Even at zero:         ~~∿~~|     ← Still clicks (derivative discontinuity)
```

**Why it sounds:** The speaker cone snaps from one position to another, creating a broadband impulse.

### 2. Buffer Boundary Issues

| Issue | Cause | Symptom |
|-------|-------|---------|
| Frame boundary clicks | FFT/processing frame not properly overlap-added | Periodic clicking at frame rate |
| Buffer underrun | Processing takes too long | Sporadic clicks under load |
| Circular buffer wraparound | Incorrect index calculation | Click at buffer wrap point |
| Buffer size mismatch | Assuming fixed buffer size | Click when DAW changes buffer |

### 3. Parameter Change Artifacts

**Zipper Noise:** Stepping through discrete parameter values creates staircase waveform modulation.

```
Without smoothing:
Gain: 1.0 → 0.8 → 0.6 → 0.4  (steps = clicks)
       │     │     │     │
       ▼     ▼     ▼     ▼
Audio: ████  ███   ██    █   (staircase = zipper noise)

With smoothing:
Gain: 1.0 ╲ 0.8 ╲ 0.6 ╲ 0.4  (smooth ramp)
           ╲    ╲    ╲
Audio:      ████████████     (clean fade)
```

### 4. Sample Rate Mismatches

- Processing at wrong sample rate
- Filter coefficients not recalculated after sample rate change
- Delay times not adjusted for sample rate
- Smoothing times inconsistent across sample rates

### 5. Threading/Real-Time Safety Violations

**CRITICAL:** These operations are FORBIDDEN on the audio thread:

| Operation | Why It Clicks | Alternative |
|-----------|---------------|-------------|
| `new` / `malloc` | OS may block, causing dropout | Pre-allocate in `prepareToPlay()` |
| `mutex.lock()` | Priority inversion, blocking | Use `std::atomic` or lock-free queues |
| File I/O | Disk access is unpredictable | Load in background thread |
| System calls | Kernel may block | Avoid entirely |
| `std::vector::push_back()` | May reallocate | Pre-reserve or use fixed arrays |

### 6. Improper Gain Staging

- Output exceeding [-1.0, 1.0] causes hard clipping
- Feedback loops accumulating to infinity
- DC offset causing asymmetric clipping
- Mixing multiple sources without level compensation

### 7. Filter Coefficient Changes

Changing IIR filter coefficients during processing causes transients because:
- Internal filter state becomes invalid for new coefficients
- Sudden coefficient changes create discontinuities in filter response

### 8. Delay Line Issues

| Issue | Cause | Solution |
|-------|-------|----------|
| Click on delay change | Read position jumps | Interpolate delay time |
| Click on freeze toggle | Loop boundary discontinuity | Crossfade at boundaries |
| Modulation clicks | Large delay changes | Limit modulation depth, use proper interpolation |

---

## Prevention Patterns

### Fade Times and Crossfading

#### Minimum Fade Durations

| Use Case | Minimum | Recommended | Maximum | Notes |
|----------|---------|-------------|---------|-------|
| Click prevention | 1 ms | 2-3 ms | 5 ms | Inaudible transition |
| Bypass switching | 5 ms | 10 ms | 25 ms | May hear if too short |
| Crossfade audio | 10 ms | 25 ms | 50 ms | Depends on material |
| Gain smoothing | 10 ms | 20-50 ms | 100 ms | Balance response vs. smoothness |
| Filter frequency | 20 ms | 50 ms | 100 ms | Prevent zipper noise |

**Converting to Samples:**
```cpp
int samplesToFade = static_cast<int>(fadeTimeMs * 0.001f * sampleRate);
// Example: 10ms at 44.1kHz = 441 samples
```

#### Crossfade Curve Types

**1. Linear Crossfade**
```cpp
// Simple but has -6dB dip at midpoint (uncorrelated signals)
float fadeIn = position;  // 0.0 to 1.0
float fadeOut = 1.0f - position;
output = signalA * fadeOut + signalB * fadeIn;
```
- **When to use:** Correlated signals, simple transitions
- **Drawback:** -6dB dip at midpoint for uncorrelated signals

**2. Equal Power Crossfade (Sine/Cosine)**
```cpp
// Maintains constant power for uncorrelated signals
float angle = position * M_PI * 0.5f;  // 0 to PI/2
float fadeIn = std::sin(angle);
float fadeOut = std::cos(angle);
output = signalA * fadeOut + signalB * fadeIn;
```
- **When to use:** Uncorrelated signals (different audio sources)
- **Why:** sin^2 + cos^2 = 1, maintaining constant power
- **Midpoint gain:** 0.707 (-3dB) for each signal, summing to unity

**3. S-Curve Crossfade**
```cpp
// Smooth ease-in/ease-out using cosine
float fadeIn = 0.5f * (1.0f - std::cos(position * M_PI));
float fadeOut = 0.5f * (1.0f + std::cos(position * M_PI));
output = signalA * fadeOut + signalB * fadeIn;
```
- **When to use:** UI-triggered transitions, bypass switching
- **Benefit:** Smooth acceleration/deceleration at endpoints

**4. Cheap Energy-Preserving Approximation**
```cpp
// From Signalsmith Audio - avoids expensive trig
float x2 = position * position;
float x3 = x2 * position;
float fadeIn = 3.0f * x2 - 2.0f * x3;  // Hermite smoothstep
float fadeOut = 1.0f - fadeIn;
// Scale for energy preservation
float scale = 1.0f / std::sqrt(fadeIn * fadeIn + fadeOut * fadeOut);
output = (signalA * fadeOut + signalB * fadeIn) * scale;
```

#### Buffer-Aligned vs Sample-Accurate Fading

**Buffer-aligned:** Apply fade over entire buffer, simpler but less precise
```cpp
void processBlock(AudioBuffer<float>& buffer) {
    if (fadeActive) {
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            float fadeGain = calculateFade(fadePosition);
            buffer.applyGain(i, 1, fadeGain);
            fadePosition += fadeIncrement;
        }
    }
}
```

**Sample-accurate:** Fade starts at exact parameter change, requires event handling
```cpp
void processBlock(AudioBuffer<float>& buffer, MidiBuffer& midi) {
    // Handle parameter changes at exact sample positions
    for (const auto& event : parameterEvents) {
        processUntil(event.samplePosition);
        startFade(event.newValue);
    }
    processRemaining();
}
```

### Windowing Functions

#### When to Apply Windowing

1. **Before FFT:** Always (prevents spectral leakage)
2. **After IFFT:** Always for WOLA (weighted overlap-add)
3. **Granular synthesis:** Each grain needs windowing
4. **Wavetable edges:** Smooth wavetable boundaries

#### Window Types and Selection

| Window | Side Lobe | Main Lobe | Best For |
|--------|-----------|-----------|----------|
| Hann | -31 dB | 4 bins | General purpose, FFT |
| Hamming | -42 dB | 4 bins | Speech, narrow-band |
| Blackman | -58 dB | 6 bins | High dynamic range |
| Blackman-Harris | -92 dB | 8 bins | Maximum dynamic range |
| Tukey (tapered cosine) | Variable | Variable | Granular, adjustable |

**Rule of Thumb:** Use Hann for most audio applications. It satisfies COLA at 50% overlap.

#### Overlap-Add Requirements

For click-free reconstruction, window must satisfy COLA (Constant Overlap-Add):

| Window | 50% Overlap | 75% Overlap |
|--------|-------------|-------------|
| Hann | Yes (sum=1) | Yes (sum=1.5) |
| Hamming | Approximate | Approximate |
| Triangular | Yes (sum=1) | No |

```cpp
// Hann window creation in JUCE
juce::dsp::WindowingFunction<float> window(
    fftSize,
    juce::dsp::WindowingFunction<float>::hann,
    false  // not normalized
);

// Apply before FFT
window.multiplyWithWindowingTable(fftData.data(), fftSize);
```

### Parameter Smoothing

#### juce::SmoothedValue Usage

```cpp
class MyProcessor : public AudioProcessor {
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> gainSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> freqSmoothed;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override {
        // Linear smoothing: good for gain (linear scale)
        gainSmoothed.reset(sampleRate, 0.020);  // 20ms ramp time

        // Multiplicative smoothing: good for frequency (logarithmic)
        freqSmoothed.reset(sampleRate, 0.050);  // 50ms ramp time
    }

    void processBlock(AudioBuffer<float>& buffer, MidiBuffer&) override {
        // Update target from parameter
        gainSmoothed.setTargetValue(*gainParameter);
        freqSmoothed.setTargetValue(*freqParameter);

        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            float gain = gainSmoothed.getNextValue();
            float freq = freqSmoothed.getNextValue();
            // Use smoothed values...
        }
    }
};
```

#### Smoothing Type Selection

| Parameter Type | Smoothing Type | Typical Time |
|----------------|----------------|--------------|
| Gain (linear) | Linear | 10-50 ms |
| Gain (dB) | Multiplicative | 10-50 ms |
| Frequency | Multiplicative | 20-100 ms |
| Pan | Linear | 10-20 ms |
| Filter Q | Linear | 20-50 ms |
| Delay time | Linear | 50-200 ms |
| Mix/Blend | Linear | 10-30 ms |

**IMPORTANT:** Multiplicative smoothing cannot reach zero! For parameters that need to reach zero, use linear smoothing or a hybrid approach.

#### One-Pole Filter Alternative

More efficient than SmoothedValue for simple cases:

```cpp
class OnePoleSmoother {
public:
    OnePoleSmoother(float smoothingTimeMs, float sampleRate) {
        const float twoPi = 6.283185307179586476925286766559f;
        a = std::exp(-twoPi / (smoothingTimeMs * 0.001f * sampleRate));
        b = 1.0f - a;
        z = 0.0f;
    }

    float process(float input) {
        z = (input * b) + (z * a);
        return z;
    }

    void setTarget(float target) {
        // One-pole naturally smooths toward target
    }

private:
    float a, b, z;
};
```

**Coefficient formula:** `a = exp(-2*pi*fc/sampleRate)` where fc is cutoff frequency

| Smoothing Time | Coefficient (a) at 44.1kHz |
|----------------|---------------------------|
| 1 ms | 0.8607 |
| 5 ms | 0.9718 |
| 10 ms | 0.9858 |
| 20 ms | 0.9929 |
| 50 ms | 0.9972 |

#### When NOT to Smooth

- **Discrete/boolean parameters:** Bypass on/off (use crossfade instead)
- **Note triggers:** MIDI note on/off
- **Tempo sync changes:** Quantized to beat boundaries
- **Sample selection:** Wavetable index (use crossfade)

### Buffer Management

#### Proper prepareToPlay() Initialization

```cpp
void prepareToPlay(double sampleRate, int samplesPerBlock) override {
    currentSampleRate = sampleRate;

    // Reset all SmoothedValues with new sample rate
    gainSmoothed.reset(sampleRate, 0.020);

    // Resize buffers - samplesPerBlock is a HINT, not guaranteed!
    // Always allocate extra to handle larger blocks
    int safeSize = samplesPerBlock * 2;  // 2x safety margin
    internalBuffer.setSize(2, safeSize);

    // Reset delay lines
    delayLine.reset();
    delayLine.prepare({ sampleRate, (uint32)samplesPerBlock, 2 });

    // Reset filters
    filter.reset();
    filter.prepare({ sampleRate, (uint32)samplesPerBlock, 2 });

    // Recalculate sample-rate-dependent values
    updateFilterCoefficients();

    // Report latency to host
    setLatencySamples(calculateLatency());
}
```

#### Handling Buffer Size Changes

**CRITICAL:** `samplesPerBlock` is only a hint. Your code MUST handle:
- Blocks larger than expected
- Blocks smaller than expected
- Zero-sample blocks
- Variable block sizes

```cpp
void processBlock(AudioBuffer<float>& buffer, MidiBuffer&) override {
    int numSamples = buffer.getNumSamples();

    // Handle zero-sample blocks
    if (numSamples == 0)
        return;

    // Resize internal buffer if needed (but avoid in audio thread!)
    if (numSamples > internalBuffer.getNumSamples()) {
        // Log warning - this shouldn't happen in production
        jassertfalse;  // Debug: block larger than prepareToPlay hint
        return;  // Safe fallback: skip processing
    }

    // Process...
}
```

#### Circular Buffer Best Practices

```cpp
class SafeCircularBuffer {
    std::vector<float> buffer;
    int writePos = 0;
    int bufferSize;

public:
    void setSize(int maxDelaySamples) {
        // Power-of-2 for fast modulo (bitwise AND)
        bufferSize = juce::nextPowerOfTwo(maxDelaySamples + 1);
        buffer.resize(bufferSize, 0.0f);
        writePos = 0;
    }

    void push(float sample) {
        buffer[writePos] = sample;
        writePos = (writePos + 1) & (bufferSize - 1);  // Fast modulo
    }

    float read(int delaySamples) const {
        jassert(delaySamples >= 0 && delaySamples < bufferSize);
        int readPos = (writePos - delaySamples + bufferSize) & (bufferSize - 1);
        return buffer[readPos];
    }

    // Fractional delay with linear interpolation
    float readFractional(float delaySamples) const {
        int delayInt = static_cast<int>(delaySamples);
        float frac = delaySamples - delayInt;

        float y0 = read(delayInt);
        float y1 = read(delayInt + 1);

        return y0 + frac * (y1 - y0);  // Linear interpolation
    }

    void clear() {
        std::fill(buffer.begin(), buffer.end(), 0.0f);
    }
};
```

### Gain Changes

#### Never Apply Instant Gain Changes

```cpp
// WRONG - causes clicks
void processBlock(AudioBuffer<float>& buffer, MidiBuffer&) {
    float gain = *gainParameter;  // Instant change!
    buffer.applyGain(gain);
}

// CORRECT - use smoothing
void processBlock(AudioBuffer<float>& buffer, MidiBuffer&) {
    gainSmoothed.setTargetValue(*gainParameter);

    if (gainSmoothed.isSmoothing()) {
        // Per-sample smoothing
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            auto* channelData = buffer.getWritePointer(ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                channelData[i] *= gainSmoothed.getNextValue();
            }
        }
    } else {
        // No smoothing needed - apply block gain
        buffer.applyGain(gainSmoothed.getCurrentValue());
    }
}
```

#### Mute/Bypass Implementation

```cpp
class SafeBypass {
    enum class State { Active, FadingOut, Bypassed, FadingIn };
    State state = State::Active;
    float fadePosition = 1.0f;
    float fadeIncrement;

public:
    void prepare(double sampleRate) {
        // 10ms fade time
        fadeIncrement = 1.0f / (0.010f * sampleRate);
    }

    void setBypass(bool shouldBypass) {
        if (shouldBypass && state == State::Active)
            state = State::FadingOut;
        else if (!shouldBypass && state == State::Bypassed)
            state = State::FadingIn;
    }

    void process(AudioBuffer<float>& wet, const AudioBuffer<float>& dry) {
        for (int i = 0; i < wet.getNumSamples(); ++i) {
            // Update fade
            switch (state) {
                case State::FadingOut:
                    fadePosition -= fadeIncrement;
                    if (fadePosition <= 0.0f) {
                        fadePosition = 0.0f;
                        state = State::Bypassed;
                    }
                    break;
                case State::FadingIn:
                    fadePosition += fadeIncrement;
                    if (fadePosition >= 1.0f) {
                        fadePosition = 1.0f;
                        state = State::Active;
                    }
                    break;
                default:
                    break;
            }

            // Apply crossfade
            for (int ch = 0; ch < wet.getNumChannels(); ++ch) {
                float wetSample = wet.getSample(ch, i);
                float drySample = dry.getSample(ch, i);
                wet.setSample(ch, i, drySample + fadePosition * (wetSample - drySample));
            }
        }
    }
};
```

#### Output Limiting/Protection

```cpp
// Soft clipper - prevents hard clipping artifacts
inline float softClip(float x) {
    // Attempt to keep output in [-1, 1] with soft knee
    if (x > 1.0f)
        return 1.0f - std::exp(1.0f - x);  // Approaches 1.0 asymptotically
    else if (x < -1.0f)
        return -1.0f + std::exp(1.0f + x);
    return x;
}

// Alternative: tanh soft clip
inline float tanhSoftClip(float x, float drive = 1.0f) {
    return std::tanh(x * drive) / std::tanh(drive);
}

// DC blocker - prevents DC offset buildup
class DCBlocker {
    float x1 = 0.0f, y1 = 0.0f;
    float R;  // Controls cutoff frequency

public:
    DCBlocker(float cutoffHz = 5.0f, float sampleRate = 44100.0f) {
        R = 1.0f - (M_PI * 2.0f * cutoffHz / sampleRate);
    }

    float process(float x) {
        float y = x - x1 + R * y1;
        x1 = x;
        y1 = y;
        return y;
    }
};
```

---

## JUCE-Specific Patterns

### juce::dsp::Gain with Smoothing

```cpp
class GainProcessor {
    juce::dsp::Gain<float> gain;

public:
    void prepare(const juce::dsp::ProcessSpec& spec) {
        gain.prepare(spec);
        gain.setRampDurationSeconds(0.020);  // 20ms ramp
    }

    void process(juce::dsp::AudioBlock<float>& block) {
        gain.setGainDecibels(targetGainDb);
        gain.process(juce::dsp::ProcessContextReplacing<float>(block));
    }

    void reset() {
        gain.reset();
    }
};
```

**WARNING:** There's a known issue where `juce::dsp::Gain` may allocate memory when channels > 1. Verify in your profiler or use SmoothedValue instead for critical paths.

### Safe Parameter Access in processBlock

```cpp
class SafeParameterAccess : public AudioProcessor {
    AudioProcessorValueTreeState parameters;

    // Store atomic pointers in constructor - safe to read from audio thread
    std::atomic<float>* gainParam;
    std::atomic<float>* freqParam;

public:
    SafeParameterAccess()
        : parameters(*this, nullptr, "PARAMS", createLayout())
    {
        // Get atomic parameter pointers ONCE in constructor
        gainParam = parameters.getRawParameterValue("gain");
        freqParam = parameters.getRawParameterValue("frequency");
    }

    void processBlock(AudioBuffer<float>& buffer, MidiBuffer&) override {
        // Safe atomic read - no locks, no allocations
        float gain = gainParam->load();
        float freq = freqParam->load();

        // Use values...
    }
};
```

### Real-Time Thread Safety Checklist

Before every processBlock operation, verify:

| Operation | Safe? | Alternative |
|-----------|-------|-------------|
| `new` / `delete` | NO | Pre-allocate |
| `std::vector::push_back()` | NO | Pre-reserve |
| `std::mutex::lock()` | NO | `std::atomic` |
| `String` operations | NO | Pre-allocate |
| `File` operations | NO | Background thread |
| `DBG()` / logging | NO | Ring buffer to other thread |
| `APVTS::copyState()` | NO | Not in audio thread |
| `APVTS::getRawParameterValue()` | YES | (returns atomic pointer) |
| `SmoothedValue::getNextValue()` | YES | - |
| `std::atomic::load()` | YES | - |
| Math operations | YES | - |

---

## Debugging Clicks

### Visual Inspection Techniques

#### In DAW
1. **Zoom in on waveform** - Clicks appear as vertical spikes
2. **Use spectrogram view** - Clicks show as vertical lines across all frequencies
3. **Solo the affected track** - Isolate the problem

#### In Code
```cpp
// Add temporary click detector
class ClickDetector {
    float lastSample = 0.0f;
    int clickCount = 0;
    float threshold = 0.1f;  // Adjust based on signal level

public:
    void process(float sample) {
        float diff = std::abs(sample - lastSample);
        if (diff > threshold) {
            clickCount++;
            DBG("Click detected! Diff: " + String(diff) +
                " at sample " + String(sampleCount));
        }
        lastSample = sample;
    }
};
```

### Common Symptom-to-Cause Mapping

| Symptom | Likely Cause | Solution |
|---------|--------------|----------|
| Periodic clicking (regular rhythm) | Buffer/frame boundary | Check overlap-add, buffer wraparound |
| Click on parameter change | No smoothing | Add SmoothedValue |
| Click on bypass toggle | Instant signal switch | Implement crossfade bypass |
| Random sporadic clicks | Thread safety violation | Audit for allocations/locks |
| Click at start of playback | Uninitialized buffers | Clear buffers in prepareToPlay |
| Click when delay time changes | Delay discontinuity | Interpolate delay reads |
| Click with filter sweep | Coefficient discontinuity | Smooth coefficients, limit sweep speed |
| Click only at high CPU load | Buffer underrun | Optimize or increase buffer size |

### Systematic Isolation Process

1. **Bypass everything** - Does click still happen?
2. **Test with silence** - Input zeros, check output
3. **Test with sine wave** - Should output clean sine
4. **Test with impulse** - Check impulse response
5. **Disable features one by one** - Binary search for cause
6. **Check parameter automation** - Disable automation
7. **Test at different buffer sizes** - 64, 256, 1024, 2048
8. **Test at different sample rates** - 44.1k, 48k, 96k

### Test Signals for Click Detection

```cpp
// Generate test signals
class TestSignalGenerator {
public:
    // Pure sine - reveals any distortion/discontinuity
    float sine(float phase) {
        return std::sin(phase * 2.0f * M_PI);
    }

    // Impulse train - reveals smearing, ringing
    float impulse(int sampleIndex, int period) {
        return (sampleIndex % period == 0) ? 1.0f : 0.0f;
    }

    // Silence - reveals noise floor, DC offset
    float silence() {
        return 0.0f;
    }

    // Full-scale square - reveals clipping behavior
    float square(float phase) {
        return phase < 0.5f ? 1.0f : -1.0f;
    }
};
```

### DAW-Specific Debugging

**Logic Pro:**
- Use "Sample Accurate Automation" to verify timing
- Check Plugin Manager for AU validation
- Use AU Lab for isolated testing

**Ableton Live:**
- Arrangement view zoom shows sample-level detail
- Check "Reduced Latency When Monitoring" setting
- Use Utility plugin for gain staging

**Reaper:**
- Built-in JS Oscilloscope for visualization
- "Take FX" for isolating plugin chain
- Extensive buffer size options

---

## Code Examples

### Click-Free Fade In/Out

```cpp
class FadeProcessor {
public:
    enum class FadeState { Idle, FadingIn, FadingOut, Active };

private:
    FadeState state = FadeState::Idle;
    float currentGain = 0.0f;
    float fadeIncrement = 0.0f;
    float targetGain = 1.0f;

public:
    void prepare(double sampleRate, float fadeTimeMs) {
        fadeIncrement = 1.0f / (fadeTimeMs * 0.001f * sampleRate);
    }

    void startFadeIn() {
        if (state != FadeState::Active) {
            state = FadeState::FadingIn;
            targetGain = 1.0f;
        }
    }

    void startFadeOut() {
        if (state != FadeState::Idle) {
            state = FadeState::FadingOut;
            targetGain = 0.0f;
        }
    }

    void process(AudioBuffer<float>& buffer) {
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            // Update gain
            if (state == FadeState::FadingIn) {
                currentGain += fadeIncrement;
                if (currentGain >= 1.0f) {
                    currentGain = 1.0f;
                    state = FadeState::Active;
                }
            } else if (state == FadeState::FadingOut) {
                currentGain -= fadeIncrement;
                if (currentGain <= 0.0f) {
                    currentGain = 0.0f;
                    state = FadeState::Idle;
                }
            }

            // Apply gain to all channels
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
                buffer.getWritePointer(ch)[i] *= currentGain;
            }
        }
    }

    bool isActive() const { return state != FadeState::Idle; }
};
```

### Safe Parameter Interpolation

```cpp
class InterpolatedParameter {
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothed;
    std::atomic<float>* rawParam = nullptr;

public:
    void prepare(double sampleRate, float rampTimeSeconds) {
        smoothed.reset(sampleRate, rampTimeSeconds);
    }

    void linkToParameter(std::atomic<float>* param) {
        rawParam = param;
    }

    // Call at start of processBlock
    void update() {
        if (rawParam != nullptr) {
            smoothed.setTargetValue(rawParam->load());
        }
    }

    // Call per-sample
    float getNext() {
        return smoothed.getNextValue();
    }

    // Call when not smoothing needed
    float getCurrent() const {
        return smoothed.getCurrentValue();
    }

    bool isSmoothing() const {
        return smoothed.isSmoothing();
    }

    // Skip samples (e.g., for efficiency when parameter is static)
    void skip(int numSamples) {
        smoothed.skip(numSamples);
    }
};
```

### Click-Free Bypass Switching

```cpp
class ClickFreeBypass {
    AudioBuffer<float> dryBuffer;
    float wetGain = 1.0f;
    float targetWetGain = 1.0f;
    float fadeIncrement = 0.0f;
    bool bypassed = false;

public:
    void prepare(double sampleRate, int maxBlockSize) {
        dryBuffer.setSize(2, maxBlockSize);
        // 15ms crossfade
        fadeIncrement = 1.0f / (0.015f * sampleRate);
    }

    void setBypass(bool shouldBypass) {
        bypassed = shouldBypass;
        targetWetGain = shouldBypass ? 0.0f : 1.0f;
    }

    void process(AudioBuffer<float>& buffer,
                 std::function<void(AudioBuffer<float>&)> processFunc) {
        // Store dry signal
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            dryBuffer.copyFrom(ch, 0, buffer, ch, 0, buffer.getNumSamples());
        }

        // Process wet signal
        processFunc(buffer);

        // Crossfade
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            // Update gain toward target
            if (wetGain < targetWetGain) {
                wetGain = std::min(wetGain + fadeIncrement, targetWetGain);
            } else if (wetGain > targetWetGain) {
                wetGain = std::max(wetGain - fadeIncrement, targetWetGain);
            }

            float dryGain = 1.0f - wetGain;

            for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
                float wet = buffer.getSample(ch, i);
                float dry = dryBuffer.getSample(ch, i);
                buffer.setSample(ch, i, wet * wetGain + dry * dryGain);
            }
        }
    }
};
```

### Crossfade Between Processing Modes

```cpp
class ProcessingModeCrossfade {
    enum class Mode { A, B, CrossfadingToA, CrossfadingToB };
    Mode currentMode = Mode::A;
    float crossfadePosition = 0.0f;  // 0 = Mode A, 1 = Mode B
    float crossfadeIncrement = 0.0f;

    AudioBuffer<float> bufferA, bufferB;

public:
    void prepare(double sampleRate, int maxBlockSize) {
        bufferA.setSize(2, maxBlockSize);
        bufferB.setSize(2, maxBlockSize);
        // 25ms crossfade
        crossfadeIncrement = 1.0f / (0.025f * sampleRate);
    }

    void setMode(bool useB) {
        if (useB && (currentMode == Mode::A || currentMode == Mode::CrossfadingToA)) {
            currentMode = Mode::CrossfadingToB;
        } else if (!useB && (currentMode == Mode::B || currentMode == Mode::CrossfadingToB)) {
            currentMode = Mode::CrossfadingToA;
        }
    }

    void process(AudioBuffer<float>& buffer,
                 std::function<void(AudioBuffer<float>&)> processModeA,
                 std::function<void(AudioBuffer<float>&)> processModeB) {

        bool needsBothModes = (currentMode == Mode::CrossfadingToA ||
                               currentMode == Mode::CrossfadingToB);

        if (needsBothModes) {
            // Copy input to both buffers
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
                bufferA.copyFrom(ch, 0, buffer, ch, 0, buffer.getNumSamples());
                bufferB.copyFrom(ch, 0, buffer, ch, 0, buffer.getNumSamples());
            }

            // Process both modes
            processModeA(bufferA);
            processModeB(bufferB);

            // Crossfade per-sample
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                // Update position
                if (currentMode == Mode::CrossfadingToB) {
                    crossfadePosition += crossfadeIncrement;
                    if (crossfadePosition >= 1.0f) {
                        crossfadePosition = 1.0f;
                        currentMode = Mode::B;
                    }
                } else {
                    crossfadePosition -= crossfadeIncrement;
                    if (crossfadePosition <= 0.0f) {
                        crossfadePosition = 0.0f;
                        currentMode = Mode::A;
                    }
                }

                // Equal power crossfade
                float angle = crossfadePosition * M_PI * 0.5f;
                float gainA = std::cos(angle);
                float gainB = std::sin(angle);

                for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
                    float sampleA = bufferA.getSample(ch, i);
                    float sampleB = bufferB.getSample(ch, i);
                    buffer.setSample(ch, i, sampleA * gainA + sampleB * gainB);
                }
            }
        } else {
            // Just process the active mode
            if (currentMode == Mode::A) {
                processModeA(buffer);
            } else {
                processModeB(buffer);
            }
        }
    }
};
```

### Delay Line with Safe Time Changes

```cpp
class SafeDelayLine {
    std::vector<float> buffer;
    int writePos = 0;
    int bufferSize;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> delaySmoothed;

public:
    void prepare(double sampleRate, float maxDelayMs) {
        bufferSize = static_cast<int>(maxDelayMs * 0.001f * sampleRate) + 1;
        buffer.resize(bufferSize, 0.0f);
        writePos = 0;

        // Smooth delay time changes over 50ms to prevent clicks
        delaySmoothed.reset(sampleRate, 0.050);
    }

    void setDelayMs(float delayMs, float sampleRate) {
        float delaySamples = delayMs * 0.001f * sampleRate;
        delaySmoothed.setTargetValue(delaySamples);
    }

    float process(float input) {
        // Write input
        buffer[writePos] = input;

        // Get smoothed delay time
        float delaySamples = delaySmoothed.getNextValue();

        // Clamp to valid range
        delaySamples = juce::jlimit(0.0f, float(bufferSize - 1), delaySamples);

        // Fractional delay with linear interpolation
        int delayInt = static_cast<int>(delaySamples);
        float frac = delaySamples - delayInt;

        int readPos0 = (writePos - delayInt + bufferSize) % bufferSize;
        int readPos1 = (writePos - delayInt - 1 + bufferSize) % bufferSize;

        float output = buffer[readPos0] * (1.0f - frac) + buffer[readPos1] * frac;

        // Advance write position
        writePos = (writePos + 1) % bufferSize;

        return output;
    }

    void clear() {
        std::fill(buffer.begin(), buffer.end(), 0.0f);
    }
};
```

---

## Quick Reference

### Recommended Smoothing Times

| Parameter | Time (ms) | Rationale |
|-----------|-----------|-----------|
| Gain (any) | 10-50 | Balance responsiveness vs. smoothness |
| Filter cutoff | 20-100 | Prevent zipper noise |
| Filter Q | 20-50 | Moderate speed acceptable |
| Pan | 10-20 | Fast response expected |
| Delay time | 50-200 | Slow to prevent pitch artifacts |
| Reverb mix | 20-50 | Natural-sounding transitions |
| Bypass crossfade | 10-25 | Imperceptible transition |

### Crossfade Formulas

```cpp
// Linear (simple)
float gainA = 1.0f - position;
float gainB = position;

// Equal Power (for uncorrelated signals)
float gainA = std::cos(position * M_PI_2);
float gainB = std::sin(position * M_PI_2);

// S-Curve (smooth transitions)
float gainB = 0.5f * (1.0f - std::cos(position * M_PI));
float gainA = 1.0f - gainB;
```

### One-Pole Smoother Coefficients

```cpp
// Given desired smoothing time in ms:
float a = std::exp(-2.0f * M_PI / (timeMs * 0.001f * sampleRate));
float b = 1.0f - a;
// Use: output = input * b + lastOutput * a
```

### Click Prevention Checklist

- [ ] All gain changes use SmoothedValue or equivalent
- [ ] Filter coefficient changes are smoothed
- [ ] Bypass uses crossfade, not instant switch
- [ ] Delay time changes are interpolated
- [ ] prepareToPlay clears all buffers
- [ ] No memory allocation in processBlock
- [ ] No mutex locks in processBlock
- [ ] Buffer boundaries handled (circular buffers wrap correctly)
- [ ] FFT processing uses COLA-compliant overlap-add
- [ ] DC offset is filtered (high-pass at 5-10 Hz)
- [ ] Output is limited/soft-clipped to prevent hard clipping

### Debug Signal Test Cases

| Test | Expected Result | Indicates Problem If |
|------|-----------------|---------------------|
| Silence input | Silence output | Any non-zero output (noise, DC) |
| 1kHz sine | Clean 1kHz sine | Distortion, harmonics visible |
| Impulse | Clean impulse response | Smearing, ringing, multiple clicks |
| Automation | Smooth transition | Steps, zipper noise |
| Bypass toggle | No click | Click on transition |
| Buffer size change | No click | Click after change |

---

## Sources

### Primary (HIGH Confidence)
- [JUCE SmoothedValue Documentation](https://docs.juce.com/master/classSmoothedValue.html)
- [JUCE dsp::Gain Documentation](https://docs.juce.com/master/classdsp_1_1Gain.html)
- [JUCE AudioProcessor Documentation](https://docs.juce.com/master/classAudioProcessor.html)
- [musicdsp.org - One-pole LPF for Parameter Smoothing](https://www.musicdsp.org/en/latest/Filters/257-1-pole-lpf-for-smooth-parameter-changes.html)
- [EarLevel Engineering - One-pole Filter](https://www.earlevel.com/main/2012/12/15/a-one-pole-filter/)

### Secondary (MEDIUM Confidence)
- [Ross Bencina - Real-time Audio Programming 101](http://www.rossbencina.com/code/real-time-audio-programming-101-time-waits-for-nothing)
- [timur.audio - Using Locks in Real-time Audio](https://timur.audio/using-locks-in-real-time-audio-processing-safely)
- [Signalsmith Audio - Cheap Energy-Preserving Crossfade](https://signalsmith-audio.co.uk/writing/2021/cheap-energy-crossfade/)
- [DSPRelated - Delay Line Interpolation](https://www.dsprelated.com/freebooks/pasp/Delay_Line_Interpolation.html)
- [KVR Forum - Parameter Smoothing](https://www.kvraudio.com/forum/viewtopic.php?t=439148)
- [KVR Forum - Circular Buffer Implementation](https://www.kvraudio.com/forum/viewtopic.php?t=408611)

### Tertiary (Reference)
- Will Pirkle - "Designing Audio Effect Plugins in C++" (2nd Edition)
- Julius O. Smith - "Physical Audio Signal Processing" (online textbook)
- BDTI - "Tips and Tricks for Debugging Audio"
- [JUCE Forum - Various threads on clicks/pops prevention](https://forum.juce.com)

---

*Document version: 1.0 | Last updated: 2026-02-04*
