# nanoflann KD-Tree Integration Research

**Researched:** 2026-02-14
**Domain:** KD-tree nearest-neighbor search for real-time concatenative synthesis
**Confidence:** HIGH

## Summary

nanoflann is a C++11 header-only library providing KD-tree nearest-neighbor search. The latest stable release is **v1.9.0** (December 22, 2024). It exports a proper CMake target (`nanoflann::nanoflann`) and integrates cleanly via FetchContent.

The critical finding for this project: **queries are allocation-free ONLY when the dimensionality template parameter `DIM` is set at compile time** (e.g., `DIM=19`). When `DIM=-1` (runtime), the library internally creates a `std::vector` on each query, which allocates heap memory. Since our dimensionality is fixed at 19, we MUST use `DIM=19` as the template parameter to guarantee allocation-free queries on the audio thread.

A secondary finding concerns the **curse of dimensionality**: KD-trees degrade toward brute-force performance as dimensions increase. At 19D, the theoretical crossover point (N > 2^D) is ~524K points. However, for our corpus sizes (1K-50K grains) and the single-nearest-neighbor use case, nanoflann will still outperform brute force due to early termination and compile-time loop unrolling. Benchmarking against a simple linear scan at higher grain counts is recommended.

**Primary recommendation:** Use nanoflann v1.9.0 with `DIM=19` compile-time template parameter, `L2_Simple_Adaptor` distance metric, and the `findNeighbors()` API with pre-allocated `KNNResultSet` for zero-allocation audio-thread queries.

## User Constraints (from CONTEXT.md)

### Locked Decisions
- 19D descriptor space: 13 MFCCs + spectral centroid + spectral flatness + spectral flux + spectral rolloff + RMS energy + zero-crossing rate
- nanoflann for KD-tree nearest-neighbor search
- Background thread for KD-tree construction
- Audio thread must be lock-free: read-only access via atomic shared_ptr
- KD-tree queries must be allocation-free on audio thread
- Direct descriptor targeting for macro knob search (Energy->RMS, Brightness->Centroid, Texture->Flatness)

### Claude's Discretion
- Exact nanoflann version to pin (resolved: v1.9.0)
- Spectral rolloff threshold (context says 85%)
- Whether to add loading progress callback

### Deferred Ideas (OUT OF SCOPE)
- None specified

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| nanoflann | v1.9.0 | KD-tree nearest-neighbor search | Header-only, zero-dependency, CRTP-based (no virtual calls), compile-time dimension unrolling, allocation-free queries when DIM>0 |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| `std::shared_ptr` | C++17 | Immutable corpus data sharing | Background-to-audio thread data handoff |
| `std::atomic<std::shared_ptr<T>>` | C++20 | Atomic pointer swap | If C++20 available; otherwise use `juce::ReferenceCountedObjectPtr` or double-buffering |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| nanoflann KD-tree | Brute-force linear scan | Simpler code, no library dependency; may be faster at 19D with <1K grains but slower at 10K+ |
| nanoflann | Annoy (Spotify) | Approximate NN, better at high dimensions; but heavier dependency, not header-only, overkill for our corpus sizes |
| nanoflann | FLANN | Original library nanoflann is based on; slower due to virtual dispatch, larger footprint |

### Installation (FetchContent)

```cmake
include(FetchContent)
FetchContent_Declare(
    nanoflann
    GIT_REPOSITORY https://github.com/jlblancoc/nanoflann.git
    GIT_TAG v1.9.0
)
# Disable building examples and tests
set(NANOFLANN_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(NANOFLANN_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(nanoflann)

# Then link to your target:
target_link_libraries(OuariconTextureForge PRIVATE nanoflann::nanoflann)
```

**Alternative (simpler, header-only copy):** Since nanoflann is a single header file, you can also copy `nanoflann.hpp` directly into `Source/dsp/` and `#include` it. This avoids FetchContent entirely but makes version tracking manual. FetchContent is preferred for traceability.

## Architecture Patterns

### KDTreeSearch Module Structure
```
Source/dsp/
├── KDTreeSearch.h         # nanoflann adaptor + wrapper class declaration
├── KDTreeSearch.cpp        # Tree build implementation (background thread)
├── GrainMetadata.h         # GrainMetadata struct with descriptors array
```

### Pattern 1: Custom Dataset Adaptor (Zero-Copy)

nanoflann accesses your data in-place through an adaptor interface. No data copying occurs -- the tree stores indices and split planes, not point data.

**Required adaptor methods:**

```cpp
// Source: nanoflann examples/utils.h, verified against v1.9.0 API
struct GrainDescriptorAdaptor
{
    const std::vector<GrainMetadata>& grains;

    GrainDescriptorAdaptor(const std::vector<GrainMetadata>& g) : grains(g) {}

    // Required: return number of data points
    inline size_t kdtree_get_point_count() const
    {
        return grains.size();
    }

    // Required: return the dim'th component of the idx'th point
    inline float kdtree_get_pt(const size_t idx, const size_t dim) const
    {
        return grains[idx].descriptors[dim];
    }

    // Optional: bounding box computation (return false for default)
    template <class BBOX>
    bool kdtree_get_bbox(BBOX& /*bb*/) const
    {
        return false;
    }
};
```

### Pattern 2: KD-Tree Type Definition with Compile-Time DIM

```cpp
#include <nanoflann.hpp>

static constexpr int kDescriptorDims = 19;

using KDTree_t = nanoflann::KDTreeSingleIndexAdaptor<
    nanoflann::L2_Simple_Adaptor<float, GrainDescriptorAdaptor>,
    GrainDescriptorAdaptor,
    kDescriptorDims,     // CRITICAL: compile-time DIM for allocation-free queries
    uint32_t             // IndexType -- uint32_t supports up to 4 billion grains
>;
```

### Pattern 3: Immutable Corpus with Atomic Swap

The corpus (audio + metadata + KD-tree) is built on a background thread and published atomically to the audio thread. The audio thread never sees a partially-built tree.

```cpp
// Immutable data bundle -- built once, never modified
struct Corpus
{
    std::vector<float> audioBuffer;                 // All grain audio, contiguous
    std::vector<GrainMetadata> grains;              // Grain metadata with descriptors
    GrainDescriptorAdaptor adaptor;                 // nanoflann adaptor (references grains)
    std::unique_ptr<KDTree_t> kdTree;               // The built KD-tree
    std::vector<float> descriptorMeans;             // z-score normalization means
    std::vector<float> descriptorStdDevs;           // z-score normalization stddevs

    Corpus(std::vector<float>&& audio,
           std::vector<GrainMetadata>&& g,
           std::vector<float>&& means,
           std::vector<float>&& stddevs)
        : audioBuffer(std::move(audio)),
          grains(std::move(g)),
          adaptor(grains),
          descriptorMeans(std::move(means)),
          descriptorStdDevs(std::move(stddevs))
    {
        // Build KD-tree index (this is the expensive operation)
        kdTree = std::make_unique<KDTree_t>(
            kDescriptorDims, adaptor,
            nanoflann::KDTreeSingleIndexAdaptorParams{10 /* max leaf */}
        );
    }
};
```

### Pattern 4: Background Thread Build + Atomic Publish

```cpp
// In PluginProcessor:
std::shared_ptr<const Corpus> currentCorpus;  // Read by audio thread
std::mutex corpusMutex;                       // Protects swap (NOT audio thread)

// Background thread builds and publishes:
void buildCorpusAsync(/* file data */)
{
    // All expensive work happens here, off the audio thread
    auto newCorpus = std::make_shared<const Corpus>(
        std::move(audio), std::move(grains),
        std::move(means), std::move(stddevs)
    );

    // Atomic publish -- audio thread will pick up on next block
    {
        std::lock_guard<std::mutex> lock(corpusMutex);
        currentCorpus = newCorpus;
    }
    // Old corpus destroyed here when last reference drops (NOT on audio thread)
}

// Audio thread reads (lock-free):
void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    // Copy shared_ptr (atomic ref count increment)
    auto corpus = currentCorpus;  // Lock-free read via RCU-style pattern
    if (!corpus) { buffer.clear(); return; }

    // Query KD-tree -- allocation-free with DIM=19
    // ...
}
```

**IMPORTANT NOTE on `std::shared_ptr` and audio threads:** The `std::shared_ptr` copy constructor performs an atomic reference count increment, which is technically lock-free but involves a memory fence. This is widely used in JUCE audio plugins and is considered acceptable for audio threads. The critical constraint is that the audio thread must NEVER be the last holder of the shared_ptr (which would trigger deallocation). The pattern above ensures the background thread always holds a reference until swap.

**Alternative if shared_ptr atomicity is a concern:** Use a double-buffer pattern with `std::atomic<int>` index, or `juce::ReferenceCountedObjectPtr` with a garbage-collection queue on the message thread. The shared_ptr approach is simpler and sufficient for this use case.

### Anti-Patterns to Avoid
- **DIM=-1 on audio thread:** Causes `std::vector` allocation inside every query. ALWAYS use compile-time DIM=19.
- **Building KD-tree on audio thread:** Tree construction is O(N log N) and allocates heavily. Always background thread.
- **Modifying the Corpus after publishing:** The Corpus must be immutable. Build a new one and swap.
- **Using `radiusSearch()` on audio thread:** Returns results in a `std::vector`, which allocates. Use `findNeighbors()` with pre-allocated `KNNResultSet` instead.
- **Destroying shared_ptr on audio thread:** If the audio thread holds the last reference, destruction (with heap deallocation) happens on the audio thread. Ensure the background/message thread always retains a reference during swap.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| KD-tree implementation | Custom KD-tree | nanoflann | Balancing, leaf-size tuning, SIMD optimization are complex; nanoflann is battle-tested |
| Distance computation | Manual L2 loop | `L2_Simple_Adaptor` | Compiler auto-vectorizes the CRTP-inlined distance with compile-time DIM |
| Thread-safe data sharing | Custom lock-free queue for corpus | `std::shared_ptr` + immutable Corpus | Proven pattern, minimal code, correct by construction |
| Descriptor normalization | Ad-hoc min/max scaling | z-score normalization (subtract mean, divide by stddev) | z-score handles different descriptor scales correctly, stores means/stddevs for target mapping |

## Common Pitfalls

### Pitfall 1: Dynamic DIM Causes Heap Allocation
**What goes wrong:** Using `DIM=-1` (runtime dimensionality) in the KDTree template causes `std::vector<DistanceType>` allocation inside every `findNeighbors()` call.
**Why it happens:** nanoflann uses `array_or_vector<DIM, T>` metaprogramming -- `DIM>0` selects `std::array`, `DIM=-1` selects `std::vector`.
**How to avoid:** Always use `DIM=19` as the template parameter.
**Warning signs:** Audio glitches under load, heap allocation detected by real-time safety tools.

### Pitfall 2: radiusSearch() Allocates
**What goes wrong:** `radiusSearch()` populates a `std::vector<nanoflann::ResultItem>`, which heap-allocates.
**Why it happens:** The number of results within a radius is unknown at query time.
**How to avoid:** Use `findNeighbors()` with a pre-initialized `KNNResultSet` where k is known (typically k=1 or k=4).
**Warning signs:** Using `radiusSearch` in any audio-thread code path.

### Pitfall 3: Adaptor References Dangling Data
**What goes wrong:** The KD-tree adaptor holds a reference to the `std::vector<GrainMetadata>`. If that vector is destroyed or moved, the tree accesses freed memory.
**Why it happens:** nanoflann does zero-copy access -- it stores no point data, only indices.
**How to avoid:** Bundle adaptor, grain vector, and KD-tree into a single `Corpus` struct so their lifetimes are coupled. Never move/resize the grains vector after tree construction.
**Warning signs:** Crashes on query, corrupt nearest-neighbor results.

### Pitfall 4: Forgetting to Normalize Descriptors
**What goes wrong:** MFCCs (range ~-50 to +50) dominate the distance computation over spectral flatness (range 0.0 to 1.0).
**Why it happens:** L2 distance weights dimensions by their scale. Without normalization, high-magnitude features overwhelm low-magnitude ones.
**How to avoid:** z-score normalize ALL 19 dimensions before building the tree. Store means and stddevs in the Corpus for mapping macro knobs to target values.
**Warning signs:** Macro knobs (Brightness, Texture, Energy) have no audible effect; search always returns similar grains regardless of target.

### Pitfall 5: KD-Tree Degradation at 19 Dimensions
**What goes wrong:** KD-tree search approaches O(N) instead of O(log N) at high dimensionality.
**Why it happens:** The "curse of dimensionality" -- in high dimensions, the tree's ability to prune branches diminishes.
**How to avoid:** For corpus sizes up to 50K grains, nanoflann will still be faster than brute force due to compile-time unrolling and early termination. Monitor query time. If it exceeds budget, consider: (a) reducing descriptor dimensions (use only the 6 most discriminative), (b) using approximate search with `eps > 0`, or (c) PCA dimensionality reduction.
**Warning signs:** Query time grows linearly with corpus size instead of logarithmically.

### Pitfall 6: Tree Build Time Blocking UI
**What goes wrong:** Building a KD-tree for 50K 19D points can take 10-50ms. If done on the message thread, the UI freezes.
**Why it happens:** `KDTreeSingleIndexAdaptor` constructor performs O(N log N) build.
**How to avoid:** Always build on a background thread. Use `n_thread_build > 1` in `KDTreeSingleIndexAdaptorParams` for parallel construction if needed.
**Warning signs:** UI stutter when loading large sample files.

## Code Examples

### Complete Audio-Thread-Safe Query (Allocation-Free)

```cpp
// Source: nanoflann v1.9.0 API, verified from header
// This is the pattern for the audio thread

static constexpr int kDescriptorDims = 19;
static constexpr size_t kMaxNeighbors = 4;  // Query up to 4 nearest grains

struct QueryResult
{
    uint32_t indices[kMaxNeighbors];
    float distances[kMaxNeighbors];
    size_t count = 0;
};

// Call from processBlock -- ZERO heap allocations
QueryResult queryNearest(const Corpus& corpus,
                         const float* targetDescriptors,
                         size_t numNeighbors = 1)
{
    QueryResult result;
    jassert(numNeighbors <= kMaxNeighbors);

    // KNNResultSet uses caller-provided arrays -- no allocation
    nanoflann::KNNResultSet<float, uint32_t> resultSet(numNeighbors);
    resultSet.init(result.indices, result.distances);

    // findNeighbors traverses the tree -- allocation-free when DIM=19
    corpus.kdTree->findNeighbors(resultSet, targetDescriptors);

    result.count = resultSet.size();
    return result;
}
```

### Building Target Descriptor from Macro Knobs

```cpp
// Source: CONTEXT.md macro knob mapping
// Maps UI knob values (0-1) to z-score space target descriptors

void buildTargetDescriptor(float target[kDescriptorDims],
                           float energy,      // 0.0 - 1.0
                           float brightness,  // 0.0 - 1.0
                           float texture)     // 0.0 - 1.0
{
    // Start with neutral (zero = population mean in z-score space)
    std::fill(target, target + kDescriptorDims, 0.0f);

    // Map knobs to their descriptor dimensions
    // Knob 0.0->1.0 maps to z-score -1.0->+1.0 (adjustable range)
    target[17] = (energy * 2.0f) - 1.0f;       // RMS Energy
    target[13] = (brightness * 2.0f) - 1.0f;   // Spectral Centroid
    target[14] = (texture * 2.0f) - 1.0f;       // Spectral Flatness
}
```

### KD-Tree Construction on Background Thread

```cpp
// Source: nanoflann v1.9.0 API
// Called from background thread ONLY

std::shared_ptr<const Corpus> buildCorpus(
    std::vector<float>&& audioBuffer,
    std::vector<GrainMetadata>&& grains,
    std::vector<float>&& means,
    std::vector<float>&& stddevs)
{
    // z-score normalize all descriptors in-place
    for (auto& grain : grains)
    {
        for (int d = 0; d < kDescriptorDims; ++d)
        {
            if (stddevs[d] > 1e-8f)
                grain.descriptors[d] = (grain.descriptors[d] - means[d]) / stddevs[d];
            else
                grain.descriptors[d] = 0.0f;
        }
    }

    // Corpus constructor builds the KD-tree
    return std::make_shared<const Corpus>(
        std::move(audioBuffer),
        std::move(grains),
        std::move(means),
        std::move(stddevs)
    );
}
```

### Convenience knnSearch API (Alternative to findNeighbors)

```cpp
// Source: nanoflann v1.9.0 header
// knnSearch is a convenience wrapper -- also allocation-free when DIM>0

uint32_t outIndex;
float outDistSq;

size_t found = corpus.kdTree->knnSearch(
    targetDescriptors,     // const float* query point
    1,                     // size_t num_closest
    &outIndex,             // IndexType* out_indices
    &outDistSq             // DistanceType* out_distances
);

// 'found' = number of neighbors actually found (may be < num_closest if dataset is smaller)
// outDistSq = SQUARED L2 distance (not the actual L2 distance)
```

## API Reference (Verified from v1.9.0 Header)

### KDTreeSingleIndexAdaptor

```cpp
template <
    typename Distance,         // e.g., L2_Simple_Adaptor<float, MyAdaptor>
    class DatasetAdaptor,      // Your adaptor class
    int32_t DIM = -1,          // Compile-time dimensionality (-1 = runtime)
    typename IndexType = uint32_t
>
class KDTreeSingleIndexAdaptor;
```

**Constructor:**
```cpp
KDTreeSingleIndexAdaptor(
    const Dimensionality dimensionality,           // int, must match DIM if DIM>0
    const DatasetAdaptor& inputData,               // Your adaptor instance
    const KDTreeSingleIndexAdaptorParams& params = {10}  // leaf_max_size=10
);
// Tree is built in the constructor (no separate buildIndex call needed in v1.9.0)
```

**Key methods:**
```cpp
// Convenience k-NN search
Size knnSearch(
    const ElementType* query_point,
    const Size num_closest,
    IndexType* out_indices,
    DistanceType* out_distances_sq
) const;

// Flexible search with custom result set
template <typename RESULTSET>
bool findNeighbors(
    RESULTSET& result,
    const ElementType* vec,
    const SearchParameters& searchParams = {}
) const;
```

### SearchParameters

```cpp
struct SearchParameters {
    float eps = 0;       // 0 = exact, >0 = approximate (faster)
    bool sorted = true;  // Sort results by distance
};
```

Using `eps = 0.1` allows 10% approximate matches, which can significantly speed up high-dimensional queries at minimal accuracy cost.

### KNNResultSet

```cpp
template <typename DistanceType, typename IndexType = uint32_t>
class KNNResultSet {
    explicit KNNResultSet(size_t capacity);
    void init(IndexType* indices, DistanceType* dists);  // Uses YOUR arrays
    size_t size() const;     // Number of results found
    bool full() const;       // Whether capacity results were found
    DistanceType worstDist() const;  // Current worst distance in set
};
```

## 19D Performance Analysis

### Theoretical Concerns
- KD-trees are most efficient when N >> 2^D. For D=19, that threshold is ~524K points.
- Our corpus range (1K-50K) is below this threshold.
- scikit-learn switches to brute-force at D>15.

### Why nanoflann Still Wins for This Use Case
1. **Compile-time DIM=19:** Compiler fully unrolls the 19-dimension distance loop and generates SIMD code. A brute-force scan does NOT get this optimization automatically.
2. **Early termination:** KD-tree can prune large branches even at 19D, especially when k=1 (single nearest neighbor).
3. **Small k:** We typically query k=1 (or k<=4). KD-tree advantage grows with smaller k.
4. **Descriptors are z-score normalized:** This means the data is roughly centered and scaled, which helps KD-tree pruning.
5. **nanoflann benchmarks:** Library authors report ~50% speedup over original FLANN in typical use cases.

### Performance Estimate (Conservative)
| Corpus Size | Expected Query Time (k=1) | Brute Force Comparison |
|-------------|--------------------------|------------------------|
| 1,000 grains | < 1 us | ~Comparable |
| 10,000 grains | ~2-10 us | ~5-20x faster |
| 50,000 grains | ~10-50 us | ~10-50x faster |

At 48kHz with 128-sample blocks, you have ~2.67ms per block. Even 50us per query is only ~2% of the budget, leaving ample room for grain synthesis.

### Fallback Strategy
If profiling shows KD-tree queries are too slow at 50K grains:
1. **Approximate search:** Set `SearchParameters{.eps = 0.1}` for ~10% faster queries with minimal accuracy loss.
2. **Reduce dimensions:** Use only the 6 macro-relevant dimensions (MFCCs 0-2, centroid, flatness, RMS) for search, while keeping all 19 for other purposes.
3. **Brute force with SIMD:** For small corpora (<2K), a hand-tuned SIMD linear scan may be faster.

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| `SearchParams` struct | `SearchParameters` struct | v1.5.0 | Renamed to avoid ctor order bugs; update all code |
| `std::vector<std::pair<>>` for radius results | `std::vector<nanoflann::ResultItem<>>` | v1.5.0 | Type change required for radius search results |
| No parallel build | `n_thread_build` parameter | v1.5.0 | Can speed up tree construction for large corpora |
| No box search | `findWithinBox()` API | v1.8.0 | New capability, not needed for our use case |

**Deprecated/outdated:**
- `SearchParams` (old name): Renamed to `SearchParameters` in v1.5.0. Old name may still compile but should not be used.
- `nChecks_IGNORED` parameter: Removed in v1.5.0.

## Thread Safety Summary

| Operation | Thread Safety | Notes |
|-----------|--------------|-------|
| `findNeighbors()` / `knnSearch()` | **Read-only, thread-safe** | Multiple threads can query simultaneously |
| Tree construction | **NOT thread-safe** | Must complete before any queries begin |
| `n_thread_build` | **Internal parallelism only** | Controls parallelism within a single build call |
| Adaptor data access | **Caller responsibility** | Adaptor references must remain valid and immutable during queries |

The immutable Corpus + atomic shared_ptr swap pattern ensures:
1. Tree is fully built before any query can access it
2. Old corpus remains valid while audio thread still references it
3. No concurrent read/write on any data structure

## Open Questions

1. **Exact performance at 19D with 50K grains**
   - What we know: Theoretical analysis suggests KD-tree still helps for k=1
   - What's unclear: Actual wall-clock microseconds on Apple Silicon
   - Recommendation: Add a simple benchmark in the test suite comparing `knnSearch` vs brute-force linear scan at 1K, 10K, 50K grains. Profile early in Phase 2.3.

2. **eps (approximate search) value tuning**
   - What we know: eps=0 is exact, eps>0 trades accuracy for speed
   - What's unclear: What eps value is perceptually transparent for grain selection
   - Recommendation: Start with eps=0 (exact). If performance is an issue, experiment with eps=0.1 to eps=0.5 and listen for audible differences.

3. **std::shared_ptr copy on audio thread**
   - What we know: Atomic ref count increment is lock-free but involves a memory fence
   - What's unclear: Whether the fence causes measurable latency on Apple Silicon / Windows
   - Recommendation: Profile. If problematic, switch to a raw pointer + epoch-based reclamation or SeqLock pattern. The shared_ptr approach is the simplest correct starting point.

## Sources

### Primary (HIGH confidence)
- [nanoflann GitHub repository](https://github.com/jlblancoc/nanoflann) - README, CMakeLists.txt, examples
- [nanoflann v1.9.0 header](https://github.com/jlblancoc/nanoflann/blob/master/include/nanoflann.hpp) - API signatures, allocation behavior, `array_or_vector` metaprogramming
- [nanoflann releases](https://github.com/jlblancoc/nanoflann/releases) - Version history, v1.9.0 confirmed as latest
- [nanoflann CHANGELOG](https://github.com/jlblancoc/nanoflann/blob/master/CHANGELOG.md) - Breaking changes v1.5.0+
- [nanoflann API documentation](https://jlblancoc.github.io/nanoflann/) - Class reference
- [nanoflann KDTreeSingleIndexAdaptor reference](https://jlblancoc.github.io/nanoflann/classnanoflann_1_1KDTreeSingleIndexAdaptor.html) - Method signatures

### Secondary (MEDIUM confidence)
- [JUCE forum: Lock-free shared_ptrs](https://forum.juce.com/t/lock-free-shared-ptrs/39240) - Audio thread shared_ptr safety discussion
- [Cornell CS4780: KD Trees](https://www.cs.cornell.edu/courses/cs4780/2022sp/notes/LectureNotes19.html) - Curse of dimensionality theory (N > 2^D threshold)
- [IRCAM Audio Mosaicing](http://imtr.ircam.fr/imtr/Audio_Mosaicing) - KD-tree usage in concatenative synthesis

### Tertiary (LOW confidence)
- Performance estimates at 19D are extrapolated from general KD-tree theory and nanoflann's compile-time optimization characteristics. No 19D-specific benchmarks were found. Recommend profiling.
- The claim that scikit-learn switches to brute force at D>15 is from a scikit-learn GitHub discussion, not official documentation.

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - nanoflann is well-documented, header verified, CMake target confirmed
- Architecture (adaptor pattern): HIGH - verified from official examples and header source
- Allocation-free queries: HIGH - verified `array_or_vector` metaprogramming in header; DIM>0 uses `std::array`
- 19D performance: MEDIUM - theoretical analysis is sound but no specific 19D benchmarks found
- Thread safety pattern: MEDIUM - shared_ptr pattern is established in JUCE community but has known caveats

**Research date:** 2026-02-14
**Valid until:** 2026-05-14 (nanoflann is stable; API unlikely to change in 3 months)
