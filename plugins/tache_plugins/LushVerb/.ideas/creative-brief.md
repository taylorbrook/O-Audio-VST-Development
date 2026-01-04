# LushVerb - Creative Brief

## Overview

**Type:** Audio Effect (Reverb)
**Core Concept:** Stunning, lush algorithmic reverb inspired by Strymon BigSky—simple controls, infinite beauty
**Status:** 💡 Ideated
**Created:** 2025-11-11

## Vision

LushVerb is designed to be a single-purpose reverb: stunning, lush, and infinitely beautiful. Inspired by the legendary Strymon BigSky, it focuses on ambient/ethereal character with smooth, endless tails that float and shimmer. The goal is simplicity without compromise—just four knobs that deliver consistently gorgeous reverb, whether used subtly in mixing or pushed to extreme creative territories.

Built-in subtle modulation adds depth and movement automatically, while a gentle shimmer layer provides ethereal high-frequency sparkle. The result is a reverb that sounds expensive and inspiring every time you turn it on, suitable for everything from subtle vocal polish to massive ambient soundscapes.

## Parameters

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Size | 0.5s - 20s+ | 2.5s | Reverb tail length (small rooms to infinite spaces) |
| Damping | 0-100% | 30% | High-frequency rolloff (darker/brighter tail character) |
| Shimmer | 0-100% | 30% | +1 octave pitch-shifted signal amount |
| Mix | 0-100% | 30% | Dry/wet blend |

## UI Concept

**Layout:** Four horizontal rotary knobs with clear value displays (Size, Damping, Shimmer, Mix), evenly spaced and aligned with LED output meter

**Visual Style:** Industrial 19" rack unit aesthetic with professional studio gear precision. Dark metal foundation with warm gold accents for premium feel.

**Key Elements:**
- Large machined rotary knobs (70px diameter) with ridged texture and conic gradients
- Clear parameter value readouts in monospace font
- LED output meter with traffic light thresholds (green/yellow/red)
- Brushed metal texture and rack mounting holes
- Pulsing power LED indicator
- Compact 500×300px form factor

## Use Cases

- **Mixing tool:** Subtle vocal/instrument reverb that adds space without smearing
- **Creative FX:** Push Size to 10s+ for ambient drones and textural soundscapes
- **Universal reverb:** Works beautifully on vocals, piano, guitar, synths, drums
- **Instant inspiration:** Turn it on and it sounds gorgeous immediately

## Inspirations

- **Strymon BigSky:** Reference for lush, high-quality algorithmic reverb
- **Valhalla VintageVerb:** Smooth, musical, with subtle modulation
- **Eventide Blackhole:** For the infinite tail character at extreme settings

## Technical Notes

### DSP Approach
The ideal algorithm should prioritize **lush, beautiful, clean, and stunning** output above all else. This likely means:
- Dense early reflections for richness
- Smooth, artifact-free tail decay
- High-quality modulation (chorus) for depth and movement
- Pristine shimmer implementation (pitch-shifted +1 octave)

Research should explore:
1. **Multi-tap delay networks** (dense, complex, potentially most lush)
2. **Velvet noise convolution** (smooth, natural decay)
3. **Hybrid approaches** (combine algorithmic tail with short convolution for early reflections)

Avoid simple Schroeder reverb if it lacks the desired character—prioritize sound quality over CPU efficiency.

### Modulation
- Built-in subtle chorus modulation in the reverb tail (not user-controlled)
- Should add depth and lushness without obvious pitch wobble
- Low-frequency, wide stereo movement

### Shimmer
- +1 octave pitch-shifted signal
- User-controllable amount (0-100%)
- Default 30% provides subtle ethereal layer
- At 0%, shimmer is disabled for traditional reverb character
- At higher values, creates ambient/pad-like textures
- Clean pitch-shifting algorithm (granular or phase vocoder)

### Damping
- High-frequency rolloff only (not decay rate)
- 0% = bright, airy, crystalline
- 100% = dark, warm, diffuse
- Smooth, musical response curve

## Design Evolution

### 2025-11-11 - UI Mockup v3 Finalization

**Changes from original vision:**

**Parameters:**
- **Shimmer control changed:** Originally "always-on subtle" → Now user-controllable (0-100%, default 30%)
  - Rationale: Provides flexibility for users who want traditional reverb (0%) or extreme shimmer textures (higher values)
  - Default 30% preserves the "subtle shimmer" character from original vision
  - Allows creative exploration beyond initial scope

**Visual Style:**
- **Aesthetic evolved:** Originally "clean, modern, inspiring" → Now "industrial 19" rack unit"
  - Rationale: Industrial aesthetic communicates professional studio quality and precision
  - Brushed metal texture, rack mounting holes, LED meters evoke high-end hardware
  - Gold accents maintain premium feel from original vision
  - Compact 500×300px form factor keeps it efficient

**Visual Feedback:**
- **LED output meter added:** Traffic light thresholds (green/yellow/red)
  - Rationale: Provides real-time level monitoring for mix decisions
  - Professional studio aesthetic aligns with rack unit design
  - Helps users avoid over-driving the reverb output

**User approval:** 2025-11-11 - Confirmed these changes align with plugin vision (industrial aesthetic matches existing plugin style, user-controllable shimmer increases versatility)

## Next Steps

- [ ] Research reverb algorithms (multi-tap delay network, velvet noise, hybrid approaches)
- [x] Create UI mockup (`/dream LushVerb` → option 3) - v3 finalized
- [ ] Start implementation (`/implement LushVerb`)
