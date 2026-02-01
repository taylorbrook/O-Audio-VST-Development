# O-Bells Creative Brief

## Vision Statement

O-Bells is a **unified physical modeling bells synthesizer** that spans the timbral range from orchestral tubular bells to church carillons, hand bells, and gamelan metallophones — all from a single coherent engine. Designed for **cinematic scoring and ambient/atmospheric production**, it combines the authentic physics of struck metal with ensemble voicing for rich, evolving textures.

## Core Concept

A physically-modeled bell engine where continuous parameters morph the instrument across bell archetypes rather than discrete model switching. The characteristic "minor third" partial structure, strike dynamics, and resonant decay of real bells emerge naturally from the physics simulation.

## Target Use Cases

**Primary:**
- Cinematic/Orchestral — Film scores, dramatic swells, tension building
- Ambient/Atmospheric — Drones, shimmering textures, soundscapes

**Secondary:**
- Electronic/Pop — Modern production accents
- Experimental — Sound design, unusual metallic timbres

## Technical Specifications

### Synthesis Engine
- **Method:** Physical modeling (modal synthesis)
- **Polyphony:** 8 voices with voice stealing
- **Ensemble:** Unison spread (1-4 bells per voice) AND octave layering (sub/fundamental/octave)

### Main Panel Parameters (7)

| Parameter | Description | Range |
|-----------|-------------|-------|
| **Strike Position** | Center (fundamental) to edge (bright partials) | 0-100% |
| **Mallet Hardness** | Soft felt → hard metal striker | 0-100% |
| **Bell Size/Mass** | Small hand bell → large church bell character | 0-100% |
| **Damping** | Hand-damped (short) → free-ring (infinite) | 0-100% |
| **Brightness** | Dark/muted → brilliant/shimmering | 0-100% |
| **Material** | Bronze → Steel → Glass → Crystal | Continuous morph |
| **Inharmonicity** | Pure harmonic → characteristic bell partials | 0-100% |

### Advanced Panel Parameters

| Parameter | Description |
|-----------|-------------|
| **Partial Tuning** | Adjust the minor-third overtone characteristic |
| **Nonlinear Effects** | Bell warping/distortion at high velocity |
| **Sympathetic Resonance** | Other voices ring when one strikes |
| **Strike Noise Character** | Transient texture (click, thud, ping) |
| **Decay Shape** | Linear vs exponential vs multi-stage |
| **Velocity Curve** | Response shaping |
| **Pitch Envelope** | Initial pitch drop (large bells) |

### Ensemble Section

| Parameter | Description |
|-----------|-------------|
| **Unison Count** | 1-4 bells per voice |
| **Unison Detune** | Spread amount between unison voices |
| **Octave Blend** | Sub (-1) / Fundamental / Octave (+1) mix |
| **Stereo Spread** | Ensemble panning width |

## Reference Instruments

- **Spectrasonics Omnisphere** — Bells patches, lush layering
- **Pianoteq Tubular Bells** — Physical modeling authenticity, parameter depth

## UI/UX Design

### Aesthetic Direction
**Ouaricon Botanical** theme with snail motif

### Visual Elements
- **Hero Image:** Spiral snail shell (Architectonica perspectiva species)
- **Color Palette:** Warm amber, bronze, cream, aged gold — reflecting bell metal patina
- **Layout:** Two-panel design (Main / Advanced tabs)

### Image Asset
```
Source: /Users/taylorbrook/Dev/Ouaricon Audio Images/insects/snails_spciesgnra12kiene_0169.png
```

The spiral shell geometry echoes:
- The circular cross-section of bells
- The mathematical spiral of harmonic series
- The golden ratio found in acoustic design

### Panel Layout Concept

**Main Panel:**
```
┌─────────────────────────────────────────────────┐
│  [Snail Image]           O-BELLS                │
│                                                 │
│  Strike ──●──  Mallet ──●──  Size ──●──        │
│  Damping ──●── Bright ──●──  Material ──●──    │
│                Inharm ──●──                     │
│                                                 │
│  ═══════════ ENSEMBLE ═══════════              │
│  Unison [1-4]  Detune ──●──  Spread ──●──      │
│  Oct Blend: [Sub ──●── Fund ──●── Oct]         │
│                                                 │
│  [Main]  [Advanced]                             │
└─────────────────────────────────────────────────┘
```

## Audio Characteristics

### Timbral Goals
- Authentic bell partials (minor third at ~2.4x fundamental)
- Natural velocity response (harder = brighter + more partials)
- Long, shimmering decay with subtle beating
- Strike transients that vary with mallet hardness

### CPU Considerations
- 8-voice polyphony is reasonable for modal synthesis
- Ensemble multiplies voice count (up to 32 simultaneous modes)
- Consider quality settings if needed

## Preset Categories

1. **Orchestral** — Tubular bells, chimes, glockenspiels
2. **Sacred** — Church bells, carillons, meditation bowls
3. **World** — Gamelan, Tibetan bowls, ethnic percussion
4. **Ambient** — Evolving pads, frozen shimmer, drones
5. **Cinematic** — Tension risers, horror stingers, epic swells

## Success Criteria

- [ ] Seamless morphing between bell types via unified parameters
- [ ] Authentic bell timbre with characteristic partial structure
- [ ] Rich ensemble textures without excessive CPU
- [ ] Pianoteq-level parameter depth with Spectrasonics-level musicality
- [ ] Botanical aesthetic with snail motif fully integrated

## Research Requirements

Before implementation, investigate:
- Modal synthesis techniques for bells (Aramaki, Smith, Rossing papers)
- Partial frequency ratios for different bell types
- Pianoteq's approach to tubular bell modeling
- Efficient ensemble voice management

---

*Created: 2026-02-01*
*Status: Ideated*
