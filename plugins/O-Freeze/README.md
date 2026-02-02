# O-Freeze

A granular freeze effect plugin by Ouaricon Development.

## Overview

O-Freeze captures and sustains audio using granular synthesis, creating smooth, evolving textures from any sound source. When engaged, the plugin loops a short segment of audio through 8 overlapping grains with asymmetric windowing for artifact-free sustain.

## Features

- **8-Grain Granular Engine** - Ultra-smooth texture with 87.5% grain overlap
- **Dual Trigger Modes** - Manual button or automatic threshold detection
- **Drift Control** - Add subtle variation to frozen textures
- **Smooth Crossfades** - 50ms fade-in, 100ms fade-out for click-free transitions
- **RMS Threshold Gate** - 20ms detection window with 3dB hysteresis
- **2-Second Freeze Buffer** - Captures rich harmonic content

## Parameters

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| FREEZE | On/Off | Off | Manual freeze trigger (Manual mode only) |
| THRESHOLD | -60 to 0 dB | -40 dB | Auto-freeze level (Threshold mode only) |
| MODE | Manual/Threshold | Manual | Trigger mode selection |
| DRIFT | 0-100% | 25% | Grain position randomization |
| MIX | 0-100% | 100% | Dry/Wet blend |

## Installation

### macOS

Copy the plugin to the appropriate folder:

- **VST3:** `~/Library/Audio/Plug-Ins/VST3/O-Freeze.vst3`
- **AU:** `~/Library/Audio/Plug-Ins/Components/O-Freeze.component`

After installation, restart your DAW or rescan plugins.

## Usage

### Manual Mode

1. Set MODE to "Manual"
2. Play audio through the plugin
3. Click the FREEZE button to capture and sustain the current sound
4. Click again to release and return to live audio

### Threshold Mode

1. Set MODE to "Threshold"
2. Adjust THRESHOLD to the desired level
3. When input drops below threshold, freeze engages automatically
4. When input rises above threshold (+3dB hysteresis), freeze releases

### Drift Control

- **0%** - Static frozen texture (perfect loops)
- **25%** (default) - Subtle variation for natural movement
- **50-100%** - More pronounced texture evolution

### Mix Control

- **100%** (default) - Fully wet (frozen signal only when engaged)
- **50%** - Parallel blend (frozen + dry)
- **0%** - Bypass (dry signal only)

## System Requirements

- **macOS:** 10.13 or later
- **Formats:** VST3, AU
- **CPU:** Intel or Apple Silicon
- **DAW:** Any VST3/AU compatible host (Logic Pro, Ableton Live, etc.)

## Version

V1.0.0

---

*Ouaricon Development - Taylor Brook*
