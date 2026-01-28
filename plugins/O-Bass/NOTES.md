# O-Bass Notes

## Status
- **Current Status:** 📦 Installed
- **Version:** 1.0.2
- **Type:** Audio Effect (Bass Enhancer)

## Lifecycle Timeline

- **2026-01-27 (v1.0.2):** Renamed from OBass to O-Bass (matches Ouaricon naming convention)
- **2026-01-27 (v1.0.1):** Fixed Logic Pro crash - buffer size validation in CleanModeProcessor, HarmonicGenerator, PitchTracker
- **2026-01-27 (v1.0.0):** Registered in PLUGINS.md, initial release

## Description

Bass enhancement plugin with crossover filtering and harmonic generation. Features:
- Crossover filter separating low and high frequency bands
- Mono summing for bass frequencies
- Clean mode processing (transparent bass enhancement)
- Colored mode processing (harmonic generation)
- Envelope follower for dynamic response
- Pitch tracking for intelligent processing
- WebView-based UI with preset management

## DSP Components

- **CrossoverFilter** - Separates audio into low/high bands with selectable latency modes
- **MonoSummer** - Converts bass frequencies to mono with stereo expansion
- **CleanModeProcessor** - Transparent bass enhancement
- **ColoredModeProcessor** - Harmonic generation for richer bass
- **EnvelopeFollower** - Dynamic response tracking
- **PitchTracker** - Pitch detection for intelligent processing
- **HarmonicGenerator** - Adds harmonics to bass content

## Parameters

- Enhance amount
- Crossover frequency
- Mode selection (Clean/Colored)
- Output gain
- Latency mode (Low Latency / High Fidelity)

## Known Issues

None documented.

## Additional Notes

- Uses WebView UI with botanical/paper aesthetic
- Includes Ouaricon preset manager
- Built with JUCE 8
