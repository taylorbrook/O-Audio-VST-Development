# Stage 2: DSP Implementation - Research Synthesis

**Date:** 2026-02-14
**Confidence:** HIGH (all findings verified against JUCE 8.0.4 source + existing codebase)

## Overview

Stage 2 implements the complete DSP pipeline for O-TextureForge's concatenative synthesis engine across 4 sub-phases:

| Phase | Scope | Key Findings |
|-------|-------|--------------|
| 2.1 | File loading & segmentation | LagrangeInterpolator for resampling, atomic raw pointer handoff (C++17), juce::Thread subclass |
| 2.2 | Descriptor extraction (19D) | `performFrequencyOnlyForwardTransform` (NOT `performRealOnlyForwardTransform`), normalise=false for Hann |
| 2.3 | KD-tree search | nanoflann v1.9.0, **DIM=19 compile-time** for allocation-free queries, L2_Simple_Adaptor |
| 2.4 | Grain scheduler + MIDI | 64-voice pool (O-GrainScatter pattern), Drone mode first, sample-accurate MIDI via range-for |

---

## Critical Findings

### 1. ARCHITECTURE.md FFT Data Layout Error
The ARCHITECTURE.md contains an **incorrect magnitude extraction pattern** for JUCE's FFT:
```cpp
// WRONG (ARCHITECTURE.md)
float real = fftData[k];
float imag = (k < FFT_SIZE/2) ? fftData[FFT_SIZE - k] : 0;

// CORRECT: JUCE outputs interleaved Complex<float>
// BUT: use performFrequencyOnlyForwardTransform instead (outputs magnitudes directly)
```
**Resolution:** Use `performFrequencyOnlyForwardTransform(data, true)` which returns magnitudes in `data[0..FFT_SIZE/2]`. No manual complex-to-magnitude conversion needed.

### 2. nanoflann DIM Must Be 19 at Compile Time
nanoflann's `array_or_vector<DIM, T>` metaprogramming means:
- **DIM=19** (compile-time) → `std::array` → allocation-free queries (audio-thread safe)
- **DIM=-1** (runtime) → `std::vector` → heap allocation on every query (NOT safe)

This is the single most important implementation detail for Phase 2.3.

### 3. WindowingFunction normalise Must Be False
JUCE's default `normalise=true` scales the window so its sum equals `size` (for spectrum analyzers). For MFCC extraction, use `false` to get the standard Hann window coefficients.

### 4. C++17 Constraint (No atomic shared_ptr)
The project uses C++17 (set by JUCE 8.0.4). `std::atomic<std::shared_ptr<>>` requires C++20. Use `std::atomic<SharedCorpus*>` + `std::shared_ptr<SharedCorpus>` on the message thread instead.

### 5. Single-Frame Analysis for 50ms Grains
At 44.1kHz, 50ms = 2205 samples -- barely larger than one 2048-point FFT frame. Single-frame analysis is correct and sufficient. Spectral flux should be set to 0 for grains < 2*FFT_SIZE.

---

## Phase 2.1: File Loading & Grain Segmentation

### Standard Stack
| Component | Tool | Notes |
|-----------|------|-------|
| File reading | `juce::AudioFormatManager` + `registerBasicFormats()` | WAV, AIFF, FLAC, OGG, MP3 (CoreAudio on macOS) |
| Resampling | `juce::LagrangeInterpolator` | 4-point, good quality, NOT ResamplingAudioSource |
| Background thread | `juce::Thread` subclass | Check `threadShouldExit()` between major ops |
| Lock-free handoff | `std::atomic<SharedCorpus*>` + `std::shared_ptr` | Message thread holds shared_ptr, audio reads raw ptr |
| Drag-and-drop | `juce::FileDragAndDropTarget` on Editor | Works over WebView, native OS-level drops |

### Pipeline Order
```
Load file → Downmix to mono → Resample to DAW SR → Segment into grains → Handoff
```
Resample AFTER downmix to minimize work (1 channel instead of N).

### Key Patterns
- **Stereo downmix:** `addFrom()` + `applyGain(1.0f/numChannels)` (SIMD-optimized)
- **Memory management:** Release intermediate buffers early (`buffer = AudioBuffer<float>()`)
- **Previous corpus lifetime:** Keep `previousCorpus` shared_ptr alive for one swap cycle
- **State persistence:** Save file path in APVTS ValueTree child ("CORPUS"), reload async on restore
- **Re-segmentation:** Detect sample rate change in `prepareToPlay()`, trigger re-load

### Segmentation Formula
```cpp
int numGrains = (totalSamples >= grainSize)
    ? ((totalSamples - grainSize) / hopSize) + 1 : 0;
// hop = grainSize / 2 (50% overlap)
// Only full-length grains, no partial grains at end
```

### Source Files (Phase 2.1)
```
Source/dsp/
  SharedCorpus.h       // Immutable corpus struct
  CorpusLoader.h/.cpp  // juce::Thread subclass: load + downmix + resample + segment
```

**Detailed research:** `research-file-loading.md` (1070 lines)

---

## Phase 2.2: Descriptor Extraction (19D)

### Standard Stack
| Component | Tool | Notes |
|-----------|------|-------|
| FFT | `juce::dsp::FFT(11)` | 2048-point, order=11, vDSP-accelerated on macOS |
| Magnitudes | `performFrequencyOnlyForwardTransform(data, true)` | Returns magnitudes in data[0..1024] |
| Windowing | `WindowingFunction<float>(2048, hann, false)` | normalise=false for MFCC |
| Mel filterbank | Custom (40 triangular filters, 20Hz-Nyquist) | Pre-computed once in `prepare()` |
| DCT | Custom 13x40 matrix | Pre-computed, simple matrix multiply |

### 19D Descriptor Mapping
| Dim | Descriptor | Macro Knob | Computation |
|-----|-----------|------------|-------------|
| 0-12 | 13 MFCCs | (indirect) | FFT → mel filterbank → log → DCT |
| 13 | Spectral Centroid | Brightness | Weighted mean of frequencies |
| 14 | Spectral Flatness | Texture | Geometric/arithmetic mean ratio (log domain) |
| 15 | Spectral Flux | — | Half-wave rectified spectral difference (0 for short grains) |
| 16 | Spectral Rolloff | — | 85% energy threshold frequency |
| 17 | RMS Energy | Energy | sqrt(mean(x^2)) on raw samples |
| 18 | Zero-Crossing Rate | — | Sign changes / (N-1) on raw samples |

### Extraction Order (Single Grain)
1. Compute RMS and ZCR from **raw** samples (before any processing)
2. Copy to work buffer, apply pre-emphasis (alpha=0.97)
3. Apply Hann window
4. `performFrequencyOnlyForwardTransform(data, true)` → magnitudes[0..1024]
5. Compute spectral centroid, flatness, rolloff, flux from magnitudes
6. Apply mel filterbank (40 filters) → log compress → DCT → 13 MFCCs

### Z-Score Normalization
- Two-pass: compute means, then stddevs, then normalize
- Store `NormalizationStats` in Corpus for target mapping
- Guard against zero stddev: `if (stddev < 1e-10f) stddev = 1.0f`

### Performance Estimate
~75us per grain, ~750ms for 10K grains (single-threaded). Well within NFR-2 (10s limit).

### Source Files (Phase 2.2)
```
Source/dsp/
  DescriptorExtractor.h/.cpp  // Orchestrates full 19D extraction pipeline
  MFCCExtractor.h/.cpp        // FFT, mel filterbank, DCT (reusable)
```

**Detailed research:** `research-mfcc-descriptors.md` (1010 lines)

---

## Phase 2.3: KD-Tree Nearest-Neighbor Search

### Standard Stack
| Component | Tool | Notes |
|-----------|------|-------|
| KD-tree | nanoflann v1.9.0 | Header-only, FetchContent, CMake target `nanoflann::nanoflann` |
| Distance | `L2_Simple_Adaptor<float, Adaptor>` | Compile-time unrolled at DIM=19 |
| Query | `findNeighbors()` with `KNNResultSet` | Pre-allocated arrays, zero allocation |

### CMake Integration
```cmake
FetchContent_Declare(nanoflann
    GIT_REPOSITORY https://github.com/jlblancoc/nanoflann.git
    GIT_TAG v1.9.0)
set(NANOFLANN_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(NANOFLANN_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(nanoflann)
target_link_libraries(OuariconTextureForge PRIVATE nanoflann::nanoflann)
```

### Key Type Definition
```cpp
static constexpr int kDescriptorDims = 19;

using KDTree_t = nanoflann::KDTreeSingleIndexAdaptor<
    nanoflann::L2_Simple_Adaptor<float, GrainDescriptorAdaptor>,
    GrainDescriptorAdaptor,
    kDescriptorDims,  // CRITICAL: compile-time DIM
    uint32_t>;
```

### Immutable Corpus Pattern
Bundle audio buffer + grain metadata + adaptor + KD-tree into single `Corpus` struct. Build on background thread, publish via atomic pointer swap. Audio thread reads through raw pointer — zero ref-counting overhead.

### Macro Knob → Target Descriptor
```cpp
target[17] = (energy * 2.0f) - 1.0f;       // RMS Energy (z-score space)
target[13] = (brightness * 2.0f) - 1.0f;   // Spectral Centroid
target[14] = (texture * 2.0f) - 1.0f;       // Spectral Flatness
// All other dimensions = 0.0f (neutral)
```

### Performance at 19D
| Corpus Size | Expected k=1 Query | Notes |
|-------------|-------------------|-------|
| 1K grains | < 1 us | Comparable to brute force |
| 10K grains | ~2-10 us | 5-20x faster than brute force |
| 50K grains | ~10-50 us | Compile-time unrolling + early termination |

Fallback: `SearchParameters{.eps = 0.1}` for approximate search if needed.

### Source Files (Phase 2.3)
```
Source/dsp/
  KDTreeSearch.h/.cpp    // nanoflann adaptor, tree wrapper, query interface
  GrainMetadata.h        // GrainMetadata struct with 19D descriptors array
```

**Detailed research:** `research-nanoflann.md` (546 lines)

---

## Phase 2.4: Polyphonic Grain Scheduler + MIDI Modes

### Standard Stack
| Component | Tool | Notes |
|-----------|------|-------|
| Voice pool | `std::array<GrainVoice, 64>` | Pre-allocated, zero heap allocation |
| MIDI parsing | `juce::MidiMessage` + range-for on `MidiBuffer` | NOT deprecated Iterator |
| Gain | `juce::Decibels::decibelsToGain(db, -60.0f)` | Returns 0.0 at minimum |
| Denormals | `juce::ScopedNoDenormals` | RAII at top of processBlock |
| Buffer ops | `juce::FloatVectorOperations::multiply` | SIMD-accelerated output gain |

### GrainVoice Struct
```cpp
struct GrainVoice {
    bool active;
    int grainIndex, grainStartSample, grainLengthSamples;
    float readPosition, playbackRate, gain, envelope;
    int samplesElapsed, ageCounter;
    int midiNote, midiChannel;
};
```

### Voice Allocation: Round-Robin + Oldest-Steal
1. Scan from `nextVoice` index for first inactive voice
2. If all 64 active: steal voice with highest `ageCounter`
3. Assign `voice.ageCounter = ++globalAge` on allocation

### Three MIDI Modes (implement in order)

**Mode 2: Generative Drone (FIRST)**
- Internal timer: `samplesUntilNextGrain` countdown
- Density (1-64) → inter-grain interval = sampleRate/density
- Query KD-tree at current macro knob positions
- Playback rate = 1.0 (no pitch shifting)
- Still processes MIDI CC for parameter modulation

**Mode 0: Pitch-Mapped**
- Note-on → KD-tree query → spawn voice with pitch ratio
- Pitch: `pow(2, (midiNote - 60) / 12.0f)` (C3 = original)
- Velocity → gain and/or energy descriptor bias
- Note-off → unlink from MIDI note, let grain finish naturally

**Mode 1: Trigger + Modulate**
- Note-on → spawn grain at current scatter cursor + variation
- Velocity → variation radius
- No pitch shifting
- CC1 → Scatter X, Aftertouch → Scatter Y

### Grain Rendering
- Linear interpolation from corpus buffer at fractional read position
- Hann envelope: `0.5f * (1.0f - cos(twoPi * phase))` (inline, matching O-GrainScatter)
- Bounds check: deactivate if readPosition exceeds grain or corpus length
- Mono corpus → write same sample to both output channels
- Output gain applied per-block via FloatVectorOperations

### VizSnapshot for 30Hz WebView Updates
```cpp
struct VizSnapshot {
    struct ActiveGrain { int grainIndex; float envelope, readPositionNorm; };
    int activeCount;
    std::array<ActiveGrain, 64> activeGrains;
    float cursorX, cursorY;
};
```
Written by audio thread via double-buffer pattern (existing `vizWriteIndex` atomic).

### Source Files (Phase 2.4)
```
Source/dsp/
  GrainVoice.h          // Voice struct + pool class
  GrainScheduler.h/.cpp // MIDI routing, drone timer, voice spawning
```

**Detailed research:** `research-grain-scheduler.md` (887 lines)

---

## Common Anti-Patterns

| Anti-Pattern | Consequence | Correct Pattern |
|-------------|-------------|-----------------|
| `DIM=-1` in nanoflann template | Heap allocation on every query | Use `DIM=19` compile-time |
| `normalise=true` in WindowingFunction | Non-standard MFCCs | Explicitly pass `false` |
| `performRealOnlyForwardTransform` for magnitudes | Wrong data layout interpretation | Use `performFrequencyOnlyForwardTransform` |
| RMS/ZCR on windowed data | Wrong loudness/transient values | Compute from raw samples BEFORE processing |
| `radiusSearch()` on audio thread | Returns std::vector (allocates) | Use `findNeighbors()` with KNNResultSet |
| Destroy shared_ptr on audio thread | Heap deallocation in processBlock | Audio thread reads raw ptr only |
| `MidiBuffer::Iterator` | Deprecated in JUCE 8 | Use range-for with `cbegin()`/`cend()` |
| Velocity-0 note-on not handled | Voices never release | Check `isNoteOff() || (isNoteOn() && velocity == 0)` |

---

## Dependencies to Add to CMakeLists.txt

```cmake
# nanoflann KD-tree (header-only, Phase 2.3)
include(FetchContent)
FetchContent_Declare(nanoflann
    GIT_REPOSITORY https://github.com/jlblancoc/nanoflann.git
    GIT_TAG v1.9.0)
set(NANOFLANN_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(NANOFLANN_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(nanoflann)

# Link to plugin target
target_link_libraries(OuariconTextureForge PRIVATE nanoflann::nanoflann)
```

JUCE modules `juce_audio_formats` and `juce_dsp` are already linked in Stage 1.

---

## New Source Files (All of Stage 2)

```
Source/dsp/
  SharedCorpus.h             // Immutable corpus data bundle
  GrainMetadata.h            // GrainMetadata struct with 19D descriptors
  CorpusLoader.h/.cpp        // Background thread: load + downmix + resample + segment + extract + build tree
  MFCCExtractor.h/.cpp       // FFT pipeline, mel filterbank, DCT
  DescriptorExtractor.h/.cpp // Orchestrates 19D extraction (MFCCs + spectral + temporal)
  KDTreeSearch.h/.cpp        // nanoflann adaptor, tree build, allocation-free query
  GrainVoice.h               // Voice struct + pool (64 voices, round-robin + oldest-steal)
  GrainScheduler.h/.cpp      // MIDI mode routing, drone timer, voice spawning
```

---

## Open Questions for Planning Phase

1. **nanoflann exact CMake target name:** Confirmed `nanoflann::nanoflann` from v1.9.0 CMakeLists.txt
2. **Spectral rolloff threshold:** 85% (standard, per CONTEXT.md)
3. **Loading progress callback:** Recommended but not required for v1. Can emit WebView event from `MessageManager::callAsync` at each pipeline stage.
4. **eps (approximate search):** Start with 0 (exact). Tune if profiling shows >50us at 50K grains.
5. **Grain length at playback:** Use `min(grainSizeParam, grainDatabase[idx].durationSamples)`

---

## Sub-Research Documents

| File | Lines | Scope |
|------|-------|-------|
| `research-file-loading.md` | 1070 | AudioFormatManager, resampling, background thread, handoff, drag-and-drop, state persistence |
| `research-mfcc-descriptors.md` | 1010 | JUCE FFT API, windowing, mel filterbank, DCT, spectral descriptors, z-score normalization |
| `research-nanoflann.md` | 546 | nanoflann API, DIM=19 requirement, immutable corpus pattern, 19D performance analysis |
| `research-grain-scheduler.md` | 887 | Voice pool, 3 MIDI modes, sample-accurate processing, VizSnapshot, pitfalls |

**Total research:** ~3,500 lines across 4 documents.
