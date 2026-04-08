---
title: "Flute Physical Model: JUCE 8 Implementation Research"
created: 2026-04-04
juce_version: "8.0.4"
summary: "Comprehensive implementation research for a digital waveguide flute synthesizer in JUCE 8, covering DSP class mapping, signal flow architecture, parameter design, and performance analysis with comparison to O-Bowed architecture."
domain: dsp
type: research
keywords:
  - physical-modeling
  - flute
  - waveguide
  - jet-drive
  - tone-holes
  - wind-instrument
  - embouchure
  - overblowing
---

# Flute Physical Model: JUCE 8 Implementation Research

## 1. JUCE DSP Classes for Waveguide Flute

### 1.1 Bore Waveguide -- `juce::dsp::DelayLine`

The flute bore is the primary resonator, analogous to the bowed string waveguide in O-Bowed. Unlike strings (which use two delay lines split at the bow contact point), the flute bore is modeled as a single bidirectional waveguide loop: a forward-traveling wave and a backward-traveling wave, each represented by a delay line.

**Interpolation type: Thiran allpass** is the correct choice for the bore delay, for the same reasons documented in O-Bowed Decision 5. Thiran provides flat amplitude response (critical -- the bore should not color the spectrum through interpolation artifacts) with accurate fractional delay for tuning precision. The JUCE documentation confirms: "This method is very efficient, and features a flat amplitude frequency response in exchange for less accuracy in the phase response. This interpolation method is stateful so is unsuitable for applications requiring fast delay modulation."

The statefulness caveat matters less for flute bore pitch (which changes discretely on fingering change) than for vibrato. However, vibrato in a flute model is typically applied to the jet delay or breath pressure, not the bore length directly, so Thiran remains appropriate for the bore.

**Bore delay length:** `sampleRate / f0` samples for the round-trip, split into two halves (forward and backward). For a concert flute with range C4 (262 Hz) to C7 (2093 Hz), the maximum delay at 44.1 kHz is ~168 samples (C4). At 2x oversampling, this doubles to ~336 samples. Maximum delay buffer should accommodate down to ~C3 (131 Hz) for safety = ~672 samples at 2x.

**JUCE API for per-sample waveguide use:**
- `pushSample(channel, sample)` -- inject into delay line
- `popSample(channel, delayInSamples, updateReadPointer)` -- read from delay line with fractional delay

This push/pop API is explicitly designed for feedback loops, as stated in the JUCE header: "Use this function and popSample instead of process if you need to modulate the delay in real time instead of using a fixed delay value, or if you want to code a delay effect with a feedback loop."

### 1.2 Jet Delay -- `juce::dsp::DelayLine`

The jet delay models the time it takes for the air jet to travel from the player's lips to the labium (edge). This is a shorter delay than the bore -- typically the jet delay length is approximately half the bore delay length (the "jet ratio" or `jetRatio` in STK).

**Interpolation type: Lagrange3rd** is recommended here rather than Thiran. The jet delay is modulated in real time for embouchure control and overblowing effects. JUCE documentation warns that Thiran "is unsuitable for applications requiring fast delay modulation." Lagrange3rd "reduces the low-pass filtering effect whilst remaining amenable to real time delay modulation" -- the right tradeoff for the jet delay which must respond to continuous embouchure changes.

The jet delay length relative to the bore delay controls which harmonic register the flute sounds in. At the standard ratio (~0.4-0.5 of bore delay), the flute plays the fundamental. Shortening the jet delay causes the model to overblow to the second harmonic (octave), and further shortening reaches the third harmonic. This is the primary mechanism for register control.

### 1.3 Jet Nonlinearity -- `juce::dsp::LookupTableTransform`

The jet-labium interaction is the core nonlinear element, analogous to the bow friction junction in O-Bowed. The classical model uses a cubic polynomial:

```
f(x) = x - x^3
```

This creates a soft saturation that prevents unbounded signal growth and introduces the energy-limiting mechanism that sustains oscillation. The STK uses a `JetTable` class for this. Some implementations use `tanh(x)` instead, which has similar saturation behavior but different harmonic content.

**`juce::dsp::LookupTableTransform`** is ideal for this. From the JUCE source:

```
LookupTableTransform<float> jetNonlinearity(
    [] (float x) { return x - x * x * x; },  // cubic polynomial
    -2.0f, 2.0f, 128);  // input range, 128 points
```

The input range of [-2, 2] covers the expected signal range with headroom. 128 points provides sufficient accuracy for a cubic curve with negligible interpolation error. The `operator()` call is then a single table lookup with linear interpolation per sample -- much cheaper than computing the cubic directly, and identical cost regardless of whether we later switch to tanh or a custom jet curve.

**Alternative: direct computation.** The cubic `x - x*x*x` is only 2 multiplies and 1 subtract -- a lookup table may not actually be faster for something this simple. However, `LookupTableTransform` becomes valuable if we want a more complex jet curve (e.g., asymmetric, or with a tunable "sharpness" parameter). Recommend starting with direct computation and switching to lookup if the jet function grows more complex.

### 1.4 Loss and Reflection Filters -- `juce::dsp::IIR::Filter`

Multiple IIR filters are needed throughout the model:

**Bore end reflection filter (one-pole lowpass):** Models frequency-dependent reflection at the open end of the bore. High frequencies radiate more efficiently, so less high-frequency energy reflects back. A first-order lowpass is standard:
- `juce::dsp::IIR::ArrayCoefficients<float>::makeFirstOrderLowPass(sampleRate, cutoffFreq)`
- Cutoff around 2000-5000 Hz depending on bore geometry
- This filter sits at the bore termination in the waveguide loop

**Radiation filter (one-pole highpass):** Models the radiation impedance at the open end. The radiated sound is the output signal. A simple first-order highpass approximates the derivative-like radiation characteristic:
- `juce::dsp::IIR::ArrayCoefficients<float>::makeFirstOrderHighPass(sampleRate, freq)`

**Viscothermal loss filter (one-pole lowpass):** Models wall losses inside the bore due to thermal and viscous boundary layer effects. Losses increase with frequency. Implemented as a gentle lowpass within the waveguide loop:
- Cutoff typically very high (8000-15000 Hz) with subtle attenuation
- Combined with the bore reflection filter for efficiency (cascade or single two-pole design)

**Tone hole filters:** Each tone hole junction requires frequency-dependent scattering. Open and closed states have different filter characteristics. Second-order IIR filters approximate the reflection/transmission transfer functions:
- `juce::dsp::IIR::ArrayCoefficients<float>::makeLowPass(sampleRate, freq, Q)` for closed-hole reflection
- `juce::dsp::IIR::ArrayCoefficients<float>::makeHighPass(sampleRate, freq, Q)` for open-hole transmission
- Coefficients switch (with crossfade) between open and closed states

**DC Blocker (one-pole/one-zero):** Essential after the jet nonlinearity to prevent DC accumulation in the feedback loop. The standard formula is:
```
y[n] = x[n] - x[n-1] + 0.995 * y[n-1]
```
This is a custom two-coefficient filter, not directly available from JUCE's `IIR::ArrayCoefficients` factory methods. Implement as a trivial inline filter (2 state variables: `x_prev`, `y_prev`).

### 1.5 Oversampling -- `juce::dsp::Oversampling`

**2x oversampling is recommended** for the jet nonlinearity, matching the O-Bowed decision. The cubic/tanh saturation expands bandwidth, and without oversampling, aliasing folds back into the audible range. This is especially problematic at high pitches where the fundamental is already above 1 kHz.

The oversampled section should wrap: jet nonlinearity + bore waveguide loop (they are coupled via feedback, so both must run at the same rate). Tone hole filters and the radiation output filter can run at the native rate if desired, though keeping everything at 2x simplifies the architecture.

`juce::dsp::Oversampling<float>` with `2x, polyphase halfband` configuration. Latency is reported via `getLatencyInSamples()` and must be passed to `setLatencySamples()` in `prepareToPlay()`.

### 1.6 What JUCE Does NOT Provide (Custom Implementation Required)

| Component | Why Custom |
|-----------|-----------|
| Jet-labium nonlinear junction | No JUCE class for this physics-specific interaction |
| Three-port tone hole scattering junction | No multi-port junction primitive in JUCE |
| DC blocker | Trivial but not in JUCE's coefficient factory |
| Breath envelope / attack noise model | Physics-specific envelope shaping |
| Overblowing logic | Register selection via jet delay ratio |
| Turbulence noise shaping | Filtered noise injection correlated with breath |

---

## 2. Signal Flow Architecture

### 2.1 Complete Per-Voice Signal Flow

```
Breath Input (envelope * pressure + noise * turbulenceGain)
     |
     v
[Embouchure Summation]  <-------- feedback * jetReflection (from bore output)
     |
     v
[Jet Delay Line] (Lagrange3rd, length ~ 0.4-0.5 * boreDelay)
     |
     v
[Jet Nonlinearity] (cubic: x - x^3, or tanh)
     |
     v
[DC Blocker]
     |
     v
[Bore Input Summation]  <-------- endReflection * filtered bore return
     |
     v
[Bore Waveguide] (Thiran, bidirectional)
     |    |
     |    +---> [Tone Hole Junctions] (3-port scattering, one per hole)
     |              modify traveling waves at each hole position
     |
     v
[Bore End Reflection Filter] (one-pole lowpass, sign inversion)
     |
     +-------> feedback path back to Embouchure Summation
     |
     v
[Radiation Filter] (highpass, models open-end radiation)
     |
     v
Output (mono, per-voice)
```

### 2.2 Feedback Loop and One-Sample Delay

The critical feedback path is: bore output -> reflection filter -> scaled by `jetReflection` -> summed at embouchure input -> jet delay -> nonlinearity -> back into bore. This is a **closed loop** and must be computable sample-by-sample.

The jet delay provides the inherent delay in the feedback loop (minimum 1 sample when jet delay ~ 0). The bore delay provides additional delay. Together, these ensure the loop is always computable without implicit delay -- unlike O-Bowed's friction junction which requires careful treatment of the one-sample delay constraint at the scattering junction.

**Key architectural simplification vs. O-Bowed:** The flute model does NOT have a two-port scattering junction with simultaneous bidirectional coupling. The jet is a one-directional exciter that reads the bore output (from the previous sample) and injects energy. This makes the per-sample computation straightforward:

1. Read bore output from previous cycle
2. Compute breath + feedback at embouchure
3. Process through jet delay
4. Apply jet nonlinearity + DC blocker
5. Inject into bore
6. Advance bore waveguide one sample
7. Apply tone hole scattering
8. Apply bore end reflection filter
9. Tap output via radiation filter

### 2.3 Tone Hole Implementation (Simplified vs. Full)

**Simplified approach (recommended for initial implementation):** Model tone holes as switches that change the effective bore length. When a hole opens, the bore reflection point moves to that hole's position. This means the bore delay length changes discretely on fingering changes. A short crossfade (1-5 ms) prevents clicks. This is computationally cheap and produces correct pitches.

**Full three-port junction approach (for realism):** Each tone hole is a three-port scattering junction where the bore waveguide meets a side branch. The scattering coefficients depend on the hole's acoustic impedance:

- **Open hole:** Mostly transmits energy out (radiation), reflects some back, passes some downstream. Acts approximately as an open termination.
- **Closed hole:** Mostly passes energy through with minor perturbation. Small mass loading effect.
- **Partially open (half-holing):** Intermediate scattering, enables pitch bends and timbral effects.

The three-port junction requires computing reflection (R) and transmission (T) coefficients:
```
R = (Z_hole - Z_bore) / (Z_hole + Z_bore)    (simplified)
T = 1 + R
```

For a full flute with 6-8 tone holes, this means 6-8 scattering junctions inline with the bore waveguide, each requiring filter evaluations. CPU impact is moderate (6-8 filter pairs).

**Recommendation:** Start with the simplified bore-length-switching model. Add three-port junctions as an enhancement tier (similar to O-Bowed's friction tier system).

### 2.4 Polyphony Structure

Each voice is an independent physical model instance containing:
- 1 bore delay line (Thiran)
- 1 jet delay line (Lagrange3rd)
- 1 jet nonlinearity (inline or LUT)
- 1 DC blocker (2 state variables)
- 1 bore reflection filter
- 1 radiation filter
- 1 viscothermal loss filter
- 6-8 tone hole filters (if using full model)
- Breath envelope state
- Noise generator state

Voice allocation via `juce::SynthesiserVoice` subclass, managed by `juce::Synthesiser`.

---

## 3. Parameter Mapping (APVTS)

### 3.1 User-Facing Parameters

| Parameter ID | Type | Range | Default | DSP Target | Notes |
|-------------|------|-------|---------|------------|-------|
| BREATH_PRESSURE | Float | 0.0-1.0 | 0.5 | Excitation amplitude | Nonlinear map: low values = breathy/airy, high = full tone. Maps through `pressure^1.5` curve for natural feel |
| EMBOUCHURE | Float | 0.0-1.0 | 0.5 | Jet delay ratio (0.3-0.6 of bore) | Controls jet angle. Low = focused/bright, high = spread/dark. Primary overblowing control |
| TONE_COLOR | Float | 0.0-1.0 | 0.5 | Bore reflection filter cutoff | Brightness control, 1000-12000 Hz range (log scale) |
| BREATH_NOISE | Float | 0.0-1.0 | 0.15 | Turbulence noise gain | Amount of breathy noise mixed into excitation |
| VIBRATO_RATE | Float | 2.0-8.0 | 5.0 | LFO frequency (Hz) | Standard flute vibrato range |
| VIBRATO_DEPTH | Float | 0.0-1.0 | 0.3 | Modulation depth | Applied to breath pressure, not pitch directly (more natural) |
| JET_REFLECTION | Float | -1.0-1.0 | 0.5 | Feedback coefficient from bore to jet | How much bore energy feeds back to excitation. Negative values invert phase |
| END_REFLECTION | Float | -1.0-1.0 | 0.5 | Bore end reflection coefficient | Controls sustain and resonance character |
| AIR_COLUMN | Float | 0.0-1.0 | 0.5 | Viscothermal loss amount | High = warmer (more HF loss), low = brighter (less loss) |
| OUTPUT_LEVEL | Float | -60-12 | 0.0 | Master gain (dB) | Standard output control |
| WIDTH | Float | 0.0-2.0 | 1.0 | Stereo decorrelation | Same pattern as O-Bowed |

### 3.2 Internal (Non-Exposed) Parameters

- **Jet ratio base:** Fixed at ~0.4, modulated by EMBOUCHURE
- **Bore loss per round trip:** Derived from AIR_COLUMN and TONE_COLOR
- **Noise spectrum shape:** Bandpass around 2-6 kHz, amplitude from BREATH_NOISE * BREATH_PRESSURE
- **Attack breath ramp:** 5-30 ms depending on velocity (faster attack = shorter ramp)
- **Release breath decay:** 20-100 ms depending on release velocity

### 3.3 MIDI/MPE Mapping

| Source | Target | Mapping |
|--------|--------|---------|
| Note number | Bore delay length | `sampleRate / freq` via tuning engine |
| Velocity | Breath attack intensity | 0-127 -> ramp time 30ms-5ms |
| Aftertouch (Z) | Breath pressure | Continuous, replaces BREATH_PRESSURE knob |
| Slide (Y / CC74) | Embouchure | Continuous, replaces EMBOUCHURE knob |
| Pitch bend (X) | Bore delay length modulation | +/- semitones (smooth portamento) |
| CC1 (Mod wheel) | Vibrato depth | Standard mapping |
| CC2 (Breath) | Breath pressure | Primary controller for wind controller users |
| CC11 (Expression) | Output level | Standard dynamics |

### 3.4 Fingering System

For a realistic flute, fingering maps MIDI notes to a set of tone hole open/closed states. This can be implemented as a lookup table:

- **Simple mode:** MIDI note -> effective bore length (single delay value). Tone holes are abstracted away entirely.
- **Advanced mode:** MIDI note -> 6-8 boolean tone hole states. Each state configures the corresponding three-port junction. Enables cross-fingering (alternative fingerings with different timbres for the same pitch) and multiphonics.

Recommend exposing a "FINGERING_MODE" choice parameter: Simple (0) vs. Advanced (1).

---

## 4. Comparison with O-Bowed Architecture

### 4.1 Shared Patterns

| Pattern | O-Bowed | O-Wind Flute |
|---------|---------|--------------|
| Core resonator | `DelayLine<Thiran>` waveguide | `DelayLine<Thiran>` bore waveguide |
| Nonlinear excitation | Bow friction junction (hyperbolic/elasto-plastic) | Jet-labium junction (cubic/tanh) |
| Loss filters | Bridge filter (IIR lowpass) | Bore reflection + viscothermal (IIR lowpass) |
| Oversampling | 2x for friction junction | 2x for jet nonlinearity |
| Fractional delay | Thiran allpass | Thiran allpass (bore), Lagrange3rd (jet) |
| Voice model | SynthesiserVoice subclass | SynthesiserVoice subclass |
| MIDI/MPE | Full MPE support | Full MPE support |
| Tuning | Scala/MTS-ESP/12TET | Scala/MTS-ESP/12TET (shared module) |
| Stereo width | Mid-side decorrelation | Mid-side decorrelation |

### 4.2 Key Differences

**Resonator topology:**
- O-Bowed: String waveguide split at bow contact point into bridgeDelay + neckDelay. Bow position (beta) determines split ratio. Body resonator is SEPARATE from the string (parallel biquad bank post-waveguide).
- O-Wind Flute: Bore IS the resonator AND the body. No separate body resonator needed. The bore's geometry (with tone holes) defines the instrument's formant structure inherently. This is a significant architectural simplification.

**Excitation coupling:**
- O-Bowed: Bow friction is a two-port scattering junction with simultaneous bidirectional coupling. The friction force depends on the string's velocity at the contact point, which depends on the friction force -- a circular dependency resolved by the nonlinear solver (Newton-Raphson for enhanced/quality tiers).
- O-Wind Flute: Jet excitation reads the bore output from the previous sample and injects new energy. No simultaneous coupling -- the jet delay provides inherent temporal separation. No iterative solver needed.

**Pitch control:**
- O-Bowed: Continuous pitch via delay line length. One pitch per string.
- O-Wind Flute: Discrete pitch changes via tone hole configuration (bore length or junction states). Pitch bends possible via half-holing or bore length interpolation.

**No body resonator:**
- O-Bowed uses an 8-biquad parallel bank for body resonance (morphable between wood/metal/membrane/glass).
- O-Wind Flute does not need this. The bore waveguide with tone holes IS the resonant body. The radiation filter shapes the output spectrum. If additional timbral shaping is desired, a small formant filter bank (2-3 biquads) could add "material" character, but this is optional creative territory, not physics modeling.

**Additional excitation components:**
- O-Bowed: Bow noise generator (rosin scratch), direction-change bursts.
- O-Wind Flute: Breath turbulence noise (always present in varying amounts), attack transient noise (tongue articulation), and potentially key click noise.

### 4.3 Module Reuse from O-Bowed

These components can be shared or adapted directly:
- Tuning engine (Scala/MTS-ESP module)
- Stereo width processor
- Oversampling wrapper pattern
- MIDI/MPE routing and voice allocation
- Parameter smoothing patterns (`juce::SmoothedValue`)
- State persistence (APVTS + ValueTree pattern)

---

## 5. Performance Considerations

### 5.1 Per-Voice CPU Estimate

The flute model is **significantly cheaper per-voice than O-Bowed** because:
1. No iterative nonlinear solver (jet uses memoryless function, not Newton-Raphson)
2. No separate body resonator (8 fewer biquad evaluations)
3. Single waveguide loop (not split at contact point)
4. No sympathetic string coupling

**Estimated per-voice cost breakdown (at 2x oversampling, 44.1 kHz base):**

| Component | Operations/sample | Est. CPU % |
|-----------|------------------|-----------|
| Bore waveguide (2 delay read/write) | ~10 ops | 0.3% |
| Jet delay (Lagrange3rd read/write) | ~12 ops | 0.3% |
| Jet nonlinearity (cubic) | ~3 ops | <0.1% |
| DC blocker | ~3 ops | <0.1% |
| Bore reflection filter (1-pole) | ~3 ops | <0.1% |
| Radiation filter (1-pole) | ~3 ops | <0.1% |
| Viscothermal filter (1-pole) | ~3 ops | <0.1% |
| Breath + noise computation | ~8 ops | 0.1% |
| Oversampling overhead (2x) | 2x multiplier | +100% of above |
| **Total per voice (simple model)** | | **~2.0-2.5%** |
| Tone hole junctions (6x, full model) | ~36 ops | +0.5% |
| **Total per voice (full model)** | | **~2.5-3.5%** |

Compare to O-Bowed: ~2-5% per string (core-quality tiers) + 0.5% body resonator + 0.1-0.3% per sympathetic string. The flute is roughly 30-50% cheaper per voice than a single O-Bowed string.

### 5.2 Polyphony Budget

Wind instruments are inherently monophonic in the real world, but a synthesizer can be polyphonic for creative use.

- **4 voices:** ~10-14% CPU total. Comfortable on all modern machines. Recommended default.
- **8 voices:** ~20-28% CPU total. Feasible but aggressive. Good for pad-like sustained textures.
- **12+ voices:** Possible given the low per-voice cost, but likely unnecessary for wind instrument sound design.

Recommend: **8 voice maximum, 4 voice default.** This is more generous than O-Bowed (which targets 1-4 strings per voice due to higher per-voice cost).

### 5.3 SIMD Opportunities

Limited. The waveguide feedback loop is inherently serial -- each sample depends on the previous. SIMD can help with:
- Processing multiple voices in parallel (pack 4 voices into SSE registers)
- Tone hole filter bank (6-8 identical filter structures, amenable to SIMD)
- Oversampling polyphase filters (JUCE's `Oversampling` already uses SIMD internally)

Multi-voice SIMD packing is the biggest win but requires careful data layout (struct-of-arrays for voice state).

### 5.4 ProcessorChain vs. Manual Loop

**Manual sample-by-sample loop is required.** `juce::dsp::ProcessorChain` processes blocks through a linear chain, but the flute model has:
1. A feedback loop (bore output feeds back to jet input)
2. Per-sample state dependencies (each sample depends on the previous)
3. Nonlinear elements that cannot be expressed as block-level operations

This matches the O-Bowed conclusion. The `processBlock()` method will contain an explicit `for (int i = 0; i < numSamples; ++i)` loop with all waveguide computation inline. `ProcessorChain` is only useful for post-processing (e.g., output EQ, limiter) applied after voice summation.

---

## 6. Risk Assessment and Mitigations

### 6.1 Oscillation Startup

**Risk:** The model may fail to self-oscillate or require careful parameter tuning to start.

**Mitigation:** The breath input provides continuous excitation energy. Unlike a bowed string (which can fail to "grab" if pressure/speed is wrong), the jet-driven model is more forgiving -- any nonzero breath pressure combined with bore feedback will produce oscillation. The jet nonlinearity's saturation prevents runaway. Initial noise burst at note-on helps seed oscillation.

### 6.2 Overblowing Instability

**Risk:** Incorrect jet delay ratio can cause the model to jump between registers unpredictably.

**Mitigation:** Constrain EMBOUCHURE parameter range to values that produce stable results. Use hysteresis in register detection -- once the model locks onto a register, resist small perturbations. The jet delay ratio should be smoothed (`juce::SmoothedValue`) to prevent abrupt register jumps.

### 6.3 Tone Hole Click Artifacts

**Risk:** Discrete fingering changes cause discontinuities in bore delay length or junction coefficients.

**Mitigation:** Crossfade bore delay length over 1-5 ms on fingering change. For three-port junctions, smoothly interpolate scattering coefficients between open and closed states over the same window. Test with fast trills (rapid alternation between two notes) as worst case.

### 6.4 DC Accumulation

**Risk:** The cubic nonlinearity and feedback loop can accumulate DC offset, causing speaker excursion and headroom loss.

**Mitigation:** DC blocker is mandatory inside the waveguide loop, placed immediately after the jet nonlinearity. The standard formula `y[n] = x[n] - x[n-1] + 0.995 * y[n-1]` with pole at 0.995 provides ~7 Hz cutoff at 44.1 kHz -- low enough to be inaudible, high enough to prevent accumulation.

---

## 7. Implementation Staging Recommendation

1. **Foundation:** Minimal model with bore delay + jet delay + cubic nonlinearity + DC blocker + bore reflection filter. No tone holes (pitch via bore length only). Monophonic. Validate that self-oscillation works and pitch tracks correctly.

2. **Core DSP:** Add oversampling, viscothermal loss, radiation filter, breath noise, vibrato. Add simplified tone holes (bore length switching with crossfade). Polyphony (4 voices).

3. **Advanced Features:** Full three-port tone hole junctions, half-holing for pitch bends, cross-fingering timbral variations, attack articulation (tonguing), key click noise. Register hole for overblowing assistance.

4. **Polish:** MPE expression mapping, preset library, stereo width, tuning system integration, CPU optimization.

---

## References

- [CCRMA Physical Modelling Synthesis Tutorial](https://ccrma.stanford.edu/software/clm/compmus/clm-tutorials/pm.html) -- Slide flute model with cubic nonlinearity, feedback coefficients, DC blocker
- [Virtual Flute REALSIMPLE Project (Berdahl & Smith)](https://ccrma.stanford.edu/realsimple/vir_flute/vir_flute.pdf) -- Full waveguide flute with tone holes in Pd
- [STK Flute Class](https://ccrma.stanford.edu/software/stk/Flute_8h_source.html) -- Reference C++ implementation with JetTable, bore/jet delays, ADSR
- [Improved Digital Waveguide Model of Flute (Valimaki et al. 1996)](https://quod.lib.umich.edu/i/icmc/bbp2372.1996.001?rgn=main&view=fulltext) -- Fractional delay filters, full fingering system
- [Digital Waveguide Modeling of Woodwind Toneholes](https://www.researchgate.net/publication/2518519_Digital_Waveguide_Modeling_of_Woodwind_Toneholes) -- Three-port junction theory and one-multiply implementation
- [Resonarium (JUCE waveguide synth)](https://github.com/gabrielsoule/resonarium) -- Working JUCE physical modeling reference
- [JUCE Tutorial: String Model with Delay Lines](https://juce.com/tutorials/tutorial_dsp_delay_line/) -- Official JUCE waveguide tutorial
- [Flute-LV2 Waveguide Plugin](https://github.com/timowest/flute-lv2) -- Open source waveguide flute implementation
- [Julius Smith: Digital Waveguide Models](https://ccrma.stanford.edu/~jos/pasp/Digital_Waveguide_Models.html) -- Foundational theory
- [Nord Modular Physical Modeling Chapter](https://cim.mcgill.ca/~clark/nordmodularbook/nm_physical.html) -- Practical slide-flute model with tone hole discussion
