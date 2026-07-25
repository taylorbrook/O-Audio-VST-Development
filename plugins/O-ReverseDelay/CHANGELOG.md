# Changelog — O-ReverseDelay

All notable changes to the O-ReverseDelay granular reverse delay.
Format loosely follows [Keep a Changelog]. **v1.0.0 is the first shipped product
version** — there is no earlier release track.

## [1.1.0] — 2026-07-24 — Grain randomisation + UI chassis

Minor release implementing **section B3** of the v1.0.0 review
(`improvements/2026-07-24-v1.1-review.md`): the four grain randomisations that
close the gap between "many reverse delays" and "a granular cloud". Also
expands the editor chassis **once**, sized for the controls planned through
v1.6, so later releases drop into space that already exists.

Builds on v1.0.1's grown capture ring, as the review required.

### The compatibility guarantee, and how it was verified

All four new parameters default to **0**, which is the exact no-op in the
engine — not a small value, a genuine no-op. Every randomisation is gated on
`amount > 0` and draws **nothing** from the RNG when off, so the pan sequence
is untouched and existing work renders identically.

This is measured, not asserted. The v1.0.1 harness was rebuilt from commit
`78af47b` and run head-to-head with v1.1.0:

| Check | Result |
|-------|--------|
| v1.0.1's 49 probe result lines vs v1.1.0's | **byte-for-byte identical** |
| Offline render harness | **63/63 probes PASS, exit 0** (49 + 14 new) |
| `ui_frontend_check.js` | **ALL CHECKS PASSED** (sections 1–15) |
| `ui_tooltip_clamp_check.js` @ 940×743 | **ALL CHECKS PASSED**, 14/14 anchors |
| `pluginval --strictness-level 10` VST3 | **SUCCESS ×3**, zero failures |
| `pluginval --strictness-level 10` AU | **SUCCESS ×3**, zero failures |
| `auval -v aufx ORvD OuDv` | **AU VALIDATION SUCCEEDED** |
| AU component version | **65792** (= 1.1.0) |

Because nothing is renamed, removed, re-ranged or re-typed, and no existing
session or preset changes value or sound, this is MINOR rather than MAJOR.

### Added

- **`jitter` (0–100 %, default 0)** — randomises the grain **spawn interval**,
  `interval · (1 ± 0.9·jitter·u)`. Through v1.0.1 the scheduler was a strictly
  periodic countdown, and a fixed interval against a fixed grain length is a
  comb — the reason sustained material read as metallic rather than as a cloud.
  The deviation is symmetric, so the *mean* interval, and with it the average
  overlap and the feedback loop's duty cycle, are exactly unchanged. Capped at
  ±90 % rather than ±100 % so the low tail cannot approach a zero-length
  interval (i.e. a spawn every sample).
- **`delayScatter` (0–500 ms, default 0)** — randomises each grain's latched
  delay by ±this. Thickens the smear without moving the rhythmic anchor,
  because the mean delay is unchanged. This is the parameter that required
  v1.0.1 first: it can push a grain's latched delay 500 ms *past* the delayTime
  maximum, so the worst-case read span became 4.5 + 2·0.5 = **5.5 s** — which
  v1.0.1's 5.5 s ring met by a single sample. The ring is now **6.0 s**
  (+192 KB stereo at 48 kHz) for a real margin.
- **`sizeRandom` (0–100 %, default 0)** — randomises each grain's latched
  length, clamped back into `grainSize`'s own range. Jitter alone leaves a
  residual periodicity because every grain still shares one envelope length;
  this removes it. Clamping to the parameter's own endpoints means a randomised
  grain is never longer than one the user could dial in by hand, which is what
  keeps the ring bound above true.
- **`gainRandom` (0–100 %, default 0)** — randomises per-grain gain for depth
  and shimmer, applied **after** the feedback tap (see below). Power-normalised
  by `1/sqrt(1 + dev²/3)`, so it changes spread and not level.
- **RANDOM panel** holding the four new knobs, with tooltip copy, dblclick
  reset and keyboard/wheel adjustment on the same footing as every other knob.
- **`getActiveGrainCount()`** — exposes the live concurrent-grain count.
  `GrainPool::countActive()` had existed since Stage 2 and was called by
  nothing; the harness now reports peak concurrency as a measured number.

### Changed

- **`GrainPool::obtain()` refuses the spawn when no slot is free, instead of
  stealing the oldest grain.** v1.0 overwrote the oldest slot in place, which
  cut a live Hann envelope from mid-window to zero in one sample — a click, not
  a crossfade. It was unreachable in v1.0 steady state (max overlap 8 against
  32 slots), but all four randomisations raise the transient concurrent-grain
  peak, so it had to be safe *before* they landed. Refusing costs one
  contributor out of a wash of 8–32 and is inaudible.
- **The wet path is now accumulated twice** — once with per-grain random gain
  (the output) and once without (the feedback tap). This is what keeps
  `gainRandom` downstream of the loop: a randomised gain inside a recirculating
  path compounds every generation, so the knob would control *how long the tail
  lasts* rather than how it shimmers, and at feedback = 100 would make the
  decay rate itself stochastic. Costs two extra mul-adds per grain-sample.
- **`rngState` seeds from a per-instance hash** rather than the shared literal
  `0x12345678`. v1.0 gave every instance the same seed, so two instances on two
  tracks produced identical pan sequences — and would have produced identical
  grain randomisation too, correlating exactly where a wide cloud is wanted.
  The seed is fixed for the *lifetime of the instance*, not re-rolled per
  `prepareToPlay`, so one instance still reproduces across prepare/reset cycles.
  Under `OUARICON_RENDER_HARNESS=1` it collapses back to v1.0's literal.
- **Editor 940 × 484 → 940 × 743.** A second panel row (RANDOM | WINDOW |
  MOTION | SPACE) sharing row 1's pinned width contract (190 | 190 | 276 |
  190), so the two rows align column-for-column. `215 + 14 + 245 = 474`
  consumes the height increase exactly — row 1 and the footer do not move.
  Capacity is ~27 knob-cell slots against the ~26 controls planned through
  v1.6. WINDOW / MOTION / SPACE are framed and labelled but empty, carrying a
  dimmed fleuron; filling one in a later release is an HTML change with no
  resize and no re-verification.
- `.botanical-overlay` height pinned at 340 px instead of `70%`. Under a
  percentage the resize scaled the plate to ~520 px and it began reading as
  clutter behind two rows of translucent panels.
- `kDelayTimeMinMs` named, replacing the `50.0f` literals in the parameter
  range and the tempo-sync clamp — the same single-definition discipline A1
  established for the maximum.

### Fixed

- **The ui-stub's `delayTime` range was stale at 50–2000 ms**, missed when
  v1.0.1 widened it to 50–4000. Any browser render of the page — which is the
  gate for the failure classes C++ builds cannot see — was showing a readout
  that disagreed with the plugin.

### Verification added

Fourteen new render-harness probes (T–Y) and two new frontend sections:

- **T `random-live` / `random-zero-determinism`** — each randomisation
  measurably changes the render (no dead controls), and two independent
  all-zero renders are bit-identical.
- **U `level-flat`** — wet RMS within ±1 dB across {0, 50, 100 %} for all four,
  the same budget probe D holds density to. Catches a character knob that is
  really a loudness knob.
- **V `jitter-breaks-grid`** — at density 0 a regular spawn grid overlap-adds
  perfectly flat (probe Q), so flatness reads grid regularity directly:
  1.0000 at jitter 0, 0.0339 at jitter 100. Asserts *both* ends, so a dead
  jitter fails rather than passing quietly.
- **W `scatter-ring-worst-case` / `scatter-blocksize-invariance`** — the 5.5 s
  read span against the 6.0 s ring, and 512-vs-4096 bit equality with all four
  randomisations on.
- **X `gainrandom-loop-neutral`** — loop decay at feedback 100 with gainRandom
  0 vs 100: −2.493 vs −2.527 dB/s, delta 0.034. The single assertion behind
  "applied after the feedback tap".
- **Y `pool-pressure-clickfree`** — grainSize swept under maximum
  randomisation; peak concurrency reported (14/32), click-freedom asserted.
- **`ui_frontend_check.js` §15** — four-way knob closure across
  `createParameterLayout` / `kSliderIds` / `KNOB_IDS` / the `knob-*` and
  `val-*` elements, plus a FORMAT entry and a ui-stub range for each, plus the
  four defaults pinned at 0. A knob wired in three of the four places is a
  silently dead control.
- **`ui_tooltip_clamp_check.js`** (new file) — drives the real page in a
  browser at the real 940 × 743 and measures every tooltip rectangle. The
  static check can prove the clamp *code* is correct but not that it *fires*,
  because that depends entirely on viewport width
  (`pattern_tooltip_clamp_gate_viewport_sensitive`). It asserts both edges, not
  just width, and fails if the clamp never engages at all.

### Two block-size bugs caught during this work

Both were found by the new probes, and both would have shipped silently:

1. **Jitter draws batched per pass.** The scheduler consumes its RNG inside a
   per-sample countdown while the spawn handler consumes after the whole pass
   is scheduled. Sharing one stream interleaved them differently at 512 than at
   4096 samples, so an offline bounce would not match what was monitored. Fixed
   by splitting into two streams, each consumed a fixed number of times per
   spawn — making consumption a function of spawn *index*, which is block-size
   invariant.
2. **The scatter clamp was derived from `passLen`.** A2 bounds each engine pass
   to `D`; negative scatter can put a grain's latched delay below that, and the
   obvious repair — clamp the latched delay up to `passLen` — makes the latched
   value itself depend on the host block size. Fixed by keying both the pass
   bound and the clamp off `grainDelayFloor`, a function of the parameters
   alone.

Per-grain randomisation values are also now drawn *before* the pool slot is
requested, so a refused spawn consumes exactly what a granted one does and RNG
consumption cannot depend on pool occupancy.

---

## [1.0.1] — 2026-07-24 — DSP correctness

Patch release fixing the three defects found in the v1.0.0 read-only review
(`improvements/2026-07-24-v1.1-review.md`, sections A and C). **No new
parameters, no UI change, no window resize.** All three were invisible to auval,
pluginval-10 and the shipped 41-probe harness, which is why they survived Stage 4
— the harness gained 8 probes that each fail on v1.0.0 and pass here.

Validated at release:

| Gate | Result |
|------|--------|
| Offline render harness | **49/49 probes PASS, exit 0** (41 shipped + 8 new) |
| Same harness vs v1.0.0 DSP | **12 FAIL** — the new probes are not vacuous |
| `pluginval --strictness-level 10` VST3 | **SUCCESS ×3**, zero failures |
| `pluginval --strictness-level 10` AU | **SUCCESS ×3**, zero failures |
| `auval -v aufx ORvD OuDv` | **AU VALIDATION SUCCEEDED** |
| `ui_frontend_check.js` | **ALL CHECKS PASSED, exit 0** |
| AU component version | **65537** (= 1.0.1) |

### Fixed

- **A1 — tempo sync silently clamped across this plugin's own tempo range.**
  `delayTime` maxed at 2000 ms and sync-derived times were clamped into it, so
  `1/1` collapsed below 120 BPM, `1/2D` below 90 and `1/2` below 60. In the
  70–100 BPM band the brief targets, the UI named a division the engine was not
  playing and two divisions landed on the same delay with no indication.
  **Root cause:** a literal `2000.0` in the sync clamp
  (`PluginProcessor.cpp:323`) duplicating the parameter's max instead of
  referencing it. `delayTime` max is now **4000 ms** (covers `1/1` at 60 BPM),
  the clamp reads the same `kDelayTimeMaxMs` constant, and the capture ring grows
  **3.5 s → 5.5 s** to cover `Dmax + 2·Gmax` = 4.0 + 1.0 s (≈ 2.1 MB stereo at
  48 kHz). The skew centre deliberately stays at **316 ms**, so short delay times
  keep their knob resolution.
- **A2 — grains read unwritten capture at large block sizes.** A grain spawned at
  block offset `i` latched `readAbs = blockStart + i − D` and rendered *before*
  the block's capture write, so reads were already-written only while `i < D`.
  With `D` bottoming out at 2205 samples (50 ms at 44.1 kHz), every 2048- or
  4096-sample buffer — routine in offline bounce and high-latency live rigs — had
  its late grains reading a full ring lap of stale audio, or silence early on.
  **Fix:** each engine pass is now bounded to `D` samples, so `i < D` holds by
  construction at any host block size. Chosen over clamping `D ≥ numSamples`
  (which would silently lengthen the delay at large buffers) and over
  write-input-first (which still drops the block's own feedback regeneration).
  The engine is now block-size **invariant**: a 4096-sample render is
  bit-identical to a 512-sample one (`max|Δ| = 0.000000000`, probe O). At the
  shipped 512-sample block with `D ≥ 2400`, the code path is a single pass and
  bit-identical to v1.0.0 — the fix costs nothing where it was already correct.
- **A3 — the bottom ~14 % of Density was a full-depth tremolo.** `overlap` mapped
  to `1 + density·7`, so at low density the hop equalled the grain length and
  Hann grains **abutted** — the wet output amplitude-modulated to true silence at
  every boundary (a 5 Hz, 100 %-depth gate at `grainSize = 200 ms`). Hann reaches
  constant-overlap-add at hop `G/2`, i.e. `overlap ≥ 2`. Remapped to
  **`overlap = 2 + density·6`**: same maximum (8), whole travel now a genuine
  smooth→dense sweep. Measured envelope min/max at `density = 0` goes
  **0.0000 → 1.0000** (probe Q).
- **C — `processBlock`'s oversized-block bail left the extra output channel
  unwritten.** The v1.0.0 bare `return` did already pass channel 0 dry through
  (the review's "bails to total silence" reading is wrong), but in a mono→stereo
  layout channel 1 is never written by this plugin and carried stale host memory.
  Dry is now explicitly duplicated to any unfilled output channel before bailing.
- **C — no `AudioProcessor::reset()` override.** Hosts calling `reset()` left the
  capture ring, grain pool, scheduler countdown and filter states populated, so a
  stale reverse tail survived a host-level reset. Now cleared, alloc-free, with
  the RNG re-seeded to the same fixed value `prepareToPlay` uses.

### Changed

- **Factory presets re-authored and re-seeded.** The A3 remap changes what a
  given `density` value means, so every preset's density is rewritten to
  `(7·d_old − 100)/6` — the value that reproduces its **shipped** overlap exactly
  (60→53.3, 55→47.5, 70→65, 30→18.3, 90→88.3, 65→59.2, 80→76.7). All eight
  presets therefore render as they did at v1.0.0; only the knob's scale moved.
  The `VERSION 1.0.0 → 1.0.1` bump is what invalidates the `.factory-version`
  sentinel and lets these edits actually reach
  `~/Library/O-ReverseDelay/Presets/Factory` — at a static version they would
  have been a silent no-op.
- **Feedback decay re-measured at `feedback = 100`** (probe S), since overlap
  sets both the spawn hop and `grainGain = 1/√overlap`, i.e. the loop's duty
  cycle. At the overlap-matched density (5.20) the decay is **−2.955 dB/s**
  against v1.0.0's **−2.958 dB/s** at the same overlap — unchanged to 0.003 dB/s.
  At the *same knob position* (density 60, overlap now 5.6) it is **−2.493 dB/s**,
  i.e. ~0.46 dB/s more sustain. No shipped preset moves, because all eight are
  overlap-matched.
- **Dead code removed:** `GrainScheduler::sampleRate` was stored in `prepare()`
  and never read. (`CaptureBuffer::readAbs()` is still uncalled and deliberately
  kept — it is the entry point for the planned stereo-source mode.)

### Migration Notes

The two persistence formats needed **opposite** treatment, and the review's
premise that both recall by normalised fraction is only half right:

- **Sessions need no migration.** APVTS stores each `PARAM`'s *denormalised*
  value — literal milliseconds — and JUCE restores it through
  `setDenormalisedValue()`, which re-normalises against whatever range is
  current. A v1.0.0 session saved at 1400 ms recalls 1400 ms under the 4000 ms
  range. Rescaling it would have **corrupted** it. Probe P asserts the round trip
  directly, and it passes against both the v1.0.0 and v1.0.1 DSP.
- **User presets do need migration.** `OuariconPresetManager::createPresetJson`
  stores `RangedAudioParameter::getValue()` — the normalised 0–1 fraction — so a
  v1.0.0 preset saved at 1400 ms would have read back as **2450.5 ms** under the
  wider range. `migrateUserPresets()` rewrites the `delayTime` fraction of every
  `"version": "1.0.0"` file in `Presets/User/` through the reconstructed v1.0.0
  range, then re-stamps the file. One-shot, guarded by a
  `.user-migration-version` sentinel mirroring the factory one (without it, every
  processor construction would re-read every preset on the message thread and
  concurrent constructions would race). **Known limit:** a v1.0.0 preset restored
  from a backup *after* the sentinel is stamped will not be migrated.

Automation, parameter IDs, ranges of the other nine parameters, and the state
format are all unchanged — this is not a breaking release.

## [1.0.0] — 2026-07-24 — first release

First shipped version: granular reverse-delay engine, Ouaricon Naturalist WebView
editor, 8 factory presets, preset bar and hover help.

**DSP is frozen as verified in Stage 2.** Stage 4 (Polish) shipped **zero** audio
changes — the D11 feedback-tap makeup constant was auditioned in Standalone and
**explicitly declined**: the wash decays as intended at `feedback = 100`, so the
topology's inherent ≈ −7.3 dB/generation pre-damping loss (−4.3 dB Hann² duty
+ −3.0 dB pan→mono-sum round trip) stands as the shipped character.

Validated at release:

| Gate | Result |
|------|--------|
| Offline render harness | **41/41 probes PASS, exit 0** (33 Stage-2 + 8 factory-preset audits) |
| `pluginval --strictness-level 10` VST3 | **SUCCESS ×3**, zero failures |
| `pluginval --strictness-level 10` AU | **SUCCESS ×3**, zero failures |
| `auval -v aufx ORvD OuDv` | **AU VALIDATION SUCCEEDED** |
| `ui_frontend_check.js` | **76/76 PASS, exit 0** |
| AU component version | **65536** (= 1.0.0) |

pluginval strictness 10 covers Editor, Open editor whilst processing, Automation,
Editor Automation, Plugin state, Plugin state restoration, Parameter thread
safety and Fuzz parameters.

Windows is **deferred to CI** — the CMake already carries `NEEDS_WEBVIEW2` and
`JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`, and the two `FileChooser`
completions hoist their `SafePointer` to a local rather than init-capturing it in
a nested lambda, which is what MSVC rejects.

### Added — Stage 4: Polish

- **Factory presets:** 8 presets seeded to
  `~/Library/O-ReverseDelay/Presets/Factory/` on first run — Reverse Bloom,
  Guitar Swell, Vocal Halo, Slow Wash, Tight Smear, Dark Cavern, Near-Infinite,
  Rhythmic Reverse. Authored in **engineering units** (ms / % / Hz / choice index)
  and converted skew-safe through each parameter's own `NormalisableRange` via
  `convertTo0to1`; `delayTime`, `grainSize`, `lowCut` and `highCut` are skewed, and
  a hand-written normalised fraction on any of them would recall 10–30× wrong.
  Harness probe N audits all eight through the **shipping** `loadPreset()` — the
  measured round-trip error is **0.0000 on every parameter of every preset**.
  *Near-Infinite* runs `feedback = 100` and renders 30 s in the harness as a
  preset-driven stability statement. *Rhythmic Reverse* is the one tempo-synced
  preset (1/8 dotted), audited against a 120 BPM playhead.
- **Preset bar:** window grows 940 × 440 → **940 × 484** for a 44 px band under
  the header carrying `◀ ▶ [ name ] Save Load Delete`. The band and the height
  increase are the same 44 px, so panel heights and the footer are untouched.
  Styling reuses the page's own `.segment` / `.division-select` vocabulary rather
  than importing a dark chrome strip that would give the page a second title bar.
  Delete uses a **two-click inline confirm**, never the browser confirm dialog,
  which is a silent no-op or a throw in some JUCE WebView backends.
- **OuariconPresetManager v1.0.5** integrated via CMake include (header-only, no
  vendored copy). Session state now routes through it, so the current preset name
  survives a save/reload; pre-Stage-4 APVTS sessions still load unchanged.
- **Tooltips** on all 10 controls, authored as `data-tip-title` / `data-tip` in
  `index.html`. Hover only — no toggle, no persisted state. The tip measures its
  width at `left: 0` and **pins it before placing**, so the right-most control
  (`mix`) gets a full 230 px tip instead of a shrink-wrapped ribbon.

### Added — Stage 3: GUI

- Ouaricon Naturalist WebView editor: four framed group panels in signal-flow
  order (TIME | GRAIN | FEEDBACK | OUTPUT), all 10 parameters bound two-way
  through `Web*Relay` / `Web*ParameterAttachment`.
- Sync/Free control swap on a shared fixed-size slot — both controls stay
  relay-bound at all times, so neither is ever a dead control.
- Readouts and knob angles come exclusively from `SliderState.getScaledValue()` /
  `getNormalisedValue()`; the C++ `NormalisableRange` is the only source of range
  and skew. Double-click resets to the engineering default fetched from C++.

### Added — Stage 2: DSP

- Reverse grain engine over a 3.5 s stereo capture ring (reverse read offset
  D + 2n), Hann-windowed grains from a 32-slot preallocated pool with per-grain
  parameter latching for click-free changes.
- Feedback loop through the shared capture buffer: wet → gain → high-pass →
  low-pass → `tanh` → non-finite guard. 2nd-order Butterworth damping filters
  updated in place with `ArrayCoefficients` (never `Coefficients::makeXXX` on the
  audio thread), cutoffs clamped to 0.49·fs.
- Tempo sync across a 13-entry note-division table with a no-BPM fallback; width
  spread via an RT-safe xorshift32 with alternating pan sign; custom equal-power
  dry/wet mix (zero latency).

### Added — Stage 1: Foundation

- JUCE 8 plugin shell, VST3 + AU + Standalone, `PLUGIN_CODE ORvD`.
- Bus layouts mono→mono, mono→stereo, stereo→stereo.
- APVTS with the 10-parameter contract: `delayTime`, `syncMode`, `noteDivision`,
  `grainSize`, `density`, `feedback`, `lowCut`, `highCut`, `width`, `mix`.

### Notes

- Preset library location is `~/Library/O-ReverseDelay/Presets/{Factory,User}/`
  (**not** `~/Library/Application Support/`). The name is hardcoded without the
  dev suffix, so dev and release builds share one library.
- Factory presets only re-seed when `JucePlugin_VersionString` changes. While the
  version is frozen at 1.0.0, editing the factory table is a silent no-op until
  `~/Library/O-ReverseDelay/Presets/Factory` is removed. (v1.0.1 bumps the
  version, which re-seeds them.)
