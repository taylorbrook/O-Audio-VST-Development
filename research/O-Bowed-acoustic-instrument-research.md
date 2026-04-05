---
title: "O-Bowed: Acoustic Instrument Research - Violin, Cello, Erhu, Nyckelharpa"
created: 2026-04-04
juce_version: "8.0.4"
summary: "Deep-dive into acoustic properties, physical modeling feasibility, and market analysis for four candidate bowed instruments. Includes string parameters, body resonance data, bow mechanics, and implementation recommendations."
domain: dsp
type: research
keywords:
  - physical-modeling
  - bowed-string
  - waveguide
  - violin
  - cello
  - erhu
  - nyckelharpa
  - body-resonance
  - sympathetic-strings
  - bow-friction
stages: [0]
agents: [research, dsp]
---

# O-Bowed: Acoustic Instrument Research

## Table of Contents
1. [Violin Acoustics](#1-violin-acoustics)
2. [Cello Acoustics](#2-cello-acoustics)
3. [Erhu Acoustics](#3-erhu-acoustics)
4. [Nyckelharpa Acoustics](#4-nyckelharpa-acoustics)
5. [Bow-String Interaction Model](#5-bow-string-interaction-model)
6. [Body Resonance Modeling Approaches](#6-body-resonance-modeling-approaches)
7. [Sympathetic Resonance Modeling](#7-sympathetic-resonance-modeling)
8. [Competitive Landscape](#8-competitive-landscape)
9. [Feasibility Assessment Matrix](#9-feasibility-assessment-matrix)
10. [Recommendation](#10-recommendation)

---

## 1. Violin Acoustics

### 1.1 String Properties

Tuning: G3 - D4 - A4 - E5 (tuned in perfect fifths)
Vibrating string length: 325-328 mm (standard 4/4 violin)

| String | Note | Frequency (Hz) | Typical Tension (kg) | Tension (N) | Material |
|--------|------|-----------------|---------------------|-------------|----------|
| I (E)  | E5   | 659.3           | 7.2 - 8.2           | 70.6 - 80.4 | Plain steel (unwound) |
| II (A) | A4   | 440.0           | 5.5 - 5.8           | 53.9 - 56.9 | Aluminum-wound synthetic/gut |
| III (D)| D4   | 293.7           | 4.5 - 4.8           | 44.1 - 47.1 | Aluminum or silver-wound |
| IV (G) | G3   | 196.0           | 4.3 - 4.8           | 42.2 - 47.1 | Silver-wound (dense for thinner gauge) |

Reference string set (Thomastik-Infeld Dominant Medium): Total tension ~21.2 kg (208 N).

Key observations for modeling:
- E string is unique: plain steel, not wound. Fastest response, brightest tone, thinnest gauge.
- G string uses silver winding to keep diameter manageable while achieving low pitch.
- Total tension across all 4 strings: approximately 200-230 N (45-52 lbs).
- String diameters range from ~0.26 mm (E) to ~0.81 mm (G).

### 1.2 Body Resonance: Signature Modes

The violin body has well-characterized "signature modes" that are remarkably consistent across instruments (Stradivari, Guarneri, modern makers). These are the critical targets for body modeling.

| Mode | Name | Typical Freq (Hz) | Character | Radiation |
|------|------|--------------------|-----------|-----------|
| A0   | Air (Helmholtz) | ~272 | Body "breathes" through f-holes | Strong |
| CBR  | C-bouts rhomboidal | ~407 | Top/back move similarly, minimal volume change | Weak (not a strong radiator) |
| B1-  | Baseball mode 1 | ~462 | Sinuous nodal line, significant volume variation | Very strong |
| B1+  | Baseball mode 2 | ~551 | Twin to B1-, different nodal pattern | Very strong |

Additional critical features:
- **Bridge hill**: Broad peak at 2-3 kHz (centered ~2.3 kHz), 20 dB elevation in bridge admittance. Created by bridge rocking resonance. This is the "brilliance" formant of the violin.
- **Bridge bouncing resonance**: ~6 kHz. Secondary bridge mode.
- **Above ~2 kHz**: Modal overlap increases dramatically; individual modes become indistinguishable. Statistical energy analysis is more appropriate than mode-by-mode modeling.

### 1.3 Bridge Transfer Function

The bridge acts as a linear filter between string and body:
- Takes input from the string vibration
- Produces two modified outputs at the feet (treble foot on soundpost side, bass foot on bass bar side)
- The bridge hill at 2-3 kHz followed by a steady high-frequency rolloff is the defining timbral characteristic
- Bridge can compensate weak/strong areas in body response
- Modeled effectively with a 2-pole resonance (~2.5 kHz) with appropriate Q and gain

### 1.4 Bowing Position Effects

Relative bow position beta = bow-bridge distance / string length:
- **Sul ponticello** (near bridge): beta ~ 0.02-0.05. Bright, glassy, rich in harmonics. Higher partials dominate.
- **Ordinario** (normal): beta ~ 0.08-0.13. Balanced, full tone.
- **Sul tasto** (over fingerboard): beta ~ 0.15-0.25. Flute-like, warm, fundamental-heavy.

The harmonic content of the bowed string is inversely related to beta: the nth harmonic is suppressed when the bow is at 1/n of the string length.

### 1.5 Vibrato Mechanics

- Finger oscillation on string: 5-7 Hz rate, pitch deviation ~20-50 cents
- Modulates both pitch AND coupling to body resonances
- The spectral centroid oscillation from body-coupling modulation is perceptually as important as the pitch modulation
- Implementation: modulate delay line length + body filter parameters simultaneously

### 1.6 Wolf Tone

Less prominent in violin than cello. When string fundamental aligns with a strong body mode (typically near B1- or B1+), the string splits into two close frequencies producing beating. In violin, manageable due to higher frequency range with less body-string coupling overlap.

### 1.7 Key Frequency Range

- Open strings: G3 (196 Hz) to E5 (659 Hz)
- Practical range with fingering: G3 to E7+ (~2637 Hz+)
- Harmonics extend to 10-15 kHz

---

## 2. Cello Acoustics

### 2.1 String Properties

Tuning: C2 - G2 - D3 - A3 (tuned in perfect fifths, one octave + fifth below violin)
Vibrating string length: 690-700 mm (standard 4/4 cello)

| String | Note | Frequency (Hz) | Typical Tension (lbs) | Tension (N) | Material |
|--------|------|-----------------|----------------------|-------------|----------|
| I (A)  | A3   | 220.0           | 37-41                | 165-182     | Steel core, flat-wound |
| II (D) | D3   | 146.8           | 29-32                | 129-142     | Steel/synthetic core, wound |
| III (G)| G2   | 98.0            | 28-32                | 125-142     | Tungsten or silver-wound |
| IV (C) | C2   | 65.4            | 27-31                | 120-138     | Tungsten or silver-wound |

Reference: Larsen Magnacore Medium set. Total tension ~135 lbs (600 N) -- nearly 3x violin.

String diameters: A ~0.77 mm, D ~0.87 mm, G ~0.99 mm, C ~1.37 mm. Much heavier strings than violin, requiring a heavier bow with wider hair ribbon.

### 2.2 Body Resonance: Signature Modes

The cello body is acoustically "better matched" to its string frequencies than the violin -- its body resonances overlap more directly with the fundamental frequencies of the strings.

| Mode | Typical Freq (Hz) | Notes |
|------|--------------------|-------|
| A0 (Helmholtz) | ~93-100 | Air resonance -- much lower than violin due to larger cavity |
| CBR | ~170-200 | C-bouts rhomboidal mode |
| B1- | ~160-185 | First strong body bending mode |
| B1+ | ~530-610 | Second strong body bending mode |
| Main Body Resonance (MBR) | 147-196 (D3-G3) | Strongest overall body resonance |

Critical difference from violin: the MBR sits right in the fundamental frequency range of the open strings (D3=147 Hz, G2=98 Hz), creating much stronger string-body coupling -- which is both a blessing (rich tone) and a curse (wolf tones).

### 2.3 Wolf Tone (Critical for Cello Modeling)

The wolf tone is the defining acoustic challenge of the cello:
- Occurs between E3 and F#3 (typically around F3, ~175 Hz)
- Most prominent on C string in higher positions, G string in mid-positions
- The body MBR "pushes" the string away from its resonant frequency
- String splits into two frequencies separated by 3-10 Hz, producing beating/pulsing
- Musically, this is a critical expressive detail -- cellists work around and sometimes exploit it
- **For modeling**: if you don't have the wolf, it won't sound like a real cello

### 2.4 Endpin Coupling

Unique to cello (and bass): the endpin transmits vibration directly to the floor.
- Creates additional radiation path through the floor surface
- Affects low-frequency response significantly
- Can be modeled as a secondary impedance termination with floor coupling filter
- Material matters: carbon fiber endpin vs. steel vs. aluminum all change the coupling

### 2.5 Bow Differences from Violin

- Cello bow: ~72 cm length, ~80-85 g weight (violin: ~74 cm, ~60 g)
- Wider hair ribbon: ~10 mm vs. violin's ~8 mm
- Different stick-slip dynamics due to heavier strings and lower frequencies
- Bow speed range: typically 5-40 cm/s
- Bow force: higher minimum force needed due to string mass

### 2.6 Key Frequency Range

- Open strings: C2 (65 Hz) to A3 (220 Hz)
- Practical range with fingering: C2 to A5+ (~880 Hz+)
- Harmonics: significant energy below 100 Hz, harmonics to 5-8 kHz

---

## 3. Erhu Acoustics

### 3.1 Physical Overview

The erhu is fundamentally different from the violin family:
- **Two strings only**: D4 (294 Hz) and A4 (440 Hz)
- **No fingerboard**: fingers press strings in mid-air, allowing continuous portamento
- **Snakeskin membrane resonator**: NOT a wooden box -- a cylindrical wooden shell with python skin stretched across one end
- **Bow between strings**: hair passes between the two strings; both sides of the hair are used
- **Open back**: rear of the resonator is open to the air

### 3.2 Membrane Resonator Acoustics

This is the most distinctive acoustic feature and the hardest to model:

| Property | Value | Notes |
|----------|-------|-------|
| Soundbox face diameter | ~10 cm | Circular, covered with python skin |
| Soundbox length | ~13 cm | Similar to human vocal tract length |
| Lowest membrane mode | ~2 kHz | Much higher than wooden box modes |
| Mode structure | Odd harmonic ratios (1:3:5:7...) | Pipe-like behavior due to cylindrical shape |
| Radiation | Stronger from open rear | Counter-intuitive -- sound projects backward |

Critical acoustic findings:
- **Coupled membrane-cavity resonances** dominate the radiation spectrum
- These resonance pairs **resemble human vocal formants** -- this is why the erhu sounds "voice-like" and "nasal"
- The membrane's large admittance provides the instrument's power
- Half the membrane-cavity modes are radiation-efficient "breathing modes"
- Membrane thickness affects volume/timbre: thicker = louder/brighter, thinner = softer/mellower

### 3.3 Bow-Between-Strings Mechanics

This is unique among bowed instruments:
- Bow hair is permanently threaded between the two strings
- **Inner string (D4)**: played by pulling bow toward player (hair presses against string)
- **Outer string (A4)**: played by pushing bow away (stick presses hair against string)
- **Double stops**: rare and noisy because only inner string gets bowed properly; outer string gets col legno (stick contact)
- The bow is mechanically coupled to both strings at all times -- there is always some sympathetic coupling through the bow hair

### 3.4 Expressive Characteristics

- **Portamento**: no frets, no fingerboard. Finger slides produce continuous pitch transitions -- this is THE defining expressive gesture
- **Vibrato**: finger oscillation, typically wider and more varied than violin vibrato
- **Range**: D4 to D7 (~294 Hz to ~2349 Hz), though most playing is D4 to A6
- **Timbre**: nasal, focused, speech-like. Narrower dynamic range than violin but extremely expressive within that range

### 3.5 Modeling Implications

A simple coupled-oscillator model (membrane + cavity) has been shown to reproduce many measured acoustic properties. Key insight: the body model is conceptually simpler than violin (cylindrical geometry, membrane resonator vs. complex wooden plate modes) but acoustically different enough that no violin body model can approximate it.

---

## 4. Nyckelharpa Acoustics

### 4.1 Physical Overview

The nyckelharpa is a Swedish keyed fiddle with extraordinary resonance:
- **16 strings total**: 3 melody + 1 drone + 12 sympathetic resonance strings
- **Key/tangent mechanism**: wooden keys slide under strings; pressing a key pushes a tangent up against the string to change pitch (like a movable fret)
- **Bowed with a short bow**: similar to a violin bow but shorter
- **Larger body than violin**: resembles a large fiddle

### 4.2 String Configuration

| String Group | Count | Tuning | Role |
|-------------|-------|--------|------|
| Melody strings | 3 | A3, C4, G4 (varies by instrument) | Primary played strings, bowed |
| Drone string | 1 | C3 (typical) | Continuous drone when bowed |
| Sympathetic strings | 12 | Chromatic half-steps | Resonate with played notes, creating "built-in reverb" |

### 4.3 Sympathetic Resonance System

This is the defining feature:
- 12 sympathetic strings tuned chromatically (one per semitone)
- They vibrate in response to frequencies played on the melody/drone strings
- Creates rich, shimmering overtone halo around every note
- The effect is similar to "built-in reverb" but harmonically structured
- Any melody note excites sympathetic strings at unison, octave, and fifth relationships

### 4.4 Tangent Mechanism

Unlike violin fingering:
- Tangent presses perpendicular to the string from below
- Creates a precise, repeatable pitch (like frets)
- Attack transient differs from violin: includes the mechanical "click" of tangent contact
- Portamento is NOT possible (fretted mechanism)
- Vibrato is limited -- possible by wobbling key pressure but much narrower than violin

### 4.5 Acoustic Characteristics

- Sound: violin-like but with much more resonance depth due to sympathetic strings
- The sympathetic strings ring at overtone frequencies of each played note
- Creates a "halo" effect that is harmonically locked to the melody
- Dynamic range similar to violin but with constant sympathetic activity
- Less articulation variety than violin (no true pizzicato, limited col legno)

### 4.6 Modeling Implications

- **Extreme sympathetic string cost**: 12 additional waveguide strings, each needing to respond to the excitation from the melody strings
- **Tangent mechanism**: simpler than bow-string interaction for pitch changes (discrete rather than continuous)
- **Body model**: similar complexity to violin (wooden box with plate modes)
- **Total waveguide count**: 16 strings = 16 coupled delay lines minimum

---

## 5. Bow-String Interaction Model

This is the core nonlinear element shared across all four instruments. Based on the CCRMA digital waveguide framework (Julius O. Smith III).

### 5.1 Architecture

The bow divides the string into two sections. Each section contains left-going and right-going velocity wave components. The bow-string junction is a nonlinear two-port scattering junction.

```
     Nut/Finger                           Bridge
  [Reflection] <-- Left DL --> [BOW] <-- Right DL --> [Reflection + Body]
                               |   |
                          Bow Force, Bow Velocity
```

### 5.2 Friction Model

The fundamental friction equation:

```
f_friction = mu(v_delta) * F_bow
```

where:
- `v_delta` = bow velocity - string velocity at contact point (differential velocity)
- `F_bow` = normal bow force (N)
- `mu(v_delta)` = friction coefficient, a nonlinear function of differential velocity

**Friction curve shape**:
- Static region (|v_delta| < v_capture): mu is constant at static friction coefficient (~0.8)
- Dynamic region (|v_delta| > v_capture): mu drops quickly to dynamic coefficient (~0.3) and continues decreasing
- This creates the classic "stick-slip" alternation (Helmholtz motion)

**Advanced friction models**:
- Elasto-plastic model: models contact as elastic elements with strain/plasticity
- Thermal friction model: accounts for rosin temperature affecting viscosity
- Finite-width bow: simulates distributed contact across bow hair width

### 5.3 Bow Control Parameters

| Parameter | Range (Violin) | Range (Cello) | Effect |
|-----------|----------------|---------------|--------|
| Bow force (F_bow) | 0.5 - 3.0 N | 1.0 - 5.0 N | Controls amplitude, tone brightness, Helmholtz stability |
| Bow velocity (v_bow) | 5 - 50 cm/s | 5 - 40 cm/s | Controls amplitude, interacts with force for tone quality |
| Bow position (beta) | 0.02 - 0.25 | 0.02 - 0.20 | Controls harmonic content (see Schelleng diagram) |
| Bow tilt | 0 - 45 degrees | 0 - 45 degrees | Reduces effective hair width |

### 5.4 Schelleng Diagram

The playable region is a wedge-shaped area in log(bow force) vs. log(bow position) space:
- **Below minimum force**: double-slipping, "surface sound" (airy, unfocused)
- **Above maximum force**: raucous "crunch" (scratchy, overdriven)
- **Within the wedge**: stable Helmholtz motion (normal bowed tone)

Maximum force scales linearly with bow velocity. Minimum force is approximately constant across velocities (contradicting Schelleng's original prediction).

Experimental values (cello G string, beta=0.0786):
- Minimum playable force: ~1 N
- Bow velocities tested: 5, 10, 15, 20 cm/s
- String tension: ~87 N

### 5.5 Erhu Bow Specifics

The erhu bow-string interaction differs:
- Bow hair passes between the two strings
- When playing inner string: hair contacts string directly
- When playing outer string: bow stick pushes hair against string (reversed force direction)
- The unused string still has some coupling through the shared bow hair
- Simpler in some ways (no position variation along string -- the bow always contacts at the same point near the resonator)

---

## 6. Body Resonance Modeling Approaches

### 6.1 Approach Comparison

| Approach | CPU Cost | Accuracy | Flexibility | Best For |
|----------|----------|----------|-------------|----------|
| **Impulse Response Convolution** | Medium-High | Very High (if measured from real instrument) | Low (fixed response) | Capturing specific instrument character |
| **Biquad Filter Bank** | Low-Medium | High (if well-tuned) | High (parametric control) | Real-time parameter tweaking |
| **Modal Synthesis** | Medium | High | Medium | Instruments with well-separated modes |
| **Commuted Synthesis** (Smith) | Low | High | Medium | CPU-efficient string instruments |
| **Hybrid (commuted + parametric)** | Medium | Very High | High | Best overall approach |

### 6.2 Commuted Synthesis (Recommended Core Approach)

Julius Smith's commuted synthesis is the established best practice for bowed strings:
1. Factor body response into **slowly-decaying modes** (parametric recursive filters, NOT commuted) and **rapidly-decaying modes** (commuted as short impulse response data)
2. The commuted impulse response becomes a short, high-frequency noise burst -- very cheap to convolve
3. The long-ringing modes are modeled as parallel biquad sections (few needed: 5-15 for low-frequency modes)
4. The string acts as a "pitch-synchronous comb filter" following excitation

**Why this works**: the body response commutes with the linear string model. You pre-compute the body response into the excitation signal, so you only need the (expensive) nonlinear bow model + the string delay lines + a few parametric resonators for the main modes.

### 6.3 Per-Instrument Body Modeling Recommendations

**Violin Body**:
- 4 parametric biquads for signature modes (A0, CBR, B1-, B1+)
- 1 resonant peak for bridge hill (~2.5 kHz, Q ~2-3)
- 1 low-pass for bridge rolloff above 6 kHz
- Short IR convolution (~50 ms) for high-frequency statistical modes
- Total: ~6 biquads + short convolution

**Cello Body**:
- Same structure as violin but lower frequencies
- 4 parametric biquads for signature modes (A0 ~95 Hz, CBR ~180 Hz, B1- ~175 Hz, B1+ ~570 Hz)
- 1 resonant peak for bridge hill (lower than violin, ~1.5-2 kHz)
- Additional filter for endpin coupling (high-pass shelf modeling floor radiation)
- Wolf tone simulation: coupling filter between string fundamental and MBR
- Total: ~8 biquads + short convolution

**Erhu Body**:
- Fundamentally different: membrane resonator, not wooden box
- Coupled membrane-cavity oscillator model (can be 2-4 resonant sections)
- Formant-like resonances at ~2 kHz and harmonics (odd ratio: 1:3:5:7)
- Open-back radiation model (rear radiation > front radiation)
- Simpler geometry = fewer modes needed, but unique resonance structure
- Total: ~4-6 biquads (membrane-cavity pairs)

**Nyckelharpa Body**:
- Similar to violin body model (wooden box with plate modes)
- Additional sympathetic string coupling (see Section 7)
- Body modes at similar frequencies to viola/small cello
- Total: ~6 biquads + 12 sympathetic waveguides (this is where the CPU goes)

---

## 7. Sympathetic Resonance Modeling

### 7.1 How It Works

When a string is played, its sound radiates through the bridge and body. Other strings that have resonant frequencies at or near harmonics of the played note will begin to vibrate sympathetically. This creates the characteristic "singing" quality of bowed string instruments.

### 7.2 Per-Instrument Sympathetic String Count

| Instrument | Playable Strings | Sympathetic Strings | Total Waveguides Needed |
|------------|-----------------|---------------------|------------------------|
| Violin | 4 | 3 (other open strings) | 4 + 3 = 7 |
| Cello | 4 | 3 (other open strings) | 4 + 3 = 7 |
| Erhu | 2 | 1 (the other string via bow coupling) | 2 + 1 = 3 |
| Nyckelharpa | 4 (3 melody + 1 drone) | 12 (dedicated sympathetic) | 4 + 12 = 16 |

### 7.3 Efficient Implementation

The sympathetic strings are passive -- they receive excitation but are not bowed:
1. Feed attenuated bridge output into each sympathetic string's delay line
2. Each sympathetic string is a simple Karplus-Strong-style waveguide (no bow interaction)
3. Coupling coefficient: ~0.001 to 0.01 of bridge amplitude
4. Only need to compute sympathetic strings that are near resonance (optimization: gate based on frequency proximity)

**CPU Cost Per Sympathetic String**: approximately 0.1-0.3% of a single core at 44.1 kHz. For 12 nyckelharpa strings: 1.2-3.6% additional CPU.

### 7.4 Perceptual Impact

Sympathetic resonance adds:
- Increased sense of "liveness" and acoustic space
- Harmonic richness that changes with each note played
- Subtle pitch-dependent reverb character
- Critical for realism in the nyckelharpa (it IS the instrument's identity)
- Important but not essential for violin/cello (can be a "quality" toggle)

---

## 8. Competitive Landscape

### 8.1 Physical Modeling Bowed String Plugins

| Product | Developer | Approach | Instruments | Price | Quality |
|---------|-----------|----------|-------------|-------|---------|
| **SWAM Solo Strings** | Audio Modeling | Physical modeling (waveguide + behavioral) | Violin, Viola, Cello, Double Bass | ~$150 each / $500 bundle | Industry gold standard |
| **Soliste** | Expressive E | Physical modeling (MPE-focused) | Violin, Viola, Cello, Double Bass | ~$100 each | Strong, MPE-native |
| **String Studio VS-3** | Applied Acoustics | Physical modeling (multi-exciter) | Generic string model | ~$200 | Good for sound design, less realistic |
| **Preparation 2** | Physical Audio | Physical modeling (experimental) | Abstract bowed/struck strings | ~$100 | Sound design, not realistic instruments |
| **Atoms** | Baby Audio | Mass-spring + bow exciter | Abstract | ~$50 | Novelty/sound design |
| **Sakura** | Image-Line | Physical modeling strings | Generic plucked/bowed | Bundled with FL Studio | Basic |

### 8.2 Sample-Based Bowed String Libraries (for context)

Dominant category. Hundreds of options from Spitfire, CSS, Berlin Strings, etc. Sample-based libraries own the "realistic orchestral" market but cannot match physical modeling for real-time expressiveness, continuous control, or novel timbres.

### 8.3 Erhu Plugins (ALL sample-based)

| Product | Developer | Type | Price |
|---------|-----------|------|-------|
| Ample China Erhu | Ample Sound | Sample (7.6 GB) | ~$120 |
| Chang Erhu | Embertone | Sample (1.6 GB, legato) | ~$80 |
| Neo Erhu | Sound Magic | Hybrid synthesis | ~$50 |
| EastWest Silk Road | EastWest | Sample | ~$200 (bundle) |
| Street Erhu | Soundiron | Phrase library | ~$40 |

**No physically modeled erhu exists.** Neo Erhu uses "Music Domain Synthesis" (proprietary hybrid) but is not a waveguide physical model.

### 8.4 Nyckelharpa Plugins

| Product | Developer | Type | Price |
|---------|-----------|------|-------|
| Nyckelharpas | Soniccouture | Sample (Kontakt) | ~$100 |
| Medieval Phrases | Steinberg | Phrase-based | ~$50 |

**No physically modeled nyckelharpa exists.** The Soniccouture library is the only serious option and it is sample-based.

### 8.5 Market Gap Analysis

| Instrument | Physical Modeling Available? | Sample Libraries? | Market Gap |
|------------|-----------------------------|--------------------|------------|
| Violin | Yes (SWAM, Soliste) -- very strong | Hundreds | Small gap -- would need to beat SWAM |
| Cello | Yes (SWAM, Soliste) -- very strong | Hundreds | Small gap -- would need to beat SWAM |
| Erhu | No true physical model | Several (decent quality) | **Large gap** -- physically modeled erhu with continuous portamento would be unique |
| Nyckelharpa | No physical model | 1 (Soniccouture only) | **Very large gap** -- barely any plugins at all |

---

## 9. Feasibility Assessment Matrix

| Factor | Violin | Cello | Erhu | Nyckelharpa |
|--------|--------|-------|------|-------------|
| **Body model complexity** | High (complex plate modes, bridge hill) | High (plate modes + endpin + wolf tone) | Medium (membrane-cavity oscillator, simpler geometry) | High (violin-like body + 12 sympathetic strings) |
| **String model complexity** | Medium (4 strings, well-understood waveguide) | Medium (4 strings, heavier, lower freq) | Low (2 strings, simpler coupling) | Very High (16 strings total, tangent mechanism) |
| **Bow interaction complexity** | High (full Schelleng, position/force/velocity) | High (heavier bow, stronger wolf coupling) | Medium (fixed bow position, hair-between-strings) | Medium (standard bowing, tangent attack hybrid) |
| **Uniqueness in plugin market** | Low (SWAM dominates) | Low (SWAM dominates) | **Very High** (no physical model exists) | **Extremely High** (almost no plugins at all) |
| **Musical expressiveness** | Very High (vibrato, position, dynamics, articulations) | Very High (deep register, wolf tone character) | Very High (portamento, vibrato, voice-like) | High (resonance depth, but limited articulation) |
| **CPU cost estimate** | Medium (~3-5% per voice) | Medium (~3-5% per voice) | Low-Medium (~2-3% per voice) | High (~8-15% per voice due to sympathetic strings) |
| **Reference recordings availability** | Excellent (abundant isolated recordings) | Excellent | Good (growing, some solo recordings) | Limited (niche instrument) |
| **Market demand** | High (but saturated) | High (but saturated) | Medium-High (world music, film scoring, growing) | Low-Medium (niche but passionate community) |
| **Development time estimate** | 4-6 weeks (body model most work) | 4-6 weeks (wolf tone is tricky) | 3-5 weeks (membrane model is novel) | 6-10 weeks (sympathetic string system is costly) |
| **Overall feasibility score** | 7/10 | 7/10 | **9/10** | 5/10 |

---

## 10. Recommendation

### Primary Recommendation: Erhu-First, Multi-Instrument Architecture

**Build O-Bowed with erhu as the flagship instrument, on a shared waveguide engine that can later support violin and cello.**

### Rationale

1. **Market uniqueness is the strongest differentiator.** No physically modeled erhu exists. Competing with SWAM on violin/cello is a losing proposition -- they have years of refinement and brand recognition. An erhu physical model would be a world-first.

2. **Erhu is the most feasible starting point.** Only 2 strings, simpler body geometry (cylindrical membrane vs. complex violin plates), fixed bow position (no beta parameter). The membrane-cavity model is novel but actually well-characterized by recent acoustic research.

3. **Erhu's expressiveness maps perfectly to physical modeling.** The continuous portamento (no frets, no fingerboard) is EXACTLY what physical modeling excels at and what sample libraries struggle with. This is the killer feature: seamless pitch slides that sound natural, not crossfaded samples.

4. **The voice-like quality is a selling point.** The erhu's formant-like membrane resonances create a timbre that composers and film scorers actively seek. "A physically modeled instrument that sings" is a compelling product story.

5. **Growing market.** Chinese/Asian instrument interest in Western music production is increasing. Film scoring (Kung Fu Panda, Mulan, Shang-Chi, etc.) creates demand. World music fusion is a growing genre.

### Architecture for Future Expansion

Design the engine with these abstractions from day one:

```
[Bow Model] --> [String Waveguide(s)] --> [Bridge Filter] --> [Body Resonator] --> [Output]
                      ^                                              |
                      |---- [Sympathetic Coupling] <-----------------
```

- **Bow Model**: shared nonlinear friction engine (same stick-slip for all instruments)
- **String Waveguide**: parameterized by tension, mass/length, damping, stiffness
- **Bridge Filter**: per-instrument 2-4 biquad sections
- **Body Resonator**: pluggable -- membrane model for erhu, plate model for violin/cello
- **Sympathetic Coupling**: optional module, 0-12 additional passive waveguides

This means adding violin/cello later is primarily a matter of:
1. New body resonator preset (different biquad coefficients)
2. String parameter presets (4 strings instead of 2)
3. Bow position (beta) parameter for violin/cello (erhu doesn't need it)

### Implementation Priority

**Phase 1 (MVP): Erhu**
- 2-string waveguide with bow-between-strings model
- Membrane-cavity body resonator (4-6 biquads)
- Continuous portamento via delay-line interpolation
- Vibrato with body-coupling modulation
- Expression: bow force, bow velocity mapped to MIDI CC

**Phase 2 (v1.x): Add Violin**
- 4-string waveguide with bow position parameter
- Violin body resonator (A0, B1-, B1+, bridge hill)
- Sympathetic resonance (3 passive strings)
- Sul ponticello / sul tasto via beta parameter

**Phase 3 (v2.x): Add Cello**
- Lower-frequency body model with wolf tone simulation
- Endpin coupling model
- Heavier bow dynamics

**Phase 4 (v3.x, if demand): Nyckelharpa**
- 16-string system with 12 sympathetic waveguides
- Tangent mechanism modeling
- Drone string support

### What NOT to Build First

- Do NOT start with nyckelharpa. 16 coupled waveguides is a CPU and development time sink, and the market is too niche to justify as the flagship.
- Do NOT try to beat SWAM at violin/cello as the primary goal. Approach those as "also included" instruments in a multi-instrument plugin, not as the selling point.

---

## References and Key Resources

### Academic / Technical
- Smith, J.O. III. "Digital Waveguide Modeling of Bowed Strings." CCRMA, Stanford University. https://ccrma.stanford.edu/~jos/BowedStrings/BowedStrings.pdf
- Smith, J.O. III. "Nonlinear Commuted Synthesis of Bowed Strings." ICMC 1997. https://quod.lib.umich.edu/i/icmc/bbp2372.1997.071
- Jaffe, D. and Smith, J.O. "Performance Expression in Commuted Waveguide Synthesis of Bowed Strings." https://ccrma.stanford.edu/~jos/pdf/JaffeSmith95.pdf
- Gough, C.E. "Violin Acoustics." Acoustics Today, Summer 2016. https://acousticstoday.org/wp-content/uploads/2016/06/Gough.pdf
- "A simple model of the Erhu soundbox." Proceedings of Meetings on Acoustics, AIP Publishing. https://pubs.aip.org/asa/poma/article/35/1/035002/995555
- Guettler, K. "Some typical properties of bowed strings." http://knutsacoustics.com/files/Typical-string-properties.pdf
- Guettler, K. "Schelleng in Retrospect." ISMA 2007. http://knutsacoustics.com/files/schelleng-in-retrospect.pdf
- Euphonics: "Signature modes and formants." https://euphonics.org/5-3-signature-modes-and-formants/
- Euphonics: "Summary of bowed-string behaviour." https://euphonics.org/11-3-0-summary-of-bowed-string-behaviour/
- Smith, J.O. III. "Bowed Strings." Physical Audio Signal Processing (online textbook). https://www.dsprelated.com/freebooks/pasp/Bowed_Strings.html

### Erhu Acoustics
- UBC Physics: "Erhu Acoustics." https://wiki.ubc.ca/Course:PHYS341/Archive/2016wTerm2/Erhu_Acoustics
- "Characterization and modeling of the Erhu." ResearchGate. https://www.researchgate.net/publication/328387884

### Nyckelharpa
- American Nyckelharpa Association: "What is a nyckelharpa?" https://www.nyckelharpa.org/about/what-is-a-nyckelharpa/
- NyckelPhonics: https://www.nyckelphonics.com/nyckelharpa

### Competitive Analysis
- Audio Modeling SWAM Engine: https://audiomodeling.com/swam-engine/
- SWAM Violin: https://audiomodeling.com/strings/swam-violin/
- Soniccouture Nyckelharpas: https://www.soniccouture.com/en/products/g61-nyckelharpas/
- Ample China Erhu: https://www.amplesound.net/en/pro-pd.asp?id=41
- Violin string tensions: https://www.violinwiki.org/wiki/Violin_strings_tension_charts
- Cello string tensions: https://www.aitchisoncellos.com/string-tensions-charts/
