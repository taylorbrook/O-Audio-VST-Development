# O-Wind Changelog

## [1.2.0] - 2026-04-06

### Added — Attack Transient "Chiff" Modeling

Models the four-phase flute onset (noise burst, vortex shedding, edge-tone, pipe-tone) per Auvray et al. (2014):

**Chiff Noise Burst (JetExciter):**
- On noteOn, turbulence noise is boosted 3-6x above steady-state level (scaled by `attackChiff * velocity`)
- Exponential decay over 20-40ms (faster at higher velocity) back to normal noise floor
- Near-zero CPU cost — envelope-gated, active only during first ~40ms per note

**Pitch Overshoot (FluteSynthVoice):**
- Bore delay starts 1-2% shorter than target at noteOn (sharper pitch), scaled by `attackChiff * velocity`
- Settles to target pitch over 50-100ms via one-pole smoothing filter
- Models jet-bore coupling delay before pipe-tone regime locks

**New APVTS Parameter:**
- `attackChiff` (0.0-1.0, default 0.5) — controls transient noise burst intensity and pitch overshoot amount

**Factory Preset Values:**
- Shakuhachi: 0.70 (prominent chiff), Pan Flute: 0.65, Bansuri: 0.55
- Concert Flute: 0.50, Piccolo: 0.45, Native Am. Flute: 0.40
- Recorder: 0.25, Ocarina: 0.15 (minimal chiff)

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
