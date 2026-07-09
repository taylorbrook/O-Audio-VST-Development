# O-Lyrica Notes

## Status
- **Current Status:** 📦 Installed
- **Version:** 2.3.1
- **Type:** Synth (Physical Modeling Harp)

## Lifecycle Timeline

- **2026-04-24 (v2.3.0):** Adopted the shared `note-expression` module (VST3 Note Expression for
  Dorico microtonal playback); O-Lyrica is the reference consumer. Spike diagnostic code stripped.
- **2026-07-08 (v2.3.1):** Resolved the Critical + Warning findings (plus defense-in-depth Info
  riders) from the v2.3.0 deep code review (`CODE_REVIEW.md`) via `/improve-review`:
  - **CR-01** WebView `launchAsync` UAF → `SafePointer` + bare-`return` on all 7 file dialogs.
  - **CR-02** `loadEmbeddedTuning` dropped the period → all 24 library tunings mistuned; now appends
    `tuning->period`.
  - **CR-03…CR-06** audio-thread heap allocs on IIR coefficient rebuilds (EQ / BodyResonance /
    SympatheticResonance / PluckExciter) → `ArrayCoefficients` `operator=` into pre-seeded buffers.
  - **CR-07** `StringMaterial::name` `juce::String` off the render path (member removed).
  - **WR-01…WR-04** sample-rate stability (Nyquist cutoff clamp, waveguide finite guard + validated
    `trigger()` frequency, rail-delay floor, HF-correct delay buffer).
  - **WR-05** `sympatheticQ` factory values skew-corrected (`sqrt(old)`).
  - **WR-06** `stringCrosstalk` documented as intentionally hidden (see Known Limitations).
  - **WR-07 / WR-08** double-click reset (new `getParameterDefaults` native fn) + skew-aware knob
    readouts (`getScaledValue()`).
  - **WR-09…WR-12** cached-pointer discipline, reused MIDI buffer, on-screen-keyboard thread race,
    voice `ScopedNoDenormals`.
  - **IN-01/02/03/06/07/09** NaN guards, integrator bound, per-block hoist, EQ cache reset, acquire
    ordering.

## Known Issues / Limitations

- **`stringCrosstalk` has no UI control (by design).** It is a real, audible parameter (adjacent-string
  soundboard coupling, default 0.2) driven exclusively by factory/user presets and host automation.
  It is intentionally kept off the editor panel; the absence of a knob is not a bug (WR-06). Removing
  it would be a breaking state-format change.
- **v2.3.1 changed the sound of all 48 factory presets' sympathetic resonance (WR-05).** Sharpness was
  previously pinned near the minimum (Q≈0.13) for every preset due to a skew mismatch; presets now
  reach their intended Q (≈0.7–3.7 spread). An audition pass is recommended to confirm taste.
- **`auval` static "Meta Param Flag" warning is pre-existing and benign.** It is caused by the
  intentional `freeToggle`/`scaleToggle` mutual exclusion (v1.30.0). All render / MIDI / parameter
  round-trip tests pass; the AU loads and runs correctly.

## Additional Notes

- Physical-modeling harp: digital-waveguide string with stiffness/dispersion allpass chain, plucked
  comb-filter excitation, string-material morphing, 5-mode body resonator, sympathetic-string feedback
  bank, EQ/reverb/delay/chorus, Scala/KBM microtonal tuning (VST3 Note Expression — O-Lyrica is the
  project's validated NE reference/spike), glissando controller, self-contained WebView UI.
- **Build:** target name is `OLyrica` (folder is `O-Lyrica`); build `OLyrica_VST3` / `OLyrica_AU`.
  `build-and-install.sh` currently assumes the folder name for the target and fails for this plugin
  (`unknown target 'O-Lyrica_VST3'`) — build the `OLyrica_*` targets directly and install manually.
- **AU triple:** `aumu OLyr <mfr>` (dev builds: manufacturer `OuDv`).
