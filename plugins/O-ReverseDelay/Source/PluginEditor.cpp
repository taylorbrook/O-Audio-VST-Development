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

    // ── The ONLY native function ───────────────────────────────────────────
    // Dblclick-reset needs each parameter's default in ENGINEERING units: the
    // properties payload pushed to the page carries start/end/skew but no
    // default, and a hardcoded JS default table would drift from the C++
    // NormalisableRange (pattern_webview_knob_readout_scaled_value). The page
    // converts back through the same live properties, so the round-trip is exact
    // for the four skewed params.
    //
    // Keeping the surface at exactly one function makes the JS-vs-C++ bridge
    // grep-diff trivially verifiable (pattern_webview_native_fn_bridge_gap).
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

    setSize (940, 440);   // fixed: one row of four framed group panels
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
