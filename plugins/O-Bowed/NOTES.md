# O-Bowed Notes

## Status
- **Current Status:** 📦 Installed
- **Version:** 1.9.4
- **Type:** Synth (Physical Model Bowed String)

## Lifecycle Timeline

- **2026-04-17 (v1.2.0):** Humanize section added.
- **2026-04-19 (v1.2.1):** Humanize panel layout fix.
- **2026-04-26 (v1.3.0):** VST3 Note Expression microtonal support for Dorico (shared `note-expression` module).
- **2026-07-08 (v1.4.0):** Resolved the Critical + Warning findings from the v1.3.0 deep code review
  (`CODE_REVIEW.md`) via `/improve-review`. MINOR bump — see CHANGELOG for the full per-finding
  breakdown. Highlights:
  - **CR-01/CR-02:** removed audio-thread heap allocation on bridge loss-filter and BodyResonator
    coefficient updates (in-place `ArrayCoefficients` / seeded-storage writes).
  - **CR-03:** registered the 8 missing tuning-panel native functions (library list, embedded-tuning
    load with period-append, generators, apply, SCL/KBM/HTML export); Tuning panel is now functional.
  - **CR-04:** bound `sympatheticDecay` / `bodyAmount` / `stringGauge` / `bowHairStiffness` to the UI.
  - **CR-05:** SafePointer guards on all 6 FileChooser `launchAsync` completions (teardown UAF).
  - **WR-01:** NaN/Inf guard on the waveguide excitation (source reset).
  - **WR-02:** implemented the Core↔bristle friction blend so `bowHairStiffness` is audible;
    **default changed 0.5 → 0.0** to preserve the shipped timbre (backward-compatible).
  - **WR-03/04/05/06/07:** voice denormal guard; per-sample bow-position/brightness smoothing;
    sympathetic Decay fixed (sub-unity feedback gain vs. LP pole); cached APVTS atomic pointers in
    `processBlock` (+ removed 8 dead reads); skew-correct knob readouts + double-click reset.
  - Validated: `auval -v aumu OBwd OuDv` PASS; render-harness PASS at both default 0.0 (pure core,
    peak 0.053) and 0.5 (bristle, peak 0.166), no NaN/Inf, block-time maxRatio ≤ 1.86× (RT-clean).
- **2026-07-08 (v1.4.1):** Resolved the three runtime-affecting Info findings (PATCH):
  **IN-06** BodyResonator biquad NaN guard (sticky-silence; complements WR-01), **IN-07** Humanize
  drift rate now uses actual block size (was max-block-size → ~8× fast on small buffers),
  **IN-09** DC-blocker + engine state cleared in `prepareToPlay` (startup transient on SR change).
  Validated: auval PASS, render-harness PASS (peak 0.053, no NaN/Inf, maxRatio 1.85).
- **2026-09-23 (v1.9.1):** Resolved the v1.9.0 review's pitch-path findings via `/improve-review`
  (PATCH): **CR-02** pitch bend no longer restarts the string (`WaveguideString::setFrequency` +
  5 ms multiplicative glide; `trigger()` is note-on only), **WR-01** the Dorico Note Expression
  offset survives pitch bends (`noteTuningRatio`), **WR-05** the bridge filter is compensated with
  its phase delay at f0. The review's "+1 sample per rail" half of WR-05 was measured and rejected
  (JUCE `DelayLine` pop-then-push is exactly `setDelay`). Harness gained `--brightness`,
  `--ne-semis` and `--bend-vibrato`. Validated: auval PASS, pluginval 5 SUCCESS. The canonical golden
  sha256 stays the v1.9.0 anchor (the default render moves by design: A4 +4.2 c → +0.3 c).
- **2026-09-24 (v1.9.2):** Resolved **CR-01** via `/improve-review` (PATCH): the upper register
  plays its fundamental. `WaveguideString` raises β to a register floor (0 up to C#5, 0.30 from
  D5, a one-semitone step that skips the H2/H3 lock-in islands at β 0.10–0.22). MIDI ≤ 73 is
  byte-identical to v1.9.1. New `tests/register_gate.py` (10 factory presets × MIDI 45–96, gated
  from 74) PASSES at 44.1/48/96 kHz; v1.9.1 fails it. Harness gained `--sample-rate` and
  `--param id=norm`. Validated: auval PASS, pluginval 5 SUCCESS.
- **2026-09-24 (v1.9.3):** Resolved **CR-03 / CR-04 / WR-07 / WR-10** via `/improve-review`
  (PATCH), porting the O-Contrabass tuning-ownership design. `tuningSystem` owns the engine mode
  (async apply on the message thread; panel scale loads select Scala), with a new Tuning System select
  in the tuning overlay. `referencePitch` is applied as a voice-side ratio over an engine held at 440,
  so the full 220–880 Hz range works (CR-04 option chosen over narrowing the range or widening the
  shared clamp). Tuning state (intervals, name, temperament, tonic, stretch, KBM) persists through
  `customState`, and `setStateInformation` strips the stale `<CustomState>` child that the shared
  preset manager leaves in the tree. The default render is byte-identical (`c8aa14d6…`). The harness
  gained `--edo` / `--roundtrip-edo`. Validated: all pitch checks within +0.6 c, i18n/UI gates pass,
  auval SUCCEEDED.
- **2026-09-24 (v1.9.4):** A loaded `.kbm` now keeps its reference **note**. `referencePitch`
  gets the A4 that the file's reference note and frequency imply (60 @ 261.6256 → 440), not the
  frequency clamped to 400–480 Hz. The engine pins the reference note through the new
  scala-tuning-engine 3.2.0 `setKbmReferenceFrequency()`. Save .kbm writes the real reference
  frequency. The harness gained `--kbm` / `--kbm-roundtrip` / `--save-kbm`. The default render is
  byte-identical (`c8aa14d6…`).

## Known Issues

Open from the v1.9.0 review (`CODE_REVIEW.md`, 2026-09-23): **WR-02 / WR-03** (sympathetic loop),
**WR-04, WR-06, WR-08, WR-09, WR-11, WR-12**, and IN-01..IN-10.

- **Shared preset-manager bug: stale `<CustomState>` in the restored tree (found in v1.9.3).**
  `OuariconPresetManager::setStateFromXml` (module v1.0.7) leaves the `<CustomState>` child inside
  the APVTS tree, so every plugin that uses `setCustomStateCallbacks` (O-Contrabass, O-Wind and
  others) restores the state from the FIRST reopen at the second and later ones. O-Bowed strips it
  locally in `setStateInformation`. The module-level fix belongs in `preset-manager` (strip it in
  `setStateFromXml`, or in `getStateAsXml` before appending), which is a module upgrade across its
  consumers.
- **The tuning panel's reference knob spans 400–480 Hz (shared `tuning-panel.js`).** It writes
  `referencePitch`, so it works inside that range, but it can't reach 220–400 or 480–880 (use the
  footer Ref Pitch knob), and it only re-reads the parameter when the panel re-initialises.
- **Scale name when the scale is loaded but 12-TET is selected.** The engine only reports a loaded
  scale's own name in Scala mode, so a session saved with 12-TET selected restores the scale's
  intervals under the temperament menu's name. The intervals are exact.
- **Saved .kbm exports write A4 = 440.** The engine's A4 is held at 440 by design (CR-04), so "Save
  .kbm" writes 440 as the reference frequency, not the Ref Pitch value.
- **zh-Hans `tip.tuningSystem` is at `reviewed: 'mt'`** and fr at `reviewed: false`. Both need the
  normal promotion (blind back-translation for zh, a read for fr).

- **Mode lock-in islands below the register floor (found in v1.9.2 measurement, pre-existing).**
  CR-01's floor only covers D5 and up. Lower down, some (β, bow speed) pairs still lock to H2/H3.
  β ≈ 0.19 (Bow Position ≈ 60 %) at the default speed sounds H2 on every note C4–F#6. Viola sounds
  H3 at MIDI 63–66, and Double Bass H2 at 69–73. The likely root is the capped-injection junction
  (`min(frictionVel, |Δv|)` instead of STK's `ρ·Δv`), which picks modes chaotically. A fix means
  re-voicing every preset: an `/improve-milestone`, not a patch.
- **Reversed Friction sharpens pitch as β grows (pre-existing).** C4 at Bow Position max is
  +48 c with Reversed Friction 0.3. Since v1.9.2 the register floor puts β at 0.30 from D5, so
  Impossible Strings runs +14 to +24 c sharp at MIDI 74–86, and is still silent from 88.
- **Low Brightness doesn't speak (found in v1.9.1 measurement, pre-existing in v1.9.0).** At default
  bow settings the string is silent (rms < 5e-5) at Brightness 300 and 1000 Hz for A3/A4/A5, and
  for A4/A5 at 3 kHz. The loss filter takes too much loop gain for the bow to sustain the
  oscillation. The review's WR-05 pitch table for those settings came from a numeric model and was
  never audible. Not in the scope of the WR-05 fix. Worth a finding next to CR-01.

Older deferred items from the v1.3.0 review:

Remaining deferred Info-level findings from the v1.3.0 review (cosmetic / non-behavioral — IN-06,
IN-07, IN-09 were resolved in v1.4.1; IN-01, IN-03 in v1.4.0):
- **IN-02 / IN-04** — `ThermalFriction.h` and the elasto-plastic Newton-Raphson helper are unused dead code.
- **IN-05** — factory-preset authoring comment inverts the `brightness` skew exponent (stored values are correct).
- **IN-08** — dead per-block constant `setPan`; mislabeled step comments.
- **IN-11 / IN-12 / IN-13** — (IN-10's dead `bindComboBox`/`tuningSystem` select was resolved by CR-03 in v1.9.3) unused `savePreset` fn,
  unconditional 15 Hz visualization poll, and several unused registered tuning fns.
- Per-voice `updateParametersFromAPVTS` still uses string-keyed parameter lookups (WR-06 covered only
  the processor's `processBlock`, as the finding scoped it).
- Sessions saved under ≤1.3.0 that stored the old `bowHairStiffness=0.5` default will now play with
  50% bristle friction — unavoidable side effect of activating a previously-inert parameter.

## Additional Notes

Physical-modeling bowed-string synth: digital-waveguide string with nonlinear bow-friction excitation
(Hyperbolic + elasto-plastic bristle blend), 8-mode morphing body resonator, sympathetic-string
feedback bank, bow-noise / sub-harmonics / stereo-width / humanize stages, MPE synthesiser,
Scala/KBM microtonal tuning (shared `scala-tuning-engine`) with VST3 Note Expression, WebView UI.

- **AU triple:** `aumu` / `OBwd` / `OuAu` (release) or `OuDv` (dev).
- **Backup before v1.4.0:** `backups/O-Bowed/v1.3.0/`.
