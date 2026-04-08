# Stage 2 DSP -- Phase 3.4 Research: Advanced Friction + Impossible Physics

**Plugin:** O-Bowed
**Phase:** 3.4 (Enhanced Friction Tiers + Reversed Friction + Sub-Harmonics)
**Date:** 2026-04-05
**Input:** CONTEXT-3.4.md, existing friction research, Serafin/Dupont/Woodhouse literature
**Confidence:** HIGH

---

## 1. ElastoPlasticFriction (Enhanced Tier)

### Model Selection

**Recommendation:** Serafin/Avanzini 2003 adaptation of Dupont 2002 single-state elasto-plastic model. This is the standard for real-time bowed string synthesis with the best quality/CPU tradeoff.

### Core ODE

```
State variable: z (average bristle displacement, meters)

Bristle displacement ODE:
    dz/dt = v_rel * alpha(z, v_rel)

    where alpha(z, v_rel) transitions from 0 (pure stick) to 1 (pure slip):

    alpha = 0.0                                          if |z| < z_ba
    alpha = 0.5 * (1 + sin(pi*(|z| - z_m) / (z_ss - z_ba)))  if z_ba <= |z| < z_ss
    alpha = 1.0                                          if |z| >= z_ss

    z_ba = mu_s * F_n / sigma_0      (breakaway displacement)
    z_ss = mu_d * F_n / sigma_0      (steady-state displacement)
    z_m  = 0.5 * (z_ba + z_ss)       (midpoint)

Friction force:
    F = sigma_0 * z + sigma_1_eff * dz/dt + sigma_2 * v_rel

Passivity fix (Frontiers 2025):
    sigma_1_eff = sigma_1 * (1.0 + v_rel * v_rel)
```

### Why Serafin's Sinusoidal Alpha Over Dupont's Piecewise

Serafin noted "significant deviations from these curves can be tolerated without affecting model behavior significantly." The sinusoidal transition:
- Avoids derivative discontinuities at z_ba and z_ss boundaries
- Better behaved under Newton-Raphson iteration (smooth derivative)
- Equivalent perceptual result for audio synthesis

### Parameter Values (from Serafin/Avanzini SMAC 2003, Willemsen/Bilbao DAFx 2019)

| Parameter | Symbol | Value | Unit | Notes |
|-----------|--------|-------|------|-------|
| Bristle stiffness | sigma_0 | 6000 | N/m | From Serafin; controls stick-slip sharpness |
| Bristle damping | sigma_1 | 0.5 | Ns/m | With passivity fix applied |
| Viscous friction | sigma_2 | 0.002 | Ns/m | Minor velocity-proportional term |
| Static friction | mu_s | 0.8 | - | Same as core tier |
| Dynamic friction | mu_d | 0.3 | - | Same as core tier |

**z_ba and z_ss depend on F_n (bow force):**
- At F_n = 1.0 N: z_ba = 0.8/6000 = 0.000133 m, z_ss = 0.3/6000 = 0.00005 m
- Note: z_ba > z_ss since mu_s > mu_d. The transition zone is z_ss to z_ba.
- Correction: z_ba (breakaway, larger friction) should be > z_ss (steady-state, smaller friction)

**Wait -- z_ba should be LARGER than z_ss.** Since mu_s > mu_d:
- z_ba = mu_s * F_n / sigma_0 = 0.000133 m (stiction limit)
- z_ss = mu_d * F_n / sigma_0 = 0.000050 m (sliding steady-state)

The alpha function in Serafin transitions from stick (|z| small) through elasto-plastic to slip (|z| large). The correct ordering:
- |z| < z_ss: pure elastic stick (alpha = 0, bristles store energy)
- z_ss <= |z| < z_ba: transition zone (alpha ramps 0 -> 1)
- |z| >= z_ba: breakaway / pure slip (alpha = 1)

### Numerical Integration

**Method:** Forward Euler with clamping. Serafin used bilinear transform (trapezoidal rule) requiring NR iteration for the integration itself. For our tiered approach, forward Euler is simpler and sufficient at 2x oversampled rates (88.2 kHz effective).

```cpp
// Forward Euler with clamp (per oversampled sample)
float dt = 1.0f / (sampleRate * 2.0f);  // 2x oversampled

float dz = v_rel * alpha;
z += dz * dt;

// Clamp bristle displacement to prevent runaway
z = std::clamp(z, -z_ba * 1.5f, z_ba * 1.5f);
```

**Why not bilinear transform:** The trapezoidal rule is implicit (requires NR to solve the integration step itself, adding ~7 iterations inside the existing NR friction solver). Forward Euler is explicit and sufficient when oversampled. The 2x oversampling in Phase 3.5 provides the stability margin.

**Denormal protection:** Clamp z to zero when |z| < 1e-15f. Bristle state can accumulate tiny values after bow release.

### Reflection Coefficient Computation

The elasto-plastic model computes friction FORCE, not reflection coefficient directly. Conversion:

```cpp
float computeReflectionCoefficient(float v_delta, float F_bow, float dt)
{
    float v_rel = v_delta;  // bow velocity minus string velocity

    // Update bristle state
    float z_ba_val = mu_s * F_bow / sigma_0;
    float z_ss_val = mu_d * F_bow / sigma_0;
    float alpha = computeAlpha(z, z_ss_val, z_ba_val);

    float dz = v_rel * alpha;
    z += dz * dt;
    z = std::clamp(z, -z_ba_val * 1.5f, z_ba_val * 1.5f);

    // Friction force with passivity fix
    float sigma_1_eff = sigma_1 * (1.0f + v_rel * v_rel);
    float F_friction = sigma_0 * z + sigma_1_eff * dz + sigma_2 * v_rel;

    // Convert force to waveguide reflection coefficient
    // rho = F_friction / (4 * R_s * v_delta) approximately
    // Use same r / (1+r) mapping as core:
    float r = std::abs(F_friction) / (4.0f * R_s);
    float rho = r / (1.0f + r);

    return rho;
}
```

**Key difference from core tier:** This function is NOT const -- it modifies bristle state z. The voice must own the friction model, and each voice has its own instance.

### CPU Cost Estimate

Per sample: 1 alpha computation (branch + sin or branch-free polynomial), 1 Euler step, 1 clamp, 3 multiplies for F_friction, 1 division for rho. Approximately **3x core tier** as estimated in ROADMAP.

---

## 2. ThermalFriction (Quality Tier)

### Model Selection

**Recommendation:** Simplified Woodhouse 2003 thermal model. The full Smith/Woodhouse tribology model requires contact mechanics parameters that are hard to tune. Use a simplified version where temperature directly modulates the friction coefficient via an exponential decay above the glass transition.

### Core Equations

```
State variables: z (bristle displacement, from elasto-plastic), T_contact (temperature, Celsius)

Temperature ODE:
    dT/dt = (Q_gen - Q_loss) / C_thermal

    Q_gen  = |F_friction * v_rel|     (frictional heating power)
    Q_loss = k_cool * (T_contact - T_ambient)  (Newton cooling)

Friction coefficient modulation:
    mu_effective(T) = mu_base                                    if T <= T_glass
    mu_effective(T) = mu_base * exp(-alpha_T * (T - T_glass))   if T > T_glass

    where mu_base is the value from the underlying elasto-plastic model
```

### Thermal Parameters

| Parameter | Symbol | Value | Unit | Notes |
|-----------|--------|-------|------|-------|
| Glass transition temp | T_glass | 49.0 | C | Standard violin rosin (calorimetry measured) |
| Ambient temp | T_ambient | 20.0 | C | Room temperature |
| Thermal decay rate | alpha_T | 0.15 | 1/C | Controls how fast mu drops above T_glass |
| Thermal capacitance | C_thermal | 0.002 | J/C | Controls temperature inertia (smaller = faster heating) |
| Cooling coefficient | k_cool | 0.5 | W/C | Controls return to ambient rate |
| Max temperature clamp | T_max | 150.0 | C | Prevent numerical runaway |

### Thermal Time Constants

From Woodhouse 2003 simulations:
- Mean contact temperature during Helmholtz motion: ~70 C (well above T_glass)
- Temperature oscillates ~30 C per stick-slip cycle
- **Heating time constant:** tau_heat = C_thermal / Q_gen_typical. At F=1N, v=0.3m/s: Q_gen ~ 0.3W. tau_heat = 0.002/0.3 = ~6.7ms (~3 vibration cycles at 440Hz)
- **Cooling time constant:** tau_cool = C_thermal / k_cool = 0.002/0.5 = 4ms

These fast time constants ensure temperature tracks the stick-slip cycle, producing the characteristic thermal hysteresis loops that distinguish the quality tier from enhanced.

### Why Quality Extends Enhanced (Not Replaces)

The quality tier IS the elasto-plastic model PLUS thermal modulation. ThermalFriction inherits/composes ElastoPlasticFriction:

```
ThermalFriction {
    ElastoPlasticFriction elastoPlastic;  // bristle state z
    float T_contact = T_ambient;           // temperature state

    computeReflectionCoefficient(v_delta, F_bow, dt) {
        // 1. Get base friction from elasto-plastic
        float rho_base = elastoPlastic.computeReflectionCoefficient(v_delta, F_bow, dt);

        // 2. Compute thermal modulation
        float v_rel = v_delta;
        float F_approx = rho_base * 4.0f * R_s * std::abs(v_delta);  // approximate friction force
        float Q_gen = std::abs(F_approx * v_rel);
        float Q_loss = k_cool * (T_contact - T_ambient);
        T_contact += (Q_gen - Q_loss) / C_thermal * dt;
        T_contact = std::clamp(T_contact, T_ambient, T_max);

        // 3. Scale reflection coefficient by thermal factor
        float thermalScale = 1.0f;
        if (T_contact > T_glass)
            thermalScale = std::exp(-alpha_T * (T_contact - T_glass));

        return rho_base * thermalScale;
    }
}
```

### Perceptual Effect

- **Below T_glass (~49 C):** Temperature too low to matter -- identical to enhanced tier
- **Near T_glass:** Transition zone -- subtle tone evolution as rosin softens
- **Above T_glass:** Friction drops, reducing bow grip. During sustained notes, temperature rises and the tone evolves: initial "bite" (cold rosin, high friction) mellows into warmer sustain (hot rosin, lower friction)
- **After bow release:** Temperature cools exponentially back to ambient over ~4ms. Quick recovery for next note.

This sustained-tone evolution is the key musical difference. Enhanced tier gives better attacks; quality tier gives better sustained tone evolution.

### CPU Cost Estimate

Per sample: all of enhanced tier PLUS 1 multiply (Q_gen), 1 subtract + multiply (Q_loss), 1 add + clamp (T update), 1 exp (thermal scale). The exp() is the expensive part. Approximately **5x core tier** as estimated in ROADMAP.

**Optimization:** Precompute exp() LUT for the T range [T_glass, T_max] at prepare time. 256-entry table with linear interpolation reduces exp() to 1 multiply + 1 add.

---

## 3. Newton-Raphson Solver Integration

### When NR Is Needed

| Tier | NR Required? | Reason |
|------|-------------|--------|
| Core (Hyperbolic) | No | Memoryless, uses incoming v_delta directly |
| Enhanced (ElastoPlastic) | Partial | State variable z creates mild implicit coupling, but forward Euler avoids NR for the ODE integration. NR only needed for the waveguide junction equation. |
| Quality (Thermal) | Partial | Same as enhanced -- temperature adds another state but doesn't make the junction equation harder. |

**Key insight from literature:** The elasto-plastic model **naturally resolves the Friedlander ambiguity** because the bristle state z provides memory. There is a unique solution at each time step given the current state. This is a major advantage over memoryless curves.

### Simplified NR for Enhanced/Quality Tiers

Because we use forward Euler for the bristle ODE (not implicit bilinear transform), the NR iteration only needs to solve the scattering junction equation, not the ODE integration. This reduces iteration count from Serafin's ~7 to typically **3-4 iterations**.

```cpp
// NR solver for enhanced/quality friction junction
// Finds v_string that satisfies: F_friction(v_bow - v_string) = R_s * (v_string - v_incoming)
float solveJunction(float v_bow, float v_incoming, float F_bow, float dt)
{
    float v = v_incoming;  // Initial guess: incoming string velocity
    float prev_v = v;

    for (int i = 0; i < 6; ++i)
    {
        float v_rel = v_bow - v;

        // Compute friction force (updates bristle state z on first iteration only)
        float F_fric = computeFrictionForce(v_rel, F_bow, dt, i == 0);

        // Residual: g(v) = F_fric - R_s * (v - v_incoming)
        float g = F_fric - R_s * (v - v_incoming);

        // Derivative: dg/dv = -dF/dv_rel - R_s
        float dF_dv = computeFrictionDerivative(v_rel, F_bow);
        float dg = -dF_dv - R_s;

        if (std::abs(dg) < 1e-10f) break;  // Avoid division by zero

        float delta = g / dg;
        v -= delta;

        if (std::abs(delta) < 1e-6f) break;  // Converged
    }

    // Bailout: if NR diverged, use previous sample's solution
    if (std::isnan(v) || std::isinf(v) || std::abs(v) > 10.0f)
        v = prev_v;

    return v;
}
```

### Initial Guess Strategy

**Use incoming string velocity v_incoming** (the sum of waves arriving at the bow point). This is the same value the core tier uses directly. It's a good predictor because at audio sample rates the solution changes incrementally between samples.

**Alternative:** Cache previous sample's solution as initial guess. Slightly better convergence (1 fewer iteration on average) but adds a state variable. Not worth the complexity for Phase 3.4.

### Friction Derivative for NR

The NR solver needs dF/dv_rel. For the elasto-plastic model:

```cpp
float computeFrictionDerivative(float v_rel, float F_bow)
{
    float z_ba_val = mu_s * F_bow / sigma_0;
    float z_ss_val = mu_d * F_bow / sigma_0;
    float alpha_val = computeAlpha(z, z_ss_val, z_ba_val);

    // dF/dv_rel = sigma_0 * dz/dv_rel + sigma_1_eff * d(dz/dt)/dv_rel + sigma_2
    // dz/dv_rel = alpha * dt (from forward Euler: z += v_rel * alpha * dt)
    // d(dz/dt)/dv_rel = alpha (since dz/dt = v_rel * alpha)
    float sigma_1_eff = sigma_1 * (1.0f + v_rel * v_rel);
    return sigma_0 * alpha_val * dt + sigma_1_eff * alpha_val + sigma_2;
}
```

### NR State Update Timing

**Critical:** Only update bristle state z on the FINAL NR iteration (or first iteration if using forward Euler pre-update). If z is updated on every NR iteration, the state accumulates error from rejected iterations.

**Recommended pattern:** Compute z tentatively during NR, only commit the final z value after convergence:

```cpp
float z_tentative = z;
// ... NR loop using z_tentative ...
z = z_tentative;  // Commit after convergence
```

---

## 4. WaveguideString Interface Refactor

### Current Interface

```cpp
float processSample(float v_bow, float F_bow, const HyperbolicFriction& friction);
```

This doesn't work for enhanced/quality tiers because:
1. Friction models are NOT const (they update state z, T)
2. The voice needs to dispatch between three friction models
3. NR solver needs access to v_string_incoming

### Recommended Refactor: Split Into Read/Compute/Write

```cpp
// WaveguideString.h -- new interface
struct JunctionState {
    float bridgeReflection;
    float nutReflection;
    float v_string_incoming;
};

JunctionState readJunction(float v_bow);  // steps 2-4: read delays, compute v_delta
float writeJunction(float rho, float v_delta, const JunctionState& state);  // steps 6-8

// Keep original for DroneStringEngine (core tier only, no interface change needed)
float processSample(float v_bow, float F_bow, const HyperbolicFriction& friction);
```

### Voice Per-Sample Loop (Phase 3.4)

```cpp
while (--numSamples >= 0)
{
    bowModel.updateEnvelope();
    float v_bow = bowModel.getBowVelocity();
    float F_bow = bowModel.getBowForce();

    auto jState = waveguideString.readJunction(v_bow);
    float v_delta = v_bow - jState.v_string_incoming;

    float rho;
    switch (currentTier)
    {
        case 0:
            rho = coreFriction.computeReflectionCoefficient(v_delta, F_bow);
            break;
        case 1:
            rho = enhancedFriction.computeReflectionCoefficient(v_delta, F_bow, dt);
            break;
        case 2:
            rho = qualityFriction.computeReflectionCoefficient(v_delta, F_bow, dt);
            break;
    }

    // Reversed friction transform (wraps any tier)
    if (reversedAmount > 0.001f)
        rho = rho + reversedAmount * (1.0f - 2.0f * rho);
        // = lerp(rho, 1.0 - rho, reversedAmount)

    float sample = waveguideString.writeJunction(rho, v_delta, jState);

    // Sub-harmonics (per-voice, inside waveguide feedback conceptually)
    if (subHarmonicsAmount > 0.001f)
        sample = applySubHarmonics(sample, subHarmonicsAmount);

    sample *= outputGainLinear;
    sample = juce::jlimit(-2.0f, 2.0f, sample);

    outputBuffer.addSample(0, startSample, sample * panL);
    outputBuffer.addSample(1, startSample, sample * panR);
    ++startSample;
}
```

### DroneStringEngine Compatibility

DroneStringEngine uses core tier only (per CONTEXT-3.4.md decision). It continues using the existing `processSample(v_bow, F_bow, friction)` interface unchanged. No modification needed.

---

## 5. Reversed Friction Transform

### Implementation

The reversed friction is a post-processing transform on rho, independent of which tier computed it:

```cpp
// rho_final = lerp(rho, 1.0 - rho, reversedAmount)
// Equivalent to: rho + reversedAmount * (1.0 - 2.0 * rho)
float applyReversedFriction(float rho, float reversedAmount)
{
    return rho + reversedAmount * (1.0f - 2.0f * rho);
}
```

### Behavior Analysis

| REVERSED_FRICTION | Effect | Sound Character |
|-------------------|--------|-----------------|
| 0% | rho unchanged | Normal bowed string |
| 25% | Slightly flattened dynamic range | Subtly compressed stick-slip |
| 50% | rho always = 0.5 | Flat response, no stick-slip differential |
| 75% | Partially inverted | Synthetic, reversed excitation |
| 100% | rho = 1 - rho | Fully inverted: slip where stick should be, stick where slip should be |

### Stability

At 100% reversed: rho inverts. Where the normal model produces high rho (sticking), reversed produces low rho (slipping) and vice versa. This is stable because rho remains bounded in [0, 1]. The waveguide injection `v_delta * rho` remains bounded. The result is an unusual synthetic timbre, not instability.

### CPU Cost

One multiply + one add per sample. Negligible.

---

## 6. Sub-Harmonics Generator

### Mechanism

The CONTEXT-3.4.md specifies "asymmetric soft clipping on waveguide feedback signal" that "clips positive peaks harder than negative, introducing even harmonics at f/2."

**Physical basis:** In real bowed strings, sub-harmonics (ALF / Anomalous Low Frequency) arise from period-doubling bifurcation when bow pressure exceeds the Helmholtz regime. This is a natural waveguide phenomenon, not simple waveshaping.

**Practical implementation:** Asymmetric waveshaping in the feedback path biases the waveguide toward period-doubling dynamics. At low settings it adds mild timbral asymmetry; at high settings the nonlinearity is strong enough to trigger period doubling in the waveguide loop, producing genuine f/2 content.

### Recommended Clipping Function

Asymmetric tanh waveshaper with controllable depth:

```cpp
class SubHarmonicsGenerator
{
public:
    float process(float input, float amount) const noexcept
    {
        if (amount < 0.001f)
            return input;

        // Asymmetric: positive peaks compressed more than negative
        float depth = amount * 3.0f;  // scale amount to useful range
        float shaped;
        if (input >= 0.0f)
            shaped = std::tanh(input * (1.0f + depth));      // harder clip
        else
            shaped = std::tanh(input * (1.0f + depth * 0.3f)); // softer clip

        // Wet/dry blend
        return input + amount * (shaped - input);
    }
};
```

### Placement in Signal Chain

**Per-voice, post-waveguide, pre-body resonator.** Applied to the output sample from the waveguide AFTER the bridge filter but BEFORE the body resonator. This ensures:
1. Sub-harmonic content gets shaped by the body resonator (correct)
2. Per-voice processing prevents polyphonic muddiness (correct)
3. No interference with the friction junction computation (clean separation)

The CONTEXT-3.4.md says "inside waveguide loop, before bridge filter." For maximum period-doubling effect, applying INSIDE the loop (before bridge filter) is physically correct because the nonlinearity feeds back into the next cycle. However, this risks instability at high settings.

**Safer alternative:** Apply post-waveguide with a feedback tap. At 0%: no cost, bypassed. At 100%: strong asymmetric coloring. The waveguide's own natural period-doubling at high pressures provides authentic subharmonics without external injection.

**Recommendation:** Post-waveguide placement for Phase 3.4 (safe). If perceptual testing shows insufficient sub-octave effect, move inside the loop in a follow-up pass.

### CPU Cost

One branch + one tanh per sample when active. Bypassed at 0% (no cost). ~0.5x core tier cost.

---

## 7. Friction Tier Parameter

### JUCE Implementation

**Use AudioParameterChoice** (same pattern as existing `tuningSystem` parameter in O-Bowed):

```cpp
layout.add(std::make_unique<juce::AudioParameterChoice>(
    juce::ParameterID { "frictionTier", 1 },
    "Friction Tier",
    juce::StringArray { "Core", "Enhanced", "Quality" },
    0  // Default: Core
));
```

### Audio-Thread Reading

```cpp
// In BowedStringVoice::updateParametersFromAPVTS()
int frictionTier = static_cast<int>(
    parameters->getRawParameterValue("frictionTier")->load()
);
```

This is the same pattern used by O-Freeze (MODE), O-Reed (instrumentPreset, boreProfile), and O-Prism (multiple choice params). `getRawParameterValue()->load()` returns 0.0f, 1.0f, or 2.0f which cast cleanly to int.

### Why Choice Over Int

A choice parameter with string labels ("Core"/"Enhanced"/"Quality") provides:
- Named labels in DAW automation lanes
- Correct display in host parameter lists
- Dropdown selector in future GUI (Phase 4.x)

An int parameter (0-2) would display as a number, requiring manual label mapping in the UI.

---

## 8. Bristle/Temperature State Reset on Tier Switch

### Problem

When switching friction tiers mid-note, stale bristle (z) or temperature (T_contact) state from the previous tier can cause artifacts.

### Recommendation: Exponential Decay Reset

```cpp
void onTierSwitch(int newTier, int oldTier)
{
    if (newTier == oldTier) return;

    // Reset enhanced tier bristle state
    if (newTier != 1 && oldTier == 1)
        enhancedFriction.resetState();

    // Reset quality tier bristle + temperature state
    if (newTier != 2 && oldTier == 2)
        qualityFriction.resetState();

    // When switching TO enhanced or quality, start from clean state
    if (newTier == 1 && oldTier != 1)
        enhancedFriction.resetState();
    if (newTier == 2 && oldTier != 2)
        qualityFriction.resetState();
}

// In friction model:
void resetState()
{
    z = 0.0f;           // bristle at equilibrium
    T_contact = T_ambient;  // temperature at room temp (quality tier only)
}
```

### Why Hard Reset (Not Exponential Decay)

A hard reset to zero/ambient is simpler and works because:
1. Bristle state z converges from zero to steady-state within ~2-5 vibration cycles (~5-11ms at 440Hz)
2. Temperature reaches operating range from ambient within ~7ms (tau_heat)
3. The convergence time is shorter than the perceptual threshold for tier-switch artifacts
4. Exponential decay adds per-sample branching that runs every sample even when not switching

### Click Prevention

Read the tier parameter once per block (in updateParametersFromAPVTS). If it changed since last block:
1. Reset state
2. Crossfade over 32 samples between old and new tier output (optional, likely unnecessary given fast convergence)

---

## 9. New Source Files

| File | Description | Size Estimate |
|------|-------------|---------------|
| `Source/DSP/ElastoPlasticFriction.h` | Enhanced tier: bristle ODE, alpha transition, reflection coefficient | ~100 lines |
| `Source/DSP/ThermalFriction.h` | Quality tier: composes elasto-plastic + temperature tracking | ~80 lines |
| `Source/DSP/SubHarmonicsGenerator.h` | Header-only asymmetric waveshaper | ~30 lines |

### Modified Files

| File | Changes |
|------|---------|
| `Source/DSP/WaveguideString.h/.cpp` | Add readJunction()/writeJunction() split methods (keep existing processSample for drone compatibility) |
| `Source/BowedStringVoice.h` | Add ElastoPlasticFriction + ThermalFriction + SubHarmonicsGenerator members, tier dispatch |
| `Source/BowedStringVoice.cpp` | New per-sample loop with tier switch, reversed friction, sub-harmonics |
| `Source/PluginProcessor.cpp` | Add frictionTier choice parameter to layout, read reversedFriction + subHarmonics + frictionTier in processBlock |
| `CMakeLists.txt` | Add 3 new header files |

---

## 10. Parameters Connected After Phase 3.4

**New in 3.4 (3):**
- frictionTier (Choice: Core/Enhanced/Quality)
- reversedFriction (already in APVTS, now connected to voice)
- subHarmonics (already in APVTS, now connected to voice)

**Total connected: 20/23**

**Remaining for Phase 3.5 (3):**
- referencePitch (voice tuning -- currently drone only)
- tuningSystem (Scala/TUN/MTS-ESP engine integration)
- bowNoise (noise generator in signal chain)

---

## 11. CPU Budget Analysis

| Configuration | Core | Enhanced | Quality |
|---------------|------|----------|---------|
| 1 voice, no drone | ~2% | ~6% | ~10% |
| 2 voices, 2 drones | ~8% | ~12%* | ~16%* |
| 4 voices, 4 drones | ~16% | ~20%* | ~24%* |

*Voiced strings use selected tier; drones always use core.*

These estimates are within ROADMAP budget. The quality tier on 4 polyphonic voices (no drones) would be ~40%, which is high but viable for a premium sound mode. The tier selector lets users choose their CPU/quality tradeoff.

---

## 12. Risk Assessment

| Risk | Severity | Mitigation |
|------|----------|------------|
| NR divergence (enhanced/quality) | Medium | 6-iteration cap, bailout to previous rho, clamp output |
| Bristle state runaway | Low | Clamp z to ±1.5 * z_ba, denormal flush |
| Temperature runaway | Low | Clamp T to [T_ambient, 150], reset on tier switch |
| Reversed friction instability | Low | rho stays bounded [0,1], waveguide gain structure unchanged |
| Sub-harmonics feedback buildup | Low | Post-waveguide placement, wet/dry blend |
| Core tier regression | Medium | Existing processSample unchanged, drone engine unaffected |
| Tier switch click | Low | Hard reset converges in <5 cycles (~11ms), below perception |

---

## 13. Academic References

| Reference | Relevance |
|-----------|-----------|
| Dupont et al., IEEE Trans. Automatic Control, 2002 | Single-state elasto-plastic friction model |
| Serafin & Avanzini, SMAC 2003 | Real-time bowed string with elasto-plastic model, sinusoidal alpha |
| Willemsen & Bilbao, DAFx 2019 | FD scheme implementation of elasto-plastic friction |
| Willemsen, Bilbao, Serafin, Frontiers 2025 | Passivity guarantee for discretized elasto-plastic model |
| Smith & Woodhouse, J. Mech. Phys. Solids, 2000 | Tribology of rosin, thermal model foundation |
| Woodhouse, Acustica, 2003 | Thermal friction model for bowed strings |
| Woodhouse & Galluzzo, Tribology Letters, 2025 | Enhanced tribological modeling of violin rosin |
| Smith, Physical Audio Signal Processing (CCRMA) | Bow-string scattering junction, Friedlander construction |

---

_Research complete. Ready for plan phase._
