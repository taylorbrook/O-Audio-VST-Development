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
    if (url == "/" || url == "/index.html")
        return makeBinaryResource (BinaryData::index_html, BinaryData::index_htmlSize, "text/html");

    if (url == "/css/styles.css")
        return makeBinaryResource (BinaryData::styles_css, BinaryData::styles_cssSize, "text/css");

    if (url == "/js/app.js")
        return makeBinaryResource (BinaryData::app_js, BinaryData::app_jsSize, "application/javascript");

    if (url == "/js/juce/index.js")
        return makeBinaryResource (BinaryData::index_js, BinaryData::index_jsSize, "application/javascript");

    if (url == "/js/juce/check_native_interop.js")
        return makeBinaryResource (BinaryData::check_native_interop_js,
                                   BinaryData::check_native_interop_jsSize, "application/javascript");

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

    setSize (760, 720);
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

    const auto& spec  = vizAnalyzer.getSpectrum();   // 256 dB bins, ~[-100, 0]
    const auto& scope = vizAnalyzer.getScope();       // 128 pts, [-1, 1]

    juce::Array<juce::var> specArr, scopeArr;
    specArr.ensureStorageAllocated ((int) spec.size());
    scopeArr.ensureStorageAllocated ((int) scope.size());
    for (float v : spec)  specArr.add (v);
    for (float v : scope) scopeArr.add (v);

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
