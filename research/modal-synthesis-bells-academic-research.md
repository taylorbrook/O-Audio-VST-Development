---
title: "Modal Synthesis for Bells and Metallic Percussion: Academic Research Compilation"
created: 2026-02-02
last_verified: 2026-02-06
juce_version: "8.0.4"
summary: "Academic research compilation from CCRMA Stanford, IRCAM, and key papers on modal synthesis for bell and metallic percussion sounds, including frequency-dependent damping formulas, multi-stage decay envelopes, and specific parameter values for implementation."
domain: dsp
type: algorithm
keywords:
  - modal-synthesis
  - bells
  - metallic-percussion
  - physical-modeling
  - frequency-damping
  - decay-envelopes
  - partials
  - ccrma
stages: [0, 2]
agents: [dsp, research]
---

# Modal Synthesis for Bells and Metallic Percussion: Academic Research Compilation

## Executive Summary

This document compiles research from CCRMA Stanford, IRCAM, and key academic papers on modal synthesis for bell and metallic percussion sounds. It includes mathematical formulas for frequency-dependent damping, multi-stage decay envelopes, and specific parameter values for implementation.

---

## 1. CCRMA Stanford - Julius Smith's Physical Modeling

### 1.1 Modal Synthesis Fundamentals

Modal synthesis models sound as a sum of decaying sinusoids, where each mode has three parameters:
- **Frequency** (f_k): The resonant frequency in Hz
- **Amplitude** (A_k): The initial amplitude of the mode
- **Decay/Loss Factor** (R_k): Controls how quickly the mode decays

**Core Modal Signal:**
```
y(t) = sum_{k=1}^{N} A_k * e^(-R_k * t) * sin(2*pi*f_k*t + phi_k)
```

### 1.2 Frequency-Dependent Damping Formula

From Nathan Ho's synthesis research (building on CCRMA principles):

**Basic Model:**
```
R_k = b_1 + b_3 * f_k^2
```

Where:
- R_k = loss factor (reciprocal of decay time tau_k)
- b_1 = frequency-independent damping (units: s^-1)
- b_3 = frequency-dependent damping coefficient (units: s)
- f_k = frequency of mode k (Hz)

**Reference Piano Parameters (Middle C):**
- b_1 = 0.5 s^-1
- b_3 = 1.58 x 10^-10 s

**Extended Model (Pitch-Dependent):**
```
R_k = b_1 + (b_3 * f_k^2) / f_0^2 + b_0 * f_0^2
```

This allows:
- High-frequency damping (upper partials decay faster)
- Different decay times for each note
- Decoupled pitch and frequency damping

### 1.3 T60 Decay Time Relationship

From CCRMA's spectral audio signal processing:

```
T60 = ln(1000) * tau ≈ 6.91 * tau
```

Where:
- T60 = time to decay by 60 dB
- tau = time constant (time to decay to 1/e)

**Relationship to Loss Factor:**
```
R_k = 1 / tau_k
T60_k = 6.91 / R_k
```

### 1.4 Biquad Resonator Implementation

**Pole Radius to Bandwidth:**
```
R = e^(-pi * B * T)
```

Where:
- R = pole radius
- B = bandwidth in Hz
- T = sampling period (1/fs)

**Biquad Coefficients:**
```
theta = 2 * pi * f_k / fs  (resonant frequency in radians)
a1 = -2 * R * cos(theta)
a2 = R^2
```

**Decay Time to Pole Radius:**
```
R = e^(-T / tau)
```

Or equivalently:
```
R = e^(-6.91 * T / T60)
```

---

## 2. IRCAM Modalys System

### 2.1 Modal Theory Foundation

Modal synthesis is based on the principle that any vibration of a structure can be decomposed as a superposition of its basic modes of vibration. Each mode is defined by:
- A particular shape the structure assumes when vibrating
- A specific resonant frequency
- A damping factor for energy dissipation

### 2.2 Modal Data Sources

1. **Analytic Modeling**: For simple structures (ideal strings, tubes, plates)
2. **Experimental Measurements**: For real structures (violin body, piano soundboard)
3. **Finite Element Methods (FEM)**: For arbitrary virtual shapes

### 2.3 Damping Model

Total modal damping = Material damping + Acoustic damping

The damping of each eigenmode is independent:
```
zeta_total = zeta_material + zeta_acoustic
```

---

## 3. Risset Bell Algorithm

### 3.1 Original 11-Partial Structure

Jean-Claude Risset's bell uses additive synthesis with 11 inharmonic partials:

| Partial | Freq Multiplier | Amplitude | Duration Ratio |
|---------|-----------------|-----------|----------------|
| 1       | 0.56            | 1.0       | 1.0            |
| 2       | 0.563           | 0.67      | 0.9            |
| 3       | 0.92            | 1.0       | 0.65           |
| 4       | 0.923           | 1.8       | 0.55           |
| 5       | 1.19            | 2.67      | 0.325          |
| 6       | 1.7             | 1.46      | 0.35           |
| 7       | 2.0             | 1.33      | 0.25           |
| 8       | 2.74            | 1.33      | 0.2            |
| 9       | 3.0             | 1.0       | 0.15           |
| 10      | 3.74            | 1.33      | 0.1            |
| 11      | 4.07            | 0.75      | 0.075          |

### 3.2 Envelope Structure

Each partial uses a quartic (fourth power) amplitude envelope:
- **Attack**: Very fast (1-2 ms), curved exponential
- **Decay**: Exponential decay with partial-specific duration
- **No Sustain or Release**: Pure percussion behavior

**Envelope Formula:**
```
// Attack phase (t < attack_time)
env = (t / attack_time)^4

// Decay phase (t >= attack_time)
env = e^(-alpha * (t - attack_time))
```

### 3.3 Key Insight: Inharmonicity

The frequency ratios are NOT integer harmonics:
- Creates beating and "shimmer" characteristic of bells
- Pairs of near frequencies (0.56/0.563, 0.92/0.923) create slow amplitude modulation

---

## 4. Church Bell Partial Ratios (Fletcher & Rossing)

### 4.1 Named Partials and Standard Tuning

| Partial Name   | Ratio to Nominal | Ratio to Hum | Musical Interval |
|----------------|------------------|--------------|------------------|
| Hum            | 0.25             | 1.0          | Octave below fundamental |
| Prime          | 0.50             | 2.0          | Fundamental |
| Tierce         | 0.60             | 2.4          | Minor 3rd |
| Quint          | 0.75             | 3.0          | Perfect 5th |
| Nominal        | 1.00             | 4.0          | Octave |
| Superquint     | 1.5              | 6.0          | Octave + 5th |
| Octave Nominal | 2.0              | 8.0          | Double octave |

### 4.2 Strike Note (Virtual Pitch)

The strike note is a perceptual phenomenon created by:
- Nominal, Superquint, and Octave Nominal
- These have ratios approximately 2:3:4
- The ear perceives a "missing fundamental" one octave below the nominal

### 4.3 Real Bell Example (Great Bede)

| Partial   | Frequency (Hz) | Ratio to Hum |
|-----------|----------------|--------------|
| Hum       | 102.5          | 1.00         |
| Prime     | 205.5          | 2.00         |
| Tierce    | 246.0          | 2.40         |
| Quint     | 306.0          | 2.99         |
| Nominal   | 409.5          | 3.99         |
| Octave N. | 836.0          | 8.15         |

---

## 5. Chaigne & Doutaut: Xylophone/Mallet Percussion

### 5.1 Key Publications

1. "Numerical simulations of xylophones. I. Time-domain modeling of the vibrating bars" - JASA 101(1), 1997
2. "Numerical simulations of xylophones. II. Time-domain modeling of the resonator" - JASA 104(3), 1998

### 5.2 Frequency-Dependent Damping in Bars

Their research confirmed:
- **Low frequencies**: Air viscosity damping predominates
- **High frequencies**: Internal material friction prevails
- **Upper partials decay faster than lower ones**

### 5.3 Bar Mode Frequencies

For a uniform bar with free ends:
```
f_n = (pi * h / (8 * L^2)) * sqrt(E / rho) * (n + 0.5)^2
```

Where:
- h = thickness
- L = length
- E = Young's modulus
- rho = density

Resulting frequency ratios: 1 : 2.76 : 5.40 : 8.93 : ...

---

## 6. Modal Audio Effects (DAFx 2017 - Carillon Study)

### 6.1 Modal Decomposition

Bell sounds decomposed into sum of decaying sinusoids:
```
s(t) = sum_k a_k * e^(-t/tau_k) * cos(omega_k * t + phi_k)
```

Parameters extracted per mode:
- omega_k: Modal frequency (rad/s)
- tau_k: Decay rate (seconds)
- a_k: Complex amplitude (includes phase)

### 6.2 Data Format

CSV columns: FREQUENCY, AMPLITUDE, DECAY, PHASE
- Each row = one vibrational mode
- Typical bell: 50-100 significant modes

---

## 7. Implementation Formulas

### 7.1 Mode Filter (Biquad Implementation)

**Transfer Function:**
```
H(z) = G * (1 + b1*z^-1) / (1 + a1*z^-1 + a2*z^-2)
```

**Coefficient Calculation from Mode Parameters:**
```
// Given: f (Hz), tau (seconds), fs (sample rate)
theta = 2 * pi * f / fs
R = exp(-1.0 / (tau * fs))

a1 = -2 * R * cos(theta)
a2 = R * R
b1 = 0  // or -1 for DC-blocking
G = (1 - R) * amplitude  // normalize for unity peak gain
```

### 7.2 Frequency-Dependent Decay Model

**For Bell-Like Sounds:**
```cpp
float computeDecayTime(float freq, float baseDecay, float highDamp) {
    // Higher frequencies decay faster
    // baseDecay: decay time at reference frequency (e.g., 440 Hz)
    // highDamp: controls how much faster high frequencies decay

    float ratio = freq / 440.0f;
    float decayMultiplier = 1.0f / (1.0f + highDamp * ratio * ratio);
    return baseDecay * decayMultiplier;
}
```

**Alternative (Nathan Ho's Model):**
```cpp
float computeLossFactor(float freq, float b1, float b3) {
    // b1: frequency-independent loss (typ. 0.5)
    // b3: frequency-dependent coefficient (typ. 1.58e-10 for piano)
    return b1 + b3 * freq * freq;
}

float decayTimeFromLoss(float R) {
    return 1.0f / R;  // tau in seconds
}

float T60fromLoss(float R) {
    return 6.91f / R;  // T60 in seconds
}
```

### 7.3 Multi-Stage Envelope for Bells

**Three-Phase Bell Envelope:**
```cpp
struct BellEnvelope {
    // Phase 1: Strike transient (very short, bright)
    float strikeAttack = 0.001f;   // 1ms
    float strikeDecay = 0.05f;     // 50ms
    float strikeLevel = 1.0f;

    // Phase 2: Body tone (medium decay)
    float bodyDecay = 2.0f;        // 2 seconds
    float bodyLevel = 0.7f;

    // Phase 3: Hum tail (long, low frequency)
    float tailDecay = 8.0f;        // 8 seconds
    float tailLevel = 0.3f;
};

float computeEnvelope(float t, const BellEnvelope& env) {
    if (t < env.strikeAttack) {
        // Attack ramp
        return env.strikeLevel * (t / env.strikeAttack);
    }
    else if (t < env.strikeAttack + env.strikeDecay) {
        // Strike decay to body
        float phase = (t - env.strikeAttack) / env.strikeDecay;
        return lerp(env.strikeLevel, env.bodyLevel, phase);
    }
    else {
        // Combined body and tail decay
        float elapsed = t - env.strikeAttack - env.strikeDecay;
        float bodyEnv = env.bodyLevel * exp(-elapsed / env.bodyDecay);
        float tailEnv = env.tailLevel * exp(-elapsed / env.tailDecay);
        return bodyEnv + tailEnv;
    }
}
```

### 7.4 Per-Partial Decay Scaling

```cpp
// Apply frequency-dependent decay to each partial
for (int i = 0; i < numPartials; i++) {
    float freq = baseFreq * partialRatios[i];

    // Higher partials decay faster
    float decayScale = 1.0f / (1.0f + freqDampCoeff * pow(freq / baseFreq, 2));
    float partialDecay = baseDecay * decayScale * durationRatios[i];

    partials[i].setDecayTime(partialDecay);
}
```

---

## 8. Key Insights for Implementation

### 8.1 What Makes Bells Sound Like Bells

1. **Inharmonic partials**: Non-integer frequency ratios
2. **Minor third presence**: The tierce (2.4x ratio) is essential
3. **Frequency-dependent decay**: Upper partials die faster
4. **Beating**: Near-frequency partial pairs create shimmer
5. **Strike transient**: Bright, noisy attack that fades quickly

### 8.2 Envelope Architecture

- **Attack**: 1-5 ms (quartic or exponential curve)
- **Strike decay**: 20-100 ms (noise/transient component)
- **Body decay**: 0.5-3 seconds (main harmonic content)
- **Tail/Hum**: 5-15 seconds (low partials only)

### 8.3 Typical Mode Count

- Simple bell synth: 8-16 modes
- Realistic bell: 30-60 modes
- High-fidelity carillon: 80-120 modes per bell

---

## 9. References

### Academic Papers
- Chaigne, A. & Doutaut, V. (1997). "Numerical simulations of xylophones. I." JASA 101(1)
- Doutaut, V., Matignon, D., & Chaigne, A. (1998). "Numerical simulations of xylophones. II." JASA 104(3)
- Canfield-Dafilou, E.K. & Werner, K.J. (2017). "Modal Audio Effects: A Carillon Case Study." DAFx-17
- Carvalho, M., Debut, V. & Antunes, J. (2021). "Physical modelling techniques for bell characterization." Heritage Science

### Books
- Fletcher, N.H. & Rossing, T.D. (1998). "The Physics of Musical Instruments" (2nd ed.). Springer.
- Roads, C. (1996). "The Computer Music Tutorial." MIT Press.
- Farnell, A. (2010). "Designing Sound." MIT Press.
- Zoelzer, U. (ed.) (2011). "DAFX: Digital Audio Effects" (2nd ed.). Wiley.

### Online Resources
- CCRMA Physical Audio Signal Processing: https://ccrma.stanford.edu/~jos/pasp/
- Nathan Ho's Modal Synthesis: https://nathan.ho.name/posts/exploring-modal-synthesis/
- Sound on Sound - Synthesizing Bells: https://www.soundonsound.com/techniques/synthesizing-bells
- Lurie Carillon Modal Data: https://ccrma.stanford.edu/~kermit/website/bells.html

---

## 10. Quick Reference: Formulas Summary

| Parameter | Formula | Notes |
|-----------|---------|-------|
| Loss Factor | R_k = b_1 + b_3*f_k^2 | Frequency-dependent decay |
| Time Constant | tau = 1/R | Decay to 1/e |
| T60 | T60 = 6.91*tau | 60 dB decay time |
| Pole Radius | R = e^(-pi*B*T) | B=bandwidth, T=1/fs |
| Biquad a1 | -2*R*cos(theta) | theta = 2*pi*f/fs |
| Biquad a2 | R^2 | Pole radius squared |
| Hum Ratio | 0.25 (to nominal) | Lowest partial |
| Tierce Ratio | 0.60 (to nominal) | Minor 3rd character |
