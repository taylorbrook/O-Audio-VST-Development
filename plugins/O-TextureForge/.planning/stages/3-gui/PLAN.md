# Stage 3: GUI Implementation - Execution Plan

**Date:** 2026-02-14
**Plugin:** O-TextureForge
**Stage:** 3-gui
**Phase:** plan

---

## Goal

Implement the full WebView GUI for O-TextureForge: Ouaricon Naturalist aesthetic, WebGL scatter plot (regl-scatterplot via webpack), PCA/UMAP dimensionality reduction, bidirectional scatter-to-APVTS sync, real-time 30Hz grain visualization, and all 12 parameter relay/attachment bindings. Also wire remaining DSP gaps (crossfade, position, grain-size) and add corpus state persistence.

---

## Tasks

### Phase 3.1: WebView + Ouaricon Naturalist Aesthetic + npm Pipeline

#### Task 1: Copy fern botanical asset and optimize
- **Files:** `Source/ui/public/images/fern.png`
- **Action:** Copy from `plugins/O-Lyrica/Resources/ui/images/fern_naturalistsmisc1Geor_0089.png`. If >500KB, resize to max 400px width with `sips`.
- **Depends on:** none

#### Task 2: Create Ouaricon Naturalist CSS
- **Files:** `Source/ui/public/css/ouaricon-naturalist.css`
- **Action:** Aged paper background (#F5E6D3), earth-tone palette, Garamond typography, seed cross-section knob styling (conic-gradient), fern overlay at 8% opacity, fleuron dividers. Port from O-GrainScatter patterns.
- **Depends on:** Task 1 (fern.png path reference in CSS)

#### Task 3: Set up npm project with webpack + regl-scatterplot
- **Files:** `Source/ui/package.json`, `Source/ui/webpack.config.js`
- **Action:** Create package.json with regl-scatterplot, regl, pub-sub-es as deps, webpack + webpack-cli as devDeps. Create webpack.config.js (entry: `./src/app.js`, output: `public/js/app.bundle.js`, mode: production, target: web).
- **Depends on:** none

#### Task 4: Create main application JS entry point
- **Files:** `Source/ui/src/app.js`
- **Action:** Import createScatterplot. Set up DOMContentLoaded handler. Initialize scatter plot, knob controllers, viz event listeners. Import JUCE bridge modules. This is the webpack entry point that bundles into `app.bundle.js`.
- **Depends on:** Task 3

#### Task 5: npm install and webpack build
- **Action:** Run `cd Source/ui && npm install && npm run build` to generate `public/js/app.bundle.js`. Verify bundle exists and is <200KB.
- **Depends on:** Tasks 3, 4

#### Task 6: Create full HTML layout
- **Files:** `Source/ui/public/index.html`
- **Action:** Replace placeholder with full 900x600 layout:
  - Header (36px): plugin name + tagline
  - Main area: scatter canvas (left 67%) + macro knobs (right 33%): Energy, Brightness, Texture, Scatter X, Scatter Y, Variation + UMAP progress bar
  - Bottom strip (120px): Position, Grain Size, Density, Crossfade, Gain knobs + MIDI Mode dropdown + drop zone
  - Link CSS and load app.bundle.js (non-module script tag)
  - Keep JUCE bridge as type="module"
- **Depends on:** Task 2 (CSS ready)

#### Task 7: Add new resources to CMakeLists BinaryData
- **Files:** `CMakeLists.txt`
- **Action:** Add to juce_add_binary_data: `app.bundle.js`, `ouaricon-naturalist.css`, `fern.png`. Note BinaryData name mangling rules (dots → underscores, slashes → underscores).
- **Depends on:** Tasks 5, 6

#### Task 8: Update resource provider with new URL mappings
- **Files:** `Source/PluginEditor.cpp`
- **Action:** Add getResource() mappings:
  - `/js/app.bundle.js` → `application/javascript`
  - `/css/ouaricon-naturalist.css` → `text/css`
  - `/images/fern.png` → `image/png`
- **Depends on:** Task 7

#### Task 9: Create all 12 parameter relays
- **Files:** `Source/PluginEditor.h`, `Source/PluginEditor.cpp`
- **Action:** Declare 11 WebSliderRelays (Energy, Brightness, Texture, Position, GrainDensity, GrainSize, ScatterX, ScatterY, Variation, Crossfade, OutputGain) + 1 WebComboBoxRelay (MidiMode) BEFORE webView declaration. Create in constructor BEFORE webView construction. Chain all relays with `.withOptionsFrom(*relay)` on webView options.
- **Depends on:** none

#### Task 10: Create all 12 parameter attachments
- **Files:** `Source/PluginEditor.h`, `Source/PluginEditor.cpp`
- **Action:** Declare 11 WebSliderParameterAttachments + 1 WebComboBoxParameterAttachment AFTER webView declaration. Create AFTER webView construction. Connect each to APVTS parameter + corresponding relay.
- **Depends on:** Task 9

#### Task 11: Build and static render test
- **Action:** `ninja OuariconTextureForge_VST3 OuariconTextureForge_AU`. Verify: WebView loads, Naturalist styling visible, knobs render (no data yet), no console errors.
- **Depends on:** Tasks 8, 9, 10

### Phase 3.2: WebGL Scatter Plot + PCA + Interaction

#### Task 12: Implement PCA projection (C++, Eigen)
- **Files:** `Source/dsp/PCAProjection.h`, `Source/dsp/PCAProjection.cpp`
- **Action:** Using Eigen (arrives as umappp dependency). 19D→2D via covariance eigendecomposition. Input: vector<GrainMetadata>. Output: vector<float> x, vector<float> y (normalized 0-1). <10ms for any corpus size.
- **Depends on:** Task 16 (umappp FetchContent brings Eigen)

#### Task 13: Add PCA computation to CorpusLoader pipeline
- **Files:** `Source/dsp/CorpusLoader.h`, `Source/dsp/CorpusLoader.cpp`, `Source/dsp/SharedCorpus.h`
- **Action:** After descriptor extraction + normalization + KD-tree build, compute PCA. Store pcaX/pcaY vectors in SharedCorpus. Add fields: `std::vector<float> pcaX, pcaY;`
- **Depends on:** Task 12

#### Task 14: Implement getCorpusData native function
- **Files:** `Source/PluginEditor.cpp`
- **Action:** Add `.withNativeFunction("getCorpusData", ...)` to webView construction. Returns JSON array of `[x, y, pitchNorm, energyNorm]` per grain from SharedCorpus PCA data. Called once on corpus load.
- **Depends on:** Tasks 9, 13

#### Task 15: Implement scatter plot initialization in app.js
- **Files:** `Source/ui/src/app.js`
- **Action:** On `corpusLoaded` event: call `getCorpusData()`, initialize regl-scatterplot with earth-tone colors (#8B6914 amber, #6B8E4E green active), transparent background, pointSize 3-8, opacity 0.75. Subscribe to `select` event for grain clicks.
- **Depends on:** Task 14

#### Task 16: Integrate umappp + Eigen via FetchContent
- **Files:** `CMakeLists.txt`
- **Action:** Add FetchContent for umappp v3.2.0. Link `libscran::umappp` to target. umappp auto-fetches Eigen, knncolle, CppIrlba, aarand, subpar, sanisizer.
- **Depends on:** none

#### Task 17: Implement scatter click → APVTS parameter update
- **Files:** `Source/ui/src/app.js`, `Source/PluginEditor.cpp`
- **Action:** On scatter `select` event: get clicked point's normalized x,y. Call `setScatterPosition(x, y)` native function. In C++: set SCATTER_X and SCATTER_Y parameter values. Also implement `selectGrain(grainIndex)` native function.
- **Depends on:** Tasks 14, 15

#### Task 18: Implement bidirectional scatter cursor sync
- **Files:** `Source/ui/src/app.js`, `Source/PluginEditor.cpp`
- **Action:** C++→JS: vizUpdate event already carries cursor x,y. JS: draw Canvas2D overlay crosshair at cursor position (thin amber line + radius circle). JS→C++: scatter click updates APVTS (Task 17). DAW automation of SCATTER_X/Y → relay → JS display update.
- **Depends on:** Task 17

#### Task 19: Rebuild webpack bundle and test scatter plot
- **Action:** `cd Source/ui && npm run build`. Build plugin. Load audio file → verify scatter plot renders with PCA layout, click selects grains, pan/zoom works.
- **Depends on:** Tasks 15, 17, 18

### Phase 3.3: Real-Time Viz + UMAP + DSP Gap Fixes

#### Task 20: Wire timerCallback to push VizSnapshot to WebView
- **Files:** `Source/PluginEditor.cpp`
- **Action:** In timerCallback: read vizSnapshot, build compact JSON `{cx, cy, g:[{i, e}...]}`, call `webView->emitEventIfBrowserIsVisible("vizUpdate", json)`. Also emit `corpusLoaded` event when corpus changes.
- **Depends on:** Task 11

#### Task 21: Implement active grain pulsing in JS
- **Files:** `Source/ui/src/app.js`
- **Action:** On `vizUpdate` event: extract active grain indices + envelopes. Use `scatterplot.select(indices, {preventEvent: true})` to highlight active grains in green. Modulate opacity based on envelope values.
- **Depends on:** Tasks 15, 20

#### Task 22: Implement animated cursor crosshair + radius circle
- **Files:** `Source/ui/src/app.js`, `Source/ui/public/index.html`
- **Action:** Add Canvas2D overlay on top of scatter canvas. On vizUpdate: draw crosshair at cursor (cx, cy) position with amber color. Draw radius circle proportional to Variation parameter. Coordinate transform: scatter plot view → canvas pixel coordinates.
- **Depends on:** Tasks 18, 20

#### Task 23: Implement UMAP projection (C++ wrapper)
- **Files:** `Source/dsp/UMAPProjection.h`, `Source/dsp/UMAPProjection.cpp`
- **Action:** Wrap umappp API. Convert row-major grain descriptors to column-major for umappp. Use VptreeBuilder, 15 neighbors, 0.1 min_dist, spectral init. Epoch-by-epoch progress reporting (batch of 50 epochs). Output: vector<float> umapX, umapY normalized to [0,1].
- **Depends on:** Task 16

#### Task 24: Add UMAP computation to CorpusLoader pipeline
- **Files:** `Source/dsp/CorpusLoader.h`, `Source/dsp/CorpusLoader.cpp`, `Source/dsp/SharedCorpus.h`
- **Action:** After PCA (step 7 in pipeline): compute UMAP on same background thread with progress. Add umapX/umapY vectors + umapReady flag to SharedCorpus. Add progress callback to CorpusLoader for UI updates. Use AsyncUpdater or lambda to report progress to editor.
- **Depends on:** Tasks 13, 23

#### Task 25: Implement PCA→UMAP animated transition
- **Files:** `Source/ui/src/app.js`, `Source/PluginEditor.cpp`
- **Action:** Emit `umapProgress` event at ~2Hz during UMAP computation. On `umapComplete` event with new coordinates: call `scatterplot.draw(umapPoints, {transition: true, transitionDuration: 1500})`. JS: show/hide progress bar based on `umapProgress` events.
- **Depends on:** Tasks 21, 24

#### Task 26: Wire CROSSFADE parameter into DSP
- **Files:** `Source/dsp/GrainScheduler.cpp`, `Source/dsp/GrainVoice.h`
- **Action:** Use crossfadePercent to modulate Hann envelope width per grain voice. Higher crossfade = wider envelope overlap.
- **Depends on:** none

#### Task 27: Wire POSITION parameter into DSP
- **Files:** `Source/dsp/GrainScheduler.cpp`
- **Action:** Use position (0-1) to bias grain selection toward temporal region of source file. Add positional weighting to KD-tree query or post-filter results by temporal position.
- **Depends on:** none

#### Task 28: Wire GRAIN_SIZE parameter into DSP
- **Files:** `Source/dsp/GrainScheduler.cpp`
- **Action:** Use grainSizeMs to set variable grain lengths. Apply to new grain triggers (not currently playing grains). Recalculate grain envelope window length on each trigger.
- **Depends on:** none

#### Task 29: Add corpus file path persistence
- **Files:** `Source/PluginProcessor.cpp`
- **Action:** In getStateInformation: save currentCorpus->filePath as XML child element. In setStateInformation: read saved path, reload if file exists. Ensures corpus survives DAW project save/load.
- **Depends on:** none

#### Task 30: Final webpack rebuild + full integration test
- **Action:** `cd Source/ui && npm run build`. Full ninja build. Test: load file → PCA scatter → UMAP transition → click grains → viz updates → all knobs work → save/load state preserves corpus.
- **Depends on:** all previous tasks

---

## Files to Create

| File | Type | Purpose |
|------|------|---------|
| `Source/ui/package.json` | config | npm dependencies |
| `Source/ui/webpack.config.js` | config | webpack bundler config |
| `Source/ui/src/app.js` | JS | main application (webpack entry) |
| `Source/ui/public/js/app.bundle.js` | JS (generated) | webpack output (committed) |
| `Source/ui/public/css/ouaricon-naturalist.css` | CSS | Naturalist aesthetic |
| `Source/ui/public/images/fern.png` | image | botanical overlay |
| `Source/dsp/PCAProjection.h` | C++ header | PCA 19D→2D |
| `Source/dsp/PCAProjection.cpp` | C++ source | PCA implementation |
| `Source/dsp/UMAPProjection.h` | C++ header | UMAP wrapper |
| `Source/dsp/UMAPProjection.cpp` | C++ source | UMAP implementation |

## Files to Modify

| File | Changes |
|------|---------|
| `CMakeLists.txt` | Add umappp FetchContent, new source files, expanded BinaryData |
| `Source/PluginEditor.h` | Add 12 relay + 12 attachment declarations |
| `Source/PluginEditor.cpp` | Relay/attachment construction, native functions, resource provider, timerCallback, corpusLoaded event |
| `Source/PluginProcessor.cpp` | State persistence (corpus file path save/restore) |
| `Source/ui/public/index.html` | Full Naturalist layout (scatter + knobs + controls) |
| `Source/dsp/SharedCorpus.h` | Add pcaX/Y, umapX/Y vectors, umapReady flag |
| `Source/dsp/CorpusLoader.h` | Add PCA/UMAP pipeline steps, progress callback |
| `Source/dsp/CorpusLoader.cpp` | Integrate PCA + UMAP after descriptor extraction |
| `Source/dsp/GrainScheduler.cpp` | Wire crossfade, position, grain-size parameters |
| `Source/dsp/GrainVoice.h` | Variable envelope width for crossfade |

---

## Success Criteria

- [ ] WebView loads with Ouaricon Naturalist aesthetic (aged paper, fern, earth tones)
- [ ] All 12 knobs responsive via relay/attachment bindings
- [ ] regl-scatterplot renders corpus grains at 60fps with PCA layout on file load
- [ ] Click on scatter plot updates SCATTER_X/Y parameters (JS→C++ bidirectional)
- [ ] DAW automation of SCATTER_X/Y moves cursor on scatter plot (C++→JS)
- [ ] Active grains pulse green in scatter plot at 30Hz
- [ ] Cursor crosshair + variation radius circle visible on scatter
- [ ] UMAP computation runs on background thread with progress bar
- [ ] PCA→UMAP animated transition (1.5s) when UMAP completes
- [ ] CROSSFADE parameter modulates grain envelope width
- [ ] POSITION parameter biases grain selection by temporal region
- [ ] GRAIN_SIZE parameter controls per-trigger grain length
- [ ] Corpus file path saved/restored with DAW project (state persistence)
- [ ] Plugin builds and passes AU validation
- [ ] No WebView console errors or resource loading failures

---

## Dependency Graph

```
Phase 3.1 (Tasks 1-11): WebView + Aesthetic
  Task 1 (fern) ─────────┐
  Task 2 (CSS) ←──────── Task 1
  Task 3 (npm) ───────────┤
  Task 4 (app.js) ←────── Task 3
  Task 5 (build) ←──────── Tasks 3,4
  Task 6 (HTML) ←───────── Task 2
  Task 7 (CMake) ←──────── Tasks 5,6
  Task 8 (resources) ←──── Task 7
  Task 9 (relays) ─────────┤ (independent)
  Task 10 (attachments) ←── Task 9
  Task 11 (test) ←───────── Tasks 8,9,10

Phase 3.2 (Tasks 12-19): Scatter Plot + PCA
  Task 16 (umappp) ─────── (independent, start early)
  Task 12 (PCA) ←───────── Task 16 (Eigen dep)
  Task 13 (PCA→loader) ←── Task 12
  Task 14 (getCorpus) ←─── Tasks 9,13
  Task 15 (scatter init) ← Task 14
  Task 17 (click→APVTS) ←─ Tasks 14,15
  Task 18 (bidirectional) ← Task 17
  Task 19 (test) ←───────── Tasks 15,17,18

Phase 3.3 (Tasks 20-30): Viz + UMAP + DSP Gaps
  Task 20 (timerCB) ←────── Task 11
  Task 21 (grain pulse) ←── Tasks 15,20
  Task 22 (cursor) ←─────── Tasks 18,20
  Task 23 (UMAP impl) ←──── Task 16
  Task 24 (UMAP→loader) ←── Tasks 13,23
  Task 25 (PCA→UMAP) ←───── Tasks 21,24
  Tasks 26,27,28 (DSP gaps)  (independent)
  Task 29 (state persist) ── (independent)
  Task 30 (final test) ←──── all
```

---

## Estimated Effort

| Phase | Tasks | Est. Hours |
|-------|-------|-----------|
| 3.1: WebView + Aesthetic | 11 | 4-6 |
| 3.2: Scatter + PCA | 8 | 5-8 |
| 3.3: Viz + UMAP + DSP | 11 | 5-8 |
| **Total** | **30** | **14-22** |

---

## Risk Mitigations

| Risk | Mitigation | Fallback |
|------|-----------|----------|
| WebGL in JUCE WebView fails | Test on macOS first (WKWebView has full WebGL2) | Canvas2D scatter for <5K points |
| umappp FetchContent conflicts | Pin v3.2.0, isolate in subdirectory | PCA-only mode (functional, less clustered) |
| webpack bundle too large | Production mode minification, tree-shaking | Expected ~100-150KB, well under limit |
| BinaryData name collisions | Check BinaryData:: names after CMake configure | Rename files if collision occurs |

---

**END OF PLAN**
