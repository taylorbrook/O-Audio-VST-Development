# OrganicHats - Creative Brief

## Overview

**Type:** Synth
**Core Concept:** Warm, organic noise-based hi-hat synthesizer with separate closed and open sounds
**Status:** 💡 Ideated
**Created:** 2025-11-12

## Vision

OrganicHats is a hi-hat synthesizer designed for electronic music production, offering warm, organic noise-based sounds as an alternative to metallic digital or sampled hi-hats. Inspired by classic drum machines like the CR-78 and 808 but with a warmer, less metallic character, it generates hi-hat sounds through filtered noise and resonators rather than traditional metallic synthesis.

The plugin provides both closed and open hi-hat sounds within a single instrument, each controllable via separate MIDI notes. Closed hi-hats automatically choke open hi-hats with instant cutoff, mimicking realistic hi-hat pedal behavior. The synthesis approach uses filtered noise shaped through fixed resonance peaks to create body and warmth, avoiding harsh metallic characteristics while maintaining the essential percussive quality of hi-hats.

## Parameters

### Closed Hi-Hat

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Tone | 0-100% | 50% | Brightness control (dark to bright) |
| Decay | 20-200ms | 80ms | Envelope decay time |
| Noise Color | 0-100% | 50% | Noise filtering for warmth vs brightness |

### Open Hi-Hat

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Tone | 0-100% | 50% | Brightness control (shared character with closed) |
| Release | 100ms-1s | 400ms | Sustained decay time for open sound |
| Noise Color | 0-100% | 50% | Noise filtering (shared character with closed) |

## UI Concept

**Layout:** Dual-panel layout with separate sections for closed (left) and open (right) hi-hats
**Visual Style:** Studio Hardware aesthetic - professional 19" rack unit with industrial precision, dark brushed metal, yellow-gold accents
**Window Size:** 600x590px
**Key Elements:**
- Separate parameter sections for closed (left) and open (right) hi-hats
- Power LED indicator showing plugin active status
- Digital readouts below each knob for precise value display
- Mounting holes and rack hardware styling

## Velocity Sensitivity

Velocity affects both volume and tone brightness. This behavior is always enabled in the DSP (no user controls for velocity sensitivity amount).

## Use Cases

- Electronic music production requiring warm, synthesized hi-hat sounds
- Layering with acoustic drum samples to add synthetic character
- Creating unique percussive textures that blend organic warmth with electronic precision
- Alternative to harsh digital hi-hat samples in genres like house, techno, and ambient

## Inspirations

- CR-78/808 drum machines (classic electronic character)
- Organic analog drum machines (Jomox, Vermona)
- Warm noise-based synthesis approach

## Technical Notes

### Synthesis Architecture
- **Noise generation:** White/pink noise source with no random variation per hit (static, predictable)
- **Filtering:** Tone parameter controls high-pass/low-pass filtering for brightness
- **Noise Color:** Additional filtering stage for warmth vs brightness character
- **Resonators:** Fixed resonance peaks tuned for organic body and warmth
- **Envelope:** Simple decay envelope for closed, release envelope for open

### MIDI Implementation
- Closed hi-hat: Triggered by specific MIDI note (e.g., C1)
- Open hi-hat: Triggered by different MIDI note (e.g., D1)
- Choke behavior: Closed hi-hat trigger instantly cuts open hi-hat (<5ms)
- Velocity sensitivity: Affects both volume and tone brightness

### Presets
- Preset management handled by DAW
- No factory presets (users create their own sounds)

## Next Steps

- [ ] Create UI mockup (`/dream OrganicHats` → option 3)
- [ ] Start implementation (`/implement OrganicHats`)
