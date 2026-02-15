# Stage 3: GUI Implementation - Context

## Discussion Summary

**Date:** 2026-02-14
**Participants:** User, Claude

## Requirements Confirmed

- **Full regl-scatterplot + webpack pipeline** — npm build step bundling regl-scatterplot into a single app.js for BinaryData. Best performance for 50K+ grain points at 60fps with built-in pan/zoom/select.
- **PCA + UMAP (full spec)** — PCA instant preview on file load (<100ms), animated transition to UMAP layout computed on background thread (2-15s). Requires umappp + Eigen + knncolle via FetchContent.
- **Fern botanical asset provided** — `fern_naturalistsmisc1Geor_0089.png` (exists in O-Lyrica, copy to O-TextureForge Resources). Low-opacity overlay in bottom-right corner behind controls.
- **Ouaricon Naturalist aesthetic** — Aged paper background, warm earth tones, serif typography (Garamond), botanical seed cross-section knobs, earth-tone scatter points (amber/brown dots, green highlights for active grains).
- **Scatter click updates APVTS** — Click/drag on scatter plot writes to ScatterX/ScatterY parameters. DAW can record and automate scatter position. Bidirectional sync (DAW automation also moves cursor).
- **Fixed 900x600 window** — No resize. Consistent layout across all screens.
- **Wire Stage 2 DSP gaps in Stage 3** — Fix crossfade, position, and grain-size parameter wiring as part of GUI integration. Also add corpus state persistence (save/restore file path).

## Constraints Identified

- **WebView resource provider** — All JS/CSS/images served via explicit URL mapping in getResource(). No file:// URLs.
- **BinaryData size limit** — Fern PNG + bundled app.js + CSS must fit in reasonable BinaryData. Optimize fern image (target <500KB).
- **30Hz update rate** — VizSnapshot already double-buffered. Push active grain data + cursor position to WebView via `emitEventIfBrowserIsVisible`.
- **No Node.js at runtime** — webpack bundle is a build-time step only. The bundled app.js is pure browser JS.
- **Cross-platform WebView** — Resource provider uses `getResourceProviderRoot()` (juce:// on macOS, https://juce.backend/ on Windows). Already configured in Stage 1.
- **FetchContent dependency chain** — umappp depends on Eigen, knncolle, CppIrlba. All header-only but adds CMake configure time.

## Approach Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Scatter plot renderer | regl-scatterplot (npm + webpack) | 50K+ points at 60fps, built-in interaction, proven library |
| Dimensionality reduction | PCA (instant) + UMAP (background) | Best clustering quality; animated transition provides feedback |
| Scatter → APVTS | Bidirectional sync | Enables DAW automation of scatter position |
| Window size | Fixed 900x600 | Simpler layout, consistent across screens |
| Botanical asset | Copy fern PNG from O-Lyrica | Existing asset, proven in Naturalist aesthetic |
| DSP gap fixes | Wire in Stage 3 | Natural integration point when connecting GUI to DSP |
| Knob style | Ouaricon Naturalist seed cross-section | Consistent with brand aesthetic |
| Scatter point colors | Amber/brown (inactive), green (active) | Earth-tone palette matching Naturalist theme |
| Layout | 60% scatter plot, 3 vertical macros right, secondary strip below | Per BRIEF wireframe |
| UMAP library | umappp v3.x via FetchContent | Header-only, BSD-2, C++ native |

## Existing Assets from Stage 1/2

### Already in place (PluginEditor.cpp):
- WebView with resource provider + native integration
- WinWebView2 user data folder configured
- 30Hz timer running (currently no-op)
- File drag-and-drop working
- Member declaration order scaffolded (relays → webView → attachments)

### Already in place (PluginProcessor):
- 12 APVTS parameters (all cached as atomic pointers)
- VizSnapshot double-buffer (activeGrains + cursor position)
- SharedCorpus with grain database + KD-tree
- CorpusLoader with completion callback ready

### WebView files to expand:
- `Source/ui/public/index.html` — placeholder, needs full layout
- `Source/ui/public/js/juce/index.js` — JUCE bridge, keep as-is
- `Source/ui/public/js/juce/check_native_interop.js` — JUCE requirement, keep as-is

### New files needed:
- `Source/ui/package.json` — npm deps (regl-scatterplot, regl, pub-sub-es)
- `Source/ui/webpack.config.js` — bundle to single app.js
- `Source/ui/src/app.js` — main application JS (scatter plot, interaction, viz)
- `Source/ui/public/css/ouaricon-naturalist.css` — Naturalist aesthetic
- `Source/ui/public/images/fern.png` — botanical overlay (copied from O-Lyrica)
- `Source/dsp/PCAProjection.h/.cpp` — PCA 19D→2D (Eigen or custom)
- `Source/dsp/UMAPProjection.h/.cpp` — UMAP wrapper (umappp)

## Phase Breakdown

### Phase 3.1: WebView + Ouaricon Naturalist Aesthetic
- Set up npm project with regl-scatterplot + webpack
- Create Naturalist CSS (aged paper, earth tones, Garamond)
- Build full HTML layout (scatter area, macro knobs, secondary controls, fern overlay)
- Add fern PNG to BinaryData
- Wire resource provider for new assets (CSS, bundled JS, fern image)
- Static render test (no data yet, just layout/aesthetic)

### Phase 3.2: WebGL Scatter Plot + PCA + Interaction
- Implement PCA 19D→2D projection in C++ (Eigen or custom)
- Add `withNativeFunction("getCorpusData")` — returns grain positions as JSON
- Initialize regl-scatterplot with corpus data on file load
- Configure point colors (amber/brown by default, green for active)
- Implement click/drag → ScatterX/ScatterY APVTS parameter update (JS→C++)
- Implement DAW parameter → scatter cursor sync (C++→JS)
- Pan/zoom navigation

### Phase 3.3: Real-Time Viz + UMAP + DSP Gap Fixes
- Wire 30Hz timerCallback to push VizSnapshot JSON to WebView
- Implement active grain pulsing (opacity modulated by envelope)
- Animated cursor crosshair + variation radius circle
- Integrate umappp via FetchContent (+ Eigen, knncolle deps)
- Background UMAP computation with progress reporting
- Animated PCA→UMAP transition (regl-scatterplot point transitions)
- Wire crossfade, position, grain-size parameters into DSP
- Add corpus file path to state persistence (getStateInformation)

## Open Questions

- **Knob rendering approach:** Use SVG knobs in WebView (consistent with other Ouaricon plugins) or HTML/CSS-only knobs? (Resolve in research phase — check O-Lyrica/O-GrainScatter patterns)
- **Fern image optimization:** Current O-Lyrica fern may be large. May need to resize/compress for BinaryData budget. (Resolve in Phase 3.1)
- **webpack vs esbuild:** webpack is specified but esbuild is faster. Worth considering during Phase 3.1 setup. (Low priority — either works)

## Next Phase

Ready for: **research** phase — investigate regl-scatterplot integration patterns, umappp API, PCA implementation options, and knob rendering approach for WebView.
