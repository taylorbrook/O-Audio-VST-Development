# O-ReverseDelay Notes

## Status
- **Current Status:** 📦 Installed
- **Version:** 1.7.0
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

- **2026-07-25 (v1.6.0):** Minor — the **MOTION** panel (review B4 #1–#3): `freeze` (bool, off), `direction` (0–100 %, 0 = all-reverse) and `regenMakeup` (0–6 dB, 0 dB). All three no-ops are plain zero, which makes this the first release since v1.1.0 where the reflex "default the new control to 0" is *correct* — v1.2.0 (tilt 0.5), v1.3.0 (count 8) and v1.4.0 (taper 0.5) each trapped it. Fills the last reserved panel (SPACE → MOTION), **markup + one width alias + two scoped rules, no resize** — so the 940 × 743 frame and the tooltip clamp geometry are the ones already verified. The chassis framed at v1.1.0 is now full; the next new control is genuinely the row-3 / MORE-page decision from the review's section D. First `WebToggleButtonRelay` in the plugin (`freeze` is its only `AudioParameterBool`). Harness 108→**122 probes**, all passing; `ui_frontend_check.js` 129 checks; auval SUCCEEDED, pluginval-10 ×3.

  **Freeze took three implementations.** Stopping the write head makes every grain spawned during the hold latch the *same* `readAbs` — the output goes strictly periodic at the spawn interval, a ~28 Hz buzz. Advancing the head without writing fixes that and then falls **silent**, because the read sweeps over the whole 13 s ring including however much was never written; probe AP measured rms at 10 s / 30 s / 60 s all `0.000000` on a freeze 3 s after load. What ships keeps writing and writes a **copy of the ring `freezeLoopSamples` back**, latched on the rising edge to how much has *actually* been captured. Unity-gain copy, so a hold cannot grow or decay: 60 s frozen renders at **0.04 dB** of drift. The ~20 ms transition crossfade blends against the looped material rather than ramping the input to zero (which would *erase* the ring), so it doubles as the loop's seam.

  **Forward grains sum coherently, and that is why this is a reverse delay.** At output time `t` a grain reads `2s − gD − t` reversed but `t − gD` forward — independent of its spawn sample — so every forward grain in flight reads the *same* source sample and the forward set adds in amplitude (`N·m`) where the reverse set adds in power (`√(N·q)`): **+7.3 dB** uncorrected at overlap 8. The s-dependence that decorrelates grains comes from the read head moving opposite to the write head; a unit-rate forward read is a plain delay tap by construction. `WindowLut::getForwardNorm` cancels it on the **output** only (`√(q_eff/N)/m_eff`, derived); the feedback tap needs nothing because `getLoopNorm` already models the loop as a coherent sum. Measured spread across the knob: **0.95 dB**. The residual ~0.9 dB mid-travel sag is derived, not overlooked — mixing a coherent set with an incoherent one gives `q·[p² + p(1−p)q/(Nm²) + (1−p)]`.

  **The collision question answered the other way.** A forward read head moves *toward* the write head, so the review flagged it as possibly needing its own clamp. It needs none: at pass-relative `k` a forward grain reads `passStartAbs − gD + k`, and v1.0.1's A2 pass bound already gives `k < passLen ≤ grainDelayFloor ≤ gD`. Its ring span is `gD + G` — **smaller** than reverse's `gD + 2·G`. Probe AM proves it in audio: correlation with the input delayed by exactly D is **1.0000** forward, 0.0038 reverse.

  **The regen ceiling is read off a ladder, not chosen.** At feedback 100 / width 0: sustain arrives at **2 dB** (peak 0.445), and past ~6 dB the control stops doing anything because the tanh is already limiting (12 dB buys 0.026 dB/s over 6 dB). Hence 6 dB. What the cap does **not** give is a bounded output: the tanh bounds the *loop* to ±1, but the wet path sums `overlap` grains of self-similar limited content and approaches `√overlap·mean·windowNorm` — 1.41 (Hann, overlap 8) to 1.55 (Tukey, overlap 16). Same shortfall as v1.3.0's 1.28 peak, which `loopCountTrim` fixed by *preventing* self-oscillation rather than bounding the sum. A cap holding peak < 1.0 everywhere would be ~1 dB and reach sustain nowhere. So **peak < 1.0 is an invariant of the 0 dB engine** — the default and every factory preset — and probe AO requires finite + convergent + under a hard 1.8 above it. Direction likewise costs up to 1.09 dB/s of decay (at direction 100 the loop *is* a plain feedback delay); every rate stays negative.

  "Near-Infinite" is deliberately **not** re-authored to self-sustain, even though that is the review's stated motivation — it is a shipped sound, and true sustain is now one knob away.

  Two stale test fixtures fixed, both of which had been passing while lying. `ui_tooltip_clamp_check.js` asserted a hardcoded `boundReadouts === 15` and had failed for the same non-reason three releases running; it now parses `KNOB_IDS`. And the new bool-binding check in `ui_frontend_check.js` flagged app.js's own header comment, which explains the trap in prose containing `getSliderState("freeze")` — comments are stripped before the binding regexes run, the same fix the `.group-motion` selector check already carried for CSS.

- **2026-07-25 (v1.7.0):** Minor — the last three parameters of review B4: `duck` (#4, 0–100 %, default 0), `sourceMode` (#5, Mono Sum / Stereo, default Mono Sum) and `driftRate`/`driftDepth` (#6, 0.02–5 Hz / 0–100 %, depth default 0). All four no-ops are the range minimum or index 0, so a v1.0–v1.6 session, preset or factory patch is bit-identical. Editor **940 × 743 → 940 × 972**: the reserve v1.1.0 framed is spent, so this is the row-3 decision from the review's section D, paid at the same 190 | 190 | 276 | 190 contract with one panel (COLOUR) framed and empty for the remaining loop-character items (#7 diffusion, #8 loop drive). Ring 13.0 → 14.0 s. Harness 122→**138 probes**; `ui_frontend_check.js` 129→**145 checks**; `ui_tooltip_clamp_check.js` re-measured at 940 × 972 (27/27 anchors, clamp firing for 5); auval SUCCEEDED, pluginval-10 ×3 both formats.

  **The duck follower runs per SAMPLE, and the brief specified per block.** A block-rate envelope (block RMS, `exp(−N/(τ·fs))`, gain ramped across the block) is cheaper and is what most ambient delays do — and it breaks the bit-identical 512-vs-4096 invariant that probes O, W2 and AQ assert and that three earlier releases spent real effort earning. Not subtly, either: at 4096 the attack resolves to **85 ms against 10.7 ms** at 512, so an offline bounce would duck audibly later than the same session monitored. What ships is a one-pole on `|dry|` advanced per sample with coefficients computed once per block, running inside the existing mix loop (the last place the dry input is still readable, so no second pass and no copy). There is no block RMS and therefore no `numSamples <= 0` division to guard — the guard is absent because the quantity is. Probe M holds `duck` to the **smooth** click tier, which is the assertion: a per-block gain step fails that line and passes every other probe in the suite.


  **The duck follower was the first state here that could not heal.** `duckEnv` is persistent, and `rect + c·(duckEnv − rect)` reproduces a NaN for any finite `rect` — so one bad input sample poisoned the duck gain for the life of the instance (an infinity gets there by a second route: `inf/(inf + knee)` is NaN, not 1). Nothing else in the engine behaves that way: the ring ages a bad sample out after a lap, the feedback loop's `isfinite` guard resets the filters within the pass. Measured before the fix, 10 ms NaN/inf burst at 1 s, tail [20 s, 30 s] after the 14 s ring has lapped — **duck 0: finite, rms 0.0638; duck 80: NaN, permanently**. auval, pluginval-10 ×3 on both formats and all 137 other probes passed in that state. Fixed with an `isfinite` reset to 0 (un-ducked — garbage must not attenuate the wet); one bit test per sample in a loop that already runs `cos` and `sin`, cannot fire for finite input, so the duck-0 identity and probe AX's invariance are untouched. Probe AY measures duck 80 **against duck 0**, since the shipped path's recovery is the standard.
  **A correlated input does not imply a correlated capture ring.** Probe AT's bit-identity assertion (`0.5·(L+R)` is exactly `L` when `L == R`, so the two source modes coincide sample-for-sample) failed by 0.0154 at feedback 40 / width 60 — and the engine was right. The ring is written with `input + feedback return`, and the feedback return is the **width-panned wet**, so the moment anything recirculates at width > 0 a correlated *input* stops implying a correlated *ring*. Runs at feedback 0 now (width stays 60, so the pan path is still exercised) and measures exactly 0.

  **The mono fold's 0.7071 is unchanged, and the first probe of it was 3 dB wrong.** Mono output is only reachable with mono input — the bus layout rejects stereo→mono — and a mono input leaves `L == R` in the ring, so the two source modes read identical material *by construction*: the constant cannot depend on the mode because the mode cannot reach it. The value was harder to test than to derive. Comparing mono-out RMS against the stereo render's **L channel** reported +3.010 dB, which reads exactly like a broken constant and is not: at width 0 each channel carries `1/√2` of the grain sum. The right reference is **total power** `√(rmsL² + rmsR²)`, precisely because it is fold-*independent* — the same number whatever constant the branch uses, so the comparison tests the constant instead of restating it. Measured **−0.000 dB**.

  **Width 0 has never been bitwise dual-mono**, and that belongs to v1.0.0. The gains are `cos(π/4)` and `sin(π/4)` — mathematically equal, one ulp apart as the library computes them — which is why probe K has always used a tolerance. Measured 1.1e-8 against a 0.107 peak (−136 dB).

  **Drift's LFO phase comes from the absolute spawn position, not an accumulator**, which makes it block-size invariant exactly rather than nearly. The cost is that moving the rate knob re-derives the phase instead of continuing it — at worst one grain landing at a different delay, inaudible against Scatter, which does the same thing deliberately. `srcCh` follows **panSign** and not the resolved pan position for the mirror-image reason: at width 0 every pan is exactly 0.5, so a position test would send every grain to the same channel and Stereo would silently become "mono, but only the left input".

  Two verification gaps closed. `ui_frontend_check.js` gained a **choice closure** (APVTS choices ↔ `kComboIds` ↔ `getComboBoxState`, both directions) — the gap v1.6.0's bool closure left open, since choice ids were excluded from the knob closure by a hand-written list and then checked against nothing; `sourceMode` is exactly that case. And v1.6.0's three MOTION tooltip anchors were never added to the clamp check's inventory, which is the hand-maintained-fixture drift `pattern_test_fixture_mirrors_drift_silently` describes.

## Known Issues

- **The WINDOW panel has ~1 px of vertical slack.** Its budget is 212 of 213 px
  (select-cell 44 + knob-cell 78 + env-cell 72 + two 9 px row gaps). Adding
  anything to that panel, or growing any of its parts, means re-doing that sum —
  the numbers are in the CSS comment beside the `.group-window` overrides. Those
  overrides are deliberately SCOPED: `.knob`, `.knob-cell`, `.select-cell` and
  `.division-select` are shared by all eight panels.
- **The UI chassis is FULL as of v1.6.0.** Row 2 is
  RANDOM | WINDOW | COUNT | MOTION and nothing is reserved any more. The next new
  control forces a third row or a MORE page (v1.0.0 review, section D). A resize
  invalidates the tooltip edge-clamp verification, which is viewport-sensitive
  and must be re-measured at the real shipping width, not a default browser one.
- **`regenMakeup` above ~2 dB can put the wet output over 1.0**, and this is
  documented rather than fixed. The tanh bounds the *loop* to ±1 at every
  setting; it does not bound the output, which sums `overlap` grains of
  self-similar limited content and approaches `√overlap·mean·windowNorm` —
  measured 0.99 (Hann, overlap 8) to 1.55 (Tukey, overlap 16) at the 6 dB
  ceiling. Same mechanism as v1.3.0's 1.28 peak, which `loopCountTrim` fixed by
  *preventing* self-oscillation rather than by bounding the sum. A cap holding
  peak < 1.0 everywhere would be ~1 dB and would reach sustain nowhere, so
  **peak < 1.0 is an invariant of the 0 dB engine** — the default, every factory
  preset and every pre-v1.6.0 session. Probe AO still requires finite, convergent
  and under a hard 1.8 above it.
- **`direction` shortens the tail**, by up to 1.09 dB/s at feedback 100. Not a
  missing trim: at direction 100 the loop *is* a plain feedback delay
  (N grains reading one sample), which is a different feedback structure from a
  reverse smear and cannot have the same decay rate. Every rate stays negative.
  The 75 % dip below both endpoints is the same mutual-decorrelation effect as
  the output-level sag — a mix of the two sets sums to `√(fwd² + rev²)`.
- **`direction` at 100 % with `delayScatter` at 0 is a clean delay, not a
  cloud.** Forward grains all read the same source sample, so they cannot
  decorrelate among themselves at unit read rate — that is exactly why the plugin
  is a *reverse* delay. Scatter is the companion control: a scattered grain
  latches a different `gD` and therefore reads a different point. Noted in the
  Direction tooltip.
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
