# Changelog

All notable changes to O-Texture will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.1.0] - 2026-02-15

### Added

- Neural texture synthesis engine (1D CNN VAE, 32-dimensional latent space)
- 10 real-time parameters: Source, Mode, X, Y, Character A/B, Evolve, Freeze, Brightness, Mix
- 6 source categories: Rain, Metal, Wind, Crowd, Synth, Organic
- Generate and Transform modes (IS_SYNTH instrument plugin)
- XY pad with orbital trail animation (Canvas 2D, 30fps throttled)
- Evolve modulation via 28-channel Perlin noise with quintic interpolation
- Freeze mode (halts latent evolution, pauses trail animation)
- Brightness tilt filter (1-pole, 800 Hz pivot, SmoothedValue for zipper-free)
- Stereo decorrelation via latent offset (0.1 on X/Y dimensions)
- Overlap-add crossfading (4096-sample blocks, 2048-sample hop, Hann window)
- ONNX Runtime decoder inference (direct C++ API, synchronous)
- Ouaricon Naturalist WebView UI (aged paper, botanical motifs, serif typography)
- 3 vertical sliders (Character A, B, Evolve) with naturalist styling
- 6 source icon buttons with inline SVG line art
- 2 rotary knobs (Brightness, Mix) with seed cross-section visuals
- Fern botanical overlay (bottom-right, low opacity)
- State serialization including evolve noise seed and cursor positions
- Full JUCE 8 WebView relay/attachment parameter binding system
- ANIRA v2.0.3 + ONNX Runtime 1.19.2 embedded in plugin bundles (macOS)
- Ad-hoc code signing for local testing

### Technical Notes

- Uses placeholder ONNX models; real trained models will be integrated in a future version
- Plugin registers as AU instrument (aumu OuTx OuDv)
- JUCE 8.0.4, C++20
- Latent space mapping loaded from dim_map_rain.json (BinaryData)
- Pre-allocated decoder output buffer for real-time safety
