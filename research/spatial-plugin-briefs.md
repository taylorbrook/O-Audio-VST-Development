---
title: "Spatial Audio Plugin & Application Briefs"
created: 2026-02-08
domain: market-research
type: market-research
keywords:
  - spatial-audio
  - plugin-briefs
  - product-planning
  - ambisonics
  - binaural
---
# Spatial Audio Plugin & Application Briefs

> Based on research: `sound-spatialization-algorithms.md`, `spatial-audio-plugins-market-research.md`
> Date: 2026-02-08

Five concepts ranging from accessible stereo effects to full 3D spatial engines. Each brief is self-contained and ready for `/plan` if selected.

---

## Brief 1: O-Orbit

### Concept
A **stereo spatial motion engine** that moves sound through space using orbital paths, pendulum swings, and random drift. Think auto-panner meets spatial designer -- every track gets a sense of physical movement without requiring headphones or surround setups.

### Character
- **Kinetic** -- sound is always in motion, from subtle drift to aggressive spirals
- Combines equal-power panning with Haas-effect delay, decorrelation widening, and frequency-dependent ILD for convincing motion on regular speakers
- Musical -- paths lock to tempo, so motion becomes rhythmic

### Why It Works
- **Market position:** No plugin combines stereo motion paths + distance modeling + tempo sync in one focused UI. Waves Brauer Motion ($35) does rhythmic panning but no distance. Panagement ($59) does binaural distance but no motion paths. This merges both.
- **Accessibility:** Stereo in/stereo out. Works everywhere. No surround or headphone requirement.
- **Low DSP complexity:** All techniques are simple-to-moderate. No HRTF convolution needed.

### Signal Flow
```
Input -> [Path Engine (XY position over time)]
              |
              v
         [Stereo Panner (equal-power)]
         [Haas Delay (0-1.5ms ITD)]
         [Distance Attenuation (1/r)]
         [Air Absorption (1-pole LPF)]
         [Width (decorrelation)]
              |
              v
         [Dry/Wet Mix] -> Output (stereo)
```

### Parameters (~10)
| Parameter | Range | Description |
|-----------|-------|-------------|
| **Path** | Orbit / Pendulum / Drift / Linear / Custom | Motion trajectory shape |
| **Speed** | 0.01 - 20 Hz (or sync to tempo) | Path traversal rate |
| **Width** | 0 - 200% | Spatial width of the path |
| **Depth** | 0 - 100% | Distance variation (near-far movement) |
| **Tilt** | -90 to +90 deg | Rotate the path plane |
| **Phase** | 0 - 360 deg | Starting position on path |
| **Distance** | 0 - 100% | Constant distance offset (affects LPF + level) |
| **Haas** | 0 - 100% | ITD contribution to positioning |
| **Spread** | 0 - 100% | Decorrelation amount for width |
| **Mix** | 0 - 100% | Dry/wet blend |

### UI Vision
- Central animated path visualizer showing the sound position orbiting in 2D
- Path shape selection as visual icons
- Tempo-sync toggle snaps speed to musical divisions
- Warm, dark background with glowing position trail

### Complexity Assessment
- **DSP:** Simple (panning + delay + LPF + decorrelation)
- **External deps:** None
- **Estimated build time:** 1 stage cycle

### Target Format
- VST3 + AU, WebView UI, Stereo in/Stereo out

---

## Brief 2: O-Sphere

### Concept
A **binaural 3D panner and room simulator** that positions sound anywhere in a virtual sphere around the listener using HRTF convolution. Directly fills the gap left by dearVR's discontinuation. Ship a polished, affordable alternative with SOFA file support and a WebGL-powered 3D interface.

### Character
- **Immersive** -- convincing 3D placement with elevation, azimuth, and distance on headphones
- Natural room simulation using early reflections (image source method) + late reverb (FDN)
- Ships with MIT KEMAR HRTF; users can load custom SOFA files for personalized spatialization

### Why It Works
- **Market gap:** dearVR discontinuation (July 2025) leaves no mid-range ($99-199) binaural panner with room simulation. The only free options (SPARTA, IEM) have academic UIs.
- **Differentiation:** Modern WebView UI with WebGL 3D sphere visualization. SOFA file loading. Distance modeling with air absorption.
- **Technical feasibility:** SAF library (ISC license) provides HRTF convolution, room simulation, and interpolation. libmysofa (BSD-3) handles SOFA loading. Both proven with JUCE.

### Signal Flow
```
Input (mono/stereo) -> [HRTF Convolution (azimuth, elevation)]
                            |
                            v
                       [Distance Model]
                       - Level attenuation (1/r)
                       - Air absorption (freq-dependent LPF)
                       - Near-field correction
                            |
                            v
                       [Room Simulation]
                       - Early reflections (ISM, order 1-2)
                       - Late reverb (FDN, parameterized by room size)
                            |
                            v
                       [Dry/Wet] -> Output (binaural stereo)
```

### Parameters (~12)
| Parameter | Range | Description |
|-----------|-------|-------------|
| **Azimuth** | -180 to +180 deg | Horizontal angle (automatable) |
| **Elevation** | -90 to +90 deg | Vertical angle |
| **Distance** | 0.1 - 30 m | Source distance from listener |
| **Room Size** | Small / Medium / Large / Hall | Virtual room dimensions |
| **Room Damping** | 0 - 100% | Wall absorption (affects reverb brightness) |
| **Reflections** | 0 - 100% | Early reflection level |
| **Reverb** | 0 - 100% | Late reverb level |
| **Air Absorption** | 0 - 100% | Distance-dependent HF rolloff |
| **Width** | 0 - 200% | Stereo input width before spatialization |
| **Near Field** | On/Off | Proximity effect for close sources |
| **HRTF Set** | Default / Custom SOFA | HRTF dataset selection |
| **Mix** | 0 - 100% | Dry/wet blend |

### UI Vision
- WebGL 3D sphere showing source position as a glowing dot
- Click and drag to position source in 3D space
- Room visualization behind the sphere (subtle wireframe walls)
- Distance shown as dot size (near = large, far = small)
- SOFA file drag-and-drop loading zone

### External Dependencies
- **SAF** (ISC) -- HRTF convolution, room simulation, interpolation
- **libmysofa** (BSD-3) -- SOFA HRTF file loading
- **MIT KEMAR HRTF** -- bundled default dataset

### Complexity Assessment
- **DSP:** Moderate-Complex (HRTF convolution + ISM reflections + FDN reverb)
- **External deps:** SAF + libmysofa (both CMake-compatible)
- **Estimated build time:** 2-3 stage cycles

### Target Format
- VST3 + AU, WebView UI (WebGL), Stereo in/Stereo out (binaural)

---

## Brief 3: O-Drift

### Concept
A **spatial texture effect** that takes incoming audio and slowly drifts copies of it through 3D space, creating evolving ambient halos and spatial clouds. Part delay, part granular, part spatializer -- each "echo" gets its own position, movement, and distance in binaural 3D.

### Character
- **Ethereal** -- sounds dissolve into spatial clouds
- Each delay tap / grain has independent azimuth, elevation, and distance that drift over time
- Creates "spatial reverbs" that aren't room-based but movement-based
- Feedback creates spiraling spatial trails

### Why It Works
- **Unique concept:** No plugin spatializes individual delay taps or grains in true 3D. Spacelab touches this (spectral-spatial reverb) but costs significantly more. This is "granular meets binaural" in an effect plugin.
- **Leverages existing skills:** Combines Ouaricon's granular expertise (O-GrainScatter, O-Freeze) with the spatialization research.
- **Creative tool, not technical tool:** Aimed at producers and sound designers, not immersive mixing engineers.

### Signal Flow
```
Input -> [Delay Buffer / Grain Capture]
              |
              v
         [Voice Pool (8-16 spatial voices)]
         Each voice has:
           - Delay/grain playback
           - Independent HRTF position (az, el, dist)
           - LFO-driven position drift
           - Feedback path
              |
              v
         [Binaural Sum (HRTF per voice)] -> [Mix] -> Output (stereo)
```

### Parameters (~12)
| Parameter | Range | Description |
|-----------|-------|-------------|
| **Voices** | 2 - 16 | Number of spatial delay/grain voices |
| **Time** | 10ms - 2s (or sync) | Base delay time / grain interval |
| **Scatter** | 0 - 100% | Random time offset between voices |
| **Drift Speed** | 0.01 - 5 Hz | How fast voices move through space |
| **Drift Range** | 0 - 360 deg | Angular range of voice movement |
| **Elevation** | 0 - 100% | How much voices move vertically |
| **Distance** | 0.5 - 20 m | How far voices spread from center |
| **Feedback** | 0 - 90% | Voice output fed back (creates spirals) |
| **Damping** | 0 - 100% | HF loss per feedback iteration |
| **Pitch** | -12 to +12 st | Pitch shift per voice (stacks on feedback) |
| **Freeze** | On/Off | Capture and loop current buffer |
| **Mix** | 0 - 100% | Dry/wet blend |

### UI Vision
- Dark sphere with glowing dots for each voice, slowly orbiting
- Trails showing recent voice positions (spatial echoes visible)
- Central "source" pulsing with input signal
- Minimal controls -- the visualization is the main interface

### External Dependencies
- **SAF** or lightweight custom HRTF engine (could use minimum-phase HRIRs for efficiency)
- **MIT KEMAR HRTF** bundled

### Complexity Assessment
- **DSP:** Moderate-Complex (multi-voice HRTF + delay + feedback)
- **Unique risk:** CPU budget for 16 simultaneous HRTF convolutions -- may need sparse HRIRs or partitioned convolution
- **Estimated build time:** 2-3 stage cycles

### Target Format
- VST3 + AU, WebView UI (WebGL), Stereo in/Stereo out (binaural)

---

## Brief 4: O-Widen

### Concept
A **stereo width and spatial enhancement** plugin. The Swiss Army knife for making mixes wider, deeper, and more dimensional -- without leaving stereo. Combines M/S processing, frequency-dependent widening, Haas effect, decorrelation, and distance simulation in one clean interface.

### Character
- **Transparent** -- enhances space without coloring the sound
- Mono-compatible by design (all widening techniques checked against mono fold-down)
- Three modes: Subtle (mastering), Creative (mixing), Extreme (sound design)

### Why It Works
- **Everyday utility plugin:** Every mix needs stereo work. This is the plugin you put on every bus.
- **Market position:** Competes with Ozone Imager (free), Infected Mushroom Wider, S1 Stereo Imager -- but adds depth dimension (distance/proximity) that none of them have.
- **Simplest build:** No external dependencies. Pure JUCE DSP. Could be done in a single stage cycle.

### Signal Flow
```
Input (stereo) -> [M/S Encode]
                       |
                  [Mid Processing]        [Side Processing]
                  - Level                  - Level
                  - EQ (tilt)             - EQ (tilt)
                  - Bass mono (< crossover) - Decorrelation
                       |                       |
                  [M/S Decode] -> [Haas Widening] -> [Distance Sim]
                                                          |
                                                     [Mix] -> Output
```

### Parameters (~8)
| Parameter | Range | Description |
|-----------|-------|-------------|
| **Width** | -100% to +200% | Stereo width (0 = mono, 100 = original, 200 = hyper-wide) |
| **Bass Mono** | 0 - 500 Hz | Crossover below which signal is mono-summed |
| **Tilt** | -6 to +6 dB | Frequency-dependent widening (positive = widen highs more) |
| **Haas** | 0 - 100% | Precedence effect widening (careful: mono compat) |
| **Depth** | 0 - 100% | Distance simulation (LPF + level = perceived depth) |
| **Proximity** | 0 - 100% | Near-field boost (bass lift for close sources) |
| **Correlation** | Display only | Real-time correlation meter (mono safety check) |
| **Mix** | 0 - 100% | Dry/wet blend |

### UI Vision
- Central vectorscope / correlation display
- Width shown as expanding/contracting field
- Correlation meter with green/yellow/red zones (mono safety)
- Clean, mastering-grade aesthetic

### Complexity Assessment
- **DSP:** Simple (M/S + EQ + delay + decorrelation)
- **External deps:** None
- **Estimated build time:** 1 stage cycle

### Target Format
- VST3 + AU, WebView UI, Stereo in/Stereo out

---

## Brief 5: O-Rooms

### Concept
A **spatial reverb application** (standalone + plugin) that models real acoustic spaces. Unlike convolution reverbs that replay static impulse responses, O-Rooms uses the image source method for early reflections and an FDN for late reverb, with all reflections spatialized in binaural 3D. Move the source and listener positions in real-time and hear the room change around you.

### Character
- **Architectural** -- the room is the instrument
- Physical parameters: room dimensions (L x W x H), wall materials, source/listener positions
- Every early reflection arrives from a physically correct direction via HRTF
- Late reverb density and color respond to room geometry

### Why It Works
- **Novel approach:** No affordable plugin combines geometric room modeling with binaural spatialization of individual reflections. Convolution reverbs are static. Algorithmic reverbs (Valhalla, FabFilter Pro-R) don't model specific geometry. This is the bridge.
- **Educational appeal:** Seeing reflections bounce off walls in the UI teaches acoustics intuitively.
- **Scalable complexity:** Can start with simple rectangular rooms and later add irregular geometry, material presets, etc.

### Signal Flow
```
Input (mono) -> [Direct Path: HRTF(source->listener direction)]
                     |
                     v
                [Image Source Method (order 1-3)]
                - For each reflection:
                  - Compute image source position
                  - Calculate path length -> delay
                  - Apply wall absorption (material-dependent)
                  - Spatialize via HRTF at arrival direction
                     |
                     v
                [FDN Late Reverb]
                - RT60 derived from room volume + absorption
                - Diffuse, omnidirectional tail
                     |
                     v
                [Sum: direct + reflections + tail] -> Output (binaural stereo)
```

### Parameters (~14)
| Parameter | Range | Description |
|-----------|-------|-------------|
| **Room Length** | 2 - 50 m | Room dimension |
| **Room Width** | 2 - 50 m | Room dimension |
| **Room Height** | 2 - 10 m | Room dimension |
| **Wall Material** | Concrete / Wood / Carpet / Glass / Curtain | Absorption coefficients |
| **Source X** | 0 - 100% | Source position in room (left-right) |
| **Source Y** | 0 - 100% | Source position in room (front-back) |
| **Source Height** | 0 - 100% | Source elevation |
| **Listener X** | 0 - 100% | Listener position (left-right) |
| **Listener Y** | 0 - 100% | Listener position (front-back) |
| **Reflection Order** | 1 / 2 / 3 | Complexity of early reflections (6 / 30 / 150) |
| **Late Reverb** | 0 - 100% | FDN reverb level |
| **Damping** | 0 - 100% | HF absorption in reverb tail |
| **Pre-Delay** | 0 - 100 ms | Gap before reverb onset |
| **Mix** | 0 - 100% | Dry/wet blend |

### UI Vision
- **Top-down room view** showing walls, source (dot), listener (head icon), and reflection paths as bouncing lines
- Drag source and listener to reposition
- Reflection paths animate when audio plays
- Wall material selector with visual texture hints
- Room presets: Bedroom, Studio, Concert Hall, Cathedral, Warehouse

### External Dependencies
- **SAF** (ISC) -- HRTF convolution, room acoustics utilities
- **libmysofa** (BSD-3) -- SOFA file loading
- **MIT KEMAR HRTF** bundled

### Complexity Assessment
- **DSP:** Complex (ISM + per-reflection HRTF + FDN reverb)
- **CPU risk:** Order 3 = 150 reflections x HRTF convolution. May need to limit to order 1-2 at high polyphony, or use sparse HRIRs.
- **Estimated build time:** 3-4 stage cycles

### Target Format
- VST3 + AU + Standalone, WebView UI (Canvas 2D room view), Stereo in/Stereo out (binaural)

---

## Comparison Matrix

| Brief | Name | Type | Complexity | Ext. Deps | Headphones Required | Market Gap |
|-------|------|------|-----------|-----------|-------------------|------------|
| 1 | **O-Orbit** | Stereo motion | Simple | None | No | Rhythmic spatial motion |
| 2 | **O-Sphere** | 3D binaural panner | Moderate-Complex | SAF, libmysofa | Yes | dearVR replacement |
| 3 | **O-Drift** | Spatial delay/grain | Moderate-Complex | SAF or custom | Yes | Spatial granular FX |
| 4 | **O-Widen** | Stereo enhancer | Simple | None | No | Utility with depth |
| 5 | **O-Rooms** | Geometric reverb | Complex | SAF, libmysofa | Yes | Physical room modeling |

### Build Order Recommendation
1. **O-Widen** or **O-Orbit** first (no external deps, stereo-only, fast to ship)
2. **O-Sphere** next (fills biggest market gap, establishes SAF integration)
3. **O-Drift** or **O-Rooms** as ambitious follow-ups
