# Phase 3.1: Core Waveguide + Basic Friction - Research

**Researched:** 2026-04-04
**Domain:** Physical modeling DSP -- bowed string waveguide with hyperbolic friction
**Confidence:** HIGH

## Summary

Phase 3.1 establishes the foundational bowed string engine: a single `BowedStringVoice` (extending `juce::SynthesiserVoice`) containing a `WaveguideString` (two `juce::dsp::DelayLine<float, Thiran>` rails split at the bow contact point), a `BowModel` (envelope-driven bow velocity/force signals from MIDI), a `HyperbolicFriction` module (memoryless friction curve computing waveguide reflection coefficients), and a bridge loss filter. The output is a mono per-voice signal scaled by OUTPUT_LEVEL.

The architecture follows the proven O-Lyrica pattern: processor owns a `juce::Synthesiser` with N `BowedStringVoice` instances, each holding its own DSP objects. APVTS pointer is passed to each voice at construction time; parameters are atomically read once per block in `updateParametersFromAPVTS()`. The critical difference from O-Lyrica is that bowed strings use **continuous excitation** (bow stays in contact during sustain) rather than **impulsive excitation** (pluck-and-decay), which changes voice lifecycle management significantly.

**Primary recommendation:** Implement the STK-style scattering junction algorithm (memoryless, O(1) per sample) using the hyperbolic friction curve. This avoids the implicit equation entirely, guarantees stability, and produces good sound quality for v1.0. Newton-Raphson solving is deferred to Phase 3.4 (elasto-plastic tier).

---

## Algorithm Specification

### Complete Per-Sample Processing Chain (Phase 3.1)

This is the exact sample-by-sample algorithm for `BowedStringVoice::renderNextBlock`:

```
For each sample:
  1. Update bow envelope (compute current v_bow, F_bow)
  2. Read incoming waves from delay line ends:
       bridgeReflection = -bridgeLossFilter.processSample(bridgeDelay.popSample(0))
       nutReflection    = -neckDelay.popSample(0)          // sign inversion = hard boundary
  3. Combine traveling waves at bow point:
       v_string_incoming = bridgeReflection + nutReflection
  4. Compute differential velocity:
       v_delta = v_bow - v_string_incoming
  5. Evaluate friction -> reflection coefficient:
       mu = mu_d + (mu_s - mu_d) * v_0 / (v_0 + |v_delta|)
       r  = 0.25 * mu * F_bow / R_s
       rho = r / (1.0 + r)
  6. Compute injected velocity:
       newVelocity = v_delta * rho
  7. Write outgoing waves into both delay lines:
       bridgeDelay.pushSample(0, bridgeReflection + newVelocity)
       neckDelay.pushSample(0, nutReflection + newVelocity)
  8. Take output from bridge end:
       output = bridgeDelay output (the sample popped in step 2)
  9. Apply output gain:
       output *= outputGainLinear
```

### Key Algorithm Details

**Why bridge filter is INSIDE the loop (step 2):** The bridge loss filter models frequency-dependent energy loss at the bridge termination. It processes the wave arriving at the bridge end BEFORE reflecting it back. This is physically correct -- the bridge absorbs energy from the wave on each round-trip, creating the natural decay of higher harmonics.

**Why nut is sign-inversion only:** The nut/finger end is a nearly ideal rigid boundary. The wave reflects with inverted sign (phase flip). No filtering needed at nut for Phase 3.1.

**Bridge loss filter equation:** One-pole lowpass:
```
H(z) = g * (1 - p) / (1 - p * z^-1)
```
- `g` = loop gain (< 1.0 for natural decay, approaches 1.0 with INFINITE_SUSTAIN)
- `p` = pole position (controls brightness rolloff), derived from BRIGHTNESS parameter
- For Phase 3.1: `g = 0.995`, `p = BRIGHTNESS_freq / (BRIGHTNESS_freq + sampleRate / (2*pi))`

**Delay line split at bow position:**
```cpp
float totalDelay = sampleRate / f0;
float bridgeDelaySamples = totalDelay * bowPosition;    // bowPosition = beta
float neckDelaySamples   = totalDelay * (1.0f - bowPosition);
```

**Filter group delay compensation (from O-Lyrica):** The bridge loss filter adds group delay that effectively lengthens the delay line, lowering pitch. Compensate:
```cpp
float filterGroupDelay = sampleRate / (2.0f * pi * bridgeCutoffHz);
float compensatedTotal = totalDelay - filterGroupDelay;
float bridgeDelaySamples = compensatedTotal * bowPosition;
float neckDelaySamples   = compensatedTotal * (1.0f - bowPosition);
```

### Bow Model Envelope

The bow model produces `v_bow` and `F_bow` signals from MIDI input and parameter values:

```
On note-on:
  v_bow_target = mapVelocity(midiVelocity) * bowSpeedParam
  F_bow_target = bowPressureParam
  attackTime   = lerp(50ms, 5ms, midiVelocity)  // harder velocity = faster attack
  bowActive    = true

On note-off:
  releaseTime  = 30ms (fixed for Phase 3.1)
  bowActive    = false  // starts release ramp

Per-sample envelope update:
  if (bowActive):
    v_bow += (v_bow_target - v_bow) * attackCoeff     // one-pole smoothing
    F_bow += (F_bow_target - F_bow) * attackCoeff
  else:
    v_bow *= releaseCoeff                             // exponential decay to 0
    F_bow *= releaseCoeff
```

**MIDI velocity mapping:**
```cpp
// Map 0-127 velocity to bow speed multiplier (0.0 - 1.0)
// Use a slight curve for musical response
float velocityNorm = velocity;  // already 0-1 from JUCE SynthesiserVoice
float bowSpeedMultiplier = 0.2f + 0.8f * velocityNorm;  // minimum 20% speed
float v_bow_target = bowSpeedParam * bowSpeedMultiplier;
```

---

## JUCE API Patterns (JUCE 8.0.4)

### DelayLine with Thiran Interpolation

```cpp
// Declaration
juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Thiran> bridgeDelay;
juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Thiran> neckDelay;

// Preparation (in voice::prepare or waveguide::prepare)
int maxDelaySamples = static_cast<int>(sampleRate / 20.0) + 100;  // lowest note ~20Hz

juce::dsp::ProcessSpec spec { sampleRate, static_cast<juce::uint32>(maxBlockSize), 1 };
bridgeDelay.prepare(spec);
neckDelay.prepare(spec);
bridgeDelay.setMaximumDelayInSamples(maxDelaySamples);
neckDelay.setMaximumDelayInSamples(maxDelaySamples);

// Set delay length (call on trigger and when bowPosition changes)
bridgeDelay.setDelay(bridgeDelaySamples);
neckDelay.setDelay(neckDelaySamples);

// Per-sample usage (push/pop pattern)
float out = bridgeDelay.popSample(0);     // read from delay output
bridgeDelay.pushSample(0, newSample);      // write to delay input
```

**CRITICAL: Thiran is stateful.** From JUCE docs: "This interpolation method is stateful so is unsuitable for applications requiring fast delay modulation." For Phase 3.1 this is fine -- delay length only changes on note trigger and bow position parameter change (not per-sample). For vibrato/portamento in later phases, the neckDelay may need Lagrange3rd interpolation instead. BUT: for Phase 3.1, Thiran gives the best pitch accuracy for static pitch, which is the priority.

**Alternative consideration:** O-Lyrica uses `Lagrange3rd` because it needs smooth pitch changes for glissando. O-Bowed Phase 3.1 only has static pitch, so Thiran is correct. Phase 3.2+ (portamento/vibrato) may need to switch neckDelay to Lagrange3rd while keeping bridgeDelay as Thiran.

### IIR Filter for Bridge Loss

```cpp
// Declaration
juce::dsp::IIR::Filter<float> bridgeLossFilter;

// Preparation
juce::dsp::ProcessSpec spec { sampleRate, static_cast<juce::uint32>(maxBlockSize), 1 };
bridgeLossFilter.prepare(spec);

// Coefficient update (on parameter change, NOT per-sample)
// Option A: Use JUCE's built-in first-order lowpass
auto coeffs = juce::dsp::IIR::Coefficients<float>::makeFirstOrderLowPass(sampleRate, cutoffHz);
*bridgeLossFilter.coefficients = *coeffs;

// Option B: Custom one-pole with explicit gain control (recommended for bridge loss)
// H(z) = g * (1-p) / (1 - p*z^-1)
// This maps to IIR coefficients: b0 = g*(1-p), b1 = 0, a0 = 1, a1 = -p
float g = loopGain;  // < 1.0 for natural decay
float p = std::exp(-2.0f * juce::MathConstants<float>::pi * cutoffHz / static_cast<float>(sampleRate));
auto coeffs = new juce::dsp::IIR::Coefficients<float>(g * (1.0f - p), 0.0f, 1.0f, -p);
*bridgeLossFilter.coefficients = *coeffs;

// Per-sample usage
float filtered = bridgeLossFilter.processSample(input);
```

**Recommended: Custom one-pole (Option B)** because it separates gain (g) from brightness (p), allowing INFINITE_SUSTAIN to control g independently.

### APVTS Atomic Parameter Reading

```cpp
// Pattern from O-Lyrica -- read once per block, not per sample
void BowedStringVoice::updateParametersFromAPVTS()
{
    if (parameters == nullptr) return;

    float bowSpeed    = parameters->getRawParameterValue("bowSpeed")->load();
    float bowPressure = parameters->getRawParameterValue("bowPressure")->load();
    float bowPosition = parameters->getRawParameterValue("bowPosition")->load();
    float rosin       = parameters->getRawParameterValue("rosin")->load();
    float brightness  = parameters->getRawParameterValue("brightness")->load();
    float infSustain  = parameters->getRawParameterValue("infiniteSustain")->load();
    float outputLevel = parameters->getRawParameterValue("outputLevel")->load();

    // Update DSP components with new values
    bowModel.setBowSpeed(bowSpeed);
    bowModel.setBowPressure(bowPressure);
    waveguideString.setBowPosition(bowPosition);
    frictionModel.setRosin(rosin);
    waveguideString.setBrightness(brightness);
    waveguideString.setInfiniteSustain(infSustain);
    outputGainLinear = juce::Decibels::decibelsToGain(outputLevel);
}
```

---

## Voice Architecture

### Class Structure

```
OBowedAudioProcessor
  |-- juce::Synthesiser synthesiser
  |     |-- BowedStringSound (one instance, appliesToNote=true for all)
  |     |-- BowedStringVoice[8] (one per polyphony slot)
  |           |-- WaveguideString waveguideString
  |           |     |-- DelayLine<float, Thiran> bridgeDelay
  |           |     |-- DelayLine<float, Thiran> neckDelay
  |           |     |-- IIR::Filter<float> bridgeLossFilter
  |           |-- BowModel bowModel
  |           |-- HyperbolicFriction frictionModel
  |           |-- APVTS* parameters (non-owning pointer)
  |-- APVTS parameters
  |-- TuningEngine tuningEngine
```

### Voice Count: 8

Physical modeling bowed strings are CPU-heavy. Each voice runs:
- 2 delay line reads + 2 writes per sample
- 1 IIR filter per sample
- 1 friction computation per sample
- 1 bow envelope update per sample

At 44.1kHz, 8 voices = ~350k operations/second per voice = ~2.8M total. This is comfortable on modern hardware. 12 voices is possible but provides diminishing returns -- bowed instruments rarely need more than 4-6 simultaneous notes. Start with 8, profile, adjust if needed.

**No oversampling in Phase 3.1.** The memoryless hyperbolic friction is smooth enough that aliasing is manageable at native sample rate. Oversampling is Phase 3.5.

### Voice Lifecycle (Bowed vs Plucked)

Critical difference from O-Lyrica: bowed strings produce sound continuously while the bow is in contact. The voice is active from `startNote` until:
1. `stopNote(allowTailOff=true)`: bow lifts, short release ramp, then natural decay
2. `stopNote(allowTailOff=false)`: hard stop, immediate silence
3. Energy drops below threshold (bow lifted, string decayed)

```cpp
void BowedStringVoice::startNote(int midiNote, float velocity, ...)
{
    // Calculate frequency from MIDI note
    currentFrequency = juce::MidiMessage::getMidiNoteInHertz(midiNote);
    // (Or use TuningEngine if available)

    // Configure waveguide for this pitch
    waveguideString.trigger(currentFrequency);

    // Start bow envelope
    bowModel.startBow(velocity);
}

void BowedStringVoice::stopNote(float velocity, bool allowTailOff)
{
    if (allowTailOff)
    {
        // Start bow release -- friction drops to zero over ~30ms
        bowModel.stopBow();
        // Voice stays active until waveguide energy decays
    }
    else
    {
        // Hard stop
        clearCurrentNote();
        waveguideString.reset();
        bowModel.reset();
    }
}

void BowedStringVoice::renderNextBlock(AudioBuffer<float>& outputBuffer,
                                        int startSample, int numSamples)
{
    updateParametersFromAPVTS();

    // Check if voice should still be active
    if (!bowModel.isActive() && !waveguideString.isActive())
    {
        clearCurrentNote();
        return;
    }

    while (--numSamples >= 0)
    {
        // 1. Update bow envelope
        bowModel.updateEnvelope();
        float v_bow = bowModel.getBowVelocity();
        float F_bow = bowModel.getBowForce();

        // 2-7. Process waveguide with friction
        float sample = waveguideString.processSample(v_bow, F_bow);

        // 8. Apply output gain
        sample *= outputGainLinear;

        // 9. Write to output
        for (int ch = outputBuffer.getNumChannels(); --ch >= 0;)
            outputBuffer.addSample(ch, startSample, sample);

        ++startSample;
    }
}
```

---

## HyperbolicFriction Implementation

### The Core Friction Module

```cpp
class HyperbolicFriction
{
public:
    // Compute reflection coefficient from incoming differential velocity
    // This is the STK-style memoryless approach: no iteration needed
    float computeReflectionCoefficient(float v_delta, float F_bow) const
    {
        float absV = std::abs(v_delta);
        float mu = mu_d + (mu_s - mu_d) * v_0 / (v_0 + absV);

        // Convert friction coefficient to waveguide reflection coefficient
        // r = (1/4) * mu * F_bow / R_s
        // Physical meaning: ratio of bow interaction impedance to string impedance
        float r = 0.25f * mu * F_bow / R_s;
        float rho = r / (1.0f + r);

        return rho;  // Range: [0, ~0.5] -- bounded, stable
    }

    void setRosin(float rosinParam)
    {
        // ROSIN 0.0 = smooth (v_0 = 0.1), ROSIN 1.0 = aggressive (v_0 = 0.01)
        v_0 = 0.1f * std::exp(-4.6f * rosinParam);
    }

    void setStringImpedance(float impedance)
    {
        R_s = impedance;
    }

private:
    float mu_s = 0.8f;    // static friction coefficient
    float mu_d = 0.3f;    // dynamic friction coefficient
    float v_0  = 0.05f;   // characteristic velocity (from ROSIN param)
    float R_s  = 0.5f;    // string wave impedance (simplified constant for Phase 3.1)
};
```

### Understanding R_s (String Wave Impedance)

Physically: `R_s = sqrt(tension * linearMass)`. For Phase 3.1, use a fixed constant:
- `R_s = 0.5` is a reasonable default that produces good results
- The exact value affects the "strength" of the bow-string interaction
- Higher R_s = harder to excite (more bow force needed)
- Lower R_s = easier to excite (more sensitive to bow)
- Phase 3.2+ can make this frequency-dependent

### Why This Works Without Iteration

The STK insight: use the **incoming** differential velocity `v_delta+ = v_bow - (bridgeReflection + nutReflection)` rather than the true differential velocity. The incoming waves are already known (read from delay lines), and `v_bow` is known (from bow model). So `v_delta+` is fully determined -- no circular dependency.

The approximation error is small because the incoming velocity is a good predictor of the true velocity at the junction. The error is at most `rho * v_delta+` (the reflected component), which is bounded by 0.5 * v_delta+ given that rho < 0.5 for typical parameters.

---

## Bridge Loss Filter Parameters

### Initial Values for Natural Decay

| Parameter | Symbol | Good Starting Value | Musical Meaning |
|-----------|--------|--------------------|--------------------|
| Loop gain | g | 0.995 | Controls overall decay rate. Lower = faster decay. |
| Pole position | p | Computed from BRIGHTNESS | Controls high-frequency rolloff per cycle |
| BRIGHTNESS default | - | 8000 Hz | Warm but clear tone |

**Computing g from INFINITE_SUSTAIN parameter:**
```cpp
float g = 0.990f + 0.010f * infiniteSustainParam;
// infiniteSustain=0.0 -> g=0.990 (normal decay, ~2-4 seconds for mid-range)
// infiniteSustain=1.0 -> g=1.000 (infinite sustain, no energy loss)
```

**Computing p from BRIGHTNESS:**
```cpp
float cutoffHz = brightnessParam;  // BRIGHTNESS param IS the cutoff frequency (20-20000 Hz)
float p = std::exp(-2.0f * MathConstants<float>::pi * cutoffHz / static_cast<float>(sampleRate));
```

**Decay time estimate:** For a string at frequency f0:
- Cycles per second = f0
- Per-cycle gain = g (roughly, ignoring frequency dependence)
- Time to -60dB: `T60 = -3 / (f0 * log10(g))` seconds
- At f0=440Hz, g=0.995: T60 = -3/(440*log10(0.995)) = ~3.1 seconds
- At f0=440Hz, g=0.999: T60 = ~15.7 seconds
- At f0=110Hz (low strings), g=0.995: T60 = ~12.5 seconds (lower strings ring longer -- physically correct)

---

## File Structure Recommendation

```
plugins/O-Bowed/Source/
  |-- PluginProcessor.h           (existing -- add voice creation)
  |-- PluginProcessor.cpp         (existing -- add voice wiring)
  |-- PluginEditor.h              (existing -- no changes needed)
  |-- PluginEditor.cpp            (existing -- no changes needed)
  |-- BowedStringSound.h          (existing -- no changes needed)
  |-- BowedStringVoice.h          (NEW -- SynthesiserVoice subclass)
  |-- BowedStringVoice.cpp        (NEW -- voice implementation)
  |-- DSP/
  |   |-- WaveguideString.h       (NEW -- bidirectional delay lines + bridge filter)
  |   |-- WaveguideString.cpp     (NEW -- waveguide implementation)
  |   |-- BowModel.h              (NEW -- bow envelope + velocity/force signals)
  |   |-- BowModel.cpp            (NEW -- bow model implementation)
  |   |-- HyperbolicFriction.h    (NEW -- friction curve + reflection coefficient)
  |-- TuningEngine.h              (existing stub)
  |-- ScaleGenerator.h            (existing stub)
  |-- EmbeddedTunings.h           (existing stub)
  |-- TuningExporter.h            (existing stub)
```

### Rationale

- **BowedStringVoice** is the voice-level orchestrator (like HarpSynthVoice). Owns WaveguideString, BowModel, and HyperbolicFriction. Reads APVTS parameters and drives the per-sample loop.
- **WaveguideString** encapsulates the delay lines + bridge loss filter + termination logic. It does NOT know about friction -- it exposes `popSample`/`pushSample`-style methods for the voice to use.
- **BowModel** is the exciter -- produces v_bow and F_bow signals from MIDI input and envelope state.
- **HyperbolicFriction** is a pure function module (no state for memoryless tier). Computes reflection coefficient from v_delta and F_bow.

**Alternative: monolithic WaveguideString that includes friction.** This is the O-Lyrica pattern (WaveguideString includes PluckExciter). But for O-Bowed, separating friction from waveguide is cleaner because:
1. Friction model will be swappable (core/enhanced/quality tiers in Phase 3.4)
2. The voice needs direct access to v_bow and F_bow for the scattering junction
3. The friction computation sits BETWEEN the delay line read and write

**Recommended: voice owns the per-sample loop, calling waveguide and friction as needed.**

### CMakeLists.txt Additions

```cmake
target_sources(O-Bowed
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/BowedStringSound.h
        Source/BowedStringVoice.h       # NEW
        Source/BowedStringVoice.cpp     # NEW
        Source/DSP/WaveguideString.h    # NEW
        Source/DSP/WaveguideString.cpp  # NEW
        Source/DSP/BowModel.h           # NEW
        Source/DSP/BowModel.cpp         # NEW
        Source/DSP/HyperbolicFriction.h # NEW
        # Tuning module files...
)
```

---

## Common Pitfalls

### Pitfall 1: Denormal CPU Spikes

**What goes wrong:** After the bow lifts and the string decays, the waveguide signal approaches zero but never reaches it exactly. Denormalized floating-point numbers (< ~1.2e-38) cause 10-100x CPU spikes on x86.

**Why it happens:** IIR filter state accumulates tiny values. Delay lines hold near-zero samples.

**How to avoid:**
1. Use `juce::ScopedNoDenormals` at the top of `processBlock()` (already in processor)
2. Add explicit denormal flush in the per-sample loop:
   ```cpp
   if (std::abs(output) < 1e-15f) output = 0.0f;
   ```
3. Clear voice when energy drops below threshold:
   ```cpp
   if (!bowModel.isActive() && !waveguideString.isActive())
       clearCurrentNote();
   ```

**Warning signs:** CPU meter shows spikes when many notes have been played and released.

### Pitfall 2: Pitch Accuracy and Filter Group Delay

**What goes wrong:** The bridge loss filter adds group delay to the waveguide loop, effectively lengthening the delay line. Without compensation, all notes play flat. Changing BRIGHTNESS changes the pitch.

**Why it happens:** A first-order lowpass at cutoff f_c adds approximately `sampleRate / (2*pi*f_c)` samples of group delay at DC. For f_c=2000Hz at 44.1kHz: ~3.5 samples. At f_c=500Hz: ~14 samples. That is significant relative to a total delay of ~100 samples (440Hz).

**How to avoid:** Subtract filter group delay from total delay before splitting:
```cpp
float filterDelay = sampleRate / (2.0f * pi * cutoffHz);
float compensatedDelay = totalDelay - filterDelay;
```

Recalculate delay lengths whenever BRIGHTNESS changes. O-Lyrica does this -- follow the same pattern.

**Warning signs:** Notes sound flat. Changing brightness knob changes pitch.

### Pitfall 3: Delay Line Minimum Length

**What goes wrong:** Very high notes (above ~4kHz) require very short delay lines. With filter group delay compensation, the compensated delay can go negative.

**Why it happens:** At f0=4000Hz, totalDelay = 44100/4000 = ~11 samples. If filter group delay is 5 samples, compensated delay is 6 samples, split as 3+3. That works. But at f0=8000Hz with 5 samples group delay, compensated delay is 0.5 -- too short.

**How to avoid:** Clamp minimum delay to 2.0 samples per rail:
```cpp
float railDelay = std::max(2.0f, compensatedDelay * bowPosition);
```
Also, limit the BRIGHTNESS parameter interaction for very high notes.

**Warning signs:** Clicking/artifacts on very high notes.

### Pitfall 4: Bow Force = 0 After Note-Off

**What goes wrong:** If bow force drops to exactly 0.0, the friction computation divides by zero or produces NaN (depending on implementation).

**Why it happens:** `r = 0.25 * mu * F_bow / R_s` -- if F_bow = 0, r = 0, rho = 0. This is actually CORRECT behavior (no bow interaction). But ensure the release envelope smoothly approaches zero without creating transient artifacts.

**How to avoid:** The formula `rho = r / (1+r)` is safe at r=0 (returns 0). Just ensure no division by F_bow elsewhere. When bowModel.isActive() returns false and v_bow is negligible, the voice should check waveguide energy and clear itself.

### Pitfall 5: Thread Safety of Filter Coefficient Updates

**What goes wrong:** Updating IIR filter coefficients from the message thread while the audio thread is calling `processSample()` causes race conditions -- garbled audio or crashes.

**Why it happens:** `juce::dsp::IIR::Filter` stores coefficients as a `ReferenceCountedObjectPtr`. Assignment is not atomic from the audio thread's perspective.

**How to avoid (O-Lyrica v1.7.10 pattern):**
- Store pending cutoff values in `std::atomic<float>` members
- Set `std::atomic<bool> filterUpdatePending`
- At the start of `processSample()`, check the flag and apply updates
- This ensures coefficient changes happen on the audio thread only

### Pitfall 6: Thiran Interpolation State on Delay Change

**What goes wrong:** Changing Thiran delay length causes a click because Thiran allpass has internal state.

**Why it happens:** Thiran interpolation is a first-order allpass filter. Changing the delay (and thus the allpass coefficient) creates a transient in the filter state.

**How to avoid:** For Phase 3.1 (static pitch per note), this is a non-issue -- delay is set once at `trigger()` and the delay lines are reset. For Phase 3.2+ with pitch modulation, consider using Lagrange3rd for the modulated delay line (neckDelay) while keeping Thiran for the static delay line (bridgeDelay).

### Pitfall 7: Initial Silence on Note Start

**What goes wrong:** The waveguide delay lines start empty (all zeros). The bow friction computation depends on waves arriving from the delay lines. With empty delay lines, v_string_incoming = 0, so v_delta = v_bow. The friction computation returns a nonzero reflection, but there is nothing to reflect yet.

**Why it happens:** It takes one full round-trip through the delay lines for the initial excitation to propagate.

**How to avoid:** This is actually physically correct and self-resolving. The sequence is:
1. First sample: v_delta = v_bow (nothing in delay lines), newVelocity = v_bow * rho. This small signal enters both delay lines.
2. After one round-trip: the signal returns, creating the first reflection. v_string_incoming becomes nonzero.
3. Over 5-20 cycles: Helmholtz motion builds up naturally.

The bow attack envelope handles the perceptual startup gracefully -- the gradual ramp of v_bow from 0 to target means the initial excitation starts small and grows, just like a real bow engaging a string.

---

## Initial Parameter Values and Ranges

### For Testing Phase 3.1

| Parameter | Value | Why |
|-----------|-------|-----|
| BOW_SPEED | 0.2 m/s | Moderate bowing -- clear Helmholtz motion |
| BOW_PRESSURE | 0.5 N | Middle of Schelleng range |
| BOW_POSITION | 0.12 | Standard violin ordinario position |
| ROSIN | 0.5 | Moderate friction curve (v_0 ~ 0.032) |
| BRIGHTNESS | 8000 Hz | Warm but clear |
| INFINITE_SUSTAIN | 0.0 | Natural decay |
| OUTPUT_LEVEL | 0 dB | Unity gain |
| mu_s | 0.8 | Standard static friction |
| mu_d | 0.3 | Standard dynamic friction |
| R_s | 0.5 | Default string impedance |

### Expected Sound

With these values, playing a note at A4 (440Hz) should produce:
- A sawtooth-ish waveform (Helmholtz motion)
- Attack transient of ~20ms before stable oscillation
- Natural decay of ~3-4 seconds (from bridge loss filter)
- Brightness rolling off gradually above ~8kHz

If the sound is harsh/buzzy: reduce BOW_PRESSURE or increase BOW_POSITION (move toward fingerboard).
If no sound: check that v_bow is nonzero and F_bow is nonzero.
If pure tone (no harmonics): reduce BOW_POSITION (move toward bridge) or increase ROSIN.

---

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| STK 4th-power bow table | Hyperbolic friction curve | Ongoing shift since 2000s | More physically grounded, same CPU cost |
| Fixed delay interpolation | Thiran allpass for fractional delay | JUCE 7+ | Sub-cent tuning accuracy |
| Lagrange3rd everywhere | Thiran for static, Lagrange for modulated | Best practice | Better pitch accuracy for static delays |
| Single friction model | Tiered friction (memoryless -> state-based) | ~2019 (Willemsen) | Quality/CPU tradeoff at user's choice |

---

## Sources

### Primary (HIGH confidence)
- `plugins/O-Lyrica/Source/DSP/WaveguideString.h/.cpp` -- Verified bidirectional waveguide pattern with filter group delay compensation, delay line usage, energy tracking
- `plugins/O-Lyrica/Source/HarpSynthVoice.h/.cpp` -- Verified SynthesiserVoice lifecycle, APVTS reading pattern, renderNextBlock per-sample loop
- `plugins/O-Bowed/.planning/research/ARCHITECTURE.md` -- Full DSP architecture specification (locked)
- `research/bow-string-friction-models.md` -- Friction model equations, STK algorithm, scattering junction math
- `/Users/taylorbrook/JUCE/modules/juce_dsp/processors/juce_DelayLine.h` -- JUCE 8 DelayLine API (push/pop pattern, Thiran docs)
- `/Users/taylorbrook/JUCE/modules/juce_dsp/processors/juce_IIRFilter.h` -- JUCE 8 IIR filter API (makeFirstOrderLowPass, coefficient access)

### Secondary (MEDIUM confidence)
- STK Bowed.cpp algorithm reconstruction from friction models research -- cross-verified with ARCHITECTURE.md spec
- Bridge loss filter parameters (g=0.995 starting value) -- derived from O-Lyrica decay time calculations, not directly measured

### Tertiary (LOW confidence)
- R_s = 0.5 default value -- reasonable simplification for Phase 3.1 but not physically calibrated to any specific string type. May need tuning during sound design.

## Metadata

**Confidence breakdown:**
- Algorithm specification: HIGH -- directly from ARCHITECTURE.md + friction research + O-Lyrica reference code
- JUCE API patterns: HIGH -- verified against local JUCE 8.0.4 source headers
- Voice architecture: HIGH -- follows proven O-Lyrica pattern with bowed-specific adaptations
- Pitfalls: HIGH -- most are directly observed in O-Lyrica development history (version comments in code)
- Initial parameter values: MEDIUM -- reasonable starting points but will need ear-tuning

**Research date:** 2026-04-04
**Valid until:** 2026-05-04 (stable domain, JUCE 8 API is mature)
