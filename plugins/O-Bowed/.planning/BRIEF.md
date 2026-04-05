# O-Bowed - Creative Brief

## Overview

**Type:** Synth (Physical Modeling Bowed String)
**Core Concept:** General-purpose bowed string physical modeling synthesizer using digital waveguide strings and nonlinear bow-string friction modeling. NOT an instrument emulator — a bowed string SYNTHESIZER where violin, cello, erhu, and synthetic/impossible instruments are presets within a continuous parameter space.
**Status:** Ideated
**Created:** 2026-04-04

## Vision

The bow-string friction interaction (stick-slip behavior) is the synthesis engine. A morphable body resonator and configurable string parameters give each instrument its identity. The same engine produces realistic bowed tones AND sound design territory — sustained, evolving, organic tones that nothing else in the Ouaricon catalog touches.

This is philosophically closest to AAS String Studio VS-3 (bowed string as synthesis) but with vastly superior friction modeling, a morphable body resonator that no competitor offers, and full microtonal support consistent with the Ouaricon brand.

**Key differentiators:**
- Tiered friction model quality (hyperbolic -> elasto-plastic -> thermal rosin)
- Morphable body resonator with Material and Size macro parameters (uncontested in market)
- Non-Western instruments (erhu, sarangi) come "for free" as presets
- Continuous "impossible physics" knobs blending physical to synthetic
- Full microtonal tuning (Scala/TUN, MTS-ESP, per-string user-configurable)

## Signal Flow

```
MIDI/MPE -> [Bow Model] -> [Nonlinear Friction Junction] <-> [String Waveguide(s)]
           -> [Bridge Filter] -> [Body Resonator] -> [Sympathetic Coupling]
           -> [Bow Noise Generator] -> [Stereo Width] -> Output
```

## Friction Model (Tiered Architecture)

The friction model quality is THE differentiator. Everyone uses the same delay-line structure (Smith 1986). The excitation quality separates good from great.

| Tier | Model | Feature | CPU Impact |
|------|-------|---------|------------|
| Core | Enhanced hyperbolic bow table | Always active, smooth stick-slip | Low |
| Enhanced | + Elasto-plastic bristle state | Attack "bite", hysteresis | Moderate |
| Quality | + Thermal rosin temperature | Sustained tone evolution | Higher |

## Morphable Body Resonator

Parallel biquad bank (6-10 sections) with two macro parameters:

- **Material:** membrane (erhu, voice-like) <-> wood (violin/cello) <-> metal <-> glass/synthetic
- **Size:** small (violin range) <-> large (cello/bass range)

Body resonance is the weak link in ALL existing PM bowed string plugins (SWAM users complain about "overwhelming mids"). This is where O-Bowed differentiates.

## Parameters

### Bow Controls

| Parameter | Range | Default | Description | MIDI Map |
|-----------|-------|---------|-------------|----------|
| Bow Speed | 0.02-2.0 m/s | 0.2 | Velocity of bow across string | CC11/Expression |
| Bow Pressure | 0.01-5.0 N | 0.5 | Normal force of bow on string | CC2/Breath/Aftertouch |
| Bow Position (beta) | 0.02-0.30 | 0.12 | Contact point — sul ponticello to sul tasto | CC74/MPE Y |
| Rosin | 0-100% | 50% | Friction curve shape — smooth to aggressive | — |

### Body Controls

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Material | 0-100% | 40% (wood) | Body resonator morph: membrane <-> wood <-> metal <-> glass |
| Size | 0-100% | 50% | Body resonant frequency scaling: violin <-> cello/bass |
| Brightness | 20-20000 Hz | 8000 Hz | Bridge filter cutoff |

### String Configuration

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| String Count | 1-4 | 1 | Number of active bowed strings |
| Per-String Tuning | +/- 2400 cents | 0 | Independent pitch offset per string (cent resolution) |
| Sympathetic Amount | 0-100% | 0% | Coupling to passive sympathetic strings |
| Sympathetic Strings | 0-12 | 0 | Number of passive waveguide strings |

### Output

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Width | 0-200% | 100% | Stereo spread of multi-string output |
| Output Level | -inf to +12 dB | 0 dB | Master output gain |

### Impossible Physics (Continuous Blendable)

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Infinite Sustain | 0-100% | 0% | Reduces damping toward zero — endless resonance |
| Reversed Friction | 0-100% | 0% | Inverts friction curve — synthetic excitation |
| Sub-Harmonics | 0-100% | 0% | Introduces sub-octave content via nonlinear feedback |

### Microtonal Tuning

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Reference Pitch | 220-880 Hz | 440 Hz | A4 reference frequency |
| Tuning System | File/MTS-ESP/12TET | 12TET | Scala/TUN import, MTS-ESP real-time retuning, or standard |

## Bow Behavior

**Hybrid model:** Bow is sustained while MIDI note is held, with release tail on note-off. Velocity and CC control stroke articulation style:
- Legato/sustained bowing (default)
- Detache (re-attack on each note)
- Spiccato (short bouncing strokes via velocity)
- Tremolo (rapid re-bowing via CC)

MPE per-note expression directly controls bow pressure (Z/aftertouch) and bow position (Y/slide).

## Architecture

- **Monophonic per string** (authentic physical modeling), optional 2-voice for double stops
- **2x internal oversampling** for friction junction stability
- **Zero algorithmic latency** (waveguide is causal)
- **MPE support from day one** — per-note pitch, pressure, slide
- **Configurable 1-4 active strings** (not hardcoded per instrument)
- **0-12 passive sympathetic strings** (optional)

## Performance Targets

| Metric | Target |
|--------|--------|
| CPU per string (core tier) | <2% |
| CPU total (2 strings + body) | <6% |
| Oversampling | 2x (friction junction) |
| Latency | 0 samples (waveguide is causal) |

## Instrument Presets

Realistic:
- Violin (G-D-A-E fifths, wood body, small size)
- Cello (C-G-D-A fifths, wood body, large size)
- Viola (C-G-D-A fifths, wood body, medium size)
- Double Bass (E-A-D-G fourths, wood body, large size)
- Erhu (D-A fifth, membrane body, small size)
- Sarangi (membrane body, medium size)
- Nyckelharpa (wood body, sympathetic strings active)

Sound Design:
- Glass Bow (glass body, high brightness, infinite sustain)
- Metal Drone (metal body, sub-harmonics, reversed friction)
- Impossible Strings (full impossible physics, synthetic territory)
- Breath of Strings (bow noise emphasis, low pressure, ethereal)

## Use Cases

- **Realistic bowed instruments** — violin, cello, erhu, sarangi leads and sustained lines
- **Non-Western music** — erhu, sarangi, nyckelharpa with authentic tuning systems
- **Microtonal composition** — full Scala/TUN/MTS-ESP support for any tuning system
- **Sound design** — evolving organic drones, impossible physics territory
- **Film/game scoring** — expressive solo strings with MPE control
- **Ambient/experimental** — sympathetic resonance, sub-harmonics, reversed friction
- **Live performance** — MPE controllers (Linnstrument, Seaboard, Sensel) for expressive real-time bowing

## Inspirations

- **AAS String Studio VS-3** — philosophy of bowed string as synthesis (not emulation)
- **SWAM Strings** — reference for what realistic PM bowed strings sound like (NOT competing with)
- **STK (Synthesis Toolkit)** — Bowed class implementation, academic reference
- **Julius Smith waveguide papers** — foundational delay-line topology
- **Woodhouse bow-string friction research** — tiered friction modeling approach

## Technical Notes

- Friction model is MORE important than waveguide topology for differentiation
- Body morphing is completely uncontested in the market
- Non-Western instruments come "for free" as body+string presets
- Price target: $49-79
- Research documents available in `research/`:
  - `bow-string-friction-models.md` — 4 friction models with equations and C++ code
  - `O-Bowed-market-research.md` — competitive analysis and market gaps
  - `O-Bowed-acoustic-instrument-research.md` — instrument acoustics and parameters
  - `O-Bowed-research-synthesis.md` — unified architecture and implementation roadmap

## Competitive Position

- NOT trying to beat SWAM at instrument emulation — different category
- Body morphing is completely uncontested in the market
- Microtonal support differentiates from all competitors
- Non-Western instruments (erhu, sarangi) come "for free" as presets
- Closest comparison: AAS String Studio VS-3 philosophy but with vastly better friction modeling and body resonance

## Next Steps

- [ ] Stage 0: Planning — research DSP approach and create architecture (`/plan O-Bowed`)
- [ ] Create UI mockup (`/start O-Bowed` -> option 3)
- [ ] Start implementation (`/implement O-Bowed`)
