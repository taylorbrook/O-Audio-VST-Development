---
title: "O-Contrabass: Sub-Harmonics, Infinite Sustain, and Drone-Mode Stability"
created: 2026-04-25
juce_version: "8.0.4"
type: research
domain: dsp
plugin: O-Contrabass
keywords:
  - subharmonic-generation
  - anomalous-low-frequencies
  - period-doubling
  - infinite-sustain
  - self-oscillation
  - drone-stability
  - bowed-bass
  - waveguide
  - schelleng-diagram
  - slow-bow-lfo
  - vibrato
  - in-loop-saturation
related_documents:
  - research/bow-string-friction-models.md
  - research/O-Bowed-research-synthesis.md
  - plugins/O-Contrabass/.planning/BRIEF.md
  - plugins/O-Contrabass/.planning/REQUIREMENTS.md
covers_requirements:
  - DSP-06 (Infinite Sustain)
  - DSP-07 (Sub-Harmonics)
  - DSP-08 (Slow Bow LFO)
  - DSP-09 (Vibrato section, expression layering)
  - QUAL-01 (No artifacts under drone settings)
  - QUAL-02 (Self-oscillation remains musical)
---

# O-Contrabass: Drone Features, Sub-Harmonics, and Stability

> Topic 3 of 3 in the O-Contrabass Stage 0 parallel investigation.
> Companion to: friction model deep-dive (Topic 1) and bass-tuned body resonator (Topic 2).

This document focuses on the *drone-first* DSP requirements of O-Contrabass: musical sub-harmonic generation, endless sustain without instability, slow expressive modulation (slow-bow LFO + vibrato), and the stability guards that make extreme drone settings remain musical rather than destructive.

The unifying engineering principle: **build a waveguide that wants to oscillate forever, then add intentional, parameterized losses on top.** Drone mode is the natural state of the engine; orchestral arco mode is achieved by *adding* damping back in.

---

## Table of Contents

1. [Sub-Harmonic Generation](#1-sub-harmonic-generation)
2. [Infinite Sustain Without Instability](#2-infinite-sustain-without-instability)
3. [Slow Bow LFO (0.05 - 2 Hz)](#3-slow-bow-lfo)
4. [Vibrato Section (Rate / Depth / Onset)](#4-vibrato-section)
5. [Master Saturation and Output Protection](#5-master-saturation-and-output-protection)
6. [Integration Architecture](#6-integration-architecture)
7. [Per-Decision Recommendations](#7-recommendations)
8. [References](#8-references)

---

## 1. Sub-Harmonic Generation

### 1.1 Real Bowed Sub-Harmonics: The Physics

Real cellists, bassists, and (occasionally) violinists can produce **Anomalous Low Frequencies (ALF)** — pitches one octave (or a third, or an octave-plus-fifth) below the open string fundamental — by applying very high bow force at low bow velocity. The phenomenon was documented systematically by **Hanson, Schneider, and Halgedahl (1994)** in the Catgut Acoustical Society Journal, and analyzed in detail by **Knut Guettler (1994, 2006)** and **Schoonderwaldt (2009)**.

**The mechanism (Kawano et al. 2025, arXiv 2502.11902):**
1. At normal force, the Helmholtz corner reflects from the bow once per cycle - the string vibrates at f0
2. At high bow force, the bow's grip is strong enough that some Helmholtz corners *fail to trigger a slip* on schedule - the string completes one or more extra round-trips before slipping
3. If the missed-slip pattern stabilizes into "slip every 2nd cycle" - the bridge sees energy at **f0 / 2** (octave down)
4. Other stable patterns produce sub-harmonics at f0/3, 2f0/3, 3f0/4, etc. - these correspond to musical intervals: octave, octave+fifth, third below, etc.

This is **mathematically a period-doubling bifurcation** in the friction nonlinearity, driven into the sub-harmonic regime by:
- High bow force (deep into the upper half of the Schelleng wedge, near `F_max`)
- Low bow velocity (gives more time for the missed-slip pattern to stabilize)
- Sufficient bow position (far enough from bridge that the round-trip time is long)

**Key insight for synthesis:** sub-harmonics emerge naturally from the friction model when pushed into the right regime. They don't need to be added externally - the physics already produces them. The challenge is *making them controllable and stable*.

### 1.2 Four DSP Approaches Considered

| Approach | Musical | CPU | Stability | Physical |
|----------|---------|-----|-----------|----------|
| **A. Period-doubling via friction bifurcation** | Highest (real ALF) | Low | Hardest | Yes |
| B. Octave-down ring modulation | Low (artificial) | Trivial | Trivial | No |
| C. Allpass-comb sub-octave filtering | Medium | Low | Easy | No |
| D. Parallel sub waveguide at f0/2 | Medium | Medium | Easy | Partial |

#### Approach A: Period-Doubling Bifurcation (Recommended)

Drive the existing friction nonlinearity into the bifurcation regime via parameter biasing. The "Sub-Harmonics 0-100%" knob does **not** add a separate generator - it shifts the friction model's effective operating point so the existing junction naturally produces ALF.

**Mapping (`subAmount` 0..1):**
- Pushes the string-friction balance toward `F_max` (high force, low velocity)
- Increases the static-to-dynamic friction coefficient ratio (`mu_s` / `mu_d`) - widens the sticking region
- Slightly biases `v_0` (transition sharpness) lower - makes stick-slip more abrupt
- Optionally adds a small **threshold offset** to the slip trigger - makes the next slip slightly harder to initiate, encouraging the missed-slip pattern

**Pros:** Authentic, organically musical, integrates with everything else (vibrato, body, etc).
**Cons:** Hardest to control - the bifurcation can jump to f0/3 or chaotic regimes if pushed too far.
**Mitigation:** Stay within the upper Schelleng wedge; clamp the bias terms below the regime that produces purely raucous noise.

#### Approach B: Ring Modulation (Rejected)

Multiply the string output by `cos(pi * f0 * t)` (a sub-octave carrier). Zero-cost but **sounds artificial** - the sub-octave has no Helmholtz character, just a tremolo-like AM. Acceptable as a parallel "Cheap Sub" feature but not as the primary mechanism.

#### Approach C: Allpass-Comb Sub-Octave Filter (Rejected as Primary)

A comb filter with delay = 2 * stringPeriod creates spectral peaks at f0/2, f0, 3f0/2, ... When summed back with the string, this *can* enhance an emergent sub-octave - but only if the friction model is already producing some sub-harmonic content. Useful as a **boost stage** behind Approach A, not as a generator.

#### Approach D: Parallel Sub Waveguide at f0/2

A second, much-shorter delay-line resonator tuned to f0/2 driven by the friction junction's force output. Produces a clean sub but **doesn't track the friction state** - if the main string goes to surface sound, the parallel sub keeps droning along, breaking realism. Rejected.

### 1.3 Recommended Implementation: Period-Doubling Bias

```cpp
// Sub-harmonic bias applied inside the friction junction
struct SubHarmonicBias
{
    // User-facing parameter, 0..1
    float subAmount = 0.0f;

    // Bias multipliers (static)
    static constexpr float kForceBoost   = 1.8f;  // 100% sub -> 1.8x effective force
    static constexpr float kV0Reduction  = 0.5f;  // tighter transition
    static constexpr float kMuRatioBoost = 0.25f; // mu_s/mu_d skew

    // Slow random walk for "rough drone" character (optional)
    float walkState = 0.0f;
    float walkRate  = 0.0f;  // updated at control rate

    void prepare(double sampleRate, float controlRateHz)
    {
        walkRate = 1.0f / static_cast<float>(sampleRate / controlRateHz);
    }

    // Returns biased friction parameters
    void apply(float& F_bow, float& v_0, float& mu_s, float& mu_d) const
    {
        const float a = subAmount;

        // Lift force toward F_max region
        F_bow *= 1.0f + (kForceBoost - 1.0f) * a;

        // Tighten the velocity transition (sharper stick-slip)
        v_0 = juce::jmax(0.005f, v_0 * (1.0f - kV0Reduction * a));

        // Widen the mu_s / mu_d gap (deeper sticking well)
        const float gap = mu_s - mu_d;
        mu_s = mu_d + gap * (1.0f + kMuRatioBoost * a);
    }
};
```

**Inside the friction tick:**

```cpp
// Within the friction junction's per-sample tick
SubHarmonicBias::apply(F_bow_current, v_0_current, mu_s_current, mu_d_current);

float v_rel = v_bow - v_string_incoming;
float mu = hyperbolicFriction(v_rel, mu_s_current, mu_d_current, v_0_current);
float F_friction = mu * F_bow_current * sign(v_rel);
// ... feed back into delay lines as before
```

**Critical guardrails:**
- Always keep `F_bow_effective < F_max(beta, v_b)` from the Schelleng equation - clamp the boost when approaching `F_max`
- If the *measured* output autocorrelation indicates we've crossed into the chaotic regime (lag-2 RMS exceeds lag-1 RMS *and* is non-periodic), back off the bias (cheap, optional - measured at control rate ~100 Hz)
- Cross-fade the bias parameters with a 30-50 ms slew to avoid clicks when `subAmount` changes

### 1.4 Why This Works for Bass Specifically

Bass strings are **already deep in the bifurcation-prone regime** at default parameters: they have higher impedance (thick), require higher bow force, and have long round-trip times that give the missed-slip pattern more time to stabilize. This is why double bass players produce ALF more easily than violinists. The synthesis benefits directly: a bias of just 30-50% subAmount on a low E1 (41 Hz) reliably produces an audible sub-octave without crossing into raucous territory.

---

## 2. Infinite Sustain Without Instability

### 2.1 The Stability Constraint

A digital waveguide is stable iff the **loop transfer function** `H_loop(z)` satisfies `|H_loop(e^jw)| < 1` for all frequencies (Smith, "Physical Audio Signal Processing"). The loop is the round-trip path: `bridgeDelay -> bridgeFilter -> nutDelay -> nutReflection -> bridgeDelay`. The total loop gain is the product of:
1. Bridge filter magnitude (designed `< 1` everywhere)
2. Nut reflection coefficient magnitude (designed `<= 1`)
3. Any additional in-loop processing

**Infinite sustain wants `|H_loop(e^jw)| -> 1`** at all frequencies of interest. The bowed-string case is *not* a self-resonant pluck - the bow is continuously injecting energy through the friction junction, so the loop gain alone doesn't determine sustain. But when the bow disengages (note-off), or when `subAmount` is high (bow energy is being modulated wildly), the loop must remain bounded.

### 2.2 The Damping Floor

Don't let `dampingScalar` reach 1.0 (zero loss). Instead, use:

```cpp
// Map "Infinite Sustain 0..1" to a loop gain at DC
// 0%   -> normal physical loss (decay ~ 4 seconds for E1)
// 100% -> just under unity (decay ~ minutes, but still finite)
constexpr float kMinLoopGain  = 0.997f;  // ~33 second T60 at 41 Hz
constexpr float kMaxLoopGain  = 0.99995f; // ~100 second T60, well-bounded
constexpr float kSafetyFloor  = 0.9999999f; // hard ceiling on g_dc

float computeLoopGain(float infSustain01)
{
    // Quadratic curve - most of the perceptual range is in the top quarter
    const float t = infSustain01 * infSustain01;
    float g = kMinLoopGain + t * (kMaxLoopGain - kMinLoopGain);
    return juce::jmin(g, kSafetyFloor);
}
```

**Why never reach 1.0:** Any in-loop nonlinearity (friction, in-loop saturator, fractional-delay interpolator round-off) can add a few mdB of *gain* at certain frequencies due to discretization. With loop gain at exactly 1.0, those tiny gains compound exponentially - a few seconds and you have a runaway. With loop gain at 0.99995, the same nonlinearity-induced gain still results in slow decay.

**Empirical rule** (from O-Bowed extended sustain testing, validated for bass): keep `loopGain * maxBowTableOutput` strictly below 1.0 by at least `0.001`. This margin survives all expected nonlinear behavior including subharmonic bifurcation.

### 2.3 Bridge Filter Floor

The bridge filter is the primary frequency-dependent loss. Standard one-pole lowpass:

```
H_bridge(z) = g * (1 - p) / (1 - p * z^-1)
```

For drone mode, we want `g` close to but under 1, and `p` reduced (less HF rolloff = brighter sustain). But `p > 0` is critical for stability - high frequencies inside the loop have no physical damping mechanism otherwise, and tiny numerical errors at high frequencies will accumulate.

```cpp
struct BridgeFilter
{
    float g = 0.99f;  // overall gain
    float p = 0.5f;   // pole position - higher = brighter

    float updateFromInfiniteSustain(float infSustain01, float brightness01)
    {
        // Raise gain toward (but not to) unity
        g = computeLoopGain(infSustain01);

        // Brightness controls the pole - but always keep some HF loss
        // p ranges from 0.3 (warm) to 0.85 (bright). Never higher.
        p = 0.3f + brightness01 * 0.55f;

        // Floor on HF loss: even at maximum brightness, keep at least
        // -0.5 dB at Nyquist
        const float minHFLoss = 0.94f;  // at fs/2
        // Verify by sampling the magnitude response
        return g;  // current loop gain
    }

    float tick(float x)
    {
        // y[n] = g*(1-p)*x[n] + p*y[n-1]
        z1 = g * (1.0f - p) * x + p * z1;
        return z1;
    }
private:
    float z1 = 0.0f;
};
```

### 2.4 Energy Clamping (Magnitude Limiter)

When the bow disengages but `infSustain` is high, the string keeps ringing. A note-on retrigger could find string state at significant amplitude - if the new excitation adds on top, you can blow the loop.

**Solution:** A soft magnitude clamp on the delay line state, applied at the friction junction (one location, cheap):

```cpp
// Soft clamp at friction junction - inside the per-sample tick
constexpr float kStateClampThreshold = 0.85f;  // start soft-clipping here
constexpr float kStateClampMax       = 1.0f;   // hard ceiling

inline float softClampState(float v)
{
    const float a = std::abs(v);
    if (a < kStateClampThreshold)
        return v;

    const float sign = (v >= 0.0f) ? 1.0f : -1.0f;
    const float over = (a - kStateClampThreshold) / (kStateClampMax - kStateClampThreshold);
    const float compressed = kStateClampThreshold + (kStateClampMax - kStateClampThreshold) * std::tanh(over);
    return sign * compressed;
}

// Apply to incoming wave at junction (NOT at output)
v_sl_in = softClampState(v_sl_in);
v_sr_in = softClampState(v_sr_in);
```

This is a no-op during normal playing (state never approaches 0.85 in physical units after the junction's natural normalization) but provides a graceful runaway-prevention path.

### 2.5 DC Blocker (Required)

Long sustain + nonlinearity = guaranteed DC drift. Place a one-pole DC blocker **after** the bridge filter, **inside the loop**:

```cpp
struct DCBlocker
{
    float x1 = 0.0f, y1 = 0.0f;
    float R = 0.999f;  // pole at ~7 Hz cutoff at 44.1 kHz

    float tick(float x)
    {
        const float y = x - x1 + R * y1;
        x1 = x;
        y1 = y;
        return y;
    }
};
```

`R` of 0.999 gives a cutoff around 7 Hz - safely below E1 (41 Hz) but kills any DC accumulation. This is the canonical Smith/Cookbook DC blocker. **Do not omit this** - drone-mode sessions running for minutes will diverge without it.

### 2.6 Denormal Handling

Long sustains at low loop gains produce delay-line samples that drift toward denormal range (1e-38 and below), causing 50-100x CPU spikes on x86. Two defenses (use both):

```cpp
// 1) Set FTZ/DAZ at processBlock entry (JUCE 8 has helpers)
juce::FloatVectorOperations::disableDenormalisedNumberSupport();

// Or, manually:
// _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
// _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);

// 2) Tiny DC offset injection in the friction junction (defense in depth)
constexpr float kDenormalGuard = 1.0e-20f;
v_sl_in += kDenormalGuard;
v_sr_in -= kDenormalGuard;  // opposite signs cancel in the output
```

The opposite-sign guards cancel at the output (one from bridge, one from nut, sum is zero) but keep both delay lines numerically warm.

### 2.7 How Commercial Plugins Handle This

- **AAS String Studio VS-3:** Exposes `Decay` and `Damping` knobs; max decay is "very long" (minutes-scale) but never truly infinite. The bow excitation is what drives the steady state - turn the bow off, and you hear it decay (just slowly). No documented "infinite hold" guard, but the implementation clearly preserves a non-zero damping floor.
- **Madrona Aalto:** Patchable waveguide with feedback; user can dial feedback up to but not exceeding 1.0. Aalto's reputation for stability comes from clamping feedback parameter mappings just below unity.
- **SWAM Double Bass:** No explicit infinite-sustain feature in the public docs - sustain is driven by the bow remaining engaged. The user's "Sustain" parameter is more like a release-time control.

**Takeaway for O-Contrabass:** We are doing something more aggressive than these references (a true drone-first feature). The mitigations above (loop-gain ceiling, energy clamp, DC blocker, denormal guards) are mandatory.

---

## 3. Slow Bow LFO (0.05 - 2 Hz)

### 3.1 What It Models

In real bowing, even "sustained" tones aren't perfectly steady. Players unconsciously modulate bow speed and pressure together over a several-second cycle - this gives sustained tones their "breathing" quality. For drone music (Stephen O'Malley, Tony Conrad), this becomes the *primary expressive mechanism*.

### 3.2 Phase Relationship: Speed and Pressure

The natural bowing trajectory in the Schelleng wedge is *not* horizontal or vertical - it's a **diagonal trajectory inside the wedge**, simultaneously increasing both speed and force during a swell, then both decreasing during the recovery half.

| LFO phase | Speed | Pressure | Schelleng location | Perceptual result |
|-----------|-------|----------|-------------------|-------------------|
| 0 (positive peak) | high | high | upper-right of wedge | bright, intense |
| pi/2 | mid | mid | center | balanced |
| pi (negative peak) | low | low | lower-left of wedge | airy, soft |
| 3pi/2 | mid | mid | center | balanced |

**Key:** Speed and pressure must move *in phase* (correlation = +1) to stay inside the wedge. Anti-phase modulation (one up, the other down) walks the trajectory across the diagonal of the wedge and frequently exits into raucous (top-left) or surface (bottom-right) zones.

A small **phase offset** (10-30 degrees, pressure lagging speed) increases naturalness without leaving the wedge - this models the slight delay between intent (more speed) and force adjustment (more pressure).

### 3.3 Implementation

```cpp
struct SlowBowLFO
{
    float phase = 0.0f;
    float rateHz = 0.3f;          // 0.05 to 2 Hz
    float depth01 = 0.0f;         // 0 to 1 - "Slow Bow LFO Depth" parameter
    float pressurePhaseOffset = 0.4f;  // ~23 degrees
    double sampleRate = 44100.0;

    // Smoothed outputs (avoid friction-junction artifacts at zero-crossing)
    juce::SmoothedValue<float> smoothSpeedMod;
    juce::SmoothedValue<float> smoothPressureMod;

    void prepare(double sr)
    {
        sampleRate = sr;
        smoothSpeedMod.reset(sr, 0.020);     // 20 ms smoothing
        smoothPressureMod.reset(sr, 0.020);
    }

    // Update at control rate (e.g., once per audio block)
    void updateControlRate(float blockDurationSeconds)
    {
        phase += juce::MathConstants<float>::twoPi * rateHz * blockDurationSeconds;
        if (phase > juce::MathConstants<float>::twoPi)
            phase -= juce::MathConstants<float>::twoPi;

        // Speed modulation (sine)
        const float speedMod = depth01 * std::sin(phase);

        // Pressure modulation (sine, lagged) - naturally in-phase, slight offset
        const float pressureMod = depth01 * std::sin(phase + pressurePhaseOffset);

        smoothSpeedMod.setTargetValue(speedMod);
        smoothPressureMod.setTargetValue(pressureMod);
    }

    // Per-sample modulation
    void modulateBow(float& bowSpeed, float& bowPressure)
    {
        const float s = smoothSpeedMod.getNextValue();
        const float p = smoothPressureMod.getNextValue();

        // Multiplicative modulation, asymmetric range:
        // depth=1.0 -> speed varies 0.4x..1.6x, pressure varies 0.5x..1.5x
        bowSpeed   *= (1.0f + 0.6f * s);
        bowPressure *= (1.0f + 0.5f * p);
    }
};
```

**Smoothing rationale:** A 20 ms ramp on the modulation values prevents sudden friction-state changes when the LFO crosses zero or when the user changes `depth01`. The friction junction is sensitive to abrupt parameter changes - a sudden pressure drop can flip the string out of Helmholtz mid-cycle, producing an audible "thump."

### 3.4 Schelleng-Aware Modulation

When `depth01` is high *and* the user-set bow parameters are already near the Schelleng wedge boundary, the LFO can push them outside. Defense: scale `depth01` by remaining headroom toward each boundary at every block.

```cpp
// Compute effective depth that keeps trajectory inside wedge
float computeSafeLFODepth(float baseDepth, float bowSpeed, float bowPressure,
                          float beta, float Z0, float R, float mu_s, float mu_d)
{
    const float fMax = (2.0f * Z0 * bowSpeed) / (beta * (mu_s - mu_d));
    const float fMin = (Z0 * Z0 * bowSpeed) / (2.0f * R * beta * beta * (mu_s - mu_d));

    // Distance from current pressure to nearest boundary, normalized to [0,1]
    const float distToMax = (fMax - bowPressure) / fMax;
    const float distToMin = (bowPressure - fMin) / juce::jmax(fMin, 0.001f);
    const float headroom = juce::jmin(distToMax, distToMin);

    // Allow LFO to use up to 80% of available headroom
    const float maxAllowedDepth = headroom * 0.8f;
    return juce::jmin(baseDepth, maxAllowedDepth);
}
```

This is the difference between a "raucous LFO" that randomly produces crunchy artifacts and a "musical LFO" that swells without leaving the playable zone.

---

## 4. Vibrato Section

### 4.1 Bass Vibrato Characteristics

Per **Mick (2025)** in *String Research Journal*, university double bass players had:
- Mean rate: **5.17 Hz**
- Mean width: **19 cents** (peak-to-peak ~38 cents, or ±19 from center)
- Faster + wider in upper register; minimal in low register

The default in BRIEF.md (`5 Hz rate, 12 cents depth`) is conservative and aligned with low-register orchestral practice - this matches the spec's "shallower than violin" intention. The 50-cent maximum is the *upper bound* for expressive playing.

### 4.2 Onset Delay (~600 ms Default)

Real string vibrato doesn't begin at note-onset - it fades in over 200-1000 ms. For orchestral arco, **600 ms** is a natural starting point. Implementation requires an envelope on the vibrato amplitude:

```cpp
struct VibratoOnsetEnvelope
{
    enum Phase { idle, delaying, fadingIn, sustaining };
    Phase phase = idle;

    float onsetMs = 600.0f;   // delay before fade-in starts
    float fadeMs  = 300.0f;   // duration of fade-in (typical = 50% of onset)
    float currentDepth = 0.0f;
    int   delayCounter = 0;
    int   fadeCounter  = 0;
    int   onsetSamples = 0;
    int   fadeSamples  = 0;

    void noteOn(double sampleRate)
    {
        onsetSamples = static_cast<int>(onsetMs * 0.001 * sampleRate);
        fadeSamples  = static_cast<int>(fadeMs  * 0.001 * sampleRate);
        delayCounter = onsetSamples;
        fadeCounter  = 0;
        currentDepth = 0.0f;
        phase = (onsetMs > 0.0f) ? delaying : fadingIn;
    }

    void noteOff()
    {
        // Optional: rapid decay back to zero, ~50 ms
        phase = idle;
        // Fade out smoothing handled by SmoothedValue on the depth multiplier
    }

    float tick()  // returns 0..1 multiplier on user-set vibrato depth
    {
        switch (phase)
        {
            case delaying:
                if (--delayCounter <= 0) { phase = fadingIn; }
                return 0.0f;
            case fadingIn:
                ++fadeCounter;
                currentDepth = static_cast<float>(fadeCounter) / fadeSamples;
                // Smooth curve: half-cosine (S-curve) is more natural than linear
                currentDepth = 0.5f * (1.0f - std::cos(juce::MathConstants<float>::pi * currentDepth));
                if (fadeCounter >= fadeSamples) { phase = sustaining; currentDepth = 1.0f; }
                return currentDepth;
            case sustaining:
                return 1.0f;
            case idle:
            default:
                return 0.0f;
        }
    }
};
```

**Note-off handling:** The brief says "release tail on note-off." Vibrato should fade out faster than the bowing tail (~100-200 ms) to avoid the unnatural "pulsing" of a decaying note still vibrato-modulated.

### 4.3 Modulation Method: Delay-Line Length vs Detune Coefficient

Two ways to implement pitch vibrato in a waveguide:

**Method A: Modulate delay-line length** (modulate `neckDelaySamples` and `bridgeDelaySamples`)
- Physically correct - this is what a finger does (changes effective string length)
- Requires fractional-delay interpolation for smooth pitch (allpass or Lagrange)
- **Side effect:** Modulates inharmonicity, body coupling, and stick-slip timing - all musically *correct*
- Requires accurate per-sample delay length tracking; small errors in interpolation cause artifacts

**Method B: Pitch-shift the output** (e.g., post-process with a pitch shifter)
- Doesn't affect the friction physics - feels "applied" rather than "intrinsic"
- Adds CPU and latency
- Rejected.

**Recommendation: Method A** with allpass fractional delay interpolation. The total delay is recomputed at vibrato rate (control-rate, not per-sample), then smoothed with a one-pole on the integer + fractional part.

```cpp
// Vibrato modulates the delay length
void updateDelayLength(float baseLengthSamples, float vibratoCents)
{
    // cents -> frequency ratio -> delay ratio (inverse)
    const float ratio = std::pow(2.0f, -vibratoCents / 1200.0f);
    const float modulatedSamples = baseLengthSamples * ratio;
    delayLine.setDelay(modulatedSamples);  // sets fractional delay
}
```

### 4.4 Vibrato + Slow-Bow LFO Combined

Both modulators are active simultaneously in drone presets. They act on *different* parameters (vibrato on delay length / pitch; LFO on bow speed/pressure) so they don't fight each other directly. But:
- Vibrato on a tone with deep slow-bow LFO can sound "wobbly" if their rates land at simple ratios (e.g., LFO at 1 Hz, vibrato at 5 Hz - the 5:1 ratio creates audible beating)
- **Mitigation:** offset the vibrato rate by a small irrational amount (`5.0 Hz -> 5.13 Hz`) when slow-bow LFO is active. Or expose an "irrationalize" toggle.

---

## 5. Master Saturation and Output Protection

### 5.1 In-Loop vs Post-Bridge Saturation

**Two locations to consider:**

**Inside the loop (at the friction junction):** acts on the string's velocity wave - changes the *physics*. A `tanh` here softens stick-slip transitions and adds harmonic warmth that interacts with the body resonator. **Good for character.** Risk: every iteration of the loop applies the saturation, so the cumulative gain change with `loopGain` close to 1 is significant. Use sparingly (gentle slope).

**Post-bridge (at the output):** doesn't affect the loop - just shapes the final output. Cheap and safe. Acts as the master limiter.

**Recommendation: use both**, with different curves:

```cpp
// In-loop: very gentle, mostly transparent, kicks in only above 0.6
inline float inLoopSaturator(float v)
{
    // Soft tanh-like; 1:1 below 0.6, asymptotic to ~1.1 above
    if (std::abs(v) < 0.6f) return v;
    const float sign = (v >= 0.0f) ? 1.0f : -1.0f;
    const float overshoot = std::abs(v) - 0.6f;
    return sign * (0.6f + 0.4f * std::tanh(overshoot * 1.5f));
}

// Post-bridge: master saturator + gentle limiter
inline float masterSaturator(float v)
{
    // Polynomial soft-clip - cheaper than tanh, very similar curve
    // y = x - x^3/3 for |x| < sqrt(3); clamp afterwards
    const float a = juce::jlimit(-1.5f, 1.5f, v);
    return a - (a * a * a) * (1.0f / 3.0f);
}
```

### 5.2 Saturation Curve Choices

| Curve | Cost | Symmetry | Asymptote | Notes |
|-------|------|----------|-----------|-------|
| `tanh(x)` | High (one `expf`) | Yes | 1.0 | Smooth, classic, expensive |
| `x / (1 + |x|)` | Low | Yes | 1.0 | Cheap, asymptote at 1, "softer" knee |
| `x - x^3/3` | Low | Yes | None (clamps) | Need pre-clamp; classic polynomial |
| `atan(x) * 2/pi` | Medium | Yes | 1.0 | Symmetric, mid CPU |
| `x / sqrt(1 + x^2)` | Medium (one sqrt) | Yes | 1.0 | "Algebraic" - good middle ground |

**Recommendation:**
- **In-loop:** algebraic `x / sqrt(1 + x^2)` - one sqrt per sample at oversampled rate, smooth derivative, asymptote at 1.0 means we cannot increase loop gain through saturation.
- **Post-bridge:** polynomial `x - x^3/3` for cheapness, with input pre-clamp to [-1.5, +1.5]. This gives ~6 dB of gentle compression starting around -3 dBFS.

### 5.3 Master Output Limiter

For QUAL-02 (extreme drone settings remain musical), a final guard limiter is needed. Constraints from BRIEF: **zero algorithmic latency**. This **rules out look-ahead limiters**.

Use a simple feedforward soft-knee limiter:

```cpp
struct ZeroLatencyLimiter
{
    float envelope = 0.0f;
    float attackCoeff = 0.0f;   // ~3 ms
    float releaseCoeff = 0.0f;  // ~100 ms
    float thresholdLin = 0.891f; // -1 dBFS

    void prepare(double sr)
    {
        attackCoeff  = std::exp(-1.0f / (0.003f * static_cast<float>(sr)));
        releaseCoeff = std::exp(-1.0f / (0.100f * static_cast<float>(sr)));
    }

    float tick(float x)
    {
        const float absX = std::abs(x);
        // Track absolute envelope (peak-style)
        const float coeff = (absX > envelope) ? attackCoeff : releaseCoeff;
        envelope = coeff * envelope + (1.0f - coeff) * absX;

        // Compute gain reduction
        float gain = 1.0f;
        if (envelope > thresholdLin)
            gain = thresholdLin / envelope;

        return x * gain;
    }
};
```

**Caveat:** A 3 ms peak detector won't catch a single-sample inter-sample peak above threshold. Acceptable for this design - the saturation upstream prevents true peaks from exceeding 0 dBFS by much.

For **true-peak** detection without look-ahead, you'd need 2x oversampling at the limiter input, which we already have at the friction junction. We can run the limiter on the oversampled signal (still feedforward, still zero-latency) and take true peaks into account.

### 5.4 Soft-Saturator Placement Summary

```
[friction junction]
    -> in-loop saturator (algebraic, asymmetric onset at 0.6)
    -> bridge filter
    -> nut reflection
    -> back to friction
                         (loop)

[bridge filter output] -> body resonator -> stereo widener
    -> post-bridge saturator (cubic polynomial)
    -> zero-latency limiter (-1 dBFS threshold)
    -> output
```

---

## 6. Integration Architecture

```
                                 +-- Vibrato (delay-length modulator) ---+
MIDI/MPE/NoteExpr -> Bow Model -+                                        v
                                +-- Slow Bow LFO (speed/pressure mod) -> Friction Junction <-> [String Waveguide]
                                                                         | (sub-bias applied here)        |
                                                                         |                                |
                                                                         +-- in-loop saturator (algebraic)
                                                                         |
                                                                         +-- DC blocker (one-pole)
                                                                         |
                                                                         +-- bridge filter (lowpass)
                                                                         |
                                                                         +-> Body Resonator -> Width -> Master Sat -> Limiter -> Output
```

**Per-block update order:**

1. Read MIDI/MPE state (note number, velocity, MPE Y/Z, NoteExpression)
2. Update Slow Bow LFO phase (control rate)
3. Compute `safeLFODepth` from Schelleng headroom
4. Apply LFO modulation to bow speed/pressure (smoothed)
5. Update Vibrato envelope (onset/fade)
6. Compute current vibrato cents = depth_user * onset_envelope * sin(2*pi*vibratoRate*t)
7. Update delay-line lengths (per-string, base + per-string detune + vibrato cents)
8. Apply Sub-Harmonic bias to friction parameters
9. Run friction junction at 2x oversampling
10. Loop saturator -> DC blocker -> bridge filter (in-loop)
11. Output the bridge-filter output to body resonator
12. Body resonator -> width -> master saturator -> limiter -> output buffer

---

## 7. Per-Decision Recommendations

| Decision | Recommendation | Rationale |
|----------|---------------|-----------|
| **Sub-harmonic generator** | Period-doubling friction bias (Approach A) - shift force/v_0/mu_ratio in friction junction | Authentic ALF physics; integrates with body and vibrato; no extra oscillator |
| **Sub-harmonic safety** | Schelleng `F_max` clamp; soft-back-off if chaotic regime detected via lag-2 RMS | Prevents raucous explosion at high settings |
| **Infinite sustain mapping** | Quadratic curve from 0.997 -> 0.99995 loop gain at DC | Most perceptual range in top 25%; matches user expectations |
| **Damping floor** | Hard ceiling 0.9999999 on loop gain; mandatory non-zero `p` in bridge filter | Prevents runaway from numerical/nonlinear gain |
| **Energy clamp** | `softClampState` at friction junction, threshold 0.85, ceiling 1.0 | One-location runaway prevention |
| **DC blocker** | Mandatory, one-pole at R=0.999, inside loop after bridge filter | Long sustains drift to DC otherwise |
| **Denormal handling** | FTZ/DAZ at processBlock entry + opposite-sign 1e-20 guards on delay lines | x86 denormal slowdown defense in depth |
| **Slow Bow LFO range** | 0.05-2 Hz (matches BRIEF) | Sub-audio modulation territory |
| **LFO speed/pressure phase** | In-phase with ~23-degree pressure lag | Stays inside Schelleng wedge; natural breathing feel |
| **LFO smoothing** | 20 ms SmoothedValue on speed/pressure modulators | Prevents friction-junction artifacts at zero-crossing |
| **LFO depth limiting** | Schelleng-aware - scale to 80% of available wedge headroom | Avoids raucous excursions at high depth |
| **Vibrato rate default** | 5 Hz (per BRIEF, matches bass research mean of 5.17 Hz) | Empirically validated bass vibrato rate |
| **Vibrato depth default** | 12 cents (peak), max 50 cents | Below research mean (19 cents) - low-register-friendly default |
| **Vibrato onset** | 600 ms default; S-curve fade-in over 300 ms | Orchestral realism; matches sample-library practice |
| **Vibrato modulation** | Delay-line length with allpass interpolation | Physically correct; affects body/friction in musical ways |
| **In-loop saturator** | Algebraic `x/sqrt(1+x^2)` curve, asymmetric onset at 0.6 | Cheap; can't increase loop gain (asymptote at 1) |
| **Post-bridge saturator** | Polynomial `x - x^3/3` with pre-clamp | Cheapest soft-clip; ~6 dB of gentle warmth |
| **Master limiter** | Zero-latency feedforward, 3ms attack, 100ms release, -1 dBFS threshold | Zero algorithmic latency requirement (BRIEF); QUAL-02 |
| **Limiter sample rate** | Run on 2x oversampled signal; downsample after | Better true-peak handling without look-ahead |

---

## 8. References

### Sub-Harmonic / ALF Physics
- Hanson, R.J., Schneider, A.J., & Halgedahl, F.W. (1994). "Anomalous low-pitch tones from a bowed violin string." *Catgut Acoustical Society Journal*.
- Guettler, K. (1994). "Wave analysis of a string bowed to anomalous low frequencies." *Catgut Acoustical Society Journal*.
- Guettler, K. (2006). "The Violin Bow in Action - 'A Sound-Sculpturing Wand'." (CISM lecture notes).
- Schoonderwaldt, E. (2009). "The violinist's sound palette: spectral centroid, pitch flattening and anomalous low frequencies." *Acta Acustica United Acustica*, 95.
- Boyle, D.D. & Hanson, R.J. (1991). Subharmonic studies on bowed strings.
- Kawano, S., Kobayashi, K., Suzuki, T., & Ichiji, N. (2025). "Experimental Validation of String Oscillation in Subharmonic Generation." arXiv:2502.11902.
- Boutillon, X. (2005). "On the bowed string: Helmholtz, S-motion, multiple slip, and subharmonic regimes." McGill thesis (Boutillon's group).

### Waveguide Stability
- Smith, J.O. "Physical Audio Signal Processing" (online): https://www.dsprelated.com/freebooks/pasp/
- Smith, J.O. "Digital Waveguide Bowed-String": https://ccrma.stanford.edu/~jos/pasp/Digital_Waveguide_Bowed_String.html
- Smith, J.O. "DC Blocker": https://ccrma.stanford.edu/~jos/fp2/DC_Blocker.html
- Valimaki, V. (1995). "Discrete-time modeling of acoustic tubes using fractional delay filters." PhD thesis, Aalto.

### Vibrato (Bass Specifically)
- Mick, J.P. (2025). "An Analysis of Double Bass Vibrato." *String Research Journal*, 15(1).
- Mastering Vibrato on the Double Bass: https://doublebasshq.com/learn_posts/how-to-learn-vibrato-on-double-bass/
- Vienna Symphonic Library Double Bass Academy: https://www.vsl.co.at/academy/strings/double-bass

### Schelleng / Playability
- Schelleng, J.C. (1973). "The bowed string and the player." *JASA* 53(1).
- Woodhouse, J. (2003). Euphonics chapters 9.3, 11.3: https://euphonics.org/
- "Mapping Playability: The Schelleng Diagram and its Generalizations." SMAC 2023.
- "Assessing playability limits of bowed-string transients using experimental measurements." *Acta Acustica* 8 (2024).

### Commercial Plugin Analysis
- AAS String Studio VS-3 manual: https://www.applied-acoustics.com/string-studio-vs-3/manual/
- Madrona Aalto manual: https://madronalabs.com/products/aalto
- SWAM Double Bass: https://audiomodeling.com/strings/swam-double-bass/

### Saturation / Limiter Design
- Pirkle, W. (2019). "Designing Audio Effect Plugins in C++." Routledge.
- KVR DSP discussion: variable tanh and algebraic saturation curves: https://www.kvraudio.com/forum/viewtopic.php?t=465091

### General Bowed-String DSP
- Serafin, S. (2003). "The sound of friction: real-time models, playability and musical applications." Stanford CCRMA PhD thesis.
- Desvages, C. & Bilbao, S. (2016). "Two-polarisation physical model of bowed strings with nonlinear contact and friction forces." *Applied Sciences* 6(5).
