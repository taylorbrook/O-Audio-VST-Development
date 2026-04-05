# Stage 2: DSP - Conical Bore Waveguide Research

**Researched:** 2026-04-05
**Domain:** Spherical wave propagation in conical bore digital waveguides (Strategy C)
**Confidence:** HIGH (theory) / MEDIUM (implementation details)

## Summary

Strategy C -- true conical waveguide sections with spherical wave scaling -- is the physically accurate approach to modeling conical bore instruments (saxophone, oboe, bassoon, zurna, shehnai). Unlike the cylindrical waveguide where plane waves propagate with constant amplitude, conical bores support spherical waves whose amplitude scales with 1/r (distance from the cone apex). This research synthesizes findings from Julius O. Smith (CCRMA), Valimaki & Karjalainen, Scavone, van Walstijn, and the foundational acoustic literature to provide a complete implementation specification.

The core insight: a single conical section is computationally identical to a cylindrical section (bidirectional delay lines) plus two scalar multiplications per sample (the r_in/r_out scaling). The complexity comes from junctions between cone segments of different taper, which require first-order filters (potentially with unstable poles) instead of the simple scalar scattering coefficients used at cylinder junctions. For O-Reed Phase 3.1, we use a **single conical section** with the scaling applied at the endpoints, avoiding junction filters entirely.

**Primary recommendation:** Implement the bore as a single-section conical waveguide with spherical scaling factors `r_in/r_out` and `r_out/r_in` applied to forward and backward traveling waves respectively. When bore_character=0 (cylindrical), both radii are equal and the scale factors become 1.0 -- the cone degenerates to a cylinder with zero cost. Use Scavone's "cyclone" insight: model the mouthpiece as a short cylindrical section whose volume equals the truncated cone tip volume, avoiding the truncated cone singularity at the apex.

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

---

## 1. Spherical Wave Theory for Conical Bores

### 1.1 The Fundamental Equation

In a conical tube, pressure wave propagation follows the Webster horn equation. For a cone with cross-sectional area `A(x) = alpha * x^2` (where x is distance from the apex and alpha is a constant independent of taper angle), the solution decomposes into spherical traveling waves:

```
p(t, x) = f(t - x/c) / x  +  g(t + x/c) / x
```

Where:
- `f(t - x/c)` = outgoing (expanding) spherical wave
- `g(t + x/c)` = incoming (converging) spherical wave
- `x` = radial distance from cone apex
- `c` = speed of sound (~343 m/s)
- The `1/x` factor = spherical spreading/focusing

**Key property:** The amplitude of a pressure wave DECREASES as it travels away from the apex (expanding) and INCREASES as it travels toward the apex (converging). This is the 1/r law of spherical spreading.

**Contrast with cylinder:** In a cylindrical tube, `p(t,x) = f(t - x/c) + g(t + x/c)` -- no amplitude scaling. This is why a cylindrical waveguide uses bare delay lines, while a conical waveguide needs delay lines plus scaling.

**Confidence:** HIGH -- this is textbook acoustics from Smith (CCRMA PASP), Morse & Ingard, Fletcher & Rossing.

### 1.2 Wave Impedance

The wave impedance in a cone is frequency-dependent (unlike a cylinder where it's constant):

```
Z_cone(s) = (rho * c / A(x)) * s / (s +/- c/x)
```

Where the +/- corresponds to expanding/converging waves.

Along the frequency axis (s = j*omega):

```
Z_cone(j*omega) = (rho * c / A(x)) * 1 / (1 +/- j*c/(omega*x))
```

**Important:** The imaginary part of the impedance is significant at low frequencies (when wavelength >> x). Near the narrow end of the cone (small x), the impedance becomes highly reactive. Near the wide end (large x), it approaches the cylindrical value `rho*c/A`.

**Consequence for O-Reed:** The frequency-dependent impedance is what gives conical bore instruments their characteristic all-harmonics spectrum (vs. odd-harmonics-only for cylindrical). Strategy C captures this naturally through the 1/r scaling; Strategy B approximates it with a correction filter.

### 1.3 The Truncated Cone Problem

Real instruments do not extend to the cone apex -- the tube is truncated at the mouthpiece. The "missing" conical tip creates several issues:

1. **Singularity at apex:** As x -> 0, pressure -> infinity (1/x blows up). We never compute at x=0.
2. **Missing volume:** The truncated tip volume must be compensated. In real instruments, the mouthpiece chamber provides this compensation.
3. **Input inertance:** The truncation creates a lumped acoustic inertance at the input: `M_0 = rho * x_0 / A_0`, where x_0 is the distance from apex to the input end, and A_0 is the input cross-section.

**Scavone's solution (the "Cyclone" model):** Replace the truncated cone tip with a short cylindrical section whose volume equals the missing cone volume. Length of cylinder = `x_0 / 3` (for volume equivalence without diameter discontinuity at the junction). This avoids both the singularity and the need for a lumped inertance element, and it physically models the mouthpiece cavity.

**For O-Reed Phase 3.1:** The mouthpiece volume parameter (deferred to Phase 3.2) naturally provides the cylinder-cone compound structure. In Phase 3.1, we treat the input end as if the cone starts at a finite distance from the apex (throat radius > 0), which gives finite scaling factors.

**Confidence:** HIGH -- Scavone 2002 (ICMC), Benade 1976/1988.

---

## 2. Digital Waveguide Implementation of a Single Conical Section

### 2.1 Scaling Factor Derivation

For a single conical section spanning from distance `r_in` (input end, near apex) to `r_out` (output end, far from apex):

**Forward-traveling wave** (expanding, moving away from apex):
```
p_plus_out = (r_in / r_out) * delay_forward.read()
```
The wave amplitude decreases because the wavefront spreads over a larger spherical area.

**Backward-traveling wave** (converging, moving toward apex):
```
p_minus_out = (r_out / r_in) * delay_backward.read()
```
The wave amplitude increases because the wavefront concentrates into a smaller spherical area.

**Verification of the user's formula:** The formula `p_plus_out = (r_in / r_out) * delay.read()` from the CONTEXT description is **CORRECT** for the forward (expanding) wave. The backward wave uses the reciprocal `r_out / r_in`.

### 2.2 Computing r_in and r_out from Bore Geometry

For a truncated cone with:
- `throat_radius` = radius at the narrow (reed) end
- `bell_radius` = radius at the wide (bell) end  
- `bore_length` = physical length of the tube
- `half_angle` = cone half-angle (taper angle)

The distance from the imaginary apex to each end:
```
// From cone geometry: radius = x * tan(half_angle)
// So: x = radius / tan(half_angle)

r_in  = throat_radius / tan(half_angle)      // distance from apex to reed end
r_out = bell_radius / tan(half_angle)         // distance from apex to bell end

// Equivalently, from the bore dimensions:
half_angle = atan((bell_radius - throat_radius) / bore_length)
r_in  = throat_radius / tan(half_angle)
r_out = r_in + bore_length  // (since the bore spans bore_length along the axis)
```

**Important relationship:**
```
r_out = r_in + bore_length
scale_forward = r_in / r_out = r_in / (r_in + bore_length)
scale_backward = r_out / r_in = (r_in + bore_length) / r_in
```

### 2.3 Mapping bore_character Parameter to Cone Geometry

The `bore_character` parameter (0.0 = cylindrical, 1.0 = full cone) controls the taper angle:

```cpp
// bore_character: 0.0 = cylindrical, 1.0 = full conical
// BORE_DIAMETER: controls the throat diameter (reed end)
// These together define the cone geometry.

float bore_character = pBoreCharacter->load();  // 0.0 to 1.0
float bore_diam_norm = pBoreDiameter->load();   // 0.0 to 1.0

// Map bore_diameter to physical throat radius (meters)
// Range: 4mm (narrow oboe) to 40mm (baritone sax)
float throat_radius = juce::jmap(bore_diam_norm, 0.002f, 0.020f);

// Maximum taper: alto sax is about 1.6 degrees half-angle
// bell_radius_at_max_taper = throat_radius + bore_length * tan(max_half_angle)
float max_half_angle_rad = 1.6f * (juce::MathConstants<float>::pi / 180.0f);
float half_angle = bore_character * max_half_angle_rad;

// Compute scaling factors
if (half_angle < 1e-6f)
{
    // Cylindrical: no scaling
    scaleForward = 1.0f;
    scaleBackward = 1.0f;
}
else
{
    float r_in = throat_radius / std::tan(half_angle);
    float r_out = r_in + boreLength;
    scaleForward = r_in / r_out;
    scaleBackward = r_out / r_in;
}
```

### 2.4 Cylindrical Degeneration (bore_character = 0)

When bore_character = 0:
- half_angle = 0
- The cone becomes a cylinder
- r_in -> infinity (apex is infinitely far away)
- r_in / r_out -> 1.0 (since r_out = r_in + bore_length, and bore_length << r_in)
- Both scale factors = 1.0
- The waveguide behaves as a pure cylindrical bore

**Implementation:** Check `bore_character < epsilon` and set both scale factors to 1.0. This avoids the division by zero in `tan(0)`.

**Energy consideration:** When scale_forward * scale_backward = (r_in/r_out) * (r_out/r_in) = 1.0 always, the scaling is energy-neutral for a round trip. The scale factors redistribute amplitude spatially but don't create or destroy energy in the waveguide loop. This is a critical stability property.

**Confidence:** HIGH -- mathematical identity.

### 2.5 Reverse Bore (bore_character < 0 or REVERSE_BORE parameter)

A reverse conical bore (narrows toward the bell end, like hichiriki) simply inverts the relationship:

```cpp
// Reverse bore: the wide end is at the reed, narrow end at the bell
// Effectively: swap r_in and r_out
// Or equivalently: use negative half_angle

float effective_half_angle = half_angle * (1.0f - 2.0f * reverse_bore);
// reverse_bore = 0: normal cone (positive taper)
// reverse_bore = 1: fully reversed cone (negative taper)
```

When the bore narrows toward the bell:
- Forward waves (reed -> bell) are now converging: amplitude increases (scale > 1)
- Backward waves (bell -> reed) are now expanding: amplitude decreases (scale < 1)

This produces the unique timbral character of hichiriki.

**Confidence:** MEDIUM -- acoustically sound but specific implementation of the reverse_bore parameter interaction needs testing.

---

## 3. Per-Sample Processing Algorithm

### 3.1 Complete Single-Section Conical Bore Loop

```cpp
float BoreWaveguide::processSample(float reedJunctionPressure)
{
    // --- Step 1: Read delayed waves from both ends ---
    float p_plus_raw  = forwardDelay.popSample(0);   // arriving at bell end
    float p_minus_raw = backwardDelay.popSample(0);   // arriving at reed end

    // --- Step 2: Apply conical scaling (spherical wave amplitude) ---
    // Forward (expanding): pressure decreases as wave moves toward bell
    float p_plus_at_bell = p_plus_raw * scaleForward;   // r_in / r_out
    // Backward (converging): pressure increases as wave moves toward reed
    float p_minus_at_reed = p_minus_raw * scaleBackward; // r_out / r_in

    // --- Step 3: Bell end processing ---
    // Viscothermal loss (applied in the loop)
    float p_plus_lossy = viscothermalFilter.processSample(p_plus_at_bell);

    // Bell reflection: lowpass reflection (most energy reflected at low freq)
    float p_reflected = bellReflectionFilter.processSample(p_plus_lossy);
    float p_radiated  = p_plus_lossy + p_reflected;  // transmitted = output

    // Write reflected wave into backward delay
    backwardDelay.pushSample(0, p_reflected);

    // --- Step 4: Reed end processing ---
    // The incoming wave at the reed end feeds into the nonlinear junction
    // p_minus_at_reed is returned to the caller for junction computation
    // reedJunctionPressure is the new outgoing wave from the junction

    // Write new forward wave from reed junction
    forwardDelay.pushSample(0, reedJunctionPressure);

    // --- Step 5: Output ---
    return p_radiated;
}
```

### 3.2 Where to Apply Scaling -- Design Choice

There are two valid approaches:

**Approach A: Scale after delay read (shown above)**
- Apply `r_in/r_out` to the forward wave after reading from delay
- Apply `r_out/r_in` to the backward wave after reading from delay
- Most intuitive: the delay line stores "raw" wave values, scaling converts to physical pressure at the endpoints

**Approach B: Scale before delay write (Valimaki commutation)**
- Apply scaling before writing into the delay line
- The scaling factors can be "commuted" and lumped at the output
- More efficient when multiple cone sections are cascaded (you only scale once at the output)

**Recommendation for O-Reed:** Use Approach A (scale after read). It's clearer, debuggable, and for a single-section bore there's no efficiency difference. If multi-segment bore is added in Phase 3.4, reconsider commutation.

### 3.3 Integration with Thiran Fractional Delay

The Thiran allpass interpolation in `juce::dsp::DelayLine` operates on the stored wave samples. The conical scaling happens AFTER the delay/interpolation read, not inside it. This is correct because:

1. The Thiran filter interpolates between stored samples (which are the raw traveling wave values)
2. The scaling converts interpolated values to physical pressure at the boundary
3. Scaling does not need to be interpolated -- it's a geometric property that changes only when bore parameters change (much slower than audio rate)

**No modification to the JUCE DelayLine is needed.** The standard push/pop pattern from O-Bowed works directly; just add the two multiplications after pop.

### 3.4 Filter Group Delay Compensation with Conical Scaling

The total delay in the waveguide loop must account for:
```
total_delay = fs / f0  (samples for one round-trip at frequency f0)

// Subtract filter group delays:
compensated_delay = total_delay
    - bellReflectionFilter_groupDelay
    - viscothermalFilter_groupDelay
    - reedInputAllpass_groupDelay (if using Scavone input inertance filter)

// Split between forward and backward:
forwardDelay.setDelay(compensated_delay / 2.0f);
backwardDelay.setDelay(compensated_delay / 2.0f);
```

The conical scaling factors do NOT affect the delay length or pitch. They only affect amplitude. This is because the `1/r` scaling is a spatial property (amplitude at a given position) not a temporal property (when the wave arrives).

**Confidence:** HIGH -- the scaling is memoryless (no state, no group delay).

---

## 4. Multi-Section Bore (Phase 3.4 -- Throat/Body/Bell)

### 4.1 Cascaded Cone Sections

For a multi-segment bore (e.g., throat + body + bell), each section is a separate conical waveguide with its own:
- Delay length (proportional to physical length)
- Scaling factors (from its own r_in and r_out)
- Taper angle (can differ between sections)

### 4.2 Junction Scattering Between Cone Segments

At the junction between two cone segments of different taper, a first-order reflection filter replaces the simple scalar scattering coefficient used for cylinders.

For cylinders, the scattering coefficient at a diameter discontinuity is:
```
k = (Z2 - Z1) / (Z2 + Z1) = (A1 - A2) / (A1 + A2)
```

For cones, the impedances are frequency-dependent, so the reflection becomes a first-order digital filter:
```
R(z) = (a + z^-1) / (1 + a * z^-1)   [allpass-like structure]
```
where `a` depends on the taper angles and distances from their respective apices.

### 4.3 The Unstable Pole Problem

**Critical issue:** When waves traveling in an expanding cone reflect from a section with a smaller (or negative) taper angle, the junction reflection filter has a pole OUTSIDE the unit circle. The impulse response contains growing exponentials.

**Why it's stable anyway:** In a finite-length physical bore, these growing exponentials are always canceled by reflections from the terminations (bell end, reed end). The overall system is passive and stable, but individual filters within it are unstable.

**Solutions (in order of preference for O-Reed):**

1. **Avoid the problem in Phase 3.1:** Use a single conical section. No junctions = no junction filters = no unstable poles.

2. **TIIR filters (Smith):** For Phase 3.4 multi-segment bore, use Truncated Infinite Impulse Response filters. The TIIR approach runs an unstable IIR filter but periodically subtracts a delayed copy of its own tail to create a finite-length response:
   ```
   y[n] = p * y[n-1] + h0 * (x[n] - p^(N+1) * x[n-(N+1)])
   ```
   Requires double-buffering (two TIIR instances, alternated and reset) to prevent roundoff noise accumulation. Reset interval >= t60 of the exponential decay.

3. **FIR truncation:** Replace the unstable IIR junction filter with a truncated FIR that explicitly implements the growing-then-canceled exponential. Expensive but simple.

4. **Wave Digital Filters (van Walstijn):** Use WDM formulation instead of DWM. The WDM approach defines wave variables differently, avoiding the unstable pole at the cost of slightly different (but stable) scattering equations. More complex to implement but inherently stable.

**Recommendation:** Phase 3.1 = single section (no issue). Phase 3.4 = start with TIIR approach, fall back to FIR truncation if TIIR proves fragile in real-time parameter modulation.

**Confidence:** HIGH (theory), MEDIUM (practical TIIR implementation in real-time context).

---

## 5. Stability and Energy Analysis

### 5.1 Round-Trip Energy Conservation

For a single conical section, the product of forward and backward scaling:
```
scaleForward * scaleBackward = (r_in / r_out) * (r_out / r_in) = 1.0
```

This means a round-trip through the bore preserves wave energy from the scaling alone. All energy loss comes from the filters in the loop (viscothermal, bell reflection) -- the conical geometry doesn't add or remove energy. This is the correct physical behavior: a lossless cone is lossless.

### 5.2 Stability During Parameter Morphing

When bore_character changes in real-time:
- scaleForward and scaleBackward change
- The delay lines contain samples computed with the old scaling
- Abrupt changes could cause energy discontinuities

**Smoothing strategy:**
```cpp
// In voice per-block parameter update:
float targetScaleForward = computeScaleForward(boreCharacter);
float targetScaleBackward = computeScaleBackward(boreCharacter);

// Smooth over ~50ms (e.g., 0.999 coefficient at 44100 Hz)
float smoothCoeff = 0.999f;  // tune to ~50ms convergence
currentScaleForward  = currentScaleForward  * smoothCoeff + targetScaleForward  * (1.0f - smoothCoeff);
currentScaleBackward = currentScaleBackward * smoothCoeff + targetScaleBackward * (1.0f - smoothCoeff);
```

**Alternative: per-block interpolation.** Compute target scale at block start, interpolate per-sample across the block. This is simpler and avoids the exponential smoothing's infinite tail.

### 5.3 Preventing Blowup at Extreme Settings

The backward scaling `r_out / r_in` can be large when the cone is very steep (small r_in, large r_out):
```
Example: throat_radius = 2mm, bore_length = 600mm, half_angle = 1.6 deg
r_in = 0.002 / tan(0.028) = 0.0716 m
r_out = 0.0716 + 0.6 = 0.6716 m
scaleBackward = 0.6716 / 0.0716 = 9.38
```

A backward scaling of ~9.4x means the backward wave is amplified significantly. This is physically correct (sound gets louder as it converges toward the apex), but numerically it means the feedback path has higher gain.

**Safeguards:**
1. The bell reflection filter must attenuate enough to keep the total loop gain < 1.0
2. Viscothermal loss helps dampen the loop
3. Clamp extreme scale ratios if needed: `scaleBackward = std::min(scaleBackward, 20.0f)`
4. The reed junction is nonlinear and naturally limits energy injection

**Critical insight:** The overall loop gain is:
```
loop_gain = scaleForward * scaleBackward * bellReflection_gain * viscothermal_gain * reedReflection_gain
          = 1.0 * bellReflection_gain * viscothermal_gain * reedReflection_gain
```

Since scaleForward * scaleBackward = 1.0, the conical scaling does not affect loop gain. Stability depends entirely on the filter gains, just like a cylindrical bore.

### 5.4 When bore_character = 0 (Cylindrical)

Both scale factors = 1.0. The two multiplications become multiply-by-one (which the compiler may optimize away if the values are known at compile time, but at runtime with smooth interpolation they're just very cheap multiplies). There is effectively zero overhead for cylindrical operation.

**Confidence:** HIGH -- mathematical proof.

---

## 6. Bore Profile Parameters Interaction

### 6.1 BORE_DIAMETER Effect

BORE_DIAMETER controls the throat radius, which affects:
1. **Viscothermal losses:** Narrower bore = more wall losses = darker, more intimate sound
2. **Wave impedance:** Affects reed-bore coupling strength
3. **Cone geometry:** With a fixed taper angle, a larger throat_radius means the apex is farther away (larger r_in), which reduces the scaling ratio

```cpp
// Larger bore diameter -> larger r_in -> smaller scaling ratio -> less conical effect
// This is physically correct: a wide conical bore acts more like a cylinder
float throat_radius = juce::jmap(bore_diam_norm, 0.002f, 0.020f);
```

### 6.2 BORE_LENGTH Effect

BORE_LENGTH affects the effective tube length (and thus pitch register density). For the conical geometry:
```
r_out = r_in + bore_length * bore_length_scale
```
Longer bore = larger r_out = larger scaling ratio for the same taper angle.

### 6.3 BELL_SIZE Effect

BELL_SIZE controls the bell flare, which in Strategy C manifests as:
1. The bell reflection filter cutoff (larger bell = more high-frequency radiation)
2. In multi-segment mode (Phase 3.4): the bell section's taper angle increases

### 6.4 Instrument Preset Bore Profiles

| Instrument | Taper (deg) | Throat (mm) | bore_character | BORE_DIAMETER |
|------------|-------------|-------------|----------------|---------------|
| Bb Clarinet | ~0 | 14.5 | 0.00 | 0.70 |
| Alto Sax | ~1.6 | 12 | 0.80 | 0.55 |
| Soprano Sax | ~1.0 | 8 | 0.55 | 0.30 |
| Tenor Sax | ~1.8 | 14 | 0.90 | 0.60 |
| Oboe | ~0.7 | 4 | 0.40 | 0.10 |
| Bassoon | ~0.4 | 4 | 0.25 | 0.10 |
| Duduk | ~0 | 10 | 0.00 | 0.40 |
| Shehnai | ~2.5 | 6 | 1.00 | 0.20 |
| Zurna | ~2.0 | 8 | 0.85 | 0.30 |
| Hichiriki | ~-0.3 | 9 | 0.00 + reverse | 0.35 |

**Confidence:** MEDIUM -- taper angles from research/O-Reed-acoustic-properties-reed-instruments.md, bore_character mapping is custom design.

---

## 7. Scavone's Cyclone Model (Reference Architecture)

### 7.1 Architecture

The Cyclone model uses a compound cylinder-cone structure:

```
[Reed/Mouthpiece] -> [Cylindrical Section] -> [Junction Filter] -> [Conical Section] -> [Bell]
```

- **Cylindrical section:** Models the mouthpiece cavity. Length = `x_0 / 3` (for volume equivalence with the missing cone tip). Uses standard cylindrical waveguide (no scaling).
- **Junction filter:** First-order digital filter (high-pass character) derived from the cone's input inertance. Accounts for the impedance mismatch between cylinder and cone.
- **Conical section:** Standard waveguide with 1/r scaling.
- **Bell:** Reflection filter as usual.

### 7.2 Junction Filter (Cylinder-to-Cone)

The input inertance `M_0 = rho * x_0 / A_0` creates a reflectance at the cylinder-cone junction:

Continuous-time reflectance:
```
R_0(s) = (x_0 * s - c) / (x_0 * s + c)
```

Discretized via bilinear transform:
```
R_0(z) = (-a1 - z^-1) / (1 + a1 * z^-1)

where: a1 = (c - alpha * x_0) / (c + alpha * x_0)
       alpha = 2 * fs  (bilinear transform constant)
```

This is a first-order allpass filter with a single coefficient. It accounts for the phase shift that spherical waves experience at the cone entrance.

### 7.3 Why Cyclone Matters for Phase 3.2+

The Cyclone architecture naturally maps to O-Reed's parameter structure:
- The mouthpiece volume parameter (Phase 3.2) = the cylindrical section length
- The junction filter = the input inertance that shapes low-frequency response
- This structure produces near-harmonic mode ratios (critical for stable oscillation), whereas a plain truncated cone has inharmonic modes

**For Phase 3.1:** We can use a simpler approach -- a single conical section with the input end at finite r_in (no apex singularity). The mouthpiece volume / Cyclone structure is deferred to Phase 3.2.

**Confidence:** HIGH -- Scavone 2002, Benade 1988.

---

## 8. The "Blowed String" Alternative (Scavone)

Scavone also describes a "virtual" approach: a cylindrical waveguide with the excitation point positioned at a variable location along the bore (not at one end). This is Strategy A from our BRIEF.md.

- Excitation at 1/2 length: odd harmonics only (clarinet-like)
- Excitation off-center: all harmonics (saxophone-like)
- Computationally trivial (two delay lines, one filter, one nonlinearity)

**Scavone's assessment:** "An accurate physical model does not always make the best musical instrument." The blowed-string model is more flexible for synthesis but less physically accurate.

**Relevance to O-Reed:** We explicitly chose Strategy C for accuracy. The blowed-string model is Strategy A (rejected in BRIEF.md). However, Scavone's insight is useful: if Strategy C proves problematic during implementation, we have a known fallback that still produces good sound.

---

## 9. Comparison: Strategy B vs Strategy C

| Aspect | Strategy B (Correction Filter) | Strategy C (True Conical Sections) |
|--------|-------------------------------|-----------------------------------|
| Implementation | Cylindrical waveguide + one filter | Conical waveguide + two multiplications |
| Per-sample cost | 1 filter evaluation (~4-5 ops) | 2 multiplications (2 ops) |
| Physical accuracy | Approximation (static spectral shape) | Exact spherical wave physics |
| Parameter modulation | Filter coefficients change with bore_character | Scale factors change with bore_character |
| Morphing quality | Filter interpolation artifacts possible | Scale factor interpolation is smooth |
| Multi-segment | Would need multiple correction filters | Natural extension (cascade sections) |
| Stability | Straightforward (stable filter) | Scale factors always stable; junctions may need care |
| bore_character=0 | Bypass filter (unity) | Scale = 1.0 (multiply by 1) |

**Verdict:** Strategy C is both cheaper and more accurate for a single section. The complexity only increases for multi-section bores (Phase 3.4), and even then it's manageable with TIIR or WDM approaches.

**Confidence:** HIGH.

---

## 10. Common Pitfalls

### Pitfall 1: Applying Scaling Inside the Delay Line
**What goes wrong:** Trying to modify the delay line's internal state with the scaling factor. The delay stores raw wave samples; scaling is a geometric property of the observation point.
**How to avoid:** Apply scaling AFTER popSample(), not inside the delay.

### Pitfall 2: Forgetting the Backward Scaling
**What goes wrong:** Only applying the forward scaling (r_in/r_out) but using raw values for the backward wave. The backward wave converging toward the reed end must be amplified by r_out/r_in.
**How to avoid:** Always apply both scale factors. The round-trip product = 1.0 is a good sanity check.

### Pitfall 3: Division by Zero at bore_character = 0
**What goes wrong:** Computing `throat_radius / tan(0)` = infinity when bore is cylindrical.
**How to avoid:** Short-circuit to scale = 1.0 when bore_character < epsilon (e.g., 1e-6f).

### Pitfall 4: Abrupt Scale Factor Changes During Bore Morphing
**What goes wrong:** Changing bore_character abruptly creates energy discontinuities in the delay lines, producing clicks.
**How to avoid:** Smooth scale factors with exponential smoothing or per-block linear interpolation (~50ms time constant).

### Pitfall 5: Assuming Conical Scaling Affects Pitch
**What goes wrong:** Trying to compensate delay length for the conical scaling. The 1/r scaling affects amplitude only, not delay (group delay is zero for a memoryless multiplier).
**How to avoid:** Keep delay length calculation identical to cylindrical case. Only the loop filters affect pitch via their group delay.

### Pitfall 6: Extreme Backward Scaling with Steep Cones
**What goes wrong:** Very steep cones (high bore_character + narrow throat) can produce backward scaling factors of 10-20x. Combined with a less-than-perfect reed junction, this amplified signal can cause clipping or instability.
**How to avoid:** The round-trip gain is always 1.0, so this is safe in theory. In practice, add soft clipping (tanh) as a safety net on the bore output. Monitor energy levels.

### Pitfall 7: DC Paradox in Piecewise Conical Models
**What goes wrong:** At DC (zero frequency), the cone junction reflectance has coincident poles and zeros that cancel, but naive evaluation gives R(1) = -1 (total reflection with inversion). This is mathematically correct but requires careful handling.
**How to avoid:** For Phase 3.1 (single section), this doesn't apply. For Phase 3.4 (multi-section), use the L'Hopital limiting approach described by Smith: the DC reflectance of the complete cone assembly is +1 (rigid termination behavior), achieved through pole-zero cancellation.

---

## 11. Code Sketch: BoreWaveguide Class

```cpp
class BoreWaveguide
{
public:
    void prepare(double sampleRate, int maxBlockSize);
    void setFrequency(float hz);
    void reset();

    // Per-block parameter update (from voice)
    void updateBoreGeometry(float boreCharacter, float boreDiameter,
                            float bellSize, float boreLength);

    // Returns: p_minus_at_reed (incoming wave for reed junction)
    // Takes: p_plus_from_reed (outgoing wave from reed junction)
    // Returns via parameter: radiated output
    float processSample(float p_plus_from_reed, float& radiatedOutput);

private:
    void updateScaleFactors();
    void updateDelayLengths();
    void updateFilterCoeffs();

    double sampleRate = 44100.0;
    float currentFreq = 440.0f;

    // Delay lines (Thiran interpolation for fractional delay)
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Thiran>
        forwardDelay { 40000 };
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Thiran>
        backwardDelay { 40000 };

    // Loop filters
    juce::dsp::IIR::Filter<float> viscothermalFilter;
    juce::dsp::IIR::Filter<float> bellReflectionFilter;

    // Conical scaling (Strategy C)
    float targetScaleForward  = 1.0f;
    float targetScaleBackward = 1.0f;
    float currentScaleForward  = 1.0f;  // smoothed
    float currentScaleBackward = 1.0f;  // smoothed
    static constexpr float scaleSmooth = 0.999f;

    // Bore geometry cache
    float boreCharacter = 0.0f;
    float boreDiameter  = 0.5f;
    float bellSize      = 0.5f;
    float boreLength    = 0.5f;

    bool geometryDirty = true;
};
```

### processSample implementation:

```cpp
float BoreWaveguide::processSample(float p_plus_from_reed, float& radiatedOutput)
{
    // Smooth scale factors toward target
    currentScaleForward  += (targetScaleForward  - currentScaleForward)  * (1.0f - scaleSmooth);
    currentScaleBackward += (targetScaleBackward - currentScaleBackward) * (1.0f - scaleSmooth);

    // 1. Read delayed waves
    float p_plus_raw  = forwardDelay.popSample(0);
    float p_minus_raw = backwardDelay.popSample(0);

    // 2. Apply conical scaling
    float p_plus_at_bell  = p_plus_raw  * currentScaleForward;
    float p_minus_at_reed = p_minus_raw * currentScaleBackward;

    // 3. Viscothermal loss (in the forward path before bell)
    float p_plus_lossy = viscothermalFilter.processSample(p_plus_at_bell);

    // 4. Bell reflection
    float p_reflected = bellReflectionFilter.processSample(p_plus_lossy);
    radiatedOutput = p_plus_lossy + p_reflected;

    // 5. Write waves into delay lines
    backwardDelay.pushSample(0, p_reflected);
    forwardDelay.pushSample(0, p_plus_from_reed);

    // 6. Return incoming wave at reed end for junction computation
    return p_minus_at_reed;
}
```

---

## 12. Academic References

### Primary Sources
- **Smith, J.O.** (2010). Physical Audio Signal Processing. CCRMA Stanford. -- Chapters on horns, conical bores, wave impedance in cones, TIIR filters. [https://ccrma.stanford.edu/~jos/pasp/](https://ccrma.stanford.edu/~jos/pasp/)
- **Scavone, G.P.** (2002). "Time-Domain Synthesis of Conical Bore Instrument Sounds." Proc. ICMC. -- The Cyclone model, blowed-string comparison, practical synthesis approaches.
- **Valimaki, V. & Karjalainen, M.** (1994). "Digital Waveguide Modeling of Wind Instrument Bores Constructed of Truncated Cones." Proc. ICMC, 423-430. -- Foundational paper on cone scaling factors and junction filters.
- **van Walstijn, M. & Campbell, D.M.** (2003). "Discrete-time modeling of woodwind instrument bores using wave variables." JASA 113(1), 575-585. -- Wave Digital Modeling approach, stable junction formulation.
- **Smith, J.O. & van Walstijn, M.** -- TIIR filter paper. Use of truncated infinite impulse response filters for conical bore modeling. [https://ccrma.stanford.edu/~jos/tiirts/](https://ccrma.stanford.edu/~jos/tiirts/)

### Secondary Sources
- **Benade, A.H.** (1988). "Equivalent circuits for conical waveguides." JASA 83(5), 1764-1769. -- The cylinder-cone compound horn (basis of Cyclone model).
- **Martinez, J. & Agullo, J.** (1988). "Conical bores. Part I: Reflection functions associated with discontinuities." JASA 84(5), 1613-1619. -- Junction reflection theory.
- **Gilbert, J., Kergomard, J., & Polack, J.D.** (1990). "On the reflection functions associated with discontinuities in conical bores." JASA 87(4), 1773-1780.
- **Guillemain, P., Kergomard, J., & Voinier, T.** (2005). "Real-time synthesis of clarinet-like instruments using digital impedance models." JASA 118. -- Reed-bore coupling signal flow.
- **Guillemain, P.** (2004). "A digital synthesis model of double-reed wind instruments." EURASIP JASP. -- The Psi confinement parameter, conical bore impedance approach.

### Tertiary (Background)
- **Morse, P.M.** (1948). Vibration and Sound. -- Classical derivation of spherical waves in cones.
- **Fletcher, N.H. & Rossing, T.D.** (1998). The Physics of Musical Instruments. Springer. -- Comprehensive reference on bore acoustics.
- **Nederveen, C.J.** (1969). Acoustical Aspects of Woodwind Instruments. -- Bore profile data for real instruments.

---

## Sources

### Primary (HIGH confidence)
- [CCRMA PASP - Wave Impedance in Cone](https://ccrma.stanford.edu/~jos/pasp/Wave_Impedance_Cone.html) -- spherical wave impedance formula
- [CCRMA PASP - Conical Bores (cmj96)](https://ccrma.stanford.edu/~jos/cmj96/Conical_Bores.html) -- junction instability, TIIR reference
- [dsprelated.com PASP mirror - Non-Cylindrical Acoustic Tubes](https://www.dsprelated.com/freebooks/pasp/Non_Cylindrical_Acoustic_Tubes.html) -- complete derivation of traveling wave solution, Webster equation, impedance, scattering
- [CCRMA Horn Modeling lectures](https://ccrma.stanford.edu/~jos/HornModeling/) -- TIIR filters, piecewise conical modeling, DC paradox
- [McGill MUMT618 - Scavone conical synthesis notes](https://caml.music.mcgill.ca/~gary/618/week10/conesynthesis.html) -- Cyclone model, input inertance, blowed-string alternative
- Existing O-Reed research: `research/reed-physical-modeling-dsp.md` -- Strategy A/B/C definitions, bore geometry data

### Secondary (MEDIUM confidence)  
- WebSearch findings on Valimaki & Karjalainen 1994, van Walstijn & Campbell 2003 -- confirmed paper existence and core claims, could not extract full equations from PDFs
- WebSearch findings on TIIR filters -- confirmed algorithm structure, implementation requires referencing original paper for numerical details

### Tertiary (LOW confidence)
- Instrument bore taper angles and dimensions -- taken from `research/O-Reed-acoustic-properties-reed-instruments.md` (itself based on secondary sources)
- bore_character parameter mapping to taper angle -- custom design choice, no external validation

## Metadata

**Confidence breakdown:**
- Spherical wave theory: HIGH -- textbook acoustics, multiple authoritative sources
- Single-section implementation: HIGH -- straightforward application of theory
- Scale factor derivation: HIGH -- mathematical identity, verified formula
- Stability analysis: HIGH -- mathematical proof that round-trip gain = 1.0
- Multi-section junctions: MEDIUM -- theory well understood, practical TIIR implementation needs validation
- Bore parameter mapping: MEDIUM -- acoustically reasonable but custom design
- Instrument preset values: MEDIUM -- based on research documents, not measured

**Research date:** 2026-04-05
**Valid until:** Indefinite (acoustic theory is stable; implementation patterns are well-established)
