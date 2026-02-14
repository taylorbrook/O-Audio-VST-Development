# Stage 0 (Ideation) - Research & Planning Complete

**Date:** 2026-02-13
**Agent:** research-planning-agent
**Complexity Score:** 5.0 (COMPLEX)
**Implementation Strategy:** Staged (4 major stages with 10 phases)

---

## What Was Accomplished

### Research Depth: DEEP (Tier 6 Complexity)

O-TextureForge required maximum research depth due to:
- **File I/O** with drag-and-drop
- **Custom DSP pipeline** (7 descriptor extractors: MFCCs + 6 spectral features)
- **Real-time KD-tree queries** on audio thread (allocation-free critical)
- **UMAP dimensionality reduction** (2-15s background computation)
- **WebGL scatter plot** (50K+ points at 60fps)
- **Polyphonic grain synthesis** (up to 64 simultaneous voices)
- **Three MIDI modes** with different routing strategies

### Professional Plugin Analysis

Researched three commercial concatenative synthesis plugins:

1. **AudioTexture (Le Sound)** - Adaptive segmentation, descriptor controls (Energy, Noisiness, Brightness)
2. **The Concatenator (DataMind Audio)** - Bayesian particle filter for grain selection, complexity independent of corpus size
3. **Mosaic (Echobit)** - Grid-based corpus visualization organized by audio features

**Key insight:** No existing VST/AU offers an interactive WebGL-accelerated 2D scatter plot for descriptor space navigation. This is O-TextureForge's primary differentiator.

### JUCE 8 API Validation

Validated all JUCE classes via Context7-MCP (authoritative JUCE 8 docs):
- ✅ `juce::dsp::FFT` - Real-time FFT for MFCC extraction
- ✅ `juce::dsp::WindowingFunction` - Hann windows for grain crossfading
- ✅ `juce::AudioFormatManager` - Multi-format file loading (WAV, AIFF, MP3, FLAC)
- ✅ `juce::WebBrowserComponent` - WebView integration with resource provider
- ✅ `juce::AudioProcessorValueTreeState` - 12 parameters with automation

### Third-Party Libraries Validated

**Header-only (CMake FetchContent):**
- **nanoflann** (BSD) - KD-tree for 19D descriptor space, allocation-free queries (<10μs)
- **umappp** (BSD-2) - UMAP dimensionality reduction (2-15s with progress reporting)
- **Eigen** (MPL2) - PCA instant preview + UMAP dependency

**npm (bundled via webpack):**
- **regl-scatterplot** (MIT) - WebGL scatter plot (20M points at 60fps, far exceeds needs)

### Architecture Decisions Made

**1. Fixed-size segmentation over onset detection**
- Rationale: Deterministic grain count, simpler implementation, uniformly covers entire file
- Research-backed: CataRT, FluCoMa, AudioTexture all use fixed-size by default

**2. UMAP primary + PCA fallback**
- Rationale: FluCoMa research shows UMAP has highest trustworthiness for audio descriptors (Bernardo et al., 2021)
- Fallback: PCA provides instant preview (<100ms) while UMAP computes (2-15s)

**3. regl-scatterplot over custom WebGL**
- Rationale: Purpose-built for scatter plots, 20M point capacity, built-in pan/zoom/lasso
- Alternative rejected: Canvas2D (too slow, drops to 22 FPS at 50K points per benchmarks)

**4. nanoflann over FLANN**
- Rationale: Header-only (no linking complexity), allocation-free queries (real-time safe)
- 19D is optimal range for KD-trees (efficiency degrades >20D)

**5. Three MIDI modes instead of one**
- Rationale: Different use cases demand different MIDI behaviors
  - **Pitch-mapped:** Traditional keyboardists
  - **Trigger+Modulate:** Live performers (mod wheel/aftertouch control)
  - **Generative Drone:** Ambient producers (continuous output, no note-on)

### Risks Identified & Mitigated

**HIGH Risk:**
1. **UMAP computation time (30-60s for 50K grains)**
   - Mitigation: Use HNSW approximate neighbors (80x speedup per benchmarks)
   - Fallback: PCA-only mode (acceptable quality for v1)

2. **MFCC extraction bugs (NaN values, incorrect range)**
   - Mitigation: Unit tests with known audio, validate against Python librosa
   - Fallback: Use simpler descriptors (centroid + energy only)

3. **KD-tree query latency (>100μs unacceptable)**
   - Mitigation: Profile with Tracy/Instruments, reduce descriptor count if needed
   - Fallback: Approximate NN (radius search)

**MEDIUM Risk:**
1. **Descriptor normalization consistency** - Store means/stddevs, apply to new points
2. **WebView2 unavailability (Windows 7/8)** - Show download link or Canvas2D fallback

**LOW Risk:**
1. **Grain pitch shifting artifacts** - Limit to ±1 octave, use 4-point Lagrange interpolation

### Existing Reference Plugin

**O-GrainScatter** provides proven patterns for:
- Polyphonic grain scheduler with voice pool (64 voices)
- WebView communication at 30Hz (double-buffered visualization snapshot)
- Lock-free audio thread → GUI thread communication
- JUCE WebView resource provider with explicit URL mapping

Studied source files:
- `plugins/O-GrainScatter/Source/dsp/GrainScheduler.h` - Voice allocation, crossfading
- `plugins/O-GrainScatter/Source/PluginProcessor.h` - Viz snapshot double-buffering
- `plugins/O-GrainScatter/Source/PluginEditor.cpp` - 30Hz timer + emitEventIfBrowserIsVisible

---

## Key Constraints & Assumptions

### Constraints
1. **Single file corpus** (v1) - Multi-file layering deferred to v2
2. **Fixed grain size** (50ms) - User-adjustable, but no adaptive segmentation
3. **Stereo output only** - No multi-channel routing (simplifies bus config)
4. **19D descriptor space** - Optimal for KD-trees, balanced richness vs performance
5. **Desktop only** - WebView + UMAP unsuitable for iOS/Android

### Assumptions
1. Users have WebView2 on Windows (or willing to install)
2. Corpus files <100MB (warn if larger)
3. Users accept 2-15s UMAP wait for large corpora
4. Scatter plot is primary UI (not optional)
5. Target users: ambient/drone producers, sound designers, game audio designers

---

## Implementation Strategy Rationale

**Why staged implementation (vs single-pass):**

O-TextureForge combines 5 high-complexity subsystems:
1. Custom descriptor extraction (7 algorithms)
2. KD-tree real-time queries (thread safety critical)
3. UMAP background computation (new dependency)
4. WebGL scatter plot (first use in project)
5. Polyphonic grain synthesis (64-voice scheduler)

Each subsystem must be validated independently before integration. Single-pass risks cascading failures.

**Stage breakdown:**
- **Stage 1:** Foundation (2-4h) - Build system, APVTS, plugin shell
- **Stage 2:** DSP Engine (16-24h, 4 phases) - File load, descriptors, KD-tree, grain scheduler
- **Stage 3:** GUI (12-20h, 3 phases) - WebView, scatter plot, real-time viz, UMAP
- **Stage 4:** Polish (8-12h) - Presets, Windows WebView2, cross-platform testing

**Total estimated:** 38-60 hours

**Git commits per phase:** Atomic progress tracking, rollback points if subsystem fails.

---

## What's Next (Stage 1: Foundation)

**Ready for implementation:** Yes

**Prerequisites met:**
- ✅ BRIEF.md (creative vision)
- ✅ REQUIREMENTS.md (functional + non-functional specs)
- ✅ ARCHITECTURE.md (complete DSP specification with JUCE classes)
- ✅ ROADMAP.md (complexity score 5.0, staged implementation plan)

**Stage 1 goal:** Working plugin shell that loads in DAW and exposes all 12 parameters.

**First task:** Create CMakeLists.txt with:
- Format: VST3, AU, Standalone
- `IS_SYNTH TRUE` (instrument plugin)
- `NEEDS_WEB_BROWSER TRUE` (WebView UI)
- `NEEDS_MIDI_INPUT TRUE` (three MIDI modes)
- Output-only bus configuration (no input bus for instrument)

**Expected outcome:** Plugin appears in DAW instrument browser, GUI opens, 12 parameters visible in automation list.

**Command to proceed:** `/implement O-TextureForge` (invokes foundation-shell-agent for Stage 1)

---

## Research Resources Utilized

**Project-internal research documents:**
- `research/concatenative-synthesis-comprehensive.md` - MFCC implementation, KD-tree patterns, descriptor theory
- `research/2d-scatter-plot-concatenative-synthesis.md` - WebGL rendering tech, regl-scatterplot integration, 30Hz WebView communication
- `research/umap-dimensionality-reduction-audio-plugins.md` - umappp integration, PCA fallback, performance benchmarks

**External research:**
- IRCAM CataRT documentation - Real-time descriptor space navigation
- FluCoMa UMAP validation - Audio descriptor trustworthiness research (Bernardo et al., 2021)
- Schwarz (2006) - "Concatenative Sound Synthesis: The Early Years" (foundational theory)
- Tralie & Cantil (2024) - "The Concatenator: A Bayesian Approach" (particle filter grain selection)

**Critical patterns document:**
- `troubleshooting/patterns/juce8-critical-patterns.md` - JUCE 8 non-negotiable patterns (WebView resource provider, ES6 module loading, IS_SYNTH flag, thread safety)

---

## Success Criteria Met

✅ ARCHITECTURE.md created with ALL 11 required sections
✅ ROADMAP.md created with complexity score (5.0) and staged implementation plan
✅ All 12 features from BRIEF identified and researched
✅ Every JUCE class validated via Context7-MCP (authoritative JUCE 8 docs)
✅ Every HIGH risk feature has documented fallback architecture
✅ Integration analysis covers dependencies, interactions, processing order, threads
✅ Processing chain shows complete signal flow (file load → descriptors → KD-tree → grains → audio output)
✅ State files updated (STATUS.md to be updated next)

---

**Stage 0 Status:** ✅ COMPLETE
**Next Stage:** Stage 1 (Foundation) - Ready to invoke foundation-shell-agent
