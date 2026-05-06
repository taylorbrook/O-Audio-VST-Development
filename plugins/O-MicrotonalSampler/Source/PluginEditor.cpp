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
    // v1.16.7 (HIGH-03): JSON payload for held-notes / freezes broadcasts.
    // Format is `{"notes":[i…],"freqs":[f…]}` with 4-digit freq precision —
    // load-bearing for the TuningPanel TrueKeys / Circle / Polar visualisations.
    inline juce::String buildNotesFreqsJson (const std::vector<int>& notes,
                                             const std::vector<double>& freqs)
    {
        juce::String notesArr = "[", freqsArr = "[";
        const auto n = std::min (notes.size(), freqs.size());
        for (size_t i = 0; i < n; ++i)
        {
            if (i > 0) { notesArr += ","; freqsArr += ","; }
            notesArr += juce::String (notes[i]);
            freqsArr += juce::String (freqs[i], 4);
        }
        notesArr += "]";
        freqsArr += "]";
        return "{\"notes\":" + notesArr + ",\"freqs\":" + freqsArr + "}";
    }

    // v1.16.7 (HIGH-04): cents-array → JSON list with 6-digit precision.
    // Used by the four scale-generator native fns and the tuning-engine
    // intervals readout. Precision is load-bearing — JS parses with parseFloat
    // and any drift would silently desync the Rank2/EDO previews.
    inline juce::String centsArrayToJson (const std::vector<double>& cents)
    {
        juce::String json = "[";
        for (size_t i = 0; i < cents.size(); ++i)
        {
            if (i > 0) json += ",";
            json += juce::String (cents[i], 6);
        }
        json += "]";
        return json;
    }

    // v1.16.7 (HIGH-05): boolean-arg APVTS setter for the KS / CC / PC enable
    // toggles. Returns true iff the param was located and updated, matching
    // the prior {complete(false) on miss, complete(true) on success} contract.
    inline bool setBoolParamFromArgs (juce::AudioProcessorValueTreeState& apvts,
                                      juce::StringRef paramId,
                                      const juce::Array<juce::var>& args)
    {
        if (args.size() < 1) return false;
        auto* p = apvts.getParameter (paramId);
        if (p == nullptr) return false;
        p->setValueNotifyingHost (static_cast<bool> (args[0]) ? 1.0f : 0.0f);
        return true;
    }

    // v1.16.10 (MEDIUM-03): generic JSON-array builder. Each remaining
    // indexed comma-skip loop in the native-fn registry collapses to one
    // line via this helper. Format identity to the prior loops is
    // load-bearing — UI sites parse with JSON.parse / parseFloat.
    template <class Vec, class Formatter>
    inline juce::String joinJsonArray (const Vec& v, Formatter&& fmt)
    {
        juce::String out = "[";
        bool first = true;
        for (const auto& x : v)
        {
            if (! first) out += ",";
            out += fmt (x);
            first = false;
        }
        out += "]";
        return out;
    }
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
    expressionRelay         = std::make_unique<juce::WebSliderRelay> ("expression");      // v1.7.0
    outputGainRelay         = std::make_unique<juce::WebSliderRelay> ("output_gain");

    // ----------------------------------------------------------------
    // 1.5️⃣ CONSTRUCT DROP-SESSION MANAGER (v1.13.0 — ARCH-02)
    //      Must outlive the WebView; its native-function lambdas are
    //      captured into the registry below. Pass per-plugin tempDirPrefix
    //      so the stale-session reaper doesn't collide with future plugins
    //      that adopt this module.
    // ----------------------------------------------------------------
    {
        Ouaricon::WebViewDropStreaming::SessionManager::Config cfg;
        cfg.tempDirPrefix = "o-microtonalsampler-drop-";
        cfg.onCommitFolder = [this] (const juce::File& dir,
                                     const juce::String& displayName,
                                     int targetLayer,
                                     const juce::String& modeStr,
                                     bool overrideTokens,
                                     bool embedAudio,
                                     int targetTechnique,
                                     bool overrideTechnique)
        {
            LoadMode mode = LoadMode::ReplaceAll;
            if      (modeStr == "append")        mode = LoadMode::Append;
            else if (modeStr == "replace_layer") mode = LoadMode::ReplaceLayer;
            else if (modeStr == "merge_rr")      mode = LoadMode::MergeRR;

            processorRef.loadSampleFolder (dir, targetLayer, mode, overrideTokens,
                                           "drag-drop", displayName, embedAudio,
                                           targetTechnique, overrideTechnique);
        };
        cfg.onCommitFile = [this] (const juce::File& file, int midi, int vel)
        {
            processorRef.loadSingleSample (midi, vel, file);
        };
        dropSessions = std::make_unique<
            Ouaricon::WebViewDropStreaming::SessionManager> (std::move (cfg));
    }

    // ----------------------------------------------------------------
    // 2️⃣ CREATE WEBVIEW with options
    // ----------------------------------------------------------------
    webView = std::make_unique<juce::WebBrowserComponent> (
        [this]
        {
            // v1.12.4: native functions registered via the data-driven
            // registry below (buildNativeFunctionRegistry) instead of 42
            // inline chained .withNativeFunction(...) calls (~1400 lines
            // of organic v1.5.0–v1.12.0 growth). Constructor stays small;
            // each native function is one vector entry. Behaviour is
            // unchanged — every entry preserves its original name,
            // capture list, and body verbatim.
            auto opts = juce::WebBrowserComponent::Options{}
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
                .withOptionsFrom (*expressionRelay)        // v1.7.0
                .withOptionsFrom (*outputGainRelay);

            for (auto& [name, handler] : buildNativeFunctionRegistry())
                opts = opts.withNativeFunction (name, std::move (handler));

            return opts;
        }());

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
    expressionAttachment = std::make_unique<juce::WebSliderParameterAttachment> (    // v1.7.0
        *apvts.getParameter ("expression"), *expressionRelay, nullptr);
    outputGainAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *apvts.getParameter ("output_gain"), *outputGainRelay, nullptr);

    // Add WebView to editor
    addAndMakeVisible (*webView);

#if OUARICON_LICENSING_ENABLED
    // Licensing: activation overlay (visible until licensed)
    // Native WebView renders on top of JUCE components, so we must
    // hide the WebView while the overlay is showing.
    // License manager lives on the processor (persists across editor open/close).
    auto& license = processorRef.getLicenseManager();
    licenseOverlay = std::make_unique<OuariconLicenseOverlay>(license);
    addAndMakeVisible(licenseOverlay.get());

    license.addListener(this);

    if (! license.isLicensed())
        webView->setVisible(false);
    else
        licenseOverlay->setVisible(false);
#endif

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
    //
    // v1.12.0: payload widened from a bare path string to a {path, kind, name}
    // object. JS branches on `kind` ("filesystem" or "drag-drop") to render
    // the appropriate modal copy. drag-drop missings have empty `path` and
    // a meaningful `name` lifted from the original drop's FileSystemEntry.
    processorRef.setMissingFolderCallback (
        [this] (const juce::String& savedPath,
                const juce::String& kind,
                const juce::String& name)
        {
            if (webView == nullptr) return;
            auto* obj = new juce::DynamicObject();
            obj->setProperty ("path", savedPath);
            obj->setProperty ("kind", kind);
            obj->setProperty ("name", name);
            webView->emitEventIfBrowserIsVisible (
                "folderMissing", juce::var (obj));
        });

    // v1.14.0: subscribe to technique-state changes (vocab rename, count
    // change, KS toggle, MIDI keyswitch event). The JS layer polls
    // getTechniqueState on the resulting event to refresh the tab strip.
    processorRef.setTechniqueStateChangedCallback (
        [this]
        {
            if (webView != nullptr)
                webView->emitEventIfBrowserIsVisible (
                    "techniqueStateUpdated", juce::var (true));
        });

    // v1.15.0: subscribe to trigger-state changes (CC mapping slot edit,
    // PC mapping slot edit, defaults reset, project restore). The JS
    // layer polls getTriggerState on the resulting event.
    processorRef.setTriggerStateChangedCallback (
        [this]
        {
            if (webView != nullptr)
                webView->emitEventIfBrowserIsVisible (
                    "triggerStateUpdated", juce::var (true));
        });

    // v1.8.0: subscribe to ambiguous-duplicate callback. The folder loader
    // detected (midi, layer) groups with > 1 file but no rr/take/tk tokens —
    // surface a confirmation modal in the WebView. Payload is a JSON array
    // of { midiNote, velocityLayer, filenames: [...] } entries.
    processorRef.setAmbiguousDuplicateCallback (
        [this] (const std::vector<SampleLoader::AmbiguousDuplicate>& dups)
        {
            if (webView == nullptr)
                return;

            juce::var arr (juce::Array<juce::var>{});
            auto* a = arr.getArray();
            for (const auto& d : dups)
            {
                auto* obj = new juce::DynamicObject();
                obj->setProperty ("midiNote",      d.midiNote);
                obj->setProperty ("velocityLayer", d.velocityLayer);
                obj->setProperty ("technique",     d.technique);    // v1.14.0
                juce::var fnArr (juce::Array<juce::var>{});
                auto* fa = fnArr.getArray();
                for (const auto& s : d.filenames)
                    fa->add (juce::var (s));
                obj->setProperty ("filenames", fnArr);
                a->add (juce::var (obj));
            }
            webView->emitEventIfBrowserIsVisible (
                "ambiguousDuplicates",
                juce::var (juce::JSON::toString (arr, /*allOnOneLine*/ true)));
        });

    // Navigate to the resource provider's root (cross-platform — never
    // hard-code juce:// vs https://juce.backend/).
    webView->goToURL (juce::WebBrowserComponent::getResourceProviderRoot());

    // Window: resizable, default 900×640, min 720×480, max 1600×1080 (D3-14).
    setResizable (true, true);
    setSize (900, 640);
    setResizeLimits (720, 480, 1600, 1080);

    // v1.7.1: 30 Hz tuning-note polling. Diffs the synth's lock-free
    // active-notes bitmask against prevActiveNotes* and emits per-note
    // events to the WebView so the TuningPanel views (Circle / Polar /
    // TrueKeys) can highlight in real time. Cheap — at most 16 voices and
    // bit ops are branch-free.
    startTimerHz (30);
}

OMicrotonalSamplerAudioProcessorEditor::~OMicrotonalSamplerAudioProcessorEditor()
{
#if OUARICON_LICENSING_ENABLED
    processorRef.getLicenseManager().removeListener(this);
#endif

    // Stop the tuning-note timer before any of our members go away — the
    // callback touches webView and processorRef.
    stopTimer();

    // Detach the processor's callbacks to prevent post-destruction calls.
    processorRef.setSampleMapChangedCallback (nullptr);
    processorRef.setMissingFolderCallback (nullptr);
    processorRef.setAmbiguousDuplicateCallback (nullptr);   // v1.8.0
    processorRef.setTechniqueStateChangedCallback (nullptr); // v1.14.0
    processorRef.setTriggerStateChangedCallback (nullptr);   // v1.15.0
    // unique_ptr members destroy in reverse declaration order:
    //   attachments (each calls evaluateJavascript on webView during dtor)
    //   webView
    //   relays
}

//==============================================================================
// v1.7.1: 30 Hz tuning-note polling.
//
// Reads the synth's lock-free active-notes bitmask, diffs against the previous
// snapshot, and emits three event types to the WebView:
//
//   tuningNoteOn  : { midi: <int> }     — fired once per new note-on this tick
//   tuningNoteOff : { midi: <int> }     — fired once per new note-off this tick
//   tuningHeldNotes: { notes:[...], freqs:[...] } — fired only on changes
//
// sampler-app.js subscribes to all three and forwards to the TuningPanel
// instance so the Circle / Polar views highlight active scale degrees and
// TrueKeys can compute interval cents from the live tuning frequencies.
//
// Cost is negligible — at most 16 voices, branch-free bit ops, plus one
// std::vector population per tick when the held set changes.
void OMicrotonalSamplerAudioProcessorEditor::timerCallback()
{
    if (webView == nullptr)
        return;

    juce::uint64 low = 0, high = 0;
    processorRef.getActiveNotesAtomic (low, high);

    if (low == prevActiveNotesLow && high == prevActiveNotesHigh)
        return; // No change — skip event emission entirely.

    const juce::uint64 turnedOnLow   = low  & ~prevActiveNotesLow;
    const juce::uint64 turnedOnHigh  = high & ~prevActiveNotesHigh;
    const juce::uint64 turnedOffLow  = prevActiveNotesLow  & ~low;
    const juce::uint64 turnedOffHigh = prevActiveNotesHigh & ~high;

    // Note-on events — base offset 0 for low half, 64 for high half.
    for (int i = 0; i < 64; ++i)
    {
        const juce::uint64 mask = (juce::uint64) 1 << i;
        if (turnedOnLow  & mask)
            webView->emitEventIfBrowserIsVisible ("tuningNoteOn",  juce::var (i));
        if (turnedOnHigh & mask)
            webView->emitEventIfBrowserIsVisible ("tuningNoteOn",  juce::var (i + 64));
    }
    for (int i = 0; i < 64; ++i)
    {
        const juce::uint64 mask = (juce::uint64) 1 << i;
        if (turnedOffLow  & mask)
            webView->emitEventIfBrowserIsVisible ("tuningNoteOff", juce::var (i));
        if (turnedOffHigh & mask)
            webView->emitEventIfBrowserIsVisible ("tuningNoteOff", juce::var (i + 64));
    }

    // TrueKeys payload — full held set + tuned frequencies. JSON-stringify
    // here so the JS side can pass it to TuningPanel.updateHeldNotes(...)
    // without further parsing.
    std::vector<int>    heldNotes;
    std::vector<double> heldFreqs;
    processorRef.getHeldNotesData (heldNotes, heldFreqs);

    webView->emitEventIfBrowserIsVisible ("tuningHeldNotes",
                                          juce::var (buildNotesFreqsJson (heldNotes, heldFreqs)));

    prevActiveNotesLow  = low;
    prevActiveNotesHigh = high;
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

#if OUARICON_LICENSING_ENABLED
    if (licenseOverlay != nullptr)
        licenseOverlay->setBounds(getLocalBounds());
#endif
}

//==============================================================================
// v1.13.0 (ARCH-02): cleanupStaleDropSessions() removed. The 5-min reaper
// for `o-microtonalsampler-drop-*` temp dirs now lives inside the shared
// modules/core/webview-drop-streaming module's SessionManager, scoped to
// the per-plugin tempDirPrefix passed at construction.

//==============================================================================
// Resource provider — direct URL→BinaryData equality, matches O-Bells
// pattern (PluginEditor.cpp:941-998). The callback receives PATHS, not
// full URLs; never strip schemes via fromFirstOccurrenceOf("://").
std::optional<juce::WebBrowserComponent::Resource>
OMicrotonalSamplerAudioProcessorEditor::getResource (const juce::String& url)
{
    // Helper: copy a BinaryData char array into a vector<byte> for the
    // WebView resource type. v1.16.9 (LOW-05): scoped to getResource — the
    // only caller — instead of file-anonymous-namespace.
    auto makeVector = [] (const char* data, int size)
    {
        return std::vector<std::byte> (
            reinterpret_cast<const std::byte*> (data),
            reinterpret_cast<const std::byte*> (data) + size);
    };

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

    if (url == "/js/sampler-app.js")
    {
        return juce::WebBrowserComponent::Resource {
            makeVector (BinaryData::samplerapp_js, BinaryData::samplerapp_jsSize),
            juce::String ("text/javascript") };
    }

    // v1.13.0 (ARCH-02): shared webview-drop-streaming module JS, served
    // from BinaryData via the module-JS path the import in sampler-app.js
    // points to ('./modules/webview-drop-streaming.js' — relative to the
    // sampler-app.js URL, so resolves to '/js/modules/...').
    if (url == "/js/modules/webview-drop-streaming.js")
    {
        return juce::WebBrowserComponent::Resource {
            makeVector (BinaryData::webviewdropstreaming_js,
                        BinaryData::webviewdropstreaming_jsSize),
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

    // v1.10.0: Naturalist aesthetic overlay — antique anatomical engraving
    if (url == "/images/brains.png")
    {
        return juce::WebBrowserComponent::Resource {
            makeVector (BinaryData::brains_png, BinaryData::brains_pngSize),
            juce::String ("image/png") };
    }

    // v1.11.0: paper textures — page (paper1) + card surfaces (paper2)
    if (url == "/images/paper1.jpg")
    {
        return juce::WebBrowserComponent::Resource {
            makeVector (BinaryData::paper1_jpg, BinaryData::paper1_jpgSize),
            juce::String ("image/jpeg") };
    }

    if (url == "/images/paper2.jpg")
    {
        return juce::WebBrowserComponent::Resource {
            makeVector (BinaryData::paper2_jpg, BinaryData::paper2_jpgSize),
            juce::String ("image/jpeg") };
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

//==============================================================================
// v1.12.4: Data-driven native function registry. Replaces 42 inline
// .withNativeFunction(...) chained calls that lived in the constructor
// (~1400 lines, organic v1.5.0–v1.12.0 growth). The constructor iterates
// this vector once and forwards each entry to the Options builder.
//
// Behaviour is unchanged — every entry preserves its original name,
// capture list, and body verbatim. See REVIEW-architecture.md §1.
// Pattern is reusable in O-Bells / O-Lyrica which carry similar boilerplate.
std::vector<std::pair<juce::Identifier, juce::WebBrowserComponent::NativeFunction>>
OMicrotonalSamplerAudioProcessorEditor::buildNativeFunctionRegistry()
{
    std::vector<std::pair<juce::Identifier, juce::WebBrowserComponent::NativeFunction>> registry = {
        { "getSampleMap",
                [this] (const juce::Array<juce::var>&,
                        std::function<void(juce::var)> complete)
                {
                    complete (juce::var (processorRef.snapshotSampleMapJson()));
                }
        },

        // ---- Tuning reads (TuningEngine accessors) ----
        { "getTuningName",
                [this] (const juce::Array<juce::var>&,
                        std::function<void(juce::var)> complete)
                {
                    auto* engine = processorRef.getTuningEngine();
                    complete (juce::var (engine != nullptr
                                             ? engine->getActiveTuningName()
                                             : juce::String ("12-TET")));
                }
        },

        { "getTuningIntervals",
                [this] (const juce::Array<juce::var>&,
                        std::function<void(juce::var)> complete)
                {
                    auto* engine = processorRef.getTuningEngine();
                    complete (juce::var (engine != nullptr
                                             ? centsArrayToJson (engine->getIntervals())
                                             : juce::String ("[]")));
                }
        },

        { "getTonicNote",
                [this] (const juce::Array<juce::var>&,
                        std::function<void(juce::var)> complete)
                {
                    auto* engine = processorRef.getTuningEngine();
                    complete (juce::var (engine != nullptr
                                             ? engine->getTonicNote()
                                             : 60));
                }
        },

        { "getOctaveStretch",
                [this] (const juce::Array<juce::var>&,
                        std::function<void(juce::var)> complete)
                {
                    auto* engine = processorRef.getTuningEngine();
                    complete (juce::var (engine != nullptr
                                             ? engine->getOctaveStretch()
                                             : 0.0f));
                }
        },

        { "getPluginVersion",
                [] (const juce::Array<juce::var>&,
                    std::function<void(juce::var)> complete)
                {
                    complete (juce::var (JucePlugin_VersionString));
                }
        },

        // v1.7.1: catch-up pull for the TuningPanel. The 30 Hz timer
        // only emits tuningHeldNotes on change, so a panel that mounts
        // (lazy — first Tuning-tab activation) while notes are already
        // held would otherwise see no event until the next change.
        // Returns the same payload shape as the tuningHeldNotes event:
        // `{"notes":[...],"freqs":[...]}` JSON string.
        { "getHeldNotesJson",
                [this] (const juce::Array<juce::var>&,
                        std::function<void(juce::var)> complete)
                {
                    std::vector<int>    notes;
                    std::vector<double> freqs;
                    processorRef.getHeldNotesData (notes, freqs);
                    complete (juce::var (buildNotesFreqsJson (notes, freqs)));
                }
        },

        { "getEmbeddedTuningList",
                [] (const juce::Array<juce::var>&,
                    std::function<void(juce::var)> complete)
                {
                    const auto& tunings = EmbeddedTunings::getAllTunings();
                    auto json = joinJsonArray (tunings, [] (const auto& t)
                    {
                        return juce::String ("{")
                             + "\"id\":\""       + juce::String (t.id)       + "\","
                             + "\"name\":\""     + juce::String (t.name)     + "\","
                             + "\"category\":\"" + juce::String (t.category) + "\","
                             + "\"noteCount\":"  + juce::String (static_cast<int> (t.intervals.size()))
                             + "}";
                    });
                    complete (juce::var (json));
                }
        },

        { "getEmbeddedTuningCategories",
                [] (const juce::Array<juce::var>&,
                    std::function<void(juce::var)> complete)
                {
                    auto categories = EmbeddedTunings::getCategories();
                    auto json = joinJsonArray (categories, [] (const auto& c)
                    {
                        return "\"" + juce::String (c) + "\"";
                    });
                    complete (juce::var (json));
                }
        },

        // ---- reportCellLayout : JS publishes grid layout for hit-testing ----
        { "reportCellLayout",
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
                }
        },

        // ============================================================
        // SKELETONS — full implementations land in 3.2/3.3/3.4.
        // Each returns a sane default so JS callers don't crash.
        // ============================================================

        // ---- pickSampleFolder (v1.12.0) ----
        //
        // Replaces v1.6.0's combined `loadSampleFolderDialog` with a pure
        // file picker. JS sequences: pickSampleFolder() → maybe
        // estimateFolderAudioSize() → maybe size-confirm modal →
        // loadSampleFolderByPath(). The split lets JS show a real
        // size-warning between selection and load when the user opted
        // into embedAudio.
        //
        // Resolves an object: { path: "...", cancelled: false } on success,
        // { path: "", cancelled: true } when the user dismissed the picker.
        // (Always returns an object so JS can `await` once and inspect.)
        { "pickSampleFolder",
                [] (const juce::Array<juce::var>&,
                    std::function<void(juce::var)> complete)
                {
                    auto chooser = std::make_shared<juce::FileChooser> (
                        "Choose folder containing sample files",
                        juce::File{},
                        juce::String{});

                    auto flags = juce::FileBrowserComponent::openMode
                               | juce::FileBrowserComponent::canSelectDirectories;

                    chooser->launchAsync (flags,
                        [chooser, complete] (const juce::FileChooser& fc) mutable
                        {
                            auto* obj = new juce::DynamicObject();
                            const auto results = fc.getResults();

                            if (results.isEmpty() || ! results.getFirst().isDirectory())
                            {
                                obj->setProperty ("path", juce::String());
                                obj->setProperty ("cancelled", true);
                                complete (juce::var (obj));
                                return;
                            }

                            const juce::File folder = results.getFirst();
                            obj->setProperty ("path",      folder.getFullPathName());
                            obj->setProperty ("name",      folder.getFileName());
                            obj->setProperty ("cancelled", false);
                            DBG ("pickSampleFolder: " << folder.getFullPathName());
                            complete (juce::var (obj));
                        });
                }
        },

        // ---- estimateFolderAudioSize (v1.12.0) ----
        //
        // Returns the total size (in bytes) of audio files inside `path`,
        // recursively. Used by JS to populate the "Embed will add ~X MB"
        // size-warning modal before the user commits to an embed load.
        //
        // Extension set matches SampleLoader's wildcard
        // (*.wav;*.aif;*.aiff;*.flac). Source-file size is reported (not
        // post-resample PCM size); this is honest about the disk cost
        // and acceptable as an order-of-magnitude estimate.
        //
        //   args[0] = absolute folder path
        // Returns: bytes (juce::int64). Returns 0 on invalid path.
        { "estimateFolderAudioSize",
                [] (const juce::Array<juce::var>& args,
                    std::function<void(juce::var)> complete)
                {
                    if (args.isEmpty())
                    {
                        complete (juce::var (juce::int64 (0)));
                        return;
                    }
                    const juce::File folder (args[0].toString());
                    if (! folder.isDirectory())
                    {
                        complete (juce::var (juce::int64 (0)));
                        return;
                    }

                    juce::int64 totalBytes = 0;
                    const juce::String wildcards = "*.wav;*.aif;*.aiff;*.flac";
                    for (const auto& entry : juce::RangedDirectoryIterator (
                                                  folder, /*recursive=*/true,
                                                  wildcards,
                                                  juce::File::findFiles))
                    {
                        totalBytes += entry.getFile().getSize();
                    }
                    complete (juce::var (totalBytes));
                }
        },

        // ---- loadSampleFolderByPath (v1.12.0; v1.17.0 +technique args) ----
        //
        // Loads a folder by absolute path with full origin/embed metadata.
        // Used by JS after `pickSampleFolder` (and any embed-confirm
        // modal). The pre-pick options modal returns layer/mode/override/
        // embed/technique/overrideTechnique; this fn stitches them
        // together with the picked path.
        //
        //   args[0] = absolute folder path (required)
        //   args[1] (optional) = targetLayer 0..3       — default 0
        //   args[2] (optional) = mode string             — default "replace_all"
        //                        ("append" | "replace_layer" | "replace_all" | "merge_rr")
        //   args[3] (optional) = overrideTokens 0/1     — default 0
        //   args[4] (optional) = embedAudio 0/1         — default 0
        //   args[5] (optional) = targetTechnique 0..7   — default 0 (v1.17.0)
        //   args[6] (optional) = overrideTechnique 0/1  — default 0 (v1.17.0)
        //
        // Resolves true if the load was dispatched; false if path is
        // invalid. The actual scan + load is async — sampleMapUpdated
        // fires when the new map has been atomic-stored.
        { "loadSampleFolderByPath",
                [this] (const juce::Array<juce::var>& args,
                        std::function<void(juce::var)> complete)
                {
                    if (args.isEmpty())
                    {
                        complete (juce::var (false));
                        return;
                    }
                    const juce::File folder (args[0].toString());
                    if (! folder.isDirectory())
                    {
                        DBG ("loadSampleFolderByPath: not a directory: "
                             << folder.getFullPathName());
                        complete (juce::var (false));
                        return;
                    }

                    const int  targetLayer  = args.size() > 1
                        ? juce::jlimit (0, 3, static_cast<int> (args[1])) : 0;
                    const auto modeStr      = args.size() > 2 ? args[2].toString()
                                                              : juce::String ("replace_all");
                    const bool overrideTok  = args.size() > 3
                        ? static_cast<int> (args[3]) != 0 : false;
                    const bool embedAudio   = args.size() > 4
                        ? static_cast<int> (args[4]) != 0 : false;
                    const int  targetTech   = args.size() > 5
                        ? juce::jlimit (0, 7, static_cast<int> (args[5])) : 0;
                    const bool overrideTech = args.size() > 6
                        ? static_cast<int> (args[6]) != 0 : false;

                    LoadMode mode = LoadMode::ReplaceAll;
                    if (modeStr == "append")        mode = LoadMode::Append;
                    else if (modeStr == "replace_layer") mode = LoadMode::ReplaceLayer;
                    else if (modeStr == "merge_rr")      mode = LoadMode::MergeRR;

                    DBG ("loadSampleFolderByPath: folder="
                         << folder.getFullPathName()
                         << " layer=" << targetLayer
                         << " mode=" << static_cast<int> (mode)
                         << " override=" << (int) overrideTok
                         << " embed=" << (int) embedAudio
                         << " technique=" << targetTech
                         << " overrideTech=" << (int) overrideTech);
                    processorRef.loadSampleFolder (folder, targetLayer, mode, overrideTok,
                                                    "filesystem", folder.getFileName(),
                                                    embedAudio, targetTech, overrideTech);
                    complete (juce::var (true));
                }
        },

        // ---- dropSession* (4 native fns) ----
        //
        // v1.13.0 (ARCH-02): the content-streaming drag-drop pattern is
        // now in modules/core/webview-drop-streaming. The 4 native
        // function handlers (dropSessionStart / dropSessionAddFile /
        // dropSessionCommitFolder / dropSessionCommitFile), session
        // lifecycle, 5-min stale-temp-dir reaper, and DropSessionGuard
        // validators all live there. We splice the module-supplied entries
        // into this vector after the brace-initialized list closes (see
        // bottom of this function).

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
        { "handleWebViewFileDrop",
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
                }
        },

        // ---- clearSampleMap (v1.0.2 — destructive: empties the current map) ----
        //
        // JS calls: await Juce.getNativeFunction('clearSampleMap')(). The JS
        // side is responsible for surfacing a confirmation dialog before
        // invoking this — the native function performs the clear
        // unconditionally. Resolves true once the map has been atomic-stored
        // and the sampleMapUpdated push event has fired.
        { "clearSampleMap",
                [this] (const juce::Array<juce::var>&,
                        std::function<void(juce::var)> complete)
                {
                    processorRef.clearSampleMap();
                    complete (juce::var (true));
                }
        },

        // ---- loadSingleSampleDialog (Phase 3.2 — FileChooser per cell) ----
        //
        // JS calls: await Juce.getNativeFunction('loadSingleSampleDialog')
        //              (midi, vel, mergeAsRr=false).
        // Resolves true on a successful selection (file passed to processor),
        // false on cancel or invalid args. The actual load is async — the
        // sampleMapUpdated event fires when the map has been atomic-stored.
        //
        // v1.9.0: optional mergeAsRr arg. true = append to existing cell's
        // variants vector instead of replacing the cell (per-cell round-
        // robin layering). The JS UI is responsible for surfacing the
        // merge prompt before calling — see showPerCellMergeDialog.
        { "loadSingleSampleDialog",
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

                    const int  midi      = static_cast<int> (args[0]);
                    const int  vel       = static_cast<int> (args[1]);
                    const bool mergeAsRr = args.size() > 2 && static_cast<bool> (args[2]);
                    // v1.14.0: optional technique slot (default = current
                    // active technique). The active technique mirrors what
                    // the user sees as the highlighted tab in the UI; if the
                    // JS layer doesn't pass an explicit slot, we route the
                    // load to wherever the user is editing.
                    const int  technique = args.size() > 3
                        ? juce::jlimit (0, 7, static_cast<int> (args[3]))
                        : processorRef.getActiveTechnique();

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
                        [this, chooser, midi, vel, mergeAsRr, technique, complete]
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
                                 << " tech=" << technique
                                 << " mergeAsRr=" << (int) mergeAsRr
                                 << " file=" << file.getFullPathName());

                            // Kick off the async per-cell load. The processor
                            // will fire sampleMapChangedCallback on completion
                            // (which we forward as the sampleMapUpdated WebView
                            // event in the editor's setSampleMapChangedCallback
                            // lambda). JS resolves immediately with `true` to
                            // unblock the await — the visual update arrives
                            // via the push event.
                            processorRef.loadSingleSample (midi, vel, file, mergeAsRr, technique);
                            complete (juce::var (true));
                        });
                }
        },

        { "getSkippedFiles",
                [this] (const juce::Array<juce::var>&,
                        std::function<void(juce::var)> complete)
                {
                    const auto& sk = processorRef.getLastSkippedFiles();
                    auto json = joinJsonArray (sk, [] (const juce::String& s)
                    {
                        return juce::JSON::toString (juce::var (s));
                    });
                    complete (juce::var (json));
                }
        },

        // ---- overrideLoopPoints (Phase 3.4 — full impl) ----
        //
        // JS calls: await Juce.getNativeFunction('overrideLoopPoints')
        //              (midi, vel, loopStart, loopEnd, crossfadeLen).
        // Routes to processorRef.overrideLoopPoints(...). The
        // sampleMapUpdated push event fires automatically via the
        // processor's atomic-store + sampleMapChangedCallback.
        // Returns true on dispatch (not on audible application — that
        // happens on the next note-on per EC3-6).
        { "overrideLoopPoints",
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
                                             : 8;
                    const int variantIdx = (args.size() >= 6)
                                              ? static_cast<int> (args[5])
                                              : -1;   // v1.8.0: -1 = primary
                    // v1.14.0: optional technique slot (default = current
                    // active technique).
                    const int technique = (args.size() >= 7)
                        ? juce::jlimit (0, 7, static_cast<int> (args[6]))
                        : processorRef.getActiveTechnique();

                    processorRef.overrideLoopPoints (midi, vel, loopStart, loopEnd,
                                                     xfade, /*resetToAutoDetect*/ false,
                                                     variantIdx, technique);
                    complete (juce::var (true));
                }
        },

        // ---- resetLoopToAutoDetect (Phase 3.4 — full impl) ----
        //
        // JS calls: await Juce.getNativeFunction('resetLoopToAutoDetect')(midi, vel).
        // Routes to processorRef.resetLoopToAutoDetect(...). Push update
        // arrives via sampleMapUpdated.
        { "resetLoopToAutoDetect",
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
                    const int variantIdx = (args.size() >= 3)
                                              ? static_cast<int> (args[2])
                                              : -1;   // v1.8.0: -1 = primary
                    // v1.14.0: optional technique slot.
                    const int technique = (args.size() >= 4)
                        ? juce::jlimit (0, 7, static_cast<int> (args[3]))
                        : processorRef.getActiveTechnique();

                    processorRef.resetLoopToAutoDetect (midi, vel, variantIdx, technique);
                    complete (juce::var (true));
                }
        },

        // ---- getWaveformPeaks (Phase 3.4 — full impl) ----
        //
        // JS calls: await Juce.getNativeFunction('getWaveformPeaks')(midi, vel, bins).
        // Returns the JSON snapshot from snapshotWaveformPeaks per
        // RESEARCH §RQ3-5 schema. Click-driven path (loop-editor open),
        // so message-thread O(N) scan is acceptable (≈1 ms / 5 s sample).
        { "getWaveformPeaks",
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
                    const int variantIdx = (args.size() >= 4)
                                              ? static_cast<int> (args[3])
                                              : 0;   // v1.8.0: default to primary
                    // v1.14.0: optional technique slot.
                    const int technique = (args.size() >= 5)
                        ? juce::jlimit (0, 7, static_cast<int> (args[4]))
                        : processorRef.getActiveTechnique();

                    complete (juce::var (
                        processorRef.snapshotWaveformPeaks (midi, vel, bins, variantIdx, technique)));
                }
        },

        // ---- v1.8.0: confirmRoundRobinLoad(accept) ----
        //
        // JS calls await Juce.getNativeFunction('confirmRoundRobinLoad')(true)
        // when the user accepts ambiguous duplicates as RR variants, or
        // (false) to discard the staged map.
        { "confirmRoundRobinLoad",
                [this] (const juce::Array<juce::var>& args,
                        std::function<void(juce::var)> complete)
                {
                    if (args.size() < 1)
                    {
                        complete (juce::var (false));
                        return;
                    }
                    const bool accept = static_cast<bool> (args[0]);
                    processorRef.confirmRoundRobinLoad (accept);
                    complete (juce::var (true));
                }
        },

        // ============================================================
        // v1.2.0: TUNING WRITE-SIDE BRIDGES
        // The Stage 3 read-only design (§RQ3-1) is reversed in v1.2.0:
        // the panel is now editable. All write-side native functions
        // forward to the shared scala-tuning-engine module, the same
        // single-source-of-truth that VST3 Note Expression overrides
        // at note-on time, so Dorico microtonal playback is preserved.
        // ============================================================

        // ---- setSingleInterval(index, cents) ----
        { "setSingleInterval",
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
                }
        },

        // ---- setTonicNote(0..11) ----
        { "setTonicNote",
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
                }
        },

        // ---- setOctaveStretch(stretch) ----
        { "setOctaveStretch",
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
                }
        },

        // ---- setMasterTune(hz) ----
        { "setMasterTune",
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
                }
        },

        // ---- loadEmbeddedTuning(id) — apply a factory preset by ID ----
        { "loadEmbeddedTuning",
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
                }
        },

        // ---- loadScalaFile() — open .scl file picker, return scale name on success ----
        { "loadScalaFile",
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
                }
        },

        // ---- loadKBMFile() — open .kbm file picker ----
        { "loadKBMFile",
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
                }
        },

        // ---- saveScalaFile() — write current intervals as .scl ----
        { "saveScalaFile",
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
                }
        },

        // ---- saveKBMFile() — write current keyboard mapping as .kbm ----
        { "saveKBMFile",
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
                }
        },

        // ---- generateEDO(divisions, period) → JSON intervals ----
        { "generateEDO",
                [] (const juce::Array<juce::var>& args,
                    std::function<void(juce::var)> complete)
                {
                    if (args.size() >= 2)
                    {
                        complete (juce::var (centsArrayToJson (
                            ScaleGenerator::generateEDO (
                                static_cast<int>    (args[0]),
                                static_cast<double> (args[1])))));
                        return;
                    }
                    complete (juce::var());
                }
        },

        // ---- generateHarmonicSeries(start, end) → JSON intervals ----
        { "generateHarmonicSeries",
                [] (const juce::Array<juce::var>& args,
                    std::function<void(juce::var)> complete)
                {
                    if (args.size() >= 2)
                    {
                        complete (juce::var (centsArrayToJson (
                            ScaleGenerator::generateHarmonicSeries (
                                static_cast<int> (args[0]),
                                static_cast<int> (args[1])))));
                        return;
                    }
                    complete (juce::var());
                }
        },

        // ---- generateRank2(generator, period, count) → JSON intervals ----
        { "generateRank2",
                [] (const juce::Array<juce::var>& args,
                    std::function<void(juce::var)> complete)
                {
                    if (args.size() >= 3)
                    {
                        complete (juce::var (centsArrayToJson (
                            ScaleGenerator::generateRank2 (
                                static_cast<double> (args[0]),
                                static_cast<double> (args[1]),
                                static_cast<int>    (args[2])))));
                        return;
                    }
                    complete (juce::var());
                }
        },

        // ---- applyGeneratedScale(intervalsJson, name) ----
        { "applyGeneratedScale",
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
                }
        },

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
        { "saveCurrentPreset",
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
                }
        },

        // ---- loadPreset() — read .omspreset and restore state ----
        //
        // Replaces APVTS, tuning, and the loaded folder. If the saved
        // folder path no longer exists, surfaces the standard missing-
        // folder modal (same pathway used during DAW project reopen).
        { "loadPreset",
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
                }
        },

        // ---- locateMissingFolder() — folder picker for missing-folder modal ----
        //
        // JS surfaces this from the modal's "Locate folder…" button.
        // Reuses the existing loadSampleFolder pathway, which clears the
        // pending missing-folder slot via setStateInformation's normal
        // success flow. JS resolves true if a folder was selected (and
        // forwarded to the processor), false on cancel.
        { "locateMissingFolder",
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
                }
        },

        // ---- dismissMissingFolder() — user chose Skip on the modal ----
        { "dismissMissingFolder",
                [this] (const juce::Array<juce::var>&,
                        std::function<void(juce::var)> complete)
                {
                    processorRef.clearPendingMissingFolder();
                    complete (juce::var (true));
                }
        },

        // ---- getPendingMissingFolder() — covers boot-time race ----
        //
        // setStateInformation may run before the WebView has registered
        // its folderMissing event listener (DAW project reopen → state
        // restore happens before editor attach). JS calls this once on
        // boot to recover any missed event.
        //
        // v1.12.0: returns an object {path, kind, name} — string for
        // legacy v1.11.x JS bundles is no longer compatible. JS must
        // detect the object form and branch on `kind` ("filesystem" or
        // "drag-drop"). Empty path + empty name = no pending missing.
        { "getPendingMissingFolder",
                [this] (const juce::Array<juce::var>&,
                        std::function<void(juce::var)> complete)
                {
                    auto* obj = new juce::DynamicObject();
                    obj->setProperty ("path", processorRef.getPendingMissingFolderPath());
                    obj->setProperty ("kind", processorRef.getPendingMissingFolderKind());
                    obj->setProperty ("name", processorRef.getPendingMissingFolderName());
                    complete (juce::var (obj));
                }
        },

        // ---- exportTuningHTML() — write current tuning to HTML doc ----
        { "exportTuningHTML",
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
                }
        },

        // ============================================================
        // v1.14.0 — Playing Techniques bridge (engine + KS slice)
        // ============================================================
        //
        // The technique vocabulary lives in PluginProcessor (string list +
        // atomic active-cursor + APVTS-mirrored params). These natives
        // surface read/write access to the WebView UI: tab-strip render,
        // tab-click selection, slot rename, and KS toggle / range edits.
        //
        // The KS toggle / range numbers are APVTS params (ks_enabled,
        // ks_low_note, ks_high_note) so they round-trip through the
        // standard parameter machinery — JS reads them via the existing
        // Juce.getSliderState path; only writes go through native fns
        // because we want the technique callback to fire for UI repaints.

        // ---- getTechniqueState() — current vocab + active cursor ----
        // Returns: { names: ["ord","sp",…], active: <int>,
        //            ksEnabled: bool, ksLow: int, ksHigh: int,
        //            count: int }
        { "getTechniqueState",
                [this] (const juce::Array<juce::var>&,
                        std::function<void(juce::var)> complete)
                {
                    auto* obj = new juce::DynamicObject();
                    juce::var arr (juce::Array<juce::var>{});
                    auto* a = arr.getArray();
                    for (const auto& n : processorRef.getTechniqueNames())
                        a->add (juce::var (n));
                    obj->setProperty ("names",  arr);
                    obj->setProperty ("active", processorRef.getActiveTechnique());

                    auto& apvts = processorRef.getAPVTS();
                    if (auto* p = apvts.getRawParameterValue ("technique_count"))
                        obj->setProperty ("count", (int) p->load());
                    if (auto* p = apvts.getRawParameterValue ("ks_enabled"))
                        obj->setProperty ("ksEnabled", (p->load() > 0.5f));
                    if (auto* p = apvts.getRawParameterValue ("ks_low_note"))
                        obj->setProperty ("ksLow", (int) p->load());
                    if (auto* p = apvts.getRawParameterValue ("ks_high_note"))
                        obj->setProperty ("ksHigh", (int) p->load());

                    complete (juce::var (obj));
                }
        },

        // ---- setActiveTechnique(index) ----
        { "setActiveTechnique",
                [this] (const juce::Array<juce::var>& args,
                        std::function<void(juce::var)> complete)
                {
                    if (args.size() < 1) { complete (juce::var (false)); return; }
                    processorRef.setActiveTechnique (static_cast<int> (args[0]));
                    complete (juce::var (true));
                }
        },

        // ---- setTechniqueName(index, name) ----
        { "setTechniqueName",
                [this] (const juce::Array<juce::var>& args,
                        std::function<void(juce::var)> complete)
                {
                    if (args.size() < 2) { complete (juce::var (false)); return; }
                    processorRef.setTechniqueName (static_cast<int>    (args[0]),
                                                    args[1].toString());
                    complete (juce::var (true));
                }
        },

        // ---- resetTechniqueNames() — reset all 8 to default vocabulary ----
        { "resetTechniqueNames",
                [this] (const juce::Array<juce::var>&,
                        std::function<void(juce::var)> complete)
                {
                    processorRef.resetTechniqueNames();
                    complete (juce::var (true));
                }
        },

        // ---- addTechniqueSlot() — increase technique_count by 1 (cap 8) ----
        // Routes through the APVTS so the change persists with project state.
        { "addTechniqueSlot",
                [this] (const juce::Array<juce::var>&,
                        std::function<void(juce::var)> complete)
                {
                    auto& apvts = processorRef.getAPVTS();
                    if (auto* cp = apvts.getParameter ("technique_count"))
                    {
                        const auto range = apvts.getParameterRange ("technique_count");
                        const int  cur   = juce::jlimit (1, 8,
                            (int) apvts.getRawParameterValue ("technique_count")->load());
                        const int  next  = juce::jmin (8, cur + 1);
                        cp->setValueNotifyingHost (range.convertTo0to1 ((float) next));
                        complete (juce::var (true));
                        return;
                    }
                    complete (juce::var (false));
                }
        },

        // ---- removeTechniqueSlot(index) ----
        // Lowers technique_count by 1 (floor 1). Cells in the dropped slot
        // are NOT erased — the cell's variants survive in the SampleMap
        // and the user can recover them by raising the count back. Active
        // technique is clamped to the new max if necessary.
        { "removeTechniqueSlot",
                [this] (const juce::Array<juce::var>&,
                        std::function<void(juce::var)> complete)
                {
                    auto& apvts = processorRef.getAPVTS();
                    if (auto* cp = apvts.getParameter ("technique_count"))
                    {
                        const auto range = apvts.getParameterRange ("technique_count");
                        const int  cur   = juce::jlimit (1, 8,
                            (int) apvts.getRawParameterValue ("technique_count")->load());
                        const int  next  = juce::jmax (1, cur - 1);
                        cp->setValueNotifyingHost (range.convertTo0to1 ((float) next));
                        if (processorRef.getActiveTechnique() >= next)
                            processorRef.setActiveTechnique (next - 1);
                        complete (juce::var (true));
                        return;
                    }
                    complete (juce::var (false));
                }
        },

        // ---- setKeyswitchEnabled(bool) / setKeyswitchRange(low, high) ----
        // Both wrap the APVTS params so changes survive project save. The
        // first one also re-emits techniqueStateUpdated so the UI panel
        // reflects the toggle without a separate poll.
        { "setKeyswitchEnabled",
                [this] (const juce::Array<juce::var>& args,
                        std::function<void(juce::var)> complete)
                {
                    complete (juce::var (setBoolParamFromArgs (
                        processorRef.getAPVTS(), "ks_enabled", args)));
                }
        },

        { "setKeyswitchRange",
                [this] (const juce::Array<juce::var>& args,
                        std::function<void(juce::var)> complete)
                {
                    if (args.size() < 2) { complete (juce::var (false)); return; }
                    auto& apvts = processorRef.getAPVTS();
                    auto setIntParam = [&apvts] (const char* id, int v)
                    {
                        if (auto* p = apvts.getParameter (id))
                        {
                            const auto range = apvts.getParameterRange (id);
                            p->setValueNotifyingHost (range.convertTo0to1 ((float) v));
                        }
                    };
                    setIntParam ("ks_low_note",  juce::jlimit (0, 127, static_cast<int> (args[0])));
                    setIntParam ("ks_high_note", juce::jlimit (0, 127, static_cast<int> (args[1])));
                    complete (juce::var (true));
                }
        },

        // ====================================================================
        // v1.15.0 — CC + PC trigger native functions
        // ====================================================================
        //
        // The 8-slot CC + PC mapping tables live outside APVTS (the host UI
        // wouldn't usefully render 24 numerical slots). Reads use one
        // bulk-snapshot fn (getTriggerState); writes are per-slot mutators.
        // The cc_select_enabled / cc_number / pc_enabled GATES are normal
        // APVTS params so the JS layer can use Juce.getSliderState for them
        // (matches the technique_count / ks_enabled pattern).

        // ---- getTriggerState() — bulk snapshot of CC + PC tables + gates.
        // Returns:
        //   {
        //     ccEnabled: bool, ccNumber: int,
        //     ccMapping: [{rangeLow, rangeHigh, tech}, …8],
        //     pcEnabled: bool,
        //     pcMapping: [{pc, tech}, …8]
        //   }
        { "getTriggerState",
                [this] (const juce::Array<juce::var>&,
                        std::function<void(juce::var)> complete)
                {
                    auto* obj = new juce::DynamicObject();
                    auto& apvts = processorRef.getAPVTS();

                    if (auto* p = apvts.getRawParameterValue ("cc_select_enabled"))
                        obj->setProperty ("ccEnabled", (p->load() > 0.5f));
                    if (auto* p = apvts.getRawParameterValue ("cc_number"))
                        obj->setProperty ("ccNumber",  (int) p->load());
                    if (auto* p = apvts.getRawParameterValue ("pc_enabled"))
                        obj->setProperty ("pcEnabled", (p->load() > 0.5f));

                    {
                        const auto cc = processorRef.getCcMapping();
                        juce::var arr (juce::Array<juce::var>{});
                        auto* a = arr.getArray();
                        for (const auto& s : cc)
                        {
                            auto* slotObj = new juce::DynamicObject();
                            slotObj->setProperty ("rangeLow",  s.rangeLow);
                            slotObj->setProperty ("rangeHigh", s.rangeHigh);
                            slotObj->setProperty ("tech",      s.technique);
                            a->add (juce::var (slotObj));
                        }
                        obj->setProperty ("ccMapping", arr);
                    }
                    {
                        const auto pc = processorRef.getPcMapping();
                        juce::var arr (juce::Array<juce::var>{});
                        auto* a = arr.getArray();
                        for (const auto& s : pc)
                        {
                            auto* slotObj = new juce::DynamicObject();
                            slotObj->setProperty ("pc",   s.pc);
                            slotObj->setProperty ("tech", s.technique);
                            a->add (juce::var (slotObj));
                        }
                        obj->setProperty ("pcMapping", arr);
                    }

                    complete (juce::var (obj));
                }
        },

        // ---- setCcEnabled(bool) — gate the CC trigger scan.
        { "setCcEnabled",
                [this] (const juce::Array<juce::var>& args,
                        std::function<void(juce::var)> complete)
                {
                    complete (juce::var (setBoolParamFromArgs (
                        processorRef.getAPVTS(), "cc_select_enabled", args)));
                }
        },

        // ---- setCcNumber(int) — controller number to listen on (0..119).
        { "setCcNumber",
                [this] (const juce::Array<juce::var>& args,
                        std::function<void(juce::var)> complete)
                {
                    if (args.size() < 1) { complete (juce::var (false)); return; }
                    auto& apvts = processorRef.getAPVTS();
                    if (auto* p = apvts.getParameter ("cc_number"))
                    {
                        const auto range = apvts.getParameterRange ("cc_number");
                        const int v = juce::jlimit (0, 119, static_cast<int> (args[0]));
                        p->setValueNotifyingHost (range.convertTo0to1 ((float) v));
                        complete (juce::var (true));
                        return;
                    }
                    complete (juce::var (false));
                }
        },

        // ---- setCcMapping(slot, rangeLow, rangeHigh, tech)
        { "setCcMapping",
                [this] (const juce::Array<juce::var>& args,
                        std::function<void(juce::var)> complete)
                {
                    if (args.size() < 4) { complete (juce::var (false)); return; }
                    processorRef.setCcMappingSlot (
                        static_cast<int> (args[0]),
                        static_cast<int> (args[1]),
                        static_cast<int> (args[2]),
                        static_cast<int> (args[3]));
                    complete (juce::var (true));
                }
        },

        // ---- setPcEnabled(bool) — gate the PC trigger scan.
        { "setPcEnabled",
                [this] (const juce::Array<juce::var>& args,
                        std::function<void(juce::var)> complete)
                {
                    complete (juce::var (setBoolParamFromArgs (
                        processorRef.getAPVTS(), "pc_enabled", args)));
                }
        },

        // ---- setPcMapping(slot, pcNumber, tech)
        { "setPcMapping",
                [this] (const juce::Array<juce::var>& args,
                        std::function<void(juce::var)> complete)
                {
                    if (args.size() < 3) { complete (juce::var (false)); return; }
                    processorRef.setPcMappingSlot (
                        static_cast<int> (args[0]),
                        static_cast<int> (args[1]),
                        static_cast<int> (args[2]));
                    complete (juce::var (true));
                }
        },

        // ---- resetTriggerMappings() — restore CC + PC defaults.
        { "resetTriggerMappings",
                [this] (const juce::Array<juce::var>&,
                        std::function<void(juce::var)> complete)
                {
                    processorRef.resetTriggerMappings();
                    complete (juce::var (true));
                }
        },
    };

    // v1.13.0 (ARCH-02): splice the 4 dropSession* native function handlers
    // supplied by the shared modules/core/webview-drop-streaming module's
    // SessionManager. These used to be 4 inline registry entries (~290 lines)
    // owned by this editor; they now live in the module and bridge to
    // processorRef.loadSampleFolder / loadSingleSample via the commit
    // callbacks we set up at SessionManager construction.
    for (auto& entry : dropSessions->getNativeFunctions())
        registry.push_back (std::move (entry));

    return registry;
}

#if OUARICON_LICENSING_ENABLED
//==============================================================================
void OMicrotonalSamplerAudioProcessorEditor::licenseStatusChanged(
    OuariconLicense&, OuariconLicense::Status newStatus)
{
    juce::MessageManager::callAsync([this, newStatus]()
    {
        bool licensed = (newStatus == OuariconLicense::Status::Licensed);
        webView->setVisible(licensed);

        if (licenseOverlay)
            licenseOverlay->setVisible(! licensed);
    });
}
#endif
