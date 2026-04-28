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

            // ---- loadSampleFolderDialog (Phase 3.3 — folder picker for FUNC-05) ----
            //
            // JS calls: await Juce.getNativeFunction('loadSampleFolderDialog')().
            // Resolves true on a successful folder selection (forwarded to
            // processor.loadSampleFolder), false on cancel. The actual scan +
            // load is async — sampleMapUpdated fires when the new map has been
            // atomic-stored.
            .withNativeFunction ("loadSampleFolderDialog",
                [this] (const juce::Array<juce::var>&,
                        std::function<void(juce::var)> complete)
                {
                    auto chooser = std::make_shared<juce::FileChooser> (
                        "Choose folder containing sample files",
                        juce::File{},
                        juce::String{});

                    auto flags = juce::FileBrowserComponent::openMode
                               | juce::FileBrowserComponent::canSelectDirectories;

                    chooser->launchAsync (flags,
                        [this, chooser, complete]
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
                                 << folder.getFullPathName());
                            processorRef.loadSampleFolder (folder);
                            complete (juce::var (true));
                        });
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
    // Detach the processor's callback to prevent post-destruction calls.
    processorRef.setSampleMapChangedCallback (nullptr);
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
