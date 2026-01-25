/*
  ==============================================================================

    O-MultiBandCompressor - Editor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

    Phase 5.1: WebView integration with layout structure

  ==============================================================================
*/

#include "PluginEditor.h"
#include "BinaryData.h"

OMultiBandCompressorAudioProcessorEditor::OMultiBandCompressorAudioProcessorEditor(OMultiBandCompressorAudioProcessor& p)
    : AudioProcessorEditor(&p)
    , processorRef(p)
{
    // Phase 5.1: Initialize WebView without parameter bindings
    // Parameter relays and attachments will be added in Phase 5.2

    // Create WebView with options
    webView = std::make_unique<juce::WebBrowserComponent>(
        juce::WebBrowserComponent::Options{}
            .withNativeIntegrationEnabled()
            .withResourceProvider([this](const auto& url) { return getResource(url); })
            // Phase 5.2: .withOptionsFrom() calls for parameter relays will be added here
    );

    addAndMakeVisible(*webView);

    // Set window size
    setSize(900, 600);

    // Note: Navigation happens in parentHierarchyChanged() for JUCE 8 compatibility
}

OMultiBandCompressorAudioProcessorEditor::~OMultiBandCompressorAudioProcessorEditor()
{
    // WebView automatically cleaned up by unique_ptr
}

void OMultiBandCompressorAudioProcessorEditor::paint(juce::Graphics& g)
{
    // WebView handles all rendering
    juce::ignoreUnused(g);
}

void OMultiBandCompressorAudioProcessorEditor::resized()
{
    // WebView fills entire editor
    if (webView)
        webView->setBounds(getLocalBounds());
}

void OMultiBandCompressorAudioProcessorEditor::parentHierarchyChanged()
{
    // Navigate to UI on first hierarchy change (JUCE 8 pattern)
    if (!hasNavigated && isShowing())
    {
        webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
        hasNavigated = true;
    }
}

std::optional<juce::WebBrowserComponent::Resource>
OMultiBandCompressorAudioProcessorEditor::getResource(const juce::String& url)
{
    // Map URLs to embedded resources from BinaryData
    auto resource = url.replaceCharacter('\\', '/');

    // Root "/" → index.html
    if (resource == "/" || resource.isEmpty())
        resource = "/index.html";

    // Remove leading slash for BinaryData lookup
    auto path = resource.substring(1);

    // Find in binary data (files embedded from ui/public/)
    for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
    {
        if (path == BinaryData::namedResourceList[i])
        {
            int dataSize = 0;
            const char* data = BinaryData::getNamedResource(
                BinaryData::namedResourceList[i], dataSize);

            // Determine MIME type
            juce::String mimeType = "text/html";
            if (path.endsWith(".css")) mimeType = "text/css";
            if (path.endsWith(".js")) mimeType = "application/javascript";
            if (path.endsWith(".png")) mimeType = "image/png";
            if (path.endsWith(".jpg") || path.endsWith(".jpeg")) mimeType = "image/jpeg";
            if (path.endsWith(".svg")) mimeType = "image/svg+xml";

            // Convert char* to std::vector<std::byte> for JUCE 8
            std::vector<std::byte> dataVector(dataSize);
            std::memcpy(dataVector.data(), data, dataSize);

            return juce::WebBrowserComponent::Resource{
                std::move(dataVector), mimeType
            };
        }
    }

    // Resource not found
    juce::Logger::writeToLog("Resource not found: " + url);
    return std::nullopt;
}
