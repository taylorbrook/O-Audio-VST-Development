---
title: "Advanced Glottal Pulse Modeling for O-Formant"
created: 2026-04-04
juce_version: "8.0.4"
summary: "Deep technical reference for LF model implementation, anti-aliased glottal synthesis, musical parameter design, creative sound design, and formant-pitch interaction for a playable vocal synthesizer."
domain: dsp
type: research
keywords:
  - glottal-pulse
  - LF-model
  - Rd-parameter
  - PolyBLEP
  - vocal-synthesis
  - formant-synth
  - anti-aliasing
  - wavetable
stages: [0, 1, 2]
agents: [dsp, research]
---

# Advanced Glottal Pulse Modeling for O-Formant

**Deep Technical Reference**

**Created:** April 2026
**Version:** 1.0
**Research Depth:** Level 4 (Deep Investigation)

---

## 1. The Liljencrants-Fant (LF) Model -- Complete Reference

### 1.1 Time-Domain Equations

The LF model describes the **derivative of glottal flow** (not the flow itself) as a piecewise function over one glottal period T0:

```
        | E0 * exp(alpha * t) * sin(omega_g * t),   0 <= t <= Te    (open phase)
U'(t) = |
        | (-Ee / (epsilon * Ta)) * [exp(-epsilon*(t-Te)) - exp(-epsilon*(Tc-Te))],  Te < t <= Tc  (return phase)
```

Where the closed phase (Tc < t <= T0) has U'(t) = 0.

**Key timing parameters:**
- **T0** = 1/F0 : fundamental period
- **Tp** : instant of maximum glottal flow (peak of the integrated pulse)
- **Te** : instant of maximum excitation (glottal closure, largest negative derivative)
- **Ta** : effective duration of return phase (exponential time constant)
- **Tc** : instant of complete closure (often Tc = T0 for fully closed glottis)

**Derived quantities:**
- **omega_g** = pi / Tp : angular frequency of the sine component
- **E0** = -Ee / (exp(alpha * Te) * sin(omega_g * Te)) : amplitude scaling
- **Ee** = |U'(Te)| : excitation amplitude at closure

### 1.2 The R-Parameters (Fant et al., 1985)

The R-parameters provide a normalized, physically meaningful parameterization:

```
Rg = T0 / (2 * Tp)           -- inverse normalized rise time (glottal frequency ratio)
Rk = (Te - Tp) / Tp          -- asymmetry quotient (closing/opening ratio)
Ra = Ta / T0                  -- normalized return phase duration
```

**Relationships to timing:**
```
Tp = T0 / (2 * Rg)
Te = Tp * (1 + Rk) = T0 * (1 + Rk) / (2 * Rg)
Ta = Ra * T0
```

**Open Quotient:**
```
OQ = Te / T0 = (1 + Rk) / (2 * Rg)
```

### 1.3 The Rd Parameter (Fant, 1995) -- One-Knob Voice Quality

Rd is a single "global waveshape parameter" derived from statistical regression over natural speech data. It captures the dominant covariation between glottal parameters.

**Definition:**
```
Rd = (1 / 0.11) * (F0 * Up / Ee)
```
Where Up is the peak glottal flow amplitude.

**Valid range:** 0.3 (extremely pressed/tense) to 2.7 (extremely breathy/lax)

**Voice quality mapping:**
| Rd Range    | Voice Quality     | Character                          |
|-------------|-------------------|------------------------------------|
| 0.3 - 0.7  | Pressed/Tense     | Bright, strong harmonics, edgy     |
| 0.7 - 1.2  | Modal (normal)    | Natural speech, balanced spectrum   |
| 1.2 - 2.0  | Relaxed/Soft      | Gentle, reduced upper harmonics     |
| 2.0 - 2.7  | Breathy/Lax       | Airy, steep spectral tilt, noisy    |

### 1.4 Rd to R-Parameter Regression (Fant, 1995)

These are the standard regression equations used in the transformed LF model. They approximate the natural covariation found in speech data:

```
Rap = (-1 + 4.8 * Rd) / 100                     -- clamp to [0.001, ...]
Rkp = (22.4 + 11.8 * Rd) / 100                  -- typical range ~0.3-0.6
Rgp = (Rg0 / 4) * (Rd >= 0.21) + ...            -- see piecewise below
```

**More precisely, the Fant/Gobl formulations:**

```cpp
// Rd -> R-parameter mapping (Fant 1995, refined by Gobl 2003/2017)
float computeRap(float Rd) {
    return juce::jmax(0.001f, (-1.0f + 4.8f * Rd) / 100.0f);
}

float computeRkp(float Rd) {
    return (22.4f + 11.8f * Rd) / 100.0f;
}

float computeRgp(float Rd) {
    // Piecewise: different behavior in low vs high Rd
    // Based on OQ regression
    float OQ;
    if (Rd < 0.5f)
        OQ = 1.0f - (1.0f - 0.5f * Rd / 0.5f) * 0.5f;  // tight mapping
    else
        OQ = 0.5f + 0.3f * (Rd - 0.5f) / 2.2f;           // gentler slope

    // Cap OQ
    OQ = juce::jlimit(0.3f, 0.95f, OQ);

    float Rkp = computeRkp(Rd);
    // Rg derived from OQ = (1 + Rk) / (2 * Rg)
    return (1.0f + Rkp) / (2.0f * OQ);
}
```

**Simplified direct regression (widely used in implementations):**
```cpp
// Direct Fant 1995 regression (commonly cited form)
void rdToRParams(float Rd, float& Ra, float& Rk, float& Rg, float& OQ)
{
    Ra = juce::jmax(0.001f, (-1.0f + 4.8f * Rd) / 100.0f);
    Rk = (22.4f + 11.8f * Rd) / 100.0f;

    // OQ from Rd (Fant regression)
    if (Rd < 0.21f)
        OQ = 1.0f / (2.3f - 0.3f * Rd);
    else
        OQ = (0.5f + 0.17f * Rd);

    OQ = juce::jlimit(0.3f, 0.98f, OQ);

    // Derive Rg from OQ and Rk
    Rg = (1.0f + Rk) / (2.0f * OQ);
}
```

### 1.5 Computing Alpha and Epsilon

**These cannot be solved analytically.** Both require iterative numerical methods.

**Epsilon (from the return phase constraint):**

The return phase must integrate to the correct area. The constraint equation is:

```
epsilon * Ta = 1 - exp(-epsilon * (Tc - Te))
```

When Tc = T0 (fully closed glottis), this becomes:

```
epsilon * Ta = 1 - exp(-epsilon * Tb)     where Tb = T0 - Te
```

**Newton-Raphson for epsilon:**
```cpp
float solveEpsilon(float Ta, float Tb, int maxIter = 20)
{
    // Initial estimate: for small Ta, epsilon ~ 1/Ta
    float eps = 1.0f / (Ta + 1e-10f);

    for (int i = 0; i < maxIter; ++i)
    {
        float expTerm = std::exp(-eps * Tb);
        float f = eps * Ta - 1.0f + expTerm;
        float fPrime = Ta + Tb * expTerm;

        float delta = f / fPrime;
        eps -= delta;

        if (std::abs(delta) < 1e-6f) break;
        eps = juce::jmax(eps, 0.001f);  // prevent negative
    }
    return eps;
}
```

**Alpha (from the zero-integral constraint):**

The entire glottal flow derivative must integrate to zero over one period (conservation of flow). The constraint:

```
Integral[0..Te] E0*exp(alpha*t)*sin(omega_g*t) dt  +  Integral[Te..Tc] returnPhase dt  =  0
```

The open-phase integral (Ao) and return-phase integral (Ar):

```
Ao(alpha) = E0 * exp(alpha*Te) / sqrt(omega_g^2 + alpha^2)
            * sin(omega_g*Te - atan(omega_g/alpha))
            + E0 * omega_g / (omega_g^2 + alpha^2)

Ar(alpha) = -Ee / (epsilon^2 * Ta) * [1 - exp(-epsilon*Tb)*(1 + epsilon*Tb)]
```

Solve: Ao(alpha) + Ar(alpha) = 0

**Newton-Raphson for alpha:**
```cpp
float solveAlpha(float Ee, float Te, float Tp, float Ta, float Tb, float epsilon, int maxIter = 30)
{
    float omega_g = juce::MathConstants<float>::pi / Tp;

    // Return phase integral (constant w.r.t. alpha)
    float expEpsTb = std::exp(-epsilon * Tb);
    float Ar = -Ee / (epsilon * epsilon * Ta) * (1.0f - expEpsTb * (1.0f + epsilon * Tb));

    // Initial guess
    float alpha = 1.0f;

    for (int i = 0; i < maxIter; ++i)
    {
        float sinTe = std::sin(omega_g * Te);
        float cosTe = std::cos(omega_g * Te);
        float expAlphaTe = std::exp(alpha * Te);
        float denom = omega_g * omega_g + alpha * alpha;

        float E0 = -Ee / (expAlphaTe * sinTe);

        // Open-phase integral
        float sqrtDenom = std::sqrt(denom);
        float atanTerm = std::atan2(omega_g, alpha);
        float Ao = E0 * expAlphaTe / sqrtDenom
                   * std::sin(omega_g * Te - atanTerm)
                   + E0 * omega_g / denom;

        float f = Ao + Ar;

        // Numerical derivative (finite difference)
        float h = 0.001f;
        float alpha2 = alpha + h;
        float E0_2 = -Ee / (std::exp(alpha2 * Te) * sinTe);
        float denom2 = omega_g * omega_g + alpha2 * alpha2;
        float Ao2 = E0_2 * std::exp(alpha2 * Te) / std::sqrt(denom2)
                     * std::sin(omega_g * Te - std::atan2(omega_g, alpha2))
                     + E0_2 * omega_g / denom2;
        float fPrime = (Ao2 + Ar - f) / h;

        if (std::abs(fPrime) < 1e-12f) break;

        float delta = f / fPrime;
        alpha -= delta;

        if (std::abs(delta) < 1e-6f) break;
        alpha = juce::jmax(alpha, 0.001f);
    }
    return alpha;
}
```

### 1.6 Spectral Tilt as a Function of Rd

The spectral tilt of the LF model (in dB/octave) is primarily controlled by **Ra** (the return phase parameter) and **alpha**.

**Approximate relationship:**
```
Spectral tilt (dB/oct) ~ -6 - 6 * (1 - Ra * F0 * T0)
                       ~ -6 * (1 + some_function_of_Rd)
```

**Practical approximation for H1-H2 (dB difference between first two harmonics):**
```
H1-H2 ~ -6.0 + 9.0 * Rd       (very rough, from regression on speech data)
```

| Rd   | H1-H2 (dB) | Spectral Character                    |
|------|-------------|---------------------------------------|
| 0.3  | -3.3        | Negative (H2 > H1), strong harmonics  |
| 0.7  | +0.3        | Nearly flat, modal voice              |
| 1.0  | +3.0        | Mild roll-off                         |
| 1.7  | +9.3        | Significant tilt, softer              |
| 2.7  | +18.3       | Very steep tilt, breathy              |

**For implementation, direct spectral tilt in dB/octave:**
```
approx_tilt_dB_oct(Rd) ~ -12.0 - 3.5 * (Rd - 1.0)
```
- Rd=0.3: about -9.5 dB/oct (flat, buzzy)
- Rd=1.0: about -12 dB/oct (modal voice standard)
- Rd=2.7: about -18 dB/oct (breathy, steep)

### 1.7 Simplified Real-Time Approximation

For per-sample computation, solving Newton-Raphson every period is acceptable (it runs once per F0 cycle, not per sample). However, if you want to avoid iteration entirely:

**Pre-computed wavetable approach (recommended for O-Formant):**

Following the GOLF paper (2023), pre-compute a bank of LF derivative pulses:

```cpp
class LFWavetableBank
{
public:
    static constexpr int NUM_RD_STEPS = 128;   // resolution along Rd axis
    static constexpr int TABLE_SIZE = 2048;     // samples per period

    void initialize()
    {
        for (int k = 0; k < NUM_RD_STEPS; ++k)
        {
            // Log-spaced Rd from 0.3 to 2.7
            float t = (float)k / (float)(NUM_RD_STEPS - 1);
            float Rd = 0.3f * std::pow(2.7f / 0.3f, t);

            // Compute LF parameters from Rd
            float Ra, Rk, Rg, OQ;
            rdToRParams(Rd, Ra, Rk, Rg, OQ);

            // Compute timing (normalized to period = 1.0)
            float Tp = 1.0f / (2.0f * Rg);
            float Te = Tp * (1.0f + Rk);
            float Ta = Ra;  // already normalized to T0
            float Tb = 1.0f - Te;

            // Solve implicit parameters
            float epsilon = solveEpsilon(Ta, Tb);
            float alpha = solveAlpha(1.0f, Te, Tp, Ta, Tb, epsilon);

            // Render one period of the derivative
            renderLFPulse(tables[k], TABLE_SIZE, alpha, epsilon, Te, Tp, Ta, Tb);

            // Normalize: unit energy
            normalizeTable(tables[k], TABLE_SIZE);
        }
    }

    // Bilinear interpolation for real-time use
    float readSample(float phase, float rdNormalized) const
    {
        // rdNormalized: 0.0 = Rd 0.3, 1.0 = Rd 2.7
        float kFloat = rdNormalized * (NUM_RD_STEPS - 1);
        int k0 = juce::jlimit(0, NUM_RD_STEPS - 2, (int)kFloat);
        int k1 = k0 + 1;
        float kFrac = kFloat - k0;

        float lFloat = phase * TABLE_SIZE;
        int l0 = ((int)lFloat) % TABLE_SIZE;
        int l1 = (l0 + 1) % TABLE_SIZE;
        float lFrac = lFloat - (int)lFloat;

        // Bilinear interpolation
        float s00 = tables[k0][l0];
        float s01 = tables[k0][l1];
        float s10 = tables[k1][l0];
        float s11 = tables[k1][l1];

        float s0 = s00 + lFrac * (s01 - s00);
        float s1 = s10 + lFrac * (s11 - s10);

        return s0 + kFrac * (s1 - s0);
    }

private:
    float tables[NUM_RD_STEPS][TABLE_SIZE];

    void renderLFPulse(float* table, int size, float alpha, float epsilon,
                       float Te, float Tp, float Ta, float Tb)
    {
        float omega_g = juce::MathConstants<float>::pi / Tp;
        float sinTe = std::sin(omega_g * Te);
        float Ee = 1.0f;  // normalized
        float E0 = -Ee / (std::exp(alpha * Te) * sinTe);

        for (int i = 0; i < size; ++i)
        {
            float t = (float)i / (float)size;  // normalized [0, 1)

            if (t <= Te)
            {
                // Open phase: sine * exponential
                table[i] = E0 * std::exp(alpha * t) * std::sin(omega_g * t);
            }
            else if (t <= Te + Tb && Tb > 0.0f)
            {
                // Return phase: exponential decay
                float tRel = t - Te;
                float expTerm = std::exp(-epsilon * tRel) - std::exp(-epsilon * Tb);
                table[i] = (-Ee / (epsilon * Ta)) * expTerm;
            }
            else
            {
                // Closed phase
                table[i] = 0.0f;
            }
        }
    }

    void normalizeTable(float* table, int size)
    {
        float energy = 0.0f;
        for (int i = 0; i < size; ++i)
            energy += table[i] * table[i];
        float scale = 1.0f / std::sqrt(energy / size + 1e-10f);
        for (int i = 0; i < size; ++i)
            table[i] *= scale;
    }
};
```

**Memory cost:** 128 * 2048 * 4 bytes = ~1 MB. Very manageable.

---

## 2. Anti-Aliasing the Glottal Source

### 2.1 Why Glottal Pulses Alias

The LF derivative waveform has two discontinuities:
1. **At Te (glottal closure):** A sharp negative spike -- the return phase begins abruptly
2. **At the junction between return phase and closed phase:** A slope discontinuity

At high F0 values (soprano range, 500+ Hz), the harmonics extend well above Nyquist and naive sample-by-sample generation produces severe aliasing.

### 2.2 PolyBLEP for Glottal Pulses

PolyBLEP corrects discontinuities by adding a polynomial residual at the transition points. The core function:

```cpp
// dt = phase increment per sample (frequency / sampleRate)
// t = current phase position [0, 1)
float polyBLEP(float t, float dt)
{
    if (t < dt)
    {
        // Just after transition
        t /= dt;
        return t + t - t * t - 1.0f;
    }
    else if (t > 1.0f - dt)
    {
        // Just before transition
        t = (t - 1.0f) / dt;
        return t * t + t + t + 1.0f;
    }
    return 0.0f;
}
```

**Applying to the LF model:**

The main discontinuity is at Te (the glottal closure instant). For the wavetable approach, the BLEP correction goes at the normalized phase position `Te`:

```cpp
float antiAliasedLFSample(float phase, float phaseIncrement, float Rd,
                          const LFWavetableBank& bank)
{
    float rdNorm = (std::log(Rd) - std::log(0.3f)) / (std::log(2.7f) - std::log(0.3f));
    rdNorm = juce::jlimit(0.0f, 1.0f, rdNorm);

    float sample = bank.readSample(phase, rdNorm);

    // Get Te for this Rd
    float Ra, Rk, Rg, OQ;
    rdToRParams(Rd, Ra, Rk, Rg, OQ);
    float Te = OQ;  // normalized to period

    // Apply PolyBLEP at the closure instant (Te)
    float tePhase = phase - Te;
    if (tePhase < 0.0f) tePhase += 1.0f;
    float blepCorrection = polyBLEP(tePhase, phaseIncrement);

    // Scale correction by the discontinuity magnitude
    // (estimated from the pulse shape at Te)
    float discontinuityMag = estimateDiscontinuity(Rd);
    sample -= blepCorrection * discontinuityMag;

    return sample;
}
```

### 2.3 PolyBLAMP for Slope Discontinuities

Where the return phase meets the closed phase, there is a **slope discontinuity** (first derivative jump, not a value jump). PolyBLEP handles value discontinuities; for slope discontinuities, use **PolyBLAMP** (integrated PolyBLEP):

```cpp
float polyBLAMP(float t, float dt)
{
    if (t < dt)
    {
        t /= dt;
        // Integrated PolyBLEP: 4th-point residual
        float t2 = t * t;
        return t * t2 / 3.0f - t2 / 2.0f + t / 3.0f;
    }
    else if (t > 1.0f - dt)
    {
        t = (t - 1.0f) / dt;
        float t2 = t * t;
        return -(t * t2 / 3.0f + t2 / 2.0f + t / 3.0f);
    }
    return 0.0f;
}
```

### 2.4 Oversampling as a Complement

PolyBLEP/BLAMP are efficient but imperfect -- they reduce aliasing but don't eliminate it. For highest quality:

```
Strategy                  | Aliasing Reduction | CPU Cost
--------------------------|--------------------|----------
Naive (no correction)     | 0 dB               | 1x
PolyBLEP only             | ~30 dB             | ~1.05x
PolyBLEP + PolyBLAMP      | ~45 dB             | ~1.1x
2x oversampling + poly    | ~60 dB             | ~2.2x
4x oversampling + poly    | ~80 dB             | ~4.4x
Wavetable (mipmap)        | ~90 dB             | ~1.1x
```

**Recommendation for O-Formant:** Use the pre-computed wavetable bank with mipmap levels (reduced harmonic count for higher pitches). This gives the best quality/cost ratio and naturally band-limits the output.

### 2.5 Wavetable Mipmapping for Glottal Pulses

```cpp
class MipmappedLFBank
{
public:
    // Multiple table sizes for different pitch ranges
    // Level 0: 2048 samples (low pitches, ~C1-C3, full harmonics)
    // Level 1: 1024 samples (mid pitches, ~C3-C5)
    // Level 2: 512 samples (high pitches, ~C5-C7)
    // Level 3: 256 samples (very high, > C7)

    void initialize()
    {
        for (int level = 0; level < NUM_LEVELS; ++level)
        {
            int tableSize = MAX_TABLE_SIZE >> level;
            // Generate with bandwidth limited to tableSize/2 harmonics
            for (int k = 0; k < NUM_RD_STEPS; ++k)
            {
                generateBandLimitedLF(levels[level].tables[k], tableSize, rdForIndex(k));
            }
        }
    }

    float readSample(float phase, float rdNorm, float frequency, float sampleRate) const
    {
        // Select mipmap level based on fundamental frequency
        float periodsInTable = sampleRate / frequency;
        int level = 0;
        while (level < NUM_LEVELS - 1 && periodsInTable < (MAX_TABLE_SIZE >> level))
            ++level;

        // Interpolate between adjacent levels for smooth transitions
        // ... (bilinear on Rd axis, linear on phase, crossfade between levels)
    }

    static constexpr int NUM_LEVELS = 4;
    static constexpr int MAX_TABLE_SIZE = 2048;
    static constexpr int NUM_RD_STEPS = 128;
};
```

---

## 3. BLIT-Based Glottal Source

### 3.1 Concept

A BLIT (Band-Limited Impulse Train) generates a series of sinc-like pulses, each containing only harmonics below Nyquist. By integrating a BLIT you get a band-limited sawtooth. By **shaping the spectral envelope** of the BLIT, you can approximate a glottal pulse.

### 3.2 BLIT to Glottal Pulse Pipeline

```
BLIT -> Spectral Tilt Filter -> Glottal Approximation
```

**Step 1: Generate BLIT**
```cpp
float blit(float phase, float phaseIncrement, float sampleRate, float frequency)
{
    int M = (int)(sampleRate / (2.0f * frequency));  // max harmonic
    int N = 2 * M + 1;
    float denom = std::sin(juce::MathConstants<float>::pi * phase);

    if (std::abs(denom) < 1e-7f)
        return 1.0f;  // at the peak

    return std::sin(N * juce::MathConstants<float>::pi * phase)
           / (N * denom);
}
```

**Step 2: Integrate (leaky) to get sawtooth-like pulse**
```cpp
// Leaky integrator prevents DC drift
float integratedBlit = leakyAlpha * prevOutput + blitSample;
prevOutput = integratedBlit;
// leakyAlpha ~ 0.999 at 44.1kHz
```

**Step 3: Apply spectral tilt to approximate glottal source**
```cpp
// One-pole lowpass for spectral tilt control
// Rd controls the tilt: lower Rd = less tilt, higher Rd = more tilt
class SpectralTiltFilter
{
public:
    void setTilt(float Rd, float sampleRate)
    {
        // Cutoff frequency from Rd: lower Rd -> higher cutoff -> less tilt
        float cutoffHz = 200.0f + (4000.0f - 200.0f) * std::exp(-0.8f * Rd);
        float w = 2.0f * juce::MathConstants<float>::pi * cutoffHz / sampleRate;
        coeff = w / (1.0f + w);
    }

    float process(float input)
    {
        state += coeff * (input - state);
        return state;
    }

private:
    float coeff = 0.5f;
    float state = 0.0f;
};
```

### 3.3 BLIT Limitations for Glottal Synthesis

- BLIT naturally produces **symmetric** pulses; glottal pulses are asymmetric
- The integrated BLIT gives a sawtooth-like shape, not the opening-closing-return shape
- **Best used as:** a computationally cheap fallback or a "synthetic" voice mode
- For accurate LF behavior, the wavetable approach is superior

### 3.4 Hybrid Approach: BLIT + Shaping

A practical middle ground uses BLIT for band-limited harmonic generation, then waveshapes:

```cpp
float glottalFromBLIT(float phase, float phaseInc, float Rd)
{
    float raw = blit(phase, phaseInc, sampleRate, frequency);

    // Integrate
    integrated = 0.999f * integrated + raw;

    // Apply asymmetric waveshaping based on Rd
    // Positive half (opening): gentle curve
    // Negative half (closing): sharper
    float shaped;
    if (integrated > 0.0f)
        shaped = std::pow(integrated, 1.0f + 0.5f * Rd);
    else
        shaped = -std::pow(-integrated, 1.0f / (1.0f + 0.5f * Rd));

    return spectralTiltFilter.process(shaped);
}
```

---

## 4. Musical Glottal Parameters

### 4.1 Parameter Hierarchy (Most to Least Musically Useful)

**Tier 1 -- Expose prominently (primary controls):**

| Parameter       | Range          | Musical Effect                           | UI Suggestion      |
|-----------------|----------------|------------------------------------------|---------------------|
| **Rd / Voice Quality** | 0.3 - 2.7 | Pressed to breathy; entire timbre character | Large knob or XY pad |
| **Breathiness** | 0 - 100%       | Mix of aspiration noise with voiced source | Knob               |
| **Pitch (F0)**  | MIDI note      | Fundamental frequency                     | Keyboard/knob       |

**Tier 2 -- Expose as secondary controls:**

| Parameter         | Range       | Musical Effect                                  | UI Suggestion     |
|-------------------|-------------|--------------------------------------------------|-------------------|
| **Open Quotient**  | 0.3 - 0.95  | Duty cycle; affects brightness and body          | Knob (detented at 0.5) |
| **Spectral Tilt**  | -6 to -24 dB/oct | Independent brightness control            | Knob              |
| **Jitter**         | 0 - 5%     | F0 perturbation; adds life and character         | Knob              |
| **Shimmer**        | 0 - 5%     | Amplitude perturbation; adds roughness           | Knob              |

**Tier 3 -- Advanced/hidden panel:**

| Parameter          | Range       | Musical Effect                                |
|--------------------|-------------|------------------------------------------------|
| **Speed Quotient**  | 0.5 - 4.0  | Asymmetry of open/close; subtle timbral shift |
| **Aspiration Freq** | 500-4000 Hz | Center freq of aspiration noise band          |
| **F0 Vibrato Rate** | 3-8 Hz     | Vibrato speed                                 |
| **F0 Vibrato Depth**| 0-100 cents | Vibrato amount                               |
| **Subharmonics**    | 0-100%     | Period doubling for vocal fry/growl           |

### 4.2 Jitter Implementation

Jitter = random perturbation of the fundamental period, cycle-to-cycle.

```cpp
class JitterGenerator
{
public:
    // jitterAmount: 0.0 = none, 0.05 = 5% (maximum natural range)
    float getNextPeriod(float nominalPeriodSamples, float jitterAmount)
    {
        // Gaussian jitter (more natural than uniform)
        float gauss = gaussianRandom();  // mean=0, std=1
        float perturbation = gauss * jitterAmount * nominalPeriodSamples;
        return nominalPeriodSamples + perturbation;
    }

private:
    float gaussianRandom()
    {
        // Box-Muller transform
        float u1 = juce::Random::getSystemRandom().nextFloat();
        float u2 = juce::Random::getSystemRandom().nextFloat();
        return std::sqrt(-2.0f * std::log(u1 + 1e-10f))
               * std::cos(2.0f * juce::MathConstants<float>::pi * u2);
    }
};
```

**Musical range:** 0-1% sounds natural. 1-3% sounds rough/characterful. 3-5% enters creaky/pathological territory. Beyond 5% becomes obviously synthetic but potentially creative.

### 4.3 Shimmer Implementation

Shimmer = random perturbation of pulse amplitude, cycle-to-cycle.

```cpp
class ShimmerGenerator
{
public:
    // shimmerAmount: 0.0 = none, typical 0.01-0.05
    float getNextGain(float shimmerAmount)
    {
        float gauss = gaussianRandom();
        float perturbation = gauss * shimmerAmount;
        return 1.0f + perturbation;
    }
};
```

### 4.4 Breathiness / Aspiration Noise

Aspiration noise is not white -- it's shaped by the vocal tract:

```cpp
class AspirationNoise
{
public:
    void prepare(double sampleRate)
    {
        // Bandpass filter centered around 2-3 kHz
        // This simulates turbulent airflow through the glottis
        bandpass.setCoefficients(
            juce::IIRCoefficients::makeBandPass(sampleRate, 2500.0f, 1.5f));

        // Additional high-shelf to add breathiness above 4 kHz
        highShelf.setCoefficients(
            juce::IIRCoefficients::makeHighShelf(sampleRate, 4000.0f, 0.7f, 1.5f));
    }

    float process(float noiseInput)
    {
        return highShelf.processSingleSampleRaw(
               bandpass.processSingleSampleRaw(noiseInput));
    }

    // Gate noise with glottal phase for more realism
    // Aspiration is strongest during the open phase
    float processWithGating(float noiseInput, float glottalPhase, float OQ)
    {
        float gate = (glottalPhase < OQ) ? 1.0f : 0.1f;  // some leakage
        return process(noiseInput) * gate;
    }

private:
    juce::IIRFilter bandpass, highShelf;
};
```

### 4.5 Subharmonics / Vocal Fry

For creative vocal sounds, alternating strong/weak pulses:

```cpp
class SubharmonicGenerator
{
public:
    // amount: 0.0 = normal, 1.0 = full period doubling
    float getGainForPulse(float amount)
    {
        bool isEvenPulse = (pulseCount % 2 == 0);
        pulseCount++;

        if (amount <= 0.0f) return 1.0f;

        // Even pulses are stronger, odd pulses are weaker
        if (isEvenPulse)
            return 1.0f;
        else
            return 1.0f - amount;  // at amount=1.0, every other pulse is silent
    }

private:
    int pulseCount = 0;
};
```

---

## 5. Creative Glottal Pulse Shaping

### 5.1 Beyond Realism: The Glottal Source as an Oscillator

The glottal pulse doesn't have to be realistic. When pushed to extremes, it becomes a powerful waveshaping oscillator. Key insight: **formant filters make anything sound vocal**.

**What happens at parameter extremes:**

| Extreme                    | Sound Character                              |
|----------------------------|----------------------------------------------|
| Rd -> 0.1 (hyper-pressed) | Buzzy, almost square-wave-like, aggressive   |
| Rd -> 5.0 (hyper-breathy) | Nearly pure noise, whispery drone            |
| OQ -> 0.1 (very narrow)   | Bright, clicky, thin impulse-like            |
| OQ -> 0.99 (fully open)   | Round, dark, almost sinusoidal               |
| Jitter -> 20%+            | Chaotic, granular-like, textural             |
| Shimmer -> 50%+           | Ring-modulation-like artifacts               |

### 5.2 Morphable Source: Crossfading Between Models

A powerful creative feature: morph between different source types.

```cpp
enum class SourceType { Rosenberg, LF, Sawtooth, Noise, Sine };

class MorphableGlottalSource
{
public:
    // position: 0-4 continuously morphs through all source types
    float process(float phase, float phaseInc, float morphPosition)
    {
        int srcA = (int)morphPosition;
        int srcB = juce::jmin(srcA + 1, 4);
        float blend = morphPosition - srcA;

        float sampleA = generateSource((SourceType)srcA, phase, phaseInc);
        float sampleB = generateSource((SourceType)srcB, phase, phaseInc);

        return sampleA * (1.0f - blend) + sampleB * blend;
    }

private:
    float generateSource(SourceType type, float phase, float phaseInc)
    {
        switch (type)
        {
            case SourceType::Rosenberg:
                return rosenbergPulse(phase);
            case SourceType::LF:
                return lfBank.readSample(phase, currentRdNorm);
            case SourceType::Sawtooth:
            {
                float saw = 2.0f * phase - 1.0f;
                saw -= polyBLEP(phase, phaseInc);  // anti-aliased
                return saw;
            }
            case SourceType::Noise:
                return juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f;
            case SourceType::Sine:
                return std::sin(2.0f * juce::MathConstants<float>::pi * phase);
            default:
                return 0.0f;
        }
    }

    float rosenbergPulse(float phase)
    {
        float openPhase = 0.4f;
        float closePhase = 0.16f;

        if (phase < openPhase)
        {
            float t = phase / openPhase;
            return 0.5f * (1.0f - std::cos(juce::MathConstants<float>::pi * t));
        }
        else if (phase < openPhase + closePhase)
        {
            float t = (phase - openPhase) / closePhase;
            return std::cos(juce::MathConstants<float>::pi * 0.5f * t);
        }
        return 0.0f;
    }

    LFWavetableBank lfBank;
    float currentRdNorm = 0.5f;
};
```

### 5.3 Glottal Wavetable Import/Drawing

Allow users to draw custom glottal pulses (like Serum's wavetable editor):

```cpp
class CustomGlottalTable
{
public:
    // User draws the pulse shape, we normalize and band-limit it
    void setFromUserDrawing(const float* points, int numPoints)
    {
        // Resample to table size
        for (int i = 0; i < TABLE_SIZE; ++i)
        {
            float pos = (float)i / TABLE_SIZE * numPoints;
            int idx = (int)pos;
            float frac = pos - idx;
            table[i] = points[idx] * (1.0f - frac) + points[juce::jmin(idx+1, numPoints-1)] * frac;
        }

        // Remove DC
        float dc = 0.0f;
        for (int i = 0; i < TABLE_SIZE; ++i) dc += table[i];
        dc /= TABLE_SIZE;
        for (int i = 0; i < TABLE_SIZE; ++i) table[i] -= dc;

        // Generate band-limited versions (mipmap)
        generateMipmaps();
    }

private:
    static constexpr int TABLE_SIZE = 2048;
    float table[TABLE_SIZE];
};
```

### 5.4 Creative Parameter Interactions

**Rd + Formant position = vowel character control:**
- Low Rd + tight formants = aggressive, cutting vocal
- High Rd + wide formants = ethereal, choir-like pad

**Jitter + Shimmer = organic texture:**
- Low amounts = lifelike vocal
- High amounts = granular, textural sound design

**Source morph + formant morph = complete vocal transformation:**
- Morph from LF pulse through sawtooth to noise while morphing formants from /a/ to /i/ = alien vocal effects

---

## 6. Formant-Pitch Interaction

### 6.1 The Problem

When F0 is high (soprano range, >500 Hz), the harmonics are widely spaced. Formant peaks may fall between harmonics, causing:
- Weak or missing formant resonances
- Unstable vowel identity
- Thin, hollow timbre

When F0 is low (bass range, <100 Hz), harmonics are densely packed and formants are well-excited, but:
- Individual harmonics become audible as a "buzz"
- The voice sounds less pitch-like and more noise-like

### 6.2 Formant Tuning in Singing

Trained singers instinctively adjust their vocal tract (and hence formant frequencies) to align formants with nearby harmonics. This is called "resonance tuning" or "formant tuning."

**Key strategies:**
1. **F1 tuning (sopranos):** Above ~C5 (523 Hz), sopranos widen their jaw to raise F1 to match the fundamental. Without this, the first formant falls below F0 and the vowel collapses.
2. **Singer's formant cluster:** Classically trained singers cluster F3, F4, F5 around 2500-3500 Hz. This creates a formant peak that always coincides with some harmonic, providing "ring" and projection.
3. **Vowel modification:** At high pitches, singers subtly shift vowels toward more open variants (/i/ becomes more /I/, /u/ becomes more /U/) to keep formants aligned.

### 6.3 Implementation Options for O-Formant

**Option A: No automatic tuning (simplest, recommended as default)**
- Formant frequencies stay fixed regardless of pitch
- Sounds natural for most of the range
- Only problematic at extreme high pitches
- Most predictable for music production

**Option B: Automatic F1 tracking (subtle, natural)**
```cpp
float autoTuneF1(float f1Target, float f0)
{
    // If F0 exceeds F1, push F1 up to at least F0 * 1.1
    if (f0 > f1Target * 0.9f)
    {
        float minF1 = f0 * 1.1f;
        return juce::jmax(f1Target, minF1);
    }
    return f1Target;
}
```

**Option C: Full harmonic-formant alignment (exposed as parameter)**
```cpp
float formantTune(float formantFreq, float f0, float tuneAmount)
{
    if (tuneAmount <= 0.0f || f0 <= 0.0f)
        return formantFreq;

    // Find nearest harmonic to this formant
    float harmonicNum = std::round(formantFreq / f0);
    float nearestHarmonic = harmonicNum * f0;

    // Blend between natural and tuned position
    return formantFreq + tuneAmount * (nearestHarmonic - formantFreq);
}
```

**Option D: Singer's Formant injection (additional resonance)**
```cpp
class SingersFormant
{
public:
    void prepare(double sampleRate)
    {
        // Bandpass at ~3000 Hz, moderately narrow
        filter.setCoefficients(
            juce::IIRCoefficients::makeBandPass(sampleRate, 3000.0f, 3.0f));
    }

    float process(float input, float amount)
    {
        float resonance = filter.processSingleSampleRaw(input);
        return input + resonance * amount;
    }

private:
    juce::IIRFilter filter;
};
```

### 6.4 Recommendation for O-Formant

Expose formant tuning as a single knob (0-100%):
- **0%** = Fixed formants (predictable, good for most use)
- **50%** = Subtle F1 tracking + mild harmonic alignment
- **100%** = Aggressive alignment (singer's tuning, great for leads/solos)

This gives users the creative choice without needing to understand the acoustics.

### 6.5 Pitch-Dependent Spectral Considerations

The interaction between pitch and the glottal source spectrum matters:

```
At low F0 (80 Hz):   harmonics at 80, 160, 240, 320, 400... (dense, all formants well-excited)
At mid F0 (220 Hz):  harmonics at 220, 440, 660, 880...      (moderate spacing)
At high F0 (600 Hz): harmonics at 600, 1200, 1800, 2400...   (sparse, formants may be missed)
```

**Practical implication:** At high pitches, the glottal source spectrum matters less (fewer harmonics to shape), and the formant filter tuning matters more.

---

## 7. Complete Glottal Source Architecture for O-Formant

### 7.1 Recommended Signal Flow

```
                                +---> [Aspiration Noise] --+
                                |       (phase-gated)      |
[LF Wavetable Bank] --+--> [Source Mix] --> [Spectral Tilt] --> [Formant Filters] --> Output
                       |        ^
[Source Morph] --------+        |
  (Rosenberg/LF/Saw/etc)       |
                           [Breathiness Mix]
                                ^
                           [Jitter/Shimmer]
                           (applied per-cycle)
```

### 7.2 Per-Cycle Processing

```cpp
class GlottalSource
{
public:
    float processSample()
    {
        // Check for new cycle
        if (phase >= 1.0f)
        {
            phase -= 1.0f;

            // Per-cycle: apply jitter to period
            float jitteredPeriod = jitter.getNextPeriod(currentPeriod, jitterAmount);
            phaseIncrement = 1.0f / jitteredPeriod;

            // Per-cycle: apply shimmer to amplitude
            cycleGain = shimmer.getNextGain(shimmerAmount);

            // Per-cycle: subharmonic pattern
            cycleGain *= subharmonic.getGainForPulse(subharmonicAmount);
        }

        // Generate source
        float source = lfBank.readSample(phase, rdNormalized) * cycleGain;

        // Mix aspiration noise (phase-gated)
        float noise = aspirationNoise.processWithGating(
            whiteNoise(), phase, currentOQ);
        source = source * (1.0f - breathiness) + noise * breathiness;

        // Advance phase
        phase += phaseIncrement;

        return source;
    }

private:
    LFWavetableBank lfBank;
    JitterGenerator jitter;
    ShimmerGenerator shimmer;
    SubharmonicGenerator subharmonic;
    AspirationNoise aspirationNoise;

    float phase = 0.0f;
    float phaseIncrement = 0.0f;
    float currentPeriod = 200.0f;
    float cycleGain = 1.0f;
    float rdNormalized = 0.5f;
    float currentOQ = 0.5f;

    float jitterAmount = 0.005f;
    float shimmerAmount = 0.01f;
    float breathiness = 0.05f;
    float subharmonicAmount = 0.0f;
};
```

### 7.3 User-Facing Parameter Summary

| Parameter          | Internal Mapping              | Range       | Default |
|--------------------|-------------------------------|-------------|---------|
| **Voice Quality**  | Rd (0.3-2.7)                 | 0-100%      | 40%     |
| **Breathiness**    | Noise mix + aspiration filter | 0-100%      | 5%      |
| **Brightness**     | Spectral tilt filter cutoff   | 0-100%      | 50%     |
| **Open Quotient**  | OQ override (bypasses Rd OQ) | 0-100%      | Auto    |
| **Roughness**      | Jitter + Shimmer combined    | 0-100%      | 2%      |
| **Growl**          | Subharmonic amount            | 0-100%      | 0%      |
| **Source Shape**    | Morph position (5 sources)   | 0-100%      | 25%     |
| **Formant Tune**   | Harmonic-formant alignment   | 0-100%      | 0%      |

---

## 8. References and Key Sources

### Academic Papers
- Fant, G. (1995). "The LF-model revisited. Transformations and frequency domain analysis." STL-QPSR, 36(2-3), 119-156. -- Defines Rd parameter and R-parameter regression.
- Fant, G., Liljencrants, J., & Lin, Q. (1985). "A four-parameter model of glottal flow." STL-QPSR, 26(4), 1-13. -- Original LF model.
- Gobl, C. (2017). "Reshaping the Transformed LF Model: Generating the Glottal Source from the Waveshape Parameter Rd." Interspeech. -- Newton-Raphson solver for Rd control.
- Yuen-Chen Yang et al. (2023). "GOLF: A Singing Voice Synthesiser with Glottal Flow Wavetables and LPC Filters." -- Wavetable approach to LF synthesis.
- Valimaki, V., et al. (2010). "Perceptually informed synthesis of bandlimited classical waveforms using integrated polynomial interpolation." JASA. -- PolyBLEP/BLEP theory.

### Implementations
- Python LF model: https://github.com/mvsoom/lf-model
- Tolg vocoder (C++): https://github.com/tolg-voice/tolg
- GOLF synthesizer: https://github.com/yoyololicon/golf
- PolyBLEP oscillator (C++): https://github.com/martinfinke/PolyBLEP
- sndkit BLEP: https://paulbatchelor.github.io/sndkit/blep/

### Reference Implementations for O-Formant
- Singing synthesis formant tracking: https://ccrma.stanford.edu/CCRMA/Courses/152/singing.html
- Voice quality analysis: https://pmc.ncbi.nlm.nih.gov/articles/PMC4818273/
- Glottal source processing survey: https://arxiv.org/pdf/1912.12604
