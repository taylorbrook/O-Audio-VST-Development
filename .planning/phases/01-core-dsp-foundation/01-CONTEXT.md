# Phase 1: Core DSP Foundation - Context

**Gathered:** 2026-01-22
**Status:** Ready for planning

<domain>
## Phase Boundary

Establish the audio processing architecture that all enhancement algorithms depend on: crossover filtering, mono bass summing, parameter framework, and latency management. This phase delivers the signal path — harmonic generation is Phase 2.

</domain>

<decisions>
## Implementation Decisions

### Crossover Behavior
- 24dB/octave (4-pole) Linkwitz-Riley filter slope
- Two modes: Low-latency (minimum-phase IIR) and High-fidelity (linear-phase FIR)
- User-exposed toggle to switch between modes
- Smooth coefficient interpolation when adjusting crossover frequency during playback (no clicks)
- Crossover range: 40-200Hz (from requirements)

### Mono Summing
- Hard sum (L+R)/2 below crossover — coherent source for harmonic generation
- Enhanced bass output has stereo toggle: mono (default) or match original L/R balance
- 5th control added: Stereo output mode toggle
- Above-crossover content passes through untouched (no processing, just recombined)

### Bypass Behavior
- Full bypass: 100% dry signal, no crossover, no mono summing — as if plugin wasn't there
- Zero-latency bypass: dry signal passes immediately (not delay-compensated)
- Instant toggle: no crossfade, accept possible small click
- Obvious visual indicator when bypassed (dimmed UI or clear state change)

### Latency Targets
- Low-latency mode: under 1ms (true real-time, accept quality tradeoffs)
- High-fidelity mode: no latency constraint (prioritize quality for mixing/mastering)

### Claude's Discretion
- Latency reporting strategy to host (always accurate vs fixed HiFi value)
- Buffer pre-allocation strategy (worst-case vs per-mode)
- Exact filter implementation details
- Crossfade behavior on mode switch

</decisions>

<specifics>
## Specific Ideas

- Low-latency mode should feel usable for live tracking without perceptible delay
- High-fidelity mode should be "whatever it takes" for quality — latency is secondary
- The latency mode toggle is architectural, not a "quality" slider — clear binary choice

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope

</deferred>

---

**Note:** Control count increased from 4 to 5 for v1.0:
1. Frequency (crossover)
2. Enhance (intensity)
3. Output (gain)
4. Mode (Clean/Colored)
5. Stereo (mono/stereo output) — NEW

This affects Phase 4 (Controls & Refinement) and Phase 5 (WebView UI) requirements.

---

*Phase: 01-core-dsp-foundation*
*Context gathered: 2026-01-22*
