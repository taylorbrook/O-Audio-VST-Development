---
title: "Freeze & Spectral Freeze Effects"
created: 2026-03-07
juce_version: "8.0.4"
summary: "Complete technical reference for audio freeze effects in plugins, covering spectral freeze via FFT phase vocoder, buffer capture and loop, granular freeze, time-domain crossfade freeze, and JUCE implementation with real-time performance considerations."
domain: dsp
type: research
keywords:
  - freeze
  - spectral-freeze
  - phase-vocoder
  - buffer-capture
  - granular-freeze
  - fft
  - stft
  - sustain
  - juce-dsp
stages: [0, 1, 2]
agents: [dsp, research]
---

# Freeze & Spectral Freeze Effects

**Complete Technical Reference for Audio Freeze Effects in Plugins**

**Created:** March 2026
**Version:** 1.0
**Research Depth:** Level 3 (Comprehensive Investigation)

---

## Executive Summary

This document covers audio freeze effects for plugin development. A freeze effect captures a moment of audio and sustains it indefinitely, creating pads, drones, and textural elements from any input. Multiple approaches exist -- spectral freeze (FFT-based), buffer loop freeze, and granular freeze -- each with distinct sonic characteristics.

**Key Findings:**
- Spectral freeze (locking FFT magnitudes while resynthesizing with rotating phases) produces the smoothest sustain
- Buffer loop freeze with crossfade is the simplest approach but can sound repetitive
- Granular freeze scatters small grains from a captured buffer for organic, evolving textures
- Phase vocoder spectral freeze requires careful overlap-add (75% overlap minimum) for artifact-free output
- Capture timing and crossfade envelope design are critical for clean freeze activation
- JUCE's FFT module provides the building blocks; custom overlap-add windowing is needed

---

## Table of Contents

### Part 1: Fundamentals
1. [Freeze Effect Overview](#1-freeze-effect-overview)
2. [Buffer Capture Strategies](#2-buffer-capture-strategies)
3. [Crossfade Techniques](#3-crossfade-techniques)

### Part 2: Freeze Approaches
4. [Buffer Loop Freeze](#4-buffer-loop-freeze)
5. [Spectral Freeze (FFT-Based)](#5-spectral-freeze-fft-based)
6. [Granular Freeze](#6-granular-freeze)
7. [Hybrid Approaches](#7-hybrid-approaches)

### Part 3: Advanced Topics
8. [Spectral Processing During Freeze](#8-spectral-processing-during-freeze)
9. [Stereo and Spatial Considerations](#9-stereo-and-spatial-considerations)
10. [Transition Design](#10-transition-design)

### Part 4: JUCE Implementation
11. [JUCE FFT for Spectral Freeze](#11-juce-fft-for-spectral-freeze)
12. [Complete Freeze Plugin Architecture](#12-complete-freeze-plugin-architecture)
13. [Optimization and Real-Time Safety](#13-optimization-and-real-time-safety)

### Part 5: References
14. [References and Further Reading](#14-references-and-further-reading)

---

## Part 1: Fundamentals

## 1. Freeze Effect Overview

### 1.1 What Freeze Does

A freeze effect captures a short segment of audio (typically 50-500 ms) and sustains it indefinitely, creating a continuous tone or texture from a transient input. When deactivated, the output crossfades back to the live input.

### 1.2 Freeze Approaches Comparison

| Approach | Sonic Character | Complexity | CPU Cost | Artifacts |
|----------|----------------|------------|----------|-----------|
| **Buffer loop** | Repetitive, rhythmic | Low | Very low | Loop point clicks |
| **Spectral freeze** | Smooth, pad-like | High | Medium | Phase coherence |
| **Granular freeze** | Evolving, organic | Medium | Medium | Grain boundary |
| **Phase vocoder** | Very smooth, synthetic | High | Medium-high | Phasiness |

### 1.3 Common Use Cases

- Creating sustained pad textures from any input
- "Infinite sustain" effects for guitar or synth
- Sound design: capturing and manipulating frozen moments
- Ambient/atmospheric effects in live performance
- Background drone generation from musical material

---

## 2. Buffer Capture Strategies

### 2.1 Fixed-Length Capture

Capture a fixed number of samples when freeze is activated:

```cpp
class BufferCapture
{
public:
    void prepare(double sampleRate, float captureLengthMs)
    {
        int captureSamples = static_cast<int>(captureLengthMs * 0.001f * sampleRate);
        captureBuffer.resize(captureSamples, 0.0f);
        captureIndex = 0;
        isCapturing = false;
    }

    void startCapture()
    {
        captureIndex = 0;
        isCapturing = true;
    }

    void writeSample(float sample)
    {
        if (!isCapturing) return;

        captureBuffer[captureIndex++] = sample;

        if (captureIndex >= (int)captureBuffer.size())
            isCapturing = false;
    }

    bool captureComplete() const { return !isCapturing && captureIndex > 0; }

    const std::vector<float>& getBuffer() const { return captureBuffer; }

private:
    std::vector<float> captureBuffer;
    int captureIndex = 0;
    bool isCapturing = false;
};
```

### 2.2 Circular Buffer Capture

For zero-latency freeze, continuously record to a circular buffer and "freeze" the current contents:

```cpp
class CircularCapture
{
public:
    void prepare(double sampleRate, float bufferLengthMs)
    {
        int bufferSamples = static_cast<int>(bufferLengthMs * 0.001f * sampleRate);
        buffer.resize(bufferSamples, 0.0f);
        writePos = 0;
    }

    void writeSample(float sample)
    {
        if (frozen) return; // Stop writing when frozen
        buffer[writePos] = sample;
        writePos = (writePos + 1) % (int)buffer.size();
    }

    void freeze() { frozen = true; }
    void unfreeze() { frozen = false; }

    float readSample(int offset) const
    {
        int idx = (writePos - (int)buffer.size() + offset + (int)buffer.size() * 2)
                  % (int)buffer.size();
        return buffer[idx];
    }

    int getLength() const { return (int)buffer.size(); }

private:
    std::vector<float> buffer;
    int writePos = 0;
    bool frozen = false;
};
```

### 2.3 Zero-Crossing Detection

For cleaner loop points, detect zero crossings near the desired capture boundaries:

```cpp
int findNearestZeroCrossing(const float* buffer, int targetPos, int searchRange, int bufferSize)
{
    int bestPos = targetPos;
    float bestScore = std::abs(buffer[targetPos]);

    for (int offset = -searchRange; offset <= searchRange; ++offset)
    {
        int pos = (targetPos + offset + bufferSize) % bufferSize;
        float score = std::abs(buffer[pos]);

        // Prefer zero crossings (sign changes)
        int nextPos = (pos + 1) % bufferSize;
        if (buffer[pos] * buffer[nextPos] < 0.0f)
            score *= 0.01f; // Strong preference for zero crossing

        if (score < bestScore)
        {
            bestScore = score;
            bestPos = pos;
        }
    }

    return bestPos;
}
```

---

## 3. Crossfade Techniques

### 3.1 Equal-Power Crossfade

Essential for smooth transitions between live and frozen audio:

```cpp
struct EqualPowerCrossfade
{
    static void compute(float position, float& gainA, float& gainB)
    {
        // position: 0.0 = full A, 1.0 = full B
        gainA = std::cos(position * juce::MathConstants<float>::halfPi);
        gainB = std::sin(position * juce::MathConstants<float>::halfPi);
    }
};
```

### 3.2 Loop Crossfade

For buffer loop freeze, crossfade the loop boundaries to eliminate clicks:

```cpp
void applyLoopCrossfade(std::vector<float>& buffer, int crossfadeSamples)
{
    int length = (int)buffer.size();
    crossfadeSamples = juce::jmin(crossfadeSamples, length / 4);

    for (int i = 0; i < crossfadeSamples; ++i)
    {
        float t = (float)i / (float)crossfadeSamples;
        float fadeIn = t;
        float fadeOut = 1.0f - t;

        // Blend start with end
        int startIdx = i;
        int endIdx = length - crossfadeSamples + i;

        buffer[startIdx] = buffer[startIdx] * fadeIn + buffer[endIdx] * fadeOut;
    }
}
```

---

## Part 2: Freeze Approaches

## 4. Buffer Loop Freeze

### 4.1 Basic Loop Freeze

The simplest freeze: capture audio and loop it continuously:

```cpp
class LoopFreeze
{
public:
    void prepare(double sampleRate, float loopLengthMs)
    {
        sr = sampleRate;
        int loopSamples = static_cast<int>(loopLengthMs * 0.001f * sampleRate);
        loopBuffer.resize(loopSamples, 0.0f);
        crossfadeSamples = static_cast<int>(0.01f * sampleRate); // 10 ms crossfade
        readPos = 0;
    }

    float process(float input)
    {
        if (!frozen)
        {
            circularWrite(input);
            return input;
        }

        // Read from loop with crossfade at boundaries
        float output = readWithCrossfade();
        readPos = (readPos + 1) % (int)loopBuffer.size();

        // Mix frozen and live based on transition
        return output;
    }

    void setFrozen(bool shouldFreeze)
    {
        if (shouldFreeze && !frozen)
        {
            // Capture the current buffer state
            applyLoopCrossfade(loopBuffer, crossfadeSamples);
        }
        frozen = shouldFreeze;
    }

private:
    std::vector<float> loopBuffer;
    int readPos = 0;
    int writePos = 0;
    int crossfadeSamples = 0;
    bool frozen = false;
    double sr = 44100.0;

    void circularWrite(float sample)
    {
        loopBuffer[writePos] = sample;
        writePos = (writePos + 1) % (int)loopBuffer.size();
    }

    float readWithCrossfade()
    {
        float sample = loopBuffer[readPos];

        // Apply crossfade near loop boundary
        int distToEnd = (int)loopBuffer.size() - readPos;
        if (distToEnd <= crossfadeSamples)
        {
            float t = (float)distToEnd / (float)crossfadeSamples;
            float nextSample = loopBuffer[readPos - (int)loopBuffer.size() + crossfadeSamples];
            sample = sample * t + nextSample * (1.0f - t);
        }

        return sample;
    }
};
```

### 4.2 Pitch-Shifted Loop

Vary the loop playback speed for pitch effects on frozen material:

```cpp
float readPitchShifted(float playbackRate)
{
    float frac = fractionalReadPos - std::floor(fractionalReadPos);
    int idx0 = ((int)fractionalReadPos) % (int)loopBuffer.size();
    int idx1 = (idx0 + 1) % (int)loopBuffer.size();

    float sample = loopBuffer[idx0] * (1.0f - frac) + loopBuffer[idx1] * frac;

    fractionalReadPos += playbackRate;
    if (fractionalReadPos >= (float)loopBuffer.size())
        fractionalReadPos -= (float)loopBuffer.size();

    return sample;
}
```

---

## 5. Spectral Freeze (FFT-Based)

### 5.1 Phase Vocoder Architecture

Spectral freeze uses the STFT (Short-Time Fourier Transform) to lock the spectral content:

```
Input --> [Window] --> [FFT] --> [Freeze Magnitudes] --> [IFFT] --> [Window] --> [Overlap-Add] --> Output
```

### 5.2 The Freeze Algorithm

When freeze is activated:
1. Capture the current FFT frame's magnitudes
2. Stop updating magnitudes from input
3. Continue advancing phases to maintain smooth output
4. The frozen magnitudes create a sustained spectral snapshot

```cpp
class SpectralFreeze
{
public:
    static constexpr int FFTOrder = 11;          // 2048-point FFT
    static constexpr int FFTSize = 1 << FFTOrder; // 2048
    static constexpr int HopSize = FFTSize / 4;   // 75% overlap (512)

    void prepare(double sampleRate)
    {
        sr = sampleRate;
        std::fill(inputBuffer.begin(), inputBuffer.end(), 0.0f);
        std::fill(outputBuffer.begin(), outputBuffer.end(), 0.0f);
        std::fill(frozenMagnitudes.begin(), frozenMagnitudes.end(), 0.0f);
        std::fill(runningPhases.begin(), runningPhases.end(), 0.0f);
        inputWritePos = 0;
        outputReadPos = 0;
        hopCounter = 0;
    }

    void processFrame()
    {
        // Copy input to FFT buffer with Hann window
        for (int i = 0; i < FFTSize; ++i)
        {
            int idx = (inputWritePos - FFTSize + i + (int)inputBuffer.size())
                      % (int)inputBuffer.size();
            fftBuffer[i] = inputBuffer[idx] * hannWindow(i, FFTSize);
        }

        // Forward FFT
        fft.performRealOnlyForwardTransform(fftBuffer.data());

        if (frozen)
        {
            // Use frozen magnitudes, advance phases
            for (int bin = 0; bin <= FFTSize / 2; ++bin)
            {
                int re = bin * 2;
                int im = bin * 2 + 1;

                // Keep frozen magnitudes, advance phase
                float magnitude = frozenMagnitudes[bin];
                runningPhases[bin] += phaseIncrements[bin];

                fftBuffer[re] = magnitude * std::cos(runningPhases[bin]);
                fftBuffer[im] = magnitude * std::sin(runningPhases[bin]);
            }
        }
        else
        {
            // Normal processing: extract magnitudes and phases
            for (int bin = 0; bin <= FFTSize / 2; ++bin)
            {
                int re = bin * 2;
                int im = bin * 2 + 1;

                float magnitude = std::sqrt(fftBuffer[re] * fftBuffer[re]
                                          + fftBuffer[im] * fftBuffer[im]);
                float phase = std::atan2(fftBuffer[im], fftBuffer[re]);

                frozenMagnitudes[bin] = magnitude;
                runningPhases[bin] = phase;

                // Expected phase increment per hop for this bin
                phaseIncrements[bin] = 2.0f * juce::MathConstants<float>::pi
                                     * (float)bin * (float)HopSize / (float)FFTSize;
            }
        }

        // Inverse FFT
        fft.performRealOnlyInverseTransform(fftBuffer.data());

        // Window and overlap-add to output
        for (int i = 0; i < FFTSize; ++i)
        {
            int outIdx = (outputReadPos + i) % (int)outputBuffer.size();
            outputBuffer[outIdx] += fftBuffer[i] * hannWindow(i, FFTSize)
                                   * (2.0f / 3.0f); // Normalization for 75% overlap
        }
    }

    void activateFreeze()
    {
        frozen = true;
    }

    void deactivateFreeze()
    {
        frozen = false;
    }

private:
    juce::dsp::FFT fft{FFTOrder};
    std::array<float, FFTSize * 2> fftBuffer{};
    std::array<float, FFTSize> inputBuffer{};
    std::array<float, FFTSize * 2> outputBuffer{};
    std::array<float, FFTSize / 2 + 1> frozenMagnitudes{};
    std::array<float, FFTSize / 2 + 1> runningPhases{};
    std::array<float, FFTSize / 2 + 1> phaseIncrements{};

    int inputWritePos = 0;
    int outputReadPos = 0;
    int hopCounter = 0;
    bool frozen = false;
    double sr = 44100.0;

    static float hannWindow(int index, int size)
    {
        return 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi
                                       * (float)index / (float)(size - 1)));
    }
};
```

### 5.3 Phase Advancement Strategies

When frozen, phases must advance to produce continuous output. Three strategies:

**1. Fixed phase increment (most common):**
Each bin advances by its natural frequency increment per hop.

**2. Randomized phase (ethereal quality):**
Add random offsets to phases for a "shimmery" frozen texture.

```cpp
// Randomized phase for dreamy freeze
runningPhases[bin] += phaseIncrements[bin]
    + (juce::Random::getSystemRandom().nextFloat() - 0.5f) * randomAmount;
```

**3. Phase locked (original phase relationships):**
Maintain the original phase relationships between bins for a more coherent sound.

### 5.4 Overlap-Add Parameters

| Overlap | Hop Size (2048 FFT) | Quality | CPU Cost |
|---------|---------------------|---------|----------|
| 50% | 1024 | Moderate artifacts | Low |
| 75% | 512 | Good quality | Medium |
| 87.5% | 256 | Very smooth | High |

75% overlap (hop = FFTSize/4) is the standard for spectral freeze. Below 75%, artifacts become audible.

---

## 6. Granular Freeze

### 6.1 Granular Freeze Architecture

Granular freeze captures a buffer and plays it back using overlapping grains scattered across the buffer:

```cpp
class GranularFreeze
{
public:
    struct Grain
    {
        int startPos = 0;
        int currentPos = 0;
        int length = 0;
        float pan = 0.5f;
        bool active = false;

        float getEnvelope() const
        {
            if (!active || length == 0) return 0.0f;
            float progress = (float)currentPos / (float)length;
            // Tukey window (tapered cosine)
            float taperRatio = 0.3f;
            if (progress < taperRatio * 0.5f)
                return 0.5f * (1.0f + std::cos(juce::MathConstants<float>::pi
                       * (2.0f * progress / taperRatio - 1.0f)));
            if (progress > 1.0f - taperRatio * 0.5f)
                return 0.5f * (1.0f + std::cos(juce::MathConstants<float>::pi
                       * (2.0f * (progress - 1.0f + taperRatio * 0.5f) / taperRatio)));
            return 1.0f;
        }
    };

    void prepare(double sampleRate, float bufferLengthMs)
    {
        sr = sampleRate;
        int bufferSamples = static_cast<int>(bufferLengthMs * 0.001f * sampleRate);
        captureBuffer.resize(bufferSamples, 0.0f);
        grainLengthSamples = static_cast<int>(grainLengthMs * 0.001f * sampleRate);
        spawnCounter = 0;
    }

    void process(float input, float& outL, float& outR)
    {
        if (!frozen)
        {
            captureBuffer[captureWritePos] = input;
            captureWritePos = (captureWritePos + 1) % (int)captureBuffer.size();
            outL = input;
            outR = input;
            return;
        }

        // Spawn new grains
        if (--spawnCounter <= 0)
        {
            spawnGrain();
            spawnCounter = grainLengthSamples / grainOverlap;
        }

        // Sum active grains
        outL = 0.0f;
        outR = 0.0f;

        for (auto& grain : grains)
        {
            if (!grain.active) continue;

            int bufIdx = (grain.startPos + grain.currentPos) % (int)captureBuffer.size();
            float sample = captureBuffer[bufIdx] * grain.getEnvelope();

            outL += sample * (1.0f - grain.pan);
            outR += sample * grain.pan;

            grain.currentPos++;
            if (grain.currentPos >= grain.length)
                grain.active = false;
        }
    }

    bool frozen = false;
    float grainLengthMs = 80.0f;  // Grain duration
    float scatter = 0.5f;          // Random position scatter
    int grainOverlap = 4;          // Number of overlapping grains

private:
    static constexpr int MaxGrains = 32;
    std::array<Grain, MaxGrains> grains{};
    std::vector<float> captureBuffer;
    int captureWritePos = 0;
    int grainLengthSamples = 0;
    int spawnCounter = 0;
    double sr = 44100.0;
    juce::Random rng;

    void spawnGrain()
    {
        for (auto& grain : grains)
        {
            if (grain.active) continue;

            grain.startPos = rng.nextInt((int)captureBuffer.size());
            grain.currentPos = 0;
            grain.length = grainLengthSamples
                         + rng.nextInt(grainLengthSamples / 4) - grainLengthSamples / 8;
            grain.pan = 0.2f + rng.nextFloat() * 0.6f; // Spread in stereo
            grain.active = true;
            break;
        }
    }
};
```

### 6.2 Granular Freeze Parameters

| Parameter | Effect | Typical Range |
|-----------|--------|---------------|
| Grain size | Longer = more tonal, shorter = more textural | 20-200 ms |
| Scatter | How randomly grains are positioned in buffer | 0-100% |
| Overlap | Number of simultaneous grains | 2-8 |
| Pitch | Grain playback speed | 0.5-2.0x |
| Spray | Random variation in grain spacing | 0-100% |

---

## 7. Hybrid Approaches

### 7.1 Spectral + Granular

Combine spectral freeze for the tonal component with granular freeze for texture:

```cpp
float processHybrid(float input)
{
    float spectral = spectralFreeze.process(input);
    float granular = granularFreeze.process(input);

    // Blend: spectral provides the sustain, granular adds movement
    return spectral * (1.0f - granularAmount) + granular * granularAmount;
}
```

### 7.2 Convolution Freeze

Capture an impulse response of the frozen moment and convolve the input with it:
- Creates a "resonance" effect rather than a static freeze
- The frozen spectrum colors the input signal
- Natural decay when freeze is released

---

## Part 3: Advanced Topics

## 8. Spectral Processing During Freeze

### 8.1 Spectral Filtering

While frozen, manipulate the magnitude spectrum to shape the sound:

```cpp
void applySpectralTilt(float* magnitudes, int numBins, float tiltdBPerOctave, double sampleRate)
{
    float binFreqStep = (float)sampleRate / (float)(numBins * 2);

    for (int bin = 1; bin < numBins; ++bin)
    {
        float freq = (float)bin * binFreqStep;
        float octavesFromRef = std::log2(freq / 1000.0f); // Reference: 1 kHz
        float gaindB = octavesFromRef * tiltdBPerOctave;
        magnitudes[bin] *= juce::Decibels::decibelsToGain(gaindB);
    }
}
```

### 8.2 Spectral Blur

Smear the spectrum across neighboring bins for a diffuse, evolving sound:

```cpp
void spectralBlur(float* magnitudes, int numBins, float blurAmount)
{
    std::vector<float> blurred(numBins);

    int blurWidth = static_cast<int>(blurAmount * 20.0f) + 1;

    for (int bin = 0; bin < numBins; ++bin)
    {
        float sum = 0.0f;
        int count = 0;

        for (int offset = -blurWidth; offset <= blurWidth; ++offset)
        {
            int idx = bin + offset;
            if (idx >= 0 && idx < numBins)
            {
                sum += magnitudes[idx];
                count++;
            }
        }

        blurred[bin] = sum / (float)count;
    }

    std::copy(blurred.begin(), blurred.end(), magnitudes);
}
```

### 8.3 Spectral Shift

Shift the frozen spectrum up or down in frequency:

```cpp
void spectralShift(float* magnitudes, int numBins, int shiftBins)
{
    std::vector<float> shifted(numBins, 0.0f);

    for (int bin = 0; bin < numBins; ++bin)
    {
        int targetBin = bin + shiftBins;
        if (targetBin >= 0 && targetBin < numBins)
            shifted[targetBin] = magnitudes[bin];
    }

    std::copy(shifted.begin(), shifted.end(), magnitudes);
}
```

---

## 9. Stereo and Spatial Considerations

### 9.1 Mid-Side Spectral Freeze

Process mid and side channels independently for stereo-aware freeze:

```cpp
void processMidSideFreeze(float left, float right, float& outL, float& outR)
{
    float mid  = (left + right) * 0.5f;
    float side = (left - right) * 0.5f;

    float frozenMid  = midFreezer.process(mid);
    float frozenSide = sideFreezer.process(side);

    // Widen or narrow the frozen image
    frozenSide *= stereoWidth;

    outL = frozenMid + frozenSide;
    outR = frozenMid - frozenSide;
}
```

### 9.2 Independent Channel Freeze

Freeze left and right channels independently for maximum stereo width and decorrelation.

---

## 10. Transition Design

### 10.1 Freeze Activation Envelope

The transition into and out of freeze must be smooth:

```cpp
class FreezeTransition
{
public:
    void prepare(double sampleRate, float fadeTimeMs = 50.0f)
    {
        fadeSamples = static_cast<int>(fadeTimeMs * 0.001f * sampleRate);
        fadeCounter = 0;
        currentGain = 0.0f;
        targetGain = 0.0f;
    }

    void activate()   { targetGain = 1.0f; fadeCounter = fadeSamples; }
    void deactivate() { targetGain = 0.0f; fadeCounter = fadeSamples; }

    void process(float liveInput, float frozenOutput, float& output)
    {
        if (fadeCounter > 0)
        {
            float step = (targetGain - currentGain) / (float)fadeCounter;
            currentGain += step;
            fadeCounter--;
        }
        else
        {
            currentGain = targetGain;
        }

        // Equal-power crossfade between live and frozen
        float liveGain = std::cos(currentGain * juce::MathConstants<float>::halfPi);
        float freezeGain = std::sin(currentGain * juce::MathConstants<float>::halfPi);

        output = liveInput * liveGain + frozenOutput * freezeGain;
    }

private:
    int fadeSamples = 0;
    int fadeCounter = 0;
    float currentGain = 0.0f;
    float targetGain = 0.0f;
};
```

---

## Part 4: JUCE Implementation

## 11. JUCE FFT for Spectral Freeze

### 11.1 Using juce::dsp::FFT

```cpp
class JUCESpectralFreeze
{
public:
    static constexpr int FFTOrder = 11;
    static constexpr int FFTSize = 1 << FFTOrder; // 2048
    static constexpr int HopSize = FFTSize / 4;    // 512

    void prepare(double sampleRate)
    {
        sr = sampleRate;
        fifo.fill(0.0f);
        fifoIndex = 0;
        outputAccumulator.fill(0.0f);
        outputReadIndex = 0;

        setLatencySamples(FFTSize); // Report latency to host
    }

    void pushSample(float sample)
    {
        fifo[fifoIndex++] = sample;

        if (fifoIndex >= HopSize)
        {
            fifoIndex = 0;
            processFFTFrame();
        }
    }

    float popSample()
    {
        float output = outputAccumulator[outputReadIndex];
        outputAccumulator[outputReadIndex] = 0.0f;
        outputReadIndex = (outputReadIndex + 1) % (FFTSize * 2);
        return output;
    }

private:
    juce::dsp::FFT fft{FFTOrder};
    std::array<float, FFTSize> fifo{};
    std::array<float, FFTSize * 2> fftData{};
    std::array<float, FFTSize * 2> outputAccumulator{};
    std::array<float, FFTSize / 2 + 1> storedMagnitudes{};
    std::array<float, FFTSize / 2 + 1> storedPhases{};
    std::array<float, FFTSize / 2 + 1> phaseAdvance{};

    int fifoIndex = 0;
    int outputReadIndex = 0;
    double sr = 44100.0;
    bool frozen = false;

    void processFFTFrame()
    {
        // Prepare FFT data with window
        for (int i = 0; i < FFTSize; ++i)
        {
            float window = 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi
                                                    * (float)i / (float)FFTSize));
            int fifoIdx = (fifoIndex + i) % FFTSize;
            fftData[i] = fifo[fifoIdx] * window;
            fftData[i + FFTSize] = 0.0f;
        }

        fft.performRealOnlyForwardTransform(fftData.data());

        // Process bins
        for (int bin = 0; bin <= FFTSize / 2; ++bin)
        {
            float re = fftData[bin * 2];
            float im = fftData[bin * 2 + 1];
            float mag = std::sqrt(re * re + im * im);
            float phase = std::atan2(im, re);

            if (frozen)
            {
                mag = storedMagnitudes[bin];
                storedPhases[bin] += phaseAdvance[bin];
                phase = storedPhases[bin];
            }
            else
            {
                storedMagnitudes[bin] = mag;
                storedPhases[bin] = phase;
                phaseAdvance[bin] = 2.0f * juce::MathConstants<float>::pi
                                  * (float)bin * (float)HopSize / (float)FFTSize;
            }

            fftData[bin * 2]     = mag * std::cos(phase);
            fftData[bin * 2 + 1] = mag * std::sin(phase);
        }

        fft.performRealOnlyInverseTransform(fftData.data());

        // Window and overlap-add
        for (int i = 0; i < FFTSize; ++i)
        {
            float window = 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi
                                                    * (float)i / (float)FFTSize));
            int outIdx = (outputReadIndex + i) % (FFTSize * 2);
            outputAccumulator[outIdx] += fftData[i] * window * (2.0f / 3.0f);
        }
    }
};
```

---

## 12. Complete Freeze Plugin Architecture

### 12.1 Plugin Processor

```cpp
class FreezeProcessor : public juce::AudioProcessor
{
public:
    FreezeProcessor()
        : AudioProcessor(BusesProperties()
            .withInput("Input", juce::AudioChannelSet::stereo(), true)
            .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    {
    }

    void prepareToPlay(double sampleRate, int samplesPerBlock) override
    {
        for (int ch = 0; ch < 2; ++ch)
            spectralFreeze[ch].prepare(sampleRate);

        transition.prepare(sampleRate, 50.0f);

        setLatencySamples(JUCESpectralFreeze::FFTSize);
    }

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        juce::ScopedNoDenormals noDenormals;

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            {
                float input = buffer.getSample(ch, i);
                spectralFreeze[ch].pushSample(input);
                float frozen = spectralFreeze[ch].popSample();

                float output;
                transition.process(input, frozen, output);
                buffer.setSample(ch, i, output);
            }
        }
    }

private:
    JUCESpectralFreeze spectralFreeze[2];
    FreezeTransition transition;
};
```

---

## 13. Optimization and Real-Time Safety

### 13.1 FFT Performance

- Use power-of-2 FFT sizes for optimal performance
- JUCE's FFT uses vDSP (macOS) or FFTW-style implementations
- 2048-point FFT at 44.1 kHz takes ~0.1 ms per frame

### 13.2 Memory Considerations

Pre-allocate all buffers in `prepareToPlay()`:

```cpp
void prepareToPlay(double sampleRate, int samplesPerBlock) override
{
    // All allocations here
    captureBuffer.resize(captureLength);
    outputBuffer.resize(outputLength);
    // Never allocate in processBlock
}
```

### 13.3 Common Pitfalls

| Pitfall | Impact | Solution |
|---------|--------|----------|
| No crossfade on freeze activation | Click/pop | 20-100 ms equal-power crossfade |
| Overlap-add < 75% | Audible flutter/pulsing | Use 75% overlap (hop = FFTSize/4) |
| Not reporting latency | DAW timing offset | Call setLatencySamples() in prepareToPlay() |
| Modifying FFT buffer during transform | Corruption | Use separate input/output buffers |
| Denormals in frozen output | CPU spike | ScopedNoDenormals + denormal guards |

---

## Part 5: References

## 14. References and Further Reading

### Academic Papers
- Dolson, M. (1986). "The Phase Vocoder: A Tutorial." Computer Music Journal, 10(4).
- Laroche, J. & Dolson, M. (1999). "Improved Phase Vocoder Time-Scale Modification." IEEE Trans. Speech Audio Processing.
- Roads, C. (2001). "Microsound." MIT Press. Chapter on granular time-stretching and freezing.

### Books
- Smith, J.O. "Spectral Audio Signal Processing." Online book, Stanford.
- Zolzer, U. (2011). *DAFX: Digital Audio Effects*. Chapter 8: Time-Frequency Processing.
- Pirkle, W. (2019). *Designing Audio Effect Plugins in C++*. Chapter 19: Spectral Processing.

### Software References
- `juce::dsp::FFT` for FFT computation
- `juce::dsp::Oversampling` for quality enhancement
- Paulstretch: Open-source extreme time-stretch/freeze algorithm (Nasca Octavian Paul)

### Commercial References
- Soundtoys Crystallizer (granular freeze/pitch shift)
- GRM Tools Freeze (IRCAM spectral freeze)
- Output Portal (granular freeze effects)
- Unfiltered Audio Sandman (buffer freeze and manipulation)

---

*Research document for O-Freeze. Covers spectral freeze via FFT, buffer loop freeze, granular freeze, and hybrid approaches with JUCE implementation.*
