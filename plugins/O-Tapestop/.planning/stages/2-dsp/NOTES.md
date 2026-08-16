# Stage 2: DSP — Implementation Notes

**Date:** 2026-08-15 (Phase 2.2)

## Division table assumes 4/4

The sync division table {1/16, 1/8, 1/4, 1/2, 1 bar, 2 bars, 4 bars} maps to
beats {0.25, 0.5, 1, 2, 4, 8, 16} — i.e., "1 bar" = 4 beats. This assumes a
4/4 time signature (suite precedent; O-Polystutter does the same). No
time-signature read exists in v1.0; a 3/4 host bar will run 4/3 long on the
bar-denominated divisions. `durationSamples = beats · (60/bpm) · fs`,
converted ONCE at the gesture edge (latch contract — a live ramp never
retargets on a tempo change).

BPM fallbacks (O-Polystutter pattern): no playhead / offline / missing
`getBpm()` → 120; clamp `jlimit(20, 999)`.

## Skip-splice gain-law A/B (CONTEXT open question)

Both laws are compiled in `TapestopTransport` and selected via
`setSpliceLawForTest(bool linear)` (behind `OUARICON_RENDER_HARNESS`):

- **Equal-power:** `fadeOut = hann(0.5 + φ/2)`, `fadeIn = hann(φ/2)` —
  sin²+cos² = 1. Correct for UNCORRELATED material; over-sums correlated
  material by up to +3 dB at the fade midpoint.
- **Linear:** `fadeOut = 1 − φ`, `fadeIn = φ` — amplitude-complementary.
  Correct for correlated material; dips up to −3 dB (power) on uncorrelated.

**Methodology (harness probe AB):** identical sustained pad (220 Hz sine bed +
one-pole low-passed noise) rendered through a full stop/start/resync cycle
under each law; the splice blends content ~0.3 s apart (the sine bed is
highly self-correlated at a deterministic phase offset). Metric: max/min
480-sample short-RMS inside the fade window ± one window, in dB relative to
the post-resync steady RMS.

**Evidence (harness run, 2026-08-15, 36/36 green):**

| Law         | bump (max short-RMS vs ref) | dip (min short-RMS vs ref) |
|-------------|-----------------------------|-----------------------------|
| Equal-power | **−0.47 dB**                | **−6.20 dB**                |
| Linear      | −0.57 dB                    | −6.98 dB                    |

**Decision: SHIP EQUAL-POWER** (the ARCHITECTURE construction stands).

Rationale:
- Neither law over-sums on this material — the feared +3 dB correlated bump
  never materialises (both "bumps" are actually slightly below reference).
- Equal-power measures better on BOTH metrics and keeps the splice region
  ~0.8 dB fuller at the dip.
- The −6.2 dB dip is NOT a gain-law artifact: it is phase cancellation of the
  correlated 220 Hz bed between content ~0.3 s apart, and both laws show it
  within 0.8 dB of each other. At the exact fade midpoint (fadeOut = fadeIn)
  an anti-phase correlated component cancels under ANY complementary law —
  that is splice physics, and its depth is content-phase-dependent (a
  different stop length shifts the offset and the dip anywhere from 0 to a
  deep null on pathological single-tone material).
- **Caveat, judged acceptable:** the dip is a 480-sample-window minimum inside
  a 50 ms fade, arriving immediately after 250 ms of audible 1.25× catchup
  motion — the rejoin is a foregrounded DJ-release gesture, not a transparent
  edit, so a brief level valley on sustained correlated material is masked by
  the gesture's own motion. Real program material is less self-correlated
  than the probe's deliberate worst-case sine bed.
- Fallback (repeated small crossfade-skips during Catchup) is NOT needed and
  remains unbuilt, per plan.

Both laws stay compiled behind the harness flag so the A/B remains
reproducible; the AB probe continues to print the numbers on every run.

## Engaged-trim blend (OUTPUT_GAIN vs the bitwise bypass)

The Bypassed path is a TRUE hard pass-through (no arithmetic — the APVTS
normalized round-trip of the 0 dB default over −24..+12 stores ~1.2e-6 dB, so
`decibelsToGain` is ≈ 1.0000001f, never exactly 1.0f; any multiply flips
bits). OUTPUT_GAIN therefore acts on the engaged chain only, applied as
`1 + (g − 1) · trimAmount`, where the transport ramps `trimAmount` 1 → 0
across exactly the ResyncXfade window and back toward 1 in every engaged
state. Consequences:

- The resync→Bypassed handoff lands on gain EXACTLY 1.0 — a non-default trim
  cannot step at the splice (Phase-2.1 deferred item, resolved).
- On engage, a non-default trim glides in over 50 ms instead of stepping.
- At the default trim the whole term is ~1e-7 and invisible to every probe.

## Two-voice pool: fade force-complete

Fades start only at resync entry and catchup-retrigger. If a new fade must
start while one is still running (reachable only when a full
release→spin-up→catchup cycle fits inside one 50 ms fade — sub-20 ms gesture
chains), `startXfade` force-completes the old fade by dropping the old fading
voice; its residual gain `fadeOut(φ)` is small by then in every musical
timing. This is the mechanism that keeps the pool at exactly 2 voices — a
third voice is structurally unreachable, per the ReverseGrain contract.
