/*
  ==============================================================================

    O-ReverseDelay — Plugin Editor (implementation)

    8 WebSliderRelay knobs (delayTime, grainSize, density, feedback, lowCut,
    highCut, width, mix) + 2 WebComboBoxRelay controls (syncMode, noteDivision)
    bound two-way to the APVTS. The UI-02 Sync/Free control swap is pure JS —
    both controls stay relay-bound at all times, so neither is ever dead.

  ==============================================================================
*/

#include "PluginEditor.h"
#include "UIBinaryData.h"   // distinct namespace — see CMakeLists juce_add_binary_data

namespace
{
    // Parameter IDs — must match createParameterLayout() exactly. An ID typo is
    // a silently dead control, so every lookup below is jassert'ed.
    const juce::StringArray kSliderIds {
        "delayTime",
        "grainSize", "density",
        "feedback", "lowCut", "highCut",
        "width", "mix"
    };

    const juce::StringArray kComboIds { "syncMode", "noteDivision" };

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

// ── Resource provider ───────────────────────────────────────────────────────
// The WKWebView / WebView2 resource callback receives a BARE PATH ("/",
// "/js/app.js", ...) — there is no scheme or host to strip, and stripping one
// would collapse every lookup to an empty string. Match by direct equality and
// never hard-code juce:// vs https://juce.backend
// (critical_webview_resource_provider_and_schemes).
std::optional<juce::WebBrowserComponent::Resource>
ReverseDelayEditor::getResource (const juce::String& url)
{
    // charset=utf-8 on text resources — the page uses UTF-8 entities (fleurons,
    // en-dashes, hair spaces) that mojibake without it on some hosts.
    if (url == "/" || url == "/index.html")
        return makeBinaryResource (UIBinaryData::index_html, UIBinaryData::index_htmlSize,
                                   "text/html; charset=utf-8");

    if (url == "/css/styles.css")
        return makeBinaryResource (UIBinaryData::styles_css, UIBinaryData::styles_cssSize,
                                   "text/css; charset=utf-8");

    if (url == "/js/app.js")
        return makeBinaryResource (UIBinaryData::app_js, UIBinaryData::app_jsSize,
                                   "application/javascript; charset=utf-8");

    // Shared preset-manager module. Embedded from modules/… but SERVED under
    // /js/ so app.js can reach it with a plain relative `import("./preset-manager.js")`.
    // NOTE the symbol: juce_add_binary_data STRIPS the hyphen rather than
    // converting it to an underscore, so "preset-manager.js" becomes
    // `presetmanager_js` — not `preset_manager_js`.
    if (url == "/js/preset-manager.js")
        return makeBinaryResource (UIBinaryData::presetmanager_js,
                                   UIBinaryData::presetmanager_jsSize,
                                   "application/javascript; charset=utf-8");

    if (url == "/js/juce/index.js")
        return makeBinaryResource (UIBinaryData::index_js, UIBinaryData::index_jsSize,
                                   "application/javascript; charset=utf-8");

    if (url == "/js/juce/check_native_interop.js")
        return makeBinaryResource (UIBinaryData::check_native_interop_js,
                                   UIBinaryData::check_native_interop_jsSize,
                                   "application/javascript; charset=utf-8");

    if (url == "/img/birds.png")
        return makeBinaryResource (UIBinaryData::birds_png, UIBinaryData::birds_pngSize,
                                   "image/png");

    return std::nullopt;
}

// ── Construction ────────────────────────────────────────────────────────────
ReverseDelayEditor::ReverseDelayEditor (ReverseDelayProcessor& p)
    : AudioProcessorEditor (&p),
      processorRef (p)
{
    // 1. RELAYS (must exist before the WebView) ------------------------------
    for (const auto& id : kSliderIds)
        sliderRelays.push_back (std::make_unique<juce::WebSliderRelay> (id));

    for (const auto& id : kComboIds)
        comboRelays.push_back (std::make_unique<juce::WebComboBoxRelay> (id));

    // 2. WEBVIEW options + relay registration --------------------------------
    auto options = juce::WebBrowserComponent::Options{}
        .withNativeIntegrationEnabled()
        .withKeepPageLoadedWhenBrowserIsHidden()
        .withResourceProvider ([this] (const auto& url) { return getResource (url); });

    for (const auto& relay : sliderRelays)
        options = options.withOptionsFrom (*relay);

    for (const auto& relay : comboRelays)
        options = options.withOptionsFrom (*relay);

    // ── NATIVE FUNCTIONS — exactly 11 ──────────────────────────────────────
    // 1 for dblclick-reset + the 10 that js/preset-manager.js fetches. The count
    // is grep-diffed against app.js + preset-manager.js at the Stage-4 gate: an
    // unregistered fn leaves its control silently dead while build, auval and
    // pluginval all pass (pattern_webview_native_fn_bridge_gap).

    // Dblclick-reset needs each parameter's default in ENGINEERING units: the
    // properties payload pushed to the page carries start/end/skew but no
    // default, and a hardcoded JS default table would drift from the C++
    // NormalisableRange (pattern_webview_knob_readout_scaled_value). The page
    // converts back through the same live properties, so the round-trip is exact
    // for the four skewed params.
    options = options.withNativeFunction ("getParameterDefaults",
        [this] (auto&, auto complete)
        {
            auto* obj = new juce::DynamicObject();

            for (const auto& id : kSliderIds)
            {
                if (auto* param = processorRef.parameters.getParameter (id))
                    obj->setProperty (id, param->convertFrom0to1 (param->getDefaultValue()));
            }

            complete (juce::var (obj));
        });

    // ── Preset bridge (OuariconPresetManager v1.0.5 contract) ──────────────
    options = options
        .withNativeFunction ("savePreset", [this] (const auto& args, auto complete)
        {
            if (args.size() < 1 || ! args[0].isString()) { complete (juce::var (false)); return; }
            complete (juce::var (processorRef.getPresetManager().savePreset (args[0].toString())));
        })

        .withNativeFunction ("loadPreset", [this] (const auto& args, auto complete)
        {
            if (args.size() < 1 || ! args[0].isString()) { complete (juce::var (false)); return; }
            complete (juce::var (processorRef.getPresetManager().loadPreset (args[0].toString())));
        })

        .withNativeFunction ("getPresetList", [this] (const auto&, auto complete)
        {
            juce::Array<juce::var> arr;
            for (const auto& name : processorRef.getPresetManager().getPresetList())
                arr.add (name);
            complete (juce::var (arr));
        })

        .withNativeFunction ("getCurrentPreset", [this] (const auto&, auto complete)
        {
            complete (juce::var (processorRef.getPresetManager().getCurrentPresetName()));
        })

        // Return the neighbour NAME only — preset-manager.js calls loadPreset()
        // on the result itself, so loading here as well would double-load.
        .withNativeFunction ("selectNextPreset", [this] (const auto&, auto complete)
        {
            complete (juce::var (processorRef.getPresetManager().getNextPreset()));
        })

        .withNativeFunction ("selectPreviousPreset", [this] (const auto&, auto complete)
        {
            complete (juce::var (processorRef.getPresetManager().getPreviousPreset()));
        })

        .withNativeFunction ("deletePreset", [this] (const auto& args, auto complete)
        {
            if (args.size() < 1 || ! args[0].isString()) { complete (juce::var (false)); return; }
            complete (juce::var (processorRef.getPresetManager().deletePreset (args[0].toString())));
        })

        .withNativeFunction ("isFactoryPreset", [this] (const auto& args, auto complete)
        {
            if (args.size() < 1 || ! args[0].isString()) { complete (juce::var (false)); return; }
            complete (juce::var (processorRef.getPresetManager().isFactoryPreset (args[0].toString())));
        })

        // Both dialog fns MUST resolve {success, name} — preset-manager.js checks
        // `result && result.success`, so a bare bool silently no-ops the bar.
        .withNativeFunction ("savePresetWithDialog", [this] (const auto&, auto complete)
        {
            // MSVC: the SafePointer is hoisted to a LOCAL here and captured by
            // copy below. Writing `[safeThis = SafePointer<...>(this), ...]`
            // directly in the nested launchAsync lambda (as O-Contrabass does)
            // resolves `this` to the enclosing closure on MSVC — a hard compile
            // error that Apple Clang accepts silently and that only surfaces on
            // the first Windows CI build
            // (critical_msvc_safepointer_init_capture_nested_lambda).
            auto safeThis = juce::Component::SafePointer<ReverseDelayEditor> (this);

            auto makeResult = [] (bool ok, const juce::String& name)
            {
                auto* obj = new juce::DynamicObject();
                obj->setProperty ("success", ok);
                obj->setProperty ("name", name);
                return juce::var (obj);
            };

            if (fileDialogOpen) { complete (makeResult (false, {})); return; }
            fileDialogOpen = true;

            fileChooser = std::make_shared<juce::FileChooser> (
                "Save Preset",
                processorRef.getPresetManager().getUserPresetsDirectory(),
                "*.json");

            fileChooser->launchAsync (
                juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                [safeThis, complete, makeResult] (const juce::FileChooser& fc)
                {
                    // Editor destroyed while the dialog was up: BARE return.
                    // `complete` is owned by the now-dead WebView Impl, so even
                    // calling complete(false) here is itself a use-after-free
                    // (pattern_webview_launchasync_safepointer_no_complete).
                    if (safeThis == nullptr)
                        return;

                    safeThis->fileDialogOpen = false;

                    auto result = fc.getResult();
                    if (result == juce::File{}) { complete (makeResult (false, {})); return; }

                    auto  name = result.getFileNameWithoutExtension();
                    auto& pm   = safeThis->processorRef.getPresetManager();

                    // Honour the directory the user picked — savePreset() always
                    // writes to the user-presets dir regardless (O-Wind WR-12).
                    const bool ok = result.isAChildOf (pm.getUserPresetsDirectory())
                                      ? pm.savePreset (name)
                                      : pm.savePresetToFile (result.withFileExtension ("json"));

                    complete (makeResult (ok, name));
                });
        })

        .withNativeFunction ("loadPresetFromFile", [this] (const auto&, auto complete)
        {
            auto safeThis = juce::Component::SafePointer<ReverseDelayEditor> (this);   // hoisted — see above

            auto makeResult = [] (bool ok, const juce::String& name)
            {
                auto* obj = new juce::DynamicObject();
                obj->setProperty ("success", ok);
                obj->setProperty ("name", name);
                return juce::var (obj);
            };

            if (fileDialogOpen) { complete (makeResult (false, {})); return; }
            fileDialogOpen = true;

            fileChooser = std::make_shared<juce::FileChooser> (
                "Load Preset",
                processorRef.getPresetManager().getUserPresetsDirectory(),
                "*.json");

            fileChooser->launchAsync (
                juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                [safeThis, complete, makeResult] (const juce::FileChooser& fc)
                {
                    if (safeThis == nullptr)
                        return;   // bare return — see savePresetWithDialog

                    safeThis->fileDialogOpen = false;

                    auto file = fc.getResult();
                    if (! file.existsAsFile()) { complete (makeResult (false, {})); return; }

                    const bool ok = safeThis->processorRef.getPresetManager().loadPresetFromFile (file);
                    complete (makeResult (ok, file.getFileNameWithoutExtension()));
                });
        });

   #if JUCE_WINDOWS
    // WebView2's default user-data folder is denied in most DAW hosts; a failed
    // construction falls back to the IE backend with no resource provider, which
    // presents as a blank page and no error
    // (critical_webview2_runtime_gotchas_windows).
    options = options.withWinWebView2Options (
        juce::WebBrowserComponent::Options::WinWebView2{}
            .withUserDataFolder (
                juce::File::getSpecialLocation (juce::File::tempDirectory)
                    .getChildFile ("OReverseDelay_WebView"))
            .withStatusBarDisabled()
            .withBuiltInErrorPageDisabled());
   #endif

    webView = std::make_unique<juce::WebBrowserComponent> (options);

    // 3. ATTACHMENTS (after the WebView; 3-arg ctor, nullptr undoManager) -----
    for (int i = 0; i < kSliderIds.size(); ++i)
    {
        auto* param = processorRef.parameters.getParameter (kSliderIds[i]);
        jassert (param != nullptr);   // ID drift → silently dead control
        if (param != nullptr)
            sliderAttachments.push_back (
                std::make_unique<juce::WebSliderParameterAttachment> (
                    *param, *sliderRelays[(size_t) i], nullptr));
    }

    for (int i = 0; i < kComboIds.size(); ++i)
    {
        auto* param = processorRef.parameters.getParameter (kComboIds[i]);
        jassert (param != nullptr);
        if (param != nullptr)
            comboAttachments.push_back (
                std::make_unique<juce::WebComboBoxParameterAttachment> (
                    *param, *comboRelays[(size_t) i], nullptr));
    }

    addAndMakeVisible (*webView);
    webView->goToURL (juce::WebBrowserComponent::getResourceProviderRoot());

    // Fixed. 440 (Stage 3: header + one row of four framed group panels + footer)
    // + 44 for the Stage-4 preset band. The band and the extra frame height are
    // the SAME 44 px, so .groups' flex slack — and with it panel heights and the
    // footer — are untouched. Must stay in sync with styles.css (html/body and
    // .frame both read 484px).
    setSize (940, 484);
}

ReverseDelayEditor::~ReverseDelayEditor() = default;

// ── Layout ──────────────────────────────────────────────────────────────────
void ReverseDelayEditor::paint (juce::Graphics&)
{
    // The WebView fills the editor — nothing to paint.
}

void ReverseDelayEditor::resized()
{
    if (webView != nullptr)
        webView->setBounds (getLocalBounds());
}
