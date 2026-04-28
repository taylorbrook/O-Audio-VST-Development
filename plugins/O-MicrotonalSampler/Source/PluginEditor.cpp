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

            .withNativeFunction ("loadSampleFolderDialog",
                [] (const juce::Array<juce::var>&,
                    std::function<void(juce::var)> complete)
                {
                    DBG ("loadSampleFolderDialog (skeleton — Phase 3.3)");
                    complete (juce::var (false));
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

            .withNativeFunction ("overrideLoopPoints",
                [] (const juce::Array<juce::var>& args,
                    std::function<void(juce::var)> complete)
                {
                    DBG ("overrideLoopPoints (skeleton — Phase 3.4): args="
                         << args.size());
                    juce::ignoreUnused (args);
                    complete (juce::var (false));
                })

            .withNativeFunction ("resetLoopToAutoDetect",
                [] (const juce::Array<juce::var>& args,
                    std::function<void(juce::var)> complete)
                {
                    DBG ("resetLoopToAutoDetect (skeleton — Phase 3.4): args="
                         << args.size());
                    juce::ignoreUnused (args);
                    complete (juce::var (false));
                })

            .withNativeFunction ("getWaveformPeaks",
                [] (const juce::Array<juce::var>& args,
                    std::function<void(juce::var)> complete)
                {
                    DBG ("getWaveformPeaks (skeleton — Phase 3.4): args="
                         << args.size());
                    juce::ignoreUnused (args);
                    complete (juce::var (juce::String ("{}")));
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
// FileDragAndDropTarget — Phase 3.1 skeletons. Full routing in Phase 3.3
// per RESEARCH §RQ3-6 (cell-layout shadow + folder-zone hit-test).
bool OMicrotonalSamplerAudioProcessorEditor::isInterestedInFileDrag (
    const juce::StringArray& files)
{
    return ! files.isEmpty();
}

void OMicrotonalSamplerAudioProcessorEditor::filesDropped (
    const juce::StringArray& files, int x, int y)
{
    juce::ignoreUnused (files, x, y);
    // Phase 3.3 implements the full hit-test + dispatch.
}

void OMicrotonalSamplerAudioProcessorEditor::fileDragEnter (
    const juce::StringArray& files, int x, int y)
{
    juce::ignoreUnused (files, x, y);
}

void OMicrotonalSamplerAudioProcessorEditor::fileDragMove (
    const juce::StringArray& files, int x, int y)
{
    juce::ignoreUnused (files, x, y);
}

void OMicrotonalSamplerAudioProcessorEditor::fileDragExit (
    const juce::StringArray& files)
{
    juce::ignoreUnused (files);
}
