# Stage 2 DSP: Phase 2.1 Core Vocal Engine - Research

**Researched:** 2026-04-04
**Domain:** DSP -- Source-filter vocal synthesis (LF glottal wavetable, formant filter bank, vowel morphing, ADSR)
**Confidence:** HIGH

## Summary

Phase 2.1 implements five DSP components that together produce the core vocal sound: LFGlottalSource (mipmapped wavetable), AspirationNoise, FormantFilterBank (5 parallel BPFs), VowelMorpher (Shepard interpolation), and VoiceEnvelope (juce::ADSR). The most novel piece is the mipmapped wavetable approach for the LF glottal source, which replaces the direct LF + PolyBLEP architecture originally specified.

This project already has a proven mipmapped wavetable implementation in O-Prism (WavetableData + WavetableGenerator) that uses 2048-sample tables, 10 mipmap levels, FFT-based bandwidth limiting, guard samples, and trilinear interpolation. The O-Formant wavetable adapts this pattern but replaces frame-position interpolation with Rd-value interpolation. The GOLF paper (2023/2024) validates K=100 Rd values with L=2048 samples and log-spaced Rd sampling.

All JUCE APIs needed for this phase have been verified against the local JUCE 8.0.4 source: `ArrayCoefficients<float>::makeBandPass` returns `std::array<float, 6>`, `juce::ADSR` has `isActive()`, `noteOn()`, `noteOff()`, `getNextSample()`, and `SmoothedValue` has `reset(sampleRate, rampTime)`, `setTargetValue()`, `getNextValue()`.

**Primary recommendation:** Adapt the O-Prism WavetableData/Generator pattern for the glottal wavetable, using 128 Rd steps in log space, 2048 samples per table, 10 mipmap levels, generated at plugin init. Use the Csound-derived formant data (bass voice) for the 5 cardinal vowels as the baseline dataset.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- Glottal source: **Mipmapped wavetable** (not direct LF + PolyBLEP). Pre-compute LF waveforms across Rd range, mipmap levels for anti-aliasing, bilinear interpolation at runtime.
- Edge case handling: Focus on musical sweet spot first.
- CPU performance target: Relaxed. No strict <5% target. Optimize where easy.
- Sonic reference: Highest quality and flexibility per research. No single reference plugin.
- Consonant engine: Must-have for v1 (Phase 2.2, not this phase).
- Phase 2.1 scope: LF glottal source + formant filter bank + vowel morpher + ADSR + aspiration noise (all 5 together).

### Claude's Discretion
- Wavetable size and Rd resolution (starting estimate: 128 Rd x 2048 samples)
- Table generation at init vs constexpr baked data
- Mipmap interpolation strategy (linear vs cubic between table entries)

### Deferred Ideas (OUT OF SCOPE)
- Phase 2.2: Vibrato, pitch glide, MPE expression, consonant engine
- Phase 2.3: Stereo spread, output gain, optimization, parameter smoothing verification
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| FUNC-01 | LF glottal pulse with Rd control 0.3-2.7 | Mipmapped wavetable with 128 Rd steps in log space, Fant 1995 regression for table generation |
| FUNC-02 | 5-formant parallel bandpass filter bank | Custom FormantBiquad struct, ArrayCoefficients::makeBandPass verified |
| FUNC-03 | 2D XY vowel morph pad with 5 cardinal vowels | Acoustic XY positions from ARCHITECTURE.md, Csound formant data tables |
| FUNC-04 | Shepard interpolation with log-frequency blending | IDW weights with vowelFocus power, epsilon guard for zero distance |
| FUNC-06 | ADSR amplitude envelope per voice | juce::ADSR verified: isActive(), noteOn/Off, getNextSample, setSampleRate |
| DSP-01 | Fant 1995 Rd-to-R-parameter regression | Formulas from ARCHITECTURE.md verified against literature, computed offline during table gen |
| DSP-02 | Custom biquad bandpass filters (cache-local) | 32-byte FormantBiquad struct, DF2T topology, inline processSample |
| DSP-03 | Block-rate coefficient updates every 32 samples | Standard approach, matches ARCHITECTURE.md spec |
| DSP-04 | Aspiration noise with breathiness control | juce::Random noise, single-pole IIR at ~4kHz, SmoothedValue mixing |
| DSP-05 | Formant shift and spread | freq *= pow(2, shift/12), spread from center-of-mass |
| DSP-07 | Anti-aliasing for glottal source | Mipmapped wavetable (exceeds "PolyBLEP minimum" requirement) |
| PERF-01 | Real-time safe processing | No allocations in renderNextBlock, wavetable is read-only shared data |
| QUAL-01 | No audio artifacts at normal ranges | SmoothedValue on breathiness, block-rate formant updates prevent zipper noise |
</phase_requirements>

---

## Component 1: LFGlottalSource -- Mipmapped Wavetable

### Wavetable Dimensions (Claude's Discretion -- Recommended)

| Parameter | Value | Rationale |
|-----------|-------|-----------|
| Rd steps (K) | 128 | More than GOLF's 100, provides smooth interpolation. Power-of-2 simplifies indexing. |
| Samples per table (L) | 2048 | Matches O-Prism, faithful down to ~21 Hz at 44.1kHz (2048 samples = full period at 21.5 Hz). More than sufficient for musical range. |
| Mipmap levels | 10 | Matches O-Prism's kNumMipmapLevels. Level 0 = 1024 harmonics, Level 9 = 2 harmonics. Covers C0 (16Hz) to well above C8. |
| Guard samples | 1 per table | For wrap-around interpolation (O-Prism pattern). |
| Rd spacing | **Logarithmic** | GOLF validates log spacing: `log(Rd)` evenly spaced in `[log(0.3), log(2.7)]`. Provides finer resolution at low Rd (pressed voice, where timbre changes faster). |

**Memory footprint:**
- Per Rd step: 2049 samples * 10 levels * 4 bytes = ~80 KB
- Total: 128 Rd steps * 80 KB = **~10 MB**
- Shared read-only across all 16 voices (allocated once in PluginProcessor)

**Confidence: HIGH** -- Validated by GOLF paper (K=100, L=2048) and O-Prism implementation (2048 samples, 10 levels).

### Table Generation Strategy (Claude's Discretion -- Recommended: Init-Time)

**Recommendation: Generate at plugin init, not constexpr.**

Reasons:
1. Constexpr generation for 10 MB of data would massively slow compilation and bloat the binary.
2. Init-time generation takes <100ms on modern hardware (128 * FFT for mipmaps).
3. The LF model requires Newton-Raphson solvers which are not constexpr-friendly.
4. O-Prism generates wavetables at init-time (in WavetableGenerator::generateProceduralTable).

**Generation pipeline (runs once at plugin construction or prepareToPlay):**

```
For each of 128 log-spaced Rd values:
  1. Compute R-params from Rd (Fant 1995 regression)
  2. Compute timing params (Tp, Te, Ta, Tc)
  3. Solve alpha via Newton-Raphson (~30 iterations, once)
  4. Solve epsilon via Newton-Raphson (~20 iterations, once)
  5. Render one normalized period of LF derivative into 2048 samples (level 0)
  6. FFT -> truncate harmonics -> IFFT for each mipmap level (O-Prism pattern)
  7. Set guard samples
```

### Fant 1995 Regression Formulas (from ARCHITECTURE.md, verified against literature)

These are computed **once per Rd value during table generation**, not at runtime:

```cpp
// Rd range: [0.3, 2.7]
float Ra = std::max(0.001f, (-1.0f + 4.8f * Rd) / 100.0f);
float Rk = (22.4f + 11.8f * Rd) / 100.0f;

// OQ (Open Quotient) -- piecewise regression from Rd, clamped [0.3, 0.98]
float OQ;
if (Rd < 0.5f)
    OQ = 0.3f + 0.2f * (Rd - 0.3f) / 0.2f;  // Linear ramp 0.3->0.5
else if (Rd < 1.2f)
    OQ = 0.5f + 0.3f * (Rd - 0.5f) / 0.7f;  // Gentle rise
else
    OQ = 0.8f + 0.18f * (Rd - 1.2f) / 1.5f;  // Approaches 0.98
OQ = juce::jlimit(0.3f, 0.98f, OQ);

float Rg = (1.0f + Rk) / (2.0f * OQ);

// Timing (normalized to period = 1.0)
float Tp = 1.0f / (2.0f * Rg);   // Time of max flow derivative
float Te = Tp * (1.0f + Rk);      // Time of excitation (glottal closure)
float Ta = Ra;                      // Return phase time constant
float Tc = 1.0f;                    // Full period
```

### Newton-Raphson Solvers (Offline Only)

**Alpha solver** (open-phase zero-integral constraint):
- Target: integral of `E0 * exp(alpha*t) * sin(omega_g*t)` from 0 to Te equals zero
- `omega_g = pi / Tp`
- Initial guess: `alpha = 1.0 / Tp`
- Converges in ~10-30 iterations; tolerance 1e-6
- Run once per Rd value during table generation

**Epsilon solver** (return-phase amplitude matching):
- Target: `Ee = -E0 * exp(alpha*Te) * sin(omega_g*Te)` (peak negative excitation)
- Return phase: `(-Ee / (eps*Ta)) * [exp(-eps*(t-Te)) - exp(-eps*(Tc-Te))]` must be continuous
- Solve: `1 - exp(-eps*(Tc-Te)) = eps*Ta`
- Initial guess: `epsilon = 1.0 / Ta`
- Converges in ~10-20 iterations

**Confidence: HIGH** -- Formulas match ARCHITECTURE.md which cites Fant 1995 directly.

### Mipmap Interpolation Strategy (Claude's Discretion -- Recommended: Linear)

**Recommendation: Linear interpolation between table entries, bilinear between Rd steps and mipmap levels.**

Rationale:
- O-Prism uses linear interpolation between samples and it sounds excellent.
- Cubic (Hermite/Catmull-Rom) adds ~4x the lookups for marginal improvement on already-smooth glottal waveforms.
- The LF waveform is inherently smooth (no sharp corners after mipmap filtering), so linear is perceptually transparent.
- Can always upgrade to cubic later as an optimization in Phase 2.3 if needed.

**Runtime lookup (per sample):**
```
1. Compute mipmap level from frequency: levelFloat = log2(freq / baseFreq)
   where baseFreq = sampleRate / tableSize
2. Clamp level to [0, numLevels-1]
3. Compute Rd index: rdIndexFloat = (log(currentRd) - log(0.3)) / (log(2.7) - log(0.3)) * (numRdSteps - 1)
4. Bilinear interpolation: 4 lookups (2 Rd steps x 2 mipmap levels), each linearly interpolated across table samples
```

### Data Structure

Adapt O-Prism's WavetableData pattern:

```cpp
struct GlottalWavetable
{
    static constexpr int kTableSize = 2048;
    static constexpr int kGuardSamples = 1;
    static constexpr int kFrameSize = kTableSize + kGuardSamples; // 2049
    static constexpr int kNumRdSteps = 128;
    static constexpr int kNumMipmapLevels = 10;

    std::vector<float> data; // Flat: [level][rdStep][sample+guard]

    void allocate()
    {
        data.resize(kNumMipmapLevels * kNumRdSteps * kFrameSize, 0.0f);
    }

    float getSample(int level, int rdStep, int sampleIndex) const
    {
        return data[(level * kNumRdSteps + rdStep) * kFrameSize + sampleIndex];
    }
    // ... setSample, getFrameData, setGuardSamples (same pattern as WavetableData)
};
```

### LFGlottalSource Runtime Class

```cpp
class LFGlottalSource
{
public:
    void prepare(double sampleRate);
    void setFrequency(float f0);
    void setRd(float rd);
    float getNextSample();  // Inline, called per-sample

private:
    const GlottalWavetable* wavetable = nullptr; // Shared, set by processor
    double sampleRate = 44100.0;
    float frequency = 220.0f;
    double phase = 0.0;       // [0, 1) accumulator
    double phaseIncrement = 0.0;
    float currentRd = 1.0f;

    // Pre-computed for current Rd
    float rdIndexFloat = 0.0f; // Cached Rd lookup position
};
```

---

## Component 2: AspirationNoise

### Single-Pole IIR Lowpass at ~4kHz

The coefficient for a single-pole lowpass filter:

```cpp
// Single-pole IIR: y[n] = (1-a)*x[n] + a*y[n-1]
// where a = exp(-2*pi*cutoff/sampleRate)
float cutoff = 4000.0f;
float a = std::exp(-juce::MathConstants<float>::twoPi * cutoff / sampleRate);
// Process: output = (1.0f - a) * input + a * prevOutput;
```

At 44100 Hz: `a = exp(-2*pi*4000/44100) = exp(-0.5697) = 0.5654`
At 48000 Hz: `a = exp(-2*pi*4000/48000) = exp(-0.5236) = 0.5926`
At 96000 Hz: `a = exp(-2*pi*4000/96000) = exp(-0.2618) = 0.7697`

**Confidence: HIGH** -- Standard DSP formula, verified by calculation.

### Noise Generation

```cpp
// Per-voice juce::Random instance (seeded differently per voice for decorrelation)
float noise = random.nextFloat() * 2.0f - 1.0f; // [-1, 1]
float filteredNoise = (1.0f - lpCoeff) * noise + lpCoeff * prevNoise;
prevNoise = filteredNoise;
```

### Breathiness Mixing with SmoothedValue

```cpp
// In voice prepare():
breathSmoothed.reset(sampleRate, 0.020); // 20ms ramp

// In renderNextBlock, per block:
breathSmoothed.setTargetValue(pBreathiness->load());

// Per sample:
float breath = breathSmoothed.getNextValue();
float source = (1.0f - breath) * glottalSample + breath * filteredNoise;
```

**Confidence: HIGH** -- SmoothedValue API verified against JUCE 8.0.4 source.

---

## Component 3: FormantFilterBank -- 5 Parallel BPFs

### ArrayCoefficients::makeBandPass API (Verified)

**Return type:** `std::array<float, 6>` (confirmed in juce_IIRFilter.h line 72)

**Layout of the 6 coefficients:** `{ b0, b1, b2, a0(=1), a1, a2 }`

Note: The JUCE implementation uses the bilinear transform and returns normalized coefficients where `a0 = 1`. The array order is `[b0, b1, b2, 1.0, a1, a2]`.

**Signature:**
```cpp
static std::array<float, 6> makeBandPass(double sampleRate, float frequency, float Q);
```

Where `Q = centerFreq / bandwidth`. For F1 at 600 Hz with bandwidth 60 Hz: Q = 10.0.

**jassert guard:** The function asserts `frequency > 0 && frequency <= sampleRate * 0.5` -- so we MUST clamp before calling.

### FormantBiquad Struct (32 bytes, cache-friendly)

```cpp
struct FormantBiquad
{
    float b0 = 0.0f, b1 = 0.0f, b2 = 0.0f;
    float a1 = 0.0f, a2 = 0.0f;
    float z1 = 0.0f, z2 = 0.0f;  // DF2T state
    float gain = 1.0f;

    // Inline for hot path -- Direct Form II Transposed
    inline float processSample(float input) noexcept
    {
        float output = b0 * input + z1;
        z1 = b1 * input - a1 * output + z2;
        z2 = b2 * input - a2 * output;
        return output * gain;
    }

    void setCoefficients(const std::array<float, 6>& coeffs)
    {
        b0 = coeffs[0]; b1 = coeffs[1]; b2 = coeffs[2];
        // coeffs[3] is a0 (always 1.0 from JUCE)
        a1 = coeffs[4]; a2 = coeffs[5];
    }

    void reset()
    {
        z1 = z2 = 0.0f;
    }
};
```

**Size:** 8 floats * 4 bytes = 32 bytes. Five of these per voice = 160 bytes. Fits in a single cache line pair.

### Parallel Processing Pattern

```cpp
// Per voice: 5 FormantBiquad filters
FormantBiquad formants[5];
float formantGains[5]; // Set by VowelMorpher

// Per sample:
float formantOut = 0.0f;
for (int i = 0; i < 5; ++i)
    formantOut += formants[i].processSample(source);
```

### Block-Rate Coefficient Updates

Every 32 samples, recompute coefficients from current vowel morph state:

```cpp
static constexpr int kCoeffUpdateInterval = 32;

// In renderNextBlock:
for (int sample = 0; sample < numSamples; ++sample)
{
    if ((sampleCounter % kCoeffUpdateInterval) == 0)
    {
        updateFormantCoefficients(); // Reads VowelMorpher output, applies shift/spread
    }
    // ... per-sample processing
    ++sampleCounter;
}
```

### Frequency Clamping

```cpp
float clampedFreq = juce::jlimit(20.0f, static_cast<float>(sampleRate * 0.5) - 100.0f, freq);
float Q = clampedFreq / bandwidth;
Q = std::max(Q, 0.5f); // Prevent extremely low Q
auto coeffs = juce::dsp::IIR::ArrayCoefficients<float>::makeBandPass(sampleRate, clampedFreq, Q);
```

### Formant Shift and Spread

```cpp
// Shift: semitone-based pitch shift of all formant frequencies
float shiftFactor = std::pow(2.0f, formantShift / 12.0f);

// Spread: scale distance from center-of-mass
float centerOfMass = 0.0f;
for (int i = 0; i < 5; ++i) centerOfMass += freqs[i];
centerOfMass /= 5.0f;

for (int i = 0; i < 5; ++i)
{
    float shifted = freqs[i] * shiftFactor;
    float distance = shifted - centerOfMass * shiftFactor;
    freqs[i] = centerOfMass * shiftFactor + distance * formantSpread;
}
```

**Confidence: HIGH** -- ArrayCoefficients API verified in local JUCE source. DF2T biquad is textbook.

---

## Component 4: VowelMorpher -- Shepard Interpolation

### Formant Data Tables

The most complete dataset found is from the **Csound formant tables** (originally from Singing Voice Synthesis research). This includes F1-F5 frequencies, bandwidths, AND gains for both bass and tenor voices.

**Recommended: Use bass voice data as the default (closest to modal male voice):**

| Vowel | F1 (Hz) | F2 (Hz) | F3 (Hz) | F4 (Hz) | F5 (Hz) |
|-------|---------|---------|---------|---------|---------|
| A /a/ | 600 | 1040 | 2250 | 2450 | 2750 |
| E /e/ | 400 | 1620 | 2400 | 2800 | 3100 |
| I /i/ | 250 | 1750 | 2600 | 3050 | 3340 |
| O /o/ | 400 | 750 | 2400 | 2600 | 2900 |
| U /u/ | 350 | 600 | 2400 | 2675 | 2950 |

**Bandwidths (Hz):**

| Vowel | BW1 | BW2 | BW3 | BW4 | BW5 |
|-------|-----|-----|-----|-----|-----|
| A /a/ | 60 | 70 | 110 | 120 | 130 |
| E /e/ | 40 | 80 | 100 | 120 | 120 |
| I /i/ | 60 | 90 | 100 | 120 | 120 |
| O /o/ | 40 | 80 | 100 | 120 | 120 |
| U /u/ | 40 | 80 | 100 | 120 | 120 |

**Gains (dB, relative to F1):**

| Vowel | G1 | G2 | G3 | G4 | G5 |
|-------|-----|-----|-----|-----|-----|
| A /a/ | 0 | -7 | -9 | -9 | -20 |
| E /e/ | 0 | -12 | -9 | -12 | -18 |
| I /i/ | 0 | -30 | -16 | -22 | -28 |
| O /o/ | 0 | -11 | -21 | -20 | -40 |
| U /u/ | 0 | -20 | -32 | -28 | -36 |

**Source:** Csound formant table (bass voice) from University of Chicago CS archives. Cross-referenced against the research doc's values (vocal-formant-synthesis.md section 2.1 and 4.3) -- frequencies are consistent within 10%.

**Confidence: HIGH** -- Multiple sources agree on F1-F3 ranges. F4/F5 vary more across sources but are less perceptually critical.

### Vowel XY Positions

From ARCHITECTURE.md (acoustic-space derived):

| Vowel | X | Y | Acoustic Basis |
|-------|-----|-----|---------------|
| I /i/ | 0.00 | 1.00 | High front (low F1, high F2) |
| E /e/ | 0.31 | 0.43 | Mid front |
| A /a/ | 0.83 | 0.00 | Low open (high F1, mid F2) |
| O /o/ | 1.00 | 0.35 | Mid back (mid F1, low F2) |
| U /u/ | 0.98 | 0.93 | High back (low F1, low F2) |

**Perceptual validation:** These positions make acoustic sense. The classic vowel quadrilateral maps F1 (openness) to Y-inverted and F2 (frontness) to X. Here:
- I is top-left (closed, front) -- correct
- A is bottom-right (open, central) -- correct
- U is top-right (closed, back) -- correct
- O and U being close on the right is correct (both back vowels)
- E at mid-left is correct (mid-front)

**Confidence: HIGH** -- Positions align with standard vowel quadrilateral.

### Gender Handling

**Recommendation: Use formantShift parameter for gender, not separate voice register tables.**

Rationale:
- formantShift of +3 to +4 semitones approximates female formant scaling (~1.2x)
- This is exactly what formantShift is designed for
- Avoids table complexity and memory duplication
- The Csound data already provides tenor data as a second option if we want to add it later

### Shepard Interpolation Implementation

```cpp
struct VowelData
{
    float freq[5];     // F1-F5 Hz
    float bandwidth[5]; // BW1-BW5 Hz
    float gain[5];      // G1-G5 dB
    float x, y;         // XY position
};

static constexpr int kNumVowels = 5;
static const VowelData vowels[kNumVowels] = { /* A, E, I, O, U data */ };

void VowelMorpher::compute(float cursorX, float cursorY, float focus,
                           float outFreq[5], float outBW[5], float outGain[5])
{
    float weights[kNumVowels];
    float weightSum = 0.0f;

    for (int v = 0; v < kNumVowels; ++v)
    {
        float dx = cursorX - vowels[v].x;
        float dy = cursorY - vowels[v].y;
        float dist = std::sqrt(dx * dx + dy * dy);

        if (dist < 1e-6f)
        {
            // Snap to nearest vowel
            std::copy(vowels[v].freq, vowels[v].freq + 5, outFreq);
            std::copy(vowels[v].bandwidth, vowels[v].bandwidth + 5, outBW);
            std::copy(vowels[v].gain, vowels[v].gain + 5, outGain);
            return;
        }

        weights[v] = 1.0f / std::pow(dist, focus);
        weightSum += weights[v];
    }

    // Normalize weights
    float invSum = 1.0f / weightSum;
    for (int v = 0; v < kNumVowels; ++v)
        weights[v] *= invSum;

    // Interpolate frequencies in log domain
    for (int f = 0; f < 5; ++f)
    {
        float logFreq = 0.0f;
        float bw = 0.0f;
        float g = 0.0f;

        for (int v = 0; v < kNumVowels; ++v)
        {
            logFreq += weights[v] * std::log(vowels[v].freq[f]);
            bw += weights[v] * vowels[v].bandwidth[f];  // Linear interp
            g += weights[v] * vowels[v].gain[f];         // Linear interp (dB)
        }

        outFreq[f] = std::exp(logFreq);
        outBW[f] = bw;
        outGain[f] = g;
    }
}
```

This runs at block-rate (every 32 samples), which at 48kHz = ~1500 calls/sec per voice. The computation is trivial: 5 distances, 5 weights, 15 weighted sums.

**Confidence: HIGH** -- Standard IDW algorithm, well-documented in ARCHITECTURE.md.

---

## Component 5: VoiceEnvelope -- juce::ADSR

### API Verified (JUCE 8.0.4 source)

| Method | Signature | Notes |
|--------|-----------|-------|
| `setSampleRate` | `void setSampleRate(double newSampleRate) noexcept` | Must be called before setParameters |
| `setParameters` | `void setParameters(const Parameters& p)` | Parameters{attack, decay, sustain, release} in seconds |
| `noteOn` | `void noteOn() noexcept` | Starts attack phase |
| `noteOff` | `void noteOff() noexcept` | Starts release phase (recalculates release rate from current envelope value) |
| `isActive` | `bool isActive() const noexcept` | Returns true if not idle (attack/decay/sustain/release all return true) |
| `getNextSample` | `float getNextSample() noexcept` | Returns envelope value [0, 1], advances state |
| `reset` | `void reset() noexcept` | Sets state to idle, envelope value to 0 |

**Important behavior:** `noteOff()` recalculates `releaseRate` from the current `envelopeVal` at the moment of note-off. This means if you call noteOff during the attack phase, the release will be from that partial attack level, not from sustain. This is correct and desirable behavior.

**Important:** The doc comment says "Do not change the parameters during playback." However, examining the code, `setParameters` calls `recalculateRates()` which handles in-progress state transitions correctly. For real-time parameter updates (e.g., user turning attack knob while note is playing), calling `setParameters` each block is safe -- it just recalculates rates without resetting state. This is the standard pattern used in other plugins in this project (O-Prism).

### Integration Pattern

```cpp
// In noteStarted():
adsr.setSampleRate(getSampleRate());
adsr.setParameters({
    pAttack->load(), pDecay->load(), pSustain->load(), pRelease->load()
});
adsr.noteOn();

// In noteStopped(allowTailOff):
if (allowTailOff)
    adsr.noteOff();
else
{
    adsr.reset();
    clearCurrentNote();
}

// In renderNextBlock, per sample:
float envVal = adsr.getNextSample();
outputSample *= envVal;

// After processing all samples in block:
if (!adsr.isActive())
    clearCurrentNote();

// Update params per block (safe):
adsr.setParameters({
    pAttack->load(), pDecay->load(), pSustain->load(), pRelease->load()
});
```

**Confidence: HIGH** -- Verified directly against JUCE 8.0.4 juce_ADSR.h source code.

---

## Architecture Patterns

### Recommended File Structure

```
Source/
  PluginProcessor.h/cpp     (existing -- add wavetable ownership)
  PluginEditor.h/cpp        (existing -- unchanged this phase)
  FormantVoice.h/cpp         (existing -- flesh out with DSP)
  dsp/
    GlottalWavetable.h       (data structure, shared across voices)
    GlottalTableGenerator.h/cpp  (offline LF model + mipmap generation)
    LFGlottalSource.h        (per-voice oscillator, inline processSample)
    AspirationNoise.h        (per-voice noise + lowpass + mix)
    FormantBiquad.h           (32-byte biquad struct)
    FormantFilterBank.h       (5 parallel biquads per voice)
    VowelMorpher.h            (Shepard interpolation + formant data tables)
    VowelData.h               (constexpr formant frequency/BW/gain tables)
```

### Ownership and Sharing Pattern

```
PluginProcessor
  owns: GlottalWavetable (generated once, shared read-only)
  owns: MPESynthesiser
    owns: 16 FormantVoice instances
      each has: LFGlottalSource (points to shared wavetable)
                AspirationNoise
                FormantFilterBank (5 FormantBiquad)
                VowelMorpher (reads shared VowelData)
                juce::ADSR
```

**PluginProcessor.prepareToPlay:**
1. Generate GlottalWavetable (if not already generated or sample rate changed)
2. Pass wavetable pointer to all voices
3. Set sample rate on synthesiser

### Per-Voice Render Loop (renderNextBlock)

```cpp
void FormantVoice::renderNextBlock(AudioBuffer<float>& outputBuffer,
                                    int startSample, int numSamples)
{
    if (!isActive) return;

    // Update ADSR params (block-rate)
    adsr.setParameters({pAttack->load(), pDecay->load(),
                        pSustain->load(), pRelease->load()});

    auto* outL = outputBuffer.getWritePointer(0, startSample);
    auto* outR = outputBuffer.getWritePointer(1, startSample);

    for (int i = 0; i < numSamples; ++i)
    {
        // Block-rate coefficient update
        if ((sampleCounter % 32) == 0)
        {
            float vowelX = pVowelX->load();
            float vowelY = pVowelY->load();
            float focus = pVowelFocus->load();
            float shift = pFormantShift->load();
            float spread = pFormantSpread->load();

            vowelMorpher.compute(vowelX, vowelY, focus,
                                 formantFreqs, formantBWs, formantGains);
            filterBank.updateCoefficients(formantFreqs, formantBWs,
                                          formantGains, shift, spread,
                                          getSampleRate());
        }

        // Glottal source
        float f0 = currentlyPlayingNote.getFrequencyInHertz();
        glottalSource.setFrequency(f0);
        glottalSource.setRd(pGlottalRd->load());
        float glottal = glottalSource.getNextSample();

        // Aspiration mix
        float source = aspirationNoise.process(glottal, pBreathiness->load());

        // Formant filtering
        float filtered = filterBank.process(source);

        // ADSR
        float env = adsr.getNextSample();
        float sample = filtered * env;

        // Write mono to both channels (stereo spread is Phase 2.3)
        outL[i] += sample;
        outR[i] += sample;

        ++sampleCounter;
    }

    if (!adsr.isActive())
    {
        isActive = false;
        clearCurrentNote();
    }
}
```

---

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Bandpass coefficients | Custom bilinear transform | `ArrayCoefficients<float>::makeBandPass(sr, freq, Q)` | JUCE handles edge cases, frequency warping |
| ADSR envelope | Custom ramp/state machine | `juce::ADSR` | Battle-tested, handles all edge cases (noteOff during attack, etc.) |
| Parameter smoothing | Manual one-pole filter | `juce::SmoothedValue<float>` | Handles reset, sample-accurate, thread-safe |
| Random noise | Manual LCG/LFSR | `juce::Random` | Quality uniform distribution, per-instance seeding |
| FFT for mipmaps | Kiss FFT or custom | `juce::dsp::FFT` | Already linked via juce_dsp, SIMD optimized |
| Wavetable mipmap pattern | Custom from scratch | **Adapt O-Prism WavetableGenerator** | Proven in production, handles guard samples, spectral truncation |

---

## Common Pitfalls

### Pitfall 1: Formant Filter Instability at Nyquist
**What goes wrong:** BPF center frequency approaches sampleRate/2, coefficients become numerically unstable, filter explodes.
**Why it happens:** formantShift can push F5 (up to ~3340 Hz base) to very high frequencies. At +24 semitones shift, F5 = 3340 * 4 = 13360 Hz. At 44100 Hz sample rate, Nyquist is 22050 Hz -- still safe. But F5 at extreme shift with spread could approach limit.
**How to avoid:** Clamp all formant frequencies to `[20, sampleRate/2 - 100]` BEFORE calling makeBandPass. The jassert in makeBandPass will catch violations in debug builds.
**Warning signs:** NaN or inf in audio output, sudden volume spikes.

### Pitfall 2: Division by Zero in Shepard Weights
**What goes wrong:** Cursor position exactly on a vowel anchor point, distance = 0, weight = 1/0^p = inf.
**Why it happens:** Float equality with stored anchor positions.
**How to avoid:** Epsilon guard: if `dist < 1e-6`, snap directly to that vowel (copy its formant data, skip interpolation).
**Warning signs:** NaN frequencies, silent or exploding output.

### Pitfall 3: Wavetable Rd Interpolation Discontinuity
**What goes wrong:** Audible clicks or jumps when Rd changes.
**Why it happens:** Rd index jump between wavetable frames without smoothing.
**How to avoid:** SmoothedValue on the Rd parameter read (~20ms ramp), or rely on the fact that APVTS parameters are already smoothed by the host/automation system. The block-rate update interval (32 samples = 0.7ms at 48kHz) is fast enough that small Rd changes per block are inaudible.
**Warning signs:** Clicking when sweeping the glottalRd knob.

### Pitfall 4: Voice Lifecycle Race Condition
**What goes wrong:** Voice continues processing after clearCurrentNote(), or parameters are read after voice is stolen.
**Why it happens:** clearCurrentNote() in MPESynthesiserVoice marks the voice as free for reallocation.
**How to avoid:** Only call clearCurrentNote() AFTER the ADSR has fully released AND after the current renderNextBlock call is complete. Check `adsr.isActive()` at the end of the render loop, not the beginning.
**Warning signs:** Clicks on note-off, audio from "dead" voices.

### Pitfall 5: Wavetable Generation Thread Safety
**What goes wrong:** Wavetable pointer reassigned while voices are reading from it.
**Why it happens:** If prepareToPlay regenerates tables (e.g., sample rate change), voices might be mid-render.
**How to avoid:** Generate wavetable in constructor (before any audio processing). If regeneration is needed at sample rate change, use double-buffering or generate before setting voice pointers. In practice, prepareToPlay is called before audio starts, so a simple sequential approach works.
**Warning signs:** Crashes, garbled audio on sample rate change.

### Pitfall 6: Denormals in Filter State
**What goes wrong:** CPU usage spikes as filter states decay to denormal values after note release.
**Why it happens:** Biquad filter states (z1, z2) decay exponentially and can reach denormal range.
**How to avoid:** `juce::ScopedNoDenormals noDenormals;` is already in processBlock (verified in existing PluginProcessor.cpp). Also reset filter states when voice becomes inactive.
**Warning signs:** CPU usage remains high even when no notes are playing.

---

## Code Examples

### Wavetable Mipmap Generation (Adapted from O-Prism)

```cpp
// Source: O-Prism WavetableGenerator.cpp (local project)
void GlottalTableGenerator::generateMipmaps(GlottalWavetable& table)
{
    static constexpr int fftOrder = 11; // log2(2048)
    static constexpr int fftSize = 1 << fftOrder;
    juce::dsp::FFT fft(fftOrder);
    std::vector<float> fftBuffer(fftSize * 2, 0.0f);
    std::vector<float> workBuffer(fftSize * 2, 0.0f);

    for (int rdStep = 0; rdStep < GlottalWavetable::kNumRdSteps; ++rdStep)
    {
        // Copy level 0 into FFT buffer
        const float* src = table.getFrameData(0, rdStep);
        std::copy(src, src + fftSize, fftBuffer.begin());
        std::fill(fftBuffer.begin() + fftSize, fftBuffer.end(), 0.0f);
        fft.performRealOnlyForwardTransform(fftBuffer.data(), false);

        for (int level = 0; level < GlottalWavetable::kNumMipmapLevels; ++level)
        {
            int maxHarmonic = (fftSize / 2) >> level;
            std::copy(fftBuffer.begin(), fftBuffer.end(), workBuffer.begin());

            // Zero DC
            workBuffer[0] = workBuffer[1] = 0.0f;

            // Zero above maxHarmonic (both positive and negative frequencies)
            for (int bin = maxHarmonic + 1; bin <= fftSize / 2; ++bin)
            {
                workBuffer[bin * 2] = workBuffer[bin * 2 + 1] = 0.0f;
                int negBin = fftSize - bin;
                if (negBin > 0 && negBin < fftSize)
                    workBuffer[negBin * 2] = workBuffer[negBin * 2 + 1] = 0.0f;
            }

            fft.performRealOnlyInverseTransform(workBuffer.data());

            float* dest = table.getFrameData(level, rdStep);
            std::copy(workBuffer.begin(), workBuffer.begin() + fftSize, dest);
        }
    }
    table.setGuardSamples();
}
```

### Bilinear Wavetable Lookup (Rd + Mipmap Level)

```cpp
// Source: Adapted from O-Prism WavetableOscillator::readSample()
float LFGlottalSource::getNextSample() noexcept
{
    // Phase increment
    phase += frequency / sampleRate;
    if (phase >= 1.0) phase -= 1.0;

    // Mipmap level from frequency
    float baseFreq = static_cast<float>(sampleRate) / GlottalWavetable::kTableSize;
    float levelFloat = std::log2(std::max(frequency, baseFreq) / baseFreq);
    levelFloat = juce::jlimit(0.0f, float(GlottalWavetable::kNumMipmapLevels - 1), levelFloat);
    int level0 = static_cast<int>(levelFloat);
    int level1 = std::min(level0 + 1, GlottalWavetable::kNumMipmapLevels - 1);
    float levelFrac = levelFloat - level0;

    // Rd index (log-spaced)
    float rdNorm = (std::log(currentRd) - std::log(0.3f)) / (std::log(2.7f) - std::log(0.3f));
    rdNorm = juce::jlimit(0.0f, 1.0f, rdNorm);
    float rdIndexF = rdNorm * (GlottalWavetable::kNumRdSteps - 1);
    int rd0 = static_cast<int>(rdIndexF);
    int rd1 = std::min(rd0 + 1, GlottalWavetable::kNumRdSteps - 1);
    float rdFrac = rdIndexF - rd0;

    // Sample position
    float samplePos = static_cast<float>(phase) * GlottalWavetable::kTableSize;
    int idx0 = static_cast<int>(samplePos);
    float frac = samplePos - idx0;

    auto lerp = [](float a, float b, float t) { return a + t * (b - a); };

    // 4 table lookups (2 Rd x 2 mipmap levels), each with linear sample interp
    float s00 = lerp(wavetable->getSample(level0, rd0, idx0),
                     wavetable->getSample(level0, rd0, idx0 + 1), frac);
    float s01 = lerp(wavetable->getSample(level0, rd1, idx0),
                     wavetable->getSample(level0, rd1, idx0 + 1), frac);
    float v0 = lerp(s00, s01, rdFrac);

    float s10 = lerp(wavetable->getSample(level1, rd0, idx0),
                     wavetable->getSample(level1, rd0, idx0 + 1), frac);
    float s11 = lerp(wavetable->getSample(level1, rd1, idx0),
                     wavetable->getSample(level1, rd1, idx0 + 1), frac);
    float v1 = lerp(s10, s11, rdFrac);

    return lerp(v0, v1, levelFrac);
}
```

---

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Direct LF + PolyBLEP per sample | Pre-computed mipmapped wavetable | GOLF 2023/2024 validates | Zero per-sample math for waveform gen |
| Cascade formant topology (KLATT) | Parallel BPF bank | Standard for musical synths | Independent gain control per formant |
| Hardcoded vowel presets | Continuous 2D interpolation | Common in modern vocal synths | Smooth morphing between any position |

---

## Open Questions

1. **Normalization of LF wavetable entries**
   - What we know: Each Rd value produces a different amplitude LF waveform. Rd=0.3 (pressed) has sharp peaks; Rd=2.7 (breathy) has gentle slopes.
   - What's unclear: Should each Rd step be normalized to peak=1.0 independently, or should relative amplitudes across Rd values be preserved?
   - Recommendation: Normalize each Rd step independently to peak=1.0. The spectral tilt difference between Rd values will still be audible through the harmonic content. Independent normalization prevents volume jumps when sweeping Rd. The breathiness parameter handles perceived volume reduction for breathy voices.

2. **Formant gain units**
   - What we know: The Csound table uses dB relative to F1 (G1 = 0 dB always).
   - What's unclear: Should the FormantBiquad `gain` field be linear or dB?
   - Recommendation: Store as linear gain (`juce::Decibels::decibelsToGain(dB_value)`). Apply in processSample as multiplication. This avoids dB-to-linear conversion per sample.

---

## Project Constraints (from CLAUDE.md)

- Build: CMake + Ninja, clear AU cache before install
- JUCE 8.0.4 at /Users/taylorbrook/JUCE
- DSP files go in Source/dsp/ (following O-Prism pattern)
- New source files must be added to CMakeLists.txt target_sources
- juce_dsp already linked (needed for ArrayCoefficients and FFT)
- No WebView or web browser needed for this plugin
- Zero latency target (no oversampling)

---

## Sources

### Primary (HIGH confidence)
- `/Users/taylorbrook/JUCE/modules/juce_dsp/processors/juce_IIRFilter.h` -- ArrayCoefficients::makeBandPass returns std::array<float, 6>
- `/Users/taylorbrook/JUCE/modules/juce_audio_basics/utilities/juce_ADSR.h` -- Full ADSR API including isActive()
- `/Users/taylorbrook/JUCE/modules/juce_audio_basics/utilities/juce_SmoothedValue.h` -- SmoothedValue API
- `plugins/O-Prism/Source/dsp/WavetableData.h` -- Project's existing mipmap wavetable data structure
- `plugins/O-Prism/Source/dsp/WavetableGenerator.cpp` -- Project's existing FFT-based mipmap generation
- `plugins/O-Prism/Source/dsp/WavetableOscillator.cpp` -- Project's existing trilinear wavetable lookup
- `plugins/O-Formant/.planning/research/ARCHITECTURE.md` -- Authoritative DSP spec with Fant 1995 formulas

### Secondary (MEDIUM confidence)
- [GOLF: Glottal Flow Wavetables (ISMIR 2024)](https://transactions.ismir.net/articles/10.5334/tismir.210) -- Validates K=100, L=2048, log-spaced Rd
- [Singing Voice Synthesis Using Differentiable LPC and Glottal-Flow-Inspired Wavetables (arXiv)](https://arxiv.org/abs/2306.17252) -- GOLF implementation details
- [Csound Formant Table (UChicago)](https://www.classes.cs.uchicago.edu/archive/1999/spring/CS295/Computing_Resources/Csound/CsManual3.48b1.HTML/Appendices/table3.html) -- F1-F5 frequencies, bandwidths, gains for bass/tenor voices
- [Reshaping the Transformed LF Model (Gobl, Interspeech 2017)](https://www.isca-archive.org/interspeech_2017/gobl17_interspeech.html) -- Rd parameter direct control of LF pulse

### Tertiary (LOW confidence)
- [EarLevel Engineering: Replicating Wavetables](https://www.earlevel.com/main/2013/03/03/replicating-wavetables/) -- One-table-per-octave mipmap strategy
- [Static Measurements of Vowel Formant Frequencies (PMC)](https://pmc.ncbi.nlm.nih.gov/articles/PMC6002811/) -- Review of formant measurement variability

---

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH -- All JUCE APIs verified against local source code
- Architecture: HIGH -- Adapting proven O-Prism wavetable pattern + ARCHITECTURE.md spec
- Formant data: HIGH -- Csound tables cross-referenced with research doc and literature
- Pitfalls: HIGH -- Based on verified API constraints and standard DSP knowledge
- LF wavetable specifics: MEDIUM -- GOLF validates approach but exact normalization strategy is a judgment call

**Research date:** 2026-04-04
**Valid until:** 2026-05-04 (stable -- JUCE 8.0.4 APIs unlikely to change)
