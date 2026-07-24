# Stage 2: DSP - Context

## Discussion Summary

**Date:** 2026-07-23
**Participants:** User, Claude

Stage 2 implements the ARCHITECTURE.md contract (immutable, checksummed) across three roadmapped phases: 2.1 core reverse wet path (feedback OFF), 2.2 feedback + damping + stability, 2.3 tempo sync + width + parameter polish. The architecture already fixes the algorithm; this discussion resolved the three items ARCHITECTURE.md explicitly deferred to Stage 2, plus workflow gates.

## Requirements Confirmed

Stage-2 verifies 11 requirements: FUNC-01..04, DSP-01..04, PERF-01, COMPAT-02, QUAL-01 (see REQUIREMENTS.md acceptance criteria — all defined as offline-render assertions or code-review checks).

- FUNC-01: granular reverse smear (reverse bloom, not chunked blocks) — single-grain reversed-ramp probe + impulse envelope-slope probe
- FUNC-02: Sync (13 divisions incl. dotted/triplet) + Free (50–2000 ms); 120 BPM 1/4 → 500 ms ±1 block
- FUNC-03: damped regeneration; feedback=0 → exactly one generation
- FUNC-04: per-grain equal-power width spread
- DSP-01: Hann-windowed grains, click-free; density sweep loudness-flat ±1 dB
- DSP-02: ArrayCoefficients in-place IIR updates, no audio-thread allocation
- DSP-03: 60 s render @ 100% feedback, default damping — below ceiling, zero NaN/Inf
- DSP-04: log-skewed ranges already in APVTS (Stage 1); presets in engineering units (Stage 4 consumes)
- PERF-01: no alloc/locks/logging in processBlock; pluginval-10 ×3
- COMPAT-02: no-playhead harness pass falls back to free delayTime
- QUAL-01: automated all-parameter sweep pass, no clicks/zipper/NaN/Inf

## Constraints Identified

- ARCHITECTURE.md is the immutable contract: reverse read law (offset D+2n over 3.5 s capture ring), per-grain latching of D/G/pan/gain, overlap = 1 + (density/100)·7 with 1/sqrt(overlap) compensation applied BEFORE the feedback tap, tanh loop stability, single shared capture buffer (alternating-direction regenerations = intended character), custom equal-power mix, 2nd-order Butterworth ArrayCoefficients damping with 0.49·fs clamp.
- Render harness is the FIRST deliverable of Phase 2.1 — it is the correctness gate for every phase (compiles PluginProcessor without editor; no WebView types exist yet, Stage 1 used GenericAudioProcessorEditor with zero editor-only includes).
- Per-block processing order is REQUIRED (ARCHITECTURE.md §Processing Order): BPM/D resolve → smoothers+coeffs → schedule → render grains → feedback return (gain→HP→LP→tanh→guard) → capture write → mix. Wet renders into a preallocated scratch buffer, never in-place over input.
- Suite footguns on the checklist: never Coefficients::makeXXX on audio thread; never memcpy raw 6-array over normalised 5-value storage; cached-cutoff guard gates only recompute; non-finite guard resets filters AND zeroes feedback source.
- Performance target: < 5% of one core @ 48 kHz / 128 samples (≤ 8 grains × 2 ch table-lerp + 4 biquads, no oversampling/FFT).
- Bus layouts from Stage 1 (D1): mono→mono, mono→stereo, stereo→stereo — grain engine and capture buffer must handle mono input feeding a stereo wet path.
- git commit per phase; harness green before advancing (complexity 5.0 staged-implementation rule).

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| D4: Grain stereo source under width | **Mono-sum source** — grain reads 0.5(L+R) from capture; equal-power pan places the mono grain | User choice. Cleaner, more focused ping-left/right placement; resolves the rule ARCHITECTURE.md left open ("mono-sum vs per-channel read"). Wet path discards input stereo image by design; dry path keeps it. |
| D5: Harness gate strictness | **Hard pass/fail exit codes** — every acceptance criterion is an automated assertion; non-zero exit on any failure | Phase advancement requires green; one-command re-run; CI-able later. Tuning constants (overlap compensation, pan bias) are tuned until assertions pass, then frozen in the assertion. |
| D6: Listening checkpoints | **One audition after Phase 2.3** — 2.1/2.2 advance on harness alone; Standalone listen at end of Stage 2 before verify | Confirms character (smear quality, feedback wash, width) once the full parameter surface exists; keeps 2.1/2.2 cadence fast. |
| Algorithm approach | Granular reverse engine per ARCHITECTURE.md (not relitigated) | Stage-0 decision, checksummed contract; fallback (dual crossfaded reverse heads) only if DSP-01 click criteria unpassable after tuning. |
| Phase structure | 2.1 → 2.2 → 2.3 exactly as ROADMAP.md | Stage-0 decision; per-phase commits + test criteria already enumerated. |

## Open Questions

Deferred to the harness (tuning, not decisions — resolved by making assertions pass):
- Exact overlap-compensation constant (±1 dB density-sweep flatness target)
- Alternating-sign pan bias amount for width spread
- Mono-input handling detail: with D4 mono-sum, mono→stereo layouts feed identical L/R capture channels — verify the 0.5(L+R) sum path degrades to identity, no 6 dB surprise (harness check in 2.3 width pass)

## Next Phase

Ready for: **research** phase (`/plugin-research O-ReverseDelay 2-dsp`) — expected focus: harness scaffolding from O-simpleGrain's harness structure, DelayBuffer/GrainScheduler adaptation details from O-GrainScatter, probe implementations (single-grain autocorrelation direction probe, envelope-slope bloom probe, click detector).
