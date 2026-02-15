# Stage 4: Integration & Polish - Execution Plan

**Plugin:** O-TextureForge
**Stage:** 4-polish
**Created:** 2026-02-14
**Based on:** CONTEXT.md, RESEARCH.md

---

## Goal

Harden O-TextureForge for release quality: pass pluginval strictness 10, handle all edge cases gracefully, optimize performance bottlenecks, and audit Windows WebView2 configuration.

---

## Tasks

### 1. [ ] Harden processBlock for pluginval compliance
- **Files:** `Source/PluginProcessor.cpp`
- **Depends on:** none
- **Changes:**
  - Add zero-sample buffer early return at top of processBlock (`if (buffer.getNumSamples() == 0) return;`)
  - Add NaN/Inf output clipping after grain rendering: `juce::FloatVectorOperations::clip(channelData, channelData, -1.0f, 1.0f, numSamples)` for each channel
  - Call `grainScheduler.reset()` in `prepareToPlay` when sample rate changes (clear stale voice state during corpus reload)

### 2. [ ] Add UMAP cancel support (C++ atomic flag + native function)
- **Files:** `Source/dsp/CorpusLoader.h`, `Source/dsp/CorpusLoader.cpp`, `Source/PluginProcessor.h`, `Source/PluginProcessor.cpp`, `Source/PluginEditor.cpp`
- **Depends on:** none
- **Changes:**
  - Add `std::atomic<bool> umapCancelRequested { false }` to CorpusLoader
  - In `run()`, pass `umapCancelRequested` check into UMAP shouldCancel callback (alongside `threadShouldExit`)
  - Add `cancelUmap()` method to CorpusLoader that sets flag (UMAP stops, PCA layout preserved)
  - Add `cancelUmap()` forwarding method on TextureForgeProcessor
  - Register `cancelUmap` native function in PluginEditor WebView options
  - After UMAP cancel, send `umapCancelled` event to WebView so JS can hide progress bar and show "Using PCA layout" message

### 3. [ ] Handle edge case: no file loaded (empty state UI)
- **Files:** `Source/ui/src/app.js`
- **Depends on:** none
- **Changes:**
  - In `corpusLoaded` event handler, check if parsed points array is empty; if so, keep placeholder visible with "Drop audio file here" text
  - Ensure scatter placeholder is visible on initial load (already default — verify)

### 4. [ ] Handle edge case: invalid file format notification
- **Files:** `Source/dsp/CorpusLoader.cpp`, `Source/PluginProcessor.cpp`, `Source/PluginEditor.h`, `Source/PluginEditor.cpp`, `Source/ui/src/app.js`
- **Depends on:** none
- **Changes:**
  - In CorpusLoader `run()`, when `loadAudioFile` returns false, invoke a failure callback via `MessageManager::callAsync`
  - Add `LoadFailureCallback` typedef and parameter to `loadFile()`
  - In TextureForgeProcessor `loadCorpusFile()`, pass failure callback that notifies editor via `onCorpusLoadFailed()`
  - Add `onCorpusLoadFailed(const juce::String& reason)` to PluginEditor — emits `loadFailed` event to WebView
  - In app.js, listen for `loadFailed` event and show toast message (styled with Naturalist CSS)

### 5. [ ] Handle edge case: large file warning (>100MB)
- **Files:** `Source/PluginEditor.cpp`, `Source/ui/src/app.js`
- **Depends on:** none
- **Changes:**
  - In `filesDropped()`, check `file.getSize()` before calling `loadCorpusFile()`
  - If > 100MB (104857600 bytes), emit `fileSizeWarning` event to WebView with file size in MB
  - In app.js, listen for `fileSizeWarning` and show overlay with file size + "Load Anyway" / "Cancel" buttons
  - "Load Anyway" calls `confirmLargeLoad` native function which triggers `loadCorpusFile`
  - "Cancel" dismisses overlay
  - Register `confirmLargeLoad` native function in PluginEditor
  - Store pending large file path as member variable

### 6. [ ] Handle edge case: corpus file missing on session restore
- **Files:** `Source/PluginProcessor.cpp`, `Source/PluginEditor.h`, `Source/PluginEditor.cpp`, `Source/ui/src/app.js`
- **Depends on:** none
- **Changes:**
  - In `setStateInformation()`, when `file.existsAsFile()` returns false but path was saved, call `onCorpusMissing(savedPath)` on editor via `MessageManager::callAsync`
  - Add `onCorpusMissing(const juce::String& savedPath)` to PluginEditor — emits `corpusMissing` event to WebView
  - In app.js, listen for `corpusMissing` and show "File not found: [path]. Drop a new file to continue." message in scatter area

### 7. [ ] Handle edge case: WebView2 availability detection (Windows config audit)
- **Files:** `Source/PluginEditor.cpp`
- **Depends on:** none
- **Changes:**
  - Add `#if JUCE_WINDOWS` block before WebView creation
  - Use `WebBrowserComponent::areOptionsSupported()` to check WebView2 availability
  - If not supported, create a `juce::Label` fallback with styled message: "WebView2 required. Download from microsoft.com/edge/webview2"
  - Guard WebView creation and relay/attachment setup behind WebView2 availability check

### 8. [ ] Optimize timerCallback JSON string building
- **Files:** `Source/PluginEditor.cpp`
- **Depends on:** none
- **Changes:**
  - Pre-allocate a `juce::String` member with `preallocateBytes(512)` for reuse across timer ticks
  - Use the pre-allocated string (clear + rebuild) instead of creating new String each frame
  - Reduces 30Hz allocation pressure

### 9. [ ] Run pluginval at strictness 10 (VST3 + AU)
- **Files:** none (validation only)
- **Depends on:** Tasks 1, 2, 3, 4, 5, 6, 7, 8
- **Actions:**
  - Build O-TextureForge: `ninja O-TextureForge_VST3 O-TextureForge_AU`
  - Run pluginval strictness 10 on VST3: `pluginval --validate-in-process --strictness-level 10 --output-dir /tmp/pluginval build/plugins/O-TextureForge/O-TextureForge_artefacts/Release/VST3/O-TextureForge.vst3`
  - Run pluginval strictness 10 on AU: `pluginval --validate-in-process --strictness-level 10 --output-dir /tmp/pluginval build/plugins/O-TextureForge/O-TextureForge_artefacts/Release/AU/O-TextureForge.component`
  - Fix any failures discovered
  - Verify AU with `auval -v aumu OuTF OuDv`

### 10. [ ] Rebuild webpack bundle and verify end-to-end
- **Files:** `Source/ui/src/app.js` (already modified), `Source/ui/public/js/app.bundle.js` (webpack output)
- **Depends on:** Tasks 2, 3, 4, 5, 6
- **Actions:**
  - `cd plugins/O-TextureForge/Source/ui && npm run build`
  - Build plugin and install to system folders
  - DAW test: load in Ableton/Logic, drag-and-drop audio file, verify scatter plot renders
  - Test UMAP cancel (if UI button wired)
  - Test session save/restore
  - Verify all 12 parameters respond to automation

### 11. [ ] Update STATUS.md
- **Files:** `.planning/STATUS.md`
- **Depends on:** Tasks 9, 10
- **Changes:**
  - Mark Stage 4 as complete
  - Update progress to 100%
  - Document pluginval results
  - Document any known limitations

---

## Success Criteria

- [ ] Pluginval strictness 10 passes for both VST3 and AU
- [ ] AU validated via `auval -v aumu OuTF OuDv`
- [ ] Zero-sample buffer handling (no crash with empty buffers)
- [ ] NaN/Inf output protection (output always clipped to [-1, 1])
- [ ] prepareToPlay handles sample rate change (voices reset, corpus reloads)
- [ ] Empty state shows "Drop audio file here" message
- [ ] Invalid file format shows error toast
- [ ] Large file (>100MB) shows warning with load/cancel option
- [ ] UMAP can be cancelled (PCA layout preserved)
- [ ] Missing corpus file on restore shows notification
- [ ] Windows WebView2 detection code present (config audit — can't test on macOS)
- [ ] timerCallback uses pre-allocated string buffer
- [ ] All 12 parameters respond to DAW automation
- [ ] Plugin runs stable for 5+ minutes of sustained playback

---

## File Change Summary

| File | Action | Purpose |
|------|--------|---------|
| `Source/PluginProcessor.cpp` | MODIFY | Zero-buffer guard, NaN clip, prepareToPlay reset, missing file notify, failure callback, cancelUmap forwarding |
| `Source/PluginProcessor.h` | MODIFY | Add cancelUmap method, onCorpusMissing forwarding |
| `Source/PluginEditor.cpp` | MODIFY | Native functions (cancelUmap, confirmLargeLoad), file size check, missing file handler, string pre-alloc, WebView2 check |
| `Source/PluginEditor.h` | MODIFY | Add onCorpusLoadFailed, onCorpusMissing, pending file member, pre-alloc string |
| `Source/dsp/CorpusLoader.h` | MODIFY | Add umapCancelRequested flag, cancelUmap(), LoadFailureCallback |
| `Source/dsp/CorpusLoader.cpp` | MODIFY | UMAP cancel check, failure callback invocation |
| `Source/ui/src/app.js` | MODIFY | Empty state, error toast, file size warning, UMAP cancel button, missing file message |
| `.planning/STATUS.md` | MODIFY | Mark Stage 4 complete |

---

## Risk Assessment

| Risk | Severity | Mitigation |
|------|----------|------------|
| Pluginval finds unexpected failures | MEDIUM | Research identified specific fixes; iterative fix cycle |
| Webpack bundle changes break existing UI | LOW | Test end-to-end before declaring complete |
| UMAP cancel flag race condition | LOW | Atomic bool, checked per-epoch — well-bounded |
| Large file warning interrupts drag-drop UX | LOW | Only for >100MB; most files much smaller |
