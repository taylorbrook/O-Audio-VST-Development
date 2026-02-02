# Multi-Stage Decay Envelopes for Bell/Metallic Sounds

## Research Summary

Professional synthesizers implement multi-stage decay for bells and metallic sounds through various approaches: physical modeling with material-based damping, per-partial decay control, and frequency-dependent decay shaping.

---

## Comparison Table

| Synthesizer | Approach | Decay Stages/Modes | Key Decay Parameters | Per-Partial Control | Frequency-Dependent Decay |
|-------------|----------|-------------------|---------------------|---------------------|---------------------------|
| **Ableton Collision** | Physical Modeling (Mallet+Resonators) | Single continuous with damping | Decay (0-100%), Material, Note Off (0-100%) | Indirect via Material | Yes - Material slider controls damping variation across frequencies |
| **Native Instruments Kontakt** | Sampling + AHDSR/Flexible Envelopes | 4-stage ADSR or 32-point Flexible | Attack, Decay, Sustain, Release + flexible breakpoints | Via scripting/groups | Via filter modulation |
| **Arturia Pigments 6** | Modal Synthesis | Continuous per-mode decay | Decay, Brilliance, Damping, Timbre | Yes - each mode has own decay rate | Brilliance controls high-frequency decay |
| **AAS Chromaphone 3** | Physical Modeling (Dual Resonator) | Continuous with material-based variation | Decay, Release (% of decay), Material, Tone | Yes - via Material parameter | Material adjusts decay per frequency range |
| **Modartt Pianoteq** | Physical Modeling (Soundboard) | ADSR-like (Syngular) or physical | Impedance, Cutoff, Q-Factor (Slope), Attack, Decay, Release | Yes (Pro version: per-note) | Cutoff+Slope control high-freq decay rate |

---

## Detailed Parameter Breakdown

### 1. Ableton Collision (Co-developed with AAS)

**Resonator Types:** Beam, Marimba, String, Membrane, Plate, Pipe, Tube

| Parameter | Range | Function |
|-----------|-------|----------|
| **Decay** | 0-100% | Internal damping of resonator |
| **Material** | Continuous | Frequency-dependent damping variation. Lower values = low-freq sustain (wood/rubber). Higher = high-freq sustain (glass/metal) |
| **Note Off** | 0-100% | Damping response to MIDI Note Off. 0% = natural decay. 100% = immediate mute |
| **Inharm** | -100 to +100 | Partial frequency stretch. Negative = compressed. Positive = stretched (bell-like) |
| **Radius** | (Pipe/Tube only) | Affects decay time and pitch |
| **Hit** | 0-100% | Strike location affects partial excitation |
| **Brightness** | Continuous | Frequency component amplitude balance |

**Key Insight:** Collision uses a single decay parameter but achieves multi-stage-like behavior through the Material slider, which creates different decay rates for low vs. high frequencies.

**Source:** [Ableton Collision](https://www.ableton.com/en/packs/collision/), [Live Instrument Reference](https://www.ableton.com/en/manual/live-instrument-reference/)

---

### 2. Native Instruments Kontakt

**Envelope Types:** AHDSR, DBD (Decay-Breakpoint-Decay), 32-point Flexible

| Parameter | Range | Function |
|-----------|-------|----------|
| **Attack** | 0ms to seconds | Fade-in time |
| **Hold** | 0ms to seconds | Time at peak before decay |
| **Decay** | 0ms to seconds | Time to reach sustain level |
| **Sustain** | 0-100% | Held level during note |
| **Release** | 0ms to seconds | Fade-out after note-off |
| **Flexible Envelope** | Up to 32 breakpoints | Arbitrary modulation shapes with modulatable positions |

**Symphony Series Percussion Parameters:**
- Attack: Controls transient hardness/fade-in
- Release: Additional envelope to fade-out phase

**Multi-Stage Implementation:** Via:
1. Groups with different envelopes per sample layer
2. Flexible Envelope with arbitrary breakpoint curves
3. Scripting (KSP) for complex decay behaviors
4. Full decay samples (no looping) for natural ring-out

**Source:** [Kontakt Modulation](https://www.native-instruments.com/ni-tech-manuals/kontakt-manual/en/modulation), [Symphony Series Percussion](https://www.native-instruments.com/en/products/komplete/cinematic/symphony-series-percussion/)

---

### 3. Arturia Pigments 6 (Modal Engine)

**Resonator Types:** String, Beam, Partials

| Parameter | Range | Function |
|-----------|-------|----------|
| **Decay** | 0-100% | Mode decay rate (0=instant, 100=infinite) |
| **Brilliance** | 0-100% | High-frequency decay rate |
| **Damping** | Continuous | Overall resonance damping |
| **Timbre** | Pure/Pinch/Hollow/Nylon/Full/Bass | Modal characteristic preset |
| **Warp** | Continuous | Harmonic/inharmonic morphing |
| **Partials** | Low/Medium/High/Full | Mode density (affects complexity) |

**Modulation Sources:**
- 3x Envelopes (ADSR-type)
- 3x Functions (tempo-sync multi-point envelopes)
- Envelope Follower (audio-reactive)
- 3x LFOs, 3x Random sources

**Key Insight:** Modal synthesis inherently creates per-partial decay. Each mode (partial) decays exponentially at its own rate. The Decay and Brilliance parameters control the overall rate and frequency-dependent variation.

**Source:** [Arturia Pigments 6 Review (CDM)](https://cdm.link/pigments-6-review/), [Pigments 6 Review (SynthAnatomy)](https://synthanatomy.com/2025/01/arturia-pigments-6-review-and-sound-demo-a-deep-dive-into-physical-modeling-synthesis-and-filtering.html)

---

### 4. AAS Chromaphone 3

**Resonator Types:** String, Beam, Marimba Bar, Plate, Membrane, Drumhead, Open Tube, Closed Tube, Manual

| Parameter | Range | Function |
|-----------|-------|----------|
| **Decay** | Continuous | Partial decay time (modulatable by note/velocity) |
| **Release** | Percentage | Release as % of total decay time (simulates dampers) |
| **Material** | Negative to Positive | Frequency-dependent decay. Negative = low freq sustain. Positive = high freq sustain |
| **Tone** | Negative to Positive | Partial amplitude balance (dB/octave) |
| **Hit Position** | 0-100% | Affects partial excitation pattern |
| **Mode Density** | Low(4)/Medium(16)/High(30)/Full(70) | Number of calculated partials |
| **Low Cut** | Frequency | -24 dB/octave highpass |
| **Radius** | (Tube only) | Affects decay and frequency response |

**Envelope for Noise Source:**
- Switchable ADSR or AHD mode
- Controls noise amplitude, frequency, density

**Resonator Coupling:**
- Parallel mode: Simple mix of two resonators
- Coupled mode: Bidirectional energy flow modeling

**Key Insight:** Material parameter is crucial - it creates frequency-dependent decay similar to real-world materials. Metal = longer high-freq decay. Wood = longer low-freq decay.

**Source:** [Chromaphone 3 Manual](https://www.applied-acoustics.com/chromaphone-3/manual/), [Chromaphone 3 Review (SynthAndSoftware)](https://synthandsoftware.com/2021/03/exclusive-synth-and-software-review-applied-acoustic-systems-chromaphone-3/)

---

### 5. Modartt Pianoteq (Including Bells/Mallet Instruments)

**Instruments:** Celesta, Vibraphone, Marimba, Xylophone, Steel Pans, Church Bells, Tubular Bells, Cimbalom

| Parameter | Range | Function |
|-----------|-------|----------|
| **Impedance** | 2D Control | Soundboard resistance to vibration. Higher = longer sustain |
| **Cutoff** | Frequency | Highest frequency soundboard carries |
| **Q-Factor/Slope** | Continuous | Rate of high-frequency reduction |
| **String Length** | Continuous | Affects inharmonicity (shorter = more bell-like) |
| **Harmonicity** | Continuous | Lower values = bell-like character |

**Syngular Engine (Pianoteq 9):**
- Attack, Decay, Release envelope controls
- Cutoff and Slope for frequency-response shaping during decay

**Mallet Bounce Controls:**
- Initial Delay
- Velocity Sensitivity
- Delay Loss (between bounces)
- Velocity Loss
- Attack Envelope

**Pro Version Per-Note Adjustments:**
- Volume, Detune, Attack envelope
- Hammer hardness, Soundboard
- String length, Sympathetic resonance
- Duplex scale resonance

**Key Insight:** Impedance/Cutoff/Slope system provides physics-based control over decay. Unlike traditional ADSR, these control how energy transfers from strings to soundboard, naturally creating frequency-dependent decay curves.

**Source:** [Pianoteq User Manual](https://www.modartt.com/user_manual?product=pianoteq&lang=en), [Pianoteq 7 Review (SOS)](https://www.soundonsound.com/reviews/modartt-pianoteq-7)

---

## UI Design Patterns

### Common Approaches

| Pattern | Used By | Description |
|---------|---------|-------------|
| **X-Y Pad** | Collision, Chromaphone | 2D control for Decay vs Material/Brightness |
| **Material Slider** | Collision, Chromaphone | Single control affecting frequency-dependent decay |
| **Decay + Brilliance** | Pigments | Separate controls for overall decay and high-freq decay |
| **Impedance 2D Control** | Pianoteq | Position controls decay, slope controls frequency response |
| **Multi-segment Envelope** | Kontakt, Pigments (Functions) | Visual envelope editor with draggable breakpoints |
| **Preset Modes** | Chromaphone (Mode Density), Pianoteq | Quick access to complexity levels |

### Naming Conventions

| Concept | Collision | Chromaphone | Pigments | Pianoteq |
|---------|-----------|-------------|----------|----------|
| Overall decay | Decay | Decay | Decay | Impedance |
| Freq-dependent | Material | Material | Brilliance | Cutoff + Q/Slope |
| Damping control | Note Off | Release (%) | Damping | Sustained notes damping |
| Mode count | N/A | Mode Density | Partials | N/A |

---

## Bell Sound: Three Decay Phases

Research from [Sound On Sound - Synthesizing Bells](https://www.soundonsound.com/techniques/synthesizing-bells) identifies three distinct phases in bell sounds:

1. **Strike Phase** (~0-50ms)
   - Inharmonic impact sound
   - Dies away quickly
   - Contains noise/transient components

2. **Strike Note Phase** (~50ms-2s)
   - Dominated by strong, low harmonics
   - Main perceived pitch
   - Rapid exponential decay

3. **Hum Tone Phase** (2s+)
   - Sub-harmonic an octave below fundamental
   - Lingers longest
   - Very slow decay

**Implementation in O-Bells:**

Current O-Bells (v1.1.1) implements this via:
- `DECAY_MULTIPLIERS[8]` = {1.2, 1.0, 0.85, 0.7, 0.6, 0.5, 0.4, 0.3} - Higher partials decay faster
- Material decay multipliers: Bronze (1.0x), Steel (1.4x), Glass (2.5x), Crystal (5.0x)
- Decay Shape parameter: Linear / Exponential / Multi-stage

---

## Recommendations for O-Bells Enhancement

Based on this research, potential improvements:

### 1. Material-Based Frequency-Dependent Decay
Like Collision/Chromaphone - add a "Material Damping" parameter that shifts decay balance between low and high partials.

### 2. Separate Decay Phases
Implement distinct:
- **Strike Decay** (fast, ~10-50ms)
- **Body Decay** (medium, ~0.5-3s)
- **Hum Decay** (slow, ~2-10s)

### 3. Brilliance/Cutoff Control
Like Pigments/Pianoteq - separate control for high-frequency decay rate vs overall decay.

### 4. Mode Density Control
Like Chromaphone - user-selectable partial count (4/8/16/32) for CPU/quality tradeoff.

### 5. Release Envelope
Like Chromaphone - Release as percentage of decay time for damping simulation.

---

## Sources

- [Ableton Collision](https://www.ableton.com/en/packs/collision/)
- [Ableton Live Instrument Reference](https://www.ableton.com/en/manual/live-instrument-reference/)
- [Chromaphone 3 Manual](https://www.applied-acoustics.com/chromaphone-3/manual/)
- [Chromaphone 3 Review](https://synthandsoftware.com/2021/03/exclusive-synth-and-software-review-applied-acoustic-systems-chromaphone-3/)
- [Arturia Pigments 6 Review (CDM)](https://cdm.link/pigments-6-review/)
- [Arturia Pigments 6 Review (SynthAnatomy)](https://synthanatomy.com/2025/01/arturia-pigments-6-review-and-sound-demo-a-deep-dive-into-physical-modeling-synthesis-and-filtering.html)
- [Pianoteq User Manual](https://www.modartt.com/user_manual?product=pianoteq&lang=en)
- [Pianoteq 7 Review (Sound On Sound)](https://www.soundonsound.com/reviews/modartt-pianoteq-7)
- [Sound On Sound - Synthesizing Bells](https://www.soundonsound.com/techniques/synthesizing-bells)
- [Native Instruments Kontakt Modulation](https://www.native-instruments.com/ni-tech-manuals/kontakt-manual/en/modulation)
- [Symphony Series Percussion](https://www.native-instruments.com/en/products/komplete/cinematic/symphony-series-percussion/)
- [Risset's Bell - Additive Synthesis](http://msp.ucsd.edu/techniques/v0.11/book-html/node71.html)
- [Native Instruments - What is Additive Synthesis](https://blog.native-instruments.com/what-is-additive-synthesis/)
