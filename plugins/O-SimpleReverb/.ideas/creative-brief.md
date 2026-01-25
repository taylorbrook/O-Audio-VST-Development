# Ouaricon Simple Reverb - Creative Brief

## Overview

**Name:** Ouaricon Simple Reverb
**Type:** Audio Effect (Reverb)
**Purpose:** Lightweight, CPU-efficient reverb designed to add subtle color and realism to instrument chains. Removes the "in-a-box" feel without dominating the mix.

**Design Philosophy:** Simple, focused, efficient. This reverb isn't meant to be the star - it's meant to make everything else sound more real and present.

---

## Use Cases

- **Primary:** Instrument chain integration - adding room tone and space to dry synths, samplers, and virtual instruments
- **Secondary:** Standalone effect for subtle spatial enhancement
- **Future:** Module version for embedding in other VSTs with simplified UI

**Target User:** Producers and sound designers who want quick, musical reverb without deep-diving into complex parameters. Set it and forget it.

---

## Parameters

| Parameter | Type | Range | Default | Description |
|-----------|------|-------|---------|-------------|
| Type | Choice (Dropdown) | Booth, Room, Hall, Spring, Plate, Ambient | Room | Selects reverb algorithm/character |
| Character | Float (Knob) | Warm ← → Bright | Neutral (center) | Tonal coloration of the reverb tail |
| Wet | Float (Knob) | 0-100% | 25% | Reverb signal level |
| Dry | Float (Knob) | 0-100% | 100% | Original signal level |
| Decay | Float (Knob) | 0.1s - 10s | 1.5s | Reverb tail length |
| Size | Float (Knob) | Small ← → Large | Medium (center) | Virtual room dimensions |

### Parameter Notes

**Type Presets:**
- **Booth:** Tight, intimate space. Very short reflections. For close-mic feel.
- **Room:** Natural small-medium room. Versatile, musical default.
- **Hall:** Large concert hall. Long, diffuse tail. For epic/orchestral.
- **Spring:** Classic spring reverb character. Metallic, vintage.
- **Plate:** Dense, smooth plate reverb. Studio classic.
- **Ambient:** Washy, ethereal. Long diffuse tails, minimal early reflections.

**Character Control:**
- Warm: High-frequency rolloff, darker tail
- Neutral: Flat frequency response
- Bright: High-frequency emphasis, airy tail

---

## Technical Requirements

### CPU Efficiency
- **Priority:** High - must be lightweight enough for multiple instances
- **Target:** Lower CPU than convolution reverbs
- **Approach:** Algorithmic reverb (Schroeder/Freeverb style or similar efficient algorithm)

### Formats
- VST3
- AU (Audio Unit)
- Standalone

### Module Integration (Future)
- Same DSP core
- Simplified UI (fewer visual elements, same controls)
- Designed for embedding in instrument plugins

---

## Visual Design

**Aesthetic:** Ouaricon Naturalist (ouaricon-naturalist-001)
- Aged paper background
- Botanical illustration overlay (suggest: flora category - flowing, ethereal)
- Seed cross-section knobs
- Garamond serif typography
- Warm earth-tone palette with green accents

**Layout Suggestion (6 parameters):**
- Type dropdown at top or grouped with controls
- 5 knobs in balanced arrangement
- Medium knob size (60px)
- Botanical overlay on right side

**Recommended Botanical Image:** Flora category - something ethereal and flowing to match reverb's spatial character. Consider `flora-1.png` or similar organic flowing image.

---

## Sound Character

**Overall Vibe:** Subtle, musical, transparent. Should enhance without drawing attention.

**Key Qualities:**
- Natural-sounding early reflections
- Smooth, non-metallic decay (except Spring type)
- Musical frequency response that sits well in a mix
- No harsh artifacts or ringing

**What it's NOT:**
- Not a special effect reverb (no shimmer, modulation, etc.)
- Not meant to be obvious or dramatic
- Not a convolution reverb (too CPU heavy)

---

## Differentiation

**vs. DriveVerb/FlutterVerb:** Those are character reverbs with specific sonic signatures. Simple Reverb is neutral and versatile.

**vs. Complex reverbs (Valhalla, FabFilter):** Simple Reverb trades deep control for immediate usability and CPU efficiency.

**Unique Value:** The "always-on" reverb for instrument chains. Quick to set up, light on CPU, musically useful defaults.

---

## Success Criteria

1. CPU usage significantly lower than convolution reverbs
2. All 6 reverb types sound distinct and musical
3. Character control provides useful tonal range
4. Can be used on multiple tracks without CPU strain
5. Sounds good on first load with default settings
6. Visual design matches Ouaricon brand aesthetic

---

## Version History

- **v0.1 (Creative Brief):** Initial concept - 2026-01-13

---

**Status:** Ready for planning (/plan) or implementation (/implement)
