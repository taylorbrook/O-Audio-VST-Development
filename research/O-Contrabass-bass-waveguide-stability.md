---
title: O-Contrabass — Low-Frequency Waveguide Stability and Bass-String Stiffness
created: 2026-04-25
juce_version: 8.0.4
type: research
keywords:
  - digital waveguide
  - bowed string
  - inharmonicity
  - dispersion filter
  - allpass cascade
  - Thiran allpass
  - Lagrange interpolation
  - denormal prevention
  - oversampling
  - bass register
  - O-Contrabass
related:
  - bow-string-friction-models.md
  - O-Bowed-research-synthesis.md
  - O-Bowed-acoustic-instrument-research.md
---

# O-Contrabass — Low-Frequency Waveguide Stability and Bass-String Stiffness

> Research scope: long delay lines (E1 = 41 Hz, ~1075 samples at 44.1 kHz), thick wound bass-string dispersion, denormal accumulation, fractional-delay tuning precision, oversampling at the friction junction, and click-free per-string detuning over ±1200 cents. Companion to `bow-string-friction-models.md` (friction & scattering) — this document focuses on the waveguide loop and string-stiffness modeling for the bass register specifically.

---

## Table of Contents

1. [Bass-Register Context: What Changes vs. Treble](#1-bass-register-context)
2. [Stiffness Dispersion in Thick Wound Bass Strings](#2-stiffness-dispersion)
   - 2.1 The Inharmonicity Coefficient B (numerical values)
   - 2.2 Bowed vs Plucked: Why Bowing Reduces Inharmonicity
   - 2.3 Cascaded Allpass Dispersion Filter (Rauhala/Välimäki)
   - 2.4 C++ Implementation Pattern
3. [Low-Fundamental Waveguide Stability (E1 = 41 Hz)](#3-low-fundamental-stability)
   - 3.1 Delay-Line Length Budget
   - 3.2 Denormal Accumulation in Long Feedback Loops
   - 3.3 Fractional-Delay Interpolation: Linear vs Lagrange vs Thiran
   - 3.4 Bridge Loss Filter Design
   - 3.5 Numerical Precision: float vs double in the Loop
4. [Oversampling at the Friction Junction (2x at 44.1 kHz)](#4-oversampling)
5. [Per-String Detune ±1200 cents (Click-Free Automation)](#5-detune-automation)
6. [Recommended Architecture for O-Contrabass](#6-recommendation)
7. [References](#7-references)

---

## 1. Bass-Register Context

The contrabass operates in the **41 Hz – 196 Hz** fundamental range (E1 to G3 open strings; ~250 Hz at the top of normal first-position fingering). Compared to treble bowed-string synthesis (violin/viola, fundamentals 200–1000 Hz+), the bass register changes the engineering budget in four important ways:

| Issue | Treble (e.g. violin G3 = 196 Hz) | Bass (E1 = 41.2 Hz) |
|------|----------------------------------|---------------------|
| Delay line at 44.1 kHz | ~225 samples | **~1071 samples** |
| Round-trip period | 5.1 ms | **24.3 ms** |
| Audible partials in 12 kHz | ~60 | **~290** |
| String stiffness contribution | Small (thin core) | **Large (thick wound construction)** |
| Denormal exposure | Decay <1 s, low risk | **Long sustains, high risk** |
| Cents/sample tuning resolution at 44.1 kHz | ~0.4 cents | **~1.6 cents** (need fractional delay) |

The fundamentals shape every DSP decision: dispersion modeling becomes essential, denormal handling is mandatory, and integer-sample delay rounding gives unacceptable tuning error.

---

## 2. Stiffness Dispersion in Thick Wound Bass Strings

### 2.1 The Inharmonicity Coefficient B

The classical dispersion relation for a stiff string pinned at both ends:

```
f_n  =  n * f_0 * sqrt(1 + B * n^2)
```

where `B` is the **inharmonicity coefficient** (dimensionless). Theoretical formula:

```
B = (pi^3 * E * a^4) / (16 * L^2 * T)         (solid-core, Russell)
B = (pi^2 * Y * S * kappa^2) / (T * L^2)       (general form)
```

- `E` (or `Y`) — Young's modulus of the core
- `a` — string radius (or `S` cross-sectional area, `kappa` radius of gyration)
- `L` — speaking length
- `T` — tension

**Key observation for wound strings:** The above formulas describe an *unwrapped* string. Wound bass strings have a thin steel core with overwinding, which dramatically *reduces* effective stiffness compared to a solid wire of the same outer diameter. This is by design — without overwinding, a solid bass string would have intolerable inharmonicity.

#### Numerical values

Direct published B measurements for double bass are sparse, but the picture from related instruments:

| String / Note | f0 (Hz) | B (typical) | Notes / Source |
|---------------|---------|-------------|----------------|
| Piano A0 | 27.5 | ~3.1e-4 | Investigating Inharmonicity (Edinburgh, 2024) |
| Piano Bb-1 (subcontra) | ~29 | ~4.5e-4 | Acta Acustica (2021) |
| Piano A-1 (bass extension) | 27.5 | ~5.75e-4 | Acta Acustica (2021) |
| Piano A1 | 55 | ~2.5e-4 | (avg of partials 6–12) |
| **Double bass E1 (open)** | **41.2** | **~5e-5 to 2e-4 (estimated)** | No direct published measurement; bow steady-state mode-locks anyway (see 2.2) |
| Bass guitar E1 | 41.2 | order ~1e-4 | Springer 2020 (tapered/lumped construction) |
| Piano A4 | 440 | ~7.5e-4 | Reference |
| Piano top treble | ~4000 | ~1.5e-2 | Reference |

**Practical bass-range estimate for O-Contrabass:**
- E string: **B ≈ 1e-4** (low-stiffness wound)
- A string: **B ≈ 7e-5**
- D string: **B ≈ 5e-5**
- G string: **B ≈ 3e-5**

Expose this as the `String Stiffness 0–100%` parameter in BRIEF.md by mapping 0% → 1e-6 (effectively ideal string), 50% → measured value, 100% → 5x measured (heavily inharmonic / character).

### 2.2 Bowed vs Plucked: Why Bowing Reduces Inharmonicity

**Critical insight that simplifies the bass dispersion problem:**

> "Double bass strings exhibit inharmonicity when notes are plucked using pizzicato, but this inharmonicity disappears when the strings are bowed, because the bow's stick-slip action is periodic, driving all of the resonances of the string at exactly harmonic ratios even if it has to drive them slightly off their natural frequency."
> — Phys.UNSW Music Acoustics

The nonlinear stick-slip force at the bow contact point is a **strongly periodic forcing function** at exactly `f_0`. This phase-locks the string into a Helmholtz mode whose partials ride at integer multiples — the dispersive eigenmodes get pulled into harmonic ratios by the friction-junction nonlinearity.

**Implications for O-Contrabass:**
1. **Steady-state bowed tone is much less inharmonic than the open-loop string**. A modest dispersion filter is sufficient — this is *not* piano synthesis.
2. **Inharmonicity is musically audible during attack transients** (before mode-locking is established), in pizzicato (not v1.0), and when bow pressure is too low to enforce stick-slip. These are exactly the moments where the "stiffness" character appears.
3. The dispersion filter primarily affects: (a) attack character / "wood" quality, (b) damped tail timbre after note-off, (c) sub-harmonic content in drone mode where the friction nonlinearity is intentionally weakened.

Practical consequence: aim for a **lightweight dispersion filter** (1–4 first-order allpass sections per string, not the 8+ used in piano synthesis).

### 2.3 Cascaded Allpass Dispersion Filter (Rauhala/Välimäki)

The standard digital-waveguide approach to dispersion: insert a chain of first-order allpass sections in the loop. Each section provides frequency-dependent phase delay that approximates the `sqrt(1 + B*n^2)` dispersion curve.

#### Single first-order allpass

```
A(z) = (a + z^-1) / (1 + a * z^-1)
```

- `|a| < 1` for stability
- Magnitude `|A(e^jw)| = 1` exactly (no energy added or removed)
- Phase delay rises with frequency for `a > 0`

#### Cascade design (Rauhala & Välimäki, 2006)

For a cascade of `M` identical first-order allpass sections, the closed-form coefficient is computed from `f0`, `B`, and `M`. The published derivation uses a Thiran-style maximally-flat group-delay design:

```
// Tuning offset (key on 88-key piano scale, generalized to any f0)
I  = log2(f0 / 440) * 12 + 49        // virtual key number
k1, k2, k3 = constants from paper Table 1
m1..m4    = constants from paper Table 1

C  = m1 * log(B) + m2 * log(M) + m3 * log(B) * log(M) + m4
k  = k1 + k2 * I + k3 * I * I

a  = -C / k                          // first-order allpass coefficient
```

Numerical references (Rauhala 2008, Fig. 2): for B in [1e-6, 1e-3] and M = 4, `a` ranges roughly from -0.05 (low B) to -0.5 (high B). Negative `a` produces the rising-phase-delay characteristic of dispersive stiffness.

#### Stability

- `|a| < 1` strictly; cascading M sections never compromises stability if each section is stable.
- Coefficient computation is **closed-form**, so no iterative optimization runs at note-on (good for low-latency note triggering and continuous re-tuning).

#### Filter order recommendations

| f0 range | B (estimate) | Recommended M (first-order sections) |
|---------|--------------|--------------------------------------|
| E1–A1 (41–55 Hz) | 1e-4 to 7e-5 | **M = 4** (high-quality phase match for low partials) |
| D2–G2 (73–98 Hz) | 5e-5 to 4e-5 | **M = 2** |
| A2–G3 (110–196 Hz) | 3e-5 to 2e-5 | **M = 1** (or skip for ideal string) |

For the O-Contrabass `String Stiffness` parameter at 100%, max out at `M = 4` everywhere; at 0%, bypass dispersion entirely (one branch in the loop).

### 2.4 C++ Implementation Pattern

```cpp
// First-order allpass section (transposed direct form II — fewer multiplies, lower noise)
struct AllpassSection
{
    float a = 0.0f;     // coefficient, -1 < a < 1
    float z = 0.0f;     // single state element

    inline float tick(float x) noexcept
    {
        // y[n]  = a * x[n] + z[n-1]
        // z[n]  = x[n] - a * y[n]
        const float y = a * x + z;
        z = x - a * y;
        return y;
    }

    inline void reset() noexcept { z = 0.0f; }
};

// Dispersion filter: cascaded first-order allpasses
template <int MaxSections = 4>
struct DispersionFilter
{
    AllpassSection sections[MaxSections];
    int activeSections = 0;

    void prepare(float f0Hz, float B, int M)
    {
        activeSections = juce::jlimit(0, MaxSections, M);
        const float a = computeAllpassCoeff(f0Hz, B, M);
        for (int i = 0; i < activeSections; ++i)
            sections[i].a = a;
    }

    inline float process(float x) noexcept
    {
        for (int i = 0; i < activeSections; ++i)
            x = sections[i].tick(x);
        return x;
    }

    void reset() { for (auto& s : sections) s.reset(); }

private:
    static float computeAllpassCoeff(float f0, float B, int M)
    {
        // Closed-form from Rauhala/Valimaki 2006, simplified.
        // For production: use the full equation set (3..7) from the paper.
        constexpr float k1 = -0.0135f, k2 = 0.0058f, k3 = -0.000004f;
        constexpr float m1 =  0.0034f, m2 = 0.0179f, m3 = -0.0009f, m4 = -0.4986f;

        const float I  = std::log2(f0 / 440.0f) * 12.0f + 49.0f;  // key number
        const float lB = std::log(juce::jmax(B, 1e-9f));
        const float lM = std::log(static_cast<float>(juce::jmax(M, 1)));

        const float C = m1 * lB + m2 * lM + m3 * lB * lM + m4;
        const float k = k1 + k2 * I + k3 * I * I;

        return juce::jlimit(-0.99f, 0.99f, -C / k);
    }
};
```

**Loop placement:** Insert the dispersion filter once per round trip — typically **immediately before the bridge reflection filter**, on the right-going (toward bridge) wave path. This way each round trip gets one pass of dispersion, matching the physical situation where stiffness affects every traversal.

```
v_sr+ -> [DispersionFilter] -> [BridgeLossFilter] -> sign-invert -> v_sr-
```

If `M > 1` becomes too aggressive at low f0 (all-zeros piling up), distribute sections across both directions:

```
v_sr+ -> [Disp_M/2] -> [BridgeLoss] -> v_sr-
v_sl+ -> [Disp_M/2] -> [NutReflection] -> v_sl-
```

This gives lower group-delay-error per filter pass at the cost of one extra state-update per sample.

---

## 3. Low-Fundamental Waveguide Stability

### 3.1 Delay-Line Length Budget

Delay-line length per round trip:

```
L_samples = sample_rate / f_0
```

| Note | f0 (Hz) | 44.1 kHz | 88.2 kHz |
|------|---------|----------|----------|
| E1 (open low) | 41.2 | **1070.4** | **2140.8** |
| A1 | 55.0 | 801.8 | 1603.6 |
| D2 | 73.4 | 600.8 | 1201.6 |
| G2 | 98.0 | 450.0 | 900.0 |
| G3 (top open) | 196.0 | 225.0 | 450.0 |
| Detune −1200 cents from E1 | 20.6 | **2140.8** | **4281.6** |

Allocate delay line buffers for the **worst case at oversampled rate including detune**: at 88.2 kHz internal with full ±1200-cent detune from E1, allow **~4500 samples** of headroom per delay line. Round up to the next power of two (8192) for circular-buffer efficiency.

### 3.2 Denormal Accumulation in Long Feedback Loops

This is the **#1 stability risk** for an infinite-sustain bass waveguide.

When `Infinite Sustain = 100%` (loop gain near 1.0) and the user releases a note, the signal decays exponentially toward zero. As samples drop below `~1e-38`, they enter denormal range, and on x86/x64 each denormal multiplication can cost **30-100x normal CPU**. With a 1070-sample delay line at 41 Hz and a 100ms processing buffer, this can spike CPU from <5% to >50%.

#### Required mitigations

**1. ScopedNoDenormals at processBlock entry (mandatory):**

```cpp
void processBlock(AudioBuffer<float>& buffer, MidiBuffer& midi) override
{
    juce::ScopedNoDenormals noDenormals;   // sets MXCSR FTZ + DAZ on x86/x64,
                                           // FPSCR FZ on ARM
    // ... processing
}   // restores host's flags on scope exit
```

JUCE's `ScopedNoDenormals` handles x86/x64 (SSE2 MXCSR bits 0x8040) and ARM (FPSCR bit 24) transparently.

**2. DC-blocker / leak on the loop (defense in depth):**

Even with FTZ/DAZ active, very-long-tail decay can drift toward subnormals on hosts that have toggled flags. Add a tiny constant leak inside the loop filter:

```cpp
inline float bridgeFilter(float x) noexcept
{
    // One-pole LP with DC leak
    constexpr float kLeak = 1.0e-20f;
    y = g * ((1.0f - p) * x + p * y_prev) - kLeak;   // tiny negative bias
    y_prev = y;
    return y;
}
```

A constant `1e-20` is below the noise floor (-400 dB) but keeps numbers out of subnormal range during long fadeouts.

**3. Avoid the textbook "anti-denormal noise injection" trick:**
The classic fix of adding `1e-25 * noise` adds audible noise on long sustains and is unnecessary on JUCE 8 with `ScopedNoDenormals`. Skip it.

**4. ARM/Apple Silicon caveat:**
ARM cores still benefit from FZ mode, but the math library on macOS may misbehave with FZ active for some `std::expf`, `std::sin`, etc. calls. Keep math-heavy work (friction temperature evolution, vibrato LFO) **outside** any block where you've fudged additional flags, and rely on JUCE's default `ScopedNoDenormals` which is tested cross-platform.

### 3.3 Fractional-Delay Interpolation: Linear vs Lagrange vs Thiran

To tune to E1 = 41.2 Hz at 44.1 kHz, the required delay is **1070.388** samples — fractional. Integer rounding to 1070 gives 41.215 Hz (an error of 0.6 cents); rounding to 1071 gives 41.176 Hz (-1 cent). Tolerable for E1, but at higher pitches the fractional component is much more critical, and **modulation** (vibrato 12 cents at 5 Hz) absolutely requires fractional delay.

JUCE's `juce::dsp::DelayLine<T, InterpType>` provides three options:

| Type | Order | Latency-on-modulation | Phase response | Best for |
|------|-------|-----------------------|----------------|----------|
| `Linear` | 1 (FIR) | None — stateless | Mild HF rolloff | **Fast modulation** (vibrato, pitch automation) |
| `Lagrange3rd` | 3 (FIR) | None — stateless | Better HF accuracy | **Static delay**, accurate fundamental tuning |
| `Thiran` | 1 (IIR allpass) | **Stateful — transient on modulation** | Flat magnitude, distorted phase near Nyquist | **Static** delay, low CPU |

#### JUCE-documented caveat

> "[Thiran] is stateful so is unsuitable for applications requiring fast delay modulation."
> — JUCE `dsp::DelayLineInterpolationTypes::Thiran` reference

For O-Contrabass, **`Lagrange3rd` is the correct default** for the main string delay lines:

- Fundamental tuning is precise (key for low f0 where fractional component is critical).
- Stateless — no transient on `setDelay()` changes for vibrato, slow LFO, or per-string detune automation.
- HF response is well-behaved up to ~10 kHz at 44.1 kHz, ~20 kHz at 88.2 kHz internal — well above the audible content of any contrabass tone.
- 3rd-order FIR is cheap (4 multiplies, 3 adds per sample on modern CPUs).

For dispersive ringing of low partials specifically, the *integer* delay-line length combined with the dispersion filter does most of the tuning work. Lagrange handles the fractional offset accurately at f0.

#### Sub-sample accuracy
Lagrange3rd resolves to about 0.001 sample of effective delay error at the fundamental — at 44.1 kHz / 41 Hz, that's ~0.04 cents pitch error, well below perceptual threshold (~3-5 cents).

### 3.4 Bridge Loss Filter Design

The bridge filter is a per-round-trip lowpass that models energy radiated through the bridge into the body. It controls overall decay and brightness.

For bass strings, the budget is tight: at f0 = 41 Hz the round trip is 24 ms, so to get 5 second sustain at -60 dB you need round-trip gain of `10^(-60/20 / (5/0.024)) = 0.9986` — extremely close to unity. To get *infinite* sustain (drone mode) you need exactly 1.0, which is why drone mode requires extra care.

**One-pole lowpass (cheap, sufficient):**

```
H(z) = g * (1 - p) / (1 - p * z^-1)

DC gain = g
HF gain = g * (1-p) / (1+p)
```

| Mode | g (DC gain) | p (pole) | -3 dB freq @ 44.1 kHz | Equivalent T60 at f0=41 Hz |
|------|-------------|----------|-----------------------|-----------------------------|
| Pizzicato (not v1.0) | 0.995 | 0.5 | ~4 kHz | ~3.6 s |
| Bowed sustain | 0.999 | 0.3 | ~6 kHz | ~18 s (effectively ∞ while bowed) |
| Drone / Infinite Sustain | 0.99995 → 1.0 | 0.2 | ~8 kHz | minutes |

For **Brightness 80–12000 Hz** parameter (BRIEF.md), map to `p`:

```cpp
// Brightness in Hz → one-pole coefficient
float p = std::exp(-2.0f * juce::MathConstants<float>::pi * brightnessHz / sampleRate);
```

At 80 Hz cutoff, `p ≈ 0.9886` (very dark, drone-friendly). At 12 kHz, `p ≈ 0.184` (bright, bow-grit forward).

**Stability constraint:** Across the audio band, `|H(e^jw)| <= 1`. The magnitude is bounded by `|g|`, so just keep `g <= 1.0` (and ideally `g <= 0.9999` outside drone mode, with a separate gain-clamp branch for drone).

### 3.5 Numerical Precision: float vs double in the Loop

**Recommendation: float in the delay line, double in the friction-junction state.**

| Component | Type | Why |
|-----------|------|-----|
| Delay-line buffer | `float` (32-bit) | 1070-sample buffer, want cache-friendly. 24-bit mantissa is plenty for audio waveforms. |
| Allpass dispersion state `z` | `float` | Single-sample state, no accumulation issue. |
| Bridge filter state `y_prev` | `float` | Same reasoning. |
| Friction-junction (Newton-Raphson) | `double` | Iterative root-find benefits from double precision; otherwise convergence stalls. |
| Thermal friction temperature | `double` | Slow integrator over many samples; float drift is audible. |
| Phase accumulator (LFO, vibrato) | `double` | 0.01 Hz LFO over 10 minutes accumulates float error. |
| sample → delay-time computation | `double` | Tuning math, especially for ±1200 cent detune × 41 Hz fundamental. |

The delay line itself can stay in `float` as long as the **address arithmetic** (delay length, fractional offset) is computed in `double`. Lagrange interpolation should also be done in `double` if you can afford it — the difference is barely measurable in CPU but eliminates a class of subtle pitch-modulation artifacts.

JUCE's `juce::dsp::DelayLine<T, ...>` is templated; instantiate as `juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd>` and convert delay times from `double` to `float` only at the `setDelay()` call site.

---

## 4. Oversampling at the Friction Junction

### 4.1 Is 2x enough at 44.1 kHz host rate?

**Verdict: 2x is sufficient for E1–G3 sustained bowing. Consider 4x only for low-pressure attacks where elasto-plastic friction can produce wideband transients.**

The critical aliasing source in a bowed-string waveguide is the **friction-junction nonlinearity**. The hyperbolic friction curve `mu(v_rel) = mu_d + (mu_s - mu_d) * v_0 / (v_0 + |v_rel|)` is smooth (C∞), and its harmonic content is dominated by low orders — at typical bow speeds, energy above the 8th harmonic of `f0` is small relative to the fundamental.

For E1 (41 Hz): the 8th harmonic is 328 Hz, well below Nyquist even at 44.1 kHz. Aliasing risk is minimal in steady state. **The exception is the attack transient**, where the friction curve traverses its full range rapidly and produces broadband content — this is why O-Bowed already runs 2x oversampling in practice.

The friction-models doc (line 818) recommends 2x:

> "Use 2x oversampling (internal 88.2 kHz at 44.1 kHz host rate). This provides adequate resolution for the nonlinear friction computation and attack transients without excessive CPU cost."

For O-Contrabass specifically, the **slow attacks** of the orchestral preset (Bow Speed default 0.15 m/s, 5x slower than violin defaults) actually *reduce* aliasing pressure compared to a violin synth. **2x is the right answer.**

### 4.2 JUCE pattern: `juce::dsp::Oversampling`

```cpp
class OContrabassEngine
{
    juce::dsp::Oversampling<float> oversampler;   // member

public:
    OContrabassEngine()
        : oversampler(/*numChannels*/   1,
                      /*factor*/        1,                                            // 2^1 = 2x
                      /*type*/          juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
                      /*isMaxQuality*/  true,
                      /*useIntegerLatency*/ false)
    {}

    void prepare(double hostSr, int blockSize)
    {
        oversampler.initProcessing(static_cast<size_t>(blockSize));
        // Internal sample rate for the friction junction:
        const double internalSr = hostSr * 2.0;
        engine.prepare(internalSr, blockSize * 2);
    }

    void processBlock(juce::AudioBuffer<float>& buffer)
    {
        juce::ScopedNoDenormals noDenormals;

        juce::dsp::AudioBlock<float> hostBlock(buffer);
        auto upBlock = oversampler.processSamplesUp(hostBlock);

        // Friction junction + waveguide loop run at 2x rate
        engine.process(upBlock);

        oversampler.processSamplesDown(hostBlock);
    }
};
```

#### Filter type choice: `filterHalfBandPolyphaseIIR` vs `filterHalfBandFIREquiripple`

| Filter | Phase | Latency @ 2x | CPU | Choice for O-Contrabass |
|--------|-------|--------------|-----|--------------------------|
| `filterHalfBandPolyphaseIIR` | Minimum-phase (nonlinear) | ~1–3 samples | Low | **Recommended** — minimal latency preserves "0 algorithmic latency" goal |
| `filterHalfBandFIREquiripple` | Linear-phase | 32–64 samples | Higher | Only if mastering-grade phase accuracy needed |

The polyphase IIR option uses cascaded second-order allpass sections (similar to dispersion filter math) and is the canonical choice for plugin-internal oversampling where transparent high-frequency response matters more than linear phase.

#### Latency
With `useIntegerLatency = false`, the oversampler reports a fractional latency. For O-Contrabass the BRIEF specifies "0 samples (causal waveguide)" — note that this refers to **algorithmic latency in the waveguide path**, not the oversampler's anti-aliasing latency. Report `oversampler.getLatencyInSamples()` to the host via `setLatencySamples()` for sample-accurate DAW alignment.

```cpp
prepareToPlay(sampleRate, blockSize):
    setLatencySamples(static_cast<int>(std::ceil(oversampler.getLatencyInSamples())));
```

### 4.3 When does 2x fail?

Documented cases from the literature where 2x is insufficient:

1. **Elasto-plastic friction with hard rosin** (Serafin/Smith). The bristle-spring derivative can produce sub-sample-fast force transitions that need 4x.
2. **Pizzicato / col legno** (out of scope for O-Contrabass v1.0).
3. **Newton-Raphson divergence** at extreme parameters — not really an aliasing issue but a stability one; mitigate with iteration cap and fallback to incoming-velocity approximation.

**For BRIEF.md's hyperbolic-friction-leaning bass model: 2x is correct.**

---

## 5. Per-String Detune ±1200 cents (Click-Free Automation)

The `Per-String Detune` parameter modulates the *delay-line length* of each string from -1200 to +1200 cents:

```
delaySamples_target  =  (sampleRate / f0_nominal) * 2^(-cents/1200)
```

At E1 with -1200 cents, `delaySamples_target ≈ 2140` (octave down). At +1200 cents, `delaySamples_target ≈ 535` (octave up).

### Click-free patterns

**1. SmoothedValue with per-sample setDelay():**

```cpp
juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> detuneSmoothed[4];

void prepare(double sr) {
    for (auto& s : detuneSmoothed)
        s.reset(sr, /*rampLength*/ 0.020);   // 20 ms ramp — perceptually smooth, not too slow
}

void processSampleString(int stringIdx)
{
    const float currentDelay = detuneSmoothed[stringIdx].getNextValue();
    delayLines[stringIdx].setDelay(currentDelay);
    const float out = delayLines[stringIdx].popSample(0);
    delayLines[stringIdx].pushSample(0, /*input from junction*/ in);
    // ...
}
```

**Critical:** call `setDelay()` and `popSample()` **once per sample** during a ramp. Block-rate updates produce zipper noise.

**2. Automation curve, not raw user value:**
DAW automation can produce step changes. Filter the parameter through a **separate** smoother that runs at the audio-thread rate (not the parameter-update rate):

```cpp
// In APVTS listener (UI / param thread):
atomicDetune[s].store(newCents);

// In audio thread, every block:
detuneSmoothed[s].setTargetValue(centsToDelay(atomicDetune[s].load()));
```

This decouples DAW automation from the audio-rate ramp — the smoother always sees a stable target and ramps continuously.

**3. Lagrange3rd interpolation absorbs the modulation:**
Because the chosen interpolation is **stateless** (FIR), `setDelay()` mid-stream produces no transient. Thiran would click; Lagrange does not.

**4. Use moderate ramp time (10–30 ms):**
- **5 ms or less** — risk of audible discontinuity, fast enough that the bowed-string steady state hasn't re-established.
- **20 ms** — recommended default. Fast enough for expressive scordatura performance; slow enough to be artifact-free on rapid automation.
- **>50 ms** — feels sluggish for live MPE pitch bend.

**5. For *just-intoned drone* mode, ramp slower (100–500 ms):**
The drone use case (BRIEF.md "Just-Intoned Drone preset") settles into long sustains where the user wants gentle pitch motion. Expose a separate "Detune Glide" parameter for drone use cases.

**6. Avoid resetting state on retune:**
Do *not* call `delayLines[s].reset()` on detune changes — that nukes the in-flight wave content and produces a click. The waveguide should glide through the new delay length while continuing to ring.

### Edge case: detune crossing through unison (0 cents)

When the detune sweeps through 0 cents, no special handling is required if the smoother is linear in delay-samples. If the user maps the parameter as cents (logarithmic in delay), use a quadratic smoother or pre-compute samples from cents and smooth in samples space.

```cpp
// Convert cents to delay samples, then smooth in samples-domain:
float detuneToDelay(float cents, float baseDelay) {
    return baseDelay * std::exp2f(-cents / 1200.0f);
}
detuneSmoothed[s].setTargetValue(detuneToDelay(cents, baseDelay));   // smooth on delay axis
```

---

## 6. Recommended Architecture for O-Contrabass

Concrete configuration aligned with `BRIEF.md`:

```
processBlock:
    juce::ScopedNoDenormals noDenormals;
    Oversampling 2x (filterHalfBandPolyphaseIIR, isMaxQuality=true)

For each of 4 string voices (only one active under bow at a time):
    DelayLine<float, Lagrange3rd>  size = 8192 samples (worst case detune)
    DispersionFilter<MaxSections=4>
        E string: M=4, B = 1e-4 * stiffnessParam
        A string: M=2, B = 7e-5 * stiffnessParam
        D string: M=1, B = 5e-5 * stiffnessParam
        G string: M=1, B = 3e-5 * stiffnessParam
    BridgeFilter (one-pole LP)
        p = exp(-2*pi*brightnessHz / 88200)        // 88.2 kHz internal
        g = 1.0 - dampingPercent * 0.001 - 1e-5    // tiny leak unless drone
    NutReflection (sign-flip + soft -1 multiplier ~ -0.998)
    DetuneSmoother (LinearSmoothedValue, 20 ms ramp)
    StringStateUpdate (per-sample setDelay during ramp)

Friction Junction (2x rate):
    Hyperbolic friction curve (BRIEF.md default model)
    Bow-table approach (no Newton-Raphson) — sufficient for sustained bowing
    State in float (pre-computed table) — friction-models.md section 3.3.A

Body Resonator (1x rate, post-string):
    Bass-tuned wood modes (separate research thread)
```

#### Stability checklist (verify in pluginval)

- [ ] `ScopedNoDenormals` at processBlock entry
- [ ] All filter coefficients `|a| < 1`, `|p| < 1`, `g <= 1.0` (or strictly == 1.0 only in drone mode)
- [ ] Bridge filter includes leak term (`-1e-20` constant) outside drone mode
- [ ] DelayLine size >= worst-case (E1 - 1200 cents at 88.2 kHz internal) = 4282 samples → allocate 8192
- [ ] `setLatencySamples(oversampler.getLatencyInSamples())` reported to host
- [ ] Detune smoother ramp time = 20 ms default, exposed as parameter
- [ ] `Lagrange3rd` interpolation (NOT Thiran for the main delay line)
- [ ] Per-sample `setDelay()` calls during ramp (not block-rate)
- [ ] Friction junction state (Newton if used) in `double`; delay buffers in `float`

#### CPU budget (target <5%, BRIEF.md)

Rough budget on M1/Apple Silicon, 44.1 kHz host, 256-sample block, single voice:

| Component | CPU |
|-----------|-----|
| 2x Oversampling (polyphase IIR up + down) | ~0.4% |
| 4× DelayLine<Lagrange3rd> push/pop @ 2x | ~0.6% |
| 4× DispersionFilter (avg M=2, 8 allpass total) @ 2x | ~0.3% |
| 4× BridgeFilter @ 2x | ~0.1% |
| Friction junction (table lookup + linear interp) @ 2x | ~0.2% |
| Body resonator (separate research) | ~1.0% |
| Sub-harmonic generator + bow noise | ~0.4% |
| SmoothedValue ramps + APVTS | ~0.2% |
| **Total** | **~3.2%** |

Headroom to add: secondary modal body resonator, FFT-based bow-noise spectrum shaping.

---

## 7. References

### Papers (string dispersion and waveguides)

- Karjalainen, M., Välimäki, V., & Tolonen, T. (1998). "Plucked-string models: From the Karplus-Strong algorithm to digital waveguides and beyond." *Computer Music Journal*, 22(3), 17–32. — Seminal cascaded-allpass dispersion treatment.
- Bensa, J., Bilbao, S., Kronland-Martinet, R., & Smith, J. O. (2003). "The simulation of piano string vibration: from physical models to finite difference schemes and digital waveguides." *J. Acoust. Soc. Am.*, 114(2), 1095–1107. https://hal.science/hal-00088329/
- Rauhala, J., & Välimäki, V. (2006). "Tunable dispersion filter design for piano synthesis." *IEEE Signal Processing Letters*. https://ieeexplore.ieee.org/document/1618690
- Rauhala, J., & Välimäki, V. (2006). "Dispersion modeling in waveguide piano synthesis using tunable allpass filters." *Proc. DAFx-06*, pp. 71–76.
- Abel, J., Välimäki, V., & Smith, J. O. (2010). "Robust, Efficient Design of Allpass Filters for Dispersive String Sound Synthesis." *IEEE Sig. Proc. Letters*. https://ieeexplore.ieee.org/document/5395664
- Rauhala, J. (2008). "Dispersion modulation using allpass filters." *Proc. DAFx-08*. https://dafx.de/paper-archive/2008/papers/dafx08_36.pdf
- Rauhala, J. (2006). "Robust Design of Very High-Order Allpass Dispersion Filters." *Proc. DAFx-06*. https://www.dafx.de/paper-archive/2006/papers/p_013.pdf
- Smith, J. O. *Physical Audio Signal Processing*. CCRMA online textbook. https://ccrma.stanford.edu/~jos/pasp/

### Inharmonicity measurements

- Russell, D. *Inharmonicity due to Stiffness for Guitar Strings*. PSU. https://www.acs.psu.edu/drussell/Demos/Stiffness-Inharmonicity/Stiffness-B.html
- Trujillo, R. et al. (2020). "On inharmonicity in bass guitar strings with application to tapered and lumped constructions." *Discover Applied Sciences*. https://link.springer.com/article/10.1007/s42452-020-2391-2
- Mamou-Mani, A. et al. (2021). "Piano bass strings with reduced inharmonicity: theory and experiments." *Acta Acustica*. https://acta-acustica.edpsciences.org/articles/aacus/full_html/2021/01/aacus200053/aacus200053.html
- Investigating the Inharmonicity of Piano Strings (2024). *Edinburgh Student J. Sci.* https://journals.ed.ac.uk/esjs/article/download/9815/12844/35937
- Phys.UNSW Music Acoustics — *Bowed Strings*. https://www.phys.unsw.edu.au/jw/bows.html (key citation: bow stick-slip enforces harmonic mode-locking)

### JUCE references

- `juce::dsp::DelayLine` — https://docs.juce.com/master/classdsp_1_1DelayLine.html
- `juce::dsp::DelayLineInterpolationTypes::Thiran` — https://docs.juce.com/master/structdsp_1_1DelayLineInterpolationTypes_1_1Thiran.html
- `juce::dsp::DelayLineInterpolationTypes::Lagrange3rd` — https://docs.juce.com/master/structdsp_1_1DelayLineInterpolationTypes_1_1Lagrange3rd.html
- `juce::dsp::Oversampling` — https://docs.juce.com/master/classdsp_1_1Oversampling.html
- `juce::ScopedNoDenormals` — JUCE forum: "State of the Art Denormal Prevention" https://forum.juce.com/t/state-of-the-art-denormal-prevention/16802
- JUCE forum: "Pitch Shifting Delay, Smoothing Artifacts" https://forum.juce.com/t/pitch-shifting-delay-smoothing-artifacts-and-the-doppler-effect/41488
- JUCE forum: "Delay Line smooth" https://forum.juce.com/t/delay-line-smooth/62391
- JUCE tutorial: "Create a string model with delay lines" https://juce.com/tutorials/tutorial_dsp_delay_line/

### Fractional-delay and interpolation theory

- Välimäki, V. (1995). PhD thesis: "Discrete-time modeling of acoustic tubes using fractional delay filters." Aalto. http://users.spa.aalto.fi/vpv/publications/vesan_vaitos/
- Smith, J. O. *Interpolated Delay Lines, Ideal Bandlimited Interpolation, and Fractional Delay Filter Design*. CCRMA. https://ccrma.stanford.edu/~jos/Interpolation/
- Laakso, T. I., Välimäki, V., Karjalainen, M., & Laine, U. K. (1996). "Splitting the Unit Delay." *IEEE Sig. Proc. Magazine*, 13(1), 30–60.

### Cross-references in this codebase

- `research/bow-string-friction-models.md` — friction models, scattering, Newton-Raphson, friction junction theory.
- `research/O-Bowed-research-synthesis.md` — general-purpose bowed string synth architecture.
- `research/O-Bowed-acoustic-instrument-research.md` — acoustic measurements that informed O-Bowed.
- `plugins/O-Contrabass/.planning/BRIEF.md` — feature spec being supported by this research.
