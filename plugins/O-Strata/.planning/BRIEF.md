# O-Strata - Creative Brief

## Overview

**Type:** Synth
**Core Concept:** A microtonal wavetable synthesizer whose wavetables are generated from 3D geometry — sliced meshes, orbits through signed-distance and noise volumes, and image terrains — baked alias-free into an O-Prism-class mipmap engine, with a live 3D view of the geometry being scanned.
**Status:** 💡 Ideated
**Created:** 2026-09-07

## Vision

O-Strata is O-Prism's voice, filters, modulation, effects and tuning engine with the wavetable *library* replaced by a wavetable *generator*. Instead of picking a table from a list, the player picks a shape — a torus, a twisted star, a crescent, an imported OBJ, a gyroid volume, a greyscale PNG — and a way of moving through it. A plane sweeps through the mesh bottom to top; a knot orbits inside the volume; an orbit grows across the terrain. Every step of that motion becomes one frame of a 256-frame wavetable, and the Position knob morphs through the geometry the way Serum's Position knob morphs through a table.

The sound-design proposition is that shape is a better mental model than spectrum. "Rotate the slicing plane 20 degrees" and "make the orbit fatter" are things a player can see and predict; the resulting harmonic changes are continuous, physically motivated and often surprising. Mesh contours give mild, string-like spectra (roughly −12 dB/oct from polygon corners), noise volumes give broadband grit, SDF fields shaped through tanh sit between — so the three source families cover the timbral range a wavetable synth needs.

Three things nobody else ships, per the September 2026 landscape survey: mesh slicing, SDF-volume orbits, and terrain synthesis paired with a real microtonal engine. Aaron Anderson's *Terrain* (free, GPL) owns live analytic wave-terrain; Conductive Labs and Carbon Electra 2 own PNG terrains; *Terrain*'s open issue #5 asks for 3D model loading with no implementation. O-Strata answers that request and inherits the 24+ factory tunings, Scala/KBM import and EDO/rank-2 generators that O-Prism already has.

Everything in v1.0 is **baked**. The generators run on a background thread, publish a finished `WavetableData` through the atomic swap and reaper O-Prism already owns, and the audio thread runs the unchanged O-Prism oscillator. That means exact FFT-truncation anti-aliasing, no oversampling, no new audio-thread code paths, and presets that store generator parameters rather than tables. The live wave-terrain oscillator — per-oscillator 2× oversampling, pitch-scaled terrain spatial frequency, and the verified Chebyshev "bandlimited terrain" mode that needs no oversampling at all — is the planned v1.1 improvement, with its own CPU gate.

## Parameters

O-Strata inherits O-Prism v1.24.0's parameter set unchanged for the **Envelope, Filter, LFO/Mod Matrix, Tuning, Effects and Global** sections (see `plugins/O-Prism/.planning/BRIEF.md`). Only the oscillator section changes: `oscATable` / `oscBTable` are replaced by a per-oscillator **Geometry** block. All other Osc A/B parameters (Position, Level, Pan, Coarse, Fine, Phase, Unison, Detune, Width, warps, FM) carry over; **Position** becomes the frame index through the geometry sweep.

Every parameter below exists for both Osc A (`oscA…`) and Osc B (`oscB…`). Parameters marked **(bake)** trigger a debounced background re-bake when changed; they are persisted in state and presets and are exposed to automation, but automating them causes re-bakes, not per-sample modulation.

### Geometry — common

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Source Type (bake) | Mesh / Volume / Terrain | Mesh | Which generator family feeds this oscillator |
| Frame Count (bake) | 64 / 128 / 256 | 256 | Frames in the baked table; fewer = faster bake, coarser morph |
| Shape Drive (bake) | 0.0–1.0 | 0.0 | Post-unwrap tanh drive before normalisation; adds harmonics to mild mesh contours |

### Geometry — Mesh (slice sweep)

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Mesh (bake) | Library index or Imported | Torus | Built-in library (sphere, torus, twisted 5-star, crescent, torus knot, fBm blob, …) or a user OBJ/STL |
| Slice Tilt X (bake) | −90–+90° | 0 | Rotation of the slicing plane about X; second morph axis |
| Slice Tilt Y (bake) | −90–+90° | 0 | Rotation of the slicing plane about Y |
| Projection Angle φ (bake) | 0–360° | 0 | Emits cos φ·x(s) + sin φ·y(s) of the contour; a free timbre axis (0° and 90° are the two Lissajous shadows) |
| Unwrap Mode (bake) | XY Projection / Centroid Distance | XY Projection | Centroid distance d(s) is brighter but sits behind a flatness floor (near-circular slices fall back to XY) |
| Loop Policy (bake) | Largest / Sum | Largest | Multi-loop slices (torus): keep the largest-area loop with hysteresis, or sum all loops |

### Geometry — Volume (orbit through a field)

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Field (bake) | Torus SDF / Box SDF / Gyroid / Sphere SDF / fBm Noise | Torus SDF | The scalar field f(x,y,z) sampled by the orbit |
| Orbit (bake) | Torus Knot (2,3) / (3,5) / (5,7) / Lissajous Knot | Torus Knot (2,3) | Closed 3D curve traced once per cycle |
| Sweep Axis (bake) | Orbit Scale / Knot Phase / Z Offset | Orbit Scale | Which orbit property advances frame-to-frame |
| Sweep Range (bake) | 0.0–1.0 | 0.6 | How far the sweep axis travels across the 256 frames |
| Field Detail (bake) | 0.0–1.0 | 0.3 | Spatial frequency of the field (noise octaves / SDF repeat); higher = more partials |

### Geometry — Terrain (orbit sweep over a heightmap)

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Terrain (bake) | Library index or Imported PNG | Sine Product | Built-in analytic terrains (sine product, radial rings, clean-room re-derivations) or a greyscale PNG |
| Orbit Shape (bake) | Ellipse / Epitrochoid 3·5·7 / Hypocycloid 3·5·7 / Superellipse | Ellipse | Closed 2D curve scanned once per cycle |
| Orbit Centre X / Y (bake) | −1.0–1.0 | 0.0 / 0.0 | Centre offset → asymmetry / even harmonics; also driven by dragging the 3D view |
| Orbit Aspect (bake) | 0.1–1.0 | 1.0 | Ellipse minor/major ratio |
| Orbit Rotation (bake) | 0–360° | 0 | Phase relationship between X and Y → PWM / phase-distortion-like effects |
| Sweep Axis (bake) | Radius / Rotation / Centre X / Centre Y | Radius | Which orbit property advances frame-to-frame (radius = brightness ramp) |
| Sweep Range (bake) | 0.0–1.0 | 0.8 | Travel of the sweep axis across the frames; orbit is clamped inside [−1,1]² |
| Image Blur (bake) | 0.0–1.0 | 0.2 | Gaussian pre-blur on PNG terrains (pixel detail is broadband) |
| Edge Mode (bake) | Mirror / Window | Mirror | Mitsuhashi edge handling for PNG terrains |

### Non-parameter state

- Imported OBJ/STL/PNG sources are persisted in plugin state (gzip + base64) up to a size cap; above the cap the state stores path + SHA-256 and the UI shows a "source missing, using library fallback" notice on load. Exact cap is a Stage 0 decision.
- The baked tables themselves are never stored; they are regenerated from parameters on load, on a background thread, with the previous table (or silence on first load) playing until the swap.

## UI Concept

**Layout:** O-Prism's 1200 × 800 layout and section structure. Each oscillator panel's wavetable display becomes a two-mode view: the existing Serum-style stacked-frame wavetable view (reusing `WavetableDisplay`) and a **3D geometry view** showing what is being scanned — the mesh with the slicing plane and the extracted contour highlighted; the volume as a low-res point cloud with the orbit and current scan point; the terrain as a displaced wireframe heightmap with the orbit drawn on it. A playhead (frame index + phase) is pushed at 30 Hz.
**Visual Style:** Ouaricon Naturalist brand (ouaricon-naturalist-001), as O-Prism.
**Key Elements:**
- 3D view: hand-rolled WebGL2 with a Canvas 2D fallback in the same file, no library (prototype: `research/wavetable-synthesis-3d-geometry-prototypes/webgl-3d/terrain-proto.html`, 5.5 KB gzipped).
- Drag on the terrain/mesh view moves orbit centre or slice tilt; scroll wheel changes sweep range. Document-level move/up pattern, `wheel {passive:false}`.
- Drag-and-drop OBJ/STL/PNG onto the oscillator panel (macOS `webkitGetAsEntry` pattern) plus a file-chooser button.
- Bake progress indicator per oscillator (O-TextureForge precedent); the view redraws only when a push dirtied state.
- "WebGL unavailable" placeholder as a localised `data-i18n` node (en / fr / zh-Hans).

## Use Cases

- **Evolving pads from a shape.** Load the twisted-star mesh, set Position under a slow LFO, tilt the slicing plane with the mod wheel: the pad morphs through cross-sections the player can watch.
- **Microtonal wavetable lead.** Bohlen-Pierce or 31-EDO from the tuning tab, a torus-knot orbit through a gyroid volume for a metallic, formant-like spectrum that stays alias-free at the top of the keyboard.
- **Bring your own geometry.** Drop an OBJ exported from Blender or a greyscale texture PNG; the plugin slices or scans it into a playable table in under a second and stores it in the preset.
- **Sound-design exploration.** Sweep Projection Angle φ and Shape Drive on a mesh contour to find timbres between the two Lissajous shadows, then freeze the result as a factory-style wavetable.
- **Film / game texture beds.** fBm noise volumes with small Sweep Range give dense, slowly varying noise tables for drones, run through the O-Prism FX rack.

## Inspirations

- **Aaron Anderson — *Terrain*** (JUCE, GPL-3.0, 2024): live analytic wave terrain, ~20 orbit curves, trajectory feedback. Formulas re-derived clean-room; no code copied (O-Strata is AGPL-3.0, Terrain is GPL-3.0).
- **Conductive Labs *Terrain Synth*** (hardware, 2025) and the ADC25 talk "Implementing Wave Terrain Synthesis": PNG/RGB-channel terrains, clean single-cycle pitch.
- **Xfer Serum**: Position-morph workflow, stacked-frame 3D table view.
- **Scanned synthesis** (Verplank / Mathews / Shaw; Csound `scanu/scans`; Wablet): the idea that a geometry can animate itself — deferred, see Out of Scope.
- **3D-printing slicers** (Minetto et al. 2017): plane-mesh contour extraction as a solved problem.
- **DAFx 2026 "Arbitrary Polygon Oscillator"** (Argentieri & Scagliola): independent prior art for constant-arc-length traversal of a plane through a polyhedron.
- **O-Prism** (v1.24.0): the engine, UI shell, tuning engine and preset system O-Strata forks from.

## Technical Notes

Full research: `research/wavetable-synthesis-3d-geometry.md` (Level 2 + Level 3, 2026-09-07). Prototypes and raw benchmark outputs: `research/wavetable-synthesis-3d-geometry-prototypes/` (Python slicer, C++ terrain-oscillator benchmark, WebGL2 HTML prototype).

**Fork base.** O-Prism v1.24.0: `WavetableData` (2048 samples + guard, ≤256 frames, 10 mipmap levels), `WavetableGenerator::generateMipmaps()` (FFT truncation per level), atomic `WavetableData*` publish with generation-counted retired-table reaper, scala-tuning-engine v3.0.1, preset-manager v1.0.6, en/fr/zh-Hans i18n.

**Mesh slicer (verified in Python, 128 slices × 2048 samples on sphere/torus/star/fBm-sphere/crescent):**
- Arc-length resampling (uniform s, cumulative chord length) emitting cos φ·x(s) + sin φ·y(s). Radial r(θ) unwrap is rejected (fails on non-star-shaped loops). Centroid distance d(s) needs a flatness floor (~0.02 ptp/r_mean) or circular slices normalise facet noise to 0 dBFS.
- Multi-loop: largest-area loop with centroid-hysteresis tracking (adjacent-frame RMS 0.017 mean / 0.034 max), sum as an option, never concatenate (0.287 / 0.607).
- Frame alignment: FFT cross-correlation with polarity test, chained frame-to-frame. Fixed-ray alignment fails on geometry that twists about the slice axis. Cost: one 2048-point FFT pair per frame.
- Normalisation: per-frame DC removal, global peak across all frames (matches `WavetableImporter.cpp:189-205`). Never per-frame peak.
- C++ carry-overs: nudge vertices with |z−h| < ε off the plane; key intersections by sorted vertex-pair edge id (each key appears in exactly two segments on a closed manifold → chaining is an adjacency walk); orient loops CCW by signed area; area-weighted polygon centroid.
- Cost estimate: 10–30 ms for a 256-frame bake at 40 k triangles in C++, ~1 s at 1 M triangles. Volume bake 0.1–0.7 ms/frame.

**Volume and terrain generators.** SDF fields (torus, box, sphere, gyroid) and fBm noise; tanh-shaped before unwrap. Terrain orbit sweep reproduces most of what *Terrain* does at control rate, alias-free. PNG terrains: decode via `juce::ImageFileFormat` on the message thread, pre-blur, mirror or window edges (Mitsuhashi continuity constraints), keep the orbit strictly inside [−1, 1]².

**Threading.** Bake on a background thread, debounced (~150 ms) after the last bake-parameter change; publish via the existing atomic swap; retire the old table on the message thread. Two memory patterns apply: *source-swap needs a lock if prepareToPlay publishes*, *retired-map reaper must not free on the audio thread*. OBJ/STL/PNG parsing is never touched from `processBlock`.

**3D view.** Hand-rolled WebGL2, R32F heightmap texture, `texSubImage2D` on table update; Canvas 2D fallback mandatory (WebView2 `getContext('webgl2')` can return null; WKWebView `flat` interpolation crashes the shader compiler — avoid it; handle `webglcontextlost` / `webglcontextrestored`). 30 Hz push via `emitEventIfBrowserIsVisible` + backend event listener, not `evaluateJavascript` (JUCE issue #1415). Playhead event is a sub-100-byte change-gated JSON; the 64×64 view table crosses as base64 Float32 (~21 KB) on change; the full 256×2048 table never crosses as text. Re-push after every preset apply (one-shot push goes stale on preset load). Measured in Chromium: 0.12 ms/frame WebGL2, 0.25 ms Canvas 2D; WKWebView/WebView2 numbers are unverified and are a Stage 3 gate.

**Roadmap mapping.** Research §7.4 stages ≠ plugin-workflow stages: research Stage 1 (baked generators) is workflow Stage 2 (DSP); research Stage 3 (3D view) is workflow Stage 3 (GUI); research Stage 2 (live terrain oscillator) is the v1.1 improvement.

**Licence.** AGPL-3.0-or-later, as O-Prism. Terrain (GPL-3.0) formulas re-derived from the mathematics with attribution; no source copied.

## Out of Scope (v1.0)

| Feature | Reason | Target |
|---------|--------|--------|
| Live wave-terrain oscillator (per-osc 2× oversampling, HQ 4×, pitch-scaled terrain spatial frequency, Chebyshev bandlimited mode at 1×, live PNG terrain via atomic swap, unison cap 4, trajectory feedback) | New audio-thread code path with its own CPU budget (8.8–16% core at 16 voices × 2 osc, unison 1); the research roadmap orders it after baking | v1.1 |
| Scanned synthesis (mass-spring mesh evolving at 0–15 Hz, scanned into frames at block rate) | Stability (clamp + leak) and a new block-rate simulation; fits the bake pipeline once that exists | v1.2+ |
| ADAA for terrains | Mathematically unsound for arbitrary composite f(x(θ), y(θ)); open research item, not a plan | none |
| RGB-channel terrain morphing (three terrains from one PNG) | Conductive Labs feature; easy extension of the terrain generator once PNG import is stable | v1.1 |
| In-plugin mesh editing / sculpting | Out of the instrument's remit; import from Blender instead | none |
| three.js / lit mesh rendering | No bundler in O-Prism; three.js is ESM-only (79 KB gz); revisit only if lit OBJ display is demanded | v1.x |

## Next Steps

- [ ] Create UI mockup (`/start O-Strata` → option 3)
- [ ] Start implementation (`/implement O-Strata`)
