# O-MicrotonalSampler v1.11.1 — Architecture Review

## Executive Summary

O-MicrotonalSampler is a mature, well-structured JUCE 8 sampler at **1620 + 1746 = 3366 lines C++ core** (PluginProcessor + PluginEditor). Compared to peer plugins (O-Bells 1557 + 1014 = 2571, O-Lyrica 1929 + 1273 = 3202), it carries significant **native function boilerplate** (42 separate `withNativeFunction()` registrations in the editor constructor) and has become the **single largest editor in the codebase at 1746 lines**.

**Key findings:**
- PluginProcessor: balanced, well-partitioned — sample-map, loader, voice, state persistence neatly separated.
- PluginEditor: bloated editor constructor (~700 lines); WebView native function registrations are verbose and repetitive.
- State management: dual truth (APVTS parameters + SmoothedValue smoothers) handled cleanly but could be data-driven.
- Tuning: correctly uses shared `scala-tuning-engine` module (no duplication).
- Sample loading: SampleLoader thread is well-designed; LoadMode / LoadOp pattern is sound.
- Build system: clean, correct; `VERSION` token properly wired post-v1.11.0 fix.

**Biggest ROI win:** Extract WebView native function registrations into a data-driven descriptor table. Reduces editor boilerplate by ~200 lines, improves maintainability, and sets a pattern for other plugins (O-Bells, O-Lyrica) to follow.

---

## 1. God-Class Hotspots

### PluginEditor.cpp Constructor (lines 1170–1760 approx)

**Current state:** Constructor spans ~590 lines. Contains 42 separate `withNativeFunction()` lambdas registered inline, each 8–40 lines. Interspersed WebSliderRelay creation, parameter attachment wiring, processor callback subscription, and WebView configuration all in sequence.

**Issues:**
- **Visual clutter:** Reading the constructor requires parsing 40+ lambda blocks to understand the overall structure.
- **Repetitive:** Each native function follows the same pattern: extract args, validate, call processor method, emit callback. The boilerplate obscures domain logic.
- **Error-prone:** Adding a new native function means finding the right insertion point in a 590-line sequence, easy to miss edge cases (args validation, callback firing order).
- **Testability:** Unit testing native function logic requires instantiating the full editor and WebView (heavy, slow).

**Proposed change:**
1. Extract a **`NativeFunctionDescriptor`** struct:
   ```cpp
   struct NativeFunctionDescriptor {
       juce::String name;
       std::function<void(const juce::Array<juce::var>&, std::function<void(juce::var)>)> handler;
       const char* docstring;  // optional — helps maintainers
   };
   ```
2. Build a **`std::vector<NativeFunctionDescriptor>`** as a class member or static const, populated in a dedicated method `buildNativeFunctionRegistry()`.
3. In constructor, iterate the registry and call `.withNativeFunction(desc.name, desc.handler)`.

**Result:** Constructor shrinks to ~100 lines. Native function logic moves to focused, testable handlers. New functions add 1 line to the registry vector instead of 30 lines in the constructor.

**Estimated effort:** M — requires extracting 40+ lambdas, testing each pathway.

---

### PluginProcessor.cpp Public Interface (lines 154–379)

**Current state:** `OMicrotonalSamplerAudioProcessor` has 40+ public methods. Most are narrowly scoped (loadSampleFolder, overrideLoopPoints, snapshotWaveformPeaks, etc.), but the public surface is large.

**Issues:**
- **Interface churn:** v1.6.0 added `targetLayer`, v1.8.0 added `mergeAsRr`, v1.9.0 added variant cap logic. Each iteration added overloads or new methods.
- **Callback spray:** 5 separate callbacks (`sampleMapChangedCallback`, `missingFolderCallback`, `ambiguousDuplicateCallback`, etc.) instead of a unified event bus or callback registry.
- **State redundancy:** Processor holds both `currentSampleFolder: juce::File` and `loadOpHistory: std::vector<LoadOp>` — the folder is derivable from the history's latest path.

**Current design is defensible** because:
- Each method serves a specific UI gesture (button click, drag-drop, dialog response).
- Callbacks are narrow and typed (reduces C++ cast overhead).
- Public interface is stable (no breakage risk post-v1.6.0).

**Proposed change (if refactoring):**
- Keep public methods as-is for now.
- Consolidate callbacks into a single **`ProcessorEventBus`** or **`ProcessorObserver`** pattern (struct with virtual methods for each event type).
- Editor registers one observer instead of five callbacks.

**Estimated effort:** M — requires testing each callback pathway.

---

## 2. Module Extraction Candidates

### WebView Native Function Boilerplate

**Location:** PluginEditor.cpp, constructor lines 1170–1760.

**Current state:** 42 native function registrations, each with:
- Args extraction (1–3 lines).
- Type coercion and validation (1–5 lines).
- Processor method call (1 line).
- Complete callback (1 line).
- Repeated for 40+ functions.

**Duplication pattern across plugins:**
- **O-Bells** (1014 lines): 15 native functions, similar boilerplate.
- **O-Lyrica** (1273 lines): 12 native functions, same pattern.
- **O-TextureForge** (similar era): ~20+ native functions.

**Extract to:** `modules/core/webview-native-functions/` or inline helpers within a `WebViewNativeFunctionBuilder` class.

**Benefits:**
- Single place to document the args validation contract.
- Reusable for other plugins (O-Bells, O-Lyrica already use similar registrations).
- Reduces O-MicrotonalSampler editor by ~150 lines.

**Estimated effort:** L — requires coordinating across multiple plugins.

---

### Drag-Drop File Handling (v1.0.4 Pattern)

**Location:** PluginEditor.cpp lines 360–550, plus `sampler-app.js` lines 600–800.

**Current state:** Five entry points for sample loading:
1. File-chooser dialog (loadSampleFolderDialog).
2. Drag-drop folder (filesDropped path 2).
3. Drag-drop single file (filesDropped path 1).
4. Drag-drop via streaming (dropSessionAddFile, dropSessionCommitFolder).
5. Per-cell load (loadSingleSampleDialog).

Each invokes a slightly different loading path on the processor.

**Pattern:** Three C++ methods (`dropSessionStart`, `dropSessionAddFile`, `dropSessionCommitFolder`) + four JS methods (`streamFolderEntryToCpp`, `streamSingleFileEntryToCpp`, etc.) to unify drag-drop across WKWebView limitations.

**Observation:** This is clever and necessary (WKWebView strips paths for security), but the pattern repeats exactly in any plugin using drag-drop + WebView. Could be extracted to a reusable module.

**Extract to:** `modules/core/webview-drag-drop-streaming/` — provides the C++ handlers + JS helpers, parameterized by the sample-load callback signature.

**Estimated effort:** L — high complexity due to JS/native bridging, but high ROI (benefits 3+ plugins).

---

### Parameter Layout + Smoother Initialization

**Location:** PluginProcessor.cpp lines 25–100 (createParameterLayout) and prepareToPlay.

**Current state:**
```cpp
outputGainSmoother.reset (sampleRate, 0.01);
expressionSmoother.reset (sampleRate, 0.01);
```

**Pattern:** Each parameter with post-processing (gain, expression squared) gets a SmoothedValue member + manual reset in prepareToPlay.

**Observation:** No duplication within this plugin, but the pattern is manual. Could be data-driven:
```cpp
struct SmootherDescriptor {
    juce::String paramId;
    double timeMs;
    std::function<float(float)> curveFunc;  // e.g., x => x*x for expression
};
std::vector<SmootherDescriptor> smoothers = {...};
// In prepareToPlay:
for (auto& s : smoothers) s.smoother.reset(sampleRate, s.timeMs / 1000.0);
```

**Not a candidate for extraction** because the pattern is single-instance-per-plugin. But worth noting for future simplification if 5+ parameters gain post-processing curves.

---

## 3. Layering Issues

### Editor Directly Consuming Processor Internals

**Location:** PluginEditor.cpp lines 1320–1380 (sample map callback subscription).

**Current state:**
```cpp
processorRef.setSampleMapChangedCallback([this] { 
    webView->emitEventIfBrowserIsVisible("sampleMapUpdated", ...);
});
```

The editor registers a callback and directly reads processor state:
```cpp
webView->emitEventIfBrowserIsVisible("sampleMapUpdated", 
    juce::var(processorRef.snapshotSampleMapJson()));
```

**Observation:** This is a reasonable pattern — the processor exposes a snapshot method, editor consumes the event. No layering violation.

**However:** The callback parameter is opaque (`std::function<void()>`). The processor fires the event but the editor has to pull state via `snapshotSampleMapJson()`. A push model would be cleaner:
```cpp
// Alternative:
void setSampleMapChangedCallback(std::function<void(const juce::String& mapJson)> cb);
```

**Current design is acceptable** because:
- Snapshot is cheap (JSON serialization is <1ms for typical maps).
- Decouples editor from processor state schema changes.
- Lazy evaluation — if multiple listeners subscribe, each pulls once on demand.

**No action required** unless performance bottleneck appears.

---

### Voice Dependency Chain

**Location:** MicrotonalSamplerVoice.h setters, PluginProcessor.cpp constructor.

**Current state:**
```cpp
// In processor ctor, for each voice:
voice->setAPVTS(&parameters);
voice->setTuningEngine(&tuningEngine);
voice->setPendingTuningSource(&vst3Extensions.pendingTuningTable);
voice->setSampleMapSource(&currentSampleMap);
voice->setRrCounterArray(&rrCounters);
```

Each setter is a pointer assignment. Voice holds pointers to processor state that could change (e.g., `currentSampleMap` is atomic-swapped on folder load).

**Observation:** This is intentional and correct. Voice doesn't own the state; it snapshots what it needs at startNote (line 336 in MicrotonalSamplerVoice.cpp):
```cpp
currentMap = sampleMapSource->load();
```

**No layering violation** — voice is a pure consumer. State ownership is clear.

---

### Processor → Editor Bidirectional Communication

**Location:** PluginProcessor.h callbacks (5 separate std::function members).

**Current state:** Processor exposes callbacks for:
1. `sampleMapChangedCallback` — sample map updated.
2. `missingFolderCallback` — saved folder missing on restore.
3. `ambiguousDuplicateCallback` — RR confirm modal.
4. (Implicitly: APVTS parameter changes routed through WebSliderParameterAttachment).

Editor subscribes to all callbacks in constructor.

**Observation:** Tight coupling is acceptable here because:
- Plugin has single editor instance (not multiple views).
- Callbacks are narrow and typed (no string-based event routing).
- Alternative (event bus) would add indirection without benefit at this scale.

**No action required.**

---

## 4. Dead Code

### Disabled Test Fixtures (CMakeLists.txt, lines 111–127)

**Location:** `OMTS_PHASE_2_1_TEST_FIXTURE` option (OFF by default).

**Current state:**
```cmake
option(OMTS_PHASE_2_1_TEST_FIXTURE
    "Enable Phase 2.1 in-memory sine-burst SampleMap for early audio testing" OFF)
if(OMTS_PHASE_2_1_TEST_FIXTURE)
    target_compile_definitions(O-MicrotonalSampler PRIVATE
        O_MICROTONAL_SAMPLER_PHASE_2_1_TEST_FIXTURE=1)
endif()
```

In PluginProcessor.cpp prepareToPlay, the gate is checked but the comment says "Phase 2.2 flipped this OFF (real folder load via SampleLoader supersedes the fixture); the macro-gated code remains in source as a regression safety net."

**Assessment:** Code is explicitly gated and documented. The gate has remained OFF since Phase 2.2 (April 2026, 4+ months). The feature is a regression safety net, not dead code.

**Action:** Keep as-is. Remove only if v2.0.0 is released with breaking changes that make regression testing unnecessary.

---

### Unused Test Targets (CMakeLists.txt, lines 128–160)

**Location:** Two standalone test executables (`O-MicrotonalSampler_AliasingCheck`, `O-MicrotonalSampler_MergeRrCheck`) with `EXCLUDE_FROM_ALL`.

**Current state:** Both targets build successfully but are not part of the default plugin build. Intended for manual developer run.

**Assessment:** Not dead code — they're developer tools. The EXCLUDE_FROM_ALL flag is correct (don't bloat CI builds). Keep.

---

### Unused WebView Drag Overlay Classes (v1.0.2)

**Location:** CHANGELOG.md lines 831–870.

**Current state:** v1.0.2 attempted a transparent overlay NSView (`WebViewDragOverlay.{h,mm}`) for drag-drop interception. v1.0.3 moved to JS-layer handling. The files are **no longer in the repo** (CHANGELOG confirms removal). Not dead code.

**Assessment:** Cleaned up correctly post-v1.0.3.

---

### Fallback Paths in Native Function Handlers

**Location:** PluginEditor.cpp lines 287–310 (loadSampleFolderDialog args validation).

**Current state:**
```cpp
const int  targetLayer = args.size() > 0 ? ... : 0;
const auto modeStr     = args.size() > 1 ? ... : juce::String("replace_all");
const bool overrideTok = args.size() > 2 ? ... : false;
```

These fallbacks allow stale JS bundles to call the native function without args and get v1.5.x behavior. Appropriate for forward compatibility (newer DAW host with old plugin JS).

**Assessment:** Not dead code — it's a compatibility bridge. Keep.

---

### tuning-panel-readonly.css

**Location:** `Resources/ui/css/tuning-panel-readonly.css` (embedded in binary, not used by index.html).

**Current state:** CHANGELOG v1.2.0 notes: "The read-only CSS file is preserved on disk and as a binary resource for backward compatibility but is no longer applied."

**Assessment:** Dead code. Safe to remove unless a future variant re-enables read-only mode.

**Action (S):** Remove `tuning-panel-readonly.css` from CMakeLists.txt and Source tree.

---

## 5. Simplification Opportunities

### Data-Driven Native Function Registry (HIGH ROI)

**Issue:** 42 `withNativeFunction()` calls in constructor, each 8–40 lines of boilerplate.

**Root cause:** Native functions are registered inline; each registration is ad-hoc.

**Proposal:**
1. Define a descriptor table (can be in header or .cpp):
   ```cpp
   static constexpr std::array<NativeFunctionDescriptor, 42> NATIVE_FUNCTIONS = {{
       { "loadSampleFolderDialog", &handleLoadSampleFolder, "Load folder into grid" },
       { "clearSampleMap", &handleClearSampleMap, "Wipe all samples" },
       ...
   }};
   ```
2. Iterate in constructor:
   ```cpp
   for (const auto& desc : NATIVE_FUNCTIONS) {
       webView->registerNativeFunction(desc.name, desc.handler);
   }
   ```

**Benefits:**
- Constructor shrinks by ~200 lines.
- Handlers become focused, testable functions instead of lambdas.
- New functions add 1 line to table instead of 30 lines in constructor.
- Documentation (docstring) is colocated with handler.
- Pattern is reusable in O-Bells, O-Lyrica (3+ plugins share this pattern).

**Estimated effort:** M (42 lambda extractions, each 1–2 min).

**Impact:** PluginEditor.cpp drops from 1746 to ~1500 lines. Makes the editor readable again.

---

### Consolidate State Mutation in LoadOp Replay

**Location:** PluginProcessor.cpp `setStateInformation` and `kickNextReplayOp()` (lines 630–700).

**Issue:** State restore triggers a sequence of async folder loads (replay queue). Each load appends to `loadOpHistory` and fires callbacks. The flow is spread across three methods (`restoreStateValueTree`, `restoreStateValueTree -> kickNextReplayOp`, `applyFolderLoad`).

**Current state:** Clear and correct, but could be tightened.

**Proposal:** Introduce a **`StateRestoreContext`** struct to carry the replay queue + missing-folder callback through the sequence, making the flow explicit.

**Estimated effort:** S — mostly refactoring for clarity, no functional change.

**Impact:** Easier to reason about state restore lifecycle. Helps future maintainers understand the replay-queue contract.

---

### Parameter Value Lookups (Minor DRY)

**Location:** PluginProcessor.cpp lines 182, 189, 318, 326, 338, 357 (parameter value reads).

**Current state:**
```cpp
if (auto* gp = parameters.getRawParameterValue("output_gain"))
    outputGainSmoother.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(gp->load()));
```

Repeated 6 times (output_gain, expression, polyphony). The pattern is consistent.

**Observation:** Not a significant issue — only 6 reads in 1620-line processor. The direct calls are readable.

**No action required** unless the parameter count grows 5x.

---

## 6. Build System

### CMakeLists.txt Analysis

**Location:** O-MicrotonalSampler/CMakeLists.txt (lines 1–144).

**Current state:** Clean, well-structured. Correctly:
- Uses `juce_add_plugin` with `VERSION "1.11.1"` (post-v1.11.0 fix).
- Includes all source files explicitly.
- Embeds binary resources (`juce_add_binary_data`) for UI and images.
- Links shared tuning module via `ouaricon_add_module`.
- Disables JUCE's default curl, adds WebView2.
- Gates licensing module on compile flag.

**Issues identified:** None.

**Minor observations:**
- Test targets (aliasing_check, merge_rr_check) are EXCLUDE_FROM_ALL, not part of the plugin build. Correct.
- Binary resources (paper1.jpg, paper2.jpg, brains.png) are embedded; no streaming overhead. Good for runtime reliability.

**Recommended improvement (S):**
Separate the test executables into a dedicated CMakeLists.txt in `Source/tests/` for clarity, but current approach is acceptable.

---

## 7. Code Quality Observations (Positive)

### Strong Points

1. **RT-Safe Audio Thread:**
   - Voice rendering uses lock-free atomics (rrCounters array).
   - Sample-map swaps use atomic_store/load (no locks in audio path).
   - Smoothers (ADSR, gain, expression) are pre-allocated, no RT allocations.

2. **State Persistence:**
   - Folder load ops are replay-able (loadOpHistory).
   - ValueTree schema is versioned (v1.5.x compat for missing-folder paths).
   - Tuning state is captured + restored via TuningEngine accessors.

3. **WebView Architecture:**
   - Native function names are consistent with JS caller expectations.
   - Binary resource provider avoids network latency.
   - Drag-drop streaming works around WKWebView sandbox (clever v1.0.4 workaround).

4. **Voice Design:**
   - Per-cell variant selection (round-robin) is pure integer math (no allocation).
   - Dual-cell velocity crossfade is equal-power (audio-quality correct).
   - Loop-point boundary crossfade (8-sample xfade) eliminates clicks.

---

## Ranking by ROI (High Impact + Low Effort First)

| **Issue** | **File** | **Type** | **Effort** | **Impact** | **Priority** |
|-----------|----------|---------|----------|-----------|--------------|
| Remove `tuning-panel-readonly.css` | CMakeLists.txt + Resources/ | Dead code | S | Low | DEFERRED |
| Data-driven native function registry | PluginEditor.cpp | Simplification | M | HIGH — shrinks editor 200 lines, sets pattern for peers | **1st** |
| Extract drag-drop streaming module | PluginEditor.cpp + sampler-app.js | Module extraction | L | HIGH — 3+ plugins need this; unifies WKWebView workaround | **2nd** |
| Consolidate callbacks into observer pattern | PluginProcessor.h | Refactoring | M | Medium — improves clarity; late-stage optimization | **3rd** |
| Separate test targets to tests/CMakeLists.txt | CMakeLists.txt | Build cleanup | S | Low | Nice-to-have |

---

## Summary of Recommended Actions

### Immediate (v1.12.0)

1. **Extract native function registry** — data-driven approach reduces PluginEditor from 1746 → 1500 lines, improves readability. Establishes pattern for O-Bells, O-Lyrica refactoring.

### Near-term (v1.13.0 / v2.0.0)

2. **Extract drag-drop streaming** — WKWebView workaround is reusable across 3+ plugins. Centralizes the knowledge, reduces duplication.

### Low-priority / Deferred

3. Clean up dead CSS file.
4. Refactor state restore into explicit context object (clarity, not critical).
5. Move test targets to dedicated CMakeLists.

---

## Conclusion

O-MicrotonalSampler is **well-architected and production-ready**. The plugin has grown maturely across 11 versions with sound design decisions (RT-safety, modular state loading, WebView integration). No urgent refactoring is needed for correctness or performance.

**Biggest win:** Extracting the native function registry (42 lambdas → data table) yields immediate code clarity and establishes a reusable pattern for the plugin ecosystem.
