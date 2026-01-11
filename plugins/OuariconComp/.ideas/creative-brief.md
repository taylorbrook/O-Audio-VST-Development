# OuariconComp - Creative Brief

## Core Vision

A simple, transparent compressor designed for mixing individual instruments within effects chains. Lightweight, focused, and musical - compression that serves the mix without imposing character.

## Plugin Identity

- **Name:** OuariconComp
- **Type:** Audio Effect (Dynamics/Compressor)
- **Character:** Transparent, clean, utilitarian
- **Use Case:** Insert on individual tracks for gentle dynamic control during mixing

## Design Philosophy

Less is more. This compressor exists to tame dynamics without coloring the sound. No vintage warmth, no aggressive pumping - just honest, predictable compression that stays out of the way. The kind of tool you reach for when you need control, not flavor.

## Parameters

| Parameter | Type | Range | Default | Purpose |
|-----------|------|-------|---------|---------|
| Threshold | Float | -60 to 0 dB | -20 dB | Level where compression begins |
| Ratio | Float | 1:1 to 20:1 | 4:1 | Compression intensity |
| Attack | Float | 0.1 to 100 ms | 10 ms | How fast compression engages |
| Release | Float | 10 to 1000 ms | 100 ms | How fast compression releases |
| Knee | Float | 0 to 20 dB | 6 dB | Transition sharpness (0=hard, higher=softer) |
| Output Gain | Float | -12 to +24 dB | 0 dB | Makeup gain compensation |
| Auto-Gain | Bool | On/Off | Off | Automatic makeup gain based on threshold/ratio |

**Total: 7 parameters** (6 continuous knobs + 1 toggle button)

## Technical Requirements

### DSP Approach

- **Algorithm:** Feed-forward compressor design for predictability
- **Detection:** Peak or RMS detection (peak preferred for transparency)
- **Ballistics:** Smooth envelope following with logarithmic attack/release curves
- **Knee:** Variable soft-knee implementation using polynomial or exponential interpolation
- **Auto-gain:** Calculate makeup gain from threshold and ratio: `makeupGain = -threshold * (1 - 1/ratio)`

### Audio Specifications

- **Latency:** Zero or minimal (< 1 sample for peak detection)
- **Quality:** Clean, aliasing-free gain reduction
- **CPU:** Lightweight - suitable for multiple instances

## Visual Design

### Aesthetic

**Ouaricon Audio Naturalist** - the signature brand aesthetic with:
- Aged paper background (#F5E6D3)
- Garamond serif typography
- Botanical seed cross-section knobs
- Warm earth-tone palette

### Botanical Imagery

For dynamics/compression plugins, use anatomical or skeletal imagery:
- **Recommended:** `anatomy/` or `skeletons/` folder
- **Rationale:** Structure, control, force - the essence of compression
- **Placement:** Right side, 35% opacity, click-through

### Layout

7 parameters suggests:
- Medium knobs (60px diameter)
- Moderate spacing (30px gaps)
- Two-row or grouped single-row layout
- Consider grouping: [Threshold, Ratio, Knee] + [Attack, Release] + [Output, Auto-Gain]

### Suggested Layout Structure

```
+--------------------------------------------------+
|            OUARICONCOMP                          |
|                                                  |
|   [THRESHOLD]  [RATIO]  [KNEE]      [botanical]  |
|                                      [overlay]   |
|   [ATTACK]  [RELEASE]  [OUTPUT]      [image]    |
|                                                  |
|              [AUTO-GAIN]                         |
+--------------------------------------------------+
```

### Metering (Optional)

A subtle gain reduction meter would provide useful visual feedback:
- Vertical bar, left side or integrated with threshold knob
- Earth-tone colors (tan to brown gradient)
- Shows dB of gain reduction in real-time

## User Experience Goals

1. **Immediate comprehension** - All controls visible, no hidden pages
2. **Predictable response** - What you set is what you get
3. **Mix-friendly** - Sits well alongside other Ouaricon plugins
4. **Low friction** - Drag threshold, adjust ratio, done

## Target Users

- Producers mixing individual tracks
- Sound designers shaping dynamics in effects chains
- Anyone needing clean, surgical compression

## Success Criteria

- Transparent sound that doesn't alter tonal character
- Responsive controls that feel natural
- CPU efficiency for multi-instance use
- Visual consistency with Ouaricon Audio brand

---

**Created:** 2026-01-11
**Status:** Ideated
**Aesthetic:** Ouaricon Audio Naturalist (ouaricon-naturalist-001)
