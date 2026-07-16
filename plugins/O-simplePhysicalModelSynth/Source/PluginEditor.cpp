/*
  ==============================================================================

    O-simplePhysicalModelSynth - Plugin Editor (implementation)

    Stage 3 (GUI) — Phase 3.1: Ouaricon-Naturalist WebView UI. 14 slider relays +
    3 combo relays bound two-way to the APVTS; preset-bar shell (10 native fns) +
    on-screen keyboard (uiMidi). Spectrum/scope/diagram land in Phases 3.2/3.3.

  ==============================================================================
*/

#include "PluginEditor.h"
#include "BinaryData.h"

// ── Resource provider ───────────────────────────────────────────────────────
// The platform WebView resource callback receives a BARE PATH ("/", "/index.html",
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
OSimplePhysicalModelSynthAudioProcessorEditor::getResource (const juce::String& url)
{
    // charset=utf-8 on text resources — the page uses UTF-8 entities (♪, fleurons,
    // en-dashes) that can mojibake without an explicit charset on some hosts.
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

    return std::nullopt;
}

// ── Construction ────────────────────────────────────────────────────────────
OSimplePhysicalModelSynthAudioProcessorEditor::OSimplePhysicalModelSynthAudioProcessorEditor (
    OSimplePhysicalModelSynthAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processorRef (p)
{
    using namespace ParamIDs;

    // 14 sliders + 3 combos, 0 toggles. coarseTune is AudioParameterInt but binds as
    // an ordinary WebSliderRelay (it is a RangedAudioParameter; steps by 1).
    const juce::StringArray sliderIds {
        excitationPosition, excitationColor, bowForce,
        inharmonicity, modeBrightness,
        damping, decay, material,
        coarseTune, fineTune,
        ampAttack, ampRelease, velToBrightness, outputLevel
    };
    const juce::StringArray comboIds { excitationType, resonatorType, stringModel };

    // 1. RELAYS (before the WebView) ----------------------------------------
    for (const auto& id : sliderIds)
        sliderRelays.push_back (std::make_unique<juce::WebSliderRelay> (id));
    for (const auto& id : comboIds)
        comboRelays.push_back (std::make_unique<juce::WebComboBoxRelay> (id));

    // 2. WEBVIEW options + relay registration -------------------------------
    auto options = juce::WebBrowserComponent::Options{}
        .withNativeIntegrationEnabled()
        .withKeepPageLoadedWhenBrowserIsHidden()   // prevents a blank page when the host hides the editor
        .withResourceProvider ([this] (const auto& url) { return getResource (url); });

    for (const auto& relay : sliderRelays)
        options = options.withOptionsFrom (*relay);
    for (const auto& relay : comboRelays)
        options = options.withOptionsFrom (*relay);

    // ── Preset manager native functions (WebView ↔ OuariconPresetManager) ──
    // Loading a preset calls setValueNotifyingHost on each param; the relays/
    // attachments propagate that back to the JS controls — no extra UI wiring.
    options = options
        .withNativeFunction ("savePreset", [this] (const juce::Array<juce::var>& args, auto complete) {
            if (args.size() > 0)
                complete (processorRef.getPresetManager().savePreset (args[0].toString()));
            else
                complete (false);
        })
        .withNativeFunction ("savePresetWithDialog", [this] (auto&, auto complete) {
            fileChooser = std::make_unique<juce::FileChooser> (
                "Save Preset", processorRef.getPresetManager().getUserPresetsDirectory(), "*.json");
            fileChooser->launchAsync (
                juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                [safeThis = juce::Component::SafePointer (this), complete] (const juce::FileChooser& fc) {
                    // CR-02: the host may destroy the editor while the native dialog
                    // is open. `complete` is owned by the dead WebView Impl, so on
                    // teardown bail with a bare return — calling complete() on ANY
                    // path (even complete(false)) is itself a use-after-free.
                    if (safeThis == nullptr)
                        return;
                    auto& processorRef = safeThis->processorRef;
                    auto* result = new juce::DynamicObject();
                    auto results = fc.getResults();
                    if (results.isEmpty()) {
                        result->setProperty ("success", false);
                        result->setProperty ("name", juce::String());
                    } else {
                        auto name = results.getFirst().getFileNameWithoutExtension();
                        bool ok = processorRef.getPresetManager().savePreset (name);
                        result->setProperty ("success", ok);
                        result->setProperty ("name", ok ? name : juce::String());
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
            fileChooser->launchAsync (
                juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                [safeThis = juce::Component::SafePointer (this), complete] (const juce::FileChooser& fc) {
                    // CR-02: see savePresetWithDialog — bare return on dead editor,
                    // never touch complete() after teardown.
                    if (safeThis == nullptr)
                        return;
                    auto& processorRef = safeThis->processorRef;
                    auto* result = new juce::DynamicObject();
                    auto results = fc.getResults();
                    if (results.isEmpty()) {
                        result->setProperty ("success", false);
                        result->setProperty ("name", juce::String());
                    } else {
                        auto file = results.getFirst();
                        bool ok = processorRef.getPresetManager().loadPresetFromFile (file);
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
        // Host sample rate — used by the spectrum canvas for its log-frequency axis
        // labels (so the 100 Hz / 1 k / 10 k ticks land at the right Nyquist).
        .withNativeFunction ("getSampleRate", [this] (auto&, auto complete) {
            complete (processorRef.getSampleRate());
        });

   #if JUCE_WINDOWS
    options = options.withWinWebView2Options (
        juce::WebBrowserComponent::Options::WinWebView2{}
            .withUserDataFolder (
                juce::File::getSpecialLocation (juce::File::tempDirectory)
                    .getChildFile ("OsimplePMS_WebView"))
            .withStatusBarDisabled()
            .withBuiltInErrorPageDisabled());
   #endif

    webView = std::make_unique<juce::WebBrowserComponent> (options);

    // 3. ATTACHMENTS (after the WebView) ------------------------------------
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
        jassert (param != nullptr);   // ID drift → silently dead combo; catch in debug
        if (param != nullptr)
            comboAttachments.push_back (
                std::make_unique<juce::WebComboBoxParameterAttachment> (
                    *param, *comboRelays[(size_t) i], nullptr));
    }

    addAndMakeVisible (*webView);
    webView->goToURL (juce::WebBrowserComponent::getResourceProviderRoot());

    setSize (1040, 860);   // fixed window — left→right signal-flow layout

    startTimerHz (30);     // drive the spectrum + scope (analyzer runs here, off the audio thread)
}

OSimplePhysicalModelSynthAudioProcessorEditor::~OSimplePhysicalModelSynthAudioProcessorEditor()
{
    // Stop the Timer FIRST — timerCallback() touches vizAnalyzer and webView, both
    // of which are destroyed below. The Timer base destructor would only stop it
    // AFTER the members are gone, so stop it explicitly here.
    stopTimer();
}

// ── Timer: run the analyzer, push spectrum + scope to the page ───────────────
void OSimplePhysicalModelSynthAudioProcessorEditor::timerCallback()
{
    // FFT + scope downsample on the message thread (PERF-01: never on audio).
    vizAnalyzer.process (processorRef.getVizTap().waveform, processorRef.getSampleRate());

    if (webView == nullptr)
        return;

    const auto& spec  = vizAnalyzer.getSpectrum();   // 256 dB bins, ~[-100, 0]
    const auto& scope = vizAnalyzer.getScope();      // 128 pts, [-1, 1]

    juce::Array<juce::var> specArr, scopeArr;
    specArr.ensureStorageAllocated ((int) spec.size());
    scopeArr.ensureStorageAllocated ((int) scope.size());
    for (float v : spec)  specArr.add (v);
    for (float v : scope) scopeArr.add (v);

    webView->emitEventIfBrowserIsVisible ("spectrumUpdate", juce::var (std::move (specArr)));
    webView->emitEventIfBrowserIsVisible ("scopeUpdate",    juce::var (std::move (scopeArr)));

    // ── Loop / flow diagram (Phase 3.3) ──────────────────────────────────────
    // Read the live taps the audio thread published: the KS circulating-energy
    // envelope (drives the String-skin pulse dampening, UI-02) and the 8 modal
    // (freq, amp) stems (drive the Modal-skin stems, UI-05). JS picks the skin
    // from resonatorType, so we don't send it. The DynamicObject is handed to a
    // juce::var, which takes shared ownership (ReferenceCountedObjectPtr) — no leak.
    const auto& viz = processorRef.getVizTap();
    auto* loopObj = new juce::DynamicObject();
    loopObj->setProperty ("energy", viz.getLoopEnergy());

    juce::Array<juce::var> stemFreqs, stemAmps;
    stemFreqs.ensureStorageAllocated (VizTap::kStems);
    stemAmps.ensureStorageAllocated (VizTap::kStems);
    for (int k = 0; k < VizTap::kStems; ++k)
    {
        stemFreqs.add (viz.getStemFreq (k));
        stemAmps.add  (viz.getStemAmp  (k));
    }
    loopObj->setProperty ("stemFreqs", std::move (stemFreqs));
    loopObj->setProperty ("stemAmps",  std::move (stemAmps));

    webView->emitEventIfBrowserIsVisible ("loopUpdate", juce::var (loopObj));
}

// ── Layout ──────────────────────────────────────────────────────────────────
void OSimplePhysicalModelSynthAudioProcessorEditor::paint (juce::Graphics&)
{
    // WebView fills the editor — nothing to paint.
}

void OSimplePhysicalModelSynthAudioProcessorEditor::resized()
{
    if (webView != nullptr)
        webView->setBounds (getLocalBounds());
}
