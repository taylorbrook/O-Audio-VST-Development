# MinimalKick - Creative Brief

## Overview

**Type:** Synth
**Core Concept:** Minimal house kick drum synthesizer with sine wave + pitch envelope architecture
**Status:** 💡 Ideated
**Created:** 2025-11-12

## Vision

MinimalKick is a focused kick drum synthesizer designed for modern minimal and tech house production. Built around a classic sine wave oscillator with pitch envelope, it delivers deep, subby kicks that sit perfectly in minimal house tracks. The plugin emphasizes simplicity and immediacy—five essential parameters that shape the fundamental character of house music kick drums: pitch behavior, envelope timing, and saturation for warmth and presence.

The synthesis approach is deliberately minimal: a pure sine wave provides the sub-fundamental, shaped by a pitch envelope that drops from a defined starting point. Drive/saturation adds harmonics and analog warmth, transforming the clinical sine wave into something with character and presence in a mix.

## Parameters

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Sweep | 0-24 semitones | 12 st | Pitch envelope amount (starting pitch offset from MIDI note) |
| Time | 5-500 ms | 50 ms | Pitch envelope decay time (short = clicky, long = laser) |
| Attack | 0-50 ms | 5 ms | Amplitude envelope attack time |
| Decay | 50-2000 ms | 400 ms | Amplitude envelope decay time (sustain always 0) |
| Drive | 0-100% | 20% | Saturation/distortion amount for harmonics and warmth |

## UI Concept

**Layout:** Five rotary knobs in a horizontal row, minimal and clean (730×280px)
**Visual Style:** Vintage hardware aesthetic with warm analog character - dark browns, brass accents, subtle scan lines, and vignette effects
**Key Elements:**
- Large 90px rotary knobs with brass indicator lines
- Uppercase brass/gold typography with wide letter-spacing
- Two-layer knob structure (static body + rotating indicator)
- Skeuomorphic 3D depth with layered shadows
- Focus on immediacy and workflow speed

## Use Cases

- Live performance: Fast MIDI triggering during minimal/tech house DJ sets
- Studio production: Detailed kick sound design for track production and layering
- Rapid sketching: Quick kick sounds for demo tracks and idea generation
- Sound design: Creating custom kick libraries for minimal house productions

## Inspirations

- Modern minimal house and tech house kick drums (deep, subby, controlled)
- Classic drum machine architecture (808/909) with contemporary sound design
- Simplicity and focus over feature bloat

## Technical Notes

**Synthesis Architecture:**
- Single sine wave oscillator as fundamental
- Base pitch controlled via MIDI note (standard synth architecture)
- Exponential pitch envelope (Sweep semitones above base → decays to base over Time)
- AD amplitude envelope with sustain always 0 (kick-appropriate, no Release needed)
- Waveshaping/saturation stage for harmonic content and warmth

**DSP Considerations:**
- Phase-continuous pitch envelope to avoid clicks
- Oversampling for drive/saturation to minimize aliasing
- Efficient single-voice architecture (monophonic, retriggerable)

## Design Evolution

**2025-11-12 - UI Mockup v2:**
- Parameter architecture refined: Pitch (fixed Hz) → Sweep (semitone offset) with MIDI note for base pitch (more flexible)
- Envelope simplified: ADSR → AD with sustain=0 (kick-appropriate, no Release needed)
- Visual aesthetic evolved: Modern minimal → Vintage hardware minimal (vintage-hardware-001 aesthetic)
- Rationale: MIDI note pitch is standard synth architecture, decay-only envelope suits kick drums, vintage warmth complements analog Drive parameter

## Next Steps

- [x] Create UI mockup (v2 finalized 2025-11-12)
- [ ] Start implementation (`/implement MinimalKick`)
