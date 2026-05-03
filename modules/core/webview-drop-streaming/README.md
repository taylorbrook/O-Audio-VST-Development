# webview-drop-streaming

macOS WKWebView drag-drop content-streaming pattern for JUCE 8 plugins with WebView UIs.

## The Problem

WKWebView (the macOS WebView2 backend) does two things that defeat ordinary drag-drop:

1. **Consumes OS-level drag events at the AppKit layer.** JUCE's
   `FileDragAndDropTarget` overrides never fire. Two pre-v1.0.4 attempts in
   O-MicrotonalSampler (`-unregisterDraggedTypes` + a transparent NSView
   overlay) both failed because WebView OS rendering sits above JUCE
   Components.
2. **Sandboxes the JS DataTransfer interface so absolute paths are stripped.**
   Even if you intercept the drop in JS, `dataTransfer.files[i].path` is
   empty and `text/uri-list` carries no `file://` URLs. So you can't tell
   the C++ side "load this folder from disk."

What the user sees: drag a folder of samples onto your plugin, and nothing
happens. No callback, no event, no error.

## The Solution

WKWebView **does** expose a `FileSystemEntry` per dropped item via
`webkitGetAsEntry()`. Walk that tree on the JS side, read each audio file
via `FileReader`, base64-encode it, and stream the bytes through 4 native
functions into a session-scoped temp directory on the C++ side. The
existing host loader (`loadSampleFolder` / `loadSingleSample`) then runs
against the temp dir, exactly as if the user had picked it from a
`juce::FileChooser`.

Performance: base64 has ~33% size overhead, so a 250 MB folder spends a few
seconds on the JS message thread before the host's background loader
starts. The streaming loop emits per-file toast feedback.

## Installation

```cmake
include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)
ouaricon_add_module(MyPlugin webview-drop-streaming)
```

This adds:
- `cpp/WebViewDropStreaming.h` + `cpp/DropSessionGuard.h` to your target's sources/include path
- `js/webview-drop-streaming.js` copied to `Source/ui/public/modules/`

## Architecture

```
   JS (sampler-app.js)              C++ (PluginEditor)
   ─────────────────────            ──────────────────────────────
                                     SessionManager (this module)
                                       owns: temp dir, total bytes,
                                             folder name
   bindWebViewFileDrop({...}) ◄────┐  exposes: 4 native fn handlers
        │                          │
        │ document drop event      │
        ▼                          │
   streamFolderEntryToCpp ─────────┤
        │                          │
        │ dropSessionStart   ──────┼─► creates temp dir, reaps stale
        │                          │
        │ dropSessionAddFile ──────┼─► validates relPath + size cap,
        │   × N files              │   decodes base64, writes file
        │                          │
        │ dropSessionCommitFolder ─┼─► invokes onCommitFolder callback
        │                          │       │
        │                          │       └─► your loadSampleFolder
        ▼                          │           runs against the temp dir
                                   │
                                  end
```

## C++ Usage

Construct one `SessionManager` per editor and merge its native functions
into your registry:

```cpp
#include "WebViewDropStreaming.h"

class MyEditor : public juce::AudioProcessorEditor
{
public:
    MyEditor (MyProcessor& p) : processorRef (p)
    {
        // Construct BEFORE the WebView (so destruction order is safe —
        // see webview-relay-manager pattern).
        Ouaricon::WebViewDropStreaming::SessionManager::Config cfg;
        cfg.tempDirPrefix = "myplugin-drop-";   // namespace-isolated reaper
        cfg.onCommitFolder = [&p] (const juce::File& dir,
                                   const juce::String& displayName,
                                   int targetLayer,
                                   const juce::String& mode,
                                   bool overrideTokens,
                                   bool embedAudio)
        {
            p.loadSampleFolder (dir, targetLayer, parseMode (mode),
                                overrideTokens, "drag-drop", displayName,
                                embedAudio);
        };
        cfg.onCommitFile = [&p] (const juce::File& file, int midi, int vel)
        {
            p.loadSingleSample (midi, vel, file);
        };
        dropSessions = std::make_unique<
            Ouaricon::WebViewDropStreaming::SessionManager> (std::move (cfg));

        // Then in your buildNativeFunctionRegistry() (or equivalent):
        for (auto& entry : dropSessions->getNativeFunctions())
            registry.push_back (std::move (entry));
    }

private:
    // RELAYS first → SessionManager → WebView → ATTACHMENTS last
    std::unique_ptr<Ouaricon::WebViewDropStreaming::SessionManager> dropSessions;
    std::unique_ptr<juce::WebBrowserComponent> webView;
};
```

### `Config` reference

| Field | Type | Notes |
|-------|------|-------|
| `tempDirPrefix` | `juce::String` | **Required.** Per-plugin temp-dir name prefix (e.g., `"myplugin-drop-"`). Used both for new-session naming and stale-session reaping. MUST be unique per plugin so reaping doesn't collide. |
| `onCommitFolder` | `std::function<void(const File&, const String&, int, const String&, bool, bool)>` | **Required.** Called by `dropSessionCommitFolder`. Args: `(sessionDir, displayName, targetLayer, modeStr, overrideTokens, embedAudio)`. The plugin parses `modeStr` to its own LoadMode enum. |
| `onCommitFile` | `std::function<void(const File&, int, int)>` | **Required.** Called by `dropSessionCommitFile`. Args: `(sessionFile, midi, vel)`. |
| `staleSessionRetentionMinutes` | `double` | Optional. Default: 5. Sessions older than this are deleted at the start of each new `dropSessionStart`. Tune up if your background loader is slower than 5 min for typical libraries. |

### Native function protocol

The 4 handlers exposed by `getNativeFunctions()`:

| Name | Args | Returns | Purpose |
|------|------|---------|---------|
| `dropSessionStart` | `(sessionId: string, folderName?: string)` | `bool` | Reaps stale sessions, creates `<tempDir>/<prefix><sessionId>/`, captures folder name for missing-folder modal copy. |
| `dropSessionAddFile` | `(sessionId, relPath, base64)` | `bool` | Validates session match, relPath safety, size caps; decodes base64 to disk. |
| `dropSessionCommitFolder` | `(sessionId, layer?, mode?, override?, embed?)` | `bool` | Invokes `onCommitFolder` with the populated session dir. |
| `dropSessionCommitFile` | `(sessionId, relPath, midi, vel)` | `bool` | Invokes `onCommitFile` with the single streamed file. |

## JavaScript Usage

Import the module from your `sampler-app.js` (or equivalent shell):

```js
import * as Juce from './juce/index.js';
import { bindWebViewFileDrop } from './modules/webview-drop-streaming.js';

bindWebViewFileDrop({
    juce: Juce,                            // ES-module namespace, NOT window.__JUCE__
    dropZoneSelector: '#folder-drop-zone',
    cellSelector: '[data-note]',
    setDropZoneHover: (on) => { /* toggle CSS class */ },
    showFolderLoadOptionsModal: async (totalBytes) => {
        // return { layer, mode, override, embedAudio } or null/undefined to cancel
        return await myModal.show(totalBytes);
    },
    cellMidiVelExtractor: (cellEl) => ({
        midi: parseInt(cellEl.dataset.note,  10),
        vel:  parseInt(cellEl.dataset.layer, 10),
    }),
    showToast: (msg) => myToast.show(msg),

    // Optional:
    audioExtensions: /\.(wav|aif|aiff|flac)$/i,
    onPathFastPath: async (paths, x, y) => {
        // For Linux/Windows hosts that DO expose absolute paths.
        await Juce.getNativeFunction('handleWebViewFileDrop')(paths, x, y);
    },
    showDiagnosticDialog: (title, body) => { /* surface debug info */ },
});
```

### `opts` reference

| Field | Type | Required | Notes |
|-------|------|:-:|-------|
| `juce` | ES-module namespace | ✓ | `import * as Juce from './juce/index.js'`. Must expose `getNativeFunction`. |
| `dropZoneSelector` | CSS selector string | ✓ | Element that accepts folder drops. Folder drops outside it are silently rejected. |
| `cellSelector` | CSS selector string | ✓ | Per-cell elements that accept single-file drops. |
| `setDropZoneHover(bool)` | function | ✓ | UI hover callback for the folder drop zone. |
| `showFolderLoadOptionsModal(totalBytes)` | async function | ✓ | Returns `{layer, mode, override, embedAudio}` or `null` to cancel. |
| `cellMidiVelExtractor(cellEl)` | function | ✓ | Returns `{midi, vel}` from a cell DOM element. |
| `showToast(msg)` | function | ✓ | Brief user feedback during streaming. |
| `audioExtensions` | RegExp | – | Default `/\.(wav\|aif\|aiff)$/i`. |
| `onPathFastPath(paths, x, y)` | async function | – | Called when host exposes absolute paths in DataTransfer. Bypasses streaming entirely. |
| `showDiagnosticDialog(title, body)` | function | – | Surfaced when a drop has no path AND no FileSystemEntry — rare, means we need a new fallback for that host. |

### Exported helpers (for advanced use)

| Export | Purpose |
|--------|---------|
| `streamFolderEntryToCpp(dirEntry, opts)` | Folder streaming pipeline — call directly if you bind your own document listener. |
| `streamSingleFileEntryToCpp(fileEntry, midi, vel, opts)` | Single-file pipeline. |
| `readFileEntryAsBase64(fileEntry)` | `FileSystemFileEntry` → base64 string. |
| `arrayBufferToBase64(buf)` | Chunked btoa() (handles 50+ MB files without crashing). |

## Critical Gotchas (do not regress)

1. **`Juce` namespace, NOT `window.__JUCE__`.** Pass the ES-module namespace
   imported from `./juce/index.js` as `opts.juce`. `window.__JUCE__` is the
   low-level postMessage handler and has **no `getNativeFunction` method**;
   calls silently throw `TypeError` and get eaten by the streaming
   pipeline's try/catch. Symptom: drop appears to do nothing, no error in
   console.

2. **Standard base64, NOT JUCE's proprietary format.** The C++ side decodes
   via `juce::Base64::convertFromBase64`. Do **not** route to
   `MemoryBlock::fromBase64Encoding` — that's JUCE's own
   `<size>.<altAlphabet>` format and silently rejects `btoa()` output.

3. **Per-plugin `tempDirPrefix`.** Reaper deletes anything matching
   `<tempDirPrefix>*` older than 5 min. If two plugins share the same
   prefix, plugin A's reaper will delete plugin B's in-flight session and
   the streaming will fail mid-load.

4. **`arrayBufferToBase64` chunks at 32 K.** This works around
   `String.fromCharCode.apply`'s arg-count limit, which crashes silently
   for 50+ MB samples. If you reuse the helper, keep the chunking.

## Security

Bundled validators (in `DropSessionGuard.h`) reject before any allocation
or filesystem write:

- **Path traversal:** rejects empty, absolute, backslash-containing,
  NUL-containing, or `..`-containing relative paths.
- **Symlink escape:** walks the parent chain upward toward the session dir
  and rejects any existing ancestor that's a symlink (defence against
  pre-existing symlinks that could redirect writes outside the sandbox).
- **Per-file cap:** 256 MB (generous for any realistic sample).
- **Per-session cap:** 4 GB (caps total temp-dir usage per drop).

## History

- **2026-01** — Pattern originated in O-MicrotonalSampler v1.0.4 after
  three earlier attempts (`unregisterDraggedTypes`, JUCE Component overlay,
  NSView sibling overlay) all failed.
- **2026-04** — v1.11.2 added the path-traversal and size-cap guards
  (REVIEW CR-02, CR-03).
- **2026-05** — v1.12.0 added folder-name lift for friendly missing-folder
  modal copy on session reload.
- **2026-05** — v1.12.2 added per-iteration FileReader/native-fn try/catch
  (FE-01/02).
- **2026-05-02** — Promoted to this shared module in O-MicrotonalSampler
  v1.13.0 (ARCH-02 from REVIEW-architecture.md).
