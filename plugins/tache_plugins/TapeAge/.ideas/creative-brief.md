# TAPE AGE - Creative Brief

## Overview

**Type:** Audio Effect (Tape Saturator)
**Core Concept:** Vintage tape saturation with musical degradation
**Status:** 💡 Ideated
**Created:** 2025-11-10

## Vision

TAPE AGE is a vintage tape saturator that captures the warmth and character of 60s/70s tape machines. It combines smooth saturation with controllable tape degradation—wow, flutter, dropout, and noise—all tuned to remain musical even at maximum settings. The plugin delivers warm lo-fi saturation with authentic tape artifacts, designed for adding vintage character without crossing into unusable territory.

## Parameters

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| DRIVE | 0-100% | 50% | Tape saturation amount—warm harmonic distortion |
| AGE | 0-100% | 25% | Tape degradation (wow/flutter/dropout/noise)—max = very old but musical |
| MIX | 0-100% | 100% | Dry/wet blend |

## UI Concept

**Layout:**
- Medium rectangle (nearly square dimensions)
- Top section: Vintage VU meter with moving needle (output level, peak metering)
- Center: "TAPE AGE" title in clean sans-serif, all caps, generous letter spacing
- Bottom section: 3 brass knobs in horizontal row (DRIVE, AGE, MIX)
- Labels below each knob in all caps

**Visual Style:**
- Background: Slightly textured creamy beige
- Color palette: Cream/beige base with burnt orange and brown accents
- Typography: Clean sans-serif, all caps, spacious letter spacing
- Knobs: Realistic vintage brass-colored knobs with pointer line indicators (no numeric values)
- Subtle shadow and depth on knobs for dimensionality

**Key Elements:**
- Vintage VU meter with authentic needle movement
- Brass vintage-style knobs with position indicators
- 60s/70s tape machine aesthetic with earth tone palette
- Centered, balanced layout—everything feels intentional and beautiful

## Use Cases

- Adding warmth and character to digital recordings
- Lo-fi processing for vocals, drums, or full mixes
- Vintage tape simulation for retro productions
- Subtle saturation (low Drive/Age) or heavy degradation (high settings)
- Parallel processing with Mix control for blend flexibility

## Inspirations

- 60s/70s tape machines (visual and sonic reference)
- Warm tape saturation with lo-fi character
- Musical degradation that remains usable at extremes

## Parameter Behavior

### DRIVE (Tape Saturation)
- **Character:** Really nice, tasty tape saturator—warm, harmonic, pleasing
- **Behavior:** Should sound excellent at all settings
- **Goal:** Add warmth and analog character without harshness

### AGE (Tape Degradation)
- **0% (Min):** Clean tape—no degradation artifacts, just the saturation from DRIVE
- **25% (Default):** Subtle vintage character—light wobble and texture
- **100% (Max):** Very lo-fi and wobbly, sounds cool and vintage—BUT STILL MUSICAL

**CRITICAL AGE Constraint:**
- AGE controls wow, flutter, dropout, and noise artifacts
- **Noise must remain SUBTLE at all settings**
- At max AGE: Sound should be wobbly, vintage, characterful—NOT buried in hiss
- Zero AGE = no tape artifacts. Max AGE = sounds great (not ruined by noise)
- Goal: Age adds vibe and movement, not a noise floor problem

**What AGE should NOT do:**
- ❌ Burst with loud, consistent hiss at high settings
- ❌ Add a constant noise floor that ruins the signal
- ❌ Make the plugin unusable or "stupid" sounding at max

**What AGE SHOULD do:**
- ✅ Add pitch wobble (wow/flutter) that sounds musical
- ✅ Add texture and movement to the sound
- ✅ Make it sound like vintage tape with character
- ✅ Sound cool and usable even at 100%

### MIX (Dry/Wet Blend)
- **0%:** Fully dry (bypassed)
- **100%:** Fully processed (default)
- **Purpose:** Parallel processing flexibility—blend processed signal with dry

## Technical Notes

**Saturation character:** Warm harmonic distortion, lo-fi edge, tape compression behavior.

**AGE artifact balance:**
- Wow/flutter: Primary character source (pitch modulation)
- Dropout: Occasional, adds realism
- Noise: SUBTLE enhancement, never overwhelming

## DSP Implementation Specifications

### DRIVE Saturation Behavior
- **Type:** Progressive (clean at low settings, heavy at high settings)
- **0-30%:** Subtle warmth, gentle harmonics, mostly transparent
- **30-70%:** Noticeable warmth and thickness, clear harmonic content
- **70-100%:** Heavy saturation, thick colored vintage sound, strong harmonic distortion
- **Character:** Should sound excellent at all positions—never harsh or unpleasant

### WOW/FLUTTER Modulation
- **Type:** Slow and smooth—gentle pitch drift
- **Frequency range:** Mostly wow (0.5-2Hz), very subtle flutter
- **Character:** Dreamy feel, gentle warble, not chaotic or fast
- **Scaling:** AGE knob controls depth of modulation (0% = none, 100% = very noticeable but still musical)

### DROPOUT Characteristics
- **Frequency:** Rare—occasional brief dips (every 5-10 seconds at max AGE)
- **Duration:** Brief (50-150ms)
- **Depth:** Partial signal loss (not complete muting)
- **Purpose:** Adds subtle realism and vintage character without being distracting
- **Scaling:** AGE knob controls probability (0% = none, 100% = rare but present)

### MIX Control Behavior
- **Type:** Center-dip compensated—constant perceived loudness
- **Curve:** Equal-power crossfade to avoid volume drop in middle positions
- **0%:** Fully dry (100% dry, 0% wet)
- **50%:** Equal blend without volume loss
- **100%:** Fully wet (0% dry, 100% wet)
- **Goal:** Smooth blending without perceived loudness changes

## Next Steps

- [ ] Create UI mockup (`/dream TapeAge` → option 3)
- [ ] Start implementation (`/implement TapeAge`)
