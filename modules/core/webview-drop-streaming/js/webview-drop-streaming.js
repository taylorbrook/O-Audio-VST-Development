// ============================================================================
// webview-drop-streaming.js
// Ouaricon Module — macOS WKWebView drag-drop content-streaming pattern.
//
// Why this exists
// ---------------
// WKWebView (macOS) consumes OS-level drag events at the AppKit layer AND
// sandboxes the JS DataTransfer interface so absolute paths are stripped.
// We can't tell the C++ side "load this folder from disk". Instead we walk
// the dropped FileSystemEntry tree, read each audio file via FileReader,
// base64-encode it, and stream the bytes through 4 native functions
// (dropSessionStart / dropSessionAddFile / dropSessionCommitFolder /
// dropSessionCommitFile) into a session-scoped temp dir on the C++ side.
// The host then runs its existing "load folder" path against the temp dir
// as if the user had picked it via FileChooser.
//
// History
// -------
// Originated in O-MicrotonalSampler v1.0.4 (2026-01). Hardened in v1.11.2
// (per-iteration try/catch, modal rejection plumbing, fast-path probe).
// Promoted to this shared module in O-MicrotonalSampler v1.13.0 (ARCH-02 —
// REVIEW-architecture.md, May 2026).
//
// Surface
// -------
//   bindWebViewFileDrop(opts)              — document-level drop listener
//   streamFolderEntryToCpp(dirEntry, opts) — folder streaming pipeline
//   streamSingleFileEntryToCpp(fileEntry, midi, vel, opts) — single-file pipeline
//   readFileEntryAsBase64(fileEntry)       — FileSystemEntry → base64 string
//   arrayBufferToBase64(buf)               — chunked btoa() (handles big files)
//
// CRITICAL gotchas (do not regress these)
// ---------------------------------------
// 1. Pass `Juce` (the ES-module namespace from `import * as Juce from
//    './juce/index.js'`) as opts.juce — NOT `window.__JUCE__`. The latter
//    has no getNativeFunction method; calls silently throw and get eaten.
// 2. The C++ side decodes via juce::Base64::convertFromBase64 (standard
//    base64). MemoryBlock::fromBase64Encoding is JUCE's proprietary
//    "<size>.<altAlphabet>" format and silently rejects btoa() output.
// 3. arrayBufferToBase64 chunks at 32K to stay under
//    String.fromCharCode.apply's arg-count limit (50+ MB samples crash
//    without chunking).
//
// ============================================================================

const DEFAULT_AUDIO_EXTENSIONS_RE = /\.(wav|aif|aiff)$/i;

/**
 * Bind a document-level drop listener that routes WKWebView drops through
 * the streaming pipeline.
 *
 * Options:
 *   juce                  — REQUIRED. The Juce ES-module namespace
 *                           (import * as Juce from './juce/index.js').
 *                           Must expose getNativeFunction.
 *   dropZoneSelector      — REQUIRED. CSS selector for the folder drop zone
 *                           (e.g., '#folder-drop-zone'). Folder drops outside
 *                           this element are silently rejected.
 *   cellSelector          — REQUIRED. CSS selector for individual cells that
 *                           accept single-file drops (e.g., '[data-note]').
 *   setDropZoneHover(bool)— REQUIRED. UI callback to highlight/unhighlight
 *                           the drop zone during dragenter/dragover/dragleave.
 *   showFolderLoadOptionsModal(totalBytes) — REQUIRED. Returns a Promise
 *                           resolving to {layer, mode, override, embedAudio}
 *                           or null/undefined if cancelled.
 *   cellMidiVelExtractor(cellEl) — REQUIRED. Returns {midi, vel} from a
 *                           cell DOM element, or {midi: NaN, vel: NaN} if
 *                           the cell is malformed.
 *   showToast(msg)        — REQUIRED. Brief user feedback during streaming.
 *   audioExtensions       — Optional regex (default /\.(wav|aif|aiff)$/i).
 *   onPathFastPath(paths, x, y) — Optional. Called when host exposes
 *                           absolute paths in DataTransfer (Linux/Win/some
 *                           WebKit configs). Pass through to your existing
 *                           filesDropped router. Skip the slow streaming
 *                           path entirely when this fires.
 *   showDiagnosticDialog(title, body) — Optional. Surfaced when a drop
 *                           has no path AND no FileSystemEntry (rare —
 *                           means we need a new fallback for that host).
 */
export function bindWebViewFileDrop (opts) {
    if (!opts || !opts.juce) {
        console.error('[webview-drop-streaming] bindWebViewFileDrop: opts.juce is required');
        return;
    }

    const audioRe = opts.audioExtensions || DEFAULT_AUDIO_EXTENSIONS_RE;

    document.addEventListener('dragenter', (e) => {
        e.preventDefault();
        const z = document.querySelector(opts.dropZoneSelector);
        if (!z) return;
        const r = z.getBoundingClientRect();
        opts.setDropZoneHover(pointInClientRect(e.clientX, e.clientY, r));
    });

    document.addEventListener('dragover', (e) => {
        e.preventDefault();
        // Tell the OS we accept a Copy-style drop so the user sees the
        // green '+' cursor instead of the 'no-drop' cursor.
        try { e.dataTransfer.dropEffect = 'copy'; } catch (_) { /* read-only in some hosts */ }
        const z = document.querySelector(opts.dropZoneSelector);
        if (!z) return;
        const r = z.getBoundingClientRect();
        opts.setDropZoneHover(pointInClientRect(e.clientX, e.clientY, r));
    });

    document.addEventListener('dragleave', (e) => {
        // dragleave fires when the cursor leaves a child element too — only
        // clear when leaving the document entirely.
        if (e.relatedTarget === null) opts.setDropZoneHover(false);
    });

    document.addEventListener('drop', async (e) => {
        e.preventDefault();
        opts.setDropZoneHover(false);

        if (!window.__JUCE__) return;

        // Fast path: if the host happens to expose absolute paths (Linux/
        // Win, certain WebKit configs), let the plugin's existing path
        // router handle it. WKWebView strips paths so this almost never
        // fires in practice — defence in depth only.
        if (typeof opts.onPathFastPath === 'function') {
            const probe = extractDroppedFilePaths(e.dataTransfer);
            if (probe.paths.length > 0) {
                try {
                    await opts.onPathFastPath(probe.paths,
                        Math.round(e.clientX), Math.round(e.clientY));
                } catch (err) {
                    console.error('[webview-drop-streaming] onPathFastPath failed:', err);
                }
                return;
            }
        }

        // Slow path: WKWebView gives us a FileSystemEntry but no absolute
        // path. Stream the entry tree's content through the bridge.
        const items = Array.from(e.dataTransfer?.items || []);
        const firstEntry = items.length > 0
            && typeof items[0].webkitGetAsEntry === 'function'
            ? items[0].webkitGetAsEntry()
            : null;

        if (!firstEntry) {
            const probe = extractDroppedFilePaths(e.dataTransfer);
            console.warn('[webview-drop-streaming] WebView drop with no path and no entry.',
                         probe.diagnostics);
            if (typeof opts.showDiagnosticDialog === 'function') {
                opts.showDiagnosticDialog(
                    'Drop diagnostic — no path and no FileSystemEntry',
                    probe.diagnostics);
            }
            return;
        }

        // DOM hit-test the drop point so we can route exactly like a C++
        // filesDropped() routing matrix.
        const target  = document.elementFromPoint(e.clientX, e.clientY);
        const cellEl  = target ? target.closest(opts.cellSelector) : null;
        const zoneEl  = target ? target.closest(opts.dropZoneSelector) : null;

        try {
            if (firstEntry.isDirectory) {
                if (cellEl) {
                    opts.showToast('Drop a single file on a cell, or a folder on the top zone.');
                    return;
                }
                if (!zoneEl) {
                    // Out-of-bounds — silent reject.
                    return;
                }
                await streamFolderEntryToCpp(firstEntry, opts);
            } else if (firstEntry.isFile) {
                const isAudio = audioRe.test(firstEntry.name);
                if (cellEl) {
                    if (!isAudio) { opts.showToast('Drop a .wav/.aif on a cell'); return; }
                    const { midi, vel } = opts.cellMidiVelExtractor(cellEl);
                    if (!Number.isFinite(midi) || !Number.isFinite(vel)) return;
                    await streamSingleFileEntryToCpp(firstEntry, midi, vel, opts);
                } else if (zoneEl) {
                    opts.showToast('Drop a folder, not a file');
                }
                // else: out-of-bounds silent reject
            }
        } catch (err) {
            console.error('[webview-drop-streaming] drop streaming failed:', err);
            opts.showToast(`Drop failed: ${err && err.message ? err.message : err}`);
        }
    });
}

// ============================================================================
// Drop content streaming
// ============================================================================
//
// Performance note: base64 has ~33% size overhead. For a ~250 MB folder
// (typical instrument library), expect a few seconds of streaming on the
// JS message thread before the SampleLoader background thread starts.

/**
 * Stream a FileSystemDirectoryEntry's audio files into a C++ session.
 * Calls dropSessionStart → dropSessionAddFile×N → dropSessionCommitFolder.
 *
 * Per-iteration try/catch around BOTH the FileReader read AND the native-fn
 * round-trip — a single corrupted .wav or backend stall surfaces as a
 * skip+toast, not a silent hang.
 */
export async function streamFolderEntryToCpp (dirEntry, opts) {
    const audioRe = opts.audioExtensions || DEFAULT_AUDIO_EXTENSIONS_RE;

    opts.showToast('Scanning folder…');
    const all = [];
    await collectAudioFilesFromDir(dirEntry, '', all, audioRe);

    if (all.length === 0) {
        opts.showToast('No audio files in folder');
        return;
    }

    const totalBytes = all.reduce((acc, x) => acc + (x.size || 0), 0);

    // Ask for layer / mode / override BEFORE we start base64-streaming.
    // A 250 MB drop spends ~5 s on streaming alone — wasted work if the
    // user changes their mind. Cancel = silent abort (no toast — Cancel
    // was an explicit user action).
    let pickedOpts;
    try {
        pickedOpts = await opts.showFolderLoadOptionsModal(totalBytes);
    } catch (e) {
        // Modal promise rejection (DOM tear-down, unexpected exception in
        // cleanup) — surface and abort instead of hanging.
        console.error('[webview-drop-streaming] folder-load options modal failed:', e);
        opts.showToast('Folder load dialog failed — aborted');
        return;
    }
    if (!pickedOpts) return;

    const sessionId = newDropSessionId();

    // Pass the folder name (from FileSystemEntry::name) to dropSessionStart
    // so the saved-state missing-folder modal can render
    // "Samples were drag-dropped from <name>" copy on reload. macOS
    // WKWebView strips the original disk path; this name is the only
    // cross-session-stable signal we get.
    const folderName = dirEntry && dirEntry.name ? dirEntry.name : '';
    let startOk;
    try {
        startOk = await opts.juce.getNativeFunction('dropSessionStart')(
            sessionId, folderName);
    } catch (e) {
        console.error('[webview-drop-streaming] dropSessionStart failed:', e);
        opts.showToast('Drop session start failed');
        return;
    }
    if (!startOk) { opts.showToast('Drop session start failed'); return; }

    let failed = 0;
    for (let i = 0; i < all.length; i++) {
        const { entry, relativePath } = all[i];
        opts.showToast(`Loading ${i + 1} of ${all.length}: ${entry.name}`);
        try {
            const base64 = await readFileEntryAsBase64(entry);
            const ok = await opts.juce.getNativeFunction('dropSessionAddFile')(
                sessionId, relativePath, base64);
            if (!ok) {
                console.warn(`[webview-drop-streaming] addFile rejected: ${relativePath}`);
                opts.showToast(`Skipped: ${entry.name} (backend rejected)`);
                failed++;
            }
        } catch (e) {
            console.error(`[webview-drop-streaming] file stream failed for ${relativePath}:`, e);
            opts.showToast(`Skipped: ${entry.name} (read failed)`);
            failed++;
        }
    }

    const okCount = all.length - failed;
    if (okCount === 0) {
        opts.showToast('No samples loaded — all files failed');
        // Still call commit so the C++ side can clean up the empty session.
        try {
            await opts.juce.getNativeFunction('dropSessionCommitFolder')(
                sessionId,
                pickedOpts.layer,
                pickedOpts.mode,
                pickedOpts.override         ? 1 : 0,
                pickedOpts.embedAudio       ? 1 : 0,
                pickedOpts.technique         ?? 0,
                pickedOpts.overrideTechnique ? 1 : 0);
        } catch (e) {
            console.error('[webview-drop-streaming] dropSessionCommitFolder failed:', e);
        }
        return;
    }

    opts.showToast(failed > 0
        ? `Loading ${okCount} of ${all.length} sample${all.length === 1 ? '' : 's'} (${failed} skipped)…`
        : `Loading ${all.length} sample${all.length === 1 ? '' : 's'}…`);
    try {
        await opts.juce.getNativeFunction('dropSessionCommitFolder')(
            sessionId,
            pickedOpts.layer,
            pickedOpts.mode,
            pickedOpts.override         ? 1 : 0,
            pickedOpts.embedAudio       ? 1 : 0,
            pickedOpts.technique         ?? 0,
            pickedOpts.overrideTechnique ? 1 : 0);
    } catch (e) {
        // Commit-step rejection — without this catch the UI never sees
        // the "Loading X samples…" toast cleared.
        console.error('[webview-drop-streaming] dropSessionCommitFolder failed:', e);
        opts.showToast('Folder load failed at commit step');
    }
    // sampleMapUpdated push event drives the grid refresh + final state.
}

/**
 * Stream a single FileSystemFileEntry into a C++ session and commit it as
 * a single sample (midi/vel). Each native-fn await is wrapped so a backend
 * stall can't hang the UI permanently.
 */
export async function streamSingleFileEntryToCpp (fileEntry, midi, vel, opts) {
    const sessionId = newDropSessionId();

    let startOk;
    try {
        startOk = await opts.juce.getNativeFunction('dropSessionStart')(sessionId);
    } catch (e) {
        console.error('[webview-drop-streaming] dropSessionStart failed:', e);
        opts.showToast('Drop session start failed');
        return;
    }
    if (!startOk) { opts.showToast('Drop session start failed'); return; }

    opts.showToast(`Loading ${fileEntry.name}…`);

    let base64;
    try {
        base64 = await readFileEntryAsBase64(fileEntry);
    } catch (e) {
        // FileReader reject on a single-file drop — the .wav was unreadable.
        console.error(`[webview-drop-streaming] file read failed for ${fileEntry.name}:`, e);
        opts.showToast(`Skipped: ${fileEntry.name} (read failed)`);
        return;
    }

    let addOk;
    try {
        addOk = await opts.juce.getNativeFunction('dropSessionAddFile')(
            sessionId, fileEntry.name, base64);
    } catch (e) {
        console.error('[webview-drop-streaming] dropSessionAddFile failed:', e);
        opts.showToast('File transfer failed');
        return;
    }
    if (!addOk) { opts.showToast('File transfer failed'); return; }

    try {
        await opts.juce.getNativeFunction('dropSessionCommitFile')(
            sessionId, fileEntry.name, midi, vel);
    } catch (e) {
        console.error('[webview-drop-streaming] dropSessionCommitFile failed:', e);
        opts.showToast('File load failed at commit step');
    }
}

/**
 * Read a FileSystemFileEntry's content as a base64 string. Returns a
 * Promise resolving to the encoded string, or rejecting if FileReader fails.
 */
export async function readFileEntryAsBase64 (fileEntry) {
    const file = await new Promise((resolve, reject) =>
        fileEntry.file(resolve, reject));
    const buf = await file.arrayBuffer();
    return arrayBufferToBase64(buf);
}

/**
 * Encode an ArrayBuffer as a standard base64 string. Chunks at 32K to stay
 * under String.fromCharCode.apply's arg-count limit (which crashes silently
 * for 50+ MB samples).
 *
 * Output is compatible with juce::Base64::convertFromBase64 (NOT
 * MemoryBlock::fromBase64Encoding — that's a JUCE proprietary format).
 */
export function arrayBufferToBase64 (buf) {
    const bytes = new Uint8Array(buf);
    const chunkSize = 0x8000;  // 32K bytes per chunk
    let binary = '';
    for (let i = 0; i < bytes.length; i += chunkSize) {
        binary += String.fromCharCode.apply(
            null, bytes.subarray(i, i + chunkSize));
    }
    return btoa(binary);
}

// ============================================================================
// Internals
// ============================================================================

function newDropSessionId () {
    return `s${Date.now()}-${Math.random().toString(36).slice(2, 8)}`;
}

function pointInClientRect (x, y, rect) {
    return x >= rect.left && x < rect.right
        && y >= rect.top  && y < rect.bottom;
}

async function collectAudioFilesFromDir (dirEntry, prefix, out, audioRe) {
    const reader = dirEntry.createReader();
    // FileSystemDirectoryReader.readEntries returns batches; keep reading
    // until an empty batch arrives (terminator).
    while (true) {
        const batch = await new Promise((resolve, reject) =>
            reader.readEntries(resolve, reject));
        if (!batch || batch.length === 0) break;

        for (const entry of batch) {
            if (entry.name.startsWith('.')) continue;  // skip hidden / .DS_Store
            const rel = prefix ? `${prefix}/${entry.name}` : entry.name;
            if (entry.isFile) {
                if (audioRe.test(entry.name)) {
                    // Cache the File metadata's size so the embed-size
                    // label in the load-options modal can sum without a
                    // second walk. entry.file() is metadata-only and cheap.
                    let size = 0;
                    try {
                        const file = await new Promise((resolve, reject) =>
                            entry.file(resolve, reject));
                        size = file.size;
                    } catch (e) {
                        // Best-effort: missing size just means a less
                        // accurate embed estimate — don't block the load.
                        console.warn('[webview-drop-streaming] file() metadata read failed:', e);
                    }
                    out.push({ entry, relativePath: rel, size });
                }
            } else if (entry.isDirectory) {
                await collectAudioFilesFromDir(entry, rel, out, audioRe);
            }
        }
    }
}

// Probe a DataTransfer for absolute file paths. Used by the fast path for
// hosts that expose them (Linux, Windows, some WebKit configs). WKWebView
// on macOS strips them; this almost always returns paths.length === 0.
function extractDroppedFilePaths (dt) {
    const diagnostics = { types: [], items: [], files: [] };
    if (!dt) return { paths: [], diagnostics };

    try { diagnostics.types = Array.from(dt.types || []); } catch (_) {}

    const paths = [];

    // text/uri-list — Linux/Windows convention
    try {
        const uriList = dt.getData('text/uri-list');
        if (uriList) {
            for (const line of uriList.split(/\r?\n/)) {
                const trimmed = line.trim();
                if (!trimmed || trimmed.startsWith('#')) continue;
                if (trimmed.startsWith('file://')) {
                    paths.push(decodeURIComponent(trimmed.slice('file://'.length)));
                }
            }
        }
    } catch (_) {}

    // File.path — Electron / some WebKit configs
    try {
        const fileList = Array.from(dt.files || []);
        for (const f of fileList) {
            diagnostics.files.push({ name: f.name, size: f.size, hasPath: !!f.path });
            if (f.path) paths.push(f.path);
        }
    } catch (_) {}

    return { paths, diagnostics };
}
