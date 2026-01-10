/*
  ==============================================================================

    OuariconSaturationModeling - Editor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginEditor.h"
#include "BinaryData.h"

OuariconSaturationModelingAudioProcessorEditor::OuariconSaturationModelingAudioProcessorEditor(OuariconSaturationModelingAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    // 1️⃣ Create relays FIRST (Pattern 11: Order critical for destruction)
    intensityRelay = std::make_unique<juce::WebSliderRelay>("INTENSITY");
    modelRelay = std::make_unique<juce::WebSliderRelay>("MODEL");
    qualityRelay = std::make_unique<juce::WebSliderRelay>("QUALITY");
    autogainRelay = std::make_unique<juce::WebToggleButtonRelay>("AUTOGAIN");

    // 2️⃣ Create WebView with options (Pattern 9: NEEDS_WEB_BROWSER required)
    webView = std::make_unique<juce::WebBrowserComponent>(
        juce::WebBrowserComponent::Options{}
            .withNativeIntegrationEnabled()  // CRITICAL: Enables JUCE JavaScript library
            .withResourceProvider([this](const auto& url) { return getResource(url); })
            .withOptionsFrom(*intensityRelay)
            .withOptionsFrom(*modelRelay)
            .withOptionsFrom(*qualityRelay)
            .withOptionsFrom(*autogainRelay)
    );

    // 3️⃣ Create attachments LAST (Pattern 12: 3 params required - parameter, relay, nullptr)
    intensityAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("INTENSITY"), *intensityRelay, nullptr);

    modelAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("MODEL"), *modelRelay, nullptr);

    qualityAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("QUALITY"), *qualityRelay, nullptr);

    autogainAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.parameters.getParameter("AUTOGAIN"), *autogainRelay, nullptr);

    // Add WebView to editor
    addAndMakeVisible(*webView);

    // Navigate to UI
    webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());

    // Set editor size (from v4 mockup: 600x450)
    setSize(600, 450);

    // Start VU meter update timer (30Hz = ~33ms)
    startTimerHz(30);
}

OuariconSaturationModelingAudioProcessorEditor::~OuariconSaturationModelingAudioProcessorEditor()
{
    // Stop timer before destruction
    stopTimer();

    // Destruction happens in reverse order (attachments → webView → relays)
    // Pattern 11: No manual cleanup needed (unique_ptr handles it)
}

void OuariconSaturationModelingAudioProcessorEditor::timerCallback()
{
    // Read levels from processor (atomic, thread-safe)
    const float inputDB = processorRef.currentInputLevel.load(std::memory_order_relaxed);
    const float outputDB = processorRef.currentOutputLevel.load(std::memory_order_relaxed);

    // Send to WebView via JavaScript
    juce::String jsCode = juce::String("if (window.receiveMeterLevels) { window.receiveMeterLevels(")
        + juce::String(inputDB, 1) + ", "
        + juce::String(outputDB, 1) + "); }";

    webView->evaluateJavascript(jsCode, nullptr);
}

void OuariconSaturationModelingAudioProcessorEditor::paint(juce::Graphics& g)
{
    // WebView handles all painting
    juce::ignoreUnused(g);
}

void OuariconSaturationModelingAudioProcessorEditor::resized()
{
    // WebView fills entire editor
    webView->setBounds(getLocalBounds());
}

// Pattern 8: Explicit URL mapping (required for WebView resource loading)
std::optional<juce::WebBrowserComponent::Resource>
OuariconSaturationModelingAudioProcessorEditor::getResource(const juce::String& url)
{
    // Helper to convert BinaryData to std::vector<std::byte>
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

    // JUCE JavaScript bridge (Pattern 13: check_native_interop.js required)
    if (url == "/js/juce/index.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::index_js, BinaryData::index_jsSize),
            juce::String("text/javascript")
        };
    }

    if (url == "/js/juce/check_native_interop.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::check_native_interop_js, BinaryData::check_native_interop_jsSize),
            juce::String("text/javascript")
        };
    }

    // Image assets (paper texture)
    if (url == "/img/paper1.jpg") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::paper1_jpg, BinaryData::paper1_jpgSize),
            juce::String("image/jpeg")
        };
    }

    // Snake images
    if (url == "/img/snake_mobot31753000317195_0068.png") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::snake_mobot31753000317195_0068_png,
                      BinaryData::snake_mobot31753000317195_0068_pngSize),
            juce::String("image/png")
        };
    }

    if (url == "/img/snake_mobot31753000317195_0070.png") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::snake_mobot31753000317195_0070_png,
                      BinaryData::snake_mobot31753000317195_0070_pngSize),
            juce::String("image/png")
        };
    }

    if (url == "/img/snake_NA_0145.png") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::snake_NA_0145_png, BinaryData::snake_NA_0145_pngSize),
            juce::String("image/png")
        };
    }

    if (url == "/img/snake_snakesaustralia00kref_0145.png") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::snake_snakesaustralia00kref_0145_png,
                      BinaryData::snake_snakesaustralia00kref_0145_pngSize),
            juce::String("image/png")
        };
    }

    // Resource not found
    juce::Logger::writeToLog("Resource not found: " + url);
    return std::nullopt;
}
