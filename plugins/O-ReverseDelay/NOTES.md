# O-ReverseDelay Notes

## Status
- **Current Status:** 📦 Installed
- **Version:** 1.5.0
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

- **2026-07-25 (v1.4.0):** Minor — **continuous Tukey taper + a window-shape display**. `tukeyTaper` unfreezes the α that v1.2.0 hard-coded at 0.5, over **[0.01, 1.00] step 0.01**: 0.01 is near-rectangular (fast edge, "open"), and 1.00 *is* Hann, reached exactly. Rendered with **no new table and no new transcendental** — Tukey's taper is literally a Hann half, so `Tukey(φ) = Hann(min(φ,1−φ)/taperEnd, clamped)` is one phase remap into the existing Hann table. The step is chosen so every reachable α lands exactly on the 100-entry stats grid WindowLut precomputes, which is what keeps α = 0.5's normalisation constants bitwise. New **ENVELOPE** panel (was reserved SPACE) draws the live envelope — shape, tilt and taper composed — with the curve fetched from C++ via `getWindowCurve` rather than recomputed in JS, so there is exactly one definition of the window. Harness 93→**106 probes**; **92 of 93 shared probe results byte-for-byte identical** against a rebuild of `8a52c33`.

  The display lives **inside the WINDOW panel**, under Shape/Tilt/Taper, so the panel reads cause-then-effect; fitting it shrank that panel's own knobs (56→46 px), select padding and gaps, all via `.group-window`-scoped rules that touch no other panel. Measured budget: 212 of 213 px. SPACE therefore stays reserved and row 2 keeps a free slot.

  Two things worth knowing. First, the requested range was 0.01–9.9; Tukey's taper is mathematically bounded to [0, 1] (1.0 = full taper = Hann), so 1.0–9.9 would have been ~90 % dead knob travel. Corrected to [0.01, 1.00] after checking with the user. Second, the remap is **not bitwise** for Tukey: it deviates from v1.3.0's stored table by up to **2.4e-6 (−112.5 dB)**, which is LUT lerp error, not a shape change, and cannot be avoided — reading a 2048-point table at an arbitrary phase is not the same operation as evaluating `cos` there. Confined to Tukey; the cross-version diff confirms it, with `window-live-Tukey` the single probe line that moved (0.075537 → 0.075538) and every other shape untouched. All eight factory presets are on Hann.

  α needed **two** different normalisation corrections, for the third release running: power duty 0.994→0.375 (4.2 dB) on the output, amplitude duty 0.995→0.500 (6.0 dB) on the feedback tap. Level ends up flat to **0.010 dB** across the whole range, and decay flat to **0.030 dB/s** (fb 60) / **0.056** (fb 100) for α ≥ 0.1. **α = 0.01 is a documented exception**: 0.240 dB/s at fb 60 and 0.791 at fb 100, because a near-rectangular window has crest factor ~1.0 against Hann's 1.63 and overlaps to something near-constant, neither of which a linear duty constant equalises — the same class of exception the header already records for Expo-Decay. Its grain edge measures 0.0112 against 0.0058 at α = 0.5, i.e. twice as fast but nowhere near a click.

- **2026-07-25 (v1.5.0):** Minor — **`grainSize` max 500 → 4000 ms**, on `delayTime`'s exact range and taper (skew centre 316 ms), so the two long-throw time knobs read alike. Default stays 200 ms and a new instance is unchanged. Measured off the rendered UI: 25 % = 68 ms, 50 % = 316 ms, 75 % = 1339 ms — steep at the bottom, which is the price of 4 s on one knob and the price `delayTime` has always paid. Harness 106→**108 probes**, all passing; auval SUCCEEDED, pluginval-10 ×3 both formats, **user DAW sign-off**.

  The load-bearing half was the **capture ring, 6.0 → 13.0 s**. The requirement is `gD_max + 2·G_max` — a grain's last read lands at `(s − gD − G)` while the write head has reached `(s + G)` — so `4.5 + 2·4.0 = 12.5 s`. Shipping the wider range against the old ring would **not have faulted**: long grains would have wrapped onto overwritten material with no NaN, no discontinuity, and every existing probe green — audible only as "the long settings sound crunchy". Cost ~5.0 MB stereo at 48 kHz (~20 MB at 192 kHz), allocated once in `prepareToPlay`. The invariant is now a **`static_assert`**, because every prose statement of it was already correct at v1.4.0 and none of them stopped this release from invalidating it.

  User presets are migrated; sessions are not (APVTS stores denormalised ms — rescaling those would corrupt them). The trap was the **version gate**: `delayTime`'s range moved at v1.0.1 so its gate is `< 1.0.1`, but `grainSize`'s moved at v1.5.0 so its gate is `< 1.5.0`. Reusing the existing `!= "1.0.0"` test would have migrated v1.0.0 presets and silently left every v1.1–v1.4 preset — the bulk of any real library — on the old curve, still loading fine and recalling the wrong size. `grainSize` is also the harder rescale: `delayTime` kept its skew centre and moved only its max, while `grainSize` moved **both**, so no scale factor works — only reconstructing the old range and round-tripping through ms. The gate is per-file, so migration is idempotent if a pass is interrupted before the sentinel writes.

  Two stale mirrors found and fixed, both of which would have kept passing while lying. `tests/render-harness/CMakeLists.txt` pinned `JucePlugin_VersionString="1.2.0"` while the plugin shipped 1.3.0 and 1.4.0 — the file's own comment warns this value is load-bearing (both preset sentinels key off it), so probes N and R spent two releases auditing v1.2.0's on-disk presets. And `tests/ui-stub/juce-stub.js` still declared `grainSize` as 50–500 centre-158, which would have made every browser-rendered readout disagree with the plugin. The new stub assertion checks against the **C++ constants** rather than repeating literals, since a literal in the test drifts exactly as silently as the one it is guarding.

## Known Issues

- **The WINDOW panel has ~1 px of vertical slack.** Its budget is 212 of 213 px
  (select-cell 44 + knob-cell 78 + env-cell 72 + two 9 px row gaps). Adding
  anything to that panel, or growing any of its parts, means re-doing that sum —
  the numbers are in the CSS comment beside the `.group-window` overrides. Those
  overrides are deliberately SCOPED: `.knob`, `.knob-cell`, `.select-cell` and
  `.division-select` are shared by all eight panels.
- **Row 2 still has one reserved panel (SPACE).** The next new control after that
  forces a third row or a MORE page (v1.0.0 review, section D), and a resize
  invalidates the tooltip edge-clamp verification, which is viewport-sensitive.
- `tukeyTaper` is **inert unless `grainShape` is Tukey**. The knob dims and sets
  `aria-disabled`, but stays relay-bound and adjustable, so a value set before
  switching to Tukey is honoured. Deliberate: hiding it would make the WINDOW
  panel jump as Shape changes.

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
