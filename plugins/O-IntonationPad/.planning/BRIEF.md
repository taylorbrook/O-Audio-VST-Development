# O-IntonationPad - Creative Brief

## Overview

**Type:** Synth (Wavetable Pad)
**Core Concept:** Smart harmonic pad synthesizer with 1-note chord generation using just intonation and microtonality
**Status:** 💡 Ideated
**Created:** 2026-01-28

## Vision

O-IntonationPad is a wavetable-based pad synthesizer designed specifically for creating rich, harmonically complex pad textures with proper microtonal and just intonation support. Its defining feature is the "1-note mode" where playing a single note triggers a full chord voicing—but unlike typical harmonizers, the generated voices use mathematically pure intervals from just intonation rather than equal temperament.

This builds on the microtonality foundation established in O-Lyrica, extending those concepts into a chord-aware harmonization engine. The result is pads that resonate with the natural harmonic series, creating textures impossible to achieve with standard equal-tempered synthesizers.

The synth targets diverse use cases: ambient/drone music where pure intervals create meditative qualities, film/game scoring where emotional impact benefits from natural harmonics, electronic production where unique textures differentiate tracks, and experimental/avant-garde composition where microtonal exploration is the primary goal.

## Parameters

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| voiceCount | 2-12 | 5 | Number of harmony voices in 1-note mode |
| complexity | 0-100% | 50% | Chord complexity (triads → 7ths → 9ths → 11ths → 13ths) |
| keyRoot | C-B | C | Root note of current key |
| keyScale | Major/Minor/Modes | Major | Scale/mode for chord generation |
| tuningSystem | JI/Pyth/Hist/Scala/Manual | JI | Active tuning system |
| inversionRandom | 0-100% | 30% | Randomization of chord inversions |
| timingRandom | 0-100ms | 10ms | Voice timing stagger for organic feel |
| detuneRandom | 0-50 cents | 5 cents | Micro-detuning per voice for width |
| wavetablePos | 0-100% | 50% | Position in wavetable |
| lfoRate | 0.01-20 Hz | 0.5 Hz | LFO speed for modulation |
| lfoDepth | 0-100% | 25% | LFO amount to wavetable position |
| attackTime | 1-5000 ms | 500 ms | Amplitude envelope attack |
| releaseTime | 10-10000 ms | 2000 ms | Amplitude envelope release |
| filterCutoff | 20-20000 Hz | 8000 Hz | Low-pass filter frequency |
| masterVolume | -inf to +6 dB | 0 dB | Output level |

## Tuning System Details

**Just Intonation (JI):**
- Pure ratios: 3:2 (fifth), 5:4 (major third), 6:5 (minor third), etc.
- Dynamically calculated based on root note and chord type

**Pythagorean:**
- 3-limit tuning using only powers of 2 and 3
- Medieval/early music character

**Historical Temperaments:**
- Meantone (1/4 comma, 1/6 comma)
- Werckmeister III
- Kirnberger III
- Well-tempered options

**Scala Support:**
- Import .scl files for custom tunings
- Standard format used by microtonal community

**Manual Entry:**
- Per-note cent offset adjustment
- Full 1200-cent octave customization

## Harmonizer Behavior

**1-Note Mode:**
- Single MIDI note triggers full chord
- Chord type determined by scale position (I, ii, iii, IV, V, vi, vii°)
- Voice count and complexity settings shape voicing
- All generated voices tuned according to active tuning system
- Randomization adds organic variation to inversions, timing, and micro-pitch

**Polyphonic Mode:**
- Traditional polyphonic operation
- Each note still benefits from selected tuning system
- No automatic harmonization

## Wavetable Oscillator

**Built-in Wavetables (Pad-Focused):**
- Warm Analog (classic pad shapes)
- Choir/Vocal (formant-rich)
- Strings (bowed character)
- Glass/Bell (harmonic-rich)
- Evolving (morphing textures)
- Organ (sustained tones)
- Ethereal (airy, breathy)
- Dark Matter (low harmonic content)

**Modulation:**
- LFO → Wavetable position for movement
- Envelope → Wavetable position for timbral shaping
- Both can be combined for complex evolution

## Use Cases

- Ambient and drone music with pure harmonic resonance
- Film and game scoring with emotional pad beds
- Electronic production (synthwave, downtempo, chill)
- Experimental/avant-garde microtonal composition
- Meditation and healing music using natural intervals
- World music fusion incorporating non-Western tuning systems

## Inspirations

- O-Lyrica microtonality implementation (internal reference)
- Spectrasonics Omnisphere (wavetable depth)
- Xfer Serum (wavetable morphing)
- U-he Zebra (pad character)
- Scala tuning ecosystem (microtonal standard)
- La Monte Young's just intonation works
- Traditional Indian shruti system

## Technical Notes

**DSP Considerations:**
- Wavetable interpolation for smooth position morphing
- Per-voice pitch calculation based on tuning system ratios
- Anti-aliased wavetable playback to prevent artifacts
- Efficient polyphony management for up to 12 voices × polyphony count
- Scala file parser for .scl import

**Chord Generation Algorithm:**
- Scale-degree aware chord construction
- Extension logic based on complexity parameter
- Voice leading optimization for smooth transitions
- JI ratio calculation relative to played root

**Performance:**
- Target: 12 voices × 8 polyphony = 96 simultaneous oscillators
- Wavetable approach is efficient (table lookup vs. real-time synthesis)
- Consider voice stealing strategy for CPU management

## Next Steps

- [ ] Create UI mockup (`/start O-IntonationPad` → option 3)
- [ ] Start implementation (`/implement O-IntonationPad`)
