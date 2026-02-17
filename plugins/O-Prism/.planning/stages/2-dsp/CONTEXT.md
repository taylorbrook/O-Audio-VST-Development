# Stage 2: DSP - Context

## Discussion Summary

**Date:** 2026-02-16
**Participants:** User, Claude

## Requirements Confirmed

- Procedural wavetables only for Phase 2.1 (saw, square, sine, triangle generated at startup). Real wavetable files deferred to Stage 4 or separate pass.
- Full 10 mipmap levels for anti-aliasing (industry standard quality, ~20MB per table with float storage). No reduced-level compromise.
- All 6 noise types implemented (White, Pink, Brown, Digital, Vinyl, Wind). No scope reduction.
- All 7 filter types including BP24 via cascaded 2x BP12 SVF stages.
- Sub oscillator routes direct to output (bypasses filters), with amplitude envelope. Standard Serum-like behavior.
- No dynamic voice limiting — user manages their own CPU. Full 16 voices x 8 unison allowed.
- 2x oversampling on distortion effect for clean aliasing rejection.

## Constraints Identified

- Memory: ~20MB per loaded table (float mipmap storage). Only active tables loaded — lazy loading strategy required.
- CPU worst-case: ~50% single core at 16 voices x 8 unison x 2 oscillators. Accepted by user — no auto-limiting.
- 74 APVTS parameters already in place from Stage 1 (not 68 as originally estimated).
- `numSliderParams` bug in PluginEditor.h (67 vs 73) — fix deferred to Stage 3.
- PrismVoice currently a stub with `float` audio buffer (`renderNextBlock` takes `AudioBuffer<float>&`). Internal processing will use double precision, convert to float for output.

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Factory wavetables | Procedural only (Phase 2.1) | Fastest path to hearing sound; real tables sourced later |
| Mipmap levels | 10 (full quality) | Industry standard, best anti-aliasing across MIDI range |
| Effects chain order | Dist -> Chorus -> Delay -> EQ -> Reverb | EQ before reverb for tone shaping before spatial processing |
| Distortion oversampling | 2x | Clean distortion without excessive CPU cost |
| CPU management | No limiting | User manages polyphony; no auto-reduction with high unison |
| Noise types | All 6 | Full implementation as specified in architecture |
| BP24 implementation | Cascade 2x BP12 SVFs | Standard approach, tighter resonance peak |
| Sub oscillator routing | Direct to output | Bypasses filters, clean sub bass, Serum-like behavior |

## Architecture Changes from Original Plan

1. **Effects chain order modified:** EQ moved before Reverb (was after). New order: Distortion -> Chorus -> Delay -> EQ -> Reverb. This allows tonal shaping before spatial processing.

2. **All other architecture decisions confirmed as-is** from ARCHITECTURE.md.

## DSP Phase Breakdown (from ROADMAP.md)

| Phase | Goal | Key Components |
|-------|------|----------------|
| 2.1 | Basic wavetable playback | WavetableOscillator (Osc A), amp ADSR, TuningEngine integration, procedural test tables |
| 2.2 | Mipmap + Osc B + mixing | FFT-based mipmap generation (10 levels), Osc B activation, oscMix crossfade, level/pan |
| 2.3 | Unison + sub + noise + glide | UnisonEngine (1-8 voices), SubOscillator (polyBLEP), NoiseGenerator (6 types), GlideProcessor |
| 2.4 | Dual filters + filter envelope | SVFFilter wrapper (7 types, 24dB cascading), drive, key tracking, filter ADSR, serial/parallel routing |
| 2.5 | Effects chain + master | Distortion (4 types, 2x OS), Chorus, Delay (ping-pong, sync), EQ (3-band), Reverb (pre-delay), master volume |

## Open Questions

- None. All approach decisions confirmed.

## Key References

- Architecture: `plugins/O-Prism/.planning/research/ARCHITECTURE.md`
- Roadmap: `plugins/O-Prism/.planning/ROADMAP.md`
- Stage 1 Verification: `plugins/O-Prism/.planning/stages/1-foundation/VERIFICATION.md`
- Reference synth code: `plugins/O-Lyrica/Source/` (voice architecture pattern)
- Tuning module: `modules/tuning/scala-tuning-engine/` v2.1.0
- Critical patterns: `troubleshooting/patterns/juce8-critical-patterns.md`

## Next Phase

Ready for: **research** phase
