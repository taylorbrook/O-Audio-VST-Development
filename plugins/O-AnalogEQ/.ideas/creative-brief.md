# Ouaricon Analog EQ - Creative Brief

## Vision

A lightweight, knob-based 4-band EQ plugin inspired by the Waves V-EQ4 and Neve 1081 console module. Emphasizes simplicity and musicality with dual-layer knob controls and subtle analog warmth.

## Inspiration

- **Waves V-EQ4** - Modeled after the Neve 1081 console module
- **Neve 1081** - Classic British console EQ known for its musical character
- **Design philosophy**: Vintage tone-shaping rather than surgical correction

## Core Features

### Band Configuration

1. **Low Frequency Band (LF)**
   - Dual-layer knob: outer ring = frequency, inner = gain
   - Shelving EQ curve
   - Frequency range: ~30Hz - 500Hz
   - Gain range: +/- 12dB

2. **Low-Mid Band (LMF)**
   - Dual-layer knob: outer ring = frequency, inner = gain
   - 3-way Q toggle: Low / Mid / High
   - Bell/parametric curve
   - Frequency range: ~100Hz - 2kHz
   - Gain range: +/- 12dB

3. **High-Mid Band (HMF)**
   - Dual-layer knob: outer ring = frequency, inner = gain
   - 3-way Q toggle: Low / Mid / High
   - Bell/parametric curve
   - Frequency range: ~500Hz - 8kHz
   - Gain range: +/- 12dB

4. **High Frequency Band (HF)**
   - Dual-layer knob: outer ring = frequency, inner = gain
   - Shelving EQ curve
   - Frequency range: ~2kHz - 20kHz
   - Gain range: +/- 12dB

### Sound Character

- **Subtle analog warmth** - Gentle harmonic saturation
- **Musical EQ curves** - Neve-inspired filter shapes
- **Non-linear behavior** - Slight compression at extreme boosts
- **Console coloration** - Even with flat EQ, adds subtle character

## UI/UX Design

### Control Layout

```
┌───────────────────────────────────────────────────────────────────┐
│                       OUARICON ANALOG EQ                          │
├───────────────────────────────────────────────────────────────────┤
│                                                                   │
│  ┌─────────┐    ┌─────────┐    ┌─────────┐    ┌─────────┐        │
│  │   LOW   │    │  L-MID  │    │  H-MID  │    │   HIGH  │        │
│  │ ◎ freq  │    │ ◎ freq  │    │ ◎ freq  │    │ ◎ freq  │        │
│  │  ○ gain │    │  ○ gain │    │  ○ gain │    │  ○ gain │        │
│  │  shelf  │    │ [L|M|H] │    │ [L|M|H] │    │  shelf  │        │
│  └─────────┘    └─────────┘    └─────────┘    └─────────┘        │
│                                                                   │
└───────────────────────────────────────────────────────────────────┘
```

### Visual Style

- Clean, minimal layout
- Vintage-inspired knob aesthetics
- Warm color palette (creams, browns, subtle gold accents)
- Clear frequency/gain labeling
- LED-style indicators for active bands

### Dual-Layer Knob Behavior

- **Outer ring**: Frequency selection (click and drag on outer edge)
- **Inner knob**: Gain adjustment (click and drag on center)
- Visual feedback showing both values simultaneously
- Tooltip/readout showing exact Hz and dB values

### Q Toggle

- Physical 3-position switch aesthetic
- Labels: "WIDE" / "MED" / "TIGHT" (or "L" / "M" / "H")
- Click to cycle through positions

## Technical Specifications

- **Plugin Formats**: VST3, AU
- **Sample Rates**: 44.1kHz - 192kHz
- **Latency**: Zero (or minimal for oversampling if needed)
- **CPU**: Lightweight - suitable for many instances

## Parameters Summary

| Parameter | Type | Range | Default |
|-----------|------|-------|---------|
| LF Freq | Float | 30-500 Hz | 100 Hz |
| LF Gain | Float | -12 to +12 dB | 0 dB |
| LF On | Bool | On/Off | On |
| LMF Freq | Float | 100-2000 Hz | 500 Hz |
| LMF Gain | Float | -12 to +12 dB | 0 dB |
| LMF Q | Choice | Low/Mid/High | Mid |
| LMF On | Bool | On/Off | On |
| HMF Freq | Float | 500-8000 Hz | 2000 Hz |
| HMF Gain | Float | -12 to +12 dB | 0 dB |
| HMF Q | Choice | Low/Mid/High | Mid |
| HMF On | Bool | On/Off | On |
| HF Freq | Float | 2000-20000 Hz | 8000 Hz |
| HF Gain | Float | -12 to +12 dB | 0 dB |
| HF On | Bool | On/Off | On |
| Output Gain | Float | -12 to +12 dB | 0 dB |
| Analog | Bool | On/Off | On |

## Success Criteria

1. Clean, intuitive UI with the dual-layer knob interaction working smoothly
2. Musical EQ curves that feel good, not surgical
3. Subtle warmth that enhances without coloring too heavily
4. Lightweight enough to use on every channel
5. Stable across all sample rates

## Reference

- [Waves V-EQ4](https://www.waves.com/plugins/v-eq4) - Primary inspiration
- Neve 1081 console module - Hardware reference

---

*Created: 2026-01-11*
*Status: Ideated*
