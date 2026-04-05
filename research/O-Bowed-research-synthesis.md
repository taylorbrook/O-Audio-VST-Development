---
title: "O-Bowed: Research Synthesis"
created: 2026-04-04
juce_version: "8.0.4"
summary: "Synthesized findings from 3 parallel Level 3 research agents covering bow-string friction DSP, commercial landscape, and instrument acoustics. General-purpose bowed string synthesizer with continuous body morphing and instrument presets."
domain: dsp
type: research-synthesis
keywords:
  - physical-modeling
  - bowed-string
  - waveguide
  - friction-model
  - erhu
  - violin
  - cello
  - o-bowed
stages: [0]
agents: [research]
source_documents:
  - research/bow-string-friction-models.md
  - research/O-Bowed-market-research.md
  - research/O-Bowed-acoustic-instrument-research.md
---

# O-Bowed: Research Synthesis

## 1. Strategic Direction: General-Purpose Bowed String Synthesizer

**O-Bowed is a bowed string synthesizer, not a single-instrument emulation.** The engine exposes a continuous parameter space where violin, cello, erhu, and entirely synthetic bowed instruments are presets within a unified model — not separate products or locked modes.

### Core Philosophy

- **Body resonance is a continuous parameter** — morph freely between membrane (erhu-like), wood plate (violin/cello-like), metal, and synthetic body types
- **String count is configurable** (1-4 strings) — not hardcoded per instrument
- **Bow position (beta) is always exposed** — defaults differ per preset but the user has full control
- **Instrument presets are starting points, not boundaries** — "Violin", "Cello", "Erhu" presets configure the full parameter set but everything remains tweakable
- **Sound design territory is built in from day one** — the same engine that produces realistic bowed tones can create impossible physics, reversed friction, sub-harmonic bowing

### Why General-Purpose Over Instrument-Specific

| Factor | General-Purpose | Instrument-Specific |
|--------|----------------|---------------------|
| Market position | Unique — no PM bowed string synth has serious friction + body morphing | Competes directly with SWAM/Soliste |
| Creative range | Realistic instruments + sound design from one engine | Locked to specific instrument character |
| Development | One engine, many presets | Separate tuning per instrument |
| User appeal | Composers AND sound designers | Primarily composers |
| Body morphing | Core feature — morph between any body type | Not possible (fixed per instrument) |

### Competitive Positioning
- Closer to AAS String Studio VS-3's philosophy (bowed string as synthesis) but with **vastly better friction modeling and body resonance** than VS-3
- The friction model quality (tiered: hyperbolic -> elasto-plastic -> thermal) gives it the expressiveness that VS-3 lacks
- Body morphing is something NO competitor offers — not SWAM, not Soliste, not VS-3
- Erhu, nyckelharpa, and other non-Western presets come "for free" as parameter configurations

---

## 2. Architecture: Modular Waveguide Engine

Design for multi-instrument from day one. Each component is pluggable.

```
MIDI/MPE Input
    |
    v
[Bow Model] --> [Nonlinear Friction Junction] <--> [String Waveguide(s)]
                       |                                    |
                  (stick-slip)                         [Bridge Filter]
                                                            |
                                                    [Body Resonator]  <-- pluggable per instrument
                                                            |
                                                    [Sympathetic Coupling] (optional)
                                                            |
                                                    [Bow Noise Generator]
                                                            |
                                                        Output
```

### Component Abstractions

All components are parameterized — instrument identity emerges from parameter configuration, not separate code paths.

| Component | Parameter Space | Preset Examples |
|-----------|----------------|-----------------|
| **Strings** | 1-4 strings, each with tension/mass/damping/stiffness | Erhu: 2 strings (D4, A4). Violin: 4 (G3-E5). Cello: 4 (C2-A3). |
| **Body Resonator** | Parallel biquad bank with morphable coefficients. Material (membrane <-> wood <-> metal), Size (scales frequencies), Damping | Erhu: membrane-cavity (odd harmonics). Violin: plate modes + bridge hill. Cello: plate modes + wolf coupling. Synthetic: user-defined. |
| **Bow Position (beta)** | Continuous 0.02-0.30 | Erhu default ~0.15. Violin ordinario ~0.10. Sul ponticello ~0.03. |
| **Sympathetic Coupling** | 0-12 passive waveguides, coupling coefficient per string | Off (simple). 3 open strings (violin/cello). 12 chromatic (nyckelharpa-style). |
| **Bridge Filter** | 2-4 biquads, parameterized cutoff/Q/gain | Violin: bridge hill at 2.5 kHz. Cello: lower at 1.5 kHz. Erhu: membrane coupling peak. |

---

## 3. DSP Architecture: The Friction Model

**The friction model differentiates products more than the waveguide topology.** Everyone uses the same delay-line structure (Smith 1986). The friction/excitation quality is what separates good from great.

### Tiered Friction Approach

| Tier | Model | CPU | When |
|------|-------|-----|------|
| **Core** | Enhanced hyperbolic bow table | 1x (baseline) | Always active |
| **Enhanced** | + Elasto-plastic bristle state | 3x | Default on, toggleable |
| **Quality** | + Thermal rosin temperature | 5x | Optional "hi-fi" mode |

### Core Friction: Enhanced Hyperbolic Bow Table

```cpp
// Replaces STK's empirical 4th-power with physics-derived hyperbolic curve
float bowFriction(float v_delta, float mu_s, float mu_d, float v_0)
{
    float absV = std::abs(v_delta);
    float mu = mu_d + (mu_s - mu_d) * v_0 / (v_0 + absV);
    return mu;  // convert to reflection coefficient at junction
}
```

Key parameters:
- `mu_s` = 0.8 (static friction, sticking)
- `mu_d` = 0.3 (dynamic friction, sliding)
- `v_0` = 0.01-0.1 (transition sharpness -- maps to "Rosin" character knob)

### Elasto-Plastic Enhancement (Attack Quality)

Adds a bristle displacement state variable `z` that tracks stick-slip transitions. This produces:
- Pre-sliding displacement (realistic micromotion during stick phase)
- Natural hysteresis between stick and slip states
- More realistic attack transients -- the musically critical "bite"
- Must use velocity-dependent damping for passivity: `sigma_1_eff = sigma_1 * (1 + v_rel^2)`

### Thermal Enhancement (Sustained Tone Quality)

Models rosin temperature at contact point. Rosin glass transition at ~49C creates natural hysteresis loops:
- Stick phase: low heat generation -> cooling -> friction rises -> maintains stick
- Slip phase: high heat generation -> heating -> friction drops -> maintains slip
- Adds subtle evolution to sustained tones that memoryless models can't produce

### Solving the Nonlinear Junction

Three options ranked by practicality:

1. **Memoryless table lookup** (STK approach) -- use incoming differential velocity as proxy. O(1), no iteration. Good enough for core tier.
2. **Newton-Raphson** (4 iterations typical) -- needed for elasto-plastic/thermal tiers. O(4) function evaluations per sample.
3. **Precomputed 2D LUT** -- presolve (v_delta, bowForce) grid offline, bilinear interpolate at runtime. O(1) with thermal accuracy.

**Recommendation:** Start with memoryless table for core, add NR solver for enhanced/quality modes.

---

## 4. Waveguide String Model

### Architecture
```
    Nut/Finger                              Bridge
  [Hard reflect] <-- neckDelay --> [BOW] <-- bridgeDelay --> [Loss filter + Body]
                                    |
                              Friction junction
                              (nonlinear 2-port)
```

- **bridgeDelay**: length = sampleRate * beta / f0
- **neckDelay**: length = sampleRate * (1-beta) / f0
- **Bridge termination**: lowpass filter (string losses + body coupling)
- **Nut termination**: near-total reflection with sign inversion

### Multi-String Coupling
- When multiple strings are active, non-bowed strings couple sympathetically through the bridge
- Coupling coefficient: ~0.001-0.01 of bridge amplitude (user-adjustable)
- Erhu preset: additional bow-hair coupling between strings (~0.01-0.05)
- Each sympathetic string is a simple Karplus-Strong waveguide (no bow interaction, low CPU)

### Fractional Delay for Tuning
- Allpass interpolation for fine pitch tuning
- Critical for portamento: must interpolate delay length smoothly for glissando
- Use first-order allpass: `y[n] = a * x[n] + x[n-1] - a * y[n-1]` where `a = (1-d)/(1+d)`, d = fractional part

### 2x Internal Oversampling
- The nonlinear friction junction benefits from oversampling to avoid aliasing
- Process friction/waveguide at 88.2 kHz (when host is 44.1 kHz)
- Downsample output with polyphase halfband filter
- CPU cost: ~2.2x (acceptable for the quality improvement)

---

## 5. Body Resonance: The Key Differentiator

SWAM users consistently complain about "overwhelming mids" and missing wooden body resonance. **Body modeling is where O-Bowed differentiates.**

### Morphable Body Resonator (Core Feature)

The body is a parallel biquad bank (6-10 sections) with continuously morphable coefficients. Two macro parameters control the character:

**Material** (continuous knob):
- Membrane (0.0): odd-harmonic resonances (1:3:5:7), formant-like, voice-like (erhu character)
- Wood (0.33): signature modes (A0 air, B1-, B1+), bridge hill at 2-3 kHz (violin/cello character)
- Metal (0.66): dense inharmonic modes, long sustain, bright (glockenspiel-like)
- Glass/Synthetic (1.0): sparse, tuned resonances, otherworldly

**Size** (continuous knob):
- Small: high resonant frequencies, violin-like range
- Medium: mid-range, viola/erhu character
- Large: low resonant frequencies, cello/bass character
- Scales all body mode frequencies proportionally

Implementation: interpolate biquad coefficients between preset banks:
```
bodyCoeffs = lerp(presetA.coeffs, presetB.coeffs, morphPosition)
```

### Preset Body Configurations

| Preset | Material | Size | Modes | Character |
|--------|----------|------|-------|-----------|
| Violin | Wood (0.33) | Small | A0 ~272Hz, B1- ~462Hz, B1+ ~551Hz, bridge hill ~2.5kHz | Bright, focused |
| Cello | Wood (0.33) | Large | A0 ~95Hz, B1- ~175Hz, B1+ ~570Hz, bridge hill ~1.5kHz | Deep, warm, wolf-prone |
| Erhu | Membrane (0.0) | Medium | Membrane ~2kHz (1:3:5:7 ratio), cavity Helmholtz | Nasal, voice-like |
| Synthetic | Glass (1.0) | Variable | User-defined resonances | Otherworldly |

### Wolf Tone Coupling (Cello Preset and Beyond)

When body MBR frequency aligns with a string fundamental, the coupled string-body system produces beating. This is:
- Essential for cello realism (wolf at ~175 Hz / F3)
- An interesting sound design tool at other settings
- Controlled via a "Body Coupling" parameter that sets string-to-body energy transfer

---

## 6. Bow Noise Generator

Separate from the friction model. Adds the "scratch" and "rosin" texture that pure waveguide models lack.

- Filtered noise modulated by bow pressure and velocity
- Harder bowing = more high-frequency noise content
- Light bowing = subtle airy texture
- Bow change transients (direction reversal) need a noise burst
- Mix level: user-controllable ("Rosin" or "Texture" knob)

---

## 7. Performance Targets

| Metric | Target | Notes |
|--------|--------|-------|
| CPU per string (core) | <2% | At 44.1 kHz, modern i7/M-series |
| CPU per string (quality) | <5% | With thermal + 2x oversampling |
| CPU total (erhu, 2 strings) | <6% | Including body + sympathetic |
| Polyphony | Monophonic per string | Authentic. Enable 2 voices for double-stop if needed |
| Latency | 0 algorithmic | Waveguide is causal. Only buffer latency. |
| Memory | <1 MB per voice | Delay lines + filter states + LUTs |
| Sample rate | 44.1-192 kHz | Internal 2x oversampling at lower rates |

---

## 8. Parameter Design

### User-Facing Controls

| Control | Internal Mapping | Range | MIDI CC |
|---------|-----------------|-------|---------|
| **Bow Speed** | v_b (bow velocity) | Log scale, 0.02-2.0 m/s | CC11 (expression) |
| **Bow Pressure** | F_bow + bow table slope | 0.01-5.0 N | CC2 (breath) or aftertouch |
| **Bow Position** | beta (delay line ratio) | 0.02-0.25 | CC4 or CC74 (MPE Y) |
| **Vibrato Rate** | LFO frequency | 4-8 Hz | CC1 (mod wheel) depth |
| **Vibrato Depth** | LFO amplitude -> delay modulation | 0-50 cents | CC1 amount |
| **Rosin** | mu_s/mu_d ratio, v_0 | Smooth <-> aggressive | Knob |
| **Body Character** | Body biquad coefficients | Material morph | Knob |
| **Brightness** | Bridge filter cutoff | Dark <-> bright | Knob |
| **String Select** | Which string is bowed | D4 / A4 (erhu) | Keyswitch or CC |

### Playability Guard (Schelleng)

```
F_max = (2 * Z_0 * v_b) / (beta * (mu_s - mu_d))
F_min = (Z_0^2 * v_b) / (2 * R * beta^2 * (mu_s - mu_d))
```

Within the Schelleng wedge: stable Helmholtz motion (good tone).
Below F_min: surface sound (airy, double-slip).
Above F_max: raucous (crunchy, irregular).

Expose as "Playability" toggle: ON = soft-clamp force to playable region, OFF = full range including extended techniques.

### MPE Support (Table Stakes for 2026)

| MPE Dimension | Mapping |
|---------------|---------|
| Note velocity | Attack intensity / bow speed |
| Aftertouch (Z) | Bow pressure |
| Slide (Y) | Bow position (sul tasto <-> sul ponticello) |
| Pitch bend (X) | Pitch / vibrato |

---

## 9. Competitive Positioning

### Price Point
- **$49-79 per instrument** (erhu solo: $49-59)
- Undercuts SWAM ($99/inst) and Soliste ($99/inst)
- Above free/educational tier
- Bundle pricing for multi-instrument (when available)

### Key Differentiators vs Competition

| Factor | O-Bowed | SWAM | Soliste | String Studio VS-3 |
|--------|---------|------|---------|---------------------|
| Concept | Bowed string synthesizer | Instrument emulator | Instrument emulator | String oscillator synth |
| Body morphing | **Continuous Material/Size** | Fixed convolution | Fixed | Basic resonator |
| Friction model | Tiered (hyp/elasto/thermal) | Proprietary | Unknown | Basic |
| Instrument range | Any bowed string (presets) | Specific instruments | Specific instruments | Abstract |
| Sound design | Built-in from day one | Limited | Limited | Primary focus |
| Non-Western | Erhu/sarangi/etc. as presets | No | No | No |
| CPU | Target 50% of SWAM | Heavy | Moderate | Low |
| Price | $49-79 | $99/inst | $99/inst | ~$88-199 |

### Market Gaps Addressed
1. No PM bowed string synth combines realistic instruments AND sound design in one engine
2. Body resonance morphing is completely uncontested
3. Non-Western bowed instruments as presets (not separate products)
4. Lower CPU than SWAM with comparable expressiveness
5. Accessible defaults with progressive depth

---

## 10. Implementation Roadmap

### Phase 1: Core Engine
- Single bowed string waveguide (bow model + friction junction + delay lines + bridge filter)
- Enhanced hyperbolic friction (core tier)
- Morphable body resonator (6-10 biquad bank with Material/Size parameters)
- Configurable string count (1-4)
- Bow parameters: velocity, force, position (beta)
- Continuous portamento via delay line interpolation
- Basic vibrato (LFO + body coupling modulation)
- MIDI CC mapping (expression, breath, mod wheel)
- MPE support
- 2x internal oversampling

### Phase 2: Expressiveness + Presets
- Elasto-plastic friction tier (attack transient quality)
- Bow noise generator (rosin texture)
- Sympathetic resonance module (0-4 passive strings)
- Instrument presets: Violin, Cello, Erhu, Viola, Bass
- Sound design presets: Metal Bow, Glass Resonator, Sub Drone, etc.
- Playability guard (Schelleng soft-clamp, toggleable)
- Articulation support: detache, spiccato, tremolo, martele

### Phase 3: Quality + Sound Design
- Thermal friction tier (quality mode for sustained tones)
- Wolf tone coupling model
- Extended sympathetic resonance (up to 12 strings, nyckelharpa-style)
- "Impossible physics" mode: infinite bow, reversed friction, sub-harmonic bowing
- Extended techniques: col legno, harmonics, Bartok pizzicato
- Body morphing automation (morph between body types over time)

### Phase 4: UI + Polish
- WebView UI with bow-string visualization
- Real-time body resonance spectrum display
- Schelleng diagram visualization (show playable region)
- Comprehensive preset library
- User preset morphing

---

## 11. Key References

### Essential Reading (Implementation)
- Smith, J.O. "Physical Audio Signal Processing" -- Bowed Strings chapter (CCRMA online book)
- STK Bowed.cpp source code (github.com/thestk/stk)
- FAUST physmodels.lib -- modular bowed string chain

### Friction Models
- Woodhouse (2003) "Bowed string simulation using a thermal friction model"
- Serafin & Avanzini (2003) "Bowed string simulation using an elasto-plastic friction model"
- Willemsen & Bilbao (2019) "Real-time implementation of an elasto-plastic friction model"
- Frontiers (2025) "Numerical modelling of elasto-plastic friction with guaranteed passivity"

### Instrument Acoustics
- Gough (2016) "Violin Acoustics" (Acoustics Today)
- Euphonics.org -- Woodhouse's comprehensive violin acoustics reference
- AIP (2020) "A simple model of the Erhu soundbox"
- Guettler "Schelleng in Retrospect" (ISMA 2007)

### Competitive Intelligence
- Audio Modeling SWAM Solo Strings v3.8.0 User Manual
- Sound on Sound review: Expressive E Soliste (June 2025)
- VI-Control forum: "Let's discuss physically modelled bowed strings"
