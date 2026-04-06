# O-Wind Changelog

## [1.1.0] - 2026-04-06

### Improved — Noise Model & Spectral Realism

Three-pass physical model refinement for more realistic flute timbre:

**Pass A: Jet-Bore Energy Balance**
- Replaced single 2nd-order lowpass bore loss filter with two cascaded 1st-order lowpass filters (~2kHz base damping + ~8kHz harmonic rolloff) for frequency-dependent viscothermal loss — higher harmonics now lose more energy per round trip, creating natural spectral thinning
- Lowered radiation filter cutoff across all presets (concert flute 300→150Hz, proportional for others) to retain more fundamental energy in low notes

**Pass B: Register Transitions**
- End reflection filter replaced with high-shelf (-6dB above ~2kHz) — higher frequencies reflect less from the open bore end, modeling realistic radiation impedance and improving octave transition behavior

**Pass C: Noise & Spectral Realism**
- Replaced single lowpass noise filter with 2nd-order bandpass centered on Strouhal frequency (f_s = 0.2 × jet_velocity / jet_diameter) — turbulence spectrum now physically tracks breath pressure
- Added `jetDiameter` to InstrumentPreset struct (~7mm piccolo to ~15mm shakuhachi) for per-instrument Strouhal tuning

## [1.0.1] - 2026-04-05

### Fixed

- **Stuck voice / infinite sustain bug:** Notes triggered on note-off and sustained forever
  - **Root cause 1:** `JetNonlinearity` velocity floor (`max(0.01, vel)`) provided residual drive to bore waveguide even after breath stopped, keeping the feedback loop alive indefinitely
  - **Root cause 2:** Voice clearing required waveguide silence (`silentSampleCount >= 512`), which never occurred due to the residual drive, trapping voices in permanent "releasing" state
  - **Fix:** Gate nonlinearity output to zero when jet velocity < 0.001 (breaks feedback loop on release); add 10ms release tail fade with guaranteed voice clearing after breath envelope completes

## [1.0.0] - 2026-04-05

- Initial release: Physical modeling flute synthesiser (Verge 1995 jet-drive model)
