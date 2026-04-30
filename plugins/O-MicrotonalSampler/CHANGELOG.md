# O-MicrotonalSampler Changelog

## [1.4.0] - 2026-04-30

### Changed
- **Loaded samples now loop the entire file by default.** Each slot is
  initialized with `loopStart = 0`, `loopEnd = N - 2`, `loopMode = Auto`
  on load. The renderer's existing 8-sample equal-power crossfade at
  the wrap handles click prevention. Replaces the v1.0–v1.3 RMS-based
  auto-detector that searched for a quiet sustain region in the latter
  60% of the file.
- **"Reset" in the loop-point editor** now snaps the slot back to
  whole-file loop instead of re-running auto-detect.

### Fixed
- **V11-LOOP-FALLBACK** (deferred from Stage 4 verification): sustained
  material with constant RMS (sine waves, drones, organ samples) used
  to fall through the auto-detector's variance gate and silently revert
  to one-shot, going silent before note-off. With whole-file loop as
  the default, these samples now sustain correctly.

### Removed
- `Source/LoopDetector.{h,cpp}` (Phase 2.5 RMS scan + variance gate +
  zero-crossing snap module — ~230 LOC) and the include sites in
  `SampleLoader.cpp` and `PluginProcessor.cpp`. The detector's
  defensive constraints (`loopEnd <= N - 2`, min loop length 16) are
  preserved as inline guards in the new whole-file path.

### Migration notes
- **Behavior change, not breaking.** v1.3.0 sessions/presets load
  cleanly. State persistence is unaffected (no parameter changes, no
  state schema changes). Audio output for one-shot percussive samples
  may differ — they now loop the whole file by default. Use the
  per-slot loop-point editor (Stage 3 UI) to set Manual loop points
  if a particular sample needs different behavior.

### Root cause notes
- The original auto-detector was tuned for sustained instrumental
  material with a clear noise-floor sustain region (e.g. piano
  release tails). On constant-RMS or transient material it
  conservatively rejected and fell back to one-shot — surprising
  default behavior for "load a sample and play it as a sustained
  pitched instrument," which is what the plugin is for.

## [1.3.0] - 2026-04-29

### Added
- **Full state persistence across DAW sessions.** Reopening a project
  now restores the loaded sample folder, tuning state (intervals, A4
  master tune, octave stretch, tonic, mode, KBM mapping), and all
  parameter values exactly as they were when the project was saved.
  Pre-v1.3.0 only persisted parameters — folder and tuning were lost.
- **Save/Load preset (`.omspreset`)** buttons in the header. Captures
  the same state used for project save/load (params + folder path +
  tuning) as a portable XML file. Per design Q1=A: paths only — sample
  audio is referenced, not embedded, so presets stay small but require
  matching folder structure across machines.
- **Missing-folder modal.** When DAW project reopen finds the saved
  folder no longer exists at its original path, a modal surfaces the
  path and offers "Locate folder…" (file picker, reuses
  `loadSampleFolder`) or "Skip" (clears pending state, sampler stays
  empty).

### Changed
- `PluginProcessor::getStateInformation` / `setStateInformation` now
  serialize a wrapped `ValueTree`: APVTS state plus `<SampleFolder>`
  and `<TuningState>` sibling children. Backward-compatible — v1.2.0
  sessions load cleanly (children absent → defaults), v1.3.0 sessions
  in v1.2.0 silently drop the new children.
- Tuning state is captured via the engine's existing accessors plus
  `generateScalaFileContent` / `generateKBMFileContent` round-trips,
  so no fork of the shared `scala-tuning-engine` module is required.
- Added 5 native functions to the WebView bridge:
  `saveCurrentPreset`, `loadPreset`, `locateMissingFolder`,
  `dismissMissingFolder`, `getPendingMissingFolder` — the last covers
  the boot-time race where state restore runs before the WebView has
  registered its `folderMissing` listener.

### Technical notes
- **Root cause** (pre-v1.3.0): `getStateInformation` only emitted
  `parameters.copyState()`, which is APVTS-only. The `currentSampleMap`
  was rebuilt from a folder reference held in memory but never written
  to the persisted state.
- **Threading**: `setStateInformation` runs on the message thread.
  Tuning restore is in-memory and synchronous; folder reload reuses
  the existing async `SampleLoader`. Missing-folder detection is
  synchronous (`File::isDirectory()`); the modal is surfaced via
  `emitEventIfBrowserIsVisible` plus a parked-path pull on WebView
  attach to cover the boot-time race.
- **Backup**: `backups/O-MicrotonalSampler/v1.2.0/` (rollback path).

## [1.2.0] - 2026-04-29

### Added
- **Tuning tab is now an editable authoring surface.** Reverses the
  Stage 3 §RQ3-1 read-only design. Users can:
  - **Select factory tunings** from the library (24+ presets across
    Historical, Just Intonation, Equal Divisions, Non-Octave, World).
  - **Load `.scl` (Scala scale) and `.kbm` (keyboard mapping) files**
    via native file pickers. Save also supported.
  - **Edit individual interval cents** by typing into the table on
    the left.
  - **Change tonic** (rotates 12-note scales).
  - **Adjust A4 reference pitch** (400–480 Hz) via the round knob.
  - **Apply octave stretch** (0.95–1.25 ×) for physical-modeling
    voicings.
  - **Generate scales** from EDO, harmonic series, or rank-2
    temperament parameters and apply them to the engine.
  - **Export the current tuning** as an HTML documentation page
    (with SVG pitch circle).
- Tuning Library and Scale Generator sections auto-expand on first
  Tuning-tab activation, so the right column shows selectable items
  immediately.

### Changed
- `PluginEditor.cpp` registers ~13 new WebView native functions that
  bridge `tuning-panel.js` calls to the shared `scala-tuning-engine`
  module: `setSingleInterval`, `setTonicNote`, `setOctaveStretch`,
  `setMasterTune`, `loadEmbeddedTuning`, `loadScalaFile`,
  `loadKBMFile`, `saveScalaFile`, `saveKBMFile`, `generateEDO`,
  `generateHarmonicSeries`, `generateRank2`, `applyGeneratedScale`,
  `exportTuningHTML`. All file-picker variants use
  `juce::FileChooser::launchAsync` with a `shared_ptr` capture so the
  chooser outlives the async callback.
- `index.html` no longer links `tuning-panel-readonly.css`. The
  read-only CSS file is preserved on disk and as a binary resource
  for backward compatibility but is no longer applied.
- `sampler-app.js` removes the `applyIntervalReadonlyShim` span-swap
  and its `MutationObserver`; the editable `.interval-input`
  elements are now visible and wired to `setSingleInterval` via the
  panel's existing `handleIntervalChange` flow.

### Root Cause
- **Empty intervals table** — TWO root causes:
  1. `.interval-input` rows were hidden by `tuning-panel-readonly.css`
     and replaced with a static `interval-display` span.
  2. **Pre-existing latent bug since v1.0**: `tuning-panel.js` was
     instantiated with `window.__JUCE__` (the low-level postMessage
     handler), but every backend call inside the panel uses
     `juceApi.getNativeFunction(name)` — that method lives on the
     ES-module namespace `Juce` (imported in sampler-app.js as
     `import * as Juce from './juce/index.js'`), NOT on
     `window.__JUCE__`. Every call (`getTuningIntervals`,
     `getEmbeddedTuningList`, `setSingleInterval`, `loadScalaFile`,
     `generateEDO`, etc.) silently threw a `TypeError` and was
     swallowed by the panel's try/catch blocks. Fixed by passing
     `Juce` instead: `new TuningPanel(container, Juce)`.
- **Library: categories visible but no tunings** — `library-content`
  was collapsed by default; `loadEmbeddedTunings()` only fired on
  toggle-expand. Even after manual expansion, clicking an item
  silently failed because the write-side native function
  `loadEmbeddedTuning` was not registered.
- **Missing Load .SCL / .KBM buttons** — `.tuning-file-section` was
  hidden by the readonly CSS overlay, and the underlying
  `loadScalaFile`/`loadKBMFile` natives were never bridged to JS.

### Notes
- **Dorico microtonal playback is preserved.** The shared
  `TuningEngine` remains the single source of truth. Library/file
  loads call `setCustomIntervals()`, which is the same path VST3
  Note Expression overrides per-note at note-on time.
- **No state-format or APVTS changes.** Existing presets and
  sessions load unchanged.
- The `tuning-panel-readonly.css` stylesheet is intentionally kept
  in `Resources/ui/css/` and in `juce_add_binary_data` so a future
  variant could re-enable read-only mode by re-linking it from
  `index.html`.

## [1.1.0] - 2026-04-29

### Added
- Sample-map grid axis labels: velocity-range row labels on the left
  (`97–127`, `65–96`, `33–64`, `1–32`) and C-note column labels below
  (`C1`–`C8`). Velocity labels stay visible during horizontal scroll;
  C labels pan with the grid.
- Cell hover tooltip now shows note name, MIDI number, and velocity
  range. Format: `<filename | Empty> · <NoteName> (<midi>) · Vel <lo>–<hi>`
  (e.g. `vlnsolo_C4_mf.wav · C4 (60) · Vel 65–96`).

### Changed
- `renderGrid()` populates new `#sample-grid-vel-labels` (sidebar) and
  appends `#sample-grid-col-labels` inside the scroll container.
- New helpers `velocityLayerToRange(layer)` and `midiToNoteName(midi)`
  in `sampler-app.js`. Velocity ranges match
  `MicrotonalSamplerVoice.cpp` quartile layer mapping
  (`layerWidth = 128/4 = 32`).

### Notes
- Pure UX/cosmetic change. Cell DOM structure unchanged
  (`.grid-cell` selector intact); drag-drop hit-testing
  (`reportCellLayout`), click routing, and context menu unaffected.
- No DSP, parameter, or state-format changes — preset/session
  compatibility preserved.

## [1.0.4] - 2026-04-29

### Fixed
- Drag-drop folder loading now actually loads samples on macOS (fourth
  attempt — finally working). Drag-drop a single `.wav`/`.aif` onto a
  grid cell also routes correctly via the same code path.

### Why v1.0.3's "fix" wasn't a fix
v1.0.3 moved drag-drop to the JS layer and tried to extract absolute file
paths from `DataTransfer` (`text/uri-list`, `public.file-url`,
`text/plain`). The empirical diagnostic on a real folder drop returned:

```
types=[Files]; files=1 (first: name="vlnsolo_flaut", size=0, type="",
path=undefined, webkitRelativePath=""); items=1 (file:?,
entry=dir:/vlnsolo_flaut); tried: file.path:0/1
```

WKWebView's sandbox strips absolute paths from JS for security; only
`Files` is exposed and `File.path` is undefined. No path-bearing type
was reachable through any combination of `getData(...)` calls. The fast
path was therefore unreachable in production.

### Fix
v1.0.4 streams file *content* through the WebView↔native bridge into a
session-scoped temp dir and runs the existing `loadSampleFolder` /
`loadSingleSample` paths against that temp dir, as if the user had picked
it from a `juce::FileChooser`.

JS side (`sampler-app.js`):
- On drop, `dataTransfer.items[0].webkitGetAsEntry()` returns a
  `FileSystemEntry`.
- For `isDirectory` entries: walk the tree via
  `FileSystemDirectoryReader.readEntries()`, collect every `.wav` /
  `.aif` / `.aiff` (skip dotfiles), preserve relative paths.
- For `isFile` entries: take the single file.
- For each file: read via `FileSystemFileEntry.file(...)` →
  `File.arrayBuffer()` → chunked `String.fromCharCode` → `btoa()` for
  base64. Stream `(sessionId, relativePath, base64)` to C++ via
  `dropSessionAddFile` native function.
- Commit via `dropSessionCommitFolder(sessionId)` or
  `dropSessionCommitFile(sessionId, relPath, midi, vel)`.
- DOM hit-test via `document.elementFromPoint(...)` chooses the routing
  arm (cell vs folder zone vs out-of-bounds) so the existing C++
  `filesDropped` routing matrix (cell hit, folder-zone hit, mismatched
  payload toasts) is mirrored exactly.
- Progress feedback via the existing `showToast` (`Loading 5 of 88: …`).

C++ side (`PluginEditor.cpp`):
- 4 new native functions: `dropSessionStart`, `dropSessionAddFile`,
  `dropSessionCommitFolder`, `dropSessionCommitFile`.
- `dropSessionStart` creates `<temp>/o-microtonalsampler-drop-<sessionId>/`
  and calls `cleanupStaleDropSessions()` to delete prior session dirs
  older than 5 minutes (a window comfortably larger than typical
  SampleLoader read times — avoids racing an in-flight background read).
- `dropSessionAddFile` base64-decodes via `juce::MemoryBlock::fromBase64Encoding`
  and writes via `juce::File::replaceWithData` into the session dir.
- Commit functions call `processorRef.loadSampleFolder` /
  `processorRef.loadSingleSample` on the session temp dir / file. The
  async `SampleLoader` thread reads from there and posts the new
  `SampleMap` via the existing `sampleMapChangedCallback` channel — no
  changes to the loader, parser, loop detector, or grid renderer.

### Performance
Base64 has ~33% size overhead and string-encoding is on the JS message
thread. For a ~250 MB instrument library the streaming pass takes a few
seconds before the background `SampleLoader` starts; the loader itself
is unchanged from v1.0.0. The toast region updates per-file so the user
sees progress instead of a frozen UI.

### Preserved fast-path
The v1.0.3 path-extraction probe (`text/uri-list`, `public.file-url`,
`text/plain`, `File.path`) still runs first as defence-in-depth. If any
host eventually exposes paths (Linux/Win, future WebKit), the fast path
fires immediately and the streaming path is skipped — no rebuild needed
to take advantage of it.

### v1.0.3 → v1.0.4 file delta
- M `Source/PluginEditor.h` — `currentDropSessionId`, `currentDropSessionDir` members; `cleanupStaleDropSessions()` method
- M `Source/PluginEditor.cpp` — 4 new native functions + cleanup helper
- M `Resources/ui/js/sampler-app.js` — `streamFolderEntryToCpp`, `streamSingleFileEntryToCpp`, `collectAudioFilesFromDir`, `readFileEntryAsBase64`, `arrayBufferToBase64`, drop-handler rewrite

## [1.0.3] - 2026-04-29

### Fixed
- Drag-and-drop a folder onto the load zone now actually works on macOS
  (third attempt). v1.0.1 (`-unregisterDraggedTypes` on the WKWebView
  NSView) and v1.0.2 (transparent JUCE Component overlay) both failed.
- Drag-and-drop a single `.wav`/`.aif` onto a grid cell uses the same
  routing path and is fixed by the same change.

### Root Cause (third pass)
WKWebView and its internal content subviews consume OS drag events at the
AppKit layer before JUCE's parent `FileDragAndDropTarget` can route them.
v1.0.1 and v1.0.2 both attempted to fix this at the AppKit/JUCE level:

- **v1.0.1** called `-unregisterDraggedTypes` on the outer WKWebView
  NSView (via `juce::NSViewComponent::getView()`). No effect — WebKit
  re-registers drag types on internal content subviews.
- **v1.0.2** placed a transparent JUCE Component overlay on top of the
  WebView. No effect — the WebView's OS-level rendering paints over JUCE
  Components, and AppKit hit-tests prefer the WebView's own
  drag-destination registration.

Both approaches treated the symptom in C++. The JUCE forum thread on this
issue (`forum.juce.com/t/webbrowsercomponent-consumes-drag-events/45733`,
`forum.juce.com/t/webview-drop-file-from-daw-into-plugin/66000`) confirms
that the WebView consuming drops is fundamental to WKWebView's
architecture and cannot be reliably blocked at the JUCE/AppKit level.

### Fix
v1.0.3 handles drag-drop in the WebView's own JavaScript layer. WKWebView
fires standard DOM `dragenter`/`dragover`/`drop` events for files dragged
from Finder. On drop, JS extracts absolute file paths from the
`DataTransfer` (primary: `text/uri-list`; fallbacks: `public.file-url`,
`text/plain`) and forwards them to a new C++ native function
`handleWebViewFileDrop(paths, x, y)`. That function calls the existing
`FileDragAndDropTarget::filesDropped` routing unchanged — cell hit-test,
folder-zone hit-test, mismatched-payload toasts, and out-of-bounds reject
all behave exactly as designed in Phase 3.3 (RESEARCH §RQ3-6).

Hover visuals (the `.drag-over` class on `#folder-drop-zone`) are now
driven from JS via `getBoundingClientRect()` checks on the cursor
position, replacing the dead C++→JS `hostFileDragMove`/`hostFileDragExit`
event channel.

If the host's `DataTransfer` does not expose any path-bearing type, the
drop is rejected with a diagnostic toast naming the available types so
fallback strategies can be added if a particular host requires them.

### Removed
- `Source/WebViewDragOverlay.{h,mm}` (v1.0.2 attempt — superseded)

### Files
- `Source/PluginEditor.cpp` — `handleWebViewFileDrop` native function;
  top-of-file note documents why JS-side handling is the working approach
- `Resources/ui/js/sampler-app.js` — `bindWebViewFileDrop`,
  `extractDroppedFilePaths`, `uriToPath`, `setFolderDropZoneHover`
- C++ `FileDragAndDropTarget` overrides on the editor are kept as
  defence-in-depth but never fire under v1.0.3.

## [1.0.2] - 2026-04-29

### Fixed
- Drag-and-drop a folder onto the sample-load area now actually works on
  macOS (the v1.0.1 attempt was insufficient — see Root Cause).
- Drag-and-drop a single `.wav`/`.aif` onto an individual grid cell now
  routes to the per-cell loader (was also broken for the same reason).

### Added
- **Clear samples** button next to *Load Folder…* in the drop-zone strip.
  Disabled until at least one sample is loaded; on click, an in-WebView
  confirmation dialog warns before the destructive action. Active voices
  finish playing through their snapshotted map (Stage 2 EC-3 invariant);
  new note-ons after the clear produce silence until samples are loaded
  again.

### Root Cause (v1.0.1 → v1.0.2)
v1.0.1 called `-unregisterDraggedTypes` on the outer `WKWebView` NSView via
`juce::NSViewComponent::getView()`. That call ran successfully but had no
effect because WebKit re-registers drag types on internal content subviews
that are descendants of the WKWebView, so the OS dragging session continued
to land on the WebView and consume the drop before the parent JUCE NSView
could route it to `FileDragAndDropTarget`.

### Fix
v1.0.2 takes a different approach: a transparent **overlay NSView** is
added as a sibling of the WKWebView, addAndMakeVisible'd AFTER the WebView
so it sits later in the AppKit subview order (= on top in z-order). The
overlay implements `<NSDraggingDestination>` (`registerForDraggedTypes:` +
`draggingEntered/Updated/Exited:`, `prepareForDragOperation:`,
`performDragOperation:`) and forwards every event to the editor's existing
`juce::FileDragAndDropTarget` callbacks (`isInterestedInFileDrag`,
`fileDragEnter`, `fileDragMove`, `fileDragExit`, `filesDropped`). Mouse
events fall through to the WebView underneath because the overlay's
`-hitTest:` returns nil — drag-destination selection in AppKit is
independent of `-hitTest:`, so this gives drag interception without
blocking clicks.

Files: `Source/WebViewDragOverlay.{h,mm}` (replaces the v1.0.1
`WebViewMacHelpers.{h,mm}`). The non-mac build returns an inert empty
Component so the editor compiles unmodified on Windows.

### Testing
Manual DAW spot check on macOS (Logic AU + Standalone): folder drop, single-
file cell drop, non-folder rejection toast, file-dialog button regression,
hover visual update during drag, and Clear samples confirmation flow all
verified.

## [1.0.1] - 2026-04-29

### Fixed
- Drag-and-drop a folder onto the sample-load area now loads samples. Dropping
  a single `.wav`/`.aif` onto a grid cell also now routes through the editor's
  `juce::FileDragAndDropTarget` correctly.

### Root Cause
On macOS, `juce::WebBrowserComponent` embeds a WKWebView via NSViewComponent.
The WKWebView's NSView is registered by WebKit as an `NSDraggingDestination`,
so the OS dragging session lands on it first and consumes the drop before the
parent JUCE NSView can route it to `FileDragAndDropTarget::filesDropped`.
The "Load Folder…" button worked because `juce::FileChooser` never traverses
the WebView's drag path.

### Fix
Added `Source/WebViewMacHelpers.{h,mm}` providing `disableWebViewNativeDragDrop`,
which walks the `WebBrowserComponent`'s child `NSViewComponent` and calls
`-unregisterDraggedTypes` on the underlying WKWebView NSView. The editor calls
this once after `addAndMakeVisible(*webView)`. Drops now bubble to the parent
JUCE NSView and reach `filesDropped` as designed in Phase 3.3 (RESEARCH §RQ3-6).

### Testing
Manual DAW spot check on macOS (Logic AU + Standalone): folder drop, single-file
cell drop, non-folder rejection toast, and file-dialog button path all verified.
No automated regression baseline exists for this plugin.

### Notes
- A no-op stub is provided for non-macOS builds; if the same symptom appears
  on Windows WebView2 it will need a separate fix (different native API).
- O-TextureForge v1.0.1 (2026-02-15) hit the identical bug and worked around it
  with a click-to-open file dialog. The same fix can be backported there if
  drag-drop is desired.

## [1.0.0] - 2026-04-29

### Added
- Initial release: microtonal sample engine with Scala tuning support
- VST3 Note Expression for Dorico microtonal playback
- 7 APVTS parameters: attack, decay, sustain, release, polyphony,
  velocity_crossfade, output_gain
- WebView UI with sample-map grid, tuning panel, drag-drop folder/cell loading,
  embedded tuning library, and per-cell file picker
- Stage 4 verified — all 22 requirements complete; pluginval strictness 10
  (with and without GUI) and `auval -v aumu OMtS OuDv` pass.
