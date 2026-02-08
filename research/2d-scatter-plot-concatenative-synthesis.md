---
title: "2D Scatter Plot Visualization for Concatenative Synthesis"
created: 2026-02-07
last_verified: 2026-02-07
juce_version: "8.0.4"
summary: "Deep research on building a real-time 2D scatter plot in a WebView-based JUCE plugin for concatenative synthesis corpus exploration, including CataRT/FluCoMa analysis, rendering technology comparison, JUCE-WebView communication patterns, dimensionality reduction, and differentiation analysis."
domain: ui
type: research
keywords:
  - scatter-plot
  - concatenative-synthesis
  - webgl
  - webview
  - umap
  - pca
  - corpus-exploration
  - catart
  - flucoma
  - regl-scatterplot
  - real-time-visualization
  - juce
stages: [0, 1]
agents: [research, ui]
---

# 2D Scatter Plot Visualization for Concatenative Synthesis in JUCE WebView

**Researched:** 2026-02-07
**Domain:** Real-Time Audio Visualization, Concatenative Synthesis, WebGL, JUCE WebView
**Confidence:** HIGH (cross-referenced across IRCAM CataRT docs, FluCoMa docs, WebGL benchmarks, JUCE source code, and published plugins)

---

## Executive Summary

Building a real-time 2D scatter plot for corpus exploration in a WebView-based JUCE plugin is technically feasible and would be a strong differentiator. No existing VST/AU concatenative synthesis plugin offers an interactive, GPU-accelerated 2D descriptor space visualization with real-time playback cursor tracking. The recommended stack is **regl-scatterplot** (WebGL) for rendering, **UMAP via umappp** (C++ header-only) for dimensionality reduction, and **JUCE 8's emitEventIfBrowserIsVisible()** at 30Hz for real-time cursor updates. Canvas2D is sufficient for corpora under ~5,000 points; WebGL is required for 10,000-50,000+ points at 60fps.

---

## 1. How CataRT and FluCoMa 2D Scatter Plots Work

### 1.1 IRCAM CataRT -- The Canonical Implementation

CataRT (2005-present) is the foundational concatenative synthesis system. It plays grains from a large corpus of segmented, descriptor-analyzed sounds according to proximity to a target position in a 2D descriptor space.

**2D Interface Mechanics:**
- Each grain/segment is a **point** in a 2D scatter plot
- **X axis** and **Y axis** each map to a user-selectable audio descriptor (e.g., SpectralCentroid, Loudness, Periodicity, Pitch, Duration)
- **Color** can map to a third descriptor dimension
- **Opacity** can map to a fourth descriptor (e.g., FrequencyMean -- higher values = more opaque)
- **Size** maps to segment duration (longer = bigger point)
- The user moves a **cursor** (orange circle) through the 2D space
- The **nearest grain** to the cursor is selected and played
- A **selection radius** can be dragged wider (drag right/left) for random selection within radius

**Trigger Modes:**
- **Bow (W):** Triggers closest unit each time the mouse moves
- **Fence (F):** Plays a unit when a *different* unit becomes closest (like clattering a stick along a fence)
- **Beat (B):** Triggers via metronome (speed = grain rate + random)
- **Chain (L):** Triggers next unit when previous finishes (loop mode)
- **Quant (Q):** Quantized metronome (MIDI only)

**Interaction:**
- Click to freeze cursor position; double-click to unfreeze
- Zoom in/out with `=`/`-` keys or by changing min/max of descriptor range
- Multi-touch support (each finger = independent cursor in bow/fence mode)
- OSC control: `/catart/select <x> <y>` with coordinates 0-1

**Implementation:** Built in Max/MSP using MuBu (multi-buffer) library with PiPo framework for descriptor analysis. The 2D display is rendered via Max's `lcd` or `imubu` object -- software rendered, not GPU accelerated.

**Sources:**
- [IRCAM CataRT](https://ismm.ircam.fr/catart/)
- [CataRT Documentation](https://ircam-ismm.github.io/max-msp/catart.html)
- [CataRT-MuBu GitHub](https://github.com/ircam-ismm/catart-mubu)

### 1.2 FluCoMa 2D Corpus Explorer

FluCoMa (Fluid Corpus Manipulation) provides a modern toolkit for corpus-based music making. Their 2D Corpus Explorer is a patch-based approach (Max, SuperCollider, Pure Data).

**Pipeline:**
1. **Segment** audio corpus into slices
2. **Analyze** each slice with machine listening (Loudness, SpectralShape, MFCC, Pitch, etc.)
3. **Reduce dimensions** using `fluid.umap~` or `fluid.pca~` to get 2D coordinates
4. **Plot** using `fluid.plotter` (or `fluid.datasetplot~`) -- displays points in a 2D space
5. **Navigate** with mouse -- nearest point = nearest sound = trigger playback

**UMAP Parameters in FluCoMa:**
- `mindist` (0.0-3.0): Small values pack points tightly, preserving local structure
- `numneighbours`: Balances global vs. local structure preservation (relative to dataset size)
- `iterations`: More iterations = better structure discernment

**Key Design Principle:** The visualization is not just decorative -- it IS the instrument interface. The spatial layout creates an explorable "sound map" where proximity = perceptual similarity.

**Sources:**
- [FluCoMa 2D Corpus Explorer](https://learn.flucoma.org/learn/2d-corpus-explorer/)
- [FluCoMa UMAP Reference](https://learn.flucoma.org/reference/umap/)
- [FluidCorpusMap GitHub](https://github.com/flucoma/FluidCorpusMap)
- [FluCoMa Video Tutorial Series](https://discourse.flucoma.org/t/creating-a-2d-corpus-explorer-video-tutorial-series/1157)

### 1.3 What Makes These Visualizations Effective

| Feature | CataRT | FluCoMa | Why It Matters |
|---------|--------|---------|----------------|
| User-selectable axes | Yes (dropdown) | Yes (via analysis chain) | Lets user explore different "views" of corpus |
| Real-time cursor | Orange circle | Mouse position | Shows "where you are" in timbre space |
| Nearest-neighbor highlight | Currently playing grain | Closest slice plays | Direct sonic feedback of spatial position |
| Zoom/pan | Yes (`=`/`-` keys) | Yes (plotter controls) | Navigate large corpora without losing detail |
| Multi-dimensional encoding | X, Y, color, opacity, size | X, Y (post-reduction) | Convey 3-5 descriptors simultaneously |
| Selection radius | Drag to widen | N/A (single nearest) | Adds controlled randomness |

---

## 2. WebView Rendering Technologies for Real-Time Scatter Plots

### 2.1 Technology Comparison

| Technology | Max Points at 60fps | Startup Time | GPU Accel | Interaction Support | Complexity |
|------------|---------------------|--------------|-----------|---------------------|------------|
| **SVG** | ~1,000 | Fast | No | Excellent (DOM events) | Low |
| **Canvas 2D** | ~5,000-8,000 | ~15ms | No (CPU) | Manual hit testing | Medium |
| **WebGL (raw)** | 500,000+ | ~40ms | Yes | Manual hit testing | High |
| **WebGL (regl)** | 20,000,000 | ~40ms | Yes | Built-in lasso/click | Medium |
| **WebGL (Three.js)** | 100,000+ | ~100ms | Yes | Raycasting | Medium-High |

**Key benchmark finding:** On a 50,000-point scatter plot, Canvas2D dropped to 22 FPS while WebGL maintained 58 FPS. SVG and Canvas fall below 30 FPS past 10,000 elements. WebGL holds steady thanks to instanced rendering and uniform buffers.

**Canvas 2D characteristics:**
- Matrix transformations bound to CPU -- performance degrades linearly with point count
- Initial load is fast (~15ms vs ~40ms for WebGL)
- `fillRect` is faster than `arc` for drawing points
- Batching by fill color avoids repeated state machine changes

**WebGL characteristics:**
- GPU-parallel: each point rendered independently
- Point data stored in GPU buffers -- pan/zoom is a matrix multiply, no re-upload
- WebGL advantage only appears past a data-volume threshold (~5,000 points) due to CPU-to-GPU marshaling overhead
- For scatter plots: use `gl.POINTS` with `gl_PointSize` in vertex shader for maximum throughput

**Sources:**
- [Canvas vs WebGL Comparison](https://2dgraphs.netlify.app/)
- [2D vs WebGL Canvas Performance](https://semisignal.com/a-look-at-2d-vs-webgl-canvas-performance/)
- [Canvas vs WebGL for Chart Performance](https://digitaladblog.com/2025/05/21/comparing-canvas-vs-webgl-for-javascript-chart-performance/)

### 2.2 Library Recommendations

**Tier 1 (Recommended): regl-scatterplot**
- Purpose-built for exactly this use case
- Renders up to **20 million points** at interactive frame rates
- Built-in: pan, zoom, rotate, lasso selection, click selection, point hover
- Per-point encoding: color (categorical/continuous), size, opacity via `[x, y, valueA, valueB]` format
- Animated transitions with `draw(newPoints, { transition: true })`
- Small footprint: only depends on `regl` and `pub-sub-es`
- RGBA texture encoding: x/y -> red/green, data values -> blue/alpha
- MIT licensed
- [GitHub](https://github.com/flekschas/regl-scatterplot) | [Demo](https://flekschas.github.io/regl-scatterplot/) | [npm](https://www.npmjs.com/package/regl-scatterplot)

**Tier 2: scatter-gl (Google PAIR)**
- WebGL-accelerated 2D/3D scatter plot from TensorFlow Embedding Projector
- Handles tens of thousands of points
- Three.js-based (heavier dependency)
- Built-in orbit controls, onClick callbacks
- [GitHub](https://github.com/PAIR-code/scatter-gl)

**Tier 3: Raw regl or raw WebGL**
- Maximum control, minimal overhead
- Write custom vertex/fragment shaders for point rendering
- Must implement all interaction (pan, zoom, hit testing) from scratch
- Best if you need custom visual effects (glow on active grains, trails, etc.)

**Not recommended for this use case:**
- **D3.js**: SVG-based; too slow for 10,000+ points
- **deck.gl**: Overkill (geo-spatial focus); heavy bundle size
- **PixiJS**: General-purpose 2D engine; not scatter-plot-optimized
- **Three.js directly**: 3D engine overhead unnecessary for 2D scatter

### 2.3 Recommended Approach: Tiered Rendering

```
Points < 5,000:   Canvas 2D (simpler code, fast startup)
Points 5,000-50,000: regl-scatterplot (WebGL, built-in interactions)
Points > 50,000:  regl-scatterplot or custom regl shaders
```

For a concatenative synthesis plugin with typical corpora of 1,000-50,000 grains, **regl-scatterplot** covers the full range with room to spare.

---

## 3. JUCE C++ to WebView Communication for Real-Time Updates

### 3.1 Architecture Overview

```
Audio Thread (processBlock)
    |
    | lock-free FIFO / atomic writes
    v
GUI Thread (timerCallback @ 30Hz)
    |
    | emitEventIfBrowserIsVisible("eventName", jsonString)
    v
WebView JavaScript
    |
    | window.__JUCE__.backend.addEventListener("eventName", callback)
    v
regl-scatterplot.draw() / canvas update
```

### 3.2 Existing Pattern in This Project (O-GrainScatter)

The O-GrainScatter plugin already implements this exact pattern at 30Hz:

```cpp
// In PluginEditor.cpp -- timerCallback at 30Hz
startTimerHz(30);

void GrainScatterEditor::timerCallback()
{
    const auto& snap = audioProcessor.getVizSnapshot();
    juce::String grainJson = "{\"g\":[";
    // ... build JSON with position, pitch, pan, envelope, reverse, frozen per voice
    webView->emitEventIfBrowserIsVisible("grainUpdate", grainJson);
}
```

This sends up to 64 grain states per frame as a JSON string. For a scatter plot, the same pattern applies but with different data.

### 3.3 Data Flows for Scatter Plot

**Flow 1: Corpus Loaded (C++ -> JS, one-time, large)**
```
Trigger: User loads audio file / corpus analysis completes
Data: Array of {x, y, color, size, sourceFile, startSample, duration} for all grains
Size: ~50,000 points x ~7 values = ~350,000 numbers = ~2-4MB JSON
Method: withNativeFunction() callback returning full dataset
Frequency: Once per corpus load (not real-time)
```

**Flow 2: Playback Cursor Position (C++ -> JS, real-time)**
```
Trigger: Timer callback
Data: {cx, cy, activeGrainIndices[], radius}
Size: ~100 bytes
Method: emitEventIfBrowserIsVisible("cursorUpdate", json)
Frequency: 30Hz (matches existing pattern)
```

**Flow 3: Active Grain Highlights (C++ -> JS, real-time)**
```
Trigger: Timer callback
Data: Array of currently-playing grain indices + envelopes
Size: ~500 bytes (up to ~64 active grains)
Method: emitEventIfBrowserIsVisible("grainActivity", json)
Frequency: 30Hz
```

**Flow 4: User Click/Selection (JS -> C++, event-driven)**
```
Trigger: User clicks/drags on scatter plot
Data: {x, y} in descriptor space, or grainIndex
Size: ~50 bytes
Method: withNativeFunction("selectPoint", callback)
Frequency: Event-driven (only on user action)
```

### 3.4 Message Passing Latency

**macOS (WKWebView):**
- Round-trip latency: ~3ms on modern hardware (WKWebView uses IPC due to process isolation)
- One-way (C++ -> JS via evaluateJavascript/emitEvent): ~1-2ms
- Acceptable for 30Hz visual updates (33ms budget per frame)

**Windows (WebView2):**
- Uses PostMessage API with JSON serialization
- Microsoft is actively optimizing this (Delayed Message Timing API in 2025/2026)
- Slightly higher overhead than macOS due to COM marshaling
- SharedBuffer API available for large binary transfers

**Critical warning:** Never call `emitEventIfBrowserIsVisible` or `evaluateJavascript` before the first page has fully loaded -- this freezes the DAW UI thread.

**Performance guideline:** At 30Hz with ~500 bytes of JSON per message, the overhead is negligible (~15KB/s). Even the corpus-load payload of 2-4MB JSON is a one-shot cost of ~50-100ms parsing time, which is acceptable during a "loading" state.

### 3.5 Push vs. Poll Architecture

**Push (recommended):** C++ timerCallback emits events to JS. This is the established JUCE pattern.
- 3DVerb specifically switched FROM polling TO push after discovering that "frequent polling via fetch calls from frontend were overwhelming DAW UI thread causing program crashes."
- The push model lets the C++ side control update rate and bundle multiple data streams into a single timer tick.

**Sources:**
- [JUCE 8 WebView UIs](https://juce.com/blog/juce-8-feature-overview-webview-uis/)
- [Sound-Field GitHub](https://github.com/mbarzach/Sound-Field)
- [3DVerb GitHub](https://github.com/joe-mccann-dev/3DVerb)
- [JUCE WebBrowserComponent API](https://docs.juce.com/master/classjuce_1_1WebBrowserComponent.html)
- [WKWebView Communication Latency](https://blog.persistent.info/2015/01/wkwebview-communication-latency.html)

---

## 4. UMAP / PCA Dimensionality Reduction

### 4.1 Why Dimensionality Reduction Is Needed

A typical concatenative synthesis descriptor set has 10-20+ dimensions:
- Loudness, Pitch, SpectralCentroid, SpectralSpread, SpectralRolloff, SpectralFlatness
- MFCCs (13 coefficients)
- Periodicity, ZeroCrossingRate, etc.

To plot on a 2D scatter, you must reduce N dimensions to 2. Three options:

### 4.2 PCA vs. UMAP vs. t-SNE for Audio Descriptors

| Property | PCA | UMAP | t-SNE |
|----------|-----|------|-------|
| **Speed (10K points, 19D)** | < 1 second | 2-10 seconds | 1-5 minutes |
| **Local structure** | Poor | Excellent | Excellent |
| **Global structure** | Good | Good | Poor |
| **Interpretability** | High (axes = linear combos) | Low (axes meaningless) | Low |
| **Deterministic** | Yes | No (seed-dependent) | No |
| **Incremental update** | Yes (project new points) | Partial (transform) | No |
| **Best for** | Quick preview, axis labels | Exploration, clustering | Publication figures |

**Recommendation: UMAP as primary, PCA as fallback/fast-preview**

For audio descriptor spaces specifically:
- UMAP preserves the "cluster" structure that matters for perceptual similarity
- PCA's linear assumption misses non-linear relationships between descriptors
- t-SNE is too slow for interactive use and doesn't preserve global structure
- FluCoMa uses UMAP as their primary reduction method

### 4.3 C++ UMAP Implementation: umappp

**umappp** is a header-only C++ port of the UMAP algorithm, derived from the uwot R package.

**Integration:**
```cmake
# CMakeLists.txt
include(FetchContent)
FetchContent_Declare(umappp
    GIT_REPOSITORY https://github.com/libscran/umappp
    GIT_TAG master
)
FetchContent_MakeAvailable(umappp)
target_link_libraries(YourPlugin PRIVATE libscran::umappp)
```

**Usage:**
```cpp
#include "umappp/umappp.hpp"

// Input: column-major, ndim rows x nobs columns
// 19 descriptors, 10000 grains
int ndim = 19, nobs = 10000;
std::vector<double> data(ndim * nobs); // fill with descriptor values

// Output: 2D embedding
int out_dim = 2;
std::vector<double> embedding(out_dim * nobs);

// Configure
umappp::Options opt;
opt.num_neighbors = 15;   // neighborhood size
opt.min_dist = 0.1;       // packing tightness
opt.num_epochs = 500;     // iteration count

// Build neighbor graph (uses VP-tree via knncolle)
knncolle::VpTreeEuclidean<int, double> vp_builder;
auto status = umappp::initialize(ndim, nobs, data.data(), vp_builder, out_dim, embedding.data(), opt);

// Run all epochs
status.run(embedding.data());

// Or run partial epochs for progress feedback:
status.run(embedding.data(), 100); // run 100 epochs
// ... update progress bar ...
status.run(embedding.data());      // run remaining
```

**Dependencies:** knncolle (neighbor search, supports VP-trees, Annoy)

**Performance estimate for 10,000 points x 19 dimensions:**
- Based on published benchmarks: MNIST (70,000 points x 784D) takes ~42 seconds
- UCI Shuttle (43,500 points x 8D) takes ~44 seconds
- Estimated for 10K x 19D: **2-8 seconds** on modern hardware
- PCA equivalent: **< 100ms** (Eigen SVD)

### 4.4 C++ PCA Implementation

PCA is trivial with Eigen (likely already a JUCE dependency via some modules):

```cpp
#include <Eigen/Dense>

// data: ndim x nobs matrix
Eigen::MatrixXd centered = data.colwise() - data.rowwise().mean();
Eigen::JacobiSVD<Eigen::MatrixXd> svd(centered, Eigen::ComputeThinU);
Eigen::MatrixXd projection = svd.matrixU().leftCols(2).transpose() * centered;
// projection is now 2 x nobs
```

For 10,000 points x 19 dimensions, this completes in under 100ms.

### 4.5 Incremental UMAP (Adding New Points)

UMAP supports `transform()` to project new points onto an existing embedding without full recomputation:
- The new point is placed at the weighted average of its neighbors' embedding positions
- Performance scales with test set size, not training set size
- **umappp does NOT currently support transform** -- you would need to either:
  1. Re-run full UMAP when corpus changes (acceptable if < 10 seconds)
  2. Use PCA for incremental additions (instant projection)
  3. Implement nearest-neighbor interpolation yourself (find k-nearest in descriptor space, average their 2D positions)

**Practical recommendation:** Run UMAP once on full corpus load (background thread, 2-8 seconds). For small additions, use PCA projection or nearest-neighbor interpolation. For large corpus changes, re-run UMAP.

**Sources:**
- [umappp GitHub](https://github.com/libscran/umappp)
- [UMAP Performance Benchmarks](https://umap-learn.readthedocs.io/en/latest/benchmarking.html)
- [UMAP Transform Documentation](https://umap-learn.readthedocs.io/en/latest/transform.html)
- [Comparative Audio Analysis with UMAP](https://medium.com/@LeonFedden/comparative-audio-analysis-with-wavenet-mfccs-umap-t-sne-and-pca-cb8237bfce2f)
- [Comparison of Dimensionality Reduction on Audio Signals](https://ceur-ws.org/Vol-2718/paper04.pdf)

---

## 5. Existing Examples of WebView-Based Scatter Plots in Audio Plugins

### 5.1 Audio Plugins with 2D Corpus Visualization

| Plugin | Visualization | Format | Technology | Scatter Plot? |
|--------|--------------|--------|------------|---------------|
| **Mosaic (Echobit)** | 2D corpus view organized by features | VST3/AU/AAX | Proprietary (not WebView) | Partial -- feature-organized grid |
| **AudioTexture (Le Sound)** | Semantic descriptor UI | VST3/AU | Proprietary | No |
| **Concatenator (DataMind Audio)** | Basic waveform | VST3/AU | Proprietary | No |
| **Catecophony** | None (WIP) | VST3/AU | JUCE | No |
| **rhythmCAT** | None | VST | JUCE/Max | No |

**No existing VST/AU plugin offers an interactive WebGL-accelerated 2D scatter plot for corpus exploration.** Mosaic comes closest but uses a grid-based layout rather than a continuous 2D descriptor space.

### 5.2 JUCE Plugins with WebView Real-Time Visualization

| Plugin | Visualization | Framework | Points | Update Rate |
|--------|--------------|-----------|--------|-------------|
| **Sound-Field** | 3D particle field (13K+ points) | React + Three.js + GLSL | 13,000 | Timer-based |
| **3DVerb** | 3D reverb space | Three.js + Three Nebula | ~1,000 | Timer-based (push) |
| **O-GrainScatter** (this project) | 2D grain activity | Vanilla JS + Canvas | 64 | 30Hz |

Sound-Field proves that JUCE 8 WebView + WebGL can handle 13,000+ animated points in real-time. The architecture is directly applicable to a scatter plot.

### 5.3 Web-Based Audio Scatter Plots (Not Plugins)

- **FluCoMa FluidCorpusMap**: SuperCollider plot + web interface for multi-touch exploration ([GitHub](https://github.com/flucoma/FluidCorpusMap))
- **Google's Embedding Projector**: scatter-gl for visualizing audio embeddings (TensorFlow)
- **AudioStellar**: Standalone app for audio corpus exploration with 2D/3D plot (not a plugin)

**Sources:**
- [Sound-Field GitHub](https://github.com/mbarzach/Sound-Field)
- [3DVerb GitHub](https://github.com/joe-mccann-dev/3DVerb)
- [Mosaic by Echobit](https://echobit.myshopify.com/products/mosaic)
- [Catecophony GitHub](https://github.com/ben-hayes/catecophony)

---

## 6. Why This Would Be Differentiating

### 6.1 Limitations of Native JUCE Graphics

JUCE's native `Graphics` class is CPU-bound (software rasterized on all platforms except Windows with Direct2D):
- Drawing 10,000 `fillEllipse` calls per frame at 60fps is prohibitively expensive
- No built-in pan/zoom/lasso infrastructure
- No point-based instancing or batch rendering
- `juce::OpenGLContext` exists but is deprecated for plugin UIs and conflicts with host rendering
- Direct2D (Windows only, JUCE 8) helps but still requires manual path generation on CPU

WebGL in a WebView bypasses all of these limitations: GPU-parallel point rendering, built-in browser compositor, and access to the entire web visualization ecosystem.

### 6.2 Why No One Has Done This in a Plugin Before

1. **WebView in plugins is new:** JUCE 8 (2024) was the first framework to make WebView practical in plugins. Before that, native JUCE graphics or OpenGL were the only options.

2. **Concatenative synthesis in plugins is rare:** Most concatenative tools live in Max/MSP, SuperCollider, or standalone apps (CataRT, FluCoMa, AudioStellar). The few VST implementations (Mosaic, Concatenator) prioritize the synthesis engine over visualization.

3. **Cross-domain expertise required:** Building this requires expertise in audio DSP (grain scheduling, descriptor analysis), machine learning (UMAP), GPU graphics (WebGL shaders), and web development (JavaScript, DOM) -- a rare combination.

4. **Performance skepticism:** Many developers assume WebView is too slow for real-time audio visualization. The evidence from Sound-Field (13K particles) and 3DVerb proves otherwise.

### 6.3 Risks and Challenges

| Risk | Severity | Mitigation |
|------|----------|------------|
| WebView memory overhead (~50-100MB per instance) | Medium | Acceptable for desktop plugin; problematic if user opens 10 instances |
| WebView2 not available on older Windows | Low | JUCE falls back to IE (no WebGL) -- show fallback UI |
| Message passing latency spikes | Low | 30Hz is conservative; visual-only (not audio-critical) |
| UMAP computation blocks UI | Medium | Run on background thread with progress callback |
| Large corpus JSON transfer | Low | One-time cost; chunked transfer if > 5MB |
| WebView process crash | Low | WKWebView crash recovery; save state in C++ |
| Cross-platform WebView differences | Medium | Test on macOS (WKWebView) + Windows (WebView2) + Linux (WebKitGTK) |
| regl-scatterplot bundle size in BinaryData | Low | Library is small; gzip-embed in plugin binary |

### 6.4 Competitive Advantage Summary

**What exists today:**
- CataRT: Max/MSP only, software-rendered, no plugin format
- FluCoMa: Max/SC/PD only, not a plugin
- Mosaic: Plugin but no real scatter plot (grid-based)
- Concatenator: Plugin but no visualization
- AudioTexture: Plugin but no scatter plot

**What this plugin would offer:**
- First VST3/AU with interactive GPU-accelerated 2D descriptor space
- Real-time playback cursor in descriptor space
- UMAP-reduced multi-dimensional corpus visualization
- Click-to-play interaction directly in the scatter plot
- Lasso selection for region-based playback
- Per-point encoding: color = pitch, size = loudness, opacity = duration
- Zoom into clusters of perceptually similar sounds
- Works in any DAW (Logic, Ableton, Reaper, FL Studio, etc.)

---

## 7. Recommended Implementation Architecture

### 7.1 C++ Side

```
PluginProcessor
  |-- CorpusManager
  |     |-- AudioAnalyzer (descriptors: MFCC, loudness, pitch, centroid, etc.)
  |     |-- DescriptorDatabase (stores N x D matrix)
  |     |-- DimensionReducer
  |     |     |-- PCAReducer (Eigen, instant)
  |     |     |-- UMAPReducer (umappp, background thread, 2-8s)
  |     |-- NearestNeighborSearch (KD-tree for real-time query)
  |
  |-- GrainScheduler
  |     |-- Target position in 2D space (from UI interaction)
  |     |-- Nearest-neighbor grain selection
  |     |-- Active grain tracking (indices + envelopes)
  |
  |-- VizDataBridge (lock-free FIFO)
        |-- cursorPosition {x, y}
        |-- activeGrains [{index, envelope}, ...]
        |-- corpusPoints [{x, y, color, size}, ...] (on load)

PluginEditor
  |-- WebBrowserComponent (with resource provider)
  |-- Timer @ 30Hz
  |     |-- Read VizDataBridge
  |     |-- emitEventIfBrowserIsVisible("cursorUpdate", ...)
  |     |-- emitEventIfBrowserIsVisible("grainActivity", ...)
  |
  |-- withNativeFunction("loadCorpus", ...) -> returns all points
  |-- withNativeFunction("selectPoint", ...) -> receives {x, y} from JS
  |-- withNativeFunction("requestReduction", ...) -> triggers UMAP/PCA
```

### 7.2 JavaScript Side

```
index.html
  |-- regl-scatterplot (WebGL scatter plot)
  |-- Event listeners
  |     |-- "cursorUpdate" -> animate cursor overlay
  |     |-- "grainActivity" -> highlight active point indices
  |     |-- "corpusLoaded" -> scatterplot.draw(points)
  |
  |-- Click handler -> call __JUCE__.backend.selectPoint({x, y})
  |-- Lasso handler -> call __JUCE__.backend.selectRegion(indices)
  |
  |-- Cursor overlay (Canvas2D layer on top of WebGL)
  |     |-- Crosshair at current target position
  |     |-- Selection radius circle
  |     |-- Pulsing highlight on active grains
```

### 7.3 Data Format for Corpus Transfer

```json
{
  "points": [
    [0.23, 0.87, 0, 0.5],
    [0.45, 0.12, 1, 0.8],
    ...
  ],
  "meta": {
    "count": 10000,
    "xLabel": "UMAP-1",
    "yLabel": "UMAP-2",
    "colorBy": "pitch",
    "sizeBy": "loudness"
  },
  "descriptors": {
    "names": ["loudness", "pitch", "centroid", "mfcc1", ...],
    "ranges": [[0, 1], [-1, 1], [20, 20000], ...]
  }
}
```

Each point in the `points` array is `[x, y, valueA, valueB]` matching regl-scatterplot's expected format.

### 7.4 Performance Budget

| Operation | Budget | Actual Estimate |
|-----------|--------|-----------------|
| UMAP reduction (10K x 19D) | < 10 seconds | 2-8 seconds |
| Corpus JSON transfer to WebView | < 500ms | ~100ms for 10K points |
| Scatter plot initial render | < 100ms | ~40ms (WebGL init) |
| Per-frame cursor update (30Hz) | < 5ms | ~1-2ms (emit + parse + draw) |
| Point click hit testing | < 16ms | ~1ms (WebGL picking) |
| Pan/zoom interaction | 60fps | 60fps (GPU matrix transform) |

---

## 8. Summary of Key Decisions

| Decision | Recommendation | Rationale |
|----------|---------------|-----------|
| Rendering engine | regl-scatterplot (WebGL) | 20M point capacity, built-in interactions, small footprint |
| Dim. reduction primary | UMAP (umappp, C++) | Best local structure for audio, FluCoMa-proven |
| Dim. reduction fallback | PCA (Eigen) | Instant, interpretable, good for quick preview |
| C++ -> JS real-time | emitEventIfBrowserIsVisible @ 30Hz | Proven pattern in this project (O-GrainScatter) |
| C++ -> JS bulk | withNativeFunction returning JSON | One-time corpus load |
| JS -> C++ interaction | withNativeFunction | Click/lasso selection events |
| Visual encoding | X=UMAP1, Y=UMAP2, color=pitch, size=loudness | Follows CataRT conventions |
| Cursor display | Canvas2D overlay on WebGL | Simpler; cursor doesn't need GPU acceleration |
| Update architecture | Push (timer) not poll | Avoids DAW UI thread starvation (3DVerb lesson) |
