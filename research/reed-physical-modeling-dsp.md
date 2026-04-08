---
title: "O-Reed: Reed Instrument Physical Modeling DSP Research"
created: 2026-04-04
juce_version: "8.0.4"
summary: "Comprehensive DSP research for a general-purpose reed wind instrument synthesizer covering single/double reed excitation, cylindrical and conical bore waveguides, nonlinear reed-bore coupling, tone holes, expressive control, and real-time implementation."
domain: dsp
type: deep-research
keywords:
  - physical-modeling
  - reed
  - waveguide
  - clarinet
  - saxophone
  - oboe
  - wind-instrument
  - nonlinear
  - o-reed
stages: [0]
agents: [research]
---

# O-Reed: Reed Instrument Physical Modeling DSP Research

## Design Philosophy

O-Reed is a general-purpose reed wind instrument synthesizer. Instrument identity (clarinet, saxophone, oboe, duduk, shehnai, zurna, hichiriki, or impossible fantasy reeds) emerges from a continuous parameter space -- not from separate code paths or instrument-locked modes. The same engine that produces a convincing Bb clarinet can morph continuously into a soprano saxophone, then into a zurna, then into something that has never existed.

This matches the O-Bowed philosophy: **presets are starting points, not boundaries.**

---

## 1. Reed Excitation Models

### 1.1 The Single Reed (Clarinet, Saxophone)

The single reed is a thin piece of cane (or synthetic material) clamped to a mouthpiece. It vibrates against a fixed facing, alternately opening and closing the air channel. The canonical model treats the reed as a **mass-spring-damper** system driven by the pressure difference between the mouth cavity and the bore inlet.

**Reed displacement equation (lumped model):**

```
mu_r * d2x/dt2 + g_r * dx/dt + k_r * x = (p_mouth - p_bore) * A_reed
```

Where:
- `mu_r` = effective reed mass per unit area (~0.023 kg/m^2 for clarinet)
- `g_r` = damping coefficient (~2900 s^-1 for clarinet)
- `k_r` = reed stiffness (determines resonance frequency)
- `x` = reed tip displacement from equilibrium
- `omega_r = sqrt(k_r / mu_r)` = reed angular resonance frequency
- `f_r = omega_r / (2*pi)` = reed resonance frequency (~3700 Hz for clarinet)
- `A_reed` = effective driving area of the reed

**Typical parameter ranges for different instruments:**

| Parameter | Clarinet | Saxophone | Oboe |
|-----------|----------|-----------|------|
| Reed resonance freq | ~3700 Hz | ~2500-3500 Hz | ~3000-4000 Hz |
| Reed damping | ~2900 s^-1 | ~2000-3000 s^-1 | Higher (stiffer) |
| Reed mass/area | ~0.023 kg/m^2 | ~0.02-0.03 kg/m^2 | Lower mass |
| Rest opening (H) | ~0.4 mm | ~0.5-1.0 mm | ~0.2-0.4 mm |
| Channel width (w) | ~13 mm | ~15-20 mm | ~7 mm |

**Static vs. dynamic reed:** When the reed resonance frequency is well above the playing range (as in clarinet, where f_r ~ 3700 Hz vs. playing range 150-1500 Hz), the reed mass can be neglected and the reed treated as a memoryless spring. This "static reed" simplification is what STK and most real-time models use -- the reed responds instantaneously to pressure changes. Including reed dynamics (mass and damping) adds realism to attack transients and high-register behavior but costs more CPU.

**For O-Reed:** Use the full mass-spring-damper model. The extra cost is modest (one second-order filter per voice) and the payoff in transient realism and parameter range is significant. When the user dials reed mass to near-zero, we converge on the static reed model naturally.

### 1.2 Reed Channel Flow (Bernoulli Model)

Air flows through the slit between reed and mouthpiece facing. The quasi-static Bernoulli equation relates flow velocity to the pressure drop across the reed channel:

```
p_mouth = p_bore + (rho / 2) * v^2
```

Volume flow through the reed channel:

```
u(t) = w * max(x(t) + H, 0) * sign(p_mouth - p_bore) * sqrt(2 * |p_mouth - p_bore| / rho)
```

Where:
- `w` = width of the reed channel
- `H` = rest opening of the reed (equilibrium gap)
- `x(t)` = reed displacement (negative = closing)
- `rho` = air density (~1.2 kg/m^3)
- The `max(..., 0)` enforces reed closure -- when `x(t) + H <= 0`, the reed has fully closed against the mouthpiece and flow drops to zero

**The closing pressure threshold:** When mouth pressure exceeds a critical value `p_close = k_r * H / A_reed`, the reed is forced fully shut by static pressure alone. Above this threshold, the reed "beats" against the mouthpiece -- it closes during part of each cycle, producing the characteristic bright tone of reed instruments. Below this threshold, the reed never fully closes (subtone territory for saxophone).

**The nonlinear characteristics curve:** Plotting volume flow `u` vs. pressure difference `delta_p = p_mouth - p_bore` yields the characteristic S-shaped curve that defines reed behavior. This curve has three regions:

1. **Linear region** (small delta_p): Flow increases roughly linearly with pressure
2. **Peak flow** (around delta_p = p_close/3): Maximum volume flow
3. **Closing region** (delta_p approaching p_close): Reed closing reduces channel area faster than velocity increases, so flow decreases
4. **Closed** (delta_p >= p_close): Reed sealed against mouthpiece, zero flow

### 1.3 Double Reed Models (Oboe, Bassoon, Duduk, Zurna)

Double reeds consist of two symmetric blades bound together, forming a complete air channel. Key differences from single reeds:

**Coupled blade mechanics:**
- Two blades vibrate symmetrically (ideally) -- the opening area is proportional to `(x + H/2)` rather than `(x + H)` since both blades move
- Players can vary effective stiffness by applying horizontal lip forces that flatten the blades
- The rest opening is much smaller than single reeds (~0.2-0.4 mm for oboe vs ~0.4-1.0 mm for clarinet/sax)
- Reed width is narrower: oboe ~7 mm, bassoon ~13.5-16 mm

**Confined jet (the critical double-reed difference):** After passing through the narrow reed slit, the air jet enters a short conical staple/bocal before reaching the main bore. This confined jet experiences additional aerodynamic losses that are absent in single-reed instruments. Guillemain's model captures this with an additional loss parameter:

```
p_jet(t) = p_bore(t) + (rho * Psi / 2) * u(t)^2 / S_r^2
```

Where:
- `Psi` = confinement loss parameter (0 = no confinement losses, equivalent to single reed; >0 = increasing double-reed character)
- `S_r` = cross-sectional area of the reed channel outlet / staple entrance

This single parameter `Psi` is what makes a "double reed" sound like a double reed -- the confined jet creates additional spectral energy in the upper harmonics and affects the attack characteristics. In O-Reed, `Psi` becomes a continuous control: at 0 the excitation behaves as a single reed, increasing it morphs toward double-reed timbre.

**The full double-reed volume flow equation (Guillemain):**

```
u(t) = sign(p_m - p_r(t)) * alpha * S_i(t) * sqrt(2 * |p_m - p_r(t)| / (rho * (1 + Psi * alpha^2 * S_i(t)^2 / S_r^2)))
```

Where `alpha` is the vena contracta ratio (jet cross-section / reed opening, typically ~0.6-0.8).

### 1.4 Beating Reed vs. Free Reed

**Beating reeds** (clarinet, sax, oboe, etc.) move toward and beat against a fixed surface (mouthpiece facing for single reed, or the opposing blade for double reed). The reed is slightly wider than the opening, so it can seal the channel completely. Sound production depends on coupling to a resonant air column.

**Free reeds** (accordion, harmonica, harmonium) swing freely through a slot that is fractionally wider than the reed. They never seal the opening. Pitch is determined primarily by the reed's own physical properties (mass, length, stiffness) rather than by a coupled resonator.

**For O-Reed:** Focus exclusively on beating reeds. Free reeds are a fundamentally different excitation mechanism that would require separate modeling (no bore coupling, different flow equations). They could be a separate plugin (O-FreeReed) or an optional excitation mode in the future.

### 1.5 Reed Stiffness, Damping, and Mass: Timbral Effects

| Parameter | Low Value Effect | High Value Effect |
|-----------|-----------------|-------------------|
| **Stiffness (k_r)** | Soft, dark tone; easy onset; wider vibrato range | Bright, edgy tone; harder onset; more stable pitch |
| **Damping (g_r)** | Longer reed ring; more reed resonance coloring | Heavily damped; purer bore-driven tone |
| **Mass (mu_r)** | Fast response; reed follows bore instantly | Sluggish attack; reed resonance more prominent |
| **Rest opening (H)** | Small: easy to close, brighter, more harmonics | Large: harder onset, softer, fewer harmonics |

### 1.6 Lip Pressure / Embouchure Modeling

The player's lip contact adds:
- **Additional damping** to the reed (lip acts as a damper -- crucial for subtone)
- **Effective stiffness change** (biting harder raises effective stiffness)
- **Rest position offset** (lip force pre-closes the reed, changing the effective H)

In the model, embouchure is parameterized as:
```
g_eff = g_r + g_lip           // total damping
k_eff = k_r + k_lip           // total stiffness  
H_eff = H - x_lip             // reduced rest opening from lip pressure
```

This maps naturally to aftertouch/bite pressure on wind controllers.

---

## 2. Bore/Tube Models (Waveguide Approach)

### 2.1 Cylindrical Bore (Clarinet-like)

The clarinet has an approximately cylindrical bore (~14.5 mm diameter, ~60 cm effective length). A closed-end cylindrical pipe supports only odd harmonics of the fundamental:

```
f_n = (2n - 1) * c / (4L)    for n = 1, 2, 3, ...
```

This gives the clarinet its characteristic hollow, "woody" quality -- the even harmonics are attenuated (not absent, due to bore perturbations and the bell).

**Digital waveguide implementation:** Two delay lines carry right-going (p+) and left-going (p-) pressure wave samples:

```cpp
// Cylindrical bore waveguide - one sample step
float p_plus_out  = delayLine_forward.read();   // arriving at bell end
float p_minus_out = delayLine_backward.read();   // arriving at reed end

// Bell reflection (frequency-dependent)
float p_minus_new = bellReflectionFilter.process(p_plus_out);
float p_radiated  = p_plus_out + p_minus_new;  // transmitted = radiated sound

// Reed end: handled by nonlinear junction (see Section 3)
float p_plus_new = reedJunction(p_minus_out, p_mouth);

delayLine_forward.write(p_plus_new);
delayLine_backward.write(p_minus_new);
```

**Delay line length** determines pitch: `N = fs / f0 - filter_delay_compensation` where `fs` is sample rate and `f0` is fundamental frequency. Use fractional delay (allpass interpolation or Lagrange) for fine pitch control.

### 2.2 Conical Bore (Saxophone, Oboe, Bassoon)

Saxophones, oboes, and bassoons have conical bores. A conical bore supports all harmonics (both odd and even), giving these instruments a brighter, richer harmonic spectrum:

```
f_n = n * c / (2L)    for n = 1, 2, 3, ...
```

**The conical waveguide challenge:** In a cylinder, plane waves propagate with constant amplitude. In a cone, spherical waves propagate and amplitude scales with `1/r` (distance from cone apex). This creates several complications:

1. **Variable impedance:** Wave impedance in a cone is frequency-dependent: `Z(s) = (rho*c/A) * s/(s + 1/t_x)` where `t_x = x/c` is propagation time from the apex. In a cylinder, impedance is simply `rho*c/A`.

2. **Junction instabilities:** At points where taper angle changes (e.g., bore-to-bell transition), the reflection/transmission impulse responses contain growing exponentials. These are physically canceled by later reflections, but in a discrete model they cause numerical instability.

3. **Truncated cone problem:** Real instruments truncate the cone (the apex would be at the reed end). The missing conical tip volume is compensated by the reed's acoustic compliance.

**Three implementation strategies (in order of increasing accuracy and cost):**

#### Strategy A: "Fake Conical" (STK Saxofony approach)
Use a cylindrical waveguide but place the excitation at a variable position along the bore. At the midpoint, it sounds like a clarinet (odd harmonics). Closer to one end, it approximates conical behavior (all harmonics). This is computationally trivial but physically incorrect.

```cpp
// STK Saxofony: two delay lines with variable blow position
float bore_position = 0.2;  // 0.5 = clarinet, <0.5 = more sax-like
int delay1_len = total_delay * bore_position;
int delay2_len = total_delay * (1.0 - bore_position);
```

**Pros:** Cheap, simple, some timbral variation.
**Cons:** Doesn't capture the true conical physics. Timbre change is crude.

#### Strategy B: Cylindrical waveguide + spectral correction filter
Use a cylindrical waveguide but add a filter that boosts even harmonics to approximate conical behavior. The correction filter shape is derived from the ratio of conical to cylindrical impedance.

**Pros:** Moderate cost, better timbral accuracy than Strategy A.
**Cons:** Static approximation; doesn't capture the dynamic behavior of conical propagation.

#### Strategy C: True conical waveguide sections
Model the bore as cascaded truncated cone segments, each with proper spherical wave scaling:

```cpp
// Conical section: pressure scales with 1/r
float r_in = distance_from_apex_input;
float r_out = distance_from_apex_output;
float scale_factor = r_in / r_out;  // amplitude correction for expanding cone

// Forward-traveling wave through conical section
p_plus_out = scale_factor * delayLine.read();
// Backward-traveling wave (converging = amplitude increases)
p_minus_out = (1.0f / scale_factor) * delayLine_back.read();
```

At junctions between cone segments, use scattering matrices that account for impedance mismatch. The radiation impedance at the open end uses the spherical source approximation.

**Pros:** Physically accurate; captures all conical bore effects.
**Cons:** More complex, potential stability issues at taper transitions.

**Recommended for O-Reed:** Start with Strategy B (cylindrical + correction filter) for initial development, then upgrade to Strategy C for the conical bore path. Strategy A is too crude for a serious physical modeling synth.

### 2.3 Tone Hole Modeling

Tone holes are side openings in the bore that change the effective acoustic length and modify the impedance. Each tone hole acts as a **three-port scattering junction**: two ports connecting the bore sections on either side, and one port radiating to the outside.

**The Keefe tonehole model** (standard for waveguide synthesis): Each tone hole has a series impedance `Z_a` (inertance of air in the connecting channel) and a shunt impedance `Z_b` (dependent on whether the hole is open or closed):

- **Open hole:** `Z_b` is a radiation impedance (primarily inertive at low frequencies). The open hole creates a pressure node, effectively shortening the bore.
- **Closed hole:** `Z_b` is a compliance (the volume of air under the closed pad). A closed hole slightly flattens the pitch.

**Digital waveguide implementation:** Each tonehole is a three-port scattering junction implemented with two second-order digital filters (or, in the efficient Keefe formulation, a single first-order filter per hole):

```cpp
struct ToneHole {
    float scatter_coeff;    // scattering coefficient (open/closed state)
    OnePoleFilter filter;   // frequency-dependent behavior
    
    void process(float& p_plus_left, float& p_minus_left,
                 float& p_plus_right, float& p_minus_right,
                 float opening) {  // 0.0 = closed, 1.0 = fully open
        // Interpolate scattering coefficient based on opening
        float rho = lerp(closed_scatter, open_scatter, opening);
        
        // Three-port scattering
        float p_junction = rho * (p_plus_left + p_minus_right);
        p_minus_left = p_junction - p_plus_left;
        p_plus_right = p_junction - p_minus_right;
        
        // Radiated sound from open hole
        float p_radiated = filter.process((1.0f - rho) * (p_plus_left + p_minus_right));
    }
};
```

**Gradual hole opening for legato:** The `opening` parameter (0.0-1.0) enables smooth transitions between notes, simulating the gradual lifting of fingers. This is essential for realistic legato and half-holing effects.

**Computational cost:** Each tone hole adds ~5-10 multiplies per sample. A full clarinet (24 tone holes + register hole) costs ~150-250 multiplies per sample for the tone hole lattice alone.

**Simplified approach for O-Reed:** Rather than modeling every physical tone hole, use a **variable-length delay line** for basic pitch control and add 2-4 "virtual tone holes" for:
1. Cutoff frequency simulation (tone holes create a natural high-frequency rolloff)
2. Register hole (for overblowing behavior)
3. One or two open holes above the playing hole (for radiation character)

This reduces cost from ~24 tone holes to ~4, with a spectral shaping filter to compensate.

### 2.4 Register Hole and Overblowing

**Cylindrical bore (clarinet):** The register hole forces the instrument to jump to the **third harmonic** (a twelfth = octave + fifth). This is because a closed cylindrical pipe only supports odd harmonics, so the second resonance is the third harmonic.

**Conical bore (sax, oboe):** The register hole forces a jump to the **second harmonic** (an octave). Conical bores support all harmonics.

**Implementation:** The register hole is modeled as a special tone hole near the mouthpiece end of the bore. When opened, it introduces a pressure leak that suppresses the fundamental mode:

```cpp
float register_hole_opening = 0.0;  // 0 = closed, 1 = open

// Register hole acts as a small side branch near the reed
if (register_hole_opening > 0.0f) {
    float leak = register_hole_opening * register_scatter * p_bore_at_hole;
    p_bore_at_hole -= leak;  // reduce fundamental
}
```

**For O-Reed:** Expose register hole as a parameter. Combined with bore type (cylindrical/conical), this determines whether overblowing produces a twelfth or an octave.

### 2.5 Bell Radiation

The bell flare at the open end of the instrument creates a frequency-dependent reflection:
- **Low frequencies** are mostly reflected back into the bore (the bell is small relative to wavelength)
- **High frequencies** radiate efficiently into the room

The bell reflection filter is a **lowpass filter** (reflecting lows, transmitting highs):

```cpp
// Simple first-order bell reflection
// H_r(z) = -(a + z^-1) / (1 + a*z^-1)  where a ~ -0.9 to -0.99
float bell_reflection = bellFilter.process(p_arriving_at_bell);
float p_radiated = p_arriving_at_bell + bell_reflection;  // complementary
```

The `a` coefficient controls the cutoff frequency -- larger bells (saxophone) radiate more low-frequency energy than smaller bells (oboe). The bell also adds a directivity pattern: high frequencies radiate forward, low frequencies radiate omnidirectionally.

**For O-Reed:** Bell size/flare is a continuous parameter that controls both the reflection filter cutoff and the radiation characteristics. Small bell = more internal resonance, darker external sound. Large bell = brighter, more projected sound.

### 2.6 Multi-Segment Bore Profiles

Real instruments are not pure cylinders or cones. They have complex bore profiles:

- **Clarinet:** Cylindrical body + slight taper at the top + flared bell
- **Saxophone:** Conical body + parabolic bell flare
- **Oboe:** Very narrow cone (0.7 degree taper) + complex bell
- **Bassoon:** Folded narrow cone (0.4 degree taper) + minimal bell

Model this with cascaded waveguide sections, each with its own taper angle and loss characteristics. For O-Reed, parameterize the bore as:

```
bore_profile = [throat_diameter, body_taper, body_length, bell_flare, bell_length]
```

Where `body_taper = 0` is cylindrical, `body_taper > 0` is conical.

### 2.7 Viscothermal Losses

Air in the bore loses energy to thermal conduction at the walls and viscous friction. These losses are frequency-dependent (roughly proportional to `sqrt(f)`) and bore-diameter-dependent (narrower bores = more loss).

Model with a low-order IIR filter in each delay line loop:

```cpp
// Viscothermal loss filter - one-pole lowpass approximation
// Loss increases with sqrt(frequency) and inversely with bore diameter
float loss_coeff = exp(-alpha * sqrt(f) / bore_diameter);
```

Narrow bores (oboe, clarinet) have more viscothermal loss than wide bores (bass clarinet, baritone sax), contributing to their relatively quieter, more intimate sound.

---

## 3. Nonlinear Junction (Reed-Bore Coupling)

### 3.1 The Reflection Function Approach (Smith)

Julius Smith's formulation treats the reed as a **signal-dependent reflection coefficient** at the bore entrance. The incoming pressure wave from the bore (`p_bore+`) interacts with the mouth pressure to produce an outgoing wave:

```
p_bore- = rho_hat(h_delta+) * h_delta+  +  h_mouth
```

Where:
- `h_delta+ = p_mouth/2 - p_bore+` (half-pressure difference)
- `rho_hat(h_delta+)` = the "reed table" -- a nonlinear function mapping pressure difference to reflection coefficient
- `h_mouth = p_mouth / 2`

**The Reed Table function** (STK implementation):

```cpp
// STK ReedTable: piecewise linear with saturation
inline float reedTable(float input) {
    float output = offset + slope * input;
    return clamp(output, -1.0f, 1.0f);
}
// offset ~ 0.7 (controls rest opening)
// slope ~ -0.44 + 0.26 * reedStiffness (controls stiffness)
```

This is the simplest possible reed nonlinearity -- a linear function clamped to [-1, 1]. The clamp models reed closure (output hits -1 when reed is fully shut). Despite its simplicity, it produces surprisingly convincing reed tones.

### 3.2 Complete Single-Reed Waveguide Signal Flow

The per-sample computation for a basic clarinet model:

```cpp
void processOneSample(float mouth_pressure, float& output) {
    // 1. Calculate breath pressure with modulation
    float breath = mouth_pressure * envelope.tick();
    breath += breath * noiseGain * noise.tick();        // turbulence
    breath += breath * vibratoGain * vibrato.tick();     // lip vibrato
    
    // 2. Read returning wave from bore
    float p_bore_returning = -0.95f * lossFilter.tick(delayLine.lastOut());
    
    // 3. Pressure difference at reed
    float pressureDiff = breath - p_bore_returning;
    
    // 4. Reed nonlinearity
    float reedReflection = reedTable.tick(pressureDiff);
    
    // 5. New wave entering bore
    float p_bore_outgoing = breath - pressureDiff * reedReflection;
    
    // 6. Feed into delay line (bore)
    delayLine.tick(p_bore_outgoing);
    
    // 7. Output (radiated sound at bell)
    output = p_bore_returning + p_bore_outgoing;  // simplified
}
```

**Cost per voice:** ~15-20 multiplies + 1 delay line read/write + 1 filter + 1 table lookup = very cheap.

### 3.3 Solving the Implicit Nonlinear Equation

When the reed has dynamics (mass and damping), the coupling between reed and bore becomes implicit: the bore pressure depends on the reed position, which depends on the bore pressure. Three approaches:

**A. Table Lookup (fastest):**
Pre-compute the reed reflection for all possible pressure values and store in a lookup table. Linear interpolation between entries. Cost: 2 subtractions + 1 multiply + 1 table read per sample.

**B. Polynomial Approximation:**
Replace the table with a cubic polynomial fit to the reed characteristics:
```cpp
float reedPoly(float x) {
    // Cubic approximation to reed nonlinearity
    return a0 + a1*x + a2*x*x + a3*x*x*x;
}
```
Cost: 4 multiplies + 3 adds per sample. No memory access overhead.

**C. Newton-Raphson Iteration (most accurate):**
For the full dynamic reed model where mass creates a feedback loop:
```cpp
float x = x_prev;  // initial guess from previous sample
for (int i = 0; i < 3; i++) {  // typically converges in 2-3 iterations
    float f = reed_equation(x, p_bore, p_mouth);
    float df = reed_equation_derivative(x, p_bore, p_mouth);
    x -= f / df;
}
```
Cost: 3 iterations * ~10 ops = ~30 ops per sample. Use previous sample as initial guess for fast convergence.

**Recommended for O-Reed:** Use polynomial approximation (B) for the basic model, with Newton-Raphson (C) available when the full dynamic reed model is enabled. The table lookup (A) is fastest but least flexible for parameter morphing.

### 3.4 Mouthpiece Volume (Helmholtz Resonance)

The air volume inside the mouthpiece between the reed and the bore entrance acts as a Helmholtz resonator. For clarinet, this volume is approximately 60 microliters. Its acoustic compliance:

- Slightly lowers the playing frequency (by shortening the effective bore)
- Creates a secondary resonance that can color the upper harmonics
- In conical instruments (oboe, sax), the mouthpiece volume compensates for the truncated cone tip

Model as a single compliance element at the reed-bore junction:

```cpp
float mouthpiece_compliance = volume / (rho * c * c);
float p_mouthpiece = p_mouthpiece_prev + (u_reed - u_bore) / (mouthpiece_compliance * fs);
```

For O-Reed, expose mouthpiece volume as a parameter. Larger mouthpiece = lower pitch correction, more sub-resonance coloring.

### 3.5 Stability Considerations

The coupled reed-bore system can go unstable when:
1. Reed damping is too low (self-excited oscillation at reed frequency)
2. Bore loss is too low (energy accumulates without dissipation)
3. Numeric precision issues at extreme parameter values

**Safeguards:**
- Clamp reed displacement to physical range: `x >= -(H + lip_thickness)`
- Ensure bore loop gain < 1.0 (always include some loss)
- Use energy-preserving discretization (port-Hamiltonian or symplectic Euler) for the reed ODE
- Add a tiny amount of damping (epsilon) as a safety floor

---

## 4. Sound Quality Features

### 4.1 Tone Hole Lattice for Note Changes

For realistic note transitions, model at least 3-6 tone holes as three-port scattering junctions (see Section 2.3). Each hole has a continuous `opening` parameter (0.0 = fully closed, 1.0 = fully open) enabling:

- **Gradual legato transitions** (smooth frequency glide between notes)
- **Half-holing** (partially open holes for microtonal inflection)
- **Fork fingerings** (open holes below closed holes for alternate notes)
- **Cross-fingerings** (characteristic of baroque instruments)

**Simplified pitch model for O-Reed:**
Use a primary delay-line length for pitch, plus 2-4 tone hole junctions for spectral character. The tone holes create a natural cutoff frequency above which the bore stops reflecting (sound radiates out the holes). This cutoff frequency is instrument-specific:

| Instrument | Approximate cutoff frequency |
|------------|------------------------------|
| Oboe | ~1500 Hz |
| Clarinet | ~1500 Hz |
| Saxophone | ~800-1200 Hz |
| Bassoon | ~600 Hz |

### 4.2 Register Key Simulation

Implement as a switchable tone hole near the reed end:
```cpp
float register_key = 0.0f;  // 0 = first register, 1 = second register

// Cylindrical bore: overblows at the 12th (3rd harmonic)
// Conical bore: overblows at the octave (2nd harmonic)
if (register_key > 0.5f) {
    applyRegisterHoleLeak(bore_pressure_near_reed, register_key);
}
```

The register key position and size affect intonation of the upper register -- expose as a tweakable parameter for sound design.

### 4.3 Vibrato

Real woodwind vibrato comes from multiple sources:

- **Lip pressure modulation** (jaw vibrato -- most common for sax and clarinet)
- **Diaphragm/breath pressure modulation** (breath vibrato -- common for flute-type)
- **Throat/vocal tract modulation** (classical oboe vibrato)

Model all three as LFO modulation of different parameters:

```cpp
float vibrato_lfo = sin(2.0f * PI * vibrato_freq * t) * vibrato_depth;

// Lip vibrato: modulates reed parameters
float H_vibrato = H_eff + vibrato_lfo * lip_vibrato_amount;

// Breath vibrato: modulates mouth pressure
float p_mouth_vibrato = p_mouth * (1.0f + vibrato_lfo * breath_vibrato_amount);

// Throat vibrato: modulates bore impedance (upstream)
float throat_impedance_mod = 1.0f + vibrato_lfo * throat_vibrato_amount;
```

Map MPE Y-axis or CC1 to vibrato depth; aftertouch or CC2 to vibrato rate.

### 4.4 Growl / Multiphonics (Vocal Fold Coupling)

Growling involves the player singing or humming into the instrument, coupling vocal fold vibrations with the reed/bore system. This creates amplitude modulation and intermodulation products.

**Model as a secondary oscillator coupled to the mouth pressure:**

```cpp
float vocal_freq;     // sung pitch (can differ from played pitch)
float vocal_coupling; // 0 = no growl, 1 = full coupling

// Vocal fold modulation of mouth pressure
float vocal_osc = sin(2.0f * PI * vocal_freq * t);
float p_mouth_growl = p_mouth * (1.0f + vocal_coupling * vocal_osc * 0.3f);

// Strong coupling creates multiphonics (sum and difference tones)
```

When vocal frequency is near the played frequency, beating occurs (slow amplitude modulation). When far apart, distinct sum/difference tones create multiphonic textures. The vocal fold coupling strength controls how aggressively the growl affects the tone.

### 4.5 Key Click / Pad Noise

Mechanical transients from key mechanisms:
- **Click on closure:** Short impulse when pad hits the tone hole rim
- **Click on release:** Shorter, quieter impulse when pad lifts
- **Pad resonance:** The pad itself has a small acoustic cavity that rings briefly

```cpp
void triggerKeyClick(int hole_index, bool is_closing) {
    float click_amplitude = is_closing ? 0.15f : 0.05f;
    float click_freq = 2000.0f + random(-500.0f, 500.0f);  // randomized
    float click_decay = 0.001f;  // very short, ~1ms
    
    // Inject filtered noise burst into the bore at the tone hole location
    keyClickEnvelope[hole_index].trigger(click_amplitude, click_decay);
}
```

This adds significant realism during fast passages. Keep it subtle -- it should be felt more than heard.

### 4.6 Breath Noise (Turbulence)

Air turbulence in the mouthpiece and bore produces broadband noise that is especially audible during:
- Attack transients (the initial "chiff")
- Soft playing (subtone -- air noise becomes a significant component)
- Breathy playing styles

**Model as filtered noise mixed into the excitation:**

```cpp
float turbulence = noiseGenerator.tick();

// Shape noise spectrum: more energy at higher frequencies
turbulence = breathNoiseFilter.process(turbulence);  // bandpass ~1-8 kHz

// Modulate noise amplitude with flow velocity
float noise_amplitude = noiseGain * abs(volume_flow) * breathNoiseMix;

// Add to excitation signal
float excitation = reed_signal + noise_amplitude * turbulence;
```

The noise level scales with flow velocity (faster air = more turbulence). At very soft playing levels, the noise-to-tone ratio increases, creating the characteristic breathy quality.

### 4.7 Attack Transients (Chiff)

The initial attack of a reed note has a characteristic "chiff" -- a burst of broad-spectrum energy before the reed locks into periodic oscillation. This transient is where much of the instrument's character lies.

**Physical origin:** When the player first applies breath pressure, the reed displacement grows exponentially from noise until it reaches the nonlinear saturation region (reed beating against mouthpiece). During this growth phase, the signal contains strong broadband energy.

**The attack is controlled by:**
- Rate of pressure increase (fast tongue attack = more chiff)
- Reed stiffness (harder reed = more pronounced chiff)
- Mouth pressure overshoot (attacking above steady-state pressure creates a bright burst)

```cpp
// Attack transient modeling
float attack_rate;     // how quickly mouth pressure reaches target
float attack_overshoot; // temporary pressure boost at note onset (0-0.3)

// During note onset (first ~20-50ms)
float p_mouth_attack = p_mouth_target * (1.0f + attack_overshoot * attackEnvelope.tick());
```

### 4.8 Overblowing Behavior

As mouth pressure increases beyond normal playing range, the instrument jumps to higher registers:

1. **First register:** Fundamental mode
2. **Second register:** 3rd harmonic (cylindrical bore) or 2nd harmonic (conical bore)
3. **Third register and beyond:** Higher modes, increasingly unstable

The model naturally produces overblowing when the excitation amplitude exceeds the stability region of the current mode, causing the system to jump to the next stable mode. No special code is needed -- the waveguide physics handles this if the nonlinearity is correctly implemented.

### 4.9 Subtone (Saxophone Technique)

Subtone occurs when the reed never fully closes during oscillation, producing a soft, airy tone with:
- Weak upper harmonics (spectral centroid drops significantly above 3rd harmonic)
- Prominent breath noise component
- Warm, intimate character

**Model by controlling the effective reed opening:**
```cpp
// Subtone: increase lip damping, reduce mouth pressure relative to closing pressure
float subtone_amount;  // 0 = normal, 1 = full subtone
float g_lip_subtone = g_lip * (1.0f + subtone_amount * 5.0f);  // heavy damping
float p_mouth_subtone = p_mouth * (1.0f - subtone_amount * 0.3f);  // reduced pressure
float noise_gain_subtone = noiseGain * (1.0f + subtone_amount * 3.0f);  // more breath
```

The key: when `p_mouth < p_close` and lip damping is high, the reed oscillates without fully closing. The increased damping prevents the reed from building enough amplitude to beat against the mouthpiece.

---

## 5. Parametric Control Space

### 5.1 What Makes Each Instrument?

The core insight: every reed instrument is a point in a continuous parameter space. Here is what distinguishes the major instrument families:

| Parameter | Clarinet | Alto Sax | Soprano Sax | Oboe | Bassoon | Duduk |
|-----------|----------|----------|-------------|------|---------|-------|
| **Bore type** | Cylindrical | Conical | Conical | Conical (narrow) | Conical (narrow) | Cylindrical |
| **Bore diameter** | ~15mm | ~12-40mm | ~8-25mm | ~4-12mm | ~4-20mm | ~10mm |
| **Bore length** | ~60cm | ~65cm | ~35cm | ~32cm | ~250cm | ~35cm |
| **Reed type** | Single | Single | Single | Double | Double | Double |
| **Reed stiffness** | Medium | Medium-soft | Medium | Stiff | Medium-stiff | Soft |
| **Rest opening** | Medium | Large | Medium | Small | Small | Very small |
| **Confinement (Psi)** | 0 | 0 | 0 | 0.3-0.6 | 0.2-0.5 | 0.1-0.3 |
| **Bell size** | Medium flare | Large flare | Medium flare | Small flare | Minimal | None |
| **Tone hole cutoff** | ~1500 Hz | ~1000 Hz | ~1200 Hz | ~1500 Hz | ~600 Hz | ~2000 Hz |
| **Overblow interval** | 12th | Octave | Octave | Octave | Octave | 12th* |
| **Pitch range** | D3-C7 | Db3-Ab5 | Ab3-Eb6 | Bb3-A6 | Bb1-Eb5 | A3-B5 |

*Duduk has a cylindrical bore with double reed -- unique combination.

### 5.2 Non-Western Reed Instruments as Parameter Configurations

| Instrument | Region | Bore | Reed | Unique Character |
|------------|--------|------|------|-----------------|
| **Shehnai** | India | Conical, wide bell | Quadruple (4 reeds!) | Brilliant, nasal, very bright |
| **Suona** | China | Conical + metal bell | Double, loose | Piercing, very loud, wide vibrato |
| **Zurna** | Middle East | Conical | Double | Bright, outdoor character |
| **Hichiriki** | Japan | Cylindrical | Double, wide | Nasal, clarinet-like but with double reed |
| **Arghul** | Egypt | Dual cylindrical (drone + melody) | Single | Parallel bore drone effect |
| **Mijwiz** | Levant | Dual cylindrical | Single | Two-pipe unison/harmony |
| **Piri** | Korea | Cylindrical | Double, large | Loud, bright, wide range |
| **Duduk** | Armenia | Cylindrical | Double, very wide | Dark, mournful, human-voice quality |

The hichiriki is particularly interesting: cylindrical bore + double reed, a combination that produces a sound somewhere between clarinet and oboe. This naturally emerges from O-Reed's parameter space without special-casing.

### 5.3 Continuous Morphing Between Instrument Types

The parameter space enables smooth morphing that is impossible with sampled instruments:

```
Clarinet -> Saxophone: increase bore_taper (0 -> cone), increase H (opening), add bell_flare
Clarinet -> Duduk: decrease bore_diameter, decrease H, increase Psi, remove bell
Saxophone -> Oboe: decrease bore_diameter, increase Psi, decrease H, reduce bell
Oboe -> Zurna: increase bore_diameter, widen bell, reduce damping
```

All transitions are smooth -- there's no discontinuity in the parameter space.

### 5.4 Most Perceptually Meaningful Parameters to Expose

**Tier 1 -- Primary (always visible):**
1. **Breath Pressure** (maps to mouth pressure) -- dynamics control
2. **Embouchure / Bite** (maps to lip force / effective stiffness) -- timbre/brightness
3. **Reed Hardness** (maps to k_r) -- attack character and brightness
4. **Bore Character** (maps to bore_taper: 0=cylindrical, 1=full cone) -- harmonic content
5. **Pitch / Note** (maps to delay length + tone holes)

**Tier 2 -- Secondary (expandable panel):**
6. **Reed Opening** (H) -- ease of onset, dynamic range
7. **Bell Size** (reflection filter cutoff) -- projection, brightness
8. **Air Noise** (breath noise mix) -- breathiness
9. **Double Reed Amount** (Psi confinement parameter) -- single vs double reed character
10. **Bore Diameter** (viscothermal loss, impedance) -- intimacy vs projection

**Tier 3 -- Advanced / Sound Design:**
11. Reed Mass (mu_r) -- reed resonance effects
12. Reed Damping (g_r) -- reed ring / muting
13. Mouthpiece Volume -- pitch correction, sub-resonance
14. Tone Hole Cutoff -- spectral envelope
15. Register Hole -- overblowing behavior
16. Bore Profile (multi-segment) -- complex bore shapes

### 5.5 MIDI / MPE / Breath Controller Mapping

| Physical Input | MIDI Source | Physical Parameter |
|---------------|------------|-------------------|
| Breath | CC2 (Breath) / MPE Pressure | Mouth pressure (p_mouth) |
| Bite | CC1 (Mod) / MPE Slide (Y) | Lip force / embouchure |
| Lip position | Aftertouch / CC11 | Reed opening offset |
| Tongue attack | Velocity | Attack rate / overshoot |
| Vibrato speed | CC76 or custom | Vibrato LFO rate |
| Vibrato depth | CC77 or custom | Vibrato LFO depth |
| Growl | CC80 or custom | Vocal fold coupling |
| Pitch bend | Pitch Bend / MPE X | Delay line fine-tuning + reed H |

**Breath controller integration:** CC2 should map directly to mouth pressure with a configurable curve (linear, exponential, or S-curve). When no breath controller is detected, use velocity for initial attack and a sustain level for held notes, with CC11 (Expression) for dynamics.

**MPE mapping:** MPE is ideal for reed instruments:
- **Strike (Note On velocity):** Attack force / tongue articulation
- **Pressure (channel aftertouch):** Sustained breath pressure
- **Slide (Y-axis, CC74):** Embouchure / brightness
- **Glide (X-axis, pitch bend):** Pitch bend / vibrato

---

## 6. Specific Algorithm Recommendations

### 6.1 Algorithm Comparison

| Approach | Quality | CPU Cost | Flexibility | Best For |
|----------|---------|----------|-------------|----------|
| **STK Clarinet** | Basic | Very low | Limited | Learning, prototyping |
| **STK Saxofony** | Basic | Very low | Moderate (blow position) | Crude sax approximation |
| **Smith Waveguide** | Good | Low | Good | Cylindrical bore instruments |
| **Guillemain Impedance** | Very good | Medium | Excellent | Double reeds, research quality |
| **Port-Hamiltonian (2025)** | Excellent | High | Excellent | Maximum accuracy, stability guarantee |
| **Faust STK Models** | Good | Low-medium | Good | Rapid prototyping |

### 6.2 Recommended Architecture for O-Reed

**Base algorithm: Smith-style waveguide with Guillemain extensions.**

The core loop is a Smith digital waveguide (cheap, stable, proven) enhanced with:
1. Full mass-spring-damper reed model (Avanzini/van Walstijn formulation)
2. Guillemain's confined jet parameter (Psi) for double-reed support
3. Conical bore correction filter (upgradable to true conical sections)
4. Keefe tone hole junctions (2-4 virtual tone holes)
5. Frequency-dependent bell radiation filter

```
Signal Flow:
                                    +-----------+
                                    | Breath    |
                                    | Noise Gen |
                                    +-----+-----+
                                          |
+---------+   +--------+   +------+  +----v----+   +----------+
| Mouth   |-->| Reed   |-->| NL   |->| Bore    |-->| Bell     |--> Output
| Pressure|   | M-S-D  |   | Jctn |  | Waveguide|  | Radiation|
+---------+   +--------+   +--^---+  | + Tone  |  +----------+
      ^                       |      | Holes   |
      |                       +------| + Losses |
      |                              +----------+
+----------+
| Vibrato  |
| Growl    |
| Controls |
+----------+
```

**Per-sample pseudocode:**

```cpp
struct ReedVoice {
    // Reed state
    float x = 0, x_dot = 0;   // displacement, velocity
    
    // Bore
    FractionalDelay bore_forward, bore_backward;
    OnePoleFilter boreLoop_loss;
    OnePoleFilter bellReflection;
    
    // Tone holes
    ToneHole toneHoles[4];
    
    // Excitation
    ReedTable reedNL;
    NoiseGenerator breathNoise;
    
    float tick(float p_mouth, float embouchure, const ReedParams& params) {
        // 1. Reed dynamics (mass-spring-damper)
        float p_bore_in = bore_backward.read();  // returning wave from bore
        float delta_p = p_mouth - p_bore_in;
        
        float force = delta_p * params.A_reed;
        float x_ddot = (force - params.g_eff * x_dot - params.k_eff * x) / params.mu_r;
        x_dot += x_ddot / sampleRate;
        x += x_dot / sampleRate;
        
        // Clamp reed displacement (can't go past mouthpiece)
        x = max(x, -(params.H_eff + 0.001f));
        
        // 2. Volume flow through reed channel (Bernoulli + confinement)
        float opening = max(params.w * (x + params.H_eff), 0.0f);
        float sign_dp = (delta_p >= 0) ? 1.0f : -1.0f;
        float flow_denom = params.rho * (1.0f + params.Psi * opening * opening 
                           / (params.S_r * params.S_r));
        float u = sign_dp * opening * sqrtf(2.0f * fabsf(delta_p) / flow_denom);
        
        // 3. Add breath noise (scaled by flow velocity)
        u += params.noiseGain * fabsf(u) * breathNoise.tick();
        
        // 4. Convert flow to pressure wave entering bore
        float p_bore_out = p_bore_in + params.Z_bore * u;
        
        // 5. Process through tone hole lattice
        for (int i = 0; i < numActiveHoles; i++) {
            toneHoles[i].process(p_bore_out, params.holeOpenings[i]);
        }
        
        // 6. Propagate through bore (with viscothermal loss)
        bore_forward.write(p_bore_out);
        float p_at_bell = boreLoop_loss.process(bore_forward.read());
        
        // 7. Bell reflection + radiation
        float p_reflected = bellReflection.process(p_at_bell);
        float p_radiated = p_at_bell + p_reflected;
        
        bore_backward.write(p_reflected);
        
        return p_radiated;
    }
};
```

### 6.3 Internal Sample Rate Considerations

**Reed resonance frequency** (~2500-4000 Hz) means the reed dynamics contain energy up to ~8-10 kHz. At 44.1 kHz, the Nyquist frequency (22.05 kHz) provides adequate headroom. However:

**Oversampling is recommended (2x) for:**
- Reducing aliasing from the reed nonlinearity (the clipping/saturation generates harmonics)
- Improving the accuracy of the reed dynamics integration (Euler method stability)
- Better fractional delay filter performance at high frequencies

**4x oversampling is overkill** for waveguide reed models -- the nonlinearity is relatively mild compared to, say, virtual analog oscillators. 2x (88.2/96 kHz internal rate) is the sweet spot.

```cpp
// Processing block with 2x oversampling
void processBlock(float* output, int numSamples) {
    for (int i = 0; i < numSamples; i++) {
        // Two internal samples per output sample
        float s1 = voice.tick(p_mouth, embouchure, params);
        float s2 = voice.tick(p_mouth, embouchure, params);
        
        // Simple decimation (or use a proper anti-alias filter)
        output[i] = (s1 + s2) * 0.5f;
    }
}
```

### 6.4 CPU Budget Estimation

**Per voice at 44.1 kHz (with 2x oversampling = 88.2 kHz internal):**

| Component | Operations per sample | Cost estimate |
|-----------|----------------------|---------------|
| Reed dynamics (2nd order ODE) | ~15 mults, 10 adds | ~25 ops |
| Bernoulli flow + sqrt | ~8 mults, 1 sqrt | ~15 ops |
| Reed nonlinearity (table/poly) | ~4 mults | ~4 ops |
| Bore delay line (fractional) | ~6 mults (allpass interp) | ~6 ops |
| Viscothermal loss filter | ~3 mults | ~3 ops |
| Bell reflection filter | ~3 mults | ~3 ops |
| Tone holes (x4) | ~20 mults | ~20 ops |
| Breath noise + filtering | ~5 mults | ~5 ops |
| Vibrato LFO | ~3 mults | ~3 ops |
| **Total per internal sample** | | **~84 ops** |
| **Total per output sample (2x OS)** | | **~168 ops** |

**At 44.1 kHz, single core, ~4 GHz modern CPU:**
- Available ops per sample: ~90,000
- Ops per voice: ~168
- **Theoretical max polyphony: ~530 voices**
- **Practical polyphony (accounting for cache, branches, overhead): ~32-64 voices**

This is extremely efficient. Even with generous headroom for additional features (growl, key clicks, advanced bore modeling), 8-16 voices of polyphony is comfortably achievable.

**For monophonic use** (most common for reed instruments): CPU is a non-issue. Use the budget for higher quality: 4x oversampling, more tone holes, more complex bore model.

### 6.5 Polyphony Architecture

Reed instruments are primarily monophonic, but polyphony is useful for:
- Layering / unison for ensemble sound
- Sound design applications
- Playing chords in synth-mode

Recommend: **8 voices default, expandable to 16.** Each voice is fully independent with its own bore state, reed state, and tone hole configuration.

---

## 7. Key References

### Foundational Papers
- Smith, J.O. (1986). "Efficient simulation of the reed-bore and bow-string mechanisms." ICMC Proceedings.
- McIntyre, M.E., Schumacher, R.T., Woodhouse, J. (1983). "On the oscillations of musical instruments." JASA 74(5).
- Schumacher, R.T. (1981). "Ab initio calculations of the oscillations of a clarinet." Acustica 48.

### Reed Models
- Avanzini, F., van Walstijn, M. (2004). "Modelling the mechanical response of the reed-mouthpiece-lip system of a clarinet. Part I & II." Acta Acustica.
- Guillemain, P., Kergomard, J., Voinier, T. (2005). "Real-time synthesis of clarinet-like instruments using digital impedance models." JASA 118(1).

### Double Reed
- Guillemain, P. (2004). "A digital synthesis model of double-reed wind instruments." EURASIP Journal on Applied Signal Processing 2004:7, 990-1000.
- Almeida, A. (2006). "The physics of double-reed wind instruments and its application to sound synthesis." PhD Thesis.

### Bore and Tone Holes
- Valimaki, V. et al. (1994). "Digital waveguide modeling of wind instrument bores constructed of truncated cones." ICMC Proceedings.
- Scavone, G., Smith, J.O. (1996). "Digital waveguide modeling of woodwind toneholes." ICMC Proceedings.
- Keefe, D.H. (1981). "Woodwind tone-hole acoustics and the spectrum transformation function." PhD Thesis.

### Bell Radiation
- Scavone, G. (1999). "Modeling wind instrument sound radiation using digital waveguides." ICMC Proceedings.

### Register Holes and Overblowing
- Music acoustics research, UNSW Sydney (ongoing). "Clarinet acoustics" and "Double reed acoustics." newt.phys.unsw.edu.au

### Recent Advances
- Music et al. (2025). "Discrete port-Hamiltonian system model of a single-reed woodwind instrument." Frontiers in Signal Processing. (Energy-based stability framework with Hunt-Crossley contact model and switching PHS for tone holes.)

### Implementations
- Cook, P., Scavone, G. (1999). "The Synthesis Toolkit (STK)." C++ library. github.com/thestk/stk
- Michon, R. (2011). "Faust-STK: A set of linear and nonlinear physical models for the Faust programming language." DAFx-11.

---

## 8. Implementation Roadmap Summary

### Stage 1 -- Foundation
- Basic cylindrical waveguide with static reed table (STK-style)
- Fractional delay for pitch control
- Simple bell reflection filter
- Monophonic with breath pressure and embouchure control
- **Goal:** Convincing clarinet-like tone

### Stage 2 -- Reed Dynamics + Conical Bore
- Full mass-spring-damper reed model
- Conical bore correction filter (Strategy B)
- Guillemain confined jet parameter (Psi) for double reed
- Breath noise injection
- **Goal:** Distinguish clarinet, sax, oboe by parameters alone

### Stage 3 -- Expression + Polish
- Tone hole lattice (4 virtual holes + register hole)
- Vibrato (lip, breath, throat)
- Attack transient modeling (chiff, tongue articulation)
- Subtone mode
- Key click / pad noise
- Growl / vocal coupling
- **Goal:** Expressive, responsive instrument

### Stage 4 -- Sound Design Territory
- Impossible bore geometries (reverse taper, branching bores)
- Reed parameter extremes (negative stiffness, ultra-high mass)
- Cross-modulation between reed and bore
- Feedback path manipulation
- Dual-bore configurations (arghul-like)
- **Goal:** Sound design tool beyond acoustic emulation

---

## Sources

- [Digital Waveguide Synthesis - Wikipedia](https://en.wikipedia.org/wiki/Digital_waveguide_synthesis)
- [Physical Audio Signal Processing - Julius O. Smith III](https://www.dsprelated.com/freebooks/pasp/Single_Reed_Instruments.html)
- [Single-Reed Theory - Smith](https://www.dsprelated.com/freebooks/pasp/Single_Reed_Theory.html)
- [STK Synthesis Toolkit - GitHub](https://github.com/thestk/stk)
- [STK ReedTable Class Reference](https://ccrma.stanford.edu/software/stk/classstk_1_1ReedTable.html)
- [STK Saxofony Class Reference](https://ccrma.stanford.edu/software/stk/classstk_1_1Saxofony.html)
- [Faust Physical Modeling Libraries](https://faustlibraries.grame.fr/libs/physmodels/)
- [Faust Clarinet DSP Source](https://github.com/grame-cncm/faust/blob/master-dev/examples/physicalModeling/faust-stk/clarinet.dsp)
- [Guillemain - Double Reed Synthesis Model (EURASIP 2004)](https://docslib.org/doc/1346490/a-digital-synthesis-model-of-double-reed-wind-instruments)
- [Guillemain et al. - Real-time Clarinet Synthesis (JASA 2005)](https://hal.science/hal-00009254)
- [Double Reed Acoustics - UNSW](https://newt.phys.unsw.edu.au/jw/double-reed-acoustics.html)
- [Wave Impedance in a Cone - Smith](https://ccrma.stanford.edu/~jos/pasp/Wave_Impedance_Cone.html)
- [Conical Bores - Smith](https://ccrma.stanford.edu/~jos/cmj96/Conical_Bores.html)
- [Tone Hole Models - Smith](https://ccrma.stanford.edu/~jos/tonehole/Tonehole_Models.html)
- [Discrete Port-Hamiltonian Woodwind Model (Frontiers 2025)](https://www.frontiersin.org/journals/signal-processing/articles/10.3389/frsip.2025.1519450/full)
- [Resonarium Waveguide Synthesizer - GitHub](https://github.com/gabrielsoule/resonarium)
- [SWAM Clarinets - Audio Modeling](https://audiomodeling.com/swam-engine/solo-woodwinds/swam-clarinets/)
- [SWAM Double Reeds - Audio Modeling](https://audiomodeling.com/swam-engine/solo-woodwinds/swam-double-reeds/)
- [Faust-STK Paper (DAFx 2011)](https://ccrma.stanford.edu/~rmichon/publications/doc/DAFx11-Faust-STK.pdf)
- [Real-time Woodwind Modeling - Scavone (ISMA 1998)](https://ccrma.stanford.edu/~gary/papers/RTisma98.pdf)
