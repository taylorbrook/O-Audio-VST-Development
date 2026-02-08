# FluCoMa C++ Library Integration Research for Concatenative Synthesis

## Research Date: 2026-02-07

This document provides deep, practical research on integrating FluCoMa's C++ algorithms into a JUCE-based VST/AU plugin for concatenative synthesis. It covers architecture, API surface, build integration, real-time considerations, licensing, and pragmatic alternatives.

---

## 1. FluCoMa-Core C++ Library Architecture

### Repository: https://github.com/flucoma/flucoma-core

### Directory Structure

```
flucoma-core/
  include/flucoma/
    algorithms/
      public/          # 48 algorithm headers (the core DSP/ML algorithms)
      util/            # 28 utility headers (helpers, filters, distance funcs)
    clients/
      common/          # Shared client infrastructure
      nrt/             # 39 non-real-time (offline/buffer) client headers
      rt/              # 26 real-time (streaming) client headers
    data/
      FluidDataSet.hpp       # Labeled dataset container
      FluidDataSetSampler.hpp
      FluidIndex.hpp
      FluidJSON.hpp          # JSON serialization
      FluidMemory.hpp        # Custom allocator system
      FluidMeta.hpp
      FluidTensor.hpp        # N-dimensional tensor (NOT Eigen-backed)
      FluidTensor_Support.hpp
      SimpleDataSampler.hpp
      TensorTypes.hpp        # Type aliases (RealVector, RealMatrix, etc.)
    FluidVersion.hpp
  examples/
    describe.cpp       # Audio file descriptor extraction
    dataset.cpp        # Multi-file dataset building
    umap.cpp           # UMAP dimensionality reduction
  CMakeLists.txt       # Defines FLUID_DECOMPOSITION interface library
```

### Key Design Principles

- **Two-tier architecture**: `algorithms/` contains pure DSP/ML algorithms; `clients/` wraps them with I/O handling, parameter management, and host-environment glue
- **Interface library**: CMake target `FLUID_DECOMPOSITION` is an `INTERFACE` library -- it does not compile object files itself but propagates compilation requirements to consumers
- **Custom tensor system**: `FluidTensor<T, N>` is FluCoMa's own N-dimensional array, NOT backed by Eigen. It uses `rt::vector` internally and supports views, slicing, and raw pointer interop
- **Eigen used for compute**: Algorithms internally convert to Eigen types for linear algebra operations (matrix multiplication, norms, decompositions)

### Namespaces

```cpp
fluid::                         // Top-level namespace
fluid::algorithm::              // All algorithms (STFT, KDTree, UMAP, etc.)
fluid::index                    // Size/index type alias
fluid::RealVector               // FluidTensor<double, 1>
fluid::RealMatrix               // FluidTensor<double, 2>
fluid::ComplexVector             // FluidTensor<std::complex<double>, 2>
fluid::RealVectorView           // Non-owning view into RealVector
fluid::RealMatrixView           // Non-owning view into RealMatrix
fluid::FluidDataSet<>           // Labeled dataset container
```

### Header-Only Status

**Not header-only in practice.** While the algorithms are defined in headers (template-heavy), the library has compiled dependencies:
- `foonathan/memory` requires compilation
- `HISSTools_Library` has compiled components (FFT)
- `fmt` library needs compilation (unless header-only mode)

The CMake INTERFACE library model means you link against `FLUID_DECOMPOSITION` and it transitively pulls everything in.

### Dependencies (as of latest)

| Dependency | Purpose | License | Notes |
|---|---|---|---|
| **Eigen 3.4** | Linear algebra, matrix operations | MPL2 | Header-only, ~25MB headers |
| **HISSTools Library** | FFT backend, DSP utilities | BSD-3 | Uses Accelerate/vDSP on macOS |
| **Spectra 1.0.1** | Eigenvalue computation | MPL2 | Header-only, used for PCA/MDS |
| **nlohmann/json 3.11.3** | JSON parsing/serialization | MIT | Header-only |
| **foonathan/memory** | Custom memory allocators | Zlib | Compiled library |
| **fmt** | String formatting | MIT | Compiled or header-only |
| **Accelerate.framework** | macOS hardware-accelerated DSP | Apple | macOS only, linked by HISSTools |

### License

**BSD-3-Clause**, copyright University of Huddersfield. Fully compatible with commercial closed-source plugins. Requirements:
1. Retain copyright notice in source redistributions
2. Include copyright notice in binary distribution documentation
3. Don't use University of Huddersfield name for endorsement

Per the FluCoMa developers on discourse: "if you don't distribute the source or the binary that flucoma has provided then you can legally do whatever you want, because what you are distributing is your own." A simple license text file included with your plugin satisfies the requirements.

All dependencies also have permissive licenses (BSD-3, MPL2, MIT, Zlib). No GPL contamination.

### flucoma-core vs flucoma-juce

- **flucoma-core**: The algorithms and client wrappers. This is what you want.
- **flucoma-juce**: This repository does NOT exist (404). There is no official JUCE wrapper from the FluCoMa team.
- The host-specific repositories are: `flucoma-max`, `flucoma-pd`, `flucoma-sc`, `flucoma-cli`
- For JUCE integration, you use `flucoma-core` directly at the `fluid::algorithm` level

---

## 2. Key FluCoMa Algorithms for Concatenative Synthesis

### Algorithm-to-C++ Class Mapping

All classes live in `fluid::algorithm` namespace. Include path: `<flucoma/algorithms/public/ClassName.hpp>`

| FluCoMa Object | C++ Class | Header | Purpose |
|---|---|---|---|
| `fluid.bufstft~` | `STFT` / `ISTFT` | `STFT.hpp` | Forward/inverse STFT |
| `fluid.bufmfcc~` | `MelBands` + `DCT` | `MelBands.hpp`, `DCT.hpp` | MFCC extraction (composed) |
| `fluid.bufspectralshape~` | `SpectralShape` | `SpectralShape.hpp` | 7 spectral descriptors |
| `fluid.bufloudness~` | `Loudness` | `Loudness.hpp` | Loudness + true peak |
| `fluid.bufpitch~` | `YINFFT` | `YINFFT.hpp` | YIN pitch detection |
| `fluid.bufonsetslice~` | `OnsetSegmentation` | `OnsetSegmentation.hpp` | Onset detection/slicing |
| `fluid.bufstats~` | `MultiStats` | `MultiStats.hpp` | Statistical summarization |
| `fluid.kdtree~` | `KDTree` | `KDTree.hpp` | K-nearest-neighbor search |
| `fluid.umap~` | `UMAP` | `UMAP.hpp` | UMAP dimensionality reduction |
| `fluid.normalize~` | `Normalization` | `Normalization.hpp` | Min-max normalization |
| `fluid.standardize~` | `Standardization` | `Standardization.hpp` | Z-score standardization |
| `fluid.pca~` | `PCA` | `PCA.hpp` | PCA dimensionality reduction |
| `fluid.kmeans~` | `KMeans` | `KMeans.hpp` | K-means clustering |
| `fluid.mlpregressor~` | `MLP` | `MLP.hpp` | Neural network |

### Detailed API for Key Algorithms

#### STFT (include/flucoma/algorithms/public/STFT.hpp)

```cpp
fluid::algorithm::STFT stft{windowSize, fftSize, hopSize};
// or with window type and allocator:
// STFT stft{windowSize, fftSize, hopSize, windowType, alloc};

// Process a single frame:
fluid::ComplexVector frame(nBins);  // nBins = fftSize/2 + 1
fluid::RealVectorView window = audio(fluid::Slice(i * hopSize, windowSize));
stft.processFrame(window, frame);

// Extract magnitude:
fluid::RealVector magnitude(nBins);
stft.magnitude(frame, magnitude);
```

#### MelBands + DCT = MFCC Pipeline

```cpp
// NOTE: MFCC is NOT a single algorithm class. It is composed from MelBands + DCT.
// (Issue #222 tracks making it a standalone algorithm)

fluid::algorithm::MelBands bands{nBands, fftSize};
fluid::algorithm::DCT dct{nBands, nCoefs};

bands.init(minFreq, maxFreq, nBands, nBins, sampleRate, windowSize);
dct.init(nBands, nCoefs);

// Per frame:
fluid::RealVector mels(nBands);
fluid::RealVector mfccs(nCoefs);
bands.processFrame(magnitude, mels, false, false, true, FluidDefaultAllocator());
dct.processFrame(mels, mfccs);
```

#### SpectralShape (include/flucoma/algorithms/public/SpectralShape.hpp)

Computes 7 descriptors per frame: centroid, spread, skewness, kurtosis, rolloff, flatness, crest.

```cpp
fluid::algorithm::SpectralShape shape(FluidDefaultAllocator());

fluid::RealVector shapeDesc(7);
shape.processFrame(magnitude, shapeDesc, sampleRate,
                   0,      // minFreq
                   -1,     // maxFreq (-1 = Nyquist)
                   0.95,   // rolloff target
                   false,  // logFreq
                   false,  // usePower
                   FluidDefaultAllocator());
// shapeDesc contains: [centroid, spread, skewness, kurtosis, rolloff, flatness, crest]
```

#### Loudness (include/flucoma/algorithms/public/Loudness.hpp)

ITU-R BS.1770 aligned loudness with optional K-weighting and true peak.

```cpp
fluid::algorithm::Loudness loudness{windowSize};
loudness.init(windowSize, sampleRate);

fluid::RealVector loudnessDesc(2);
loudness.processFrame(window, loudnessDesc, true /*weighting*/, true /*truePeak*/);
// loudnessDesc[0] = loudness in dB
// loudnessDesc[1] = peak level in dB
```

#### YINFFT / Pitch Detection (include/flucoma/algorithms/public/YINFFT.hpp)

```cpp
fluid::algorithm::YINFFT yin{nBins, FluidDefaultAllocator()};

fluid::RealVector pitch(2);
yin.processFrame(magnitude, pitch, minFreq, maxFreq, sampleRate);
// pitch[0] = detected frequency in Hz
// pitch[1] = confidence (0-1)
```

#### OnsetSegmentation (include/flucoma/algorithms/public/OnsetSegmentation.hpp)

```cpp
fluid::algorithm::OnsetSegmentation onset{maxSize, maxFilterSize, FluidDefaultAllocator()};
onset.init(windowSize, fftSize, filterSize);

// Per frame:
double detected = onset.processFrame(window, function, filterSize,
                                     threshold, debounce, frameDelta,
                                     FluidDefaultAllocator());
// detected = 1.0 when onset found, 0.0 otherwise
// function = index selecting detection algorithm (energy, HFC, spectral flux, etc.)
```

#### MultiStats (include/flucoma/algorithms/public/MultiStats.hpp)

Computes 7 statistics (mean, stddev, skewness, kurtosis, low percentile, median, high percentile) across time frames.

```cpp
fluid::algorithm::MultiStats stats;
stats.init(0, 0, 50, 100);  // derivatives, outlier settings, low/high percentiles

fluid::RealMatrix descriptorFrames(nFrames, nDescriptors);
// ... fill with per-frame descriptors ...

fluid::RealMatrix result(nDescriptors, 7);
stats.process(descriptorFrames.transpose(), result);
// Each row of result = [mean, std, skew, kurt, low, mid, high] for one descriptor
```

#### KDTree (include/flucoma/algorithms/public/KDTree.hpp)

277 lines total. Uses Eigen internally for distance computation.

```cpp
// Build from a FluidDataSet:
using DataSet = fluid::FluidDataSet<std::string, double, 1>;
DataSet corpus(nDims);
// ... populate with analyzed grain descriptors ...

fluid::algorithm::KDTree tree(corpus);

// Query k-nearest neighbors:
fluid::RealVector queryPoint(nDims);
// ... fill with real-time descriptors of input ...
auto [distances, ids] = tree.kNearest(queryPoint, k);
// distances = vector of distances
// ids = vector of string ID pointers matching the nearest points

// Optionally with radius constraint:
auto [distances, ids] = tree.kNearest(queryPoint, k, radius);
```

#### UMAP (include/flucoma/algorithms/public/UMAP.hpp)

```cpp
fluid::algorithm::UMAP umap;

// Train on dataset (offline, expensive):
DataSet reduced = umap.train(
    inputDataset,
    15,     // k neighbors
    2,      // output dimensions
    0.1,    // minDist
    200,    // maxIter
    0.1     // learningRate
);

// Transform a new point (after training):
fluid::RealVector input(inputDims);
fluid::RealVector output(2);
umap.transformPoint(input, output);
```

#### Normalization (include/flucoma/algorithms/public/Normalization.hpp)

```cpp
fluid::algorithm::Normalization norm;

// Fit to data range:
norm.init(0.0, 1.0, dataMatrix);  // normalize to [0, 1]

// Transform single point:
fluid::RealVector normalized(dims);
norm.processFrame(input, normalized, false);  // false = forward, true = inverse
```

---

## 3. Building and Integrating flucoma-core with CMake

### Method 1: FetchContent (Recommended)

```cmake
cmake_minimum_required(VERSION 3.18)
project(MyPlugin VERSION 1.0.0)

include(FetchContent)

# Fetch flucoma-core (it will fetch its own dependencies)
FetchContent_Declare(
    flucoma-core
    GIT_REPOSITORY https://github.com/flucoma/flucoma-core.git
    GIT_TAG main  # or pin to a specific commit/tag
)
FetchContent_MakeAvailable(flucoma-core)

# Your JUCE plugin target
juce_add_plugin(MyPlugin ...)

# Link flucoma-core
target_link_libraries(MyPlugin PRIVATE FLUID_DECOMPOSITION)
```

### Method 2: Git Submodule

```bash
cd your-plugin-project
git submodule add https://github.com/flucoma/flucoma-core.git deps/flucoma-core
```

```cmake
add_subdirectory(deps/flucoma-core)
target_link_libraries(MyPlugin PRIVATE FLUID_DECOMPOSITION)
```

### Method 3: Local Path (for development)

```cmake
# Point to local checkout
set(FETCHCONTENT_SOURCE_DIR_FLUCOMA-CORE /path/to/flucoma-core)
FetchContent_Declare(flucoma-core ...)
FetchContent_MakeAvailable(flucoma-core)
```

### Compiler Requirements

- **C++17** minimum (standard-compliant, no compiler extensions)
- Tested on: Clang (macOS), MSVC (Windows), GCC (Linux)
- JUCE already requires C++17, so no conflict

### Platform Compatibility

| Platform | Status | Notes |
|---|---|---|
| macOS 10.9+ | Full support | Uses Accelerate.framework for FFT |
| macOS ARM64 | Full support | Apple Silicon native |
| Windows 10+ | Full support | |
| Linux (Ubuntu 20.04+) | Full support | |
| 32-bit Intel | Supported | |
| 64-bit Intel | Supported | |

### Build Size Impact

Estimated impact based on dependency analysis:
- **Eigen**: Header-only, only instantiated templates compile. Adds ~1-3MB to binary depending on which features are used. Significant compile-time impact (templates).
- **HISSTools FFT**: Small compiled library, ~100KB
- **foonathan/memory**: ~200-500KB compiled
- **Spectra**: Header-only, only if PCA/MDS used
- **nlohmann/json**: Header-only, ~1MB if fully instantiated (only needed for serialization)
- **fmt**: ~200KB compiled

**Total estimated binary size addition**: 2-5MB for a typical concatenative synthesis plugin using STFT, MelBands, DCT, SpectralShape, KDTree, and UMAP.

**Compile time impact**: Moderate to significant. Eigen templates can add 30-60 seconds to a clean build. First build also downloads dependencies via FetchContent.

### Known Issues and Gotchas

1. **FetchContent downloads at configure time**: First `cmake` invocation downloads ~100MB of dependencies (Eigen alone is ~25MB). Cache them locally for faster iteration.

2. **CMake not yet decoupled for standalone use**: The FluCoMa developers acknowledge: "we haven't done yet is decouple and export the algorithms' CMake stuff so that it can be used without pain in another project." You may need to massage CMake configuration.

3. **MFCC is not a standalone algorithm**: It must be composed from `MelBands` + `DCT` (see Issue #222). This is a minor inconvenience, not a blocker.

4. **Eigen version pinning**: FluCoMa pins Eigen to 3.4.0. If your JUCE project also uses Eigen, you may get conflicts. Use `FETCHCONTENT_SOURCE_DIR_EIGEN` to point both to the same copy.

5. **foonathan/memory build**: This dependency requires actual compilation and can be slow on first build. It also may conflict with other allocator strategies in your project.

---

## 4. Using FluCoMa Algorithms in a JUCE processBlock

### Real-Time Suitability Assessment

| Algorithm | Real-Time Safe? | Audio Thread? | Notes |
|---|---|---|---|
| `STFT::processFrame` | Mostly | Yes, with caveats | Allocates on first call; pre-allocate |
| `SpectralShape::processFrame` | Mostly | Yes, with caveats | Uses `FluidDefaultAllocator()` (heap) |
| `MelBands::processFrame` | Mostly | Yes, with caveats | Same allocator concern |
| `DCT::processFrame` | Yes | Yes | Pure matrix multiply after init |
| `YINFFT::processFrame` | Mostly | Yes, with caveats | Allocator parameter |
| `Loudness::processFrame` | Yes | Yes | After init |
| `OnsetSegmentation::processFrame` | Mostly | Yes, with caveats | Allocator parameter |
| `KDTree::kNearest` | **Yes** | **Yes** | No allocation, tree traversal only |
| `MultiStats::process` | No | Background only | Processes entire time series |
| `UMAP::train` | **No** | **Background only** | Very expensive (seconds to minutes) |
| `UMAP::transformPoint` | Yes | Yes | After training |
| `Normalization::processFrame` | Yes | Yes | Simple arithmetic |

### Critical: FluidDefaultAllocator is NOT Real-Time Safe

FluCoMa's `FluidDefaultAllocator()` wraps `foonathan::memory::heap_allocator()`, which performs dynamic heap allocation. This is **not safe for the audio thread**.

**Mitigation strategies:**
1. **Pre-allocate all buffers** during `prepareToPlay()`, not in `processBlock()`
2. **Supply a custom allocator** -- FluCoMa's algorithm `processFrame()` methods accept an `Allocator&` parameter. You could provide a pool allocator pre-allocated at init time.
3. **Run analysis on a background thread** and only use the KDTree query (which is allocation-free) on the audio thread.

### Recommended Architecture for Concatenative Synthesis in JUCE

```
Audio Thread (processBlock):
  1. Copy incoming audio to ring buffer
  2. Query KDTree with latest descriptors (safe, no allocation)
  3. Play matched grain from corpus
  4. Crossfade/overlap-add

Analysis Thread (background):
  1. Read from ring buffer when enough samples available
  2. STFT -> magnitude spectrum
  3. Extract descriptors (MFCC, SpectralShape, Pitch, Loudness)
  4. Store latest descriptor vector
  5. Signal audio thread that new descriptors are ready

Corpus Analysis (on load, one-time):
  1. Segment corpus audio (OnsetSegmentation or fixed-size grains)
  2. Extract descriptors for each segment
  3. Compute statistics (MultiStats)
  4. Normalize (Normalization)
  5. Build KDTree
  6. Optionally: UMAP for 2D visualization
```

### Feeding JUCE AudioBuffer Data into FluCoMa

```cpp
// In processBlock:
void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    // FluCoMa uses double precision internally
    const int numSamples = buffer.getNumSamples();
    const float* channelData = buffer.getReadPointer(0);

    // Option A: Copy to FluidTensor (involves allocation if not pre-allocated)
    fluid::RealVector input(numSamples);
    for (int i = 0; i < numSamples; i++)
        input(i) = static_cast<double>(channelData[i]);

    // Option B: Use FluidTensorView wrapping existing memory (zero-copy for double)
    // Only works if you have a pre-allocated double buffer
    // fluid::RealVectorView view(doubleBuffer.data(), 0, numSamples);

    // Better approach: accumulate in ring buffer, analyze on background thread
    ringBuffer.write(channelData, numSamples);
}
```

### Memory Management Summary

- `FluidTensor` allocates on heap via `rt::vector` (which uses foonathan allocators)
- Algorithm `processFrame()` methods take optional `Allocator&` parameters
- **Pre-allocate everything in `prepareToPlay()`** and reuse buffers
- KDTree queries after construction are allocation-free (traversal only)
- UMAP training is extremely allocation-heavy; always run offline

---

## 5. Cherry-Picking Algorithms vs Full Integration

### Can You Cherry-Pick?

**Partially.** The algorithms in `include/flucoma/algorithms/public/` are relatively self-contained, but they depend on:

1. **FluidTensor/FluidTensorView** (data types) -- in `include/flucoma/data/`
2. **FluidMemory** (allocator) -- in `include/flucoma/data/`
3. **Eigen** (linear algebra) -- most algorithms use this
4. **HISSTools FFT** -- anything involving spectral analysis (STFT, YINFFT, etc.)
5. **Utility headers** -- `algorithms/util/` (FFT wrappers, distance functions, etc.)

### Dependency Graph for Key Algorithms

```
SpectralShape
  -> FluidTensor, FluidMemory, Eigen (for internal compute)

MelBands
  -> FluidTensor, FluidMemory, Eigen, WindowFuncs

DCT
  -> FluidTensor, Eigen

STFT
  -> FluidTensor, FluidMemory, HISSTools FFT, WindowFuncs

YINFFT
  -> FluidTensor, FluidMemory, HISSTools FFT

KDTree
  -> FluidTensor, FluidMemory, Eigen (for distance), FluidDataSet
  -> rt::vector, rt::string (foonathan containers)

UMAP
  -> FluidTensor, FluidMemory, Eigen, Spectra
  -> KDTree, SpectralEmbedding, NNFuncs

Normalization
  -> FluidTensor (minimal dependencies)

OnsetSegmentation
  -> FluidTensor, FluidMemory, HISSTools FFT, OnsetDetectionFuncs
```

### Minimum Viable Dependencies for Concatenative Synthesis

If you use FluCoMa for everything (recommended path):
- Full `flucoma-core` + all its dependencies
- Link against `FLUID_DECOMPOSITION`

If you want to minimize:
- **MFCC pipeline**: Requires STFT (HISSTools FFT), MelBands, DCT, plus FluidTensor + Eigen
- **SpectralShape**: Requires FluidTensor + Eigen (no FFT if you provide magnitudes)
- **KDTree alone**: Requires FluidTensor + FluidDataSet + Eigen + foonathan/memory
- **UMAP alone**: Requires everything KDTree needs + Spectra + more Eigen

**Verdict**: Cherry-picking individual algorithms saves very little because most share the same dependency chain. You'd still need Eigen, FluidTensor, and FluidMemory for almost anything.

### Comparison: FluCoMa KDTree vs nanoflann

| Aspect | FluCoMa KDTree | nanoflann |
|---|---|---|
| **License** | BSD-3 | BSD |
| **Header-only** | No (needs FluidTensor, Eigen, foonathan) | **Yes** (single header) |
| **Dependencies** | Eigen, FluidTensor, foonathan/memory | **None** |
| **Lines of code** | ~277 (algorithm only) | ~2500 (single file) |
| **Dynamic insertion** | Yes (`addNode()`) | No (rebuild required) |
| **Thread-safe queries** | Not documented | **Yes** (built-in) |
| **Distance metrics** | L2 only | L1, L2, SO(2), SO(3) |
| **Performance** | Good | **Excellent** (CRTP, inlined, auto-vectorized) |
| **Data adaptors** | FluidDataSet only | Eigen, std::vector, custom |
| **Integration effort** | Part of FluCoMa ecosystem | Drop-in single header |

**Recommendation**: If you're NOT using FluCoMa for other things, **nanoflann is clearly superior** for KD-tree -- single header, no dependencies, thread-safe, faster. If you're already using FluCoMa for descriptors, its KDTree is adequate.

### Comparison: FluCoMa UMAP vs umappp

| Aspect | FluCoMa UMAP | umappp (libscran) |
|---|---|---|
| **License** | BSD-3 | **BSD-2** |
| **Header-only** | No | **Yes** |
| **Dependencies** | Eigen, Spectra, foonathan, etc. | knncolle, Eigen, Boost |
| **C++ Standard** | C++17 | C++17 |
| **API simplicity** | `train()`, `transformPoint()` | `initialize()`, `run()` |
| **Incremental execution** | No | **Yes** (step-by-step epochs) |
| **CMake integration** | Part of FLUID_DECOMPOSITION | FetchContent support |
| **Maturity** | Research-grade | Derived from uwot (R package) |

**Recommendation**: Both require Eigen. umappp is more modular but adds Boost as a dependency. If you're already pulling in FluCoMa for descriptors, use its UMAP. If rolling your own descriptors, umappp with nanoflann is a clean alternative stack.

### Roll Your Own MFCC/Spectral Descriptors?

**Feasibility**: MFCC is well-documented (Mel filterbank + DCT). Spectral shape descriptors (centroid, spread, etc.) are textbook formulas. You could implement these in ~500 lines of C++ with no external dependencies beyond an FFT library.

**Pros of rolling your own:**
- Zero external dependencies (use JUCE's built-in FFT or KissFFT)
- Full control over real-time safety (no heap allocation)
- Smaller binary
- No CMake integration headaches

**Cons:**
- Must implement and test: Mel filterbank, DCT, spectral shape, YIN pitch detection
- FluCoMa's implementations are research-grade and well-tested
- Bug risk in custom implementations
- Missing statistical summarization (MultiStats)

**Verdict**: For a commercial plugin, the practical choice depends on how many FluCoMa algorithms you need:
- **1-3 algorithms**: Roll your own or use lightweight alternatives
- **4+ algorithms**: Use FluCoMa -- the dependency overhead is justified

---

## 6. Real-World Examples of FluCoMa C++ Integration

### Official Examples (flucoma-core/examples/)

The three bundled examples are the best reference for standalone C++ usage:

1. **`describe.cpp`** -- Complete working example of:
   - Loading audio via HISSTools `in_audio_file`
   - Frame-by-frame STFT analysis
   - Extracting MFCC, SpectralShape, Pitch, Loudness per frame
   - Computing statistics with MultiStats
   - This is essentially the full descriptor extraction pipeline for concatenative synthesis

2. **`dataset.cpp`** -- Batch processing multiple files into a JSON dataset (168 features per file)

3. **`umap.cpp`** -- Loading a JSON dataset, training UMAP, saving reduced dataset

### Catecophony (github.com/ben-hayes/catecophony)

A real JUCE VST3/AU plugin doing real-time concatenative synthesis. Does NOT use FluCoMa -- uses Essentia + FFTW3 instead. Status: "Very WIP. Much broken." Demonstrates that the architecture is feasible in JUCE.

### FluCoMa Discourse Discussion

Thread: "Using FluCoMa Core with JUCE, iPlug2 etc" (discourse.flucoma.org/t/1815)

Developer `weefuzzy` confirms: "Stuff in the `fluid::algorithm` namespace should be usable with other stuff, although there's some utility types to get one's head around."

Key caveat from the FluCoMa team: "we haven't done yet is decouple and export the algorithms' CMake stuff so that it can be used without pain in another project."

The team expressed openness: "be very keen to both improve the algorithms' usefulness as an API and to expand on the examples."

### No flucoma-juce Wrapper

There is no official JUCE wrapper. The `flucoma-juce` repository does not exist. You must integrate at the `fluid::algorithm` level directly.

---

## 7. Practical Recommendation

### Three Integration Paths

#### Path A: Full FluCoMa Integration (Recommended for feature-richness)

**What**: Add `flucoma-core` as a FetchContent dependency. Use `fluid::algorithm` classes directly for all descriptor extraction, KDTree search, UMAP visualization, and normalization.

**Pros:**
- Battle-tested algorithms from academic research
- Complete concatenative synthesis pipeline in one dependency
- BSD-3 license, commercially safe
- Consistent API and data types across all algorithms
- Good examples to reference

**Cons:**
- Heavy dependency chain (Eigen, foonathan, HISSTools, Spectra, fmt, nlohmann)
- Compile time increase (~30-60s for clean build)
- Binary size increase (~2-5MB)
- Not designed as a library API -- CMake integration may require effort
- FluidDefaultAllocator is not real-time safe (must work around)
- All algorithms use double precision (JUCE uses float by default)

**Best for**: Plugin where descriptor quality matters, where you want UMAP visualization, and where binary size is not critical.

#### Path B: Hybrid -- Cherry-Pick FluCoMa + Lightweight Alternatives

**What**: Use FluCoMa for descriptor extraction (STFT, MelBands, DCT, SpectralShape) but use nanoflann for KD-tree and umappp for UMAP.

**Pros:**
- Best-of-breed for each component
- nanoflann is thread-safe and has zero dependencies
- More control over real-time behavior

**Cons:**
- Still need most of FluCoMa's dependency chain for descriptors
- Data format conversion between libraries
- More integration complexity
- Boost dependency from umappp

**Best for**: When you need guaranteed thread-safe KNN queries and want maximum control.

#### Path C: Roll Your Own + Lightweight Libraries (Recommended for commercial plugin)

**What**: Implement MFCC and spectral descriptors yourself (or use a lightweight library like compute-mfcc). Use JUCE's built-in FFT (or KissFFT). Use nanoflann for KD-tree. Skip UMAP or use umappp only for offline visualization.

**Stack:**
- FFT: JUCE `juce::dsp::FFT` (already available, zero additional deps)
- MFCC: Custom implementation (~200 lines: Mel filterbank + DCT matrix)
- Spectral descriptors: Custom implementation (~100 lines: centroid, spread, flatness, etc.)
- Pitch: Custom YIN implementation (~300 lines) or skip if not needed
- Loudness: Custom RMS/peak (~50 lines) or JUCE utilities
- Onset detection: Custom spectral flux (~100 lines)
- KD-tree: nanoflann (single header, BSD, thread-safe)
- Normalization: Trivial to implement (~30 lines)
- UMAP: Only if needed for visualization; use umappp offline

**Pros:**
- Minimal dependencies (nanoflann is the only external dep, single header)
- Full real-time safety control (no unexpected heap allocation)
- Fast compile times
- Small binary (< 500KB additional)
- No float/double conversion overhead (work in float throughout)
- Full understanding of every line of code

**Cons:**
- More development time upfront (~2-3 days for core descriptors)
- Must test thoroughly (FluCoMa's algorithms are well-validated)
- Missing advanced algorithms (UMAP, PCA, NMF, MLP) unless added later
- No statistical summarization (MultiStats) unless implemented

**Best for**: Commercial plugin where binary size, compile time, real-time safety, and dependency minimalism matter.

### Final Recommendation

**For your concatenative synthesis VST/AU plugin, Path C (roll your own + nanoflann) is the most pragmatic choice**, with the option to add FluCoMa later if you need its advanced algorithms.

Here's why:
1. The core algorithms needed (MFCC, spectral shape, onset detection, KD-tree) are well-documented and straightforward to implement
2. nanoflann alone gives you a production-quality, thread-safe, zero-dependency KD-tree
3. FluCoMa's real-time safety concerns (heap allocation in processFrame) would require significant workaround effort
4. The CMake integration with JUCE is undocumented and acknowledged as painful by the FluCoMa developers themselves
5. You avoid pulling in ~100MB of transitive dependencies (Eigen, foonathan, etc.)
6. If you later decide you need UMAP for a 2D corpus visualization UI, you can add it as an offline-only component

**However**, if you value algorithm correctness over implementation effort and are comfortable with the dependency chain, **Path A (full FluCoMa) is the safest choice** -- the `describe.cpp` example literally shows the exact pipeline you need for concatenative synthesis descriptor extraction.

### License Summary for All Paths

| Component | License | Commercial OK? |
|---|---|---|
| flucoma-core | BSD-3 | Yes |
| Eigen | MPL2 | Yes |
| HISSTools | BSD-3 | Yes |
| nanoflann | BSD | Yes |
| umappp | BSD-2 | Yes |
| Essentia | **AGPL-3** | **No** (requires commercial license from UPF) |
| foonathan/memory | Zlib | Yes |
| JUCE | Dual (GPL/commercial) | Yes (with JUCE license) |

**All recommended paths use commercially-compatible licenses.** Avoid Essentia (AGPL) unless you obtain a commercial license from the Music Technology Group at UPF.

---

## Appendix A: Complete Working Example -- FluCoMa Descriptor Extraction

From `flucoma-core/examples/describe.cpp`, adapted for clarity:

```cpp
#include <flucoma/algorithms/public/DCT.hpp>
#include <flucoma/algorithms/public/Loudness.hpp>
#include <flucoma/algorithms/public/MelBands.hpp>
#include <flucoma/algorithms/public/MultiStats.hpp>
#include <flucoma/algorithms/public/STFT.hpp>
#include <flucoma/algorithms/public/SpectralShape.hpp>
#include <flucoma/algorithms/public/YINFFT.hpp>
#include <flucoma/data/FluidMemory.hpp>
#include <flucoma/data/TensorTypes.hpp>

using namespace fluid;
using namespace fluid::algorithm;

// Configuration
const index nBins = 513;           // fftSize/2 + 1
const index fftSize = 1024;        // 2 * (nBins - 1)
const index hopSize = 512;
const index windowSize = 1024;
const index nBands = 40;           // Mel bands
const index nCoefs = 13;           // MFCC coefficients
const index minFreq = 20;
const index maxFreq = 5000;

// Initialize algorithms
STFT stft{windowSize, fftSize, hopSize};
MelBands bands{nBands, fftSize};
DCT dct{nBands, nCoefs};
YINFFT yin{nBins, FluidDefaultAllocator()};
SpectralShape shape(FluidDefaultAllocator());
Loudness loudness{windowSize};

// Init with sample rate
bands.init(minFreq, maxFreq, nBands, nBins, sampleRate, windowSize);
dct.init(nBands, nCoefs);
loudness.init(windowSize, sampleRate);

// Per-frame processing:
ComplexVector frame(nBins);
RealVector magnitude(nBins);
RealVector mels(nBands);
RealVector mfccs(nCoefs);
RealVector pitch(2);
RealVector shapeDesc(7);
RealVector loudnessDesc(2);

// window = audio slice of windowSize samples
stft.processFrame(window, frame);
stft.magnitude(frame, magnitude);
bands.processFrame(magnitude, mels, false, false, true, FluidDefaultAllocator());
dct.processFrame(mels, mfccs);
yin.processFrame(magnitude, pitch, minFreq, maxFreq, sampleRate);
shape.processFrame(magnitude, shapeDesc, sampleRate, 0, -1, 0.95, false, false, FluidDefaultAllocator());
loudness.processFrame(window, loudnessDesc, true, true);

// Total descriptor vector per frame: 13 (MFCC) + 7 (shape) + 2 (pitch) + 2 (loudness) = 24 dimensions
```

## Appendix B: nanoflann Integration Example

```cpp
#include "nanoflann.hpp"
#include <vector>

// Adaptor for your corpus data
struct CorpusAdaptor {
    const std::vector<std::vector<float>>& points;

    inline size_t kdtree_get_point_count() const { return points.size(); }

    inline float kdtree_get_pt(const size_t idx, const size_t dim) const {
        return points[idx][dim];
    }

    template <class BBOX> bool kdtree_get_bbox(BBOX&) const { return false; }
};

using KDTree = nanoflann::KDTreeSingleIndexAdaptor<
    nanoflann::L2_Simple_Adaptor<float, CorpusAdaptor>,
    CorpusAdaptor,
    -1  // runtime dimensionality
>;

// Build tree (do this once when corpus is loaded)
CorpusAdaptor adaptor{corpusDescriptors};
KDTree tree(numDimensions, adaptor, {10 /* max leaf size */});
tree.buildIndex();

// Query k-nearest (thread-safe, no allocation)
std::vector<nanoflann::ResultItem<size_t, float>> results(k);
nanoflann::KNNResultSet<float> resultSet(k);
resultSet.init(results.data(), k);
tree.findNeighbors(resultSet, queryPoint.data());

// results[0].first = index of nearest corpus grain
// results[0].second = squared L2 distance
```

## Appendix C: Sources

- [flucoma-core GitHub repository](https://github.com/flucoma/flucoma-core)
- [FluCoMa Build System Documentation](https://develop.flucoma.org/build-system/overview.html)
- [FluCoMa Developer Documentation -- KDTreeClient Walkthrough](https://develop.flucoma.org/clients/kdtree.html)
- [FluCoMa Developer Documentation -- PitchClient Walkthrough](https://develop.flucoma.org/clients/pitch.html)
- [Discourse: Using FluCoMa Core with JUCE, iPlug2 etc](https://discourse.flucoma.org/t/using-flucoma-core-with-juce-iplug2-etc/1815)
- [Discourse: BSD-3 and Distribution](https://discourse.flucoma.org/t/bsd3-and-distribution/342)
- [Discourse: Concatenative Synthesis SC](https://discourse.flucoma.org/t/concatenative-synthesis-sc/1850)
- [GitHub Issue #222: Make MFCC an algorithm](https://github.com/flucoma/flucoma-core/issues/222)
- [FluCoMa LICENSE.md (BSD-3)](https://github.com/flucoma/flucoma-core/blob/main/LICENSE.md)
- [nanoflann GitHub repository](https://github.com/jlblancoc/nanoflann)
- [umappp GitHub repository](https://github.com/libscran/umappp)
- [Catecophony -- JUCE Concatenative Synthesis Plugin](https://github.com/ben-hayes/catecophony)
- [Essentia Licensing Information](https://essentia.upf.edu/licensing_information.html)
- [FluCoMa Contributing Guide](https://develop.flucoma.org/contributing.html)
- [HISSTools Library](https://github.com/AlexHarker/HISSTools_Library)
