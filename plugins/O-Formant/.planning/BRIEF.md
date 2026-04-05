# O-Formant - Creative Brief

## Overview

**Type:** Synth (MIDI Instrument)
**Core Concept:** A playable physical-model vocal synthesizer built from the source-filter model — generates voice from scratch using LF glottal excitation, 5-formant parallel bandpass filtering, and consonant noise injection. NOT a vocoder, NOT sample-based, NOT an effect.
**Status:** Ideated
**Created:** 2026-04-04

## Vision

O-Formant is a true parametric vocal instrument with no commercial equivalent. Every "vocal synth" on the market is either a vocoder (needs carrier+modulator), an effects processor (needs input audio), or AI-based (black box). O-Formant generates voice entirely from DSP primitives — a glottal pulse source shaped by a formant filter bank — giving sound designers and producers direct, musical control over every aspect of the voice.

The synthesizer is built on three pillars:

1. **Source:** LF (Liljencrants-Fant) glottal pulse model with Rd voice quality control spanning pressed (Rd=0.3) through modal (Rd=1.0) to breathy (Rd=2.7). At extremes, the LF model morphs from a near-saw wave to pure noise, making it a versatile morphable oscillator.

2. **Filter:** 5-formant parallel bandpass bank with a 2D XY vowel morph pad mapping the 5 cardinal vowels (A, E, I, O, U) at their acoustic positions. Shepard interpolation (p=2.5) with log-frequency domain blending creates smooth, perceptually linear vowel transitions. A "Vowel Focus" parameter controls interpolation sharpness — washy ambient blends at low values, snapping vowel articulation at high values.

3. **Noise:** Consonant injection via KLATT-derived dual-branch topology. A parallel noise branch shapes fricatives, plosives, and sibilants through musical controls (level, tone, sibilance) rather than phoneme selection. An "Auto-Consonant" mode fires plosive bursts on note attacks for natural consonant-vowel articulation.

MPE support maps physical gestures to vocal expression: per-note pressure controls breathiness, per-note slide morphs the vowel Y-axis. Full MIDI C0-C8 range for everything from subsonic bass drones to ultrasonic textures.

## Parameters

### Vowel Morph (3)

| Parameter | ID | Range | Default | Description |
|-----------|-----|-------|---------|-------------|
| Vowel X | vowelX | 0.0-1.0 | 0.5 | XY pad horizontal (front/back) |
| Vowel Y | vowelY | 0.0-1.0 | 0.5 | XY pad vertical (open/close) |
| Vowel Focus | vowelFocus | 1.0-6.0 | 2.5 | Shepard interpolation power (washy to snappy) |

### Glottal Source (5)

| Parameter | ID | Range | Default | Description |
|-----------|-----|-------|---------|-------------|
| Voice Quality | glottalRd | 0.3-2.7 | 1.0 | Rd parameter (pressed -> modal -> breathy) |
| Breathiness | breathiness | 0.0-1.0 | 0.1 | Aspiration noise mix |
| Vibrato Rate | vibratoRate | 0.5-12.0 Hz | 5.5 | Vibrato LFO speed |
| Vibrato Depth | vibratoDepth | 0.0-100.0 cents | 15.0 | Vibrato LFO amount |
| Vibrato Delay | vibratoDelay | 0.0-2000 ms | 300 | Onset delay after note-on |

### Consonant / Noise (4)

| Parameter | ID | Range | Default | Description |
|-----------|-----|-------|---------|-------------|
| Consonant Level | consonantLevel | 0.0-1.0 | 0.3 | Overall consonant noise mix |
| Consonant Tone | consonantTone | 0.0-1.0 | 0.5 | Dark (/f/) to bright (/s/) noise filter |
| Sibilance | sibilance | 0.0-1.0 | 0.0 | High-frequency /s/ and /sh/ emphasis |
| Auto-Consonant | autoConsonant | on/off | off | Plosive burst on note attack |

### Envelope (4)

| Parameter | ID | Range | Default | Description |
|-----------|-----|-------|---------|-------------|
| Attack | attack | 0.001-5.0 s | 0.01 | Attack time (skewed fast) |
| Decay | decay | 0.001-5.0 s | 0.3 | Decay time |
| Sustain | sustain | 0.0-1.0 | 0.8 | Sustain level |
| Release | release | 0.001-10.0 s | 0.5 | Release time |

### Voice Character (3)

| Parameter | ID | Range | Default | Description |
|-----------|-----|-------|---------|-------------|
| Formant Shift | formantShift | -24 to +24 st | 0 | Gender knob (semitones) |
| Formant Spread | formantSpread | 0.5-2.0 | 1.0 | Formant spacing multiplier |
| Pitch Glide | pitchGlide | 0.0-1000 ms | 0 | Portamento time |

### Output (2)

| Parameter | ID | Range | Default | Description |
|-----------|-----|-------|---------|-------------|
| Output Gain | outputGain | -60 to +12 dB | 0 | Master output level |
| Stereo Width | stereoWidth | 0.0-1.0 | 0.5 | Per-voice pan spread by pitch |

**Total: 21 parameters** (all global, shared across voices; per-voice expression via MPE)

## UI Concept

**Layout:** Two-column — large XY vowel morph pad on the left, parameter groups stacked on the right (Glottal, Consonant, Character). ADSR and Output along the bottom.

**Key Elements:**
- 2D XY Vowel Morph Pad (central, large, draggable) with 5 vowel labels at acoustic positions
- Formant peaks overlay (F1-F5 real-time frequency bars on the XY pad)
- Cursor with trailing glow on the pad

## Use Cases

- **Film/game sound design:** Creature voices, alien vocalizations, sci-fi atmospheres, monster growls
- **Electronic production:** Vocal pads, choir-like tones, formant sweeps without sampling
- **Experimental/ambient:** Evolving vocal drones, otherworldly speech, textural soundscapes
- **Education:** Demonstrating vocal acoustics, formant relationships, source-filter model interactively

## Inspirations

- **Academic:** Klatt 1980 formant synthesizer, Fant 1995 LF model
- **Software:** Pink Trombone (Neil Thapen), Cantor Digitalis (academic), eSpeak
- **Commercial gap:** No DAW plugin exists in this category — vocoders, vocal effects, and AI tools all require input audio

## Technical Notes

### Architecture
- **Voice framework:** `juce::MPESynthesiser` + `juce::MPESynthesiserVoice` with `enableLegacyMode()` fallback
- **Polyphony:** 16 voices default (32 max) — ~95-100 FLOPS/sample/voice, ~1.5% single core at 48kHz
- **Formant filters:** Custom biquad structs (cache-local, no JUCE ProcessorState overhead)
- **Coefficient updates:** Block-rate every 32 samples (avoids per-sample trig)
- **Formant topology:** Parallel for v1 (simpler, more musical control)

### DSP Pipeline (Per-Voice)
```
MIDI/MPE → Voice Allocation (MPESynthesiser, 16 voices)
  Per Voice:
    F0 from MPE note → Vibrato LFO → Pitch (+ jitter)
    → LF Glottal Pulse (Rd from voice quality)
    + Aspiration noise (breathiness)
    → 5 Formant BPFs (vowel from XY pad + formant shift + spread)
    + Consonant noise → Noise shaping filter (parallel branch)
    → Mix cascade + parallel → ADSR envelope → Voice out (mono)
  Sum all voices → Stereo spread (per-voice pan by pitch)
  → Output Gain → Stereo output
```

### Anti-Aliasing Strategy
- **Prototyping:** PolyBLEP + PolyBLAMP correction at LF glottal discontinuities
- **Release quality:** Pre-computed mipmapped wavetable (128 Rd x 2048 samples, bilinear interpolation, ~90dB alias rejection)

### Vowel Interpolation
- Shepard interpolation (modified IDW) with power p=2.5
- Log-frequency domain for formant frequencies (perceptually linear)
- Linear interpolation for bandwidths and gains (dB)
- Two-layer smoothing: XY position (~30ms) + formant parameters (~20ms)

### Consonant System
- KLATT dual-branch topology (cascade for vowels, parallel for consonants)
- Musical parameter mapping (level/tone/sibilance) rather than phoneme selection
- Auto-consonant: 10-25ms plosive burst on note-on, crossfading into vowel ADSR

### MPE Mapping
| MPE Dimension | Target | Rationale |
|---------------|--------|-----------|
| Per-note pitchbend | F0 (pitch) | Direct |
| Per-note pressure | Breathiness | Physical: pressure = breath force |
| Per-note timbre (slide) | Vowel Y-axis | Finger position = timbral control |
| Velocity | Attack character + burst strength | Impact = vocal effort |

### Preset Strategy
Genre packs (~16+ presets) grouped by use case:
- **Cinematic:** Creature growl, alien whisper, sci-fi choir, spectral voice
- **Electronic:** Formant bass, vowel pad, glitch vocal, robotic speech
- **Ambient:** Ethereal drone, breath texture, overtone chant, wind voice
- **Speech:** Natural tenor, breathy soprano, pressed baritone, child voice

### Pitch Range
Full MIDI C0-C8 — unrestricted for creative use (bass drones to ultrasonic squeals).

### Deferred to Future Versions
- Built-in reverb and chorus effects (v1.1+)
- Cascade formant topology option (v2)
- Formant sync mode (harmonics alignment)
- Subharmonic/growl synthesis

## Research Documents

| Document | Path |
|----------|------|
| Master synthesis | `research/O-Formant-deep-research.md` |
| Market validation | `research/O-Formant-market-research.md` |
| Formant foundations | `research/vocal-formant-synthesis.md` |
| XY pad geometry | `research/2d-vowel-morph-xy-pad.md` |
| Consonant system | `research/consonant-noise-synthesis.md` |
| Glottal pulse model | `research/glottal-pulse-modeling-deep-dive.md` |

## Next Steps

- [ ] Create UI mockup (`/start O-Formant` -> option 3)
- [ ] Start implementation (`/implement O-Formant`)
