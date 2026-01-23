# Phase 2: Clean Mode - Context

**Gathered:** 2026-01-23
**Status:** Ready for planning

<domain>
## Phase Boundary

Implement psychoacoustic harmonic generation that creates perceived bass on limited playback systems. The enhancement should be transparent with no audible artifacts, use 4x oversampling, and preserve transient character. This phase covers the "Clean Mode" algorithm only — Colored Mode saturation is Phase 3.

</domain>

<decisions>
## Implementation Decisions

### Harmonic Character
- Harmonic series selection (even vs odd emphasis): Claude's discretion based on psychoacoustic research
- Adaptive harmonic count based on input frequency — more harmonics for very low content, fewer for higher bass
- Pitch tracking for harmonics — harmonics follow the musical content, stay in tune with the bass
- Monophonic fundamental tracking — lock onto loudest bass note only, no polyphonic detection

### Intensity Curve
- Compressed response to input level — harmonics level out at higher inputs for consistent enhancement
- Enhance knob controls both harmonic amount AND level — richer enhancement at higher settings, subtle at lower
- Auto-limit ceiling on harmonic generation — prevents over-processing, no user override needed

### Frequency Behavior
- Crossover boundary transition: Claude's discretion for cleanest sound
- Extra harmonics for sub-bass (under 40Hz) — more aggressive enhancement where content is least audible
- Spectral-aware harmonic blending — reduce generated harmonics where high band already has energy to avoid buildup

### Transient Handling
- Balanced envelope timing — moderate attack/release that works for both drums and sustained bass
- Lookahead tied to latency mode — High Fidelity mode uses lookahead for clean transient capture, Low Latency mode relies on fast envelope follower
- Transient ducking enabled — momentarily reduce harmonics on sharp attacks to preserve punch

### Claude's Discretion
- Exact harmonic series weighting (odd vs even ratios)
- Crossover boundary transition shape (sharp vs soft)
- Envelope follower timing constants
- Lookahead duration in High Fidelity mode
- Transient detection threshold and ducking amount
- Spectral awareness algorithm specifics

</decisions>

<specifics>
## Specific Ideas

- "Enhancement should translate to perceived bass weight on laptop/phone speakers" — the psychoacoustic effect is the primary goal, not raw level boost
- Use existing latency_mode toggle to control lookahead behavior — maintains consistency with Phase 1 architecture
- Professional reference: MaxxBass-style psychoacoustic approach with pitch tracking and controlled harmonic generation

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope

</deferred>

---

*Phase: 02-clean-mode*
*Context gathered: 2026-01-23*
