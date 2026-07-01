---
phase: editor-bridge-review
reviewed: 2026-06-30T00:00:00Z
depth: deep
files_reviewed: 2
files_reviewed_list:
  - plugins/O-MicrotonalSampler/Source/PluginEditor.cpp
  - plugins/O-MicrotonalSampler/Source/PluginEditor.h
findings:
  critical: 0
  warning: 2
  info: 3
  total: 5
status: issues_found
---

# O-MicrotonalSampler: WebView Editor + C++/JS Bridge — Code Review Report

**Reviewed:** 2026-06-30
**Depth:** deep
**Files Reviewed:** 2 (`PluginEditor.cpp` ~2307 lines, `PluginEditor.h` ~149 lines)
**Status:** issues_found

## Summary

This is a mature (v1.23.0), heavily-iterated WebView bridge. The domain gotchas
flagged for verification are handled correctly — I traced each and they pass:

- **Resource provider (correct):** `getResource` matches bare paths by direct
  equality (`url == "/"`, `url == "/js/sampler-app.js"`, …). No scheme/host
  stripping (`fromFirstOccurrenceOf("://")` is absent). Equality-only matching
  also closes the path-traversal surface — arbitrary paths simply return
  `std::nullopt`. MIME types are correct (`text/javascript`, `image/jpeg`, etc.).
- **Cross-platform WebView (correct):** navigation uses
  `getResourceProviderRoot()` (no hard-coded `juce://` vs `https://`); Windows
  `withUserDataFolder()` is set to a temp subdir; backend is explicitly
  `webview2`. (The Windows static-linking CMake flag is out of scope for these
  two files.)
- **Base64 drag-drop (correct, delegated):** the decode path lives in the shared
  `modules/core/webview-drop-streaming` `SessionManager` (spliced into the
  registry at `PluginEditor.cpp:2302`) and uses
  `juce::Base64::convertFromBase64` — NOT `MemoryBlock::fromBase64Encoding` —
  with relPath rejection and size caps. The reviewed files only bridge the two
  commit callbacks; nothing here reintroduces the JUCE-format decoder.
- **Native-fn registration (no gaps):** I diffed every `getNativeFunction(...)`
  and `invokeNative(...)` name across the entire `Resources/ui/js` tree against
  the C++ registry. **Every JS-invoked native function is registered.** No
  silently-dead bridge calls.
- **JUCE namespace:** C++ side is namespace-agnostic (registers via
  `withNativeFunction`); the `Juce` vs `window.__JUCE__` split is a JS concern
  and is out of these files.

Remaining issues are lifetime/robustness and dead-code, detailed below. None are
security vulnerabilities; the most serious is a conditional use-after-free in the
async file-dialog callbacks.

## Warnings

### WR-01: Async FileChooser/`launchAsync` completions capture raw `this` and invoke `complete` after possible editor teardown (UAF)

**File:** `PluginEditor.cpp` — `loadSingleSampleDialog` (1156-1192), `loadScalaFile` (1536-1549), `loadKBMFile` (1563-1572), `saveScalaFile` (1590-1604), `saveKBMFile` (1622-1636), `saveCurrentPreset` (1744-1762), `loadPreset` (1780-1793), `locateMissingFolder` (1815-1827), `exportTuningHTML` (1878-1893). (`pickSampleFolder`/`estimateFolderAudioSize` are exempt — they capture `[]`, not `this`.)

**Issue:** Each of these `chooser->launchAsync(...)` completion lambdas captures
`[this, chooser, complete]`. The `chooser` is kept alive by the shared_ptr
capture, but `this` is not. If the plugin editor is destroyed while a native
file dialog is still open (host tears the window down, or a non-window-modal
dialog on some hosts), the completion later dereferences a dangling `this` via
`processorRef.<...>()` and calls `complete(...)` — the WebView's JS-resolution
callback — after the `WebBrowserComponent` has been destroyed. Both are
use-after-free and can crash the host. This is the standard JUCE async-chooser
foot-gun; window-modality mitigates it on most macOS hosts but does not
guarantee safety.

**Fix:** Guard the editor with a `juce::Component::SafePointer` and bail if it
died:
```cpp
chooser->launchAsync (flags,
    [safeThis = juce::Component::SafePointer<OMicrotonalSamplerAudioProcessorEditor>(this),
     chooser, midi, vel, mergeAsRr, technique, complete] (const juce::FileChooser& fc) mutable
    {
        if (safeThis == nullptr) { complete (juce::var (false)); return; }
        // ... use safeThis->processorRef, safeThis->... instead of this
    });
```
Apply the same pattern to every `this`-capturing `launchAsync` completion above.

### WR-02: Silent save failure reported to JS as success

**File:** `PluginEditor.cpp:1599` (`saveScalaFile`), `1631` (`saveKBMFile`), `1888` (`exportTuningHTML`)

**Issue:** These three save handlers call `file.replaceWithText(...)` and then
unconditionally `complete(juce::var(true))`, discarding the `bool` return. If the
write fails (read-only location, permission denied, disk full), the JS layer
receives `true` and shows a success state while nothing was written — silent data
loss from the user's perspective. Note the inconsistency: `saveCurrentPreset`
(1756) *does* check the return (`|| ! file.replaceWithText(xml)`), so the
codebase already knows the correct pattern.

**Fix:** Propagate the write result, matching `saveCurrentPreset`:
```cpp
const bool ok = file.replaceWithText (eng->generateScalaFileContent());
complete (juce::var (ok));
```

## Info

### IN-01: Dead native functions registered but never invoked by any JS

**File:** `PluginEditor.cpp` — `getEmbeddedTuningCategories` (796-807), `getSkippedFiles` (1196-1207), `resetTechniqueNames` (1996-2003)

**Issue:** A full-tree scan of `Resources/ui/js` (both `getNativeFunction(...)`
and `invokeNative(...)` call sites) shows these three registered handlers are
never called. `getSkippedFiles` is superseded — the JS reads `snap.skippedFiles`
directly off the `sampleMap` snapshot (sampler-app.js:700). `resetTechniqueNames`
is superseded by `applyTechniqueNames`/per-slot renames. `getEmbeddedTuning
Categories` has no consumer. They are harmless but add registry weight and
maintenance ambiguity (a future reader can't tell they're dead).

**Fix:** Remove the three registry entries (and, for `getSkippedFiles`, the now-
unused `getLastSkippedFiles` accessor if nothing else uses it), or add a one-line
comment marking each as intentionally-retained API surface.

### IN-02: `reportCellLayout` never resets `folderZoneRect` when the payload omits `folderZone`

**File:** `PluginEditor.cpp:837-844`

**Issue:** `cellLayout.clearQuick()` is called before repopulating cells, but
`folderZoneRect` is only overwritten when the incoming JSON contains a
`folderZone` object. If a subsequent layout report omits it (e.g. the drop zone
is scrolled out or removed from the DOM), the stale rectangle persists and
`filesDropped` (638-652) can still route a folder drop to a zone that is no
longer on screen. Low impact in the current UI (the zone is effectively always
present), but it is latent mis-routing.

**Fix:** Reset before conditionally repopulating:
```cpp
folderZoneRect = {};
if (auto* fz = obj->getProperty ("folderZone").getDynamicObject()) { ... }
```
Also consider clearing `cellLayout`/`folderZoneRect` when the top-level JSON
fails to parse (currently stale layout survives a malformed report).

### IN-03: `filesDropped` cell-hit path loads without an existence check

**File:** `PluginEditor.cpp:632` (and folder path 645)

**Issue:** On a cell hit the handler calls
`processorRef.loadSingleSample(c.midiNote, c.velocityLayer, file)` without a
`file.existsAsFile()` guard — unlike the dialog path (`loadSingleSampleDialog`,
1169) which does check. Because `handleWebViewFileDrop` (1046-1086) forwards
JS-supplied path strings straight into `filesDropped`, a filename-only or
relative string (when the host doesn't expose absolute paths) yields a
non-existent `juce::File`, and a phantom load is dispatched (also trips a
`juce::File` absolute-path jassert in debug). The processor may absorb this, but
the bridge should not forward unvalidated paths.

**Fix:** Gate the cell-hit load on existence and route a toast otherwise:
```cpp
if (! file.existsAsFile()) { emitToast (webView.get(), "File not found"); return; }
processorRef.loadSingleSample (c.midiNote, c.velocityLayer, file);
```

---

## Notes on items explicitly checked and cleared (not findings)

- **DynamicObject/`var` ownership:** every `new juce::DynamicObject()` is handed
  to `juce::var(ReferenceCountedObject*)`, which ref-counts and frees it. No
  leaks (e.g. `fileDragMove` payload, `getTechniqueState`, `getTriggerState`,
  `getPendingMissingFolder`).
- **Index validation:** the bridge passes several indices unclamped
  (`setCcMapping` slot, `setPcMapping` slot, `setLayerTrim`/`setTechniqueTrim`
  technique/layer, `setActiveTechnique`), but the processor is the validation
  boundary and clamps/range-checks all of them (`mutateMappingSlot` slot guard,
  `setLayerTrim`/`setTechniqueTrim` range guards, `setActiveTechnique`
  `jlimit(0,7)`). No OOB.
- **Member destruction order:** relays → webView → attachments is correct;
  attachments (which touch the WebView during dtor) are declared last and destroy
  first. Timer is stopped and all processor callbacks are detached in `~Editor`
  before members die.
- **Timer bitmask diffing:** MIDI 0-127 coverage is correct (low half offset 0,
  high half offset +64); note-on/off emission is guarded by `webView != nullptr`.

---

_Reviewed: 2026-06-30_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: deep_
