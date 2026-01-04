# GainKnob - Creative Brief

## Overview

**Type:** Effect (Utility)
**Core Concept:** Single knob volume attenuation control
**Status:** 💡 Ideated
**Created:** 2025-11-10

## Vision

A minimalist gain utility plugin featuring a single large knob for volume control. Designed for testing the UI mockup workflow with the simplest possible parameter set—pure attenuation from silence to unity gain.

## Parameters

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Gain | -∞ to 0dB | 0dB | Volume attenuation control |

## UI Concept

**Layout:** Single centered knob occupying most of the plugin window
**Visual Style:** Clean, minimal
**Key Elements:**
- Large rotary knob
- dB value display

## Use Cases

- Testing UI mockup workflow
- Simple volume attenuation
- Learning plugin development pipeline

## Inspirations

- Basic utility gain plugins
- Minimal interface design principles

## Technical Notes

- Linear or logarithmic gain curve (typically logarithmic for dB control)
- -∞ dB represents complete silence (gain multiplier of 0.0)
- 0 dB represents unity gain (gain multiplier of 1.0)

## Next Steps

- [ ] Create UI mockup (`/dream GainKnob` → option 3)
- [ ] Start implementation (`/implement GainKnob`)
