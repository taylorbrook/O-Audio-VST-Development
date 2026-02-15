# Stage 2: DSP Implementation — Execution Plan

**Plugin:** O-TextureForge
**Stage:** 2 of 4 (DSP)
**Created:** 2026-02-14
**Source:** CONTEXT.md, RESEARCH.md, 4 sub-research documents (~3,500 lines)

---

## Goal

Implement the complete concatenative synthesis DSP engine: background file loading with resampling, 19D descriptor extraction (13 MFCCs + 6 spectral/temporal features), nanoflann KD-tree integration with allocation-free audio-thread queries, and a 64-voice polyphonic grain scheduler with 3 MIDI modes — all producing audible output from a loaded audio corpus with no GUI beyond the existing Stage 1 WebView placeholder.

---

## Task Breakdown

### Phase 2.1: File Loading & Grain Segmentation

#### Task 1: Create SharedCorpus and GrainMetadata data structures
- **Files:** `Source/dsp/SharedCorpus.h`, `Source/dsp/GrainMetadata.h`
- **Depends on:** nothing
- **Details:**
  - `GrainMetadata` struct: `startSample` (int), `lengthSamples` (int), `descriptors` (`std::array<float, 19>`)
  - `NormalizationStats` struct: `means[19]`, `stddevs[19]` (for z-score mapping)
  - `SharedCorpus` struct (immutable after construction):
    - `juce::AudioBuffer<float> audioBuffer` (mono, resampled)
    - `std::vector<GrainMetadata> grains`
    - `NormalizationStats normStats`
    - `int grainSizeSamples`, `int hopSizeSamples`
    - `double sampleRate`
    - `juce::String filePath` (for state persistence)
  - All fields set at construction time, read-only thereafter

#### Task 2: Create CorpusLoader background thread
- **Files:** `Source/dsp/CorpusLoader.h`, `Source/dsp/CorpusLoader.cpp`
- **Depends on:** Task 1
- **Details:**
  - Subclass `juce::Thread` ("TextureForge-CorpusLoader")
  - Own `juce::AudioFormatManager` (call `registerBasicFormats()` in constructor)
  - `startLoading(const juce::File& file, double targetSampleRate, float grainSizeMs)` — stores params, starts thread
  - `run()` pipeline:
    1. Read file via `AudioFormatManager::createReaderFor(file)`
    2. Read into `AudioBuffer<float>` (check `threadShouldExit()` after)
    3. Downmix to mono: `addFrom()` + `applyGain(1.0f / numChannels)`
    4. Resample to target SR using `juce::LagrangeInterpolator`
    5. Segment into grains: `numGrains = ((totalSamples - grainSizeSamples) / hopSizeSamples) + 1` (full grains only, 50% overlap)
    6. Populate `GrainMetadata` array with `startSample` and `lengthSamples` for each grain
    7. *(Phase 2.2 will add descriptor extraction + KD-tree build here)*
    8. Create `SharedCorpus` on heap, publish via callback
  - Thread-safe result delivery: `std::function<void(std::shared_ptr<SharedCorpus>)> onCorpusReady` callback, invoked via `juce::MessageManager::callAsync`
  - Release intermediate buffers early (`monoBuffer = {}` after resampling)
  - Check `threadShouldExit()` between each major pipeline step

#### Task 3: Integrate CorpusLoader into PluginProcessor
- **Files:** `Source/PluginProcessor.h`, `Source/PluginProcessor.cpp`
- **Depends on:** Task 2
- **Details:**
  - Add members: `std::unique_ptr<CorpusLoader> corpusLoader`, `std::shared_ptr<SharedCorpus> currentCorpus` (message-thread owned), `std::atomic<SharedCorpus*> corpusForAudio { nullptr }` (audio-thread reads)
  - Add `AudioFormatManager formatManager` member (used by CorpusLoader)
  - Add `loadFile(const juce::File& file)` method:
    - Stop any in-progress load (`corpusLoader->signalThreadShouldExit(); corpusLoader->waitForThreadToFinish(2000)`)
    - Create new CorpusLoader, set callback to receive SharedCorpus
    - Callback: store in `currentCorpus`, publish raw ptr via `corpusForAudio.store()`
  - In `prepareToPlay()`: detect sample rate change, re-trigger load if corpus exists
  - In `getStateInformation()`: save corpus file path in ValueTree child "CORPUS"
  - In `setStateInformation()`: restore file path, call `loadFile()` async
  - In destructor: stop loader thread before member destruction

#### Task 4: Add drag-and-drop to PluginEditor
- **Files:** `Source/PluginEditor.h`, `Source/PluginEditor.cpp`
- **Depends on:** Task 3
- **Details:**
  - Add `juce::FileDragAndDropTarget` to TextureForgeEditor inheritance
  - Implement `isInterestedInFileDrag()` — accept `.wav`, `.aiff`, `.mp3`, `.flac`, `.ogg`, `.m4a`
  - Implement `filesDropped()` — call `processorRef.loadFile(files[0])` (first file only)
  - This works over WebView (native OS-level drops intercept before WebView)

---

### Phase 2.2: Descriptor Extraction (19D)

#### Task 5: Create MFCCExtractor
- **Files:** `Source/dsp/MFCCExtractor.h`, `Source/dsp/MFCCExtractor.cpp`
- **Depends on:** nothing (standalone utility)
- **Details:**
  - Constructor: `MFCCExtractor(double sampleRate, int fftOrder = 11)` (2048-point FFT)
  - `prepare()` — pre-compute:
    - `juce::dsp::FFT fft { 11 }` (2048-point)
    - `juce::dsp::WindowingFunction<float> window { 2048, juce::dsp::WindowingFunction<float>::hann, false }` (normalise=false)
    - Mel filterbank: 40 triangular filters, 20Hz to Nyquist
    - DCT matrix: 13x40 (pre-computed cosine coefficients)
  - `extractMFCCs(const float* grainSamples, int numSamples, float* outMFCCs13)`:
    1. Copy to work buffer (2048 samples, zero-pad if shorter)
    2. Apply pre-emphasis (alpha = 0.97): `out[n] = in[n] - 0.97 * in[n-1]`
    3. Apply Hann window
    4. `fft.performFrequencyOnlyForwardTransform(data, true)` → magnitudes in `data[0..1024]`
    5. Apply mel filterbank (40 filters) → 40 mel energies
    6. Log compress: `log(max(energy, 1e-10f))`
    7. DCT → 13 MFCCs
  - **CRITICAL:** Use `performFrequencyOnlyForwardTransform` (NOT `performRealOnlyForwardTransform`)

#### Task 6: Create DescriptorExtractor
- **Files:** `Source/dsp/DescriptorExtractor.h`, `Source/dsp/DescriptorExtractor.cpp`
- **Depends on:** Task 5
- **Details:**
  - Constructor: `DescriptorExtractor(double sampleRate)`
  - Owns an `MFCCExtractor` instance
  - `extractDescriptors(const float* grainSamples, int numSamples, std::array<float, 19>& out)`:
    - **From raw samples** (before any processing):
      - `out[17]` = RMS Energy: `sqrt(mean(x^2))`
      - `out[18]` = Zero-Crossing Rate: sign changes / (N-1)
    - **From MFCCExtractor** (handles windowing, FFT internally):
      - `out[0..12]` = 13 MFCCs
    - **From magnitude spectrum** (reuse FFT output from MFCCExtractor):
      - `out[13]` = Spectral Centroid: `sum(k * mag[k]) / sum(mag[k])`
      - `out[14]` = Spectral Flatness: `exp(mean(log(mag))) / mean(mag)`
      - `out[15]` = Spectral Flux: half-wave rectified difference (0 for single-frame grains, i.e. grains < 2 * FFT_SIZE)
      - `out[16]` = Spectral Rolloff: frequency below which 85% of energy is concentrated
    - Return magnitudes from MFCCExtractor so spectral descriptors reuse the same FFT pass (avoid double-FFT)
  - `normalizeCorpus(std::vector<GrainMetadata>& grains, NormalizationStats& stats)`:
    - Two-pass z-score normalization across all grains
    - Pass 1: compute mean per dimension
    - Pass 2: compute stddev per dimension (guard: `if (stddev < 1e-10f) stddev = 1.0f`)
    - Normalize all grain descriptors in-place
    - Store means/stddevs in `NormalizationStats` for target mapping

#### Task 7: Integrate descriptor extraction into CorpusLoader
- **Files:** `Source/dsp/CorpusLoader.cpp`
- **Depends on:** Tasks 2, 6
- **Details:**
  - After segmentation step in `run()`:
    1. Create `DescriptorExtractor(targetSampleRate)`
    2. For each grain: extract 19D descriptors into `grain.descriptors`
    3. Call `normalizeCorpus()` to z-score normalize all descriptors
    4. Check `threadShouldExit()` every 100 grains

---

### Phase 2.3: KD-Tree Nearest-Neighbor Search

#### Task 8: Add nanoflann dependency to CMakeLists.txt
- **Files:** `CMakeLists.txt` (plugin-level)
- **Depends on:** nothing
- **Details:**
  - Add `include(FetchContent)` (if not already present)
  - `FetchContent_Declare(nanoflann GIT_REPOSITORY https://github.com/jlblancoc/nanoflann.git GIT_TAG v1.9.0)`
  - `set(NANOFLANN_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)`
  - `set(NANOFLANN_BUILD_TESTS OFF CACHE BOOL "" FORCE)`
  - `FetchContent_MakeAvailable(nanoflann)`
  - `target_link_libraries(OuariconTextureForge PRIVATE nanoflann::nanoflann)`

#### Task 9: Create KDTreeSearch wrapper
- **Files:** `Source/dsp/KDTreeSearch.h`, `Source/dsp/KDTreeSearch.cpp`
- **Depends on:** Tasks 1, 8
- **Details:**
  - `static constexpr int kDescriptorDims = 19;`
  - `GrainDescriptorAdaptor` class wrapping `std::vector<GrainMetadata>&`:
    - `kdtree_get_point_count()` → `grains.size()`
    - `kdtree_get_pt(idx, dim)` → `grains[idx].descriptors[dim]`
    - `kdtree_get_bbox()` → return false (auto-compute)
  - Type alias: `KDTree_t = nanoflann::KDTreeSingleIndexAdaptor<L2_Simple_Adaptor<float, GrainDescriptorAdaptor>, GrainDescriptorAdaptor, kDescriptorDims, uint32_t>`
  - `KDTreeSearch` class:
    - `build(const std::vector<GrainMetadata>& grains)` — constructs adaptor + tree (background thread)
    - `findNearest(const std::array<float, kDescriptorDims>& target)` → `uint32_t grainIndex` — allocation-free k=1 query (audio-thread safe)
    - `findKNearest(const std::array<float, kDescriptorDims>& target, int k, uint32_t* outIndices, float* outDistances)` — pre-allocated arrays
    - Uses `nanoflann::KNNResultSet<float, uint32_t>` with stack-allocated arrays

#### Task 10: Integrate KD-tree into SharedCorpus and CorpusLoader
- **Files:** `Source/dsp/SharedCorpus.h`, `Source/dsp/CorpusLoader.cpp`
- **Depends on:** Tasks 9, 7
- **Details:**
  - Add `std::unique_ptr<KDTreeSearch> kdTree` to `SharedCorpus`
  - In CorpusLoader `run()`, after descriptor normalization:
    1. Create `KDTreeSearch`
    2. Call `build(grains)` to construct KD-tree index
    3. Store in `SharedCorpus.kdTree`
  - KD-tree now published atomically with corpus (single pointer swap)

---

### Phase 2.4: Polyphonic Grain Scheduler + MIDI Modes

#### Task 11: Create GrainVoice and voice pool
- **Files:** `Source/dsp/GrainVoice.h`
- **Depends on:** nothing
- **Details:**
  - `GrainVoice` struct (POD, no heap allocation):
    - `bool active`, `int grainIndex`, `int grainStartSample`, `int grainLengthSamples`
    - `float readPosition`, `float playbackRate`, `float gain`
    - `int samplesElapsed`, `int ageCounter`
    - `int midiNote` (-1 = no MIDI association), `int midiChannel`
    - `reset()` method to zero all fields
  - `GrainPool` class:
    - `std::array<GrainVoice, 64> voices`
    - `int nextVoice = 0`, `int globalAge = 0`
    - `GrainVoice& allocate()` — round-robin scan for inactive, fallback to oldest-steal
    - `void deactivate(int index)`
    - `int activeCount() const`

#### Task 12: Create GrainScheduler
- **Files:** `Source/dsp/GrainScheduler.h`, `Source/dsp/GrainScheduler.cpp`
- **Depends on:** Tasks 11, 9, 1
- **Details:**
  - Owns a `GrainPool`
  - `prepare(double sampleRate)` — store SR, reset pool
  - `processBlock(juce::AudioBuffer<float>& output, juce::MidiBuffer& midi, const SharedCorpus* corpus, const SchedulerParams& params)`:
    - `SchedulerParams` struct: energy, brightness, texture, grainDensity, grainSizeMs, variation, crossfade, outputGainDb, midiMode (all read from atomic params)
    - `juce::ScopedNoDenormals` at top
    - Build target descriptor: `target[17] = (energy * 2) - 1`, `target[13] = (brightness * 2) - 1`, `target[14] = (texture * 2) - 1`, all others = 0
    - Process per MIDI mode:
      - **Mode 2 (Drone):** Internal timer `samplesUntilNextGrain`, countdown per sample, on trigger: KD-tree query → allocate voice → set rate=1.0, gain=1.0
      - **Mode 0 (Pitch-Mapped):** MIDI note-on → KD-tree query → allocate voice → rate = `pow(2, (note-60)/12.0f)`, gain from velocity. Note-off → mark voice midiNote=-1 (let grain finish)
      - **Mode 1 (Trigger+Modulate):** MIDI note-on → KD-tree query with velocity-scaled variation → allocate voice → rate=1.0, CC1→scatterX, aftertouch→scatterY
    - MIDI iteration: range-for on `MidiBuffer` (NOT deprecated Iterator), handle `isNoteOn()`, `isNoteOff()`, velocity-0 note-on = note-off
    - Render all active voices:
      - For each sample in block: iterate 64 voices, linear interpolation from corpus buffer, Hann envelope `0.5f * (1.0f - cos(twoPi * phase))` where `phase = samplesElapsed / (float)grainLengthSamples`
      - Bounds check: deactivate if readPosition exceeds grain length or corpus length
      - Write mono grain sample to both output channels (mono corpus → stereo output)
      - Advance `readPosition += playbackRate`, `samplesElapsed++`
    - Apply output gain: `juce::FloatVectorOperations::multiply()` on both channels
  - `getVizSnapshot()` — populate VizSnapshot with active grain indices + envelopes

#### Task 13: Update VizSnapshot and integrate scheduler into processBlock
- **Files:** `Source/PluginProcessor.h`, `Source/PluginProcessor.cpp`
- **Depends on:** Tasks 12, 3
- **Details:**
  - Update `VizSnapshot` struct:
    ```
    struct ActiveGrain { int grainIndex; float envelope; float readPositionNorm; };
    int activeCount = 0;
    std::array<ActiveGrain, 64> activeGrains {};
    float cursorX = 0.5f, cursorY = 0.5f;
    ```
  - Add `GrainScheduler grainScheduler` member
  - In `prepareToPlay()`: call `grainScheduler.prepare(sampleRate)`
  - In `processBlock()`:
    1. Read all atomic param values into `SchedulerParams` struct
    2. Load `corpus = corpusForAudio.load(std::memory_order_acquire)`
    3. If corpus != nullptr: call `grainScheduler.processBlock(buffer, midiMessages, corpus, params)`
    4. Else: `buffer.clear()`
    5. Write VizSnapshot to double-buffer

#### Task 14: Add new source files to CMakeLists.txt
- **Files:** `CMakeLists.txt` (plugin-level)
- **Depends on:** Tasks 1-13 (final wiring)
- **Details:**
  - Add all new .cpp files to `target_sources`:
    - `Source/dsp/CorpusLoader.cpp`
    - `Source/dsp/MFCCExtractor.cpp`
    - `Source/dsp/DescriptorExtractor.cpp`
    - `Source/dsp/KDTreeSearch.cpp`
    - `Source/dsp/GrainScheduler.cpp`
  - Header-only files (SharedCorpus.h, GrainMetadata.h, GrainVoice.h) don't need listing

---

## New Files Created (10)

```
Source/dsp/
  SharedCorpus.h             Task 1   Immutable corpus data bundle
  GrainMetadata.h            Task 1   Grain metadata + 19D descriptor array
  CorpusLoader.h             Task 2   Background thread: load + downmix + resample + segment + extract + build tree
  CorpusLoader.cpp           Task 2   Implementation
  MFCCExtractor.h            Task 5   FFT pipeline, mel filterbank, DCT (13 MFCCs)
  MFCCExtractor.cpp          Task 5   Implementation
  DescriptorExtractor.h      Task 6   Orchestrates full 19D extraction + z-score normalization
  DescriptorExtractor.cpp    Task 6   Implementation
  KDTreeSearch.h             Task 9   nanoflann adaptor, tree wrapper, allocation-free query
  KDTreeSearch.cpp           Task 9   Implementation
  GrainVoice.h               Task 11  Voice struct + pool (64 voices, round-robin + oldest-steal)
  GrainScheduler.h           Task 12  MIDI mode routing, density timer, voice spawning
  GrainScheduler.cpp         Task 12  Implementation
```

## Files Modified (4)

```
CMakeLists.txt               Tasks 8, 14  Add nanoflann FetchContent + new source files
Source/PluginProcessor.h     Tasks 3, 13  Add CorpusLoader, corpus pointers, GrainScheduler, VizSnapshot
Source/PluginProcessor.cpp   Tasks 3, 13  Integrate loading, processBlock, state persistence
Source/PluginEditor.h/.cpp   Task 4       Add FileDragAndDropTarget
```

---

## Dependency Graph

```
Task 1 (SharedCorpus/GrainMetadata)
  └── Task 2 (CorpusLoader) ──── Task 3 (Processor integration) ──── Task 4 (Drag-and-drop)
  │                                   │
  │                                   └── Task 13 (processBlock integration)
  │                                            ↑
Task 5 (MFCCExtractor)                         │
  └── Task 6 (DescriptorExtractor) ── Task 7 (integrate into CorpusLoader)
                                            │
Task 8 (nanoflann CMake)                    │
  └── Task 9 (KDTreeSearch) ────── Task 10 (integrate into CorpusLoader/Corpus)
                                            │
Task 11 (GrainVoice/Pool)                  │
  └── Task 12 (GrainScheduler) ── Task 13 (processBlock integration)
                                            │
                                   Task 14 (CMakeLists source files)
```

## Execution Waves

**Wave 1** (parallel, no dependencies):
- Task 1: SharedCorpus + GrainMetadata
- Task 5: MFCCExtractor
- Task 8: nanoflann CMake
- Task 11: GrainVoice + Pool

**Wave 2** (depends on Wave 1):
- Task 2: CorpusLoader (depends on Task 1)
- Task 6: DescriptorExtractor (depends on Task 5)
- Task 9: KDTreeSearch (depends on Tasks 1, 8)

**Wave 3** (depends on Wave 2):
- Task 3: Processor integration (depends on Task 2)
- Task 7: Descriptor integration in CorpusLoader (depends on Tasks 2, 6)
- Task 10: KD-tree integration in CorpusLoader (depends on Tasks 7, 9)
- Task 12: GrainScheduler (depends on Tasks 9, 11)

**Wave 4** (depends on Wave 3):
- Task 4: Drag-and-drop (depends on Task 3)
- Task 13: processBlock integration (depends on Tasks 3, 12, 10)
- Task 14: CMakeLists source files (depends on all source tasks)

---

## Critical Anti-Patterns to Avoid

| Anti-Pattern | Consequence | Correct Pattern |
|-------------|-------------|-----------------|
| `DIM=-1` in nanoflann template | Heap allocation on every query (audio thread unsafe) | `DIM=19` compile-time constant |
| `normalise=true` in WindowingFunction | Non-standard MFCCs | Explicitly pass `false` |
| `performRealOnlyForwardTransform` | Wrong data layout for magnitudes | Use `performFrequencyOnlyForwardTransform(data, true)` |
| RMS/ZCR on windowed data | Wrong loudness/transient values | Compute from raw samples BEFORE windowing |
| `radiusSearch()` on audio thread | Returns `std::vector` (allocates) | Use `findNeighbors()` with `KNNResultSet` |
| Destroying `shared_ptr` on audio thread | Heap deallocation in processBlock | Audio thread reads raw pointer only |
| `MidiBuffer::Iterator` | Deprecated in JUCE 8 | Use range-for with `cbegin()`/`cend()` |
| Velocity-0 note-on not handled | Voices never release | Check `isNoteOff() \|\| (isNoteOn() && velocity == 0)` |
| `std::atomic<std::shared_ptr<>>` | Requires C++20 (project uses C++17) | `std::atomic<SharedCorpus*>` + `shared_ptr` on message thread |

---

## Success Criteria

- [ ] Load WAV/AIFF/MP3/FLAC file via drag-and-drop → corpus built on background thread
- [ ] 19D descriptors extracted for every grain (13 MFCCs + centroid + flatness + flux + rolloff + RMS + ZCR)
- [ ] KD-tree queries return grain indices in <10us (allocation-free)
- [ ] Generative Drone mode: continuous grain output without MIDI notes
- [ ] Pitch-Mapped mode: C3 = original pitch, C4 = octave up
- [ ] Trigger+Modulate mode: velocity controls variation radius
- [ ] Energy/Brightness/Texture knobs audibly shift grain selection character
- [ ] 64 simultaneous voices without audio glitches or clicks
- [ ] No heap allocations on audio thread
- [ ] Plugin builds and passes AU validation
- [ ] 5-minute sustained playback with no crashes or memory leaks
