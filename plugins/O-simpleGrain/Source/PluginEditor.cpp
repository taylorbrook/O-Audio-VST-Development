/*
  ==============================================================================

    O-simpleGrain - Plugin Editor (implementation)

    Stage 3 (GUI): Ouaricon "Granular Field Guide" WebView UI. 15 slider relays
    (grain / spray-scatter / amp ADSR / output) + 2 combo relays (sourceSample,
    windowShape) + 1 toggle relay (freeze) bound two-way to the APVTS. The five
    fixed-name source native functions (dropSessionStart/AddFile/CommitFile/
    CommitFolder, loadSourceFromFileChooser) bridge the JS drag-drop / picker to
    the processor's already-built decode path.

    Phase 3.1: layout + 18-param binding + load-your-own wiring. The 30 Hz Timer
    body (four visualizations + readout) lands in Phase 3.2 — it is an empty stub
    here.

  ==============================================================================
*/

#include "PluginEditor.h"
#include "UIBinaryData.h"   // UI resources (distinct from the samples' BinaryData.h)

// ── Resource provider ───────────────────────────────────────────────────────
// The WebView2/WKWebView resource callback receives a BARE PATH ("/", "/index.html",
// "/js/app.js", ...). Compare by direct string equality — never strip a scheme/host
// (a bare path has none, which would collapse every lookup to an empty string).
namespace
{
    auto makeBinaryResource (const char* data, int size, const char* mimeType)
        -> std::optional<juce::WebBrowserComponent::Resource>
    {
        auto* bytes = reinterpret_cast<const std::byte*> (data);
        return juce::WebBrowserComponent::Resource {
            std::vector<std::byte> (bytes, bytes + size),
            juce::String (mimeType)
        };
    }
}

std::optional<juce::WebBrowserComponent::Resource>
OSimpleGrainAudioProcessorEditor::getResource (const juce::String& url)
{
    // charset=utf-8 on text resources — the page relies on UTF-8 entities
    // (fleurons, en-dashes, arrows); a missing charset can mojibake them on some hosts.
    if (url == "/" || url == "/index.html")
        return makeBinaryResource (UIBinaryData::index_html, UIBinaryData::index_htmlSize, "text/html; charset=utf-8");

    if (url == "/css/styles.css")
        return makeBinaryResource (UIBinaryData::styles_css, UIBinaryData::styles_cssSize, "text/css; charset=utf-8");

    if (url == "/js/app.js")
        return makeBinaryResource (UIBinaryData::app_js, UIBinaryData::app_jsSize, "application/javascript; charset=utf-8");

    if (url == "/js/juce/index.js")
        return makeBinaryResource (UIBinaryData::index_js, UIBinaryData::index_jsSize, "application/javascript; charset=utf-8");

    if (url == "/js/juce/check_native_interop.js")
        return makeBinaryResource (UIBinaryData::check_native_interop_js,
                                   UIBinaryData::check_native_interop_jsSize, "application/javascript; charset=utf-8");

    // Shared drag-drop streaming module (base64 primitives) — imported by app.js
    // as ./modules/webview-drop-streaming.js (relative to /js/app.js).
    if (url == "/js/modules/webview-drop-streaming.js")
        return makeBinaryResource (UIBinaryData::webviewdropstreaming_js,
                                   UIBinaryData::webviewdropstreaming_jsSize, "application/javascript; charset=utf-8");

    if (url == "/img/insects.png")
        return makeBinaryResource (UIBinaryData::insects_png, UIBinaryData::insects_pngSize, "image/png");

    return std::nullopt;
}

// ── Construction ────────────────────────────────────────────────────────────
OSimpleGrainAudioProcessorEditor::OSimpleGrainAudioProcessorEditor (OSimpleGrainAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processorRef (p)
{
    using namespace OSimpleGrain::ParamIDs;

    // 18 APVTS params = 15 float sliders + 2 combos + 1 toggle.
    // (sourceSample / windowShape are choices; freeze is the bool — NOT sliders.)
    const juce::StringArray sliderIds {
        grainSize, density, position, scan,
        pitchSpray, positionSpray, scatter, grainPitch, panSpray, velToDensity,
        ampAttack, ampDecay, ampSustain, ampRelease,
        outputLevel
    };
    const juce::StringArray comboIds  { sourceSample, windowShape };
    const juce::StringArray toggleIds { freeze };

    // 1. RELAYS (before the WebView) ----------------------------------------
    for (const auto& id : sliderIds)
        sliderRelays.push_back (std::make_unique<juce::WebSliderRelay> (id));
    for (const auto& id : comboIds)
        comboRelays.push_back (std::make_unique<juce::WebComboBoxRelay> (id));
    for (const auto& id : toggleIds)
        toggleRelays.push_back (std::make_unique<juce::WebToggleButtonRelay> (id));

    // 2. WEBVIEW options + relay registration -------------------------------
    auto options = juce::WebBrowserComponent::Options{}
        .withNativeIntegrationEnabled()
        .withKeepPageLoadedWhenBrowserIsHidden()
        .withResourceProvider ([this] (const auto& url) { return getResource (url); });

    for (const auto& relay : sliderRelays)
        options = options.withOptionsFrom (*relay);
    for (const auto& relay : comboRelays)
        options = options.withOptionsFrom (*relay);
    for (const auto& relay : toggleRelays)
        options = options.withOptionsFrom (*relay);

    // ── Native functions ───────────────────────────────────────────────────
    // Source drag-drop + file-picker. The NAMES ARE FIXED by the shared module
    // and the processor's C++ handlers — do NOT rename. Each forwards to the
    // matching processor method (which decodes/resamples/publishes off the audio
    // thread) and completes with the returned bool ("ok"); the JS toasts on false.
    options = options
        .withNativeFunction ("dropSessionStart", [this] (const juce::Array<juce::var>& args, auto complete) {
            const juce::String sessionId  = args.size() > 0 ? args[0].toString() : juce::String();
            const juce::String folderName = args.size() > 1 ? args[1].toString() : juce::String();
            complete (juce::var (processorRef.dropSessionStart (sessionId, folderName)));
        })
        .withNativeFunction ("dropSessionAddFile", [this] (const juce::Array<juce::var>& args, auto complete) {
            if (args.size() >= 3)
                complete (juce::var (processorRef.dropSessionAddFile (
                    args[0].toString(), args[1].toString(), args[2].toString())));
            else
                complete (juce::var (false));
        })
        .withNativeFunction ("dropSessionCommitFile", [this] (const juce::Array<juce::var>& args, auto complete) {
            // grain signature: (sessionId, filename, base64) — NOT (sessionId, name, midi, vel).
            if (args.size() >= 3)
                complete (juce::var (processorRef.dropSessionCommitFile (
                    args[0].toString(), args[1].toString(), args[2].toString())));
            else
                complete (juce::var (false));
        })
        .withNativeFunction ("dropSessionCommitFolder", [this] (const juce::Array<juce::var>& args, auto complete) {
            const juce::String sessionId = args.size() > 0 ? args[0].toString() : juce::String();
            complete (juce::var (processorRef.dropSessionCommitFolder (sessionId)));
        })
        .withNativeFunction ("loadSourceFromFileChooser", [this] (auto&, auto complete) {
            processorRef.loadSourceFromFileChooser();   // async; opens its own FileChooser
            complete (juce::var (true));
        })
        // Whether the last load was truncated to the 10 s source cap (UI notice).
        .withNativeFunction ("wasLastLoadTruncated", [this] (auto&, auto complete) {
            complete (juce::var (processorRef.wasLastLoadTruncated()));
        })
        // Sample rate for the spectrum's frequency-axis labels (used in 3.2).
        .withNativeFunction ("getSampleRate", [this] (auto&, auto complete) {
            complete (processorRef.getCurrentSampleRate());
        })
        // Source min/max envelope for the UI-02 waveform background. The JS fetches
        // it on load + at boot (not per frame); a flat [min,max,…] var array in
        // [-1,1] (~512 pairs). Read-only message-thread snapshot — audio untouched.
        .withNativeFunction ("getSourceThumbnail", [this] (const juce::Array<juce::var>& args, auto complete) {
            const int numPairs = (args.size() > 0 && (int) args[0] > 0) ? (int) args[0] : 512;
            const auto env = processorRef.getSourceThumbnail (numPairs);
            juce::Array<juce::var> arr;
            arr.ensureStorageAllocated ((int) env.size());
            for (float v : env) arr.add (v);
            complete (juce::var (std::move (arr)));
        });

   #if JUCE_WINDOWS
    // WebView2 user-data folder MUST be a writable temp dir (DAW hosts deny the
    // default location → silent IE fallback → blank page). Unique prefix so the
    // stale-session reaper can't touch another plugin's in-flight sessions.
    options = options.withWinWebView2Options (
        juce::WebBrowserComponent::Options::WinWebView2{}
            .withUserDataFolder (
                juce::File::getSpecialLocation (juce::File::tempDirectory)
                    .getChildFile ("OsimpleGrain_WebView"))
            .withStatusBarDisabled()
            .withBuiltInErrorPageDisabled());
   #endif

    webView = std::make_unique<juce::WebBrowserComponent> (options);

    // 3. ATTACHMENTS (after the WebView; 3-arg ctor + nullptr undoManager) ----
    for (int i = 0; i < sliderIds.size(); ++i)
    {
        auto* param = processorRef.getAPVTS().getParameter (sliderIds[i]);
        jassert (param != nullptr);   // ID drift → silently dead control; catch in debug
        if (param != nullptr)
            sliderAttachments.push_back (
                std::make_unique<juce::WebSliderParameterAttachment> (
                    *param, *sliderRelays[(size_t) i], nullptr));
    }

    for (int i = 0; i < comboIds.size(); ++i)
    {
        auto* param = processorRef.getAPVTS().getParameter (comboIds[i]);
        jassert (param != nullptr);
        if (param != nullptr)
            comboAttachments.push_back (
                std::make_unique<juce::WebComboBoxParameterAttachment> (
                    *param, *comboRelays[(size_t) i], nullptr));
    }

    for (int i = 0; i < toggleIds.size(); ++i)
    {
        auto* param = processorRef.getAPVTS().getParameter (toggleIds[i]);
        jassert (param != nullptr);
        if (param != nullptr)
            toggleAttachments.push_back (
                std::make_unique<juce::WebToggleButtonParameterAttachment> (
                    *param, *toggleRelays[(size_t) i], nullptr));
    }

    addAndMakeVisible (*webView);
    webView->goToURL (juce::WebBrowserComponent::getResourceProviderRoot());

    setSize (900, 760);   // 2×2 grid + side rail + keyboard; the frame scrolls on shorter screens
    startTimerHz (30);
}

OSimpleGrainAudioProcessorEditor::~OSimpleGrainAudioProcessorEditor()
{
    stopTimer();
}

// ── Timer (30 Hz): run the analyzer + push the four viz events ───────────────
// Everything here runs on the MESSAGE thread. The audio thread only copies into
// the VizRing / fills the GrainCloudFrame (lock-free); the FFT + scope downsample
// happen inside vizAnalyzer.process below (PERF-01 / invariant 7). The analyzer
// copies the scope window BEFORE its in-place FFT (invariant 8 — VizAnalyzer.h:88).
void OSimpleGrainAudioProcessorEditor::timerCallback()
{
    vizAnalyzer.process (processorRef.getVizRing(), processorRef.getCurrentSampleRate());

    if (webView == nullptr)
        return;

    // 1. Scope + spectrum (UI-04) — verbatim FM pattern (256 dB bins / 128 pts).
    {
        const auto& spec  = vizAnalyzer.getSpectrum();   // 256 bins, ~[-100, 0] dB
        const auto& scope = vizAnalyzer.getScope();       // 128 pts, [-1, 1]

        juce::Array<juce::var> specArr, scopeArr;
        specArr.ensureStorageAllocated ((int) spec.size());
        scopeArr.ensureStorageAllocated ((int) scope.size());
        for (float v : spec)  specArr.add (v);
        for (float v : scope) scopeArr.add (v);

        webView->emitEventIfBrowserIsVisible ("spectrumUpdate", juce::var (std::move (specArr)));
        webView->emitEventIfBrowserIsVisible ("scopeUpdate",    juce::var (std::move (scopeArr)));
    }

    // 2. Grain cloud + playheads (UI-01/02) — read the published frame, build a
    //    DynamicObject { count, grains:[[readPosNorm,sizeMs,pitchSemis,pan,spawn],…],
    //    playheadNorm, positionNorm, positionSprayNorm, frozen }.
    {
        const GrainCloudFrame& f = processorRef.getGrainCloudBuffer().read();

        juce::Array<juce::var> grains;
        grains.ensureStorageAllocated (f.count);
        for (int i = 0; i < f.count; ++i)
        {
            const GrainEvent& g = f.events[(size_t) i];
            juce::Array<juce::var> tuple;
            tuple.ensureStorageAllocated (5);
            tuple.add (g.readPosNorm);
            tuple.add (g.sizeMs);
            tuple.add (g.pitchSemis);
            tuple.add (g.pan);
            tuple.add (g.spawnSample);
            grains.add (juce::var (std::move (tuple)));
        }

        auto* cloud = new juce::DynamicObject();
        cloud->setProperty ("count",             f.count);
        cloud->setProperty ("grains",            juce::var (std::move (grains)));
        cloud->setProperty ("playheadNorm",      f.playheadNorm);
        cloud->setProperty ("positionNorm",      f.positionNorm);
        cloud->setProperty ("positionSprayNorm", f.positionSprayNorm);
        cloud->setProperty ("frozen",            f.frozen);

        webView->emitEventIfBrowserIsVisible ("grainCloudUpdate", juce::var (cloud));
    }

    // 3. Grain meter (UI-05) — active grain count over kGlobalGrainCap (192).
    webView->emitEventIfBrowserIsVisible ("grainMeterUpdate",
                                          juce::var (processorRef.getActiveGrainCount()));

    // NB: windowInsetUpdate is NOT pushed here — the window only changes on the
    // windowShape combo, so the inset is recomputed JS-side on that change
    // (invariant: no per-frame inset push — Pitfall 4).
}

// ── Layout ──────────────────────────────────────────────────────────────────
void OSimpleGrainAudioProcessorEditor::paint (juce::Graphics&)
{
    // WebView fills the editor — nothing to paint.
}

void OSimpleGrainAudioProcessorEditor::resized()
{
    if (webView != nullptr)
        webView->setBounds (getLocalBounds());
}
