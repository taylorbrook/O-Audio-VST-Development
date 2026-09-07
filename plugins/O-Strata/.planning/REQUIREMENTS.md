# O-Strata - Requirements

---
version: 1.0.0
plugin: O-Strata
created: 2026-09-07
lastUpdated: 2026-09-07
---

## Overview

**Target Milestone:** v1.0
**Total Requirements:** 27
**Coverage:** must: 18 | should: 7 | nice: 2

## Requirements

### Functional (FUNC)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| FUNC-01 | Each oscillator (A, B) selects a geometry source type (Mesh / Volume / Terrain) whose generator bakes a multi-frame wavetable; Position morphs through the frames | must | pending | stage-2 |
| FUNC-02 | Mesh slice sweep: a plane sweeps a closed triangle mesh bottom→top, each slice contour becomes one frame; Slice Tilt X/Y and Projection Angle φ shape the result | must | pending | stage-2 |
| FUNC-03 | Volume orbit sweep: a closed 3D orbit samples an SDF or fBm field; Sweep Axis (scale / phase / z-offset) advances frame-to-frame | must | pending | stage-2 |
| FUNC-04 | Terrain orbit sweep: a closed 2D orbit scans an analytic or PNG heightmap; Sweep Axis (radius / rotation / centre) advances frame-to-frame | must | pending | stage-2 |
| FUNC-05 | Built-in mesh library (≥6 meshes) and built-in terrain library (≥4 analytic terrains) embedded as binary data | must | pending | stage-2 |
| FUNC-06 | User OBJ and STL import via file chooser and drag-and-drop; parse on the message thread | must | pending | stage-3 |
| FUNC-07 | User greyscale PNG terrain import via file chooser and drag-and-drop with Image Blur and Edge Mode | must | pending | stage-3 |
| FUNC-08 | Imported sources persist in plugin state (gzip+base64 under a cap; path+SHA above it) and presets regenerate tables on load | must | pending | stage-4 |
| FUNC-09 | All O-Prism v1.24.0 non-oscillator sections (envelopes, dual filters, LFOs, mod matrix, FX rack, global) carry over unchanged | must | pending | stage-2 |
| FUNC-10 | Full microtonal tuning engine (scala-tuning-engine v3.0.1: factory tunings, Scala/KBM import, EDO / harmonic / rank-2 generators, tuning tab) | must | pending | stage-2 |
| FUNC-11 | Factory presets showcasing each source family (≥5 per family) | should | pending | stage-4 |
| FUNC-12 | Frame Count selectable 64 / 128 / 256 | should | pending | stage-2 |

### DSP (DSP)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| DSP-01 | Baked tables pass through the O-Prism FFT-truncation mipmap pipeline; oscillator read path is unchanged (exact anti-aliasing, no oversampling) | must | pending | stage-2 |
| DSP-02 | Mesh contour → cycle uses arc-length resampling emitting cos φ·x(s)+sin φ·y(s); Centroid Distance mode sits behind a flatness floor | must | pending | stage-2 |
| DSP-03 | Multi-loop slices resolve by largest-area loop with centroid hysteresis by default; Sum optional; concatenation never | must | pending | stage-2 |
| DSP-04 | Adjacent frames are aligned by FFT cross-correlation with polarity test, chained frame-to-frame | must | pending | stage-2 |
| DSP-05 | Normalisation: per-frame DC removal, global peak across all frames (never per-frame peak) | must | pending | stage-2 |
| DSP-06 | Terrain orbits are clamped inside [−1,1]² and PNG terrains are pre-blurred with Mirror/Window edge handling | must | pending | stage-2 |
| DSP-07 | Shape Drive applies tanh post-unwrap, pre-normalisation | should | pending | stage-2 |

### UI (UI)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| UI-01 | Per-oscillator 3D geometry view (mesh + slicing plane + contour / volume point cloud + orbit / terrain wireframe + orbit) with a 30 Hz playhead, hand-rolled WebGL2 | should | pending | stage-3 |
| UI-02 | Canvas 2D fallback in the same view when WebGL2 is unavailable; localised placeholder in en/fr/zh-Hans | must | pending | stage-3 |
| UI-03 | Drag on the view edits orbit centre / slice tilt; wheel edits sweep range; document-level move/up pattern | should | pending | stage-3 |
| UI-04 | Per-oscillator bake progress indicator; stacked-frame wavetable view retained as a second mode | should | pending | stage-3 |
| UI-05 | 3D view and playhead re-push after every preset apply | must | pending | stage-3 |

### Performance (PERF)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| PERF-01 | Real-time safe audio processing (no allocations, locks, file I/O or bake work in processBlock) | must | pending | stage-2 |
| PERF-02 | Bake runs on a background thread, debounced; 256-frame bake of a 40 k-triangle mesh completes in ≤ 100 ms; audio continues on the previous table until the atomic swap | must | pending | stage-2 |
| PERF-03 | 3D view costs ≤ 2 ms per frame in WKWebView and WebView2 at DPR 2 (Chromium-measured 0.12 ms is not evidence for either) | should | pending | stage-3 |

### Compatibility (COMPAT)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| COMPAT-01 | Passes pluginval validation (VST3 and AU) at strictness 10, and auval | must | pending | stage-1 |
| COMPAT-02 | Windows VST3 build via CI (WebView2, static linking) | must | pending | stage-4 |

### Quality (QUAL)

| ID | Description | Priority | Status | Verified At |
|----|-------------|----------|--------|-------------|
| QUAL-01 | No audio artifacts at normal parameter ranges; non-harmonic energy at C6 ≤ −90 dB relative to the fundamental on any baked table | must | pending | stage-2 |
| QUAL-02 | No clicks when morphing Position through a baked table (adjacent-frame RMS after alignment ≤ 0.05 on the built-in library) | must | pending | stage-2 |
| QUAL-03 | Loading a preset whose imported source is missing degrades to the library fallback with a visible notice, never silence or a crash | nice | pending | stage-4 |
| QUAL-04 | Listening pass on the exported built-in library tables | nice | pending | stage-4 |

## Acceptance Criteria Details

### FUNC-01: Geometry source per oscillator

**Description:** Each oscillator picks Mesh / Volume / Terrain; changing any bake parameter regenerates the table; Position morphs through it.

**Acceptance Criteria:**
- [ ] Switching Source Type on Osc A produces a different table within one debounce interval, audible and visible in the stacked-frame view
- [ ] Position 0.0→1.0 sweeps frame 0→N−1 with linear frame interpolation, as O-Prism
- [ ] Osc A and Osc B bake independently and can hold different source types

### FUNC-02: Mesh slice sweep

**Acceptance Criteria:**
- [ ] The built-in torus bakes 2 loops per slice and selects the largest without frame-to-frame loop flipping
- [ ] Slice Tilt X = 45° on the twisted star produces a table distinct from 0° (spectral centroid differs by > 10%)
- [ ] φ = 0° and φ = 90° on the crescent produce the two distinct Lissajous projections; the sphere at any φ bakes a pure sine (THD < −60 dB)

### FUNC-03: Volume orbit sweep

**Acceptance Criteria:**
- [ ] Torus-knot (2,3) through the Torus SDF bakes without polarity flips between frames (max adjacent-frame RMS ≤ 0.05)
- [ ] Field Detail raised from 0.3 to 0.8 on fBm Noise increases partials above −40 dB

### FUNC-04: Terrain orbit sweep

**Acceptance Criteria:**
- [ ] Ellipse over Sine Product with Sweep Axis = Radius gives monotonically increasing spectral centroid across frames
- [ ] Orbit never leaves [−1,1]² at any Sweep Range / Centre / Aspect combination (assert in the bake)

### FUNC-05: Built-in libraries

**Acceptance Criteria:**
- [ ] ≥6 meshes (sphere, torus, twisted 5-star, crescent, torus knot, fBm blob) and ≥4 analytic terrains ship as binary data
- [ ] Every library entry bakes at 256 frames with no NaN and no frame exceeding 0 dBFS

### FUNC-06 / FUNC-07: Import

**Acceptance Criteria:**
- [ ] OBJ, STL (ASCII + binary) and PNG load via file chooser and via drag-and-drop on macOS and Windows
- [ ] Non-manifold or open meshes bake with a warning rather than a crash; a 1 M-triangle OBJ bakes in ≤ 2 s
- [ ] Parsing never runs on the audio thread (thread-name assert in the parser)

### FUNC-08: Persistence

**Acceptance Criteria:**
- [ ] Save/reload a preset with an imported OBJ under the cap: identical table (SHA-256 of the baked frames matches)
- [ ] Above the cap: state stores path + SHA; reload with the file present regenerates; with the file missing shows the fallback notice (QUAL-03)

### FUNC-09 / FUNC-10: Inherited sections

**Acceptance Criteria:**
- [ ] Parameter IDs for envelope, filter, LFO, mod matrix, FX, tuning and global sections match O-Prism v1.24.0
- [ ] The tuning tab loads a Scala file and a 31-EDO generator; pitch verified against O-Prism on the same tuning

### DSP-01: Mipmap pipeline unchanged

**Acceptance Criteria:**
- [ ] The oscillator source files diff-clean against O-Prism v1.24.0 except for the table-source plumbing
- [ ] A baked table at C6 measures ≤ −90 dB non-harmonic energy (QUAL-01)

### DSP-02 through DSP-05: Slicer maths

**Acceptance Criteria:**
- [ ] C++ slicer output for the twisted star matches the Python prototype's `twisted_star_d_128` within 1e−3 RMS per frame
- [ ] Centroid Distance on the sphere falls back to XY projection (flatness floor) instead of amplifying facet noise
- [ ] Disabling alignment in a debug build raises adjacent-frame RMS on the torus from ≤ 0.034 to ≥ 0.4 (negative control)
- [ ] Per-frame peak normalisation is not reachable from any parameter

### DSP-06: Terrain safety

**Acceptance Criteria:**
- [ ] A 512² PNG with hard edges and Edge Mode = Mirror bakes with no discontinuity at the orbit's edge crossing (no partial above −40 dB introduced by the edge)

### UI-01 through UI-05: 3D view

**Acceptance Criteria:**
- [ ] WebGL2 view renders in the Standalone, in Logic (WKWebView) and in a Windows host (WebView2)
- [ ] Forcing `getContext('webgl2')` to null shows the Canvas 2D fallback and the localised placeholder in all three languages
- [ ] Context loss (simulated via `WEBGL_lose_context`) recovers without a stuck view
- [ ] Dragging the terrain view moves Orbit Centre X/Y through the APVTS slider relay with drag-start/drag-end notifications
- [ ] After a preset apply the view shows the new geometry within one push interval (UI-05)

### PERF-01: Real-time safety

**Acceptance Criteria:**
- [ ] pluginval strictness 10 passes; no allocation in processBlock under a debug allocator
- [ ] Bake-parameter automation at audio rate never stalls or glitches the audio thread (bake is debounced and off-thread)

### PERF-02: Bake budget

**Acceptance Criteria:**
- [ ] 256-frame bake of the 40 k-triangle fBm blob ≤ 100 ms on Apple M-series; a 1 M-triangle OBJ ≤ 2 s
- [ ] Audio plays the previous table throughout; no dropouts during a bake at 16 voices

### COMPAT-01 / COMPAT-02: Validation

**Acceptance Criteria:**
- [ ] pluginval strictness 10 VST3 + AU pass; auval pass
- [ ] CI Windows build green; WebView2 static linking and `withUserDataFolder()` in place

### QUAL-01 / QUAL-02: Artifacts

**Acceptance Criteria:**
- [ ] Offline render harness sweeps every library entry at C2, C4, C6: non-harmonic energy ≤ −90 dB, no NaN
- [ ] Position sweep 0→1 over 2 s at C4 on every library entry: no click (no sample-to-sample step > 0.1 in the rendered output)

---

## Traceability

| Stage | Requirements Verified |
|-------|----------------------|
| stage-1 | COMPAT-01 |
| stage-2 | FUNC-01..05, FUNC-09, FUNC-10, FUNC-12, DSP-*, PERF-01, PERF-02, QUAL-01, QUAL-02 |
| stage-3 | FUNC-06, FUNC-07, UI-*, PERF-03 |
| stage-4 | FUNC-08, FUNC-11, COMPAT-02, QUAL-03, QUAL-04, all remaining |

## Out of Scope (v1.0)

| Feature | Reason | Future Version |
|---------|--------|----------------|
| Live wave-terrain oscillator (2× per-osc oversampling, Chebyshev bandlimited mode, live PNG terrain, trajectory feedback) | New audio-thread path with its own CPU gate; research roadmap orders it after baking | v1.1 |
| RGB-channel terrain morphing | Extension of PNG import once stable | v1.1 |
| Scanned synthesis (mass-spring mesh) | Stability + block-rate simulation; fits the bake pipeline later | v1.2+ |
| ADAA for terrains | Mathematically unsound for arbitrary composite paths | none |
| In-plugin mesh editing | Outside the instrument's remit | none |
| three.js / lit mesh rendering | No bundler; ESM-only library | v1.x |

---
*Generated from BRIEF.md on 2026-09-07*
*Schema: .planning/workflow/schemas/plugin-requirements.schema.json*
