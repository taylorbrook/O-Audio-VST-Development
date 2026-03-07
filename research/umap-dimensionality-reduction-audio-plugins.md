---
title: "UMAP Dimensionality Reduction for Audio Plugin Visualization"
created: 2026-02-08
domain: ml
type: algorithm
keywords:
  - umap
  - dimensionality-reduction
  - visualization
  - audio-features
  - pca
  - t-sne
---
# UMAP Dimensionality Reduction for Audio Plugin Visualization

## Research Summary

This document evaluates C++ implementations of UMAP (Uniform Manifold Approximation and Projection) for reducing 19-dimensional audio descriptor spaces to 2D for scatter plot visualization in a real-time audio plugin. It covers available implementations, performance characteristics, comparison with PCA and t-SNE, incremental update strategies, and practical integration with JUCE.

---

## 1. Available C++ UMAP Implementations

### 1.1 umappp (libscran/umappp) -- RECOMMENDED

**Repository:** https://github.com/libscran/umappp
**Documentation:** https://libscran.github.io/umappp/
**License:** BSD-2-Clause (fully permissive, commercial use allowed)
**Author:** Aaron Lun
**Latest release:** v3.2.0 (November 2025)
**Header-only:** Yes
**Language standard:** C++17

**Dependencies (all header-only, fetched via CMake FetchContent):**
- **Eigen** (v5.0.0) -- linear algebra
- **knncolle** (v3.0.0) -- nearest-neighbor search framework (supports VP-trees, Annoy, HNSW)
- **CppIrlba** (v3.0.0) -- truncated SVD for spectral initialization
- **aarand** (v1.1.0) -- portable random number generation
- **subpar** (v0.4.0) -- parallelization utilities
- **sanisizer** (v0.1.3) -- integer sanitization

**Key strengths:**
- Production-quality, actively maintained (117 commits, CI/CD with uwot validation)
- Column-major data layout (compatible with Eigen)
- Epoch-by-epoch execution for progress reporting
- Built-in multi-threading via `subpar` or custom parallel backend
- Spectral initialization with automatic fallback to random
- Multiple neighbor search backends (exact VP-trees, approximate Annoy/HNSW)
- Derived from the C++ code in the uwot R package, validated against it

**API surface:**

```cpp
#include <umappp/umappp.hpp>
#include <knncolle/knncolle.hpp>

// Data: column-major, each column = one observation
// For 19 descriptors x 10000 points: double data[19 * 10000]
std::vector<double> data(ndim * nobs);
std::vector<double> embedding(2 * nobs); // output: 2D

// Configure
umappp::Options opt;
opt.num_neighbors = 15;
opt.num_epochs = 500;    // auto-chosen if unset
opt.min_dist = 0.1;
opt.num_threads = 4;     // parallel neighbor search
opt.parallel_optimization = false; // usually not worth it

// Option A: Let umappp handle neighbor search (VP-tree by default)
knncolle::VptreeBuilder<int, double> vp_builder;
auto status = umappp::initialize(
    ndim, nobs, data.data(),
    vp_builder,
    2,                    // output dimensions
    embedding.data(),
    opt
);

// Option B: Use approximate neighbors (faster for large N)
// Requires knncolle_annoy or knncolle_hnsw
// knncolle_annoy::AnnoyBuilder<int, double, double, Annoy::Euclidean> annoy_builder;

// Run all epochs at once
status.run(embedding.data());

// OR run epoch-by-epoch for progress reporting
for (int epoch = 50; epoch <= status.num_epochs(); epoch += 50) {
    status.run(embedding.data(), epoch);
    float progress = (float)epoch / status.num_epochs();
    // update progress bar on UI thread
}
```

**Platform support:** macOS, Windows, Linux (standard C++17)

### 1.2 FluCoMa's Internal UMAP (flucoma-core)

**Repository:** https://github.com/flucoma/flucoma-core
**Source file:** `include/flucoma/algorithms/public/UMAP.hpp`
**License:** BSD-3-Clause (University of Huddersfield, permissive, commercial use allowed)
**Language standard:** C++17

**Dependencies:**
- **Eigen** (3.4) -- core linear algebra
- **Eigen unsupported** -- NonLinearOptimization, NumericalDiff (for finding a/b parameters)
- **KDTree** (FluCoMa internal) -- neighbor search
- **SpectralEmbedding** (FluCoMa internal) -- initialization
- FluCoMa's own data types (FluidDataSet, FluidTensor, FluidMemory)

**Key characteristics:**
- Self-contained UMAP implementation (~400 lines)
- Includes `train()`, `transform()`, and `transformPoint()` methods
- `transformPoint()` is designed for projecting single new points onto an existing embedding
- Uses FluCoMa's own KDTree and data structures -- not easily extractable
- No built-in multi-threading in the UMAP optimization loop
- Proven in production across Max, SuperCollider, and Pure Data environments

**API surface (within FluCoMa ecosystem):**

```cpp
algorithm::UMAP umap;

// Train: produces DataSet with 2D coordinates
DataSet result = umap.train(
    inputDataSet,       // N points x D dimensions
    15,                 // k neighbors
    2,                  // output dimensions
    0.1,                // min_dist
    200,                // max iterations
    1.0                 // learning rate
);

// Transform new points using existing model
DataSet newProjected = umap.transform(newData, 200, 1.0);

// Transform a single point (useful for incremental updates)
umap.transformPoint(inputVector, outputVector);
```

**Verdict:** Excellent reference implementation and proof that UMAP works well for audio descriptors. However, extracting it from FluCoMa's ecosystem requires pulling in its data structures, KDTree, and Eigen unsupported modules. Better to use umappp directly unless you are already integrating FluCoMa.

### 1.3 78Spinoza/UMAP (C++ with C# wrapper)

**Repository:** https://github.com/78Spinoza/UMAP
**License:** Not explicitly stated (based on umappp, likely BSD-2-Clause)

**Notable features:**
- Built on top of umappp
- Adds HNSW (hnswlib) integration for dramatically faster neighbor search
- Includes transform functionality with outlier detection
- Model persistence (serialization)
- OpenMP parallelization for transforms
- C# wrapper for .NET integration

**Benchmark data from this project:**

| Dataset Size | Exact kNN Time | HNSW Time | Speedup |
|---|---|---|---|
| 1,000 x 100 | 2.5s | 0.8s | 3x |
| 5,000 x 200 | 45s | 1.2s | 37x |
| 20,000 x 300 | 8.5 min | 12s | 42x |
| 100,000 x 500 | 4+ hours | 180s | 80x |

**Verdict:** Useful reference for how to add HNSW to umappp, but the C# wrapper focus makes it less directly useful. The benchmark data is valuable for estimating performance.

### 1.4 GerardoBelic/UMAP-in-Cpp

**Repository:** https://github.com/GerardoBelic/UMAP-in-Cpp
**License:** Not specified
**Status:** Appears to be an academic/educational implementation. Minimal documentation, Windows-focused (.exe), not header-only. Not recommended for production use.

---

## 2. Performance Estimates

### 2.1 Expected Timings for Audio Descriptor Data

Based on benchmark data from 78Spinoza/UMAP and the Python umap-learn documentation, extrapolated for 19-dimensional audio descriptors:

| Dataset | Exact kNN | With HNSW | Notes |
|---|---|---|---|
| 1,000 x 19 | ~0.5s | ~0.2s | Small corpus, near-instant |
| 5,000 x 19 | ~5-10s | ~0.5s | Medium corpus |
| 10,000 x 19 | ~15-30s | ~1-2s | Typical large corpus |
| 50,000 x 19 | ~3-8 min | ~5-15s | Very large corpus |
| 100,000 x 19 | ~15-30 min | ~30-60s | Extreme case |

**Important caveats:**
- These are rough estimates. 19 dimensions is much lower than the 100-500 dimensions in the benchmarks above, so neighbor search will be faster.
- The dominant cost is neighbor search (O(N^2) exact, O(N log N) approximate), not the optimization loop.
- Python umap-learn embeds 70,000 points in 784 dimensions in ~42 seconds -- a C++ implementation should be comparable or faster.
- With HNSW approximate neighbors, even 50,000 points should complete in under 30 seconds.

### 2.2 Memory Usage

For N observations with 19 dimensions and 2D output:
- Input data: N x 19 x 8 bytes (doubles)
- kNN graph: N x k x 16 bytes (index + distance per neighbor)
- Sparse similarity matrix: ~N x k x 24 bytes
- Embedding: N x 2 x 8 bytes
- Working memory for optimization: ~N x 2 x 8 bytes

**Approximate total memory:**

| N | Memory |
|---|---|
| 10,000 | ~10 MB |
| 50,000 | ~50 MB |
| 100,000 | ~100 MB |

This is well within the memory budget of a desktop audio plugin.

### 2.3 Epoch Count Selection (umappp default behavior)

umappp automatically selects the number of epochs:
- N <= 10,000: 500 epochs
- N > 10,000: scales down from 500 toward 200 (inversely proportional to N)
- This keeps total computation roughly constant regardless of dataset size

---

## 3. UMAP vs PCA vs t-SNE for Audio Descriptors

### 3.1 Comparison Table

| Property | PCA | t-SNE | UMAP |
|---|---|---|---|
| **Type** | Linear | Non-linear | Non-linear |
| **Speed (10k points)** | <1 second | 1-5 minutes | 2-30 seconds |
| **Deterministic** | Yes | No | No (but seedable) |
| **Local structure** | Poor | Excellent | Excellent |
| **Global structure** | Good | Poor | Good |
| **Scalability** | O(N*D*k) | O(N^2) | O(N log N) with HNSW |
| **Incremental** | Yes (project) | No | Possible (transform) |
| **Implementation complexity** | Trivial (~30 lines with Eigen) | Complex | Complex (use library) |
| **Dependencies** | Eigen only | Multiple | Eigen + knncolle + ... |

### 3.2 When is PCA "Good Enough"?

PCA is adequate when:
- The descriptor space is roughly linearly separable (e.g., bright vs dark sounds lie along a principal axis)
- Speed is paramount (real-time or near-real-time updates needed)
- You need deterministic, reproducible results every time
- The user is adding points one at a time and needs instant projection

PCA is NOT adequate when:
- Audio descriptors have complex non-linear relationships (common in timbral spaces)
- You need tight clustering of perceptually similar sounds
- The 19 dimensions contain redundant or correlated features that PCA cannot disentangle
- You want the scatter plot to show meaningful "islands" of similar sounds

**In practice for audio timbral descriptors, PCA tends to produce a single blob with gradual variation, while UMAP produces distinct clusters of perceptually similar sounds.** This is the primary reason FluCoMa chose UMAP.

### 3.3 Why FluCoMa Chose UMAP

From research published in "A General Framework for Visualization of Sound Collections in Musical Interfaces" (Bernardo et al., 2021):

1. **Trustworthiness:** t-SNE and UMAP both score significantly higher than PCA/MDS/Isomap for preserving local neighborhoods -- meaning nearby points in the scatter plot are truly similar sounds.
2. **Speed:** UMAP is much faster than t-SNE for large datasets, making it practical for interactive music-making applications.
3. **Robustness:** UMAP handles noisy, sparse data (common in audio descriptors) better than linear techniques.
4. **The researchers explicitly concluded UMAP is "better suited for music creation applications."**

### 3.4 Recommended Strategy: PCA Fast Path + UMAP Quality Mode

Use both algorithms with a clear UX distinction:

1. **PCA (immediate):** When audio is first loaded, immediately compute PCA for an instant 2D scatter plot. This takes <1 second for any corpus size.
2. **UMAP (background):** Simultaneously launch UMAP on a background thread. When complete, swap in the UMAP projection with a smooth animation.
3. **User choice:** Provide a toggle: "Fast Layout (PCA)" vs "Detailed Layout (UMAP)" so users can switch.

This gives the best of both worlds: instant feedback with PCA, superior clustering with UMAP.

---

## 4. Incremental/Online UMAP

### 4.1 The Core Problem

When a user adds new audio to an existing corpus, we need to update the 2D visualization. The options are:

### 4.2 Option A: Full Re-computation

Simply re-run UMAP on the entire corpus (old + new points). This is the simplest and most reliable approach.

**Pros:** Always correct, simple implementation
**Cons:** Slow for large corpora, existing point positions change (disorienting for the user)

**Mitigation:** If the corpus is small (<5000 points), re-computation is fast enough (<5 seconds with HNSW) to be acceptable.

### 4.3 Option B: UMAP Transform (Project New Points)

Both umappp (via the knncolle neighbor search) and FluCoMa's UMAP provide `transform()` functionality:

1. Keep the trained UMAP model (neighbor graph, a/b parameters, embedding)
2. For new points, find their k nearest neighbors in the original high-dimensional space
3. Initialize the new point's 2D position as a weighted average of its neighbors' 2D positions
4. Optionally run a few optimization epochs to refine

**FluCoMa's approach:**

```cpp
// FluCoMa provides transformPoint() for single-point projection
umap.transformPoint(newDescriptorVector, output2D);
```

This is essentially a nearest-neighbor weighted interpolation. It is fast (milliseconds per point) but may not perfectly respect the global structure.

**Pros:** Fast, existing points don't move, simple UX
**Cons:** Quality degrades as more points are added without re-training; new points can only land near existing clusters

**Important:** umappp itself does NOT include a built-in `transform()` method. You would need to implement the transform logic yourself using the stored neighbor index and embedding, following the same approach as FluCoMa's `transformPoint()`.

### 4.4 Option C: Parametric UMAP

Train a small neural network to learn the mapping from 19D to 2D. Once trained, projecting new points is a simple forward pass.

**Current status:** Only available in Python (via Keras/TensorFlow). No C++ implementation exists. Could theoretically be implemented using ONNX Runtime or a custom tiny neural network, but this adds significant complexity.

**Not recommended for a JUCE plugin at this time** due to implementation complexity and the added dependency of a neural network inference engine.

### 4.5 Recommended Incremental Strategy

```
When user adds new audio to corpus:
  1. If corpus < 5000 points:
     - Re-run UMAP on full corpus (background thread, ~2-5 seconds)
  2. If corpus >= 5000 points:
     - Use transform/weighted-neighbor projection for immediate placement
     - Queue a full UMAP re-computation for later
     - When re-computation finishes, smoothly animate points to new positions
  3. Always update PCA instantly (it's fast enough for any size)
```

### 4.6 How FluCoMa Handles This

FluCoMa provides both `train()` (full computation) and `transform()` (project new points onto existing embedding). In practice, FluCoMa users typically:
1. Build a corpus
2. Run `fluid.umap~` to get the embedding
3. If they add more audio, they re-run `fluid.umap~` on the full corpus

The `transformPoint()` method exists for real-time single-point projection (e.g., "where does this live sound fall on the scatter plot?"), which is a compelling use case for a live performance plugin.

---

## 5. Integration with JUCE Plugin Architecture

### 5.1 Threading Architecture

```
Audio Thread (real-time)
  |
  v
  [Audio Analysis] --> writes descriptors to lock-free queue

Background Thread (juce::Thread subclass)
  |
  v
  [Descriptor Accumulation] --> stores N x 19 matrix
  [PCA Computation] --> instant, sends 2D coords to UI
  [UMAP Computation] --> slower, sends 2D coords to UI when done

Message Thread (UI)
  |
  v
  [Scatter Plot Component] --> renders 2D points
```

### 5.2 Key JUCE Classes to Use

- **`juce::Thread`**: Subclass for the UMAP computation thread
- **`juce::AsyncUpdater`**: Trigger UI updates from the background thread (call `triggerAsyncUpdate()` from the UMAP thread, handle in `handleAsyncUpdate()` on the message thread)
- **`juce::ThreadWithProgressWindow`**: For showing a progress bar during initial UMAP computation (optional -- only needed for very large corpora)
- **`std::atomic<float>`**: For communicating progress (0.0-1.0) from the UMAP thread to the UI

### 5.3 Workflow Implementation

```cpp
class UMAPThread : public juce::Thread
{
public:
    UMAPThread() : Thread("UMAP Computation") {}

    void setData(const std::vector<double>& descriptors, int numPoints, int numDims)
    {
        // Copy data for thread safety
        data = descriptors;
        nobs = numPoints;
        ndim = numDims;
    }

    void run() override
    {
        // 1. Build neighbor index
        knncolle::VptreeBuilder<int, double> builder;

        // 2. Configure UMAP
        umappp::Options opt;
        opt.num_neighbors = 15;
        opt.min_dist = 0.1;
        opt.num_threads = std::max(1, (int)std::thread::hardware_concurrency() - 2);

        // 3. Initialize
        embedding.resize(2 * nobs);
        auto status = umappp::initialize(
            ndim, nobs, data.data(),
            builder, 2, embedding.data(), opt
        );

        // 4. Run with progress reporting
        int totalEpochs = status.num_epochs();
        int step = std::max(1, totalEpochs / 20); // report ~20 times

        for (int epoch = step; epoch <= totalEpochs; epoch += step)
        {
            if (threadShouldExit()) return;

            status.run(embedding.data(), std::min(epoch, totalEpochs));
            progress.store((float)epoch / totalEpochs);
        }

        // 5. Normalize to 0-1 range
        normalizeEmbedding();

        // 6. Notify UI
        completed.store(true);
    }

    std::atomic<float> progress{0.0f};
    std::atomic<bool> completed{false};
    std::vector<double> embedding;

private:
    void normalizeEmbedding()
    {
        double minX = std::numeric_limits<double>::max();
        double maxX = std::numeric_limits<double>::lowest();
        double minY = minX, maxY = maxX;

        for (int i = 0; i < nobs; ++i)
        {
            // Column-major: dim 0 at [i], dim 1 at [nobs + i]
            minX = std::min(minX, embedding[i]);
            maxX = std::max(maxX, embedding[i]);
            minY = std::min(minY, embedding[nobs + i]);
            maxY = std::max(maxY, embedding[nobs + i]);
        }

        double rangeX = maxX - minX;
        double rangeY = maxY - minY;
        if (rangeX < 1e-10) rangeX = 1.0;
        if (rangeY < 1e-10) rangeY = 1.0;

        for (int i = 0; i < nobs; ++i)
        {
            embedding[i] = (embedding[i] - minX) / rangeX;
            embedding[nobs + i] = (embedding[nobs + i] - minY) / rangeY;
        }
    }

    std::vector<double> data;
    int nobs = 0;
    int ndim = 0;
};
```

### 5.4 User Wait Times and UX

| Corpus Size | PCA Wait | UMAP Wait (exact kNN) | UMAP Wait (HNSW) | UX Recommendation |
|---|---|---|---|---|
| 100-1,000 | Instant | <1s | <0.5s | No progress bar needed |
| 1,000-5,000 | Instant | 5-10s | 0.5-1s | Show spinner |
| 5,000-10,000 | Instant | 15-30s | 1-2s | Show progress bar |
| 10,000-50,000 | Instant | 3-8 min | 5-15s | Show progress bar, allow cancel |

**Recommendation:** Use HNSW approximate neighbors for any corpus over 1,000 points. The quality loss is negligible (MSE < 0.01 compared to exact) and the speedup is dramatic.

### 5.5 Can UMAP Run While the User is Playing?

Yes, absolutely. UMAP runs entirely on a background thread and does not touch the audio thread. The workflow is:

1. User loads audio and starts playing immediately
2. Background thread segments audio, extracts descriptors
3. PCA runs instantly, scatter plot appears within 1 second
4. UMAP starts computing in background
5. User can interact with the plugin, play audio, adjust parameters
6. When UMAP finishes, the scatter plot smoothly transitions to the UMAP layout
7. A small indicator shows "Computing optimal layout..." while UMAP runs

---

## 6. Practical Code Example: Full Pipeline

### 6.1 Complete Example with umappp

```cpp
#include <umappp/umappp.hpp>
#include <knncolle/knncolle.hpp>
#include <vector>
#include <cmath>

struct Point2D { float x, y; };

// Input: descriptorMatrix is row-major (each row = one audio segment's 19 descriptors)
// Output: vector of 2D points normalized to [0, 1]
std::vector<Point2D> computeUMAP(
    const std::vector<std::vector<double>>& descriptorMatrix,
    int numNeighbors = 15,
    double minDist = 0.1,
    int numThreads = 4)
{
    const int nobs = (int)descriptorMatrix.size();
    const int ndim = (int)descriptorMatrix[0].size(); // 19

    // Convert to column-major format for umappp
    // umappp expects: data[dim * nobs], column-major
    // Column i (observation i) occupies indices [i*ndim .. (i+1)*ndim-1]
    // Wait -- umappp docs say "rows are dimensions, columns are observations"
    // So data layout is: data[dim + obs * ndim] -- but that IS row-major for a dim x obs matrix
    // Actually for column-major: data[dim + obs * ndim] where dim is row, obs is column
    std::vector<double> data(ndim * nobs);
    for (int i = 0; i < nobs; ++i)
        for (int d = 0; d < ndim; ++d)
            data[d + i * ndim] = descriptorMatrix[i][d];

    // Configure
    umappp::Options opt;
    opt.num_neighbors = numNeighbors;
    opt.min_dist = minDist;
    opt.num_threads = numThreads;
    opt.initialize_method = umappp::InitializeMethod::SPECTRAL;
    // opt.num_epochs is auto-selected: 500 for N<=10000, scales down for larger

    // Allocate output embedding (column-major: 2 rows x nobs columns)
    std::vector<double> embedding(2 * nobs);

    // Build neighbor index and initialize UMAP
    knncolle::VptreeBuilder<int, double> builder;
    auto status = umappp::initialize(
        (size_t)ndim,     // data dimensions
        nobs,             // number of observations
        data.data(),      // input data pointer
        builder,          // neighbor search algorithm
        (size_t)2,        // embedding dimensions
        embedding.data(), // output embedding pointer
        opt
    );

    // Run all epochs
    status.run(embedding.data());

    // Extract and normalize to [0, 1]
    // Column-major output: x coords at embedding[0..nobs-1], y at embedding[nobs..2*nobs-1]
    double minX = embedding[0], maxX = embedding[0];
    double minY = embedding[nobs], maxY = embedding[nobs];
    for (int i = 1; i < nobs; ++i) {
        minX = std::min(minX, embedding[i]);
        maxX = std::max(maxX, embedding[i]);
        minY = std::min(minY, embedding[nobs + i]);
        maxY = std::max(maxY, embedding[nobs + i]);
    }

    double rangeX = (maxX - minX > 1e-10) ? (maxX - minX) : 1.0;
    double rangeY = (maxY - minY > 1e-10) ? (maxY - minY) : 1.0;

    std::vector<Point2D> result(nobs);
    for (int i = 0; i < nobs; ++i) {
        result[i].x = (float)((embedding[i] - minX) / rangeX);
        result[i].y = (float)((embedding[nobs + i] - minY) / rangeY);
    }

    return result;
}
```

### 6.2 PCA Fallback (Trivial Implementation with Eigen)

```cpp
#include <Eigen/Dense>
#include <Eigen/Eigenvalues>
#include <vector>

std::vector<Point2D> computePCA(
    const std::vector<std::vector<double>>& descriptorMatrix)
{
    const int nobs = (int)descriptorMatrix.size();
    const int ndim = (int)descriptorMatrix[0].size(); // 19

    // Build Eigen matrix (each row = one observation)
    Eigen::MatrixXd X(nobs, ndim);
    for (int i = 0; i < nobs; ++i)
        for (int d = 0; d < ndim; ++d)
            X(i, d) = descriptorMatrix[i][d];

    // Center the data (subtract mean of each column)
    Eigen::VectorXd mean = X.colwise().mean();
    X.rowwise() -= mean.transpose();

    // Compute covariance matrix (19x19, always small)
    Eigen::MatrixXd cov = (X.transpose() * X) / (nobs - 1);

    // Eigenvalue decomposition
    Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> solver(cov);

    // Take the 2 largest eigenvectors (Eigen sorts ascending, so take last 2)
    Eigen::MatrixXd components = solver.eigenvectors().rightCols(2);

    // Project data onto 2 principal components
    Eigen::MatrixXd projected = X * components; // nobs x 2

    // Normalize to [0, 1]
    double minX = projected.col(0).minCoeff();
    double maxX = projected.col(0).maxCoeff();
    double minY = projected.col(1).minCoeff();
    double maxY = projected.col(1).maxCoeff();
    double rangeX = (maxX - minX > 1e-10) ? (maxX - minX) : 1.0;
    double rangeY = (maxY - minY > 1e-10) ? (maxY - minY) : 1.0;

    std::vector<Point2D> result(nobs);
    for (int i = 0; i < nobs; ++i) {
        result[i].x = (float)((projected(i, 0) - minX) / rangeX);
        result[i].y = (float)((projected(i, 1) - minY) / rangeY);
    }

    return result;
}
```

**PCA performance:** For 50,000 x 19 data, the covariance matrix is 19x19, eigendecomposition is trivial. Total time: well under 100ms regardless of corpus size.

### 6.3 CMake Integration for umappp

```cmake
include(FetchContent)

# Fetch umappp and all its dependencies
FetchContent_Declare(
    umappp
    GIT_REPOSITORY https://github.com/libscran/umappp
    GIT_TAG v3.2.0
)
FetchContent_MakeAvailable(umappp)

# Link to your plugin target
target_link_libraries(YourPlugin PRIVATE libscran::umappp)
```

This will automatically pull in Eigen, knncolle, CppIrlba, and all other dependencies.

---

## 7. Descriptor Normalization Before Dimensionality Reduction

Before feeding descriptors to UMAP or PCA, each feature dimension should be normalized to prevent features with large numeric ranges from dominating the distance calculations.

**Recommended approach: z-score normalization (standardization)**

```cpp
// For each of the 19 descriptor dimensions:
// 1. Compute mean and standard deviation across all observations
// 2. Subtract mean and divide by stddev
// This makes each dimension zero-mean with unit variance

for (int d = 0; d < ndim; ++d) {
    double sum = 0, sumSq = 0;
    for (int i = 0; i < nobs; ++i) {
        sum += data[d + i * ndim];
        sumSq += data[d + i * ndim] * data[d + i * ndim];
    }
    double mean = sum / nobs;
    double variance = (sumSq / nobs) - (mean * mean);
    double stddev = std::sqrt(std::max(variance, 1e-10));

    for (int i = 0; i < nobs; ++i) {
        data[d + i * ndim] = (data[d + i * ndim] - mean) / stddev;
    }
}
```

Store the mean and stddev per dimension so that new points can be normalized consistently before projection.

---

## 8. Recommendations Summary

### Recommended Implementation: umappp

**Use umappp** as the primary UMAP library. It is:
- Header-only and easy to integrate via CMake
- BSD-2-Clause licensed (safe for commercial plugins)
- Production-quality with active maintenance
- Well-documented with clear API
- Supports epoch-by-epoch execution for progress reporting
- Supports multi-threaded neighbor search

### Recommended Architecture

1. **Always compute PCA first** (instant, deterministic, shows something immediately)
2. **Compute UMAP in background** (better clustering, runs async)
3. **Use HNSW approximate neighbors** for corpora over 1,000 points
4. **For incremental updates**: use nearest-neighbor weighted projection for new points, queue full re-computation
5. **Normalize descriptors** (z-score) before dimensionality reduction
6. **Normalize output** to [0, 1] for scatter plot rendering
7. **Show progress** for UMAP computation (epoch-by-epoch via `status.run(embedding, epoch_limit)`)

### Dependency Impact

Adding umappp to a JUCE plugin introduces:
- **Eigen** (~15 MB of headers, compile time impact, but header-only)
- **knncolle** (tiny, header-only)
- **CppIrlba** (small, header-only)
- **aarand, subpar, sanisizer** (tiny utility headers)

Since Eigen is already required by FluCoMa and many DSP libraries, this is a reasonable dependency. Eigen headers can be pruned to only include `Core`, `Dense`, and `SparseCore` modules.

---

## Sources

- [libscran/umappp GitHub Repository](https://github.com/libscran/umappp)
- [umappp API Documentation](https://libscran.github.io/umappp/)
- [78Spinoza/UMAP - C++ UMAP with HNSW and C# wrapper](https://github.com/78Spinoza/UMAP)
- [FluCoMa Core Repository](https://github.com/flucoma/flucoma-core)
- [FluCoMa UMAP Reference](https://learn.flucoma.org/reference/umap/)
- [FluCoMa UMAP Discussion - Feature Request](https://discourse.flucoma.org/t/new-dimensionality-reduction-algorithm-umap/593)
- [UMAP Performance Benchmarking (Python)](https://umap-learn.readthedocs.io/en/latest/benchmarking.html)
- [Transforming New Data with UMAP](https://umap-learn.readthedocs.io/en/latest/transform.html)
- [Parametric UMAP Paper (Sainburg et al., 2021)](https://direct.mit.edu/neco/article/33/11/2881/107068/Parametric-UMAP-Embeddings-for-Representation-and)
- [Bernardo et al. - A General Framework for Visualization of Sound Collections in Musical Interfaces](https://www.mdpi.com/2076-3417/11/24/11926)
- [Comparative Audio Analysis with UMAP, t-SNE and PCA (Fedden)](https://medium.com/@LeonFedden/comparative-audio-analysis-with-wavenet-mfccs-umap-t-sne-and-pca-cb8237bfce2f)
- [knncolle - Collection of KNN algorithms in C++](https://github.com/knncolle/knncolle)
- [UMAP Original Paper (McInnes, Healy, Melville 2020)](https://arxiv.org/abs/1802.03426)
- [JUCE AsyncUpdater Documentation](https://docs.juce.com/master/classAsyncUpdater.html)
- [JUCE ThreadWithProgressWindow](https://docs.juce.com/master/classjuce_1_1ThreadWithProgressWindow.html)
- [PCA Implementation in C++ with Eigen](https://blog.demofox.org/2022/07/10/programming-pca-from-scratch-in-c/)
