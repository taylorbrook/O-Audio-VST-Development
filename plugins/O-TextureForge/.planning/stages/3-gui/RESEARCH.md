# Stage 3: GUI Implementation - Research

**Date:** 2026-02-14
**Plugin:** O-TextureForge
**Stage:** 3-gui
**Phase:** research

---

## 1. regl-scatterplot Integration

### 1.1 Library Overview

**Repository:** https://github.com/flekschas/regl-scatterplot
**License:** MIT
**Dependencies:** `regl`, `pub-sub-es`, `kdbush` (built-in)
**Capacity:** 20 million points at 60fps (WebGL instanced rendering)
**Bundle size:** ~50-80KB gzipped (regl + regl-scatterplot + pub-sub-es)

### 1.2 Initialization Pattern

```javascript
import createScatterplot from 'regl-scatterplot';

const canvas = document.querySelector('#scatter-canvas');
const scatterplot = createScatterplot({
  canvas,
  width: canvas.clientWidth,
  height: canvas.clientHeight,
  pointSize: 4,
  pointColor: ['#8B6914', '#C9A27B', '#6B8E4E'],  // Earth tones
  opacity: 0.8,
  backgroundColor: [0, 0, 0, 0],  // Transparent (over Naturalist bg)
  showReticle: false,  // We draw our own cursor
  deselectOnDblClick: true,
  deselectOnEscape: true,
  mouseMode: 'panZoom',
});
```

### 1.3 Data Format

Points are `number[][]` where each inner array is `[x, y, category, value]`:
```javascript
// [x, y, colorEncoding, sizeEncoding]
const points = grainData.map(g => [
  g.x,           // UMAP/PCA x (0-1 normalized)
  g.y,           // UMAP/PCA y (0-1 normalized)
  g.pitchNorm,   // Color by pitch (-1 to 1)
  g.rmsNorm      // Size by energy (0-1)
]);

scatterplot.draw(points, {
  zDataType: 'continuous',   // z = pitch (color encoding)
  wDataType: 'continuous',   // w = energy (size encoding)
});
```

Alternative object format also supported:
```javascript
const pointsObj = {
  x: Float32Array.from(xs),
  y: Float32Array.from(ys),
  z: Float32Array.from(pitchValues),
  w: Float32Array.from(energyValues),
};
```

### 1.4 Point Styling

```javascript
scatterplot.set({
  pointColor: ['#8B6914'],        // Single amber color for all
  pointColorActive: '#6B8E4E',    // Green for active grains
  pointColorHover: '#C9A27B',     // Light brown on hover
  pointSize: [3, 8],              // Size range mapped to w value
  pointSizeSelected: 10,
  opacity: 0.75,
  opacityInactiveMax: 0.3,        // Dim inactive points
  colorBy: 'valueA',              // Map z to color
  sizeBy: 'valueB',               // Map w to size
});
```

### 1.5 Interaction Events

```javascript
// Select event - fired on click
scatterplot.subscribe('select', ({ points }) => {
  if (points.length > 0) {
    const grainIdx = points[0];
    // Send to C++ via native function
    window.__JUCE__.backend.emitEvent('__juce__invoke', {
      name: 'selectGrain',
      params: [grainIdx],
      resultId: promiseHandler.createPromise()[0]
    });
  }
});

// Point hover
scatterplot.subscribe('pointOver', (pointIndex) => {
  // Highlight hovered grain
});

scatterplot.subscribe('pointOut', () => {
  // Remove highlight
});

// View change (pan/zoom)
scatterplot.subscribe('view', ({ camera, xScale, yScale }) => {
  // Update cursor overlay coordinates
});
```

### 1.6 Animated Transitions (PCA → UMAP)

```javascript
// Initial draw with PCA layout
scatterplot.draw(pcaPoints);

// When UMAP completes, animate to new positions
scatterplot.draw(umapPoints, {
  transition: true,
  transitionDuration: 1500,     // 1.5s transition
  transitionEasing: (t) => t * (2 - t),  // Ease-out quadratic
});

// Listen for transition complete
scatterplot.subscribe('transitionEnd', () => {
  console.log('UMAP layout active');
});
```

### 1.7 Programmatic Selection (for active grains)

```javascript
// Highlight active grains from C++ viz update
function updateActiveGrains(activeIndices) {
  scatterplot.select(activeIndices, { preventEvent: true });
}

// Or use opacity array for envelope-based pulsing
function updateGrainOpacity(opacityArray) {
  scatterplot.set({ opacity: opacityArray });
}
```

### 1.8 Key API Details from TypeScript Definitions

- **Points type:** `number[][] | { x, y, z?, w?, line?, lineOrder? }`
- **Draw options:** `{ transition, transitionDuration, transitionEasing, preventFilterReset, hover, select, filter, zDataType, wDataType }`
- **Events:** `init`, `destroy`, `select` (→ `{ points: number[] }`), `pointOver` (→ `number`), `pointOut` (→ `number`), `view`, `draw`, `transitionStart`, `transitionEnd`
- **Camera:** `{ target: [x, y], distance, rotation, view: Float32Array }`
- **Mouse modes:** `panZoom`, `lasso`, `rotate`

---

## 2. webpack Bundling for JUCE BinaryData

### 2.1 Approach Decision: webpack (proven) over esbuild (faster)

Both work, but webpack is more battle-tested for WebGL libraries. The build is a one-time developer step — bundle size and correctness matter more than build speed.

### 2.2 Package Configuration

**Source/ui/package.json:**
```json
{
  "name": "o-textureforge-ui",
  "version": "1.0.0",
  "private": true,
  "scripts": {
    "build": "webpack --mode production",
    "dev": "webpack --mode development --watch"
  },
  "dependencies": {
    "regl-scatterplot": "^1.9.0",
    "regl": "^2.1.0",
    "pub-sub-es": "^3.1.0"
  },
  "devDependencies": {
    "webpack": "^5.90.0",
    "webpack-cli": "^5.1.0"
  }
}
```

**Source/ui/webpack.config.js:**
```javascript
const path = require('path');

module.exports = {
  entry: './src/app.js',
  output: {
    filename: 'app.bundle.js',
    path: path.resolve(__dirname, 'public/js'),
  },
  target: 'web',
  resolve: {
    extensions: ['.js'],
  },
};
```

### 2.3 Entry Point Structure

**Source/ui/src/app.js:**
```javascript
import createScatterplot from 'regl-scatterplot';
import { getSliderState, getComboBoxState, getNativeFunction } from '../public/js/juce/index.js';

// Initialize after JUCE bridge is ready
document.addEventListener('DOMContentLoaded', () => {
  initScatterPlot();
  initControls();
  listenForVizUpdates();
});
```

### 2.4 CMake Integration

The bundled `app.bundle.js` file is committed to the repo (not built at CMake time). This avoids requiring Node.js on all build machines and keeps the CMake configure fast.

```cmake
juce_add_binary_data(OuariconTextureForge_UIResources
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/js/app.bundle.js     # webpack output (committed)
        Source/ui/public/js/juce/index.js
        Source/ui/public/js/juce/check_native_interop.js
        Source/ui/public/css/ouaricon-naturalist.css
        Source/ui/public/images/fern.png
)
```

### 2.5 Resource Provider Updates

New URL mappings needed in PluginEditor.cpp `getResource()`:
```
/js/app.bundle.js   → application/javascript
/css/ouaricon-naturalist.css → text/css
/images/fern.png    → image/png
```

---

## 3. umappp C++ Integration

### 3.1 CMake FetchContent

```cmake
# umappp (UMAP dimensionality reduction)
FetchContent_Declare(
    umappp
    GIT_REPOSITORY https://github.com/libscran/umappp.git
    GIT_TAG v3.2.0
)
FetchContent_MakeAvailable(umappp)

target_link_libraries(OuariconTextureForge PRIVATE
    libscran::umappp
)
```

umappp automatically pulls its dependencies (Eigen, knncolle, CppIrlba, aarand, subpar, sanisizer) via its own CMake FetchContent declarations.

### 3.2 API Usage

```cpp
#include <umappp/umappp.hpp>
#include <knncolle/knncolle.hpp>

// Data must be column-major: ndim rows x nobs columns
// For 19 descriptors x N grains:
std::vector<double> data(19 * numGrains);
// Fill column-major: data[dim + obs * 19]

std::vector<double> embedding(2 * numGrains);

umappp::Options opt;
opt.num_neighbors = 15;
opt.min_dist = 0.1;
opt.num_epochs = 500;       // or leave unset for auto
opt.num_threads = std::max(1, (int)std::thread::hardware_concurrency() - 2);
opt.parallel_optimization = false;  // Not worth it for <50K points
opt.initialize = umappp::InitializeMethod::SPECTRAL;

knncolle::VptreeBuilder<int, double> builder;
auto status = umappp::initialize(
    19, numGrains, data.data(),
    builder,
    2,                    // 2D output
    embedding.data(),
    opt
);
```

### 3.3 Epoch-by-Epoch Progress

```cpp
int totalEpochs = status.num_epochs();
int batchSize = 50;

for (int epoch = batchSize; epoch <= totalEpochs; epoch += batchSize) {
    if (threadShouldExit()) break;  // juce::Thread cancellation

    status.run(embedding.data(), epoch);

    float progress = static_cast<float>(epoch) / totalEpochs;
    // Report progress to UI via AsyncUpdater or lambda
    onProgress(progress);
}

// Final run to complete any remaining epochs
status.run(embedding.data());
```

### 3.4 Default Epoch Count Formula

From umappp source (`initialize.hpp`):
```
N <= 10000: 500 epochs
N > 10000:  200 + ceil(300 * 10000 / N)
```

So for N=50000 → 260 epochs, keeping total work roughly constant.

### 3.5 Performance Estimates (19D, VP-tree exact neighbors)

| N grains | Epochs | Est. Time (single thread) | Est. Time (4 threads) |
|----------|--------|---------------------------|----------------------|
| 1,000    | 500    | ~0.2-0.5s                 | ~0.1-0.3s           |
| 5,000    | 500    | ~2-5s                     | ~1-3s               |
| 10,000   | 500    | ~5-15s                    | ~3-8s               |
| 50,000   | 260    | ~20-60s                   | ~10-30s             |

For corpora > 10K grains, consider approximate neighbors via knncolle_annoy or knncolle_hnsw for 30-80x speedup on neighbor search.

### 3.6 Data Layout Considerations

umappp uses column-major layout (Fortran-style): each column is one observation. Our GrainMetadata stores descriptors per-grain, so conversion is needed:

```cpp
// Convert row-major grain descriptors to column-major for umappp
std::vector<double> columnMajor(19 * numGrains);
for (size_t i = 0; i < numGrains; ++i) {
    for (int d = 0; d < 19; ++d) {
        columnMajor[d + i * 19] = static_cast<double>(grains[i].descriptors[d]);
    }
}
```

### 3.7 Thread Architecture

UMAP runs on the existing CorpusLoader background thread, after descriptor extraction:

```
CorpusLoader::run() {
    1. Load audio file
    2. Downmix + resample
    3. Segment into grains
    4. Extract descriptors (19D)
    5. Normalize (z-score)
    6. Build KD-tree → atomic swap
    7. Compute PCA → send to UI (AsyncUpdater)
    8. Compute UMAP (with progress) → send to UI (AsyncUpdater)
}
```

---

## 4. PCA Implementation (Eigen)

### 4.1 Approach

Since Eigen arrives as a umappp dependency, use it for PCA. The covariance matrix is only 19x19 (tiny), so PCA is effectively instant regardless of corpus size.

### 4.2 Implementation

```cpp
#include <Eigen/Dense>

// Input: N grains x 19 descriptors (already z-score normalized)
// Output: N x 2 coordinates in [0, 1]
void computePCA(const std::vector<GrainMetadata>& grains,
                std::vector<float>& outX, std::vector<float>& outY)
{
    const int N = static_cast<int>(grains.size());
    const int D = 19;

    // Build Eigen matrix (N rows x 19 cols)
    Eigen::MatrixXd data(N, D);
    for (int i = 0; i < N; ++i)
        for (int d = 0; d < D; ++d)
            data(i, d) = grains[i].descriptors[d];

    // Center data (subtract column means)
    Eigen::VectorXd mean = data.colwise().mean();
    data.rowwise() -= mean.transpose();

    // Compute covariance matrix (19x19 — tiny)
    Eigen::MatrixXd cov = (data.transpose() * data) / (N - 1);

    // Eigenvalue decomposition
    Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> solver(cov);

    // Take top 2 eigenvectors (highest eigenvalues are last)
    Eigen::MatrixXd pc = solver.eigenvectors().rightCols(2).rowwise().reverse();

    // Project data onto 2 principal components
    Eigen::MatrixXd projected = data * pc;  // N x 2

    // Normalize to [0, 1] range
    double minX = projected.col(0).minCoeff();
    double maxX = projected.col(0).maxCoeff();
    double minY = projected.col(1).minCoeff();
    double maxY = projected.col(1).maxCoeff();
    double rangeX = std::max(maxX - minX, 1e-10);
    double rangeY = std::max(maxY - minY, 1e-10);

    outX.resize(N);
    outY.resize(N);
    for (int i = 0; i < N; ++i) {
        outX[i] = static_cast<float>((projected(i, 0) - minX) / rangeX);
        outY[i] = static_cast<float>((projected(i, 1) - minY) / rangeY);
    }
}
```

### 4.3 Performance

- **19x19 covariance + eigendecomposition:** <1ms for any N
- **N x 19 matrix multiply:** ~1ms for 10K points, ~5ms for 50K
- **Total PCA time:** <10ms even for large corpora — effectively instant

---

## 5. WebView Relay/Attachment Pattern (from O-GrainScatter)

### 5.1 Critical Declaration Order

From the established pattern across 15+ Ouaricon plugins:

```cpp
// PluginEditor.h
private:
    // 1. RELAYS first (no dependencies)
    std::unique_ptr<juce::WebSliderRelay> energyRelay;
    std::unique_ptr<juce::WebSliderRelay> brightnessRelay;
    // ... all 10 slider relays + 1 combobox relay

    // 2. WEBVIEW second (chains .withOptionsFrom(*relay))
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3. ATTACHMENTS last (depend on relay + APVTS param)
    std::unique_ptr<juce::WebSliderParameterAttachment> energyAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> brightnessAttachment;
    // ... all 10 slider attachments + 1 combobox attachment
```

Destroyed in reverse order: attachments → webView → relays. This prevents dangling references.

### 5.2 WebView Construction with Relays

```cpp
// Constructor
energyRelay = std::make_unique<juce::WebSliderRelay>("energy");
brightnessRelay = std::make_unique<juce::WebSliderRelay>("brightness");
// ... create all relays

webView = std::make_unique<juce::WebBrowserComponent>(
    juce::WebBrowserComponent::Options{}
        .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
        .withWinWebView2Options(...)
        .withNativeIntegrationEnabled()
        .withResourceProvider([this](const auto& url) { return getResource(url); })
        .withOptionsFrom(*energyRelay)
        .withOptionsFrom(*brightnessRelay)
        // ... chain all relays
        .withNativeFunction("getCorpusData", [this](auto& args, auto complete) {
            // Return corpus JSON for scatter plot
        })
        .withNativeFunction("selectGrain", [this](auto& args, auto complete) {
            // Handle grain selection from scatter click
        })
);

// After webView creation, create attachments
energyAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
    *processorRef.parameters.getParameter("ENERGY"), *energyRelay);
```

### 5.3 Timer Callback Pattern (30Hz viz updates)

```cpp
void TextureForgeEditor::timerCallback()
{
    const auto& snap = processorRef.getVizSnapshot();

    // Build compact JSON
    juce::String json = "{\"cx\":" + juce::String(snap.cursorX, 3)
        + ",\"cy\":" + juce::String(snap.cursorY, 3)
        + ",\"g\":[";

    for (int i = 0; i < snap.activeCount; ++i) {
        if (i > 0) json += ",";
        json += "{\"i\":" + juce::String(snap.activeGrains[i].grainIndex)
            + ",\"e\":" + juce::String(snap.activeGrains[i].envelope, 2) + "}";
    }
    json += "]}";

    webView->emitEventIfBrowserIsVisible("vizUpdate", json);
}
```

### 5.4 JavaScript Side — Knob Setup

From O-GrainScatter `app.js` (proven pattern):

```javascript
import { getSliderState, getComboBoxState } from './juce/index.js';

const ANGLE_MIN = -135;
const ANGLE_RANGE = 270;
const SENSITIVITY = 0.005;

function setupKnob(paramId, state, formatter, defaultNorm) {
    const knobEl = document.querySelector(`.knob[data-param="${paramId}"]`);
    const indicator = knobEl.querySelector('.knob-indicator');
    const valueEl = document.querySelector(`[data-value="${paramId}"]`);

    let isDragging = false, lastY = 0;

    function updateDisplay(norm) {
        const angle = ANGLE_MIN + norm * ANGLE_RANGE;
        indicator.style.transform = `rotate(${angle}deg)`;
        if (valueEl) valueEl.textContent = formatter(norm);
    }

    state.valueChangedEvent.addListener(() => {
        updateDisplay(state.getNormalisedValue());
    });

    knobEl.addEventListener('mousedown', (e) => {
        isDragging = true;
        lastY = e.clientY;
        state.sliderDragStarted();
    });

    document.addEventListener('mousemove', (e) => {
        if (!isDragging) return;
        const deltaY = lastY - e.clientY;
        const norm = state.getNormalisedValue();
        const newNorm = Math.max(0, Math.min(1, norm + deltaY * SENSITIVITY));
        state.setNormalisedValue(newNorm);
        lastY = e.clientY;
    });

    document.addEventListener('mouseup', () => {
        if (isDragging) { isDragging = false; state.sliderDragEnded(); }
    });

    knobEl.addEventListener('dblclick', (e) => {
        e.preventDefault();
        state.setNormalisedValue(defaultNorm);
    });
}
```

---

## 6. Ouaricon Naturalist Aesthetic

### 6.1 Design System (from O-GrainScatter, O-Lyrica, O-Orbit)

**Palette:**
- Paper background: `#F5E6D3` (main), `#EBD9C7` (gradient mid), `#E8D0B8` (gradient end)
- Text primary: `#3C2F2F` (dark brown)
- Text secondary: `#8B7355` (sepia)
- Knob body: `#FFF8DC` (cream), `#F5DEB3` / `#E8D5B7` (wheat alternating rings)
- Knob border: `#8B7355` (bronze)
- Knob indicator: `#8B6914` (dark gold)
- Active/green: `#6B8E4E` (botanical green), `#5C7A3A` (dark green border)
- Toggle active: `rgba(107,142,35,0.5)` (olive green)

**Typography:**
- Primary: `'Garamond', 'Georgia', 'Times New Roman', serif`
- Headers: uppercase, letter-spacing 2-3px
- Labels: 8-9px uppercase, letter-spacing 0.5-1.5px

**Visual treatment:**
- Botanical overlay (fern PNG) at 8% opacity, positioned bottom-right
- Paper texture background with grain via repeating-linear-gradient
- Subtle box-shadow on controls: `inset 1px 1px 3px rgba(0,0,0,0.3)`
- Fleuron dividers: `&#10087;` at 30% opacity

### 6.2 Scatter Plot Color Mapping

For O-TextureForge specifically (from CONTEXT.md):
- **Inactive grains:** Amber/brown (`#8B6914`, `#C9A27B`)
- **Active grains:** Green (`#6B8E4E`)
- **Cursor:** Dark gold crosshair with radius circle
- **Background:** Transparent (shows Naturalist paper through)

### 6.3 Fern Asset

**Source:** `/Users/taylorbrook/Dev/VST-development/plugins/O-Lyrica/Resources/ui/images/fern_naturalistsmisc1Geor_0089.png`
**Copy to:** `Source/ui/public/images/fern.png`
**Usage in CSS:**
```css
body::after {
    content: '';
    position: fixed;
    bottom: 0; right: 0;
    width: 300px; height: 400px;
    background-image: url('/images/fern.png');
    background-size: contain;
    background-repeat: no-repeat;
    opacity: 0.08;
    pointer-events: none;
    z-index: -1;
}
```

### 6.4 Knob CSS (seed cross-section pattern)

From O-GrainScatter — conic-gradient simulating botanical seed cross-section:
```css
.knob {
    width: 48px; height: 48px;
    border-radius: 50%;
    border: 2px solid #8B7355;
    background:
        radial-gradient(circle, transparent 86%, #C9A27B 86%, #C9A27B 90%,
            #8B7355 90%, #8B7355 92%, transparent 92%),
        conic-gradient(from 0deg,
            #F5DEB3 0deg, #F5DEB3 18deg, #8B7355 18deg, #8B7355 19deg,
            #E8D5B7 19deg, #E8D5B7 36deg, #8B7355 36deg, #8B7355 37deg,
            /* ... repeating 18deg wheat + 1deg separator pattern */
        ),
        radial-gradient(circle, #FFF8DC 0%, #FFF8DC 20%, transparent 20%);
    box-shadow: inset 1px 1px 3px rgba(0,0,0,0.3),
                inset -1px -1px 2px rgba(255,248,220,0.5),
                2px 2px 6px rgba(0,0,0,0.2);
}
```

---

## 7. Layout Plan (900x600 Fixed)

### 7.1 Wireframe

```
┌──────────────────────────────────────────────────────────────┐
│ O-TextureForge     Concatenative Texture Engine              │ Header (36px)
├──────────────────────────────────────────┬───────────────────┤
│                                          │  Energy    ○      │
│                                          │  Brightness ○     │
│        WebGL Scatter Plot                │  Texture   ○      │ Main Area
│        (regl-scatterplot canvas)         │                   │ (~430px)
│                                          │  [UMAP ▪▪▪▪ 60%] │
│        + Canvas2D cursor overlay         │                   │
│                                          │  Scatter X  ○     │
│                                          │  Scatter Y  ○     │
│                                          │  Variation  ○     │
├──────────────────────────────────────────┴───────────────────┤
│  Position ○  Grain Size ○  Density ○  Crossfade ○  Gain ○   │ Bottom Strip
│  [MIDI Mode ▾]  [Drop audio file here]                      │ (~120px)
├──────────────────────────────────────────────────────────────┤
│                                              🌿 fern overlay │ Footer (14px)
└──────────────────────────────────────────────────────────────┘
```

### 7.2 Layout Allocation

- **Header:** 36px — Plugin name + tagline
- **Scatter area:** ~430px height × 600px width (left 67%)
- **Macro knobs:** ~430px height × 300px width (right 33%) — 3 timbral macros + scatter XY + variation + UMAP progress
- **Bottom strip:** ~120px — Position, Grain Size, Density, Crossfade, Gain, MIDI mode selector, drop zone
- **Footer/fern:** 14px — Fleuron divider, fern overlay bleeds from bottom-right

---

## 8. Native Functions Needed

### 8.1 C++ → JS (withNativeFunction)

| Function | Args | Returns | Purpose |
|----------|------|---------|---------|
| `getCorpusData` | none | JSON array of `[x, y, pitch, energy]` per grain | One-time corpus load for scatter plot |
| `selectGrain` | `grainIndex: int` | void | Scatter click → play grain |
| `setScatterPosition` | `x: float, y: float` | void | Scatter drag → update SCATTER_X/Y params |

### 8.2 C++ → JS (emitEventIfBrowserIsVisible)

| Event | Frequency | Data | Purpose |
|-------|-----------|------|---------|
| `vizUpdate` | 30Hz | `{cx, cy, g:[{i,e}...]}` | Cursor + active grains |
| `corpusLoaded` | once | `{count, pcaReady}` | Trigger scatter plot init |
| `umapProgress` | ~2Hz | `{progress: 0.0-1.0}` | UMAP progress bar |
| `umapComplete` | once | JSON array of new `[x, y]` per grain | Trigger PCA→UMAP transition |

---

## 9. DSP Gap Fixes (Wire in Stage 3)

### 9.1 Parameter Gaps from Stage 2 Verification

These parameters exist in APVTS but aren't fully wired to DSP:

1. **CROSSFADE** — Crossfade percentage needs to modulate Hann envelope width per grain
2. **POSITION** — Temporal position filtering needs to be wired into GrainScheduler's query logic
3. **GRAIN_SIZE** — Dynamic grain size changes need to re-segment or use variable-length grains

### 9.2 Corpus State Persistence

Add to `getStateInformation` / `setStateInformation`:
```cpp
// Save corpus file path
auto corpusXml = xml->createNewChildElement("CORPUS");
if (currentCorpus != nullptr)
    corpusXml->setAttribute("filePath", currentCorpus->filePath);

// On restore: reload corpus from saved path
auto* corpusXml = xml->getChildByName("CORPUS");
if (corpusXml != nullptr) {
    juce::File file(corpusXml->getStringAttribute("filePath"));
    if (file.existsAsFile())
        loadCorpusFile(file);
}
```

---

## 10. Open Questions Resolved

### Q1: Knob rendering approach
**Answer:** CSS conic-gradient knobs (same as O-GrainScatter). No SVG needed. The seed cross-section pattern is pure CSS with alternating wheat/separator rings. This is proven across 15+ Ouaricon plugins.

### Q2: Fern image optimization
**Answer:** Copy from O-Lyrica. If file size is too large for BinaryData, resize to max 400px width and compress with pngquant. Target <200KB.

### Q3: webpack vs esbuild
**Answer:** webpack. It's proven with regl-scatterplot, has wider compatibility with WebGL shader imports, and build speed is irrelevant (one-time developer step).

---

## 11. Risk Assessment

### HIGH Risk: regl-scatterplot in JUCE WebView
- **Concern:** WebGL context creation in WKWebView/WebView2 may differ from Chrome
- **Mitigation:** Test on macOS first (WKWebView has full WebGL2 support). WebView2 on Windows also supports WebGL2. Existing JUCE WebView plugins with Canvas2D work fine.
- **Fallback:** Canvas2D renderer for <5K points if WebGL fails

### MEDIUM Risk: umappp Dependency Chain
- **Concern:** 7 transitive FetchContent dependencies may conflict with existing CMake or slow configure
- **Mitigation:** Pin exact version tags. umappp v3.2.0 is well-tested. Test CMake configure time increase.
- **Fallback:** PCA-only mode (still functional, just less clustered layout)

### LOW Risk: Bundle Size
- **Concern:** webpack bundle + fern image + CSS might bloat BinaryData
- **Mitigation:** Production webpack minification, image compression. Expected total: ~300-500KB.

---

## 12. Implementation Order Recommendation

### Phase 3.1: WebView + Aesthetic + npm Pipeline
1. Copy fern asset from O-Lyrica
2. Create `ouaricon-naturalist.css` (from O-GrainScatter patterns)
3. Set up `package.json` + `webpack.config.js`
4. `npm install && npm run build` → generate `app.bundle.js`
5. Create full HTML layout (scatter canvas + knob grid + bottom strip)
6. Add all new resources to BinaryData + resource provider
7. Create relays for all 12 parameters
8. Wire attachments
9. Static render test — layout visible, knobs responsive

### Phase 3.2: Scatter Plot + PCA + Interaction
1. Implement `PCAProjection.h/cpp` using Eigen
2. Add PCA computation to CorpusLoader pipeline (after descriptor extraction)
3. Implement `getCorpusData` native function (returns PCA-projected grain array)
4. Initialize regl-scatterplot in `app.js` with corpus data
5. Configure earth-tone colors, point sizes
6. Implement click → `selectGrain` native function → SCATTER_X/Y update
7. Implement bidirectional sync (DAW automation ↔ scatter cursor)
8. Pan/zoom navigation

### Phase 3.3: Real-Time Viz + UMAP + DSP Gaps
1. Wire timerCallback to push VizSnapshot JSON to WebView
2. Implement active grain pulsing (opacity modulated by envelope)
3. Draw cursor crosshair + radius circle on Canvas2D overlay
4. Integrate umappp via FetchContent
5. Add UMAP computation to CorpusLoader (after PCA, with progress)
6. Implement PCA→UMAP animated transition
7. Wire CROSSFADE, POSITION, GRAIN_SIZE parameters into DSP
8. Add corpus file path persistence to state save/restore

---

**END OF RESEARCH**
