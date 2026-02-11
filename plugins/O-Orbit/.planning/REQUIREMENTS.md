# O-Orbit Requirements

> Auto-extracted from BRIEF.md on 2026-02-09

## Functional Requirements

### FR-1: Motion Engine
- FR-1.1: Generate source position (azimuth, elevation, distance) over time
- FR-1.2: Support path shapes: Orbit, Pendulum, Linear, Drift, Custom
- FR-1.3: Configurable speed from 0.01 to 20 Hz
- FR-1.4: Tempo sync to host BPM (1/16 note through 4 bars)
- FR-1.5: Angular width control (0-360 degrees)
- FR-1.6: Distance variation (depth) for near-far movement
- FR-1.7: Path tilt for 3D rotation of the path plane
- FR-1.8: Phase offset (starting position on path)
- FR-1.9: Optional elevation enable for 3D paths (off by default)

### FR-2: VBAP Renderer
- FR-2.1: Compute per-speaker gains from source position and speaker layout
- FR-2.2: 2D VBAP for ear-level speaker arrays
- FR-2.3: 3D VBAP (convex hull triangulation) for arrays with elevation
- FR-2.4: Handle non-equidistant speaker placements
- FR-2.5: Gain interpolation between blocks (anti-zipper)
- FR-2.6: Center divergence parameter for phantom center spread

### FR-3: Speaker Layout System
- FR-3.1: Preset layouts: Stereo, Quad, 5.1, 7.1, 5.1.4, 7.1.4, Hexaphonic, Octaphonic
- FR-3.2: Custom layout editor: add, remove, reposition speakers
- FR-3.3: Speaker positions defined by azimuth, elevation, distance
- FR-3.4: Save/load custom layouts as user presets
- FR-3.5: Import/export layout files for sharing

### FR-4: Distance Model
- FR-4.1: Level attenuation based on distance
- FR-4.2: Selectable attenuation curve: Linear, Inverse, Inverse Square
- FR-4.3: Air absorption as frequency-dependent LPF (HF rolloff with distance)

### FR-5: Source Handling
- FR-5.1: Accept mono or stereo input
- FR-5.2: Mono mode: sum input to single source
- FR-5.3: L/R Split mode: stereo input becomes two independent orbiting sources
- FR-5.4: Configurable phase offset between L and R sources in split mode

### FR-6: Auto-Downmix
- FR-6.1: Detect when DAW provides fewer channels than configured layout
- FR-6.2: Energy-preserving fold-down to available channel count
- FR-6.3: Stereo fallback uses equal-power panning from VBAP azimuth
- FR-6.4: Visual warning in UI showing layout vs. DAW mismatch

### FR-7: Mix Control
- FR-7.1: Dry/wet mix parameter (0-100%)
- FR-7.2: Dry signal passes through original channel configuration

## Non-Functional Requirements

### NFR-1: Performance
- NFR-1.1: VBAP gain computation per audio block, not per sample
- NFR-1.2: Total CPU lighter than a typical reverb plugin
- NFR-1.3: Support buffer sizes 64-2048 at sample rates 44.1-192 kHz

### NFR-2: Compatibility
- NFR-2.1: VST3 + AU formats
- NFR-2.2: macOS and Windows
- NFR-2.3: Multi-channel output bus negotiation (2 to 24 channels)
- NFR-2.4: Windows WebView2 with static linking
- NFR-2.5: Auto-downmix ensures usability in any DAW regardless of surround support

### NFR-3: UI
- NFR-3.1: WebView-based UI
- NFR-3.2: Animated orbital visualizer with source trail
- NFR-3.3: Speaker position display with channel labels
- NFR-3.4: Interactive speaker layout editor
- NFR-3.5: Dark aesthetic with glowing accents
- NFR-3.6: Responsive to window resize

## Parameters Summary

| ID | Parameter | Range | Automatable |
|----|-----------|-------|-------------|
| P1 | Path | Orbit/Pendulum/Linear/Drift/Custom | Yes |
| P2 | Speed | 0.01-20 Hz | Yes |
| P3 | Width | 0-360 deg | Yes |
| P4 | Depth | 0-100% | Yes |
| P5 | Tilt | -90 to +90 deg | Yes |
| P6 | Phase | 0-360 deg | Yes |
| P7 | Elevation Enable | On/Off | Yes |
| P8 | Elevation Range | 0-90 deg | Yes |
| P9 | Tempo Sync | Off/musical divisions | Yes |
| P10 | Speaker Layout | Preset + Custom | No (state) |
| P11 | Distance | 0.1-30 m | Yes |
| P12 | Air Absorption | 0-100% | Yes |
| P13 | Attenuation Curve | Linear/Inverse/InvSquare | Yes |
| P14 | Center Diverge | 0-100% | Yes |
| P15 | Source Mode | Mono/L+R Split | Yes |
| P16 | L/R Offset | 0-360 deg | Yes |
| P17 | Mix | 0-100% | Yes |
