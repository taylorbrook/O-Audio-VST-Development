# O-Tapestop — Stage 0 Context (Discuss/Research Findings)

**Date:** 2026-08-15
**Phase:** Stage 0 Research & Planning (research-planning-agent)

## What was decided and why

### 1. Engine shape: single interpolated playhead, not periodic overlap-add grains
The brief says "rides on the O-ReverseDelay grain engine." Research honored the substrate but rejected the *periodic scheduler*:

- A continuous fractional playhead under any bounded speed law is click-free **by construction** — clicks only occur at position jumps, and every jump in this design is an explicit 50 ms windowed two-voice crossfade.
- Periodic forward grains at near-unit rate read coherent material and sum in **amplitude** against the pool's 1/√overlap **power** gain (documented suite memory pattern) — level error + comb coloration on clean ramp portions.
- DSP-03 requires a bitwise-dry null post-resync. An overlap-add engine can never emit bitwise dry; an integer-offset ring read can, and does.

**What IS reused:** CaptureBuffer verbatim (absolute-index ring), WindowLut (Hann-half crossfades), ReverseGrain's POD/latch-at-spawn/direction-field contracts (adapted to fractional position + Catmull-Rom), O-Polystutter tempo-sync + UI-bridge patterns. GrainScheduler: unused (spawns are event-driven, ≤2 voices).

**Fallback if scratch artifacts appear:** 25–50 ms Hann micro-grains at latched speed for the scratch pass only.

### 2. Curve law: r(u) = (1−u)^p with p = 2^(2c)
c=0 linear, c=0.5 **exactly x²** (DSP-02 by construction), c=1 quartic. Mid-ramp reversal is speed-continuous via inverse-curve seeding u0 = r0^(1/p_new).

### 3. Resync (DSP-03): fall-behind → 1.25× catchup (≤250 ms) → single 50 ms crossfade-skip
Skip lands on integer offset 0 at r=1 → null vs dry is exact. The up-to-~5 s content jump is the DJ-releases-the-record gesture, i.e. intended. Open aesthetic question (Phase 2.2 A/B): equal-power vs linear skip crossfade; fallback to repeated small skips if the single splice reads as a hard edit.

### 4. Scratch envelope: state blob + baked LUT, r = 2y ∈ [−2, +2]
- Point list {x, y∈[−1,1], curve} as versioned JSON string in APVTS state (not automatable).
- Message thread bakes 2048-pt speed LUT → atomic double-buffer publish; audio thread latches pointer **at the engage edge** (edits apply next pass).
- y=+0.5 is 1× (labelled grid line); negative y = reverse. Direction flips are position-continuous palindrome corners — authentic, not artifacts.

### 5. Ring sizing: kCaptureSeconds = 26.0
Worst case = full-reverse scratch: d(debt)/dt = 1−r, r=−2 → 3 s/s × 8 s = 24 s; +2 s margin. Stop-mode bound (~16 s) is inside it. Debt clamped with interp guard; jassert in debug (memory pattern: capture ring must span max playback debt).

### 6. Bypassed = hard pass-through
Disengaged output is bitwise input (ring still recording). Makes FUNC-02/DSP-03/QUAL-01 structural. MIX inert while disengaged — correct.

### 7. toneTrack: FirstOrderTPTFilter, log cutoff law, 16-sample absolute-grid updates
fc = 20k·(150/20k)^(a·(1−min(|r|,1))); engaged-only with state reset at engage edge; |r|>1 clamps open. Verified against local JUCE 8.0.14; precedent in O-Bells/O-Formant.

### 8. Block-size invariance plan (QUAL-01)
No RNG, no envelope followers (the two historical failure modes are absent). Remaining exposure: block-quantized ENGAGE edges (harness schedules edges on 4096-aligned boundaries) and cutoff update grid (anchored to absolute sample count, not block starts).

## Complexity & strategy
- Score 5.0 (capped; raw 8.0 = 2.0 params + 5 algorithms + 1 modulation feature) → **staged**: DSP phases 2.1 core stop/start → 2.2 resync+sync → 2.3 scratch+toneTrack; GUI phases 3.1 layout → 3.2 binding+engage → 3.3 envelope editor.
- Raw score overstates effort (3/5 components are proven substrate); brief's ~1-week estimate stands.

## Constraints carried into implementation
- Per-sample write-then-read on the ring (bit-identity structural)
- All durations/LUT pointers latched at gesture edges
- `#if JUCE_WEB_BROWSER` guard on createEditor from Stage 1 (harness survives WebView swap)
- No allocations/locks in processBlock; JSON strictly message-thread
- Zero latency; no setLatencySamples

## Open items for later stages
- No UI mockup yet — envelope editor (bipolar canvas, 1× line) is the design centerpiece; parameter set is fixed by this spec, mockup owns layout only
- parameter-spec-draft.md → promote to parameter-spec.md at mockup finalization
- Resync splice A/B decision recorded in Phase 2.2
