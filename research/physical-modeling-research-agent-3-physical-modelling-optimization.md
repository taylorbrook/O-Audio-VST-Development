---
title: "Physical Modelling Synthesis: Performance Optimization and Production Guide"
created: 2026-01-09
last_verified: 2026-02-06
juce_version: "8.0.4"
summary: "Practical guide to CPU optimization, SIMD vectorization, algorithm complexity analysis, and hybrid approaches for physical modelling synthesis in real-time audio plugins."
domain: dsp
type: guide
keywords:
  - physical-modeling
  - cpu-optimization
  - simd
  - performance
  - delay-lines
  - waveguide
  - real-time
  - benchmarks
stages: [2]
agents: [dsp]
---

# Physical Modelling Synthesis: Performance Optimization and Production Guide

## Research Agent 3 Report
**Focus: Practical Optimization, Commercial Analysis, and Implementation Strategies**

---

## Table of Contents
1. [CPU Optimization Fundamentals](#1-cpu-optimization-fundamentals)
2. [Algorithm Complexity Analysis](#2-algorithm-complexity-analysis)
3. [Commercial Physical Modelling Analysis](#3-commercial-physical-modelling-analysis)
4. [Effects vs Instruments: Design Patterns](#4-effects-vs-instruments-design-patterns)
5. [Production Techniques and Hybrid Approaches](#5-production-techniques-and-hybrid-approaches)
6. [Modern Advances (2020-2026)](#6-modern-advances-2020-2026)
7. [Practical Implementation Roadmap](#7-practical-implementation-roadmap)
8. [Performance Benchmarks and Expectations](#8-performance-benchmarks-and-expectations)
9. [Optimization Checklist](#9-optimization-checklist)
10. [Actionable Recommendations](#10-actionable-recommendations)

---

## 1. CPU Optimization Fundamentals

### 1.1 Understanding PM Computational Costs

Physical modelling synthesis is inherently more CPU-intensive than sample-based synthesis because:

1. **Real-time calculation**: Every sample must be computed mathematically
2. **Interconnected systems**: Physical models often have feedback loops and coupled equations
3. **Non-linear behavior**: Realistic physics requires expensive non-linear calculations
4. **High sample rates**: Many PM algorithms require internal oversampling for stability

### 1.2 SIMD Optimization Opportunities

**Single Instruction Multiple Data (SIMD)** is crucial for PM performance:

```cpp
// Non-SIMD (scalar) - processes 1 sample at a time
for (int i = 0; i < numSamples; ++i) {
    output[i] = delayLine[readIndex] * dampingCoeff;
    delayLine[writeIndex] = input[i] + output[i] * feedback;
}

// SIMD-optimized - processes 4 samples simultaneously
for (int i = 0; i < numSamples; i += 4) {
    __m128 delayed = _mm_loadu_ps(&delayLine[readIndex]);
    __m128 damped = _mm_mul_ps(delayed, _mm_set1_ps(dampingCoeff));
    __m128 inp = _mm_loadu_ps(&input[i]);
    __m128 fb = _mm_mul_ps(damped, _mm_set1_ps(feedback));
    _mm_storeu_ps(&delayLine[writeIndex], _mm_add_ps(inp, fb));
    _mm_storeu_ps(&output[i], damped);
}
```

**SIMD Opportunities in PM:**

| Algorithm Component | SIMD Potential | Notes |
|---------------------|----------------|-------|
| Delay lines | High | Process multiple taps in parallel |
| Filters (linear) | High | Biquad cascades, parallel topology |
| Waveguides | Medium | Dependent on junction topology |
| Non-linear elements | Low-Medium | Often requires scalar processing |
| Resonator banks | High | Process multiple resonators in parallel |
| Modal synthesis | Very High | Each mode is independent |

### 1.3 Voice Count and Polyphony Considerations

**Typical Voice Counts by PM Type:**

| PM Type | Realistic Polyphony | CPU per Voice |
|---------|---------------------|---------------|
| Simple Karplus-Strong | 64-128 voices | ~0.1-0.3% |
| Enhanced string model | 16-32 voices | ~0.5-1% |
| Waveguide wind | 8-16 voices | ~1-2% |
| Full piano model | 1-4 voices | ~5-15% |
| Complex percussion | 8-24 voices | ~0.5-2% |

**Polyphony Management Strategies:**

1. **Voice stealing**: Intelligently steal voices based on amplitude/age
2. **Dynamic complexity**: Reduce model complexity for older/quieter voices
3. **Pre-voice limiting**: Set hard limits based on CPU headroom
4. **Adaptive quality**: Monitor CPU and reduce quality globally if needed

### 1.4 Oversampling Considerations

Physical models often require internal oversampling:

**When Oversampling is Necessary:**
- Non-linear elements (waveshaping, saturation)
- Feedback systems prone to aliasing
- High-frequency content generation
- Impulse/transient generation

**Oversampling Costs:**

| Factor | 2x OS | 4x OS | 8x OS |
|--------|-------|-------|-------|
| CPU multiplier | ~2.2x | ~4.5x | ~9x |
| Latency added | Minimal | Low | Moderate |
| Quality improvement | Good | Very Good | Excellent |

**Recommendation**: Use 2x-4x internal oversampling for non-linear PM elements, with efficient polyphase filters.

---

## 2. Algorithm Complexity Analysis

### 2.1 Computational Complexity by Algorithm

**Big-O Analysis of Common PM Algorithms:**

| Algorithm | Time Complexity | Space Complexity | Notes |
|-----------|-----------------|------------------|-------|
| Karplus-Strong | O(n) | O(d) | d = delay length |
| Waveguide string | O(n) | O(d) | Linear with delay |
| Digital Waveguide mesh | O(n*m) | O(n*m) | n,m = mesh dimensions |
| Modal synthesis | O(n*k) | O(k) | k = number of modes |
| Finite Difference | O(n*g^2) | O(g^2) | g = grid resolution |
| Mass-spring | O(n*m^2) | O(m) | m = number of masses |

### 2.2 Cost Breakdown by Component

**Typical CPU Distribution in a String Model:**

```
Exciter (impulse/noise generation):     5-10%
Delay line read/write:                  10-15%
Filtering (damping, body):              30-40%
Non-linear elements:                    20-30%
Output mixing/spatialization:           5-10%
Parameter interpolation:                5-10%
```

### 2.3 Optimization Priority Matrix

**Impact vs Effort for Optimization:**

| Optimization | CPU Impact | Implementation Effort | Priority |
|--------------|------------|----------------------|----------|
| SIMD for filters | High | Medium | 1 |
| Lookup tables for non-linear | High | Low | 1 |
| Voice pooling | Medium-High | Medium | 2 |
| Delay line interpolation quality | Medium | Low | 2 |
| Branch prediction optimization | Low-Medium | Low | 3 |
| Cache-friendly memory layout | Medium | High | 3 |
| GPU offloading | High | Very High | 4 |

---

## 3. Commercial Physical Modelling Analysis

### 3.1 Modartt Pianoteq

**Company Background:**
- Founded 2004, Toulouse, France
- Spinoff from MUSIC Technology Group research
- Focus: Concert-quality physically modelled pianos

**Technical Approach:**
- Uses advanced waveguide synthesis for strings
- Models hammer-string interaction with multi-phase contact
- Soundboard modelled as resonating plate
- Real-time calculation of all sympathetic resonances

**Key Design Decisions:**

1. **No samples used**: Pure PM, resulting in ~50MB install vs 50GB+ for sampled pianos
2. **Infinite sustain**: Model doesn't decay unless physics dictates
3. **Continuous parameters**: Everything is smoothly adjustable
4. **Stretch tuning as parameter**: Not baked in like samples

**Parameter Design Philosophy:**
```
Macro Controls:          Micro Controls:
- Dynamics               - Hammer hardness curve
- Tone                   - String length scaling
- Condition              - Soundboard impedance
                         - Duplex scale resonance
                         - Una corda simulation
```

**CPU Strategy:**
- Intelligent voice allocation (88 strings always "available")
- Sympathetic resonance computed efficiently via frequency-domain coupling
- ~10-25% CPU on modern systems for full polyphony
- Quality presets: Draft (fast) to Studio (full model)

**Success Factors:**
1. Uncompromised physical accuracy
2. Tiny installation size
3. Parameters that make physical sense
4. Extensive instrument variety from one engine

### 3.2 Applied Acoustics Systems (AAS)

**Product Line:**
- **Chromaphone**: Percussion/mallet physical modelling
- **String Studio**: Bowed/plucked string modelling
- **Ultra Analog**: Analog-style synth with PM elements
- **Lounge Lizard**: Electric piano modelling

**Technical Approach:**
- Resonator-based architecture
- Modular exciter -> resonator -> body design
- Mix of waveguide and modal synthesis

**Chromaphone 3 Architecture:**

```
[Mallet/Noise/Stick] --> [Resonator A] --> [Body] --> Output
         |                    |              |
         v                    v              v
    [Exciter Mix]      [Resonator B]    [Effects]
```

**Key Design Decisions:**

1. **Two-resonator system**: Allows complex coupled resonances
2. **Multiple exciter types**: Mallet, noise, stick, bow simulation
3. **Material-based resonators**: "Wood", "Metal", "Nylon" presets map to PM parameters
4. **Rich effect chain**: Acknowledges PM needs post-processing

**Parameter Design:**
- Surface level: Material, Size, Tone, Decay
- Deep level: Full access to waveguide parameters
- Sweet spot: Middle ground for sound designers

**CPU Strategy:**
- Fixed voice allocation (16-32 typical)
- No per-sample non-linearity in resonators (linear filter-based)
- Most CPU in the exciter and coupling stages

### 3.3 What Makes Successful PM Instruments

**Common Success Patterns:**

1. **Layered UI Complexity**
   ```
   Level 1: Macro controls (Size, Brightness, Decay)
   Level 2: Component controls (Exciter, Body, Resonator)
   Level 3: Expert controls (Individual physical parameters)
   ```

2. **Meaningful Parameter Mapping**
   - "Brightness" maps to multiple filter cutoffs, damping coefficients
   - "Size" scales delay lengths, resonator frequencies, body response
   - Users think in acoustic terms, not DSP terms

3. **High-Quality Defaults**
   - Factory presets demonstrate full potential
   - Start with acoustically plausible settings
   - Avoid "synthetic" defaults that expose the model's weaknesses

4. **CPU Transparency**
   - Clear voice count indication
   - Quality/CPU trade-off controls
   - Graceful degradation under load

5. **Physical Coherence**
   - Parameters that "make sense together"
   - Impossible physical states avoided or handled gracefully
   - Internal consistency even when pushing extremes

---

## 4. Effects vs Instruments: Design Patterns

### 4.1 Physical Modelling for Effects

**PM Effect Types:**

1. **Reverb (Physical Space Modelling)**
   - Feedback Delay Networks (FDN)
   - Waveguide mesh reverbs
   - Modal reverbs (room modes)
   - Example: Exponential Audio products, Valhalla Room

2. **Resonators**
   - Comb filter banks
   - Karplus-Strong without excitation
   - Sympathetic string simulation
   - Example: Ableton Resonator, Soundtoys FilterFreak

3. **Cabinet/Speaker Simulation**
   - Impulse response convolution (not strictly PM)
   - Real-time speaker cone modelling
   - Example: Neural DSP, Line 6 Helix

4. **Body Resonance**
   - Acoustic body modelling for DI recordings
   - Apply "acoustic character" to electronic sounds
   - Example: Waves IR-based body modelling

**Effect-Specific Considerations:**

| Aspect | Effects | Instruments |
|--------|---------|-------------|
| Latency tolerance | Higher | Very low |
| CPU budget | Often generous | Per-voice limited |
| State complexity | Session-level | Per-note-level |
| Parameter changes | Smooth, slow | Instantaneous |
| Non-linearity | Often subtle | Often essential |

### 4.2 PM Instrument Architectures

**Exciter -> Resonator -> Body Pattern:**

```cpp
class PMInstrumentVoice {
    Exciter exciter;        // Impulse, noise, bow, hammer
    Resonator resonator;    // String, tube, membrane, bar
    Body body;              // Soundboard, bell, room

    float process(float excitation) {
        float resonated = resonator.process(exciter.process(excitation));
        return body.process(resonated);
    }
};
```

**Common Exciter Types:**

| Exciter | Use Case | Complexity |
|---------|----------|------------|
| Impulse | Plucked strings, struck bars | Low |
| Filtered noise | Bowed strings, wind | Medium |
| Non-linear hammer | Piano, mallets | High |
| Reed model | Woodwinds | High |
| Lip model | Brass | Very High |

**Common Resonator Types:**

| Resonator | Use Case | Complexity |
|-----------|----------|------------|
| Delay + filter | Karplus-Strong string | Low |
| Waveguide pair | Realistic string | Medium |
| Waveguide mesh | 2D surfaces | High |
| Modal bank | Bars, bells, plates | Medium |
| Finite difference | Accurate 2D/3D | Very High |

### 4.3 Hybrid Designs

**Sample + PM Hybrid:**

```
Sample Layer:           PM Layer:
[Attack sample]    +    [Sustained resonance]
[Body impulse]     *    [Real-time filtering]
[Noise texture]    +    [Modal synthesis tail]
```

**Advantages of Hybrid:**
1. Attack realism from samples
2. Sustain flexibility from PM
3. Lower CPU than pure PM
4. Best of both worlds

**Examples:**
- **Spectrasonics Keyscape**: Sample attacks + modelled resonance
- **Arturia Piano V**: Sample layer + PM sympathetic strings
- **Native Instruments**: Sample engine + convolution body modelling

---

## 5. Production Techniques and Hybrid Approaches

### 5.1 Pre-computed vs Real-time

**Pre-computation Opportunities:**

| Element | Pre-compute? | Storage Cost | Quality Benefit |
|---------|--------------|--------------|-----------------|
| Body impulse responses | Yes | 10-100KB each | Very high |
| Excitation templates | Sometimes | 1-10KB each | Medium |
| Filter coefficients | Per-note | Minimal | N/A |
| Mode frequencies/amplitudes | Per-instrument | Minimal | High |
| Lookup tables | Always | 4-64KB | High |

**Effective Pre-computation Strategy:**

```cpp
// Pre-compute body impulse response at load time
void PMInstrument::prepareToPlay(double sampleRate) {
    // Calculate body filter coefficients once
    bodyFilter.calculateCoefficients(sampleRate, bodyParams);

    // Pre-compute mode frequencies for this sample rate
    for (int i = 0; i < numModes; ++i) {
        modes[i].omega = 2.0 * PI * modeFrequencies[i] / sampleRate;
        modes[i].decay = exp(-modeDamping[i] / sampleRate);
    }

    // Build lookup tables for non-linear functions
    for (int i = 0; i < TABLE_SIZE; ++i) {
        float x = (float)i / TABLE_SIZE * 2.0f - 1.0f;
        saturationTable[i] = tanh(x * driveAmount);
    }
}
```

### 5.2 Parameter Reduction Strategies

**The Parameter Explosion Problem:**

A physically accurate piano model might have:
- 88 strings x 3 parameters each = 264 parameters
- Soundboard: 50+ modes = 150+ parameters
- Hammers: 88 x 5 parameters = 440 parameters
- **Total: 800+ parameters**

**Reduction Strategies:**

1. **Scaling Laws**
   ```cpp
   // Instead of 88 individual string lengths:
   float stringLength(int note) {
       return baseLength * pow(2.0, (60 - note) / 12.0) * lengthScaling;
   }
   // 88 parameters -> 2 parameters (baseLength, lengthScaling)
   ```

2. **Perceptual Grouping**
   ```cpp
   // Group strings into registers
   enum Register { BASS, TENOR, ALTO, TREBLE };
   float dampingForNote(int note) {
       Register reg = getRegister(note);
       return registerDamping[reg] * (1.0 + perNoteDampingVariation);
   }
   // 88 parameters -> 5 parameters
   ```

3. **Macro Parameters**
   ```cpp
   // Single "Age" parameter controls multiple physical aspects
   void setAge(float age) {
       hammerHardness = lerp(0.8f, 0.4f, age);
       stringBrightness = lerp(1.0f, 0.6f, age);
       bodyResonance = lerp(0.7f, 0.9f, age);
       tuningStability = lerp(1.0f, 0.95f, age);
   }
   ```

### 5.3 UI Design for PM Instruments

**Effective PM UI Patterns:**

1. **Visual Representation of Physical Structure**
   ```
   [Exciter Section]  [Resonator Section]  [Body Section]
        |                    |                   |
   Hammer Params        String Params       Soundboard Params
   ```

2. **Real-time Visualization**
   - String vibration display
   - Mode amplitude meters
   - Energy flow indicators

3. **Preset Morphing**
   - Interpolate between physical configurations
   - "Material" blend: Wood <-> Metal <-> Glass

4. **Contextual Help**
   - Explain what each parameter physically represents
   - Show expected ranges for realistic sounds

---

## 6. Modern Advances (2020-2026)

### 6.1 Machine Learning + Physical Modelling

**Differentiable Digital Signal Processing (DDSP)**

Google Magenta's DDSP (2020) opened new possibilities:

```python
# Conceptual DDSP approach
class DDSPSynth:
    def forward(self, f0, loudness):
        # Neural network predicts PM parameters
        harmonics = self.harmonic_encoder(f0, loudness)
        noise_mags = self.noise_encoder(f0, loudness)

        # Traditional PM synthesis with learned parameters
        harmonic_audio = harmonic_synth(f0, harmonics)
        noise_audio = filtered_noise(noise_mags)

        return harmonic_audio + noise_audio
```

**Key ML+PM Developments:**

| Year | Development | Significance |
|------|-------------|--------------|
| 2020 | Google DDSP | Differentiable synthesis, learned parameters |
| 2021 | RAVE | Real-time audio variational autoencoder |
| 2022 | NEWT | Neural waveshaping for timbre transfer |
| 2023 | DiffWave PM | Combining diffusion with physical models |
| 2024 | Hybrid Neural PM | Neural networks controlling classical PM |
| 2025 | Real-time transformer PM | Low-latency neural parameter estimation |

**Practical Applications:**

1. **Parameter Estimation**: ML learns to set PM parameters from audio examples
2. **Excitation Modeling**: Neural networks generate complex excitation signals
3. **Body Modeling**: Learn body response from recordings
4. **Timbre Transfer**: Apply one instrument's character to another

### 6.2 GPU Acceleration

**Current State (2025-2026):**

- GPU synthesis possible but latency-challenged
- Best for: Offline rendering, pre-computation
- Emerging: Low-latency GPU audio APIs

**GPU-Suitable PM Operations:**

| Operation | GPU Speedup | Latency Concern |
|-----------|-------------|-----------------|
| Modal synthesis (many modes) | 10-50x | Medium |
| FDN reverb | 5-20x | High |
| Convolution | 10-100x | Medium |
| Waveguide mesh | 20-100x | High |
| Non-linear tables | 2-5x | Low |

**Hybrid CPU/GPU Strategy:**

```cpp
// GPU handles parallel, latency-tolerant operations
GPU::computeModalBank(modeStates, frequencies, amplitudes);

// CPU handles latency-critical, sequential operations
for (int i = 0; i < bufferSize; ++i) {
    output[i] = exciter.process() + GPU::getModalOutput(i);
}
```

### 6.3 Recent Research Trends

**Academic Developments (2020-2026):**

1. **Energy-Based Physical Modelling**
   - Port-Hamiltonian systems for guaranteed stability
   - Energy conservation ensures no blow-ups

2. **Perceptual Physical Modelling**
   - Simplify models based on auditory perception
   - Only model what listeners can hear

3. **Data-Driven Material Properties**
   - Learn material parameters from recordings
   - Automatic parameter estimation from audio

4. **Multi-Scale Modelling**
   - Different resolution for different frequency ranges
   - Coarse mesh for bass, fine mesh for treble

5. **Neural Audio Codec Integration**
   - PM combined with neural compression
   - Extreme file size reduction with PM+ML

---

## 7. Practical Implementation Roadmap

### 7.1 Complexity Progression Path

**Stage 1: Foundation (Week 1-2)**

```cpp
// Simple Karplus-Strong
class BasicString {
    CircularBuffer delayLine;
    OnePoleLowpass filter;

    float process(float input) {
        float output = delayLine.read();
        delayLine.write(filter.process(input + output * feedback));
        return output;
    }
};
```

**Milestone Checklist:**
- [ ] Working delay line with interpolation
- [ ] Basic lowpass damping
- [ ] Note pitch control
- [ ] Simple ADSR for excitation
- [ ] Basic polyphony (4-8 voices)

**Stage 2: Enhanced String (Week 3-4)**

```cpp
class EnhancedString {
    WaveguidePair waveguide;      // Two traveling waves
    AllpassDispersion dispersion; // Stiffness simulation
    BodyResonator body;           // Simple formant filter

    float process(float excitation) {
        float stringOut = waveguide.process(excitation);
        stringOut = dispersion.process(stringOut);
        return body.process(stringOut);
    }
};
```

**Milestone Checklist:**
- [ ] Bidirectional waveguide
- [ ] Allpass dispersion filter
- [ ] Simple body resonance
- [ ] Improved excitation (noise burst)
- [ ] Parameter smoothing

**Stage 3: Professional Quality (Week 5-8)**

```cpp
class ProfessionalString {
    MultiModeWaveguide waveguide;
    NonlinearExciter exciter;
    SympatheticResonance sympathetic;
    ConvolutionBody body;

    float process(float velocity, float position) {
        float exc = exciter.process(velocity, position);
        float string = waveguide.process(exc);
        string = sympathetic.process(string, otherStrings);
        return body.process(string);
    }
};
```

**Milestone Checklist:**
- [ ] Multiple waveguide modes
- [ ] Non-linear hammer/pluck model
- [ ] Sympathetic resonance
- [ ] Convolution-based body
- [ ] Full polyphony with voice stealing
- [ ] SIMD optimization
- [ ] Quality/CPU presets

### 7.2 Testing and Validation

**Audio Quality Testing:**

1. **Pitch Accuracy**
   ```cpp
   // Test: Output frequency should match input MIDI note
   void testPitchAccuracy() {
       for (int note = 36; note < 96; ++note) {
           float expected = 440.0f * pow(2.0f, (note - 69) / 12.0f);
           float actual = measurePitch(synth.renderNote(note));
           EXPECT_NEAR(expected, actual, 0.5f); // Within 0.5 Hz
       }
   }
   ```

2. **Decay Characteristics**
   ```cpp
   // Test: Amplitude should decay smoothly
   void testDecay() {
       auto audio = synth.renderNote(60, 4.0f); // 4 second render
       auto envelope = extractEnvelope(audio);
       EXPECT_TRUE(isMonotonicallyDecreasing(envelope));
       EXPECT_TRUE(isExponentialDecay(envelope, tolerance));
   }
   ```

3. **CPU Performance**
   ```cpp
   void testCPUUsage() {
       const int voiceCount = 16;
       auto start = highResolutionClock::now();
       for (int i = 0; i < 1000; ++i) {
           synth.processBlock(buffer, voiceCount);
       }
       auto elapsed = highResolutionClock::now() - start;
       float cpuPercent = elapsed / expectedRealtime * 100;
       EXPECT_LT(cpuPercent, 50.0f); // Less than 50% CPU
   }
   ```

**Perceptual Testing:**

1. Compare against reference recordings
2. A/B test with sampled instruments
3. Test extreme parameter ranges for artifacts
4. Verify smooth parameter interpolation (no clicks)

### 7.3 User Expectations

**What Users Expect from PM Instruments:**

| Feature | Importance | Implementation Priority |
|---------|------------|------------------------|
| Realistic default sound | Critical | Highest |
| Responsive to velocity | Critical | Highest |
| Smooth parameter changes | High | High |
| Low latency | High | High |
| Reasonable CPU usage | High | High |
| Unique sound possibilities | Medium | Medium |
| Educational/exploratory value | Low-Medium | Lower |

**Common User Complaints (and Solutions):**

1. **"Sounds synthetic"**
   - Add subtle non-linearities
   - Include noise elements
   - Model secondary resonances

2. **"Too much CPU"**
   - Provide quality presets
   - Implement voice limiting
   - Profile and optimize hot paths

3. **"Parameters don't make sense"**
   - Use physical metaphors in UI
   - Provide presets as starting points
   - Add contextual help

4. **"Can't get the sound I want"**
   - Include comprehensive preset library
   - Add macro controls
   - Provide sound design tutorials

---

## 8. Performance Benchmarks and Expectations

### 8.1 Target Performance Metrics

**Acceptable CPU Usage (per voice, at 44.1kHz):**

| Model Complexity | CPU Target | Voice Count Target |
|------------------|------------|-------------------|
| Simple (K-S) | <0.1% | 64+ |
| Enhanced String | <0.5% | 32+ |
| Full String Model | <1% | 16+ |
| Wind Instrument | <2% | 8+ |
| Full Piano | <5% | 8+ |
| Complex Percussion | <1% | 24+ |

### 8.2 Latency Requirements

**Maximum Acceptable Latency:**

| Use Case | Max Latency | Buffer Size |
|----------|-------------|-------------|
| Live performance | 10ms | 256-512 samples |
| Studio recording | 20ms | 512-1024 samples |
| Mixing/production | 50ms | 1024-2048 samples |

**Latency Contributors in PM:**

```
Input processing:        1-5ms
Internal oversampling:   1-3ms (for 4x OS)
PM computation:          <1ms
Output filtering:        1-2ms
DAW buffer:              Varies (5-50ms)
-----------------------------------------
Total (typical):         10-60ms
```

### 8.3 Memory Usage

**Expected Memory Footprint:**

| Component | Memory (per voice) | Memory (global) |
|-----------|-------------------|-----------------|
| Delay lines | 4-64KB | N/A |
| Filter states | 100-500 bytes | N/A |
| Lookup tables | N/A | 64-256KB |
| Body IRs | N/A | 100KB-2MB |
| Mode data | 1-4KB | N/A |
| Total | 10-100KB/voice | 1-10MB |

---

## 9. Optimization Checklist

### 9.1 Pre-Implementation Checklist

- [ ] Define target voice count and CPU budget
- [ ] Choose appropriate algorithm complexity level
- [ ] Identify SIMD optimization opportunities
- [ ] Plan parameter hierarchy (macro/micro)
- [ ] Design pre-computation strategy
- [ ] Establish testing methodology

### 9.2 Implementation Checklist

- [ ] Use efficient delay line implementation (power-of-2, no modulo)
- [ ] Implement coefficient smoothing to avoid clicks
- [ ] Use lookup tables for expensive functions (tanh, sin, etc.)
- [ ] Profile before and after each optimization
- [ ] Test at multiple sample rates
- [ ] Verify stability at extreme parameter values

### 9.3 SIMD Optimization Checklist

- [ ] Identify parallelizable loops
- [ ] Align data to 16/32-byte boundaries
- [ ] Use platform-appropriate intrinsics or library
- [ ] Test SIMD vs scalar performance
- [ ] Handle non-aligned tail samples
- [ ] Consider JUCE SIMD wrappers for portability

### 9.4 Quality Assurance Checklist

- [ ] No audible clicks on parameter changes
- [ ] No blow-ups or NaN outputs
- [ ] Smooth note transitions
- [ ] Correct pitch across all notes
- [ ] Appropriate dynamic range
- [ ] CPU usage within targets

### 9.5 Release Checklist

- [ ] Multiple quality presets (Draft/Standard/High)
- [ ] CPU meter or voice count display
- [ ] Helpful parameter tooltips
- [ ] Comprehensive preset library
- [ ] Documentation of physical model

---

## 10. Actionable Recommendations

### 10.1 For Beginners

1. **Start with Karplus-Strong**
   - Simple to implement, sounds good immediately
   - Teaches fundamental concepts
   - Low CPU, easy to debug

2. **Focus on One Instrument**
   - Master string physics before attempting wind
   - Depth over breadth initially

3. **Use Reference Implementations**
   - Study open-source PM code
   - The STK (Synthesis ToolKit) is excellent
   - FAUST has many PM examples

### 10.2 For Intermediate Developers

1. **Invest in Profiling**
   - CPU profiler is essential (Instruments on macOS)
   - Profile early and often
   - Optimize based on data, not intuition

2. **Build a Testing Framework**
   - Automated pitch/decay/CPU tests
   - Regression testing for audio quality
   - A/B comparison tooling

3. **Study Commercial Products**
   - Use trial versions extensively
   - Analyze parameter design
   - Note CPU usage patterns

### 10.3 For Advanced Developers

1. **Consider Hybrid Approaches**
   - Pure PM isn't always best
   - Sample+PM hybrid often optimal
   - ML parameter estimation emerging

2. **Explore Modern Research**
   - DDSP and neural synthesis
   - Energy-based methods
   - Perceptual simplification

3. **Contribute to Knowledge Base**
   - Document your findings
   - Share optimization discoveries
   - Build community resources

### 10.4 Product Strategy Recommendations

1. **Differentiate on Sound, Not Technology**
   - Users buy sound, not algorithms
   - Technology enables but doesn't sell

2. **Provide Quality Presets**
   - 80% of users never leave presets
   - Professional sound designers essential

3. **Balance Accessibility and Depth**
   - Simple surface, complex underneath
   - Progressive disclosure of complexity

4. **Manage Expectations**
   - PM won't replace samples for everything
   - Hybrid is often the answer
   - Different tools for different jobs

---

## Appendix A: Code Snippets

### A.1 Efficient Delay Line

```cpp
class DelayLine {
    std::vector<float> buffer;
    int writeIndex = 0;
    int sizeMask;  // For power-of-2 masking

public:
    DelayLine(int maxDelay) {
        int size = nextPowerOfTwo(maxDelay);
        buffer.resize(size, 0.0f);
        sizeMask = size - 1;
    }

    void write(float sample) {
        buffer[writeIndex] = sample;
        writeIndex = (writeIndex + 1) & sizeMask;
    }

    float read(float delaySamples) {
        float readPos = writeIndex - delaySamples;
        int index0 = ((int)readPos) & sizeMask;
        int index1 = (index0 + 1) & sizeMask;
        float frac = readPos - (int)readPos;
        return buffer[index0] + frac * (buffer[index1] - buffer[index0]);
    }
};
```

### A.2 Lookup Table for Saturation

```cpp
class SaturationTable {
    static constexpr int TABLE_SIZE = 4096;
    std::array<float, TABLE_SIZE> table;

public:
    SaturationTable(float drive = 1.0f) {
        for (int i = 0; i < TABLE_SIZE; ++i) {
            float x = (i / (float)(TABLE_SIZE - 1)) * 2.0f - 1.0f;
            table[i] = std::tanh(x * drive);
        }
    }

    float process(float input) {
        float normalized = (input + 1.0f) * 0.5f;
        normalized = std::clamp(normalized, 0.0f, 1.0f);
        float indexF = normalized * (TABLE_SIZE - 1);
        int index0 = (int)indexF;
        int index1 = std::min(index0 + 1, TABLE_SIZE - 1);
        float frac = indexF - index0;
        return table[index0] + frac * (table[index1] - table[index0]);
    }
};
```

### A.3 SIMD Modal Synthesis

```cpp
void processModalBank(float* output, int numSamples,
                      float* frequencies, float* amplitudes,
                      float* phases, float* decays, int numModes) {
    // Process 4 modes at a time using SSE
    for (int m = 0; m < numModes; m += 4) {
        __m128 freq = _mm_loadu_ps(&frequencies[m]);
        __m128 amp = _mm_loadu_ps(&amplitudes[m]);
        __m128 phase = _mm_loadu_ps(&phases[m]);
        __m128 decay = _mm_loadu_ps(&decays[m]);

        for (int i = 0; i < numSamples; ++i) {
            // Compute sin(phase) using approximation
            __m128 sinVal = fastSin_SSE(phase);
            __m128 contrib = _mm_mul_ps(sinVal, amp);

            // Sum 4 contributions
            float sum = horizontalSum(contrib);
            output[i] += sum;

            // Update phase and amplitude
            phase = _mm_add_ps(phase, freq);
            amp = _mm_mul_ps(amp, decay);
        }

        // Store updated state
        _mm_storeu_ps(&phases[m], phase);
        _mm_storeu_ps(&amplitudes[m], amp);
    }
}
```

---

## Appendix B: Resource Links

### Academic Resources
- Julius O. Smith's "Physical Audio Signal Processing" (online book)
- CCRMA (Stanford) PM tutorials
- DAFx conference proceedings
- ICMC papers on physical modelling

### Open Source References
- The Synthesis ToolKit (STK)
- FAUST physical modelling examples
- Csound PM opcodes
- Pure Data externals

### Commercial Reference Products
- Modartt Pianoteq (piano)
- Applied Acoustics Chromaphone (percussion)
- Applied Acoustics String Studio (strings)
- Arturia Piano V (hybrid piano)
- Audio Modeling SWAM (wind/strings)

---

## Summary

Physical modelling synthesis offers unique expressive possibilities but demands careful attention to:

1. **CPU Budget**: Plan for 0.1-5% per voice depending on complexity
2. **SIMD Optimization**: Essential for professional-quality PM
3. **Parameter Design**: Layer complexity (macro/micro/expert)
4. **Hybrid Approaches**: Often best balance of quality/CPU
5. **User Experience**: Sound quality and presets matter most
6. **Modern Trends**: ML integration emerging as powerful tool

The key to successful PM implementation is starting simple, validating thoroughly, and incrementally adding complexity while maintaining performance targets. Physical modelling is not about perfectly recreating reality, but about creating musically expressive instruments that respond naturally to performer input.
