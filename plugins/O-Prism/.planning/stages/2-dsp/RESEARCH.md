# Stage 2: DSP Research Findings

**Date:** 2026-02-16
**Plugin:** O-Prism (Microtonal Wavetable Synthesizer)
**Researchers:** 4 parallel domain agents (wavetable engine, JUCE DSP APIs, oscillator/noise/unison, effects chain)

---

## Executive Summary

All architecture decisions from ARCHITECTURE.md are validated. Three critical corrections identified:

1. **Reverb is float-only** -- `juce::dsp::Reverb` does NOT support double. Requires float conversion boundary in effects chain.
2. **SVF has NO notch type** -- `StateVariableTPTFilterType` only supports LP/BP/HP. Notch must be implemented as LP+HP sum from a single SVF computation (6-line custom wrapper).
3. **ADSR returns float, uses LINEAR segments** -- Cast to double needed. Linear (not exponential) attack/decay/release.

---

## Phase 2.1: Basic Wavetable Playback

### Procedural Test Wavetable Generation

Generate basic waveforms additively (NOT from naive formulas -- additive synthesis produces alias-free source for mipmap generator):

```cpp
// Saw: sum(sin(n * 2pi * i / 2048) / n) for n=1..1024, normalize by 2/PI
// Square: sum(sin(n * 2pi * i / 2048) / n) for ODD n only, normalize by 4/PI
// Triangle: sum(sign * sin(n * 2pi * i / 2048) / n^2) for ODD n, normalize by 8/PI^2
// Sine: sin(2pi * i / 2048) -- single harmonic
```

### Phase Accumulator Pattern

```cpp
// 64-bit double precision phase accumulator
double phaseIncrement = frequency / sampleRate;
phaseAccumulator += phaseIncrement;
if (phaseAccumulator >= 1.0) phaseAccumulator -= 1.0;

// Sample position within 2048-sample frame
double samplePos = phaseAccumulator * 2048.0;
int idx0 = (int)samplePos;
int idx1 = (idx0 + 1) % 2048;  // Or use guard sample (2049th) to avoid modulo
double frac = samplePos - idx0;
double output = frame[idx0] + frac * (frame[idx1] - frame[idx0]);  // Linear interp
```

**Optimization:** Store 2049 samples per frame (guard sample = copy of first sample) to eliminate the modulo operation in the inner loop.

### TuningEngine Integration

Frequency calculation follows proven O-Lyrica pattern:
```cpp
double freq = tuningEngine->getFrequency(midiNoteNumber);
freq *= std::pow(2.0, (coarseSemitones + fineCents / 100.0) / 12.0);
double phaseIncrement = freq / sampleRate;
```

### Amplitude ADSR

```cpp
juce::ADSR ampEnvelope;
// In prepare(): ampEnvelope.setSampleRate(sampleRate);  // MUST call before setParameters()
// In startNote(): ampEnvelope.setParameters({attack, decay, sustain, release}); ampEnvelope.noteOn();
// In stopNote(): ampEnvelope.noteOff();
// Per-sample: float envVal = ampEnvelope.getNextSample();  // Returns FLOAT 0-1
//             outputSample *= static_cast<double>(envVal);
// Voice lifecycle: if (!ampEnvelope.isActive()) clearCurrentNote();
```

**GOTCHA:** `setSampleRate()` MUST be called before `setParameters()` (jassert in source). ADSR uses LINEAR segments (not exponential).

---

## Phase 2.2: Mipmap Anti-Aliasing + Osc B + Mixing

### FFT-Based Mipmap Generation

Using `juce::dsp::FFT` verified against JUCE 8.0.4 source (`/JUCE/modules/juce_dsp/frequency/juce_FFT.h`):

```cpp
static constexpr int fftOrder = 11;            // log2(2048)
static constexpr int fftSize = 1 << fftOrder;  // 2048
juce::dsp::FFT fft(fftOrder);                  // Reuse across frames (caches tables)

// Buffer must be 2 * getSize() = 4096 floats
std::vector<float> fftBuffer(fftSize * 2, 0.0f);

// For each frame:
// 1. Copy frame into fftBuffer[0..2047], zero fftBuffer[2048..4095]
// 2. fft.performRealOnlyForwardTransform(fftBuffer.data(), false);
//    ^^^ false = calculate full spectrum (needed for IFFT)
// 3. For each level (0-9):
//    - maxHarmonic = (fftSize/2) >> level;  // 1024, 512, 256, ...
//    - Zero bins above maxHarmonic (both positive and negative frequencies)
//    - Also zero bin 0 (DC) to prevent DC offset
//    - fft.performRealOnlyInverseTransform(workBuffer.data());
//    - JUCE IFFT divides by fftSize internally (no manual normalization)
//    - Store result in mipmap[level][frame]
```

**FFT Data Format:** Interleaved complex `[Re0, Im0, Re1, Im1, ...]`. Bin k: real at `buffer[k*2]`, imag at `buffer[k*2+1]`.

### Memory Layout

```cpp
struct WavetableData {
    static constexpr int kTableSize = 2048;
    static constexpr int kGuardSamples = 1;  // Extra sample for wrap-free interp
    static constexpr int kFrameSize = kTableSize + kGuardSamples;  // 2049
    static constexpr int kMaxFrames = 256;
    static constexpr int kNumMipmapLevels = 10;

    int numFrames = 0;
    std::vector<float> data;  // Flat: [level][frame][sample+guard]

    float getSample(int level, int frame, int sampleIndex) const {
        return data[(level * numFrames + frame) * kFrameSize + sampleIndex];
    }
};
// Memory: 10 levels x 256 frames x 2049 samples x 4 bytes = ~20 MB worst case
// Typical (16 frames): ~1.3 MB. Single-cycle: ~82 KB.
```

### Mipmap Level Selection

```cpp
double baseFrequency = sampleRate / 2048.0;  // ~21.5 Hz at 44.1kHz
double levelFloat = std::log2(frequency / baseFrequency);
levelFloat = juce::jlimit(0.0, 9.0, levelFloat);
int level0 = (int)levelFloat;
int level1 = std::min(level0 + 1, 9);
double levelFrac = levelFloat - level0;
```

### 3D Trilinear Interpolation (8 lookups per sample)

```
For each output sample:
  1. Sample interpolation: lerp within frame at samplePos (2 lookups)
  2. Frame interpolation: lerp between frame0 and frame1 at position (x2 = 4 lookups)
  3. Mipmap interpolation: lerp between level0 and level1 (x2 = 8 lookups total)

Optimization: Skip mipmap interp when pitch is stable (reduces to 4 lookups)
```

### Oscillator Mixing

```cpp
double mixedL = oscAoutL * (1.0 - oscMix) + oscBoutL * oscMix;
double mixedR = oscAoutR * (1.0 - oscMix) + oscBoutR * oscMix;
```

### Thread-Safe Wavetable Delivery

```cpp
std::atomic<WavetableData*> activeTable[2] {nullptr, nullptr};

// Background thread: load + generate mipmaps, then:
WavetableData* old = activeTable[oscIndex].exchange(newTable, std::memory_order_release);
juce::MessageManager::callAsync([old]() { delete old; });  // Deferred deletion

// Audio thread (lock-free):
const WavetableData* table = activeTable[oscIndex].load(std::memory_order_acquire);
```

---

## Phase 2.3: Unison + Sub + Noise + Glide

### polyBLEP Sub Oscillator

```cpp
static inline double polyBLEP(double t, double dt) {
    if (t < dt)       { t /= dt; return t + t - t * t - 1.0; }
    if (t > 1.0 - dt) { t = (t - 1.0) / dt; return t * t + t + t + 1.0; }
    return 0.0;
}

// Saw: value = 2*phase - 1; value -= polyBLEP(phase, phaseInc);
// Square: value = (phase < 0.5) ? 1 : -1;
//         value += polyBLEP(phase, phaseInc);
//         value -= polyBLEP(fmod(phase + 0.5, 1.0), phaseInc);
// Triangle: Leaky-integrate polyBLEP square + DC blocker
// Sine: sin(phase * twoPi) -- no correction needed

// Octave offset: subFreq = voiceFreq * pow(2.0, subOctave)  // -2, -1, or 0
```

### Noise Generation (6 types)

| Type | Algorithm | Key Detail |
|------|-----------|------------|
| White | `random.nextDouble() * 2.0 - 1.0` | No normalization needed |
| Pink | Paul Kellet economy filter (3 state vars) | `b0=0.99765*b0+w*0.099; b1=0.963*b1+w*0.297; b2=0.57*b2+w*1.053; out=(b0+b1+b2+w*0.185)*0.11` |
| Brown | Leaky integrator: `state += white*0.02; state *= 0.998` | Scale step size by `44100/sr` for higher rates. Output `*3.5` |
| Digital | Sample-and-hold: hold random value for N samples | N = `sr/5512` for ~5.5kHz effective rate. Quantize to 8 levels |
| Vinyl | Bandpass-filtered white (200-5kHz) + random crackle impulses | Poisson-distributed timing, exponential decay `*0.95` |
| Wind | LFO-modulated lowpass brown noise | LFO at 0.2Hz modulates cutoff 50-550Hz. Output `*5.0` |

### Unison Engine

```cpp
// For N voices (1-8):
double centerIndex = (N - 1) / 2.0;
double normFactor = (N > 1) ? centerIndex : 1.0;
double gainPerVoice = 1.0 / std::sqrt((double)N);

for (int i = 0; i < N; ++i) {
    double normalizedPos = (N > 1) ? (i - centerIndex) / normFactor : 0.0;
    // Detune: pow(2.0, normalizedPos * detuneAmount * 50.0 / 1200.0)
    // Pan: normalizedPos * widthAmount -> equal-power cos/sin pan law
    // Phase: random offset set once at note-on
    // Amplitude: gainPerVoice = 1/sqrt(N)
}
```

Each unison voice = separate phase accumulator sharing the same WavetableData pointer. NOT separate WavetableOscillator instances.

### Glide Processor

```cpp
// Exponential frequency interpolation (one-pole smoother)
glideCoeff = std::exp(-1.0 / (glideTime * sampleRate));
// Per-sample: currentFreq = currentFreq * glideCoeff + targetFreq * (1.0 - glideCoeff);
// Stop when within 0.01 cents of target

// Legato detection: check getCurrentlyPlayingNote() >= 0 in startNote()
// Off = instant, Legato = glide only if was already playing, Always = always glide
```

### Equal-Power Pan Law

```cpp
double panNorm = (pan + 1.0) * 0.5;  // Map [-1,+1] to [0,1]
double leftGain = std::cos(panNorm * halfPi);
double rightGain = std::sin(panNorm * halfPi);
// At center: both = 0.707 (-3dB). Constant power across stereo field.
```

---

## Phase 2.4: Dual Filters + Filter Envelope

### StateVariableTPTFilter API (Verified from JUCE 8.0.4 Source)

**Types available:** `lowpass`, `bandpass`, `highpass` only. **NO notch type in enum.**

**Resonance mapping (CRITICAL):** SVF resonance is INVERSE of Q. Lower value = MORE resonant.
```cpp
// Map user resonance (0=flat, 1=self-oscillating) to SVF resonance:
double svfResonance = 1.0 / (1.0 + resonanceParam * 19.0);
// At 0: svfResonance=1.0 (Butterworth). At 1: svfResonance=0.05 (Q~20, self-osc)
// jassert requires resonance > 0 -- never pass 0
```

**Per-sample processing for filter envelope modulation:**
```cpp
double filtEnvVal = static_cast<double>(filterADSR.getNextSample());
double modulatedCutoff = baseCutoff * std::pow(2.0, filtEnvVal * envDepth * 4.0);
modulatedCutoff = juce::jlimit(20.0, 20000.0, modulatedCutoff);
filter.setCutoffFrequency(modulatedCutoff);  // Calls tan() internally -- OK for TPT design
double out = filter.processSample(0, inputSample);
```

**24dB Cascading:** Two SVF instances in series. Apply resonance to first stage only.

**Notch Implementation (Efficient):** Copy the 6-line SVF computation to return LP+HP directly:
```cpp
// Single SVF computation, returns notch = yLP + yHP
auto yHP = h * (input - s1 * (g + R2) - s2);
auto yBP = yHP * g + s1;
s1 = yHP * g + yBP;
auto yLP = yBP * g + s2;
s2 = yBP * g + yLP;
return yLP + yHP;  // Notch output -- 1 SVF computation, not 2
```

This is 2x more efficient than running two separate SVF instances.

**Voice re-triggering:** Call `filter.reset()` in `startNote()` to clear state.

### Filter Routing

```cpp
if (routingSerial) {
    // Signal -> Drive A -> Filter A -> Drive B -> Filter B -> output
    double driven = std::tanh(input * (1.0 + driveA * 9.0));
    double filteredA = filterA.processSample(0, driven);
    driven = std::tanh(filteredA * (1.0 + driveB * 9.0));
    output = filterB.processSample(0, driven);
} else {
    // Parallel: Signal -> (Drive A -> Filter A) + (Drive B -> Filter B) -> output
    double drivenA = std::tanh(input * (1.0 + driveA * 9.0));
    double drivenB = std::tanh(input * (1.0 + driveB * 9.0));
    output = filterA.processSample(0, drivenA) + filterB.processSample(0, drivenB);
}
```

### Key Tracking

```cpp
double keyTrackOffset = keyTrackAmount * (midiNote - 60);  // Semitones from middle C
double keyTrackedCutoff = baseCutoff * std::pow(2.0, keyTrackOffset / 12.0);
```

---

## Phase 2.5: Effects Chain + Master

### Architecture Decision: Float Precision for Effects

**Recommendation:** Process the entire effects chain in float (not double). Rationale:
- `juce::dsp::Reverb` is float-only (confirmed from source: `HeapBlock<float>` buffers)
- Avoids float<->double conversion overhead per block
- Voice path (oscillators, filters, envelopes) still uses double where precision matters
- Industry standard (Serum, Vital use float for effects)

Convert once at the voice sum -> effects boundary.

### Effects Chain Order: Distortion -> Chorus -> Delay -> EQ -> Reverb

### Distortion (4 algorithms + 2x oversampling)

```cpp
juce::dsp::Oversampling<float> oversampling {
    2, 1,  // 2 channels, 2^1 = 2x oversampling
    juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,  // Low latency
    true
};
juce::dsp::DryWetMixer<float> distMixer;

// prepare(): oversampling.initProcessing(blockSize);
//            distMixer.setWetLatency(oversampling.getLatencyInSamples());
// process(): distMixer.pushDrySamples(block);
//            auto osBlock = oversampling.processSamplesUp(block);
//            // Apply waveshaping per-sample on osBlock
//            oversampling.processSamplesDown(block);
//            distMixer.mixWetSamples(block);

// Drive mapping: gain = 1.0 + drive01 * 9.0 (range 1-10x)
// Soft Clip: tanh(gain * x)
// Hard Clip: clamp(gain * x, -1, 1)
// Tube: Asymmetric -- positive: x/(1+|x|), negative: tanh(x*1.5)/1.5
// Fold: sin(gain * x * PI)
```

**GOTCHA:** Oversampling `factor` parameter is an exponent. `1` = 2x, `2` = 4x.

### Chorus

```cpp
juce::dsp::Chorus<float> chorus;
// prepare(): chorus.prepare(spec); chorus.setCentreDelay(7.0f); chorus.setFeedback(0.0f);
// process(): chorus.setRate(rate); chorus.setDepth(depth); chorus.setMix(mix);
//            chorus.process(ProcessContextReplacing(block));
// Handles dry/wet internally -- NO external DryWetMixer needed
```

### Delay (feedback + ping-pong + sync)

```cpp
juce::dsp::DelayLine<float, DelayLineInterpolationTypes::Lagrange3rd> delayL{192000}, delayR{192000};
juce::dsp::StateVariableTPTFilter<float> feedbackFilterL, feedbackFilterR;  // LP at 8kHz
juce::dsp::DryWetMixer<float> delayMixer;
float feedbackL = 0.0f, feedbackR = 0.0f;

// Per-sample feedback loop:
// Normal:   delayL.pushSample(0, inputL + feedbackL * fbAmt);
// PingPong: delayL.pushSample(0, inputL + feedbackR * fbAmt); (cross-feedback)
// Pop:      feedbackL = feedbackFilterL.processSample(0, delayL.popSample(0, delaySamples)) * fbAmt;

// Tempo sync: time = (60.0 / bpm) * subdivisionBeats[divisionIndex]
//   via getPlayHead()->getPosition()->getBpm()
```

### EQ (3-band parametric)

```cpp
juce::dsp::ProcessorDuplicator<IIR::Filter<float>, IIR::Coefficients<float>> lowShelf, midPeak, highShelf;

// Update coefficients (safe -- copies values in-place, no allocation):
*lowShelf.state = *IIR::Coefficients<float>::makeLowShelf(sr, 200.0f, 0.707f, Decibels::decibelsToGain(lowDB));
*midPeak.state = *IIR::Coefficients<float>::makePeakFilter(sr, midFreqHz, 1.0f, Decibels::decibelsToGain(midDB));
*highShelf.state = *IIR::Coefficients<float>::makeHighShelf(sr, 8000.0f, 0.707f, Decibels::decibelsToGain(highDB));

// Process in series: lowShelf.process(ctx); midPeak.process(ctx); highShelf.process(ctx);
// NOTE: gainFactor is LINEAR scale, not dB. Use Decibels::decibelsToGain().
```

### Reverb (float-only + pre-delay)

```cpp
juce::dsp::Reverb reverb;  // Float-only (Freeverb-based)
juce::dsp::DelayLine<float, Linear> preDelayL{19200}, preDelayR{19200};  // 200ms max
juce::dsp::DryWetMixer<float> reverbMixer;

// CRITICAL: Set reverb internal mix to full wet, use DryWetMixer for actual blend:
juce::Reverb::Parameters params;
params.roomSize = size; params.damping = damp;
params.wetLevel = 1.0f; params.dryLevel = 0.0f;  // External mixer handles dry/wet
params.width = 1.0f;
reverb.setParameters(params);

// Pre-delay: per-sample push/pop through DelayLine before reverb input
```

### Master Volume (smoothed)

```cpp
juce::SmoothedValue<float> masterVol{0.8f};
// prepare(): masterVol.reset(sampleRate, 0.02);  // 20ms ramp
// process(): Per-sample: buffer.sample *= masterVol.getNextValue();
// Prevents zipper noise on abrupt changes.
```

---

## Pitfalls & Gotchas Summary

| Issue | Component | Severity | Mitigation |
|-------|-----------|----------|------------|
| Reverb float-only | Effects chain | HIGH | Process effects chain in float, convert at boundary |
| SVF no notch type | Filter | HIGH | Custom 6-line LP+HP sum from single SVF computation |
| SVF resonance inverse-Q | Filter | HIGH | Map: `1.0 / (1.0 + param * 19.0)`. Never pass 0. |
| ADSR returns float | Envelope | MEDIUM | Cast to double for voice processing |
| ADSR linear segments | Envelope | LOW | Acceptable for v1.0. Exponential in v2.0 |
| ADSR setSampleRate before setParameters | Envelope | HIGH | Always call setSampleRate() in prepare() first |
| Oversampling factor is exponent | Distortion | HIGH | Pass `1` for 2x (not `2`) |
| FFT buffer size | Mipmap gen | HIGH | Must be `2 * getSize()` = 4096 floats |
| FFT onlyCalculateNonNegativeFrequencies | Mipmap gen | MEDIUM | Use `false` for IFFT compatibility |
| setCutoffFrequency calls tan() | Filter modulation | MEDIUM | Per-sample is OK for TPT design. Profile at 16 voices |
| Brown noise step size sample-rate dependent | Noise | LOW | Scale by `44100/sampleRate` |
| ADSR no mid-note parameter change | Envelope | LOW | Apply new params only on next noteOn() |
| DryWetMixer needs latency for oversampling | Distortion | MEDIUM | Set via `setWetLatency()` after `initProcessing()` |
| IIR gainFactor is linear, not dB | EQ | MEDIUM | Use `Decibels::decibelsToGain()` |

---

## Existing Codebase References

| Plugin | Relevant Pattern | File |
|--------|-----------------|------|
| O-Lyrica | Voice architecture (APVTS ptr, TuningEngine ptr, per-voice DSP) | `plugins/O-Lyrica/Source/HarpSynthVoice.cpp` |
| O-AnalogSaturation | Oversampling + waveshaping + IIR filter | `plugins/O-AnalogSaturation/Source/PluginProcessor.cpp` |
| O-DigiDelay | DelayLine with feedback, tempo sync, SmoothedValue | `plugins/O-DigiDelay/Source/PluginProcessor.cpp` |
| O-SimpleReverb | Reverb + pre-delay + ProcessorDuplicator EQ | `plugins/O-SimpleReverb/Source/PluginProcessor.h` |
| O-Chorus | Custom chorus with DelayLine + LFO | `plugins/O-Chorus/Source/DSP/ChorusEngine.h` |
| O-Bells | StateVariableTPTFilter for resonators | `plugins/O-Bells/Source/BellVoice.h` |

---

## Module Dependencies

| Module | Version | Usage |
|--------|---------|-------|
| scala-tuning-engine | v2.1.0 | TuningEngine, ScaleGenerator, EmbeddedTunings |
| webview-relay-manager | latest | WebView parameter binding (Stage 3) |

---

## Architecture Corrections

1. **ARCHITECTURE.md line 191** references `juce::dsp::Reverb` for double pipeline -- Reverb is float-only. Effects chain should use float.
2. **Effects chain order:** CONTEXT.md says Dist->Chorus->Delay->EQ->Reverb. ARCHITECTURE.md Section 8 says Dist->Chorus->Delay->Reverb->EQ. **Use CONTEXT.md order** (EQ before Reverb allows tonal shaping before spatial processing -- user-confirmed in discuss phase).
3. **Pre-delay DelayLine** must be stereo (numChannels=2), not mono.

---

## Confidence Assessment

| Area | Confidence | Notes |
|------|-----------|-------|
| FFT mipmap generation | HIGH | Verified from JUCE 8.0.4 source header |
| Wavetable playback algorithm | HIGH | Industry standard, well-documented |
| SVF filter API | HIGH | Read actual source code, all gotchas identified |
| polyBLEP algorithms | HIGH | De facto standard, multiple professional references |
| Noise generation | HIGH (White/Pink/Brown), MEDIUM (Vinyl/Wind) | Vinyl/Wind tuning may need listening tests |
| Unison engine | HIGH | Straightforward math, proven pattern |
| Effects chain | HIGH | Patterns proven in existing plugins (O-DigiDelay, O-AnalogSaturation, O-SimpleReverb) |
| CPU at max unison (256 osc) | MEDIUM | Requires empirical benchmarking |
| per-sample setCutoffFrequency cost | MEDIUM | tan() per sample per voice -- profile needed |
