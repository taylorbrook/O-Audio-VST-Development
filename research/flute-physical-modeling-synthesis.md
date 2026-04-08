---
title: "Flute Physical Modeling Synthesis"
created: 2026-04-04
juce_version: "8.0.4"
summary: "Comprehensive technical reference for flute physical modeling synthesis using digital waveguides, covering air jet excitation, bore modeling, tone hole lattice filters, embouchure/breath models, flute-specific physics, and real-time implementation considerations for the O-Wind plugin."
domain: dsp
type: research
keywords:
  - physical-modeling
  - flute
  - waveguide
  - air-jet
  - edge-tone
  - jet-drive
  - tone-hole
  - embouchure
  - bore-model
  - wind-instrument
  - overblowing
  - shakuhachi
  - recorder
stages: [0]
agents: [research, dsp]
---

# Flute Physical Modeling Synthesis

**Complete Technical Reference for O-Wind Plugin**

**Created:** April 2026
**Version:** 1.0
**Research Depth:** Level 3 (Comprehensive Investigation)

---

## Executive Summary

This document covers the physics, algorithms, and real-time implementation strategies for physical modeling synthesis of flute-like instruments. Unlike reed instruments (clarinet, saxophone) or bowed strings, flutes produce sound through *air jet excitation* -- a thin ribbon of air directed at a sharp edge (labium), creating self-sustaining oscillations coupled to a resonant bore. This mechanism is shared across the concert (Boehm) flute, recorder, shakuhachi, pan flute, and organ pipes.

**Key Findings:**
- The jet-drive model (Verge 1995) is the standard for real-time flute synthesis -- it models jet displacement with a delayed, amplified sinusoidal perturbation hitting the labium
- A cylindrical open-open bore supports all harmonics, enabling octave overblowing via increased jet velocity
- Tone holes are best modeled as two-port scattering junctions (Keefe 1990) with second-order digital filters
- The nonlinear jet-labium interaction (hyperbolic tangent saturation) is what sets amplitude and enables regime changes
- Turbulence noise injection (scaled with jet velocity squared) is essential for realistic breath sounds
- A complete per-voice model requires approximately 2 delay lines + 6-16 tone hole filters + loss/radiation filters -- feasible at 44.1 kHz without oversampling

---

## Table of Contents

### Part 1: Excitation Physics
1. [Air Jet and Edge Tone Mechanism](#1-air-jet-and-edge-tone-mechanism)
2. [Jet-Drive Model](#2-jet-drive-model)
3. [Discrete Vortex Model (Alternative)](#3-discrete-vortex-model)

### Part 2: Resonator
4. [Waveguide Bore Model](#4-waveguide-bore-model)
5. [Loss Filters for Viscothermal Losses](#5-loss-filters-for-viscothermal-losses)
6. [Radiation Impedance and End Reflections](#6-radiation-impedance-and-end-reflections)

### Part 3: Pitch Control
7. [Tone Hole Modeling](#7-tone-hole-modeling)
8. [Smooth Tone Hole Transitions](#8-smooth-tone-hole-transitions)

### Part 4: Player Model
9. [Embouchure and Breath Model](#9-embouchure-and-breath-model)
10. [Turbulence Noise Injection](#10-turbulence-noise-injection)

### Part 5: Instrument Variants
11. [Flute-Specific Physics and Overblowing](#11-flute-specific-physics-and-overblowing)
12. [Instrument Variant Differences](#12-instrument-variant-differences)

### Part 6: Implementation
13. [Complete Signal Flow](#13-complete-signal-flow)
14. [Real-Time Implementation Considerations](#14-real-time-implementation-considerations)
15. [Key Academic References](#15-key-academic-references)

---

## Part 1: Excitation Physics

### 1. Air Jet and Edge Tone Mechanism

Sound production in flute-like instruments results from the interaction of a thin air jet with a sharp edge (the labium), coupled to a resonant tube. This is fundamentally different from reed instruments: there is no vibrating mechanical element. The oscillation is *aeroacoustic* -- the jet itself is the oscillator.

#### 1.1 Physical Mechanism

The player blows air through a narrow channel (the flue or lip opening), forming a thin, laminar jet. This jet travels across an open window toward the labium (sharp edge). The process:

1. **Jet formation**: Air accelerates through the flue channel (height h ~ 1-2 mm), forming a planar jet with a Bickley velocity profile
2. **Acoustic feedback**: Sound pressure from the resonating bore creates a transverse acoustic velocity at the mouth opening
3. **Jet perturbation**: This acoustic velocity deflects the jet laterally at its origin
4. **Instability amplification**: The perturbation grows exponentially as it travels along the jet (Kelvin-Helmholtz instability)
5. **Jet-labium interaction**: The amplified perturbation reaches the labium, alternately directing flow into and out of the bore
6. **Pressure source**: The switching flow at the labium acts as an acoustic dipole source, sustaining the resonance

The key insight (Coltman 1968, Fletcher 1976): the jet acts as an *amplifier* of acoustic perturbations, not an independent oscillator. The bore resonance controls the frequency; the jet provides the energy.

#### 1.2 Foundational Research

- **Coltman (1966, 1968)**: Established the dipole character of the flute sound source. Demonstrated that the sounding frequency is controlled by the pipe resonance, not the edge tone frequency. Showed the jet switches sides of the labium once per cycle.
- **Fletcher (1976)**: Developed quantitative models relating jet velocity, embouchure geometry, and playing frequency. Showed that overblowing occurs when jet delay exceeds half the oscillation period.
- **Verge (1995)**: PhD thesis "Aeroacoustics of Confined Jets" at Eindhoven -- developed the jet-drive model with explicit equations for jet formation, propagation, and labium interaction. This became the standard model for time-domain simulation.

---

### 2. Jet-Drive Model

The jet-drive model (Verge 1995, refined by de la Cuadra 2005) is the standard approach for real-time flute synthesis. It models the jet as a one-dimensional perturbation that travels from flue exit to labium.

#### 2.1 Jet Velocity Profile

The jet emerging from the flue channel has a Bickley (sech-squared) velocity profile:

```
U(y) = U_j * sech^2(y / b)
```

Where:
- `U_j` = centerline jet velocity (determined by blowing pressure)
- `b = 2h/5` = jet half-width (assuming Poiseuille flow in the channel)
- `h` = flue channel height
- `y` = transverse position relative to jet centerline

#### 2.2 Jet Velocity from Blowing Pressure

The jet speed relates to blowing pressure via the unsteady Bernoulli equation:

```
rho_0 * l_c * (dU_j/dt) + (1/2) * rho_0 * U_j^2 = p_f - p_w
```

Where:
- `rho_0` = air density (~1.2 kg/m^3)
- `l_c` = effective flue channel length
- `p_f` = foot/cavity pressure (player's mouth pressure)
- `p_w` = window/mouth pressure (acoustic pressure at the embouchure)

In steady state, this simplifies to:

```
U_j = sqrt(2 * (p_f - p_w) / rho_0)
```

Typical jet velocities range from 5-30 m/s for normal playing.

#### 2.3 Jet Displacement at the Labium

The transverse displacement of the jet centerline at position x and time t:

```
eta(x, t) = exp(alpha_i * x) * eta_0(t - x / c_p)
```

Where:
- `alpha_i = beta / h` = spatial amplification rate (beta ~ 0.3-0.5)
- `c_p = gamma * U_j` = phase velocity of the jet perturbation (gamma ~ 0.4)
- `eta_0` = initial perturbation amplitude at the flue exit
- `x` = distance along the jet from flue to labium

The initial perturbation is driven by the acoustic velocity at the mouth:

```
eta_0(t) = (h / U_j) * v_ac(t)
```

Where `v_ac(t)` is the transverse acoustic velocity at the flue exit.

**Key parameters:**
- `W` = window length (flue-to-labium distance, typically 4-8 mm)
- Jet travel time (delay): `tau = W / c_p = W / (gamma * U_j)`
- Amplification gain: `G = exp(alpha_i * W) = exp(beta * W / h)`

The jet delay `tau` is *velocity-dependent* -- higher blowing pressure means faster jet, shorter delay. This is critical for overblowing behavior.

#### 2.4 Jet-Labium Interaction (Pressure Source)

The jet's transverse displacement at the labium determines how much flow enters the bore versus exits. The fraction of jet flow entering the pipe is modeled with a hyperbolic tangent saturation:

```
Delta_p_source = (rho_0 * delta_d * U_j * b / S_w) * d/dt[tanh((eta(W, t) - y_0) / b)]
```

Where:
- `delta_d = (4/pi) * sqrt(2*h*W)` = effective dipole source separation (Verge's formula)
- `S_w` = window area
- `y_0` = labium offset below the jet centerline
- `b` = jet half-width

The `tanh` provides the essential **nonlinear saturation**: when the jet is fully on one side of the labium, further displacement produces no additional flow change. This limits the oscillation amplitude and determines the steady-state waveform shape.

#### 2.5 Vena Contracta Loss

An additional pressure term models losses from boundary layer separation at the mouth:

```
Delta_p_loss = -(1/2) * rho_0 * (v_ac^2 * sign(v_ac)) / C^2
```

Where `C ~ 0.6` is the vena contracta coefficient. This nonlinear loss term is important for amplitude limiting and realistic dynamics.

#### 2.6 Total Mouth Pressure

```
p_mouth(t) = Delta_p_source + Delta_p_loss
```

This pressure is injected at the mouth end of the bore waveguide.

---

### 3. Discrete Vortex Model

An alternative to the jet-drive model, the discrete vortex model treats the jet as two independent shear layers along which discrete vortices form and convect toward the labium.

**When to use which model (Auvray et al. 2014):**
- **Jet-drive model**: Valid when the *dynamic aspect ratio* `lambda/h > 5` (where `lambda` is the hydrodynamic wavelength). This covers most normal playing conditions for concert flutes and recorders.
- **Discrete vortex model**: Better for `lambda/h < 5`, which occurs with very short jet-labium distances or very high frequencies. Relevant for small instruments or extreme registers.

For the O-Wind plugin targeting concert flute, recorder, and shakuhachi, the **jet-drive model is the recommended choice**. It is simpler to implement, computationally cheaper, and valid across the normal playing range.

---

## Part 2: Resonator

### 4. Waveguide Bore Model

The flute bore is modeled as a bidirectional digital waveguide -- a pair of delay lines carrying right-going (+) and left-going (-) pressure waves.

#### 4.1 Basic Architecture

```
Embouchure (mouth)                                          Open End
     |                                                         |
     |  p+(n) -----> [Delay Line L samples] -----> p+(n-L)    |
     |                                                         |
     |  p-(n) <----- [Delay Line L samples] <----- p-(n-L)    |
     |                                                         |
  [Excitation]                                    [Reflection Filter]
  [Injection]                                     [Radiation Output]
```

The total delay `L` (in samples) for a given fundamental frequency f0:

```
L = fs / f0
```

Where `fs` is the sample rate. For a concert flute playing A4 (440 Hz) at 44100 Hz:

```
L = 44100 / 440 = 100.23 samples
```

This requires **fractional delay filtering** (see Section 14).

#### 4.2 Concert Flute Bore Specifications

Physical dimensions of a modern Boehm flute relevant to modeling:

| Parameter | Value |
|-----------|-------|
| Bore diameter (body) | 19 mm |
| Bore diameter (head, at cork) | ~17 mm |
| Effective sounding length | ~600 mm (C4) to ~285 mm (C6) |
| Embouchure hole | 10 mm x 12 mm, wall height 4.2 mm |
| Tone hole diameter | ~13 mm (approximately uniform) |
| Tone holes count | 16 (covered by Boehm key mechanism) |
| Frequency range | B3/C4 to C7 (~262 Hz to ~2093 Hz) |
| Cork-to-embouchure distance | 17.0 mm (standard) |

The head joint has a parabolic taper from 19 mm to 17 mm. This taper improves low register tuning and is typically modeled as a slight pitch correction rather than a full conical waveguide section.

#### 4.3 Open-Open Bore Behavior

The concert flute is effectively an *open-open* cylindrical tube:
- Open at the embouchure (mouth hole)
- Open at the first open tone hole (or foot end)

An open-open cylinder supports **all harmonics** (both odd and even):

```
f_n = n * c / (2 * L_eff)    for n = 1, 2, 3, 4, ...
```

Where `c` = speed of sound (~343 m/s) and `L_eff` is the effective acoustic length. This is why flutes have a rich, full harmonic spectrum and overblow at the octave (2:1 ratio).

Compare with clarinet (closed-open cylinder): only odd harmonics, overblows at the twelfth (3:1).

---

### 5. Loss Filters for Viscothermal Losses

As waves propagate through the bore, they lose energy due to viscous friction and thermal conduction at the tube walls. These losses are frequency-dependent (higher frequencies attenuate more) and must be modeled for realistic tone color.

#### 5.1 Wall Loss Physics

The propagation constant for a lossy cylindrical tube:

```
Gamma(omega) = j*omega/c + (1+j) * alpha_v(omega)
```

Where the viscothermal attenuation coefficient:

```
alpha_v(omega) = (1/a) * sqrt(omega / (2*c)) * [sqrt(nu) + (gamma_air - 1) * sqrt(nu_t)]
```

- `a` = bore radius
- `nu` = kinematic viscosity of air (~1.5e-5 m^2/s)
- `nu_t` = thermal diffusivity of air (~2.1e-5 m^2/s)
- `gamma_air` = ratio of specific heats (~1.4)

Key insight: attenuation scales as `sqrt(omega)` -- a gentle roll-off that preferentially damps higher harmonics.

#### 5.2 Digital Filter Approximation

Abel, Smyth, and Smith (2003) showed that viscothermal losses can be accurately approximated with low-order IIR filters. For a typical bore segment:

**First-order approximation** (simplest, often sufficient):
```
H_loss(z) = g * (1 + a1) / (1 + a1 * z^-1)
```

Where `g` is an overall gain factor and `a1` is chosen to match the `sqrt(f)` roll-off characteristic.

**Practical approach**: Use a single one-pole low-pass filter per delay line trip, with the pole frequency and gain tuned to match the bore's viscothermal profile. The STK flute model uses coefficients `a0 = 0.7, b1 = -0.3` for the boundary loss filter.

For higher accuracy, a **fourth-order IIR filter** provides close approximation across the full audio band. The filter is placed at one end of each delay line (typically combined with the reflection filter).

---

### 6. Radiation Impedance and End Reflections

At each open end of the bore (embouchure hole and first open tone hole / foot), waves partially reflect back into the bore and partially radiate outward as sound.

#### 6.1 Open-End Reflection

For an unflanged cylindrical tube (Levine & Schwinger), the reflection coefficient magnitude:

```
|R(omega)| ~ 1 - (k*a)^2 / 2    for k*a << 1
```

Where `k = omega/c` is the wavenumber and `a` is the tube radius. At low frequencies, nearly all energy reflects (|R| ~ 1). At higher frequencies, more energy radiates.

The **end correction** (additional effective length due to radiation loading):

```
delta_L ~ 0.6133 * a    (unflanged)
delta_L ~ 0.8216 * a    (infinite flange)
```

The flute embouchure hole is partially flanged; the effective end correction lies between these extremes.

#### 6.2 Digital Filter Implementation

The open-end reflection is implemented as a first- or second-order IIR filter:

```
R(z) = (b0 + b1*z^-1) / (1 + a1*z^-1)
```

This filter has:
- Magnitude near -1 at low frequencies (inverting reflection for open end -- pressure node)
- Magnitude decreasing toward 0 at high frequencies (increasing radiation)
- Phase delay modeling the frequency-dependent end correction

The negative sign is critical: an open tube end inverts the reflected pressure wave (pressure node at the opening).

#### 6.3 Radiation Output

The radiated sound (plugin output) is the *transmitted* portion:

```
p_out = p_incident * (1 - |R|^2)^(1/2)
```

Or more practically, the output is tapped from the outgoing wave at the open end, filtered by a complementary radiation filter:

```
H_rad(z) = 1 + R(z)
```

This is a high-pass characteristic, matching the physical fact that flutes radiate higher harmonics more efficiently than the fundamental.

---

## Part 3: Pitch Control

### 7. Tone Hole Modeling

Tone holes are the primary pitch control mechanism. Each open hole acts as a shunt path to the outside air, effectively shortening the acoustic length of the bore.

#### 7.1 Keefe Tone Hole Model

Keefe (1990) developed the standard acoustic model of a single tone hole as a two-port element with series and shunt impedances.

**Transmission matrix formulation:**

```
[P_2]   [  1    R_a/2 ] [ 1    0   ] [  1    R_a/2 ] [P_1]
[U_2] = [  0    1     ] [ 1/R_s 1  ] [  0    1     ] [U_1]
```

Where:
- `R_a` = series impedance (acoustic mass of the bore segment interrupted by the hole)
- `R_s` = shunt impedance (depends on whether hole is open or closed)

**Shunt impedance -- open hole:**
```
R_s_open = R_b * (j*k*t_e + xi_e)
```

Where `R_b = rho*c / (pi*b^2)` (wave impedance at hole entrance, b = hole radius), `t_e` = effective height including end corrections, and `xi_e` = radiation resistance.

**Shunt impedance -- closed hole:**
```
R_s_closed = -j * R_b * cot(k * t_h)
```

Where `t_h` = physical hole depth. A closed hole acts as a short, closed tube stub -- adding a small compliance that slightly lowers the pitch.

**Series impedance:**
```
R_a = -j * R_b * k * t_a
```

Where `t_a` is an effective series length accounting for the bore-hole junction geometry.

#### 7.2 Scattering Junction Implementation

For digital waveguide implementation, the transmission matrix is converted to a traveling-wave scattering formulation:

**Reflectance:**
```
S(omega) = [4*R_a*R_s + R_a^2 - 4*R_0^2] / [(2*R_0 + R_a) * (2*R_0 + R_a + 4*R_s)]
```

**Transmittance:**
```
T(omega) = [8*R_0*R_s] / [(2*R_0 + R_a) * (2*R_0 + R_a + 4*R_s)]
```

Where `R_0 = rho*c / (pi*a^2)` is the bore characteristic impedance (a = bore radius).

The scattering junction takes incoming waves from both directions and produces outgoing waves:

```
p_1_out = S * (p_1_in + p_2_in) + A * p_2_in
p_2_out = S * (p_1_in + p_2_in) + A * p_1_in
```

Where `A(omega) = 1 - L(omega)` is an allpass correction filter.

#### 7.3 Simplified Implementation: Loaded Two-Port Junction

When series inertance is negligible (`R_a ~ 0`), the tone hole reduces to a loaded waveguide junction:

```
alpha = 2 * R_J(s) / (2 * R_J(s) + R_0)
```

Where `R_J = R_s` is the shunt impedance acting as a parallel load. This requires only one multiply and one filter per tone hole -- very efficient.

#### 7.4 Filter Design

The continuous-time impedances are approximated as **second-order digital IIR filters**:

- Open-hole reflectance: Weighted L2 equation-error minimization
- Closed-hole reflectance: Kopec's method combined with linear prediction

Keefe showed there is only one important tonehole resonance/anti-resonance within the audio band, making second-order filters sufficient for high accuracy.

---

### 8. Smooth Tone Hole Transitions

Abruptly switching between open and closed tone hole states produces audible clicks. Realistic transitions require smooth interpolation.

#### 8.1 Crossfade Approach

The simplest method: linearly interpolate between open-hole and closed-hole filter coefficients over a transition window (1-10 ms). This works but can produce intermediate states with no physical analog.

#### 8.2 Fractional Delay Crossfade

During legato note transitions, cross-fade from a read pointer at the old-pitch delay to a read pointer at the new-pitch delay. This avoids discontinuities in the waveguide state while smoothly transitioning pitch.

#### 8.3 Physical Transition Model

For key click realism, model the hole opening/closing as a continuous parameter (0 = closed, 1 = open) that smoothly sweeps the shunt impedance:

```
R_s(alpha) = alpha * R_s_open + (1 - alpha) * R_s_closed
```

Where `alpha` ramps over 2-5 ms (typical key travel time). This naturally produces the brief broadband "click" transient heard in real playing as the tone hole impedance passes through intermediate states.

#### 8.4 Energy-Conserving Switching

Recent research (2024-2025) proposes switching-systems approaches where tone hole sub-models connect in a modular, energy-conserving manner. This ensures global stability regardless of rapid state changes -- important when multiple holes switch simultaneously during fast passages.

---

## Part 4: Player Model

### 9. Embouchure and Breath Model

The player's breath and embouchure shape are the primary controls of the instrument. Modeling these accurately is essential for expressive synthesis.

#### 9.1 Breath Pressure to Jet Velocity

The mapping from mouth pressure to jet velocity follows Bernoulli:

```
U_j = sqrt(2 * p_mouth / rho_0)
```

Typical mouth pressures for flute playing:
- Piano (soft): 200-500 Pa
- Mezzo-forte: 500-1500 Pa
- Forte (loud): 1500-3000 Pa
- Overblowing to 2nd register: requires ~2x the pressure of 1st register

The breath pressure envelope critically affects attack transients. A realistic envelope has:
- **Attack**: 10-50 ms rise time
- **Overshoot**: Brief pressure spike at onset (produces the characteristic "chiff")
- **Sustain**: Steady pressure with slight vibrato modulation (3-6 Hz, ~2-5% depth)
- **Release**: 50-200 ms decay

#### 9.2 Embouchure Parameters

The embouchure (lip position and angle) controls several acoustic parameters:

**Lip coverage** (`y_0`): How far the lower lip covers the embouchure hole. Greater coverage:
- Reduces the effective mouth area
- Changes the jet-labium offset
- Affects which register speaks (more coverage favors lower register)

**Jet angle**: The angle at which the air stream hits the labium:
- Centered on the labium edge = strongest fundamental
- Angled more into the bore = brighter tone (more energy to upper harmonics)
- Angled more outward = softer, darker tone

**Jet width** (`h`): Controlled by lip aperture:
- Narrower jet = faster velocity for same pressure = easier overblowing
- Wider jet = more volume, richer tone

#### 9.3 Vibrato Implementation

Flute vibrato is primarily *pressure vibrato* (modulating breath pressure), not pitch vibrato:

```
p_breath(t) = p_0 * (1 + depth * sin(2*pi*f_vib*t + phi_random(t)))
```

Where `f_vib` = 4-6 Hz and `depth` = 0.02-0.08. This produces correlated pitch and amplitude modulation, matching real flute vibrato characteristics.

---

### 10. Turbulence Noise Injection

Breath noise is a defining characteristic of flute tone -- more so than any other orchestral instrument. The "breathy" quality comes from turbulence in the jet and at the labium.

#### 10.1 Noise Source Model

The turbulence noise pressure scales with the square of the jet velocity:

```
p_noise(t) = k_noise * U_j^2 * n(t)
```

Where:
- `k_noise` = noise scaling coefficient (tuned perceptually)
- `n(t)` = band-limited white noise, filtered to approximate turbulence spectrum
- `U_j` = instantaneous jet velocity

This quadratic scaling means: louder playing = proportionally more noise. This matches the physical observation that forte flute playing is breathier than piano.

#### 10.2 Noise Spectrum Shaping

Raw turbulence noise is shaped by the jet geometry:
- **Low-pass filtered** at approximately `f_cutoff = U_j / (2*h)` (related to the jet instability cutoff frequency)
- The spectrum has a roughly -5/3 power law roll-off (Kolmogorov turbulence spectrum) above the cutoff

In practice, a simple first- or second-order low-pass filter on white noise, with cutoff frequency proportional to jet velocity, produces convincing results.

#### 10.3 Injection Point

The noise is injected at the mouth of the instrument, added to the jet-drive pressure source:

```
p_total(t) = Delta_p_source(t) + Delta_p_loss(t) + p_noise(t)
```

The STK flute model uses a noise gain of approximately 0.0356 as default.

---

## Part 5: Instrument Variants

### 11. Flute-Specific Physics and Overblowing

#### 11.1 What Makes Flutes Different from Reed Instruments

| Property | Flute (Air Jet) | Clarinet (Single Reed) | Oboe (Double Reed) |
|----------|-----------------|----------------------|-------------------|
| Excitation | Air jet on labium | Vibrating cane reed | Vibrating double reed |
| Bore | Cylindrical (open-open) | Cylindrical (closed-open) | Conical (closed-open) |
| Harmonics | All (1, 2, 3, 4...) | Odd only (1, 3, 5...) | All (1, 2, 3, 4...) |
| Overblows at | Octave (2:1) | Twelfth (3:1) | Octave (2:1) |
| Noise content | High (breathy) | Low (unless growling) | Low |
| Onset | Gradual (jet startup) | Sharp (reed snap) | Medium |
| Dynamic range | Moderate | Wide | Wide |
| Register mechanism | Jet velocity increase | Register key (vent) | Half-holing / octave key |

**Crucial difference for modeling**: In reed instruments, the resonator impedance *peaks* drive the oscillation. In flutes, the resonator *admittance* peaks (inverse of impedance) drive the oscillation. This is because the jet-drive mechanism acts as a pressure source controlled by volume velocity, the inverse of the reed mechanism.

#### 11.2 Overblowing Mechanism

Overblowing in flutes occurs when the jet delay `tau` becomes a significant fraction of the oscillation period. The phase balance condition at threshold:

```
arg(Y) + pi/2 - omega * tau = 2*m*pi
```

Where `Y(omega)` is the bore input admittance and `m` is an integer (mode number).

As the player increases blowing pressure:
1. Jet velocity `U_j` increases
2. Jet delay `tau = W / (0.4 * U_j)` decreases
3. When `tau` drops below approximately `T/4` (quarter period of fundamental), the phase condition for the fundamental is no longer satisfied
4. The system locks onto the second harmonic (octave), where the phase condition is met

The **hysteresis** region between registers is musically important -- skilled players can "hold" the lower register with increased pressure before the jump, and the transition point depends on embouchure as well as pressure.

Regime change modeling (Auvray et al. 2012): The transition between registers involves a complex dynamic where mouth pressure dynamics influence whether the jump is smooth or abrupt. Slow pressure ramps produce smooth transitions; fast ramps can produce multiphonic (multiple simultaneous registers) transients.

#### 11.3 Harmonic Content by Register

- **First register** (C4-C5): Strong fundamental, moderate 2nd and 3rd harmonics, weak upper partials. Warmest, most "flute-like" tone.
- **Second register** (C5-C6): Missing or weak fundamental of the fingered note. Brighter, more present tone. Higher breath noise.
- **Third register** (C6-C7): Very bright, piercing. Requires precise embouchure. Significant breath noise component.

---

### 12. Instrument Variant Differences

The O-Wind plugin could model multiple flute-like instruments with the same core architecture, varying parameters:

#### 12.1 Concert (Boehm) Flute

- **Bore**: Cylindrical, 19 mm diameter, open-open
- **Head joint**: Parabolic taper to 17 mm at cork
- **Excitation**: Side-blown (transverse), rectangular embouchure hole 10x12 mm
- **Range**: B3/C4 to C7
- **Tone holes**: 16, ~13 mm diameter, covered by key mechanism
- **Character**: Clear, projecting, moderate breathiness

#### 12.2 Recorder

- **Bore**: Combination cylindrical/conical (tapers toward foot)
- **Excitation**: Fipple/duct -- fixed flue channel geometry (player cannot vary jet angle)
- **Range**: ~2 octaves per size (soprano: C5-D7)
- **Tone holes**: 7-8, smaller diameter, finger-covered
- **Character**: Softer, woodier, less dynamic range than transverse flute
- **Modeling note**: Simpler excitation model (fixed embouchure parameters), but the conical bore requires either a conical waveguide or digitized bore profile

#### 12.3 Shakuhachi

- **Bore**: Conical (tapers then flares at root end), bamboo
- **Excitation**: End-blown, player's lips form the jet (no duct, no transverse blowing)
- **Range**: ~2.5 octaves (typically D4-based, 1.8 shaku length)
- **Tone holes**: 4 front + 1 thumb (only 5 holes)
- **Character**: Extremely expressive, wide timbral range through embouchure variation, heavy breath noise, pitch bending through chin movement (meri/kari techniques)
- **Modeling note**: Requires detailed embouchure model since jet angle and coverage are primary expressive controls. The conical bore with bamboo root flare creates the characteristic resonant bass. Five holes means most pitch variation comes from half-holing and embouchure.

#### 12.4 Parameter Variation Table

| Parameter | Concert Flute | Recorder (Soprano) | Shakuhachi |
|-----------|--------------|-------------------|------------|
| Bore diameter | 19 mm | 11-15 mm | 18-22 mm |
| Bore type | Cylindrical | Cylindrical+conical | Conical+flared |
| Flue height (h) | Variable (lips) | Fixed (~1 mm) | Variable (lips) |
| Window length (W) | ~4 mm | ~3-4 mm | ~6-8 mm |
| Embouchure control | High | None (fixed) | Very high |
| Noise character | Moderate | Low | High |
| Overblowing ease | Moderate | Difficult | Easy |

---

## Part 6: Implementation

### 13. Complete Signal Flow

The complete flute physical model signal flow:

```
[Breath Pressure] ---> [Bernoulli: U_j = sqrt(2*p/rho)]
                              |
                              v
[Noise Generator] -------> [Noise * U_j^2] -------+
                                                    |
[Acoustic velocity       [Jet Perturbation]        |
 from bore feedback] --> [eta_0 = h*v_ac/U_j]      |
                              |                     |
                              v                     |
                    [Jet Delay: tau = W/(0.4*U_j)]  |
                              |                     |
                              v                     |
                    [Amplification: exp(beta*W/h)]  |
                              |                     |
                              v                     |
                    [tanh Saturation at Labium]     |
                              |                     |
                              v                     |
                    [Dipole Pressure Source] <-------+
                              |
                              v
                    [+ Vena Contracta Loss]
                              |
                              v
                    [Mouth Pressure Injection]
                              |
            +-----------------+------------------+
            |                                    |
            v                                    v
    [Bore Waveguide (+)]              [Embouchure Delay]
    [Tone Hole Junctions]             [Jet Reflection]
    [Loss Filter]                     [-> back to perturbation]
            |
            v
    [Open-End Reflection Filter]
            |
            +---> [Radiation Filter] ---> OUTPUT
            |
            v
    [Bore Waveguide (-)]
    [Tone Hole Junctions]
    [Loss Filter]
            |
            v
    [Back to Mouth]
    [-> Acoustic velocity feedback]
```

**Simplified STK-style implementation** (Cook/Karjalainen):

```
                +--[noise * gain]--+
                |                  |
[breath] ---> (+) --> [cubic: x-x^3] --> [embouchure delay] --+
                ^                                              |
                |                                              v
                +--- [fbk1 * bore_out] --+            [bore delay line]
                                         |                     |
                                         |              [loss filter]
                                         |                     |
                                    [fbk2 * bore_out] <--------+
                                                               |
                                                               v
                                                           [output]
```

The STK model uses:
- Cubic polynomial `x - x^3` as the jet nonlinearity (simpler than tanh)
- Two feedback paths with gains ~0.5 and ~0.55
- Embouchure delay = bore delay / 2
- One-pole loss filter (a0=0.7, b1=-0.3)

---

### 14. Real-Time Implementation Considerations

#### 14.1 Fractional Delay Lines

Since `L = fs / f0` is rarely an integer, fractional delay interpolation is required. Options:

- **Linear interpolation**: Cheapest, slight high-frequency loss. Acceptable for bore delays.
- **Lagrange interpolation (3rd-5th order)**: Standard choice. Better frequency response than linear, moderate cost. Used in Hanninen & Valimaki (1996) flute model.
- **Allpass interpolation**: Flat magnitude response, nonlinear phase. Good for critical paths (embouchure delay, tone hole placement).
- **Sinc interpolation**: Highest quality, highest cost. Rarely needed.

**Recommendation**: Lagrange 3rd order for bore delay lines, allpass for embouchure delay and tone hole positions.

#### 14.2 Stability of the Jet-Resonator Feedback Loop

The feedback loop between jet and resonator is the most stability-critical element. Key considerations:

- The `tanh` nonlinearity provides natural amplitude limiting (unlike polynomial approximations which can diverge)
- The jet gain `G = exp(beta*W/h)` must be carefully controlled -- too high and the system blows up; too low and it doesn't oscillate
- The loss filter in the bore provides essential damping at high frequencies
- **Safe practice**: Clamp the total loop gain to slightly above 1.0 at the target frequency, and ensure all higher modes are damped below unity gain

The STK approach of using `x - x^3` for the nonlinearity works but can become unstable with large input values. A `tanh` or hard clip is safer.

#### 14.3 Oversampling Requirements

Flute physical models generally do **not** require oversampling at 44.1 kHz or 48 kHz sample rates:

- The jet nonlinearity (tanh) is smooth and does not produce strong aliased harmonics
- Flute spectra naturally roll off above 4-6 kHz (low harmonic content compared to bowed strings or brass)
- The loss filter in the bore attenuates higher harmonics

**Exception**: If modeling the shakuhachi or other instruments with very noisy, bright timbres, 2x oversampling may improve quality for the noise component.

#### 14.4 CPU Cost Estimate (Per Voice)

| Component | Operations/Sample | Notes |
|-----------|------------------|-------|
| Jet velocity (Bernoulli) | ~5 ops | sqrt, multiply |
| Jet delay + amplification | ~10 ops | Fractional delay read, exp multiply |
| tanh nonlinearity | ~8 ops | Polynomial approximation or lookup table |
| Embouchure delay line | ~6 ops | Write + fractional read |
| Bore delay line (pair) | ~12 ops | Two fractional delay lines |
| Loss filter (1-pole) | ~4 ops | Per delay line direction |
| Reflection filter (2nd order) | ~10 ops | Open-end + embouchure |
| Tone holes (6-8 active) | ~48-64 ops | ~8 ops per 2-port junction |
| Noise generation + filtering | ~8 ops | White noise + LP filter |
| Vena contracta + mixing | ~6 ops | |
| **Total per voice** | **~120-135 ops** | |

At 44.1 kHz, this is approximately **6 million operations/second per voice**. On a modern CPU (single core at 3 GHz, ~1 FLOP/cycle for scalar ops), this allows **~500 simultaneous voices** -- far more than needed. Even with 8 voices of polyphony, CPU usage would be under 2%.

**This is one of the most CPU-efficient synthesis methods available.** The waveguide approach is inherently cheap because long resonances are modeled by delay lines (O(1) per sample) rather than long filter chains.

#### 14.5 Sample-Rate Dependent Delay Line Lengths

All delay line lengths must be recalculated when sample rate changes:

```
L_bore = fs / f0 - L_embouchure - sum(L_tonehole_corrections)
L_embouchure = L_bore / 2   (approximately)
L_jet = tau * fs = (W / (0.4 * U_j)) * fs
```

Tone hole positions along the bore are fixed ratios of the total bore length, so their fractional delay filters recalculate based on `fs`.

#### 14.6 Note Transition Strategy

For legato transitions (changing fingering without re-articulating):
1. Smoothly interpolate tone hole states over 2-5 ms
2. Optionally cross-fade between two bore delay line tap positions
3. Maintain the jet oscillation throughout (no re-attack)

For tongued articulation (new note attack):
1. Briefly reduce breath pressure to zero (10-30 ms gap)
2. Change tone hole configuration during the silence
3. Ramp breath pressure back up with attack envelope

---

### 15. Key Academic References

#### Primary Sources (Foundational)

1. **Smith, J.O. (1992)** -- "Physical Modeling using Digital Waveguides." Computer Music Journal, 16(4). The foundational paper on waveguide synthesis. Establishes delay-line-based modeling of vibrating strings and air columns.

2. **Verge, M.P. (1995)** -- "Aeroacoustics of Confined Jets, with Applications to the Physical Modeling of Recorder-Like Instruments." PhD thesis, Eindhoven University of Technology. The definitive jet-drive model. Equations for jet formation, propagation, amplification, and labium interaction.

3. **de la Cuadra, P. (2005)** -- "The Sound of Oscillating Air Jets: Physics, Modeling and Simulation in Flute-Like Instruments." PhD thesis, Stanford University (CCRMA). Extended Verge's model with improved receptivity modeling and experimental validation. Available at: `https://ccrma.stanford.edu/~pdelac/research/MyPublishedPapers/Thesis.pdf`

4. **Keefe, D.H. (1990)** -- "Woodwind Tone-hole Acoustics and the Spectrum Transformation Function." PhD thesis / JASA publications. The standard acoustic model of tone holes as lumped elements with series and shunt impedances.

#### Jet Physics and Excitation

5. **Coltman, J.W. (1966)** -- "Resonance and Sounding Frequencies of the Flute." JASA 40(1). Established that sounding frequency is controlled by pipe resonance.

6. **Coltman, J.W. (1968)** -- "Sounding Mechanism of the Flute and Organ Pipe." JASA 44(4). Demonstrated dipole character of the flute sound source.

7. **Fletcher, N.H. (1976)** -- "Jet-Drive Mechanism in Organ Pipes." JASA 60(2). Quantitative jet-drive analysis relating jet velocity and geometry to oscillation conditions.

8. **Auvray, R., Fabre, B., and Lagree, P.-Y. (2012)** -- "Regime Change and Oscillation Thresholds in Recorder-Like Instruments." JASA 131(4), 1574-1585. Modeling of register transitions and overblowing behavior.

9. **Auvray, R., Ernoult, A., Fabre, B., and Lagree, P.-Y. (2014)** -- "Time-domain Simulation of Flute-Like Instruments: Comparison of Jet-Drive and Discrete-Vortex Models." JASA 136(1). Establishes validity domains for each model based on dynamic aspect ratio.

#### Digital Implementation

10. **Hanninen, R. and Valimaki, V. (1996)** -- "An Improved Digital Waveguide Model of a Flute with Fractional Delay Filters." Proc. Nordic Acoustical Meeting / ICMC 1996. Introduces fractional delay tone hole positioning and complete real-time flute model with overblowing.

11. **Scavone, G. (1997)** -- "Digital Waveguide Modeling of Woodwind Toneholes." ICMC Proceedings. Converts Keefe's transmission matrix to scattering junction form for waveguide implementation.

12. **Smith, J.O. and Scavone, G. (1997)** -- "The One-Filter Keefe Clarinet Tonehole." IEEE WASPAA. Simplified one-multiply, one-filter tone hole implementation.

13. **Abel, J., Smyth, T., and Smith, J.O. (2003)** -- "A Simple, Accurate Wall Loss Filter for Acoustic Tubes." Proc. DAFx 2003. Practical bore loss filter design.

14. **Cook, P.R. (1992)** -- "A Meta-Wind-Instrument Physical Model, and a Meta-Controller for Real Time Performance Control." PhD thesis, Stanford/CCRMA. Source of the cubic polynomial jet model used in STK.

#### Software Implementations

15. **STK (Synthesis ToolKit)** -- Perry Cook and Gary Scavone. Open-source C++ library with `Flute` class. Uses polynomial jet model, two delay lines, one-pole loss filter. Reference implementation. `https://ccrma.stanford.edu/software/stk/`

16. **Faust Physical Models Library** -- `physmodels` library includes `fluteModel` with jet table, embouchure, head/foot waveguide components. Lacks tone holes but provides clean bidirectional waveguide architecture. `https://faustlibraries.grame.fr/libs/physmodels/`

17. **flute-lv2** (Timo West) -- LV2 plugin implementing waveguide flute model. `https://github.com/timowest/flute-lv2`

18. **nbrochec/flute-physical-modelling** -- Python/Jupyter + Max/MSP implementation for educational exploration. `https://github.com/nbrochec/flute-physical-modelling`

---

## Appendix A: Recommended Model Architecture for O-Wind

Based on this research, the recommended architecture for the O-Wind plugin:

**Excitation**: Jet-drive model (Verge/de la Cuadra formulation) with:
- Bickley jet profile
- Delayed, amplified perturbation via fractional delay line
- `tanh` saturation at labium (not polynomial -- more stable)
- Turbulence noise injection scaled by U_j^2

**Resonator**: Bidirectional waveguide with:
- Fractional delay lines (Lagrange 3rd order)
- First-order viscothermal loss filter per direction
- Second-order open-end reflection filter
- 6-8 tone hole two-port scattering junctions (second-order IIR each)

**Player model**: 
- Breath pressure envelope with attack/sustain/release shaping
- Embouchure parameters: jet angle, lip coverage, jet width
- Pressure vibrato (4-6 Hz)

**Instrument variants**: Switchable parameter sets for concert flute, recorder, shakuhachi, with the recorder having fixed embouchure parameters and the shakuhachi having extended embouchure control range.

**Polyphony**: 4-8 voices should be more than sufficient for any musical use case given the ~135 ops/sample/voice cost.
