/*
  ==============================================================================

    WebViewDropStreaming.h
    Ouaricon Module — macOS WKWebView drag-drop content-streaming pattern.

    Header-only. Provides the C++ side of the v1.0.4 pattern (see history
    note below): a SessionManager that owns a per-session temp directory,
    validates JS-supplied payloads against the bundled DropSessionGuard,
    materialises base64-streamed audio files into the temp dir, and forwards
    folder/single-file commits to host-supplied loader callbacks.

    Why this exists
    ---------------
    WKWebView (macOS WebView2 backend) consumes OS-level drag events at the
    AppKit layer AND sandboxes the JS DataTransfer interface so paths are
    stripped. C++ FileDragAndDropTarget overrides never fire and JS can't
    forward absolute paths. The workaround: enumerate the dropped
    FileSystemEntry tree on the JS side, read each audio file via FileReader,
    base64-encode it, and stream the bytes through 4 native functions into a
    session-scoped temp dir. The host then runs its existing
    "load folder from disk" path against the temp dir, as if the user had
    chosen it via FileChooser.

    Pattern history
    ---------------
    Originated in O-MicrotonalSampler v1.0.4 (2026-01). Hardened in v1.11.2
    with path-traversal and size-cap guards. Promoted to this shared module
    in O-MicrotonalSampler v1.13.0 (ARCH-02 — REVIEW-architecture.md, May 2026).

    Surface
    -------
    Plugins instantiate one SessionManager and merge its native functions
    into their NativeFunction registry. Two callbacks bridge to plugin-
    specific loader semantics:

        Config cfg;
        cfg.tempDirPrefix = "myplugin-drop-";   // namespace-isolated
        cfg.onCommitFolder = [&p] (const juce::File& dir,
                                   const juce::String& displayName,
                                   int targetLayer,
                                   const juce::String& mode,
                                   bool overrideTokens,
                                   bool embedAudio,
                                   int targetTechnique,
                                   bool overrideTechnique)
        {
            p.loadSampleFolder (dir, targetLayer, parseMode (mode),
                                overrideTokens, "drag-drop", displayName,
                                embedAudio, targetTechnique, overrideTechnique);
        };
        cfg.onCommitFile = [&p] (const juce::File& file, int midi, int vel)
        {
            p.loadSingleSample (midi, vel, file);
        };

        sessionManager = std::make_unique<
            Ouaricon::WebViewDropStreaming::SessionManager> (std::move (cfg));

        // In buildNativeFunctionRegistry():
        for (auto& entry : sessionManager->getNativeFunctions())
            registry.push_back (std::move (entry));

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "DropSessionGuard.h"

#include <functional>
#include <utility>
#include <vector>

namespace Ouaricon::WebViewDropStreaming
{
    /**
     * Drag-drop session manager — owns per-session temp dir + 4 native
     * function handlers. One instance per editor.
     *
     * THREADING: All public methods (and the native-function lambdas
     * returned by getNativeFunctions()) run on the message thread, which is
     * how JUCE delivers WebView native function callbacks. No locking is
     * required as long as the host stays within that contract.
     */
    class SessionManager
    {
    public:
        /**
         * Folder-commit callback signature. Invoked from dropSessionCommitFolder
         * once all files in the drop have been streamed into `dir`.
         *
         *   dir               — the session temp dir containing the streamed files
         *   displayName       — human-friendly folder name (lifted from
         *                       FileSystemEntry::name in JS) for missing-folder
         *                       modal copy on session reload. Falls back to
         *                       dir.getFileName() if JS didn't supply one.
         *   targetLayer       — 0..3, args[1] from dropSessionCommitFolder
         *   mode              — args[2] string ("replace_all" / "append" /
         *                       "replace_layer" / "merge_rr"). Plugin parses to
         *                       its own LoadMode enum.
         *   overrideTokens    — args[3], default false
         *   embedAudio        — args[4], default false
         *   targetTechnique   — args[5], default 0 (added 2026-05-06 for
         *                       O-MicrotonalSampler v1.17.0). Optional —
         *                       JS callers that don't pass it get 0/false
         *                       so the callback runs identically to the
         *                       pre-v1.17.0 5-arg form.
         *   overrideTechnique — args[6], default false
         */
        using OnCommitFolder = std::function<void (const juce::File& dir,
                                                   const juce::String& displayName,
                                                   int targetLayer,
                                                   const juce::String& mode,
                                                   bool overrideTokens,
                                                   bool embedAudio,
                                                   int targetTechnique,
                                                   bool overrideTechnique)>;

        /**
         * Single-file-commit callback. Invoked from dropSessionCommitFile.
         *
         *   file — the streamed file inside the session temp dir
         *   midi — args[2], 0..127
         *   vel  — args[3], 0..numVelocityLayers-1 (plugin defines range)
         */
        using OnCommitFile = std::function<void (const juce::File& file,
                                                 int midi,
                                                 int vel)>;

        struct Config
        {
            /**
             * Temp-dir name prefix, e.g. "o-microtonalsampler-drop-".
             * MUST be unique per plugin so the stale-session reaper doesn't
             * delete another plugin's in-flight sessions. Keep it lowercase
             * and slug-style.
             */
            juce::String tempDirPrefix;

            /** See OnCommitFolder docstring above. Required. */
            OnCommitFolder onCommitFolder;

            /** See OnCommitFile docstring above. Required. */
            OnCommitFile onCommitFile;

            /**
             * Stale-session retention window in minutes. Sessions older than
             * this are deleted at the start of each new session. Default: 5.
             * The 5-min window is comfortably larger than typical background
             * loader read times for multi-GB libraries; tune up if your
             * loader is slower.
             */
            double staleSessionRetentionMinutes = 5.0;
        };

        explicit SessionManager (Config cfg)
            : config (std::move (cfg))
        {
            jassert (config.tempDirPrefix.isNotEmpty()
                     && "tempDirPrefix must be set so the stale-session reaper "
                        "doesn't collide with other plugins");
            jassert (config.onCommitFolder
                     && "onCommitFolder is required");
            jassert (config.onCommitFile
                     && "onCommitFile is required");
        }

        /**
         * Returns the 4 native function entries (dropSessionStart /
         * dropSessionAddFile / dropSessionCommitFolder / dropSessionCommitFile)
         * ready to merge into the editor's NativeFunction registry.
         *
         * Each lambda captures `this` — the SessionManager MUST outlive the
         * WebView. Declare it before the WebView in your editor (see
         * webview-relay-manager destruction-order pattern).
         */
        std::vector<std::pair<juce::Identifier, juce::WebBrowserComponent::NativeFunction>>
        getNativeFunctions()
        {
            std::vector<std::pair<juce::Identifier, juce::WebBrowserComponent::NativeFunction>> out;
            out.reserve (4);

            // ---- dropSessionStart ----
            //
            // args[0] = sessionId (opaque string from JS)
            // args[1] (optional) = folder name (FileSystemEntry::name) for
            //         missing-folder modal copy on session reload
            //
            // Returns true if the temp dir was created.
            out.emplace_back (
                juce::Identifier ("dropSessionStart"),
                [this] (const juce::Array<juce::var>& args,
                        std::function<void (juce::var)> complete)
                {
                    if (args.isEmpty())
                    {
                        complete (juce::var (false));
                        return;
                    }
                    const auto sessionId = args[0].toString();
                    if (sessionId.isEmpty())
                    {
                        complete (juce::var (false));
                        return;
                    }
                    const auto folderName = args.size() > 1
                        ? args[1].toString()
                        : juce::String();

                    cleanupStaleSessions();

                    auto dir = juce::File::getSpecialLocation (
                                   juce::File::tempDirectory)
                                       .getChildFile (config.tempDirPrefix
                                                      + sessionId);
                    const auto result = dir.createDirectory();
                    if (! result.wasOk())
                    {
                        DBG ("dropSessionStart: createDirectory failed: "
                             << result.getErrorMessage());
                        complete (juce::var (false));
                        return;
                    }

                    currentSessionId         = sessionId;
                    currentSessionDir        = dir;
                    currentSessionTotalBytes = 0;
                    currentSessionFolderName = folderName;
                    DBG ("dropSessionStart: " << dir.getFullPathName()
                         << " name=" << folderName);
                    complete (juce::var (true));
                });

            // ---- dropSessionAddFile ----
            //
            // args[0] = sessionId  (must match currentSessionId)
            // args[1] = relativePath inside the session dir (forward slashes,
            //           never backslashes)
            // args[2] = base64-encoded file content
            //
            // Returns true on successful write.
            out.emplace_back (
                juce::Identifier ("dropSessionAddFile"),
                [this] (const juce::Array<juce::var>& args,
                        std::function<void (juce::var)> complete)
                {
                    if (args.size() < 3)
                    {
                        complete (juce::var (false));
                        return;
                    }
                    const auto sessionId = args[0].toString();
                    const auto relPath   = args[1].toString();
                    const auto base64    = args[2].toString();

                    if (sessionId != currentSessionId
                        || ! currentSessionDir.isDirectory())
                    {
                        DBG ("dropSessionAddFile: session mismatch / dir gone");
                        complete (juce::var (false));
                        return;
                    }

                    // Path-traversal guard — reject before allocating.
                    {
                        const auto reason = ouaricon::dropguard::validateRelPath (relPath);
                        if (reason.isNotEmpty())
                        {
                            DBG ("dropSessionAddFile: relPath rejected ("
                                 << reason << "): " << relPath);
                            complete (juce::var (false));
                            return;
                        }
                    }

                    // Size-cap guard — reject before allocating decode buffer.
                    juce::uint64 projectedBytes = 0;
                    {
                        const auto reason = ouaricon::dropguard::checkSizeCaps (
                            base64.length(), currentSessionTotalBytes,
                            projectedBytes);
                        if (reason.isNotEmpty())
                        {
                            DBG ("dropSessionAddFile: size cap (" << reason
                                 << "): projected=" << (juce::int64) projectedBytes
                                 << ", session=" << (juce::int64) currentSessionTotalBytes
                                 << ", relPath=" << relPath);
                            complete (juce::var (false));
                            return;
                        }
                    }

                    // STANDARD base64 decode via juce::Base64. Note: do NOT
                    // use MemoryBlock::fromBase64Encoding — that is JUCE's
                    // own non-standard "<size>.<altAlphabet>" format and
                    // will reject JS btoa() output silently.
                    juce::MemoryBlock mb;
                    {
                        juce::MemoryOutputStream stream (mb, false);
                        if (! juce::Base64::convertFromBase64 (stream, base64))
                        {
                            DBG ("dropSessionAddFile: base64 decode failed for "
                                 << relPath << " (input length " << base64.length()
                                 << ", first 32 chars: '"
                                 << base64.substring (0, 32) << "')");
                            complete (juce::var (false));
                            return;
                        }
                        stream.flush();
                    }

                    auto target = currentSessionDir.getChildFile (relPath);

                    // Symlink-escape guard — walk the parent chain.
                    {
                        const auto reason = ouaricon::dropguard::validateParentChain (
                            currentSessionDir, target);
                        if (reason.isNotEmpty())
                        {
                            DBG ("dropSessionAddFile: parent chain rejected ("
                                 << reason << "): " << target.getFullPathName());
                            complete (juce::var (false));
                            return;
                        }
                    }

                    target.getParentDirectory().createDirectory();
                    if (! target.replaceWithData (mb.getData(), mb.getSize()))
                    {
                        DBG ("dropSessionAddFile: write failed: "
                             << target.getFullPathName());
                        complete (juce::var (false));
                        return;
                    }

                    currentSessionTotalBytes += (juce::uint64) mb.getSize();

                    DBG ("dropSessionAddFile: wrote " << mb.getSize()
                         << " bytes to " << target.getFullPathName()
                         << " (session total " << (juce::int64) currentSessionTotalBytes
                         << " B)");
                    complete (juce::var (true));
                });

            // ---- dropSessionCommitFolder ----
            //
            //   args[0] = sessionId (must match)
            //   args[1] (optional) = targetLayer 0..3       — default 0
            //   args[2] (optional) = mode string            — default "replace_all"
            //   args[3] (optional) = overrideTokens 0/1     — default 0
            //   args[4] (optional) = embedAudio 0/1         — default 0
            //   args[5] (optional) = targetTechnique 0..7   — default 0 (v1.17.0)
            //   args[6] (optional) = overrideTechnique 0/1  — default 0 (v1.17.0)
            //
            // Forwards the session dir + lifted folder name to onCommitFolder.
            // Temp dir is left in place; cleaned up at next dropSessionStart.
            out.emplace_back (
                juce::Identifier ("dropSessionCommitFolder"),
                [this] (const juce::Array<juce::var>& args,
                        std::function<void (juce::var)> complete)
                {
                    if (args.isEmpty()
                        || args[0].toString() != currentSessionId
                        || ! currentSessionDir.isDirectory())
                    {
                        complete (juce::var (false));
                        return;
                    }

                    const int  targetLayer     = args.size() > 1
                        ? juce::jlimit (0, 3, static_cast<int> (args[1])) : 0;
                    const auto modeStr         = args.size() > 2
                        ? args[2].toString() : juce::String ("replace_all");
                    const bool overrideTok     = args.size() > 3
                        ? static_cast<int> (args[3]) != 0 : false;
                    const bool embedAudio      = args.size() > 4
                        ? static_cast<int> (args[4]) != 0 : false;
                    const int  targetTech      = args.size() > 5
                        ? juce::jlimit (0, 7, static_cast<int> (args[5])) : 0;
                    const bool overrideTech    = args.size() > 6
                        ? static_cast<int> (args[6]) != 0 : false;

                    const auto displayName = currentSessionFolderName.isNotEmpty()
                        ? currentSessionFolderName
                        : currentSessionDir.getFileName();

                    DBG ("dropSessionCommitFolder: "
                         << currentSessionDir.getFullPathName()
                         << " layer=" << targetLayer
                         << " mode=" << modeStr
                         << " override=" << (int) overrideTok
                         << " embed=" << (int) embedAudio
                         << " technique=" << targetTech
                         << " overrideTech=" << (int) overrideTech
                         << " name=" << displayName);

                    config.onCommitFolder (currentSessionDir, displayName,
                                           targetLayer, modeStr, overrideTok,
                                           embedAudio, targetTech, overrideTech);
                    complete (juce::var (true));
                });

            // ---- dropSessionCommitFile ----
            //
            // args[0] = sessionId (must match)
            // args[1] = relativePath of the single file inside the session dir
            // args[2] = midi note (0..127)
            // args[3] = velocity layer (0..numVelocityLayers-1)
            out.emplace_back (
                juce::Identifier ("dropSessionCommitFile"),
                [this] (const juce::Array<juce::var>& args,
                        std::function<void (juce::var)> complete)
                {
                    if (args.size() < 4
                        || args[0].toString() != currentSessionId)
                    {
                        complete (juce::var (false));
                        return;
                    }
                    const auto relPath = args[1].toString();
                    const int  midi    = static_cast<int> (args[2]);
                    const int  vel     = static_cast<int> (args[3]);

                    const auto file = currentSessionDir.getChildFile (relPath);
                    if (! file.existsAsFile())
                    {
                        DBG ("dropSessionCommitFile: file missing: "
                             << file.getFullPathName());
                        complete (juce::var (false));
                        return;
                    }

                    DBG ("dropSessionCommitFile: midi=" << midi << " vel=" << vel
                         << " file=" << file.getFullPathName());
                    config.onCommitFile (file, midi, vel);
                    complete (juce::var (true));
                });

            return out;
        }

    private:
        // Deletes prior `<tempDirPrefix>*` temp dirs older than
        // staleSessionRetentionMinutes. Called at the start of every new
        // drop session so disk usage doesn't accumulate across many drops
        // in one running instance. macOS reclaims tempDirectory contents
        // independently; this is just bookkeeping.
        void cleanupStaleSessions()
        {
            auto temp = juce::File::getSpecialLocation (juce::File::tempDirectory);
            auto matches = temp.findChildFiles (
                juce::File::findDirectories, false,
                config.tempDirPrefix + "*");

            const auto now = juce::Time::getCurrentTime();
            for (auto& d : matches)
            {
                if ((now - d.getCreationTime()).inMinutes()
                    < config.staleSessionRetentionMinutes)
                    continue;
                d.deleteRecursively();
                DBG ("cleanupStaleSessions: deleted " << d.getFullPathName());
            }
        }

        Config       config;
        juce::String currentSessionId;
        juce::File   currentSessionDir;
        juce::uint64 currentSessionTotalBytes = 0;
        juce::String currentSessionFolderName;
    };
}
