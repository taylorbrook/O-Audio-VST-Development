#include "BenchEditor.h"
#include "BenchScript.h"
#include "UIBinaryData.h"

namespace
{
    double nowMs() { return juce::Time::getMillisecondCounterHiRes(); }

    auto makeBinaryResource (const char* data, int size, const char* mimeType)
        -> std::optional<juce::WebBrowserComponent::Resource>
    {
        auto* bytes = reinterpret_cast<const std::byte*> (data);
        return juce::WebBrowserComponent::Resource { std::vector<std::byte> (bytes, bytes + size), juce::String (mimeType) };
    }

    juce::var presetResult (bool success, const juce::String& name)
    {
        auto* obj = new juce::DynamicObject();
        obj->setProperty ("success", success);
        obj->setProperty ("name", name);
        return juce::var (obj);
    }
}

BenchEditor::BenchEditor (OuariconParametricEQAudioProcessor& p, bool fallbackIn, BenchCallback cb)
    : AudioProcessorEditor (&p), audioProcessor (p), fallback (fallbackIn), onBench (std::move (cb))
{
    const double t0 = nowMs();
    const int nBands = fallback ? 1 : opeq::kNumBands;

    // 1. RELAYS
    bypassRelay     = std::make_unique<juce::WebToggleButtonRelay> ("bypass");
    outputGainRelay = std::make_unique<juce::WebSliderRelay>       ("output_gain");
    gainScaleRelay  = std::make_unique<juce::WebSliderRelay>       ("gain_scale");
    autoGainRelay   = std::make_unique<juce::WebToggleButtonRelay> ("auto_gain");
    for (int b = 0; b < nBands; ++b)
        bandRelays[(size_t) b] = std::make_unique<opeq::BandRelays> (opeq::bandPrefix (b));
    timings.numRelays = 4 + 8 * nBands;

    const double t1 = nowMs();

    // 2. WEBVIEW options
    auto options = juce::WebBrowserComponent::Options{}
        .withNativeIntegrationEnabled()
        .withResourceProvider ([this] (const auto& url) { return getResource (url); })
        .withKeepPageLoadedWhenBrowserIsHidden()
        .withOptionsFrom (*bypassRelay)
        .withOptionsFrom (*outputGainRelay)
        .withOptionsFrom (*gainScaleRelay)
        .withOptionsFrom (*autoGainRelay);

    for (int b = 0; b < nBands; ++b)
        options = bandRelays[(size_t) b]->addTo (options);

    options = addNativeFunctions (options);

    // Bench instrumentation (page stays unmodified)
    options = options.withEventListener ("__bench", [this] (const juce::var& v)
    {
        const double t = nowMs();
        if (onBench) onBench (v, t);
    });
    options = options.withUserScript (juce::String (kBenchScript).replace ("__EXPECTED__", juce::String (timings.numRelays)));

    // Size of what JUCE will inject (replicates juce_WebBrowserComponent.cpp:393-409)
    {
        juce::StringArray ids;
        for (auto& [id, fn] : options.getEventListeners()) ids.add (id.toString().quoted());
        ids.add (juce::String ("__juce__invoke").quoted());   // added by the native-functions provider
        const auto line = "window.__JUCE__.initialisationData.__juce__registeredGlobalEventIds = [" + ids.joinIntoString (",") + "];";
        timings.numEventListeners = ids.size();
        timings.estEventIdScriptBytes = (int64_t) line.getNumBytesAsUTF8() * ids.size();

        int64_t initBytes = 0;
        for (auto& [k, v] : options.getInitialisationData()) initBytes += juce::JSON::toString (v).getNumBytesAsUTF8() + 1;
        timings.estInitDataBytes = initBytes;
        int64_t userBytes = 0;
        for (auto& s : options.getUserScripts()) userBytes += s.getNumBytesAsUTF8();
        timings.estUserScriptBytes = timings.estEventIdScriptBytes + initBytes + userBytes;
    }

    const double t2 = nowMs();
    webView = std::make_unique<juce::WebBrowserComponent> (options);
    const double t3 = nowMs();

    // 3. ATTACHMENTS
    auto& apvts = audioProcessor.parameters;
    bypassAttachment     = std::make_unique<juce::WebToggleButtonParameterAttachment> (*apvts.getParameter ("bypass"), *bypassRelay, nullptr);
    outputGainAttachment = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter ("output_gain"), *outputGainRelay, nullptr);
    gainScaleAttachment  = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter ("gain_scale"), *gainScaleRelay, nullptr);
    autoGainAttachment   = std::make_unique<juce::WebToggleButtonParameterAttachment> (*apvts.getParameter ("auto_gain"), *autoGainRelay, nullptr);
    for (int b = 0; b < nBands; ++b)
        bandAttachments[(size_t) b] = std::make_unique<opeq::BandAttachments> (apvts, opeq::bandPrefix (b), *bandRelays[(size_t) b]);

    const double t4 = nowMs();

    addAndMakeVisible (*webView);
    webView->goToURL (juce::WebBrowserComponent::getResourceProviderRoot());

    setResizable (true, true);
    setResizeLimits (800, 500, 1600, 1000);
    setSize (960, 600);

    audioProcessor.setAnalyzerConsumer (true);
    startTimerHz (30);

    const double t5 = nowMs();
    timings.relaysMs  = t1 - t0;
    timings.optionsMs = t2 - t1;
    timings.webViewMs = t3 - t2;
    timings.attachMs  = t4 - t3;
    timings.restMs    = t5 - t4;
    timings.totalMs   = t5 - t0;
}

BenchEditor::~BenchEditor()
{
    stopTimer();
    audioProcessor.setAnalyzerConsumer (false);
    audioProcessor.listenBand.store (-1);
}

void BenchEditor::resized()
{
    if (webView != nullptr) webView->setBounds (getLocalBounds());
    audioProcessor.uiEditorWidth  = getWidth();
    audioProcessor.uiEditorHeight = getHeight();
}

void BenchEditor::timerCallback()
{
    if (webView == nullptr) return;

    // Output meter, template-style atomic + event (vu-meter fallback path)
    const float peak = juce::jmax (audioProcessor.readAndResetOutputPeak (0), audioProcessor.readAndResetOutputPeak (1));
    webView->emitEventIfBrowserIsVisible ("outputLevel", juce::Decibels::gainToDecibels (peak, -100.0f));

    // Response display packets: changed bands only (all after requestEqSections)
    auto changed = displayModel.poll();
    if (fullPush)
    {
        changed.clear();
        for (int b = 0; b < DisplayModel::numBands; ++b) changed.add (b);
        fullPush = false;
    }

    if (! changed.isEmpty())
    {
        auto header = displayModel.headerVar();
        auto* obj = new juce::DynamicObject();
        obj->setProperty ("sampleRate", header["sampleRate"]);
        juce::Array<juce::var> bands;
        for (int b : changed)
        {
            auto pk = displayModel.packetFor (b);
            if (auto* d = pk.getDynamicObject())
            {
                auto clone = d->clone();
                clone->setProperty ("band", b + 1);   // page is 1-based
                bands.add (juce::var (clone.release()));
            }
        }
        obj->setProperty ("bands", bands);
        webView->emitEventIfBrowserIsVisible ("eqSections", juce::var (obj));
    }
}

juce::WebBrowserComponent::Options BenchEditor::addNativeFunctions (juce::WebBrowserComponent::Options options)
{
    auto count = [this] (const char* n) { ++nativeCalls[n]; };

    options = options.withNativeFunction ("getParameterDefaults", [this, count] (auto&, auto complete)
    {
        count ("getParameterDefaults");
        auto* obj = new juce::DynamicObject();
        juce::StringArray ids { "output_gain", "gain_scale" };
        for (int b = 0; b < opeq::kNumBands; ++b)
            for (auto k : { "freq", "gain", "q" }) ids.add (opeq::bandPrefix (b) + k);
        for (auto& id : ids)
            if (auto* param = audioProcessor.parameters.getParameter (id))
                obj->setProperty (id, param->convertFrom0to1 (param->getDefaultValue()));
        complete (juce::var (obj));
    });
    options = options.withNativeFunction ("getUiLanguage", [this, count] (auto&, auto complete)
    { count ("getUiLanguage"); complete (juce::var (OuariconParametricEQAudioProcessor::languageCode (audioProcessor.uiLanguage.load()))); });
    options = options.withNativeFunction ("setUiLanguage", [count] (auto&, auto complete) { count ("setUiLanguage"); complete (juce::var()); });
    options = options.withNativeFunction ("getUiState", [this, count] (auto&, auto complete)
    {
        count ("getUiState");
        auto* obj = new juce::DynamicObject();
        obj->setProperty ("analyzerMode", audioProcessor.analyzerMode.load());
        obj->setProperty ("displayRange", audioProcessor.uiDisplayRange.load());
        obj->setProperty ("selectedBand", audioProcessor.uiSelectedBand.load());
        complete (juce::var (obj));
    });
    options = options.withNativeFunction ("setUiState", [count] (auto&, auto complete) { count ("setUiState"); complete (juce::var()); });
    options = options.withNativeFunction ("setListenBand", [count] (auto&, auto complete) { count ("setListenBand"); complete (juce::var()); });
    options = options.withNativeFunction ("requestEqSections", [this, count] (auto&, auto complete)
    { count ("requestEqSections"); fullPush = true; complete (juce::var()); });
    options = options.withNativeFunction ("resizeEditor", [count] (auto&, auto complete) { count ("resizeEditor"); complete (juce::var()); });
    options = options.withNativeFunction ("savePreset", [count] (auto&, auto complete) { count ("savePreset"); complete (false); });
    options = options.withNativeFunction ("savePresetWithDialog", [count] (auto&, auto complete) { count ("savePresetWithDialog"); complete (presetResult (false, {})); });
    options = options.withNativeFunction ("loadPreset", [count] (auto&, auto complete) { count ("loadPreset"); complete (false); });
    options = options.withNativeFunction ("loadPresetFromFile", [count] (auto&, auto complete) { count ("loadPresetFromFile"); complete (presetResult (false, {})); });
    options = options.withNativeFunction ("getPresetList", [count] (auto&, auto complete) { count ("getPresetList"); complete (juce::var (juce::Array<juce::var> { "Default" })); });
    options = options.withNativeFunction ("getCurrentPreset", [count] (auto&, auto complete) { count ("getCurrentPreset"); complete (juce::var ("Default")); });
    options = options.withNativeFunction ("selectNextPreset", [count] (auto&, auto complete) { count ("selectNextPreset"); complete (juce::var ("Default")); });
    options = options.withNativeFunction ("selectPreviousPreset", [count] (auto&, auto complete) { count ("selectPreviousPreset"); complete (juce::var ("Default")); });
    options = options.withNativeFunction ("deletePreset", [count] (auto&, auto complete) { count ("deletePreset"); complete (false); });
    options = options.withNativeFunction ("isFactoryPreset", [count] (auto&, auto complete) { count ("isFactoryPreset"); complete (true); });
    return options;
}

std::optional<juce::WebBrowserComponent::Resource> BenchEditor::getResource (const juce::String& url)
{
    ++resourceRequests;
    const char* js = "application/javascript; charset=utf-8";
    if (url == "/" || url == "/index.html")      return makeBinaryResource (UIBinaryData::index_html, UIBinaryData::index_htmlSize, "text/html; charset=utf-8");
    if (url == "/js/i18n.js")                    return makeBinaryResource (UIBinaryData::i18n_js, UIBinaryData::i18n_jsSize, js);
    if (url == "/js/juce/index.js")              return makeBinaryResource (UIBinaryData::index_js, UIBinaryData::index_jsSize, js);
    if (url == "/js/juce/check_native_interop.js") return makeBinaryResource (UIBinaryData::check_native_interop_js, UIBinaryData::check_native_interop_jsSize, js);
    if (url == "/modules/preset-manager.js")     return makeBinaryResource (UIBinaryData::presetmanager_js, UIBinaryData::presetmanager_jsSize, js);
    if (url == "/modules/vu-meter.js")           return makeBinaryResource (UIBinaryData::vumeter_js, UIBinaryData::vumeter_jsSize, js);
    if (url == "/css/eb-garamond.css")           return makeBinaryResource (UIBinaryData::ebgaramond_css, UIBinaryData::ebgaramond_cssSize, "text/css; charset=utf-8");
    if (url == "/fonts/EBGaramond-Regular.woff2") return makeBinaryResource (UIBinaryData::EBGaramondRegular_woff2, UIBinaryData::EBGaramondRegular_woff2Size, "font/woff2");
    if (url == "/fonts/EBGaramond-Italic.woff2") return makeBinaryResource (UIBinaryData::EBGaramondItalic_woff2, UIBinaryData::EBGaramondItalic_woff2Size, "font/woff2");
    if (url == "/fonts/EBGaramond-Bold.woff2")   return makeBinaryResource (UIBinaryData::EBGaramondBold_woff2, UIBinaryData::EBGaramondBold_woff2Size, "font/woff2");
    missingResources.addIfNotAlreadyThere (url);
    return std::nullopt;
}
