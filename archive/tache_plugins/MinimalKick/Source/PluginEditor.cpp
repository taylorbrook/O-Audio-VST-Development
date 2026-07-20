#include "PluginEditor.h"
#include "BinaryData.h"

MinimalKickAudioProcessorEditor::MinimalKickAudioProcessorEditor(MinimalKickAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    // Pattern #11: Initialize in order - Relays → WebView → Attachments

    // 1. Create relays FIRST (must match APVTS parameter IDs)
    sweepRelay = std::make_unique<juce::WebSliderRelay>("sweep");
    timeRelay = std::make_unique<juce::WebSliderRelay>("time");
    attackRelay = std::make_unique<juce::WebSliderRelay>("attack");
    decayRelay = std::make_unique<juce::WebSliderRelay>("decay");
    driveRelay = std::make_unique<juce::WebSliderRelay>("drive");

    // 2. Create WebView SECOND with all relays registered
    webView = std::make_unique<juce::WebBrowserComponent>(
        juce::WebBrowserComponent::Options{}
            .withNativeIntegrationEnabled()
            .withResourceProvider([this](const auto& url) { return getResource(url); })
            .withOptionsFrom(*sweepRelay)
            .withOptionsFrom(*timeRelay)
            .withOptionsFrom(*attackRelay)
            .withOptionsFrom(*decayRelay)
            .withOptionsFrom(*driveRelay)
    );

    addAndMakeVisible(*webView);

    // 3. Create attachments LAST (Pattern #12: three parameters - parameter, relay, nullptr)
    sweepAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("sweep"), *sweepRelay, nullptr);

    timeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("time"), *timeRelay, nullptr);

    attackAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("attack"), *attackRelay, nullptr);

    decayAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("decay"), *decayRelay, nullptr);

    driveAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("drive"), *driveRelay, nullptr);

    // Set editor size (730×280px from mockup)
    setSize(730, 280);
    setResizable(false, false);

    // Load UI
    webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
}

MinimalKickAudioProcessorEditor::~MinimalKickAudioProcessorEditor()
{
}

void MinimalKickAudioProcessorEditor::paint(juce::Graphics& g)
{
    // WebView handles all painting
}

void MinimalKickAudioProcessorEditor::resized()
{
    // Fill entire editor area
    webView->setBounds(getLocalBounds());
}

std::optional<juce::WebBrowserComponent::Resource>
MinimalKickAudioProcessorEditor::getResource(const juce::String& url)
{
    // Helper lambda for creating resource vectors
    auto makeVector = [](const char* data, int size) {
        return std::vector<std::byte>(
            reinterpret_cast<const std::byte*>(data),
            reinterpret_cast<const std::byte*>(data) + size
        );
    };

    // Pattern #8: Explicit URL mapping (NO generic loops)
    if (url == "/" || url == "/index.html") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::index_html, BinaryData::index_htmlSize),
            juce::String("text/html")
        };
    }

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

    return std::nullopt;  // 404
}
