---
title: "Stutter Audio Effects Research Findings"
created: 2026-01-15
last_verified: 2026-02-06
juce_version: "8.0.4"
summary: "Overview of stutter effect implementation patterns comparing buffer capture and loop, granular engine, and playhead modulation architectures with CPU and latency trade-offs for JUCE plugins."
domain: dsp
type: research
keywords:
  - stutter
  - stutter-effects
  - granular
  - buffer-capture
  - beat-repeat
  - playhead-modulation
  - juce
stages: [0, 2]
agents: [dsp, research]
---

# Stutter Audio Effects - Research Findings

**Research Date:** 2026-01-14
**Research Level:** Level 3 (Parallel Deep Investigation)
**Topic:** Stutter audio effects implementation for JUCE plugins

---

## Executive Summary

Stutter effects can be implemented using three primary architectural patterns:

| Pattern | Best For | CPU | Latency |
|---------|----------|-----|---------|
| **Buffer Capture + Loop** | Beat repeat, rhythmic stutter | Low | Low-Medium |
| **Granular Engine** | Textural, pitch-shifted fragments | High | Low |
| **Playhead Modulation** | Tape stop, scratch, speed variation | Medium | Zero |

---

## Level 1 Findings: Local Knowledge Base

### Existing Implementation: Scatter Plugin

Found complete working granular stutter implementation at:
`plugins/tache_plugins/Scatter/`

**Architecture:** `plugins/tache_plugins/Scatter/.ideas/architecture.md`
**Implementation:** `plugins/tache_plugins/Scatter/Source/PluginProcessor.cpp`

#### Key Components Already Implemented

| Component | JUCE Class | Purpose |
|-----------|------------|---------|
| Granular Delay Buffer | `juce::dsp::DelayLine<float, Lagrange3rd>` | Store audio for grain reading |
| Grain Voice Engine | Custom `std::array<GrainVoice, 64>` | Manage 64 simultaneous grains |
| Grain Scheduler | Sample counter + spawn interval | Trigger grains based on density |
| Window Function | `juce::dsp::WindowingFunction<float>` | Hann envelope (anti-click) |
| Pitch Shifter | `rate = 2^(semitones/12)` | Playback rate adjustment |
| Scale Quantization | Lookup tables | Snap pitch to musical scales |
| Dry/Wet Mixer | `juce::dsp::DryWetMixer<float>` | Blend control |

#### Core Algorithm (from PluginProcessor.cpp:400-489)

```cpp
for (auto& grain : grainVoices) {
    if (!grain.active) continue;

    // 1. Read from delay buffer at grain position
    float delayedSample = delayBuffer.popSample(0, grain.readPosition);

    // 2. Apply Hann window envelope (anti-click)
    float grainOutput = delayedSample * hannWindow[windowIndex];

    // 3. Apply stereo pan
    leftData[sample] += grainOutput * (1.0f - grain.pan);
    rightData[sample] += grainOutput * grain.pan;

    // 4. Advance positions (forward or reverse)
    grain.readPosition += grain.reverse ? -grain.playbackRate : grain.playbackRate;
    grain.windowPosition += 1.0f / grain.grainSizeSamples;
}
```

---

## Level 2 Findings: External Sources

### JUCE Forum: Beat Repeater Discussion

**Source:** https://forum.juce.com/t/beat-repeater-stutter-effect/44969

**Problem:** Clicking artifacts at loop boundaries when implementing Ableton-style beat repeat.

**Solution:** Overlapping loops with crossfade (~80ms window). The forum discussion revealed that Ableton's Beat Repeat starts recording ~40 samples before the delay point to enable smooth crossfades.

**Key Quote:**
> "You need to overlap your loops a bit and apply a crossfade" - solution to clicking artifacts

### JUCE Forum: Granular Learning Resources

**Source:** https://forum.juce.com/t/advice-for-learning-about-granular-microsound/60359

**Recommendation:** "Start by making a simple delay with a ring buffer. It is the basic building block of all granular effects."

**Resources Mentioned:**
- "Microsound" by Curtis Roads (essential reading)
- Ross Bencina's granular synthesis paper

### Open Source Projects Found

| Project | Stars | Architecture | Link |
|---------|-------|--------------|------|
| **Argotlunar** | 184 | Granular delay, 64 grains, pitch/filter | [GitHub](https://github.com/mourednik/argotlunar) |
| **Delayyyyyy** | - | Beat repeater with BPM sync | [GitHub](https://github.com/ejaaskel/Delayyyyyy) |
| **RSBrokenMedia** | - | Buffer subdivision glitch | [GitHub](https://github.com/reillypascal/RSBrokenMedia) |
| **TIME-12** | - | Envelope-modulated stutter/scratch | [GitHub](https://github.com/tiagolr/time12) |
| **FftBuffer** | - | Phase vocoder stutter with tempo sync | [GitHub](https://github.com/maxsolomonhenry/FftBuffer) |

---

## Level 3 Findings: Parallel Deep Research

### Research Thread 1: DSP Algorithms

#### Time-Domain Techniques

**Buffer Freezing Algorithm:**
```cpp
class BufferFreeze {
    void process(float input, float& output) {
        if (!frozen) {
            buffer[writePos] = input;
            writePos = (writePos + 1) % bufferSize;
            output = input;
        } else {
            output = buffer[readPos];
            readPos = (readPos + 1) % freezeLength;
        }
    }
};
```

**Crossfade Mathematics:**

| Type | Formula | Use Case |
|------|---------|----------|
| Linear | `gainA = 1.0 - t; gainB = t` | Simple, but 3dB dip at center |
| Equal-Power | `gainA = cos(t * π/2); gainB = sin(t * π/2)` | Constant perceived loudness |

Equal-power crossfade is preferred: `sin²(θ) + cos²(θ) = 1` ensures constant power throughout transition.

**Zero-Crossing Detection:**
- Cuts at zero crossings minimize discontinuities
- Limitation: Still creates derivative discontinuity (audible)
- Recommendation: Use crossfading instead for smoother transitions

**Overlap-Add (OLA) Method:**
- COLA constraint: overlapping windows must sum to constant
- Common COLA-compliant configs: Hann window with 50% overlap

#### Granular Synthesis Fundamentals

**Scheduling Algorithms:**

| Type | Character | Use Case |
|------|-----------|----------|
| Synchronous | Pitched, periodic | Pitch shifting, time stretch |
| Asynchronous (Stochastic) | Textural, cloud-like | Ambient textures |

**Window Function Selection:**

| Window | Main Lobe | Side Lobes | Click-Free | Recommendation |
|--------|-----------|------------|------------|----------------|
| Hann | Medium | -31 dB | Excellent | "When in doubt, choose Hann" |
| Gaussian | Narrow | Very low | Poor (needs truncation) | Spectral precision |
| Tukey | Configurable | Configurable | Good | Flat top with tapers |
| Rectangular | Narrowest | -13 dB | Poor | Lo-fi effects only |

**Pitch Shifting: Playback Rate vs Phase Vocoder:**

| Aspect | Playback Rate | Phase Vocoder |
|--------|---------------|---------------|
| Latency | ~0 | FFT size (2048+ samples) |
| CPU | Very low | High |
| Quality (pitched) | Good | Excellent |
| Quality (transients) | Excellent | Can smear |
| Independence | Pitch = Time | Pitch ≠ Time |

**Recommendation:** Use playback rate for granular (standard approach in Portal, GrainScanner).

#### Beat-Synchronized Effects

**PPQ-Based Timing:**
```cpp
// Samples per beat calculation
float samplesPerBeat = (60.0f / bpm) * sampleRate;

// Note value to samples
float noteToSamples(float noteValue) {
    return samplesPerBeat * 4.0f * noteValue;
    // noteValue: 1.0 = whole, 0.25 = quarter, etc.
}
```

**Subdivision Values (in beats):**
- 1/4 note: 1.0
- 1/8 note: 0.5
- 1/16 note: 0.25
- 1/8 triplet: 1.0/3.0
- 1/16 triplet: 1.0/6.0

**Euclidean Rhythm Algorithm:**
Mathematically distributes pulses as evenly as possible. Example: `euclidean(8, 3) = [1,0,0,1,0,0,1,0]` (Cuban tresillo).

#### Anti-Artifact Techniques

**Micro-Fades (1-4ms):**
```cpp
// At 44100 Hz, 4ms = ~176 samples
int fadeSamples = (int)(0.004f * sampleRate);
```

**Denormal Handling:**
```cpp
// Option 1: JUCE helper (preferred)
juce::ScopedNoDenormals noDenormals;

// Option 2: CPU flags
_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
_MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
```

**Buffer Boundary Wrapping:**
- Power-of-2 sizes: Use AND masking (`pos & mask`) for efficiency
- Arbitrary sizes: Use modulo with negative handling

---

### Research Thread 2: Commercial Plugin Analysis

#### Sugar Bytes Effectrix 2
- **Architecture:** Step sequencer-driven multi-effect (14 effects × 32 steps)
- **Key Feature:** Dual modulation sequencers per effect
- **Stutter Implementation:** Looper A (pitch-focused) + Looper B (speed-based) + Grain Engine
- **Tempo Sync:** Plain (straight), All (dotted/triplet), Milliseconds (free)

#### iZotope Stutter Edit 2
- **Architecture:** MIDI gesture triggering ("playable effect")
- **Key Feature:** Time-Variant Modifiers (TVMs) - any parameter modulated over gesture timeline
- **Buffer Mechanics:** Continuous sampling, stutter controls determine slice count/speed
- **Known Issue:** Works best at 128 sample buffer size

#### Image-Line Gross Beat
- **Architecture:** Spline-based dual envelope (time + volume) over 2-bar rolling buffer
- **Key Feature:** Time envelope controls playhead position (top = realtime, bottom = 2 bars ago)
- **Scratching:** Curves moving forward/backward create vinyl effects
- **Limitation:** First 4 beats produce no output when using reverse (buffer must fill)

#### Cableguys TimeShaper
- **Architecture:** Drawable LFO controlling virtual playhead position
- **Key Feature:** Multiband processing (separate LFOs for low/mid/high)
- **Quality:** -160dB noise ratio interpolation
- **Smooth Step Mode:** Minimizes clicks during time jumps

#### Output Portal
- **Architecture:** Granular synthesis + musical scale quantization
- **Key Feature:** Scale lock constrains pitch to selected musical scale
- **Grains:** User-defined size (can go below 1ms), real-time buffer slicing
- **Control:** Circular XY pad linked to granular parameters

#### Native Instruments Replika XT
- **Architecture:** Multi-algorithm delay with 5 modes
- **Diffusion Style:** Combines delay with allpass diffusion network
- **Key Feature:** Amount, Size, Movement controls for textural delays
- **Range:** 1ms to 2000ms (0ms creates resonator effects)

---

### Research Thread 3: Advanced JUCE Patterns

#### Real-Time Buffer Manipulation

**juce::dsp::DelayLine Interpolation Types:**
- `None`: Lo-fi effects, integer delays only
- `Linear`: Low CPU, introduces low-pass filtering
- `Lagrange3rd`: Better quality, good for modulation (recommended)
- `Thiran`: Allpass-based, preserves high frequencies

**AbstractFifo for Lock-Free Streaming:**
Essential for streaming buffer state to UI visualization without blocking audio thread.

#### Tempo Synchronization

**AudioPlayHead::PositionInfo Usage:**
```cpp
auto posInfo = playHead->getPosition();
double bpm = posInfo->getBpm().orFallback(120.0);
double ppq = posInfo->getPpqPosition().orFallback(0.0);
bool isPlaying = posInfo->getIsPlaying();
```

**Offline Rendering Fallback:**
When playhead unavailable, track position manually with sample counter using last known BPM.

#### Thread-Safe Visualization

**Atomic Metering Pattern:**
```cpp
// Audio thread: update peak
float expected = peakLevel.load();
while (newLevel > expected) {
    if (peakLevel.compare_exchange_weak(expected, newLevel))
        break;
}

// UI thread: read and reset
float peak = peakLevel.exchange(0.0f);
```

**Timer-Based Updates:**
- Use `startTimerHz(60)` for 60 FPS visualization
- Only repaint if state changed (reduce CPU)

#### Parameter Automation

**SmoothedValue for Click-Free Changes:**
```cpp
mixSmoothed.reset(sampleRate, 0.01);  // 10ms smoothing
mixSmoothed.setTargetValue(newValue);

// In processBlock:
float currentMix = mixSmoothed.getNextValue();
```

**Latency Reporting:**
For beat-aligned effects, report lookahead via `setLatencySamples()`. Note: Some hosts only check on plugin load.

#### CPU Optimization

**FloatVectorOperations for SIMD:**
```cpp
juce::FloatVectorOperations::copy(dest, src, numSamples);
juce::FloatVectorOperations::multiply(data, gain, numSamples);
juce::FloatVectorOperations::add(dest, src, numSamples);
```

**Voice Stealing Strategy:**
Score voices by: age × 0.5 + (1 - amplitude) × 0.3 + (10 - priority) × 0.2. Steal highest-scoring voice.

**Early-Exit Patterns:**
- Skip inactive voices
- Check for silent input
- Use different processing strategies based on active voice count

---

## Recommended Parameter Ranges

| Parameter | Minimum | Typical | Maximum |
|-----------|---------|---------|---------|
| Grain duration | 10 ms | 50-100 ms | 500 ms |
| Grain density | 1 Hz | 10-50 Hz | 1000 Hz |
| Crossfade length | 1 ms | 5-10 ms | 50 ms |
| Zero-crossing search | 10 samples | 50 samples | 200 samples |
| Buffer size (stutter) | 64 samples | 4096-16384 | 262144 |

---

## Key Takeaways

1. **Crossfade everything:** Loop boundaries, preset changes, parameter transitions
2. **Buffer first, then manipulate:** All approaches sample audio before processing
3. **Tempo sync is non-negotiable:** Users expect rhythmic precision
4. **Hann window is default:** "When in doubt, choose Hann" for grain envelopes
5. **Playback rate pitch shifting:** Standard for granular (not phase vocoder)
6. **Quality interpolation matters:** Lagrange3rd minimum for modulated delays
7. **Denormal protection required:** Use `ScopedNoDenormals` in every processBlock

---

## Sources

### Academic/Technical
- Ross Bencina: "Implementing Real-Time Granular Synthesis"
- Curtis Roads: "Microsound"
- Will Pirkle: "Designing Audio Effect Plugins in C++"
- Signalsmith Audio: "Cheap Energy-Preserving Crossfade"

### JUCE Resources
- [dsp::DelayLine](https://docs.juce.com/master/classdsp_1_1DelayLine.html)
- [AbstractFifo](http://docs.juce.com/master/classAbstractFifo.html)
- [AudioPlayHead::PositionInfo](https://docs.juce.com/master/classAudioPlayHead_1_1PositionInfo.html)
- [SmoothedValue](https://docs.juce.com/master/classSmoothedValue.html)
- [FloatVectorOperations](https://docs.juce.com/master/classFloatVectorOperations.html)

### Forum Discussions
- [Beat repeater (Stutter effect)](https://forum.juce.com/t/beat-repeater-stutter-effect/44969)
- [Advice for learning about granular/microsound](https://forum.juce.com/t/advice-for-learning-about-granular-microsound/60359)
- [Lock-free queues and visualization](https://forum.juce.com/t/lock-free-queues-and-visualization-of-data/20659)

### Open Source Projects
- [Argotlunar](https://github.com/mourednik/argotlunar) - GPL granular delay
- [TIME-12](https://github.com/tiagolr/time12) - Envelope stutter
- [RSBrokenMedia](https://github.com/reillypascal/RSBrokenMedia) - Glitch plugin
- [Delayyyyyy](https://github.com/ejaaskel/Delayyyyyy) - Beat repeater
