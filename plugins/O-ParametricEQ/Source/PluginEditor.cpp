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

    O-ParametricEQ - Editor implementation, Phase 3.2

    From .planning/mockups/v1-PluginEditor-TEMPLATE.cpp, mapped onto the
    Stage 2 processor (RESEARCH-3.1 Q2; PLAN-3.1 P2):

      - The editor owns a DisplayModel (3.2): the 60 Hz timer polls it and
        emits `eqSections` for the bands that changed, all 24 after
        requestEqSections. No SpectrumAnalyzer yet (3.3): building one
        registers the analyzer consumer and turns the taps on.
      - No editorOpen / outputLevelDB (the processor has neither; the meter
        reads readAndResetOutputPeak in 3.3).
      - listenBand is 0 ... 23 with -1 off; the page sends 1 ... 24 with 0 off.
      - presetManager is the processor's (call C2); saves go through
        PresetSaveGuard.h.

    UI state lives in the processor's atomics and its `UI` state child:
    saved with the session, not part of presets, not in APVTS.

  ==============================================================================
*/

#include "PluginEditor.h"
#include "PresetSaveGuard.h"
#include "analysis/DisplayModel.h"
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

    /** {success, name} result object for the preset dialog functions. */
    juce::var presetResult (bool success, const juce::String& name)
    {
        auto* obj = new juce::DynamicObject();
        obj->setProperty ("success", success);
        obj->setProperty ("name", name);
        return juce::var (obj);
    }

    /** ~/Library/O-ParametricEQ: the timing flag and its log (P10). */
    juce::File pluginLibraryDirectory()
    {
        return juce::File::getSpecialLocation (juce::File::userHomeDirectory)
                   .getChildFile ("Library")
                   .getChildFile ("O-ParametricEQ");
    }

    double nowMs() { return juce::Time::getMillisecondCounterHiRes(); }
}

#if defined (OPEQ_EDITOR_BENCH)
opeq::bench::Hooks& opeq::bench::hooks()
{
    static Hooks h;
    return h;
}
#endif

//==============================================================================
Editor::OuariconParametricEQAudioProcessorEditor (Processor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Timing log (P10): only when the flag file exists, checked once here.
    constructedAtMs  = nowMs();
    timingLogEnabled = pluginLibraryDirectory().getChildFile ("ui-timing.flag").existsAsFile();

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

    // 192 band relays. All 196 bound (RESEARCH-3.1 C1: painted page <= 274 ms
    // cold in WKWebView; no selected-band fallback).
    for (auto& band : bandRelays)
        options = band->addTo (options);

    options = addNativeFunctions (options);

   #if defined (OPEQ_EDITOR_BENCH)
    // Editor bench only (P10): the injected timing script and its report
    // channel. Registered by Identifier so the page's native-function list
    // (19) stays the only set of string-literal registrations.
    if (opeq::bench::hooks().userScript.isNotEmpty())
        options = options.withUserScript (opeq::bench::hooks().userScript);

    options = options.withNativeFunction (juce::Identifier ("benchReport"),
        [] (auto& args, auto complete)
        {
            const double t = nowMs();
            if (opeq::bench::hooks().onReport)
                opeq::bench::hooks().onReport (args.size() > 0 ? args[0] : juce::var(), t);
            complete (true);
        });
   #endif

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

    // The response display's model (PLAN-3.2 P2). Construction designs
    // nothing; its first poll() reports all 24 bands.
    displayModel = std::make_unique<DisplayModel> (audioProcessor);

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
    const int restoredWidth  = juce::jlimit (kMinWidth,  kMaxWidth,
                                             audioProcessor.uiEditorWidth.load (std::memory_order_acquire));
    const int restoredHeight = juce::jlimit (kMinHeight, kMaxHeight,
                                             audioProcessor.uiEditorHeight.load (std::memory_order_acquire));

    setResizable (true, true);
    setResizeLimits (kMinWidth, kMinHeight, kMaxWidth, kMaxHeight);

    // KEEP THIS LITERAL. The UI gates (scripts/serve-ui.js readEditorSize)
    // parse the last setSize call that has two integer literals; without one
    // boot-all-uis and check-ui-labels cannot size the page and fail.
    setSize (960, 600);

    if (restoredWidth != getWidth() || restoredHeight != getHeight())
        setSize (restoredWidth, restoredHeight);

    // The 60 Hz timer starts in the pageReady native function, not here.
}

Editor::~OuariconParametricEQAudioProcessorEditor()
{
    stopTimer();
    pageIsReady = false;

    // Listen is transient (U2): closing the editor must not leave a band soloed.
    audioProcessor.listenBand.store (-1, std::memory_order_release);

    // Members are destroyed in reverse declaration order:
    // attachments, then the WebView, then the relays. Nothing to do by hand.
}

//==============================================================================
juce::WebBrowserComponent::Options
Editor::addNativeFunctions (juce::WebBrowserComponent::Options options)
{
    // The page and preset-manager.js call exactly these 19 (grep both for
    // `getNativeFunction(` and diff against this list):
    //
    //   getParameterDefaults, getUiLanguage, setUiLanguage,
    //   getUiState, setUiState, setListenBand, requestEqSections, resizeEditor,
    //   savePreset, savePresetWithDialog, loadPreset, loadPresetFromFile,
    //   getPresetList, getCurrentPreset, selectNextPreset, selectPreviousPreset,
    //   deletePreset, isFactoryPreset, pageReady

    // ── Slider defaults, engineering units (double-click reset) ─────────────
    // The relay's properties carry start / end / skew but no default, and a JS
    // default table would drift from C++.
    options = options.withNativeFunction ("getParameterDefaults",
        [this] (auto&, auto complete)
        {
            static const juce::StringArray sliderIds = makeSliderIds();
            auto* obj = new juce::DynamicObject();

            for (const auto& id : sliderIds)
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
            obj->setProperty ("displayRange", audioProcessor.uiDisplayRange.load (std::memory_order_acquire));
            obj->setProperty ("selectedBand", audioProcessor.uiSelectedBand.load (std::memory_order_acquire));
            complete (juce::var (obj));
        });

    options = options.withNativeFunction ("setUiState",
        [this] (auto& args, auto complete)
        {
            // setUiState (key, intValue). The same validation as
            // setStateInformation; an unknown key or an out-of-range value is
            // refused (false) and the atomic is left unchanged.
            bool stored = false;

            if (args.size() >= 2)
            {
                const auto key   = args[0].toString();
                const int  value = (int) args[1];

                if (key == "analyzerMode" && value >= 0 && value <= 3)
                {
                    audioProcessor.analyzerMode.store (value, std::memory_order_release);
                    stored = true;
                }
                else if (key == "displayRange" && (value == 6 || value == 12 || value == 30))
                {
                    audioProcessor.uiDisplayRange.store (value, std::memory_order_release);
                    stored = true;
                }
                else if (key == "selectedBand" && value >= 0 && value <= opeq::kNumBands)
                {
                    audioProcessor.uiSelectedBand.store (value, std::memory_order_release);
                    stored = true;
                }
            }

            complete (stored);
        });

    // ── Listen (FUNC-08): transient, one band at a time, never saved ────────
    options = options.withNativeFunction ("setListenBand",
        [this] (auto& args, auto complete)
        {
            // Page sends 1 ... 24, or 0 for off. Engine index is 0 ... 23, -1 off.
            // The drag / latch state machine (U2) is Phase 3.3.
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
            // The next timerCallback() emits all 24 bands. The page calls this
            // at init (before pageReady: the flag waits for the first tick) and
            // after every preset load.
            fullPushPending = true;
            complete (true);
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

    // ── Page ready: its 196 states are bound, the UI state pull is queued ───
    // Starts the 60 Hz timer (nothing is sent before the page exists). A page
    // reload calls it again: idempotent. The argument is the page's own
    // performance.now() at its first painted frame (timing log, P10).
    options = options.withNativeFunction ("pageReady",
        [this] (auto& args, auto complete)
        {
            const bool first = ! pageIsReady;
            pageIsReady = true;

            if (! isTimerRunning())
                startTimerHz (60);

            if (timingLogEnabled && first)
                writeTimingLog (args.size() > 0 ? (double) args[0] : -1.0);

            complete (true);
        });

    // ── Preset manager (preset-manager 1.1.1, call C2) ──────────────────────
    // Same bridge as O-AnalogEQ. Both save paths consult PresetSaveGuard
    // (savePresetToFile() has no factory guard of its own).
    options = options.withNativeFunction ("savePreset",
        [this] (const auto& args, auto complete)
        {
            auto& pm = audioProcessor.presetManager;
            const auto name = args.size() > 0 ? args[0].toString() : juce::String();

            if (name.isEmpty())
            {
                complete (false);
                return;
            }

            // The file savePreset (name) writes: User/<sanitised name>.json
            // (the module's own path-separator rule).
            const auto target = pm.getUserPresetsDirectory()
                                  .getChildFile (name.replaceCharacters ("/\\:", "___") + ".json");

            if (opeq::presetSaveRefusal (target, pm).isNotEmpty())
            {
                complete (false);
                return;
            }

            complete (pm.savePreset (name));
        });

    options = options.withNativeFunction ("savePresetWithDialog",
        [this] (auto&, auto complete)
        {
            // IN-08: one native dialog at a time.
            if (presetDialogInFlight) { complete (presetResult (false, {})); return; }
            presetDialogInFlight = true;

            fileChooser = std::make_unique<juce::FileChooser> (
                "Save Preset", audioProcessor.presetManager.getUserPresetsDirectory(), "*.json");

            // The editor may be destroyed while the OS dialog is open. If it
            // is, return WITHOUT calling complete: the completion holds a raw
            // pointer into the destroyed WebView (juce_WebBrowserComponent.cpp
            // handleNativeFunctionCall), and the page is gone with it.
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

                    // Factory-preset overwrite protection (PresetSaveGuard.h).
                    // The page's Save is fire-and-forget, so the refusal is
                    // shown on the surface the file dialog just used.
                    if (const auto refusal = opeq::presetSaveRefusal (file, pm); refusal.isNotEmpty())
                    {
                        juce::NativeMessageBox::showAsync (
                            juce::MessageBoxOptions()
                                .withIconType (juce::MessageBoxIconType::WarningIcon)
                                .withTitle ("Save Preset")
                                .withMessage (refusal)
                                .withButton ("OK"),
                            nullptr);

                        complete (presetResult (false, {}));
                        return;
                    }

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

            // See savePresetWithDialog: no complete once the editor is gone.
            juce::Component::SafePointer<Editor> safeThis (this);
            fileChooser->launchAsync (
                juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                [safeThis, complete] (const juce::FileChooser& fc)
                {
                    if (safeThis == nullptr)
                        return;
                    safeThis->presetDialogInFlight = false;   // above every early return

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
void Editor::writeTimingLog (double pagePaintMs)
{
    // One tab-separated line per editor: wall time, constructor -> pageReady
    // on the message thread, and the page's performance.now() at first paint.
    const auto logFile = pluginLibraryDirectory().getChildFile ("Logs").getChildFile ("ui-timing.log");
    logFile.getParentDirectory().createDirectory();

    const auto line = juce::Time::getCurrentTime().toISO8601 (true)
                    + "\tversion=" + juce::String (JucePlugin_VersionString)
                    + "\thost=" + juce::PluginHostType().getHostDescription()
                    + "\twrapper=" + juce::String (juce::AudioProcessor::getWrapperTypeDescription (audioProcessor.wrapperType))
                    + "\tctor_to_pageReady_ms=" + juce::String (nowMs() - constructedAtMs, 1)
                    + "\tpage_first_paint_ms=" + juce::String (pagePaintMs, 1)
                    + "\n";

    logFile.appendText (line, false, false, "\n");
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
    audioProcessor.uiEditorWidth.store  (getWidth(),  std::memory_order_release);
    audioProcessor.uiEditorHeight.store (getHeight(), std::memory_order_release);
}

//==============================================================================
void Editor::timerCallback()
{
    // Started by pageReady only; nothing is sent before the page exists.
    if (webView == nullptr || ! pageIsReady)
        return;

    // Response display (PLAN-3.2 P2). poll() reads the parameter atomics, so
    // the curve follows host edits with the transport stopped. Only the bands
    // that changed go out; all 24 after requestEqSections; nothing at all on a
    // tick with no change (idle traffic stays 0).
    //
    // emitEventIfBrowserIsVisible drops the event while the WebView component
    // is hidden, and poll() has already consumed the change by then. This
    // editor never hides the WebView. If a later change does, skip poll()
    // while it is hidden, or set fullPushPending when it is shown again.
    const auto changed = displayModel->poll();
    const bool full = fullPushPending;
    fullPushPending = false;

    if (full || ! changed.isEmpty())
    {
        const auto header = displayModel->headerVar();

        auto* payload = new juce::DynamicObject();
        payload->setProperty ("sampleRate", header["sampleRate"]);
        payload->setProperty ("stereo",     header["stereo"]);
        payload->setProperty ("rateKnown",  header["rateKnown"]);

        juce::Array<juce::var> bands;
        const auto addBand = [this, &bands] (int b)
        {
            auto packet = displayModel->packetFor (b);
            if (auto* obj = packet.getDynamicObject())
            {
                obj->setProperty ("band", b + 1);   // the page is 1-based
                bands.add (packet);
            }
        };

        if (full)
            for (int b = 0; b < DisplayModel::numBands; ++b)
                addBand (b);
        else
            for (const int b : changed)
                addBand (b);

        payload->setProperty ("bands", bands);
        webView->emitEventIfBrowserIsVisible ("eqSections", juce::var (payload));

       #if defined (OPEQ_EDITOR_BENCH)
        if (opeq::bench::hooks().onEqSections)
        {
            juce::Array<int> sent;
            for (const auto& pk : bands)
                sent.add ((int) pk["band"] - 1);
            opeq::bench::hooks().onEqSections (sent);
        }
       #endif
    }

    // Phase 3.3: analyzer frames (`analyzerPre` / `analyzerPost`, from an
    // editor-owned SpectrumAnalyzer) and the output meter (readAndResetOutputPeak).
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

    // Shared modules, embedded from modules/ (not the configure-time copies).
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

    juce::Logger::writeToLog ("Resource not found: " + url);
    return std::nullopt;
}
