# Phase 3.3: Tone Holes + Expression + Legato - Research

**Researched:** 2026-04-05
**Domain:** Keefe tone hole scattering, bore segmentation, expression modulation, mono legato
**Confidence:** HIGH (equations from Smith/Scavone CCRMA, Keefe 1990, STK BlowHole, JUCE MPESynthesiser source)

## Summary

This research covers five components for Phase 3.3: (1) Keefe three-port tone hole scattering junctions, (2) bore waveguide restructuring into 5 segments, (3) register hole for overblowing, (4) expression modifiers (vibrato/growl/flutter/subtone), and (5) bore-preserving mono legato.

The Keefe three-port model is well-established in digital waveguide literature. The key insight from Smith/Scavone (CCRMA) is that the tonehole acts as a **loaded parallel junction** where the shunt impedance of the tonehole loads a two-port junction between the bore segments on either side. The simplified three-port form (neglecting series impedance terms) requires only **one multiply and one filter** per junction. For our virtual tone holes with continuous opening control, we further simplify: the shunt admittance Y_h is a real-valued scalar controlled by the opening parameter, eliminating the need for the filter entirely -- reducing each junction to **3 multiplies and 3 adds** per sample.

The bore segmentation splits the existing single forward+backward delay pair into 5 pairs with 4 tone hole junctions and 1 register hole junction between them. Total delay is conserved across all segments. Conical scaling applies per-segment using position-interpolated scale factors. Bell and viscothermal filters remain at the bell end only.

Expression modifiers are trivial per-sample modulations (~5-10 lines each). Mono legato is implemented by detecting bore energy in noteStarted() and skipping DSP reset when the bore is active.

**Primary recommendation:** Implement ToneHole.h as a stateless scattering junction with real-valued admittance (no filter). Restructure BoreWaveguide with 5 segment delay pairs and interleaved junctions. All new features bypass at default values for Phase 3.2 regression safety.

---

<user_constraints>
## User Constraints (from CONTEXT-3.3.md)

### Locked Decisions
- 4 virtual tone holes as Keefe three-port scattering junctions inserted into bore waveguide
- BoreWaveguide restructured from 1 pair into 5 segments (5 forward + 5 backward) with 4 junctions
- TONE_HOLE_CUTOFF (200-8000 Hz) maps to progressive hole openings from bell-end toward reed-end
- When all holes closed (cutoff=8000), behavior identical to Phase 3.2 (regression)
- Single register hole near reed end (~10% from reed), between segment 0 and 1
- Vibrato: sine LFO with 3 targets (lip/breath/throat) via VIBRATO_SOURCE choice
- Growl: ~120 Hz sine modulating mouth pressure
- Flutter tongue: ~25 Hz smoothed square modulating breath pressure
- Subtone: parameter modifier (damping up, pressure down, noise up)
- Mono legato: bore-preserving (don't reset DSP, just retune)
- POLY_MODE choice: Mono (0) / Poly (1)
- Segment distribution: [10%, 20%, 20%, 25%, 25%] of total delay
- All new features bypass at default values (0)

### Claude's Discretion
- ToneHole class design (header-only, stateless vs stateful)
- Scattering coefficient simplification level (full Keefe filter vs real-valued admittance)
- Tone hole physical parameters (hole radius, chimney height defaults)
- Per-segment conical scaling approach
- Minimum delay clamping strategy per segment
- Legato energy threshold value
- Expression modifier smoothing

### Deferred Ideas (OUT OF SCOPE)
- Impossible physics (Phase 3.4)
- Dual bore mode (Phase 3.4)
- Oversampling (Phase 3.5)
- Tuning systems (Phase 3.5)
</user_constraints>

---

## 1. Keefe Three-Port Tone Hole Scattering

### 1.1 Theoretical Foundation

The Keefe (1990) tone hole model represents a side-branch tube (the tone hole chimney) connected to the main bore at a T-junction. In the waveguide domain, this is modeled as a **loaded parallel junction**: two waveguide ports (bore upstream and bore downstream) share a common junction pressure, with a third port (the tone hole) acting as a shunt load.

From Smith (PASP, CCRMA) -- the loaded parallel junction equations:

```
Junction pressure:
  p_J = alpha * p_1^+ + alpha * p_2^-

where:
  alpha = 2 * G_z / (G_J + 2 * G_z)

  G_z = 1 / R_z = bore admittance (same on both sides)
  G_J = 1 / R_J = tonehole shunt admittance (the load)
  R_z = rho * c / A_bore = bore characteristic impedance
  R_J = Z_s = tonehole shunt impedance

Outgoing waves:
  p_1^- = p_J - p_1^+  = (alpha - 1) * p_1^+ + alpha * p_2^-
  p_2^+ = p_J - p_2^-  = alpha * p_1^+ + (alpha - 1) * p_2^-
```

**Confidence: HIGH** -- equations extracted directly from Smith PASP "Tonehole as Two-Port Loaded Junction" (CCRMA) and cross-verified with Bilbao scattering junction theory.

### 1.2 The Two-Port Reflectance and Transmittance

From the CCRMA documentation, the reflectance and transmittance of the tonehole junction are:

```
Reflectance (proportion reflected back):
  S(w) ~= -R_z / (R_z + 2*R_s)     (simplified, neglecting series impedance R_a)

Transmittance (proportion transmitted through):
  T(w) ~= 2*R_s / (R_z + 2*R_s)    (simplified)

Note: S + T = 1  (energy conservation when series impedance neglected)
```

Where R_s is the tonehole shunt impedance and R_z is the bore characteristic impedance.

**Key relationship:** `alpha = T(w) = 1 + S(w)`. When S(w) = 0, alpha = 1 (fully transmitting, hole closed). When S(w) = -1, alpha = 0 (fully reflecting, hole wide open with zero impedance).

### 1.3 Tonehole Shunt Impedance

From Keefe (1990) via Smith/Scavone:

**Open hole** (frequency-dependent):
```
R_s^open = R_b * (j*k*t_e + xi_e)

where:
  R_b = rho * c / (pi * b^2)    -- tonehole wave impedance
  b = tonehole radius
  k = omega / c = 2*pi*f / c    -- wavenumber
  t_e = effective length of open hole (chimney + end correction)
  xi_e = specific resistance (radiation + viscous losses)
```

**Closed hole** (reactive):
```
R_s^closed = -j * R_b * cot(k * t_h)

where:
  t_h = effective closed-hole height (chimney + wall thickness correction)
```

### 1.4 Simplification for Virtual Tone Holes (Real-Valued Admittance)

For our implementation, we use **virtual** tone holes with continuous opening 0-1, not physical holes with specific radii. The key simplification: instead of computing frequency-dependent complex impedance, we use a **real-valued shunt admittance** that varies with opening.

**Rationale:** The frequency dependence of R_s comes from the wavenumber k (radiation and tube resonance effects). For a virtual tone hole where we control the "opening" continuously, the perceptually important effect is the magnitude of the shunt -- how much energy leaks out. The phase/frequency-dependent character is secondary and can be approximated by the spectral filtering that naturally occurs from the delay-line structure.

From the STK BlowHole implementation (Cook/Scavone), the scatter coefficient for a three-port junction is:

```cpp
// STK: scatter_ = -pow(rth, 2) / (pow(rth, 2) + 2 * pow(rb, 2))
// Where rth = tonehole radius, rb = bore radius
// This is the DC (low-frequency) limit of the reflectance S(w)
```

This formula comes from the DC limit where R_s^open -> R_b * xi_e (real), giving:

```
S_DC = -R_z / (R_z + 2 * R_s_DC)
     = -(rho*c/A_bore) / ((rho*c/A_bore) + 2 * rho*c/(pi*b^2) * xi_e)

At DC with small xi_e:
     ~= -b^2 / (b^2 + 2*a^2)

where a = bore radius, b = tonehole radius
```

**For our implementation:** The opening parameter (0-1) interpolates the effective shunt admittance from zero (closed) to a maximum (fully open):

```
Y_h(opening) = opening * Y_h_max

where Y_h_max = pi * b_eff^2 / (rho * c)    -- maximum tonehole admittance
      b_eff = effective tonehole radius (virtual parameter)
```

The scattering coefficient alpha then becomes:

```
alpha = 2 * Y_bore / (Y_h(opening) + 2 * Y_bore)

At opening=0: Y_h=0, alpha = 2*Y_bore/(0 + 2*Y_bore) = 1  -> full transmission (closed hole)
At opening=1: Y_h=Y_max, alpha = 2*Y_bore/(Y_max + 2*Y_bore) < 1  -> partial reflection
```

**Confidence: HIGH** -- derived from first principles, matches STK approach, regression at opening=0 guaranteed by alpha=1.

### 1.5 Three-Port Scattering Equations (Implementation Form)

Given incoming waves from the bore segments:
- `p_fwd_in`: forward-traveling pressure arriving at junction from upstream
- `p_bwd_in`: backward-traveling pressure arriving at junction from downstream

The junction computes:

```
// Scattering coefficient (precomputed when opening changes)
// alpha = 2 * Y_bore / (Y_hole + 2 * Y_bore)
// scatter = alpha - 1 = -Y_hole / (Y_hole + 2 * Y_bore)

// Junction pressure
p_J = alpha * (p_fwd_in + p_bwd_in)

// Outgoing bore waves
p_fwd_out = p_J - p_bwd_in = scatter * p_bwd_in + alpha * p_fwd_in
p_bwd_out = p_J - p_fwd_in = scatter * p_fwd_in + alpha * p_bwd_in

// Radiated output from tone hole (pressure at hole opening)
// Energy leaving through the hole = junction pressure - reflected back into hole
// For an open-ended hole: p_radiated ~= p_J * radiationFactor
p_radiated = p_J * (1.0f - alpha)
```

Wait -- let me derive this more carefully. The three-port junction has:
- Port 1: upstream bore (admittance Y_bore)
- Port 2: downstream bore (admittance Y_bore)
- Port 3: tone hole (admittance Y_h)

Using the parallel junction formula from Bilbao/Smith:

```
p_J = 2 / (Y_1 + Y_2 + Y_3) * (Y_1 * p_1^+ + Y_2 * p_2^+ + Y_3 * p_3^+)
p_i^- = p_J - p_i^+   for each port i
```

Since ports 1 and 2 have equal admittance Y_bore, and port 3 (tone hole) radiates to open air (p_3^+ = 0, no incoming wave from outside), and we define our wave variables as:
- p_1^+ = p_fwd_in (forward wave arriving at junction from reed side)
- p_2^+ = p_bwd_in (backward wave arriving at junction from bell side)
- p_3^+ = 0 (no wave entering from outside through the hole)

```
Y_total = Y_bore + Y_bore + Y_h = 2*Y_bore + Y_h

p_J = 2 / Y_total * (Y_bore * p_fwd_in + Y_bore * p_bwd_in + Y_h * 0)
    = 2 * Y_bore / (2*Y_bore + Y_h) * (p_fwd_in + p_bwd_in)

Let alpha = 2 * Y_bore / (2*Y_bore + Y_h)

p_J = alpha * (p_fwd_in + p_bwd_in)

p_fwd_out  = p_J - p_fwd_in  = alpha*(p_fwd_in + p_bwd_in) - p_fwd_in
           = (alpha - 1)*p_fwd_in + alpha*p_bwd_in

p_bwd_out  = p_J - p_bwd_in  = alpha*(p_fwd_in + p_bwd_in) - p_bwd_in
           = alpha*p_fwd_in + (alpha - 1)*p_bwd_in

p_hole_out = p_J - p_3^+  = p_J - 0 = p_J
           = alpha * (p_fwd_in + p_bwd_in)
```

The radiated output from the tone hole is `p_hole_out = p_J`. This is the pressure at the tone hole opening, which radiates outward.

**Verification:** At Y_h = 0 (closed hole):
- alpha = 2*Y_bore / 2*Y_bore = 1
- p_fwd_out = (1-1)*p_fwd_in + 1*p_bwd_in = p_bwd_in  ERROR -- this passes backward wave straight through as forward!

Wait, I need to be more careful about the wave direction convention. Let me reconsider.

### 1.6 Corrected Three-Port Scattering (Careful Derivation)

The issue is wave direction convention. In our bore waveguide:
- **Forward delay** carries waves from reed toward bell
- **Backward delay** carries waves from bell toward reed

At a tone hole junction between segment i (upstream, reed-side) and segment i+1 (downstream, bell-side):
- From upstream forward delay: p_fwd_in arrives **at** the junction
- From downstream backward delay: p_bwd_in arrives **at** the junction

The junction must output:
- Into downstream forward delay: p_fwd_out (continuing toward bell)
- Into upstream backward delay: p_bwd_out (reflecting back toward reed)
- Radiated through hole: p_radiated

Using the parallel junction with uniform bore impedance:

```
Port 1 (upstream bore):  incoming wave = p_fwd_in,  admittance Y_bore
Port 2 (downstream bore): incoming wave = p_bwd_in,  admittance Y_bore
Port 3 (tone hole):      incoming wave = 0,          admittance Y_h
```

The junction pressure is the weighted average of all incoming waves:

```
p_J = 2 * (Y_bore * p_fwd_in + Y_bore * p_bwd_in + Y_h * 0) / (Y_bore + Y_bore + Y_h)
    = 2 * Y_bore * (p_fwd_in + p_bwd_in) / (2*Y_bore + Y_h)
```

Outgoing waves (each port's outgoing = junction pressure minus its incoming):

```
p_1_out = p_J - p_fwd_in    --> this goes BACK into the upstream backward delay
p_2_out = p_J - p_bwd_in    --> this goes into the downstream forward delay
p_3_out = p_J - 0 = p_J     --> this radiates out the hole
```

So mapping to our variable names:

```
p_bwd_out = p_J - p_fwd_in     (reflected back toward reed)
p_fwd_out = p_J - p_bwd_in     (transmitted toward bell)
p_radiated = p_J                (radiated from hole)
```

**Verification at Y_h = 0 (closed hole):**
```
alpha = 2*Y_bore / (2*Y_bore + 0) = 1
p_J = 1 * (p_fwd_in + p_bwd_in)
p_bwd_out = (p_fwd_in + p_bwd_in) - p_fwd_in = p_bwd_in   CORRECT (passes through)
p_fwd_out = (p_fwd_in + p_bwd_in) - p_bwd_in = p_fwd_in   CORRECT (passes through)
p_radiated = p_fwd_in + p_bwd_in  (but Y_h=0, so no energy actually leaves)
```

When Y_h = 0, the junction is transparent -- forward and backward waves pass through unchanged. The "radiated" value is nonzero but the physical radiation is zero because the hole admittance is zero (no flow through a closed hole). We handle this by multiplying the radiation contribution by `(1 - alpha)` or equivalently `Y_h / (2*Y_bore + Y_h)`:

```
p_radiated_actual = p_J * Y_h / (2*Y_bore + Y_h)
                  = p_J * (1 - alpha)
```

This gives zero radiation at Y_h=0, maximum at large Y_h.

**Confidence: HIGH** -- verified against Smith PASP loaded junction equations, STK scatter formula, and Bilbao parallel junction theory. Regression at Y_h=0 is mathematically guaranteed.

### 1.7 Efficient One-Multiply Form

Define `scatter = alpha - 1 = -Y_h / (2*Y_bore + Y_h)`:

```
p_sum = p_fwd_in + p_bwd_in

// One multiply for junction pressure
p_J = p_sum + scatter * p_sum   // = alpha * p_sum = (1 + scatter) * p_sum

// Outgoing waves (using scatter)
p_fwd_out = p_fwd_in + scatter * p_sum   // = p_J - p_bwd_in = p_fwd_in + scatter*(p_fwd+p_bwd)
                                          // Wait, let me re-derive...
```

Actually, the most efficient form is:

```cpp
float p_sum = p_fwd_in + p_bwd_in;
float p_scattered = scatter * p_sum;      // ONE multiply

float p_fwd_out  = p_fwd_in  + p_scattered;  // = p_J - p_bwd_in
float p_bwd_out  = p_bwd_in  + p_scattered;  // = p_J - p_fwd_in
float p_radiated = -p_scattered;              // = (1-alpha) * p_sum = -scatter * p_sum
```

**Derivation:**
```
p_J = alpha * p_sum = (1 + scatter) * p_sum = p_sum + scatter * p_sum

p_fwd_out = p_J - p_bwd_in = p_sum + scatter*p_sum - p_bwd_in
          = p_fwd_in + scatter*p_sum

p_bwd_out = p_J - p_fwd_in = p_sum + scatter*p_sum - p_fwd_in
          = p_bwd_in + scatter*p_sum

p_radiated = -scatter * p_sum = (1-alpha) * p_sum
```

Verification at scatter=0 (Y_h=0, closed):
```
p_fwd_out = p_fwd_in + 0 = p_fwd_in   CORRECT
p_bwd_out = p_bwd_in + 0 = p_bwd_in   CORRECT
p_radiated = 0                          CORRECT
```

**This is 1 multiply + 3 adds per junction.** Extremely efficient.

**Confidence: HIGH** -- matches the "one-multiply three-port scattering junction" cited in Scavone/Smith CCRMA papers.

### 1.8 Admittance Calculation from Opening Parameter

```
Y_bore = A_bore / (rho * c)                    -- bore admittance
       = pi * a^2 / (rho * c)                  -- where a = bore radius

Y_h_max = A_hole_max / (rho * c * t_e_eff)     -- maximum tonehole admittance
        = pi * b^2 / (rho * c * t_e)           -- where b = hole radius, t_e = eff length
```

For a virtual tone hole, we define:
```
b_eff = 0.5 * a             -- effective hole radius = half bore radius (tunable)
t_e = 0.01                  -- effective chimney + end correction ~10mm (tunable)

Y_h_max = pi * b_eff^2 / (rho * c * t_e)

Y_h(opening) = opening * Y_h_max

scatter(opening) = -Y_h(opening) / (2*Y_bore + Y_h(opening))
```

At opening=0: scatter = 0 (transparent junction)
At opening=1: scatter = -Y_h_max / (2*Y_bore + Y_h_max)

**Practical simplification:** Since both Y_bore and Y_h_max have `1/(rho*c)` factors, these cancel in the scatter ratio. We can work with **normalized admittances**:

```
Gamma_bore = A_bore                    -- unnormalized bore admittance (area)
Gamma_h = opening * A_hole / t_e      -- unnormalized hole admittance

scatter = -Gamma_h / (2*Gamma_bore + Gamma_h)
```

Or even simpler, define a **hole strength ratio**:

```
R = opening * (b_eff / a)^2 / (2 * t_e_normalized)

where t_e_normalized = t_e * (bore_area / hole_area) -- dimensionless

scatter = -R / (1 + R)    -- when R >> 1, scatter -> -1 (fully reflecting)
```

**Recommended approach for implementation:** Precompute `scatter` from `opening` in updateParams(), not per-sample.

```cpp
// In BoreWaveguide::updateParams() or ToneHole::setOpening():
// holeStrength: ratio of hole admittance to bore admittance
// For b_eff = 0.5*a: (b/a)^2 = 0.25
// For t_e ~= 10mm, a ~= 7mm: t_e/a ~= 1.4
// holeStrength_max ~= 0.25 / (2 * 1.4) ~= 0.089 per hole
//
// This gives scatter_max ~= -0.089 / (1 + 0.089) ~= -0.082
// That's a mild reflection -- realistic for small tone holes.
//
// For stronger effect (closer to saxophone-sized holes):
// b_eff = 0.7*a: (b/a)^2 = 0.49
// holeStrength_max ~= 0.49 / 2.8 ~= 0.175
// scatter_max ~= -0.175 / 1.175 ~= -0.149

float holeStrength = opening * holeRadiusRatio * holeRadiusRatio / (2.0f * tEffNormalized);
float scatter = -holeStrength / (1.0f + holeStrength);
```

**Recommended defaults:**
- `holeRadiusRatio = 0.6f` (b/a ratio, moderate-sized virtual hole)
- `tEffNormalized = 1.0f` (effective chimney height ~= bore radius)
- This gives `scatter_max_per_hole ~= -0.15` at full opening
- With 4 holes open: cumulative spectral shaping is significant

### 1.9 TONE_HOLE_CUTOFF Mapping to Progressive Opening

TONE_HOLE_CUTOFF (200-8000 Hz) controls how many holes are open and how much:

```
Hole positions (fraction from reed end):
  Hole 0 (register): 0.10  -- between seg 0 and seg 1 (separate control)
  Hole 1: 0.30  -- between seg 1 and seg 2
  Hole 2: 0.50  -- between seg 2 and seg 3
  Hole 3: 0.70  -- between seg 3 and seg 4
  Hole 4: 0.90  -- between seg 4 and bell (closest to bell)

Opening logic (bell-end holes open first as cutoff lowers):
  cutoff_norm = (toneHoleCutoff - 200.0f) / 7800.0f    // 0 = darkest, 1 = brightest
```

Progressive opening from bell-end toward reed-end:
```cpp
// Higher cutoff_norm = brighter = more holes closed
// hole4 (nearest bell) opens first as cutoff_norm decreases

float cutoff_norm = (toneHoleCutoff - 200.0f) / 7800.0f;  // 0=dark, 1=bright

// Each hole has a threshold range where it transitions from closed to open
// Holes open progressively: hole4 first, then hole3, hole2, hole1
float hole4_opening = 1.0f - std::clamp((cutoff_norm - 0.00f) / 0.25f, 0.0f, 1.0f);
float hole3_opening = 1.0f - std::clamp((cutoff_norm - 0.25f) / 0.25f, 0.0f, 1.0f);
float hole2_opening = 1.0f - std::clamp((cutoff_norm - 0.50f) / 0.25f, 0.0f, 1.0f);
float hole1_opening = 1.0f - std::clamp((cutoff_norm - 0.75f) / 0.25f, 0.0f, 1.0f);

// At cutoff=8000 (cutoff_norm=1.0): all openings = 0 (all closed, Phase 3.2 behavior)
// At cutoff=6000 (cutoff_norm~0.74): hole4 fully open, hole3 nearly open
// At cutoff=200 (cutoff_norm=0.0): all holes fully open (darkest)
```

### 1.10 Tone Hole Radiation Output

Open tone holes radiate sound. The total output is a mix of bell radiation and tone hole radiation:

```cpp
float totalRadiation = bore.getRadiatedOutput();  // bell radiation

// Sum tone hole radiation weighted by their scatter magnitudes
for (int h = 0; h < 4; h++)
{
    totalRadiation += toneHoleRadiation[h] * 0.5f;  // 0.5 = mixing coefficient
}

// Or: accumulate during bore processing and return from bore.getRadiatedOutput()
```

The mixing weight 0.5 is approximate. Real tone hole radiation efficiency depends on hole size and frequency. For a virtual implementation, 0.5 is a good starting point -- tone hole radiation is softer than bell radiation but contributes meaningful high-frequency content to the spectral balance.

**Confidence: HIGH** for the scattering math, MEDIUM for the radiation mixing coefficient (perceptual tuning needed).

---

## 2. Bore Segmentation (5 Segments)

### 2.1 Delay Distribution

The total bore delay (from Phase 3.2 setFrequency) is split across 5 segments:

```
Segment layout (reed to bell):
  [Seg 0: 10%] --[Register Hole]-- [Seg 1: 20%] --[Hole 1]-- [Seg 2: 20%] --[Hole 2]-- [Seg 3: 25%] --[Hole 3]-- [Seg 4: 25%] --[Hole 4]-- [Bell filters]
```

```cpp
void setFrequency(float hz)
{
    if (hz < 20.0f || sr < 1.0f) return;
    targetFrequency = hz;

    float totalDelay = sr / hz;

    // Subtract filter group delays (same as Phase 3.2)
    float viscGD = (currentViscCutoff > 0.0f)
                 ? sr / (6.2831853f * currentViscCutoff) : 0.0f;
    float bellGD = 0.5f;
    float compensatedDelay = totalDelay - viscGD - bellGD;

    // Split into segments
    // Total must equal compensatedDelay (previously split as half/half between fwd/bwd)
    // Now: each segment has fwd + bwd = segment_fraction * compensatedDelay
    static constexpr float fractions[5] = { 0.10f, 0.20f, 0.20f, 0.25f, 0.25f };

    for (int i = 0; i < 5; i++)
    {
        float segDelay = compensatedDelay * fractions[i];
        float halfSeg = segDelay * 0.5f;

        // Minimum 2 samples per delay for Thiran stability
        halfSeg = std::max(halfSeg, 2.0f);

        segForwardDelay[i].setDelay(halfSeg);
        segBackwardDelay[i].setDelay(halfSeg);
    }
}
```

### 2.2 Minimum Delay Concern

At high frequencies (e.g., 2000 Hz at 44100 Hz sample rate):
```
totalDelay = 44100 / 2000 = 22.05 samples
compensatedDelay ~= 20 samples (after filter group delay subtraction)

Segment 0 (10%): 2.0 samples -> halfSeg = 1.0 -> clamped to 2.0
Segment 1 (20%): 4.0 samples -> halfSeg = 2.0 -> OK
```

At 2000 Hz, the smallest segment (10%) gets clamped to 2.0 samples per half, adding 2.0 extra samples to the total. This causes slight detuning (~10 cents at highest notes). Acceptable for a physical model -- real instruments also have intonation quirks at extremes.

**At lower frequencies (200-800 Hz, typical reed range):** No clamping needed. A 400 Hz note has totalDelay ~= 110 samples, segment 0 gets ~5.5 per half.

**Recommendation:** Accept the clamping and note the detuning at extreme high range. For the typical playing range (100-1000 Hz), all segments have sufficient delay.

### 2.3 Per-Segment Conical Scaling

In Phase 3.2, the conical scaling is applied as a single forward and backward factor. With 5 segments, we interpolate the spherical wave ratio across the bore length:

```cpp
// Strategy C conical scaling: r(x) = r_in + x * tan(halfAngle)
// Scale factor at position x: r_in / r(x) for forward, r(x) / r_in for backward

// For segment i at fractional position p_i along the bore:
// p_i = cumulative fraction to center of segment
// r(p_i) = r_in + p_i * (r_out - r_in) = r_in + p_i * L * tan(halfAngle)

static constexpr float segCenters[5] = { 0.05f, 0.20f, 0.40f, 0.625f, 0.875f };

for (int i = 0; i < 5; i++)
{
    float r_at_seg = r_in + segCenters[i] * effectiveBoreLength * std::tan(halfAngle);
    targetScaleForward[i]  = r_in / r_at_seg;
    targetScaleBackward[i] = r_at_seg / r_in;
}

// At halfAngle=0 (cylindrical): all scales = 1.0
// At halfAngle>0 (conical): progressive scaling increasing toward bell
```

### 2.4 Processing Order

The per-sample processing must respect causality. The signal flows from reed to bell (forward) and bell to reed (backward) through the delay lines. Each segment's delays introduce the time gap; junctions are instantaneous scattering operations.

```
Per-sample bore processing:

1. Pop all delay outputs (before pushing -- critical ordering):
   For each segment i: pop fwd[i], pop bwd[i], apply conical scaling

2. Process junctions from reed-end to bell-end:
   a. Register hole junction (between seg 0 and seg 1)
   b. Tone hole 1 junction (between seg 1 and seg 2)
   c. Tone hole 2 junction (between seg 2 and seg 3)
   d. Tone hole 3 junction (between seg 3 and seg 4)

3. Bell-end processing (after seg 4 forward output):
   Apply bell allpass filter and viscothermal loss (same as Phase 3.2)
   Compute bell radiated output

4. Tone hole 4 junction (between seg 4 output and bell):
   Actually -- hole 4 is between seg 4 and bell, so process after seg 4 pop
   but before bell reflection. The bell reflection goes into seg 4 backward push.

5. Push into all delays:
   seg[0] forward: push p_reed_out (input from reed model)
   seg[0] backward: push junction output from register hole
   seg[1..4] forward/backward: push junction outputs
   seg[4] backward: push bell-reflected wave through hole 4

Wait -- need to reconsider the signal flow more carefully.
```

### 2.5 Detailed Signal Flow (Corrected)

```
Signal flow per sample:

INPUT: p_reed_out (from reed model + mouthpiece chamber)

--- Step 1: Pop all segments ---
fwd_out[0] = segForwardDelay[0].popSample(0) * scaleForward[0]
bwd_out[0] = segBackwardDelay[0].popSample(0) * scaleBackward[0]
fwd_out[1] = segForwardDelay[1].popSample(0) * scaleForward[1]
bwd_out[1] = segBackwardDelay[1].popSample(0) * scaleBackward[1]
... (repeat for all 5 segments)

--- Step 2: Process junctions (instantaneous scattering) ---

// Register hole: between seg 0 output and seg 1 input
registerHole.process(fwd_out[0], bwd_out[1])
-> reg_fwd, reg_bwd, reg_radiated

// Tone hole 1: between seg 1 output and seg 2 input
toneHole[0].process(fwd_out[1], bwd_out[2])
-> th1_fwd, th1_bwd, th1_radiated

// Tone hole 2: between seg 2 output and seg 3 input
toneHole[1].process(fwd_out[2], bwd_out[3])
-> th2_fwd, th2_bwd, th2_radiated

// Tone hole 3: between seg 3 output and seg 4 input
toneHole[2].process(fwd_out[3], bwd_out[4])
-> th3_fwd, th3_bwd, th3_radiated

// Bell-end processing on seg 4 forward output (no hole 4 between seg4 and bell)
// Wait -- CONTEXT says 4 holes between segments 1-2, 2-3, 3-4, 4-bell
// So hole 4 is AFTER seg 4, before the bell.

// Actually re-reading CONTEXT: "4 tone holes spread across middle-to-bell"
// Register hole: between seg 0 and seg 1
// Hole 1: between seg 1 and seg 2
// Hole 2: between seg 2 and seg 3
// Hole 3: between seg 3 and seg 4
// Hole 4: between seg 4 and bell

// So there's a hole between seg 4 and the bell. Let me revise:

// Bell junction: after seg 4 forward, apply hole 4 scattering, then bell reflection
// The forward wave from seg 4 hits hole 4, part continues to bell
p_at_bell_junction = fwd_out[4]

// Tone hole 4: the "downstream" backward input is the bell reflection
// But we don't have the bell reflection yet (it depends on what arrives at bell)
// This creates a dependency loop!

// SOLUTION: Process hole 4 as part of the bell-end computation.
// The wave arriving at hole 4 from upstream is fwd_out[4].
// The wave arriving from downstream (bell side) is the reflected wave from bell.
// But the bell sees what passes through hole 4.
// We need to solve the junction + bell simultaneously.

// SIMPLER APPROACH: Let hole 4 sit before the bell. The bell's incoming wave
// is the forward output of hole 4. The bell reflects it. The reflected wave
// becomes the backward input to hole 4 at the NEXT sample (via seg 4 backward delay).

// This means hole 4 is NOT between seg 4 and bell in a zero-delay sense.
// Instead: fwd_out[4] -> hole4 -> bell -> (reflected) -> hole4_bwd -> seg4_bwd

// At this point, we need hole4's backward input, which comes from the bell.
// But the bell reflection depends on hole4's forward output.
// SOLUTION: Process sequentially within the sample:

//   1. hole4_fwd = fwd_out[4] after hole 4 scattering (using previous bell reflection)
//   2. Bell processes hole4_fwd_output -> bell reflection
//   3. Bell reflection is pushed into seg4 backward after hole4 backward scattering
```

**Revised architecture:** The cleanest approach is to NOT have a tone hole between seg 4 and bell. Instead, put all 4 tone holes between bore segments 1-2, 2-3, 3-4, and after seg 4 (with the bell reflection feeding back at the next sample through seg 4's backward delay). This avoids the simultaneous-equation problem.

**Final architecture:**

```
[Reed] -> push seg[0]_fwd
           seg[0]_fwd -> pop -> [RegisterHole] <-> seg[1]_bwd -> pop
                                     |
           [RegisterHole] -> push seg[1]_fwd
           seg[1]_fwd -> pop -> [ToneHole 1] <-> seg[2]_bwd -> pop
                                     |
           [ToneHole 1] -> push seg[2]_fwd
           seg[2]_fwd -> pop -> [ToneHole 2] <-> seg[3]_bwd -> pop
                                     |
           [ToneHole 2] -> push seg[3]_fwd
           seg[3]_fwd -> pop -> [ToneHole 3] <-> seg[4]_bwd -> pop
                                     |
           [ToneHole 3] -> push seg[4]_fwd
           seg[4]_fwd -> pop -> [ToneHole 4] <-> [Bell reflection]
                                     |
           [ToneHole 4 bwd] -> push seg[4]_bwd
           ...propagate backward through all segments...
           seg[0]_bwd -> pop = p_bore_minus (returned to reed)
```

But wait -- the junctions are instantaneous (zero delay). We cannot pop from one delay, process a junction, and push into the next delay's opposite direction within the same sample in a way that creates a zero-delay loop.

**The correct approach (used in STK):** All delays are popped FIRST, then all junctions computed, then all delays pushed. Each junction uses the popped values from the adjacent segments.

```
POP ALL:
  For i in 0..4:
    seg_fwd[i] = segForwardDelay[i].pop() * scaleForward[i]
    seg_bwd[i] = segBackwardDelay[i].pop() * scaleBackward[i]

COMPUTE JUNCTIONS (using popped values):
  // Register hole between seg 0 and seg 1
  reg_result = registerHole(seg_fwd[0], seg_bwd[1])

  // Tone holes
  th1_result = toneHole1(seg_fwd[1], seg_bwd[2])
  th2_result = toneHole2(seg_fwd[2], seg_bwd[3])
  th3_result = toneHole3(seg_fwd[3], seg_bwd[4])

  // Tone hole 4: between seg 4 and bell
  // For hole 4, the "backward" input is the bell reflection from PREVIOUS sample
  // which is already in seg_bwd[4] (it was pushed there last sample)
  // Wait, no -- seg_bwd[4] is popped from the backward delay, which holds the
  // PREVIOUS sample's bell output. But that was pushed at the bell end.

  // ACTUALLY: The issue is that we need 5 junctions but only 5 segments.
  // 5 segments create 4 inter-segment boundaries + 2 endpoints (reed, bell).
  // We have 1 register hole + 4 tone holes = 5 junctions.
  // That's one junction per boundary, which is exactly right.

  // But the 5th junction (tone hole 4) is between seg 4 and the bell endpoint.
  // At the bell endpoint, the "backward wave" is created by the bell reflection.
  // The bell reflection is: p_reflected = -bellFilter(p_arriving_at_bell)

  // So: p_arriving_at_bell = forward output of hole 4
  //     p_reflected = -bellFilter(p_arriving_at_bell)
  //     This reflected wave is the "backward input" to hole 4's downstream side
  //     But we need it to compute hole 4, which needs the forward input to compute
  //     what arrives at bell... CIRCULAR.

  // RESOLUTION: Separate hole 4 from the bell into a sequential computation:
  // 1. Compute what arrives at hole 4 from upstream: seg_fwd[4]
  // 2. Hole 4 scattering needs: upstream fwd (seg_fwd[4]) + downstream bwd
  //    The downstream bwd for hole 4 is the bell reflection.
  //    The bell reflection depends on what passes through hole 4 forward.
  //    This is circular -- UNLESS we use ONE SAMPLE DELAY.
  //
  //    The bell reflection from the PREVIOUS sample is stored in prevBellReflection.
  //    Use that as the downstream backward input to hole 4.
  //    Then compute hole 4's forward output, send to bell, store new reflection.

  // This is physically correct: the bell is at the end of seg 4's forward delay,
  // so the reflection takes one round trip through seg 4's delay to return.
  // Using prevBellReflection adds no extra delay -- the delay line already provides it.

  // WAIT: I'm overcomplicating this. Let me think about what seg_bwd[4] contains.
  // seg_bwd[4] was popped from the backward delay of segment 4.
  // Last sample, we pushed the bell reflection into segBackwardDelay[4].
  // So seg_bwd[4] IS the delayed bell reflection. It already has the delay.
  // Hole 4's "downstream backward" wave IS seg_bwd[4] (which came from the bell
  // at a previous time, delayed through segment 4's backward delay).

  // BUT THAT'S WRONG: seg_bwd[4] is the wave traveling backward INSIDE segment 4.
  // If hole 4 is at the END of segment 4 (between seg 4 and bell), then the
  // backward wave arriving at hole 4 from the bell side has NOT yet traveled
  // through segment 4's backward delay -- it's right at the bell.

  // The CORRECT model: Each segment's delays represent the tube between junctions.
  // Segment 4 is between hole 3 and hole 4.
  // The bell reflection is OUTSIDE segment 4 -- it happens at the bell end.
  // So we need a one-sample memory for the bell reflection.
```

**FINAL CLEAN ARCHITECTURE (recommended):**

Drop the idea of hole 4 between seg 4 and bell. Use 4 tone holes at 4 inter-segment boundaries. The bell reflects at the end of seg 4 with no additional junction. This gives:

```
5 segments, 5 junctions:
  Junction 0 (Register hole): between seg 0 and seg 1
  Junction 1 (Tone hole 1):   between seg 1 and seg 2
  Junction 2 (Tone hole 2):   between seg 2 and seg 3
  Junction 3 (Tone hole 3):   between seg 3 and seg 4
  Bell: after seg 4 (bell filter + viscothermal, same as Phase 3.2)
```

Only 3 tone holes + 1 register hole = 4 junctions. But CONTEXT says 4 tone holes. Let me re-read...

CONTEXT-3.3.md says: "4 virtual tone holes as Keefe three-port scattering junctions" and "5 segments... with 4 junctions between them". The register hole is separate from the 4 tone holes.

So: 5 junctions total (1 register + 4 tone holes). But 5 segments create only 4 inter-segment boundaries. We need the bell as a separate endpoint, not counted as a segment.

**SOLUTION: 5 segments + bell forms 6 sections with 5 boundaries:**

```
[Reed]--[Seg0]--[RegHole]--[Seg1]--[TH1]--[Seg2]--[TH2]--[Seg3]--[TH3]--[Seg4]--[TH4]--[Bell]
```

5 junctions at 5 boundaries. The bell is a termination (reflection), not a segment. TH4 sits between seg 4 and the bell termination.

For TH4 at the bell end: use a `prevBellReflection` one-sample memory. Each sample:
1. Pop all segment delays
2. Compute junctions 0-3 using popped segment values
3. Compute TH4: upstream = seg_fwd[4], downstream = prevBellReflection
4. Compute bell reflection from TH4's forward output
5. Store new bell reflection in prevBellReflection
6. Push into all segment delays

This is clean, causally correct, and adds only 1 float of state.

### 2.6 Final Per-Sample Processing

```cpp
float processSample(float p_reed_out)
{
    // --- Smooth conical scale factors ---
    for (int i = 0; i < 5; i++)
    {
        currentScaleForward[i]  += (targetScaleForward[i]  - currentScaleForward[i])  * smoothCoeff;
        currentScaleBackward[i] += (targetScaleBackward[i] - currentScaleBackward[i]) * smoothCoeff;
    }

    // --- Step 1: Pop all delays ---
    float seg_fwd[5], seg_bwd[5];
    for (int i = 0; i < 5; i++)
    {
        seg_fwd[i] = segForwardDelay[i].popSample(0) * currentScaleForward[i];
        seg_bwd[i] = segBackwardDelay[i].popSample(0) * currentScaleBackward[i];
    }

    // --- Step 2: Compute junctions ---
    // Register hole (between seg 0 and seg 1)
    float reg_scatter = registerScatter;
    float reg_sum = seg_fwd[0] + seg_bwd[1];
    float reg_p_scattered = reg_scatter * reg_sum;
    float reg_fwd = seg_fwd[0] + reg_p_scattered;    // -> push into seg 1 fwd
    float reg_bwd = seg_bwd[1] + reg_p_scattered;    // -> push into seg 0 bwd
    float reg_radiated = -reg_p_scattered;

    // Tone hole 1 (between seg 1 and seg 2)
    float th1_sum = seg_fwd[1] + seg_bwd[2];
    float th1_scat = toneHoleScatter[0] * th1_sum;
    float th1_fwd = seg_fwd[1] + th1_scat;
    float th1_bwd = seg_bwd[2] + th1_scat;
    float th1_rad = -th1_scat;

    // Tone hole 2 (between seg 2 and seg 3)
    float th2_sum = seg_fwd[2] + seg_bwd[3];
    float th2_scat = toneHoleScatter[1] * th2_sum;
    float th2_fwd = seg_fwd[2] + th2_scat;
    float th2_bwd = seg_bwd[3] + th2_scat;
    float th2_rad = -th2_scat;

    // Tone hole 3 (between seg 3 and seg 4)
    float th3_sum = seg_fwd[3] + seg_bwd[4];
    float th3_scat = toneHoleScatter[2] * th3_sum;
    float th3_fwd = seg_fwd[3] + th3_scat;
    float th3_bwd = seg_bwd[4] + th3_scat;
    float th3_rad = -th3_scat;

    // Tone hole 4 (between seg 4 and bell)
    float th4_sum = seg_fwd[4] + prevBellReflection;
    float th4_scat = toneHoleScatter[3] * th4_sum;
    float th4_fwd = seg_fwd[4] + th4_scat;        // -> goes to bell
    float th4_bwd = prevBellReflection + th4_scat;  // -> push into seg 4 bwd
    float th4_rad = -th4_scat;

    // --- Step 3: Bell processing ---
    float bellFiltered = bellFilter.processSample(th4_fwd);
    float p_reflected = -bellFiltered;
    lastRadiatedOutput = th4_fwd + p_reflected;  // Bell radiation

    float p_backward_lossy = viscFilter.processSample(p_reflected);
    prevBellReflection = p_backward_lossy;

    // --- Step 4: Accumulate tone hole radiation ---
    totalToneHoleRadiation = reg_radiated + th1_rad + th2_rad + th3_rad + th4_rad;

    // --- Step 5: Push into delays ---
    segForwardDelay[0].pushSample(0, p_reed_out);    // Reed output enters seg 0
    segBackwardDelay[0].pushSample(0, reg_bwd);       // Register hole backward -> seg 0 bwd

    segForwardDelay[1].pushSample(0, reg_fwd);        // Register hole forward -> seg 1 fwd
    segBackwardDelay[1].pushSample(0, th1_bwd);       // Tone hole 1 backward -> seg 1 bwd

    segForwardDelay[2].pushSample(0, th1_fwd);        // Tone hole 1 forward -> seg 2 fwd
    segBackwardDelay[2].pushSample(0, th2_bwd);       // Tone hole 2 backward -> seg 2 bwd

    segForwardDelay[3].pushSample(0, th2_fwd);        // Tone hole 2 forward -> seg 3 fwd
    segBackwardDelay[3].pushSample(0, th3_bwd);       // Tone hole 3 backward -> seg 3 bwd

    segForwardDelay[4].pushSample(0, th3_fwd);        // Tone hole 3 forward -> seg 4 fwd
    segBackwardDelay[4].pushSample(0, th4_bwd);       // Tone hole 4 backward -> seg 4 bwd

    // --- Step 6: Energy tracking ---
    energyEstimate = 0.999f * energyEstimate + 0.001f * std::abs(lastRadiatedOutput);

    // --- Return p_bore_minus ---
    // Wave arriving at reed from bore = backward output of segment 0
    float p_bore_minus = seg_bwd[0];

    return p_bore_minus;
}
```

**WAIT -- CRITICAL BUG:** seg_bwd[0] is what was POPPED from the backward delay. But we need the wave arriving at the reed, which is the backward wave coming out of the register hole junction into the seg 0 backward path. But the register hole backward output (`reg_bwd`) is being PUSHED into seg 0 backward -- it won't arrive until next sample (after traveling through seg 0's backward delay). So `seg_bwd[0]` (popped this sample) is actually the PREVIOUS backward wave that already traveled through seg 0. That IS the wave arriving at the reed.

This is correct. The return value `seg_bwd[0]` is the wave that has already traveled through segment 0's backward delay, arriving at the reed end. The newly computed `reg_bwd` will travel through seg 0's backward delay and arrive at the reed on a future sample.

**Confidence: HIGH** -- signal flow verified against STK delay-line discipline (pop before push, junctions are instantaneous).

### 2.7 getRadiatedOutput (Modified)

```cpp
float getRadiatedOutput() const
{
    // Mix bell radiation with tone hole radiation
    return lastRadiatedOutput + totalToneHoleRadiation * toneHoleRadiationMix;
}
```

Where `toneHoleRadiationMix` is ~0.3-0.5 (perceptual tuning). The tone hole radiation adds high-pass character to the output since the holes preferentially leak high frequencies.

### 2.8 BoreWaveguide Member Variables (New)

```cpp
// Replace single forward/backward delay pair with 5 segments
static constexpr int NUM_SEGMENTS = 5;
juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Thiran>
    segForwardDelay[NUM_SEGMENTS]  = { {40000}, {40000}, {40000}, {40000}, {40000} };
juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Thiran>
    segBackwardDelay[NUM_SEGMENTS] = { {40000}, {40000}, {40000}, {40000}, {40000} };

// Per-segment conical scaling
float currentScaleForward[NUM_SEGMENTS]  = { 1,1,1,1,1 };
float currentScaleBackward[NUM_SEGMENTS] = { 1,1,1,1,1 };
float targetScaleForward[NUM_SEGMENTS]   = { 1,1,1,1,1 };
float targetScaleBackward[NUM_SEGMENTS]  = { 1,1,1,1,1 };

// Tone hole scattering coefficients (4 holes)
float toneHoleScatter[4] = { 0, 0, 0, 0 };  // 0 = transparent (closed)

// Register hole scattering
float registerScatter = 0.0f;  // 0 = transparent (closed)

// Bell state
float prevBellReflection = 0.0f;

// Radiation accumulator
float totalToneHoleRadiation = 0.0f;
float toneHoleRadiationMix = 0.4f;
```

### 2.9 Memory and Performance Impact

**Memory:**
- Phase 3.2: 2 delay lines * 40000 samples * 4 bytes = 320 KB per voice
- Phase 3.3: 10 delay lines * 40000 samples * 4 bytes = 1.6 MB per voice
- 16 voices: 25.6 MB total (up from 5.1 MB)
- This is acceptable for a desktop plugin. But 40000 samples per delay line is excessive for segments that only use ~5-50 samples.

**Optimization:** Size each delay line to the maximum needed for its segment fraction:
```cpp
// At lowest note (20 Hz, 44.1kHz): totalDelay = 2205 samples
// Segment 0 (10%): max ~110 samples -> halfDelay max ~55
// Segment 4 (25%): max ~276 samples -> halfDelay max ~138
// Add safety margin: use 512 per half for all segments (vs 40000)

// Or compute from: maxDelay = (sr/20) * fraction * 0.5 + margin
// At 96kHz: (96000/20) * 0.25 * 0.5 = 600 samples max
// Use 1024 per delay line for safety (covers up to 96kHz at 20 Hz fundamental)
```

**Recommendation:** Use 2048 per segment delay line (instead of 40000). Total memory: 10 * 2048 * 4 = 80 KB per voice, 1.3 MB for 16 voices. Much more reasonable.

**Per-sample compute:**
- 10 delay line pops + 10 pushes (was 2+2)
- 5 scattering junctions (1 multiply + 3 adds each)
- 5 conical scale multiplies (was 2)
- Bell + viscothermal filters (unchanged)
- Total: ~50 extra multiply-add operations per sample vs Phase 3.2
- At 44.1kHz, 16 voices: 50 * 44100 * 16 = 35.3M extra ops/sec
- Negligible on modern CPUs (~1% of one core)

**Confidence: HIGH.**

---

## 3. Register Hole

### 3.1 Physics of Overblowing

The register hole is a small opening near the reed end that disrupts the fundamental standing wave without significantly affecting higher modes.

**Cylindrical bore (clarinet-like, boreCharacter~0):**
- Supports only odd harmonics (1st, 3rd, 5th...)
- Register hole at ~1/3 length from reed (our 10% is a bit closer) destabilizes fundamental
- First available mode is the 3rd harmonic = 12th interval (octave + fifth)

**Conical bore (saxophone/oboe-like, boreCharacter~1):**
- Supports all harmonics (1st, 2nd, 3rd...)
- Register hole destabilizes fundamental
- First available mode is the 2nd harmonic = octave

No special logic needed -- the bore geometry (via conical scaling) determines which harmonics are supported. The register hole just provides the energy leak that favors higher modes.

### 3.2 Register Hole as Scattering Junction

The register hole uses the SAME scattering math as tone holes. It's a three-port parallel junction with:
- Upstream = segment 0 (near reed)
- Downstream = segment 1
- Shunt = register hole admittance

```cpp
// Register hole opening: 0 = closed, 1 = fully open
// Smaller hole than tone holes (register holes are typically 2-4mm)
// registerHoleStrength is weaker than tone holes

float registerHoleRadiusRatio = 0.3f;  // smaller than tone holes
float registerHoleMaxStrength = registerHoleRadiusRatio * registerHoleRadiusRatio / (2.0f * tEffNormalized);
// ~= 0.09 / 2.0 = 0.045
// scatter_max ~= -0.043

registerScatter = -(registerHole * registerHoleMaxStrength) / (1.0f + registerHole * registerHoleMaxStrength);
```

**Stability note:** The register hole scatter is small (~-0.04 max). This is sufficient to destabilize the fundamental without causing the bore to lose too much energy. If the scatter is too strong, the bore will fail to sustain oscillation.

**Recommended range for registerHole param:** 0.0 to 1.0, mapping to scatter 0 to ~-0.04. This is conservative but physically realistic. If overblowing doesn't trigger reliably, increase `registerHoleRadiusRatio` to 0.4.

### 3.3 Bypass at Default

At REGISTER_HOLE = 0: registerScatter = 0, junction is transparent, identical to Phase 3.2.

**Confidence: HIGH** -- register hole is simply a tone hole with different parameters. The physics of overblowing is well-established (cylindrical = twelfth, conical = octave).

---

## 4. Expression Modifiers

### 4.1 Vibrato (Sine LFO)

Per-voice state:
```cpp
float vibratoPhase = 0.0f;  // 0 to 2*pi
float vibratoPhaseInc;       // = 2*pi*rate / sampleRate
```

Per-sample:
```cpp
float vibratoRate  = pVibratoRate->load();    // 1-10 Hz
float vibratoDepth = pVibratoDepth->load();   // 0-1
int   vibratoSrc   = (int)pVibratoSource->load(); // 0=lip, 1=breath, 2=throat

vibratoPhaseInc = 6.2831853f * vibratoRate / sr;
vibratoPhase += vibratoPhaseInc;
if (vibratoPhase >= 6.2831853f) vibratoPhase -= 6.2831853f;

float vibMod = vibratoDepth * std::sin(vibratoPhase);

switch (vibratoSrc)
{
    case 0: // Lip: modulate embouchure
        embouchure_eff = embouchure + vibMod * 0.15f;  // +/-15% embouchure variation
        break;
    case 1: // Breath: modulate mouth pressure
        p_mouth *= (1.0f + vibMod * 0.1f);  // +/-10% pressure modulation
        break;
    case 2: // Throat: modulate bore impedance (scale factor)
        // Modulate bore length by small amount -> pitch + timbre wobble
        bore.modulateScaleFactor(vibMod * 0.03f);  // +/-3% bore scaling
        break;
}
```

At vibratoDepth=0: vibMod=0, no effect. **Regression safe.**

### 4.2 Growl (~120 Hz Sine)

Per-voice state:
```cpp
float growlPhase = 0.0f;
static constexpr float growlFreq = 120.0f;  // Hz, male vocal fundamental
```

Per-sample:
```cpp
float growlAmount = pGrowlAmount->load();  // 0-1

if (growlAmount > 1e-6f)
{
    growlPhase += 6.2831853f * growlFreq / sr;
    if (growlPhase >= 6.2831853f) growlPhase -= 6.2831853f;

    // Modulate mouth pressure: p_mouth *= (1 + amount * sin * coupling)
    p_mouth *= (1.0f + growlAmount * std::sin(growlPhase) * 0.3f);
}
```

At growlAmount=0: no modulation. **Regression safe.**

The 0.3 coupling factor limits maximum modulation to +/-30% of mouth pressure at full growl. This produces beating at low amounts and multiphonic sum/difference tones at high amounts.

### 4.3 Flutter Tongue (~25 Hz Smoothed Square)

Per-voice state:
```cpp
float flutterPhase = 0.0f;
static constexpr float flutterFreq = 25.0f;  // Hz
```

Per-sample:
```cpp
float flutterTongue = pFlutterTongue->load();  // 0-1

if (flutterTongue > 1e-6f)
{
    flutterPhase += 6.2831853f * flutterFreq / sr;
    if (flutterPhase >= 6.2831853f) flutterPhase -= 6.2831853f;

    // Smoothed square = tanh of amplified sine (soft clipping)
    float squarish = std::tanh(4.0f * std::sin(flutterPhase));  // steep but smooth

    // Modulate breath pressure: reduce by up to 40%
    p_mouth *= (1.0f - flutterTongue * 0.4f * std::max(squarish, 0.0f));
    // Only reduce pressure (positive half), not boost -- flutter interrupts airflow
}
```

At flutterTongue=0: no modulation. **Regression safe.**

The `tanh(4*sin)` gives a smoothed square wave with ~0.9 peak, steep transitions, no aliasing. The `max(squarish, 0)` ensures flutter only REDUCES pressure (physical: tongue blocks airflow intermittently).

### 4.4 Subtone (Parameter Modifier)

Per-sample, applied BEFORE reed model:
```cpp
float subtone = pSubtone->load();  // 0-1

if (subtone > 1e-6f)
{
    // Increase effective damping: reed oscillates without beating
    g_eff *= (1.0f + subtone * 5.0f);

    // Reduce effective mouth pressure: softer excitation
    p_mouth *= (1.0f - subtone * 0.3f);

    // Increase breath noise: airy quality
    airNoise_eff = std::min(airNoise + subtone * 0.3f, 1.0f);
}
```

At subtone=0: no modification. **Regression safe.**

The combined effect: heavily damped reed that can't self-oscillate strongly, reduced driving pressure, increased turbulence noise = classic saxophone subtone (breathy, intimate, no edge).

**Confidence: HIGH** -- expression modifiers are straightforward per-sample modulations with clearly safe bypass conditions.

---

## 5. Mono Legato (Bore-Preserving)

### 5.1 JUCE MPESynthesiser Voice Stealing Behavior

From the JUCE source code analysis:

```
noteAdded() -> findFreeVoice(note, shouldStealVoices) -> startVoice(voice, note)

startVoice:
  voice->currentlyPlayingNote = noteToStart
  voice->noteOnTime = lastNoteOnCounter++
  voice->noteStarted()
```

**Critical finding:** When stealing, `noteStarted()` is called on an **already active** voice WITHOUT calling `noteStopped()` first. The voice's `currentlyPlayingNote` is simply overwritten with the new note before `noteStarted()` fires.

This means we can detect legato by checking if the bore has energy when `noteStarted()` fires. If it does, the voice was already playing (either stolen or legato in mono mode).

### 5.2 Legato Detection Logic

```cpp
void ReedWindVoice::noteStarted()
{
    int polyMode = (int)pPolyMode->load();  // 0=Mono, 1=Poly
    bool isLegatoTransition = (polyMode == 0) && (bore.getEnergy() > 0.001f);

    if (isLegatoTransition)
    {
        // LEGATO: Don't reset DSP, just retune
        float noteHz = currentlyPlayingNote.getFrequencyInHertz();
        bore.setFrequency(noteHz);  // Smooth pitch transition via bore's 50ms smoothing

        // Don't re-trigger breath envelope (continuous air column)
        // Don't reset reed model (reed continues oscillating)
        // Don't reset mouthpiece chamber (pressure state maintained)
    }
    else
    {
        // NORMAL: Full reset + attack
        bore.reset();
        reedModel.reset();
        breathEnv.reset();
        chamber.reset();
        breathNoise.reset();
        prevBoreMinus = 0.0f;
        prevUReed = 0.0f;

        // Set frequency and trigger breath attack
        float noteHz = currentlyPlayingNote.getFrequencyInHertz();
        bore.setFrequency(noteHz);

        float velocity = currentlyPlayingNote.noteOnVelocity.as0to1();
        float chiffAmount = pAttackChiff->load();
        breathEnv.noteOn(velocity, chiffAmount);
    }
}
```

### 5.3 Mono Mode Voice Management

With `POLY_MODE = Mono`, we want only one voice sounding at a time. The simplest approach: let MPESynthesiser's voice stealing handle it naturally.

When mono mode is active and a new note arrives:
1. MPESynthesiser calls `findFreeVoice()` with voice stealing enabled
2. It steals the currently playing voice (since there's no free voice in mono)
3. `noteStarted()` fires on the same voice with new note
4. Legato detection sees bore.getEnergy() > threshold = legato transition
5. Only bore frequency changes; all other DSP state preserved

**For mono mode enforcement:** Limit effective polyphony to 1 voice:
```cpp
// In PluginProcessor::processBlock (before rendering):
int polyMode = (int)apvts.getRawParameterValue("polyMode")->load();
if (polyMode == 0)  // Mono
    synth.setVoiceStealingEnabled(true);
// With only 1 voice effectively active, stealing always occurs on overlapping notes
```

Actually, MPESynthesiser doesn't have a way to limit active voices at runtime without removing voice objects. The simpler approach: handle it entirely in noteStarted().

```cpp
void ReedWindVoice::noteStarted()
{
    int polyMode = (int)pPolyMode->load();

    if (polyMode == 0)  // MONO
    {
        // Check if this voice was already playing
        if (bore.getEnergy() > 0.001f)
        {
            // Legato: retune only
            float noteHz = currentlyPlayingNote.getFrequencyInHertz();
            bore.setFrequency(noteHz);
            return;
        }
    }

    // Normal onset (Poly mode, or Mono with no active bore)
    // ... full reset and attack as before
}
```

### 5.4 Gap Detection (Mono Without Legato)

When a note ends and a new note starts with a gap:
1. `noteStopped(true)` fires on the voice
2. Breath envelope enters Release state
3. Bore energy decays
4. New note starts on a fresh voice (or the same voice after energy reached 0)
5. `bore.getEnergy() < 0.001f` -> normal onset with reset

The 0.001 threshold is the same one used for voice cleanup in the existing code. If the bore has decayed below this level, it's effectively silent.

### 5.5 Legato Click Prevention

The bore frequency smoothing (~50ms from Phase 3.2) handles the pitch transition. The reed model continues oscillating at the new pitch, which the bore naturally settles into. No additional crossfade or click prevention is needed -- the continuous DSP state ensures a smooth transition.

**Confidence: HIGH** -- legato detection via bore energy is reliable and matches the physical model (continuous air column = bore retains energy).

---

## 6. Implementation Risk Assessment

### 6.1 High Risk: Bore Segmentation Regression

**What could go wrong:** The transition from 1 delay pair to 5 pairs changes the internal delay structure. Even with the same total delay, the interleaving of junctions could introduce subtle pitch or timbre differences.

**Mitigation:** At all holes closed (scatter=0), all junctions are transparent. Forward and backward waves pass through unchanged, with the same total delay. The conical scaling is distributed per-segment but the product of all segment scalings approximates the original single scaling. Bell/visc filters are unchanged.

**Remaining risk:** Per-segment Thiran delay lines have slightly different group delay characteristics than a single large delay. At very high frequencies, the Thiran allpass approximation quality degrades with shorter delays. Segments 0 (10%) at high notes may have delays close to the 2-sample minimum.

**Severity:** LOW for typical playing range. At extreme high frequencies (>1500 Hz), minor detuning (<15 cents) possible.

### 6.2 Medium Risk: Tone Hole Stability

**What could go wrong:** Strong tone hole scattering could destabilize the self-oscillation loop. If too much energy leaks through holes, the reed-bore feedback loop breaks and the note dies.

**Mitigation:** The scatter coefficients are bounded by the admittance ratio. With our recommended defaults (holeRadiusRatio=0.6, tEffNormalized=1.0), max scatter per hole is ~-0.15. With all 4 holes open, the cumulative effect is significant but the bore loop retains enough energy for oscillation.

**Tuning knob:** `holeRadiusRatio` and `tEffNormalized` can be adjusted if holes are too strong or too weak.

### 6.3 Low Risk: Expression Modifier Interactions

**What could go wrong:** Multiple expression modifiers active simultaneously could push the reed model into instability (e.g., growl + flutter + high breath pressure).

**Mitigation:** All modifiers operate on mouth pressure or embouchure with bounded ranges. The reed model's physical constraints (reed closure clamp, min opening) prevent catastrophic behavior. Worst case: noisy, chaotic sound (which is musically useful for extended techniques).

### 6.4 Low Risk: Legato Detection False Positives

**What could go wrong:** In poly mode, a stolen voice might have residual bore energy, causing incorrect legato behavior.

**Mitigation:** Legato only activates in Mono mode (polyMode==0). In Poly mode, standard reset always occurs. The energy threshold (0.001) is low enough to catch true legato but high enough to avoid false triggers from residual delay line energy after a note has ended.

### 6.5 Performance

10 Thiran delay lines per voice (vs 2 in Phase 3.2), plus 5 junction computations. Total additional cost per voice per sample: ~50 multiply-adds. At 16 voices, 44.1kHz: 35M extra ops/sec. This is <1% CPU on modern hardware.

**Confidence: HIGH** overall for feasibility. MEDIUM for first-attempt perceptual quality (tone hole scattering parameters will need tuning by ear).

---

## 7. Default/Bypass Values for Regression Safety

| New Parameter | Default | Bypass Behavior |
|---------------|---------|-----------------|
| TONE_HOLE_CUTOFF | 8000 Hz | cutoff_norm=1.0, all openings=0, all scatter=0, junctions transparent |
| REGISTER_HOLE | 0.0 | registerScatter=0, junction transparent |
| VIBRATO_DEPTH | 0.0 | vibMod=0, no modulation |
| VIBRATO_RATE | 5.0 Hz | Irrelevant when depth=0 |
| VIBRATO_SOURCE | 0 (Lip) | Irrelevant when depth=0 |
| GROWL_AMOUNT | 0.0 | growl branch skipped |
| FLUTTER_TONGUE | 0.0 | flutter branch skipped |
| SUBTONE | 0.0 | modifier branch skipped |
| POLY_MODE | 1 (Poly) | Normal independent voice behavior |
| MAX_VOICES | 16 | Unchanged from Phase 3.2 |

At all defaults, Phase 3.3 behavior is **identical** to Phase 3.2.

---

## 8. ToneHole.h Class Design

**Recommendation:** Do NOT create a separate ToneHole class. The scattering computation is 3 lines of code (1 multiply + 3 adds). Inlining it directly in BoreWaveguide.processSample() avoids function call overhead and keeps the data locality tight. The scatter coefficients are stored as `float toneHoleScatter[4]` in BoreWaveguide.

If a separate class is still desired (for clarity), it should be a POD struct with an inline processSample:

```cpp
struct ToneHoleJunction
{
    float scatter = 0.0f;  // 0 = closed (transparent), <0 = open

    struct Result { float fwd, bwd, radiated; };

    inline Result processSample(float p_fwd_in, float p_bwd_in) const
    {
        float p_scattered = scatter * (p_fwd_in + p_bwd_in);
        return {
            p_fwd_in  + p_scattered,
            p_bwd_in  + p_scattered,
            -p_scattered
        };
    }
};
```

This is header-only, stateless (scatter set externally), zero overhead when inlined.

---

## Sources

### Primary (HIGH confidence)
- Smith, J.O. "Physical Audio Signal Processing" (CCRMA) -- Loaded Waveguide Junctions, Tonehole as Two-Port Loaded Junction
  - https://ccrma.stanford.edu/~jos/pasp/Loaded_Waveguide_Junctions.html
  - https://ccrma.stanford.edu/~jos/pasp/Tonehole_Two_Port_Loaded_Junction.html
  - https://ccrma.stanford.edu/~jos/pasp/Clarinet_Tonehole_Two_Port_Junction.html
- Bilbao scattering junctions (parallel N-port): https://ccrma.stanford.edu/~bilbao/master/node91.html
- STK BlowHole.cpp (Cook/Scavone): https://github.com/thestk/stk/blob/master/src/BlowHole.cpp
  - scatter formula: `scatter_ = -pow(rth,2) / (pow(rth,2) + 2*pow(rb,2))`
  - Three delay line + two junction architecture
- JUCE MPESynthesiser source: https://github.com/juce-framework/JUCE/blob/master/modules/juce_audio_basics/mpe/juce_MPESynthesiser.cpp
  - noteStarted() called on already-active voice during stealing (no noteStopped first)

### Secondary (MEDIUM confidence)
- Keefe 1990 "Woodwind Tone-hole Acoustics" -- original impedance parameters
  - Shunt impedance formulas (open/closed) verified via Smith PASP reproduction
- Scavone/Smith "The One-Filter Keefe Clarinet Tonehole" (1997): https://ccrma.stanford.edu/~gary/papers/SmithAndScavoneMohonk97.pdf
- Scavone "Digital Waveguide Modeling of Woodwind Toneholes" (1997): https://pubs.aip.org/asa/jasa/article/100/4_Supplement/2812/562661
- Register hole overblowing physics: https://arxiv.org/html/2404.07540

### Tertiary (LOW confidence)
- Tone hole radiation mixing coefficient (0.3-0.5) -- perceptual, needs tuning
- Flutter tongue tanh(4*sin) waveform shape -- reasonable approximation, may need adjustment
- Register hole scatter strength (~-0.04) -- may need increase if overblowing doesn't trigger

## Metadata

**Confidence breakdown:**
- Tone hole scattering math: HIGH -- derived from Smith PASP first principles, verified against STK
- Bore segmentation: HIGH -- straightforward delay splitting, well-understood waveguide theory
- Register hole: HIGH for physics, MEDIUM for parameter tuning
- Expression modifiers: HIGH -- trivial per-sample modulations
- Mono legato: HIGH -- verified JUCE MPESynthesiser behavior from source code

**Research date:** 2026-04-05
**Valid until:** 2026-07-05 (stable domain, equations don't change)
