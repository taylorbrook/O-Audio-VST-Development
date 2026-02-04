/*
  ==============================================================================

    O-SpectralShaper - Editor Implementation
    Ouaricon Development
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginEditor.h"
#include "BinaryData.h"

OSpectralShaperAudioProcessorEditor::OSpectralShaperAudioProcessorEditor(
    OSpectralShaperAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    // ============================================================================
    // 1. Create relays FIRST (with parameter IDs matching APVTS)
    // ============================================================================

    mixRelay = std::make_unique<juce::WebSliderRelay>("MIX");
    attackTimeRelay = std::make_unique<juce::WebSliderRelay>("ATTACK_TIME");
    sustainTimeRelay = std::make_unique<juce::WebSliderRelay>("SUSTAIN_TIME");
    sensitivityRelay = std::make_unique<juce::WebSliderRelay>("SENSITIVITY");
    lookaheadEnabledRelay = std::make_unique<juce::WebToggleButtonRelay>("LOOKAHEAD_ENABLED");
    lookaheadTimeRelay = std::make_unique<juce::WebSliderRelay>("LOOKAHEAD_TIME");
    outputGainRelay = std::make_unique<juce::WebSliderRelay>("OUTPUT_GAIN");

    // ============================================================================
    // 2. Create WebView SECOND with all relay options registered
    // ============================================================================

    webView = std::make_unique<juce::WebBrowserComponent>(
        juce::WebBrowserComponent::Options{}
            .withNativeIntegrationEnabled()
            .withResourceProvider([this](auto& url) { return getResource(url); })
            .withOptionsFrom(*mixRelay)
            .withOptionsFrom(*attackTimeRelay)
            .withOptionsFrom(*sustainTimeRelay)
            .withOptionsFrom(*sensitivityRelay)
            .withOptionsFrom(*lookaheadEnabledRelay)
            .withOptionsFrom(*lookaheadTimeRelay)
            .withOptionsFrom(*outputGainRelay)
    );

    // ============================================================================
    // 3. Create attachments LAST (connect parameters to relays)
    // ============================================================================

    mixAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.getAPVTS().getParameter("MIX"), *mixRelay, nullptr);
    attackTimeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.getAPVTS().getParameter("ATTACK_TIME"), *attackTimeRelay, nullptr);
    sustainTimeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.getAPVTS().getParameter("SUSTAIN_TIME"), *sustainTimeRelay, nullptr);
    sensitivityAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.getAPVTS().getParameter("SENSITIVITY"), *sensitivityRelay, nullptr);
    lookaheadEnabledAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.getAPVTS().getParameter("LOOKAHEAD_ENABLED"), *lookaheadEnabledRelay, nullptr);
    lookaheadTimeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.getAPVTS().getParameter("LOOKAHEAD_TIME"), *lookaheadTimeRelay, nullptr);
    outputGainAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.getAPVTS().getParameter("OUTPUT_GAIN"), *outputGainRelay, nullptr);

    // ============================================================================
    // Add WebView to editor (navigation happens in parentHierarchyChanged)
    // ============================================================================

    addAndMakeVisible(*webView);

    // Set editor size (600x400 default for Stage 1 placeholder)
    setSize(600, 400);
}

OSpectralShaperAudioProcessorEditor::~OSpectralShaperAudioProcessorEditor()
{
}

void OSpectralShaperAudioProcessorEditor::paint(juce::Graphics& g)
{
    // WebView handles all rendering
    juce::ignoreUnused(g);
}

void OSpectralShaperAudioProcessorEditor::resized()
{
    // WebView fills entire editor
    webView->setBounds(getLocalBounds());
}

void OSpectralShaperAudioProcessorEditor::parentHierarchyChanged()
{
    // Navigate WebView only after editor is attached to a window (JUCE 8 requirement)
    // This prevents crashes during plugin scanning when no window context exists
    if (isShowing() && webView != nullptr && !hasNavigated)
    {
        webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
        hasNavigated = true;
    }
}

std::optional<juce::WebBrowserComponent::Resource>
OSpectralShaperAudioProcessorEditor::getResource(const juce::String& url)
{
    auto makeVector = [](const char* data, int size) {
        return std::vector<std::byte>(
            reinterpret_cast<const std::byte*>(data),
            reinterpret_cast<const std::byte*>(data) + size
        );
    };

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

    // JUCE interop checker
    if (url == "/js/juce/check_native_interop.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::check_native_interop_js,
                      BinaryData::check_native_interop_jsSize),
            juce::String("text/javascript")
        };
    }

    // Resource not found
    juce::Logger::writeToLog("Resource not found: " + url);
    return std::nullopt;
}
