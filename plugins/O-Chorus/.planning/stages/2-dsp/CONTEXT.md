# Stage 2: DSP - Context

## Discussion Summary

**Date:** 2026-02-07
**Participants:** User, Claude

## Requirements Confirmed

- Multi-voice BBD-style chorus engine (1-8 voices)
- Lagrange3rd interpolation for delay lines (clean code first, optimize later if needed)
- Per-voice LFO with fixed phase distribution: `(2pi * voiceIndex) / numVoices`
- Per-voice depth randomization (0.85-1.15 multiplier, seeded for repeatability)
- Tanh saturation with asymmetry for analog BBD warmth
- One-pole tone filter: 2kHz-20kHz mapped from Tone param (-1.0 to +1.0)
- Equal-power stereo panning across voice array
- Mono sum input for phase coherence
- Linear dry/wet mix crossfade

## Parameters (7 total)

| ID | Type | Range | Default | Notes |
|----|------|-------|---------|-------|
| rate | Float | 0.05-5.0 | 1.0 | LFO speed, skew 0.35 |
| depth | Float | 0.0-1.0 | 0.5 | Modulation amount |
| voices | Int | 1-8 | 4 | Voice count |
| width | Float | 0.0-1.0 | 0.7 | Stereo spread |
| tone | Float | -1.0-1.0 | 0.0 | High-freq rolloff |
| mix | Float | 0.0-1.0 | 0.5 | Dry/wet blend |
| drive | Float | 0.0-1.0 | 0.3 | Saturation amount (NEW - added during discuss) |

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Saturation control | Adjustable via Drive param (0.0-1.0, default 0.3) | User wants control over analog warmth amount |
| Voice count changes | Smooth crossfade (~50ms) in processBlock | Prevents clicks when changing voice count during playback |
| CPU optimization | Clean code first (std::tanh, no SIMD) | Optimize only if profiling shows issues |
| DC blocker | Skip | ScopedNoDenormals sufficient, most DAWs handle DC internally |
| Interpolation | Lagrange3rd | Balance of quality and CPU, industry standard for chorus |
| Input topology | Mono sum (L+R)/2 | Phase coherence, predictable stereo image |

## Constraints Identified

- Max 8 voices with independent delay lines
- ScopedNoDenormals for denormal prevention (no DC blocker)
- No memory allocation in processBlock (pre-allocate in prepareToPlay)
- Parameter reads via atomic loads from APVTS
- Voice count changes handled smoothly (crossfade, no clicks)

## Architecture Reference

Full DSP specification: `plugins/O-Chorus/.planning/research/ARCHITECTURE.md`

Key sections:
- Section 2.1: Multi-Voice Delay Line Engine
- Section 2.2: LFO Modulation System
- Section 2.3: Analog Saturation
- Section 2.4: Tone Control
- Section 2.5: Stereo Imaging
- Section 3: Processing Chain (signal flow)
- Section 6: Algorithm Details (code snippets)

## Foundation State

Stage 1 complete:
- 7 parameters in APVTS (rate, depth, voices, width, tone, mix, drive)
- WebView editor with relays and attachments for all 7 params
- VST3 and AU build successfully
- Pass-through audio (no DSP yet)

## Open Questions

None - all approach decisions confirmed.

## Next Phase

Ready for: research/plan phase
