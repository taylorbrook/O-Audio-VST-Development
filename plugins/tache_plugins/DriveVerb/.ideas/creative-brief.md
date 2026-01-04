# DriveVerb - Creative Brief

## Overview

**Type:** Effect (Reverb)
**Core Concept:** Algorithmic reverb with warm tape saturation on wet signal only
**Status:** 💡 Ideated
**Created:** 2025-11-11

## Vision

DriveVerb is a creative reverb plugin designed for adding driven, saturated reverb tails to audio sources. The plugin features an algorithmic reverb engine with independently controllable size and decay parameters, ranging from tight room ambience to expansive cathedral spaces. The defining characteristic is warm tape saturation that affects only the wet (reverb) signal, preserving the dry signal's clarity while adding harmonic richness and character to the reverb tail.

A DJ-style filter knob provides creative tone shaping of the reverb, with center position bypassing the filter, negative values applying progressively aggressive low-pass filtering (keeping bass), and positive values applying high-pass filtering (keeping treble). A switch allows the filter to be placed either before or after the drive stage, enabling both "clean reverb then filter" and "filtered reverb then drive" signal paths.

Primary use case is adding aggressive, characterful reverb to sources where standard clean reverb would be too polite—ideal for drums, synths, and creative sound design.

## Parameters

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Dry/Wet | 0-100% | 30% | Blend between dry and wet signal |
| Drive | 0-24dB | 6dB | Tape saturation amount (wet signal only, even harmonics) |
| Decay | 0.5s-10s | 2s | Reverb tail length |
| Size | 0-100% | 40% | Room dimensions (small booth to large hall) |
| Filter | -100% to +100% | 0% | DJ-style filter: center=bypass, negative=low-pass, positive=high-pass |
| Filter Position | Pre/Post | Post | Switch: filter before or after drive stage |

## UI Concept

**Layout:** 5 rotary knobs + 1 toggle switch
**Visual Style:** Modern, clean, slightly vintage warmth
**Key Elements:**
- Five knobs arranged horizontally or in an arc
- Filter position toggle switch (labeled Pre/Post)
- Visual feedback for drive saturation (optional meter)
- Parameter value displays below each knob

## Use Cases

- Adding driven ambience to drums without washing out transients
- Creative reverb effects for synths and electronic music
- Characterful space on vocals where clean reverb is too sterile
- Sound design with filtered, saturated reverb tails
- Mixing situations requiring reverb that "sits back" with harmonic color

## Inspirations

- Tape delay units with reverb (warm, saturated character)
- DJ mixer filters (intuitive, musical tone control)
- Modern algorithmic reverbs (clean, flexible parameters)
- Console/tape warmth applied to spatial effects

## Technical Notes

### Reverb Algorithm
- Algorithmic reverb (CPU-efficient)
- Size parameter controls room dimensions and early reflection density
- Decay parameter controls tail length independently of size

### Drive Saturation
- Even harmonic saturation (warm, tube/tape-like)
- 0-24dB gain range allows subtle to aggressive drive
- Applied only to wet signal path (dry signal remains clean)

### DJ-Style Filter
- Based on GainKnob filter implementation (PluginProcessor.cpp:86-130)
- Center position (0%) = bypass
- Negative values = exponential low-pass sweep (200Hz at -100% to 20kHz at 0%)
- Positive values = exponential high-pass sweep (20Hz at 0% to 10kHz at +100%)
- Q factor: 0.707 (Butterworth response)
- Filter state reset on bypass to prevent residual energy

### Signal Flow Options
- **Pre-drive:** Input → Reverb → Filter → Drive → Dry/Wet Mix → Output
- **Post-drive:** Input → Reverb → Drive → Filter → Dry/Wet Mix → Output

### DSP Considerations
- Reverb processing on wet signal only
- Drive saturation uses soft clipping or waveshaping for even harmonics
- Filter uses IIR coefficients (juce::dsp::IIR::Coefficients)
- Smoothing on parameter changes to avoid zipper noise

## Next Steps

- [ ] Create UI mockup (`/dream DriveVerb` → option 3)
- [ ] Start implementation (`/implement DriveVerb`)
