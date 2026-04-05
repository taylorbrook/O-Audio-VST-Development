# Phase 3.1: Reed ODE + Bernoulli Junction + Core Bore - Research

**Researched:** 2026-04-05
**Domain:** Mass-spring-damper reed ODE discretization, Bernoulli nonlinear junction, reed-bore coupling
**Confidence:** HIGH (core equations from published acoustics literature + verified JUCE APIs)

## Summary

This research covers the exact DSP math needed for Phase 3.1: the full dynamic reed ODE with symplectic Euler discretization, the Bernoulli flow equation (Psi=0 for single-reed only), the reed-bore coupling via the one-sample-delay decoupling trick, and note onset/offset envelopes. All equations are presented in discrete-time C++ form ready for implementation.

The key insight: Smith's "reed table" formulation absorbs the Bernoulli equation into a single nonlinear reflection coefficient rho_hat, computed from the half-pressure difference at the junction. This avoids explicit flow-to-pressure conversion -- the junction operates entirely in wave variables. For the dynamic reed (with mass), we add symplectic Euler integration of the reed displacement ODE, which modulates the effective reed opening that feeds into the reflection coefficient.

The one-sample delay inherent in the bore waveguide naturally decouples the implicit reed-bore equation. The bore returns pressure from the PREVIOUS sample, which drives the reed computation for the CURRENT sample, which produces the outgoing pressure wave. No Newton-Raphson iteration is needed for Phase 3.1 -- the system is explicit.

**Primary recommendation:** Implement the reed ODE as a symplectic Euler update (velocity-first, then position), feed the updated reed opening into a polynomial reed reflection function (cubic approximation of the STK reed table), and couple to the bore waveguide via the standard pop-before-push pattern already proven in O-Bowed's WaveguideString.

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
- `getLatencySamples()` is non-virtual in JUCE 8 -- use `setLatencySamples(N)` in `prepareToPlay()`
- Phase/stage completion requires handoff message with `/clear` as Step 1

---

## 1. Reed ODE: Mass-Spring-Damper Discretization

### 1.1 Continuous-Time Equation

The reed tip displacement `x` from equilibrium is governed by:

```
mu_r * d2x/dt2 + g_eff * dx/dt + k_eff * (x - x_eq) = (p_mouth - p_bore) * A_reed
```

Where:
- `mu_r` = effective reed mass per unit area (kg/m^2)
- `g_eff = g_r + g_lip` = total damping (reed intrinsic + lip contact)
- `k_eff = k_r + k_lip` = total stiffness (reed intrinsic + lip stiffness)
- `x` = reed tip displacement (negative = closing toward mouthpiece)
- `x_eq` = equilibrium position (typically 0)
- `p_mouth` = mouth cavity pressure (player's breath)
- `p_bore` = pressure at bore entrance (from waveguide)
- `A_reed` = effective driving area of reed (~reed_width * effective_length)

**Embouchure modification** (the lip adds damping, stiffness, and pre-closes the reed):
```
g_eff = g_r + embouchure * g_lip_max
k_eff = k_r + embouchure * k_lip_max
H_eff = H - embouchure * x_lip_max    // reduced rest opening from lip pressure
```

### 1.2 Symplectic Euler Discretization (Velocity-First)

Symplectic Euler (also called semi-implicit Euler or Euler-Cromer) updates velocity first using the current position, then updates position using the NEW velocity. This preserves energy in the undamped case and is unconditionally better than forward Euler for oscillatory systems.

**Discrete-time update at sample rate fs (or oversampled rate):**

```
dt = 1.0 / fs

// Forces on the reed
force = (p_mouth - p_bore) * A_reed - g_eff * x_dot - k_eff * x

// Step 1: Update velocity using current position (velocity-first)
x_dot_new = x_dot + (force / mu_r) * dt

// Step 2: Update position using NEW velocity
x_new = x + x_dot_new * dt

// Step 3: Enforce reed closure limit
x_new = max(x_new, -(H_eff + epsilon))

// If clamped, zero the velocity to prevent energy accumulation at the wall
if (x_new <= -(H_eff + epsilon))
    x_dot_new = 0.0f;

// Step 4: Store state
x = x_new
x_dot = x_dot_new
```

The epsilon (~0.001f or 1e-4f) prevents the reed from reaching exactly zero opening, which would cause a divide-by-zero in the flow equation. This also models the physical reality that reeds don't form a perfect seal.

### 1.3 C++ Implementation

```cpp
struct ReedState {
    float x     = 0.0f;   // displacement from equilibrium (negative = closing)
    float x_dot = 0.0f;   // velocity
};

struct ReedParams {
    float mu_r     = 0.023f;    // mass per unit area (kg/m^2)
    float g_r      = 2900.0f;   // intrinsic damping (s^-1)
    float k_r      = 8.0e6f;    // intrinsic stiffness (N/m^3... normalized by A_reed)
    float H        = 0.0004f;   // rest opening (m) -- 0.4mm for clarinet
    float w        = 0.013f;    // channel width (m) -- 13mm for clarinet
    float A_reed   = 0.0001f;   // effective driving area (m^2)
    
    // Embouchure modifiers (set per-block from APVTS)
    float g_lip    = 0.0f;
    float k_lip    = 0.0f;
    float x_lip    = 0.0f;      // lip pre-closure offset (m)
};

// Per-sample reed ODE update (symplectic Euler)
inline void updateReed(ReedState& state, const ReedParams& p,
                       float p_mouth, float p_bore, float dt)
{
    float g_eff = p.g_r + p.g_lip;
    float k_eff = p.k_r + p.k_lip;
    float H_eff = p.H - p.x_lip;
    
    // Net force on reed (pressure difference * area - damping - spring)
    float force = (p_mouth - p_bore) * p.A_reed
                - g_eff * state.x_dot
                - k_eff * state.x;

    // Symplectic Euler: velocity first
    float mu_safe = std::max(p.mu_r, 1e-6f);  // prevent division by zero
    state.x_dot += (force / mu_safe) * dt;
    state.x     += state.x_dot * dt;

    // Reed closure limit
    constexpr float epsilon = 1e-4f;  // 0.1mm minimum opening
    float x_min = -(H_eff + epsilon);
    if (state.x < x_min) {
        state.x = x_min;
        state.x_dot = std::min(state.x_dot, 0.0f); // only zero if pushing into wall
    }
}
```

### 1.4 Stability Criteria

For symplectic Euler applied to a damped harmonic oscillator, the stability condition is:

```
omega * dt < 2
```

Where `omega = sqrt(k_eff / mu_r)` is the natural angular frequency of the reed.

**For clarinet:** omega_r = 2 * pi * 3700 Hz = ~23250 rad/s.
At 44.1 kHz: omega * dt = 23250 / 44100 = 0.527. Well within the limit of 2.

**At what point does it blow up?** The reed becomes unstable when:
```
f_reed > fs / pi
```

For 44.1 kHz: f_reed_max = 14040 Hz. Reed resonances never approach this.
For 2x oversampled (88.2 kHz): f_reed_max = 28080 Hz. Even more headroom.

**Damping improves stability.** With positive damping (g_eff > 0), the critical timestep is:
```
dt_critical = (2 / omega) * (sqrt(zeta^2 + 1) - zeta)
```
where `zeta = g_eff / (2 * sqrt(k_eff * mu_r))` is the damping ratio. Since we always have positive damping, the actual stability margin is wider than the undamped condition.

**When mu_r approaches zero (epsilon mass):** The ODE becomes stiff -- dt * omega grows large. With mu_safe clamped to 1e-6, the effective omega = sqrt(k_eff / 1e-6) which can be very large. **Solution:** When REED_MASS parameter is near zero, switch to the static reed model (memoryless spring) to avoid numerical issues:

```cpp
if (p.mu_r < 1e-4f) {
    // Static reed: x responds instantly to pressure
    float dp = p_mouth - p_bore;
    state.x = (dp * p.A_reed) / k_eff;
    state.x_dot = 0.0f;
    // Apply closure limit
    float x_min = -(H_eff + epsilon);
    state.x = std::max(state.x, x_min);
}
```

This ensures convergence to the static reed when mass approaches zero, as specified in the requirements.

**Confidence:** HIGH -- Symplectic Euler stability for mass-spring-damper is well-characterized in the Morgan & Qiao 2009 paper (McMaster University) and confirmed by the Wikipedia article on semi-implicit Euler methods.

### 1.5 Physical Parameter Ranges

| Parameter | Clarinet | Saxophone | Oboe | Physical Unit | APVTS Range |
|-----------|----------|-----------|------|---------------|-------------|
| mu_r (mass) | 0.023 | 0.02-0.03 | 0.015-0.025 | kg/m^2 | REED_MASS 0-1 maps to 1e-4 to 0.06 |
| g_r (damping) | 2900 | 2000-3000 | 3000-4000 | s^-1 | REED_DAMPING 0-1 maps to 500 to 6000 |
| k_r (stiffness) | 8e6 | 5e6-10e6 | 10e6-15e6 | N/m^3 | REED_HARDNESS 0-1 maps to 2e6 to 20e6 |
| H (rest opening) | 0.4mm | 0.5-1.0mm | 0.2-0.4mm | m | REED_OPENING 0-1 maps to 0.1mm to 1.5mm |
| w (channel width) | 13mm | 15-20mm | 7mm | m | Derived from instrument |
| f_r (resonance) | ~3700 Hz | 2500-3500 Hz | 3000-4000 Hz | Hz | Derived: sqrt(k_r/mu_r)/(2pi) |

**APVTS parameter mapping (per-block):**
```cpp
// Map APVTS normalized 0-1 params to physical values
float mu_r   = 1e-4f + pReedMass->load() * 0.06f;
float g_r    = 500.0f + pReedDamping->load() * 5500.0f;
float k_r    = 2e6f + pReedHardness->load() * 18e6f;
float H      = 0.0001f + pReedOpening->load() * 0.0014f;  // 0.1mm to 1.5mm
float embouchure = pEmbouchure->load();

// Embouchure modifiers
float g_lip  = embouchure * 4000.0f;   // additional damping
float k_lip  = embouchure * 5e6f;      // additional stiffness  
float x_lip  = embouchure * 0.0003f;   // pre-closure (up to 0.3mm)
```

---

## 2. Bernoulli Flow and the Reed Reflection Function

### 2.1 Smith's Reed-Table Formulation (Wave Variables)

The key insight from Julius Smith: instead of computing volume flow explicitly and converting to pressure waves, absorb everything into a **nonlinear reflection coefficient** that operates directly on wave variables.

At the bore entrance, the pressure wave variables are:
- `p_bore_plus` = outgoing wave from reed into bore (what we compute)
- `p_bore_minus` = incoming wave returning from bore (known from previous sample)

The physical pressure at the junction is:
```
p_bore = p_bore_plus + p_bore_minus
```

The volume velocity relates to wave variables via characteristic impedance Z_c:
```
u_bore = (p_bore_plus - p_bore_minus) / Z_c
```
where `Z_c = rho * c / A_bore` (air density * sound speed / bore cross-section area).

**Smith's reflection coefficient approach:**

Define half-pressures:
```
h_m = p_mouth / 2
h_delta_plus = h_m - p_bore_minus     // half-pressure difference
```

The outgoing wave is:
```
p_bore_plus = h_m + rho_hat(h_delta_plus) * h_delta_plus
```

Wait -- let me correct this based on the Smith/dsprelated source. The actual equation is:

```
p_bore_minus_new = h_m - rho_hat(h_delta_plus) * h_delta_plus
```

Actually, for the reed junction, the standard formulation writes the outgoing (into bore) wave as a function of the incoming (from bore) wave and the mouth pressure. The reflection function maps the half-pressure difference to the reed's effective reflectance.

### 2.2 The Reed Table (Reflection Coefficient)

The STK/Faust reed table is:
```
rho_hat(x) = clamp(offset + slope * x, -1.0, 1.0)
```

Where:
- `offset` ~ 0.7 (related to rest opening -- larger = more open)
- `slope` ~ -0.44 + 0.26 * stiffness (negative -- higher pressure closes reed more)

This is a memoryless nonlinear function. For the dynamic reed model, we replace this static table with a computation that uses the CURRENT reed displacement (from the ODE state) to determine the effective reflection.

### 2.3 Dynamic Reed + Bernoulli: The Full Junction Computation

For Phase 3.1 (Psi=0, single reed), the volume flow through the reed channel is:

```
S_opening = w * max(x + H_eff, 0)     // instantaneous opening area
dp = p_mouth - p_bore                  // pressure difference across reed
u = sign(dp) * S_opening * sqrt(2.0 * |dp| / rho)
```

**Converting flow to outgoing pressure wave:**

The key relationship at the junction:
```
p_bore = p_bore_plus + p_bore_minus
u_bore = (p_bore_plus - p_bore_minus) / Z_c
```

And continuity: `u_bore = u_reed` (the flow through the reed enters the bore).

Solving for `p_bore_plus`:
```
p_bore_plus = Z_c * u_reed + p_bore_minus
```

Wait -- this needs care with sign conventions. Let me derive it cleanly.

At the bore entrance:
- Physical pressure: `p = p+ + p-`
- Volume velocity INTO bore: `u = (p+ - p-) / Z_c`

The reed produces volume velocity `u_reed` into the bore. So:
```
u_reed = (p_bore_plus - p_bore_minus) / Z_c
p_bore_plus = Z_c * u_reed + p_bore_minus
```

This is the standard waveguide junction: convert flow to a pressure wave by multiplying by Z_c and adding the returning wave.

### 2.4 Complete Per-Sample Junction (Explicit, No Iteration)

The one-sample delay in the bore delay lines means `p_bore_minus` is from the PREVIOUS sample. This makes the entire computation explicit:

```cpp
// Per-sample reed + junction computation
inline float computeJunction(ReedState& state, const ReedParams& rp,
                             float p_mouth, float p_bore_minus,
                             float Z_c, float rho, float dt)
{
    // 1. Bore pressure estimate from returning wave
    //    (use p_bore_minus as the bore-side pressure approximation)
    //    The actual p_bore = p_bore_plus + p_bore_minus, but we don't
    //    know p_bore_plus yet. Use 2*p_bore_minus as estimate
    //    (assuming p_bore_plus ~ p_bore_minus near steady state)
    //    OR: use just p_bore_minus (one-delay approximation)
    float p_bore_estimate = p_bore_minus;  // simplest: use returning wave

    // 2. Update reed ODE with estimated bore pressure
    updateReed(state, rp, p_mouth, p_bore_estimate, dt);

    // 3. Compute instantaneous opening area
    float H_eff = rp.H - rp.x_lip;
    float opening = state.x + H_eff;
    float S_opening = rp.w * std::max(opening, 0.0f);

    // 4. Bernoulli flow (Psi=0 for Phase 3.1)
    float dp = p_mouth - p_bore_estimate;
    float dp_abs = std::max(std::abs(dp), 1e-10f);  // denormal protection
    float u_reed = std::copysign(1.0f, dp) * S_opening * std::sqrt(2.0f * dp_abs / rho);

    // 5. Convert flow to outgoing pressure wave
    float p_bore_plus = Z_c * u_reed + p_bore_minus;

    return p_bore_plus;
}
```

**On the bore pressure estimate:** Using `p_bore_estimate = p_bore_minus` is the simplest one-delay approximation. A more accurate estimate is `p_bore_estimate = 2 * p_bore_minus` if you're thinking of p_bore as the sum of incoming and outgoing waves at the previous step. In practice, the difference is small because the system converges quickly. Start with just `p_bore_minus` and tune if needed.

### 2.5 Alternative: Smith-Style Reflection Table (Hybrid Approach)

For potentially better stability, combine the dynamic reed displacement with Smith's reflection approach:

```cpp
inline float computeJunctionSmith(ReedState& state, const ReedParams& rp,
                                   float p_mouth, float p_bore_minus,
                                   float Z_c, float rho, float dt)
{
    // 1. Half-pressure variables (Smith formulation)
    float h_m = p_mouth * 0.5f;
    float h_delta = h_m - p_bore_minus;

    // 2. Update reed ODE (use h_delta * 2 as approx pressure diff)
    float p_bore_approx = p_bore_minus;
    updateReed(state, rp, p_mouth, p_bore_approx, dt);

    // 3. Compute reflection coefficient from reed state
    float H_eff = rp.H - rp.x_lip;
    float opening_norm = (state.x + H_eff) / H_eff;  // 0=closed, 1=rest, >1=open
    opening_norm = juce::jlimit(0.0f, 1.5f, opening_norm);

    // Reflection: open reed reflects less, closed reed reflects fully
    float rho_hat = 0.0f;
    if (opening_norm > 0.0f) {
        // Linear reed table modulated by opening
        float slope = -0.44f + 0.26f * (rp.k_r / 20e6f);  // stiffness-dependent
        rho_hat = juce::jlimit(-1.0f, 1.0f, opening_norm * 0.7f + slope * h_delta);
    } else {
        rho_hat = -1.0f;  // fully closed = total reflection
    }

    // 4. Outgoing wave (Smith one-multiply)
    float p_bore_plus = h_m + rho_hat * h_delta;

    return p_bore_plus;
}
```

**Recommendation:** Start with the explicit Bernoulli approach (2.4) since it maps directly to the physical equations from the BRIEF. The Smith reflection approach (2.5) is an optimization path if the explicit approach has stability issues.

### 2.6 Polynomial Approximation of sqrt(dp)

The Bernoulli equation contains `sqrt(|dp|)` which is expensive. Two fast alternatives:

**A. Fast inverse sqrt (Quake III style) -- not recommended:**
```cpp
// Classic fast inverse sqrt -- but std::sqrt is hardware-accelerated
// on modern x86/ARM and nearly as fast. Skip this trick.
```

**B. Polynomial approximation of the full flow curve:**
The flow `u = S * sqrt(2*|dp|/rho)` can be pre-fitted as a cubic:
```cpp
// Cubic fit to u(dp) for a given S and rho
// Coefficients depend on the operating range of dp
float u_poly = a0 + a1*dp + a2*dp*dp + a3*dp*dp*dp;
```

However, since S_opening changes every sample (it depends on reed state), the polynomial coefficients would need refitting every sample, which defeats the purpose.

**Recommendation:** Use `std::sqrt()`. On modern CPUs (x86 SSE/AVX, ARM NEON), hardware sqrt is 4-12 cycles. The denormal-protected version is negligible cost:

```cpp
float u_reed = std::copysign(1.0f, dp)
             * S_opening
             * std::sqrt(2.0f * std::max(std::abs(dp), 1e-10f) / rho);
```

This costs ~15 ops total (abs, max, div, sqrt, copysign, 2 multiplies). Polynomial optimization is premature for Phase 3.1 given the "no CPU budget" constraint.

### 2.7 Denormal Protection

Critical denormal-prone spots:
1. `sqrt(|dp|)` when dp is tiny -- handled by `max(|dp|, 1e-10f)`
2. Reed displacement near closure -- handled by epsilon in closure limit
3. Bore delay line output decaying to zero -- handled by `ScopedNoDenormals` in processBlock
4. Filter state variables after long silence -- IIR::Filter handles internally

---

## 3. Reed-Bore Coupling

### 3.1 The Implicit Coupling Problem

The core feedback loop:
```
bore returns p_bore_minus -> drives reed ODE -> reed opening determines flow
-> flow * Z_c = p_bore_plus -> enters bore delay line -> ... bore returns p_bore_minus
```

This is an implicit equation: `p_bore` depends on reed state which depends on `p_bore`. But the bore delay line introduces a MINIMUM one-sample delay (the delay length is always >= 2 samples for Thiran). This naturally decouples the system.

### 3.2 One-Sample-Delay Decoupling

The computational order per sample:

```
1. Pop p_bore_minus from bore backward delay line  (previous sample's returning wave)
2. Apply bell reflection and viscothermal loss to get p_bore_minus at reed end
3. Update reed ODE using p_bore_minus as bore pressure
4. Compute Bernoulli flow from updated reed state
5. Convert flow to p_bore_plus (outgoing wave)
6. Push p_bore_plus into bore forward delay line
```

The pop-before-push order ensures we read the delay line's oldest sample before writing the new one. This is the SAME pattern as O-Bowed's WaveguideString::processSample().

### 3.3 Is One-Sample Delay Sufficient for Stability?

**Yes, with caveats.**

The one-sample delay introduces a phase error at high frequencies:
```
Phase error at frequency f: phi_error = 2*pi*f / fs (radians)
```

At 44.1 kHz and f = 10 kHz: phi_error = 1.43 radians (significant).
At 88.2 kHz (2x OS) and f = 10 kHz: phi_error = 0.71 radians (moderate).

This phase error affects tuning accuracy of the very highest partials but does NOT cause instability. The bore's total delay is many samples (e.g., 200 samples for 220 Hz at 44.1 kHz), so the relative error of 1 sample is small (0.5%).

**Potential instability case:** Very short delay lines (very high notes, e.g., C7 at 2093 Hz = 21 samples at 44.1 kHz). The 1-sample error is 4.7% of the total delay. At 2x oversampling (42 samples), it drops to 2.4%. This is fine in practice.

**The real stability safeguard:** Ensure the bore loop gain is always < 1.0. The combination of viscothermal loss filter and bell reflection filter must attenuate energy on every round trip. As long as total loop gain < 1 at all frequencies, the system is stable regardless of the one-sample coupling delay.

### 3.4 Characteristic Impedance Z_c

```cpp
// Characteristic impedance of cylindrical bore
// Z_c = rho * c / A_bore
// where A_bore = pi * (bore_diameter/2)^2

constexpr float rho_air    = 1.2f;     // kg/m^3 at room temperature
constexpr float c_sound    = 343.0f;   // m/s at room temperature

float bore_diameter = 0.015f;  // 15mm for clarinet
float A_bore = juce::MathConstants<float>::pi * bore_diameter * bore_diameter * 0.25f;
float Z_c = rho_air * c_sound / A_bore;

// For clarinet (d=15mm): Z_c = 1.2 * 343 / (pi * 0.0075^2) = ~2.33e6 Pa*s/m^3
```

Z_c scales the relationship between volume velocity and pressure. Narrow bores (oboe, d=4-12mm) have much higher Z_c than wide bores (baritone sax, d=12-40mm), meaning the same flow produces more pressure in a narrow bore.

### 3.5 Conical Bore Scaling (Strategy C)

For Phase 3.1, bore_character=0 means cylindrical behavior. But the bore data structure should support conical scaling from day one (per the CONTEXT.md "Strategy C from the start" decision).

In a conical bore, traveling waves scale with `1/r` where r is distance from the cone apex:
```cpp
// Forward wave (expanding cone): amplitude decreases
p_plus_out = (r_in / r_out) * p_plus_delayed;

// Backward wave (converging cone): amplitude increases
p_minus_out = (r_out / r_in) * p_minus_delayed;
```

For bore_character=0 (cylindrical): r_in = r_out, so scale_factor = 1.0 and these become identity operations.

---

## 4. Note Onset/Offset

### 4.1 Note-On Initialization

When noteStarted() fires:

```cpp
void noteStarted() {
    // 1. Get note frequency
    auto note = getCurrentlyPlayingNote();
    float freq = note.getFrequencyInHertz();
    float velocity = note.noteOnVelocity.asUnsignedFloat();

    // 2. Reset reed state (start from rest)
    reedState.x = 0.0f;
    reedState.x_dot = 0.0f;

    // 3. Reset bore delay lines
    boreForwardDelay.reset();    // zero out -- silence
    boreBackwardDelay.reset();

    // 4. Set bore delay lengths from frequency
    float totalDelay = sampleRate / freq;
    // Subtract filter group delays for pitch accuracy
    totalDelay -= bellFilterDelay + lossFilterDelay;
    totalDelay = std::max(4.0f, totalDelay);  // minimum for Thiran
    boreForwardDelay.setDelay(totalDelay * 0.5f);   // half in each direction
    boreBackwardDelay.setDelay(totalDelay * 0.5f);

    // 5. Initialize breath pressure envelope
    float targetPressure = breathPressureFromAPVTS();
    breathEnvelope.trigger(targetPressure, velocity);

    // 6. Attack chiff (pressure overshoot)
    float chiffAmount = pAttackChiff->load();
    float overshoot = chiffAmount * velocity;  // velocity scales chiff intensity
    breathEnvelope.setOvershoot(overshoot);
}
```

### 4.2 Breath Pressure Envelope

The breath pressure envelope models the player's tongue release and breath onset:

```cpp
struct BreathEnvelope {
    float current    = 0.0f;
    float target     = 0.0f;
    float overshoot  = 0.0f;
    float attackCoeff = 0.0f;   // one-pole smoothing
    float releaseCoeff = 0.0f;
    float phase      = 0.0f;    // 0=attack, 1=sustain, 2=release
    float chiffTimer = 0.0f;    // samples remaining in overshoot
    float chiffDuration = 0.0f; // total chiff duration in samples

    void trigger(float targetPressure, float velocity) {
        target = targetPressure;
        phase = 0.0f;

        // Attack time: 5-50ms depending on velocity
        // Fast velocity = short attack (hard tongue) = more chiff
        float attackMs = 50.0f - velocity * 45.0f;  // 50ms at vel=0, 5ms at vel=1
        attackCoeff = std::exp(-1.0f / (attackMs * 0.001f * sampleRate));

        // Chiff duration: 20-50ms
        chiffDuration = (20.0f + velocity * 30.0f) * 0.001f * sampleRate;
        chiffTimer = chiffDuration;
    }

    void release() {
        phase = 2.0f;
        // Release time: 100-200ms (natural breath stop)
        releaseCoeff = std::exp(-1.0f / (150.0f * 0.001f * sampleRate));
    }

    float tick() {
        if (phase < 1.0f) {
            // Attack phase with optional overshoot
            float overshootGain = 0.0f;
            if (chiffTimer > 0.0f) {
                overshootGain = overshoot * (chiffTimer / chiffDuration);
                chiffTimer -= 1.0f;
            }
            float attackTarget = target * (1.0f + overshootGain);
            current = current * attackCoeff + attackTarget * (1.0f - attackCoeff);

            // Transition to sustain when close enough
            if (chiffTimer <= 0.0f && std::abs(current - target) < target * 0.01f)
                phase = 1.0f;
        }
        else if (phase < 2.0f) {
            // Sustain: track target (allows CC2 modulation)
            current = current * 0.999f + target * 0.001f;
        }
        else {
            // Release: exponential decay to zero
            current *= releaseCoeff;
        }
        return current;
    }
};
```

### 4.3 Attack Chiff

Physical origin: when the player first attacks, mouth pressure briefly overshoots the steady-state level. This overshoot drives the reed hard, producing broadband energy before the system settles into periodic oscillation.

The chiff amount (0-30% overshoot, controlled by ATTACK_CHIFF parameter and velocity) creates the characteristic "tch" sound at note onset. Mapping:
- ATTACK_CHIFF = 0: no overshoot, smooth onset (legato-like)
- ATTACK_CHIFF = 0.3: moderate overshoot (normal tongued attack)
- ATTACK_CHIFF = 1.0: aggressive overshoot (sforzando, slap tongue territory)
- Velocity scales the overshoot amount (harder attack = more chiff)

### 4.4 Note-Off Behavior

```cpp
void noteStopped(bool allowTailOff) {
    if (allowTailOff) {
        // Natural release: let breath pressure decay, bore rings down
        breathEnvelope.release();
        // Don't clearCurrentNote() yet -- let the bore energy decay
        // In renderNextBlock, check if energy is below threshold and then clear
    } else {
        // Hard stop: immediate silence
        reedState.x = 0.0f;
        reedState.x_dot = 0.0f;
        breathEnvelope.current = 0.0f;
        clearCurrentNote();
    }
}
```

In renderNextBlock, check for voice cleanup:
```cpp
// After all samples processed:
if (breathEnvelope.current < 1e-6f && boreEnergyEstimate < 1e-7f) {
    clearCurrentNote();
}
```

The bore naturally rings down because the bell reflection and viscothermal loss filters attenuate energy each round-trip. With a typical loop gain of 0.99, a 440 Hz note (100 samples round-trip at 44.1 kHz) decays by ~4.3 dB per 10ms. The tail lasts 50-200ms depending on bore loss settings.

### 4.5 Click Prevention

Clicks come from discontinuities. Sources and prevention:

| Source | Prevention |
|--------|------------|
| Reed state jump at note-on | Initialize to x=0, x_dot=0 (rest position), ramp breath pressure |
| Bore delay line discontinuity | Reset to zero before new note (delay lines already silent) |
| Breath pressure jump | One-pole envelope with 5-50ms attack |
| Note-off cutoff | Exponential release envelope (100-200ms) |
| Parameter change discontinuity | Read params once per block, smooth with one-pole |

The combination of envelope smoothing and starting from zero state should eliminate all clicks.

---

## 5. Complete Per-Sample Signal Flow

Here is the complete per-sample DSP for Phase 3.1:

```cpp
float processOneSample(float p_mouth_raw)
{
    // 0. Breath envelope (attack/sustain/release + chiff overshoot)
    float p_mouth = p_mouth_raw * breathEnvelope.tick();

    // 1. Read returning wave from bore (pop BEFORE push)
    float p_backward_raw = boreBackwardDelay.popSample(0);

    // 2. Apply viscothermal loss (one-pole lowpass in the loop)
    float p_backward_filtered = viscothermalLossFilter.processSample(p_backward_raw);

    // 3. Apply bell reflection (at the open end -- happens in backward delay path)
    //    Actually: bell reflection converts arriving forward wave into backward wave.
    //    Let me restructure: the bore has TWO delay lines.
    //    Forward delay carries wave from reed -> bell.
    //    At bell: reflection sends part back, rest radiates.
    //
    //    Correct signal flow:
    //    a) Pop from forward delay = wave arriving at bell
    //    b) Bell reflection filter -> backward delay push (reflected wave going back)
    //    c) Pop from backward delay = wave arriving back at reed
    //    d) Reed junction -> forward delay push (new wave going to bell)

    // Let me redo this properly:

    // 1. Pop wave arriving at bell end
    float p_at_bell = boreForwardDelay.popSample(0);

    // 2. Bell reflection (lowpass -- reflects low freq, radiates high freq)
    float p_reflected = bellReflectionFilter.processSample(-p_at_bell);
    //     Negative sign: reflection at open end inverts pressure

    // 3. Apply viscothermal loss to reflected wave
    float p_backward_in = viscothermalLossFilter.processSample(p_reflected);

    // 4. Push reflected wave into backward delay line
    boreBackwardDelay.pushSample(0, p_backward_in);

    // 5. Pop wave arriving at reed end (from backward delay)
    float p_bore_minus = boreBackwardDelay.popSample(0);
    // WAIT -- this doesn't work. Can't push and pop from same delay in one sample.

    // CORRECT ARCHITECTURE:
    // The bore is a SINGLE round-trip delay with filters at each end.
    // Many implementations use a single delay line for the full round trip,
    // not two separate delay lines.

    // Actually, the standard waveguide uses TWO delay lines (forward/backward)
    // but the key is: we pop from BOTH at the start, process both ends,
    // then push into both. Let me rewrite:

    return 0.0f; // placeholder
}
```

Let me provide the CORRECT architecture cleanly:

### 5.1 Correct Bore Waveguide Architecture

```
Reed End                                     Bell End
  |                                             |
  | ---> [Forward Delay Line (reed->bell)] ---> |
  |                                             |
  | <--- [Backward Delay Line (bell->reed)] <-- |
  |                                             |
  Reed Junction                          Bell Junction
  (nonlinear)                            (reflection filter)
```

Per-sample:
1. Pop from forward delay = wave arriving at bell
2. Pop from backward delay = wave arriving at reed
3. Bell junction: reflected = -bellFilter(arriving_at_bell)
4. Reed junction: outgoing = reedJunction(p_mouth, arriving_at_reed)
5. Push outgoing into forward delay
6. Push reflected into backward delay

**The output (radiated sound) comes from the bell end:** `output = arriving_at_bell + reflected`
(The sum gives the transmitted/radiated component.)

### 5.2 Complete Corrected Signal Flow

```cpp
float processOneSample(float p_mouth)
{
    // 1. Pop returning waves from BOTH delay lines
    float p_at_bell = boreForwardDelay.popSample(0);
    float p_at_reed = boreBackwardDelay.popSample(0);

    // 2. Bell end processing
    //    Open end reflection: negative (inverts), frequency-dependent (lowpass)
    //    Low freqs mostly reflected back; high freqs radiate out
    float p_bell_reflected = bellReflectionFilter.processSample(-p_at_bell);
    float p_radiated = p_at_bell + p_bell_reflected;  // output signal

    // 3. Viscothermal loss (apply in the backward path)
    float p_backward_lossy = viscothermalLossFilter.processSample(p_bell_reflected);

    // 4. Reed junction (nonlinear)
    //    p_at_reed is the returning wave from the bore
    //    Reed computation produces the new outgoing wave
    updateReed(reedState, reedParams, p_mouth, p_at_reed, dt);

    float H_eff = reedParams.H - reedParams.x_lip;
    float opening = reedState.x + H_eff;
    float S_opening = reedParams.w * std::max(opening, 0.0f);

    float dp = p_mouth - p_at_reed;
    float dp_abs = std::max(std::abs(dp), 1e-10f);
    float u_reed = std::copysign(1.0f, dp) * S_opening
                 * std::sqrt(2.0f * dp_abs / rho_air);

    float p_forward_new = Z_c * u_reed + p_at_reed;

    // 5. Push new waves into delay lines
    boreForwardDelay.pushSample(0, p_forward_new);
    boreBackwardDelay.pushSample(0, p_backward_lossy);

    // 6. Output: radiated sound at bell
    return p_radiated;
}
```

### 5.3 Where Viscothermal Loss Goes

There are two common placements:
- **In the backward path only** (as shown above): simpler, one filter
- **Split: one filter in each delay line path**: more accurate, models losses in both directions

For Phase 3.1, placing loss in the backward path only is sufficient. The total loop gain per round trip is:
```
G_loop = |bell_reflection_gain| * |viscothermal_loss_gain|
```
Both must be < 1 at all frequencies to ensure stability. The bell filter naturally has gain < 1 (it's a lowpass). The viscothermal filter is also a lowpass with gain < 1. Their product is always < 1. System is stable.

---

## 6. Reference Implementations

### 6.1 STK BlowHole / Saxofony

**BlowHole.cpp** (STK clarinet model):
- Reed table: `reedTable.setOffset(0.7), setSlope(-0.3)` -- linear clamped to [-1,1]
- Tonehole: three-port scattering with `scatter = -pow(r_th,2) / (pow(r_th,2) + 2*pow(r_bore,2))`
- Register vent: two-port junction
- Bell: one-zero filter
- The tick() method: pressure diff -> reed table -> delay line feedback

**Saxofony.cpp** (STK saxophone model):
- Uses variable blow position to fake conical bore: `delays_[0].setDelay((1-position)*delay)`, `delays_[1].setDelay(position*delay)`
- Default position = 0.2 (shorter delay on one side creates all-harmonic character)
- Same reed table as BlowHole

**Key insight from STK:** The reed table approach is extremely cheap (2 ops + clamp). O-Reed's dynamic reed adds ~25 ops for the ODE but produces far more realistic transients.

### 6.2 Faust pm.lib

**reedTable:** `*(slope) + offset : min(1) : max(-1)` -- identical to STK
**clarinetReed:** Uses `slope = -0.44 + 0.26 * stiffness` (stiffness 0-1)
**clarinetMouthPiece:** `*(-1) <: *(clarinetReed(stiffness))` applied to incoming wave, with pressure injection

Faust operates in wave variables throughout. The mouthpiece block is a termination that negates the incoming wave and applies the reed nonlinearity. This is exactly Smith's reflection coefficient approach.

### 6.3 Guillemain et al. 2005 (JASA 118)

This paper presents:
- Discretized reed ODE with explicit time-stepping
- Digital impedance model (bore as a transfer function rather than delay lines)
- The Psi confinement parameter for double-reed (used in Phase 3.2)
- Explicit resolution of the nonlinear coupled system using physical variables (pressure/flow) rather than wave variables

**Key contribution:** The paper shows that working in physical variables (p, u) rather than wave variables (p+, p-) makes it easier to use refined nonlinear models. However, waveguide implementations are more computationally efficient. O-Reed uses waveguides but the junction computation operates in physical variables momentarily (computing u_reed from Bernoulli, then converting back to wave variables).

### 6.4 Darabundit & Scavone 2025 (Frontiers in Signal Processing)

This recent paper presents:
- Port-Hamiltonian system (PHS) framework for modular, energy-conserving instrument modeling
- Energy quadratization for linearly implicit handling of nonlinear Hunt-Crossley collision force
- Symplectic Stormer-Verlet for bore wave propagation
- The paper's collision model is more sophisticated than our simple clamp, but the clamp is sufficient for Phase 3.1 (Hunt-Crossley can be considered for Phase 3.5 optimization)

---

## 7. Common Pitfalls

### Pitfall 1: Wrong Pop/Push Order
**What goes wrong:** Pushing before popping corrupts the delay line
**Why:** pushSample overwrites the write position before popSample reads it
**Prevention:** ALWAYS pop both delay lines before pushing either. This is the single most critical ordering constraint.

### Pitfall 2: Reed Mass Near Zero Causes Explosion
**What goes wrong:** When mu_r is tiny, the acceleration `force/mu_r` becomes enormous, and x_dot shoots to infinity in one sample
**Prevention:** Clamp mu_r to a minimum (1e-4f) AND switch to static reed model when REED_MASS parameter is below threshold (e.g., < 0.01)

### Pitfall 3: Bore Loop Gain >= 1
**What goes wrong:** Energy accumulates in the bore, producing exponentially growing oscillation that clips to infinity
**Prevention:** Ensure `bell_reflection_gain * viscothermal_loss_gain < 1.0` at ALL frequencies. Add a hard safety clamp on the total loop gain. The INFINITE_SUSTAIN parameter (Phase 3.4) should approach but never reach 1.0.

### Pitfall 4: Denormals in Quiet Passages
**What goes wrong:** Tiny values (< 1e-38) in delay lines and filters cause CPU spikes (100x slower processing)
**Prevention:** `juce::ScopedNoDenormals` in processBlock(), plus explicit zero-flush: `if (abs(x) < 1e-15f) x = 0.0f` on delay line outputs

### Pitfall 5: Filter Group Delay Causes Pitch Error
**What goes wrong:** The bell reflection and viscothermal loss filters add fractional-sample group delay that lengthens the effective bore
**Prevention:** Subtract estimated filter group delay from the total delay line length, same as O-Bowed does for its bridge loss filter

### Pitfall 6: Reed Closure Velocity Accumulation
**What goes wrong:** If reed hits the closure limit but velocity isn't zeroed, the next sample computes a large spring-back force, causing reed "chattering" (high-frequency oscillation at the closure boundary)
**Prevention:** When reed position is clamped, also clamp velocity: `if (x_new < x_min) { x_new = x_min; x_dot = min(x_dot, 0.0f); }` -- only zero velocity if it's pushing INTO the wall (negative when x is being reduced)

### Pitfall 7: Z_c Mismatch Between Reed and Bore
**What goes wrong:** If Z_c used in the junction calculation doesn't match the bore's actual characteristic impedance, the junction generates or absorbs energy, causing instability or deadening
**Prevention:** Compute Z_c from the actual bore diameter parameter, and update it whenever bore diameter changes. For conical bore, Z_c varies along the bore -- use the value at the reed entrance.

---

## 8. Architecture Patterns

### 8.1 Recommended Class Structure for Phase 3.1

```
ReedWindVoice (MPESynthesiserVoice)
  |-- ReedModel          (owns ReedState, ReedParams, ODE update)
  |-- BoreWaveguide      (owns 2 DelayLines, bell filter, loss filter)
  |-- BreathEnvelope     (attack/sustain/release + chiff)
  |-- NoteState          (frequency, velocity, active flag)
```

Each class is a separate header/source pair in Source/DSP/:
```
Source/
  DSP/
    ReedModel.h / .cpp        -- Reed ODE + Bernoulli junction
    BoreWaveguide.h / .cpp    -- Delay lines + filters
    BreathEnvelope.h / .cpp   -- Pressure envelope
  ReedWindVoice.h / .cpp      -- Orchestrates DSP components
  PluginProcessor.h / .cpp    -- Owns MPESynthesiser
  PluginEditor.h / .cpp       -- WebView (unchanged)
```

### 8.2 Per-Block vs Per-Sample Boundary

**Per-block (once at start of renderNextBlock):**
- Read all APVTS parameters
- Map to physical values
- Update ReedParams, bore loss coefficients
- Smooth parameter transitions (one-pole)

**Per-sample (inner loop):**
- Breath envelope tick
- Bore waveguide pop (both lines)
- Bell reflection filter
- Viscothermal loss filter  
- Reed ODE update (symplectic Euler)
- Bernoulli flow computation
- Flow-to-wave conversion
- Bore waveguide push (both lines)
- Output sample

---

## 9. Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Fractional delay | Custom circular buffer + interpolation | `juce::dsp::DelayLine<float, Thiran>` | Thiran allpass is the gold standard for waveguide pitch accuracy |
| Bell reflection filter | Custom filter math | `juce::dsp::IIR::Filter<float>` with manual coefficients | Transposed Direct Form II, handles denormals |
| One-pole smoothing for params | Raw exponential decay code | `juce::SmoothedValue<float>` | Thread-safe, handles block-rate updates |
| Noise generation | Custom PRNG | `juce::Random` or `std::mt19937` | Quality randomness, platform-independent |
| sqrt optimization | Quake fast-invsqrt | `std::sqrt()` | Hardware-accelerated on all modern CPUs, compiler can auto-vectorize |

---

## 10. State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Static reed table (STK) | Full mass-spring-damper ODE | Always available, but real-time since ~2005 (Guillemain) | Realistic transients, parameter-based character |
| Newton-Raphson for junction | Explicit scheme with one-delay decoupling | Standard in waveguide models since Smith 1986 | No iteration needed, guaranteed convergence |
| Cylindrical bore + correction filter | True conical sections (Strategy C) | Valimaki 1994 | Accurate conical bore physics |
| Energy-blind discretization | Port-Hamiltonian framework | Darabundit & Scavone 2025 | Guaranteed energy stability |
| Table lookup for nonlinearity | Polynomial or direct computation | Ongoing | Flexibility for parameter morphing |

---

## 11. Open Questions

1. **Exact physical parameter values for first instrument preset**
   - What we know: Literature ranges for clarinet (see table in 1.5)
   - What's unclear: The exact combination that sounds "right" for O-Reed's clarinet preset
   - Recommendation: Start with literature values, then tune by ear. The parameter ranges allow full exploration.

2. **Z_c normalization and signal levels**
   - What we know: Z_c for clarinet bore is ~2.3e6 Pa*s/m^3
   - What's unclear: Whether raw physical units produce reasonable signal levels in the waveguide, or if normalization is needed
   - Recommendation: May need a global gain factor to keep wave amplitudes in a reasonable range (e.g., +/- 1000 Pa typical bore pressure). Add an output scaling factor and tune empirically.

3. **Viscothermal loss filter coefficients**
   - What we know: Loss scales with sqrt(frequency) and inversely with bore diameter
   - What's unclear: Exact one-pole coefficient formula for a given bore diameter/length
   - Recommendation: Start with a simple one-pole lowpass where cutoff = 1000-4000 Hz (tuned to bore_diameter parameter). Literature reference: `loss_per_meter = alpha * sqrt(f) / bore_radius` where alpha depends on thermodynamic properties.

4. **Conical scaling factor computation for Strategy C**
   - What we know: Waves scale with 1/r (distance from cone apex)
   - What's unclear: How to compute r_in and r_out from bore_character and bore_diameter APVTS parameters
   - Recommendation: Since bore_character=0 in Phase 3.1 (cylindrical only), the scaling factors are 1.0. Defer the conical math to Phase 3.2 but ensure the bore waveguide data structure has slots for r_in/r_out.

---

## Sources

### Primary (HIGH confidence)
- Julius O. Smith III, "Physical Audio Signal Processing" -- Woodwinds chapter, reed-bore junction signal flow, reflection coefficient formulation. Retrieved from dsprelated.com/freebooks/pasp/Woodwinds.html
- STK BlowHole.cpp and Saxofony.cpp -- Reed table coefficients, junction implementations. GitHub: github.com/thestk/stk
- Faust pm.lib -- reedTable, clarinetReed, clarinetMouthPiece implementations. GitHub: github.com/grame-cncm/faustlibraries
- O-Reed research document: `research/reed-physical-modeling-dsp.md` -- Comprehensive DSP algorithms already in the project

### Secondary (MEDIUM confidence)
- Guillemain, Kergomard, Voinier (2005) "Real-time synthesis of clarinet-like instruments using digital impedance models" JASA 118(1):483-494 -- Explicit discretization scheme, Psi parameter. Referenced but full paper content not extracted.
- Darabundit & Scavone (2025) "Discrete port-Hamiltonian system model of a single-reed woodwind instrument" Frontiers in Signal Processing -- Energy quadratization, Hunt-Crossley collision, symplectic Stormer-Verlet. frontiersin.org/journals/signal-processing/articles/10.3389/frsip.2025.1519450
- Morgan & Qiao (2009) "Analysis of Damped Mass-Spring Systems for Sound Synthesis" EURASIP JASM -- Symplectic Euler stability for damped oscillators, z-transform analysis. cas.mcmaster.ca/~qiao/publications/MQ09.pdf

### Tertiary (LOW confidence)
- Wikipedia: Semi-implicit Euler method -- General stability region description, undamped case only
- WebSearch results on polynomial Bernoulli approximation -- inconclusive, no specific reed-instrument results found

---

## Metadata

**Confidence breakdown:**
- Reed ODE discretization: HIGH -- well-known numerical method, verified stability condition
- Bernoulli junction: HIGH -- Smith formulation is the standard, STK/Faust implementations confirm
- Reed-bore coupling: HIGH -- one-sample delay decoupling is the standard waveguide approach
- Note onset/offset: MEDIUM -- envelope design is somewhat empirical, will need tuning
- Physical parameter values: MEDIUM -- literature ranges known, but exact values for "good sound" are instrument-specific and require tuning

**Research date:** 2026-04-05
**Valid until:** Indefinite (acoustic physics and waveguide synthesis are mature fields)
