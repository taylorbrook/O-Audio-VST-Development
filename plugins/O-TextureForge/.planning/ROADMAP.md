# O-TextureForge Implementation Roadmap

**Plugin Type:** Instrument (VST3/AU)
**Generated:** 2026-02-13
**JUCE Version:** 8.0.4

---

## Complexity Assessment

### Calculation Method

```
Complexity Score = min(param_count / 5, 2.0) + algorithm_count + feature_count
Capped at 5.0
```

### Extracted Metrics

**From BRIEF.md parameter table:**
- Parameter count: **12**
- Param score: min(12/5, 2.0) = **2.0** (capped)

**From ARCHITECTURE.md Core Components:**
1. MFCC Extraction (custom FFT pipeline)
2. Spectral Centroid extraction
3. Spectral Flatness extraction
4. RMS Energy extraction
5. Zero-Crossing Rate extraction
6. Fundamental Frequency (f0) extraction
7. Spectral Flux extraction
8. KD-tree nearest-neighbor search (nanoflann)
9. PCA dimensionality reduction (Eigen)
10. UMAP dimensionality reduction (umappp)
11. Polyphonic grain scheduler (64 voices)
12. WebGL scatter plot (regl-scatterplot)

**Algorithm count:** 12

**From ARCHITECTURE.md Feature Analysis (complexity flags):**
- ✅ **FFT/frequency domain:** MFCC + spectral descriptors (+1)
- ✅ **Multiband processing:** No (+0)
- ✅ **Modulation systems:** No LFO, but has descriptor weighting (+0)
- ✅ **External MIDI control:** Three MIDI modes with routing (+1)
- ✅ **File I/O:** Drag-and-drop audio loading (+1)
- ✅ **Multi-output routing:** No (stereo only) (+0)
- ✅ **Real-time visualization:** WebGL scatter 30Hz updates (+1)
- ✅ **Background thread computation:** UMAP (2-15s) with progress (+1)

**Feature complexity score:** 5

### Total Complexity Score

```
Total = 2.0 (params) + 12 (algorithms) + 5 (features) = 19.0
Capped at 5.0
Final Score: 5.0
```

**Classification:** COMPLEX (score = 5.0)

**Complexity Tier:** 6 (DEEP) - File I/O, custom DSP pipeline, real-time visualization, background UMAP, polyphonic synthesis, WebGL UI

---

## Implementation Strategy

**Decision:** Phase-based implementation (score ≥ 3.0)

O-TextureForge combines multiple high-complexity subsystems (descriptor extraction, KD-tree search, UMAP, WebGL visualization, polyphonic grain synthesis). Each subsystem must be validated independently before integration. Attempting single-pass implementation risks cascading failures.

**Rationale:**
- 7 custom descriptor extractors (MFCC + 6 spectral features) = high DSP risk
- UMAP integration (umappp) = new dependency with 2-15s computation time
- WebGL scatter plot (regl-scatterplot) = first use in this project
- KD-tree real-time audio thread queries = thread safety critical
- Three MIDI modes with different routing = integration complexity

**Staged approach allows:**
- Per-subsystem validation with test criteria
- Git commits per milestone (atomic progress tracking)
- Fallback points if UMAP or WebGL integration fails
- Parallel development (DSP engine can be built while UI is stubbed)

---

## Stage Breakdown

### Stage 0: Ideation & Research (COMPLETE)
**Status:** ✅ Complete
**Outputs:**
- BRIEF.md (plugin vision, parameters, aesthetic)
- REQUIREMENTS.md (functional + non-functional requirements)
- ARCHITECTURE.md (complete DSP specification with JUCE classes)
- ROADMAP.md (this document)

**Complexity score:** 5.0 (COMPLEX)
**Implementation strategy:** Staged (4 major stages)

---

### Stage 1: Foundation (Build System + Parameters)

**Goal:** Working plugin shell that loads in DAW and exposes all parameters.

**Tasks:**
1. Create CMakeLists.txt with JUCE 8 configuration
   - Format: VST3, AU, Standalone
   - Set `IS_SYNTH TRUE` (instrument plugin)
   - Set `NEEDS_WEB_BROWSER TRUE` (WebView UI required)
   - Set `NEEDS_MIDI_INPUT TRUE` (three MIDI modes)
2. Create PluginProcessor skeleton
   - Bus configuration: Output-only (stereo), no input bus
   - APVTS parameter layout (12 parameters)
   - Empty processBlock stub
3. Create PluginEditor skeleton
   - WebView placeholder ("Loading UI...")
4. Build and verify plugin loads in DAW
   - macOS: AU validation with `auval -v`, VST3 scan in Ableton
   - Windows: VST3 scan in Ableton/Reaper

**Test Criteria:**
- ✅ Plugin appears in DAW instrument browser
- ✅ GUI window opens without crash
- ✅ All 12 parameters visible in DAW automation list
- ✅ Parameter changes from DAW reflected in plugin (APVTS working)

**Estimated Time:** 2-4 hours

**Git Commit:** "feat: O-TextureForge Stage 1 - foundation complete (build system + parameters)"

---

### Stage 2: DSP Implementation (Audio Engine)

**Goal:** Complete concatenative synthesis engine with descriptor-based grain selection (no GUI yet).

**Phase breakdown:** 4 sub-phases (complex DSP system)

#### Phase 2.1: File Loading & Segmentation

**Tasks:**
1. Implement file loading on background thread
   - `juce::AudioFormatManager` setup (WAV, AIFF, MP3, FLAC)
   - `juce::Thread` subclass for async loading
   - Atomic `std::shared_ptr<AudioBuffer<float>>` for corpus buffer
2. Implement fixed-size grain segmentation
   - 50ms grains, 50% overlap (25ms hop)
   - Generate grain metadata array (startSample, durationSamples)
3. Add file drag-and-drop handling (PluginEditor)
   - FileDragAndDropTarget stub (no WebView integration yet)

**Test Criteria:**
- ✅ Load 1-minute audio file successfully (<1s load time)
- ✅ Grain count matches formula: floor((N - grainSize) / hopSize) + 1
- ✅ Grain boundaries do not exceed file length
- ✅ Audio thread can read corpus buffer without blocking

**Git Commit:** "feat: O-TextureForge Stage 2.1 - file loading & segmentation"

---

#### Phase 2.2: Descriptor Extraction

**Tasks:**
1. Implement MFCC extractor (custom)
   - FFT via `juce::dsp::FFT` (2048 samples)
   - Hann windowing via `juce::dsp::WindowingFunction`
   - Mel filterbank (40 filters, 20Hz-20kHz)
   - DCT transform (13 coefficients)
2. Implement spectral descriptors (custom)
   - Spectral Centroid (brightness)
   - Spectral Flatness (noisiness)
   - Spectral Flux (rate of change)
3. Implement time-domain descriptors
   - RMS Energy via `juce::Decibels`
   - Zero-Crossing Rate (custom)
   - Fundamental Frequency via autocorrelation (custom)
4. Run descriptor extraction on all grains (background thread)
5. Normalize descriptors (z-score across corpus)

**Test Criteria:**
- ✅ Extract 19D descriptor vector for each grain
- ✅ MFCC values in reasonable range (typically -10 to +10 after normalization)
- ✅ Spectral centroid correlates with perceived brightness (manual listening test)
- ✅ RMS energy matches perceived loudness
- ✅ Extraction completes in <5 seconds for 1000 grains

**Git Commit:** "feat: O-TextureForge Stage 2.2 - descriptor extraction (19D per grain)"

---

#### Phase 2.3: KD-Tree Nearest-Neighbor Search

**Tasks:**
1. Integrate nanoflann (header-only, add to CMakeLists)
2. Implement descriptor adaptor for std::vector<GrainMetadata>
3. Build KD-tree on background thread after descriptor extraction
4. Implement real-time query from audio thread
   - Target descriptor from macro knobs (Energy, Brightness, Texture)
   - k=1 nearest neighbor query (allocation-free)
5. Atomic pointer swap for KD-tree (thread-safe handoff)

**Test Criteria:**
- ✅ KD-tree builds in <10ms for 1000 grains
- ✅ Query returns grain index in <10μs
- ✅ Varying macro knobs selects perceptually different grains
- ✅ Audio thread never blocks on KD-tree operations

**Git Commit:** "feat: O-TextureForge Stage 2.3 - KD-tree nearest-neighbor search"

---

#### Phase 2.4: Polyphonic Grain Scheduler

**Tasks:**
1. Implement voice pool (64 voices, pre-allocated)
2. Implement grain rendering with crossfading
   - Hann window envelope generation
   - Overlap-add synthesis
   - Pitch shifting via linear interpolation
3. Implement voice allocation (first-free, then oldest-steal)
4. Implement three MIDI modes:
   - Mode 1: Pitch-mapped (C3 = original, velocity → energy)
   - Mode 2: Trigger + Modulate (velocity → variation, mod wheel → X, aftertouch → Y)
   - Mode 3: Generative Drone (internal timer, MIDI CC → parameters)
5. Connect grain scheduler to KD-tree queries

**Test Criteria:**
- ✅ Mode 1: Playing C3 triggers grain at original pitch
- ✅ Mode 1: Playing C4 triggers grain 1 octave higher
- ✅ Mode 2: Velocity 127 selects from wider radius than velocity 64
- ✅ Mode 3: Continuous output without MIDI notes
- ✅ Crossfading eliminates clicks between grains
- ✅ Up to 64 simultaneous grains without glitches

**Git Commit:** "feat: O-TextureForge Stage 2.4 - polyphonic grain scheduler (3 MIDI modes)"

---

**Stage 2 Complete Test Criteria:**
- ✅ Load audio file → hear grains playing
- ✅ Adjust Energy knob → timbral character shifts (loud vs quiet grains)
- ✅ Adjust Brightness knob → spectral character shifts (dark vs bright grains)
- ✅ Switch MIDI modes → behavior changes as expected
- ✅ Plugin runs for 5 minutes without audio glitches or crashes

**Estimated Time:** 16-24 hours (spread across 4 phases)

**Final Git Commit:** "feat: O-TextureForge Stage 2 - complete DSP engine (descriptor-based grain synthesis)"

---

### Stage 3: GUI Implementation (WebGL Scatter Plot)

**Goal:** Interactive WebView UI with real-time scatter plot visualization.

**Phase breakdown:** 3 sub-phases (complex WebView integration)

#### Phase 3.1: WebView Setup & Static HTML

**Tasks:**
1. Add `juce_gui_extra` module dependency to CMakeLists
2. Create WebView resource directory structure:
   ```
   Source/ui/public/
   ├── index.html
   ├── js/
   │   ├── juce/
   │   │   ├── index.js (JUCE WebView bridge)
   │   │   └── check_native_interop.js (JUCE requirement)
   │   └── app.js
   └── css/
       └── ouaricon-naturalist.css
   ```
3. Add resources to CMakeLists via `juce_add_binary_data`
4. Implement resource provider in PluginEditor (explicit URL mapping per juce8-critical-patterns.md #8)
5. Create WebView with resource provider + native integration
6. Apply Ouaricon Naturalist aesthetic (aged paper, fern illustration, earth tones)
7. Add npm dependencies: regl-scatterplot, regl

**Test Criteria:**
- ✅ WebView loads HTML without blank screen
- ✅ HTML displays placeholder text ("O-TextureForge")
- ✅ Ouaricon Naturalist styling applied (aged paper background visible)
- ✅ Console shows no resource loading errors (check with Safari/Edge DevTools)
- ✅ index.js and check_native_interop.js loaded successfully

**Git Commit:** "feat: O-TextureForge Stage 3.1 - WebView setup with Ouaricon Naturalist UI"

---

#### Phase 3.2: WebGL Scatter Plot Rendering

**Tasks:**
1. Integrate regl-scatterplot in app.js
   - Initialize scatterplot instance
   - Configure colormap (pitch: blue → red)
   - Configure size mapping (energy: small → large)
2. Implement corpus load flow (C++ → JS)
   - `withNativeFunction("getCorpusData")` returns grain array as JSON
   - Format: `[[x, y, pitch, energy], ...]` (regl-scatterplot format)
   - Call on file load complete
3. Implement PCA reduction (C++, Eigen)
   - Compute instantly (<100ms)
   - Send to WebView via native function
   - Render scatter plot with PCA layout
4. Add pan/zoom/click handlers (regl-scatterplot built-in)
5. Implement grain selection callback (JS → C++)
   - `withNativeFunction("selectGrain", grainIndex)`
   - Update scatter cursor position parameter in APVTS

**Test Criteria:**
- ✅ Scatter plot renders 1000 grains at 60 FPS
- ✅ Click on grain triggers that grain (hear audio change)
- ✅ Pan/zoom works smoothly without lag
- ✅ PCA layout shows visible structure (not random blob)
- ✅ Color gradient shows pitch variation (blue = low, red = high)

**Git Commit:** "feat: O-TextureForge Stage 3.2 - WebGL scatter plot with PCA layout"

---

#### Phase 3.3: Real-Time Visualization & UMAP

**Tasks:**
1. Implement visualization snapshot system (double-buffer, lock-free)
   - Cursor position (X, Y)
   - Active grains (indices + envelope values)
2. Implement GUI timer @ 30Hz
   - Read snapshot from audio processor
   - Build JSON string
   - `emitEventIfBrowserIsVisible("vizUpdate", json)`
3. Implement JS event handler
   - Update Canvas2D cursor overlay (crosshair + radius circle)
   - Pulse active grains (opacity modulated by envelope)
4. Integrate umappp (C++, background thread)
   - FetchContent in CMakeLists (umappp + dependencies)
   - Compute UMAP after PCA (2-15s with progress reporting)
   - Animate transition from PCA to UMAP layout (regl-scatterplot transitions)
5. Add progress bar for UMAP computation
   - Epoch-by-epoch progress via `AsyncUpdater`
   - Display in WebView: "Computing optimal layout... 60%"

**Test Criteria:**
- ✅ Cursor position updates at 30Hz without lag
- ✅ Active grains pulse in sync with audio envelopes
- ✅ UMAP computation completes in <15s for 5000 grains
- ✅ Scatter plot smoothly transitions from PCA to UMAP layout
- ✅ UMAP layout shows tighter clustering than PCA (perceptual validation)
- ✅ Progress bar shows during UMAP, disappears when complete

**Git Commit:** "feat: O-TextureForge Stage 3.3 - real-time viz (30Hz) + UMAP layout"

---

**Stage 3 Complete Test Criteria:**
- ✅ Load file → scatter plot appears within 1 second (PCA)
- ✅ Wait 5-15 seconds → scatter plot refines to UMAP layout
- ✅ Click anywhere on scatter → hear corresponding grain
- ✅ Watch cursor move in scatter as grains play
- ✅ Active grains pulse with orange highlight
- ✅ Zoom into cluster → reveals fine-grained timbral structure

**Estimated Time:** 12-20 hours (spread across 3 phases)

**Final Git Commit:** "feat: O-TextureForge Stage 3 - complete WebGL UI (scatter plot + real-time viz)"

---

### Stage 4: Integration & Polish

**Goal:** Final integration, cross-platform testing, preset system, manual.

**Tasks:**
1. Add preset system
   - Factory presets demonstrating each MIDI mode
   - Save/load corpus file path in preset
2. Windows WebView2 integration
   - Add `NEEDS_WEBVIEW2 TRUE` to CMakeLists
   - Add `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` compile definition
   - Add WebView2 user data folder (temp directory per juce8-critical-patterns.md #12)
3. Cross-platform testing
   - macOS: AU validation (`auval -v`), VST3 in Ableton/Logic
   - Windows: VST3 in Ableton/FL Studio/Reaper
4. Performance profiling
   - Measure KD-tree query time (target <10μs)
   - Measure grain rendering time (target <3ms for 64 voices)
   - Measure WebView update time (target <5ms at 30Hz)
5. Edge case handling
   - No file loaded (show "Drop audio file here" in WebView)
   - Invalid file format (show error message)
   - Corpus too large (>100MB, show warning)
   - UMAP cancellation (allow user to stop and keep PCA layout)
6. Documentation
   - User manual (markdown) with MIDI mode explanations
   - Developer notes (algorithm references)

**Test Criteria:**
- ✅ Plugin passes AU validation on macOS (all tests)
- ✅ Plugin loads in Ableton/Logic/Reaper without errors
- ✅ Windows VST3 shows scatter plot (WebView2 working)
- ✅ Load 10MB file → works without crash
- ✅ Load 100MB file → shows warning but still works
- ✅ Cancel UMAP mid-computation → PCA layout remains functional
- ✅ All 12 parameters respond to host automation

**Estimated Time:** 8-12 hours

**Git Commit:** "feat: O-TextureForge Stage 4 - integration & polish complete (v1.0.0 ready)"

---

## Risk Assessment Per Stage

### Stage 1: Foundation
**Risk Level:** LOW
**Potential Issues:**
- CMakeLists syntax errors (easily fixed)
- APVTS parameter naming conflicts (validate with grep)

**Mitigation:** Follow O-GrainScatter CMakeLists pattern exactly.

---

### Stage 2: DSP Implementation
**Risk Level:** HIGH

**Phase 2.2 (Descriptor Extraction):**
- **Risk:** MFCC implementation produces NaN values or incorrect range
- **Mitigation:** Unit tests with known audio files, validate against Python librosa output
- **Fallback:** Use simpler descriptors (spectral centroid + energy only = 2D, skip MFCCs)

**Phase 2.3 (KD-Tree):**
- **Risk:** Query time exceeds audio thread budget (>100μs at 19D)
- **Mitigation:** Profile with Tracy/Instruments, reduce descriptor count if needed
- **Fallback:** Use approximate nearest-neighbor (nanoflann radius search)

**Phase 2.4 (Grain Scheduler):**
- **Risk:** Crossfading produces clicks or phase cancellation
- **Mitigation:** Test with sine wave corpus (easiest to hear artifacts)
- **Fallback:** Use equal-power crossfade instead of linear Hann

---

### Stage 3: GUI Implementation
**Risk Level:** HIGH

**Phase 3.2 (WebGL Scatter Plot):**
- **Risk:** regl-scatterplot doesn't integrate with JUCE BinaryData (npm module loading)
- **Mitigation:** Bundle regl-scatterplot via webpack/browserify into single app.js file
- **Fallback:** Use Canvas2D with spatial hashing for click detection (slower but functional)

**Phase 3.3 (UMAP):**
- **Risk:** umappp takes >60s for large corpora (user perceives as freeze)
- **Mitigation:** Use HNSW approximate neighbors (78Spinoza/UMAP benchmarks show 80x speedup)
- **Fallback:** PCA-only mode (instant, acceptable quality for v1)

**Phase 3.3 (Real-Time Viz):**
- **Risk:** 30Hz WebView updates cause UI stuttering on Windows
- **Mitigation:** Profile with Windows Performance Analyzer, reduce update rate to 15Hz if needed
- **Fallback:** Static scatter plot (no real-time cursor, click-to-play only)

---

### Stage 4: Integration & Polish
**Risk Level:** MEDIUM

**Windows WebView2:**
- **Risk:** WebView2 not available on user's system (Windows 7/8)
- **Mitigation:** Detect WebView2 availability, show download link if missing
- **Fallback:** Canvas2D fallback for IE backend (degraded but functional)

**Cross-Platform Audio:**
- **Risk:** Sample rate mismatch causes grain boundary shifts
- **Mitigation:** Re-segment corpus on sample rate change in prepareToPlay
- **Fallback:** Document limitation: "Reloading required if DAW sample rate changes"

---

## Critical Path

The following subsystems are on the critical path (any failure blocks completion):

1. **MFCC Extraction (Phase 2.2)** → Without MFCCs, descriptor space collapses to 6D (poor clustering)
   - **Dependency:** juce::dsp::FFT
   - **Risk:** HIGH (custom implementation, easy to introduce bugs)

2. **KD-Tree Integration (Phase 2.3)** → Without KD-tree, grain selection becomes O(N) brute-force (unacceptable latency)
   - **Dependency:** nanoflann
   - **Risk:** MEDIUM (header-only, but 19D performance must be validated)

3. **UMAP Integration (Phase 3.3)** → Without UMAP, scatter plot shows PCA blob (acceptable but inferior UX)
   - **Dependency:** umappp (+ Eigen, knncolle, CppIrlba)
   - **Risk:** HIGH (complex dependency chain, long computation time)
   - **Fallback:** PCA-only mode (acceptable for v1 if UMAP fails)

4. **WebGL Scatter Plot (Phase 3.2)** → Without scatter plot, plugin has no visual feedback (dealbreaker for concept)
   - **Dependency:** regl-scatterplot (npm)
   - **Risk:** MEDIUM (npm bundling into JUCE BinaryData, but proven feasible)
   - **Fallback:** Canvas2D (degraded performance, but functional)

**Non-critical (can be deferred to v2 if needed):**
- Grain size parameter (fixed 50ms acceptable)
- Variation parameter (fixed k=1 nearest acceptable)
- Generative Drone mode (Pitch-mapped mode sufficient for v1)

---

## Implementation Notes

### Dependency Management

**CMakeLists.txt strategy:**
```cmake
# Header-only dependencies via FetchContent
include(FetchContent)

# nanoflann (KD-tree)
FetchContent_Declare(
    nanoflann
    GIT_REPOSITORY https://github.com/jlblancoc/nanoflann
    GIT_TAG v1.5.0
)
FetchContent_MakeAvailable(nanoflann)

# umappp (UMAP)
FetchContent_Declare(
    umappp
    GIT_REPOSITORY https://github.com/libscran/umappp
    GIT_TAG v3.2.0
)
FetchContent_MakeAvailable(umappp)

target_link_libraries(O-TextureForge PRIVATE
    libscran::umappp
    # nanoflann is header-only, no linking needed
)
```

**npm dependencies (bundled into app.js):**
```bash
cd Source/ui
npm install regl-scatterplot regl pub-sub-es
npm run build  # Webpack bundles into single app.js for BinaryData
```

---

### Testing Strategy

**Per-phase validation:**
- Unit tests for descriptor extraction (validate against known audio)
- Performance benchmarks for KD-tree query time (target <10μs)
- Manual listening tests for grain selection quality
- Visual inspection of scatter plot layout (PCA vs UMAP clustering)

**Integration testing:**
- Load 1MB, 10MB, 100MB files (stress test)
- Sustained playback for 10 minutes (memory leak detection)
- Parameter automation from DAW (APVTS thread safety)
- Open 5 plugin instances simultaneously (memory overhead)

**Cross-platform validation:**
- macOS: auval -v, Ableton Live, Logic Pro
- Windows: Ableton Live, FL Studio, Reaper

---

### Binary Size Impact

**Estimated plugin binary size:**
- Base JUCE (minimal): ~5MB
- juce_dsp (FFT): +500KB
- juce_gui_extra (WebView): +1MB
- Eigen headers (umappp dependency): +0 (compile-time only)
- nanoflann headers: +0 (compile-time only)
- WebView HTML/JS/CSS (BinaryData): +200KB
- regl-scatterplot bundle: +100KB
- **Total estimated:** ~7-8MB (acceptable for desktop plugin)

---

## Summary

**O-TextureForge is a HIGH-COMPLEXITY plugin (score 5.0/5.0)** requiring staged implementation across 4 major stages with 10 distinct phases. The critical path includes custom MFCC extraction, KD-tree integration, UMAP dimensionality reduction, and WebGL scatter plot visualization. Each phase has clear test criteria and git commit milestones.

**Unique challenges:**
- First plugin in this project to use UMAP (umappp)
- First to use KD-tree on audio thread (real-time safety critical)
- First to render 10K+ points in WebView (WebGL required)
- Most complex descriptor pipeline (19D vs typical 3-5D)

**Why this is worth the complexity:**
- **First VST/AU with interactive WebGL scatter plot for concatenative synthesis** (no competitor offers this)
- Research-validated approach (FluCoMa, IRCAM CataRT)
- High-value instrument (ambient/drone producers, sound designers)
- Premium price point justified ($79-129 range)

**Estimated total time:** 38-60 hours spread across 4 stages.

**Target release:** v1.0.0 with all features except multi-file layering, LFO modulation, path recorder (deferred to v2).

---

**END OF ROADMAP**
