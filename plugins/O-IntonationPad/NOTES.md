# O-IntonationPad Notes

## Status
- **Current Status:** 📦 Installed
- **Version:** 2.8.2
- **Type:** Synth (Wavetable Pad)

## Lifecycle Timeline

- **2026-04-26 (v2.8.0):** Added VST3 Note Expression microtonal support for Dorico; adopted shared `note-expression` module.
- **2026-07-09 (v2.8.2):** Safe INFO-tier cleanup sweep (IN-01/02/04/05/07) — extracted silence-threshold constant, removed dead voice methods + 5 unused native functions, fixed the `tuning-panel.js` docstring, dropped an unused UI parameter. No behavior change; auval PASSED.
- **2026-07-08 (v2.8.1):** Resolved all Critical + Warning findings from the v2.8.0 deep code review (`CODE_REVIEW.md`) via `/improve-review`. See CHANGELOG for per-finding root causes.
  - **Audio-thread RT-safety:** wavetable bank generation (CR-01), EQ coefficient rebuilds (CR-04), and automatable tuning-param rebuilds (CR-05) all moved off the audio callback.
  - **WebView correctness:** knob readouts now skew-aware via `getScaledValue()` (CR-02); `keyRoot` rewired to a ComboBox relay (WR-02).
  - **Crash safety:** 5 FileChooser dialogs now use `SafePointer` + bare-return (CR-03).
  - **Preset determinism:** factory presets seed from defaults (CR-06); preset names sanitized for filesystem safety (WR-04).
  - **DSP correctness:** chord generation allocation-free (WR-01), delay lines sized for >96 kHz (WR-03), chord base note clamped (WR-05), NE-consistent fallback path (WR-06).
  - Built VST3 + AU, auval `aumu OuIP OuDv` PASSED (incl. 192 kHz render test).

## Known Issues

Info-tier findings from `CODE_REVIEW.md` still open after the v2.8.2 sweep (opt-in, low/no priority):
- **IN-06:** `tuning_temperamentPreset` has no UI control — now formally **automation-only** (its dead WebView accessors were removed in v2.8.2 / IN-04). Add a dropdown later if UI control is wanted.
- **IN-09:** torn read of the frequency table during a concurrent rebuild — benign, and moot now that CR-05 moved rebuilds off the audio thread.
- **IN-10:** reverb pre-delay not cleared on 0↔N transition; `getActiveNotes` UI race — cosmetic/low priority.
- **IN-11:** `smoothedGainA/B` never flushed to zero — mitigated by `ScopedNoDenormals` (FTZ/DAZ).

Resolved: IN-01/02/04/05/07 (v2.8.2), IN-03 folded into CR-01 + IN-08 (v2.8.1), all CR/WR (v2.8.1).

## Additional Notes

Wavetable pad synth: dual morphing-wavetable oscillators driving up to 12 chord sub-voices per note, a full microtonal `TuningEngine` (Scala/KBM + built-in temperaments + VST3 Note Expression), a chorus→delay→EQ→reverb effects chain, 12 factory presets, and a WebView UI with a shared tuning panel.

- **Build:** `./scripts/build-and-install.sh O-IntonationPad`
- **Validate (macOS):** `auval -v aumu OuIP OuDv` (dev branding) / `auval -v aumu OuIP OuAu` (release)
- **Install locations:** `~/Library/Audio/Plug-Ins/VST3/`, `~/Library/Audio/Plug-Ins/Components/`
