# O-SpectralShaper Creative Brief

## Core Concept

**One-liner:** Per-frequency transient control with real-time visual feedback — boost the kick's low-end punch without adding click, or add snare crack only where it matters.

**Problem Statement:** Traditional transient shapers treat the entire signal identically. A snare's attack boost also boosts cymbal bleed. Taming a guitar's pick attack also dulls the body resonance. Mix engineers need surgical control.

**Solution:** O-SpectralShaper uses FFT analysis to detect and shape transients at specific frequencies independently. Users draw attack and sustain curves across a 32-band logarithmic spectrum, seeing exactly where transients occur on a live spectrogram with heat-map overlay.

## Target Audience

- **Mix engineers** seeking surgical transient control without affecting adjacent frequencies
- **Producers** wanting to shape drum punch/snap independently per element
- **Sound designers** exploring spectral manipulation for creative effects

## Key Differentiators

1. **32 logarithmic bands** vs competitors' typical 5-8 bands
2. **Per-band transient detection** — each band detects transients independently
3. **Drawable curves** with freehand + node-based editing modes
4. **Real-time spectrogram** with transient heat overlay visualization
5. **Low latency (~5ms)** suitable for live mixing, not just mastering

## User Interface Concept

### Main Display Area
- **Spectrogram** (scrolling time × frequency) with logarithmic frequency axis
- **Transient heat overlay** — color-coded visualization showing detected transients and applied shaping intensity
- Two **drawable curve areas** (Attack and Sustain) displayed below/alongside the spectrogram

### Curve Editing
- **Freehand mode:** Natural drawing with automatic smoothing
- **Node mode:** Click to place nodes, drag handles for bezier precision
- **Mode toggle** accessible via button or modifier key

### Global Parameters (Full Suite)
| Parameter | Range | Default | Purpose |
|-----------|-------|---------|---------|
| Mix | 0-100% | 100% | Wet/dry blend |
| Attack Time | 0.1-50ms | 10ms | Global attack speed multiplier |
| Sustain Time | 10-500ms | 100ms | Global sustain release time |
| Sensitivity | 0-100% | 50% | Transient detection threshold |
| Lookahead | 0-10ms | 2ms | Pre-trigger for clean attack capture |
| Output Gain | -12 to +12dB | 0dB | Compensate for level changes |

### Visual Style
- **Ouaricon dark theme** — consistent with O-* plugin family
- Clean modern aesthetic with accent colors for curves and transient visualization
- High-contrast spectrogram for readability

## DSP Architecture Overview

### Signal Flow
```
Input → Lookahead Buffer → FFT Analysis (32 bands) → Per-Band Transient Detection
                                                              ↓
                                              Per-Band Envelope Shaping
                                                              ↓
                          Overlap-Add Resynthesis ← Attack/Sustain Curve Application
                                                              ↓
                                                    Mix → Output
```

### Technical Constraints
- **FFT Size:** ~512 samples at 44.1kHz (≈11.6ms window, ~5ms latency with 50% overlap)
- **Band Count:** 32 logarithmically-spaced bands (~3 bands/octave across 20Hz-20kHz)
- **Per-band detection:** Independent envelope followers with configurable sensitivity
- **Low-latency target:** <10ms total system latency for live use

## Future Versions (Post v1.0.0)

### Adaptive Spectral Masks
- Auto-detect fundamental frequencies and adjust targeting ranges
- User can lock in auto-detected values

### Learn Mode
- User plays isolated element (kick, snare, etc.)
- Plugin analyzes and learns spectral profile for precise targeting

### Instrument Preset Categories
- Drums (kick/snare/hats variants)
- Strings (guitar/bass)
- Keys
- Organized by instrument AND by effect type ("Add Punch", "Tame Harshness", etc.)

## Success Criteria

1. **Real-time performance** — <50% CPU on single core at 44.1kHz
2. **Visual clarity** — transient detection visible and meaningful on spectrogram
3. **Intuitive workflow** — draw a curve, hear the difference immediately
4. **Mix-ready quality** — no artifacts, phase issues, or pumping

## Competitive Landscape

| Product | Bands | Detection | Visualization | Latency |
|---------|-------|-----------|---------------|---------|
| SPL Transient Designer | 1 | Global | None | ~0ms |
| Sonnox Oxford TransMod | 2 (L/H) | Split | Basic | ~1ms |
| Eventide Physion | ~8 (spectral) | Hybrid | Waveform | ~10ms |
| **O-SpectralShaper** | **32** | **Per-band** | **Spectrogram + heat** | **~5ms** |

---

*Created: 2026-02-03*
*Status: Ideation Complete*
