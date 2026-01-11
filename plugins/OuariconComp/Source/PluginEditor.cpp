/*
  ==============================================================================

    OuariconComp - Editor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

    Stage 3 (GUI) - WebView UI Implementation

  ==============================================================================
*/

#include "PluginEditor.h"
#include "BinaryData.h"

//==============================================================================
OuariconCompAudioProcessorEditor::OuariconCompAudioProcessorEditor(OuariconCompAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    //==========================================================================
    // CRITICAL: Initialization Order
    // 1. Create relays FIRST
    // 2. Create WebView with relay options
    // 3. Create attachments LAST
    //==========================================================================

    // 1️⃣ Create relays FIRST (no dependencies)
    thresholdRelay = std::make_unique<juce::WebSliderRelay>("threshold");
    ratioRelay = std::make_unique<juce::WebSliderRelay>("ratio");
    attackTimeRelay = std::make_unique<juce::WebSliderRelay>("attack_time");
    releaseTimeRelay = std::make_unique<juce::WebSliderRelay>("release_time");
    kneeRelay = std::make_unique<juce::WebSliderRelay>("knee");
    outputGainRelay = std::make_unique<juce::WebSliderRelay>("output_gain");
    autoGainRelay = std::make_unique<juce::WebToggleButtonRelay>("auto_gain");

    // 2️⃣ Create WebView with relay options (depends on relays)
    webView = std::make_unique<juce::WebBrowserComponent>(
        juce::WebBrowserComponent::Options{}
            .withNativeIntegrationEnabled()
            .withResourceProvider([this](const auto& url) { return getResource(url); })
            .withOptionsFrom(*thresholdRelay)
            .withOptionsFrom(*ratioRelay)
            .withOptionsFrom(*attackTimeRelay)
            .withOptionsFrom(*releaseTimeRelay)
            .withOptionsFrom(*kneeRelay)
            .withOptionsFrom(*outputGainRelay)
            .withOptionsFrom(*autoGainRelay)
    );

    addAndMakeVisible(*webView);

    // 3️⃣ Create attachments LAST (depend on relays and webView)
    // CRITICAL: JUCE 8 requires THREE parameters (parameter, relay, undoManager)
    // Missing nullptr causes silent failure - knobs compile but don't respond
    thresholdAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("threshold"), *thresholdRelay, nullptr);
    ratioAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("ratio"), *ratioRelay, nullptr);
    attackTimeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("attack_time"), *attackTimeRelay, nullptr);
    releaseTimeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("release_time"), *releaseTimeRelay, nullptr);
    kneeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("knee"), *kneeRelay, nullptr);
    outputGainAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("output_gain"), *outputGainRelay, nullptr);
    autoGainAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.parameters.getParameter("auto_gain"), *autoGainRelay, nullptr);

    // Load UI from resource provider
    webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());

    // Set window size from mockup dimensions (620x360px)
    setSize(620, 360);

    // Start meter update timer (30fps = ~33ms)
    startTimerHz(30);
}

OuariconCompAudioProcessorEditor::~OuariconCompAudioProcessorEditor()
{
    // Stop timer before destruction
    stopTimer();

    // Members destroyed in REVERSE order of declaration:
    // 1. Attachments destroyed FIRST (can safely call webView methods)
    // 2. WebView destroyed SECOND (attachments are gone)
    // 3. Relays destroyed LAST (nothing using them)
}

//==============================================================================
void OuariconCompAudioProcessorEditor::paint(juce::Graphics& g)
{
    // WebView handles all rendering
    g.fillAll(juce::Colours::black);
}

void OuariconCompAudioProcessorEditor::resized()
{
    // WebView fills entire editor window
    webView->setBounds(getLocalBounds());
}

//==============================================================================
void OuariconCompAudioProcessorEditor::timerCallback()
{
    // Get meter values from processor (thread-safe atomic reads)
    float inputLevel = processorRef.getInputLevelDB();
    float outputLevel = processorRef.getOutputLevelDB();
    float gainReduction = processorRef.getGainReductionDB();
    float envelope = processorRef.getEnvelopeDB();

    // Send meter data to WebView via JavaScript
    juce::String script = juce::String::formatted(
        "if (typeof updateMeters === 'function') { updateMeters(%f, %f, %f, %f); }",
        inputLevel, outputLevel, gainReduction, envelope
    );

    webView->evaluateJavascript(script, nullptr);
}

//==============================================================================
std::optional<juce::WebBrowserComponent::Resource>
OuariconCompAudioProcessorEditor::getResource(const juce::String& url)
{
    // Helper to convert BinaryData to byte vector
    auto makeVector = [](const char* data, int size) {
        return std::vector<std::byte>(
            reinterpret_cast<const std::byte*>(data),
            reinterpret_cast<const std::byte*>(data) + size
        );
    };

    //==========================================================================
    // CRITICAL: Explicit URL Mapping
    // BinaryData flattens paths (index.js → index_js)
    // HTML requests use original paths (./js/juce/index.js)
    // Must map manually with correct MIME types
    //==========================================================================

    if (url == "/" || url == "/index.html") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::index_html, BinaryData::index_htmlSize),
            juce::String("text/html")
        };
    }

    if (url == "/js/juce/index.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::index_js, BinaryData::index_jsSize),
            juce::String("application/javascript")
        };
    }

    if (url == "/js/juce/check_native_interop.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::check_native_interop_js, BinaryData::check_native_interop_jsSize),
            juce::String("application/javascript")
        };
    }

    if (url == "/paper-bg.jpg") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::paperbg_jpg, BinaryData::paperbg_jpgSize),
            juce::String("image/jpeg")
        };
    }

    if (url == "/shell.png") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::shell_png, BinaryData::shell_pngSize),
            juce::String("image/png")
        };
    }

    // 404 - Resource not found
    juce::Logger::writeToLog("Resource not found: " + url);
    return std::nullopt;
}
