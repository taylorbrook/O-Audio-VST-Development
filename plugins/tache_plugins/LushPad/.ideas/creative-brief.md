# LushPad - Creative Brief

## Overview

**Type:** Synth (Instrument)
**Core Concept:** Generative lush pad synthesizer with nested random modulation
**Status:** 💡 Ideated
**Created:** 2025-11-13

## Vision

LushPad is a polyphonic pad synthesizer designed for effortless, constantly evolving textures. The philosophy is "simplicity + deliciousness" - a minimal interface (just 3 knobs) hides a complex generative modulation system underneath.

Each voice consists of 3 subtly detuned sine waves with FM feedback and harmonic saturation. Hidden random LFOs create organic movement in panning, feedback depth, and timbre - with nested modulation where LFO speeds and depths are themselves modulated by other random LFOs. This creates a "generative" quality where the sound never repeats exactly, always evolving and breathing.

The result is a lush, spacious pad that feels alive - perfect for background atmospheres, evolving textures, and sustained harmonic beds that sit beautifully in a mix without demanding attention.

## Parameters

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Timbre | 0.0 - 1.0 | 0.35 | Controls FM feedback depth AND harmonic saturation. Subtle random motion around the set position. Max value carefully tuned to stay musical. |
| Filter Cutoff | 20 Hz - 20000 Hz | 2000 Hz | Low-pass filter brightness. Fixed subtle resonance. Affected by MIDI velocity (softer = darker). |
| Reverb Amount | 0.0 - 1.0 (dry/wet) | 0.4 | Large hall reverb wet/dry mix. Spacious, lush, expansive character. |

## DSP Architecture

**Voice Structure:**
- 8 voices polyphony
- 3 sine wave oscillators per voice
- Detuning: ±5-10 cents (subtle chorus-like thickening)

**Per-Voice Processing:**
- FM feedback on each sine oscillator
- Harmonic saturation (controlled by Timbre)
- Low-pass filter (12dB/octave, fixed subtle resonance)
- Velocity → volume + filter brightness

**Envelope:**
- Slow attack (200-500ms fade-in)
- Long release (1-3 seconds fade-out)
- Classic pad behavior

**Generative Modulation System (Hidden):**
- Random LFOs modulating panning per voice
- Random LFOs modulating FM feedback amount
- Random LFOs modulating saturation amount
- **Nested modulation:** LFO speed modulated by another random LFO
- **Nested modulation:** LFO depth modulated by another random LFO
- Creates constantly evolving, generative texture that never repeats

**Reverb:**
- Large hall reverb (spacious, long decay)
- Applied globally to all voices

## UI Concept

**Layout:** 3 large knobs in a horizontal row
**Visual Style:** Clean, minimal, lush/organic aesthetic (soft gradients, warm colors)
**Key Elements:**
- Large, easy-to-grab knobs for Timbre, Filter Cutoff, Reverb Amount
- Subtle visual feedback showing the hidden modulation activity (e.g., glow, movement, breathing animation)
- Plugin title: "LushPad"

## Use Cases

- Background atmospheric pads in ambient/electronic music
- Evolving harmonic beds for film/game scoring
- Textural layers that add depth without demanding attention
- Sustained chords in worship/cinematic contexts
- Generative soundscapes for meditation/relaxation music

## Inspirations

- FM synthesis pads (smooth, evolving, harmonically rich)
- Generative music concepts (Brian Eno-style evolving systems)
- Classic synth pads with chorus (Juno-style warmth and movement)
- Simplicity of modern "one-knob" plugins (minimal interface, maximum depth)

## Technical Notes

**FM Feedback Implementation:**
- Sine wave phase modulated by its own delayed output
- Small feedback amounts create subtle harmonics without harshness
- Modulation of feedback depth creates timbral evolution

**Nested Random Modulation:**
- Primary random LFOs (affecting panning, feedback, saturation)
- Secondary random LFOs modulating the speed of primary LFOs
- Tertiary random LFOs modulating the depth of primary LFOs
- All LFOs use smooth interpolation (no sudden jumps)
- Frequency ranges carefully tuned to avoid perceivable patterns

**Filter:**
- 12dB/octave low-pass (gentle slope)
- Fixed resonance around 0.3-0.4 (adds character without ringing)
- Velocity scaling: soft notes → cutoff reduced, hard notes → cutoff boosted

**Reverb:**
- Large hall algorithm (1-3 second decay)
- Pre-delay: 20-40ms (adds depth)
- Damping: moderate (keeps it lush without muddiness)

## Next Steps

- [ ] Create UI mockup (`/dream LushPad` → option 3)
- [ ] Start implementation (`/implement LushPad`)
