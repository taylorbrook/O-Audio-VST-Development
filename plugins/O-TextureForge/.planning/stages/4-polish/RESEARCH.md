# Stage 4: Integration & Polish - Research

**Date:** 2026-02-14
**Plugin:** O-TextureForge
**Stage:** 4-polish (Integration & Polish)
**Confidence:** HIGH

---

## 1. Pluginval Strictness 10

### What It Tests

Pluginval at strictness 10 (industry standard for release quality) runs the full test suite:

- **Plugin lifecycle:** Load/unload cycles, multiple instances, rapid create/destroy
- **Parameter automation:** All parameters respond to `setValue()` from multiple threads concurrently
- **State save/restore:** Round-trip validation — save state, randomize parameters, restore state, verify match
- **Audio processing:** Zero-sample buffers, variable buffer sizes, NaN/Inf/subnormal detection in output
- **Sample rate handling:** Multiple `prepareToPlay` calls with varying sample rates without `releaseResources` between
- **Bus layouts:** Mono/stereo configuration changes
- **MIDI handling:** Note on/off, CC, pitch bend
- **GUI validation:** Editor create/destroy cycles (rapid open/close)
- **auval integration:** macOS Audio Unit validation (at strictness 5+)

### Current State Analysis — What Needs Hardening

**PluginProcessor.cpp `processBlock`:**
- Missing zero-sample buffer guard (`buffer.getNumSamples() == 0`)
- No NaN/Inf output protection (pluginval explicitly checks for NaN/Inf in output buffers)
- Corpus pointer read is safe (atomic), but no guard for partially-loaded corpus

**GrainScheduler.cpp `processBlock`:**
- Calls `buffer.clear()` inside `renderVoices()` — safe
- `sampleFromCorpus()` has bounds check — good
- `GrainVoice::getEnvelope()` returns 0 for `grainLengthSamples == 0` — safe

**PluginProcessor.cpp `prepareToPlay`:**
- Already handles sample rate change by reloading corpus — good
- But `loadCorpusFile` runs on background thread, meaning audio thread could see stale corpus during reload
- Should also call `grainScheduler.reset()` on sample rate change to prevent stale voice state

**State save/restore:**
- `setStateInformation` already checks `file.existsAsFile()` before loading — good
- Missing: graceful fallback when file doesn't exist (currently silently ignores; should notify editor to show drop zone)
- Current CORPUS XML structure stores path correctly

### Specific Code Changes Needed

1. **processBlock zero-buffer guard** — Early return if `buffer.getNumSamples() == 0`
2. **NaN/Inf output clipping** — `juce::FloatVectorOperations::clip()` after gain stage
3. **prepareToPlay idempotency** — Reset grain voices on sample rate change, ensure no state leaks
4. **State restore missing-file handling** — Notify editor to show "file missing" message

---

## 2. Edge Cases (Full Scope from CONTEXT.md)

### Edge Case 1: No File Loaded

**Current behavior:** GrainScheduler checks `corpus != nullptr` before processing, outputs silence. WebView shows whatever was last rendered (could be blank on first load).

**Needed:** WebView should show "Drop audio file here" message when no corpus is loaded. Already partially handled — `getCorpusData` native function returns `"[]"` when no corpus. JavaScript side needs to detect empty data and show drop zone message.

**Verdict:** UI-side change in `app.js` — check for empty corpus data and show message.

### Edge Case 2: Invalid File Format

**Current behavior:** `CorpusLoader::loadAudioFile` returns false, logs error. Nothing happens visually.

**Needed:** Notify editor of failure so WebView can show error toast. Add callback for load failure.

### Edge Case 3: Large Corpus (>100MB)

**Current behavior:** Loads without any warning regardless of size.

**Needed:** Check `file.getSize()` before loading. If > 100MB, show warning in WebView (not `AlertWindow` — that's a native dialog that may look jarring in a plugin). Allow user to proceed.

**Approach:** Send a `"fileSizeWarning"` event to WebView with file size. JS shows a toast/overlay with "Load" / "Cancel" buttons. If "Load", call a native function to confirm load.

### Edge Case 4: UMAP Cancel

**Current behavior:** UMAPProjection checks `shouldCancel` callback (tied to `threadShouldExit`). CorpusLoader signals thread exit on `cancelLoad()`.

**Issue:** There's no user-facing "Cancel UMAP" button. UMAP cancel only happens when a new file is loaded (which calls `cancelLoad()` first). CONTEXT.md says: "button to stop UMAP computation, keep PCA layout."

**Needed:** JS button during UMAP progress bar. Native function `cancelUmap()` that signals CorpusLoader to stop the UMAP portion only (tricky — currently `cancelLoad()` kills the entire thread).

**Approach:** Add an atomic flag `umapCancelRequested` to CorpusLoader. UMAP computation checks this flag via the `shouldCancel` callback. When cancelled, PCA layout remains (already delivered before UMAP starts). JS shows "UMAP cancelled — using PCA layout" message.

### Edge Case 5: Corpus File Missing on Restore

**Current behavior:** `setStateInformation` checks `file.existsAsFile()` and only loads if true. If missing, parameters are restored but no corpus is loaded.

**Needed:** Notify editor that saved file is missing, so UI can show "File not found: [path]. Drop a new file to continue." message.

**Approach:** Add `onCorpusMissing(juce::String savedPath)` callback to editor. In `setStateInformation`, if file missing, call this via `MessageManager::callAsync`.

### Edge Case 6: Sample Rate Change

**Current behavior:** `prepareToPlay` detects sample rate change and reloads corpus from file.

**Issue:** This is actually correct behavior — it re-segments and re-analyzes at the new sample rate. The concern from CONTEXT.md was whether this is too complex. Looking at the code, it's already implemented and the re-load happens on background thread, so audio isn't blocked.

**Verdict:** Already handled. Ensure `grainScheduler.reset()` is called to clear stale voices during re-load. Add a brief audio silence period during reload (already natural since corpus becomes nullptr during reload).

### Edge Case 7: Multiple Instances

**Current behavior:** Each instance has its own `TextureForgeProcessor` with separate `SharedCorpus`, `GrainScheduler`, `CorpusLoader`. No shared global state.

**Verdict:** Already safe. No changes needed.

### Edge Case 8: WebView2 Availability (Windows)

**Current behavior:** WebView is created with `Backend::webview2` option. If WebView2 unavailable, JUCE silently falls back to IE backend which doesn't support resource providers = blank page.

**Needed:** Use `WebBrowserComponent::areOptionsSupported()` to detect before construction. Show fallback message with download link.

**Approach:** Config audit only (can't test on macOS). Add `#if JUCE_WINDOWS` block before WebView creation that checks `areOptionsSupported()`. If false, show a `juce::Label` with download instructions instead of WebView.

**Verified:** `areOptionsSupported()` exists in JUCE 8.0.4 (`juce_WebBrowserComponent.h:461`).

### Edge Case 9: Memory Pressure

**Current behavior:** No memory limit checks.

**Needed:** File size check before load. Warn at >100MB but don't block.

**Approach:** Same as Edge Case 3 — file size check in `filesDropped` or `loadCorpusFile`.

---

## 3. Performance Profiling Targets

### ProcessBlock CPU Budget

At 44.1kHz, 512 samples = ~11.6ms budget. Key operations:
- **Parameter reads:** 12 atomic loads — negligible (<1us)
- **KD-tree query:** nanoflann 19D, K=8 — target <10us, should be fine based on Stage 2 verification
- **64-voice grain rendering:** Linear interpolation + envelope per voice per sample — this is the hot path
- **Viz snapshot update:** Array copy — negligible

**Potential bottleneck:** 64 voices x 512 samples = 32,768 sample operations per block. Each requires floating-point interpolation + envelope. Should be well within budget but worth profiling.

### WebView Update Budget

30Hz timer = 33ms per frame. Key operations:
- Build JSON string for viz data
- `emitEventIfBrowserIsVisible()` call

**Potential bottleneck:** JSON string building with `juce::String` concatenation in `timerCallback`. For 64 active grains, this creates ~40+ string concatenations per frame. Consider pre-allocated string buffer.

### Memory Usage

- Corpus audio data: ~10MB per minute of mono 44.1kHz audio (float32)
- Grain metadata: 19 floats x 4 bytes x ~1200 grains/min = ~91KB/min
- KD-tree index: ~2x metadata size = ~182KB/min
- PCA/UMAP vectors: 2 floats x 4 bytes x grains = ~10KB/min
- **Total for 10-minute corpus:** ~100MB audio + ~3MB metadata = ~103MB

This confirms the 100MB warning threshold is appropriate.

---

## 4. Windows Configuration Audit

### Current CMakeLists.txt Status

```cmake
NEEDS_WEBVIEW2 TRUE                           # ✅ Links WebView2LoaderStatic.lib
JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1   # ✅ Uses static linking (no DLL needed)
```

### WebView2 User Data Folder

```cpp
.withWinWebView2Options(
    juce::WebBrowserComponent::Options::WinWebView2{}
        .withUserDataFolder(juce::File::getSpecialLocation(
            juce::File::SpecialLocationType::tempDirectory)
                .getChildFile("OTextureForge_WebView")))
```

**Status:** ✅ Already configured correctly. Uses temp directory to avoid DAW host permission issues.

### Cross-Platform Resource Provider

```cpp
#if JUCE_WEB_BROWSER_RESOURCE_PROVIDER_AVAILABLE
    .withResourceProvider([this](const auto& url) { return getResource(url); })
#endif
```

**Status:** ✅ Guarded by preprocessor macro. On Windows IE backend (fallback), resource provider won't be used.

### Missing: WebView2 Availability Detection

**Status:** Not implemented. Need to add `areOptionsSupported()` check (config/code audit — Windows-specific `#if` block).

---

## 5. Summary of Findings

### Changes Required

| Category | Item | Severity | Effort |
|----------|------|----------|--------|
| Pluginval | Zero-buffer guard in processBlock | HIGH | 2 lines |
| Pluginval | NaN/Inf output clipping | HIGH | 4 lines |
| Pluginval | prepareToPlay reset voices | MEDIUM | 1 line |
| Edge case | Missing file notification to editor | MEDIUM | ~20 lines |
| Edge case | Invalid file format notification | MEDIUM | ~15 lines |
| Edge case | Large file warning (>100MB) | LOW | ~25 lines |
| Edge case | UMAP cancel button (JS + native fn) | MEDIUM | ~40 lines |
| Edge case | WebView2 availability check (Windows) | LOW | ~15 lines |
| Performance | JSON string pre-allocation in timerCallback | LOW | ~10 lines |
| Performance | Output clipping (also serves pluginval) | HIGH | 4 lines |

### Already Correct (No Changes Needed)

- Parameter thread safety (APVTS + atomic pointers)
- Multiple instances (no shared global state)
- Sample rate change handling (reloads corpus on background thread)
- State save/restore round-trip (parameters via APVTS, corpus path via XML)
- Corpus null checks throughout audio path
- Voice pool pre-allocation (fixed array of 64)
- No allocations in processBlock (all pre-allocated)
- WebView2 static linking + user data folder
- Cross-platform resource provider guard

### Open Questions Resolved

| Question | Answer | Rationale |
|----------|--------|-----------|
| Sample rate: re-segment vs. document? | Already re-segments (reloads corpus) | Code already handles this in prepareToPlay |
| Memory pressure upper bound? | Warn at 100MB | ~10MB/min audio + metadata; 10-min file = ~103MB |
| WebView2 detection API? | `areOptionsSupported()` | Confirmed in JUCE 8.0.4 juce_WebBrowserComponent.h:461 |

---

## Sources

- JUCE 8.0.4 source (`/Users/taylorbrook/JUCE/modules/juce_gui_extra/`)
- Pluginval GitHub repository (BasicTests.cpp, CHANGELIST.md)
- JUCE AudioProcessor documentation
- JUCE WebBrowserComponent::areOptionsSupported() API
- O-TextureForge source code analysis
- O-Bass Stage 6 VERIFICATION.md (pluginval strictness 10 pass precedent)
- Project pluginval guide (`.claude/skills/plugin-testing/references/pluginval-guide.md`)
- Cross-platform WebView best practices (`research/cross-platform-webview-best-practices.md`)
