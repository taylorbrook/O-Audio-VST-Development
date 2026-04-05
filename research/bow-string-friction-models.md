# Bow-String Friction Models for Real-Time Physical Modeling Synthesis

> Technical reference for O-Bowed plugin DSP implementation.
> Covers friction models, digital waveguide integration, bow parameters, and real-time considerations.

---

## Table of Contents

1. [Overview: The Bow-String Interaction](#1-overview)
2. [Friction Models](#2-friction-models)
   - 2.1 Hyperbolic Friction Curve
   - 2.2 STK Bow Table (Smith)
   - 2.3 Elasto-Plastic Friction (Dupont/Serafin)
   - 2.4 Thermal Friction (Woodhouse/Smith)
3. [Digital Waveguide Integration](#3-digital-waveguide-integration)
   - 3.1 Waveguide Architecture
   - 3.2 Bow-String Scattering Junction
   - 3.3 Solving the Implicit Equation
4. [Bow Parameters and Musical Effects](#4-bow-parameters)
   - 4.1 Bow Velocity
   - 4.2 Bow Force (Pressure)
   - 4.3 Bow Position (Beta)
   - 4.4 The Schelleng Diagram
5. [Helmholtz Motion](#5-helmholtz-motion)
6. [Stability and Real-Time Considerations](#6-stability-and-real-time)
7. [Model Comparison and Recommendations](#7-comparison-and-recommendations)
8. [References](#8-references)

---

## 1. Overview: The Bow-String Interaction

The core of bowed string synthesis is the **stick-slip friction interaction** between bow hair (coated in rosin) and the string. During each vibration cycle:

1. **Stick phase**: The string adheres to the bow and moves with it (static friction dominates)
2. **Slip phase**: The string breaks free and slides back (kinetic friction, much lower)

This alternation produces **Helmholtz motion** — the characteristic sawtooth-like waveform of a bowed string. The friction model is the most critical DSP component because it determines:
- Tone quality and timbre
- Attack transients (musically critical — the "bite" of the bow)
- The boundary between musical and unmusical sound
- Stability of the steady-state oscillation

The fundamental challenge: the friction force depends on the relative velocity between bow and string, but the string velocity depends on the friction force. This creates an **implicit nonlinear equation** that must be solved at every sample.

---

## 2. Friction Models

### 2.1 Hyperbolic Friction Curve

The simplest physically-motivated friction model. Used in most academic bowed string simulations as the baseline.

**Equation:**

```
mu(v_rel) = mu_d + (mu_s - mu_d) * v_0 / (v_0 + |v_rel|)
```

**Parameters:**

| Parameter | Symbol | Typical Value | Description |
|-----------|--------|---------------|-------------|
| Static friction coeff. | mu_s | 0.8 | Maximum friction (sticking) |
| Dynamic friction coeff. | mu_d | 0.3 | Minimum friction (fast sliding) |
| Characteristic velocity | v_0 | 0.01 - 0.1 m/s | Controls transition sharpness |
| Relative velocity | v_rel | — | v_bow - v_string |

**Friction force:**

```
F_friction = mu(v_rel) * F_bow * sign(v_rel)
```

Where F_bow is the normal bow force pressing the bow onto the string.

**C++ pseudocode:**

```cpp
// Hyperbolic friction curve
float hyperbolicFriction(float v_rel, float mu_s, float mu_d, float v_0)
{
    float absV = std::abs(v_rel);
    return mu_d + (mu_s - mu_d) * v_0 / (v_0 + absV);
}
```

**Characteristics:**
- Monotonically decreasing with |v_rel|
- Smooth, differentiable everywhere
- The drop from mu_s to mu_d creates the negative-resistance region that sustains oscillation
- v_0 controls how sharp the transition is — smaller = sharper = more aggressive stick-slip

**Limitations:**
- No hysteresis (same path sticking vs slipping)
- Does not capture pre-sliding displacement behavior
- Temperature effects ignored
- Can produce unrealistic abrupt transitions

### 2.2 STK Bow Table (Smith, 1986)

The approach used in Julius O. Smith's Synthesis ToolKit (STK). Rather than computing friction force directly, it maps the **differential velocity** to a **reflection coefficient** for the waveguide junction.

**The Bow Table Function:**

```
BowTable(v_delta) = min( (|v_delta| * slope + 0.75)^(-4), 1.0 )
```

This is NOT a friction curve in the traditional physics sense — it's a **reflection coefficient** that directly tells the waveguide how much energy to reflect vs transmit at the bow point.

**Parameters:**

| Parameter | Range | Musical Meaning |
|-----------|-------|-----------------|
| slope | 1.0 - 5.0 | Inversely related to bow force. slope = 5.0 - 4.0 * bowPressure |
| offset | -1.0 to 1.0 | Bias for asymmetric friction (usually 0) |

**How slope maps to bow force:**
- slope = 5.0 (bowPressure = 0): Very narrow sticking region. Light bowing.
- slope = 1.0 (bowPressure = 1): Wide plateau at reflection = 1. Heavy bowing.

**C++ implementation (from STK):**

```cpp
// BowTable nonlinear function
// Input: differential velocity (v_bow - v_string)
// Output: reflection coefficient [0, 1]
float bowTable(float deltaV, float slope, float offset)
{
    float sample = deltaV + offset;  // add bias
    sample *= slope;                  // scale by stiffness
    sample = std::abs(sample) + 0.75f;
    sample = std::pow(sample, -4.0f);
    return std::min(sample, 1.0f);
}
```

**The complete STK tick() algorithm (from Bowed.h):**

```cpp
// Simplified version of the STK Bowed::tick() algorithm
float tick()
{
    // 1. Bow velocity from envelope
    float bowVelocity = maxVelocity * adsr.tick();

    // 2. Read reflections from delay line endpoints
    float bridgeReflection = -stringFilter.tick(bridgeDelay.lastOut());
    float nutReflection     = -neckDelay.lastOut();

    // 3. Combine traveling waves at bow point
    float stringVelocity = bridgeReflection + nutReflection;

    // 4. Differential velocity
    float deltaV = bowVelocity - stringVelocity;

    // 5. Nonlinear bow table lookup
    float newVelocity = 0.0f;
    if (bowDown)
        newVelocity = deltaV * bowTable.tick(deltaV);

    // 6. Feed back into both delay lines
    bridgeDelay.tick(bridgeReflection + newVelocity);
    neckDelay.tick(nutReflection + newVelocity);

    // 7. Apply vibrato to neck delay length
    if (vibratoGain > 0.0f)
    {
        float vibratoMod = 1.0f + vibratoGain * vibrato.tick();
        neckDelay.setDelay(baseDelay * betaRatio * vibratoMod);
    }

    // 8. Body resonance filtering (6 cascaded biquads)
    float output = bodyFilter.tick(bridgeDelay.lastOut());

    return output * 0.1248f;  // output scaling
}
```

**Key insight:** The `newVelocity = deltaV * bowTable(deltaV)` computation is the normalized reflection coefficient rho_hat applied to the incoming differential velocity. When the bow table returns 1.0 (sticking), the full differential velocity is injected — forcing the string to match the bow. When it returns 0.0 (free slip), no velocity injection occurs.

**Advantages:**
- Extremely fast — no iteration needed
- Inherently stable (output bounded [0, 1])
- Easy to tune with a single slope parameter
- Proven in real-time applications since the 1990s

**Limitations:**
- Static/memoryless — no hysteresis or state
- Simplified physics (no pre-sliding, no thermal effects)
- The 4th-power curve is empirical, not derived from physics
- Does not capture the full richness of real bow-string interaction

### 2.3 Elasto-Plastic Friction Model (Dupont 2002, Serafin 2003)

A state-based friction model that tracks **bristle displacement** — modeling the microscopic deformation of rosin bristles at the contact point. This captures pre-sliding behavior, stick-slip transitions, and frictional memory.

**State variable:** z = average bristle displacement

**Three regimes:**

| Regime | Condition | Behavior |
|--------|-----------|----------|
| Pre-sliding (elastic) | \|z\| < z_ba | Bristles deform elastically. Friction increases with displacement. |
| Elasto-plastic | z_ba < \|z\| < z_ss | Some bristles breaking. Transition zone. |
| Purely plastic (slip) | \|z\| >= z_ss | All bristles broken. Steady sliding. |

**Core equations:**

```
Bristle displacement ODE:
    dz/dt = alpha(z, v_rel) * v_rel

Friction force:
    F = sigma_0 * z + sigma_1 * (dz/dt) + sigma_2 * v_rel

Breakaway displacement:
    z_ba = mu_s * F_n / sigma_0

Steady-state displacement:
    z_ss = mu_d * F_n / sigma_0
```

**The alpha transition function** (controls stick-to-slip transition):

```
alpha(z, v_rel) = 0                                    if |z| < z_ba  (pure stick)
alpha(z, v_rel) = 0.5 * (1 + sin(pi * (z-zm)/(2*h)))  if z_ba <= |z| < z_ss  (transition)
alpha(z, v_rel) = 1                                    if |z| >= z_ss (pure slip)

where:
    zm = 0.5 * (z_ba + z_ss)
    h  = 0.5 * (z_ss - z_ba)
```

When alpha = 0, dz/dt = 0 (sticking — no bristle creep). When alpha = 1, dz/dt = v_rel (fully plastic slip).

**Parameters:**

| Parameter | Symbol | Typical Value | Description |
|-----------|--------|---------------|-------------|
| Bristle stiffness | sigma_0 | 10^4 - 10^5 N/m | Spring constant of bristle contact |
| Bristle damping | sigma_1 | 0.001 - 0.01 Ns/m | Viscous damping of bristles |
| Viscous friction | sigma_2 | 0.0001 - 0.001 Ns/m | Velocity-proportional damping |
| Static friction coeff. | mu_s | 0.8 | Maximum (sticking) friction |
| Dynamic friction coeff. | mu_d | 0.3 | Kinetic (sliding) friction |
| Normal force | F_n | 0.01 - 5 N | Bow pressing force |

**C++ pseudocode:**

```cpp
struct ElastoPlasticFriction
{
    float z = 0.0f;       // bristle displacement state
    float sigma_0, sigma_1, sigma_2;
    float mu_s, mu_d;

    float tick(float v_rel, float F_normal, float dt)
    {
        float z_ba = mu_s * F_normal / sigma_0;  // breakaway
        float z_ss = mu_d * F_normal / sigma_0;  // steady-state

        // Alpha function (smooth transition)
        float alpha;
        float absZ = std::abs(z);
        if (absZ < z_ba)
            alpha = 0.0f;  // pure stick
        else if (absZ < z_ss)
        {
            float zm = 0.5f * (z_ba + z_ss);
            float h  = 0.5f * (z_ss - z_ba);
            alpha = 0.5f * (1.0f + std::sin(M_PI * (absZ - zm) / (2.0f * h)));
        }
        else
            alpha = 1.0f;  // pure slip

        // Bristle displacement update
        float dz = alpha * v_rel;
        z += dz * dt;

        // Clamp z to prevent runaway
        z = std::clamp(z, -z_ss * 1.5f, z_ss * 1.5f);

        // Friction force
        float F = sigma_0 * z + sigma_1 * dz + sigma_2 * v_rel;
        return F;
    }
};
```

**Advantages:**
- Captures pre-sliding displacement (realistic micromotion during stick)
- Natural hysteresis emerges from the state variable
- Smooth transitions between stick and slip
- Produces more natural attack transients than memoryless models
- Physically grounded in tribology research

**Limitations:**
- State variable adds complexity to the implicit equation (harder to solve in waveguide)
- Passivity NOT guaranteed for all parameter values (bristle damping can go negative)
- Recent work (Frontiers, 2025) proposes velocity-dependent damping to fix passivity
- Higher CPU cost than bow table
- More parameters to tune

**Passivity fix (recent research):**

The original model can produce energy when sigma_1 * dz/dt is negative. The fix:

```cpp
// Velocity-dependent damping ensures passivity
float sigma_1_safe = sigma_1_base * (1.0f + v_rel * v_rel);
```

This ensures all dissipation terms remain non-negative.

### 2.4 Thermal Friction Model (Woodhouse/Smith, 2000)

The most physically accurate model. Rather than friction depending on velocity directly, friction depends on **temperature** at the bow-string contact point. The velocity dependence emerges indirectly through heat generation.

**Core concept:** Rosin undergoes a **glass transition** at ~49 degrees C. Above this temperature, it softens dramatically and friction drops. The friction coefficient varies by ~7 orders of magnitude across the relevant temperature range.

**Equations:**

```
Friction coefficient:
    mu = mu(T_contact)

Heat generation:
    Q = F_friction * |v_rel|    (friction power = force × speed)

Temperature evolution (simplified 1D heat diffusion):
    dT/dt = (Q - k * (T - T_ambient)) / (rho * c * V_contact)

    where:
        k     = thermal conductivity of rosin
        rho   = density
        c     = specific heat
        V_contact = contact volume
        T_ambient = room temperature (~20 C)
```

**The mu(T) relationship** (empirical, from Smith & Woodhouse tribology measurements):

```
mu(T) = mu_high * exp(-alpha_T * (T - T_ref))    for T > T_glass
mu(T) = mu_high                                    for T <= T_glass

Typical values:
    T_glass  = 49 C (glass transition temperature)
    T_ref    = 49 C
    mu_high  = 1.0 - 1.2  (below glass transition)
    alpha_T  = 0.1 - 0.3  (exponential decay rate)
```

**Why this matters:**

During Helmholtz motion, the contact point undergoes temperature cycling:
1. **Stick phase**: Low v_rel → low heat generation → temperature drops → friction rises → maintains stick
2. **Slip phase**: High v_rel → high heat generation → temperature rises → friction drops → maintains slip

This creates natural **hysteresis loops** in the friction-velocity plane that match experimental measurements and are absent in the simple hyperbolic model.

**C++ pseudocode:**

```cpp
struct ThermalFriction
{
    float T_contact = 20.0f;  // contact temperature (Celsius)
    float T_ambient = 20.0f;
    float T_glass   = 49.0f;  // glass transition
    float alpha_T   = 0.2f;   // exponential decay rate
    float mu_max    = 1.0f;   // friction below glass transition
    float thermalMass;         // rho * c * V_contact
    float thermalConductivity; // cooling rate

    float tick(float v_rel, float F_normal, float dt)
    {
        // Current friction coefficient from temperature
        float mu;
        if (T_contact <= T_glass)
            mu = mu_max;
        else
            mu = mu_max * std::exp(-alpha_T * (T_contact - T_glass));

        // Friction force
        float F_friction = mu * F_normal * sign(v_rel);

        // Heat generation
        float Q_gen = std::abs(F_friction * v_rel);

        // Cooling (Newton's law)
        float Q_loss = thermalConductivity * (T_contact - T_ambient);

        // Temperature update
        T_contact += (Q_gen - Q_loss) / thermalMass * dt;

        // Prevent runaway
        T_contact = std::clamp(T_contact, T_ambient, 200.0f);

        return F_friction;
    }
};
```

**Advantages:**
- Most physically accurate — matches SEM evidence of rosin melting
- Natural hysteresis emerges from thermal inertia
- Helmholtz motion establishes more reliably and quickly
- Predicts real bow behavior better than velocity-only models
- Natural thermal relaxation during stick phase

**Limitations:**
- Thermal parameters are hard to measure/tune
- Adds a slow state variable (temperature) with its own dynamics
- More complex implicit equation to solve
- Limited published real-time implementations
- Parameter sensitivity to rosin type

---

## 3. Digital Waveguide Integration

### 3.1 Waveguide Architecture

The string is modeled as two pairs of **bidirectional delay lines**, split at the bow contact point:

```
                    Bow (at position beta*L from bridge)
                         |
    [Bridge]---bridgeDelay---|---neckDelay---[Nut/Finger]
                         |
         v_sr+  -->      |     <--  v_sl+
         v_sr-  <--      |     -->  v_sl-
```

- **Bridge side (right)**: bridgeDelay with length proportional to (beta * L)
- **Nut side (left)**: neckDelay with length proportional to ((1-beta) * L)
- Each delay pair carries right-going (+) and left-going (-) velocity waves

**Delay lengths for a given frequency f0:**

```cpp
float totalDelay = sampleRate / f0;  // total round-trip in samples
float bridgeDelaySamples = totalDelay * betaRatio;
float neckDelaySamples   = totalDelay * (1.0f - betaRatio);
```

**Termination filters:**
- **Bridge**: Lowpass filter modeling bridge impedance and body coupling. Typically a one-pole: `H(z) = g * (1-p) / (1 - p*z^-1)` where g < 1 (loss) and p controls brightness.
- **Nut/Finger**: Near-total reflection with sign inversion (hard boundary). Finger stopping changes the effective string length.

### 3.2 Bow-String Scattering Junction

The bow creates a **nonlinear two-port scattering junction**. The key equations:

**Incoming signals (known at each sample):**

```
v_s+ = v_sr+(n) + v_sl+(n)    // total incoming string velocity at bow point
v_delta+ = v_b - v_s+          // incoming differential velocity
```

Where v_sr+ is the right-going wave arriving from the bridge side, and v_sl+ is the left-going wave arriving from the nut side.

**The scattering computation:**

```
rho_hat = r(v_delta) / (1 + r(v_delta))

where:
    r(v_delta) = 0.25 * R_b(v_delta) / R_s

    R_s = string wave impedance = sqrt(T * rho_linear)
    R_b(v_delta) = friction coefficient function (from chosen model)
```

**Outgoing signals:**

```
v_sr-(n) = v_sl+(n) + rho_hat(v_delta+) * v_delta+
v_sl-(n) = v_sr+(n) + rho_hat(v_delta+) * v_delta+
```

Or equivalently using the STK approach:

```
newVelocity = v_delta+ * BowTable(v_delta+)

bridgeDelay.input = bridgeReflection + newVelocity
neckDelay.input   = nutReflection    + newVelocity
```

The `newVelocity` term is the reflected component that gets injected equally into both delay lines, representing the force the bow exerts on the string.

**Physical interpretation:**
- BowTable returns 1.0 (sticking): `newVelocity = v_delta+` → full velocity injection → string forced to match bow
- BowTable returns 0.0 (no bow): `newVelocity = 0` → waves pass through unimpeded
- BowTable returns intermediate: Partial sticking/slipping

### 3.3 Solving the Implicit Equation

The problem: computing v_delta requires knowing the string velocity, which depends on the outgoing waves, which depend on the friction force, which depends on v_delta. This circular dependency creates an implicit equation.

**Three approaches:**

#### A. Memoryless Table Lookup (STK approach)

Avoids the implicit equation entirely by making the bow table a function of the **incoming** differential velocity v_delta+ rather than the true differential velocity v_delta.

```
v_delta+ = v_b - (v_sr+ + v_sl+)     // known quantities
output   = v_delta+ * BowTable(v_delta+)  // no iteration needed
```

This is an approximation but works well because the incoming velocity is a good predictor of the true velocity. The error is small relative to the friction nonlinearity.

**Cost:** O(1) per sample. One function evaluation, no iteration.

#### B. Newton-Raphson Iteration

For models where the friction force depends on the true velocity (not just the incoming velocity), we must solve:

```
g(v) = F_friction(v_b - v) - R_s * (v - v_s+) = 0
```

Newton-Raphson:

```cpp
float solveNR(float v_b, float v_incoming, float R_s, int maxIter = 4)
{
    float v = v_incoming;  // initial guess: use incoming velocity

    for (int i = 0; i < maxIter; ++i)
    {
        float v_rel = v_b - v;
        float F = frictionForce(v_rel);
        float g = F - R_s * (v - v_incoming);

        // Derivative: dg/dv = -dF/dv_rel - R_s
        float dF = frictionDerivative(v_rel);
        float dg = -dF - R_s;

        float delta = g / dg;
        v -= delta;

        if (std::abs(delta) < 1e-6f)
            break;
    }
    return v;
}
```

**Cost:** 2-6 iterations typically. ~4 function evaluations per sample.

**Stability note:** The friction derivative must be well-behaved. The hyperbolic curve has a smooth derivative everywhere. The elasto-plastic model can have discontinuous derivatives at regime boundaries — use the smooth alpha transition function to mitigate.

#### C. Precomputed Lookup Table

Precompute the solution for a grid of (v_delta+, bowForce) values:

```cpp
// At initialization:
float lut[TABLE_SIZE_V][TABLE_SIZE_F];
for (int iv = 0; iv < TABLE_SIZE_V; ++iv)
    for (int iF = 0; iF < TABLE_SIZE_F; ++iF)
    {
        float v_delta = minV + iv * vStep;
        float force   = minF + iF * fStep;
        lut[iv][iF] = solveNR(v_delta, force);  // solve offline
    }

// At runtime:
float result = bilinearInterp(lut, v_delta_plus, bowForce);
```

**Cost:** O(1) per sample (table lookup + interpolation). Memory cost for the table.

### 3.4 The Friedlander Construction and Ambiguity

The Friedlander graphical method finds solutions by intersecting:
1. The friction curve f(v_rel) (nonlinear, falling with velocity)
2. The impedance line: F = R_s * (v_s+ - v) (straight line, slope = -R_s)

**The ambiguity problem:** For certain parameter values, the impedance line can intersect the friction curve at **three points**. Two are stable (one in stick, one in slip), and the middle one is always unstable.

**Resolution rule (hysteresis):** Maintain the current state (stick or slip) as long as possible:
- If currently sticking → stay on the sticking solution until it vanishes
- If currently slipping → stay on the slipping solution until it vanishes
- The unstable middle solution is never physically realized

```cpp
// Friedlander ambiguity resolution
enum BowState { STICKING, SLIPPING };
BowState currentState = STICKING;

float resolveAmbiguity(float v_stick, float v_slip, float v_unstable)
{
    if (currentState == STICKING)
    {
        // Stay sticking if stick solution exists
        if (isValid(v_stick))
            return v_stick;
        currentState = SLIPPING;
        return v_slip;
    }
    else
    {
        // Stay slipping if slip solution exists
        if (isValid(v_slip))
            return v_slip;
        currentState = STICKING;
        return v_stick;
    }
}
```

---

## 4. Bow Parameters and Musical Effects

### 4.1 Bow Velocity (v_b)

| Range | Value | Musical Context |
|-------|-------|----------------|
| Very slow | 0.02 - 0.05 m/s | Pianissimo, delicate |
| Moderate | 0.1 - 0.5 m/s | Normal playing |
| Fast | 0.5 - 1.0 m/s | Fortissimo, aggressive |
| Maximum | ~2.0 m/s | Extreme, virtuosic |

**Effect:** Primarily controls **loudness**. Higher bow velocity injects more energy per cycle. Also affects the brightness — faster bowing produces a wider frequency spectrum.

**In the waveguide:** Bow velocity directly scales the injected velocity at the scattering junction: `newVelocity = deltaV * bowTable(deltaV)` where deltaV includes v_b.

### 4.2 Bow Force (F_bow / Pressure)

| Range | Value | Musical Context |
|-------|-------|----------------|
| Very light | 0.01 - 0.1 N | Sul tasto, harmonics |
| Normal | 0.2 - 1.0 N | Standard playing |
| Heavy | 1.0 - 3.0 N | Fortissimo, marcato |
| Extreme | 3.0 - 5.0 N | Extended technique |

**Effect:** The most critical parameter for **tone quality**. Too little force → surface sound (airy, unfocused). Too much force → raucous, crunchy sound. The sweet spot is the Helmholtz region.

**In the STK model:** Maps to bow table slope: `slope = 5.0 - 4.0 * normalizedPressure`
- High pressure → low slope → wide sticking region → strong grip
- Low pressure → high slope → narrow sticking region → easy slip

### 4.3 Bow Position (beta)

The normalized distance from the bridge, expressed as a fraction of string length.

| Beta | Position | Musical Term | Spectral Character |
|------|----------|-------------|-------------------|
| 0.02 - 0.05 | Very close to bridge | Sul ponticello | Bright, glassy, harmonics prominent |
| 0.08 - 0.12 | Normal position | Ordinario | Balanced, full tone |
| 0.15 - 0.25 | Near middle | Sul tasto | Warm, dark, fundamental dominant |
| > 0.30 | Near fingerboard | Extreme sul tasto | Very soft, hollow |

**Effect on spectrum:** Playing closer to the bridge (smaller beta) emphasizes higher harmonics because the bow excites a narrower segment of string, creating a sharper corner in the Helmholtz motion. Playing over the fingerboard (larger beta) filters out higher harmonics.

**In the waveguide:** Beta determines the delay line split ratio:

```cpp
float bridgeDelaySamples = totalDelay * beta;
float neckDelaySamples   = totalDelay * (1.0f - beta);
```

Typical beta for violin: 1/7 to 1/13 of string length (0.077 to 0.143).

### 4.4 The Schelleng Diagram

The Schelleng diagram maps the **playable region** in the force-position space. It defines where Helmholtz motion can be sustained.

**Maximum bow force (upper limit):**

```
F_max = (2 * Z_0 * v_b) / (beta * (mu_s - mu_d))
```

**Minimum bow force (lower limit):**

```
F_min = (Z_0^2 * v_b) / (2 * R * beta^2 * (mu_s - mu_d))
```

**Force ratio:**

```
F_max / F_min = 4 * beta * R / Z_0
```

**Parameters:**

| Symbol | Description |
|--------|-------------|
| Z_0 | String characteristic impedance: Z_0 = sqrt(T * m) where T = tension, m = mass per unit length |
| R | Body mechanical impedance (bridge resistance) |
| v_b | Bow velocity |
| beta | Bow position (fraction from bridge) |
| mu_s | Static friction coefficient |
| mu_d | Dynamic friction coefficient |

**Playability conditions:**
- F_min < F_bow < F_max: Helmholtz motion (good tone)
- F_bow > F_max: **Raucous sound** — chaotic, crunchy, irregular vibration
- F_bow < F_min: **Surface sound** — double-slip motion, weak airy tone at same pitch but different timbre
- F_max < F_min (when beta < Z_0 / (4*R)): **No playable region** — too close to bridge

On a log-log plot of force vs beta:
- F_max has slope -1 (inversely proportional to beta)
- F_min has slope -2 (inversely proportional to beta^2)
- They form a **wedge-shaped** playable region that narrows toward the bridge

**Practical implication for synthesis:** Use these equations to create a "playability guard" that prevents parameters from producing unrealistic sounds unless deliberately desired (extended techniques).

```cpp
float getMaxForce(float Z_0, float v_b, float beta, float mu_s, float mu_d)
{
    return (2.0f * Z_0 * v_b) / (beta * (mu_s - mu_d));
}

float getMinForce(float Z_0, float v_b, float beta, float mu_s, float mu_d, float R)
{
    return (Z_0 * Z_0 * v_b) / (2.0f * R * beta * beta * (mu_s - mu_d));
}

bool isPlayable(float force, float Z_0, float v_b, float beta,
                float mu_s, float mu_d, float R)
{
    float fMax = getMaxForce(Z_0, v_b, beta, mu_s, mu_d);
    float fMin = getMinForce(Z_0, v_b, beta, mu_s, mu_d, R);
    return (force >= fMin && force <= fMax);
}
```

---

## 5. Helmholtz Motion

The **ideal** steady-state vibration of a bowed string, first described by Hermann von Helmholtz in 1863.

**Description:**
- The string forms a "V" shape — two straight segments meeting at a sharp corner
- The corner travels back and forth along the string at the wave speed
- At the bowing point, the string alternates between:
  - **Stick**: Corner is between nut and bow → string moves with bow
  - **Slip**: Corner passes bow point → string slides back rapidly

**Waveform at the bridge:** A sawtooth wave, with the slope ratio determined by bow position:
- Stick duration: (1 - beta) of the period
- Slip duration: beta of the period
- The ratio creates the characteristic "leaning sawtooth"

**Slip velocity:**

```
v_slip = -v_b * (1 - beta) / beta
```

For typical beta = 0.1: slip velocity is 9x bow velocity in the opposite direction.

**String displacement at bow point:**

```
y_max = v_b / (2 * pi * f0)    (approximate peak displacement)
```

**What affects Helmholtz motion quality:**
- **Clean Helmholtz**: Single corner, regular period → clear, focused tone
- **Multiple slip**: Two or more corners → "surface sound," octave or higher partials
- **Irregular motion**: Random corner behavior → "raucous" noise

**For synthesis:** The friction model should naturally produce Helmholtz motion within the Schelleng playable region. If the model requires manual tuning to achieve this, the model parameters are likely wrong.

---

## 6. Stability and Real-Time Considerations

### 6.1 Numerical Stability

**Digital waveguide + bow table (STK approach):**
- Inherently stable. The bow table output is bounded [0, 1], and the delay lines are linear.
- No stability issues at any sample rate.
- The memoryless table avoids feedback loops that could cause instability.

**Newton-Raphson with hyperbolic curve:**
- Generally stable if the initial guess is good (use previous sample's solution).
- Can diverge if the friction derivative is too steep (very small v_0).
- Guard: Clamp v_0 >= 0.01 m/s.
- Maximum 4-6 iterations is typical; bail out and use last valid solution if not converged.

**Elasto-plastic model:**
- Passivity is NOT guaranteed in the original formulation.
- The bristle damping term sigma_1 * dz/dt can inject energy when dz/dt opposes the velocity.
- **Fix:** Use velocity-dependent damping: `sigma_1_eff = sigma_1 * (1 + v_rel^2)`
- Alternatively, use energy-balanced discretization schemes (Bilbao approach).
- Clamp bristle displacement z to prevent runaway.

**Thermal model:**
- Temperature must be bounded (clamp to [T_ambient, T_max]).
- Thermal time constants can cause slow drift — ensure the cooling term is always active.
- The exponential mu(T) can produce very small values — add a floor: `mu >= 0.01`.

### 6.2 Sample Rate Requirements

| Model | 44.1 kHz | 88.2 kHz | 176.4 kHz | Notes |
|-------|----------|----------|-----------|-------|
| STK Bow Table | Works fine | Recommended | Overkill | Very forgiving |
| Hyperbolic + NR | OK for most | Recommended | — | Oversampling helps attack transients |
| Elasto-Plastic | Marginal | Recommended | Ideal | Pre-sliding behavior needs temporal resolution |
| Thermal | Works fine | Recommended | — | Thermal dynamics are slow relative to audio rate |
| FDTD (Bilbao) | Insufficient | Minimum | Recommended | Grid spacing depends on sample rate |

**Recommendation for a plugin:** Use **2x oversampling** (internal 88.2 kHz at 44.1 kHz host rate). This provides adequate resolution for the nonlinear friction computation and attack transients without excessive CPU cost. Process the friction/waveguide at 2x, then downsample the output.

### 6.3 CPU Cost Estimates

| Model | Operations/Sample | Relative Cost | Real-Time? |
|-------|-------------------|---------------|------------|
| STK Bow Table | ~30 (table + delays + filters) | 1x (baseline) | Yes, trivial |
| Hyperbolic + NR (4 iter) | ~120 | 4x | Yes |
| Elasto-Plastic | ~80 (state update + force) | 3x | Yes |
| Elasto-Plastic + FDTD | ~500+ (grid computation) | 15-20x | Yes (< 6% CPU per string, Willemsen 2019) |
| Thermal + NR | ~150 | 5x | Yes |
| Full Bilbao FDTD | ~1000+ per grid point | 30-50x | Tight, needs optimization |

**For a plugin with 4 strings + body resonance:** The STK approach or enhanced STK (hyperbolic curve + NR) is the sweet spot. Budget roughly 5-10% CPU per string on a modern machine.

### 6.4 Attack Transients

The bow attack is musically critical — it defines the character of each note start. Different attacks require different friction behaviors:

| Attack Type | Bow Action | Friction Behavior |
|-------------|-----------|-------------------|
| Legato | Smooth start, gradual force increase | Slow transition from slip to Helmholtz |
| Detache | Moderate force, immediate bow change | Quick Helmholtz establishment |
| Spiccato | Drop bow, bounce | Brief contact, sharp attack-decay |
| Martele | Heavy initial force, sudden release | Strong attack transient, immediate Helmholtz |
| Col legno | Wood of bow on string | Percussive, minimal friction |

**Implementation strategy:**
- Use an ADSR or custom envelope on bow velocity
- Ramp bow force from zero to target over 5-50 ms depending on articulation
- The friction model should naturally produce pre-Helmholtz transients (multiple slips, surface sound) during the onset — don't try to suppress these, they're musically correct

```cpp
// Attack envelope example
float attackTime = 0.02f;  // 20ms for detache
float attackRate = 1.0f / (attackTime * sampleRate);

void startBow(float targetVelocity, float targetForce)
{
    bowVelocityTarget = targetVelocity;
    bowForceTarget    = targetForce;
    bowVelocity = 0.0f;
    bowForce    = 0.0f;
    bowing = true;
}

void updateBowEnvelope()
{
    if (bowing)
    {
        bowVelocity += (bowVelocityTarget - bowVelocity) * attackRate;
        bowForce    += (bowForceTarget - bowForce) * attackRate;
    }
}
```

---

## 7. Model Comparison and Recommendations

### Comparison Table

| Criterion | STK Bow Table | Hyperbolic + NR | Elasto-Plastic | Thermal |
|-----------|--------------|-----------------|----------------|---------|
| **Physical accuracy** | Low | Medium | High | Highest |
| **CPU cost** | Very low | Low | Medium | Medium |
| **Implementation complexity** | Trivial | Easy | Moderate | Moderate |
| **Stability** | Unconditional | Good | Needs care | Good |
| **Attack quality** | Simplified | Good | Excellent | Excellent |
| **Hysteresis** | None | None | Natural | Natural |
| **Parameters to tune** | 2 | 4 | 7+ | 6+ |
| **Pre-sliding** | No | No | Yes | No |
| **Proven real-time** | Yes (30+ years) | Yes | Yes (Willemsen 2019) | Limited |

### Recommended Architecture for O-Bowed

**Approach: Tiered friction model**

1. **Core (always active):** Enhanced bow table inspired by STK, but using the hyperbolic friction curve instead of the empirical 4th-power formula:

```cpp
// Enhanced bow table using hyperbolic friction
float enhancedBowTable(float v_delta, float mu_s, float mu_d, float v_0, float R_s)
{
    float absV = std::abs(v_delta);
    float mu = mu_d + (mu_s - mu_d) * v_0 / (v_0 + absV);

    // Convert friction coefficient to reflection coefficient
    float r = 0.25f * mu / R_s;
    float rho = r / (1.0f + r);

    return rho;  // [0, ~0.5] range
}
```

2. **Optional enhancement:** Add elasto-plastic bristle state for improved attack transients and hysteresis. Can be toggled for CPU savings.

3. **Quality mode:** Add thermal tracking on top of (1) for the most realistic steady-state behavior.

**Why this order:**
- The hyperbolic curve gives 90% of the character with minimal CPU
- Elasto-plastic adds the most *audible* improvement (attack transients, stick-slip transitions)
- Thermal adds subtle steady-state realism that mostly matters for sustained notes

### Parameter Mapping for User Controls

| UI Control | Internal Parameters | Range |
|-----------|-------------------|-------|
| **Bow Speed** | v_b (bow velocity) | 0.02 - 2.0 m/s (log scale recommended) |
| **Bow Pressure** | F_bow (normal force), bow table slope | 0.01 - 5.0 N |
| **Bow Position** | beta (delay line ratio) | 0.02 - 0.30 |
| **Rosin** (character) | mu_s/mu_d ratio, v_0 | Blends between smooth and aggressive friction curves |
| **Brightness** | Bridge filter cutoff, beta fine-tune | Spectral shaping |
| **Attack** | Envelope attack time, initial force profile | 5 - 200 ms |

---

## 8. References

### Foundational Papers
- McIntyre, M.E., Schumacher, R.T., & Woodhouse, J. (1983). "On the oscillations of musical instruments." JASA 74(5).
- Smith, J.O. (1986). "Efficient simulation of the reed-bore and bow-string mechanisms." Proc. ICMC.
- Friedlander, F.G. (1953). "On the oscillations of a bowed string." Proc. Cambridge Phil. Soc.

### Friction Models
- Smith, J.H. & Woodhouse, J. (2000). "The tribology of rosin." J. Mech. Phys. Solids.
- Woodhouse, J. (2003). "Bowed string simulation using a thermal friction model." Acta Acustica.
- Serafin, S. & Avanzini, F. (2003). "Bowed string simulation using an elasto-plastic friction model." Proc. SMAC.
- Dupont, P., Armstrong, B., & Hayward, V. (2002). "Elasto-plastic friction model." Proc. IEEE ICRA.

### Digital Waveguide Implementation
- Smith, J.O. "Physical Audio Signal Processing" (online book). https://www.dsprelated.com/freebooks/pasp/
- Välimäki, V. (1995). "Discrete-time modeling of acoustic tubes using fractional delay filters." PhD thesis.
- STK (Synthesis ToolKit in C++): https://github.com/thestk/stk

### Playability and Schelleng
- Schelleng, J.C. (1973). "The bowed string and the player." JASA 53(1).
- Woodhouse, J. (2004). "On the playability of violins." Acta Acustica.
- Schoonderwaldt, E. & Demoucron, M. (2009). "Extraction of bowing parameters from violin performance."

### Modern/FDTD Approaches
- Bilbao, S. (2009). "Numerical Sound Synthesis." Wiley.
- Desvages, C. & Bilbao, S. (2016). "Two-polarisation physical model of bowed strings." Applied Sciences.
- Willemsen, S. & Bilbao, S. (2019). "Real-time implementation of an elasto-plastic friction model." Proc. DAFx.
- Frontiers in Signal Processing (2025). "Numerical modelling of elasto-plastic friction in bow-string interaction with guaranteed passivity."

### Online Resources
- Euphonics (Woodhouse): https://euphonics.org/
- CCRMA Bowed Strings: https://ccrma.stanford.edu/~jos/pasp/Bowed_Strings.html
- SegfaultDSP Bowed String tutorial: https://segfaultdsp.com/posts/bowed_string/waveguide.html
- NESS Project (Edinburgh): http://www.ness.music.ed.ac.uk/archives/systems/bowed-string-instruments
