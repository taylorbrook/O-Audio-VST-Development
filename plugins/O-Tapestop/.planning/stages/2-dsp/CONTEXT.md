# Stage 2: DSP - Context

## Discussion Summary

**Date:** 2026-08-15
**Participants:** User, Claude

Stage-0 CONTEXT.md already locks the core algorithm decisions (single interpolated playhead, curve law p = 2^(2c), resync law, scratch LUT handoff, kCaptureSeconds = 26, bypass = hard pass-through, toneTrack law, block-size invariance plan). This discussion resolved only the three DSP questions that spec left open.

## Requirements Confirmed

- Stage 2 covers FUNC-01..04, DSP-01..05, PERF-01, QUAL-01 across three phases per ROADMAP.md (2.1 core stop/start → 2.2 resync + tempo sync → 2.3 scratch + toneTrack), git commit per phase, render-harness probes as gates.
- Performance target stands: < 5 % single core @ 48 kHz (2-voice interpolation worst case); memory allocated in prepareToPlay only.
- Resync splice aesthetic (equal-power vs linear skip crossfade) remains an in-phase A/B decision recorded during Phase 2.2 — intentionally not pre-decided.

## Constraints Identified

- Per-sample write-then-read on the ring; all durations/LUT pointers latched at gesture edges; no allocations/locks in processBlock; JSON strictly message-thread; zero latency (all carried from Stage 0).
- Harness must schedule engage edges on 4096-aligned boundaries; toneTrack cutoff updates on the absolute 16-sample grid.

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Stopped-hold / ring overrun | Ring keeps recording while Stopped; spin-up resumes the frozen playhead with **debt clamped to the ring bound** when the hold exceeded it (plays oldest valid material) | Crossfade-skip resync already absorbs any size of content jump, so clamping costs nothing musically; reads stay provably in-bounds (existing debt jassert covers it); authentic resume for the short gestures that dominate use. Rejected: pausing capture (breaks the per-sample write-then-read invariant) and threshold re-anchoring (behavioral discontinuity). |
| Retrigger in busy states | **Engage is honored in every state.** Catchup/crossfade-skip states hand off to the new gesture via a 50 ms two-voice crossfade from the current position; scratch re-engage mid-resync starts the new pass immediately. Stop-mode mid-ramp reversal (inverse-curve seed) unchanged. | Automatable performance trigger must have no dead zones; the Phase 2.2 10 Hz toggling stress test assumes this. Deferring edges until Bypassed would drop gestures. |
| Aliasing at \|r\| > 1 | **Accept as character** — Catmull-Rom only, no anti-alias filtering at catchup 1.25× or scratch ±2× | Matches Kilohearts Tape Stop / turntable emulations; mild at ≤2×; zero CPU and no filter-engage discontinuity to manage. toneTrack clamps open above 1× as specced. |

## Open Questions

- Resync skip-crossfade shape (equal-power vs linear; fallback repeated small skips) — deliberately deferred to the Phase 2.2 A/B on sustained material.

## Next Phase

Ready for: research phase (`/plugin-research O-Tapestop 2-dsp`)
