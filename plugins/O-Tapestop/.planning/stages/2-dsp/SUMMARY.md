# Stage 2: DSP — Execution Summary

**Date:** 2026-08-15
**Agent:** dsp-agent (3 phase dispatches + 2 fix rounds)
**Result:** Full DSP contract implemented — FUNC-01..04, DSP-01..05, PERF-01, QUAL-01. Render harness: **47/47 probes, exit 0.** Three phase commits as planned.

## Phase Commits

| Phase | Commit | Gate evidence |
|-------|--------|---------------|
| 2.1 core varispeed + stop/start | `bae01154` | 27/27 probes, exit 0 |
| 2.2 resync + tempo sync | `5c3a7cde` | 36/36 probes, exit 0; A/B recorded in NOTES.md |
| 2.3 scratch + toneTrack | `a6e0cf85` | 47/47 probes, exit 0 |

## Components Built

- **CaptureBuffer** (`Source/dsp/CaptureBuffer.h`) — trimmed O-ReverseDelay port; 26 s stereo absolute-index ring (kCaptureSeconds derivation in PluginProcessor.h with static_assert); the only audio-path allocation, in `prepareToPlay`.
- **WindowLut** (`Source/dsp/WindowLut.h`) — Hann-only table; equal-power crossfade gains.
- **VarispeedVoice** (`Source/dsp/VarispeedVoice.h`) — POD voice + hand-rolled Catmull-Rom (Horner); integer fast path at d=0/r=1 (carries the DSP-03 bitwise null); 2-tap linear onset zone below kInterpGuard=4; release-build debt clamp.
- **TapestopTransport** (`Source/dsp/TapestopTransport.h`) — states Bypassed→SpinDown→Stopped→SpinUp→Catchup→ResyncXfade (+ScratchPass); curve morph r(u)=(1−u)^p, p=2^(2c), double-precision u; mid-ramp reversal seed u₀=clamp(r₀)^(1/p); carrier-voice architecture with retrigger honored in every state; orthogonal crossfade engine (never restarts mid-fade); stored-position debt clamp on both voice advances; engaged-trim blend so non-default OUTPUT_GAIN cannot step at the resync splice.
- **ScratchEnvelope** (`Source/dsp/ScratchEnvelope.h`, header-only) — Path C curve eval → message-thread bake at 2048 φ, r=2y ∈ [−2,+2]; strict JSON sanitize ("v":1, reject-to-default); double-buffered atomic publish, LUT load-acquire once at the engage edge; default wobble never-null; persisted as `scratchEnvelopeJson` on the APVTS tree.
- **toneTrack** — FirstOrderTPTFilter LP, wet/engaged only; fc = 20000·(150/20000)^(a·(1−min(|r|,1))); absolute 16-sample update grid (`totalWritten & 15`); engage-edge prime `reset(wet)` instead of zero-reset (zero state rings — a guaranteed P6 click).
- **Tempo sync** — O-Polystutter fallbacks (120 BPM default, jlimit 20–999); sole consumer is the gesture-edge duration latch; division table assumes 4/4 (NOTES.md).
- All 14 parameters wired.

## Key Decisions Made In-Phase

- **Skip-splice A/B (CONTEXT open question): equal-power ships.** Measured: equal-power bump −0.48 dB / dip −6.21 dB vs linear −0.58 / −6.99 — better on both. Dip is splice physics (anti-phase correlated content), masked by the foregrounded catchup gesture. Both laws stay compiled behind `OUARICON_RENDER_HARNESS` for reproducibility. Fallback (repeated small skips) not needed, not built. Full evidence: NOTES.md.
- **Bypassed is a zero-arithmetic pass-through.** The planned "OUTPUT_GAIN also in bypass" idea was disproven: APVTS normalized round-trip of 0 dB over the asymmetric −24..+12 range yields gain ≈1.0000001, which broke the bitwise null at sample 0. MIX/OUTPUT_GAIN now apply inside the engaged branch only.
- **Deviations from PLAN.md** (all in NOTES.md): onset linear-interp zone instead of debt clamp-up; header-only ScratchEnvelope (avoids touching frozen CMake); toneTrack engage prime; P4 engages at 17 s so the reverse-read coherence window is non-vacuous; stored-position debt clamp extension.

## Probe Suite (tests/render-harness/, 47 checks)

P0 determinism · bypass bitwise null · P1a/P1b/P1c block-size invariance (incl. sync + toneTrack active) · post-resync bitwise-dry tail · P2 post-resync null (DSP-03) · P3 pitch-trace curve law (DSP-02) · P6 3×3 discontinuity grid (DSP-01) · mid-ramp reversal · RRT mid-fade retrigger · TOG 10 Hz stress · SYNC latch/tracking/free (FUNC-03) · AB splice evidence · P4/P4b debt bounds incl. 27 s hold (bounds parsed from source, not harness literals) · P5 pathological inputs (QUAL-01) · toneTrack centroid + transparency (DSP-05) · scratch plays-once/direction-flip/mode-switch-silent (FUNC-02, DSP-04).

Harness lesson recorded: the first-difference "hf drop" metric is structurally blind to one-pole rolloff (weighting ∝ f cancels 1/f); toneTrack verdicts use band-energy attenuation growth instead (false-FAIL post-mortem in NOTES.md).

## PERF-01

Allocation/lock grep audit clean (allocations only in constructor/prepareToPlay/message-thread paths). Full 47-probe suite (~5M rendered samples) completes in 0.16 s user time — far under the 5% single-core @ 48 kHz budget.

## Build State

`OuariconTapestop_VST3` + `_AU` + `O-Tapestop-render-test` all build clean; harness exit 0 on the committed tree. Plugin not yet reinstalled to system folders (verify phase / install step owns that).
