/*
  ==============================================================================

    Ouaricon Polystutter - Editor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginEditor.h"
#include "BinaryData.h"

OuariconPolystutterAudioProcessorEditor::OuariconPolystutterAudioProcessorEditor(OuariconPolystutterAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    // Phase 3.1: Create WebView for static layout rendering
    // Parameter bindings will be added in Phase 3.2
    webView = std::make_unique<juce::WebBrowserComponent>(
        juce::WebBrowserComponent::Options{}
            .withNativeIntegrationEnabled()  // CRITICAL: Enables JUCE JavaScript library
            .withResourceProvider([this](const auto& url) { return getResource(url); })
    );

    addAndMakeVisible(*webView);

    // Navigate to UI (served from BinaryData)
    webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());

    // Fixed window size: 1000×750px (from v5-ui.yaml spec)
    setSize(1000, 750);

    DBG("[Phase 3.1] WebView initialized - static layout only");
}

OuariconPolystutterAudioProcessorEditor::~OuariconPolystutterAudioProcessorEditor()
{
}

void OuariconPolystutterAudioProcessorEditor::paint(juce::Graphics& g)
{
    // WebView handles all rendering (no custom paint needed)
    juce::ignoreUnused(g);
}

void OuariconPolystutterAudioProcessorEditor::resized()
{
    // WebView fills entire editor window
    if (webView)
        webView->setBounds(getLocalBounds());
}

std::optional<juce::WebBrowserComponent::Resource>
OuariconPolystutterAudioProcessorEditor::getResource(const juce::String& url)
{
    // Helper lambda for converting char* to std::vector<std::byte>
    auto makeVector = [](const char* data, int size) {
        return std::vector<std::byte>(
            reinterpret_cast<const std::byte*>(data),
            reinterpret_cast<const std::byte*>(data) + size
        );
    };

    // Pattern #5: Explicit URL mapping (prevents 404 errors)
    // Map URLs to embedded resources from BinaryData

    // Root "/" → index.html
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

    if (url == "/js/juce/check_native_interop.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::check_native_interop_js, BinaryData::check_native_interop_jsSize),
            juce::String("text/javascript")
        };
    }

    // Images (paper background and botanical overlay)
    if (url == "/img/paper-background.jpg") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::paperbackground_jpg, BinaryData::paperbackground_jpgSize),
            juce::String("image/jpeg")
        };
    }

    if (url == "/img/botanical-bug.png") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::botanicalbug_png, BinaryData::botanicalbug_pngSize),
            juce::String("image/png")
        };
    }

    // Resource not found - log for debugging
    DBG("[Phase 3.1] Resource not found: " + url);
    return std::nullopt;
}
