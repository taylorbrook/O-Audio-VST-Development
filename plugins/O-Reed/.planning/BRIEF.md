# O-Reed - Creative Brief

## Overview

**Type:** Synth (Physical Modeling Reed Wind Instrument)
**Core Concept:** General-purpose reed wind instrument physical modeling synthesizer using digital waveguides and nonlinear reed-bore coupling. NOT an instrument emulator -- a reed wind SYNTHESIZER where clarinet, saxophone, oboe, duduk, zurna, shehnai, suona, hichiriki, and synthetic/impossible instruments are presets within a continuous parameter space.
**Status:** Ideated
**Created:** 2026-04-04

## Vision

The reed-bore coupling (Bernoulli flow through a vibrating reed into a resonant tube) is the synthesis engine. A parameterized bore profile (cylindrical to conical), configurable reed properties, and Guillemain's confined jet parameter (Psi) give each instrument its identity. The same engine produces realistic reed tones AND sound design territory -- sustained, organic, evolving wind textures that nothing else in the Ouaricon catalog touches.

This follows the O-Bowed philosophy: instrument identity emerges from continuous parameter configuration, not separate code paths or products. A single Psi parameter morphs single-reed to double-reed character. A single bore_taper parameter morphs cylindrical (clarinet harmonics) to conical (saxophone harmonics).

**Key differentiators:**
- Guillemain's confined jet parameter (Psi) -- one knob morphs single-reed to double-reed character (no competitor does this)
- Continuous bore morphing: cylindrical <-> conical <-> reverse conical (hichiriki) -- completely uncontested
- Non-Western reed instruments (duduk, shehnai, suona, hichiriki, zurna, piri) as first-class presets -- zero competition in desktop VST3/AU
- Realistic-to-experimental continuum in one engine (SWAM can't do weird, Respiro can't do realistic)
- Full breath controller + MPE support with per-note embouchure
- Unified product replacing $750+ of SWAM separate purchases

## Signal Flow

```
MIDI/MPE -> [Mouth Pressure + Breath Noise] -> [Reed Model (Mass-Spring-Damper)]
         -> [Nonlinear Junction (Bernoulli + Psi confinement)]
         <-> [Bore Waveguide (cylindrical/conical)]
         -> [Tone Hole Lattice (2-4 virtual holes + register hole)]
         -> [Bell Radiation Filter]
         -> [Optional: Growl/Vocal Coupling]
         -> Output
```

## Reed Model (The Excitation Engine)

Full mass-spring-damper reed with Guillemain extensions:

```
mu_r * d2x/dt2 + g_eff * dx/dt + k_eff * x = (p_mouth - p_bore) * A_reed
```

Volume flow through reed channel (Bernoulli + confinement):
```
u(t) = sign(delta_p) * alpha * S_i(t) * sqrt(2 * |delta_p| / (rho * (1 + Psi * alpha^2 * S_i^2 / S_r^2)))
```

| Psi Value | Character | Instruments |
|-----------|-----------|-------------|
| 0 | Pure single-reed | Clarinet, saxophone |
| 0.1-0.3 | Mild double-reed | Duduk, piri |
| 0.3-0.6 | Strong double-reed | Oboe, bassoon |
| 0.6+ | Extreme confinement | Zurna, shehnai (piercing) |

## Bore Model (The Resonator)

Digital waveguide with parameterized bore profile:

- **bore_taper = 0**: Cylindrical (odd harmonics only, overblows at 12th) -- clarinet, duduk, hichiriki, piri, arghul
- **bore_taper > 0**: Conical (all harmonics, overblows at octave) -- saxophone, oboe, bassoon, zurna, shehnai, suona
- **bore_taper < 0**: Reverse conical (unique to hichiriki -- narrows at bottom)

Implementation: Start with cylindrical waveguide + conical correction filter (Strategy B), upgrade to true conical sections (Strategy C) for accuracy.

## Parameters

### Primary Controls (Tier 1 -- Always Visible)

| Parameter | Range | Default | Description | MIDI Map |
|-----------|-------|---------|-------------|----------|
| Breath Pressure | 0-100% | 50% | Mouth pressure (p_mouth) -- main dynamics | CC2/Breath/MPE Pressure |
| Embouchure / Bite | 0-100% | 40% | Lip force -- brightness, pitch bending | CC1/MPE Slide Y |
| Reed Hardness | 0-100% | 50% | Reed stiffness (k_r) -- attack character, brightness | -- |
| Bore Character | 0-100% | 0% (cyl) | Bore taper -- 0=cylindrical to 100=full cone | -- |
| Instrument Morph | Preset-based | Clarinet | Macro crossfading full parameter sets between presets | -- |

### Secondary Controls (Tier 2 -- Expandable Panel)

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Reed Opening | 0-100% | 40% | Rest opening (H) -- ease of onset, dynamic range |
| Bell Size | 0-100% | 50% | Bell flare -- projection, high-frequency radiation |
| Air Noise | 0-100% | 15% | Breath noise mix -- breathiness |
| Double Reed Amount | 0-100% | 0% | Psi confinement -- single to double reed character |
| Bore Diameter | 0-100% | 50% | Viscothermal loss and impedance -- intimacy vs projection |

### Advanced / Sound Design (Tier 3)

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Reed Mass | 0-100% | 30% | mu_r -- reed resonance effects, transient character |
| Reed Damping | 0-100% | 50% | g_r -- reed ring vs. muted bore-driven tone |
| Mouthpiece Volume | 0-100% | 30% | Helmholtz resonance -- pitch correction, sub-resonance coloring |
| Tone Hole Cutoff | 200-8000 Hz | 1500 Hz | Spectral envelope from open tone holes |
| Register Hole | 0-100% | 0% | Overblowing control (12th for cyl, octave for conical) |
| Bore Length | 0-100% | 50% | Effective tube length -- register density |
| Bore Profile | Simple/Multi | Simple | Single taper vs. multi-segment (throat/body/bell) |

### Expressive Controls

| Parameter | Range | Default | Description | MIDI Map |
|-----------|-------|---------|-------------|----------|
| Vibrato Depth | 0-100% | 0% | LFO modulation depth | CC77 |
| Vibrato Rate | 1-10 Hz | 5 Hz | LFO frequency | CC76 |
| Vibrato Source | Lip/Breath/Throat | Lip | Which parameter the vibrato modulates | -- |
| Growl Amount | 0-100% | 0% | Vocal fold coupling strength | CC80 |
| Flutter Tongue | 0-100% | 0% | ~25 Hz pressure modulation | -- |
| Subtone | 0-100% | 0% | Minimum reed opening + high lip damping | -- |
| Attack Chiff | 0-100% | 30% | Pressure overshoot at note onset | Velocity |

### Impossible Physics (Sound Design)

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Infinite Sustain | 0-100% | 0% | Reduces bore losses toward zero |
| Reverse Bore | 0-100% | 0% | Negative taper (hichiriki-like, extended beyond physical) |
| Dual Bore | Off/On | Off | Second parallel waveguide (arghul/launeddas drone mode) |
| Drone Pitch | -24 to +24 semitones | 0 | Pitch offset for second bore |
| Feedback Path | 0-100% | 0% | Cross-modulation between reed and bore |

### Tuning

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Reference Pitch | 220-880 Hz | 440 Hz | A4 reference frequency |
| Tuning System | File/MTS-ESP/12TET | 12TET | Scala/TUN import, MTS-ESP, or standard |

## Note Behavior

**Monophonic primary mode** (physically accurate -- reed instruments are fundamentally monophonic):
- Breath sustains while MIDI note held
- Legato: smooth tone hole transitions between notes (no re-tongue)
- Staccato: tongue articulation on each note (velocity controls attack force)
- Release: natural pressure decay with bore ring-down

**Polyphonic mode** (for sound design/layering):
- 8 voices default, expandable to 16
- Each voice is independent reed+bore instance
- Useful for chords, unison thickening, ensemble simulation

**MPE per-note expression:**
- Strike (velocity) = tongue articulation force
- Pressure = sustained breath pressure
- Slide Y (CC74) = embouchure / brightness
- Glide X (pitch bend) = pitch bend / vibrato

**Breath controller integration (critical):**
- CC2 maps directly to mouth pressure with configurable curve (linear/exponential/S-curve)
- When no breath controller: velocity -> attack, sustain level for held notes, CC11 for dynamics
- Compatible with: TEControl, Akai EWI, Roland Aerophone, Sylphyo, Warbl

## Architecture

- **Monophonic default** (authentic reed instrument behavior)
- **8-voice polyphonic option** for sound design / layering
- **2x internal oversampling** (sweet spot for reed nonlinearity aliasing reduction)
- **4x oversampling option** for monophonic high-quality mode
- **Zero algorithmic latency** (waveguide is causal)
- **MPE support from day one** -- per-note pitch, pressure, slide
- **Tone hole lattice** -- 2-4 virtual three-port scattering junctions + register hole
- **Fractional delay lines** (allpass interpolation) for fine pitch control

## Performance Targets

| Metric | Target |
|--------|--------|
| CPU per voice (2x OS) | ~168 ops/sample (~0.2%) |
| CPU monophonic (4x OS) | ~336 ops/sample (~0.4%) |
| Polyphony | 8-16 voices practical |
| Oversampling | 2x default, 4x mono quality |
| Latency | 0 samples |

## Instrument Presets

### Western Reed Instruments
- **Bb Clarinet** -- cylindrical bore, single reed, odd harmonics, overblows at 12th
- **Bass Clarinet** -- longer cylindrical bore, darker, wider bell
- **Alto Saxophone** -- conical bore ~1.6 deg, single reed, all harmonics, bright attack
- **Tenor Saxophone** -- wider conical bore, warmer, subtone-capable
- **Soprano Saxophone** -- narrow conical, bright, piercing
- **Baritone Saxophone** -- wide conical, deep, full
- **Oboe** -- narrow conical ~0.82 deg, double reed (Psi=0.4), nasal, penetrating
- **English Horn** -- wider conical, pear-shaped bell (lowpass), mellow, plaintive
- **Bassoon** -- long narrow conical ~0.41 deg, double reed (Psi=0.3), warm, buzzy

### Non-Western Reed Instruments
- **Duduk** (Armenian) -- cylindrical bore + double reed (unusual), soft, human-voice-like, ~1.3 octave range
- **Shehnai** (Indian) -- conical bore, quadruple reed (high Psi), powerful, nasal, bright
- **Suona** (Chinese) -- conical bore + metal bell, double reed, extremely loud, shrill
- **Hichiriki** (Japanese) -- reverse conical bore (unique), double reed, extreme pitch bending (embai)
- **Zurna** (Turkish/Persian) -- wide conical, double reed, piercing, outdoor instrument, almost no soft dynamics
- **Piri** (Korean) -- cylindrical + double reed, significant pitch bending, no bell
- **Arghul** (Egyptian) -- dual cylindrical bores (drone + melody), single idioglot reed, requires dual-bore mode
- **Launeddas** (Sardinian) -- triple cylindrical pipes, single idioglot reeds, drone + two melodies
- **Mijwiz** (Levantine) -- dual equal-length pipes, natural beating/chorusing from slight detuning

### Sound Design
- **Glass Reed** -- extreme stiffness, low damping, crystalline
- **Metal Wind** -- high bore losses inverted, resonant, metallic
- **Impossible Bore** -- reverse taper + dual bore + feedback, alien wind creature
- **Breath Drone** -- high air noise, low reed, atmospheric wind texture
- **Giant Clarinet** -- bore length extreme, sub-bass territory
- **Micro Reed** -- tiny bore, extreme high register, insect-like

## Use Cases

- **Realistic solo woodwinds** -- clarinet, sax, oboe, bassoon leads and sustained lines
- **Non-Western music** -- duduk for Armenian, shehnai for Indian classical, suona for Chinese ceremonial, hichiriki for Japanese gagaku
- **Film/game scoring** -- expressive solo winds with MPE/breath control, ethnic wind cues
- **World music production** -- authentic non-Western instruments with continuous pitch bending and microtonal ornaments (impossible with sample libraries)
- **Sound design** -- evolving organic wind drones, impossible physics territory
- **Ambient/experimental** -- dual-bore drones, feedback paths, reverse bores
- **Live performance** -- breath controllers (EWI, Aerophone, Sylphyo) and MPE controllers (Osmose, LinnStrument, Seaboard)

## Inspirations

- **SWAM Woodwinds** (Audio Modeling) -- reference for Western instrument realism (NOT competing on realism alone)
- **Respiro** (Imoxplus) -- reference for experimental PM wind territory
- **GeoShred/Naada** (moForte/Julius Smith) -- reference for non-Western PM instruments (iOS-only, no desktop VST3)
- **STK** (Cook & Scavone) -- BlowHole, Saxofony classes, educational reference
- **Faust pm.lib** -- modular building blocks for PM wind instruments
- **Erica Synths Steampipe** -- proves market appetite for PM wind + experimental (hardware-only, $999)

## Competitive Position

- **The market has one dominant player** (SWAM, $750 for all woodwinds) and nothing else serious
- **Three wide-open gaps O-Reed owns:**
  1. Non-Western reed instruments -- zero desktop VST3/AU PM plugins cover duduk, shehnai, suona, hichiriki
  2. Unified reed engine -- SWAM sells clarinets, saxophones, double reeds as 3 separate products ($670 total)
  3. Realistic-to-experimental continuum -- SWAM does realistic only, Respiro does experimental only
- **Bore morphing is completely uncontested** -- no competitor morphs between cylindrical and conical
- **Price target: $129** -- undercuts SWAM Clarinets ($170) alone while covering ALL reed instruments
- **Competitive window: 12-18 months** before anyone could match this combination

## Technical Notes

### Core DSP Architecture (Smith waveguide + Guillemain extensions)
- Digital waveguide bore (two fractional delay lines with frequency-dependent bell reflection)
- Full mass-spring-damper reed model (not simplified static reed)
- Guillemain confined jet parameter (Psi) for single-to-double-reed morphing
- Conical bore: start with cylindrical + correction filter (Strategy B), upgrade to true conical sections (Strategy C)
- Keefe tone hole model: 2-4 virtual three-port scattering junctions
- Polynomial approximation for reed nonlinearity (default), Newton-Raphson for full dynamic reed

### Key Acoustic Facts
- Cylindrical bore = odd harmonics only, overblows at 12th (clarinet, duduk)
- Conical bore = all harmonics, overblows at octave (sax, oboe, bassoon, zurna)
- Duduk is unusual: cylindrical bore + double reed (most double reeds are conical)
- Hichiriki is unique: reverse conical bore (narrows at bottom)
- Bore material does NOT affect timbre (scientific consensus) -- geometry is everything
- Reed resonance freq (~2500-4000 Hz) is well above playing range -- 2x oversampling sufficient

### Research Documents (CRITICAL -- read before Stage 0 planning)
- `research/reed-physical-modeling-dsp.md` -- Full DSP algorithms with C++ pseudocode, reed equations, bore waveguide implementation, tone hole model, CPU analysis, implementation roadmap
- `research/O-Reed-market-research.md` -- SWAM/Respiro/Chromaphone/GeoShred analysis, gap analysis, pricing strategy, user needs
- `research/O-Reed-acoustic-properties-reed-instruments.md` -- Bore dimensions for 12+ instruments, comparison tables, extended techniques, material debate, morphing analysis
- `research/O-Reed-research-synthesis.md` -- Unified findings, architecture diagram, parameter presets table, 4-stage DSP roadmap
- `research/physical-modeling-research-agent-3-physical-modelling-optimization.md` -- CPU optimization, SIMD, polyphony management (general PM reference)

### Key Academic References
- Guillemain (2004) -- Double-reed synthesis model (EURASIP) -- the Psi parameter that makes this engine work
- Guillemain et al. (2005) -- Real-time clarinet synthesis (JASA 118)
- Avanzini & van Walstijn (2004) -- Reed-mouthpiece-lip system (Acta Acustica)
- Smith (1986) -- Efficient reed-bore simulation (ICMC)
- Keefe (1981) -- Woodwind tone-hole acoustics
- Maugeais & Dalmont (2024) -- "What makes the duduk special" (Acta Acustica) -- critical for duduk preset
- Music et al. (2025) -- Port-Hamiltonian woodwind model (Frontiers) -- stability framework reference

## Next Steps

- [ ] Stage 0: Planning -- create architecture and roadmap (`/plan O-Reed`)
- [ ] Create UI mockup (`/start O-Reed` -> option 3)
- [ ] Start implementation (`/implement O-Reed`)
