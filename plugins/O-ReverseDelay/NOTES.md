# O-ReverseDelay Notes

## Status
- **Current Status:** 📦 Installed
- **Version:** 1.2.0
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

## Known Issues

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

**Last Updated:** 2026-07-23
