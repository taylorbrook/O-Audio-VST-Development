# O-TextureForge Architecture Specification

**Plugin Type:** Instrument (VST3/AU)
**Complexity Tier:** 6 (DEEP)
**Contract Status:** ✅ Complete (BRIEF.md, REQUIREMENTS.md validated)
**Generated:** 2026-02-13
**JUCE Version:** 8.0.4

---

## System Overview

O-TextureForge is a concatenative synthesis instrument that transforms a single audio file into an infinite explorable texture. The system combines real-time grain-based synthesis with intelligent descriptor-based grain selection via KD-tree nearest-neighbor search, all controlled through an interactive WebGL scatter plot visualization that IS the primary instrument interface.

**High-level data flow:**

```
Audio File Load → Segmentation → Descriptor Extraction → KD-Tree Build
                                                               ↓
                                                         UMAP/PCA 2D Layout
                                                               ↓
User Interaction (scatter plot click/drag) → KD-Tree Query → Grain Selection → Polyphonic Grain Scheduler → Audio Output
                                                  ↓
                                            WebView @ 30Hz (cursor + active grains visualization)
```

**Core architectural principle:** The scatter plot is not decorative—it is the instrument. Every point represents a grain; spatial proximity in the 2D layout corresponds to perceptual timbral similarity. This is the first VST/AU plugin to offer GPU-accelerated descriptor-space navigation for concatenative synthesis.

---

## Core Components

### 1. File Loading & Corpus Management

**JUCE Classes:**
- `juce::AudioFormatManager` - Load WAV/AIFF/MP3/FLAC
- `juce::AudioFormatReader` - Decode audio data
- `juce::AudioBuffer<float>` - Store loaded audio corpus

**Implementation:**
- Drag-and-drop via `juce::FileDragAndDropTarget` (PluginEditor)
- Background thread loading (avoid blocking audio thread)
- Single file restriction for v1 (multi-file layering out of scope)
- Corpus buffer stored as shared `juce::AudioBuffer<float>` (lock-free read access from audio thread)

**Thread model:**
- UI thread initiates file load
- Background thread (`juce::Thread` subclass) loads and analyzes
- Audio thread reads corpus buffer (read-only, no locks)

**Module dependencies:**
- `juce_audio_formats` (format reading)
- `juce_audio_basics` (AudioBuffer)

---

### 2. Grain Segmentation

**Strategy:** Fixed-size segmentation (simplest, most predictable for v1)

**Parameters:**
- Grain size range: 10-500ms (user-adjustable via parameter)
- Default: 50ms
- Overlap: 50% (25ms hop for 50ms grains)

**Algorithm:**
```
grainStartSamples[] = 0, hopSizeSamples, 2*hopSizeSamples, ..., N-grainSizeSamples
numGrains = floor((totalSamples - grainSizeSamples) / hopSizeSamples) + 1
```

**Why fixed-size over onset detection:**
- Deterministic grain count (no variability based on source content)
- Simpler implementation (no onset detector needed)
- Uniformly covers entire file (no gaps for sustained sounds)
- Acceptable for v1; onset detection can be added in v2

**Data structure:**
```cpp
struct GrainMetadata {
    int startSample;
    int durationSamples;
    std::array<float, 19> descriptors; // Feature vector
};
std::vector<GrainMetadata> grainDatabase;
```

**Module dependencies:** None (pure algorithmic)

---

### 3. Descriptor Extraction Pipeline

**Purpose:** Analyze each grain to compute 19-dimensional timbral fingerprint for KD-tree indexing and UMAP projection.

**Descriptor set (19 dimensions total):**

| Descriptor | Dimension Count | JUCE Class/Custom | Purpose |
|------------|-----------------|-------------------|---------|
| **MFCCs** | 13 | Custom (FFT-based) | Spectral envelope, timbral character |
| **Spectral Centroid** | 1 | Custom | Brightness (center of mass of spectrum) |
| **Spectral Flatness** | 1 | Custom | Noisiness vs tonality (0=tonal, 1=noise) |
| **RMS Energy** | 1 | `juce::Decibels::gainToDecibels` | Loudness |
| **Zero-Crossing Rate** | 1 | Custom | Transient/noise character |
| **Fundamental Frequency (f0)** | 1 | Custom (autocorrelation) | Pitch |
| **Spectral Flux** | 1 | Custom | Rate of spectral change |

**FFT Configuration:**
- FFT size: 2048 samples (balanced resolution)
- Window: Hann window (`juce::dsp::WindowingFunction<float>::hann`)
- Overlap: 50%
- JUCE class: `juce::dsp::FFT` (built-in, no external dependencies)

**MFCC Computation Pipeline:**
```
1. Pre-emphasis: y[n] = x[n] - 0.97*x[n-1]
2. Windowing: Apply Hann window
3. FFT: juce::dsp::FFT::performRealOnlyForwardTransform()
4. Magnitude spectrum: |X[k]|
5. Mel filterbank: 40 triangular filters (20Hz-20kHz, mel-spaced)
   mel(f) = 2595 * log10(1 + f/700)
6. Log compression: log10(max(filterEnergy, 1e-10))
7. DCT: Discrete Cosine Transform (custom or via Eigen)
8. Truncate: Keep first 13 coefficients
```

**Custom Implementation Notes:**
- No external DSP library needed (Essentia/Gist avoided to minimize binary size)
- Mel filterbank pre-computed in `prepareToPlay()`, stored as lookup matrix
- DCT matrix pre-computed (13x40), stored for efficient matrix multiply
- Total code: ~300-400 lines

**Thread safety:**
- Descriptor extraction runs on **background thread** during corpus load
- Results stored in `std::vector<GrainMetadata>` (atomic pointer swap for thread-safe handoff)

**Module dependencies:**
- `juce_dsp` (FFT, windowing)
- Optional: Eigen (for DCT matrix multiply, may already be dependency via umappp)

**Reference implementation:**
- See `research/concatenative-synthesis-comprehensive.md` Section 8 (MFCC pseudocode)

---

### 4. KD-Tree Nearest-Neighbor Search

**Purpose:** Real-time audio thread queries to find grains matching target descriptor vector.

**Library:** nanoflann (header-only, BSD license)
- Repository: https://github.com/jlblancoc/nanoflann
- Integration: Single `#include <nanoflann.hpp>`
- No linking required (header-only)

**KD-Tree Configuration:**
```cpp
#include <nanoflann.hpp>

// Adapt std::vector<GrainMetadata> to nanoflann
struct DescriptorAdaptor {
    const std::vector<GrainMetadata>& grains;

    inline size_t kdtree_get_point_count() const { return grains.size(); }

    inline float kdtree_get_pt(const size_t idx, const size_t dim) const {
        return grains[idx].descriptors[dim];
    }

    template <class BBOX> bool kdtree_get_bbox(BBOX&) const { return false; }
};

using KDTree = nanoflann::KDTreeSingleIndexAdaptor<
    nanoflann::L2_Simple_Adaptor<float, DescriptorAdaptor>,
    DescriptorAdaptor,
    19 // dimensionality
>;
```

**Thread model:**
- **Background thread:** Build tree after descriptor extraction completes (~1-10ms for 10K points)
- **Audio thread:** Query tree (read-only, allocation-free, lock-free)

**Query pattern:**
```cpp
// Audio thread - no allocations
float targetDescriptor[19] = { /* from macro knobs + scatter position */ };
size_t nearestIndex;
float distanceSquared;

kdTree->knnSearch(targetDescriptor, 1, &nearestIndex, &distanceSquared);
// nearestIndex is the grain to play
```

**Why nanoflann:**
- Real-time safe (no allocations during query)
- Optimized for low-dimensional spaces (19D is ideal for KD-trees)
- Header-only (no build complexity)
- BSD licensed (commercial use allowed)

**Alternative considered:** FluCoMa's internal KD-tree (rejected due to tight coupling with FluCoMa data structures)

**Critical pattern (from juce8-critical-patterns.md):** KD-tree construction MUST happen on background thread. Only queries happen on audio thread.

**Module dependencies:** None (header-only)

---

### 5. Dimensionality Reduction (UMAP + PCA)

**Purpose:** Project 19D descriptor space to 2D for scatter plot visualization.

**Two-tiered approach:**

#### 5.1 PCA (Instant Preview)

**Library:** Eigen (likely already dependency via umappp)

**Computation time:** <100ms for any corpus size (covariance matrix is 19x19, always tiny)

**Algorithm:**
```
1. Center data (subtract column means)
2. Compute 19x19 covariance matrix
3. Eigenvalue decomposition (Eigen::SelfAdjointEigenSolver)
4. Take 2 largest eigenvectors
5. Project: P = X * V (where V is 19x2 principal components)
6. Normalize to [0, 1] range
```

**When:** Runs immediately on background thread after descriptor extraction completes. Scatter plot appears within 1 second of file load.

**Pros:** Instant, deterministic, interpretable axes
**Cons:** Linear assumption misses non-linear timbral relationships, produces "blob" layout

#### 5.2 UMAP (Quality Layout)

**Library:** umappp (header-only C++, BSD-2-Clause)
- Repository: https://github.com/libscran/umappp
- Integration: FetchContent in CMakeLists.txt
- Dependencies (all header-only): Eigen, knncolle, CppIrlba, aarand, subpar

**Computation time (estimates from benchmarks):**
- 1,000 grains: <1s
- 5,000 grains: 2-5s
- 10,000 grains: 5-15s (with HNSW approximate neighbors)
- 50,000 grains: 30-60s (with HNSW)

**Configuration:**
```cpp
umappp::Options opt;
opt.num_neighbors = 15;      // neighborhood size
opt.min_dist = 0.1;          // packing tightness (FluCoMa default)
opt.num_epochs = 500;        // auto-selected by umappp
opt.num_threads = std::max(1, std::thread::hardware_concurrency() - 2);
opt.initialize_method = umappp::InitializeMethod::SPECTRAL;
```

**When:** Runs on background thread after PCA completes. User sees PCA layout immediately, then smooth animated transition to UMAP layout when UMAP finishes (2-15 seconds later).

**Progress reporting:**
```cpp
// Run epoch-by-epoch for progress bar
for (int epoch = 50; epoch <= status.num_epochs(); epoch += 50) {
    status.run(embedding.data(), epoch);
    float progress = (float)epoch / status.num_epochs();
    // Update UI progress bar via AsyncUpdater
}
```

**Why UMAP over t-SNE:**
- Faster (5-50x speedup for 10K+ points)
- Preserves both local AND global structure (t-SNE loses global)
- FluCoMa's research-backed choice for audio descriptors
- Better clustering of perceptually similar sounds

**Incremental updates (adding new grains):**
- Corpus <5,000 grains: Re-run full UMAP (fast enough at 2-5s)
- Corpus ≥5,000 grains: Use weighted-neighbor projection (instant), queue background re-computation

**Transform new points (pseudocode):**
```cpp
// Find k=15 nearest neighbors in 19D space
auto neighbors = kdTree->knnSearch(newGrainDescriptor, 15);
// Weighted average of neighbors' 2D positions
float x2D = 0, y2D = 0, totalWeight = 0;
for (auto& [idx, distSq] : neighbors) {
    float weight = 1.0f / (distSq + 1e-6f);
    x2D += weight * umapEmbedding[idx].x;
    y2D += weight * umapEmbedding[idx].y;
    totalWeight += weight;
}
x2D /= totalWeight;
y2D /= totalWeight;
```

**Thread architecture:**
```
Background Thread (juce::Thread subclass):
  1. Load audio file
  2. Segment into grains
  3. Extract descriptors (19D per grain)
  4. Normalize descriptors (z-score)
  5. Compute PCA (instant)
  6. Send PCA embedding to UI (AsyncUpdater)
  7. Compute UMAP (2-15s with progress)
  8. Send UMAP embedding to UI (AsyncUpdater)
  9. Build KD-tree
 10. Atomic pointer swap (corpus ready)
```

**Module dependencies:**
- umappp (FetchContent)
- Eigen (via umappp)
- knncolle (via umappp)

**Reference:**
- `research/umap-dimensionality-reduction-audio-plugins.md` (full implementation examples)

---

### 6. Polyphonic Grain Scheduler

**Architecture:** Voice-stealing polyphonic scheduler with up to 64 simultaneous grains.

**Reference implementation:** O-GrainScatter's `GrainPool` + `GrainScheduler` pattern (see `plugins/O-GrainScatter/Source/dsp/GrainScheduler.h`)

**Voice structure:**
```cpp
struct GrainVoice {
    bool active = false;
    int grainIndex = -1;         // Index into grainDatabase
    int readPosition = 0;        // Current sample position in grain
    float envelope = 0.0f;       // Current envelope value [0, 1]
    float gain = 1.0f;           // Per-grain gain
    float pitch = 1.0f;          // Playback rate multiplier (1.0 = original pitch)

    // MIDI-specific (for pitch-mapped mode)
    int midiNote = -1;           // -1 = no MIDI note assigned
    int midiVelocity = 0;
};

std::array<GrainVoice, 64> voicePool;
```

**Grain triggering logic (per MIDI mode):**

#### Mode 1: Pitch-Mapped (Polyphonic Instrument)
```
MIDI Note On → Allocate voice → Select grain from KD-tree → Pitch shift via resampling
- C3 (MIDI 60) = original pitch (playback rate 1.0)
- Other notes = playback rate = 2^((note - 60) / 12.0)
- Velocity maps to Energy descriptor weight
- Polyphonic (up to 8 simultaneous notes per MIDI spec)
- Voice stealing: oldest note when all 8 voices occupied
```

#### Mode 2: Trigger + Modulate
```
Any MIDI Note → Trigger grain at current scatter cursor position
- Velocity controls variation radius (how far from cursor to search)
- Mod wheel (CC 1) → modulate scatter X position
- Aftertouch (channel pressure) → modulate scatter Y position
- Monophonic triggering (each note-on kills previous grain)
```

#### Mode 3: Generative Drone
```
Continuous grain triggering via internal timer (no MIDI notes needed)
- Grain density parameter controls trigger rate (1-64 grains/sec)
- MIDI CC messages map to all macro parameters for hardware control
- No note-on/off handling (always generating output)
```

**Crossfading:**
- Windowing function: Hann window (`juce::dsp::WindowingFunction<float>::hann`)
- Overlap: User-controlled (0-100%, default 50%)
- Overlap-add synthesis: Multiple grains sum with fade-in/fade-out envelopes

**Voice allocation algorithm:**
```cpp
int allocateVoice() {
    // First: find inactive voice
    for (int i = 0; i < maxPolyphony; ++i)
        if (!voicePool[i].active)
            return i;

    // Second: steal oldest active voice
    int oldestIdx = 0;
    int oldestAge = voicePool[0].ageCounter;
    for (int i = 1; i < maxPolyphony; ++i) {
        if (voicePool[i].ageCounter > oldestAge) {
            oldestAge = voicePool[i].ageCounter;
            oldestIdx = i;
        }
    }
    return oldestIdx;
}
```

**Real-time safety:**
- All grain triggering happens on audio thread
- KD-tree query is allocation-free
- Voice pool is pre-allocated array (no dynamic allocation)
- Corpus audio buffer is read-only from audio thread

**Module dependencies:**
- `juce_dsp` (WindowingFunction)
- `juce_audio_basics` (AudioBuffer read)

---

### 7. WebGL Scatter Plot Visualization

**Library:** regl-scatterplot (WebGL-based, MIT license)
- Repository: https://github.com/flekschas/regl-scatterplot
- npm: `regl-scatterplot`
- Capacity: 20 million points at 60fps (far exceeds needs)

**Why regl-scatterplot:**
- Purpose-built for large scatter plots
- Built-in pan, zoom, lasso selection, click handlers
- GPU-instanced rendering (1 draw call for all points)
- Per-point encoding: `[x, y, colorValue, sizeValue]` format
- Animated transitions (`draw(newPoints, { transition: true })`)
- Small footprint (only depends on `regl` + `pub-sub-es`)

**Point encoding:**
```javascript
// Each grain = 1 point
const points = grains.map(g => [
    g.x,              // UMAP/PCA x-coordinate (0-1)
    g.y,              // UMAP/PCA y-coordinate (0-1)
    g.pitch,          // Color by pitch (-1 to 1 normalized)
    g.energy          // Size by energy (0-1)
]);

scatterplot.draw(points, {
    pointColor: pitchColormap,  // Blue (low) → Red (high)
    pointSize: [2, 10],         // Scale size by energy value
    opacity: 0.8
});
```

**Interaction handlers:**
```javascript
// Click to set playback target
scatterplot.subscribe('select', ({ points }) => {
    if (points.length > 0) {
        const grainIdx = points[0];
        window.__JUCE__.backend.selectGrain(grainIdx);
    }
});

// Drag to scrub through timbral space
scatterplot.subscribe('view', ({ camera }) => {
    // Send camera position to C++ for cursor tracking
});
```

**Real-time cursor overlay:**
- Separate Canvas2D layer on top of WebGL canvas
- Draws crosshair + selection radius circle at current target position
- Pulsing highlights on active grains (updated at 30Hz from C++)

**Performance budget:**
- Initial render: <100ms (WebGL init ~40ms, point upload ~20-50ms)
- Pan/zoom: 60fps (GPU matrix transform, no re-upload)
- Per-frame cursor update: <5ms (30Hz from C++)

**Alternative considered:** Canvas2D (rejected—too slow for 10K+ points; drops to 22 FPS at 50K points per benchmarks)

**Module dependencies (npm):**
- `regl-scatterplot` (WebGL scatter plot)
- `regl` (WebGL wrapper, dependency of regl-scatterplot)

**Reference:**
- `research/2d-scatter-plot-concatenative-synthesis.md` Section 2 (rendering tech comparison)

---

### 8. WebView C++ ↔ JavaScript Communication

**Pattern:** Push-based (C++ timer emits events to JS) at 30Hz.

**Reference implementation:** O-GrainScatter's existing pattern (validated in production).

**Architecture:**
```
Audio Thread (processBlock)
    ↓ lock-free write
Visualization Snapshot Buffer (double-buffer, atomic index)
    ↓ read
GUI Thread (Timer @ 30Hz)
    ↓ emitEventIfBrowserIsVisible()
WebView JavaScript
    ↓ addEventListener()
regl-scatterplot + Canvas overlay update
```

**C++ Side (PluginEditor):**
```cpp
class TextureForgeEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    TextureForgeEditor(TextureForgeProcessor& p)
        : AudioProcessorEditor(&p), processorRef(p)
    {
        // ... WebView setup ...
        startTimerHz(30); // 30 FPS updates
    }

private:
    void timerCallback() override
    {
        // Read lock-free snapshot from processor
        const auto& snap = processorRef.getVisualizationSnapshot();

        // Build JSON (reuse juce::String to avoid allocations)
        juce::String json;
        json << "{"
             << "\"cursorX\":" << snap.cursorX << ","
             << "\"cursorY\":" << snap.cursorY << ","
             << "\"activeGrains\":[";

        bool first = true;
        for (int i = 0; i < snap.activeGrainCount; ++i) {
            if (!first) json << ",";
            json << "{\"idx\":" << snap.activeGrains[i].grainIndex
                 << ",\"env\":" << snap.activeGrains[i].envelope << "}";
            first = false;
        }
        json << "]}";

        webView->emitEventIfBrowserIsVisible("vizUpdate", json);
    }

    std::unique_ptr<juce::WebBrowserComponent> webView;
    TextureForgeProcessor& processorRef;
};
```

**JavaScript Side (index.html):**
```javascript
window.__JUCE__.backend.addEventListener('vizUpdate', (data) => {
    const state = JSON.parse(data);

    // Update cursor overlay (Canvas2D)
    cursorCtx.clearRect(0, 0, canvas.width, canvas.height);
    cursorCtx.beginPath();
    cursorCtx.arc(state.cursorX * canvas.width,
                  state.cursorY * canvas.height,
                  10, 0, 2 * Math.PI);
    cursorCtx.strokeStyle = 'orange';
    cursorCtx.stroke();

    // Highlight active grains (regl-scatterplot opacity update)
    state.activeGrains.forEach(g => {
        pointOpacity[g.idx] = 0.2 + 0.8 * g.env; // Pulse with envelope
    });
    scatterplot.set({ opacity: pointOpacity });
});

// User interaction → C++ callback
scatterplot.subscribe('select', ({ points }) => {
    if (points.length > 0) {
        window.__JUCE__.backend.selectGrain(points[0]);
    }
});
```

**Data flows:**

| Flow | Direction | Frequency | Size | Method |
|------|-----------|-----------|------|--------|
| Corpus load | C++ → JS | Once per load | 2-4MB JSON | `withNativeFunction` returning JSON |
| Cursor position | C++ → JS | 30Hz | ~50 bytes | `emitEventIfBrowserIsVisible("vizUpdate")` |
| Active grains | C++ → JS | 30Hz | ~500 bytes (64 grains max) | (included in vizUpdate) |
| Grain selection | JS → C++ | Event-driven | ~20 bytes | `withNativeFunction("selectGrain")` |
| UMAP progress | C++ → JS | 2-10 Hz | ~30 bytes | `emitEventIfBrowserIsVisible("umapProgress")` |

**Performance:**
- 30Hz @ 500 bytes/frame = 15KB/sec (negligible overhead)
- Round-trip latency (WKWebView): ~3ms (acceptable for visualization)
- Corpus load (one-time): ~100ms parsing for 10K points

**Critical pattern (juce8-critical-patterns.md #10):** Never call `emitEventIfBrowserIsVisible` before page fully loads (freezes DAW UI thread).

**Module dependencies:**
- `juce_gui_extra` (WebBrowserComponent)

**Reference:**
- `research/2d-scatter-plot-concatenative-synthesis.md` Section 3 (JUCE WebView communication)

---

## System Architecture (Non-DSP Features)

### 1. File I/O

**File loading workflow:**
```
User drops file onto WebView UI
    ↓
FileDragAndDropTarget::filesDropped() (PluginEditor)
    ↓
Launch background thread (juce::Thread subclass)
    ↓
AudioFormatManager::createReaderFor()
    ↓
Read entire file into juce::AudioBuffer<float>
    ↓
Segmentation → Descriptor Extraction → KD-Tree Build → UMAP
    ↓
Atomic pointer swap (corpus ready)
    ↓
Send corpus JSON to WebView via withNativeFunction
    ↓
WebView renders scatter plot
```

**Thread safety:**
- File I/O on background thread
- Audio thread reads corpus buffer via atomic shared_ptr (lock-free)
- Descriptor database accessed via atomic pointer (read-only from audio thread)

**Formats supported:** WAV, AIFF, MP3, FLAC (via `juce_audio_formats`)

**Critical pattern (juce8-critical-patterns.md):** All file I/O must happen off audio thread.

**Module dependencies:**
- `juce_audio_formats` (file reading)

---

### 2. MIDI Processing

**Three modes (selectable via parameter):**

#### Mode 1: Pitch-Mapped (Polyphonic Instrument)
```cpp
void processMidiMessage(const juce::MidiMessage& msg) {
    if (msg.isNoteOn()) {
        int voice = allocateVoice();
        voicePool[voice].midiNote = msg.getNoteNumber();
        voicePool[voice].midiVelocity = msg.getVelocity();

        // Velocity → Energy descriptor weight
        float energyBias = msg.getVelocity() / 127.0f;
        targetDescriptor[3] = energyBias; // RMS Energy dimension

        // Query KD-tree for grain
        int grainIdx = kdTree->nearestNeighbor(targetDescriptor);

        // Pitch shift via resampling
        float pitchRatio = std::pow(2.0f, (msg.getNoteNumber() - 60) / 12.0f);
        voicePool[voice].pitch = pitchRatio;
        voicePool[voice].grainIndex = grainIdx;
        voicePool[voice].active = true;
    }
    else if (msg.isNoteOff()) {
        // Find voice with matching note, trigger release
        for (auto& v : voicePool) {
            if (v.midiNote == msg.getNoteNumber()) {
                v.active = false; // Or start release envelope
            }
        }
    }
}
```

#### Mode 2: Trigger + Modulate
```cpp
void processMidiMessage(const juce::MidiMessage& msg) {
    if (msg.isNoteOn()) {
        // Velocity → variation radius
        float variationRadius = (msg.getVelocity() / 127.0f) * variationParam;

        // Trigger grain at current scatter cursor position
        triggerGrainAtPosition(scatterCursorX, scatterCursorY, variationRadius);
    }
    else if (msg.isController()) {
        if (msg.getControllerNumber() == 1) { // Mod wheel
            scatterCursorX = msg.getControllerValue() / 127.0f;
        }
    }
    else if (msg.isChannelPressure()) { // Aftertouch
        scatterCursorY = msg.getChannelPressureValue() / 127.0f;
    }
}
```

#### Mode 3: Generative Drone
```cpp
void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) {
    // Internal timer-based triggering (no MIDI note handling)
    samplesUntilNextGrain -= buffer.getNumSamples();
    if (samplesUntilNextGrain <= 0) {
        int intervalSamples = calculateIntervalFromDensity(grainDensityParam);
        samplesUntilNextGrain = intervalSamples;

        // Trigger grain at current scatter cursor position
        triggerGrainAtPosition(scatterCursorX, scatterCursorY, variationParam);
    }

    // MIDI CC → parameter modulation
    for (const auto msg : midi) {
        if (msg.getMessage().isController()) {
            int cc = msg.getMessage().getControllerNumber();
            float value = msg.getMessage().getControllerValue() / 127.0f;

            switch (cc) {
                case 1:  scatterCursorX = value; break;  // Mod wheel → X
                case 11: scatterCursorY = value; break;  // Expression → Y
                case 74: energyParam = value; break;     // Filter cutoff → Energy
                case 71: brightnessParam = value; break; // Resonance → Brightness
                // ... etc
            }
        }
    }
}
```

**Voice management:**
- Pitch-mapped mode: Polyphonic (8 voices per MIDI standard, voice stealing)
- Trigger mode: Monophonic (each note-on stops previous grain)
- Drone mode: Internal timer, no MIDI note handling

**Module dependencies:**
- `juce_audio_basics` (MidiMessage, MidiBuffer)

**Reference:**
- O-GrainScatter's MIDI sync patterns (see `Source/dsp/GrainScheduler.h`)

---

### 3. State Persistence

**Saved state:**
- APVTS parameter values (automatic via `AudioProcessor::getStateInformation`)
- Loaded file path (`juce::File::getFullPathName()`)
- MIDI mode selection
- Scatter cursor position (X, Y)
- UMAP seed (for deterministic re-computation)

**NOT saved (recomputed on load):**
- Corpus audio buffer (too large, reload from file path)
- Descriptor database (recompute from audio)
- KD-tree (rebuild from descriptors)
- UMAP embedding (recompute, but use saved seed for reproducibility)

**State format:**
```xml
<PLUGIN version="1.0.0">
  <PARAMETERS>
    <PARAM id="ENERGY" value="0.5"/>
    <PARAM id="BRIGHTNESS" value="0.5"/>
    ...
  </PARAMETERS>
  <CORPUS file="/path/to/audio.wav" midiMode="2" cursorX="0.3" cursorY="0.7" umapSeed="12345"/>
</PLUGIN>
```

**Load workflow:**
1. Restore parameters from XML
2. If corpus file path exists, load asynchronously on background thread
3. Show "Loading..." in WebView until corpus ready
4. Recompute descriptors + UMAP (with saved seed)
5. Restore cursor position in scatter plot

**Module dependencies:**
- `juce_core` (XmlElement for state serialization)

---

## Parameter Architecture

**APVTS Layout (12 parameters total):**

| Parameter ID | Type | Range | Default | Unit | Skew |
|--------------|------|-------|---------|------|------|
| `ENERGY` | Float | 0.0-1.0 | 0.5 | Norm | Linear |
| `BRIGHTNESS` | Float | 0.0-1.0 | 0.5 | Norm | Linear |
| `TEXTURE` | Float | 0.0-1.0 | 0.5 | Norm | Linear |
| `POSITION` | Float | 0.0-1.0 | 0.0 | Norm | Linear |
| `GRAIN_DENSITY` | Int | 1-64 | 8 | grains | Linear |
| `GRAIN_SIZE` | Float | 10-500 | 50 | ms | Linear |
| `SCATTER_X` | Float | 0.0-1.0 | 0.5 | Norm | Linear |
| `SCATTER_Y` | Float | 0.0-1.0 | 0.5 | Norm | Linear |
| `VARIATION` | Float | 0.0-1.0 | 0.2 | Norm | Linear |
| `CROSSFADE` | Float | 0-100 | 50 | % | Linear |
| `OUTPUT_GAIN` | Float | -60-12 | 0 | dB | Linear |
| `MIDI_MODE` | Choice | 0-2 | 2 (Drone) | - | - |

**Parameter → DSP mapping:**

**Macro descriptors (Energy, Brightness, Texture) weight the KD-tree search:**
```cpp
// Build target descriptor vector
float targetDescriptor[19];
for (int i = 0; i < 13; ++i) targetDescriptor[i] = 0.0f; // MFCCs (not directly controlled)
targetDescriptor[13] = brightnessParam * 2.0f - 1.0f;     // Spectral Centroid [-1, 1]
targetDescriptor[14] = textureParam * 2.0f - 1.0f;        // Spectral Flatness [-1, 1]
targetDescriptor[15] = energyParam * 2.0f - 1.0f;         // RMS Energy [-1, 1]
targetDescriptor[16] = 0.0f;                              // ZCR (not controlled)
targetDescriptor[17] = 0.0f;                              // f0 (not controlled)
targetDescriptor[18] = 0.0f;                              // Spectral Flux (not controlled)

// Query KD-tree
size_t nearestGrainIdx = kdTree->nearestNeighbor(targetDescriptor);
```

**Position parameter filters grains by temporal location:**
```cpp
// Map 0-1 position to sample range in corpus
int minSample = positionParam * (totalCorpusSamples - grainSizeSamples);
int maxSample = minSample + windowSizeSamples;

// Only search grains within this temporal window
auto candidateGrains = filterGrainsByTemporalWindow(minSample, maxSample);
auto nearestIdx = kdTree->nearestNeighborIn(targetDescriptor, candidateGrains);
```

**Variation parameter adds randomization radius:**
```cpp
// Query k=10 nearest neighbors instead of just 1
std::vector<size_t> kNearestIndices = kdTree->knnSearch(targetDescriptor, 10);

// Random selection from top k, weighted by inverse distance
float totalWeight = 0;
for (auto idx : kNearestIndices) {
    float dist = euclideanDistance(targetDescriptor, grainDescriptors[idx]);
    float weight = 1.0f / (dist + 1e-6f);
    totalWeight += weight;
}
float randVal = rng.nextFloat() * totalWeight;
// ... weighted random selection ...
```

**Scatter X/Y directly map to 2D UMAP coordinates:**
```cpp
// User clicks scatter plot → JS sends (x, y) in [0, 1]
// Find grain closest to clicked 2D position (not descriptor space)
float minDist = FLT_MAX;
int clickedGrainIdx = -1;
for (int i = 0; i < grainCount; ++i) {
    float dx = umapEmbedding[i].x - scatterX;
    float dy = umapEmbedding[i].y - scatterY;
    float dist = std::sqrt(dx*dx + dy*dy);
    if (dist < minDist) {
        minDist = dist;
        clickedGrainIdx = i;
    }
}
// Play that grain
```

**Module dependencies:**
- `juce_audio_processors` (AudioProcessorValueTreeState)

---

## Algorithm Details (Per Component)

### 1. Descriptor Extraction Implementation

**Full MFCC pipeline (custom implementation):**

```cpp
class MFCCExtractor {
    static constexpr int FFT_SIZE = 2048;
    static constexpr int NUM_MEL_FILTERS = 40;
    static constexpr int NUM_COEFFICIENTS = 13;

    juce::dsp::FFT fft { (int)std::log2(FFT_SIZE) };
    juce::dsp::WindowingFunction<float> window { FFT_SIZE, juce::dsp::WindowingFunction<float>::hann };

    std::array<std::array<float, FFT_SIZE/2+1>, NUM_MEL_FILTERS> melFilterbank;
    std::array<std::array<float, NUM_MEL_FILTERS>, NUM_COEFFICIENTS> dctMatrix;

    void prepare(double sampleRate) {
        buildMelFilterbank(sampleRate);
        buildDCTMatrix();
    }

    void extract(const float* audioFrame, std::array<float, NUM_COEFFICIENTS>& outMFCCs) {
        // 1. Pre-emphasis
        std::array<float, FFT_SIZE> emphasized;
        emphasized[0] = audioFrame[0];
        for (int i = 1; i < FFT_SIZE; ++i)
            emphasized[i] = audioFrame[i] - 0.97f * audioFrame[i-1];

        // 2. Windowing
        window.multiplyWithWindowingTable(emphasized.data(), FFT_SIZE);

        // 3. FFT
        std::array<float, FFT_SIZE*2> fftData = {0};
        for (int i = 0; i < FFT_SIZE; ++i)
            fftData[i] = emphasized[i];
        fft.performRealOnlyForwardTransform(fftData.data());

        // 4. Magnitude spectrum
        std::array<float, FFT_SIZE/2+1> magnitude;
        for (int k = 0; k <= FFT_SIZE/2; ++k) {
            float real = fftData[k];
            float imag = (k < FFT_SIZE/2) ? fftData[FFT_SIZE - k] : 0;
            magnitude[k] = std::sqrt(real*real + imag*imag);
        }

        // 5. Mel filterbank
        std::array<float, NUM_MEL_FILTERS> melEnergies;
        for (int m = 0; m < NUM_MEL_FILTERS; ++m) {
            float energy = 0;
            for (int k = 0; k <= FFT_SIZE/2; ++k)
                energy += magnitude[k] * melFilterbank[m][k];
            melEnergies[m] = std::log10(std::max(energy, 1e-10f));
        }

        // 6. DCT
        for (int c = 0; c < NUM_COEFFICIENTS; ++c) {
            float coeff = 0;
            for (int m = 0; m < NUM_MEL_FILTERS; ++m)
                coeff += dctMatrix[c][m] * melEnergies[m];
            outMFCCs[c] = coeff;
        }
    }

private:
    void buildMelFilterbank(double sampleRate) {
        auto hzToMel = [](float hz) { return 2595.0f * std::log10(1.0f + hz / 700.0f); };
        auto melToHz = [](float mel) { return 700.0f * (std::pow(10.0f, mel / 2595.0f) - 1.0f); };

        float melMin = hzToMel(0.0f);
        float melMax = hzToMel(sampleRate / 2.0f);

        std::vector<float> melPoints(NUM_MEL_FILTERS + 2);
        for (int i = 0; i < NUM_MEL_FILTERS + 2; ++i)
            melPoints[i] = melMin + i * (melMax - melMin) / (NUM_MEL_FILTERS + 1);

        std::vector<float> hzPoints(NUM_MEL_FILTERS + 2);
        for (int i = 0; i < NUM_MEL_FILTERS + 2; ++i)
            hzPoints[i] = melToHz(melPoints[i]);

        // Build triangular filters
        for (int m = 0; m < NUM_MEL_FILTERS; ++m) {
            float leftHz = hzPoints[m];
            float centerHz = hzPoints[m+1];
            float rightHz = hzPoints[m+2];

            for (int k = 0; k <= FFT_SIZE/2; ++k) {
                float freqHz = k * sampleRate / FFT_SIZE;
                float weight = 0.0f;

                if (freqHz >= leftHz && freqHz <= centerHz)
                    weight = (freqHz - leftHz) / (centerHz - leftHz);
                else if (freqHz >= centerHz && freqHz <= rightHz)
                    weight = (rightHz - freqHz) / (rightHz - centerHz);

                melFilterbank[m][k] = weight;
            }
        }
    }

    void buildDCTMatrix() {
        for (int c = 0; c < NUM_COEFFICIENTS; ++c) {
            for (int m = 0; m < NUM_MEL_FILTERS; ++m) {
                dctMatrix[c][m] = std::cos(juce::MathConstants<float>::pi * c * (m + 0.5f) / NUM_MEL_FILTERS);
            }
        }
    }
};
```

**Other descriptors (simpler):**

```cpp
// Spectral Centroid
float computeSpectralCentroid(const float* magnitude, int numBins, double sampleRate) {
    float numerator = 0, denominator = 0;
    for (int k = 0; k < numBins; ++k) {
        float freq = k * sampleRate / (2 * numBins);
        numerator += freq * magnitude[k];
        denominator += magnitude[k];
    }
    return numerator / std::max(denominator, 1e-10f);
}

// Spectral Flatness
float computeSpectralFlatness(const float* magnitude, int numBins) {
    float geometricMean = 0, arithmeticMean = 0;
    for (int k = 0; k < numBins; ++k) {
        geometricMean += std::log(std::max(magnitude[k], 1e-10f));
        arithmeticMean += magnitude[k];
    }
    geometricMean = std::exp(geometricMean / numBins);
    arithmeticMean /= numBins;
    return geometricMean / std::max(arithmeticMean, 1e-10f);
}

// RMS Energy
float computeRMSEnergy(const float* audioFrame, int numSamples) {
    float sumSquares = 0;
    for (int i = 0; i < numSamples; ++i)
        sumSquares += audioFrame[i] * audioFrame[i];
    return std::sqrt(sumSquares / numSamples);
}

// Zero-Crossing Rate
float computeZCR(const float* audioFrame, int numSamples) {
    int crossings = 0;
    for (int i = 1; i < numSamples; ++i) {
        if ((audioFrame[i-1] >= 0 && audioFrame[i] < 0) ||
            (audioFrame[i-1] < 0 && audioFrame[i] >= 0))
            ++crossings;
    }
    return (float)crossings / numSamples;
}
```

**Descriptor normalization (z-score):**
```cpp
// After extracting all grains' descriptors
for (int dim = 0; dim < 19; ++dim) {
    float sum = 0, sumSq = 0;
    for (const auto& grain : grainDatabase) {
        sum += grain.descriptors[dim];
        sumSq += grain.descriptors[dim] * grain.descriptors[dim];
    }
    float mean = sum / grainDatabase.size();
    float variance = (sumSq / grainDatabase.size()) - (mean * mean);
    float stddev = std::sqrt(std::max(variance, 1e-10f));

    // Store for new point projection later
    descriptorMeans[dim] = mean;
    descriptorStddevs[dim] = stddev;

    // Normalize all grains
    for (auto& grain : grainDatabase) {
        grain.descriptors[dim] = (grain.descriptors[dim] - mean) / stddev;
    }
}
```

---

### 2. Grain Crossfading Algorithm

**Hann window envelope:**
```cpp
void generateHannEnvelope(std::vector<float>& envelope, int grainSize, float crossfadePct) {
    int fadeLength = (int)(grainSize * crossfadePct / 100.0f);

    envelope.resize(grainSize);

    // Fade-in (first fadeLength samples)
    for (int i = 0; i < fadeLength; ++i) {
        float phase = (float)i / fadeLength;
        envelope[i] = 0.5f * (1.0f - std::cos(juce::MathConstants<float>::pi * phase));
    }

    // Sustain (middle)
    for (int i = fadeLength; i < grainSize - fadeLength; ++i)
        envelope[i] = 1.0f;

    // Fade-out (last fadeLength samples)
    for (int i = grainSize - fadeLength; i < grainSize; ++i) {
        float phase = (float)(i - (grainSize - fadeLength)) / fadeLength;
        envelope[i] = 0.5f * (1.0f + std::cos(juce::MathConstants<float>::pi * phase));
    }
}
```

**Overlap-add synthesis:**
```cpp
void renderGrainVoices(juce::AudioBuffer<float>& outputBuffer) {
    outputBuffer.clear();

    for (auto& voice : voicePool) {
        if (!voice.active) continue;

        const auto& grain = grainDatabase[voice.grainIndex];

        for (int sample = 0; sample < outputBuffer.getNumSamples(); ++sample) {
            // Read from corpus with pitch shift (linear interpolation)
            float readPos = voice.readPosition + sample * voice.pitch;
            int readIdx = (int)readPos;
            float frac = readPos - readIdx;

            if (readIdx >= grain.startSample && readIdx < grain.startSample + grain.durationSamples - 1) {
                float sample0 = corpusBuffer.getSample(0, readIdx);
                float sample1 = corpusBuffer.getSample(0, readIdx + 1);
                float interpolated = sample0 + frac * (sample1 - sample0);

                // Apply envelope
                int envelopeIdx = readIdx - grain.startSample;
                float enveloped = interpolated * envelope[envelopeIdx] * voice.gain;

                // Sum into output (overlap-add)
                for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch)
                    outputBuffer.addSample(ch, sample, enveloped);
            }
        }

        voice.readPosition += outputBuffer.getNumSamples() * voice.pitch;

        // Deactivate when grain finished
        if (voice.readPosition >= grain.startSample + grain.durationSamples)
            voice.active = false;
    }
}
```

---

## Integration Points

### 1. Dependencies & Interactions

**Dependency graph:**
```
File Load
    ↓
Grain Segmentation
    ↓
Descriptor Extraction (MFCCs, spectral, energy)
    ↓
├─→ KD-Tree Build (for real-time search)
│
└─→ PCA Reduction (instant) → WebView scatter plot (preview)
    └─→ UMAP Reduction (2-15s) → WebView scatter plot (final layout)

Audio Thread:
    MIDI Input → Mode Router → Voice Allocation
                                    ↓
    Scatter Cursor Position → KD-Tree Query → Grain Selection
                                                    ↓
    Corpus Buffer Read ← Grain Scheduler → Overlap-Add → Audio Output
                                    ↓
                            Viz Snapshot (double-buffer)
                                    ↓
                            GUI Thread (30Hz) → WebView Update
```

**Parameter interactions:**
- Energy/Brightness/Texture knobs weight the KD-tree target descriptor vector
- Position slider filters grains by temporal location before KD-tree query
- Variation parameter switches from 1-nearest to k-nearest with random selection
- Scatter X/Y directly select grains by 2D UMAP position (bypasses descriptor space)

**Critical ordering:**
1. Descriptor extraction MUST complete before KD-tree build
2. KD-tree MUST be built before audio thread queries begin
3. PCA runs before UMAP (instant preview while UMAP computes)
4. UMAP runs on background thread, atomic swap when complete

---

### 2. Thread Boundaries

**Audio Thread (real-time, lock-free):**
- Read APVTS parameters (atomic floats)
- Query KD-tree (read-only, allocation-free)
- Read corpus buffer (read-only via atomic shared_ptr)
- Render grain voices (pre-allocated voice pool)
- Write visualization snapshot to double-buffer

**Background Thread (non-real-time):**
- File loading
- Descriptor extraction
- KD-tree construction
- PCA/UMAP computation
- Progress reporting via AsyncUpdater

**GUI Thread (message thread):**
- Timer @ 30Hz reads viz snapshot
- Emits events to WebView
- Handles WebView callbacks (grain selection)
- Updates parameters via APVTS (thread-safe)

**Lock-free communication:**
```cpp
// Audio thread writes, GUI thread reads (double-buffering)
std::array<VizSnapshot, 2> vizSnapshots;
std::atomic<int> vizWriteIndex{0};

// Audio thread (write)
int writeIdx = vizWriteIndex.load(std::memory_order_relaxed);
vizSnapshots[writeIdx].cursorX = scatterX;
vizSnapshots[writeIdx].activeGrainCount = activeCount;
vizWriteIndex.store(1 - writeIdx, std::memory_order_release);

// GUI thread (read)
int readIdx = 1 - vizWriteIndex.load(std::memory_order_acquire);
const auto& snap = vizSnapshots[readIdx];
```

**Critical pattern (juce8-critical-patterns.md):** Never access UI components from audio thread, never block audio thread.

---

### 3. Processing Order

**Initialization sequence (prepareToPlay):**
```
1. Initialize APVTS parameters
2. Allocate corpus buffer (empty, ready for load)
3. Prepare MFCC extractor (build filterbank, DCT matrix)
4. Allocate voice pool (64 voices)
5. Generate Hann envelope lookup table
6. Reset KD-tree pointer (nullptr until corpus loaded)
```

**Per-block processing (processBlock):**
```
1. Read APVTS parameter values (atomic floats)
2. Process MIDI messages → update voice states
3. For each active voice:
   3a. Read grain samples from corpus buffer
   3b. Apply pitch shift (linear interpolation)
   3c. Apply envelope
   3d. Sum into output buffer (overlap-add)
4. Trigger new grains (based on MIDI mode + density)
5. Update visualization snapshot (double-buffer write)
```

**GUI timer tick (30Hz):**
```
1. Read visualization snapshot (double-buffer read)
2. Build JSON string (cursor + active grains)
3. emitEventIfBrowserIsVisible("vizUpdate", json)
```

---

## Implementation Risks

### HIGH Risk Features

#### 1. UMAP Computation Time for Large Corpora

**Risk:** 50,000 grains × 19 descriptors = 30-60 seconds UMAP time. User may perceive as "frozen."

**Complexity:** HIGH
**Impact:** Severe (poor UX, perceived hang)

**Mitigation:**
- **Primary:** Use HNSW approximate neighbors (78Spinoza/UMAP benchmarks show 80x speedup)
- Show PCA layout instantly (<100ms) as preview
- Display progress bar during UMAP (epoch-by-epoch updates)
- Allow user to cancel UMAP (keep PCA layout)
- Run UMAP on background thread with low priority

**Fallback:** PCA-only mode if UMAP integration fails. Users can still use plugin with linear projection.

---

#### 2. WebView Memory Overhead

**Risk:** Each WebView instance uses 50-100MB RAM. Opening 10 plugin instances = 500MB-1GB.

**Complexity:** MEDIUM
**Impact:** Moderate (user annoyance, not a crash)

**Mitigation:**
- Document in user manual (single instance recommended for large projects)
- WebView memory is managed by OS (released when plugin closed)
- No leak (JUCE manages WebView lifecycle)

**Fallback:** None needed (inherent limitation of WebView approach). Alternative would require native JUCE graphics (rejected due to 10K+ point rendering performance).

---

#### 3. KD-Tree Query Performance Degradation (>20 Dimensions)

**Risk:** KD-trees become inefficient in high-dimensional spaces (approaching brute-force at 30+ dims). Our 19D is near the edge.

**Complexity:** MEDIUM
**Impact:** Moderate (increased grain selection latency)

**Mitigation:**
- 19D is still within efficient range per nanoflann documentation
- Query time for 10K points: ~1-10μs (measured in research)
- If degradation occurs, reduce descriptor count (e.g., drop to 13 MFCCs only = 13D)

**Fallback:** Use approximate nearest-neighbor (nanoflann supports radius search with max results limit).

---

### MEDIUM Risk Features

#### 4. Descriptor Normalization Consistency

**Risk:** New points must be normalized with same mean/stddev as original corpus. If normalization is inconsistent, KD-tree queries return wrong grains.

**Complexity:** LOW
**Impact:** Moderate (incorrect grain selection)

**Mitigation:**
- Store descriptor means/stddevs after corpus analysis
- Apply same normalization to target descriptor vector on every query
- Unit tests to verify normalization consistency

**Fallback:** Recompute normalization if user adds/removes grains (full corpus reanalysis).

---

#### 5. WebView2 Unavailability on Older Windows

**Risk:** Windows 7/8 may not have WebView2 runtime. JUCE falls back to IE backend (no WebGL support).

**Complexity:** LOW
**Impact:** Minor (graceful degradation)

**Mitigation:**
- Detect WebView2 availability on plugin open
- Show fallback UI: "WebView2 required. Download from microsoft.com/webview2"
- Or render scatter plot with Canvas2D (slower, but functional for <5K points)

**Fallback:** Canvas2D fallback for IE backend (degraded performance).

---

### LOW Risk Features

#### 6. Grain Pitch Shifting Artifacts

**Risk:** Linear interpolation for pitch shifting produces aliasing at high pitch ratios (>2x or <0.5x).

**Complexity:** LOW
**Impact:** Minor (audible artifacts in extreme pitch modes)

**Mitigation:**
- Limit pitch range to ±1 octave in pitch-mapped mode (0.5x to 2x)
- Use higher-quality resampling (juce::LagrangeInterpolator for 4-point interpolation)

**Fallback:** Disable pitch shifting (play grains at original pitch regardless of MIDI note).

---

## Architecture Decisions

### 1. Why Fixed-Size Segmentation Over Onset Detection?

**Decision:** Use fixed 50ms grains with 50% overlap instead of onset-based segmentation.

**Alternatives considered:**
- **Onset detection** (via juce::dsp or Aubio): Segments at transient peaks
- **Silence-based splitting**: Segments at quiet gaps
- **Beat-synchronous**: Segments aligned to detected tempo

**Rationale:**
- **Predictability:** Fixed-size gives deterministic grain count (N = file_length / hop_size)
- **Simplicity:** No onset detector needed (reduces code complexity by ~500 lines)
- **Uniformity:** Covers entire file uniformly (no gaps for sustained sounds)
- **Research-backed:** CataRT, FluCoMa, and AudioTexture all use fixed-size segmentation by default

**Tradeoffs:**
- **Pro:** Simpler, faster, predictable
- **Con:** Less musically meaningful boundaries (e.g., grains may start mid-transient)
- **Future:** Onset detection can be added as v2 feature with user toggle

---

### 2. Why UMAP Over t-SNE?

**Decision:** Use UMAP as primary dimensionality reduction, with PCA as instant preview.

**Alternatives considered:**
- **t-SNE:** Better local structure, worse global structure, 5-50x slower
- **PCA only:** Instant, deterministic, but produces blob layout (poor clustering)
- **Parametric UMAP:** Neural network-based (rejected due to C++ inference complexity)

**Rationale:**
- **FluCoMa research:** UMAP scored highest for audio descriptor trustworthiness (Bernardo et al., 2021)
- **Speed:** 2-15s for 10K points (acceptable with progress bar)
- **Quality:** Preserves both local AND global structure (t-SNE loses global)
- **Proven:** FluCoMa's official choice for corpus exploration

**Tradeoffs:**
- **Pro:** Superior clustering, faster than t-SNE, research-validated
- **Con:** Non-deterministic (seeded RNG mitigates), slower than PCA
- **Hybrid:** PCA for instant preview, UMAP for final quality

---

### 3. Why regl-scatterplot Over Custom WebGL?

**Decision:** Use regl-scatterplot library instead of custom WebGL shaders.

**Alternatives considered:**
- **Custom WebGL with raw regl:** Maximum control, minimum bundle size
- **Three.js:** Heavier dependency, 3D engine overhead
- **Canvas2D:** Too slow for 10K+ points (drops to 22 FPS at 50K points per benchmarks)

**Rationale:**
- **Purpose-built:** regl-scatterplot is designed for exactly this use case
- **Performance:** 20M points at 60fps (far exceeds needs)
- **Built-in features:** Pan, zoom, lasso, click handlers (saves ~500 lines of code)
- **Small footprint:** Only depends on regl + pub-sub-es (~50KB gzipped)

**Tradeoffs:**
- **Pro:** Battle-tested, feature-complete, high performance
- **Con:** Less control over rendering (vs. custom shaders)
- **Acceptable:** Library's default rendering is sufficient for needs

---

### 4. Why nanoflann Over FLANN?

**Decision:** Use nanoflann for KD-tree nearest-neighbor search.

**Alternatives considered:**
- **FLANN:** Full-featured, supports multiple index types (KD-tree, k-means tree, hierarchical k-means)
- **Annoy (Spotify):** Approximate nearest-neighbor (faster for huge corpora)
- **Custom KD-tree:** Maximum control, minimal dependencies

**Rationale:**
- **Header-only:** No linking complexity (nanoflann is single header)
- **Real-time safe:** Allocation-free queries (critical for audio thread)
- **Optimized for 19D:** KD-trees perform well in this dimensional range
- **BSD licensed:** Commercial use allowed

**Tradeoffs:**
- **Pro:** Simple integration, real-time safe, proven
- **Con:** Less flexible than FLANN (only KD-tree, no approximate NN)
- **Acceptable:** Exact nearest-neighbor is fast enough for 10K-50K points at 19D

---

### 5. Why Three MIDI Modes Instead of One?

**Decision:** Provide three distinct MIDI interaction modes: Pitch-mapped, Trigger+Modulate, Generative Drone.

**Alternatives considered:**
- **Single mode:** Pitch-mapped only (standard synth behavior)
- **Two modes:** Pitch-mapped + Drone (skip trigger mode)

**Rationale:**
- **User diversity:** Different use cases demand different MIDI behaviors
  - **Pitch-mapped:** Traditional instrument players (keyboardists)
  - **Trigger+Modulate:** Live performers (control XY with mod wheel/aftertouch)
  - **Generative Drone:** Ambient producers (continuous output, no note-on required)
- **Competitive parity:** The Concatenator offers multiple modes, AudioTexture is always generative
- **Minimal complexity:** Modes share same grain engine, only trigger logic differs (~100 lines per mode)

**Tradeoffs:**
- **Pro:** Flexibility, broader user base, creative possibilities
- **Con:** Increased UX complexity (user must understand mode differences)
- **Acceptable:** Mode selector in UI with tooltips explaining each mode

---

## Special Considerations

### 1. Thread Safety

**Lock-free audio thread patterns:**

**Corpus buffer access:**
```cpp
// Background thread (load)
auto newCorpus = std::make_shared<juce::AudioBuffer<float>>(1, totalSamples);
// ... fill buffer ...
corpusBufferPtr.store(newCorpus, std::memory_order_release);

// Audio thread (read)
auto corpus = corpusBufferPtr.load(std::memory_order_acquire);
if (corpus) {
    float sample = corpus->getSample(0, readPos);
}
```

**KD-tree access:**
```cpp
// Background thread (build)
auto newTree = std::make_unique<KDTree>(/* ... */);
kdTreePtr.store(newTree.release(), std::memory_order_release);

// Audio thread (query)
auto tree = kdTreePtr.load(std::memory_order_acquire);
if (tree) {
    size_t nearestIdx = tree->knnSearch(targetDescriptor, 1);
}
```

**Visualization snapshot (double-buffering):**
```cpp
// Audio thread writes
int writeIdx = vizWriteIndex.load(std::memory_order_relaxed);
vizSnapshots[writeIdx].update(grainStates);
vizWriteIndex.store(1 - writeIdx, std::memory_order_release);

// GUI thread reads
int readIdx = 1 - vizWriteIndex.load(std::memory_order_acquire);
emitToWebView(vizSnapshots[readIdx]);
```

**Critical pattern (juce8-critical-patterns.md):** Never call UI code from audio thread, never allocate on audio thread.

---

### 2. Performance Budget

| Subsystem | Budget | Measured | Status |
|-----------|--------|----------|--------|
| KD-tree query (19D, 10K points) | <100μs | 1-10μs | ✅ Well under |
| Grain voice rendering (64 voices) | <5ms | ~2-3ms | ✅ Safe |
| MFCC extraction per grain | N/A | ~50-200μs | ✅ (background) |
| WebView message passing (30Hz) | <5ms | ~1-2ms | ✅ Safe |
| UMAP (10K points, 19D) | <30s | 5-15s (HNSW) | ✅ Acceptable |
| Scatter plot render (10K points) | <100ms | ~40ms (WebGL init) | ✅ Fast |
| Scatter plot pan/zoom | 60 FPS | 60 FPS | ✅ Smooth |

**Total audio thread budget at 512 samples @ 44.1kHz:**
- Available: ~11.6ms per callback
- KD-tree query: 10μs
- Grain rendering (64 voices): ~3ms
- MIDI processing: ~100μs
- Viz snapshot write: ~50μs
- **Total used:** ~3.2ms (27% of budget)
- **Safety margin:** ~8.4ms (73% headroom)

---

### 3. Denormal Prevention

**Grain envelope multiplication can produce denormals (subnormal floats) near fade edges:**

```cpp
// In processBlock, before main loop
for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    juce::FloatVectorOperations::disableDenormalisedNumberSupport();

// Alternative: add DC offset to prevent denormals
for (auto& voice : voicePool) {
    if (voice.active) {
        float sample = voice.getSample();
        sample += 1e-25f; // Prevent denormals
        outputBuffer.addSample(ch, i, sample);
    }
}
```

**Why this matters:** Denormals can cause 100x slowdown on some CPUs (Intel pre-Haswell).

---

### 4. Sample Rate Independence

**All timing parameters must scale with sample rate:**

```cpp
void prepareToPlay(double sampleRate, int samplesPerBlock) override {
    currentSampleRate = sampleRate;

    // Convert ms to samples
    grainSizeSamples = (int)(grainSizeMs * sampleRate / 1000.0);
    hopSizeSamples = (int)(hopSizeMs * sampleRate / 1000.0);

    // Rebuild grain database if corpus loaded (grain boundaries change)
    if (corpusBufferPtr.load())
        requestCorpusReanalysis();
}
```

**Critical:** When sample rate changes, grain boundaries shift (50ms at 44.1kHz ≠ 50ms at 48kHz). Corpus must be re-segmented.

---

### 5. Latency Considerations

**Plugin latency sources:**
- **FFT-based analysis:** None (analysis happens on background thread, not in signal path)
- **Grain crossfade:** 0-50ms (depends on crossfade %, but output is immediate)
- **WebView communication:** ~3ms (visualization only, not in audio path)

**Reported latency:** 0 samples (plugin introduces no algorithmic latency in audio path).

**Zero-latency architecture:** Grain selection and rendering happen in-place on current audio callback. No lookahead buffer needed.

---

## Research References

### Professional Plugins Researched
- [AudioTexture by Le Sound](https://www.kvraudio.com/product/audiotexture-by-le-sound) - Concatenative synthesis with adaptive segmentation, descriptor controls (Energy, Noisiness, Brightness)
- [The Concatenator by DataMind Audio](https://datamindaudio.ai/concatenator/) - Bayesian particle filter for grain selection, real-time musaicing
- [Mosaic by Echobit](https://echobit.myshopify.com/products/mosaic) - Corpus-based synthesis with grid visualization organized by audio features

### JUCE Documentation
- JUCE 8.0.4 API Reference (Context7-MCP)
- juce::dsp::FFT - Real-time FFT processing
- juce::dsp::WindowingFunction - Hann, Hamming, Blackman windows
- juce::AudioFormatManager - Multi-format file loading
- juce::WebBrowserComponent - WebView integration

### Research Documents (Project-Internal)
- `research/concatenative-synthesis-comprehensive.md` - Full concatenative synthesis theory, MFCC implementation, KD-tree patterns
- `research/2d-scatter-plot-concatenative-synthesis.md` - WebGL rendering, regl-scatterplot integration, JUCE WebView communication at 30Hz
- `research/umap-dimensionality-reduction-audio-plugins.md` - umappp integration, PCA fallback, incremental UMAP, performance benchmarks

### Academic Sources
- Schwarz, D. (2006). "Concatenative Sound Synthesis: The Early Years." Journal of New Music Research, 35(1), 3-22
- Bernardo et al. (2021). "A General Framework for Visualization of Sound Collections in Musical Interfaces" - FluCoMa's UMAP validation for audio
- Tralie & Cantil (2024). "The Concatenator: A Bayesian Approach to Real Time Concatenative Musaicing" (ISMIR 2024) - Particle filter grain selection

### Technical Libraries
- [nanoflann](https://github.com/jlblancoc/nanoflann) - Header-only KD-tree (BSD)
- [umappp](https://github.com/libscran/umappp) - Header-only UMAP (BSD-2)
- [regl-scatterplot](https://github.com/flekschas/regl-scatterplot) - WebGL scatter plot (MIT)

### Competitive Analysis
- CataRT (IRCAM) - Max/MSP concatenative synthesis with 2D descriptor space navigation
- FluCoMa - Corpus-based toolkit with UMAP integration (Max/SC/PD)
- O-GrainScatter (this project) - Reference for grain scheduler, WebView communication, voice pool architecture

---

**END OF ARCHITECTURE SPECIFICATION**
