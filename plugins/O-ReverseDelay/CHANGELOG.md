# Changelog — O-ReverseDelay

All notable changes to the O-ReverseDelay granular reverse delay.
Format loosely follows [Keep a Changelog]. **v1.0.0 is the first shipped product
version** — there is no earlier release track.

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
