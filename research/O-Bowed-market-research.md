# O-Bowed: Bowed String Synthesis Market Research

> Generated 2026-04-04 | Pre-development landscape survey

---

## 1. Product Comparison Table

| Product | Developer | Type | Price | Poly | MPE | Bow PM | Sections | Body Model | CPU |
|---------|-----------|------|-------|------|-----|--------|----------|------------|-----|
| SWAM Solo Strings | Audio Modeling | Pure PM | $99/inst, $339 bundle | Mono | Yes | Yes | No (separate product) | Convolution | High |
| SWAM String Sections | Audio Modeling | Pure PM | $499 | Poly (sections) | Yes | Yes | Yes (4-16 players) | Convolution + Room | Very High |
| Soliste | Expressive E | Pure PM | $99/inst, $299 suite | Mono | Yes (native) | Yes | No | Unknown | Moderate |
| String Studio VS-3 | AAS | PM Synth | ~$88-199 | 2-voice multi | No | Partial (BSO exciter) | No | Resonator | Low-Moderate |
| Preparation 2 | Physical Audio | PM Synth | ~$99 | 7-voice | Yes | Yes (bow exciter) | No | Dual string collision | Moderate |
| Sakura | Image-Line | PM Synth | ~$89 | Poly | No | Yes (bowed mode) | No | 8-resonator body | Low |
| Augmented Strings | Arturia | Hybrid (sample+synth) | $199 | Poly | No | No | Sort of (layered) | Sampled IR | Low |
| Session Strings Pro 2 | Native Instruments | Sampled | $299-499 | Poly | No | N/A | Yes | Sampled | Low |
| Arche Collection | Expressive E | PM (discontinued successor: Soliste) | Legacy | Mono | Yes | Yes | No | Yes | Moderate |

## 2. Deep Analysis of Key Competitors

### 2.1 Audio Modeling SWAM Strings (Gold Standard)

**Technology:** SWAM = Synchronous Waves Acoustic Modeling. Combines digital waveguide synthesis with "behavioral modeling" -- a proprietary approach that goes beyond pure physics to model how *players* interact with instruments. Three-component architecture:
- **Exciter:** Proprietary bow friction algorithms (not a simple STK-style bow table)
- **Resonator:** Feedback delay lines (waveguides) for string vibration
- **Radiator:** Convolution-based body/soundboard modeling

**Key Parameters Exposed:**
- Expression (CC11), Bow Pressure, Bow Position (distance from bridge)
- Vibrato Rate (Hz), Vibrato Depth, Vibrato Random
- Random Bow (randomizes pressure/speed/position subtly)
- Portamento Max Time, Portamento Threshold
- Harmonic content / fingering position
- String selection (for multi-string instruments)

**Articulations:**
- Detache (sustain pedal), Spiccato, Flautando, Tremolo (sync/unsync)
- Harmonics, Pizzicato, Portamento/Glissando
- Missing: col legno
- Portamento accidentally triggers during legato (common complaint)

**What Users Love:**
- Unmatched real-time expressiveness with breath/wind controllers
- Tiny disk footprint (~430MB total vs 50-100GB for sample libraries)
- ~350MB RAM per instance
- Genuine feeling of "performing" rather than "programming"
- Room simulator (Ambiente) for ensemble placement
- Screen reader accessible

**What Users Hate:**
- CPU hungry -- multiple instances strain even modern systems (need 2.5GHz 4-core minimum per section)
- Steep learning curve -- requires practice like a real instrument
- "Overwhelming mids" -- sound focuses on strings, lacks wooden body resonance
- Violins can sound shrill in high registers without careful parameter management
- Portamento control is fiddly
- Demands good MIDI controller for best results

**Market Position:** Premium, professional. The standard against which all PM string instruments are measured. Pricing: $99 per solo instrument, $339 for solo bundle, $699 for complete strings bundle, $1699 for full orchestra.

### 2.2 Expressive E Soliste (Newest Competitor, June 2025)

**Technology:** Pure physical modeling, zero samples. Designed ground-up for MPE controllers (Osmose). Four instruments: VLN 356, VLA 419, CLO 759, DBS 1130.

**Playing Modes:**
- **Classic Mode:** Press & tap -- tap triggers bow stroke, pressure controls sustained dynamic
- **Virtuoso Mode:** Smart back-and-forth bowing with direct real-time bow control

**Articulations:** Sul ponticello, pizzicato, Bartok pizz, staccato, real-time tremolo, all customizable per-preset.

**Sound Quality (Sound on Sound review):** "Beguiling and often ravishingly beautiful" but with "a certain quite strong synthetic quality" -- won't fool listeners in exposed solo passages. Reminiscent of Yamaha VL-series character. "Harmonically complex." Sound becomes "quite flute- or tube-like in some tessitura."

**Key Limitations:**
- No double-stopping
- Articulation presets selected by mouse only (no keyswitches)
- Preset system incomplete (doesn't recall vibrato/effects settings)
- No manual at launch
- Compact (~400MB per instrument)

**Reviewer Quote:** "Modellers are play, samplers are work."

**Market Position:** Direct SWAM competitor, positioned specifically for MPE/Osmose users. $99/instrument, $299 suite.

### 2.3 Applied Acoustics String Studio VS-3

**Technology:** String oscillator synthesizer -- replaces traditional VCOs with physically modeled string excitation. Three oscillator types:
- **BSO** (Bowed String Oscillator): Bow friction model with force and friction knobs
- **HSO** (Hammered String Oscillator)
- **PSO** (Plucked String Oscillator)

**Architecture:** Multitimbral (2 timbres, stacked or split). 800+ presets. Multi-effects: reverb, EQ, compressor, guitar amp.

**Bow Implementation:** The BSO is a creative synthesis tool, not a realistic violin emulator. Controls: bow force, friction amount. Capable of "wonderful ethereal pads" and sounds "you'd achieve elsewhere."

**Market Position:** Creative synthesis tool, not realistic instrument emulation. ~$88-199. Targets sound designers more than orchestral composers.

### 2.4 Physical Audio Preparation 2

**Technology:** Mathematical modeling of colliding strings, rattles, and dynamic frets. Bow is one of three exciter types (pluck, bow, sidechain audio).

**Bow Controls:** Force, Position, Mic Position. Bow force mapped to pitch bend, position to mod wheel. MPE compatible.

**Sound Character:** Experimental/abstract. "From lovely plucky marimbas to doomsday industrial drones." Not targeting realistic bowed string sounds.

### 2.5 Sampling-Based Context (Why PM Matters)

**Session Strings Pro 2 (NI):** 8 violins, 6 violas, 4 celli, 4 double basses. 29 articulations. Round-robin and alternating bow strokes. Pristine recorded quality.

**Where Samples Excel:** Timbral authenticity, ensemble blend, low CPU, instant gratification.

**Where Samples Fail:**
- Machine-gun effect despite round-robin mitigation
- Inflexible expression -- crossfade layers approximate dynamics but lack continuous control
- Legato transitions are pre-recorded, limiting musical freedom
- Huge disk footprint (50-100GB)
- Cannot create articulations that weren't sampled
- No real-time interaction with bow physics

---

## 3. Open-Source Implementations

### 3.1 STK Bowed Class (Perry Cook / Gary Scavone)

**Source:** `github.com/thestk/stk` -- `src/Bowed.cpp`

**Architecture:**
- Two delay lines: `neckDelay_` (bow-to-nut) and `bridgeDelay_` (bow-to-bridge)
- Position ratio `betaRatio_` splits total string length between the two delays
- `BowTable` nonlinearity: static memoryless map from differential velocity (bow - string) to reflection coefficient
  - Slope parameter: `5.0 - (4.0 * normalizedPressure)` (range 3.0-5.0)
  - Offset: 0.001
- Single-pole `stringFilter_` (pole ~0.75) for string losses
- Six cascaded BiQuad body filters (coefficients by Esteban Maestre)
- Sine vibrato at ~6.1 Hz baseline
- ADSR envelope

**Control Parameters:** CC2 (bow pressure), CC4 (bow position), CC11 (vibrato freq), CC1 (vibrato gain), CC128 (aftertouch).

**Known Limitations:**
- Simple "hyperbolic" friction model -- no thermal/elasto-plastic dynamics
- No rosin temperature modeling
- Body resonance is static BiQuad chain, not a measured/convolved body
- No sympathetic string resonance
- No bow hair dynamics or finite-width bow modeling
- Produces recognizably "digital waveguide" sound
- Patent considerations (Stanford/Yamaha waveguide patents, now largely expired)

**Value for O-Bowed:** Excellent starting reference architecture. The delay-line topology is proven and efficient. Needs significant enhancement in friction model, body modeling, and expressiveness to compete commercially.

### 3.2 FAUST Physical Modeling Library

**Source:** `github.com/grame-cncm/faustlibraries` -- `physmodels.lib`

**Bowed String Components:**
| Function | Purpose | Parameters |
|----------|---------|------------|
| `bowTable` | Generic bow friction table | offset, slope |
| `violinBowTable` | Violin-specific bow | bowPressure (0-1) |
| `bowInteraction` | Bidirectional bow-string coupling | bowTable function |
| `violinBow` | Complete bow block | bowPressure, bowVelocity (0-1) |
| `violinBowedString` | Full bowed string segment | stringLength (m), bowPressure, bowVelocity, bowPosition (0-1) |
| `violinNuts` | String termination (nut end) | -- |
| `violinBridge` | Bridge damping/coupling | -- |
| `violinBody` | Resonant lowpass body filter | -- |
| `violinModel` | Complete single-string instrument | stringLength, bowPressure, bowVelocity, bowPosition |
| `violin_ui_MIDI` | MIDI-controllable version | -- |

**Architecture:** Modular bidirectional chain system: `nuts : bowedString : bridge : body`. All waveguide-based. Components can be freely combined.

**Value for O-Bowed:** Excellent reference for modular architecture design. The chain-based approach (nuts -> string -> bridge -> body) maps cleanly to a JUCE implementation where each stage is a DSP processor class.

### 3.3 Csound `wgbow` Opcode

**Based on:** Perry Cook's waveguide model, re-coded for Csound.

**Key Parameters:**
- `kpres` (bow pressure): Varies harmonic emphasis. Lower pressure weakens fundamental, emphasizes overtones. At very low pressure, tone production collapses entirely -- this boundary is musically interesting.
- `krat` (bow position): Different positions accentuate/attenuate different partials.
- Vibrato frequency and amplitude.

**Value for O-Bowed:** Demonstrates that the Cook/Smith model is portable across platforms. The pressure-collapse boundary behavior is worth preserving as a sound design feature.

### 3.4 Tao Synth

**Source:** `github.com/lucasw/tao_synth`

Finite-element approach to elastic material modeling. Includes Bow, Hammer, Connector devices. More of a research tool than a real-time instrument. C++ API available. Interesting for understanding alternative approaches to waveguide modeling.

---

## 4. State of the Art -- Academic/Research

### 4.1 Friction Models (Progression)

1. **Hyperbolic/Static (Smith 1986, Cook/STK):** Memoryless bow table. Differential velocity -> reflection coefficient. Simple, efficient, recognizably synthetic.

2. **Elasto-Plastic (Dupont et al.):** Models stick-slip with pre-sliding displacement. More realistic transient behavior. Used in some academic bowed string models.

3. **Thermal/Rosin Temperature (Woodhouse 2003, Maestre 2014):** Models rosin softening from friction heat. Temperature affects friction coefficient dynamically. More realistic sustained tone evolution and attack transients.

4. **Finite-Width Bow + Hair Dynamics (Desvages 2018, Edinburgh):** Models the ribbon of bow hair rather than a point contact. Produces more complex, realistic friction patterns. Computationally expensive.

### 4.2 Neural/ML Approaches

**DDSP (Differentiable Digital Signal Processing):**
- Combines classical DSP elements (filters, oscillators, reverb) with deep learning
- Dramatically reduced parameter counts vs pure neural approaches
- Enables real-time and embedded deployments
- DDSP-VST demonstrates feasibility of real-time neural synthesis in plugin format
- Open problems: robustness to heterogeneous data, non-harmonic/polyphonic signals, efficient implementation

**Physics-Informed Neural Networks (PINNs):**
- Recent work applying PINNs to model nonlinear bow force scenarios
- Physics-Informed Deep Operator Networks (PI-DeepONets) perform well under low bow forces
- Still largely academic, not yet deployed in commercial plugins

**Differentiable Modal Synthesis (2024):**
- Novel model integrating modal synthesis + spectral modeling in neural framework
- Physical properties and fundamental frequencies as inputs
- Outputs string states across time and space
- Solves PDE characterizing nonlinear string behavior

**Key Takeaway:** Neural approaches are promising for parameter estimation and hybrid models, but pure PM remains the practical choice for real-time bowed string plugins in 2026. The sweet spot may be using ML to *train* friction model parameters from recorded data, then running classical PM in real-time.

### 4.3 Key Academic References

- Smith, J.O. (1986) -- Original digital waveguide bowed string
- McIntyre, Schumacher, Woodhouse (1983) -- Foundational bow-string interaction model
- Woodhouse (2003) -- Thermal friction model for bowed strings
- Maestre et al. (2014) -- Digital waveguide implementation of thermal friction
- Desvages (2018, Edinburgh PhD) -- Finite-width bow, hair dynamics, comprehensive bowed string PM
- Percival -- "Physical modelling meets machine learning: performing music with a virtual string ensemble"
- Engel et al. (2020) -- DDSP: Differentiable Digital Signal Processing

---

## 5. UI/UX Patterns and Best Practices

### 5.1 MIDI Mapping Conventions

| Parameter | Standard CC | Notes |
|-----------|------------|-------|
| Expression/Dynamics | CC11 | Universal |
| Modulation/Vibrato | CC1 | Standard mod wheel |
| Bow Pressure | CC2 (breath) or aftertouch | SWAM default |
| Bow Position | CC4 (foot) or CC74 (MPE Y) | Bridge-to-fingerboard |
| Vibrato Rate | Varies | Often internal LFO |
| Vibrato Depth | CC1 or dedicated | |
| Articulation | Keyswitches or CC | Soliste uses preset selection (criticized) |

### 5.2 MPE Mapping (Emerging Standard)

| MPE Dimension | Bowed String Mapping | Controller |
|---------------|---------------------|------------|
| Note-on velocity | Attack intensity / bow speed | Key strike |
| Polyphonic aftertouch (Z) | Bow pressure / dynamics | Key pressure |
| Slide (Y / CC74) | Bow position (sul tasto <-> sul ponticello) | Key Y-axis |
| Pitch bend (X) | Pitch / vibrato | Key X-axis |
| Channel pressure | Global expression | Aggregate |

### 5.3 Controller Ecosystem

- **Breath controllers** (TEControl, WARBL): Natural mapping to bow pressure -- inhale/exhale = down/up bow
- **Touche (Expressive E):** Pad pressure/tilt for bow dynamics
- **Osmose (Expressive E):** Full MPE -- designed specifically for instruments like Soliste
- **Linnstrument/Sensel Morph/Roli:** X/Y/Z per note
- **Ribbon controllers:** Continuous position for bow placement

### 5.4 What Makes Bowed Strings "Playable" vs "Programmatic"

**Playable traits (from SWAM/Soliste user feedback):**
- Continuous, real-time control over dynamics (not velocity layers)
- Bow noise/rosin texture that responds to pressure changes
- Natural vibrato with randomization (not perfect sine LFO)
- Attack transients that vary with bow speed and pressure
- Portamento that responds to playing speed
- Imperfections: slight pitch instability during bow changes, bow bounce

**Programmatic pitfalls:**
- Quantized velocity layers (machine-gun effect)
- Fixed vibrato rate/depth
- Identical attack transients
- No response to continuous controllers
- Keyswitched articulations with audible transitions

---

## 6. Non-Western Bowed Instruments Market

### 6.1 Current Offerings

| Instrument | Products Available | Approach | Gap? |
|------------|-------------------|----------|------|
| Erhu (Chinese) | Embertone Chang Erhu, Sound Magic Neo Erhu, Ample ACEH, Kong Audio Mini Erhu | All sample-based | **No PM erhu exists** |
| Sarangi (Indian) | Very few, mostly sample packs | Sampled | **Major gap** |
| Morin Khuur (Mongolian) | Virtually none | -- | **Total gap** |
| Kamancheh (Persian) | EastWest Silk Road (sampled) | Sampled | **No PM exists** |
| Rebab (Middle Eastern) | Very few | Sampled | **Major gap** |
| Nyckelharpa (Swedish) | None significant | -- | **Total gap** |
| Gadulka (Bulgarian) | None | -- | **Total gap** |

### 6.2 Opportunity

Every existing non-Western bowed string instrument plugin is sample-based. A physically modeled approach could offer:
- Tuning system flexibility (non-12TET scales, maqam, raga)
- Unique playing techniques not captured in sample sessions
- Authentic ornamental control (slides, microtonal bends)
- Tiny footprint vs multi-GB sample libraries

---

## 7. Market Gap Analysis

### 7.1 Price Point Landscape

| Tier | Price | Products | Positioning |
|------|-------|----------|-------------|
| Free/Budget | $0-50 | STK (open source), Csound, FAUST, Boing2 (free) | Educational/experimental |
| Mid-Range | $50-199 | String Studio VS-3 (~$88), Sakura (~$89), Soliste individual ($99), SWAM individual ($99), Preparation 2 (~$99) | Single instrument or creative tool |
| Premium | $200-500 | Soliste Suite ($299), SWAM Solo Bundle ($339), SWAM Sections ($499), Augmented Strings ($199) | Professional bundles |
| High-End | $500+ | SWAM Strings Bundle ($699), SWAM Orchestra ($1699) | Complete orchestral solution |

### 7.2 Identified Market Gaps

1. **Affordable, accessible PM bowed strings** -- SWAM is the gold standard but expensive and has a steep learning curve. Soliste is new but MPE-centric. Nothing sits in the $49-79 range offering realistic, easy-to-use PM bowed strings with sensible defaults.

2. **Non-Western bowed instruments via PM** -- Zero competition. An erhu or sarangi model would be completely unique.

3. **Hybrid PM + creative synthesis** -- String Studio VS-3 does this for creative sound design, but no product combines realistic bowed string PM with granular/spectral/wavetable layers in a single instrument (the way Arturia does with samples).

4. **Sound design-oriented bowed strings** -- Preparation 2 is experimental but niche. A product that starts from realistic bowed string PM but extends into sound design territory (extended techniques, impossible physics, resonance manipulation) could serve film/game composers.

5. **Low-CPU PM strings** -- SWAM's CPU appetite limits orchestral use. An implementation optimized for efficiency (simpler body model, streamlined friction) at the cost of some realism could serve users running large templates.

6. **PM strings with built-in ensemble intelligence** -- SWAM Sections handles this but at $499. Automatic divisi, humanized unison, section size control in a streamlined package.

7. **Body/resonance as a creative parameter** -- No existing product exposes the body resonance model as a deeply tweakable parameter. What if you could morph between violin, viola, cello, and completely synthetic bodies?

### 7.3 Where O-Bowed Could Differentiate

**Recommended Positioning: "Expressive PM Bowed Strings for Composers and Sound Designers"**

| Differentiation | How |
|----------------|-----|
| **Body morphing** | Continuous morph between instrument body types (violin -> viola -> cello -> synthetic) |
| **Non-Western modes** | Erhu/sarangi/kamancheh body + tuning presets from day one |
| **Sound design extensions** | "Impossible physics" mode: infinite bow, reversed friction, sub-harmonic bowing |
| **Accessible defaults** | Ship with presets that sound good immediately via keyboard, not just with breath controllers |
| **Modern UI** | WebView-based visualization of bow-string interaction, body resonance spectrum |
| **CPU efficiency** | Target 50% of SWAM's CPU for comparable quality through optimized body modeling |
| **Price point** | $49-79 for solo instrument, $149-199 for full bundle (undercut SWAM/Soliste) |

---

## 8. Technical Architecture Recommendations

### 8.1 Signal Flow (Based on Literature Review)

```
[Bow Exciter] -> [Nonlinear Junction] <-> [String Waveguide] <-> [Bridge/Body]
                       |                        |
                  Friction Model          Delay Lines (2 segments)
                  (Thermal or              Nut termination
                   Elasto-plastic)          Loss filters
```

### 8.2 Recommended Component Architecture

1. **Friction Model:** Start with enhanced hyperbolic (STK-style) but add thermal dynamics (Woodhouse 2003). The rosin temperature model adds significant realism at modest CPU cost. Elasto-plastic can be a "quality mode" option.

2. **String Waveguide:** Dual delay line (nut-to-bow, bow-to-bridge) with fractional delay interpolation. Allpass for tuning correction. Frequency-dependent loss filter (not just a single pole).

3. **Body Resonance:** Three options, selectable:
   - Measured IRs (convolution) -- most realistic, moderate CPU
   - Modal synthesis (resonant filter bank) -- lightweight, morphable between body types
   - Hybrid: modal for low-frequency modes + short IR for high-frequency detail

4. **Sympathetic Resonance:** Model open string sympathetic vibration via coupled waveguides. Subtle but critical for realism.

5. **Bow Noise:** Separate noise generator modulated by bow pressure/speed. Adds the "scratch" and "rosin" texture that pure waveguide models lack.

### 8.3 Key DSP Considerations

- **Sample rate:** 44.1/48kHz is fine for most bowed string content. The fundamental rarely exceeds 4kHz even for violin.
- **Oversampling:** The nonlinear friction junction benefits from 2x oversampling to avoid aliasing artifacts.
- **Latency:** Zero algorithmic latency possible (waveguide is causal). Only buffer latency.
- **Polyphony:** Monophonic per string is authentic, but enable 2-4 voice polyphony for double/triple stops.

---

## 9. Key Takeaways for O-Bowed Design

1. **SWAM is beatable.** Users consistently complain about CPU usage, steep learning curve, shrill highs, and lack of body resonance. These are solvable problems.

2. **Soliste validates the market.** A brand-new competitor entering in 2025 proves demand exists for PM bowed strings beyond SWAM.

3. **Body resonance is the weak link in existing PM.** SWAM users specifically cite "overwhelming mids" and missing wooden body character. Investing in body modeling pays disproportionate dividends.

4. **Playability trumps realism.** The Sound on Sound reviewer nailed it: "modellers are play, samplers are work." Prioritize real-time responsiveness and musical feel over acoustic accuracy.

5. **Start with solo instruments.** Every competitor started with solo strings, not sections. One excellent violin/cello is worth more than four mediocre instruments.

6. **The friction model matters more than the waveguide.** All implementations use essentially the same delay-line topology (Smith 1986). The differentiator is the friction/excitation model quality.

7. **Non-Western bowed strings are an uncontested blue ocean.** Zero PM implementations exist for erhu, sarangi, kamancheh, etc.

8. **MPE support is table stakes for 2026.** Both SWAM and Soliste support it. Any new entrant must.

9. **WebView UI is an opportunity.** No existing PM string plugin has a modern, visually engaging interface. A real-time bow-string visualization would be both educational and differentiating.

10. **Price below SWAM, above free.** The $49-79 per instrument range is underserved and appropriate for an indie developer.

---

## References

- [Audio Modeling SWAM](https://audiomodeling.com/category/swam/)
- [SWAM String Sections - Sound on Sound Review](https://www.soundonsound.com/reviews/audio-modeling-swam-string-sections)
- [SWAM Solo Strings v3.8.0 User Manual](https://static.audiomodeling.com/manuals/strings/SWAM%20Solo%20Strings%20v3.8.0%20-%20User%20Manual.pdf)
- [Expressive E Soliste](https://www.expressivee.com/139-soliste)
- [Expressive E Soliste - Sound on Sound Review](https://www.soundonsound.com/reviews/expressive-e-soliste)
- [Applied Acoustics String Studio VS-3](https://www.applied-acoustics.com/string-studio-vs-3/)
- [Physical Audio Preparation 2](https://physicalaudio.co.uk/products/preparation-2/)
- [Image-Line Sakura](https://www.image-line.com/fl-studio-learning/fl-studio-online-manual/html/plugins/Sakura.htm)
- [Arturia Augmented Strings](https://www.arturia.com/products/software-instruments/augmented-strings/overview)
- [NI Session Strings Pro 2](https://www.native-instruments.com/en/products/komplete/cinematic/session-strings-pro-2/)
- [STK Bowed.cpp Source](https://github.com/thestk/stk/blob/master/src/Bowed.cpp)
- [STK BowTable Class](https://ccrma.stanford.edu/software/stk/classstk_1_1BowTable.html)
- [FAUST Physical Modeling Library](https://faustlibraries.grame.fr/libs/physmodels/)
- [Csound wgbow Opcode](https://csound.com/docs/manual-fr/wgbow.html)
- [Tao Synth (GitHub)](https://github.com/lucasw/tao_synth)
- [Digital Waveguide Bowed String - J.O. Smith](https://www.dsprelated.com/freebooks/pasp/Digital_Waveguide_Bowed_String.html)
- [Bowed Strings - J.O. Smith (Stanford CCRMA)](https://ccrma.stanford.edu/~jos/BowedStrings/BowedStrings.pdf)
- [Desvages (2018) - Physical Modelling of the Bowed String (Edinburgh PhD)](https://www.acoustics.ed.ac.uk/wp-content/uploads/Theses/Desvages_Charlotte__PhDThesis_UniversityOfEdinburgh_2018.pdf)
- [FAUST-STK Paper (Michon, DAFx 2011)](https://ccrma.stanford.edu/~rmichon/publications/doc/DAFx11-Faust-STK.pdf)
- [DDSP: Differentiable Digital Signal Processing](https://magenta.tensorflow.org/ddsp)
- [Differentiable Modal Synthesis (arXiv 2024)](https://arxiv.org/abs/2407.05516)
- [DDSP Review Paper (Frontiers 2023)](https://www.frontiersin.org/journals/signal-processing/articles/10.3389/frsip.2023.1284100/full)
- [Embertone Chang Erhu](https://embertone.com/instruments/chang-erhu/)
- [Sound Magic Neo Erhu](https://neovst.com/product/neo-erhu/)
- [KVR Discussion: Physically Modelled Bowed Strings](https://www.kvraudio.com/forum/viewtopic.php?t=513622)
- [VI-Control: Physically Modelled Bowed Strings Discussion](https://vi-control.net/community/threads/lets-discuss-physically-modelled-bowed-strings.52471/)
