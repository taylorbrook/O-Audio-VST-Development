# O-Bass Notes

## Status
- **Current Status:** 📦 Installed
- **Version:** 1.2.1
- **Type:** Audio Effect (Bass Enhancer)

## Lifecycle Timeline

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

## Known Issues

None documented.

## Additional Notes

- Uses WebView UI with botanical/paper aesthetic
- Includes Ouaricon preset manager
- Built with JUCE 8
