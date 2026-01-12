# Ouaricon Digital Delay - Creative Brief

## Overview

A clean, versatile digital delay designed for single-instrument effects chains. Prioritizes transparency, flexibility, and ease of use across all instruments and performance contexts.

## Core Identity

**Type:** Digital Delay
**Character:** Pristine/transparent - repeats preserve the source without coloration
**Complexity:** Simple and focused - minimal controls, maximum usability
**Context:** Versatile for all instruments and synths, live performance and studio mixing

## Sonic Design

### Delay Character
- **Transparent repeats** - no filtering, saturation, or tonal degradation
- **Consistent tone** - repeats do not darken or degrade over time
- **Clean feedback path** - predictable, controllable behavior

### Timing System
- **Dual mode:** Free-running milliseconds AND host tempo sync
- **Rich subdivisions for sync mode:**
  - Straight: 1/4, 1/8, 1/16
  - Dotted: 1/4D, 1/8D, 1/16D
  - Triplets: 1/4T, 1/8T, 1/16T
  - Quintuplets: 1/4 (5), 1/8 (5), 1/16 (5)

### Feedback Behavior
- **Controllable and predictable** - no self-oscillation or runaway behavior
- **No ducking** - delay level remains constant regardless of input

### Stereo Processing
- **Stereo spread control** (0-100%)
- At **0%:** Preserves input stereo field (true stereo through)
- At **100%:** Subtle stereo widening (NOT harsh ping-pong)
- Smooth interpolation between extremes

### Modulation
- **Optional subtle modulation** on delay time
- Chorus-like movement to add life and depth
- Controllable via dedicated parameter (can be set to zero for fully static delay)

## Workflow Features

### Live Performance
- **Spillover on bypass** - delay tail continues when plugin is bypassed
- Quick to dial in during soundcheck
- Predictable behavior for live use

### Studio Mixing
- Clean enough to sit well in any mix
- Stereo spread useful for width without ping-pong artifacts
- Versatile across all instrument types

## Parameters (Estimated)

| Parameter | Range | Description |
|-----------|-------|-------------|
| Time | 1-2000ms | Delay time in free mode |
| Sync | On/Off | Toggle tempo sync |
| Division | List | Note subdivision when synced |
| Feedback | 0-100% | Amount of repeats |
| Spread | 0-100% | Stereo width (0=preserve, 100=wide) |
| Mod | 0-100% | Subtle delay time modulation |
| Mix | 0-100% | Wet/dry balance |

**Total: 7 parameters** - Focused and manageable

## Technical Considerations

- Stereo input/output processing
- Sample-accurate tempo sync
- Smooth parameter changes (no clicks/pops)
- Efficient enough for live use with low latency
- Spillover requires careful bypass implementation (process tail after bypass)

## Out of Scope

- Tape/analog emulation or coloration
- Self-oscillation / runaway feedback
- Ducking / sidechain behavior
- Complex multi-tap configurations
- Ping-pong (replaced by subtler stereo spread)
- Filtering in feedback path

## Target User

Musicians and producers who need a reliable, transparent delay that:
- Works on any instrument without imposing a sonic signature
- Offers both free and synced timing options
- Provides stereo interest without overwhelming the mix
- Is quick to set up and predictable in behavior

---

*Status: Ideation Complete*
*Ready for: /plan (architecture and implementation planning)*
