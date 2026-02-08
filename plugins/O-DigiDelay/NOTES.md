# O-DigiDelay Notes

## Status
- **Current Status:** 📦 Installed
- **Version:** 1.2.3
- **Type:** Audio Effect (Delay)
- **Complexity:** 2.4 (Moderate)

## Lifecycle Timeline

- **2026-01-12:** Creative brief completed - clean digital delay with stereo spread
- **2026-01-12 (Stage 0):** Research & Planning complete - Architecture and plan documented (Complexity 2.4)
- **2026-01-12 (v1.0.0):** Initial release - all DSP and GUI implemented
- **2026-01-12 (v1.1.0):** UI improvements - functional output meter, tempo-synced time dial, butterfly repositioned
- **2026-01-12 (v1.1.1):** Refinements - butterfly +30px, smoother time dial stepping, increased meter sensitivity
- **2026-01-12 (v1.1.2):** Time dial sync rewritten following Tremolo pattern - proper snapping to rhythmic divisions
- **2026-01-12 (v1.1.3):** Fixed time dial unresponsive in sync mode - corrected JUCE WebComboBoxState API method names
- **2026-01-12 (v1.2.0):** Preset Manager integrated - save/load presets, header layout updated (title left, preset bar right)
- **2026-01-24 (v1.2.1):** Renamed from "Ouaricon Digital Delay" to "O-DigiDelay" (DAW display, binaries, folder, presets)
- **2026-02-07 (v1.2.2):** UI title changed back to "Ouaricon Digital Delay" (display name only)
- **2026-02-07 (v1.2.3):** Knobs replaced with SVG vine-arc style from O-Detune (smooth animation, mouse wheel support)

## Known Issues

None

## Description

A clean, versatile digital delay designed for single-instrument effects chains. Prioritizes transparency, flexibility, and ease of use.

## Key Features

- **Transparent repeats** - no coloration or degradation
- **Dual timing modes** - free-running ms + tempo sync
- **Rich subdivisions** - straight, dotted, triplets, quintuplets
- **Stereo spread** - subtle widening (not ping-pong)
- **Optional modulation** - chorus-like movement on delay time
- **Spillover** - delay tail continues on bypass

## Parameters

| Parameter | Range | Description |
|-----------|-------|-------------|
| Time | 1-2000ms | Delay time in free mode |
| Sync | On/Off | Toggle tempo sync |
| Division | List | Note subdivision when synced |
| Feedback | 0-100% | Amount of repeats |
| Spread | 0-100% | Stereo width |
| Mod | 0-100% | Delay time modulation |
| Mix | 0-100% | Wet/dry balance |

## Design Philosophy

Simple and focused - minimal controls for maximum usability across all instruments and contexts (live performance + studio mixing).

## DSP Architecture

### Core Components

1. **Delay Line Engine:** juce::dsp::DelayLine with Lagrange3rd interpolation
   - Maximum delay: 2000ms
   - Dual mono processing (independent L/R)
   - Sample rate adaptive

2. **Tempo Sync System:** juce::AudioPlayHead for BPM reading
   - 12 subdivision choices (straight, dotted, triplets, quintuplets)
   - Fallback to TIME parameter if BPM unavailable

3. **Stereo Spread Processor:** Haas effect (0-15ms offset on right channel)
   - 0%: Preserves input stereo field
   - 100%: Maximum width without ping-pong

4. **Delay Time Modulation:** juce::dsp::Oscillator (sine wave, 0.3 Hz)
   - Modulation depth: 0-10ms
   - Chorus-like movement

5. **Feedback Loop:** Custom signal routing
   - Gain: 0.0-0.95 (hard-limited for stability)
   - Per-channel feedback

6. **Dry/Wet Mixer:** juce::dsp::DryWetMixer
   - Automatic latency compensation
   - Spillover support

### Signal Flow

```
Input → Dry/Wet Mixer (capture) → Delay Time Calculation
  → Dual Delay Lines (L+R) → Feedback Mixer → Dry/Wet Mixer (blend) → Output
```

### Performance Estimates

- Delay lines (Lagrange3rd): ~8% CPU
- LFO (sine oscillator): ~1% CPU
- Feedback mixing: ~1% CPU
- Dry/wet mixing: ~2% CPU
- **Total estimated: ~12% single core at 48kHz**

## Implementation Strategy

- **Single-pass implementation** (complexity 2.4, below 3.0 threshold)
- All DSP components in one phase
- All GUI controls in one phase
- Estimated duration: ~5 hours total

## Research References

### Professional Plugins Studied

1. **FabFilter Timeless 3** - Dual delay lines with tempo sync, Lagrange interpolation
2. **Waves H-Delay** - Time range: 1-3500ms, per-channel stereo control
3. **Kilohearts Haas** - Stereo widening via 5-35ms delay offset (we use 0-15ms)

### Technical Resources

- JUCE dsp::DelayLine documentation
- Dattarro - Effect Design Part 2: Delay-Line Modulation
- Physical Audio Signal Processing - Delay-Line Interpolation
- Haas Effect research (stereo widening techniques)

## Critical Implementation Notes

1. **AudioPlayHead thread safety:** Only call in processBlock()
2. **Feedback stability:** Hard limit at 0.95 (prevents self-oscillation)
3. **Spillover:** Report tail size via getTailLengthSeconds()
4. **Mono compatibility:** Haas effect may cause comb filtering (document in manual)
5. **Smooth transitions:** Use parameter smoothing for mode changes

## Next Steps

1. Run `/implement Ouaricon Digital Delay` to start Stage 1 (Foundation)
2. Review architecture.md and plan.md before implementation
3. Test spillover behavior early (critical feature)
4. Test tempo sync with multiple DAWs (BPM reliability)

## Files

- Creative brief: `.ideas/creative-brief.md`
- Architecture spec: `.planning/architecture.md`
- Implementation plan: `.planning/plan.md`
- Continuation state: `.continue-here.md`

## Preset Management

Presets are stored in:
- **User presets:** `~/Library/O-DigiDelay/Presets/User/`
- **Factory presets:** `~/Library/O-DigiDelay/Presets/Factory/`

### Factory Presets (12)
| Preset | Description |
|--------|-------------|
| Short Slap | Quick 75ms slap-back delay |
| Long Ambient | 800ms with high feedback and mod |
| Stereo Wide | Medium delay with max spread |
| Subtle Doubler | Very short delay for thickening |
| Tape Echo | Classic tape-style with warmth |
| Eighth Note Sync | Tempo-synced 1/8 notes |
| Dotted Eighth | Classic U2-style dotted 8th |
| Triplet Feel | Tempo-synced triplets |
| Swell Pad | Long, wet, high feedback for pads |
| Lo-Fi Drift | Heavy modulation for lo-fi vibe |
| Clean Repeat | Clean repeats without modulation |
| Ping Pong Style | Max spread for wide stereo

**Last Updated:** 2026-02-07
