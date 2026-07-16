/*
  ==============================================================================

    O-simpleFM - Plugin Editor (implementation)

    Stage 3 (GUI): Ouaricon-Naturalist WebView UI. 15 slider relays + 2 toggle
    relays bound two-way to the APVTS; spectrum + scope pushed at 30 Hz.

  ==============================================================================
*/

#include "PluginEditor.h"
#include "BinaryData.h"

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
OSimpleFMAudioProcessorEditor::getResource (const juce::String& url)
{
    // charset=utf-8 on text resources — the page relies on UTF-8 entities
    // (♪, fleurons, en-dashes); a missing charset can mojibake them on some hosts.
    if (url == "/" || url == "/index.html")
        return makeBinaryResource (BinaryData::index_html, BinaryData::index_htmlSize, "text/html; charset=utf-8");

    if (url == "/css/styles.css")
        return makeBinaryResource (BinaryData::styles_css, BinaryData::styles_cssSize, "text/css; charset=utf-8");

    if (url == "/js/app.js")
        return makeBinaryResource (BinaryData::app_js, BinaryData::app_jsSize, "application/javascript; charset=utf-8");

    if (url == "/js/juce/index.js")
        return makeBinaryResource (BinaryData::index_js, BinaryData::index_jsSize, "application/javascript; charset=utf-8");

    if (url == "/js/juce/check_native_interop.js")
        return makeBinaryResource (BinaryData::check_native_interop_js,
                                   BinaryData::check_native_interop_jsSize, "application/javascript; charset=utf-8");

    if (url == "/modules/preset-manager.js")
        return makeBinaryResource (BinaryData::presetmanager_js,
                                   BinaryData::presetmanager_jsSize, "application/javascript; charset=utf-8");

    if (url == "/img/insects.png")
        return makeBinaryResource (BinaryData::insects_png, BinaryData::insects_pngSize, "image/png");

    return std::nullopt;
}

// ── Construction ────────────────────────────────────────────────────────────
OSimpleFMAudioProcessorEditor::OSimpleFMAudioProcessorEditor (OSimpleFMAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processorRef (p)
{
    using namespace OSimpleFM::ParamIDs;

    // 15 float knobs + 2 toggles. Order is irrelevant across relays as long as
    // every relay is registered with the WebView before construction.
    const juce::StringArray sliderIds {
        ratio, modIndex, feedback, modFixedHz, modEnvToIndex, velToIndex,
        modAttack, modDecay, modSustain, modRelease,
        ampAttack, ampDecay, ampSustain, ampRelease, outputLevel
    };
    const juce::StringArray toggleIds { ratioSnap, modFixedMode };

    // 1. RELAYS (before the WebView) ----------------------------------------
    for (const auto& id : sliderIds)
        sliderRelays.push_back (std::make_unique<juce::WebSliderRelay> (id));
    for (const auto& id : toggleIds)
        toggleRelays.push_back (std::make_unique<juce::WebToggleButtonRelay> (id));

    // 2. WEBVIEW options + relay registration -------------------------------
    auto options = juce::WebBrowserComponent::Options{}
        .withNativeIntegrationEnabled()
        .withKeepPageLoadedWhenBrowserIsHidden()
        .withResourceProvider ([this] (const auto& url) { return getResource (url); });

    for (const auto& relay : sliderRelays)
        options = options.withOptionsFrom (*relay);
    for (const auto& relay : toggleRelays)
        options = options.withOptionsFrom (*relay);

    // ── Preset manager native functions (WebView ↔ OuariconPresetManager) ──
    // Loading a preset calls setValueNotifyingHost on each param, which the
    // relays/attachments propagate back to the JS knobs — no extra UI wiring.
    options = options
        .withNativeFunction ("savePreset", [this] (const juce::Array<juce::var>& args, auto complete) {
            if (args.size() > 0)
                complete (processorRef.getPresetManager().savePreset (args[0].toString()));
            else
                complete (false);
        })
        // The async completions below must NOT capture raw `this`: the host can
        // destroy the editor while the native dialog is open. On teardown the
        // WebView Impl that owns `complete` dies with it, so the null-SafePointer
        // path must be a BARE return — invoking complete(false) there is itself
        // a use-after-free.
        .withNativeFunction ("savePresetWithDialog", [this] (auto&, auto complete) {
            fileChooser = std::make_unique<juce::FileChooser> (
                "Save Preset", processorRef.getPresetManager().getUserPresetsDirectory(), "*.json");
            juce::Component::SafePointer<OSimpleFMAudioProcessorEditor> safeThis (this);
            fileChooser->launchAsync (
                juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                [safeThis, complete] (const juce::FileChooser& fc) {
                    if (safeThis == nullptr)
                        return;
                    auto* result = new juce::DynamicObject();
                    auto results = fc.getResults();
                    if (results.isEmpty()) {
                        result->setProperty ("success", false);
                        result->setProperty ("name", juce::String());
                    } else {
                        // Save to the file the user actually chose — savePresetToFile
                        // honors arbitrary directories (not just Presets/User/).
                        auto file = results.getFirst();
                        bool ok = safeThis->processorRef.getPresetManager().savePresetToFile (file);
                        result->setProperty ("success", ok);
                        result->setProperty ("name", ok ? file.getFileNameWithoutExtension() : juce::String());
                    }
                    complete (juce::var (result));
                });
        })
        .withNativeFunction ("loadPreset", [this] (const juce::Array<juce::var>& args, auto complete) {
            if (args.size() > 0)
                complete (processorRef.getPresetManager().loadPreset (args[0].toString()));
            else
                complete (false);
        })
        .withNativeFunction ("loadPresetFromFile", [this] (auto&, auto complete) {
            fileChooser = std::make_unique<juce::FileChooser> (
                "Load Preset", processorRef.getPresetManager().getPresetsDirectory(), "*.json");
            juce::Component::SafePointer<OSimpleFMAudioProcessorEditor> safeThis (this);
            fileChooser->launchAsync (
                juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                [safeThis, complete] (const juce::FileChooser& fc) {
                    if (safeThis == nullptr)
                        return;
                    auto* result = new juce::DynamicObject();
                    auto results = fc.getResults();
                    if (results.isEmpty()) {
                        result->setProperty ("success", false);
                        result->setProperty ("name", juce::String());
                    } else {
                        auto file = results.getFirst();
                        bool ok = safeThis->processorRef.getPresetManager().loadPresetFromFile (file);
                        result->setProperty ("success", ok);
                        result->setProperty ("name", ok ? file.getFileNameWithoutExtension() : juce::String());
                    }
                    complete (juce::var (result));
                });
        })
        .withNativeFunction ("getPresetList", [this] (auto&, auto complete) {
            juce::Array<juce::var> arr;
            for (const auto& name : processorRef.getPresetManager().getPresetList())
                arr.add (name);
            complete (juce::var (arr));
        })
        .withNativeFunction ("getCurrentPreset", [this] (auto&, auto complete) {
            complete (processorRef.getPresetManager().getCurrentPresetName());
        })
        .withNativeFunction ("selectNextPreset", [this] (auto&, auto complete) {
            complete (processorRef.getPresetManager().getNextPreset());
        })
        .withNativeFunction ("selectPreviousPreset", [this] (auto&, auto complete) {
            complete (processorRef.getPresetManager().getPreviousPreset());
        })
        .withNativeFunction ("deletePreset", [this] (const juce::Array<juce::var>& args, auto complete) {
            if (args.size() > 0)
                complete (processorRef.getPresetManager().deletePreset (args[0].toString()));
            else
                complete (false);
        })
        .withNativeFunction ("isFactoryPreset", [this] (const juce::Array<juce::var>& args, auto complete) {
            if (args.size() > 0)
                complete (processorRef.getPresetManager().isFactoryPreset (args[0].toString()));
            else
                complete (false);
        })
        // On-screen keyboard → synth. args: [noteNumber, isNoteOn, velocity?].
        .withNativeFunction ("uiMidi", [this] (const juce::Array<juce::var>& args, auto complete) {
            if (args.size() >= 2)
                processorRef.handleUiMidi ((int) args[0], (bool) args[1],
                                           args.size() >= 3 ? (float) args[2] : 0.8f);
            complete (juce::var());
        })
        // Sample rate for the spectrum's frequency-axis labels (log-Hz mapping).
        .withNativeFunction ("getSampleRate", [this] (auto&, auto complete) {
            complete (processorRef.getCurrentSampleRate());
        })
        // { paramID: normalisedDefault } for every parameter — powers the knobs'
        // double-click-reset (propertiesChanged carries the range but no default).
        .withNativeFunction ("getParameterDefaults", [this] (auto&, auto complete) {
            auto* obj = new juce::DynamicObject();
            for (auto* p : processorRef.getParameters())
                if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
                    obj->setProperty (rp->paramID, rp->getDefaultValue());
            complete (juce::var (obj));
        });

   #if JUCE_WINDOWS
    options = options.withWinWebView2Options (
        juce::WebBrowserComponent::Options::WinWebView2{}
            .withUserDataFolder (
                juce::File::getSpecialLocation (juce::File::tempDirectory)
                    .getChildFile ("OsimpleFM_WebView"))
            .withStatusBarDisabled()
            .withBuiltInErrorPageDisabled());
   #endif

    webView = std::make_unique<juce::WebBrowserComponent> (options);

    // 3. ATTACHMENTS (after the WebView) ------------------------------------
    for (int i = 0; i < sliderIds.size(); ++i)
    {
        auto* param = processorRef.getAPVTS().getParameter (sliderIds[i]);
        jassert (param != nullptr);   // ID drift → silently dead knob; catch in debug
        if (param != nullptr)
            sliderAttachments.push_back (
                std::make_unique<juce::WebSliderParameterAttachment> (
                    *param, *sliderRelays[(size_t) i], nullptr));
    }

    for (int i = 0; i < toggleIds.size(); ++i)
    {
        auto* param = processorRef.getAPVTS().getParameter (toggleIds[i]);
        jassert (param != nullptr);   // ID drift → silently dead toggle; catch in debug
        if (param != nullptr)
            toggleAttachments.push_back (
                std::make_unique<juce::WebToggleButtonParameterAttachment> (
                    *param, *toggleRelays[(size_t) i], nullptr));
    }

    addAndMakeVisible (*webView);
    webView->goToURL (juce::WebBrowserComponent::getResourceProviderRoot());

    setSize (760, 980);   // tall enough to seat the full field guide + keyboard; frame scrolls on shorter screens
    startTimerHz (30);
}

OSimpleFMAudioProcessorEditor::~OSimpleFMAudioProcessorEditor()
{
    stopTimer();
}

// ── Timer: run the analyzer, push spectrum + scope to the page ─────────────
void OSimpleFMAudioProcessorEditor::timerCallback()
{
    // FFT + scope downsample (message thread; audio thread is copy-only).
    vizAnalyzer.process (processorRef.getVizRing(), processorRef.getCurrentSampleRate());

    if (webView == nullptr)
        return;

    // Push the sample rate whenever it changes (host rate switch, or the editor
    // opened before the first prepareToPlay) — the JS log-frequency axis and
    // FM sideband markers must track the live Nyquist, not a boot-time fetch.
    const double sr = processorRef.getCurrentSampleRate();
    if (sr > 0.0 && ! juce::exactlyEqual (sr, lastPushedSampleRate))
    {
        lastPushedSampleRate = sr;
        webView->emitEventIfBrowserIsVisible ("sampleRateUpdate", juce::var (sr));
    }

    const auto& spec  = vizAnalyzer.getSpectrum();   // 256 dB bins, ~[-100, 0]
    const auto& scope = vizAnalyzer.getScope();       // 128 pts, [-1, 1]

    juce::Array<juce::var> specArr, scopeArr;
    specArr.ensureStorageAllocated ((int) spec.size());
    scopeArr.ensureStorageAllocated ((int) scope.size());
    for (float v : spec)  specArr.add (v);
    for (float v : scope) scopeArr.add (v);

    // Carrier Hz FIRST (0 = nothing sounding) so the spectrum frame below draws
    // its FM sideband markers against the current note, not last frame's.
    webView->emitEventIfBrowserIsVisible ("carrierUpdate",  juce::var (processorRef.getCarrierHz()));
    webView->emitEventIfBrowserIsVisible ("spectrumUpdate", juce::var (std::move (specArr)));
    webView->emitEventIfBrowserIsVisible ("scopeUpdate",    juce::var (std::move (scopeArr)));
}

// ── Layout ──────────────────────────────────────────────────────────────────
void OSimpleFMAudioProcessorEditor::paint (juce::Graphics&)
{
    // WebView fills the editor — nothing to paint.
}

void OSimpleFMAudioProcessorEditor::resized()
{
    if (webView != nullptr)
        webView->setBounds (getLocalBounds());
}
