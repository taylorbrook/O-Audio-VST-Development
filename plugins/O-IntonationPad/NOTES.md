# O-IntonationPad Notes

## Status
- **Current Status:** 📦 Installed
- **Version:** 2.8.1
- **Type:** Synth (Wavetable Pad)

## Lifecycle Timeline

- **2026-04-26 (v2.8.0):** Added VST3 Note Expression microtonal support for Dorico; adopted shared `note-expression` module.
- **2026-07-08 (v2.8.1):** Resolved all Critical + Warning findings from the v2.8.0 deep code review (`CODE_REVIEW.md`) via `/improve-review`. See CHANGELOG for per-finding root causes.
  - **Audio-thread RT-safety:** wavetable bank generation (CR-01), EQ coefficient rebuilds (CR-04), and automatable tuning-param rebuilds (CR-05) all moved off the audio callback.
  - **WebView correctness:** knob readouts now skew-aware via `getScaledValue()` (CR-02); `keyRoot` rewired to a ComboBox relay (WR-02).
  - **Crash safety:** 5 FileChooser dialogs now use `SafePointer` + bare-return (CR-03).
  - **Preset determinism:** factory presets seed from defaults (CR-06); preset names sanitized for filesystem safety (WR-04).
  - **DSP correctness:** chord generation allocation-free (WR-01), delay lines sized for >96 kHz (WR-03), chord base note clamped (WR-05), NE-consistent fallback path (WR-06).
  - Built VST3 + AU, auval `aumu OuIP OuDv` PASSED (incl. 192 kHz render test).

## Known Issues

Info-tier findings from `CODE_REVIEW.md` deferred from the v2.8.1 pass (opt-in, out of scope):
- **IN-01:** silence threshold `0.0001f` duplicated 4× in `WavetableVoice.cpp` (extract a constant).
- **IN-02:** dead voice methods `setWavetablePosition` / `setWavetablePosition2` (non-LFO) — never called.
- **IN-04:** 5 registered native functions never called from JS (`setTuningIntervals`, `setTemperamentPreset`, `getTemperamentPreset`, `getEmbeddedTuningCategories`, `getPresetCategories`).
- **IN-05:** `tuning-panel.js` docstring still tells callers to pass `window.__JUCE__` (should be the `Juce` module namespace) — standing MEMORY.md note.
- **IN-06:** `tuning_temperamentPreset` has no UI control (automation-only); confirm intent.
- **IN-07:** `wireWavetableDisplay` unused `numBanks` parameter.
- **IN-09:** torn read of the frequency table during a concurrent rebuild — benign, and moot now that CR-05 moved rebuilds off the audio thread.
- **IN-10:** reverb pre-delay not cleared on 0↔N transition; `getActiveNotes` UI race — cosmetic/low priority.
- **IN-11:** `smoothedGainA/B` never flushed to zero — mitigated by `ScopedNoDenormals` (FTZ/DAZ).

Note: IN-03 (bank setters / `setChordGenerationParams` run every block without a change guard) was folded into the CR-01 fix (bank setters are now change-gated).

## Additional Notes

Wavetable pad synth: dual morphing-wavetable oscillators driving up to 12 chord sub-voices per note, a full microtonal `TuningEngine` (Scala/KBM + built-in temperaments + VST3 Note Expression), a chorus→delay→EQ→reverb effects chain, 12 factory presets, and a WebView UI with a shared tuning panel.

- **Build:** `./scripts/build-and-install.sh O-IntonationPad`
- **Validate (macOS):** `auval -v aumu OuIP OuDv` (dev branding) / `auval -v aumu OuIP OuAu` (release)
- **Install locations:** `~/Library/Audio/Plug-Ins/VST3/`, `~/Library/Audio/Plug-Ins/Components/`
