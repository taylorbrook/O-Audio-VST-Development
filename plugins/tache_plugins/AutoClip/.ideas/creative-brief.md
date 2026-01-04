# AutoClip - Creative Brief

## Overview

**Type:** Effect
**Core Concept:** Hard clipper with automatic peak-based gain matching
**Status:** 💡 Ideated
**Created:** 2025-11-13

## Vision

AutoClip is a specialized hard clipper designed for processing drums (claps, hats, and other percussive elements). Its defining feature is real-time automatic gain matching that maintains consistent perceived loudness as you increase clipping intensity. As the threshold lowers and more clipping occurs, the plugin instantly compensates by adjusting the output gain, ensuring the clipped signal has the same peak level as the clean input. This allows you to dial in aggressive clipping for tone and character without constantly adjusting your mix levels.

The design prioritizes simplicity and transparency—inspired by StandardClip's clean approach but with the added intelligence of automatic level compensation. A fixed 5ms lookahead ensures the gain matching anticipates transients smoothly without introducing perceptible latency.

## Parameters

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Threshold | 0-100% | 0% | Clipping threshold (0% = no clipping, 100% = maximum clipping) |
| Clip Solo | Off/On | Off | Listen to only the clipped portion of the signal (automatable) |

## UI Concept

**Layout:** Minimalist single-screen portrait layout (300×500px) with large threshold control as primary focus
**Visual Style:** Vintage Bakelite aesthetic with warm, tactile feel
**Key Elements:**
- Large rotary knob (190px) with physical rotation behavior (texture rotates, lighting fixed)
- Horizontal slide toggle for clip solo (automatable parameter)
- Vintage Bakelite material design: warm brown gradients, subtle noise texture
- Stitched inner border detail for molded appearance
- "AUTOCLIP" title at top in bold uppercase

**Design Evolution:**
During UI mockup iteration (v1-v5), the aesthetic evolved from the initially proposed "clean and functional" approach to a vintage Bakelite hardware aesthetic. This direction provides:
- Warm, tactile feel suited to aggressive drum processing
- Physical knob rotation model enhancing realism
- Single-column portrait format (300×500px) focusing attention on the dominant threshold control
- Vintage warmth complementing the analog character of hard clipping

**Deferred Features (v2):**
- Input/output metering showing gain matching in action
- Visual indicator showing amount of clipping occurring

## Use Cases

- Aggressive drum processing for electronic music (808s, claps, hats)
- Adding harmonic distortion to percussive elements while maintaining mix balance
- Creative sound design on transient-rich material without level jumps
- Quick A/B comparison of clipped vs clean (via clip solo) at matched loudness

## Inspirations

- **StandardClip:** Clean, transparent clipping algorithm
- **Peak-based gain matching:** Maintains consistent peak levels regardless of clipping amount

## Technical Notes

**Automatic Gain Matching:**
- Uses peak-based analysis (matches loudest sample level)
- Fixed 5ms lookahead for smooth transient anticipation
- Real-time compensation with minimal latency

**Processing:**
- Hard clipping algorithm (immediate saturation at threshold)
- Lookahead buffer for gain calculation
- Peak detection and compensation applied post-clipping

**Clip Solo Feature:**
- Implemented as boolean parameter (can be automated and saved in presets)
- Outputs only the difference signal (clipped portion only)
- Useful for hearing exactly what's being affected

## Next Steps

- [ ] Create UI mockup (`/dream AutoClip` → option 3)
- [ ] Start implementation (`/implement AutoClip`)
