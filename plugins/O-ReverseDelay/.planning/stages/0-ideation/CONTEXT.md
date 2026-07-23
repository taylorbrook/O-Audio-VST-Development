# O-ReverseDelay — Stage 0 Context (Discuss Phase Findings)

**Date:** 2026-07-23
**Phase:** Research & Planning (Stage 0)

## What Was Decided

1. **Engine identity:** granular reverse smear (overlapping Hann-windowed reversed grains over a circular capture buffer) — NOT chunked block reversal. This is the plugin's defining sound and the FUNC-01 acceptance hinges on it (reverse *bloom*, not gated blocks).
2. **Reverse read law:** grain read offset from the write head grows +2 samples per output sample (`D + 2n`). Collision-free by construction (offset only increases); buffer sized 3.5 s for `Dmax + 2·Gmax`.
3. **Feedback topology:** single shared capture buffer — feedback return (HP → LP → tanh) sums with input at the write head. Consequence: regeneration direction alternates (rev/fwd/rev…), accepted as the classic hardware reverse-delay character. Always-reverse re-granulation documented as a revisit option only.
4. **Stability:** unity max loop gain + damping as the decay budget + tanh bound. No hidden 0.95 cap — "near self-sustaining" is a brief requirement.
5. **Damping filters:** 2nd-order Butterworth IIR with ArrayCoefficients in-place updates (DSP-02 mandates the pattern); TPT filters rejected (slope + requirement wording).
6. **Click-free strategy:** per-grain parameter latching (D, G, pan, gain snapshotted at spawn) — delay/grain/mode changes are click-free without smoothing; only feedback, mix, and cutoffs get SmoothedValue.
7. **Density semantics:** `overlap = 1 + density·7` (max 8), spawn interval `G/overlap`, gain `1/sqrt(overlap)` — compensation sits BEFORE the feedback tap so density never changes loop gain.
8. **Mix:** custom equal-power crossfade; DryWetMixer rejected (no latency to compensate — zero reported latency).
9. **Tempo sync:** 13 divisions (straight/dotted/triplet, 1/16…1/1), per-block `PositionInfo::getBpm()` Optional check, free-time fallback (COMPAT-02).

## Constraints Carried Forward

- Render harness FIRST (Phase 2.1) — all Stage-2 acceptance criteria are offline-render assertions; guard createEditor when WebView lands in Stage 3.
- No mockup exists yet — must be created before Stage 3; UI-02 needs sync/free conditional time-control display.
- 6 of 10 params log-skewed → factory presets in engineering units + convertTo0to1.
- Suite footguns explicitly designed around: ArrayCoefficients 5-vs-6 storage, cached-guard/enabled-flag leak, NaN-guard must reset source not just filters, pluginval-10 ×3 before release.

## Complexity & Strategy

- Tier 3 (complex DSP), MODERATE research depth. Score 5.0 (capped; 2.0 params + 4 algorithms + 1 feedback feature).
- Phased: DSP 2.1 (core wet path) → 2.2 (feedback/damping/stability) → 2.3 (sync/width/polish); GUI 3.1 → 3.2.
- Highest risk: reverse grain engine (~50%); mitigated by O-simpleGrain/O-GrainScatter prior art + single-grain harness probes. Fallback: dual crossfaded reverse heads.

## Key Files

- `plugins/O-ReverseDelay/.planning/research/ARCHITECTURE.md` — full DSP contract
- `plugins/O-ReverseDelay/.planning/ROADMAP.md` — phase breakdown + test criteria
- `plugins/O-ReverseDelay/.planning/REQUIREMENTS.md` — 14 requirements with acceptance criteria
