# OuariconLyrica - Creative Brief

## Overview

**Type:** Synth (Physical Modeling Instrument)
**Core Concept:** Flexible physical modeling harp capable of emulating concert, Celtic, lyre, and fantasy harps with full expressive control
**Status:** 💡 Ideated
**Created:** 2026-01-16

## Vision

OuariconLyrica is an ambitious physical modeling synthesizer that captures the essence of plucked string instruments, with the harp as its foundation. Unlike sample-based libraries, it uses physical modeling synthesis to create a living, breathing instrument where every parameter affects the sound organically.

The instrument is designed to be a chameleon — capable of producing authentic concert harp tones for orchestral work, intimate Celtic harp for folk arrangements, ancient lyre sounds for historical contexts, and completely fantastical impossible instruments for sound design and ambient textures. The same engine powers all these sounds through deep physical parameter control.

Inspired by Modartt Pianoteq's acclaimed harp model and real concert harps, OuariconLyrica aims to bring that level of physical modeling sophistication to a more flexible, experimental instrument.

## Parameters

### String Properties
| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| stringTension | 0.0-1.0 | 0.5 | Tension of the modeled string |
| stringMaterial | Choice | Nylon | Material type (gut, nylon, wire, carbon, metal alloy, glass, crystal, energy) |
| stringGauge | 0.0-1.0 | 0.5 | Thickness/gauge of the string |
| stringLength | 0.0-1.0 | 0.5 | Effective length scaling |

### Body/Resonance
| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| soundboardSize | 0.0-1.0 | 0.5 | Size of the resonating soundboard |
| woodType | Choice | Spruce | Body material (spruce, maple, exotic, synthetic) |
| bodyResonance | 0.0-1.0 | 0.6 | Amount of body resonance/coloration |
| sympatheticAmount | 0.0-1.0 | 0.3 | Sympathetic string resonance intensity |

### Pluck Mechanics
| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| pluckPosition | 0.0-1.0 | 0.5 | Where along the string the pluck occurs |
| pluckVelocity | 0.0-1.0 | 0.7 | Initial energy of the pluck |
| fingerHardness | 0.0-1.0 | 0.5 | Hardness of the plucking finger/pick |

### Expression & Performance
| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| technique | Choice | Normal | Playing technique (normal, harmonic, muted, près de la table) |
| glissandoMode | Choice | Off | Glissando mode (off, free, scale-locked) |
| glissandoScale | Choice | Major | Scale for scale-locked glissando |

### Global
| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| masterTune | 400-480 Hz | 440 | Master tuning reference |
| pitchBendRange | 1-48 st | 2 | Per-note pitch bend range in semitones |

## UI Concept

**Layout:** To be designed in mockup phase
**Visual Style:** To be designed in mockup phase
**Key Elements:**
- String visualization showing resonance
- Material/body selection interface
- Expression control section
- Glissando configuration
- Microtonal tuning module integration point

## Use Cases

- **Orchestral/Classical:** Realistic concert harp for traditional compositions and film scoring
- **Celtic/Folk:** Authentic traditional folk harp for Celtic arrangements
- **Ambient/Cinematic:** Ethereal textures, evolving drones, atmospheric soundscapes
- **Sound Design:** Experimental tones using impossible materials (glass, crystal, energy strings)
- **Microtonal Music:** Integration with microtonal tuning module for non-Western and experimental tunings

## Inspirations

- **Modartt Pianoteq Harp Model:** Industry-leading physical modeling approach
- **Real Concert Harps:** Authentic acoustic behavior and playing techniques
- **Physical Modeling Synthesis:** Waveguide/modal synthesis approaches

## Technical Notes

### Physical Modeling Approach
- Research needed in Stage 0 to determine optimal approach (Karplus-Strong, waveguide, modal synthesis, or hybrid)
- Sympathetic resonance requires modeling string-to-string coupling
- Per-note pitch bend requires careful voice architecture

### Performance Considerations
- Unlimited polyphony target requires efficient DSP implementation
- Physical modeling is CPU-intensive; may need quality/performance modes
- Real-time parameter modulation must be smooth and artifact-free

### Integration Points
- Microtonal tuning module from existing Ouaricon module system
- Modular FX system for post-processing
- Keyswitch system for technique selection

### Extended Techniques
- **Harmonics:** Touch harmonics at natural nodes
- **Muting:** Palm muting for dampened tones
- **Près de la table:** Playing near the soundboard for metallic timbre
- **Glissando:** Both free and scale-locked sweeping runs

### String Materials to Model
- **Traditional:** Gut (warm, historical), Nylon (modern classical), Wire (bright, Celtic bass)
- **Modern:** Carbon fiber (bright, sustaining), Metal alloys (experimental)
- **Fantasy:** Glass (crystalline), Crystal (pure, bell-like), Energy (synthetic, impossible)

## Next Steps

- [ ] Create UI mockup (`/start OuariconLyrica` → option 3)
- [ ] Start implementation (`/implement OuariconLyrica`)
