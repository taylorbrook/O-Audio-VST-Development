# O-Bowed Notes

## Status
- **Current Status:** 📦 Installed
- **Version:** 1.4.1
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

## Known Issues

Remaining deferred Info-level findings from the v1.3.0 review (cosmetic / non-behavioral — IN-06,
IN-07, IN-09 were resolved in v1.4.1; IN-01, IN-03 in v1.4.0):
- **IN-02 / IN-04** — `ThermalFriction.h` and the elasto-plastic Newton-Raphson helper are unused dead code.
- **IN-05** — factory-preset authoring comment inverts the `brightness` skew exponent (stored values are correct).
- **IN-08** — dead per-block constant `setPan`; mislabeled step comments.
- **IN-10 / IN-11 / IN-12 / IN-13** — dead `bindComboBox`/`tuningSystem` select, unused `savePreset` fn,
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
