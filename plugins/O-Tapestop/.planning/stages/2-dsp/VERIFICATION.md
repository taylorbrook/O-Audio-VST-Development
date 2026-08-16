# Stage 2: DSP - Verification

## Verification Date

2026-08-15

## Goal-Backward Analysis

### Original Goals (from CONTEXT.md / PLAN.md)

1. Complete varispeed DSP: single interpolated playhead over a 26 s capture ring with curve-morph stop/start ramps (FUNC-01, FUNC-04, DSP-01, DSP-02)
2. Signalsmith resync: fall-behind → 1.25× catchup → 50 ms crossfade-skip, bitwise post-resync null; retrigger honored in every state; tempo sync (DSP-03, FUNC-03)
3. Drawn-envelope scratch mode with reverse playback + speed-tracking toneTrack LPF (FUNC-02, DSP-04, DSP-05)
4. Real-time safety and block-size-invariant, artifact-free output (PERF-01, QUAL-01)
5. Resolve the three CONTEXT decisions in code (stopped-hold debt clamp, retrigger-everywhere, aliasing-as-character) and the deferred skip-splice A/B

### Deliverables (from SUMMARY.md, confirmed by code inspection)

1. `CaptureBuffer.h` (trimmed port), `VarispeedVoice.h` (Catmull-Rom + integer fast path), `TapestopTransport.h` (full state machine, carrier-voice architecture) — goals 1–2
2. Catchup/ResyncXfade states, orthogonal crossfade engine with force-complete 2-voice pool, O-Polystutter tempo-sync port — goal 2
3. `ScratchEnvelope.h` (message-thread bake, atomic double-buffered LUT, versioned JSON persistence), toneTrack `FirstOrderTPTFilter` on the absolute 16-sample grid with engage-edge prime — goal 3
4. Render harness (`tests/render-harness/`, 47 probes) covering determinism, invariance, nulls, debt bounds, pathological inputs — goal 4
5. All three CONTEXT decisions implemented (stored-position debt clamp; retrigger in every state incl. ResyncXfade-retrigger-on-B; no anti-alias filter at |r| > 1); A/B resolved: **equal-power ships** (evidence in NOTES.md) — goal 5

### Goal Achievement

| Goal | Status | Evidence |
|------|--------|----------|
| Core varispeed stop/start | ✅ Achieved | P3 curve-law pitch traces (8 checks), P6 3×3 discontinuity grid, mid-ramp reversal probes — all green on fresh run |
| Resync + tempo sync | ✅ Achieved | P2 post-resync bitwise null, RRT mid-fade retrigger, TOG 10 Hz stress, 3 SYNC probes, P1c invariance-with-sync — all green |
| Scratch + toneTrack | ✅ Achieved | scratch plays-once / direction-flip / mode-switch-silent, P4/P4b debt bounds, toneTrack centroid + transparency — all green |
| RT-safety + invariance | ✅ Achieved | Independent grep audit at verify: only allocation is `CaptureBuffer::prepare` (prepareToPlay); JSON/bake confined to get/setStateInformation; P0/P1a/P1b/P1c bit-identity green |
| CONTEXT decisions + A/B | ✅ Achieved | Stored-position clamp proven by P4b (27 s hold, maxDebt == bound exactly, clean tail); equal-power decision recorded with measured evidence (bump −0.48 dB / dip −6.21 dB vs linear −0.58/−6.99) |

## Requirements Verification

**Stage:** 2-dsp
**Requirements for this stage:** 11 total (7 must, 4 should)

| Requirement | Priority | Status | Acceptance Criteria |
|-------------|----------|--------|---------------------|
| FUNC-01: Engage/disengage lifecycle | must | ✅ Complete | Automation-driven gesture probes green; mid-ramp reversal click-free (maxDiff 0.0076 vs bound 0.0553); UI-click parity is Stage-3 scope |
| FUNC-02: Stop/Scratch modes | must | ✅ Complete | scratch-lut-plays-once (pitch trace follows envelope); scratch-mode-switch-silent bitwise (48000-sample memcmp) |
| FUNC-03: Tempo/free timing | must | ✅ Complete | SYNC-next-gesture-tracks-bpm; SYNC-latch-no-retarget; free time within one block (onset 27904 vs ~27818) |
| FUNC-04: Silent while stopped | should | ✅ Complete | P4b holdMag = 0.00000000 over the Stopped window |
| DSP-01: Click-free varispeed | must | ✅ Complete | P6 grid {50 ms, 500 ms, 8 s} × {0, 50, 100}% all under bound; P1a/P1b 512-vs-4096 + ragged bit-identity |
| DSP-02: Curve morph, x² default | must | ✅ Complete | P3: curve 50% matches (1−u)² within 8% at 3 u-points; curve 0/100 diverge in the expected directions |
| DSP-03: Resync null | must | ✅ Complete | P2 bitwise memcmp null from one crossfade after Catchup; P1a post-resync tail dry |
| DSP-04: Bipolar scratch reverse | should | ✅ Complete | P4 full-reverse: fRev 872.7 Hz vs 880 expected (coherent old-content read); direction flip maxDiff 0.0028 |
| DSP-05: toneTrack RT-safe LPF | should | ✅ Complete | Centroid falls (a60-vs-a0 attenuation grows −0.67 → −2.88 dB); a0 transparent (0.53 dB < 2.5 dB); coefficient updates on the absolute 16-sample grid (invariance green with it active) |
| PERF-01: RT-safe processBlock | must | ✅ Complete | Independent verify-phase grep: no alloc/lock/JSON in the audio path; 47-probe suite (~5 M samples) in 0.16 s user time |
| QUAL-01: No artifacts, invariance | must | ✅ Complete | P0 determinism; P1a/b/c invariance; P5 silence/DC/impulse/sine all finite with bitwise-dry tails (no sticky state) |

**Requirements Summary:**
- ✅ Complete: 11
- ⚠️ Partial: 0
- ⏸️ Deferred (later stage): 2 (UI-01, UI-02 — stage-3)
- ❌ Failed: 0

## Automated Checks

| Check | Result | Notes |
|-------|--------|-------|
| Build (`OuariconTapestop_VST3`, `_AU`, `O-Tapestop-render-test`) | ✅ Pass | ninja clean, no work to do on committed tree |
| Render harness (fresh run at verify) | ✅ Pass | **47/47 probes, exit 0** — independently re-run, not taken from SUMMARY |
| Phase commits | ✅ Pass | 2.1 `bae01154`, 2.2 `5c3a7cde`, 2.3 `a6e0cf85` all present |
| Allocation/lock audit (independent grep) | ✅ Pass | `setSize` only in `CaptureBuffer::prepare`; `new`/`JSON::parse` only in editor factory + message-thread state paths |
| Working tree | ✅ Pass | `git status` clean for plugins/O-Tapestop/ |

## Human Verification

- [ ] Load in DAW, engage via automation and UI: spin-down/spin-up character matches intent at default curves
- [ ] Audition scratch mode with the default wobble envelope; judge aliasing-as-character at |r| > 1
- [ ] Judge the resync splice on real program material (equal-power decision was made on worst-case probe material)

## Issues Found

- None at verify. In-phase deviations (onset linear-interp zone, header-only ScratchEnvelope, toneTrack engage prime, P4 17 s pre-roll, stored-position clamp) are all documented in NOTES.md with rationale and are consistent with the contracts.
- Known limitation carried forward (documented, accepted): sync division table assumes 4/4; a 3/4 host bar runs 4/3 long on bar-denominated divisions (NOTES.md).

## Stage Verdict

**Status:** ✅ VERIFIED

**Ready for next stage:** Yes (Stage 3 — GUI)

**Blockers:** None
