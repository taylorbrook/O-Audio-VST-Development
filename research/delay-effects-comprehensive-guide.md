# Delay Effects - Comprehensive Implementation Guide

**Complete Knowledge Base for Delay Effect Implementation in JUCE Audio Plugins**

**Created:** January 2026
**Version:** 1.0
**Research Depth:** Level 3 (Comprehensive Investigation)

---

## Executive Summary

This comprehensive guide covers delay effect implementation for JUCE audio plugins, from fundamental DSP theory through advanced techniques used in commercial products.

**Key Findings:**
- JUCE's `dsp::DelayLine` provides production-ready fractional delay with 4 interpolation types
- Thiran allpass interpolation offers best quality for modulated delays
- Feedback loops require careful filter design to prevent instability
- Tape/analog delay character comes from modulation, filtering, and saturation
- Modern delays often include ducking for mix clarity
- Reverse delay requires dual-buffer or crossfaded approaches to avoid clicks

---

## Table of Contents

### Part 1: Delay Fundamentals
1. [Basic Delay Architecture](#1-basic-delay-architecture)
2. [Circular Buffer Implementation](#2-circular-buffer-implementation)
3. [Fractional Delay and Interpolation](#3-fractional-delay-and-interpolation)

### Part 2: Delay Types
4. [Simple Digital Delay](#4-simple-digital-delay)
5. [Ping-Pong Delay](#5-ping-pong-delay)
6. [Tape Delay](#6-tape-delay)
7. [Analog/BBD Delay](#7-analogbbd-delay)
8. [Multi-Tap Delay](#8-multi-tap-delay)
9. [Reverse Delay](#9-reverse-delay)

### Part 3: Advanced Techniques
10. [Feedback Networks and Filtering](#10-feedback-networks-and-filtering)
11. [Modulation Techniques](#11-modulation-techniques)
12. [Tempo Synchronization](#12-tempo-synchronization)
13. [Ducking Delay](#13-ducking-delay)

### Part 4: JUCE Implementation
14. [JUCE DelayLine API](#14-juce-delayline-api)
15. [Complete Implementation Examples](#15-complete-implementation-examples)
16. [Real-Time Safety Considerations](#16-real-time-safety-considerations)

---

## Part 1: Delay Fundamentals

## 1. Basic Delay Architecture

### 1.1 Signal Flow

A basic delay effect consists of:

```
Input → [Delay Buffer] → Output
           ↑      ↓
           └──[Feedback]──[Filter]←┘
```

**Core Components:**
- **Delay Buffer**: Circular buffer storing audio samples
- **Read/Write Pointers**: Track buffer positions
- **Feedback Path**: Routes output back to input
- **Mix Control**: Blends dry and wet signals

### 1.2 Key Parameters

| Parameter | Range | Description |
|-----------|-------|-------------|
| Delay Time | 1ms - 2000ms | Time between dry and wet signals |
| Feedback | 0% - 95% | Amount of output fed back to input |
| Mix | 0% - 100% | Dry/wet balance |
| Filter | - | Tone shaping in feedback loop |

---

## 2. Circular Buffer Implementation

### 2.1 Basic Structure

```cpp
class CircularDelayBuffer {
    std::vector<float> buffer;
    int writeIndex = 0;
    int bufferSize;

public:
    void setSize(int size) {
        bufferSize = size;
        buffer.resize(size, 0.0f);
    }

    void push(float sample) {
        buffer[writeIndex] = sample;
        writeIndex = (writeIndex + 1) % bufferSize;
    }

    float read(int delaySamples) {
        int readIndex = (writeIndex - delaySamples + bufferSize) % bufferSize;
        return buffer[readIndex];
    }
};
```

### 2.2 JUCE DelayLine Approach

From the JUCE tutorial, the delay line uses:

```cpp
// Push: Add sample to buffer
void push(SampleType sample) {
    buffer[leastRecentIndex] = sample;
    leastRecentIndex = (leastRecentIndex + 1) % size();
}

// Get: Read at delay offset
SampleType get(size_t delayInSamples) {
    return buffer[(leastRecentIndex + 1 + delayInSamples) % size()];
}
```

---

## 3. Fractional Delay and Interpolation

### 3.1 Why Fractional Delay Matters

When delay time doesn't align exactly with sample boundaries, interpolation is required:
- **Smooth modulation**: Avoids clicks/pops when delay time changes
- **Tempo sync**: BPM-derived delays rarely align to samples
- **Pitch effects**: Chorus/flanger require sub-sample precision

### 3.2 Interpolation Methods Comparison

| Method | Quality | CPU | Best For |
|--------|---------|-----|----------|
| None | Poor | Lowest | Fixed integer delays |
| Linear | Fair | Low | Static delays, simple effects |
| Lagrange 3rd | Good | Medium | Modulated delays, chorus |
| Thiran Allpass | Excellent | Medium | High-quality pitch modulation |
| Sinc | Best | High | Offline/research |

### 3.3 Linear Interpolation

```cpp
float linearInterpolate(float* buffer, float index) {
    int idx0 = (int)index;
    int idx1 = idx0 + 1;
    float frac = index - idx0;

    return buffer[idx0] * (1.0f - frac) + buffer[idx1] * frac;
}
```

**Pros:** Simple, efficient
**Cons:** Acts as lowpass filter, frequency response degrades with fractional delay

### 3.4 Lagrange Interpolation (3rd Order)

```cpp
float lagrange3rdInterpolate(float* buffer, float index) {
    int idx = (int)index;
    float d = index - idx;

    float y0 = buffer[idx - 1];
    float y1 = buffer[idx];
    float y2 = buffer[idx + 1];
    float y3 = buffer[idx + 2];

    float c0 = y1;
    float c1 = y2 - (1.0f/3.0f)*y0 - 0.5f*y1 - (1.0f/6.0f)*y3;
    float c2 = 0.5f*(y0 + y2) - y1;
    float c3 = (1.0f/6.0f)*(y3 - y0) + 0.5f*(y1 - y2);

    return ((c3*d + c2)*d + c1)*d + c0;
}
```

**Pros:** Better frequency response than linear
**Cons:** Still has some magnitude ripple at high frequencies

### 3.5 Thiran Allpass Interpolation

The Thiran allpass filter provides **flat magnitude response** - critical for audio quality.

From Aalto University research:
- "Digital allpass filters are a good choice for fractional delay approximation since their magnitude response is exactly flat"
- "The Thiran interpolator is easy to design with closed-form formulas"
- Most accurate at low frequencies, with increasing error at high frequencies

**First-Order Thiran:**

```cpp
class ThiranAllpass {
    float a1;      // Coefficient
    float z1 = 0;  // State
    float prevOutput = 0;

public:
    void setDelay(float d) {
        // d is fractional part (0.0 to 1.0)
        a1 = (1.0f - d) / (1.0f + d);
    }

    float process(float input) {
        float output = a1 * input + z1 - a1 * prevOutput;
        z1 = input;
        prevOutput = output;
        return output;
    }
};
```

**Stability constraint:** When D < N-1, filter becomes unstable. Always ensure delay >= filter order.

---

## Part 2: Delay Types

## 4. Simple Digital Delay

### 4.1 Characteristics
- Clean, pristine repeats
- No coloration or degradation
- Exact reproduction of input

### 4.2 JUCE Implementation

```cpp
class SimpleDelay {
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> delayLine;
    juce::SmoothedValue<float> delayTimeSamples;
    float feedback = 0.5f;
    float mix = 0.5f;

public:
    void prepare(const juce::dsp::ProcessSpec& spec) {
        delayLine.setMaximumDelayInSamples(spec.sampleRate * 2.0);  // 2 sec max
        delayLine.prepare(spec);
        delayTimeSamples.reset(spec.sampleRate, 0.05);  // 50ms smoothing
    }

    float processSample(int channel, float input) {
        float delayed = delayLine.popSample(channel, delayTimeSamples.getNextValue());
        float toDelay = input + delayed * feedback;
        delayLine.pushSample(channel, toDelay);
        return input * (1.0f - mix) + delayed * mix;
    }
};
```

---

## 5. Ping-Pong Delay

### 5.1 Characteristics
- Stereo bouncing effect
- Cross-feedback between channels
- Creates wide stereo image

### 5.2 Signal Flow

```
Input L → [Delay L] → Output L
              ↓ (cross-feedback)
         [Delay R] → Output R
              ↓ (cross-feedback)
              ↑──────────────┘
```

### 5.3 Implementation

```cpp
class PingPongDelay {
    juce::dsp::DelayLine<float> delayLeft, delayRight;
    float crossFeedback = 0.6f;
    float delayTimeSamples;
    float mix = 0.5f;

public:
    void processStereo(float& left, float& right) {
        // Read delayed samples
        float delayedL = delayLeft.popSample(0, delayTimeSamples);
        float delayedR = delayRight.popSample(0, delayTimeSamples);

        // Cross-feed: left output feeds into right, right into left
        delayLeft.pushSample(0, left + delayedR * crossFeedback);
        delayRight.pushSample(0, right + delayedL * crossFeedback);

        // Mix with dry
        left = left * (1.0f - mix) + delayedL * mix;
        right = right * (1.0f - mix) + delayedR * mix;
    }
};
```

### 5.4 Variations

| Variation | Description |
|-----------|-------------|
| Mono-to-Stereo | Single input spreads across stereo field |
| Offset Times | Different L/R delay times for complexity |
| Filtered Pong | Different filters on each side |

---

## 6. Tape Delay

### 6.1 Characteristics

From ValhallaDelay analysis:
- "A detailed model of a tape delay with variable motor speed, based upon the RE-201 and RE-301"
- Wow/flutter rate and depth tied to delay time (physical accuracy)
- Age parameter controls "asperity noise" and "splice artifacts"
- Three ERA models: darker (Past/Present) to refined (Future)

### 6.2 Key Components

| Component | Description | Implementation |
|-----------|-------------|----------------|
| Wow | Slow pitch variation (0.5-2 Hz) | Modulated delay line |
| Flutter | Fast pitch variation (4-10 Hz) | Modulated delay line |
| Saturation | Tape compression/distortion | tanh() waveshaping |
| Head Bump | Low-frequency resonance | Peaking EQ ~80Hz |
| HF Rolloff | Tape bandwidth limiting | Lowpass filter |
| Hiss | Tape noise | Filtered noise generator |

### 6.3 Wow and Flutter Implementation

- "Wow is a low-frequency cyclical speed variation"
- "Flutter is a much faster-frequency version"
- "Scrape flutter is caused by friction of tape against heads"

```cpp
// LFO configuration
const float wowFreqHz = 1.0f;      // 0.5-1.5Hz typical
const float flutterFreqHz = 6.0f;  // 4-8Hz typical
const float baseDelayMs = 50.0f;   // Base delay for modulation
const float maxModDepth = 0.2f;    // +/-20% pitch deviation

// Calculate modulation
float wowOutput = std::sin(wowPhase);
float flutterOutput = std::sin(flutterPhase);
float totalModulation = (wowOutput + flutterOutput) * 0.5f;

// Apply to delay time
float modulationAmount = baseDelaySamples * maxModDepth * totalModulation;
float delayTimeSamples = baseDelaySamples + modulationAmount;

// Process through modulated delay
modulationDelay.setDelay(delayTimeSamples);
modulationDelay.pushSample(channel, input);
output = modulationDelay.popSample(channel);
```

### 6.4 Tape Speed Characteristics

| Speed | Wow/Flutter | HF Response | Character |
|-------|-------------|-------------|-----------|
| 7.5 ips | Higher | Limited (10kHz) | Lo-fi, warm |
| 15 ips | Medium | Good (15kHz) | Balanced |
| 30 ips | Lower | Excellent (20kHz) | Clean |

---

## 7. Analog/BBD Delay

### 7.1 Bucket Brigade Device (BBD) Characteristics

From ValhallaDelay:
- "Compander artifacts, BBD noise, and limited high frequency response"
- Smoothed triangle LFO modulation
- "Age parameter scales noise inversely with signal level"
- Higher Drive creates "ducking" effects

### 7.2 Key BBD Chips

| Chip | Stages | Max Delay | Used In |
|------|--------|-----------|---------|
| MN3005 | 4096 | ~300ms | Boss DM-2, EHX Memory Man |
| MN3207 | 1024 | ~50ms | Boss CE-2, Roland Jazz Chorus |
| MN3208 | 2048 | ~100ms | Various chorus units |

### 7.3 BBD Modeling Components

```cpp
class BBDDelay {
    // Core delay
    juce::dsp::DelayLine<float> delayLine;

    // Anti-aliasing filter (models BBD clock filter)
    juce::dsp::IIR::Filter<float> antiAliasFilter;  // ~8kHz lowpass

    // Compander (noise reduction artifacts)
    float compressorEnvelope = 0.0f;

    // Clock noise (characteristic BBD artifact)
    float clockNoise = 0.0f;

public:
    float processSample(float input) {
        // 1. Apply input compression (compander encode)
        float compressed = compress(input);

        // 2. Anti-alias filtering (models bandwidth limitation)
        float filtered = antiAliasFilter.processSample(compressed);

        // 3. Delay with modulation
        delayLine.pushSample(0, filtered);
        float delayed = delayLine.popSample(0, delaySamples);

        // 4. Add clock noise (frequency-dependent)
        delayed += clockNoise * noiseAmount;

        // 5. Expand (compander decode)
        float expanded = expand(delayed);

        return expanded;
    }
};
```

---

## 8. Multi-Tap Delay

### 8.1 Characteristics
- Multiple delay outputs from single buffer
- Each tap has independent time, level, pan
- Creates rhythmic patterns
- Often tempo-synced

### 8.2 Implementation

```cpp
struct DelayTap {
    float delayMs;
    float gain;
    float pan;  // 0.0 = left, 1.0 = right
};

class MultiTapDelay {
    juce::dsp::DelayLine<float> delayLine;
    std::array<DelayTap, 8> taps;  // Up to 8 taps
    float mix = 0.5f;

public:
    void processStereo(float input, float& outL, float& outR, float sampleRate) {
        delayLine.pushSample(0, input);

        float sumL = 0.0f, sumR = 0.0f;

        for (const auto& tap : taps) {
            float delaySamples = tap.delayMs * sampleRate / 1000.0f;
            float tapOutput = delayLine.popSample(0, delaySamples, false);
            tapOutput *= tap.gain;

            // Apply panning
            sumL += tapOutput * (1.0f - tap.pan);
            sumR += tapOutput * tap.pan;
        }

        outL = input * (1.0f - mix) + sumL * mix;
        outR = input * (1.0f - mix) + sumR * mix;
    }
};
```

### 8.3 Common Rhythmic Patterns

| Pattern | Tap Times (relative to beat) |
|---------|------------------------------|
| Slapback | 1/16, 1/8 |
| Dotted Eighth | 3/16 (U2 style) |
| Quarter-Eighth | 1/4, 1/2 |
| Triplet | 1/12, 2/12, 3/12 |
| Golden Ratio | 1.0, 1.618, 2.618 (natural feel) |

---

## 9. Reverse Delay

### 9.1 Overview

Reverse delay plays back buffered audio in reverse, creating ethereal, psychedelic effects where sounds swell up before the original attack. It's commonly used on vocals, guitars, and for creative sound design.

### 9.2 Core Algorithm Approaches

#### Approach 1: Dual Buffer Alternating

> "Reverse delays are commonly achieved by two or more parallel delay lines which take turns in filling their buffers in quiet, then play them back backwards aloud."

```cpp
class DualBufferReverseDelay {
    std::array<std::vector<float>, 2> buffers;  // Two buffers
    int activeBuffer = 0;
    int writeIndex = 0;
    int readIndex;
    int bufferLength;  // = delay time in samples
    float crossfadePosition = 0.0f;

    enum class State { Recording, Playing, Crossfading };
    State state = State::Recording;

public:
    void prepare(int delaySamples) {
        bufferLength = delaySamples;
        buffers[0].resize(bufferLength, 0.0f);
        buffers[1].resize(bufferLength, 0.0f);
        readIndex = bufferLength - 1;  // Start at end for reverse read
    }

    float processSample(float input) {
        // Write to active buffer
        buffers[activeBuffer][writeIndex] = input;

        // Read from inactive buffer in reverse
        int playBuffer = 1 - activeBuffer;
        float output = buffers[playBuffer][readIndex];

        // Advance indices
        writeIndex++;
        readIndex--;

        // When write buffer is full, swap buffers
        if (writeIndex >= bufferLength) {
            writeIndex = 0;
            readIndex = bufferLength - 1;
            activeBuffer = 1 - activeBuffer;  // Swap
            // Trigger crossfade here
        }

        return output;
    }
};
```

#### Approach 2: Single Buffer with Reversed Read

> "Run the read index backwards, until you run out of buffer, at which time you reset it."

**Important consideration:**
> "You'll end up with half as much delay time as you think, because while you're stepping the read index backwards at the sample rate, the write index is marching forward at the same rate, and they meet in the middle."

```cpp
class SingleBufferReverseDelay {
    std::vector<float> buffer;
    int writeIndex = 0;
    int readIndex = 0;
    int bufferSize;

public:
    void prepare(int maxDelaySamples) {
        // Double the buffer to account for read/write convergence
        bufferSize = maxDelaySamples * 2;
        buffer.resize(bufferSize, 0.0f);
        readIndex = bufferSize - 1;
    }

    float processSample(float input) {
        // Write forward
        buffer[writeIndex] = input;
        writeIndex = (writeIndex + 1) % bufferSize;

        // Read backward
        float output = buffer[readIndex];
        readIndex--;
        if (readIndex < 0) readIndex = bufferSize - 1;

        // Reset when indices meet
        if (readIndex == writeIndex) {
            readIndex = (writeIndex + bufferSize - 1) % bufferSize;
        }

        return output;
    }
};
```

#### Approach 3: Granular Reverse (Zig-Zag)

> "The read signal is enveloped by a raised cosine synchronous with the phasor so you get a stream of reversed grains but in the right temporal order."

```cpp
// Granular reverse playback
if (grain.reverse) {
    grain.readPosition -= grain.playbackRate;
    if (grain.readPosition < 0.0f) {
        grain.readPosition += currentDelayBufferSize;
    }
} else {
    grain.readPosition += grain.playbackRate;
}
```

### 9.3 Crossfading to Eliminate Clicks

**The Problem:**
> "You're jumping from one point in the delay buffer to another, and there is likely to be a discontinuity... you might reach the end of your sweep and be at a negative extreme, and when the index resets, it might be at a positive extreme, resulting in a pop."

**Solution: Dual-Reader Crossfade**

```cpp
class CrossfadedReverseDelay {
    std::vector<float> buffer;
    int writeIndex = 0;

    // Two read heads for crossfading
    int readIndexA = 0;
    int readIndexB = 0;
    float crossfade = 0.0f;
    bool readerAActive = true;

    int bufferSize;
    int crossfadeLength = 512;  // Samples for crossfade (~10ms at 48kHz)

public:
    float processSample(float input) {
        // Write to buffer
        buffer[writeIndex] = input;
        writeIndex = (writeIndex + 1) % bufferSize;

        // Read from both heads
        float outputA = buffer[readIndexA];
        float outputB = buffer[readIndexB];

        // Decrement read indices (reverse)
        readIndexA = (readIndexA - 1 + bufferSize) % bufferSize;
        readIndexB = (readIndexB - 1 + bufferSize) % bufferSize;

        // Crossfade logic
        float output;
        if (crossfade > 0.0f) {
            // During crossfade: blend both readers with raised cosine
            float fadeOut = std::cos(crossfade * M_PI * 0.5f);
            float fadeIn = std::sin(crossfade * M_PI * 0.5f);

            if (readerAActive)
                output = outputA * fadeOut + outputB * fadeIn;
            else
                output = outputB * fadeOut + outputA * fadeIn;

            crossfade -= 1.0f / crossfadeLength;
        } else {
            output = readerAActive ? outputA : outputB;
        }

        // Check for reset condition
        if (needsReset(readerAActive ? readIndexA : readIndexB)) {
            // Start crossfade to other reader
            crossfade = 1.0f;
            readerAActive = !readerAActive;

            // Reset the inactive reader to start of new reverse segment
            if (readerAActive)
                readIndexB = (writeIndex - 1 + bufferSize) % bufferSize;
            else
                readIndexA = (writeIndex - 1 + bufferSize) % bufferSize;
        }

        return output;
    }

    bool needsReset(int readIdx) {
        // Reset when read catches up to write
        int distance = (writeIndex - readIdx + bufferSize) % bufferSize;
        return distance < 10;  // Safety margin
    }
};
```

### 9.4 Raised Cosine Envelope for Smooth Grains

```cpp
float raisedCosineWindow(float phase) {
    // phase: 0.0 to 1.0
    return 0.5f * (1.0f - std::cos(phase * 2.0f * M_PI));
}

// Apply to each reversed segment
float windowedOutput = reversedSample * raisedCosineWindow(segmentPhase);
```

### 9.5 Reverse Delay with Feedback

```cpp
class ReverseDelayWithFeedback {
    // ... buffer setup ...
    float feedback = 0.5f;
    float feedbackSample = 0.0f;

    float processSample(float input) {
        // Mix feedback into input
        float toBuffer = input + feedbackSample * feedback;

        // Write and read reversed
        buffer[writeIndex] = toBuffer;
        float output = readReversed();

        // Store for next iteration (use tanh to prevent runaway)
        feedbackSample = std::tanh(output);

        return output;
    }
};
```

### 9.6 Commercial Examples

| Product | Reverse Mode Description |
|---------|-------------------------|
| ValhallaDelay | "Reverses and pitch shifts the input signal. DELAY controls splice size" |
| Boss DD-7 | Classic reverse delay pedal |
| Eventide H9 | Multiple reverse algorithms |
| Strymon Timeline | "Reverse" mode with adjustable splice/grain size |

### 9.7 Key Implementation Considerations

1. **Buffer Size**: Use 2x desired delay time to account for read/write convergence
2. **Crossfade Length**: 5-20ms (256-1024 samples) for smooth transitions
3. **Feedback Limiting**: Use tanh() to prevent runaway with reverse feedback
4. **Grain Size**: Controls how "choppy" vs "smooth" the reverse effect sounds
5. **Pre-delay**: Small delay before reverse helps with transient clarity

---

## Part 3: Advanced Techniques

## 10. Feedback Networks and Filtering

### 10.1 Feedback Stability

Key principles:
- "If the gain within the loop goes above unity then it will oscillate"
- "You need to use filters specifically designed so that no one frequency is amplified by more than 1"

**Stability Rules:**
1. Feedback gain must be < 1.0 at all frequencies
2. Use tanh() or limiter to prevent runaway oscillation
3. Consider frequency-dependent feedback

### 10.2 Lowpass Feedback Comb Filter

From CCRMA Physical Audio Signal Processing:

**Transfer Function:**
```
H(z) = z^(-N) / (1 - f * (1-d)/(1-d*z^(-1)) * z^(-N))
```

Where:
- N = delay in samples
- f = feedback coefficient (< 1 for stability)
- d = damping coefficient (lowpass)

**Implementation:**

```cpp
class LowpassFeedbackComb {
    std::vector<float> buffer;
    float lpState = 0.0f;
    float feedback = 0.84f;
    float damping = 0.2f;
    int writeIndex = 0;
    int delayLength;
    int bufferSize;

public:
    float process(float input) {
        // Read from delay
        int readIndex = (writeIndex - delayLength + bufferSize) % bufferSize;
        float delayed = buffer[readIndex];

        // Lowpass filter in feedback path
        lpState = delayed * (1.0f - damping) + lpState * damping;

        // Write input + filtered feedback
        buffer[writeIndex] = input + lpState * feedback;
        writeIndex = (writeIndex + 1) % bufferSize;

        return delayed;
    }
};
```

### 10.3 Filter Types in Feedback

| Filter | Effect | Use Case |
|--------|--------|----------|
| Lowpass | Darkening, warmth | Tape/analog simulation |
| Highpass | Thins repeats, reduces mud | Clarity in dense mixes |
| Bandpass | Telephone/radio effect | Creative effects |
| Shelving | Gentle tonal shaping | Subtle coloration |

### 10.4 Feedback with tanh() Limiting

From JUCE tutorial:
- "Calculate the sample to be pushed by mixing input with delay output weighted with feedback using std::tanh()"
- Prevents clipping and provides natural decay

```cpp
float feedbackSample = delayedOutput * feedback;
float toDelay = std::tanh(input + feedbackSample);  // Soft limiting
```

---

## 11. Modulation Techniques

### 11.1 Modulation Types

| Effect | Rate | Depth | Character |
|--------|------|-------|-----------|
| Chorus | 0.5-3 Hz | 5-20ms | Thickening, doubling |
| Flanger | 0.1-10 Hz | 1-10ms | Jet/sweep, comb filtering |
| Vibrato | 4-8 Hz | 5-10ms | Pitch wobble |
| Wow | 0.5-2 Hz | 10-50ms | Tape speed variation |
| Flutter | 4-10 Hz | 1-5ms | Motor inconsistency |

### 11.2 LFO Shapes

```cpp
enum class LFOShape { Sine, Triangle, Random };

float getLFOValue(LFOShape shape, float phase) {
    switch (shape) {
        case LFOShape::Sine:
            return std::sin(phase * 2.0f * M_PI);

        case LFOShape::Triangle:
            return 4.0f * std::abs(phase - 0.5f) - 1.0f;

        case LFOShape::Random:
            // Sample-and-hold random
            return currentRandomValue;  // Updated periodically
    }
}
```

### 11.3 Smooth Delay Modulation

From JUCE documentation:
- "If you intend to change the delay in real time, you may want to smooth changes systematically using either a ramp or a low-pass filter"

```cpp
// Option 1: SmoothedValue
juce::SmoothedValue<float> delayTime;
delayTime.reset(sampleRate, 0.05);  // 50ms smoothing

// Option 2: First-order lowpass
float smoothedDelay = smoothedDelay * 0.99f + targetDelay * 0.01f;
```

---

## 12. Tempo Synchronization

### 12.1 Getting Host Tempo in JUCE

```cpp
void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    if (auto* playHead = getPlayHead()) {
        if (auto position = playHead->getPosition()) {
            if (auto bpm = position->getBpm()) {
                currentBPM = *bpm;
            }
        }
    }

    // Calculate delay time from BPM
    float beatDurationMs = 60000.0f / currentBPM;
    float delayTimeMs = beatDurationMs * noteValue;  // e.g., 0.5 for eighth note
}
```

### 12.2 Note Value Calculations

| Note | Multiplier | At 120 BPM |
|------|------------|------------|
| Whole | 4.0 | 2000ms |
| Half | 2.0 | 1000ms |
| Quarter | 1.0 | 500ms |
| Eighth | 0.5 | 250ms |
| Sixteenth | 0.25 | 125ms |
| Dotted Eighth | 0.75 | 375ms |
| Triplet Eighth | 0.333 | 167ms |

### 12.3 Smooth Tempo Changes

```cpp
// Convert to samples with smoothing
float targetDelaySamples = (delayTimeMs / 1000.0f) * sampleRate;
smoothedDelaySamples.setTargetValue(targetDelaySamples);

// In process loop
float currentDelay = smoothedDelaySamples.getNextValue();
delayLine.setDelay(currentDelay);
```

---

## 13. Ducking Delay

### 13.1 Purpose

- "Audio ducking compresses the volume of one signal whenever another goes above threshold"
- "When dry signal is present, delay ducks. When dry stops, delay tail swells"
- "Prevents muddiness and helps clarity by minimizing clash between wet and dry"

### 13.2 Implementation

```cpp
class DuckingDelay {
    juce::dsp::DelayLine<float> delayLine;
    float envelope = 0.0f;

    // Ducking parameters
    float threshold = 0.1f;     // -20dB
    float duckAmount = 0.7f;    // Duck by 70% (about 12dB)
    float attackMs = 2.0f;      // Fast attack
    float releaseMs = 150.0f;   // Moderate release
    float attackCoeff, releaseCoeff;
    float feedback = 0.5f;
    float mix = 0.5f;
    float delaySamples;

public:
    void prepare(double sampleRate) {
        attackCoeff = std::exp(-1.0f / (attackMs * 0.001f * sampleRate));
        releaseCoeff = std::exp(-1.0f / (releaseMs * 0.001f * sampleRate));
    }

    float processSample(float input) {
        // Envelope follower on dry signal
        float inputLevel = std::abs(input);
        if (inputLevel > envelope)
            envelope = attackCoeff * envelope + (1.0f - attackCoeff) * inputLevel;
        else
            envelope = releaseCoeff * envelope + (1.0f - releaseCoeff) * inputLevel;

        // Calculate duck gain
        float duckGain = 1.0f;
        if (envelope > threshold) {
            float overshoot = envelope / threshold;
            duckGain = 1.0f - (duckAmount * (1.0f - 1.0f / overshoot));
        }

        // Process delay
        float delayed = delayLine.popSample(0, delaySamples);
        delayLine.pushSample(0, input + delayed * feedback);

        // Apply ducking to wet signal only
        return input + delayed * mix * duckGain;
    }
};
```

### 13.3 Typical Settings

| Parameter | Subtle Ducking | Aggressive Ducking |
|-----------|---------------|-------------------|
| Threshold | -10dB | -20dB |
| Duck Amount | 6dB | 12dB+ |
| Attack | 1-5ms | 1-2ms |
| Release | 150-300ms | 100-150ms |

---

## Part 4: JUCE Implementation

## 14. JUCE DelayLine API

### 14.1 Class Declaration

```cpp
template<typename SampleType,
         typename InterpolationType = DelayLineInterpolationTypes::Linear>
class juce::dsp::DelayLine
```

### 14.2 Interpolation Types

```cpp
namespace juce::dsp::DelayLineInterpolationTypes {
    struct None;        // No interpolation (integer delays only)
    struct Linear;      // Linear interpolation (default)
    struct Lagrange3rd; // 3rd-order Lagrange polynomial
    struct Thiran;      // Allpass interpolation (flat magnitude)
}
```

**Selection Guide:**

| Use Case | Recommended Type |
|----------|------------------|
| Fixed integer delay | `None` |
| Static delay, low CPU | `Linear` |
| Modulated delay, chorus | `Lagrange3rd` |
| High-quality pitch modulation | `Thiran` |

### 14.3 Key Methods

```cpp
// Initialization
DelayLine();
DelayLine(int maximumDelayInSamples);
void prepare(const ProcessSpec& spec);
void setMaximumDelayInSamples(int maxDelayInSamples);

// Delay control
void setDelay(SampleType newDelayInSamples);
SampleType getDelay() const;

// Sample-by-sample processing
void pushSample(int channel, SampleType sample);
SampleType popSample(int channel,
                     SampleType delayInSamples = -1,
                     bool updateReadPointer = true);

// Block processing (fixed delay only)
void process(const ProcessContext& context);

// State
void reset();
```

### 14.4 Typical Usage Pattern

```cpp
// Header
juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> delayLine;

// prepareToPlay()
void prepareToPlay(double sampleRate, int samplesPerBlock) {
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 2;

    int maxDelaySamples = static_cast<int>(sampleRate * 2.0);  // 2 seconds
    delayLine.setMaximumDelayInSamples(maxDelaySamples);
    delayLine.prepare(spec);
    delayLine.reset();
}

// processBlock() - sample-by-sample for modulation
for (int ch = 0; ch < numChannels; ++ch) {
    auto* channelData = buffer.getWritePointer(ch);
    for (int i = 0; i < numSamples; ++i) {
        float delayed = delayLine.popSample(ch, currentDelay);
        delayLine.pushSample(ch, channelData[i] + delayed * feedback);
        channelData[i] = channelData[i] * (1-mix) + delayed * mix;
    }
}
```

---

## 15. Complete Implementation Examples

### 15.1 Simple Stereo Delay with Feedback

```cpp
class StereoDelay : public juce::AudioProcessor {
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> delayLine;
    juce::dsp::DryWetMixer<float> dryWetMixer;
    juce::dsp::IIR::Filter<float> feedbackFilter;
    juce::SmoothedValue<float> smoothedDelay;

    float feedback = 0.5f;
    float filterFreq = 5000.0f;  // Highcut in feedback

public:
    void prepareToPlay(double sampleRate, int samplesPerBlock) override {
        juce::dsp::ProcessSpec spec { sampleRate,
                                       static_cast<juce::uint32>(samplesPerBlock),
                                       2 };

        delayLine.setMaximumDelayInSamples(sampleRate * 2);
        delayLine.prepare(spec);

        dryWetMixer.prepare(spec);
        dryWetMixer.reset();

        feedbackFilter.prepare(spec);
        *feedbackFilter.coefficients = *juce::dsp::IIR::Coefficients<float>::
            makeLowPass(sampleRate, filterFreq);

        smoothedDelay.reset(sampleRate, 0.05);  // 50ms smoothing
    }

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override {
        juce::ScopedNoDenormals noDenormals;

        juce::dsp::AudioBlock<float> block(buffer);
        dryWetMixer.pushDrySamples(block);

        const int numChannels = buffer.getNumChannels();
        const int numSamples = buffer.getNumSamples();

        for (int ch = 0; ch < numChannels; ++ch) {
            auto* data = buffer.getWritePointer(ch);

            for (int i = 0; i < numSamples; ++i) {
                float currentDelay = smoothedDelay.getNextValue();
                float delayed = delayLine.popSample(ch, currentDelay);

                // Filter in feedback path
                float filtered = feedbackFilter.processSample(delayed);

                // tanh limiting prevents runaway
                float toDelay = std::tanh(data[i] + filtered * feedback);
                delayLine.pushSample(ch, toDelay);

                data[i] = delayed;  // Output wet only (mixer handles blend)
            }
        }

        dryWetMixer.mixWetSamples(block);
    }
};
```

### 15.2 Ping-Pong Delay

```cpp
class PingPongDelay {
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd>
        delayLeft, delayRight;
    juce::dsp::DryWetMixer<float> mixer;

    float crossFeedback = 0.6f;
    float delayTimeSamples;

public:
    void prepare(const juce::dsp::ProcessSpec& spec) {
        delayLeft.setMaximumDelayInSamples(spec.sampleRate * 2);
        delayRight.setMaximumDelayInSamples(spec.sampleRate * 2);
        delayLeft.prepare(spec);
        delayRight.prepare(spec);
        mixer.prepare(spec);
    }

    void process(juce::AudioBuffer<float>& buffer) {
        juce::dsp::AudioBlock<float> block(buffer);
        mixer.pushDrySamples(block);

        auto* leftData = buffer.getWritePointer(0);
        auto* rightData = buffer.getWritePointer(1);
        const int numSamples = buffer.getNumSamples();

        for (int i = 0; i < numSamples; ++i) {
            float delayedL = delayLeft.popSample(0, delayTimeSamples);
            float delayedR = delayRight.popSample(0, delayTimeSamples);

            // Cross-feedback: L->R, R->L
            delayLeft.pushSample(0, leftData[i] + delayedR * crossFeedback);
            delayRight.pushSample(0, rightData[i] + delayedL * crossFeedback);

            leftData[i] = delayedL;
            rightData[i] = delayedR;
        }

        mixer.mixWetSamples(block);
    }
};
```

### 15.3 Tape Delay with Modulation

```cpp
class TapeDelay {
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Thiran> delayLine;
    juce::dsp::IIR::Filter<float> lowpassFilter;  // HF rolloff
    juce::dsp::IIR::Filter<float> headBumpFilter; // Low-end resonance
    juce::dsp::DryWetMixer<float> mixer;

    // Modulation state
    float wowPhase = 0.0f;
    float flutterPhase = 0.0f;
    float wowRate = 1.0f;     // Hz
    float flutterRate = 6.0f;  // Hz
    float modDepth = 0.002f;   // +/-0.2% pitch deviation

    // Tape characteristics
    float saturationAmount = 0.3f;
    float feedback = 0.5f;
    float baseDelayMs = 375.0f;  // Dotted eighth at 120 BPM
    double sampleRate;

public:
    void prepare(const juce::dsp::ProcessSpec& spec) {
        sampleRate = spec.sampleRate;

        delayLine.setMaximumDelayInSamples(spec.sampleRate * 2);
        delayLine.prepare(spec);

        // HF rolloff (~8kHz for tape character)
        lowpassFilter.prepare(spec);
        *lowpassFilter.coefficients = *juce::dsp::IIR::Coefficients<float>::
            makeLowPass(spec.sampleRate, 8000.0f);

        // Head bump (~80Hz peaking)
        headBumpFilter.prepare(spec);
        *headBumpFilter.coefficients = *juce::dsp::IIR::Coefficients<float>::
            makePeakFilter(spec.sampleRate, 80.0f, 0.7f,
                          juce::Decibels::decibelsToGain(3.0f));

        mixer.prepare(spec);
    }

    void processSample(int channel, float& sample) {
        // Calculate modulated delay time
        float wow = std::sin(wowPhase) * modDepth;
        float flutter = std::sin(flutterPhase) * modDepth * 0.5f;
        float totalMod = 1.0f + wow + flutter;

        float baseDelaySamples = (baseDelayMs / 1000.0f) * sampleRate;
        float modulatedDelay = baseDelaySamples * totalMod;

        // Read from delay
        float delayed = delayLine.popSample(channel, modulatedDelay);

        // Apply tape character
        delayed = lowpassFilter.processSample(delayed);    // HF rolloff
        delayed = headBumpFilter.processSample(delayed);   // Head bump

        // Saturation in feedback path
        float saturated = std::tanh(delayed * (1.0f + saturationAmount));
        float feedbackSample = saturated * feedback;

        // Write to delay
        delayLine.pushSample(channel, sample + feedbackSample);

        // Update LFO phases
        wowPhase += (wowRate * 2.0f * M_PI) / sampleRate;
        if (wowPhase >= 2.0f * M_PI) wowPhase -= 2.0f * M_PI;

        flutterPhase += (flutterRate * 2.0f * M_PI) / sampleRate;
        if (flutterPhase >= 2.0f * M_PI) flutterPhase -= 2.0f * M_PI;

        sample = delayed;
    }
};
```

---

## 16. Real-Time Safety Considerations

### 16.1 Essential Practices

| Practice | Reason | Example |
|----------|--------|---------|
| `juce::ScopedNoDenormals` | Prevent denormal CPU spikes | First line of `processBlock()` |
| Atomic parameter reads | Thread-safe UI<->Audio | `getRawParameterValue()->load()` |
| Smoothed value changes | Avoid clicks/pops | `SmoothedValue` for delay time |
| Pre-allocated buffers | No allocations in audio thread | Resize in `prepareToPlay()` |
| Limit feedback < 1.0 | Prevent oscillation | `feedback = juce::jmin(0.95f, fb)` |

### 16.2 Parameter Smoothing Pattern

```cpp
class DelayProcessor {
    juce::SmoothedValue<float> delayTimeSamples;
    juce::SmoothedValue<float> feedbackGain;
    juce::SmoothedValue<float> mixAmount;

    void prepareToPlay(double sampleRate, int) {
        // Reset smoothing with 50ms ramp time
        delayTimeSamples.reset(sampleRate, 0.05);
        feedbackGain.reset(sampleRate, 0.05);
        mixAmount.reset(sampleRate, 0.05);
    }

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
        // Update targets from parameters
        delayTimeSamples.setTargetValue(getDelayInSamples());
        feedbackGain.setTargetValue(getFeedback());
        mixAmount.setTargetValue(getMix());

        // Process with smoothed values
        for (int i = 0; i < numSamples; ++i) {
            float currentDelay = delayTimeSamples.getNextValue();
            float currentFeedback = feedbackGain.getNextValue();
            float currentMix = mixAmount.getNextValue();
            // ... process sample
        }
    }
};
```

### 16.3 Latency Reporting

```cpp
int getLatencySamples() const override {
    // Report latency for host compensation
    int latency = 0;
    latency += static_cast<int>(delayLine.getDelay());  // If fixed delay
    latency += mixer.getLatencyInSamples();              // DryWetMixer latency
    return latency;
}
```

---

## Summary and Recommendations

### For Simple Delay Effects

1. Use `juce::dsp::DelayLine` with `Lagrange3rd` interpolation
2. Implement feedback with tanh() limiting
3. Add lowpass filter in feedback for warmth
4. Use `SmoothedValue` for all parameters

### For Tape/Analog Character

1. Use `Thiran` interpolation for smooth modulation
2. Add dual LFO (wow/flutter) for pitch variation
3. Include saturation (tanh) in signal path
4. Add head bump (80Hz peak) and HF rolloff (8kHz LP)

### For Reverse Delay

1. Use dual-buffer approach for seamless playback
2. Implement raised-cosine crossfading (5-20ms)
3. Double buffer size to account for read/write convergence
4. Use tanh() limiting in feedback to prevent runaway

### For Professional Polish

1. Implement tempo sync from host playhead
2. Add ducking for mix clarity
3. Provide multiple filter types in feedback
4. Support ping-pong stereo mode

---

## Sources

### Official Documentation
- JUCE DelayLine Tutorial: https://juce.com/tutorials/tutorial_dsp_delay_line/
- JUCE dsp::DelayLine API: https://docs.juce.com/master/classjuce_1_1dsp_1_1DelayLine.html

### DSP Theory
- Stanford CCRMA - Interpolated Delay Lines: https://ccrma.stanford.edu/~jos/Interpolation/
- CCRMA - Lowpass Feedback Comb Filter: https://ccrma.stanford.edu/~jos/pasp/Lowpass_Feedback_Comb_Filter.html
- Aalto University - Fractional Delay Allpass Filters: http://users.spa.aalto.fi/vpv/publications/vesan_vaitos/ch3_pt3_allpass.pdf

### Commercial Analysis
- ValhallaDelay MODE Control Analysis: https://valhalladsp.com/2019/04/16/valhalladelay-the-mode-control/
- Baby Audio - Wow and Flutter Explained: https://babyaud.io/blog/wow-and-flutter
- iZotope - What is Audio Ducking: https://www.izotope.com/en/learn/what-is-audio-ducking.html

### Reverse Delay
- KVR Forum - How does reverse delay work?: https://www.kvraudio.com/forum/viewtopic.php?t=599376
- KVR Forum - Reverse Delay design: https://www.kvraudio.com/forum/viewtopic.php?t=375055
- music-dsp mailing list - Reverse delay: https://music-dsp.music.columbia.narkive.com/3kmlSDII/reverse-delay
- Roy Fox - A reverse delay plugin: https://www.royfox.co.uk/2022-10-04/reverse-delay
- ElectroSmash - Back Talk Reverse Delay: https://www.electrosmash.com/back-talk-reverse-delay/pedals/delay/back-talk-reverse-delay.html

### Open Source References
- GitHub - joonastuo/Delay: https://github.com/joonastuo/Delay
- GitHub - cryptologicpsy/DelayLine: https://github.com/cryptologicpsy/DelayLine
- GitHub - dllim/anotherdelay (Tape Delay): https://github.com/dllim/anotherdelay

### Community Resources
- KVR Audio DSP Forum - Filtered Delay Loop Stability: https://www.kvraudio.com/forum/viewtopic.php?t=234116
- Native Instruments - The Art of VA Filter Design: https://www.native-instruments.com/fileadmin/ni_media/downloads/pdf/VAFilterDesign_2.0.0a.pdf

---

*End of Complete Guide*
