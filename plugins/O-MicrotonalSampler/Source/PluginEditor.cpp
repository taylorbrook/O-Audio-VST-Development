/*
  ==============================================================================

    O-MicrotonalSampler - Editor Implementation (Phase 3.1: WebView shell)
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginEditor.h"
#include "BinaryData.h"
#include "TuningEngine.h"
#include "ScaleGenerator.h"
#include "EmbeddedTunings.h"
#include "TuningExporter.h"
// v1.0.3: drag-drop is handled at the JS layer (see sampler-app.js
// bindWebViewFileDrop) and forwarded to this editor's filesDropped()
// routing via the handleWebViewFileDrop native function below. The C++
// FileDragAndDropTarget overrides remain as defence-in-depth in case JUCE
// surfaces a drag over a non-WebView region of the editor in the future.
//
// Why JS-side: the WKWebView (and its internal content subviews) consume
// OS-level drag events at the AppKit layer before JUCE's parent
// FileDragAndDropTarget can route them. v1.0.1 tried -unregisterDraggedTypes
// (no effect — WebKit re-registers internally); v1.0.2 tried a transparent
// JUCE Component overlay (no effect — WebView OS rendering sits above
// regular JUCE Components). Per the JUCE forum thread on this issue, the
// validated approach is to handle drops in JavaScript, where WKWebView
// reliably fires DOM drop events for files dragged from Finder.

namespace
{
    // Helper: copy a BinaryData char array into a vector<byte> for the
    // WebView resource type.
    auto makeVector = [] (const char* data, int size)
    {
        return std::vector<std::byte> (
            reinterpret_cast<const std::byte*> (data),
            reinterpret_cast<const std::byte*> (data) + size);
    };
}

//==============================================================================
OMicrotonalSamplerAudioProcessorEditor::OMicrotonalSamplerAudioProcessorEditor (
    OMicrotonalSamplerAudioProcessor& p)
    : juce::AudioProcessorEditor (&p)
    , processorRef (p)
{
    // ----------------------------------------------------------------
    // 1️⃣ CREATE RELAYS FIRST — string identifier MUST match the APVTS
    //     parameter id and the Juce.getSliderState(...) call in JS.
    // ----------------------------------------------------------------
    attackRelay             = std::make_unique<juce::WebSliderRelay> ("attack");
    decayRelay              = std::make_unique<juce::WebSliderRelay> ("decay");
    sustainRelay            = std::make_unique<juce::WebSliderRelay> ("sustain");
    releaseRelay            = std::make_unique<juce::WebSliderRelay> ("release");
    polyphonyRelay          = std::make_unique<juce::WebSliderRelay> ("polyphony");
    velocityCrossfadeRelay  = std::make_unique<juce::WebSliderRelay> ("velocity_crossfade");
    outputGainRelay         = std::make_unique<juce::WebSliderRelay> ("output_gain");

    // ----------------------------------------------------------------
    // 2️⃣ CREATE WEBVIEW with options
    // ----------------------------------------------------------------
    webView = std::make_unique<juce::WebBrowserComponent> (
        juce::WebBrowserComponent::Options{}
            .withBackend (juce::WebBrowserComponent::Options::Backend::webview2)

            // Windows: explicit user-data folder under temp/ — default
            // location may be access-denied in DAW plugin hosts (memory).
            .withWinWebView2Options (
                juce::WebBrowserComponent::Options::WinWebView2{}
                    .withUserDataFolder (
                        juce::File::getSpecialLocation (
                            juce::File::SpecialLocationType::tempDirectory)
                                .getChildFile ("OMicrotonalSampler_WebView")))

            .withNativeIntegrationEnabled()
            .withResourceProvider ([this] (const auto& url) { return getResource (url); })

            // Register all relays so JS can find them via Juce.getSliderState(id).
            .withOptionsFrom (*attackRelay)
            .withOptionsFrom (*decayRelay)
            .withOptionsFrom (*sustainRelay)
            .withOptionsFrom (*releaseRelay)
            .withOptionsFrom (*polyphonyRelay)
            .withOptionsFrom (*velocityCrossfadeRelay)
            .withOptionsFrom (*outputGainRelay)

            // ============================================================
            // NATIVE FUNCTIONS (full impl in 3.1 + skeletons for later phases)
            // ============================================================

            // ---- getSampleMap : returns the JSON snapshot ----
            .withNativeFunction ("getSampleMap",
                [this] (const juce::Array<juce::var>&,
                        std::function<void(juce::var)> complete)
                {
                    complete (juce::var (processorRef.snapshotSampleMapJson()));
                })

            // ---- Tuning reads (TuningEngine accessors) ----
            .withNativeFunction ("getTuningName",
                [this] (const juce::Array<juce::var>&,
                        std::function<void(juce::var)> complete)
                {
                    auto* engine = processorRef.getTuningEngine();
                    complete (juce::var (engine != nullptr
                                             ? engine->getActiveTuningName()
                                             : juce::String ("12-TET")));
                })

            .withNativeFunction ("getTuningIntervals",
                [this] (const juce::Array<juce::var>&,
                        std::function<void(juce::var)> complete)
                {
                    auto* engine = processorRef.getTuningEngine();
                    juce::String json = "[";
                    if (engine != nullptr)
                    {
                        auto intervals = engine->getIntervals();
                        for (size_t i = 0; i < intervals.size(); ++i)
                        {
                            if (i > 0) json += ",";
                            json += juce::String (intervals[i], 6);
                        }
                    }
                    json += "]";
                    complete (juce::var (json));
                })

            .withNativeFunction ("getTonicNote",
                [this] (const juce::Array<juce::var>&,
                        std::function<void(juce::var)> complete)
                {
                    auto* engine = processorRef.getTuningEngine();
                    complete (juce::var (engine != nullptr
                                             ? engine->getTonicNote()
                                             : 60));
                })

            .withNativeFunction ("getOctaveStretch",
                [this] (const juce::Array<juce::var>&,
                        std::function<void(juce::var)> complete)
                {
                    auto* engine = processorRef.getTuningEngine();
                    complete (juce::var (engine != nullptr
                                             ? engine->getOctaveStretch()
                                             : 0.0f));
                })

            .withNativeFunction ("getPluginVersion",
                [] (const juce::Array<juce::var>&,
                    std::function<void(juce::var)> complete)
                {
                    complete (juce::var (JucePlugin_VersionString));
                })

            .withNativeFunction ("getEmbeddedTuningList",
                [] (const juce::Array<juce::var>&,
                    std::function<void(juce::var)> complete)
                {
                    const auto& tunings = EmbeddedTunings::getAllTunings();
                    juce::String json = "[";
                    for (size_t i = 0; i < tunings.size(); ++i)
                    {
                        if (i > 0) json += ",";
                        json += "{";
                        json += "\"id\":\""       + juce::String (tunings[i].id)       + "\",";
                        json += "\"name\":\""     + juce::String (tunings[i].name)     + "\",";
                        json += "\"category\":\"" + juce::String (tunings[i].category) + "\",";
                        json += "\"noteCount\":"  + juce::String (static_cast<int> (tunings[i].intervals.size()));
                        json += "}";
                    }
                    json += "]";
                    complete (juce::var (json));
                })

            .withNativeFunction ("getEmbeddedTuningCategories",
                [] (const juce::Array<juce::var>&,
                    std::function<void(juce::var)> complete)
                {
                    auto categories = EmbeddedTunings::getCategories();
                    juce::String json = "[";
                    for (size_t i = 0; i < categories.size(); ++i)
                    {
                        if (i > 0) json += ",";
                        json += "\"" + juce::String (categories[i]) + "\"";
                    }
                    json += "]";
                    complete (juce::var (json));
                })

            // ---- reportCellLayout : JS publishes grid layout for hit-testing ----
            .withNativeFunction ("reportCellLayout",
                [this] (const juce::Array<juce::var>& args,
                        std::function<void(juce::var)> complete)
                {
                    if (args.size() >= 1)
                    {
                        auto parsed = juce::JSON::parse (args[0].toString());
                        if (auto* obj = parsed.getDynamicObject())
                        {
                            cellLayout.clearQuick();
                            if (auto* cells = obj->getProperty ("cells").getArray())
                            {
                                for (const auto& c : *cells)
                                {
                                    if (auto* co = c.getDynamicObject())
                                    {
                                        CellRect r;
                                        r.midiNote      = static_cast<int> (co->getProperty ("midiNote"));
                                        r.velocityLayer = static_cast<int> (co->getProperty ("velocityLayer"));
                                        r.x             = static_cast<int> (co->getProperty ("x"));
                                        r.y             = static_cast<int> (co->getProperty ("y"));
                                        r.w             = static_cast<int> (co->getProperty ("w"));
                                        r.h             = static_cast<int> (co->getProperty ("h"));
                                        cellLayout.add (r);
                                    }
                                }
                            }
                            if (auto* fz = obj->getProperty ("folderZone").getDynamicObject())
                            {
                                folderZoneRect = juce::Rectangle<int> (
                                    static_cast<int> (fz->getProperty ("x")),
                                    static_cast<int> (fz->getProperty ("y")),
                                    static_cast<int> (fz->getProperty ("w")),
                                    static_cast<int> (fz->getProperty ("h")));
                            }
                        }
                    }
                    complete (juce::var());
                })

            // ============================================================
            // SKELETONS — full implementations land in 3.2/3.3/3.4.
            // Each returns a sane default so JS callers don't crash.
            // ============================================================

            // ---- loadSampleFolderDialog (Phase 3.3 / v1.6.0) ----
            //
            // JS calls: await Juce.getNativeFunction('loadSampleFolderDialog')(
            //               targetLayer, modeString, overrideTokens).
            //
            //   args[0] (optional) = targetLayer 0..3       — default 0
            //   args[1] (optional) = mode string            — default "replace_all"
            //                        ("append" | "replace_layer" | "replace_all")
            //   args[2] (optional) = overrideTokens 0/1     — default 0
            //
            // Missing-args path falls back to v1.5.x semantics so any caller
            // (or stale JS bundle) continues to work. Resolves true on a
            // successful folder selection (forwarded to processor.loadSampleFolder),
            // false on cancel. The actual scan + load is async — sampleMapUpdated
            // fires when the new map has been atomic-stored.
            .withNativeFunction ("loadSampleFolderDialog",
                [this] (const juce::Array<juce::var>& args,
                        std::function<void(juce::var)> complete)
                {
                    const int  targetLayer = args.size() > 0
                        ? juce::jlimit (0, 3, static_cast<int> (args[0])) : 0;
                    const auto modeStr     = args.size() > 1 ? args[1].toString()
                                                             : juce::String ("replace_all");
                    const bool overrideTok = args.size() > 2
                        ? static_cast<int> (args[2]) != 0 : false;

                    LoadMode mode = LoadMode::ReplaceAll;
                    if (modeStr == "append")        mode = LoadMode::Append;
                    else if (modeStr == "replace_layer") mode = LoadMode::ReplaceLayer;

                    auto chooser = std::make_shared<juce::FileChooser> (
                        "Choose folder containing sample files",
                        juce::File{},
                        juce::String{});

                    auto flags = juce::FileBrowserComponent::openMode
                               | juce::FileBrowserComponent::canSelectDirectories;

                    chooser->launchAsync (flags,
                        [this, chooser, complete, targetLayer, mode, overrideTok]
                            (const juce::FileChooser& fc) mutable
                        {
                            const auto results = fc.getResults();
                            if (results.isEmpty())
                            {
                                DBG ("loadSampleFolderDialog: cancelled");
                                complete (juce::var (false));
                                return;
                            }

                            const juce::File folder = results.getFirst();
                            if (! folder.isDirectory())
                            {
                                DBG ("loadSampleFolderDialog: selection is not a directory: "
                                     << folder.getFullPathName());
                                complete (juce::var (false));
                                return;
                            }

                            DBG ("loadSampleFolderDialog: folder="
                                 << folder.getFullPathName()
                                 << " layer=" << targetLayer
                                 << " mode=" << static_cast<int> (mode)
                                 << " override=" << (int) overrideTok);
                            processorRef.loadSampleFolder (folder, targetLayer, mode, overrideTok);
                            complete (juce::var (true));
                        });
                })

            // ---- dropSessionStart (v1.0.4 — content-streaming drag-drop) ----
            //
            // The user dragged a file or folder onto the WebView. WKWebView
            // exposes a FileSystemEntry to JS but strips absolute paths
            // (sandbox), so we cannot forward paths to filesDropped(). The
            // JS layer instead enumerates the entry tree, reads each audio
            // file via FileReader, and base64-streams the bytes to this
            // editor via dropSessionAddFile. We materialise them in a
            // session-scoped temp dir so the existing loadSampleFolder /
            // loadSingleSample paths consume the result as if the user had
            // picked it from a native FileChooser.
            //
            // args[0] = sessionId (opaque string from JS, used to scope
            //           the temp dir and validate subsequent calls)
            //
            // Returns true if the temp dir was created.
            .withNativeFunction ("dropSessionStart",
                [this] (const juce::Array<juce::var>& args,
                        std::function<void(juce::var)> complete)
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

                    cleanupStaleDropSessions();

                    auto dir = juce::File::getSpecialLocation (
                                   juce::File::tempDirectory)
                                       .getChildFile (
                                           "o-microtonalsampler-drop-" + sessionId);
                    const auto result = dir.createDirectory();
                    if (! result.wasOk())
                    {
                        DBG ("dropSessionStart: createDirectory failed: "
                             << result.getErrorMessage());
                        complete (juce::var (false));
                        return;
                    }

                    currentDropSessionId  = sessionId;
                    currentDropSessionDir = dir;
                    DBG ("dropSessionStart: " << dir.getFullPathName());
                    complete (juce::var (true));
                })

            // ---- dropSessionAddFile (v1.0.4) ----
            //
            // args[0] = sessionId  (must match currentDropSessionId)
            // args[1] = relativePath inside the session dir (forward slashes,
            //           never backslashes — JS controls the delimiter)
            // args[2] = base64-encoded file content
            //
            // Returns true on successful write.
            .withNativeFunction ("dropSessionAddFile",
                [this] (const juce::Array<juce::var>& args,
                        std::function<void(juce::var)> complete)
                {
                    if (args.size() < 3)
                    {
                        complete (juce::var (false));
                        return;
                    }
                    const auto sessionId = args[0].toString();
                    const auto relPath   = args[1].toString();
                    const auto base64    = args[2].toString();

                    if (sessionId != currentDropSessionId
                        || ! currentDropSessionDir.isDirectory())
                    {
                        DBG ("dropSessionAddFile: session mismatch / dir gone");
                        complete (juce::var (false));
                        return;
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

                    auto target = currentDropSessionDir.getChildFile (relPath);
                    target.getParentDirectory().createDirectory();
                    if (! target.replaceWithData (mb.getData(), mb.getSize()))
                    {
                        DBG ("dropSessionAddFile: write failed: "
                             << target.getFullPathName());
                        complete (juce::var (false));
                        return;
                    }

                    DBG ("dropSessionAddFile: wrote " << mb.getSize()
                         << " bytes to " << target.getFullPathName());
                    complete (juce::var (true));
                })

            // ---- dropSessionCommitFolder (v1.0.4 / v1.6.0) ----
            //
            // Calls processorRef.loadSampleFolder on the session temp dir.
            // The async SampleLoader thread reads the dir in the background
            // and posts the new SampleMap via sampleMapChangedCallback. The
            // temp dir is left in place; it will be cleaned up at the start
            // of the next drop session (cleanupStaleDropSessions).
            //
            //   args[0] = sessionId (must match)
            //   args[1] (optional) = targetLayer 0..3      — default 0
            //   args[2] (optional) = mode string           — default "replace_all"
            //   args[3] (optional) = overrideTokens 0/1    — default 0
            //
            // NB: drag-drop materialises files into a session temp dir so the
            // "path" the processor sees is /tmp/o-microtonalsampler-drop-<id>/.
            // That path is short-lived (cleaned up at the next drop session),
            // so a Save&Reopen cycle re-records a missing folder. This matches
            // existing v1.0.4 behaviour — drag-drop loads were never persisted.
            // Users who need persistence should use the Load Folder… picker.
            .withNativeFunction ("dropSessionCommitFolder",
                [this] (const juce::Array<juce::var>& args,
                        std::function<void(juce::var)> complete)
                {
                    if (args.isEmpty()
                        || args[0].toString() != currentDropSessionId
                        || ! currentDropSessionDir.isDirectory())
                    {
                        complete (juce::var (false));
                        return;
                    }

                    const int  targetLayer = args.size() > 1
                        ? juce::jlimit (0, 3, static_cast<int> (args[1])) : 0;
                    const auto modeStr     = args.size() > 2 ? args[2].toString()
                                                             : juce::String ("replace_all");
                    const bool overrideTok = args.size() > 3
                        ? static_cast<int> (args[3]) != 0 : false;

                    LoadMode mode = LoadMode::ReplaceAll;
                    if (modeStr == "append")        mode = LoadMode::Append;
                    else if (modeStr == "replace_layer") mode = LoadMode::ReplaceLayer;

                    DBG ("dropSessionCommitFolder: "
                         << currentDropSessionDir.getFullPathName()
                         << " layer=" << targetLayer
                         << " mode=" << static_cast<int> (mode)
                         << " override=" << (int) overrideTok);
                    processorRef.loadSampleFolder (currentDropSessionDir,
                                                    targetLayer, mode, overrideTok);
                    complete (juce::var (true));
                })

            // ---- dropSessionCommitFile (v1.0.4) ----
            //
            // args[0] = sessionId (must match)
            // args[1] = relativePath of the single file inside the session dir
            // args[2] = midi note (0..127)
            // args[3] = velocity layer (0..numVelocityLayers-1)
            .withNativeFunction ("dropSessionCommitFile",
                [this] (const juce::Array<juce::var>& args,
                        std::function<void(juce::var)> complete)
                {
                    if (args.size() < 4
                        || args[0].toString() != currentDropSessionId)
                    {
                        complete (juce::var (false));
                        return;
                    }
                    const auto relPath = args[1].toString();
                    const int  midi    = static_cast<int> (args[2]);
                    const int  vel     = static_cast<int> (args[3]);

                    const auto file = currentDropSessionDir.getChildFile (relPath);
                    if (! file.existsAsFile())
                    {
                        DBG ("dropSessionCommitFile: file missing: "
                             << file.getFullPathName());
                        complete (juce::var (false));
                        return;
                    }

                    DBG ("dropSessionCommitFile: midi=" << midi << " vel=" << vel
                         << " file=" << file.getFullPathName());
                    processorRef.loadSingleSample (midi, vel, file);
                    complete (juce::var (true));
                })

            // ---- handleWebViewFileDrop (v1.0.3 — JS-side drag-drop entry point) ----
            //
            // JS calls this from a document-level 'drop' listener on the
            // WebView. The C++ FileDragAndDropTarget overrides never fire
            // because WKWebView consumes OS-level drag events at the AppKit
            // layer (v1.0.1 -unregisterDraggedTypes and v1.0.2 overlay both
            // failed); see the comment block at the top of this file.
            //
            // args[0] = JSON-style array of absolute file paths
            //           (extracted from dataTransfer 'text/uri-list' or
            //           'public.file-url' on the JS side, or filename-only
            //           when the host doesn't expose paths)
            // args[1] = x in WebView client coords (= editor local coords)
            // args[2] = y in WebView client coords (= editor local coords)
            //
            // Forwards directly to filesDropped() so the existing Phase 3.3
            // routing matrix (cell hit / folder-zone hit / out-of-bounds /
            // toasts) is reused unchanged.
            .withNativeFunction ("handleWebViewFileDrop",
                [this] (const juce::Array<juce::var>& args,
                        std::function<void(juce::var)> complete)
                {
                    if (args.size() < 3)
                    {
                        DBG ("handleWebViewFileDrop: expected (paths, x, y), got "
                             << args.size() << " arg(s)");
                        complete (juce::var (false));
                        return;
                    }

                    juce::StringArray paths;
                    if (auto* arr = args[0].getArray())
                    {
                        for (const auto& pathVar : *arr)
                        {
                            const auto pathStr = pathVar.toString();
                            if (pathStr.isNotEmpty())
                                paths.add (pathStr);
                        }
                    }

                    const int x = static_cast<int> (args[1]);
                    const int y = static_cast<int> (args[2]);

                    DBG ("handleWebViewFileDrop: " << paths.size()
                         << " path(s) at (" << x << ", " << y << ")");

                    if (paths.isEmpty())
                    {
                        complete (juce::var (false));
                        return;
                    }

                    // Reuse the FileDragAndDropTarget routing (cell hit,
                    // folder-zone hit, toasts, out-of-bounds reject).
                    filesDropped (paths, x, y);
                    complete (juce::var (true));
                })

            // ---- clearSampleMap (v1.0.2 — destructive: empties the current map) ----
            //
            // JS calls: await Juce.getNativeFunction('clearSampleMap')(). The JS
            // side is responsible for surfacing a confirmation dialog before
            // invoking this — the native function performs the clear
            // unconditionally. Resolves true once the map has been atomic-stored
            // and the sampleMapUpdated push event has fired.
            .withNativeFunction ("clearSampleMap",
                [this] (const juce::Array<juce::var>&,
                        std::function<void(juce::var)> complete)
                {
                    processorRef.clearSampleMap();
                    complete (juce::var (true));
                })

            // ---- loadSingleSampleDialog (Phase 3.2 — FileChooser per cell) ----
            //
            // JS calls: await Juce.getNativeFunction('loadSingleSampleDialog')(midi, vel).
            // Resolves true on a successful selection (file passed to processor),
            // false on cancel or invalid args. The actual load is async — the
            // sampleMapUpdated event fires when the map has been atomic-stored.
            .withNativeFunction ("loadSingleSampleDialog",
                [this] (const juce::Array<juce::var>& args,
                        std::function<void(juce::var)> complete)
                {
                    if (args.size() < 2)
                    {
                        DBG ("loadSingleSampleDialog: expected (midi, vel), got "
                             << args.size() << " arg(s)");
                        complete (juce::var (false));
                        return;
                    }

                    const int midi = static_cast<int> (args[0]);
                    const int vel  = static_cast<int> (args[1]);

                    // Heap-allocate the FileChooser via shared_ptr so the
                    // launchAsync lambda can keep it alive until the user
                    // picks / cancels (JUCE 8 idiom — FileChooser must
                    // outlive the launchAsync call).
                    auto chooser = std::make_shared<juce::FileChooser> (
                        "Choose sample for MIDI " + juce::String (midi)
                            + " (layer " + juce::String (vel) + ")",
                        juce::File{},
                        "*.wav;*.aif;*.aiff;*.flac");

                    auto flags = juce::FileBrowserComponent::openMode
                               | juce::FileBrowserComponent::canSelectFiles;

                    // The launchAsync completion runs on the message thread.
                    // Capture chooser by value so its lifetime extends past
                    // the launch returning. Capture `complete` so JS resolves.
                    chooser->launchAsync (flags,
                        [this, chooser, midi, vel, complete]
                            (const juce::FileChooser& fc) mutable
                        {
                            const auto results = fc.getResults();
                            if (results.isEmpty())
                            {
                                DBG ("loadSingleSampleDialog: cancelled");
                                complete (juce::var (false));
                                return;
                            }

                            const juce::File file = results.getFirst();
                            if (! file.existsAsFile())
                            {
                                DBG ("loadSingleSampleDialog: selected file does not exist: "
                                     << file.getFullPathName());
                                complete (juce::var (false));
                                return;
                            }

                            DBG ("loadSingleSampleDialog: midi=" << midi
                                 << " vel=" << vel
                                 << " file=" << file.getFullPathName());

                            // Kick off the async per-cell load. The processor
                            // will fire sampleMapChangedCallback on completion
                            // (which we forward as the sampleMapUpdated WebView
                            // event in the editor's setSampleMapChangedCallback
                            // lambda). JS resolves immediately with `true` to
                            // unblock the await — the visual update arrives
                            // via the push event.
                            processorRef.loadSingleSample (midi, vel, file);
                            complete (juce::var (true));
                        });
                })

            .withNativeFunction ("getSkippedFiles",
                [this] (const juce::Array<juce::var>&,
                        std::function<void(juce::var)> complete)
                {
                    juce::String json = "[";
                    const auto& sk = processorRef.getLastSkippedFiles();
                    for (int i = 0; i < sk.size(); ++i)
                    {
                        if (i > 0) json += ",";
                        json += juce::JSON::toString (juce::var (sk[i]));
                    }
                    json += "]";
                    complete (juce::var (json));
                })

            // ---- overrideLoopPoints (Phase 3.4 — full impl) ----
            //
            // JS calls: await Juce.getNativeFunction('overrideLoopPoints')
            //              (midi, vel, loopStart, loopEnd, crossfadeLen).
            // Routes to processorRef.overrideLoopPoints(...). The
            // sampleMapUpdated push event fires automatically via the
            // processor's atomic-store + sampleMapChangedCallback.
            // Returns true on dispatch (not on audible application — that
            // happens on the next note-on per EC3-6).
            .withNativeFunction ("overrideLoopPoints",
                [this] (const juce::Array<juce::var>& args,
                        std::function<void(juce::var)> complete)
                {
                    if (args.size() < 4)
                    {
                        DBG ("overrideLoopPoints: expected (midi, vel, start, end[, xfade]), got "
                             << args.size() << " arg(s)");
                        complete (juce::var (false));
                        return;
                    }

                    const int midi      = static_cast<int> (args[0]);
                    const int vel       = static_cast<int> (args[1]);
                    const int loopStart = static_cast<int> (args[2]);
                    const int loopEnd   = static_cast<int> (args[3]);
                    const int xfade     = (args.size() >= 5)
                                             ? static_cast<int> (args[4])
                                             : 8;  // global default

                    processorRef.overrideLoopPoints (midi, vel, loopStart, loopEnd,
                                                     xfade, /*resetToAutoDetect*/ false);
                    complete (juce::var (true));
                })

            // ---- resetLoopToAutoDetect (Phase 3.4 — full impl) ----
            //
            // JS calls: await Juce.getNativeFunction('resetLoopToAutoDetect')(midi, vel).
            // Routes to processorRef.resetLoopToAutoDetect(...). Push update
            // arrives via sampleMapUpdated.
            .withNativeFunction ("resetLoopToAutoDetect",
                [this] (const juce::Array<juce::var>& args,
                        std::function<void(juce::var)> complete)
                {
                    if (args.size() < 2)
                    {
                        DBG ("resetLoopToAutoDetect: expected (midi, vel), got "
                             << args.size() << " arg(s)");
                        complete (juce::var (false));
                        return;
                    }

                    const int midi = static_cast<int> (args[0]);
                    const int vel  = static_cast<int> (args[1]);

                    processorRef.resetLoopToAutoDetect (midi, vel);
                    complete (juce::var (true));
                })

            // ---- getWaveformPeaks (Phase 3.4 — full impl) ----
            //
            // JS calls: await Juce.getNativeFunction('getWaveformPeaks')(midi, vel, bins).
            // Returns the JSON snapshot from snapshotWaveformPeaks per
            // RESEARCH §RQ3-5 schema. Click-driven path (loop-editor open),
            // so message-thread O(N) scan is acceptable (≈1 ms / 5 s sample).
            .withNativeFunction ("getWaveformPeaks",
                [this] (const juce::Array<juce::var>& args,
                        std::function<void(juce::var)> complete)
                {
                    if (args.size() < 2)
                    {
                        DBG ("getWaveformPeaks: expected (midi, vel[, bins]), got "
                             << args.size() << " arg(s)");
                        complete (juce::var (juce::String ("{}")));
                        return;
                    }

                    const int midi = static_cast<int> (args[0]);
                    const int vel  = static_cast<int> (args[1]);
                    const int bins = (args.size() >= 3)
                                        ? static_cast<int> (args[2])
                                        : 512;

                    complete (juce::var (
                        processorRef.snapshotWaveformPeaks (midi, vel, bins)));
                })

            // ============================================================
            // v1.2.0: TUNING WRITE-SIDE BRIDGES
            // The Stage 3 read-only design (§RQ3-1) is reversed in v1.2.0:
            // the panel is now editable. All write-side native functions
            // forward to the shared scala-tuning-engine module, the same
            // single-source-of-truth that VST3 Note Expression overrides
            // at note-on time, so Dorico microtonal playback is preserved.
            // ============================================================

            // ---- setSingleInterval(index, cents) ----
            .withNativeFunction ("setSingleInterval",
                [this] (const juce::Array<juce::var>& args,
                        std::function<void(juce::var)> complete)
                {
                    auto* engine = processorRef.getTuningEngine();
                    if (engine != nullptr && args.size() >= 2)
                    {
                        engine->setSingleInterval (static_cast<int>    (args[0]),
                                                   static_cast<double> (args[1]));
                        complete (juce::var (true));
                        return;
                    }
                    complete (juce::var (false));
                })

            // ---- setTonicNote(0..11) ----
            .withNativeFunction ("setTonicNote",
                [this] (const juce::Array<juce::var>& args,
                        std::function<void(juce::var)> complete)
                {
                    auto* engine = processorRef.getTuningEngine();
                    if (engine != nullptr && args.size() >= 1)
                    {
                        engine->setTonicNote (static_cast<int> (args[0]));
                        complete (juce::var (true));
                        return;
                    }
                    complete (juce::var (false));
                })

            // ---- setOctaveStretch(stretch) ----
            .withNativeFunction ("setOctaveStretch",
                [this] (const juce::Array<juce::var>& args,
                        std::function<void(juce::var)> complete)
                {
                    auto* engine = processorRef.getTuningEngine();
                    if (engine != nullptr && args.size() >= 1)
                    {
                        engine->setOctaveStretch (static_cast<float> (args[0]));
                        complete (juce::var (true));
                        return;
                    }
                    complete (juce::var (false));
                })

            // ---- setMasterTune(hz) ----
            .withNativeFunction ("setMasterTune",
                [this] (const juce::Array<juce::var>& args,
                        std::function<void(juce::var)> complete)
                {
                    auto* engine = processorRef.getTuningEngine();
                    if (engine != nullptr && args.size() >= 1)
                    {
                        engine->setMasterTune (static_cast<double> (args[0]));
                        complete (juce::var (true));
                        return;
                    }
                    complete (juce::var (false));
                })

            // ---- loadEmbeddedTuning(id) — apply a factory preset by ID ----
            .withNativeFunction ("loadEmbeddedTuning",
                [this] (const juce::Array<juce::var>& args,
                        std::function<void(juce::var)> complete)
                {
                    auto* engine = processorRef.getTuningEngine();
                    if (engine != nullptr && args.size() >= 1)
                    {
                        const auto idStr = args[0].toString().toStdString();
                        if (auto* t = EmbeddedTunings::getTuningById (idStr))
                        {
                            // EmbeddedTuning.intervals exclude the period; the
                            // engine expects intervals INCLUDING the closing
                            // period (matches setBuiltInPreset behaviour).
                            std::vector<double> withPeriod = t->intervals;
                            withPeriod.push_back (t->period);
                            engine->setCustomIntervals (withPeriod, juce::String (t->name));
                            complete (juce::var (true));
                            return;
                        }
                    }
                    complete (juce::var (false));
                })

            // ---- loadScalaFile() — open .scl file picker, return scale name on success ----
            .withNativeFunction ("loadScalaFile",
                [this] (const juce::Array<juce::var>&,
                        std::function<void(juce::var)> complete)
                {
                    auto chooser = std::make_shared<juce::FileChooser> (
                        "Load Scala Scale (.scl)",
                        juce::File::getSpecialLocation (juce::File::userDocumentsDirectory),
                        "*.scl");

                    chooser->launchAsync (juce::FileBrowserComponent::openMode
                                          | juce::FileBrowserComponent::canSelectFiles,
                        [this, chooser, complete] (const juce::FileChooser& fc)
                        {
                            auto file = fc.getResult();
                            auto* engine = processorRef.getTuningEngine();
                            if (engine != nullptr && file.existsAsFile()
                                && engine->loadScalaFile (file))
                            {
                                complete (juce::var (engine->getActiveTuningName()));
                                return;
                            }
                            complete (juce::var());  // empty → JS treats as cancel/fail
                        });
                })

            // ---- loadKBMFile() — open .kbm file picker ----
            .withNativeFunction ("loadKBMFile",
                [this] (const juce::Array<juce::var>&,
                        std::function<void(juce::var)> complete)
                {
                    auto chooser = std::make_shared<juce::FileChooser> (
                        "Load Keyboard Mapping (.kbm)",
                        juce::File::getSpecialLocation (juce::File::userDocumentsDirectory),
                        "*.kbm");

                    chooser->launchAsync (juce::FileBrowserComponent::openMode
                                          | juce::FileBrowserComponent::canSelectFiles,
                        [this, chooser, complete] (const juce::FileChooser& fc)
                        {
                            auto file = fc.getResult();
                            auto* engine = processorRef.getTuningEngine();
                            const bool ok = (engine != nullptr && file.existsAsFile()
                                             && engine->loadKBMFile (file));
                            complete (juce::var (ok));
                        });
                })

            // ---- saveScalaFile() — write current intervals as .scl ----
            .withNativeFunction ("saveScalaFile",
                [this] (const juce::Array<juce::var>&,
                        std::function<void(juce::var)> complete)
                {
                    auto* engine = processorRef.getTuningEngine();
                    if (engine == nullptr) { complete (juce::var (false)); return; }

                    auto chooser = std::make_shared<juce::FileChooser> (
                        "Save Scala Scale (.scl)",
                        juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                            .getChildFile (engine->getActiveTuningName() + ".scl"),
                        "*.scl");

                    chooser->launchAsync (juce::FileBrowserComponent::saveMode
                                          | juce::FileBrowserComponent::canSelectFiles
                                          | juce::FileBrowserComponent::warnAboutOverwriting,
                        [this, chooser, complete] (const juce::FileChooser& fc)
                        {
                            auto file = fc.getResult();
                            auto* eng = processorRef.getTuningEngine();
                            if (eng != nullptr && file != juce::File())
                            {
                                file.replaceWithText (eng->generateScalaFileContent());
                                complete (juce::var (true));
                                return;
                            }
                            complete (juce::var (false));
                        });
                })

            // ---- saveKBMFile() — write current keyboard mapping as .kbm ----
            .withNativeFunction ("saveKBMFile",
                [this] (const juce::Array<juce::var>&,
                        std::function<void(juce::var)> complete)
                {
                    auto* engine = processorRef.getTuningEngine();
                    if (engine == nullptr) { complete (juce::var (false)); return; }

                    auto chooser = std::make_shared<juce::FileChooser> (
                        "Save Keyboard Mapping (.kbm)",
                        juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                            .getChildFile ("mapping.kbm"),
                        "*.kbm");

                    chooser->launchAsync (juce::FileBrowserComponent::saveMode
                                          | juce::FileBrowserComponent::canSelectFiles
                                          | juce::FileBrowserComponent::warnAboutOverwriting,
                        [this, chooser, complete] (const juce::FileChooser& fc)
                        {
                            auto file = fc.getResult();
                            auto* eng = processorRef.getTuningEngine();
                            if (eng != nullptr && file != juce::File())
                            {
                                file.replaceWithText (eng->generateKBMFileContent());
                                complete (juce::var (true));
                                return;
                            }
                            complete (juce::var (false));
                        });
                })

            // ---- generateEDO(divisions, period) → JSON intervals ----
            .withNativeFunction ("generateEDO",
                [] (const juce::Array<juce::var>& args,
                    std::function<void(juce::var)> complete)
                {
                    if (args.size() >= 2)
                    {
                        auto intervals = ScaleGenerator::generateEDO (
                            static_cast<int>    (args[0]),
                            static_cast<double> (args[1]));
                        juce::String json = "[";
                        for (size_t i = 0; i < intervals.size(); ++i)
                        {
                            if (i > 0) json += ",";
                            json += juce::String (intervals[i], 6);
                        }
                        json += "]";
                        complete (juce::var (json));
                        return;
                    }
                    complete (juce::var());
                })

            // ---- generateHarmonicSeries(start, end) → JSON intervals ----
            .withNativeFunction ("generateHarmonicSeries",
                [] (const juce::Array<juce::var>& args,
                    std::function<void(juce::var)> complete)
                {
                    if (args.size() >= 2)
                    {
                        auto intervals = ScaleGenerator::generateHarmonicSeries (
                            static_cast<int> (args[0]),
                            static_cast<int> (args[1]));
                        juce::String json = "[";
                        for (size_t i = 0; i < intervals.size(); ++i)
                        {
                            if (i > 0) json += ",";
                            json += juce::String (intervals[i], 6);
                        }
                        json += "]";
                        complete (juce::var (json));
                        return;
                    }
                    complete (juce::var());
                })

            // ---- generateRank2(generator, period, count) → JSON intervals ----
            .withNativeFunction ("generateRank2",
                [] (const juce::Array<juce::var>& args,
                    std::function<void(juce::var)> complete)
                {
                    if (args.size() >= 3)
                    {
                        auto intervals = ScaleGenerator::generateRank2 (
                            static_cast<double> (args[0]),
                            static_cast<double> (args[1]),
                            static_cast<int>    (args[2]));
                        juce::String json = "[";
                        for (size_t i = 0; i < intervals.size(); ++i)
                        {
                            if (i > 0) json += ",";
                            json += juce::String (intervals[i], 6);
                        }
                        json += "]";
                        complete (juce::var (json));
                        return;
                    }
                    complete (juce::var());
                })

            // ---- applyGeneratedScale(intervalsJson, name) ----
            .withNativeFunction ("applyGeneratedScale",
                [this] (const juce::Array<juce::var>& args,
                        std::function<void(juce::var)> complete)
                {
                    auto* engine = processorRef.getTuningEngine();
                    if (engine != nullptr && args.size() >= 2)
                    {
                        auto parsed = juce::JSON::parse (args[0].toString());
                        if (auto* arr = parsed.getArray())
                        {
                            std::vector<double> cents;
                            cents.reserve (static_cast<size_t> (arr->size()));
                            for (const auto& v : *arr)
                                cents.push_back (static_cast<double> (v));
                            engine->setCustomIntervals (cents, args[1].toString());
                            complete (juce::var (true));
                            return;
                        }
                    }
                    complete (juce::var (false));
                })

            // ============================================================
            // v1.3.0: STATE PERSISTENCE — preset save/load + missing-folder
            // recovery. The plugin already round-trips full state through
            // get/setStateInformation (DAW project save/load), so the
            // .omspreset format simply re-uses that ValueTree as plain XML
            // text written to a user-chosen file. Missing-folder recovery
            // surfaces the saved path in a modal so the user can either
            // relocate or skip without re-loading the entire bank.
            // ============================================================

            // ---- saveCurrentPreset() — write current state to .omspreset ----
            //
            // Captures the same ValueTree that getStateInformation persists
            // (APVTS params + SampleFolder path + full TuningState) and writes
            // it as XML text to a user-chosen file. Path-only per Q1=A — the
            // .omspreset is small and shareable across projects on the same
            // machine, but breaks across machines without matching folder
            // structure. JS resolves true on success, false on cancel/fail.
            .withNativeFunction ("saveCurrentPreset",
                [this] (const juce::Array<juce::var>&,
                        std::function<void(juce::var)> complete)
                {
                    auto chooser = std::make_shared<juce::FileChooser> (
                        "Save Preset",
                        juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                            .getChildFile ("O-MicrotonalSampler.omspreset"),
                        "*.omspreset");

                    chooser->launchAsync (juce::FileBrowserComponent::saveMode
                                          | juce::FileBrowserComponent::canSelectFiles
                                          | juce::FileBrowserComponent::warnAboutOverwriting,
                        [this, chooser, complete] (const juce::FileChooser& fc) mutable
                        {
                            auto file = fc.getResult();
                            if (file == juce::File())
                            {
                                complete (juce::var (false));
                                return;
                            }
                            const auto xml = processorRef.capturePresetXml();
                            if (xml.isEmpty() || ! file.replaceWithText (xml))
                            {
                                complete (juce::var (false));
                                return;
                            }
                            complete (juce::var (true));
                        });
                })

            // ---- loadPreset() — read .omspreset and restore state ----
            //
            // Replaces APVTS, tuning, and the loaded folder. If the saved
            // folder path no longer exists, surfaces the standard missing-
            // folder modal (same pathway used during DAW project reopen).
            .withNativeFunction ("loadPreset",
                [this] (const juce::Array<juce::var>&,
                        std::function<void(juce::var)> complete)
                {
                    auto chooser = std::make_shared<juce::FileChooser> (
                        "Load Preset",
                        juce::File::getSpecialLocation (juce::File::userDocumentsDirectory),
                        "*.omspreset");

                    chooser->launchAsync (juce::FileBrowserComponent::openMode
                                          | juce::FileBrowserComponent::canSelectFiles,
                        [this, chooser, complete] (const juce::FileChooser& fc) mutable
                        {
                            auto file = fc.getResult();
                            if (! file.existsAsFile())
                            {
                                complete (juce::var (false));
                                return;
                            }
                            const auto xml = file.loadFileAsString();
                            const bool ok  = processorRef.restorePresetXml (xml);
                            complete (juce::var (ok));
                        });
                })

            // ---- locateMissingFolder() — folder picker for missing-folder modal ----
            //
            // JS surfaces this from the modal's "Locate folder…" button.
            // Reuses the existing loadSampleFolder pathway, which clears the
            // pending missing-folder slot via setStateInformation's normal
            // success flow. JS resolves true if a folder was selected (and
            // forwarded to the processor), false on cancel.
            .withNativeFunction ("locateMissingFolder",
                [this] (const juce::Array<juce::var>&,
                        std::function<void(juce::var)> complete)
                {
                    auto chooser = std::make_shared<juce::FileChooser> (
                        "Locate sample folder",
                        juce::File{},
                        juce::String{});
                    auto flags = juce::FileBrowserComponent::openMode
                               | juce::FileBrowserComponent::canSelectDirectories;

                    chooser->launchAsync (flags,
                        [this, chooser, complete] (const juce::FileChooser& fc) mutable
                        {
                            const auto results = fc.getResults();
                            if (results.isEmpty() || ! results.getFirst().isDirectory())
                            {
                                complete (juce::var (false));
                                return;
                            }
                            processorRef.clearPendingMissingFolder();
                            processorRef.loadSampleFolder (results.getFirst());
                            complete (juce::var (true));
                        });
                })

            // ---- dismissMissingFolder() — user chose Skip on the modal ----
            .withNativeFunction ("dismissMissingFolder",
                [this] (const juce::Array<juce::var>&,
                        std::function<void(juce::var)> complete)
                {
                    processorRef.clearPendingMissingFolder();
                    complete (juce::var (true));
                })

            // ---- getPendingMissingFolder() — covers boot-time race ----
            //
            // setStateInformation may run before the WebView has registered
            // its folderMissing event listener (DAW project reopen → state
            // restore happens before editor attach). JS calls this once on
            // boot to recover any missed event. Returns the saved path or
            // an empty string.
            .withNativeFunction ("getPendingMissingFolder",
                [this] (const juce::Array<juce::var>&,
                        std::function<void(juce::var)> complete)
                {
                    complete (juce::var (processorRef.getPendingMissingFolderPath()));
                })

            // ---- exportTuningHTML() — write current tuning to HTML doc ----
            .withNativeFunction ("exportTuningHTML",
                [this] (const juce::Array<juce::var>&,
                        std::function<void(juce::var)> complete)
                {
                    auto* engine = processorRef.getTuningEngine();
                    if (engine == nullptr) { complete (juce::var (false)); return; }

                    auto chooser = std::make_shared<juce::FileChooser> (
                        "Export Tuning Documentation",
                        juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                            .getChildFile (engine->getActiveTuningName() + ".html"),
                        "*.html");

                    chooser->launchAsync (juce::FileBrowserComponent::saveMode
                                          | juce::FileBrowserComponent::canSelectFiles
                                          | juce::FileBrowserComponent::warnAboutOverwriting,
                        [this, chooser, complete] (const juce::FileChooser& fc)
                        {
                            auto file = fc.getResult();
                            auto* eng = processorRef.getTuningEngine();
                            if (eng != nullptr && file != juce::File())
                            {
                                auto html = TuningExporter::toHTML (*eng, "O-MicrotonalSampler");
                                file.replaceWithText (html);
                                complete (juce::var (true));
                                return;
                            }
                            complete (juce::var (false));
                        });
                })
    );

    // ----------------------------------------------------------------
    // 3️⃣ CREATE ATTACHMENTS LAST
    // ----------------------------------------------------------------
    auto& apvts = processorRef.getAPVTS();

    attackAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *apvts.getParameter ("attack"), *attackRelay, nullptr);
    decayAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *apvts.getParameter ("decay"), *decayRelay, nullptr);
    sustainAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *apvts.getParameter ("sustain"), *sustainRelay, nullptr);
    releaseAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *apvts.getParameter ("release"), *releaseRelay, nullptr);
    polyphonyAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *apvts.getParameter ("polyphony"), *polyphonyRelay, nullptr);
    velocityCrossfadeAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *apvts.getParameter ("velocity_crossfade"), *velocityCrossfadeRelay, nullptr);
    outputGainAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *apvts.getParameter ("output_gain"), *outputGainRelay, nullptr);

    // Add WebView to editor
    addAndMakeVisible (*webView);

    // Subscribe to processor's sample-map change callback. Editor lifetime
    // is shorter than processor's — clear the callback in our destructor.
    processorRef.setSampleMapChangedCallback (
        [this]
        {
            if (webView != nullptr)
                webView->emitEventIfBrowserIsVisible (
                    "sampleMapUpdated",
                    juce::var (processorRef.snapshotSampleMapJson()));
        });

    // v1.3.0: subscribe to missing-folder callback. setStateInformation
    // fires this when a saved folder path no longer exists on disk; the JS
    // handler renders the "Locate folder?" modal. Boot-time race (state
    // restore before WebView attach) is covered by the JS-side
    // getPendingMissingFolder pull on first ready.
    processorRef.setMissingFolderCallback (
        [this] (const juce::String& savedPath)
        {
            if (webView != nullptr)
                webView->emitEventIfBrowserIsVisible (
                    "folderMissing", juce::var (savedPath));
        });

    // Navigate to the resource provider's root (cross-platform — never
    // hard-code juce:// vs https://juce.backend/).
    webView->goToURL (juce::WebBrowserComponent::getResourceProviderRoot());

    // Window: resizable, default 900×640, min 720×480, max 1600×1080 (D3-14).
    setResizable (true, true);
    setSize (900, 640);
    setResizeLimits (720, 480, 1600, 1080);
}

OMicrotonalSamplerAudioProcessorEditor::~OMicrotonalSamplerAudioProcessorEditor()
{
    // Detach the processor's callbacks to prevent post-destruction calls.
    processorRef.setSampleMapChangedCallback (nullptr);
    processorRef.setMissingFolderCallback (nullptr);
    // unique_ptr members destroy in reverse declaration order:
    //   attachments (each calls evaluateJavascript on webView during dtor)
    //   webView
    //   relays
}

//==============================================================================
void OMicrotonalSamplerAudioProcessorEditor::paint (juce::Graphics& g)
{
    // WebView paints itself.
    juce::ignoreUnused (g);
}

void OMicrotonalSamplerAudioProcessorEditor::resized()
{
    if (webView != nullptr)
        webView->setBounds (getLocalBounds());
}

//==============================================================================
// v1.0.4: deletes any prior `o-microtonalsampler-drop-*` temp dirs that
// are older than 5 minutes (a window comfortably larger than typical
// SampleLoader read times). Called at the start of every new drop
// session so disk usage doesn't accumulate across many drops in one
// running instance. macOS reclaims its tempDirectory contents
// independently; this call is just bookkeeping.
void OMicrotonalSamplerAudioProcessorEditor::cleanupStaleDropSessions()
{
    auto temp = juce::File::getSpecialLocation (juce::File::tempDirectory);
    auto matches = temp.findChildFiles (
        juce::File::findDirectories, false,
        "o-microtonalsampler-drop-*");

    const auto now = juce::Time::getCurrentTime();
    for (auto& d : matches)
    {
        if ((now - d.getCreationTime()).inMinutes() < 5.0)
            continue;
        d.deleteRecursively();
        DBG ("cleanupStaleDropSessions: deleted " << d.getFullPathName());
    }
}

//==============================================================================
// Resource provider — direct URL→BinaryData equality, matches O-Bells
// pattern (PluginEditor.cpp:941-998). The callback receives PATHS, not
// full URLs; never strip schemes via fromFirstOccurrenceOf("://").
std::optional<juce::WebBrowserComponent::Resource>
OMicrotonalSamplerAudioProcessorEditor::getResource (const juce::String& url)
{
    if (url == "/" || url == "/index.html")
    {
        return juce::WebBrowserComponent::Resource {
            makeVector (BinaryData::index_html, BinaryData::index_htmlSize),
            juce::String ("text/html") };
    }

    if (url == "/css/sampler-shell.css")
    {
        return juce::WebBrowserComponent::Resource {
            makeVector (BinaryData::samplershell_css, BinaryData::samplershell_cssSize),
            juce::String ("text/css") };
    }

    if (url == "/css/tuning-panel.css")
    {
        return juce::WebBrowserComponent::Resource {
            makeVector (BinaryData::tuningpanel_css, BinaryData::tuningpanel_cssSize),
            juce::String ("text/css") };
    }

    if (url == "/css/tuning-panel-readonly.css")
    {
        return juce::WebBrowserComponent::Resource {
            makeVector (BinaryData::tuningpanelreadonly_css, BinaryData::tuningpanelreadonly_cssSize),
            juce::String ("text/css") };
    }

    if (url == "/js/sampler-app.js")
    {
        return juce::WebBrowserComponent::Resource {
            makeVector (BinaryData::samplerapp_js, BinaryData::samplerapp_jsSize),
            juce::String ("text/javascript") };
    }

    if (url == "/js/tuning-panel.js")
    {
        return juce::WebBrowserComponent::Resource {
            makeVector (BinaryData::tuningpanel_js, BinaryData::tuningpanel_jsSize),
            juce::String ("text/javascript") };
    }

    if (url == "/js/juce/index.js")
    {
        return juce::WebBrowserComponent::Resource {
            makeVector (BinaryData::index_js, BinaryData::index_jsSize),
            juce::String ("text/javascript") };
    }

    if (url == "/js/juce/check_native_interop.js")
    {
        return juce::WebBrowserComponent::Resource {
            makeVector (BinaryData::check_native_interop_js,
                        BinaryData::check_native_interop_jsSize),
            juce::String ("text/javascript") };
    }

    juce::Logger::writeToLog ("O-MicrotonalSampler: Resource not found: " + url);
    return std::nullopt;
}

//==============================================================================
// FileDragAndDropTarget — Phase 3.3 full routing per RESEARCH §RQ3-6.
//
// Hit-test sources:
//   - cellLayout: published by JS via reportCellLayout (Phase 3.2 Task 17).
//     One CellRect per (midi, velocityLayer) in WebView client space.
//   - folderZoneRect: same publish path; rect of #folder-drop-zone.
//
// Routing matrix (RP3-3 / EC3-3):
//   Cell hit + .wav/.aif/.aiff       → loadSingleSample(midi, vel, file)
//   Cell hit + folder                → toast "Drop a single file on a cell, or a folder on the top zone."
//   Cell hit + other ext             → toast "Drop a .wav/.aif on a cell"
//   Folder-zone hit + folder         → loadSampleFolder(folder)
//   Folder-zone hit + non-folder     → toast "Drop a folder, not a file"
//   Out-of-bounds                    → silent reject
//
// Drag visuals are driven from JS by listening to hostFileDragMove /
// hostFileDragExit events emitted from fileDragMove / fileDragExit.
//
// Note: Single-file behaviour — when multiple files are dropped, we route
// the first one only and skip the rest (consistent with macOS host behaviour
// where DnD onto a single target is a single-target operation).

namespace
{
    inline bool isAudioFileExt (const juce::String& ext)
    {
        const auto e = ext.toLowerCase();
        return e == ".wav" || e == ".aif" || e == ".aiff";
    }

    // Helper: emit a toast event to JS. Single-string payload becomes the
    // toast message; JS toast queue dismisses after 3 s.
    void emitToast (juce::WebBrowserComponent* webView, const juce::String& msg)
    {
        if (webView != nullptr)
            webView->emitEventIfBrowserIsVisible ("toast", juce::var (msg));
    }
}

bool OMicrotonalSamplerAudioProcessorEditor::isInterestedInFileDrag (
    const juce::StringArray& files)
{
    return ! files.isEmpty();
}

void OMicrotonalSamplerAudioProcessorEditor::filesDropped (
    const juce::StringArray& files, int x, int y)
{
    // Always clear hover state, regardless of routing outcome.
    if (webView != nullptr)
        webView->emitEventIfBrowserIsVisible ("hostFileDragExit", juce::var());

    if (files.isEmpty())
        return;

    const juce::File file (files[0]);
    const bool fileIsDirectory = file.isDirectory();
    const auto ext = file.getFileExtension();

    // ---- 1. Cell hit-test (highest priority — the grid sits below the zone) ----
    for (const auto& c : cellLayout)
    {
        if (x >= c.x && x < c.x + c.w && y >= c.y && y < c.y + c.h)
        {
            // Cell hit. Now branch on payload type.
            if (fileIsDirectory)
            {
                // EC3-3: folder dropped onto a cell → user intent ambiguous;
                // surface the routing rule explicitly.
                emitToast (webView.get(),
                           "Drop a single file on a cell, or a folder on the top zone.");
                return;
            }

            if (! isAudioFileExt (ext))
            {
                emitToast (webView.get(), "Drop a .wav/.aif on a cell");
                return;
            }

            // Audio file on cell → forward to the per-cell loader. The
            // sampleMapUpdated push event will refresh the grid + skipped-files.
            DBG ("filesDropped: cell hit midi=" << c.midiNote
                 << " vel=" << c.velocityLayer
                 << " file=" << file.getFullPathName());
            processorRef.loadSingleSample (c.midiNote, c.velocityLayer, file);
            return;
        }
    }

    // ---- 2. Folder-zone hit-test ----
    if (! folderZoneRect.isEmpty()
        && x >= folderZoneRect.getX() && x < folderZoneRect.getRight()
        && y >= folderZoneRect.getY() && y < folderZoneRect.getBottom())
    {
        if (fileIsDirectory)
        {
            DBG ("filesDropped: folder-zone hit folder=" << file.getFullPathName());
            processorRef.loadSampleFolder (file);
        }
        else
        {
            emitToast (webView.get(), "Drop a folder, not a file");
        }
        return;
    }

    // ---- 3. Out-of-bounds — silent reject ----
}

void OMicrotonalSamplerAudioProcessorEditor::fileDragEnter (
    const juce::StringArray& files, int x, int y)
{
    juce::ignoreUnused (files);
    if (webView == nullptr) return;
    auto payload = new juce::DynamicObject();
    payload->setProperty ("x", x);
    payload->setProperty ("y", y);
    webView->emitEventIfBrowserIsVisible ("hostFileDragMove", juce::var (payload));
}

void OMicrotonalSamplerAudioProcessorEditor::fileDragMove (
    const juce::StringArray& files, int x, int y)
{
    juce::ignoreUnused (files);
    if (webView == nullptr) return;
    auto payload = new juce::DynamicObject();
    payload->setProperty ("x", x);
    payload->setProperty ("y", y);
    webView->emitEventIfBrowserIsVisible ("hostFileDragMove", juce::var (payload));
}

void OMicrotonalSamplerAudioProcessorEditor::fileDragExit (
    const juce::StringArray& files)
{
    juce::ignoreUnused (files);
    if (webView == nullptr) return;
    webView->emitEventIfBrowserIsVisible ("hostFileDragExit", juce::var());
}
