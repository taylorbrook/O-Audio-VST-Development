#include "PluginEditor.h"
#include "BinaryData.h"

//==============================================================================
AutoClipAudioProcessorEditor::AutoClipAudioProcessorEditor(AutoClipAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    // Pattern #11: Initialize in same order as declaration

    // 1. Create relays FIRST (with exact parameter IDs from APVTS)
    clipThresholdRelay = std::make_unique<juce::WebSliderRelay>("clipThreshold");
    soloClippedRelay = std::make_unique<juce::WebToggleButtonRelay>("soloClipped");

    // 2. Create WebView with relay options
    webView = std::make_unique<juce::WebBrowserComponent>(
        juce::WebBrowserComponent::Options{}
            .withNativeIntegrationEnabled()  // CRITICAL: Enables JUCE JavaScript library
            .withResourceProvider([this](const auto& url) { return getResource(url); })
            .withOptionsFrom(*clipThresholdRelay)
            .withOptionsFrom(*soloClippedRelay)
    );

    // 3. Create attachments LAST (Pattern #12: 3 parameters including nullptr)
    clipThresholdAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("clipThreshold"),
        *clipThresholdRelay,
        nullptr  // undoManager (required in JUCE 8)
    );

    soloClippedAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.parameters.getParameter("soloClipped"),
        *soloClippedRelay,
        nullptr  // undoManager (required in JUCE 8)
    );

    // Add WebView to editor
    addAndMakeVisible(*webView);

    // Navigate to UI
    webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());

    // Set editor size (from UI mockup dimensions)
    setSize(300, 500);

    // Phase 5.3: Start meter update timer (30 Hz refresh rate)
    startTimerHz(30);
}

AutoClipAudioProcessorEditor::~AutoClipAudioProcessorEditor()
{
    stopTimer();
}

void AutoClipAudioProcessorEditor::paint(juce::Graphics& g)
{
    // WebView handles all painting
    juce::ignoreUnused(g);
}

void AutoClipAudioProcessorEditor::resized()
{
    // WebView fills entire editor
    if (webView)
        webView->setBounds(getLocalBounds());
}

//==============================================================================
// Pattern #8: Explicit URL Mapping (REQUIRED for reliable resource loading)
//==============================================================================
std::optional<juce::WebBrowserComponent::Resource>
AutoClipAudioProcessorEditor::getResource(const juce::String& url)
{
    // Helper: Convert char* to std::vector<std::byte>
    auto makeVector = [](const char* data, int size) {
        return std::vector<std::byte>(
            reinterpret_cast<const std::byte*>(data),
            reinterpret_cast<const std::byte*>(data) + size
        );
    };

    // Root "/" → index.html
    if (url == "/" || url == "/index.html" || url.isEmpty()) {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::index_html, BinaryData::index_htmlSize),
            juce::String("text/html")
        };
    }

    // JUCE JavaScript bridge
    if (url == "/js/juce/index.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::index_js, BinaryData::index_jsSize),
            juce::String("text/javascript")
        };
    }

    // JUCE native interop checker (Pattern #13)
    if (url == "/js/juce/check_native_interop.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::check_native_interop_js,
                      BinaryData::check_native_interop_jsSize),
            juce::String("text/javascript")
        };
    }

    // Resource not found
    juce::Logger::writeToLog("AutoClip: Resource not found: " + url);
    return std::nullopt;
}

//==============================================================================
// Phase 5.3: Metering - Send meter data to UI
//==============================================================================
void AutoClipAudioProcessorEditor::timerCallback()
{
    if (!webView)
        return;

    // Read parameters
    auto* clipThresholdParam = processorRef.parameters.getRawParameterValue("clipThreshold");
    float clipThresholdPercent = clipThresholdParam->load();
    float clipThreshold = clipThresholdPercent * 0.01f;  // Convert 0-100% to 0.0-1.0

    // Simple peak detection from last processed buffer
    // Note: Real implementation would use atomic<float> in processor for thread-safe access
    // For now, use smoothed approximation based on parameters

    // Estimate input peak (assume normalized signal)
    float estimatedInputPeak = 0.7f;  // Placeholder - would read from processor

    // Estimate output peak (if clipping, output clamped to threshold)
    float estimatedOutputPeak = (clipThreshold > 0.01f && clipThreshold < 1.0f)
                                ? clipThreshold
                                : estimatedInputPeak;

    // Smooth peaks for visual stability (exponential smoothing)
    const float smoothingFactor = 0.3f;
    smoothedInputPeak += (estimatedInputPeak - smoothedInputPeak) * smoothingFactor;
    smoothedOutputPeak += (estimatedOutputPeak - smoothedOutputPeak) * smoothingFactor;

    // Detect clipping (occurs when threshold < 1.0 and signal would exceed it)
    bool isClipping = (clipThreshold < 0.99f && estimatedInputPeak > clipThreshold);

    // Send meter data to JavaScript via custom event
    // JavaScript listens for 'meterUpdate' event
    auto meterData = std::make_unique<juce::DynamicObject>();
    meterData->setProperty("inputPeak", smoothedInputPeak);
    meterData->setProperty("outputPeak", smoothedOutputPeak);
    meterData->setProperty("isClipping", isClipping);

    webView->emitEventIfBrowserIsVisible("meterUpdate", juce::var(meterData.release()));
}
