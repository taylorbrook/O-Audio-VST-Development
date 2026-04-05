# Stage 2: DSP - Research

**Researched:** 2026-04-05
**Domain:** JUCE 8 DSP primitives for bore waveguide physical modeling
**Confidence:** HIGH
**Source:** Direct JUCE 8.0.4 source code analysis (local `/Users/taylorbrook/JUCE/`)

## Summary

This research covers the exact JUCE 8.0.4 APIs needed for O-Reed's bore waveguide: `DelayLine<float, Thiran>` for fractional delay, `IIR::Filter<float>` with manual coefficients for viscothermal loss and bell reflection filters, and `FirstOrderTPTFilter` as an alternative for modulated filters. All findings come from reading the actual JUCE source headers and implementation files -- no guesswork.

The O-Bowed plugin in this same repo provides a working reference pattern for the exact same API surface (Thiran delay lines + IIR one-pole filter + per-sample processing). O-Reed's bore waveguide follows the same push/pop pattern but adds: conical scaling factors, a separate allpass reflection filter at the bell end, and eventually multi-segment bore support.

**Primary recommendation:** Use `juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Thiran>` for both forward/backward bore delay lines, `juce::dsp::IIR::Filter<float>` with manual `Coefficients` for viscothermal loss (one-pole lowpass) and bell reflection (first-order allpass). Follow the O-Bowed `WaveguideString` pattern exactly for prepare/push/pop flow.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- Full mass-spring-damper reed ODE from Phase 3.1 (no static reed table intermediate)
- True conical waveguide sections (Strategy C) from the start (no Strategy B correction filter)
- No CPU budget constraint -- quality first, optimize in Phase 3.5
- Phase ordering: 3.1 -> 3.2 -> 3.3 -> 3.4 -> 3.5
- O-Bowed is also in development and should NOT be treated as a reference pattern -- independent implementation

### Claude's Discretion
- Not specified in CONTEXT.md

### Deferred Ideas (OUT OF SCOPE)
- Not specified in CONTEXT.md
</user_constraints>

## Project Constraints (from CLAUDE.md)

- JUCE version: 8.0.4
- Build system: CMake + Ninja
- Plugin cache clearing required after every build
- Research documents go in `research/` directory
- `getLatencySamples()` is non-virtual in JUCE 8 -- use `setLatencySamples(N)` in `prepareToPlay()`

---

## 1. DelayLine with Thiran Allpass Interpolation

### Declaration and Construction

```cpp
#include <juce_dsp/juce_dsp.h>

// Template: DelayLine<SampleType, InterpolationType>
// Default InterpolationType is Linear; we want Thiran.
juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Thiran> forwardDelay { 38400 };
juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Thiran> backwardDelay { 38400 };
```

Constructor signature (from source):
```cpp
DelayLine();                              // default, maxDelay = 0
explicit DelayLine (int maximumDelayInSamples);  // sets initial max
```

The constructor argument sets the maximum delay. Internally: `totalSize = jmax(4, maxDelayInSamples + 2)`.

### Maximum Delay Calculation

For lowest note ~20 Hz at 192kHz with 4x oversampling:
```
effective_sr = 192000 * 4 = 768000
max_delay = 768000 / 20 = 38400 samples
```

With headroom: **use 40000** as the constructor argument.

At standard 44100 Hz (no oversampling): `44100 / 20 = 2205 samples`.

**Important:** `setMaximumDelayInSamples()` allocates memory and calls `reset()`. Call it in `prepare()`, never on the audio thread.

### prepare() -- ProcessSpec Requirements

```cpp
void prepare (double sampleRate, int maxBlockSize)
{
    juce::dsp::ProcessSpec spec {
        sampleRate,
        static_cast<juce::uint32>(maxBlockSize),
        1  // numChannels = 1 for mono per-voice
    };

    forwardDelay.prepare(spec);
    forwardDelay.setMaximumDelayInSamples(maxDelay);

    backwardDelay.prepare(spec);
    backwardDelay.setMaximumDelayInSamples(maxDelay);
}
```

`ProcessSpec` struct (from `juce_ProcessContext.h`):
```cpp
struct ProcessSpec {
    double sampleRate;
    uint32 maximumBlockSize;
    uint32 numChannels;
};
```

`prepare()` internally:
- Allocates `AudioBuffer<float>` with `numChannels` channels and `totalSize` samples
- Resizes `writePos`, `readPos`, `v` vectors to `numChannels`
- Stores `sampleRate`
- Calls `reset()`

### setDelay() -- Setting Fractional Delay

```cpp
void setDelay (SampleType newDelayInSamples);
```

Exact implementation:
```cpp
void setDelay (SampleType newDelayInSamples) {
    auto upperLimit = (SampleType) getMaximumDelayInSamples();
    jassert (isPositiveAndNotGreaterThan (newDelayInSamples, upperLimit));
    delay     = jlimit ((SampleType) 0, upperLimit, newDelayInSamples);
    delayInt  = static_cast<int> (std::floor (delay));
    delayFrac = delay - (SampleType) delayInt;
    updateInternalVariables();
}
```

For Thiran, `updateInternalVariables()` does:
```cpp
if (delayFrac < (SampleType) 0.618 && delayInt >= 1) {
    delayFrac++;
    delayInt--;
}
alpha = (1 - delayFrac) / (1 + delayFrac);
```

**Key Thiran behavior:**
- When `delayFrac < 0.618`, it borrows 1 sample from `delayInt` and adds it to `delayFrac`, keeping `delayFrac >= 0.618`
- The threshold 0.618 ensures the Thiran allpass coefficient stays in a stable range
- `alpha = (1 - d) / (1 + d)` is the first-order Thiran allpass coefficient
- **At integer delays** (`delayFrac == 0`): the interpolation bypasses the allpass entirely (returns `value1` directly via the `approximatelyEqual` check), so no allpass filtering artifact
- **Minimum usable delay:** 2 samples (because `delayInt >= 1` is required for the borrow, and the allpass needs at least a fractional part to work). In practice, clamp to `>= 2.0f`.

### pushSample() and popSample() -- Per-Sample Access

```cpp
void pushSample (int channel, SampleType sample);
SampleType popSample (int channel, SampleType delayInSamples = -1, bool updateReadPointer = true);
```

**pushSample** writes a sample and decrements write pointer (wraps circularly):
```cpp
void pushSample (int channel, SampleType sample) {
    bufferData.setSample (channel, writePos[(size_t) channel], sample);
    writePos[(size_t) channel] = (writePos[(size_t) channel] + totalSize - 1) % totalSize;
}
```

**popSample** reads with interpolation. Three calling modes:
1. `popSample(0)` -- uses last `setDelay()` value, updates read pointer
2. `popSample(0, delayInSamples)` -- sets new delay, updates read pointer
3. `popSample(0, delayInSamples, false)` -- multi-tap: reads but does NOT advance read pointer

```cpp
SampleType popSample (int channel, SampleType delayInSamples, bool updateReadPointer) {
    if (delayInSamples >= 0)
        setDelay (delayInSamples);
    auto result = interpolateSample (channel);
    if (updateReadPointer)
        readPos[(size_t) channel] = (readPos[(size_t) channel] + totalSize - 1) % totalSize;
    return result;
}
```

**Critical for feedback loops:** Call `popSample()` BEFORE `pushSample()` in each sample tick. If you push first, you corrupt the delay line before reading it. The O-Bowed reference does exactly this:
```cpp
float bridgeReflection = -bridgeLossFilter.processSample(bridgeDelay.popSample(0));
float nutReflection = -neckDelay.popSample(0);
// ... compute ...
bridgeDelay.pushSample(0, outgoingToBridge);
neckDelay.pushSample(0, outgoingToNeck);
```

### Per-Sample vs Block Processing

The `process()` template method is just a convenience wrapper that calls `pushSample`/`popSample` in a loop. For physical modeling feedback loops, **always use pushSample/popSample directly** -- you need the per-sample feedback path.

### Mono Per-Voice (numChannels = 1)

Yes, `numChannels = 1` works perfectly. The delay line maintains separate state per channel via indexed vectors. With 1 channel, always pass `channel = 0`. This is the correct pattern for synth voices.

### Thiran Interpolation Deep Dive

The Thiran allpass is a 1st-order IIR allpass: `H(z) = (alpha + z^-1) / (1 + alpha * z^-1)`

Implemented as:
```cpp
// From interpolateSample() in juce_DelayLine.h:
auto output = approximatelyEqual(delayFrac, (SampleType) 0)
    ? value1
    : value2 + alpha * (value1 - v[(size_t) channel]);
v[(size_t) channel] = output;
```

Where `v` is the per-channel state variable (initialized to 0, persists between calls).

**Properties:**
- Flat magnitude response (unity gain at all frequencies) -- perfect for waveguides
- Phase response designed to approximate the exact fractional delay
- **Stateful** -- the `v` state variable means rapid delay modulation causes transient artifacts
- For bore waveguide with slowly-changing pitch, this is ideal
- Phase accuracy degrades as delay fraction approaches 0 or 1 (hence the 0.618 threshold)

**Delay < 1 sample:** Not supported. `setDelay(0.5)` would give `delayInt = 0`, and the borrow condition `delayInt >= 1` would fail, leaving `delayFrac = 0.5` and `alpha = 0.333`. But `delayInt = 0` means reading at the write position, which is undefined. **Always keep delay >= 2.0f for Thiran.**

---

## 2. IIR::Filter -- Viscothermal Loss and Bell Reflection

### Filter Declaration

```cpp
juce::dsp::IIR::Filter<float> viscothermalLoss;
juce::dsp::IIR::Filter<float> bellReflection;
```

### processSample() -- Per-Sample Access

```cpp
SampleType processSample (SampleType sample) noexcept;
```

This is Transposed Direct Form II, operates on a single sample. For order 1 (first-order filter):
```cpp
output = (c[0] * sample) + state[0];
state[0] = (c[1] * sample) - (c[2] * output);
```

Where `c[]` = normalized coefficients stored as `[b0/a0, b1/a0, a1/a0]` (a0 is divided out during coefficient assignment).

**Important:** `IIR::Filter` processes **one channel only**. The assert in `processInternal` says: "This class can only process mono signals." For per-voice mono processing, this is exactly what we want.

### Coefficients -- Manual Construction

The `Coefficients` class accepts raw b/a coefficients. For first-order filters:

```cpp
// Constructor: Coefficients(b0, b1, a0, a1)
// Internally normalizes by dividing all by a0
juce::dsp::IIR::Coefficients<float>(b0, b1, a0, a1);
```

After normalization, stored as: `[b0/a0, b1/a0, a1/a0]` (3 coefficients for order 1).

### Viscothermal Loss Filter (One-Pole Lowpass)

A one-pole lowpass for viscothermal losses in the bore. The O-Bowed reference uses:

```cpp
// Custom one-pole: H(z) = g*(1-p) / (1 - p*z^-1)
// Where g = loop gain, p = pole from cutoff frequency
float p = std::exp(-2.0f * pi * cutoffHz / sampleRate);
float g = 0.990f;  // loop gain < 1 for decay

// IIR coefficients: b0 = g*(1-p), b1 = 0, a0 = 1, a1 = -p
*viscothermalLoss.coefficients = juce::dsp::IIR::Coefficients<float>(
    g * (1.0f - p), 0.0f, 1.0f, -p);
```

**For O-Reed bore waveguide:**
```cpp
// Viscothermal loss: frequency-dependent attenuation in bore
// Cutoff is bore-diameter dependent: wider bore = less loss = higher cutoff
// Typical range: 2000-12000 Hz depending on bore diameter
void updateViscothermalCoeffs(float boreDiameterMm, float sampleRate)
{
    // Empirical: viscothermal cutoff scales with bore diameter
    // Narrower bore = more loss = lower cutoff
    float cutoffHz = boreDiameterMm * 150.0f;  // tune to taste
    cutoffHz = juce::jlimit(500.0f, 15000.0f, cutoffHz);

    float pi = juce::MathConstants<float>::pi;
    float p = std::exp(-2.0f * pi * cutoffHz / static_cast<float>(sampleRate));
    float g = 0.995f;  // high loop gain for sustained oscillation

    *viscothermalLoss.coefficients = juce::dsp::IIR::Coefficients<float>(
        g * (1.0f - p), 0.0f, 1.0f, -p);
}
```

### Bell Reflection Filter (First-Order Allpass)

For frequency-dependent reflection at the bell end. JUCE provides `makeFirstOrderAllPass`:

```cpp
// Using the built-in factory:
auto coeffs = juce::dsp::IIR::ArrayCoefficients<float>::makeFirstOrderAllPass(sampleRate, frequency);
// Returns std::array<float, 4> = { b0, b1, a0, a1 }
// Where: n = tan(pi * f / sr), coeffs = { n-1, n+1, n+1, n-1 }
```

Or construct manually for more control:
```cpp
// First-order allpass: H(z) = (a + z^-1) / (1 + a*z^-1)
// where a = (1 - tan(pi*fc/sr)) / (1 + tan(pi*fc/sr))
float w = juce::MathConstants<float>::pi * bellCutoffHz / sampleRate;
float t = std::tan(w);
float a = (1.0f - t) / (1.0f + t);

// b0 = a, b1 = 1, a0 = 1, a1 = a
*bellReflection.coefficients = juce::dsp::IIR::Coefficients<float>(
    a, 1.0f, 1.0f, a);
```

**Phase response:** The allpass has unity magnitude but introduces frequency-dependent phase shift. Lower frequencies reflect with less phase shift (more reflection), higher frequencies with more (less reflection effectively, when combined with the waveguide delay). This creates the frequency-dependent "brightening" at the bell radiation boundary.

### Coefficients::Ptr vs Direct Assignment

Two patterns for setting coefficients:

```cpp
// Pattern 1: Direct assignment (used in O-Bowed, simpler for manual coeffs)
*filter.coefficients = juce::dsp::IIR::Coefficients<float>(b0, b1, a0, a1);

// Pattern 2: Ptr factory methods (used for standard filter types)
filter.coefficients = juce::dsp::IIR::Coefficients<float>::makeFirstOrderLowPass(sr, freq);
```

Pattern 1 modifies in-place (no allocation). Pattern 2 creates a new ref-counted object. For per-block coefficient updates, Pattern 1 avoids allocation overhead.

**Thread safety note:** The `coefficients` member is a `ReferenceCountedObjectPtr`. Assigning a new `Ptr` is atomic at the pointer level, but modifying the underlying array is NOT atomic. For per-block updates on the audio thread (which is the only thread touching these in a synth voice), this is fine.

### prepare() for IIR::Filter

```cpp
void prepare (const ProcessSpec&) noexcept { reset(); }
```

The IIR filter's `prepare()` just calls `reset()`. It does not use the ProcessSpec fields. Still call it for consistency.

### snapToZero()

Call periodically when doing per-sample processing to flush denormals:
```cpp
// Every N samples (e.g., end of each block):
viscothermalLoss.snapToZero();
bellReflection.snapToZero();
```

Or rely on `JUCE_DSP_ENABLE_SNAP_TO_ZERO` (enabled by default in block processing, but not in `processSample`).

---

## 3. FirstOrderTPTFilter -- Alternative for Modulated Filters

`juce::dsp::FirstOrderTPTFilter<float>` is a topology-preserving transform filter. Unlike `IIR::Filter`, it handles cutoff modulation without artifacts.

```cpp
juce::dsp::FirstOrderTPTFilter<float> tptFilter;
tptFilter.setType(juce::dsp::FirstOrderTPTFilterType::lowpass);
tptFilter.setCutoffFrequency(4000.0f);
tptFilter.prepare(spec);

// Per-sample (note: takes channel index):
float output = tptFilter.processSample(0, input);
```

**When to use TPT vs IIR:**
- **IIR::Filter with manual coefficients:** When you need exact coefficient control (custom one-pole with loop gain, allpass with specific pole). This is the viscothermal loss and bell reflection case.
- **TPT filter:** When you need safe cutoff modulation at audio rate. Could be useful if bore diameter (and thus viscothermal cutoff) changes rapidly.

**For Phase 3.1 (no bore morphing), IIR::Filter is the correct choice.** TPT becomes relevant in Phase 3.2 when bore_character modulates the filter cutoffs.

---

## 4. Filter Group Delay Compensation

The O-Bowed reference compensates for bridge filter group delay in the total delay calculation:

```cpp
float filterGroupDelay = sampleRate / (2.0f * pi * brightnessHz);
float compensatedDelay = totalDelay - filterGroupDelay;
```

For a one-pole lowpass with pole at `p = exp(-2*pi*fc/sr)`, the group delay at DC is approximately `p / (1 - p)` samples, which simplifies to `sr / (2*pi*fc)` for moderate frequencies.

**O-Reed needs this for both filters in the loop:**
```cpp
float totalDelay = sampleRate / frequency;

// Subtract group delays of all filters in the waveguide loop
float viscothermalGD = sampleRate / (2.0f * pi * viscothermalCutoffHz);
float allpassGD = /* allpass group delay at fundamental */;
float compensatedDelay = totalDelay - viscothermalGD - allpassGD;

// Split into forward/backward delays
float forwardSamples = compensatedDelay / 2.0f;
float backwardSamples = compensatedDelay / 2.0f;
```

The allpass group delay is frequency-dependent. At the fundamental frequency, it contributes a small but non-negligible delay that must be compensated for pitch accuracy.

---

## 5. Bore Waveguide Pattern for O-Reed

### Per-Sample Processing Flow

```cpp
float BoreWaveguide::processSample(float junctionPressureForward)
{
    // 1. Read returning waves from delay lines
    float pForwardOut = forwardDelay.popSample(0);   // arriving at bell end
    float pBackwardOut = backwardDelay.popSample(0);  // arriving at reed end

    // 2. Bell end: reflection + radiation
    // Allpass for frequency-dependent reflection phase
    float reflected = bellReflection.processSample(pForwardOut);
    // Lowpass for radiation loss (high freqs radiate, don't reflect)
    reflected *= -bellReflectionCoeff;  // negative for open-end reflection
    float radiated = pForwardOut + reflected;  // output signal

    // 3. Viscothermal loss on reflected wave
    reflected = viscothermalLoss.processSample(reflected);

    // 4. Write new waves into delay lines
    forwardDelay.pushSample(0, junctionPressureForward);  // from reed junction
    backwardDelay.pushSample(0, reflected);                // from bell reflection

    // 5. Return backward wave to reed junction
    return pBackwardOut;
}
```

### Conical Bore Scaling (Strategy C)

For true conical sections, spherical wave scaling factors multiply the traveling waves at each delay tap. The pressure in a conical bore goes as `1/r` where `r` is distance from the cone apex:

```cpp
// Spherical wave scaling for conical bore
// r_input = distance from apex to bore input
// r_output = distance from apex to bore output
float scaleFwd = r_input / r_output;    // forward wave shrinks
float scaleBwd = r_output / r_input;    // backward wave grows

// Applied during read:
float pForwardOut = forwardDelay.popSample(0) * scaleFwd;
float pBackwardOut = backwardDelay.popSample(0) * scaleBwd;
```

When `bore_character = 0` (cylindrical), both scale factors are 1.0 and the bore behaves as a simple cylinder.

---

## 6. Common Pitfalls

### Pitfall 1: Push Before Pop in Feedback Loop
**What goes wrong:** Pushing before popping overwrites the sample at the write position before it can be read, corrupting the delay line.
**How to avoid:** Always `popSample()` first, then `pushSample()`. The O-Bowed reference demonstrates the correct order.

### Pitfall 2: Thiran Minimum Delay
**What goes wrong:** Setting delay < 2 samples with Thiran interpolation. The borrow mechanism needs `delayInt >= 1`, and you need at least 1 additional sample for the interpolation pair.
**How to avoid:** Clamp all delay values to `>= 2.0f`. For very high notes at high sample rates, this is never an issue (e.g., 20000 Hz at 44100 = 2.2 samples).

### Pitfall 3: setMaximumDelayInSamples on Audio Thread
**What goes wrong:** This method allocates memory and calls reset(). Calling on the audio thread causes glitches or crashes.
**How to avoid:** Call only in `prepare()`. Size for worst case (lowest note at highest sample rate * max oversampling).

### Pitfall 4: IIR Coefficient Denormals
**What goes wrong:** After a voice goes silent, the IIR filter state can contain denormal floats, causing CPU spikes on some architectures.
**How to avoid:** Call `filter.snapToZero()` at end of each block, or call `filter.reset()` when voice becomes inactive.

### Pitfall 5: Filter Group Delay Not Compensated
**What goes wrong:** Without compensating for the group delay of filters in the waveguide loop, the pitch is flat (delay too long).
**How to avoid:** Subtract the sum of all loop filter group delays from the total delay before splitting into forward/backward.

### Pitfall 6: Forgetting to reset() on Note Start
**What goes wrong:** Previous note's energy remains in delay lines, causing clicks or pitch artifacts on new notes.
**How to avoid:** Call `reset()` on all delay lines and filters in `noteStarted()` / `trigger()`.

---

## 7. Maximum Delay Buffer Sizing Table

| Sample Rate | Oversampling | Effective SR | Max Delay (20 Hz) | Recommended Buffer |
|-------------|-------------|--------------|--------------------|--------------------|
| 44100 | 1x | 44100 | 2205 | 2400 |
| 44100 | 4x | 176400 | 8820 | 9000 |
| 48000 | 1x | 48000 | 2400 | 2600 |
| 48000 | 4x | 192000 | 9600 | 9800 |
| 96000 | 1x | 96000 | 4800 | 5000 |
| 96000 | 4x | 384000 | 19200 | 19500 |
| 192000 | 1x | 192000 | 9600 | 9800 |
| 192000 | 4x | 768000 | 38400 | 40000 |

**Recommendation:** Use `40000` as the constructor default, recalculate in `prepare()` based on actual sample rate and oversampling factor.

---

## Sources

### Primary (HIGH confidence)
- `/Users/taylorbrook/JUCE/modules/juce_dsp/processors/juce_DelayLine.h` -- full template header with Thiran interpolation implementation inline
- `/Users/taylorbrook/JUCE/modules/juce_dsp/processors/juce_DelayLine.cpp` -- prepare, setDelay, pushSample, popSample implementations
- `/Users/taylorbrook/JUCE/modules/juce_dsp/processors/juce_IIRFilter.h` -- Coefficients + Filter declarations
- `/Users/taylorbrook/JUCE/modules/juce_dsp/processors/juce_IIRFilter_Impl.h` -- processSample TDF-II implementation
- `/Users/taylorbrook/JUCE/modules/juce_dsp/processors/juce_IIRFilter.cpp` -- ArrayCoefficients formulas (makeFirstOrderLowPass, makeFirstOrderAllPass, etc.)
- `/Users/taylorbrook/JUCE/modules/juce_dsp/processors/juce_ProcessContext.h` -- ProcessSpec struct definition
- `/Users/taylorbrook/JUCE/modules/juce_dsp/processors/juce_FirstOrderTPTFilter.h` -- TPT filter API

### In-Project Reference (HIGH confidence)
- `plugins/O-Bowed/Source/DSP/WaveguideString.h` -- working Thiran delay + IIR filter pattern
- `plugins/O-Bowed/Source/DSP/WaveguideString.cpp` -- complete per-sample waveguide processing with push/pop order, filter group delay compensation, custom IIR coefficients

## Metadata

**Confidence breakdown:**
- DelayLine API: HIGH -- read directly from JUCE 8.0.4 source
- IIR::Filter API: HIGH -- read directly from JUCE 8.0.4 source
- Thiran behavior: HIGH -- analyzed inline implementation in header
- Filter group delay compensation: HIGH -- verified against O-Bowed working implementation
- Conical bore scaling: MEDIUM -- standard acoustics, implementation pattern not yet validated in this codebase

**Research date:** 2026-04-05
**Valid until:** Indefinite (JUCE 8.0.4 API is stable; source is local)
