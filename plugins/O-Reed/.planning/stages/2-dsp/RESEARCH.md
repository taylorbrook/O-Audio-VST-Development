# Stage 2: DSP - Research

**Date:** 2026-04-05
**Plugin:** O-Reed
**Stage:** 2-dsp (Phases 3.1-3.5)
**Focus:** Phase 3.1 core engine implementation

## Summary

Four parallel research threads investigated the key implementation areas for O-Reed's Phase 3.1 core DSP engine: JUCE 8 delay line and filter APIs, Strategy C conical bore waveguide, mass-spring-damper reed ODE with Bernoulli junction, and the existing O-Bowed codebase as a reference pattern. All findings are HIGH confidence from direct source analysis.

**Key conclusions:**
- Strategy C (true conical) is *cheaper* than Strategy B (correction filter): 2 multiplications vs 4-5 filter ops per sample
- Round-trip conical scaling is always 1.0 -- bore geometry never affects loop stability
- Symplectic Euler reed ODE is unconditionally stable at our sample rates (omega*dt = 0.53 for clarinet)
- One-sample delay naturally decouples the implicit reed-bore equation -- no Newton-Raphson needed
- O-Bowed's pop-before-push waveguide pattern maps directly to O-Reed's bore

---

## 1. JUCE 8 API Findings

### DelayLine (Thiran Allpass Interpolation)

```cpp
juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Thiran> delay { 40000 };
```

- **Per-sample access:** `popSample(channel)` reads, `pushSample(channel, sample)` writes
- **Critical ordering:** Pop BEFORE push in feedback loops (same as O-Bowed)
- **Thiran behavior:** When `delayFrac < 0.618`, borrows 1 from `delayInt` to keep allpass stable. At exact integer delays, allpass is bypassed entirely. Minimum usable delay: **2.0 samples**
- **Max delay sizing:** 40000 samples covers 20 Hz at 768 kHz (192kHz * 4x OS)
- **prepare():** Pass `ProcessSpec{sampleRate, maxBlockSize, 1}` (mono per-voice). `setMaximumDelayInSamples()` allocates -- call only in prepare(), never on audio thread

### IIR::Filter (Manual Coefficients)

```cpp
// First-order: Coefficients(b0, b1, a0, a1) -- normalizes by a0 internally
*filter.coefficients = juce::dsp::IIR::Coefficients<float>(b0, b1, a0, a1);
float out = filter.processSample(in);  // mono only, Transposed Direct Form II
```

- **One-pole lowpass (viscothermal loss):**
  ```cpp
  float p = std::exp(-2.0f * pi * cutoffHz / sampleRate);
  float g = 0.995f;  // loop gain < 1
  *filter.coefficients = Coefficients<float>(g*(1-p), 0, 1, -p);
  ```
- **First-order allpass (bell reflection):**
  ```cpp
  float t = std::tan(pi * bellCutoffHz / sampleRate);
  float a = (1-t) / (1+t);
  *filter.coefficients = Coefficients<float>(a, 1, 1, a);
  ```
- **Pattern 1** (in-place `*filter.coefficients = ...`) avoids allocation. Use for per-block updates.
- Call `filter.snapToZero()` at end of each block to flush denormals.

### Filter Group Delay Compensation

Subtract loop filter group delays from total bore delay for pitch accuracy:
```cpp
float totalDelay = sampleRate / frequency;
float viscGD = sampleRate / (2.0f * pi * viscCutoffHz);
float allpassGD = /* bell filter group delay at fundamental */;
float compensatedDelay = totalDelay - viscGD - allpassGD;
// Split equally between forward and backward delay lines
```

Conical scaling factors do NOT affect delay length (amplitude only, zero group delay).

---

## 2. Strategy C: Conical Bore Waveguide

### Core Finding

A single conical section is computationally identical to a cylindrical section plus **two scalar multiplications** per sample. Cheaper than Strategy B's correction filter.

### Spherical Wave Scaling

```cpp
// Forward (expanding, reed -> bell): pressure decreases
float p_at_bell = forwardDelay.popSample(0) * scaleForward;   // r_in / r_out

// Backward (converging, bell -> reed): pressure increases
float p_at_reed = backwardDelay.popSample(0) * scaleBackward;  // r_out / r_in
```

- **Formula verified correct** from Smith (CCRMA), Fletcher & Rossing
- **Round-trip product:** `(r_in/r_out) * (r_out/r_in) = 1.0` always -- energy-neutral
- **Cylindrical degeneration:** When bore_character = 0, both scales = 1.0 (multiply by 1)

### Computing Scale Factors from Parameters

```cpp
float half_angle = boreCharacter * 1.6f * (pi / 180.0f);  // max 1.6 deg (alto sax)
if (half_angle < 1e-6f) {
    scaleForward = scaleBackward = 1.0f;  // cylindrical
} else {
    float r_in = throatRadius / std::tan(half_angle);
    float r_out = r_in + boreLength;
    scaleForward = r_in / r_out;
    scaleBackward = r_out / r_in;
}
```

### Reverse Bore (REVERSE_BORE parameter)

Swaps the direction of taper -- bore narrows toward bell (hichiriki). Simply use negative effective half_angle:
```cpp
float effective_half_angle = half_angle * (1.0f - 2.0f * reverseBore);
```

### Stability

- Conical scaling never affects loop stability (product = 1.0)
- Backward scaling can be large for steep cones (~9.4x for alto sax geometry) -- physically correct
- Safety: soft clipping (tanh) on bore output if extreme scaling causes clipping
- Smooth scale factors with ~50ms exponential smoothing during bore morphing to prevent clicks

### Multi-Section (Phase 3.4 only)

Cascading cone segments of different taper requires first-order junction filters that may have unstable poles. Solutions: TIIR filters (truncated IIR) or WDM formulation. Not needed for Phase 3.1 single section.

---

## 3. Reed ODE + Bernoulli Junction

### Symplectic Euler Discretization

Velocity-first update preserves energy in the undamped case:

```cpp
float force = (p_mouth - p_bore) * A_reed - g_eff * x_dot - k_eff * x;
float mu_safe = std::max(mu_r, 1e-6f);
x_dot += (force / mu_safe) * dt;   // velocity first (using current x)
x     += x_dot * dt;                // position second (using NEW x_dot)

// Reed closure limit
float x_min = -(H_eff + 1e-4f);
if (x < x_min) { x = x_min; x_dot = std::min(x_dot, 0.0f); }
```

**Stability:** omega * dt < 2. For clarinet (f_r = 3700 Hz) at 44.1 kHz: omega*dt = 0.53. Safe by 4x margin. At 2x OS (88.2 kHz): 0.26. Even more margin.

**When reed mass -> 0:** Switch to static reed (memoryless spring) when REED_MASS < threshold to avoid stiffness blowup:
```cpp
if (mu_r < 1e-4f) {
    x = (p_mouth - p_bore) * A_reed / k_eff;
    x_dot = 0.0f;
    x = std::max(x, x_min);
}
```

### Embouchure Modifiers

```cpp
float g_eff = g_r + embouchure * 4000.0f;   // additional lip damping
float k_eff = k_r + embouchure * 5e6f;      // additional lip stiffness
float H_eff = H   - embouchure * 0.0003f;   // pre-closure (up to 0.3mm)
```

### Bernoulli Flow (Psi=0 for Phase 3.1)

```cpp
float S_opening = w * std::max(x + H_eff, 0.0f);
float dp = p_mouth - p_bore;
float u_reed = std::copysign(1.0f, dp) * S_opening
             * std::sqrt(2.0f * std::max(std::abs(dp), 1e-10f) / rho_air);
```

Use `std::sqrt()` directly -- hardware-accelerated on modern CPUs, ~15 ops total. Polynomial approximation not justified for Phase 3.1 ("no CPU budget" constraint).

### Flow-to-Wave Conversion

```cpp
float p_bore_plus = Z_c * u_reed + p_bore_minus;
```

Where `Z_c = rho_air * c_sound / A_bore` (characteristic impedance at reed end).

### One-Sample Delay Decoupling

The bore delay lines provide at least 2 samples of delay, which makes the reed-bore coupling **fully explicit** -- no Newton-Raphson iteration needed. The bore returns pressure from the PREVIOUS sample, reed computes CURRENT sample, new pressure enters bore. Same pop-before-push pattern as O-Bowed.

### Physical Parameter Mapping (APVTS -> Physics)

| APVTS | Physical | Range | Unit |
|-------|----------|-------|------|
| REED_MASS 0-1 | mu_r | 1e-4 to 0.06 | kg/m^2 |
| REED_DAMPING 0-1 | g_r | 500 to 6000 | s^-1 |
| REED_HARDNESS 0-1 | k_r | 2e6 to 20e6 | N/m^3 |
| REED_OPENING 0-1 | H | 0.1mm to 1.5mm | m |
| BORE_DIAMETER 0-1 | throat_radius | 2mm to 20mm | m |

---

## 4. O-Bowed Reference Patterns

### Waveguide Loop (WaveguideString)

- Two `DelayLine<float, Thiran>` (bridge + nut rails)
- Per-sample: pop both -> compute junction -> push both -> output from bridge
- One-pole bridge loss filter: `H(z) = g*(1-p) / (1 - p*z^-1)`
- Filter group delay compensated in delay length calculation
- `filterDirty` flag for lazy coefficient updates
- Energy tracking: `energyEstimate = 0.999f * energyEstimate + 0.001f * abs(output)`

### Voice Integration (BowedStringVoice)

- Per-block: read all APVTS params via atomic loads, update DSP component params
- Per-sample: `bowModel.updateEnvelope()` -> `waveguideString.processSample(v_bow, F_bow, friction)` -> gain + safety clip
- Auto-cleanup: `if (!bowModel.isActive() && !waveguide.isActive()) clearCurrentNote()`
- Velocity-dependent attack: 50ms (vel=0) -> 5ms (vel=1)
- Safety clipping: `juce::jlimit(-2.0f, 2.0f, sample)` -- consider `tanh()` for O-Reed

### Patterns to Replicate

1. Pop-before-push waveguide ordering
2. Per-block atomic parameter reads with dirty flags for coefficient updates
3. One-pole envelope smoothing (attack/release coefficients)
4. Energy-based voice cleanup (not timeout)
5. Modular DSP classes in Source/DSP/ folder

### Patterns to Adapt

- O-Bowed uses `SynthesiserVoice` -- O-Reed uses `MPESynthesiserVoice` (6 pure virtuals)
- O-Bowed has memoryless friction -- O-Reed has stateful reed ODE
- O-Bowed has no breath envelope -- O-Reed needs attack chiff + breath pressure ramp

---

## 5. Phase 3.1 Architecture

### Per-Sample Signal Flow

```
1. Pop from forwardDelay = wave arriving at bell
2. Pop from backwardDelay = wave arriving at reed
3. Apply conical scaling (2 multiplications)
4. Bell: reflected = bellReflectionFilter(-p_at_bell), radiated = p_at_bell + reflected
5. Viscothermal loss on reflected wave
6. Reed: updateReedODE(p_mouth, p_at_reed) -> compute Bernoulli flow -> p_bore_plus = Z_c * u + p_at_reed
7. Push p_bore_plus into forwardDelay
8. Push p_backward_lossy into backwardDelay
9. Output: p_radiated * outputGain
```

### Class Structure

```
Source/
  DSP/
    ReedModel.h / .cpp         -- ReedState, ReedParams, ODE update, Bernoulli junction
    BoreWaveguide.h / .cpp     -- 2x DelayLine<Thiran>, bell filter, loss filter, conical scaling
    BreathEnvelope.h / .cpp    -- Attack/sustain/release + chiff overshoot
  ReedWindVoice.h / .cpp       -- Orchestrates DSP, reads APVTS, owns components
  PluginProcessor.h / .cpp     -- Owns MPESynthesiser (existing)
  PluginEditor.h / .cpp        -- WebView (unchanged)
```

### Note Onset

1. Reset reed state (x=0, x_dot=0)
2. Reset both delay lines (zero out)
3. Set delay lengths from frequency (compensated for filter group delay)
4. Trigger breath envelope with velocity-scaled chiff overshoot (5-50ms attack, 0-30% overshoot)

### Note Offset

1. Exponential breath pressure release (~150ms)
2. Let bore energy decay naturally (ring-down)
3. Clean up voice when energy below threshold

---

## 6. Pitfalls and Mitigations

| # | Pitfall | Prevention |
|---|---------|------------|
| 1 | Push before pop in waveguide | ALWAYS pop both delay lines before pushing either |
| 2 | Reed mass near zero blowup | Switch to static reed when mu_r < 1e-4f |
| 3 | Bore loop gain >= 1 | Ensure bell_gain * viscothermal_gain < 1.0 at all frequencies |
| 4 | Denormals in quiet passages | `ScopedNoDenormals` + `snapToZero()` on filters |
| 5 | Filter group delay causes pitch error | Subtract filter GD from total delay (as O-Bowed does) |
| 6 | Reed closure velocity accumulation | Zero velocity when clamped at wall |
| 7 | Division by zero at bore_character=0 | Short-circuit to scale=1.0 when half_angle < epsilon |
| 8 | Abrupt parameter changes cause clicks | Per-block parameter reads + one-pole smoothing |
| 9 | Delay < 2 samples with Thiran | Clamp minimum delay to 2.0f |
| 10 | setMaximumDelayInSamples on audio thread | Call only in prepare(), size for worst case |
| 11 | Z_c mismatch between junction and bore | Compute from actual bore_diameter, update on change |

---

## 7. Open Questions

1. **Z_c normalization:** Raw physical Z_c (~2.3e6 for clarinet) may produce very large wave amplitudes. May need global scaling factor. Start with physical values, add output normalization.

2. **Viscothermal loss exact coefficients:** Start with one-pole lowpass, cutoff = bore_diameter_mm * 150 Hz, loop gain = 0.995. Tune by ear.

3. **Instrument preset parameter values:** Literature ranges documented, but "sounds right" requires empirical tuning. Start with clarinet defaults from research.

---

## 8. Phase 3.1 Parameters (Active)

| Parameter | Default | Purpose in 3.1 |
|-----------|---------|----------------|
| BREATH_PRESSURE | 0.5 | p_mouth (main dynamics) |
| EMBOUCHURE | 0.4 | Lip force -> g_lip, k_lip, x_lip |
| REED_HARDNESS | 0.5 | k_r stiffness |
| REED_OPENING | 0.4 | Rest opening H |
| REED_MASS | 0.3 | mu_r (dynamic reed) |
| REED_DAMPING | 0.5 | g_r intrinsic damping |
| BORE_CHARACTER | 0.0 | Taper (0=cyl, 1=cone) |
| BELL_SIZE | 0.5 | Bell reflection cutoff |
| BORE_DIAMETER | 0.5 | Throat radius -> viscothermal loss |
| BORE_LENGTH | 0.5 | Tube length scaling |
| OUTPUT_GAIN | 0.0 dB | Master output level |

---

## 9. References

### Academic
- Smith (1986/2010) -- Physical Audio Signal Processing, waveguide reed-bore junction
- Guillemain (2004) -- Double-reed synthesis, Psi confinement parameter (EURASIP)
- Guillemain et al. (2005) -- Real-time clarinet synthesis (JASA 118)
- Valimaki & Karjalainen (1994) -- Conical bore waveguide scaling factors (ICMC)
- Scavone (2002) -- Cyclone conical bore model (ICMC)
- van Walstijn & Campbell (2003) -- Wave Digital woodwind modeling (JASA)
- Smith & van Walstijn -- TIIR filters for conical junctions
- Keefe (1981) -- Woodwind tone-hole acoustics
- Morgan & Qiao (2009) -- Symplectic Euler stability for damped oscillators

### Implementation
- STK BlowHole.cpp, Saxofony.cpp -- Reed table + waveguide reference
- Faust pm.lib -- reedTable, clarinetReed, clarinetMouthPiece
- O-Bowed WaveguideString -- Proven Thiran + IIR pattern in this codebase

### JUCE Source (verified against 8.0.4)
- `juce_dsp/processors/juce_DelayLine.h` -- Thiran interpolation inline implementation
- `juce_dsp/processors/juce_IIRFilter.h` -- Coefficients + Filter declarations
- `juce_dsp/processors/juce_IIRFilter_Impl.h` -- processSample TDF-II

### Detailed Research Files
- `02-RESEARCH.md` -- Full JUCE API analysis
- `02-RESEARCH-conical-bore.md` -- Strategy C theory and implementation
- `phase-3.1-RESEARCH.md` -- Reed ODE, junction, coupling, onset/offset
