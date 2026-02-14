---
plugin: O-TextureForge
stage: 0
status: complete
last_updated: 2026-02-13
complexity_score: 5.0
staged_implementation: true
orchestration_mode: true
next_action: invoke_foundation_shell_agent
next_stage: 1
contract_checksums:
  brief: sha256:c73551fc5af221b929b41cda648951bda5684dafb9282ea2ff2b61ddc26bf03d
  requirements: sha256:2efd29edb5c0103c9c0d6e83f11cb57941b78e492d2c3429e8aec60ce361b432
  architecture: sha256:b73cc2c03d6bc0b987bd5c26df2d8ccaaf259989a7726b3f1fab8ec4df9a5db3
  roadmap: sha256:9719053b1115abda9a8ab9acd6244b5b8032bf3415476ce7d758e7ca6f1b7a0b
---

# O-TextureForge Status

## Current Position

Stage: 0 of 4 (Ideation) — complete
Status: Research & Planning complete, ready for implementation
Progress: [##..................] 10%

## Completed So Far

**Stage 0:** ✓ Complete
- Plugin type defined: Instrument (VST3/AU)
- Professional examples researched: 3 (AudioTexture, The Concatenator, Mosaic)
- JUCE modules identified: juce_dsp (FFT, windowing), juce_audio_formats, juce_gui_extra (WebView)
- Third-party libraries validated: nanoflann (KD-tree), umappp (UMAP), regl-scatterplot (WebGL)
- DSP feasibility verified: Custom MFCC extraction, 19D descriptor space, KD-tree <10μs queries
- Complexity score: 5.0 (COMPLEX - maximum)
- Complexity tier: 6 (DEEP research - file I/O, real-time viz, background UMAP)
- Strategy: Staged implementation (4 major stages with 10 phases)
- ARCHITECTURE.md documented (11 sections, 50+ pages)
- ROADMAP.md documented (complexity breakdown, phase-by-phase plan)

**Features researched (12 total):**
1. File loading & drag-and-drop (juce::AudioFormatManager)
2. Fixed-size grain segmentation (50ms grains, 50% overlap)
3. MFCC extraction (custom FFT pipeline, juce::dsp::FFT)
4. Spectral descriptor extraction (centroid, flatness, flux)
5. Time-domain descriptor extraction (RMS, ZCR, f0)
6. KD-tree nearest-neighbor search (nanoflann, 19D space)
7. PCA dimensionality reduction (Eigen, instant preview)
8. UMAP dimensionality reduction (umappp, 2-15s quality layout)
9. Polyphonic grain scheduler (64 voices, crossfading)
10. WebGL scatter plot (regl-scatterplot, 50K+ points at 60fps)
11. Real-time WebView communication (30Hz cursor + grain activity)
12. Three MIDI modes (Pitch-mapped, Trigger+Modulate, Generative Drone)

**High-risk features with fallbacks:**
- UMAP computation time → HNSW approximate neighbors (80x speedup) | Fallback: PCA-only
- MFCC extraction bugs → Unit tests vs librosa | Fallback: Simpler descriptors (2D)
- KD-tree query latency → Profile/optimize | Fallback: Approximate NN
- WebView2 unavailability → Detection + download link | Fallback: Canvas2D

## Next Steps

1. **Stage 1: Foundation (2-4 hours)**
   - Create CMakeLists.txt (IS_SYNTH TRUE, NEEDS_WEB_BROWSER TRUE, NEEDS_MIDI_INPUT TRUE)
   - Create PluginProcessor skeleton (output-only bus, 12 APVTS parameters)
   - Create PluginEditor skeleton (WebView placeholder)
   - Test: Plugin loads in DAW, parameters visible in automation

2. **Stage 2: DSP Implementation (16-24 hours, 4 phases)**
   - Phase 2.1: File loading & segmentation
   - Phase 2.2: Descriptor extraction (MFCCs + spectral + time-domain)
   - Phase 2.3: KD-tree integration
   - Phase 2.4: Polyphonic grain scheduler + 3 MIDI modes

3. **Stage 3: GUI Implementation (12-20 hours, 3 phases)**
   - Phase 3.1: WebView setup + Ouaricon Naturalist UI
   - Phase 3.2: WebGL scatter plot + PCA layout
   - Phase 3.3: Real-time viz (30Hz) + UMAP background computation

4. **Stage 4: Integration & Polish (8-12 hours)**
   - Windows WebView2 integration
   - Cross-platform testing
   - Preset system
   - User manual

**Total estimated time:** 38-60 hours

**To proceed:** Run `/implement O-TextureForge` (invokes foundation-shell-agent for Stage 1)

## Context to Preserve

**Key architectural decisions:**
- Fixed-size segmentation (not onset detection) - Simpler, deterministic
- UMAP primary + PCA fallback - Best clustering, instant preview
- regl-scatterplot (not custom WebGL) - Purpose-built, 20M point capacity
- nanoflann (not FLANN) - Header-only, allocation-free queries
- Three MIDI modes - Different use cases (keyboardists, performers, ambient producers)

**Critical patterns to follow:**
- WebView resource provider with explicit URL mapping (juce8-critical-patterns.md #8)
- ES6 module loading with type="module" (juce8-critical-patterns.md #21)
- IS_SYNTH TRUE for instrument (juce8-critical-patterns.md #22)
- Lock-free audio thread (atomic pointers, double-buffering)
- 30Hz WebView updates via emitEventIfBrowserIsVisible (O-GrainScatter pattern)

**Research resources:**
- `research/concatenative-synthesis-comprehensive.md` - MFCC implementation, KD-tree
- `research/2d-scatter-plot-concatenative-synthesis.md` - regl-scatterplot, WebView comm
- `research/umap-dimensionality-reduction-audio-plugins.md` - umappp integration
- O-GrainScatter source (voice pool, grain scheduler, WebView patterns)

**Dependencies to integrate:**
- CMake FetchContent: nanoflann, umappp (+ Eigen, knncolle)
- npm (bundled): regl-scatterplot, regl
- JUCE modules: juce_dsp, juce_audio_formats, juce_gui_extra

**Unique challenges:**
- First plugin in project to use UMAP
- First to use KD-tree on audio thread
- First to render 10K+ points in WebView
- Most complex descriptor pipeline (19D vs typical 3-5D)

**Why this is worth it:**
- **First VST/AU with interactive WebGL scatter plot for concatenative synthesis**
- Research-validated (FluCoMa, IRCAM CataRT)
- High-value instrument (ambient/drone, sound design, game audio)
- Premium price point ($79-129)

## Files Created
- ✅ plugins/O-TextureForge/.planning/BRIEF.md
- ✅ plugins/O-TextureForge/.planning/REQUIREMENTS.md
- ✅ plugins/O-TextureForge/.planning/research/ARCHITECTURE.md (11 sections, complete DSP spec)
- ✅ plugins/O-TextureForge/.planning/ROADMAP.md (complexity 5.0, staged plan)
- ✅ plugins/O-TextureForge/.planning/stages/0-ideation/CONTEXT.md (phase findings)

**Last Updated:** 2026-02-13
