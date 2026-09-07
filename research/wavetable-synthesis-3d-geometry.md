---
title: Wavetable Synthesis with 3D Geometries
date: 2026-09-07
level: 2 (moderate investigation — local KB + primary sources + open-source code)
confidence: MEDIUM-HIGH (established paradigms, anti-aliasing) / MEDIUM (mesh & volume pipelines — no prior art found)
tags:
  - wavetable
  - wave-terrain
  - scanned-synthesis
  - anti-aliasing
  - webgl
related:
  - research/wavetable-synthesis-comprehensive.md
  - research/wavetable-synthesis-o-prism.md
  - plugins/O-Prism/Source/dsp/WavetableGenerator.h
  - plugins/O-TextureForge (WebGL-in-WebView precedent)
---

# Wavetable Synthesis with 3D Geometries

**Question.** How can 3D geometry — heightmaps, parametric surfaces, meshes, orbits through a volume — drive or generate wavetables in a JUCE plugin, and what does that cost in aliasing, real-time safety, and UI?

**Short answer.** There are three distinct paradigms, and they differ far more in *where the geometry is evaluated* than in what geometry is used:

| Paradigm | Geometry | Scan | Prior art | Aliasing story |
|---|---|---|---|---|
| **Wave terrain** | heightmap `z = f(x,y)` (analytic or image) | 2D orbit at pitch rate, per sample | Mitsuhashi 1982 → Anderson's *Terrain* 2024, Conductive Labs hardware 2025, Carbon Electra 2 (May 2026) | not bandlimited → oversample 2× (8× worst case) |
| **Scanned synthesis** | dynamic mass-spring mesh | closed path over mesh at pitch rate; mesh evolves at 0–15 Hz | Verplank/Mathews/Shaw 1999, Csound `scanu/scans`, Wablet (DAFx12) | same as above; plus stability |
| **Baked geometry → wavetable** | any of the above, or mesh slices / SDF volumes | offline: orbit or slice → 2048-sample frame → mipmaps | none for meshes/volumes (open feature request on *Terrain*, issue #5) | **exact**, via the existing O-Prism FFT mipmap pipeline |

**Recommendation.** Build the *baked* path first on top of O-Prism's `WavetableData` (2048 × 256 frames × 10 mipmap levels): geometry becomes a *wavetable generator*, the morph axis becomes "orbit radius" or "slice position", and anti-aliasing is inherited for free. Add a *live* terrain oscillator (per-sample scan, audio-rate terrain modulation, 2× oversampling) as a second mode only if the sound design demands it. The mesh-slicing / SDF-volume generator is the genuinely novel piece and the product differentiator.

---

## 1. Paradigm A — Wave terrain (heightmap scanning)

### 1.1 Definition

Output sample `y[n] = f(x[n], y[n])` where `(x[n], y[n])` is a periodic orbit at the note frequency and `f` is the terrain. Mitsuhashi (1982) introduced it; Borgonovo & Haus (1984/86) established experimental criteria; Mills & de Souza (1999) added complex orbits, windowing corrections and stereo via displaced orbit pairs; Stuart James's 2005 ECU thesis built a polyphonic Max/Jitter instrument and showed the technique subsumes wavetable, waveshaping, FM/AM, scanned and additive synthesis as special cases.

### 1.2 Terrain representations

**Analytic.** Aaron Anderson's open-source *Terrain* (JUCE, GPL-3.0, VST3/AU/CLAP) evaluates one of nine closed-form surfaces per sample with four smoothed modulation inputs (`Source/DSP/Terrain.h`), then `tanh`-saturates the result. Examples from the code:

```
sin(6x·(a+0.5)) · sin(6y·(b+0.5))                 // sine product
cos(‖p‖·2π·(5a+1) + 2πb)                            // radial rings
sin((a'x)² + (b'y)²)                                // "system 12"
cos(a'' · sin(√((x+b')² + (y+c')²)))                // "system 14"
```

**Sampled.** A 2D float table read with bilinear (or bicubic) interpolation. Conductive Labs' hardware *Terrain Synth* imports JPG/PNG (grayscale, or morph between R/G/B channels as three terrains) and WAV wavetables as 2D lookup tables; Carbon Electra 2 imports PNG terrains; Pyo's example uses a 512×512 matrix via `MatrixPointer`; the Max package `tmhglnd/wave-terrain-synthesis` converts `jit.gl.bfg` noise into a float32 matrix.

**Mitsuhashi's constraints for wrapped tables** (via Mills & de Souza): the function and its first partial derivatives should be continuous over the domain, and boundary values should be zero (or periodic) with matching partials — otherwise orbits that cross the table edge produce discontinuities, which is the dominant aliasing source for image-derived terrains. **Implication:** window or mirror image edges, or keep orbits strictly inside `[-1, 1]²`.

### 1.3 Orbits (trajectories)

*Terrain* ships ~20 parametric closed curves in `Source/DSP/Trajectory.h`: ellipse, superellipse (`|cos/a|^n + |sin/b|^n`)^(-1/n), limaçon, butterfly, "squarcle" (`tanh(k·sin θ), tanh(k·cos θ)`), bicorn, cornoid, epitrochoids 3/5/7, hypocycloids 3/5/7, gear curves 3/5/7. Each has 1–3 mod inputs, plus a **recursive trajectory feedback** loop (previous output displaces the next orbit point) with spatial compression. The Praat-based guide adds axis-independent sine/tri/saw/square scan waveforms with rate, phase, span, offset per axis.

Design levers that map to musically obvious controls:
- **Orbit radius / span** → harmonic richness (larger orbit = more terrain features crossed = brighter)
- **Orbit centre offset** → asymmetry, even harmonics
- **Rotation / phase between X and Y** → PWM-like and phase-distortion-like effects (the ADC25 talk frames sync and phase distortion as path–terrain relationships)
- **Orbit lobes (epitrochoid k)** → formant-like resonances at the k-th harmonic

### 1.4 Anti-aliasing for live scanning

Terrain scanning is a nonlinear sampling process; the output spectrum is not predictable from the orbit fundamental alone. Findings:

- **Oversampling is the industry answer.** Anderson (KVR thread): "16× is pretty overkill … the butterfly trajectory on a fully saturated terrain, with the highest note on the keyboard — 8× was enough to remove any audible aliasing. For nearly all use cases I would recommend 2×." *Terrain* wraps `renderNextBlock` in `juce::dsp::Oversampling` (`MainProcessor.cpp:83-93`). CPU hit at 16× polyphonic is reported as noticeable.
- **ADAA is not a drop-in.** Anderson floated "antiderivative of both the trajectory and terrain function independently." For a 2D terrain along a parametric path that is not mathematically sound in general — ADAA needs the antiderivative of the *composite* `f(x(θ), y(θ))` along the path, which has no closed form for arbitrary terrains. Treat ADAA here as an open research item, not a plan.
- **Chebyshev terrains are exactly bandlimited (design lever, verified by identity).** If the terrain is a polynomial in the Chebyshev basis, `f(x,y) = Σ c_nm T_n(x) T_m(y)`, and the orbit is circular `x = cos θ, y = sin θ`, then `T_n(cos θ) = cos nθ` and `T_m(sin θ) = cos m(θ − π/2)`, so every product term is a finite sum of harmonics ≤ n+m. The output is a trigonometric polynomial with a known maximum harmonic, so you can drop coefficients per pitch exactly like a mipmap level. Ellipses and offsets keep the bound (they are affine in cos/sin). This gives a "bandlimited terrain mode" with zero oversampling — a real differentiator versus *Terrain*.
- **Image terrains alias hardest**: pixel-level detail is broadband. Pre-blur (Gaussian, radius tied to orbit speed) before upload, or generate a mip pyramid of the image and select the level from the orbit's instantaneous speed in table units per sample. The Praat guide enforces a guard: maximum instantaneous trajectory rate below 45% of the sample rate.

### 1.5 Real-time cost

*Terrain*'s per-sample cost per voice is several `sin/cos/pow/sqrt` calls for terrain plus the same for the trajectory, times the oversampling factor. Mitigations already used there: `juce::dsp::FastMathApproximations::tanh`, per-block parameter buffers (`BufferedSmoothParameter`) so modulation is read by index, no allocation in the audio path. For an image terrain the per-sample cost drops to one bilinear fetch plus the orbit evaluation.

## 2. Paradigm B — Scanned synthesis (dynamic mesh)

A mass-spring-damper network (Csound: mass M, damping D, centering C, tension T, all variable per node; connectivity via GEN23/GEN44 matrices) is simulated at haptic rates (0–15 Hz) and its displacement is read along a closed scan path at the note frequency. The Wablet (Tubb, Klapuri, Dixon, DAFx12) runs a 70×70 "square cross mesh" on a tablet with an optional averaging filter for smoothness, user-drawn scan paths, strike/grab/damp gestures, and lets masses move in two dimensions.

**Fit for a plugin:** the mesh update is cheap at block rate (4,900 nodes × a few flops per block), the scan is a wavetable read, and the "geometry" animates itself — an evolving-pad engine with no LFOs. Csound's manual warns the system "can blow up" for some mass/centering/damping values; clamp displacement and add a leak term. Aliasing is the same story as Paradigm A because the scanned shape is arbitrary; but because the mesh is *a table*, it can be scanned at block rate into a frame and pushed through the mipmap pipeline (Paradigm C) instead of read per sample.

## 3. Paradigm C — Baked geometry → wavetable (recommended first)

### 3.1 Why bake

O-Prism already owns the hard part: `WavetableData` (2048 samples + guard, up to 256 frames, 10 mipmap levels, flat `[level][frame][sample]`), `WavetableGenerator::generateMipmaps()` (FFT truncation per level), `WavetableImporter`, and the JS wavetable editor. A geometry generator that emits frames into that structure gets:

- Exact anti-aliasing (FFT truncation), no oversampling
- Zero new audio-thread code paths (the oscillator is unchanged)
- Frame morphing as the natural "move through the geometry" control
- Preset/serialisation for free (store the generator parameters, regenerate on load)

What it loses: audio-rate terrain modulation and the per-note orbit feedback loop. Those are the *Terrain* selling points; they are also the aliasing and CPU problems.

### 3.2 Generators

**Orbit sweep (terrain or mesh).** Frame `k` = one full orbit at parameter `p_k` (radius, rotation, offset, lobe count). 64–256 frames. Position knob = which orbit. This reproduces most of what *Terrain* does at control rate, alias-free.

**Slice sweep (true 3D mesh).** For a closed triangle mesh, intersect a plane at height `h_k` with the mesh: for each triangle, edge–plane intersection yields segments; chain segments into closed loops (standard contour extraction, as used in slicers for 3D printing). Convert the loop to a single cycle:
- *Radial unwrap* (star-shaped contours): `r(θ)` sampled at 2048 angles around the centroid → waveform. Fails on non-star-shaped loops (multi-valued `r`).
- *Arc-length unwrap* (general): parameterise the loop by normalised arc length `s ∈ [0,1)`, emit `x(s)` or `y(s)` or the signed distance from the centroid. Always single-valued; the waveform is a projection of the contour, like a Lissajous shadow.
- *Multiple loops per slice* (e.g., torus): sum or concatenate; choose one policy and expose it.
Sweep `h` bottom → top over 64–256 frames. Rotating the slicing plane gives a second morph axis. No prior art found for this — *Terrain* issue #5 ("FR: Ability to load 3D models") is open with no implementation, and Surge issue #4539 is the analogous request there.

**Orbit through a volume.** `f(x,y,z)` from 3D noise (Perlin/simplex — *Terrain* already depends on a PerlinNoise library) or a signed distance field of a mesh (distance to nearest triangle, sign by winding). A closed 3D orbit (torus knot, Lissajous knot) samples the field; the frame axis is orbit scale or knot phase. SDF sampling is a `tanh`-friendly smooth field, so it bakes cleanly.

### 3.3 Threading and swap

Bake on a background thread (precedent: O-TextureForge computes UMAP on a background thread with a progress indicator). Publish the new `WavetableData` via an atomic pointer swap on the audio thread and retire the old one on the message thread. Two memory patterns apply directly: *source-swap needs a lock if prepareToPlay publishes* and *retired-map reaper must not free on the audio thread*. Mesh loading (OBJ/STL parse) and image decode (`juce::ImageFileFormat`) are message-thread work; never touch them from `processBlock`.

### 3.4 Hybrid

Bake the 256-frame table, then let a *per-voice* LFO or envelope drive the frame position at control rate. For "audio-rate terrain modulation" feel, allow the frame-position parameter to be modulated at block rate with linear frame interpolation (O-Prism does this already). That covers the majority of the *Terrain* sound without its aliasing.

## 4. UI and visualisation (WebView)

- **WebGL in the JUCE WebView is proven in this repo.** O-TextureForge ships `regl-scatterplot` in a bundled `app.bundle.js`, updates at 30 Hz, and has a `placeholder.webglUnavailable` fallback string. Both macOS WKWebView and Windows WebView2 expose WebGL; assets are served through the resource provider with bare paths, so vendoring `three.min.js` (or a small custom regl/WebGL2 renderer) is the same mechanism.
- **What to draw.** Heightmap as a displaced `PlaneGeometry` wireframe (Serum-style 3D table view, which O-Prism's brief already lists), the orbit as a `Line`, the current scan point as a sprite. For meshes: the mesh, the slicing plane, and the extracted contour highlighted. For volumes: the orbit and a low-res isosurface or point cloud.
- **Playhead.** Push `{frameIndex, orbitPhase}` from native at 30 Hz via the existing one-shot state push; note the memory pattern that a one-shot push goes stale on preset load — re-push after every preset apply.
- **Interaction.** Drag on the terrain to move the orbit centre (XY pad), scroll for radius, drag-and-drop PNG/OBJ (macOS drag-drop via `webkitGetAsEntry` pattern in memory).
- **Fallback.** Keep a Canvas 2D heightmap (greyscale image + orbit polyline) for WebGL-unavailable hosts; it is also the cheaper always-on view.
- *Terrain* renders natively with OpenGL + glm; the WebView route avoids JUCE OpenGL, which is the right call on macOS.

## 5. Competitive landscape (Sept 2026)

| Product | Type | Geometry input | Notes |
|---|---|---|---|
| Aaron Anderson *Terrain* (2024, free, GPL-3.0) | live terrain, JUCE | 9 analytic terrains, ~20 curves | oversampling 2–16×, MPE, MTS-ESP, trajectory feedback, OpenGL view. Open FR: load 3D models |
| Conductive Labs *Terrain Synth* (hardware, 2025) | live terrain | JPG/PNG/WAV, RGB channel morph | ADC25 talk by Steven Barile; "clean, anti-aliased single-cycle" pitch |
| Scaler Music *Carbon Electra 2* (May 2026, $99) | terrain + wavetable + VA | PNG upload | 4 oscillators, terrain size/travel controls |
| Tone2 *Icarus* | "3D wavetable" | image → wavetable | 3D = extra morph dimension over a 2D table, not geometry |
| Csound `scanu/scans`, Wablet | scanned synthesis | mesh topology matrices | research / education |

**Gap:** nobody ships mesh slicing, SDF volumes, or a provably bandlimited (Chebyshev) terrain mode. Nobody pairs terrain synthesis with a microtonal engine (O-Prism already has one).

## 6. Risks and open questions

1. **Contour → single cycle is not unique** (radial vs arc-length, loop ordering, start point). Start-point drift between adjacent slices produces phase jumps between frames, which sounds like clicks when morphing. Align each frame's start to the previous frame's by cross-correlation or by a fixed ray from the centroid.
2. **ADAA for terrains is unproven.** Do not plan on it.
3. **Image terrains need edge handling** (Mitsuhashi constraints) or the orbit must stay inside the table.
4. **Live mode CPU** scales with oversampling × voices × transcendental calls; budget before promising audio-rate terrain modulation.
5. **Scanned-mesh instability** — clamp and leak.
6. **Level 3 candidates** if this goes to a plan: (a) the mesh-slicing and SDF resampling maths with a prototype offline in Python against real OBJ files; (b) CPU budget of a live oscillator inside the O-Prism voice at 2× oversampling; (c) three.js inside WKWebView/WebView2 in a plugin window — GPU-process behaviour, resize, and 30 Hz update cost.

## 7. Level 3 findings (parallel investigation, 2026-09-07)

Three subagents ran in parallel. Prototypes and raw outputs are in `research/wavetable-synthesis-3d-geometry-prototypes/` (Python slicer, C++ benchmark harness with outputs, WebGL2 HTML prototype). No project code was touched; Terrain's GPL sources were read only in a scratchpad and are not in the repo.

### 7.1 Mesh slice → single cycle (prototype: `mesh-slice/mesh_slice_wavetable.py`)

Tested on sphere, torus, twisted 5-point star, fBm-displaced sphere and an extruded crescent, 128 slices × 2048 samples.

| Unwrap | Works on | Fails on |
|---|---|---|
| Radial `r(θ)` | star-shaped loops only | crescent 128/128 frames, noisy sphere 16/128 (multi-valued `r`) |
| Arc-length `d(s)` centroid distance | everything | circular slices: `ptp/r_mean ≈ 0.001` → per-frame normalisation amplifies facet noise to 0 dBFS; needs a flatness floor (~0.02) |
| Arc-length `x(s)` / `y(s)` | everything; a circle gives a pure sine | none observed |

**Recommendation:** arc-length resampling (uniform `s`, cumulative chord length) emitting `cos φ·x(s) + sin φ·y(s)` with a projection-angle knob `φ` as a free timbre axis, and `d(s)` as a second mode behind the flatness floor. Independent prior art appeared in DAFx 2026 ("Arbitrary Polygon Oscillator": constant-arc-length traversal of a cutting plane through a polyhedron).

**Multi-loop policy** (torus, 2 loops every slice; noisy sphere 1→3 loops at the poles). Adjacent-frame RMS after alignment: largest-loop 0.017 mean / 0.034 max, sum 0.022 / 0.036, concatenation 0.287 / 0.607. Use **largest-area loop** by default with hysteresis (track the loop nearest the previous frame's centroid), **sum** as an option, never concatenate.

**Alignment.** Raw start-point drift was 5–17 samples between adjacent slices, up to 600 at topology changes. Fixed-ray alignment fails when the geometry twists about the slice axis. FFT cross-correlation with a polarity test (`argmax |corr|`, restore sign) chained frame-to-frame wins everywhere: twisted star `d(s)` 0.054/0.057 → 0.002/0.003, torus `d(s)` 0.455/0.827 → 0.017/0.034, torus-SDF orbit 0.017/1.407 (a pure sign flip) → 0.006/0.010. Cost: one 2048-point FFT pair per frame.

**Normalisation.** Per-frame DC removal is mandatory. Amplitude: global peak across all frames, matching O-Prism's `WavetableImporter.cpp:189-205`. Per-frame peak turns circular-slice noise into full-scale garbage.

**Harmonic content** (partials above −60 dB / −40 dB): twisted star `x(s)` 33–43 / 14–17, `d(s)` 69–88 / 21, radial 121–151 / 22; sphere and torus `x(s)` = 1 (pure sine); torus-SDF via tanh 6–27; fBm noise volume 76–377. Roll-off on the star is roughly −12 dB/oct (polygon corners). Mesh contours are mild sources; brightness comes from the noise volume or a waveshaper stage.

**Cost.** Python: 0.5 ms/frame at 4 k triangles, 1.35 ms at 40 k → a 256-frame bake in 0.2–0.35 s; C++ estimate 10–30 ms for 40 k triangles, ~1 s for a 1 M-triangle OBJ. Volume bake 0.1–0.7 ms/frame. Background-thread friendly at any realistic size.

**C++ carry-overs:** nudge vertices with `|z−h| < ε` off the plane; key intersection points by sorted vertex-pair edge id (each key appears in exactly two segments on a closed manifold, so chaining is an adjacency walk); orient loops CCW by signed area; area-weighted polygon centroid, not vertex mean.

### 7.2 Live terrain oscillator cost (benchmark: `terrain-bench/`, Apple M4 Max, clang -O2)

O-Prism today: 16 voices × 2 `WavetableOscillator`, unison up to 8, double-precision trilinear read with per-sample mip select, and a per-sample voice loop that already runs 4 LFOs, the mod matrix, 4 SVFs and several `pow`/trig calls. The processor already owns an atomic `WavetableData*` publish with a generation-counted retired-table reaper.

| Kernel (ns / sample / unison partial) | ns |
|---|---|
| O-Prism wavetable read | 14.5 |
| Orbit: ellipse / epitrochoid-7 / butterfly / superellipse (3× pow) | 12.4 / 14.0 / 29.0 / 47.0 |
| Terrain: sine-product / system-14 / 512² bilinear / Chebyshev 16×16 Clenshaw | 27.3 / 37.9 / 29.0 / 56.7 |
| Live osc: epitrochoid-7 + sine-product + fast tanh | 29.0 |
| Live osc worst: butterfly + system-14 | 52.6 |
| 2× halfband polyphase IIR up+down, per base sample | 9.0 |

Percent of one core at 48 kHz, oversampling applied to the oscillator only:

| Config | 16 v × 2 osc, unison 1 | unison 4 |
|---|---|---|
| Wavetable today, 1× | 2.2% | 8.9% |
| Live analytic, 2× | 8.8% | 35% |
| Live image 512², 2× | 11% | 44% |
| Live worst, 2× / 4× | 16% / 32% | 65% / 129% |
| Live Chebyshev 16×16, 1× (no oversampling needed) | 8.5% | 34% |

A table-based sine buys nothing on Apple libm (`sinf` ≈ 5 ns); `pow` is the only expensive primitive.

**Aliasing at C6** (non-harmonic energy relative to the fundamental): butterfly + sine-product −18.5 dB at 1×, **−60 dB at 2×**, −105 at 4×; epitrochoid-7 + sine-product +4 dB at 1×, −18 at 2×, **−66 at 4×**; system-14 with an unbounded spatial-frequency parameter is not fixable by oversampling alone (+16 dB at 1×, −26 at 4×). **The terrain's spatial-frequency parameter must scale down with pitch** (a "terrain mip"), exactly as a wavetable mip drops harmonics.

**Chebyshev claim verified.** Total-degree ≤16 terrain with a circular orbit: max harmonic 16, non-harmonic floor −116 dB at 1×. Ellipse + offset preserves the bound. A full 16×16 tensor bounds at n+m = 30, not 16; truncating to n+m ≤ 22 at C6 gives −113 dB. Generalisation: for any orbit whose x(θ), y(θ) are trig polynomials of degree K (ellipse, epitrochoids, hypocycloids — not superellipse, butterfly or squarcle) the bound is (n+m)max·K; epitrochoid-7 (K = 8) measured max harmonic 105 ≤ 128. So per-pitch coefficient truncation is an exact, zero-oversampling mipmap, and an image terrain can be projected onto the Chebyshev basis offline for an alias-free "bandlimited image" mode.

**Architecture verdict.** Affordable as a second oscillator type. Must-haves:
1. Per-oscillator 2× oversampling (one halfband decimator per oscillator, parameters held over the two sub-samples) with a 4× "HQ" option. Not whole-synth oversampling as Terrain does: O-Prism's voice loop carries filters, LFOs and the mod matrix, and 2× would double all of that for all 16 voices (+10–15% core) and force re-preparing every filter at 2·fs.
2. Pitch-scaled terrain spatial frequency, or Chebyshev mode with per-pitch truncation.
3. No `pow` in the audio path; a `switch` on an enum instead of Terrain's `std::function` array.
4. Unison cap of 4 for the live oscillator, or accept ~35% for unison 8.
5. Interface parity with `WavetableOscillator` (`prepare/setFrequency/setPosition/setUnison/setWarp*/setFMInput/getNextSampleStereo`) so Sync, Bend, FM and Window warps apply unchanged with θ as the phase accumulator; `setPosition` drives orbit size; new per-sample setters for terrain mod and orbit rotation/offset fed from the mod-matrix section of `PrismVoice::renderNextBlock`.
6. Image terrain published through `std::atomic<const TerrainImage*>` and the existing reaper; decode and pre-blur on the message/background thread; clamp the orbit to [−1, 1]² and `isfinite`-guard before the bilinear read.
7. No `BufferedSmoothParameter`-style per-block buffers (Terrain allocates on oversampling change and says so in a comment); `juce::SmoothedValue` per new parameter.

**Licence.** Terrain is GPL-3.0; O-Prism is AGPL-3.0-or-later. Combining is permitted by AGPLv3 §13 but copied code stays GPL-3.0 with its notices, producing a mixed-licence tree. Clean-room re-derivation from the formulas (mathematics is not copyrightable), citing Terrain.

### 7.3 3D view inside the JUCE WebView (prototype: `webgl-3d/terrain-proto.html`)

**Recommendation: hand-rolled WebGL2, Canvas 2D fallback in the same file, no library.** The prototype is 15.9 KB raw / 5.5 KB gzipped with zero dependencies; the heightmap lives in an `R32F` texture so a table update is one `texSubImage2D`. three.js no longer ships a UMD build (ESM only; r180 is 339 KB / 79 KB gz), O-Prism has no bundler (O-TextureForge needed webpack for regl), and `check-i18n.js` requires an authored source for any minified file. Revisit three.js only for lit meshes or OBJ display, served as `three.module.min.js` through `getResource()` and imported from a module script.

Measured in Chromium 152 / ANGLE Metal at DPR 2 (not WKWebView; those numbers are unverified): WebGL2 0.12 ms average per frame, 0.4 ms max; Canvas 2D 0.25 ms average, 0.9 ms max. Both are far inside a 33 ms budget; the loop redraws only when a push dirtied state.

Gotchas, with sources in the list below:
- **Context loss**: handle `webglcontextlost` (`preventDefault`) and `webglcontextrestored`. WebKit: GLSL `flat` interpolation crashes Apple's shader compiler and silently loses the context; a restored context is invisible until relayout. Repo precedent: O-SpectralShaper's Spectrogram component.
- **WebView2 GPU**: rendering runs in a separate GPU process; driver TDR kills it (`ProcessFailed`); users can force `--disable-gpu` system-wide, and JUCE 8.0.14's `WinWebView2` exposes no additional-args API. `getContext('webgl2')` can legitimately return null, so the Canvas fallback is mandatory.
- **30 Hz push**: use `emitEventIfBrowserIsVisible` + `window.__JUCE__.backend.addEventListener` (O-TextureForge pattern), not `evaluateJavascript` as O-Prism's `timerCallback` does today (JUCE issue #1415 crashed WebView2 release builds inside `evaluateJavascript` with relays). Events are dropped when the view is hidden; never chain the next request off a promise.
- **Pointer capture does not work in the JUCE web view**; use O-Prism's document-level move/up pattern from `bindKnob`. `wheel` needs `{passive:false}`.
- **Retina**: backing = `clientWidth × devicePixelRatio`, re-checked per draw.
- **Gates**: no prose in the view JS; the "WebGL unavailable" placeholder is an HTML `data-i18n` node added to O-Prism's `i18n.js` in en/fr/zh-Hans; readouts use declared font stacks only.

**Data transport.** O-Prism today pulls JSON text per frame (`getActiveOscFrame` → `toJsonFloatArray`, 2048 samples at stride 8). Recommend: (a) 30 Hz `{frame, phase, cx, cy, r}` as a sub-100-byte JSON event, change-gated; (b) the 64×64 view table on change (≤3 Hz) as base64 Float32 (~21 KB, one copy via `atob → Uint8Array → Float32Array`); (c) the full 256×2048 table never crosses the bridge as text (4.3 MB JSON): serve a decimated 64×256 view, or fetch `.bin` through the resource provider with a generation counter in the path to defeat WKWebView caching, scheme resolved via `Juce.getBackendResourceAddress`.

**Interaction → APVTS.** Drag unprojects to the y = 0 plane → `Juce.getSliderState('orbitX'|'orbitY')` with `sliderDragStarted / setNormalisedValue / sliderDragEnded`; wheel → `orbitRadius`. Relays are created generically from `PrismParamIds::allSliderIds()`, so new params get a `WebSliderRelay` and attachment automatically.

**Files an implementation would touch:** `PrismParamIds.h`, `PluginEditor.cpp` (resource route, native fn beside `getActiveOscFrame`, event push in `timerCallback`), new `ui/public/js/terrain-view.js`, `ui/public/index.html` (container, placeholder, extend `WavetableDisplay`), `ui/public/js/i18n.js`, `CMakeLists.txt` binary-data entry, `CHANGELOG.md`.

### 7.4 Synthesis and roadmap

Nothing in Level 3 contradicts the Level 2 recommendation; it sharpens it.

1. **Stage 1 — Baked geometry generator** (lowest risk, highest novelty): mesh slicer + SDF/noise volume orbit → frames → existing mipmap pipeline. Arc-length `x/y` projection with φ knob, largest-loop with hysteresis, xcorr alignment, global-peak normalisation, background-thread bake, atomic swap through the existing reaper. Serum-style stacked-frame view reuses `WavetableDisplay`.
2. **Stage 2 — Live terrain oscillator** as a third oscillator type: per-oscillator 2× (HQ 4×), pitch-scaled terrain spatial frequency, Chebyshev "bandlimited" mode at 1×, image terrains via atomic swap, unison cap 4. Clean-room formulas.
3. **Stage 3 — 3D view**: hand-rolled WebGL2 + Canvas 2D fallback, event-based 30 Hz push, base64 Float32 for the view table, drag/wheel → orbit params.

Open items that remain genuinely unverified: WKWebView and WebView2 frame times in a real plugin window (prototype was Chromium only); the exact bake time of the C++ slicer (Python numbers extrapolated); listening tests on the exported WAVs (`twisted_star_d_128.wav`, `torusSDF_knot_128.wav` in the session scratchpad; not committed).

## Sources

- Terrain (Aaron Anderson) — https://github.com/aaronaanderson/Terrain (Terrain.h, Trajectory.h, MainProcessor.cpp, issue #5)
- KVR thread with oversampling/ADAA discussion — https://www.kvraudio.com/forum/viewtopic.php?t=615087&start=45
- ADC 2025 "Implementing Wave Terrain Synthesis" (Steven Barile, Conductive Labs) — https://conference.audio.dev/session/2025/implementing-wave-terrain-synthesis/
- Conductive Labs Terrain Synth — https://conductivelabs.com/terrainsynth/ ; https://www.gearnews.com/conductive-labs-terrain-synth/
- Carbon Electra 2 — https://synthanatomy.com/2026/05/scaler-music-carbon-electra-2-a-musical-synth-plugin-with-wave-terrain-and-wavetables.html
- Mills & de Souza, "Gestural Sounds by Means of Wave Terrain Synthesis" (SBCM 1999) — https://compmus.ime.usp.br/sbcm/1999/papers/Anderson_Mills.pdf
- Stuart James, ECU theses (2005) — https://ro.ecu.edu.au/theses/107/ ; https://ro.ecu.edu.au/theses_ebooks/4/
- Wave Terrain Synthesis guide (terrain/trajectory taxonomy, rate guard) — https://mashav.com/sha/praat/scripts/Wave_Terrain_Synthesis.html
- tmhglnd/wave-terrain-synthesis (Max, polar/cartesian, poly~ upsampling) — https://github.com/tmhglnd/wave-terrain-synthesis
- Pyo wave terrain example — https://belangeo.github.io/pyo/examples/x15-matrix/01-wave-terrain-synthesis.html
- Scanned synthesis: Csound manual — https://csound.com/manual/siggen/scantop/ ; Wablet (DAFx12) — https://dafx12.york.ac.uk/papers/dafx12_submission_18.pdf ; Wikipedia — https://en.wikipedia.org/wiki/Scanned_synthesis
- Chebyshev waveshaping bandlimit identity — http://msp.ucsd.edu/techniques/v0.08/book-html/node80.html
- ADAA background (Parker, Zavalishin, Le Bivic, DAFx16) — http://dafx16.vutbr.cz/dafxpapers/20-DAFx-16_paper_41-PN.pdf
- Tone2 Icarus "3D wavetable" — https://www.gearnews.com/tone2-releases-icarus-long-awaited-3d-wavetable-synthesizer/
- Contour Synthesizer (shape contour as waveform, NIME 2021) — https://github.com/yonatanrozin/the-contour-synthesizer
- JUCE forum wavetable aliasing thread — https://forum.juce.com/t/wavetable-synthesis-producing-artifacts-at-high-frequencies-how-to-fix-this-aliasing/34824
- Level 3 — mesh slicing: Minetto et al., "An optimal algorithm for 3D triangle mesh slicing" (CAD 2017) — https://www.inf.ufpr.br/murilo/public/CAD-slicing.pdf ; Livesu et al., "From 3D Models to 3D Prints" — https://arxiv.org/pdf/1705.03811 ; "Triangle Mesh Slicing and Contour Construction" — https://arxiv.org/pdf/1910.04037 ; Argentieri & Scagliola, "Arbitrary Polygon Oscillator" (DAFx 2026) — https://arxiv.org/html/2608.24726 ; "On canonical parameterizations of 2D shapes" — https://arxiv.org/pdf/2303.15205
- Level 3 — WebView/WebGL: Khronos "Handling Context Lost" — https://wikis.khronos.org/webgl/HandlingContextLost ; WebKit bugs 286648 / 286297 (`flat` crashes shader compiler), 261331 (context loss on background) — https://bugs.webkit.org/show_bug.cgi?id=286648 ; WebView2 process events — https://learn.microsoft.com/en-us/microsoft-edge/webview2/concepts/process-related-events ; WebView2 performance guidance — https://learn.microsoft.com/en-us/microsoft-edge/webview2/concepts/performance ; WebView2Feedback #2421 (driver corruption) — https://github.com/MicrosoftEdge/WebView2Feedback/issues/2421 ; JUCE issue #1415 (`evaluateJavascript` crash with relays) — https://github.com/juce-framework/JUCE/issues/1415 ; JUCE forum 61472 (WebView flash) — https://forum.juce.com/t/bug-juce-8-webview-and-pagefinishedloading/61472 ; JUCE forum 61156 (pointer capture) — https://forum.juce.com/t/webview-vs-juce-graphics/61156 ; three.js WebGPU code-split PR #29404 — https://github.com/mrdoob/three.js/pull/29404
- Local: research/wavetable-synthesis-comprehensive.md, research/wavetable-synthesis-o-prism.md, plugins/O-Prism/Source/dsp/WavetableData.h, plugins/O-TextureForge/.planning/BRIEF.md
