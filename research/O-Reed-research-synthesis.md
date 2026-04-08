---
title: "O-Reed: Research Synthesis"
created: 2026-04-04
juce_version: "8.0.4"
summary: "Synthesized findings from 3 parallel Level 3 research agents covering reed DSP algorithms, commercial landscape, and instrument acoustics. General-purpose reed wind instrument synthesizer with continuous bore/reed morphing and instrument presets."
domain: dsp
type: research-synthesis
keywords:
  - physical-modeling
  - reed
  - waveguide
  - clarinet
  - saxophone
  - oboe
  - duduk
  - zurna
  - shehnai
  - suona
  - hichiriki
  - o-reed
stages: [0]
agents: [research]
source_documents:
  - research/reed-physical-modeling-dsp.md
  - research/O-Reed-market-research.md
  - research/O-Reed-acoustic-properties-reed-instruments.md
---

# O-Reed: Research Synthesis

## 1. Strategic Direction: General-Purpose Reed Wind Instrument Synthesizer

**O-Reed is a reed wind instrument synthesizer, not a single-instrument emulation.** The engine exposes a continuous parameter space where clarinet, saxophone, oboe, duduk, zurna, hichiriki, and entirely imaginary reed instruments are presets within a unified model -- not separate products or locked modes.

### Core Philosophy

- **Bore geometry is a continuous parameter** -- morph freely between cylindrical (clarinet-like), conical (sax-like), narrow conical (oboe-like), and reverse conical (hichiriki-like)
- **Reed type is parameterized** -- a single "confinement" parameter (Psi) crossfades from single-reed to double-reed character without separate code paths
- **Instrument presets are starting points, not boundaries** -- "Clarinet", "Alto Sax", "Duduk" configure the full parameter set but everything remains tweakable
- **Sound design territory is built in from day one** -- impossible bore geometries, extreme reed parameters, branching bores

### Why General-Purpose Over Instrument-Specific

| Factor | General-Purpose | Instrument-Specific |
|--------|----------------|---------------------|
| Market position | Unique -- no PM reed synth has bore morphing + non-Western instruments | Competes directly with SWAM ($750 bundle) |
| Creative range | Realistic instruments + sound design from one engine | Locked to specific instrument character |
| Development | One engine, many presets | Separate tuning per instrument |
| User appeal | Composers AND sound designers AND world music producers | Primarily orchestral composers |
| Bore morphing | Core feature -- morph between any bore/reed type | Not possible (fixed per instrument) |

### Competitive Positioning

- **SWAM** ($750 for all woodwinds): Best realism for Western instruments, but no non-Western, no sound design, no morphing, separate products per family
- **Respiro** ($165): Experimental PM, but no realistic instrument emulation, small developer
- **GeoShred/Naada** ($250 desktop): Has duduk/shehnai/suona but iOS-first, Mac AU only, no VST3, no Windows
- **Chromaphone** ($99): General PM synth but not wind-specific, no breath controller support
- **Nobody** offers: unified reed engine + non-Western instruments + realistic-to-experimental continuum + modern UI

**Recommended price: $129** -- undercuts SWAM Clarinets ($170) alone while covering ALL reed instruments.

**Competitive window: 12-18 months** before anyone could match this combination.

---

## 2. Architecture: Waveguide Reed Engine

Design for multi-instrument from day one. Each component is parameterized.

```
MIDI/MPE Input
    |
    v
[Mouth Pressure] --> [Reed Model (M-S-D)] --> [Nonlinear Junction] <--> [Bore Waveguide]
        |                    |                        |                       |
   (breath ctrl)        (embouchure)            (Bernoulli flow)      [Tone Hole Lattice]
                                                     |                       |
                                              [Breath Noise Gen]      [Register Hole]
                                                                             |
                                                                      [Bell Radiation]
                                                                             |
                                                                         Output
```

### Component Abstractions

All components are parameterized -- instrument identity emerges from parameter configuration:

**Reed Oscillator:**
- Mass-spring-damper (mu_r, g_r, k_r)
- Rest opening H, channel width w
- Confinement parameter Psi (0 = single reed, >0 = double reed character)
- Embouchure: additional lip damping g_lip, stiffness k_lip, offset x_lip

**Bore Waveguide:**
- Two fractional delay lines (forward/backward traveling waves)
- Bore taper: 0 = cylindrical, increasing = conical
- Multi-segment profile: throat, body, bell
- Viscothermal loss filter (bore-diameter dependent)

**Tone Hole Lattice:**
- 2-4 virtual three-port scattering junctions
- Continuous opening parameter (0.0-1.0) per hole for legato/half-holing
- Register hole for overblowing control

**Bell Radiation:**
- Frequency-dependent reflection filter (lowpass)
- Bell flare parameter controls cutoff

---

## 3. The Reed Model -- Key Design Decisions

### Single vs. Double Reed: One Model, One Parameter

The critical insight from Guillemain (2004): the difference between single and double reed excitation can be captured by a single **confinement loss parameter** (Psi):

```
u(t) = sign(delta_p) * alpha * S_i(t) * sqrt(2 * |delta_p| / (rho * (1 + Psi * alpha^2 * S_i^2 / S_r^2)))
```

- **Psi = 0**: Pure single-reed behavior (clarinet, saxophone). No confinement losses.
- **Psi = 0.1-0.3**: Mild double-reed character (duduk, piri). Some upper harmonic enrichment.
- **Psi = 0.3-0.6**: Strong double-reed character (oboe, bassoon). Nasal quality, bright attack.
- **Psi > 0.6**: Extreme confinement (zurna-like). Piercing, very rich spectrum.

This is O-Reed's equivalent of O-Bowed's friction model tiers -- one continuous control replaces what competitors sell as separate products.

### Reed Dynamics: Full Mass-Spring-Damper

Use the full dynamic reed model (not the simplified static reed):

```cpp
// Reed displacement ODE
float force = (p_mouth - p_bore) * A_reed;
float x_ddot = (force - g_eff * x_dot - k_eff * x) / mu_r;
x_dot += x_ddot / sampleRate;
x += x_dot / sampleRate;
x = max(x, -(H_eff + 0.001f));  // can't go past mouthpiece
```

The cost is modest (~25 ops per sample) and the payoff in transient realism is significant. When the user dials reed mass to near-zero, the model naturally converges on the cheaper static reed behavior.

### Instrument Parameter Presets

| Parameter | Bb Clarinet | Alto Sax | Oboe | Bassoon | Duduk | Zurna | Suona | Hichiriki |
|-----------|-------------|----------|------|---------|-------|-------|-------|-----------|
| Bore taper | 0 (cyl) | ~1.6 deg | ~0.82 deg | ~0.41 deg | 0 (cyl) | ~2.5 deg | ~2.5 deg | Negative (reverse) |
| Bore length | ~60cm | ~65cm | ~32cm | ~250cm | ~35cm | ~30cm | ~35cm | ~18cm |
| Bore diameter | ~15mm | 12-40mm | 4-12mm | 4-20mm | ~12mm | 8-15mm | 10-20mm | 9-13mm |
| Psi (confinement) | 0 | 0 | 0.3-0.6 | 0.2-0.5 | 0.1-0.3 | 0.5+ | 0.4-0.6 | 0.2-0.4 |
| Reed stiffness | Medium | Med-soft | Stiff | Med-stiff | Soft | Stiff | Medium | Medium |
| Rest opening H | ~0.4mm | ~0.7mm | ~0.3mm | ~0.3mm | ~0.2mm | ~0.3mm | ~0.4mm | ~0.4mm |
| Bell flare | Moderate | Large | Small | Minimal | None | Moderate | Large metal | None |
| Overblowing | 12th | Octave | Octave | Octave | 12th* | Octave | Octave | Varies |
| Blowing pressure | 2-5 kPa | 2-8 kPa | 3-7 kPa | 3-6 kPa | 2-4 kPa | 6-10 kPa | 5-9 kPa | 3-6 kPa |

*Duduk: cylindrical bore + double reed is acoustically unusual -- overblows at the 12th like a clarinet despite having a double reed.

---

## 4. The Bore Model -- Cylindrical vs. Conical

### The Critical Acoustic Difference

- **Cylindrical bore** (closed-open pipe): Only odd harmonics. Overblows at the 12th. Hollow, woody quality. Instruments: clarinet, duduk, hichiriki, piri, arghul.
- **Conical bore** (open-open equivalent): All harmonics. Overblows at the octave. Full, bright quality. Instruments: saxophone, oboe, bassoon, zurna, shehnai, suona.

### Implementation Strategy

**Recommended: Start with cylindrical waveguide + conical correction filter, upgrade to true conical sections.**

Three strategies ranked by accuracy/cost:

| Strategy | Quality | CPU | Description |
|----------|---------|-----|-------------|
| A: Fake conical (STK) | Basic | Very low | Variable blow position along cylindrical bore. Crude. |
| B: Cylindrical + correction | Good | Low-medium | Cylindrical waveguide with filter boosting even harmonics. |
| C: True conical sections | Excellent | Medium | Cascaded truncated cone segments with spherical wave scaling. |

Start with B for initial development. The correction filter is derived from the ratio of conical to cylindrical impedance. Upgrade to C when accuracy demands it.

### Bore Morphing Path

```
bore_taper = 0.0:   Pure cylindrical (clarinet character)
bore_taper = 0.4:   Narrow conical (oboe/bassoon character)
bore_taper = 0.8:   Moderate conical (saxophone character)
bore_taper = 1.0:   Wide conical (zurna character)
bore_taper < 0:     Reverse conical (hichiriki character -- unique)
```

All transitions are smooth. The overblowing interval shifts naturally as bore taper changes.

---

## 5. Non-Western Instruments -- The Differentiator

### Zero Competition in Desktop PM

No VST3/AU plugin on any platform offers physically modeled non-Western reed instruments. GeoShred has some on iOS/Mac AU only. Sample libraries exist but can't do continuous pitch bending or authentic ornamentation.

### Key Instruments for Launch

**Priority 1 (must-have):**
- **Duduk** -- Huge cinematic demand (Gladiator, The Last Temptation of Christ). Cylindrical bore + large double reed. Warm, human-voice quality. ~1 octave + 4th range. Apricot wood.
- **Shehnai** -- Indian classical/ceremonial. Conical bore + quadruple reed (4 layers). Powerful, nasal, bright. ~2 octave range.
- **Suona** -- Chinese folk/ceremonial. Conical bore + double reed + metal bell. Extremely loud and shrill. ~2 octave range.

**Priority 2 (v1.1):**
- **Hichiriki** -- Japanese gagaku. Reverse conical bore (unique). Extreme pitch bending (embai). ~1 octave range.
- **Zurna** -- Turkish/Persian outdoor ceremonies. Wide conical bore. Piercing, almost no soft dynamics.
- **Piri** -- Korean court music. Cylindrical + double reed. Significant pitch bending.

**Priority 3 (updates):**
- **Arghul** -- Egyptian drone instrument. Dual cylindrical bores (requires second waveguide).
- **Launeddas** -- Sardinian triple-pipe polyphony. Three independent bores.
- **Mijwiz** -- Levantine dual-pipe. Parallel bores creating natural beating.

### What Makes Each Distinctive (Modeling Notes)

| Instrument | Unique Feature | Model Parameter |
|------------|---------------|-----------------|
| Duduk | Cylindrical + double reed (unusual combo) | bore_taper=0, Psi=0.2, large opening |
| Shehnai | Quadruple reed (4-layer) | Very high Psi, wide channel |
| Suona | Metal bell on wooden body | High bell_radiation, mixed materials param |
| Hichiriki | Reverse conical bore | Negative bore_taper (narrows at bottom) |
| Zurna | No soft dynamics | High min blowing pressure, wide bore angle |
| Piri | Extreme embouchure bending | High pitch_bend_range, low reed stiffness |

---

## 6. Expressive Control Space

### Three-Tier Parameter Hierarchy

**Tier 1 -- Primary (always visible, 5 controls):**
1. **Breath Pressure** -- maps to mouth pressure (p_mouth). Main dynamics control.
2. **Embouchure / Bite** -- maps to lip force. Controls brightness and pitch bending.
3. **Reed Hardness** -- maps to reed stiffness (k_r). Attack character and brightness.
4. **Bore Character** -- maps to bore_taper (0=cylindrical, 1=full cone). Harmonic content.
5. **Instrument Morph** -- macro that crossfades full parameter sets between instrument presets.

**Tier 2 -- Secondary (expandable panel, 5 controls):**
6. Reed Opening (H) -- ease of onset, dynamic range
7. Bell Size -- projection, brightness
8. Air Noise -- breathiness (breath noise mix)
9. Double Reed Amount (Psi) -- single vs double reed character
10. Bore Diameter -- intimacy vs projection

**Tier 3 -- Advanced / Sound Design:**
11. Reed Mass, Reed Damping, Mouthpiece Volume
12. Tone Hole Cutoff, Register Hole position
13. Bore Profile (multi-segment), Bore Length
14. Vocal Coupling (growl), Flutter Rate
15. Subtone Amount, Attack Overshoot

### MIDI / MPE / Breath Controller Mapping

| Physical Input | MIDI Source | Physical Parameter |
|---------------|------------|-------------------|
| Breath | CC2 / MPE Pressure | Mouth pressure |
| Bite | CC1 / MPE Slide (Y) | Lip force / embouchure |
| Lip position | Aftertouch / CC11 | Reed opening offset |
| Tongue attack | Velocity | Attack rate / overshoot |
| Vibrato depth | CC77 or custom | Vibrato LFO depth |
| Growl | CC80 or custom | Vocal fold coupling |
| Pitch bend | Pitch Bend / MPE X | Delay line fine-tuning |

**Breath controller integration is critical.** CC2 maps directly to mouth pressure with configurable curve. When no breath controller detected, velocity sets initial attack and a sustain level for held notes, with CC11 for dynamics.

**MPE is ideal for reed instruments:**
- Strike = tongue articulation
- Pressure = sustained breath
- Slide Y = embouchure/brightness
- Glide X = pitch bend

---

## 7. Extended Techniques

These emerge naturally from the physical model or require minimal additional modeling:

| Technique | How It Works in the Model | Special Code Needed? |
|-----------|--------------------------|---------------------|
| Vibrato | LFO on lip pressure, breath, or throat impedance | Minimal (3 LFO targets) |
| Growl / multiphonics | Second oscillator coupled to mouth pressure | Yes (vocal fold model) |
| Flutter tongue | ~25 Hz modulation of blowing pressure | No (LFO) |
| Subtone | Minimum reed opening > 0, high lip damping | No (parameter range) |
| Slap tongue | Impulse excitation of bore (reed pulled away) | Minor (impulse mode) |
| Key clicks | Noise burst at tone hole locations | Yes (noise injection) |
| Pitch bending | Real-time reed stiffness modulation | No (parameter) |
| Overblowing | Register hole + increased pressure | No (emerges naturally) |
| Circular breathing | Brief pressure dip (~50-100ms) | No (envelope shape) |
| Attack chiff | Pressure overshoot at note onset | No (envelope shape) |

---

## 8. CPU Budget and Polyphony

### Per-Voice Cost at 44.1 kHz (2x oversampling = 88.2 kHz internal)

| Component | Ops/sample |
|-----------|-----------|
| Reed dynamics (2nd order ODE) | ~25 |
| Bernoulli flow + sqrt | ~15 |
| Reed nonlinearity | ~4 |
| Bore delay line (fractional) | ~6 |
| Viscothermal loss filter | ~3 |
| Bell reflection filter | ~3 |
| Tone holes (x4) | ~20 |
| Breath noise + filtering | ~5 |
| Vibrato LFO | ~3 |
| **Total per output sample** | **~168 ops** |

**Practical polyphony: 32-64 voices.** For monophonic use (most common), CPU is a non-issue -- use the budget for higher quality (4x oversampling, more tone holes, true conical bore).

**2x oversampling is the sweet spot.** The reed nonlinearity is relatively mild (unlike virtual analog oscillators). 4x is overkill for polyphonic use but fine for mono.

---

## 9. Implementation Roadmap

### DSP Stage 1 -- Core Engine (Clarinet)
- Cylindrical waveguide with fractional delay
- Static reed table (STK-style, simplest possible)
- Simple bell reflection filter
- Monophonic, breath pressure + embouchure control
- **Goal:** Convincing clarinet-like tone from a waveguide

### DSP Stage 2 -- Reed Dynamics + Conical Bore
- Full mass-spring-damper reed model
- Conical bore correction filter (Strategy B)
- Guillemain confinement parameter (Psi) for double reed
- Breath noise injection
- **Goal:** Distinguish clarinet, sax, oboe, duduk by parameters alone

### DSP Stage 3 -- Expression + Polish
- Tone hole lattice (4 virtual holes + register hole)
- Vibrato (lip, breath, throat sources)
- Attack transient modeling (chiff, tongue articulation)
- Subtone mode, key click/pad noise, growl/vocal coupling
- **Goal:** Expressive, responsive instrument matching SWAM playability

### DSP Stage 4 -- Sound Design Territory
- Impossible bore geometries (reverse taper, branching)
- Reed parameter extremes (negative stiffness, ultra-high mass)
- Dual-bore configurations (arghul/launeddas-like drone)
- Cross-modulation between reed and bore
- **Goal:** Sound design tool beyond acoustic emulation

---

## 10. Key Research References

### Reed Models
- Avanzini & van Walstijn (2004) -- Reed-mouthpiece-lip system, Part I & II (Acta Acustica)
- Guillemain et al. (2005) -- Real-time clarinet synthesis (JASA 118)
- Guillemain (2004) -- Double-reed synthesis model (EURASIP) -- **the Psi parameter**

### Bore Models
- Smith (1986) -- Efficient reed-bore simulation (ICMC)
- Valimaki et al. (1994) -- Truncated cone waveguides (ICMC)
- Scavone & Smith (1996) -- Woodwind tonehole modeling (ICMC)
- Keefe (1981) -- Woodwind tone-hole acoustics (PhD)

### Acoustics
- Maugeais & Dalmont (2024) -- "What makes the duduk special" (Acta Acustica) -- critical for non-Western modeling
- Wolfe (2018) -- Acoustics of woodwind instruments (Acoustics Today)
- Braasch & Cottingham (2023) -- Free reeds comparison (Acoustics Today)

### Recent Advances
- Music et al. (2025) -- Port-Hamiltonian woodwind model (Frontiers in Signal Processing) -- energy-preserving stability

### Implementations
- STK (Cook & Scavone) -- BlowHole, Saxofony classes
- Faust pm.lib -- Modular building blocks
- Scavone MATLAB Waveguide Toolkit (2024) -- Latest academic bore modeling

### Source Documents
- `research/reed-physical-modeling-dsp.md` -- Full DSP algorithms, code snippets, CPU analysis
- `research/O-Reed-market-research.md` -- Commercial landscape, gap analysis, pricing
- `research/O-Reed-acoustic-properties-reed-instruments.md` -- Instrument dimensions, comparison tables, extended techniques, material debate
