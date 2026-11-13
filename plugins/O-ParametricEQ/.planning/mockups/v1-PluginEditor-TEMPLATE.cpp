/*
   This file is part of O-ParametricEQ, an Ouaricon Audio plugin.
   Copyright (C) 2026  Ouaricon Audio

   SPDX-License-Identifier: AGPL-3.0-or-later

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
/*
  ==============================================================================

    O-ParametricEQ - Editor implementation          TEMPLATE, UI mockup v1

    REFERENCE for gui-agent (Stage 3). Not compiled. Adapt to the real
    processor.

    ASSUMED PROCESSOR API (none of it is generated here; see the integration
    checklist, section 2):

      juce::AudioProcessorValueTreeState parameters;       // 196 parameters
      OuariconPresetManager presetManager;                 // preset-manager 1.1.1
      std::atomic<int> uiLanguage { 0 };                   // + languageCode / languageIndex
      std::atomic<int> analyzerMode { 2 };                 // UI state, 0 Off 1 Pre 2 Post 3 Pre+Post
      std::atomic<int> listenBand { -1 };                  // transient, never saved
      std::atomic<bool> editorOpen { false };
      int  uiDisplayRange = 12;                            // UI state: 6 / 12 / 30
      int  uiSelectedBand = 0;                             // UI state: 0 none, 1 ... 24
      int  uiEditorWidth = 960, uiEditorHeight = 600;      // UI state
      std::atomic<float> outputLevelDB;                    // or VUMeterBridge (vu-meter 1.0.0)
      eq::DisplayModel displayModel;                       // message thread; see timerCallback()

    UI state lives in the `UI` child of the state tree (ARCHITECTURE.md "State
    Persistence"): saved with the session, not part of presets, not in APVTS.

  ==============================================================================
*/

#include "PluginEditor.h"
#include "UIBinaryData.h"

namespace
{
    using Editor    = OuariconParametricEQAudioProcessorEditor;
    using Processor = OuariconParametricEQAudioProcessor;

    auto makeBinaryResource (const char* data, int size, const char* mimeType)
        -> std::optional<juce::WebBrowserComponent::Resource>
    {
        auto* bytes = reinterpret_cast<const std::byte*> (data);
        return juce::WebBrowserComponent::Resource {
            std::vector<std::byte> (bytes, bytes + size),
            juce::String (mimeType)
        };
    }

    /** Every WebSliderRelay parameter ID (74): the page asks for their defaults
        in engineering units through getParameterDefaults. Built by loop, in
        parameter-spec.md order. */
    juce::StringArray makeSliderIds()
    {
        juce::StringArray ids { "output_gain", "gain_scale" };

        for (int b = 0; b < opeq::kNumBands; ++b)
        {
            const auto prefix = opeq::bandPrefix (b);
            ids.add (prefix + "freq");
            ids.add (prefix + "gain");
            ids.add (prefix + "q");
        }

        return ids;
    }

    const juce::StringArray kSliderIds = makeSliderIds();

    /** {success, name} result object for the preset dialog functions. */
    juce::var presetResult (bool success, const juce::String& name)
    {
        auto* obj = new juce::DynamicObject();
        obj->setProperty ("success", success);
        obj->setProperty ("name", name);
        return juce::var (obj);
    }
}

//==============================================================================
Editor::OuariconParametricEQAudioProcessorEditor (Processor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // ========================================================================
    // INITIALISATION ORDER (matches the member declaration order)
    //   1. relays   2. WebView (with every relay's options)   3. attachments
    // ========================================================================

    // 1. RELAYS -----------------------------------------------------------------
    // The relay name is the parameter ID, exactly as in parameter-spec.md.
    bypassRelay     = std::make_unique<juce::WebToggleButtonRelay> ("bypass");
    outputGainRelay = std::make_unique<juce::WebSliderRelay>       ("output_gain");
    gainScaleRelay  = std::make_unique<juce::WebSliderRelay>       ("gain_scale");
    autoGainRelay   = std::make_unique<juce::WebToggleButtonRelay> ("auto_gain");

    for (int b = 0; b < opeq::kNumBands; ++b)
        bandRelays[(size_t) b] = std::make_unique<opeq::BandRelays> (opeq::bandPrefix (b));

    // 2. WEBVIEW ----------------------------------------------------------------
    auto options = juce::WebBrowserComponent::Options{}
        // REQUIRED: JUCE frontend library (window.__JUCE__)
        .withNativeIntegrationEnabled()
        // REQUIRED: embedded files, bare paths ("/", "/js/i18n.js", ...)
        .withResourceProvider ([this] (const auto& url) { return getResource (url); })
        // REQUIRED: hosts that hide the editor (FL Studio) otherwise reload about:blank
        .withKeepPageLoadedWhenBrowserIsHidden()
        // Global relays: slider, toggle AND combo relays all need withOptionsFrom
        .withOptionsFrom (*bypassRelay)
        .withOptionsFrom (*outputGainRelay)
        .withOptionsFrom (*gainScaleRelay)
        .withOptionsFrom (*autoGainRelay);

    // 192 band relays. UNVERIFIED AT THIS SCALE (ARCHITECTURE.md "Unverified"):
    // no suite plugin has registered 196 relays. Check early in Phase 3.1 that
    // the page boots, that every state reports properties, and what the editor
    // open time is. Fallback (ROADMAP risk table): relays for the selected band
    // only, re-pointed on selection.
    for (auto& band : bandRelays)
        options = band->addTo (options);

    options = addNativeFunctions (options);

   #if JUCE_WINDOWS
    // REQUIRED on Windows: name the backend. Backend::defaultBackend is the
    // legacy IE control, which ignores withWinWebView2Options and has no
    // resource provider ("page not found").
    // The user-data folder lives in the per-user app-data tree (NOT %TEMP%,
    // which cleaners wipe and other processes lock) and is created before
    // WebView2 sees it. DAW hosts deny WebView2's default location.
    const auto webView2DataFolder = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                                        .getChildFile ("Ouaricon")
                                        .getChildFile ("WebView2")
                                        .getChildFile ("O-ParametricEQ");
    webView2DataFolder.createDirectory();

    options = options
        .withBackend (juce::WebBrowserComponent::Options::Backend::webview2)
        .withWinWebView2Options (
            juce::WebBrowserComponent::Options::WinWebView2{}
                .withUserDataFolder (webView2DataFolder)
                .withStatusBarDisabled()
                .withBuiltInErrorPageDisabled());

    // When the WebView2 environment cannot be created (runtime missing, data
    // folder unusable), JUCE silently builds the IE control instead. Probe
    // first; on failure webView stays null and paint() shows a native notice.
    // Audio, parameters and host automation do not depend on the WebView.
    const bool webView2Available = juce::WebBrowserComponent::areOptionsSupported (options);
   #else
    constexpr bool webView2Available = true;
   #endif

    if (webView2Available)
        webView = std::make_unique<juce::WebBrowserComponent> (options);

    // 3. ATTACHMENTS --------------------------------------------------------------
    // Three arguments (parameter, relay, undo manager): JUCE 8.
    // They stay valid without a WebView: the relays drop their events while no
    // browser is registered.
    auto& apvts = audioProcessor.parameters;

    bypassAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment> (
        *apvts.getParameter ("bypass"), *bypassRelay, nullptr);
    outputGainAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *apvts.getParameter ("output_gain"), *outputGainRelay, nullptr);
    gainScaleAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *apvts.getParameter ("gain_scale"), *gainScaleRelay, nullptr);
    autoGainAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment> (
        *apvts.getParameter ("auto_gain"), *autoGainRelay, nullptr);

    for (int b = 0; b < opeq::kNumBands; ++b)
        bandAttachments[(size_t) b] = std::make_unique<opeq::BandAttachments> (
            apvts, opeq::bandPrefix (b), *bandRelays[(size_t) b]);

    // WEBVIEW SETUP ---------------------------------------------------------------
    if (webView != nullptr)
    {
        addAndMakeVisible (*webView);
        webView->goToURL (juce::WebBrowserComponent::getResourceProviderRoot());
    }
    else
    {
        // Windows only: WebView2 could not start (see paint()).
        webView2RuntimeLink = std::make_unique<juce::HyperlinkButton> (
            "Download the WebView2 Runtime",
            juce::URL ("https://developer.microsoft.com/microsoft-edge/webview2/"));
        webView2RuntimeLink->setFont (juce::FontOptions (17.0f, juce::Font::underlined), false);
        webView2RuntimeLink->setColour (juce::HyperlinkButton::textColourId,
                                        juce::Colour (0xff4e6839));   // --sage-fill #4E6839
        addAndMakeVisible (*webView2RuntimeLink);
    }

    // WINDOW ----------------------------------------------------------------------
    // v1-ui.yaml: 960 x 600 default, resizable 800 x 500 to 1600 x 1000, aspect
    // FREE. The page reflows (header 40 px and strip 76 px keep their heights;
    // the response display takes the rest). This deviates from ui-design-rules
    // Rule 4 (fixed frame, or locked aspect with whole-page scale) by user
    // decision, so there is no setFixedAspectRatio here.
    //
    // Read the saved size BEFORE the first setSize: resized() writes the
    // current size back into the processor's UI state.
    const int restoredWidth  = juce::jlimit (kMinWidth,  kMaxWidth,  audioProcessor.uiEditorWidth);
    const int restoredHeight = juce::jlimit (kMinHeight, kMaxHeight, audioProcessor.uiEditorHeight);

    setResizable (true, true);
    setResizeLimits (kMinWidth, kMinHeight, kMaxWidth, kMaxHeight);

    // KEEP THIS LITERAL. The UI gates (scripts/serve-ui.js readEditorSize)
    // parse the last setSize call that has two integer literals; without one
    // boot-all-uis and check-ui-labels cannot size the page and fail.
    setSize (960, 600);

    if (restoredWidth != getWidth() || restoredHeight != getHeight())
        setSize (restoredWidth, restoredHeight);

    audioProcessor.editorOpen.store (true, std::memory_order_release);

    // 30 Hz: output meter, analyzer frames, changed eqSections packets.
    startTimerHz (30);
}

Editor::~OuariconParametricEQAudioProcessorEditor()
{
    stopTimer();
    audioProcessor.editorOpen.store (false, std::memory_order_release);
    // Listen is transient: closing the editor must not leave a band soloed.
    audioProcessor.listenBand.store (-1, std::memory_order_release);

    // Members are destroyed in reverse declaration order:
    // attachments, then the WebView, then the relays. Nothing to do by hand.
}

//==============================================================================
juce::WebBrowserComponent::Options
Editor::addNativeFunctions (juce::WebBrowserComponent::Options options)
{
    // The page calls exactly these (grep the page for `getNativeFunction(` and
    // for the preset-manager module's calls, and diff against this list):
    //
    //   getParameterDefaults, getUiLanguage, setUiLanguage,
    //   getUiState, setUiState, setListenBand, requestEqSections, resizeEditor,
    //   savePreset, savePresetWithDialog, loadPreset, loadPresetFromFile,
    //   getPresetList, getCurrentPreset, selectNextPreset, selectPreviousPreset,
    //   deletePreset, isFactoryPreset

    // ── Slider defaults, engineering units (double-click reset) ─────────────
    // The relay's properties carry start / end / skew but no default, and a JS
    // default table would drift from C++.
    options = options.withNativeFunction ("getParameterDefaults",
        [this] (auto&, auto complete)
        {
            auto* obj = new juce::DynamicObject();

            for (const auto& id : kSliderIds)
            {
                if (auto* param = audioProcessor.parameters.getParameter (id))
                    obj->setProperty (id, param->convertFrom0to1 (param->getDefaultValue()));
            }

            complete (juce::var (obj));
        });

    // ── UI language (the i18n canon's initI18n() calls both) ────────────────
    options = options.withNativeFunction ("getUiLanguage",
        [this] (auto&, auto complete)
        {
            complete (juce::var (Processor::languageCode (
                audioProcessor.uiLanguage.load (std::memory_order_acquire))));
        });

    options = options.withNativeFunction ("setUiLanguage",
        [this] (auto& args, auto complete)
        {
            // languageIndex() maps anything that is not "fr" or "zh-Hans" to en,
            // so an unexpected argument degrades to English, never stored raw.
            if (args.size() > 0)
                audioProcessor.uiLanguage.store (
                    Processor::languageIndex (args[0].toString()),
                    std::memory_order_release);
            complete (juce::var());
        });

    // ── Session UI state (NOT host parameters) ──────────────────────────────
    // The page PULLS once at init. No push from the constructor: it would race
    // the page load.
    options = options.withNativeFunction ("getUiState",
        [this] (auto&, auto complete)
        {
            auto* obj = new juce::DynamicObject();
            obj->setProperty ("analyzerMode", audioProcessor.analyzerMode.load (std::memory_order_acquire));
            obj->setProperty ("displayRange", audioProcessor.uiDisplayRange);
            obj->setProperty ("selectedBand", audioProcessor.uiSelectedBand);
            complete (juce::var (obj));
        });

    options = options.withNativeFunction ("setUiState",
        [this] (auto& args, auto complete)
        {
            // setUiState (key, intValue). Every value is validated; an unknown
            // key or an out-of-range value is ignored, never stored raw.
            if (args.size() >= 2)
            {
                const auto key   = args[0].toString();
                const int  value = (int) args[1];

                if (key == "analyzerMode" && value >= 0 && value <= 3)
                    audioProcessor.analyzerMode.store (value, std::memory_order_release);
                else if (key == "displayRange" && (value == 6 || value == 12 || value == 30))
                    audioProcessor.uiDisplayRange = value;
                else if (key == "selectedBand" && value >= 0 && value <= opeq::kNumBands)
                    audioProcessor.uiSelectedBand = value;
            }

            complete (juce::var());
        });

    // ── Listen (FUNC-08): transient, one band at a time, never saved ────────
    options = options.withNativeFunction ("setListenBand",
        [this] (auto& args, auto complete)
        {
            // Page sends 1 ... 24, or 0 for off. Engine index is 0 ... 23, -1 off.
            const int band = args.size() > 0 ? (int) args[0] : 0;
            audioProcessor.listenBand.store (
                band >= 1 && band <= opeq::kNumBands ? band - 1 : -1,
                std::memory_order_release);
            complete (juce::var());
        });

    // ── Response display: full push on request (pull, not a one-shot push) ──
    options = options.withNativeFunction ("requestEqSections",
        [this] (auto&, auto complete)
        {
            // Marks every band dirty; the next timerCallback() emits them all.
            audioProcessor.displayModel.requestFullPush();
            complete (juce::var());
        });

    // ── Corner grip in the page ──────────────────────────────────────────────
    // The WebView covers the editor, so JUCE's own corner resizer is not
    // reachable on every host. The page's grip asks for a size instead.
    options = options.withNativeFunction ("resizeEditor",
        [this] (auto& args, auto complete)
        {
            if (args.size() >= 2)
                setSize (juce::jlimit (kMinWidth,  kMaxWidth,  (int) args[0]),
                         juce::jlimit (kMinHeight, kMaxHeight, (int) args[1]));
            complete (juce::var());
        });

    // ── Preset manager (preset-manager 1.1.1) ───────────────────────────────
    // Same bridge as O-AnalogEQ. Stage 3: port its PresetSaveGuard (factory
    // overwrite protection; savePresetToFile() has no guard of its own).
    options = options.withNativeFunction ("savePreset",
        [this] (const auto& args, auto complete)
        {
            complete (args.size() > 0 && audioProcessor.presetManager.savePreset (args[0].toString()));
        });

    options = options.withNativeFunction ("savePresetWithDialog",
        [this] (auto&, auto complete)
        {
            if (presetDialogInFlight) { complete (presetResult (false, {})); return; }
            presetDialogInFlight = true;

            fileChooser = std::make_unique<juce::FileChooser> (
                "Save Preset", audioProcessor.presetManager.getUserPresetsDirectory(), "*.json");

            // The editor may be destroyed while the OS dialog is open.
            juce::Component::SafePointer<Editor> safeThis (this);
            fileChooser->launchAsync (
                juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                [safeThis, complete] (const juce::FileChooser& fc)
                {
                    if (safeThis == nullptr)
                        return;
                    safeThis->presetDialogInFlight = false;   // above every early return

                    const auto results = fc.getResults();
                    if (results.isEmpty()) { complete (presetResult (false, {})); return; }

                    auto& pm = safeThis->audioProcessor.presetManager;
                    const auto file   = results.getFirst();
                    const auto target = file.hasFileExtension ("json") ? file : file.withFileExtension ("json");

                    // Stage 3: refuse a factory-preset overwrite here (PresetSaveGuard).
                    const bool success = pm.savePresetToFile (target);
                    complete (presetResult (success, success ? pm.getCurrentPresetName() : juce::String()));
                });
        });

    options = options.withNativeFunction ("loadPreset",
        [this] (const auto& args, auto complete)
        {
            complete (args.size() > 0 && audioProcessor.presetManager.loadPreset (args[0].toString()));
        });

    options = options.withNativeFunction ("loadPresetFromFile",
        [this] (auto&, auto complete)
        {
            if (presetDialogInFlight) { complete (presetResult (false, {})); return; }
            presetDialogInFlight = true;

            fileChooser = std::make_unique<juce::FileChooser> (
                "Load Preset", audioProcessor.presetManager.getPresetsDirectory(), "*.json");

            juce::Component::SafePointer<Editor> safeThis (this);
            fileChooser->launchAsync (
                juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                [safeThis, complete] (const juce::FileChooser& fc)
                {
                    if (safeThis == nullptr)
                        return;
                    safeThis->presetDialogInFlight = false;

                    const auto results = fc.getResults();
                    if (results.isEmpty()) { complete (presetResult (false, {})); return; }

                    const auto file    = results.getFirst();
                    const bool success = safeThis->audioProcessor.presetManager.loadPresetFromFile (file);
                    complete (presetResult (success, success ? file.getFileNameWithoutExtension() : juce::String()));
                });
        });

    options = options.withNativeFunction ("getPresetList",
        [this] (auto&, auto complete)
        {
            juce::Array<juce::var> arr;
            for (const auto& name : audioProcessor.presetManager.getPresetList())
                arr.add (name);
            complete (juce::var (arr));
        });

    options = options.withNativeFunction ("getCurrentPreset",
        [this] (auto&, auto complete) { complete (audioProcessor.presetManager.getCurrentPresetName()); });

    options = options.withNativeFunction ("selectNextPreset",
        [this] (auto&, auto complete) { complete (audioProcessor.presetManager.getNextPreset()); });

    options = options.withNativeFunction ("selectPreviousPreset",
        [this] (auto&, auto complete) { complete (audioProcessor.presetManager.getPreviousPreset()); });

    options = options.withNativeFunction ("deletePreset",
        [this] (const auto& args, auto complete)
        {
            complete (args.size() > 0 && audioProcessor.presetManager.deletePreset (args[0].toString()));
        });

    options = options.withNativeFunction ("isFactoryPreset",
        [this] (const auto& args, auto complete)
        {
            complete (args.size() > 0 && audioProcessor.presetManager.isFactoryPreset (args[0].toString()));
        });

    return options;
}

//==============================================================================
void Editor::paint (juce::Graphics& g)
{
    // The WebView fills the editor: nothing to paint, unless WebView2 could not
    // start (Windows only). Colours are the page's own CSS tokens.
    if (webView != nullptr)
        return;

    g.fillAll (juce::Colour (0xfff5e6d3));   // --paper #F5E6D3

    auto area = getLocalBounds().withSizeKeepingCentre (juce::jmin (640, getWidth() - 48), 220)
                                .translated (0, -40);

    g.setColour (juce::Colour (0xff3c2f2f));   // --ink #3C2F2F
    g.setFont (juce::FontOptions (26.0f, juce::Font::bold));
    g.drawFittedText ("O-ParametricEQ needs Microsoft Edge WebView2",
                      area.removeFromTop (40), juce::Justification::centred, 2);

    area.removeFromTop (12);
    g.setFont (juce::FontOptions (17.0f));
    g.drawFittedText ("The interface is drawn with the WebView2 Runtime, which could not start on "
                      "this computer. Install it, then close and reopen the plugin window.\n"
                      "The effect itself keeps working, and host automation still "
                      "reaches every parameter.",
                      area.removeFromTop (110), juce::Justification::centred, 5);
}

void Editor::resized()
{
    if (webView != nullptr)
        webView->setBounds (getLocalBounds());

    if (webView2RuntimeLink != nullptr)
        webView2RuntimeLink->setBounds (getLocalBounds().withSizeKeepingCentre (320, 32)
                                                        .translated (0, 40));

    // Editor size is session UI state (restored in the constructor).
    audioProcessor.uiEditorWidth  = getWidth();
    audioProcessor.uiEditorHeight = getHeight();
}

//==============================================================================
void Editor::timerCallback()
{
    if (webView == nullptr)   // WebView2 unavailable (Windows)
        return;

    // Output meter (UI-06). vu-meter 1.0.0 has no listed consumer; the fallback
    // is this O-AnalogEQ-style atomic + event.
    webView->emitEventIfBrowserIsVisible ("outputLevel",
        audioProcessor.outputLevelDB.load (std::memory_order_relaxed));

    // Response display (UI-02, Phase 3.2). DisplayModel runs the same designer
    // as the audio thread, on the message thread, and returns packets for the
    // bands whose parameters changed since the last call (all of them after
    // requestEqSections or a preset load). Payload the page expects:
    //
    //   { sampleRate: 48000,
    //     bands: [ { band: 1, type: 0, placement: 0, active: true,
    //                sections: [[g, k, mL, mB, mH], ...],
    //                onePoles: [[G, cL, cH], ...] }, ... ] }
    //
    // `band` is 1-based. `sampleRate` is required: the page evaluates the
    // closed forms at w = 2 pi f / sampleRate. A disabled band is sent once with
    // empty `sections` and `onePoles` so the page drops its curve.
    if (auto packets = audioProcessor.displayModel.takeChangedPackets(); ! packets.isVoid())
        webView->emitEventIfBrowserIsVisible ("eqSections", packets);

    // Analyzer (Phase 3.3): 256 log-spaced display points, dBFS, 10 Hz to 30 kHz.
    // Mode Off stops the audio-thread writes; nothing is emitted then.
    const int mode = audioProcessor.analyzerMode.load (std::memory_order_acquire);

    if (mode == 1 || mode == 3)
        webView->emitEventIfBrowserIsVisible ("analyzerPre", audioProcessor.takeAnalyzerFrame (false));
    if (mode == 2 || mode == 3)
        webView->emitEventIfBrowserIsVisible ("analyzerPost", audioProcessor.takeAnalyzerFrame (true));
}

//==============================================================================
std::optional<juce::WebBrowserComponent::Resource>
Editor::getResource (const juce::String& url)
{
    // Explicit URL -> symbol map. The provider receives BARE PATHS. Keep the
    // literal `if (url == "...") return makeBinaryResource (UIBinaryData::...`
    // shape: scripts/serve-ui.js pairs each URL with the first symbol near it.
    // juce_add_binary_data flattens paths and STRIPS hyphens from file names.

    if (url == "/" || url == "/index.html")
        return makeBinaryResource (UIBinaryData::index_html, UIBinaryData::index_htmlSize,
                                   "text/html; charset=utf-8");

    // The i18n table. Embedded in CMakeLists.txt AND served here, in the same
    // commit (check-i18n [8]).
    if (url == "/js/i18n.js")
        return makeBinaryResource (UIBinaryData::i18n_js, UIBinaryData::i18n_jsSize,
                                   "application/javascript; charset=utf-8");

    if (url == "/js/juce/index.js")
        return makeBinaryResource (UIBinaryData::index_js, UIBinaryData::index_jsSize,
                                   "application/javascript; charset=utf-8");

    if (url == "/js/juce/check_native_interop.js")
        return makeBinaryResource (UIBinaryData::check_native_interop_js,
                                   UIBinaryData::check_native_interop_jsSize,
                                   "application/javascript; charset=utf-8");

    // Shared modules (not copied into this plugin's tree).
    if (url == "/modules/preset-manager.js")
        return makeBinaryResource (UIBinaryData::presetmanager_js, UIBinaryData::presetmanager_jsSize,
                                   "application/javascript; charset=utf-8");

    if (url == "/modules/vu-meter.js")
        return makeBinaryResource (UIBinaryData::vumeter_js, UIBinaryData::vumeter_jsSize,
                                   "application/javascript; charset=utf-8");

    // Shared EB Garamond face (modules/ui/eb-garamond): stylesheet under /css/,
    // the woff2 faces under /fonts/ where its relative font URLs land.
    if (url == "/css/eb-garamond.css")
        return makeBinaryResource (UIBinaryData::ebgaramond_css, UIBinaryData::ebgaramond_cssSize,
                                   "text/css; charset=utf-8");
    if (url == "/fonts/EBGaramond-Regular.woff2")
        return makeBinaryResource (UIBinaryData::EBGaramondRegular_woff2,
                                   UIBinaryData::EBGaramondRegular_woff2Size, "font/woff2");
    if (url == "/fonts/EBGaramond-Italic.woff2")
        return makeBinaryResource (UIBinaryData::EBGaramondItalic_woff2,
                                   UIBinaryData::EBGaramondItalic_woff2Size, "font/woff2");
    if (url == "/fonts/EBGaramond-Bold.woff2")
        return makeBinaryResource (UIBinaryData::EBGaramondBold_woff2,
                                   UIBinaryData::EBGaramondBold_woff2Size, "font/woff2");

    // Stage 3: add the paper texture and the dragonfly engraving here once the
    // assets exist (the page carries a documented placeholder for the plate).

    juce::Logger::writeToLog ("Resource not found: " + url);
    return std::nullopt;
}
