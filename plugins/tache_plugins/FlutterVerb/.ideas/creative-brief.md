# FlutterVerb - Creative Brief

## Overview

**Type:** Effect (Reverb)
**Core Concept:** Tape-driven plate reverb with extreme wow and flutter modulation for textured, analog-sounding spaces
**Status:** 💡 Ideated
**Created:** 2025-11-11

## Vision

FlutterVerb is a plate reverb processor that combines modern algorithmic clarity with vintage tape modulation character. The wet signal passes through tape-style wow and flutter modulation controlled by a single "Age" parameter, creating warbling, pitch-shifted reverb tails that add movement and texture to static sources. An adjustable tape saturation/drive stage adds harmonic warmth and analog coloration. The DJ-style filter (borrowed from GainKnob's implementation) allows quick tonal shaping—sweep left for low-pass (bass-heavy), right for high-pass (bright and airy), center for full range.

Designed for adding excitement to musical elements like pianos, guitars, flutes, and vocals, FlutterVerb makes it easy to create lush, evolving spaces without complex parameter tweaking. The minimal control set (Mix, Size, Decay, Age, Tone, Drive) keeps the workflow fast while offering wide sonic range—from subtle room ambience to extreme warped textures.

## Parameters

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Mix | 0-100% | 25% | Dry/wet blend of reverb signal |
| Size | 0-100% | 50% | Room dimensions (affects early reflections and perceived space size) |
| Decay | 0.1s - 10s | 2.5s | Reverb tail length (wide range for tight spaces to ambient washes) |
| Age | 0-100% | 20% | Tape character intensity (combines wow and flutter modulation depth for vintage tape feel) |
| Tone | -100 to +100 | 0 | DJ-style filter (center=full, left=low-pass, right=high-pass) based on GainKnob implementation |
| Drive | 0-100% | 20% | Tape saturation/warmth amount (adjustable analog coloration) |
| Mod Mode | Toggle | Wet Only | Button toggle: "Wet Only" or "Wet + Dry" (applies modulation to dry signal too) |

## UI Concept

**Layout:** Clean 6-knob + 1-toggle interface arranged in logical signal flow order
**Visual Style:** Vintage tape aesthetic with warm sepia/cream colors, VU meter visual feedback
**Key Elements:**
- Six rotary chicken-head pointer knobs arranged in two rows
  - Top row: Size, Decay, Mix (reverb controls)
  - Bottom row: Age, Drive, Tone
- Flip switch toggle for Mod Mode (Wet Only / Wet + Dry)
- VU meter centered at top (horizontal needle style)
- Typewriter/monospace typography for labels
- 700x400px window size (wide horizontal layout)

## Use Cases

- **Textured lead vocals:** Add evolving, warbling space that sits vocals in a unique sonic environment
- **Piano and keys:** Create lush, moving reverb tails that complement harmonic content
- **Guitar ambience:** Vintage spring-like character with controllable wobble for surf/indie tones
- **Mix bus texture:** Subtle settings add cohesive analog glue and spatial depth to entire mix
- **Creative sound design:** Extreme wow/flutter settings for pitch-shifted, experimental textures

## Inspirations

- **Modern digital plate reverbs** (clean algorithmic base)
- **Tape echo units** (Echoplex, Space Echo) for wow/flutter modulation character
- **Vintage studio plate reverbs** (EMT lineage) for smooth, dense tail
- **DJ mixer kill/filter controls** (quick tonal shaping, already implemented in GainKnob)

## Technical Notes

**DSP Approach:**
- Plate reverb algorithm (likely using juce::dsp::Reverb or custom Freeverb-style implementation)
- Wow and flutter implemented as dual LFO pitch modulation (different rates for wow vs flutter)
- Modulation routing: user-selectable wet-only or wet+dry paths
- Tape saturation: soft-clipping waveshaper with harmonic enhancement (adjustable drive)
- DJ-style filter: reference GainKnob's exponential mapping for low-pass/high-pass crossfade
  - Implementation at `/Users/lexchristopherson/Developer/plugin-freedom-system/plugins/GainKnob/Source/PluginProcessor.cpp:86-130`
  - Exponential cutoff mapping with state reset on filter type change
  - Low-pass: -100% = 200Hz, 0% = 20kHz
  - High-pass: 0% = 20Hz, +100% = 10kHz

**Architecture:**
- Signal flow: Input → Reverb → Wow/Flutter Mod → Tape Drive → Tone Filter → Mix → Output
- Mod Mode toggle routes modulation pre- or post-dry/wet mixer
- Size parameter affects early reflection density and spacing (room dimensions)

## Next Steps

- [ ] Create UI mockup (`/dream FlutterVerb` → option 3)
- [ ] Start implementation (`/implement FlutterVerb`)
