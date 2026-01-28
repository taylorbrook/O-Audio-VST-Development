# Phase 4: Controls & Refinement - Context

**Gathered:** 2026-01-24
**Status:** Ready for planning

<domain>
## Phase Boundary

All 4 parameters (Frequency, Enhance, Output, Mode) function with musical behavior and auto-limiting to prevent over-processing. DSP implementation exists from Phases 1-3; this phase tunes response curves, adds missing controls, and addresses intensity issues at low crossover frequencies.

**Known issues to address (from Phase 3 verification):**
- Colored mode is more subtle than Clean (should be comparable)
- Both modes need more prominent processing at low crossover frequencies
- Effect only noticeable at crossover ~200Hz, needs work at lower frequencies

</domain>

<decisions>
## Implementation Decisions

### Intensity Balance
- Tune both modes so 50% Enhance feels similar in impact, but Colored stays warmer
- Auto-compensate at low crossover frequencies (40-80Hz) — lower crossover = more drive/harmonics automatically
- Colored mode uses full even spectrum (2nd + 4th harmonics) for richer analog warmth
- Extend bandpass filter lower to 40Hz (was 60Hz) to allow harmonics closer to fundamental

### Enhance Curve
- Keep current sqrt() curve for diminishing returns — fast ramp early, slower at top
- 100% = maximum intensity, auto-limiting kicks in if needed
- Visual indicator when Enhance is near/at the auto-limit threshold (color or meter)

### Output Compensation
- Enhancement adds perceived loudness — allow slight boost at default settings (not strict unity gain)
- 0dB detent (snap to center) for easy return to neutral
- Soft clipper at extreme Output values (+15 to +18dB) to prevent digital clipping

### Auto-Limiting
- Gentle character is acceptable — slight pumping adds life, doesn't need to be invisible
- Early/conservative threshold — limit starts below the danger zone
- Visual feedback when limiting is active (tied to Enhance indicator)

### Claude's Discretion
- Enhance=0% behavior: true bypass vs crossover-active (based on latency/CPU trade-offs)
- Output auto-gain approach: pure manual, auto+trim, or switchable (choose for best ease of use)
- Per-mode ceiling tuning: same -2dB for both modes, or Colored lower for saturation headroom
- Adaptive limiting based on crossover frequency (investigate if needed)

</decisions>

<specifics>
## Specific Ideas

- At default 50% Enhance, both modes should feel equally impactful but with different character
- Low-frequency enhancement (40-80Hz crossover) should be noticeably more aggressive than high (180-200Hz)
- The limit indicator can be subtle — a color shift or small meter, not an alarm

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope

</deferred>

---

*Phase: 04-controls-refinement*
*Context gathered: 2026-01-24*
