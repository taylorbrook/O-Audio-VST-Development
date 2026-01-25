/*
  ==============================================================================

    OBass - WebView Editor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

    WebView-based editor with botanical aesthetic and JUCE 8 parameter binding.

  ==============================================================================
*/

#include "PluginEditor.h"
#include "BinaryData.h"

OBassAudioProcessorEditor::OBassAudioProcessorEditor(OBassAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    // 1. Create relays FIRST (with IDs matching HTML getSliderState/getToggleState calls)
    frequencyRelay = std::make_unique<juce::WebSliderRelay>("crossover_freq");
    enhanceRelay = std::make_unique<juce::WebSliderRelay>("enhance");
    outputRelay = std::make_unique<juce::WebSliderRelay>("output");
    modeRelay = std::make_unique<juce::WebToggleButtonRelay>("mode");

    // 2. Create WebView SECOND with relay options and native functions
    webView = std::make_unique<juce::WebBrowserComponent>(
        juce::WebBrowserComponent::Options{}
            .withNativeIntegrationEnabled()
            .withResourceProvider([this](auto& url) { return getResource(url); })
            .withOptionsFrom(*frequencyRelay)
            .withOptionsFrom(*enhanceRelay)
            .withOptionsFrom(*outputRelay)
            .withOptionsFrom(*modeRelay)
            // Limit indicator native function for UI polling
            .withNativeFunction("getLimitIndicator", [this](auto&, auto complete) {
                complete(processorRef.getLimitIndicator());
            })
    );

    // 3. Create attachments LAST (Pattern #12: 3-param constructor - parameter, relay, nullptr)
    frequencyAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("crossover_freq"), *frequencyRelay, nullptr);
    enhanceAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("enhance"), *enhanceRelay, nullptr);
    outputAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("output"), *outputRelay, nullptr);
    modeAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.parameters.getParameter("enhanceMode"), *modeRelay, nullptr);

    // Add WebView (navigation happens in parentHierarchyChanged)
    addAndMakeVisible(*webView);

    // Set size per CONTEXT.md specification
    setSize(500, 450);
}

OBassAudioProcessorEditor::~OBassAudioProcessorEditor()
{
    // Members destroyed in REVERSE order of declaration (automatic RAII cleanup)
}

void OBassAudioProcessorEditor::paint(juce::Graphics& g)
{
    // WebView handles all rendering
    juce::ignoreUnused(g);
}

void OBassAudioProcessorEditor::resized()
{
    // WebView fills entire editor
    webView->setBounds(getLocalBounds());
}

void OBassAudioProcessorEditor::parentHierarchyChanged()
{
    // Navigate WebView only after editor is attached to a window (JUCE 8 requirement)
    // This prevents crashes during plugin scanning when no window context exists
    // Use per-instance hasNavigated to allow GUI reload on editor reopen
    if (isShowing() && webView != nullptr && !hasNavigated)
    {
        webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
        hasNavigated = true;
    }
}

// Pattern #8: EXPLICIT URL MAPPING (never use generic loops)
std::optional<juce::WebBrowserComponent::Resource>
OBassAudioProcessorEditor::getResource(const juce::String& url)
{
    auto makeVector = [](const char* data, int size) {
        return std::vector<std::byte>(
            reinterpret_cast<const std::byte*>(data),
            reinterpret_cast<const std::byte*>(data) + size
        );
    };

    // Root "/" -> index.html
    if (url == "/" || url == "/index.html") {
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

    // JUCE interop checker (Pattern #13: REQUIRED for native function support)
    if (url == "/js/juce/check_native_interop.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::check_native_interop_js,
                      BinaryData::check_native_interop_jsSize),
            juce::String("text/javascript")
        };
    }

    // Background paper texture
    if (url == "/img/paper.jpg") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::paper_jpg, BinaryData::paper_jpgSize),
            juce::String("image/jpeg")
        };
    }

    // Botanical overlay image
    if (url == "/img/botanical.png") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::botanical_png, BinaryData::botanical_pngSize),
            juce::String("image/png")
        };
    }

    // Resource not found
    juce::Logger::writeToLog("O-Bass: Resource not found: " + url);
    return std::nullopt;
}
