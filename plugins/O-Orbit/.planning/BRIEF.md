# O-Orbit Creative Brief

> **Status:** Ideation
> **Date:** 2026-02-09
> **Author:** Taylor Brook + Claude
> **Based on:** `research/spatial-plugin-briefs.md` (Brief 1: O-Orbit, expanded)

---

## Concept

A **universal orbital spatializer** that moves sound through space using orbital paths, pendulum swings, linear sweeps, and random drift, rendered via **VBAP (Vector Base Amplitude Panning)** to any speaker array. From stereo to 7.1.4 Atmos to fully custom non-equidistant layouts -- one plugin handles every spatial format.

The motion engine generates source position (azimuth, elevation, distance) over time. The VBAP renderer converts that position into per-speaker gains. The result: physical, kinetic sound movement through real speaker arrays.

## Character

- **Kinetic** -- sound is always in motion, from subtle drift to aggressive spirals
- **Universal** -- same plugin works on stereo, quad, 5.1, 7.1, 7.1.4, hexaphonic, octaphonic, or any custom array
- **Musical** -- paths lock to tempo, so motion becomes rhythmic
- **Dual-purpose** -- precise spatial placement for immersive mixing AND creative motion effects for sound design
- Distance modeling with attenuation and air absorption gives a real sense of near/far

## Why It Works

- **No competitor does this:** Existing spatial motion plugins are locked to stereo (Waves Brauer Motion, Panagement) or locked to specific surround formats. No plugin combines orbital motion paths with VBAP rendering to arbitrary speaker arrays in a single focused tool.
- **VBAP is proven:** Industry-standard algorithm by Ville Pulkki. Works with any speaker geometry, 2D or 3D. CPU-cheap -- geometry + gain math, not convolution.
- **Format flexibility is the differentiator:** The custom speaker layout editor lets users define any array, even non-equidistant. This serves experimental composers, immersive installations, academic spatial audio, and standard surround mixers equally.
- **Auto-downmix fallback:** If the DAW can't provide enough channels, the plugin automatically folds down to what's available (with a visual warning), so it always works everywhere.

## Signal Flow

```
Input (mono or stereo)
        |
        v
   [Source Mode: Mono Sum / L+R Split]
        |
        v
   [Motion Engine]
   - Path shape (Orbit, Pendulum, Linear, Drift, Custom)
   - Position over time: azimuth, elevation*, distance
   - Tempo sync optional
   - *Elevation: off by default, enable for 3D paths
        |
        v
   [Distance Model]
   - Level attenuation (inverse distance law)
   - Air absorption (frequency-dependent LPF)
        |
        v
   [VBAP Renderer]
   - Speaker layout definition (preset or custom)
   - Triangulation of speaker positions
   - Per-speaker gain calculation
   - Auto-downmix when DAW channel count < layout
        |
        v
   [N-Channel Output] (2 to 24+ channels)
```

## Parameters

### Motion Engine (~8 params)
| Parameter | Range | Description |
|-----------|-------|-------------|
| **Path** | Orbit / Pendulum / Linear / Drift / Custom | Motion trajectory shape |
| **Speed** | 0.01 - 20 Hz (or sync to tempo) | Path traversal rate |
| **Width** | 0 - 360 deg | Angular span of the path |
| **Depth** | 0 - 100% | Distance variation (near-far movement) |
| **Tilt** | -90 to +90 deg | Rotate the path plane (3D mode) |
| **Phase** | 0 - 360 deg | Starting position on path |
| **Elevation** | On/Off + range 0-90 deg | Enable vertical movement |
| **Tempo Sync** | Off / 1/16 - 4 bars | Lock speed to host tempo |

### Spatial Rendering (~5 params)
| Parameter | Range | Description |
|-----------|-------|-------------|
| **Speaker Layout** | Preset list + Custom | Active speaker configuration |
| **Distance** | 0.1 - 30 m | Base distance offset |
| **Air Absorption** | 0 - 100% | Frequency-dependent distance HF rolloff |
| **Attenuation Curve** | Linear / Inverse / Inverse Square | Distance-to-level law |
| **Center Diverge** | 0 - 100% | Spread center signal to adjacent speakers |

### Mix / Source (~3 params)
| Parameter | Range | Description |
|-----------|-------|-------------|
| **Source Mode** | Mono / L+R Split | Single source or stereo split into two orbits |
| **L/R Offset** | 0 - 360 deg | Phase offset between L and R sources (split mode) |
| **Mix** | 0 - 100% | Dry/wet blend |

**Total: ~16 automatable parameters**

## Speaker Layout System

### Preset Layouts
| Layout | Channels | Description |
|--------|----------|-------------|
| Stereo | 2 | L, R at +/-30 deg |
| Quad | 4 | L, R, Ls, Rs at +/-45, +/-135 deg |
| 5.1 | 6 | L, R, C, LFE, Ls, Rs (ITU-R BS.775) |
| 7.1 | 8 | 5.1 + Lss, Rss side surrounds |
| 5.1.4 | 10 | 5.1 + 4 height speakers |
| 7.1.4 | 12 | 7.1 + 4 height speakers (Atmos bed) |
| Hexaphonic | 6 | 6 speakers at 60 deg intervals |
| Octaphonic | 8 | 8 speakers at 45 deg intervals |

### Custom Layout Editor
- Visual 2D/3D view of speaker positions
- Click to add/remove speakers
- Drag to reposition (azimuth, elevation, distance from center)
- Non-equidistant placement fully supported
- Save/load custom layouts as presets
- Import/export layout files for sharing

## Auto-Downmix Behavior

When the DAW provides fewer output channels than the configured speaker layout:
1. Display a visual warning in the UI (e.g., "Layout: 7.1 -> DAW: Stereo")
2. Fold VBAP gains down to available channels using energy-preserving downmix
3. Stereo fallback uses equal-power panning derived from VBAP azimuth
4. All spatial motion remains audible -- just rendered to fewer speakers

## UI Vision

### Main View
- Central animated **orbital visualizer** showing the source(s) moving along their path
- Speaker positions shown as icons around the perimeter (2D top-down or 3D perspective)
- Source shown as a glowing dot with a trailing path
- In L/R split mode, two dots orbit (different colors)
- Path shape selection as clickable visual icons
- Tempo sync toggle snaps speed to musical divisions

### Speaker Layout Editor (secondary panel)
- Top-down circle/sphere view
- Drag speakers to any position
- Azimuth/elevation/distance readout per speaker
- Preset buttons along the top
- Save/Load custom layout buttons

### Aesthetic
- Dark background with subtle grid
- Glowing trails in the brand accent color
- Speaker icons with channel labels
- Warm, cinematic feel -- not clinical
- Downmix warning as an unobtrusive badge, not a modal

## Technical Notes

### VBAP Implementation
- Can use SAF (Spatial Audio Framework, ISC license) which includes production VBAP
- Or implement from scratch (~300-400 lines core): Delaunay triangulation of speaker positions, gain computation per triangle
- 2D VBAP for ear-level arrays, 3D VBAP (convex hull triangulation) when elevation speakers present
- Gain interpolation between blocks to avoid zipper noise

### Multi-Channel Bus Configuration
- VST3 supports declaring multiple output bus arrangements
- Plugin advertises supported layouts and lets the DAW negotiate
- JUCE `AudioProcessor::BusesProperties` configured for flexible output
- Maximum 24 output channels covers all practical speaker arrays

### Performance
- VBAP gain calculation is per-block, not per-sample (very cheap)
- Distance model is simple: gain + 1-pole LPF per source
- Total CPU: lighter than a typical reverb plugin
- No convolution, no HRTF -- pure amplitude panning + filtering

## Complexity Assessment

- **DSP:** Simple-Moderate (VBAP math + distance filtering + LFO motion)
- **Architecture:** Moderate (multi-channel bus negotiation, speaker layout persistence)
- **UI:** Moderate (animated orbital viz + speaker layout editor)
- **External deps:** Optional SAF for VBAP, or pure custom implementation
- **Estimated build time:** 2 stage cycles

## Target Format

- VST3 + AU
- WebView UI
- Mono/Stereo in, N-channel out (2 to 24)
- macOS + Windows (with WebView2 static linking)

## Differentiation Summary

| Feature | O-Orbit | Brauer Motion | Panagement | SPARTA |
|---------|---------|---------------|------------|--------|
| Orbital motion paths | Yes | Rhythmic pan only | No | No |
| Arbitrary speaker arrays | Yes (any) | Stereo only | Stereo only | Yes |
| Custom layout editor | Yes | No | No | Yes (but academic UI) |
| Distance modeling | Yes | No | Yes | Yes |
| Tempo sync | Yes | Yes | No | No |
| Auto-downmix | Yes | N/A | N/A | No |
| Accessible UI | Yes (WebView) | Yes | Yes | No |
| Price target | Affordable | $35 | $59 | Free |
