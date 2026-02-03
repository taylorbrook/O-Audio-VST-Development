# Milestone Context: Implement Placeholder Parameters

**Plugin:** O-Detune
**Version:** 1.2.0 → 1.3.0
**Created:** 2026-02-02

## Overview

O-Detune v1.2.0 has 21 declared APVTS parameters, but only 7 are implemented in `processBlock()`. The remaining 14 parameters are visible in the UI and bound to WebView controls, but don't affect the audio output. This milestone implements all placeholder parameters with authentic vintage processing character.

## Scope: All 14 Placeholder Parameters

### Wobble Engine (4 parameters)

| Parameter | Type | Range | Purpose |
|-----------|------|-------|---------|
| `wobble_era` | Choice | 60s/70s/80s | Era character preset - affects LFO behavior and filtering |
| `wobble_shape` | Choice | Sine/Triangle/Random | LFO waveform selection |
| `wobble_sync` | Bool | On/Off | Tempo sync - read DAW BPM, sync to musical divisions |
| (feedback affects wobble recirculation) | | | |

### Unison Engine (4 parameters)

| Parameter | Type | Range | Purpose |
|-----------|------|-------|---------|
| `unison_voices` | Choice | 2/3/4/5/7 | Active voice count |
| `unison_dist` | Choice | Linear/Exp/Random | Voice pitch distribution algorithm |
| `unison_spread` | Float | 0-100% | Stereo panning width between voices |
| `random_amt` | Float | 0-100% | Per-voice pitch/timing variation |

### Character Section (3 parameters)

| Parameter | Type | Range | Purpose |
|-----------|------|-------|---------|
| `drive` | Float | 0-100% | **Tube-style saturation** - soft clipping with harmonics |
| `color` | Float | -100 to +100 | **Analog-modeled tone** - dark (LP) to bright (HP shelf) |
| `age` | Float | 0-100% | **Combined degradation** - tape hiss + filter drift |

### Output Section (3 parameters)

| Parameter | Type | Range | Purpose |
|-----------|------|-------|---------|
| `width` | Float | 0-200% | Stereo width (0=mono, 100=normal, 200=extra-wide) |
| `mono_safe` | Bool | On/Off | Apply mid/side processing to ensure mono compatibility |
| `delay` | Float | 0-50ms | Pre-delay before processing for spatial depth |
| `feedback` | Float | 0-80% | Delay recirculation for resonance |

## Processing Character

**Authentic Vintage Processing** (user selection):
- **Drive**: Tube-style saturation using tanh waveshaping with even harmonics. Subtle warmth at low settings, classic tube breakup at high.
- **Color**: Analog-modeled filter character. Negative = low-pass filtering (dark, woolly). Positive = high-shelf boost (bright, present).
- **Age**: Combined degradation effects:
  - Hiss: Low-level broadband noise scaled by age amount
  - Filter drift: Subtle random modulation of color filter cutoff
  - Wow/flutter enhancement: Subtle random variation added to wobble LFO

## Tempo Sync Specification

**DAW Tempo Integration**:
- Read host BPM from `AudioPlayHead` position info
- Convert `wobble_rate` Hz to musical divisions when sync enabled
- Supported sync divisions: 1/8, 1/4, 1/2, 1 bar, 2 bars
- Fallback to 120 BPM if host position unavailable

## Acceptance Criteria

1. All 14 parameters audibly affect the output signal
2. DAW automation works for all parameters
3. Preset save/restore preserves all parameter values
4. No audio artifacts (clicks, pops, zipper noise) during parameter changes
5. CPU usage increase < 10% compared to v1.2.0
6. Pluginval passes all tests
7. Build succeeds for VST3 and AU formats

## Out of Scope

- New UI elements (all parameters already have UI controls)
- New presets (will add in future version)
- Sample-accurate tempo sync (beat-synced LFO phase reset)
