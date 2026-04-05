---
title: "O-Formant Deep Research: Physical-Model Vocal Synthesizer"
created: 2026-04-04
juce_version: "8.0.4"
summary: "Comprehensive research synthesis for O-Formant — a playable physical-model vocal synthesizer with XY vowel morph pad, glottal pulse control, and consonant noise injection. Covers market gap, DSP architecture, interpolation geometry, signal flow, and JUCE implementation strategy."
domain: dsp
type: research
keywords:
  - formant-synthesis
  - vocal-modeling
  - physical-model
  - xy-pad
  - glottal-pulse
  - consonant-noise
  - klatt
  - mpe
  - juce-dsp
stages: [0]
agents: [research, dsp]
plugin: O-Formant
companion_docs:
  - vocal-formant-synthesis.md
  - 2d-vowel-morph-xy-pad.md
  - consonant-noise-synthesis.md
  - O-Formant-market-research.md
---

# O-Formant: Deep Research Synthesis

**A Playable Physical-Model Vocal Synthesizer**

**Created:** April 2026
**Research Depth:** Level 3 (5 parallel research agents + existing reference material)
**Status:** Pre-planning research — ready for Stage 0 architecture/planning

---

## Executive Summary

O-Formant is a MIDI-playable vocal synthesizer based on the source-filter model of the human voice. It generates sound from scratch — no input audio, no samples, no neural networks. The voice is built from three components:

1. **Source:** LF (Liljencrants-Fant) glottal pulse model with musical quality control
2. **Filter:** 5-formant parallel bandpass bank with 2D XY vowel morph
3. **Noise:** Consonant injection system (fricatives, plosives, sibilants)

**Why this matters:** There are zero commercial plugins in this category. Every "vocal synth" on the market is either a vocoder (needs carrier+modulator), an effects processor (needs input audio), or AI-based (black box). A true parametric vocal instrument with exposed physical controls is a genuine gap.

---

## 1. Market Position

### 1.1 The Gap

| Category | Examples | Requires Input Audio? | Physical Model? | Exposed Parameters? |
|----------|----------|----------------------|-----------------|---------------------|
| Vocoders | Arturia Vocoder V, XILS 5000, TAL | Yes (carrier+modulator) | No | Band levels only |
| Vocal effects | VocalSynth 2, Humanoid, MORPH | Yes | Partial (BioVox) | Limited |
| AI voice | Vocoflex, SynthV | Yes or text | No (neural) | No (black box) |
| Formant filters | The Orb, Modor | Yes | No | Filter params |
| Physical model synths | Chromaphone, Objekt | No | Yes | Yes — but not vocal |
| **O-Formant** | **—** | **No** | **Yes (vocal tract)** | **Yes (glottal, formant, consonant)** |

**Closest existing things:** Pink Trombone (browser toy), Cantor Digitalis (academic), Voc-One (abandoned ~2007 freeware). None are DAW plugins.

### 1.2 Target Audience

- **Sound designers** (film/game): vocal textures, creature voices, sci-fi atmospheres
- **Electronic producers**: vocal pads, choir-like tones, formant sweeps without sampling
- **Experimental/ambient artists**: evolving vocal drones, otherworldly speech
- **Educators/researchers**: demonstrating vocal acoustics interactively

### 1.3 Pricing

$79-99 retail, intro $59-69. Positioned at parity with Baby Audio Humanoid ($79-129).

### 1.4 Key Risks

| Risk | Mitigation |
|------|------------|
| Education gap ("not a vocoder") | Clear marketing, demo videos showing MIDI playback |
| Uncanny valley | Lean into "vocal instrument" not "realistic voice" |
| CPU cost | Formant filter bank approach (not waveguide), 16-voice cap |
| "It sounds like a robot" | LF model quality, breathiness, vibrato, micro-jitter |

---

## 2. DSP Architecture

### 2.1 Source-Filter Model

```
MIDI Note → F0 (fundamental frequency)
              |
              v
        LF Glottal Pulse Model
        (Rd = voice quality control)
              |
              +——→ Aspiration noise mix (breathiness)
              |
              v
        5-Formant Parallel Bandpass Bank
        (coefficients from XY vowel morph + formant shift)
              |
              +——→ Consonant noise injection (fricatives, plosives)
              |
              v
        ADSR Envelope → Voice output (mono)
```

### 2.2 Per-Voice Signal Flow (Detailed)

```
[MIDI/MPE Input]
    |
    v
[Voice Allocation] (MPESynthesiser, 16 voices)
    |
    v
[Per Voice] ──────────────────────────────────────────────
    |                                                      
    v                                                      
  F0 from MPE note ──→ Vibrato LFO ──→ Pitch (+ jitter)   
    |                                                      
    v                                                      
  LF Glottal Pulse (Rd from voice quality param)           
    |                                                      
    +── Aspiration noise (breathiness param)                
    |                                                      
    v                                                      
  CASCADE: Glottal + aspiration ──→ 5 Formant BPFs         
    |        (vowel from XY pad + formant shift + spread)  
    |                                                      
    +── PARALLEL: Consonant noise ──→ Noise shaping filter 
    |   (fricative tone, sibilance level, burst generator) 
    |                                                      
    v                                                      
  Mix cascade + parallel ──→ ADSR envelope ──→ Voice out   
───────────────────────────────────────────────────────────
    |
    v
Sum all voices ──→ Stereo spread (per-voice pan by pitch)
    |
    v
Post-FX: [Chorus] ──→ [Reverb] ──→ [Output Gain]
    |
    v
Stereo output
```

### 2.3 Dual-Branch Topology (KLATT-derived)

Following Klatt 1980, the synth uses two parallel signal paths:

- **Cascade branch** (vowels): Glottal source → 5 series formant filters. Natural spectral valleys, correct vowel resonance.
- **Parallel branch** (consonants): Noise source → parallel formant filters + bypass path. Independent gain per band for shaping fricatives/plosives.

For O-Formant v1, **use parallel topology for both branches** (simpler, more musical control). Cascade can be an option in v2.

---

## 3. XY Vowel Morph Pad

### 3.1 Vowel Space Mapping

The 5 cardinal vowels mapped to normalized [0,1] XY coordinates using acoustic F1/F2 relationships:

- **X axis:** Front (left) to Back (right) — inversely proportional to F2
- **Y axis:** Open (bottom) to Close (top) — inversely proportional to F1

| Vowel | IPA | F1 (Hz) | F2 (Hz) | X | Y |
|-------|-----|---------|---------|------|------|
| I | /i/ | 270 | 2290 | 0.00 | 1.00 |
| E | /e/ | 530 | 1840 | 0.31 | 0.43 |
| A | /a/ | 730 | 1090 | 0.83 | 0.00 |
| O | /o/ | 570 | 840 | 1.00 | 0.35 |
| U | /u/ | 300 | 870 | 0.98 | 0.93 |

```
Y (close)
1.0  I ·                              · U
     |
0.5  |     · E
     |
0.0  |                        · A    · O
     +————————————————————————————————→ X (back)
    0.0          0.5                 1.0
```

### 3.2 Interpolation: Shepard (Modified IDW)

**Recommended method: Shepard interpolation with power p=2.5.**

With only 5 data points, barycentric/Delaunay adds complexity for marginal benefit (triangle edge discontinuities, convex hull edge cases). Shepard is simple, smooth, handles the full XY pad naturally, and the power parameter doubles as a "focus" control.

```cpp
struct VowelPreset {
    float f[5];   // Formant frequencies (Hz)
    float bw[5];  // Bandwidths (Hz)
    float g[5];   // Gains (dB)
};

// 5 cardinal vowels with normalized XY positions
struct VowelPoint {
    float x, y;
    VowelPreset preset;
};

static const VowelPoint vowels[5] = {
    { 0.00f, 1.00f, /* I */ {{270, 2290, 3010, 3400, 4500}, {60,90,100,200,250}, {0,-10,-15,-20,-26}} },
    { 0.31f, 0.43f, /* E */ {{530, 1840, 2480, 3400, 4500}, {70,90,110,200,250}, {0,-8,-12,-18,-24}} },
    { 0.83f, 0.00f, /* A */ {{730, 1090, 2440, 3400, 4500}, {80,90,120,200,250}, {0,-6,-12,-18,-24}} },
    { 1.00f, 0.35f, /* O */ {{570, 840,  2410, 3400, 4500}, {70,80,110,200,250}, {0,-6,-12,-18,-24}} },
    { 0.98f, 0.93f, /* U */ {{300, 870,  2240, 3400, 4500}, {70,80,100,200,250}, {0,-8,-14,-20,-26}} }
};

VowelPreset interpolateVowel(float padX, float padY, float power = 2.5f)
{
    float weights[5];
    float totalWeight = 0.0f;
    
    for (int i = 0; i < 5; ++i)
    {
        float dx = padX - vowels[i].x;
        float dy = padY - vowels[i].y;
        float dist = std::sqrt(dx * dx + dy * dy);
        
        if (dist < 1e-6f)  // Exactly on a vowel point
        {
            return vowels[i].preset;
        }
        
        weights[i] = 1.0f / std::pow(dist, power);
        totalWeight += weights[i];
    }
    
    // Normalize weights
    for (int i = 0; i < 5; ++i)
        weights[i] /= totalWeight;
    
    // Interpolate formants in LOG domain for perceptually linear morphing
    VowelPreset result;
    for (int f = 0; f < 5; ++f)
    {
        float logFreq = 0.0f;
        float bw = 0.0f;
        float gain = 0.0f;
        
        for (int i = 0; i < 5; ++i)
        {
            logFreq += weights[i] * std::log(vowels[i].preset.f[f]);
            bw      += weights[i] * vowels[i].preset.bw[f];  // Linear for bandwidths
            gain    += weights[i] * vowels[i].preset.g[f];    // Linear for dB gains
        }
        
        result.f[f]  = std::exp(logFreq);  // Back to Hz
        result.bw[f] = bw;
        result.g[f]  = gain;
    }
    
    return result;
}
```

### 3.3 Optional "Focus" Parameter

The Shepard power `p` controls vowel selectivity:
- **p = 1.0**: Washy, all vowels bleed together (ambient pad sound)
- **p = 2.5**: Natural, smooth vowel transitions (default)
- **p = 6.0**: Sharp, snaps to nearest vowel (percussive, speech-like)

Exposing this as "Vowel Focus" adds a unique musical control.

### 3.4 Smoothing Strategy

Two-layer smoothing prevents zipper noise:
1. **XY position smoothing:** Exponential filter on pad coordinates (~30ms time constant)
2. **Formant parameter smoothing:** `juce::SmoothedValue` on each formant frequency (~20ms ramp)

Coefficient updates at block rate (every 32 samples), NOT per-sample.

---

## 4. Glottal Pulse Model

### 4.1 LF (Liljencrants-Fant) Model

The LF model is the standard for parametric glottal excitation. It produces the derivative of glottal airflow as a function of normalized phase (0 to 1):

**Key parameter: Rd (voice quality)**
- Rd = 0.3: Very pressed, tense (strong harmonics, flat spectrum)
- Rd = 1.0: Modal voice (normal speaking)
- Rd = 2.7: Breathy voice (steep spectral tilt, high noise)

The Rd parameter controls derived values:
- **OQ (Open Quotient):** Ratio of open phase to total period
- **SQ (Speed Quotient):** Asymmetry of opening vs closing
- **Spectral tilt:** dB/octave roll-off of the harmonic series

```cpp
class LFGlottalSource
{
public:
    void prepare(double sampleRate)
    {
        sr = sampleRate;
    }
    
    // Rd: 0.3 (pressed) to 2.7 (breathy)
    void setVoiceQuality(float Rd)
    {
        // Fant 1995 regression formulas (approximate)
        Rd = juce::jlimit(0.3f, 2.7f, Rd);
        
        float Ra = -0.01f + 0.048f * Rd;                    // Return phase ratio
        float Rk = 0.224f + 0.118f * Rd;                    // Open quotient asymmetry
        float Rg = (0.44f * Rd - 0.18f * Rd * Rd + 0.055f * Rd * Rd * Rd) / 
                   (0.5f + 1.2f * Rk);                      // Glottal frequency ratio
        
        te = 1.0f / (2.0f * Rg);                            // Excitation time
        tp = te / (1.0f + Rk);                               // Peak time
        ta = Ra;                                              // Return time
        
        // Spectral tilt increases with Rd
        tiltDB = -6.0f * (Rd - 0.5f);  // Approximate: breathy → more tilt
    }
    
    float processSample(float frequency)
    {
        float T0 = (float)sr / frequency;
        float output = 0.0f;
        
        if (phase < te)
        {
            // Opening + closing phase: sinusoidal with exponential modification
            float t = phase / te;
            output = std::sin(juce::MathConstants<float>::pi * t * tp / te)
                   * std::exp(-alpha * t);
        }
        else
        {
            // Return phase: exponential recovery
            float t = (phase - te) / (1.0f - te);
            output = -returnAmp * std::exp(-epsilon * t);
        }
        
        phase += 1.0f / T0;
        if (phase >= 1.0f) phase -= 1.0f;
        
        return output;
    }
    
    float getSpectralTiltDB() const { return tiltDB; }

private:
    double sr = 44100.0;
    float phase = 0.0f;
    float te = 0.6f;        // Excitation time (normalized)
    float tp = 0.4f;        // Peak time (normalized)
    float ta = 0.05f;       // Return time ratio
    float alpha = 3.0f;     // Opening phase tilt
    float epsilon = 10.0f;  // Return phase steepness
    float returnAmp = 0.3f; // Return phase amplitude
    float tiltDB = -12.0f;  // Spectral tilt
};
```

### 4.2 Musical Glottal Parameters

| Parameter | Range | Musical Effect | Priority |
|-----------|-------|----------------|----------|
| **Voice Quality (Rd)** | 0.3-2.7 | Pressed → modal → breathy | Essential |
| **Breathiness** | 0-100% | Aspiration noise mix | Essential |
| **Vibrato Rate** | 0.5-12 Hz | LFO speed on F0 | Essential |
| **Vibrato Depth** | 0-100 cents | LFO amount on F0 | Essential |
| **Vibrato Delay** | 0-2000 ms | Onset delay after note-on | Nice to have |
| **Jitter** | 0-5% | F0 random perturbation | Creative (subtle realism or extreme effect) |
| **Shimmer** | 0-5% | Amplitude random perturbation | Creative |

### 4.3 Anti-Aliasing

At high pitches (>500 Hz), naive glottal pulse generation aliases. Solutions (ranked):

1. **Pre-computed mipmapped wavetable (recommended):** Following the GOLF paper — pre-compute 128 Rd steps x 2048 samples, mipmapped across pitch octaves. ~1MB memory, bilinear interpolation for real-time. ~90dB alias rejection at ~1.1x CPU cost. See `glottal-pulse-modeling-deep-dive.md` for full detail.
2. **PolyBLEP + PolyBLAMP correction:** Apply polynomial correction at Te discontinuity (PolyBLEP) and return-phase slope change (PolyBLAMP). Low cost, good for prototyping.
3. **Oversampling (2x):** Simple fallback. Process glottal source at 2x sample rate, downsample with LP filter.

**Recommendation for v1:** Start with PolyBLEP for quick iteration, upgrade to mipmapped wavetable for release quality.

### 4.4 Creative Extensions

Beyond realistic voice modeling:
- **Rd extremes:** Push Rd below 0.3 → pulse-like, almost saw wave. Push above 2.7 → pure noise.
- **Glottal source as oscillator:** The LF model at extreme settings produces saw, pulse, and noise — effectively a morphable oscillator.
- **Formant sync:** Optional mode where formants auto-tune to align with harmonics (singer's formant effect).

---

## 5. Consonant Noise System

### 5.1 Architecture

Consonants use the **parallel branch** separate from the vowel cascade:

```
Noise Source (white) ──→ Shaping Filter ──→ Parallel Formants ──→ mix with vowel
                              |
                         (tone control:
                          LP = dark /f/
                          HP = bright /s/)
```

### 5.2 Consonant Types and Synthesis

| Type | Examples | Synthesis | Key Frequencies |
|------|----------|-----------|-----------------|
| **Sibilant fricatives** | /s/, /z/ | HP noise 4-8 kHz dual peak | 4500 + 7500 Hz |
| **Palatal fricatives** | /sh/, /zh/ | BP noise 2-4 kHz | 3500 + 6000 Hz |
| **Non-sibilant fricatives** | /f/, /v/, /th/ | Flat noise, bypass path | Broadband, gentle LP |
| **Plosive bursts** | /p/, /b/, /t/, /d/, /k/, /g/ | Transient noise burst 5-25ms | Varies by place of articulation |
| **Nasals** | /m/, /n/, /ng/ | Anti-formant (zero) in cascade | Zero at 750-3000 Hz |

### 5.3 Musical Control Mapping

Rather than exposing individual phonemes, O-Formant uses musical parameters:

**Essential (v1):**
| Parameter | Range | What It Does |
|-----------|-------|-------------|
| **Consonant Level** | 0-100% | Overall consonant noise mix |
| **Consonant Tone** | 0-100% | Dark (low fricative /f/) to bright (sibilant /s/) |

**Expressive:**
| Parameter | Range | What It Does |
|-----------|-------|-------------|
| **Sibilance** | 0-100% | High-frequency /s/ and /sh/ emphasis |
| **Auto-Consonant** | On/Off | Inject consonant burst on note attack (plosive onset) |
| **Burst Strength** | 0-100% | Plosive burst intensity on note-on |

**MIDI mapping:**
- Mod wheel → Consonant Level
- MIDI CC74 (brightness) → Consonant Tone
- Velocity → Burst Strength (gated: only above velocity threshold)

### 5.4 Consonant-Vowel Interaction

When "Auto-Consonant" is enabled:
1. Note-on triggers a 10-25ms plosive burst (noise shaped by Consonant Tone)
2. Burst fades as ADSR attack rises
3. Crossfade into steady-state vowel formants
4. Creates natural "consonant-vowel" articulation without explicit phoneme selection

---

## 6. Polyphonic Architecture

### 6.1 Core Classes

**Use `juce::MPESynthesiser` + `juce::MPESynthesiserVoice`** (not basic Synthesiser).

Reasons:
- Built-in MPE support (per-note pitch, pressure, slide)
- Falls back to standard MIDI with `enableLegacyMode()`
- Voice stealing, allocation already handled
- `getFrequencyInHertz()` auto-combines note + pitchbend + master bend

### 6.2 Voice Budget

Each voice costs ~95-100 floating-point ops/sample:
- LF glottal pulse: ~15-20 ops
- 5 biquad formant filters: ~50 ops
- Noise gen + shaping: ~15 ops
- ADSR + smoothing: ~15 ops

At 48 kHz, 16 voices = ~77 MFLOPS = ~1.5% of a single CPU core. **16 voices default, 32 max.**

### 6.3 MPE Dimension Mapping

| MPE Dimension | Target | Rationale |
|---------------|--------|-----------|
| Per-note pitchbend | F0 (pitch) | Direct |
| Per-note pressure | Breathiness | Physical: pressure = breath force |
| Per-note timbre (slide) | Vowel Y-axis | Finger position = timbral control |
| Velocity | Attack character + burst strength | Impact = vocal effort |

### 6.4 Formant Filter Implementation

Custom biquad struct for cache locality (avoid JUCE ProcessorState overhead):

```cpp
struct FormantBiquad
{
    float b0, b1, b2, a1, a2;
    float z1 = 0.0f, z2 = 0.0f;
    float gain = 1.0f;
    
    void setCoeffs(double sampleRate, float freq, float bw, float gaindB)
    {
        float Q = freq / juce::jmax(bw, 1.0f);
        auto c = juce::dsp::IIR::ArrayCoefficients<float>::makeBandPass(sampleRate, freq, Q);
        b0 = c[0] / c[3]; b1 = c[1] / c[3]; b2 = c[2] / c[3];
        a1 = c[4] / c[3]; a2 = c[5] / c[3];
        gain = juce::Decibels::decibelsToGain(gaindB);
    }
    
    float processSample(float x) noexcept
    {
        float y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y * gain;
    }
    
    void reset() { z1 = 0.0f; z2 = 0.0f; }
};
```

Update coefficients at **block rate** (every 32 samples), not per-sample. The trig in `makeBandPass` costs ~50ns — at 5 filters x 16 voices x 1500 blocks/sec = 120K calls/sec = ~6ms/sec. Negligible.

---

## 7. APVTS Parameter Layout

### 7.1 Complete Parameter List (22 parameters)

```
=== VOWEL MORPH ===
vowelX          | float | 0.0-1.0   | default 0.5  | XY pad horizontal
vowelY          | float | 0.0-1.0   | default 0.5  | XY pad vertical
vowelFocus      | float | 1.0-6.0   | default 2.5  | Shepard interpolation power

=== GLOTTAL SOURCE ===
glottalRd       | float | 0.3-2.7   | default 1.0  | Voice quality (pressed→breathy)
breathiness     | float | 0.0-1.0   | default 0.1  | Aspiration noise mix
vibratoRate     | float | 0.5-12.0  | default 5.5  | Vibrato LFO Hz
vibratoDepth    | float | 0.0-100.0 | default 15.0 | Vibrato cents
vibratoDelay    | float | 0.0-2000  | default 300  | Vibrato onset delay ms

=== CONSONANT / NOISE ===
consonantLevel  | float | 0.0-1.0   | default 0.3  | Overall consonant mix
consonantTone   | float | 0.0-1.0   | default 0.5  | Dark↔bright noise filter
sibilance       | float | 0.0-1.0   | default 0.0  | High-frequency emphasis
autoConsonant   | bool  | on/off    | default off  | Plosive burst on note attack

=== ENVELOPE ===
attack          | float | 0.001-5.0 | default 0.01 | seconds (skewed fast)
decay           | float | 0.001-5.0 | default 0.3  | seconds
sustain         | float | 0.0-1.0   | default 0.8  | level
release         | float | 0.001-10  | default 0.5  | seconds

=== VOICE CHARACTER ===
formantShift    | float | -24-+24   | default 0    | Semitones (gender knob)
formantSpread   | float | 0.5-2.0   | default 1.0  | Formant spacing multiplier
pitchGlide      | float | 0.0-1000  | default 0    | Portamento ms

=== OUTPUT ===
outputGain      | float | -60-+12   | default 0    | dB
stereoWidth     | float | 0.0-1.0   | default 0.5  | Voice pan spread
reverbMix       | float | 0.0-1.0   | default 0.15 | Built-in reverb
```

**All parameters are global** (shared across voices). Per-voice expression comes from MPE MIDI.

### 7.2 Parameter Smoothing

| Parameter Group | Smoothing | Time |
|----------------|-----------|------|
| Vowel X/Y, breathiness, vibrato depth | SmoothedValue (linear) | 20ms |
| Formant shift, gain | SmoothedValue (linear) | 10ms |
| ADSR values | None (envelope handles smoothing) | — |
| Consonant level/tone | SmoothedValue (linear) | 10ms |

---

## 8. Implementation Strategy

### 8.1 Recommended Staging

| Stage | Deliverable | Core Work |
|-------|-------------|-----------|
| **Stage 1: Foundation** | Building, APVTS, basic UI scaffold | CMakeLists, 22 params, MPESynthesiser skeleton |
| **Stage 2: DSP Core** | Playable vowel synth | LF glottal model + 5-formant bank + XY morph + ADSR |
| **Stage 3: Consonants** | Noise injection system | Parallel branch, fricative/plosive/sibilance, auto-consonant |
| **Stage 4: Polish** | Full UI, effects, MPE | XY pad widget, vibrato, formant shift, reverb, stereo spread |

### 8.2 Critical DSP Decisions

| Decision | Recommendation | Rationale |
|----------|---------------|-----------|
| Formant topology | Parallel (v1) | More musical control, simpler, correct for consonants |
| Formant filter type | Custom biquad (BPF) | Cache-local, no JUCE overhead per voice |
| Glottal model | LF with Rd parameter | Standard, musically controllable, covers pressed→breathy |
| Anti-aliasing | 2x oversample glottal only | Cheap, effective at high pitches |
| Vowel interpolation | Shepard p=2.5 | Simple, smooth, no edge cases, tunable focus |
| Coefficient updates | Block-rate (32 samples) | Avoids per-sample trig, still smooth |
| Voice framework | MPESynthesiser | MPE for free, standard MIDI fallback |
| Polyphony | 16 default | ~1.5% CPU single core at 48kHz |

### 8.3 Complexity Assessment

**Medium-high complexity** — comparable to a wavetable synth with filters.

- **Novel DSP:** LF model, vowel interpolation, consonant noise shaping (moderate)
- **UI complexity:** XY pad widget, formant visualizer (moderate)
- **Architecture:** Standard JUCE synth pattern with MPE (straightforward)
- **Performance risk:** Low — formant BPFs are cheap, voice count is bounded

---

## 9. UI Concept Notes

### 9.1 Core Layout

```
+—————————————————————————————————————————————————+
|  O-FORMANT                              [menu]  |
+—————————————————————————————————————————————————+
|                    |                             |
|   XY VOWEL PAD    |    GLOTTAL                  |
|                    |    [Rd/Quality]             |
|   I ·        · U  |    [Breathiness]            |
|      · E          |    [Vibrato Rate/Depth]     |
|          · A  · O |                             |
|                    |    CONSONANT                |
|   [Focus]         |    [Level] [Tone]           |
|                    |    [Sibilance]              |
+————————————————————+    [Auto-consonant]        |
|                    |                             |
|   ENVELOPE         |    CHARACTER                |
|   [A] [D] [S] [R] |    [Formant Shift]          |
|                    |    [Formant Spread]         |
+————————————————————+    [Glide]                 |
|                    |                             |
|   OUTPUT           |    [Stereo] [Reverb]        |
|   [Gain]           |                             |
+—————————————————————————————————————————————————+
```

### 9.2 XY Pad Widget

- Large, central, touchable/draggable
- 5 vowel labels at their acoustic positions
- Cursor with trailing glow
- Optional: formant frequency display overlay (real-time bars showing F1-F5)
- Optional: spectral analyzer background showing the formant peaks

---

## 10. Companion Research Documents

This synthesis draws from 5 detailed research documents:

| Document | Coverage |
|----------|----------|
| `vocal-formant-synthesis.md` | Foundation: source-filter model, formant filter banks, KLATT, FOF, vowel presets, JUCE implementation examples |
| `2d-vowel-morph-xy-pad.md` | XY pad geometry, Shepard/IDW/barycentric comparison, log-frequency interpolation, smoothing, implementation code |
| `consonant-noise-synthesis.md` | Fricative filter specs, plosive burst timing, nasal anti-formants, sibilance modeling, musical control mapping, CPU analysis |
| `O-Formant-market-research.md` | Competitive landscape (vocoders, effects, AI, formant filters), gap validation, pricing, target audience, risk analysis |
| `glottal-pulse-modeling-deep-dive.md` | LF model time-domain equations, Rd-to-R-parameter Fant 1995 regression, Newton-Raphson solvers, pre-computed wavetable (GOLF: 128 Rd x 2048 samples), PolyBLEP/PolyBLAMP anti-aliasing, mipmapped wavetables, subharmonic/growl, formant-pitch alignment |

---

## 11. References

### Academic
- Klatt, D.H. (1980). "Software for a cascade/parallel formant synthesizer." JASA 67(3).
- Fant, G. (1995). "The LF-model revisited. Transformations and frequency domain analysis." Speech Trans. Lab. Quarterly Progress and Status Report, 36(2-3).
- Rodet, X. (1984). "Time-Domain Formant-Wave-Function Synthesis." Computer Music Journal.
- Peterson & Barney (1952). "Control methods used in a study of the vowels." JASA 24(2).
- Hillenbrand et al. (1995). "Acoustic characteristics of American English vowels." JASA 97(5).

### Software
- Pink Trombone (Neil Thapen) — browser-based vocal tract model
- PaulBatchelor/voc — C port of Pink Trombone
- eSpeak — open-source formant speech synthesizer
- Praat — acoustic analysis with formant tracking

### Books
- Cook, P.R. (2002). *Real Sound Synthesis for Interactive Applications*. Ch. 9: The Voice.
- Smith, J.O. *Physical Audio Signal Processing*. Ch: Vocal Tract Models.

---

*O-Formant research synthesis. Ready for Stage 0 planning (architecture.md + plan.md).*
