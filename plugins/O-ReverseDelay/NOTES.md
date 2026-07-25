# O-ReverseDelay Notes

## Status
- **Current Status:** 📦 Installed
- **Version:** 1.3.0
- **Type:** Audio Effect (Granular Reverse Delay)

## Overview

Ambient granular reverse delay: the wet signal is assembled from overlapping Hann-windowed reversed grains over a circular capture buffer — a continuous reverse smear rather than chunked backwards blocks. Tempo-synced (dotted/triplet divisions) or free-time, with damped feedback (in-loop low-cut/high-cut) that stays loop-stable at 100% via tanh soft-clip.

**Complexity:** 5.0 (capped) — phased implementation (DSP 3 phases, GUI 2 phases)

## Lifecycle Timeline

- **2026-07-23:** Ideated — creative brief, requirements (14), draft parameters (10)
- **2026-07-23 (Stage 0):** Research & Planning complete — ARCHITECTURE.md and ROADMAP.md documented (Complexity 5.0, staged implementation)
- **2026-07-24 (Stages 1–4):** Implemented and verified — DSP engine, WebView editor, 8 factory presets, preset bar, tooltips. Shipped **v1.0.0** (harness 41/41, pluginval-10 ×3 both formats, auval SUCCEEDED, user DAW sign-off)
- **2026-07-24 (v1.0.1):** Patch — A1 sync clamp (delayTime max 2000→4000 ms, ring 3.5→5.5 s, user-preset migration), A2 unwritten-capture read at large block sizes (engine pass bounded to D — now block-size invariant), A3 density remap (`overlap = 2 + d·6`, removes the zero-overlap tremolo region), plus `reset()` override, dry-through on the oversized-block bail, and dead-code removal. Harness 41→49 probes.
- **2026-07-24 (v1.1.0):** Minor — grain randomisation (review B3): `jitter`, `delayScatter`, `sizeRandom`, `gainRandom`, all defaulting to 0 and all latched per grain. `GrainPool::obtain()` now refuses on exhaustion instead of stealing (the steal cut a live Hann envelope to zero). Wet path split into output/loop buffers so `gainRandom` sits after the feedback tap. Per-instance RNG seed. Ring 5.5→6.0 s. Editor 940×484 → **940×743** with a second panel row (RANDOM | WINDOW | MOTION | SPACE), sized once for the ~26 controls planned through v1.6. Harness 49→63 probes; new `ui_tooltip_clamp_check.js` measures tooltips in a browser at the real shipping size. **v1.0.1's 49 probe results reproduce byte-for-byte**, verified against a rebuild of commit `78af47b`.

- **2026-07-24 (v1.2.0):** Minor — grain window control (review B1): `grainShape` (Hann / Tukey / Gaussian / Triangular / Expo-Decay) and `grainTilt` (0–1, peak position within the grain), both latched per grain. Tilt is a two-segment linear phase warp that is the **bitwise identity** at its 0.5 default and power-invariant for symmetric windows by construction. Two normalisation constants, not one: POWER on the output path (decorrelated grains, all 5 shapes within 0.147 dB) and AMPLITUDE on the feedback tap (self-similar recirculating material — power-only left the decay rate spanning 4.40 dB/s at feedback 100; with the loop trim, 0.175). Fills the reserved WINDOW panel — **markup only, no resize**. Harness 63→81 probes. **v1.1.0's 63 probe results reproduce byte-for-byte**, verified against a rebuild of commit `8fa3646`.

- **2026-07-25 (v1.3.0):** Minor — grain count control (review B2): `grainCount`, an explicit **overlap ceiling** (2–16, default 8), so `overlap = 2 + density·(ceiling − 2)` replaces the hard-coded `2 + d·6`. Default 8 makes the expression bitwise v1.0.1's, which is what keeps existing sessions intact — density is stored *denormalised*, so widening the density knob's own span instead would have made every saved session ~2.3× denser with no migration available. Spawn cap 32→**128** with dropped requests and pool refusals both **counted and exposed** (the 32 was silent, and its "unreachable" justification was an argument, not a measurement). Reserved MOTION panel filled and relabelled **COUNT**, carrying the Count knob plus a live **Active / Overlap** readout polled at 15 Hz — which reverses Stage 3's decision D10 ("no C++→JS polling bridge"), deliberately, since `GrainPool::countActive()` had existed since Stage 2 and was called by nothing. Harness 81→**93 probes**. **v1.2.0's 80 shared probe results reproduce byte-for-byte**, verified against a rebuild of commit `bf5becb`.

  The gain work landed on the **opposite path from the one predicted**. The review expected `1/sqrt(overlap)` to under-correct on the output as overlap rose (partially coherent summing); measured across 2→16, it is flat to **0.07 dB** and needs nothing — at a fixed output sample the summed grains read source points multiples of `2·interval` apart, which is decorrelated for broadband input. The real defect was in the **feedback loop**, where the recirculating material genuinely is self-similar: at ceiling ≥ 10 with feedback 100 the loop crossed into self-oscillation (−0.29 → **+0.87 dB/s**) and a 90 s render **peaked at 1.28, i.e. clipped**. Fixed by `loopCountTrim` — `(N/8)^−0.5`, the fully-coherent amplitude law, anchored at the legacy ceiling and exactly `1.0f` at or below it — which brings the decay spread across all ceilings to **0.020 dB/s** and the worst-case peak to 0.28. Riding on the output/loop gain split v1.2.0 built for `gainRandom`.

## Known Issues

- The harness's shared excitation generator `randNoiseAt` is **not white** —
  measured autocorrelation reaches ±0.077 at lags 600–2400 samples, which is
  exactly where the grain spawn interval sits. Harmless for every probe that
  compares two renders (colouration cancels) but not for any probe comparing
  LEVELS while varying the spawn interval: measured that way the v1.3.0 overlap
  ladder showed a 2.5 dB non-monotonic spread that was entirely the test signal.
  `whiteNoiseAt` (murmur3 finaliser, max|acf| 0.0024) exists for those probes.
  Both are kept, so pre-v1.3.0 probe numbers stay diffable.
- `width` is also a feedback **decay** control, not only a pan control: it scales
  the per-grain pan gains, and those feed the loop tap. At width 0 grains are
  centred so the mono sum on read-back carries 0.7071; at width 100 the
  alternating hard pan makes it 0.5 — 3 dB less loop gain per generation,
  compounding. Pre-existing since v1.0.0, and the reason harness probe
  `ceiling16-loop-bounded` runs at width 0: that is the worst case for the loop,
  which is the opposite of "everything at maximum".

- A v1.0.0 user preset restored from a backup **after** the
  `.user-migration-version` sentinel is stamped will not be migrated, and its
  `delayTime` will recall high (a 1400 ms save reads back as ~2450 ms). Delete
  `~/Library/O-ReverseDelay/Presets/.user-migration-version` and reopen the
  plugin to re-run the migration.
- `width` has a hole in the middle by design: `kPanBias = 0.5` means at
  width 100 % no grain is ever centred (decision D5, not a bug).
- Expo-Decay decays faster in the feedback loop than the other four shapes
  (1.85 dB/s at feedback 60, 0.18 at feedback 100). Not a normalisation error:
  its crest factor is 3.10 against Hann's 1.63, so its peaks hit the loop's
  `tanh` harder. No linear constant removes it; measured and bounded by harness
  probe `decay-shape-fb60`.

## Key Architecture Points

- Reverse read law: grain read offset from write head grows +2 samples/output sample (`D + 2n`); collision-free by construction; 3.5 s capture buffer
- Feedback closes through the shared capture buffer → alternating-direction regenerations (intended, classic hardware character)
- Damping: 2nd-order Butterworth HP/LP via ArrayCoefficients in-place updates (RT-safe)
- Click-free via per-grain parameter latching (D/G/pan/gain snapshotted at spawn)
- Zero reported latency; estimated < 5% single core

## Planning Files

- `plugins/O-ReverseDelay/.planning/research/ARCHITECTURE.md`
- `plugins/O-ReverseDelay/.planning/ROADMAP.md`
- `plugins/O-ReverseDelay/.planning/REQUIREMENTS.md`
- UI mockup: not yet created (required before Stage 3)

**Last Updated:** 2026-07-25
