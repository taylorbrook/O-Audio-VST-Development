/*
  ==============================================================================

    Ouaricon Marimba - Editor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginEditor.h"
#include "BinaryData.h"

MicroMarimbaAudioProcessorEditor::MicroMarimbaAudioProcessorEditor(MicroMarimbaAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    // 1️⃣ Create relays FIRST (Pattern 11: Order critical for destruction)
    malletHardnessRelay = std::make_unique<juce::WebSliderRelay>("MALLET_HARDNESS");
    barMaterialRelay = std::make_unique<juce::WebSliderRelay>("BAR_MATERIAL");
    resonanceRelay = std::make_unique<juce::WebSliderRelay>("RESONANCE");
    tuningModeRelay = std::make_unique<juce::WebSliderRelay>("TUNING_MODE");
    referencePitchRelay = std::make_unique<juce::WebSliderRelay>("REFERENCE_PITCH");
    velCurveRelay = std::make_unique<juce::WebSliderRelay>("VEL_CURVE");
    outputGainRelay = std::make_unique<juce::WebSliderRelay>("OUTPUT_GAIN");

    // 2️⃣ Create WebView with options (Pattern 9: NEEDS_WEB_BROWSER required)
    webView = std::make_unique<juce::WebBrowserComponent>(
        juce::WebBrowserComponent::Options{}
            .withNativeIntegrationEnabled()  // CRITICAL: Enables JUCE JavaScript library
            .withResourceProvider([this](const auto& url) { return getResource(url); })
            .withOptionsFrom(*malletHardnessRelay)
            .withOptionsFrom(*barMaterialRelay)
            .withOptionsFrom(*resonanceRelay)
            .withOptionsFrom(*tuningModeRelay)
            .withOptionsFrom(*referencePitchRelay)
            .withOptionsFrom(*velCurveRelay)
            .withOptionsFrom(*outputGainRelay)
    );

    // 3️⃣ Create attachments LAST (Pattern 12: 3 params required - parameter, relay, nullptr)
    malletHardnessAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("MALLET_HARDNESS"), *malletHardnessRelay, nullptr);

    barMaterialAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("BAR_MATERIAL"), *barMaterialRelay, nullptr);

    resonanceAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("RESONANCE"), *resonanceRelay, nullptr);

    tuningModeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("TUNING_MODE"), *tuningModeRelay, nullptr);

    referencePitchAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("REFERENCE_PITCH"), *referencePitchRelay, nullptr);

    velCurveAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("VEL_CURVE"), *velCurveRelay, nullptr);

    outputGainAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("OUTPUT_GAIN"), *outputGainRelay, nullptr);

    // Add WebView to editor
    addAndMakeVisible(*webView);

    // Navigate to UI
    webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());

    // Set editor size (from v1 mockup: 600x400)
    setSize(600, 400);
}

MicroMarimbaAudioProcessorEditor::~MicroMarimbaAudioProcessorEditor()
{
    // Destruction happens in reverse order (attachments → webView → relays)
    // Pattern 11: No manual cleanup needed (unique_ptr handles it)
}

void MicroMarimbaAudioProcessorEditor::paint(juce::Graphics& g)
{
    // WebView handles all painting
    juce::ignoreUnused(g);
}

void MicroMarimbaAudioProcessorEditor::resized()
{
    // WebView fills entire editor
    webView->setBounds(getLocalBounds());
}

// Pattern 8: Explicit URL mapping (required for WebView resource loading)
std::optional<juce::WebBrowserComponent::Resource>
MicroMarimbaAudioProcessorEditor::getResource(const juce::String& url)
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

    // Resource not found
    juce::Logger::writeToLog("Resource not found: " + url);
    return std::nullopt;
}
