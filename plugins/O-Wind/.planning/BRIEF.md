# O-Wind - Creative Brief

## Overview

**Type:** Synth (Physical Modeling Flute)
**Core Concept:** A physically-modeled multi-instrument flute synthesizer using a jet-drive waveguide engine that produces concert flute, shakuhachi, bansuri, Native American flute, recorder, and pan flute through different bore/embouchure parameter sets.
**Status:** Ideated
**Created:** 2026-04-04

## Vision

O-Wind is a physically-modeled flute synthesizer built on the Verge (1995) jet-drive model coupled to a digital waveguide bore. Unlike sample-based ethnic wind libraries (Impact Soundworks Ventus) or expensive monophonic PM flutes (SWAM $249), O-Wind delivers dedicated, pre-tuned physical models of concert flute, shakuhachi, bansuri, and Native American flute at $99-149 with MPE polyphony.

The core architecture is a one-directional jet exciter driving a bidirectional bore waveguide -- simpler than O-Bowed's friction scattering junction. No iterative solver, no separate body resonator (the bore IS the body), no sympathetic coupling. The jet provides energy; the bore resonance controls pitch. Overblowing happens by increasing jet velocity (shortening jet delay), causing the model to lock onto higher harmonics.

The plugin is wind-controller-first (CC2 breath mapping, tonguing detection, embouchure-to-timbre control) with intelligent keyboard fallback (auto-breath envelope). MPE support enables polyphonic pan flute chords and ensemble textures -- a genuine first in PM wind instruments.

Three "impossible physics" parameters (Infinite Sustain, Reversed Jet, Sub-Harmonics) extend the engine into creative sound design territory.

## Signal Flow

```
Breath Pressure -> Bernoulli (jet velocity) -> + Turbulence Noise
                                                      |
Bore feedback (from previous sample) -> Embouchure Summation
                                                      |
                                             Jet Delay Line (Lagrange3rd, modulatable)
                                                      |
                                             Jet-Labium Nonlinearity (tanh saturation)
                                                      |
                                                 DC Blocker
                                                      |
                                             Bore Waveguide (Thiran, bidirectional)
                                               | Tone Hole Junctions
                                                      |
                                             End Reflection Filter -> feedback loop
                                                      |
                                             Radiation Filter -> Output
```

Key physics: The air jet oscillates against the labium (sharp edge). No vibrating reed. The bore resonance controls pitch; the jet provides energy. The tanh saturation at the labium limits amplitude and shapes the waveform. Turbulence noise scales with jet velocity squared (louder = breathier, physically correct).

## Parameters

### Breath/Excitation

| Parameter | Range | Default | Description | MIDI Map |
|-----------|-------|---------|-------------|----------|
| Breath Pressure | 0-100% | 50% | Jet velocity (nonlinear: pressure^1.5 curve) | CC2/Breath, Aftertouch |
| Embouchure | 0-100% | 50% | Jet delay ratio (0.3-0.6 of bore). Controls register/overblowing. Low=focused/bright, high=spread/dark | CC74/MPE Y (Slide) |
| Breath Noise | 0-100% | 15% | Turbulence noise gain. Scaled by jet velocity squared | -- |
| Tone Color | 0-100% | 50% | Bore reflection filter cutoff (1000-12000 Hz log). Brightness control | -- |

### Resonator

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Air Column | 0-100% | 50% | Viscothermal loss amount. High=warmer (more HF loss) |
| Jet Reflection | -100% to 100% | 50% | Feedback coefficient from bore to jet |
| End Reflection | -100% to 100% | 50% | Bore end reflection coefficient. Controls sustain character |

### Expression

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Vibrato Rate | 2-8 Hz | 5 Hz | Pressure vibrato (NOT pitch vibrato -- more natural for flute) |
| Vibrato Depth | 0-100% | 30% | Modulation depth on breath pressure |

### Output

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Width | 0-200% | 100% | Stereo decorrelation |
| Output Level | -inf to +12 dB | 0 dB | Master gain |

### Impossible Physics (Creative Sound Design)

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Infinite Sustain | 0-100% | 0% | Reduces bore losses toward zero -- endless resonance |
| Reversed Jet | 0-100% | 0% | Inverts jet nonlinearity curve -- synthetic excitation |
| Sub-Harmonics | 0-100% | 0% | Sub-octave content via nonlinear feedback |

### MIDI/MPE Mapping

| Source | Target | Mapping |
|--------|--------|---------|
| Note number | Bore delay length | Pitch via tuning engine |
| Velocity | Breath attack ramp time | 127=5ms sharp, 1=30ms gentle |
| Aftertouch/CC2 | Breath pressure | Primary dynamics |
| MPE Y (CC74/Slide) | Embouchure | Continuous timbre control |
| Pitch bend | Bore delay modulation | Smooth portamento |
| CC1 (Mod) | Vibrato depth | Standard mapping |
| CC11 (Expression) | Output level | Standard dynamics |

## Instrument Presets

### Core Set (Ship at Launch)

**Concert Flute (Western Boehm):** Cylindrical bore 19mm, side-blown, 16 keyed tone holes. Moderate breathiness, clear projecting tone. Range: C4-C7.

**Shakuhachi (Japanese):** Conical bore with root flare 18-22mm, end-blown, 5 finger holes. High breathiness, extreme embouchure expressiveness (meri/kari pitch bending). Extended embouchure control range. Range: D4 upward, ~2.5 octaves.

**Bansuri (Indian):** Cylindrical bamboo bore 14-16mm, side-blown, 6-7 finger holes. Moderate-high breathiness, smooth meend (pitch slides). Range: ~2.5 octaves.

**Native American Flute:** Dual-chamber design, end-blown with fipple/block. Pentatonic-friendly tuning, very breathy, meditative character. Simplest embouchure model (fixed). Range: ~1.5 octaves.

### Expansion Presets (Post-Launch)

- Recorder (Baroque soprano/alto) -- fixed embouchure, conical bore taper
- Pan Flute (Andean siku) -- polyphonic use case, short open tubes
- Piccolo -- short bore, bright
- Ocarina -- Helmholtz resonator variant

## Tone Hole Architecture (Two-Tier)

**Tier 1 (default):** Simplified bore-length-switching. MIDI note -> effective bore delay length. Crossfade over 2-5ms on fingering change. Computationally cheap.

**Tier 2 (enhancement):** Full Keefe 3-port scattering junctions per tone hole. Enables half-holing, cross-fingering timbral variations, realistic key click transients. 6-8 second-order IIR filter pairs.

## Use Cases

- Realistic solo flutes -- concert flute, shakuhachi, bansuri leads
- World/ethnic music -- shakuhachi honkyoku, bansuri ragas, Native American meditation
- Film/game scoring -- expressive ethnic wind solos with breath controller
- Ambient/meditation -- breathy sustained tones, Native American flute textures
- Sound design -- impossible physics, reversed jet, infinite sustain drones
- Ensemble textures -- MPE polyphonic pan flute, flute choir
- Live performance -- wind controllers (EWI, Aerophone, Sylphyo) with MPE

## Inspirations

- **SWAM Flutes** (Audio Modeling) -- quality reference ($249, monophonic)
- **STK Flute** (Perry Cook / CCRMA) -- canonical reference implementation
- **Faust pm.lib** -- modular waveguide components
- **Verge (1995) jet-drive model** -- standard for time-domain flute simulation
- **de la Cuadra (2005)** -- refined jet model with experimental validation
- **Keefe (1990)** -- tone hole acoustic modeling
- **O-Bowed** (Ouaricon) -- same philosophical approach, shared module patterns

## Technical Notes

### Architecture
- 2x oversampling for jet nonlinearity section
- 8-voice max polyphony, 4-voice default
- Manual sample-by-sample loop required (feedback prevents ProcessorChain)
- Bore delay: `DelayLine<float, Thiran>` (stable, no modulation needed)
- Jet delay: `DelayLine<float, Lagrange3rd>` (must support real-time modulation)
- Jet nonlinearity: `tanh` saturation (more stable than cubic polynomial)
- DC blocker: `y[n] = x[n] - x[n-1] + 0.995 * y[n-1]`
- Zero algorithmic latency (waveguide is causal)

### Shared Modules with O-Bowed
- Tuning engine (Scala/MTS-ESP)
- Stereo width processor
- MIDI/MPE routing and voice allocation
- Parameter smoothing patterns

### Performance Targets

| Metric | Target |
|--------|--------|
| CPU per voice (simple model) | <2.5% |
| CPU per voice (full tone holes) | <3.5% |
| CPU total (4 voices) | <14% |
| Oversampling | 2x (jet nonlinearity) |
| Latency | 0 samples |
| Ops/sample/voice | ~135 |

### Key Differentiators
1. First plugin with dedicated PM shakuhachi, bansuri, and Native American flute
2. $99-149 -- undercuts SWAM Flutes ($249) by 40-60%
3. MPE polyphonic -- SWAM and Respiro are monophonic
4. Wind-controller-first with keyboard fallback
5. ~30-50% cheaper CPU than O-Bowed per voice
6. No iterative friction solver needed (simpler architecture)

### Price Target
$99-$149

## Research Documents

- `research/flute-physical-modeling-synthesis.md` -- jet-drive model, waveguide bore, tone holes, embouchure physics, instrument variants, 18 references
- `research/O-Wind-market-research.md` -- competitive landscape, market gaps, wind controller hardware
- `research/flute-waveguide-juce8-implementation.md` -- JUCE 8 DSP class mapping, APVTS spec, CPU estimates, O-Bowed comparison

## Next Steps

- [ ] Stage 0: Research and planning (`/plan O-Wind`)
- [ ] Create UI mockup (`/start O-Wind` -> option 3)
- [ ] Start implementation (`/implement O-Wind`)
