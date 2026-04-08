# O-Reed: Commercial Landscape & Competitive Analysis

> Market research for a physical modeling reed wind instrument synthesizer plugin.
> Researched: 2026-04-04

---

## 1. Existing PM Wind/Reed Instrument Products

### 1.1 Audio Modeling SWAM Solo Woodwinds (The Market Leader)

| Field | Detail |
|-------|--------|
| **Developer** | Audio Modeling (Italy) |
| **Price** | Clarinets $170, Saxophones $250, Double Reeds $250, Flutes $250. Bundle $750 (sale ~$449). All-In Bundle ~$1279. |
| **Modeling approach** | Hybrid: physical modeling + behavioral modeling + multi-vector phase-synchronous sample morphing. Proprietary "SWAM" engine. Not pure PM -- uses sample components for timbre accuracy. |
| **Instruments** | Bb/A/Eb/Bass Clarinets, Soprano/Alto/Tenor/Baritone Sax, Oboe/English Horn/Bassoon/Contrabassoon, Flutes. All Western orchestral woodwinds covered. |
| **Sound quality** | Best-in-class for realistic solo woodwinds. Forum consensus: "absolute best solo instrument VSTs for both sound and playability." Brass instruments criticized for sounding too similar. |
| **Control** | Full breath controller support (TEControl, EWI, Aerophone). Expression Trigger Mode for BC. MIDI CC mapping, Hi-Res CC, NRPN. MPE support (LinnStrument, Seaboard) but with known bugs -- out-of-range CC/Pressure/PitchBend interference in MPE profile. |
| **CPU** | Low. Min 1.6 GHz Core 2 Duo for single instance. v3 significantly reduced from v2. Full orchestral templates now feasible. |
| **v3 additions** | MAQAM microtuning (buggy), alt fingering, timbral correction, EQ, room simulator, extended range, iPad playing surface. |

**Strengths:**
- Unmatched realism for Western orchestral woodwinds
- Very low CPU for a PM engine
- Excellent breath controller integration
- Continuous dynamics, no velocity layers or crossfading
- Active development (v3 released 2025)

**Weaknesses:**
- Expensive ($750 for woodwinds bundle)
- Steep learning curve -- difficult to get expressive results without practice
- UI criticized: key features hidden in menus, not immediately visible
- MPE implementation has known issues
- MAQAM/microtuning buggy at launch
- No non-Western instruments (no duduk, shehnai, suona)
- Not designed for experimental/sound design use
- "Too perfect" consistency gives synthetic quality in some contexts
- Camelot host crashes frequently, slow load times

---

### 1.2 Imoxplus Respiro

| Field | Detail |
|-------|--------|
| **Developer** | Imoxplus (France), inspired by Pedro Eustache |
| **Price** | ~$165 (intro pricing sometimes lower) |
| **Modeling approach** | Pure physical modeling + wavetable injection. No samples. State-of-the-art PM engine. |
| **Instruments** | 150+ instruments/variations. Reeds, flutes, brassy sounds, harmonica, bass tones. Does NOT target specific named instruments -- creates new expressive instruments. |
| **Sound quality** | Organic and expressive. Strong on reed-type sounds. Not going for photorealistic emulation but rather "never heard before" timbres. |
| **Control** | Designed for wind controllers (Sylphyo, EWI, WX, NuEVI, Warbl, Aerophone). MPE (single channel), hi-res pressure. Mazeka Toys controller integration. Microtuning via keyswitches or CC102-113. |
| **CPU** | Low (no samples = fast loading, small footprint) |

**Strengths:**
- Purpose-built for wind/breath controller players
- True PM synthesis -- no samples, maximum expression from playing technique
- Wide tonal range from acoustic to experimental
- Microtuning support
- Low price point relative to SWAM
- iOS version available

**Weaknesses:**
- Small/solo developer -- uncertain long-term support
- Hundreds of hidden parameters, large learning curve for sound design
- Not trying to emulate specific instruments (pro and con)
- Limited update cadence (last major update ~2023)
- Niche user base
- UI designed to hide complexity but limits power users

---

### 1.3 AAS Chromaphone 3

| Field | Detail |
|-------|--------|
| **Developer** | Applied Acoustics Systems (Montreal) |
| **Price** | $99 |
| **Modeling approach** | Pure physical modeling. Two resonators (strings, plates, drumheads, membranes, beams, bars, tubes) with coupled energy flow. No samples. |
| **Instruments** | 34 sound categories including reeds. 2210+ presets. General-purpose PM synth, not wind-specific. |
| **Sound quality** | Excellent for percussion, mallet, plucked, and experimental textures. Reed/wind sounds are secondary and lack the depth of dedicated wind PM. |
| **Control** | Standard MIDI. Velocity, mod wheel. No specific breath controller or MPE optimization. |
| **CPU** | Low-moderate |

**Strengths:**
- Incredibly affordable ($99)
- Versatile -- covers many acoustic domains
- Mature, well-designed UI
- AAS has decades of PM expertise
- Good entry point for PM synthesis

**Weaknesses:**
- Reed/wind sounds are not the focus -- generic tube resonators, not dedicated reed models
- No breath controller optimization
- No MPE support
- Cannot compete with SWAM for realistic wind instrument emulation
- Jack-of-all-trades, master of none for wind

---

### 1.4 Expressive E Imagine

| Field | Detail |
|-------|--------|
| **Developer** | Expressive E (France) + AAS collaboration |
| **Price** | ~$89-139 (varies with promotions) |
| **Modeling approach** | Physical modeling via AAS engine. Skins, strings, bars, tubes as resonant bodies. Mallet, noise, sequences as exciters. |
| **Instruments** | 400 presets combining two instrument layers. Imaginary acoustic instruments -- tubes + strings, drums + flutes, etc. |
| **Sound quality** | Bold, distinctive hybrid timbres. Not realistic emulation. Designed for Osmose controller. |
| **Control** | Deep modulation (4 macros with MSEG each). Built for Osmose MPE controller but works with standard MIDI. |

**Strengths:**
- Innovative layered approach (combine 2 resonator types)
- Beautiful, modern UI
- Deep modulation system
- Good MPE integration (designed for Osmose)
- Affordable

**Weaknesses:**
- Tubes are generic, not specialized reed models
- No breath controller focus
- No specific wind instrument emulations
- More about "imaginary" instruments than realistic ones

---

### 1.5 Arturia Augmented Woodwinds

| Field | Detail |
|-------|--------|
| **Developer** | Arturia (France) |
| **Price** | $99-149 |
| **Modeling approach** | Hybrid: sampled woodwinds + 4 synthesis engines (Virtual Analog, Granular, Wavetable, Harmonic/Additive). NOT physical modeling. |
| **Instruments** | Solo Clarinet, Bassoon, English Horn, Flute, Contra Bassoon, Bass Clarinet, Oboe + ensembles. 56 articulations from 11 musicians. |
| **Sound quality** | Good for layered/processed woodwind textures. Sample layer provides realism, synth layers add movement and evolution. |
| **Control** | Morph control, macros, 16-step arp. Standard MIDI. No breath controller or MPE optimization. |

**Strengths:**
- Excellent hybrid concept -- bridge acoustic and electronic
- Modern, polished Arturia UI
- Good for evolving textures and pads
- V Collection integration
- Affordable

**Weaknesses:**
- NOT physical modeling -- sample-based foundation
- No continuous dynamics from PM engine
- Limited solo instrument realism
- No breath controller support
- No MPE
- More "woodwind-flavored synth" than "virtual woodwind"

---

### 1.6 Acousticsamples VWinds (Clarinets & Double Reeds)

| Field | Detail |
|-------|--------|
| **Developer** | Acousticsamples (France) |
| **Price** | Clarinets Bundle ~$219 (8 instruments). Double Reeds ~$219 (8 instruments). Individual ~$109 each. |
| **Modeling approach** | Hybrid: recorded samples + proprietary H.A.T. (Harmonic Alignment Technology) modeling. Sample+model blend. |
| **Instruments** | 2 Bb Clarinets, 2 Bass Clarinets, Eb Clarinet, A Clarinet, Basset Horn, Contrabass Clarinet, 3 Oboes, 2 English Horns, 2 Bassoons, Contrabassoon. |
| **Sound quality** | Very good for realistic solo performance. ~100MB per instrument. Praised for unsurpassed playability and phrase shaping. |
| **Control** | Designed for air flow control. Works with breath controllers. No keyswitch requirement. HAT enables natural legato without complex MIDI programming. |

**Strengths:**
- Excellent playability out of the box
- Wide clarinet/double reed coverage (8 instruments each)
- Very compact file sizes
- No keyswitch workflow -- natural playing
- V2 major overhaul improved everything

**Weaknesses:**
- Sample-based foundation limits range beyond recorded material
- Not pure PM -- cannot create "impossible" instruments
- No saxophone coverage
- No non-Western instruments
- Requires UVI Workstation
- Less experimental/sound design potential

---

### 1.7 Wallander Instruments WIVI (Discontinued)

| Field | Detail |
|-------|--------|
| **Developer** | Wallander Instruments (Sweden) |
| **Price** | N/A (discontinued) |
| **Modeling approach** | Physical modeling. 10 modeled wind instruments. |
| **Instruments** | Trumpet, Trombone, French Horn, Tuba, Flute, Clarinet, Oboe, Bassoon, Tenor Sax, Soprano Recorder. |
| **Sound quality** | Pioneering for its era. Continuous dynamics via mod wheel/expression/breath controller. Virtual stage positioning. |
| **Control** | Mono and poly modes. Breath controller support. Virtual stage positioning GUI. |

**Significance for O-Reed:** WIVI was ahead of its time but is now gone. Its discontinuation left a gap that SWAM partially filled, but the "affordable PM wind ensemble" niche is wide open.

---

### 1.8 GeoShred / Naada Instruments (moForte)

| Field | Detail |
|-------|--------|
| **Developer** | moForte (Jordan Rudess / Dr. Julius O. Smith III) |
| **Price** | iOS: $14.99/instrument, Collections $74.99. Desktop (Mac AU only): $50/instrument, Collections $250. |
| **Modeling approach** | Physical modeling based on Julius O. Smith III research (Stanford/CCRMA). FDTD and waveguide methods. |
| **Instruments** | GeoSWAM: Trumpet, Bass Trombone, Bass Flute, Tenor Sax, Flute, Clarinet, Oboe. Naada Collection: Bansuri, Shehnai, Duduk, Dizi, Suona, Nadaswaram, Pan Flute + strings. |
| **Sound quality** | High quality for a mobile app. Academic pedigree (Smith's research). |
| **Control** | Touch surface with pitch rounding. Best on iPad with touch. |

**Strengths:**
- **Only product with physically modeled Shehnai, Duduk, and Suona**
- Built on world-class PM research
- Multi-award winning
- Touch performance interface is innovative

**Weaknesses:**
- iOS-first -- desktop version Mac-only AU plugin
- No Windows support
- No standard MIDI keyboard/breath controller workflow for desktop
- Expensive for desktop ($50/instrument)
- Touch-centric design doesn't translate well to DAW workflow
- No VST3 format

---

### 1.9 Erica Synths Steampipe (Hardware)

| Field | Detail |
|-------|--------|
| **Developer** | Erica Synths + 112dB |
| **Price** | ~$999 (hardware desktop synth) |
| **Modeling approach** | Modified Karplus-Strong. Steam/Pipe/Reverberator sections. No traditional oscillators. |
| **Instruments** | Pipes, strings, bells, resonant bodies. Flutes, clarinets, industrial horns, ambient textures. |
| **Sound quality** | "One of the most organic, alive-feeling synths." Ranges from lifelike to impossible. |
| **Control** | 8-voice poly. MPE support. Full wind controller support. 32 adjustable parameters. 5 LFOs. |

**Relevant insight:** Steampipe proves market appetite for PM wind synthesis + experimental use. Hardware-only limits reach.

---

### 1.10 Other Relevant Products

| Product | Developer | Type | Notes |
|---------|-----------|------|-------|
| **Modartt Pianoteq 9** | Modartt | PM Piano/Percussion | No wind instruments. Syngular expansion (2025) adds general PM synth but not wind-specific. |
| **Baby Audio Atoms** | Baby Audio | PM Synth (mass-spring) | Bowing exciter, organic textures. No wind-specific models. |
| **Madrona Labs Kaivo** | Madrona Labs | PM + Granular | FDTD models, strings/chimes/springs. Bodies include wood boxes, plates, drums. No wind/reed. Excels at "mad textural stuff." |
| **Physical Audio Derailer** | Physical Audio (UK) | PM Synth (FDTD) | Strings, bars, spring connections. Academic origin (Edinburgh). No wind. |
| **Soule DSP Resonarium** | Soule DSP | PM/Waveguide (free, OSS) | 4 waveguide blocks, modal + KS + waveguide. MPE support. Early development (v0.0.10). No wind-specific. |
| **RipplerX** | Tiagolr | Modal Synth (free, OSS) | Dual modal resonators. Wind-like textures possible but no reed models. |

### 1.11 Sample-Based Market Context (Not PM, But Competition for Mindshare)

| Product | Price | Notes |
|---------|-------|-------|
| **Spitfire Symphonic Woodwinds** | $449 | Industry standard for orchestral sections. Deep sampled, multiple mics. Not solo-expressive. |
| **Cinematic Studio Woodwinds** | ~$399 | Praised legato engine. 3 dynamics of real legato transitions. |
| **NI Action Woodwinds** | Part of Komplete | Phrase-based. 1000+ pre-built phrases. Steep learning curve. |
| **EastWest Hollywood Orchestral Woodwinds** | Part of Composer Cloud ($24.99/mo) | Multi-mic, huge sample sets. RAM-intensive. |
| **Impact Soundworks Ventus Duduk** | ~$79 | Deep-sampled duduk. Kontakt Player. |
| **EastWest Silk** | Part of Composer Cloud | Suona, duduk, ney, Chinese/Indian/Persian winds. Sample-based. |
| **Swar Systems SwarPlug** | ~$299 | 100+ Indian instruments including Shehnai. Sample-based. |

---

## 2. Open Source & Academic Implementations

### 2.1 STK (Synthesis ToolKit) -- Perry Cook & Gary Scavone

- **Repository:** [github.com/thestk/stk](https://github.com/thestk/stk)
- **Reed models:** `BlowHole` (clarinet with tonehole + register vent), `Saxofony` (pseudo-conical bore)
- **Other wind:** `Flute`, `BlowBotl`, `Brass`
- **Tech:** Digital waveguide models, nonlinear reed table, tonehole scattering junctions
- **Status:** Mature, widely integrated (ChucK, Csound, Max/MSP PeRColate, SuperCollider, Faust)
- **Limitation:** Educational quality -- sounds dated, no modern parameter space, no MPE

### 2.2 Faust Physical Modeling Library (pm.lib)

- **Repository:** [github.com/grame-cncm/faustlibraries](https://github.com/grame-cncm/faustlibraries/blob/master/physmodels.lib)
- **Reed models:** `reedTable` (basic single reed), `clarinetModel` (tube + reed + bell)
- **Other wind:** `fluteModel`, generic brass
- **Tech:** Modular building blocks -- reeds, mouthpieces, tubes, bells. Composable.
- **Status:** Active development. Can compile to VST/AU/standalone via Faust compiler.
- **Limitation:** Basic implementations -- not production quality. Building blocks, not finished instruments.
- **Relevance:** Excellent reference for architecture. Faust-STK paper ports STK models.

### 2.3 IRCAM Modalys

- **Tech:** Modal synthesis. Physical objects (strings, plates, tubes, membranes, reeds, hammers) with interaction types (striking, rubbing, mouthpiece, bow).
- **Interface:** Max and OpenMusic frontends. Modalys 3.7 freely available.
- **Limitation:** Academic tool, not a plugin. Max-based workflow. Steep learning curve.
- **Relevance:** State-of-the-art modal synthesis research. Good reference for coupling models.

### 2.4 Gary Scavone's MATLAB Waveguide Toolkit (2024)

- Open-source MATLAB class for modeling arbitrary wind instrument air columns
- Digital waveguide methods with configurable length, radii, hole geometry
- Presented at ASA 2024
- **Relevance:** Latest academic reference for bore modeling with tonehole parameters

### 2.5 Other Notable Open Source

| Project | Tech | Notes |
|---------|------|-------|
| **Resonarium** (Soule DSP) | Waveguide + Modal + KS | Free, open source, JUCE-based. MPE. Not wind-specific but architecture is relevant. |
| **OpenPiano** | PM Piano (JUCE) | Not wind, but shows JUCE PM plugin architecture |
| **AudioKit/STKAudioKit** | STK wrappers for iOS | STK instrument models in Swift/AudioKit |

---

## 3. Gap Analysis

### 3.1 The Non-Western Instrument Gap (CRITICAL)

**Current state:** Zero desktop VST/AU plugins offer physically modeled non-Western reed instruments.

- GeoShred has Shehnai, Duduk, Suona, Dizi, Nadaswaram -- but iOS-only (Mac AU desktop exists but no Windows, no VST3)
- Everything else is sample-based (Swar Systems, EastWest Silk, Impact Soundworks Ventus)
- Sample-based non-Western instruments cannot do continuous pitch bending, ornamental trills, or microtonal inflections authentically

**O-Reed opportunity:** First desktop plugin with physically modeled shehnai, suona, duduk, hichiriki, and other non-Western reed instruments in VST3/AU format. This is an uncontested market position.

### 3.2 The Sound Design / Experimental Gap

**Current state:**
- SWAM is locked to realistic emulation -- no "impossible instrument" territory
- Respiro is the only wind PM plugin that explicitly targets experimental territory, but it's a small niche product
- Chromaphone/Imagine/Kaivo offer experimental PM but not wind-specific
- Steampipe (hardware only) proves the concept works and has demand

**O-Reed opportunity:** Continuous parameter space from "realistic clarinet" through "hybrid instrument" to "impossible wind creature." The morph-between-instruments paradigm that no one else offers.

### 3.3 The Price Point Gap

| Tier | Current products | Gap? |
|------|-----------------|------|
| Free | Faust, STK, RipplerX (not wind-specific) | No free dedicated wind PM plugin exists |
| $50-100 | Chromaphone $99, Imagine $89 | General PM synths, not wind-focused |
| $100-200 | Respiro $165, SWAM Clarinets $170 | Respiro is closest competitor at this tier |
| $200-500 | SWAM Saxes $250, VWinds bundles $219 | Expensive for individual collections |
| $500+ | SWAM Bundle $750 | Very expensive for full coverage |

**O-Reed opportunity:** A unified reed instrument at $99-149 covering single reeds, double reeds, AND non-Western reeds would undercut SWAM dramatically while offering broader instrument coverage.

### 3.4 The UI/UX Gap

**Current state:**
- SWAM: Functional but clinical. Key features hidden. Steep learning curve.
- Respiro: Simplified facade hiding complex engine. Limited customization.
- Chromaphone: Best UI of the PM synths but not wind-specific.
- General consensus in forums: "Physical modeling needs a renaissance for accessibility and GUI"

**Specific complaints:**
- Programmer terminology instead of instrument terminology
- No visual feedback of what the model is doing
- Parameter relationships unclear
- Presets that sound nothing like the parameter names suggest

**O-Reed opportunity:** Modern WebView UI with:
- Visual reed/bore/bell animation responding to parameters
- Instrument morph space (2D pad from clarinet to sax to duduk)
- Breath controller setup wizard
- Real-world terminology (embouchure, reed stiffness, bore shape)

### 3.5 The Unified Instrument Gap

**Current state:** You must buy separate products for:
- Clarinets (SWAM $170 or VWinds $219)
- Saxophones (SWAM $250)
- Double Reeds (SWAM $250 or VWinds $219)
- Non-Western winds (only sample libraries, $79-299 each)
- Experimental wind textures (Respiro $165 or Steampipe $999 hardware)

Total cost for full coverage: $800-1500+ across multiple products from multiple vendors.

**O-Reed opportunity:** One plugin, one continuous parameter space, all reed instruments. The "instrument identity" comes from parameter configuration, not from buying separate products.

### 3.6 The MPE Gap

- SWAM has MPE but it's buggy
- Respiro has single-channel MPE only
- No wind PM plugin has first-class, deeply tested MPE implementation
- MPE controller market is growing (Osmose, Push 3, Seaboard, LinnStrument)

---

## 4. User Needs & Use Cases

### 4.1 Film/Game Scoring Composers

**Need:** Realistic solo woodwinds for sketching and final output. Quick turnaround. Multiple articulations without keyswitch complexity.
**Current solution:** SWAM for solos, Spitfire/CSS for sections.
**Pain points:** SWAM's learning curve. Cost of full woodwind coverage. No non-Western winds for ethnic scoring cues.
**O-Reed angle:** Quick-start presets for common instruments. Instant playability. Non-Western instruments for world-music cues without buying separate libraries.

### 4.2 Electronic/Ambient Producers

**Need:** Organic textures, evolving pads with acoustic quality, unique timbres.
**Current solution:** Arturia Augmented Woodwinds (hybrid), Chromaphone, process sample libraries.
**Pain points:** Sample-based hybrids feel static. Pure synths lack acoustic quality.
**O-Reed angle:** The experimental end of the parameter space -- morphing, impossible instruments, breath-responsive textures that no sample library can create.

### 4.3 World Music Producers

**Need:** Authentic-sounding shehnai, suona, duduk, ney with proper ornamentation and microtonal inflection.
**Current solution:** Sample libraries (EastWest Silk, Swar Systems, Impact Soundworks Ventus).
**Pain points:** Samples cannot do continuous pitch bending or authentic ornamental patterns. Limited articulation sets. "MIDI woodwind" artifacts on fast passages.
**O-Reed angle:** First physically modeled non-Western reeds with continuous pitch, natural ornamentation, and microtuning.

### 4.4 Wind Controller / EWI Players

**Need:** Expressive PM engine that responds to breath pressure, bite, head tilt. Fast attack response. Natural sustain-to-release behavior.
**Current solution:** SWAM (best), Respiro (specialized), GeoShred (touch only).
**Pain points:** Limited choice. SWAM is expensive. Respiro has narrow development.
**O-Reed angle:** First-class breath controller support rivaling SWAM, at lower price, with broader instrument coverage including non-Western.

### 4.5 MPE Controller Users (LinnStrument, Osmose, Seaboard, Push 3)

**Need:** Instruments that fully exploit per-note pressure, slide, and strike dimensions.
**Current solution:** SWAM (buggy MPE), Respiro (single channel), general PM synths.
**Pain points:** No wind PM plugin has truly robust MPE. Per-note embouchure/breath mapping would be revolutionary.
**O-Reed angle:** Per-note pressure = breath, per-note slide = pitch bend/vibrato, strike = attack character. Make MPE feel like playing a real wind instrument.

### 4.6 Sound Designers

**Need:** Novel timbres for trailers, games, installations. Controllable, automatable, non-repetitive.
**Current solution:** Layer sample libraries, granular processing, Chromaphone/Kaivo.
**Pain points:** Sample-based approaches repeat. Granular loses physical coherence. General PM synths lack wind-specific character.
**O-Reed angle:** The "impossible instrument" zone -- 10-foot clarinet, metal duduk, double-bore saxophone. Physical models with parameters pushed beyond reality.

---

## 5. Competitive Positioning for O-Reed

### 5.1 Differentiation Strategy

**Primary differentiator: Unified continuous parameter space across ALL reed types.**

No competitor offers this. SWAM sells separate products per instrument family. Respiro doesn't target specific instruments. Nobody covers both Western and non-Western in one engine.

O-Reed's core pitch: **One plugin where "instrument identity" is a continuous, morphable parameter -- not a separate product you buy.**

**Secondary differentiators:**

1. **Non-Western instrument coverage** -- completely uncontested in PM plugin space
2. **Realistic-to-experimental continuum** -- SWAM can't do experimental, Respiro can't do realistic
3. **Modern WebView UI** -- visual model feedback, instrument morph space, breath controller wizard
4. **First-class MPE** -- per-note breath/embouchure from pressure dimension
5. **Price** -- unified product vs. buying $750+ across multiple SWAM collections

### 5.2 Price Positioning

| Strategy | Price | Rationale |
|----------|-------|-----------|
| **Recommended** | **$129** | Undercuts SWAM Clarinets ($170) while covering ALL reed instruments. Above Chromaphone ($99) to signal specialization. Below SWAM Saxophones ($250). |
| Aggressive | $79-99 | Maximum market penetration. Risks perception as "budget." |
| Premium | $179-249 | Justified if sound quality matches SWAM. Risks price comparison with SWAM individual packs. |
| Intro + upgrade | $99 launch, $149 after 6 months | Builds initial user base with lower barrier. |

### 5.3 Feature Prioritization (Based on Market Gaps)

**Must-have (launch):**
1. Single reed models: clarinet, saxophone (the comparison targets for SWAM)
2. Double reed models: oboe, bassoon
3. At least 2 non-Western reeds: duduk, shehnai or suona (unique selling point)
4. Continuous parameter morphing between instrument types
5. Full breath controller support (TEControl, EWI, Aerophone)
6. Microtuning / MAQAM support (SWAM has this but it's buggy)
7. Modern UI with visual model feedback
8. Low CPU usage (must match or beat SWAM)

**Should-have (launch or v1.1):**
1. MPE support (per-note pressure/slide mapping)
2. Preset browser with instrument categories
3. Extended non-Western: hichiriki, suona, nadaswaram
4. Room/environment simulator
5. Automation of all parameters

**Nice-to-have (v1.x updates):**
1. "Impossible instrument" presets (sound design territory)
2. Per-note polyphonic mode for MPE
3. iOS/iPadOS companion
4. Ensemble mode (detune/spread multiple instances)
5. Custom bore shape editor

### 5.4 What Makes This a Must-Have vs. Nice-to-Have

**Must-have triggers:**
- "It's the only PM plugin with shehnai/duduk/suona" -- world music producers have no alternative
- "One plugin replaces $750 of SWAM woodwinds" -- budget-conscious composers
- "It morphs between instruments in ways no other plugin can" -- sound designers, electronic producers
- "It's the best wind plugin for my Osmose/LinnStrument" -- MPE early adopters

**Nice-to-have traps to avoid:**
- Trying to beat SWAM at pure realism for Western instruments alone (they have years of head start and hybrid sample+PM advantage)
- Targeting only EWI players (niche too small for standalone product)
- Over-investing in polyphonic mode (wind instruments are fundamentally monophonic)

### 5.5 Competitive Response Risk

| Competitor | Likely response | Risk level |
|------------|----------------|------------|
| Audio Modeling (SWAM) | Could add non-Western instruments in v4. Would take 1-2 years. | Medium |
| Imoxplus (Respiro) | Small team, unlikely to pivot to realistic instruments. | Low |
| AAS | Could extend Chromaphone with wind-specific features. Slow development cycle. | Low |
| Expressive E | Focused on Osmose ecosystem. Could add wind models to Imagine. | Low |
| moForte (GeoShred) | Already has Naada but locked to iOS/Mac AU. Windows VST3 unlikely soon. | Low |

**Window of opportunity:** 12-18 months before any competitor could match O-Reed's unique combination of unified reed PM + non-Western instruments + modern UI.

---

## Summary: The O-Reed Opportunity

The reed instrument PM plugin market has exactly one dominant player (SWAM) selling expensive, separate products focused exclusively on Western orchestral instruments, and a handful of niche/general-purpose alternatives. Nobody offers:

1. A unified PM engine covering single reed + double reed + non-Western reed in one product
2. Continuous morphing between instrument identities
3. Both realistic and experimental sound in one engine
4. Properly implemented MPE for wind instruments
5. Modern, visual, accessible UI for PM wind instruments
6. Physically modeled non-Western reed instruments on desktop (VST3/AU)

O-Reed can own positions 1-6 simultaneously. The competitive moat is the combination, not any single feature.
