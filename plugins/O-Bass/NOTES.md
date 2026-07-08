# O-Bass Notes

## Status
- **Current Status:** 📦 Installed
- **Version:** 1.3.3
- **Type:** Audio Effect (Bass Enhancer)

## Lifecycle Timeline

- **2026-07-08 (v1.3.3):** Resolved v1.3.2 deep code-review Critical + Warning findings — CR-01 (SafePointer-guarded the FileChooser launchAsync completions, bare-return on teardown), WR-01 (factory `crossover_freq` now authored in Hz + `convertTo0to1` so the 0.5 skew is honoured), WR-02 (`latency_mode` now applied live in `processBlock` via RT-safe atomic `setMode`), WR-03 (`applyPresetJson` resets all params to default before applying — inlined preset-manager v1.0.3 fix). Build + auval PASS.
- **2026-01-28 (v1.2.1):** Increased harmonic coefficients for more dramatic bass enhancement effect
- **2026-01-28 (v1.2.0):** Code quality cleanup - removed unused code (harmonicWeights, StereoMode::MatchOriginal, envelope followers, lookahead), extracted magic numbers to constants, added documentation for disabled features
- **2026-01-27 (v1.1.1):** Performance optimizations - IIR updates every 16 samples, debug-only buffer checks
- **2026-01-27 (v1.1.0):** Removed Colored mode, fixed dead code paths, restored limit indicator
- **2026-01-27 (v1.0.2):** Renamed from OBass to O-Bass (matches Ouaricon naming convention)
- **2026-01-27 (v1.0.1):** Fixed Logic Pro crash - buffer size validation in CleanModeProcessor, HarmonicGenerator, PitchTracker
- **2026-01-27 (v1.0.0):** Registered in PLUGINS.md, initial release

## Description

Bass enhancement plugin with crossover filtering and Chebyshev harmonic generation. Features:
- Crossover filter separating low and high frequency bands
- Mono summing for bass frequencies (phase coherence)
- Harmonic generation using Chebyshev polynomials (2nd + 3rd harmonics)
- WebView-based UI with preset management

## DSP Components

- **CrossoverFilter** - Separates audio into low/high bands with selectable latency modes
- **MonoSummer** - Converts bass frequencies to mono for phase coherence
- **CleanModeProcessor** - Orchestrates enhancement pipeline
- **HarmonicGenerator** - Adds harmonics using Chebyshev polynomials (T2, T3)
- **EnvelopeFollower** - (Reserved for future use)
- **PitchTracker** - (Reserved for future use - disabled due to RT performance)

## Parameters

- Enhance amount
- Crossover frequency
- Output gain
- Latency mode (Low Latency / High Fidelity)

## Known Issues / Limitations

- **WR-02 (High Fidelity mode, by design):** switching `latency_mode` now takes effect live,
  but in High Fidelity (FIR) mode a *crossover-frequency* change only reloads the FIR taps at
  the next `prepareToPlay()` (deferred-update design — reloading the convolution IR is not
  RT-safe). The IIR ↔ FIR toggle itself is immediate.
- **Deferred code-review Info findings (v1.3.2 review, opt-in):** IN-01 (dead
  `calculateHighBandEnergy` path over partly-stale tail samples), IN-02 (factory presets
  rewritten on every construction — NOT the v1.0.3 `.factory-version` sentinel; only the
  v1.0.3 `applyPresetJson` reset was inlined), IN-03 (per-sample `getSample`/`setSample` in
  output-gain + IIR loops), IN-04 (JS knob readout hardcodes ranges instead of
  `getScaledValue()`), IN-05 (Bypass isn't a true passthrough; two registered native fns
  unused). None affect correctness of the shipped paths.

## Additional Notes

- Uses WebView UI with botanical/paper aesthetic
- Includes Ouaricon preset manager
- Built with JUCE 8
