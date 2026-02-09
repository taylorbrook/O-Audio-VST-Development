/*
  ==============================================================================

    OuariconSaturationModelingAudioProcessorEditor.cpp

    TEMPLATE FILE - Generated from v4 UI mockup finalization

    This is a REFERENCE TEMPLATE for gui-agent to adapt during Stage 3 (GUI).
    DO NOT copy-paste directly - gui-agent will integrate into actual plugin structure.

    Generated: 2026-01-09
    Plugin: OuariconSaturationModeling
    Version: v4

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
OuariconSaturationModelingAudioProcessorEditor::OuariconSaturationModelingAudioProcessorEditor(
    OuariconSaturationModelingAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    // =========================================================================
    // CRITICAL: Initialization order matches declaration order
    // 1. Create relays FIRST
    // 2. Create WebView with relay options
    // 3. Create attachments LAST
    // =========================================================================

    // 1. CREATE RELAYS FIRST
    intensityRelay = std::make_unique<juce::WebSliderRelay>("INTENSITY");
    modelRelay = std::make_unique<juce::WebSliderRelay>("MODEL");
    qualityRelay = std::make_unique<juce::WebSliderRelay>("QUALITY");
    autogainRelay = std::make_unique<juce::WebToggleButtonRelay>("AUTOGAIN");

    // 2. CREATE WEBVIEW WITH RELAY OPTIONS
    webView = std::make_unique<juce::WebBrowserComponent>(
        juce::WebBrowserComponent::Options{}
            .withNativeIntegrationEnabled()
            .withResourceProvider([this](const auto& url) { return getResource(url); })
            .withOptionsFrom(*intensityRelay)
            .withOptionsFrom(*modelRelay)
            .withOptionsFrom(*qualityRelay)
            .withOptionsFrom(*autogainRelay)
    );

    // 3. CREATE ATTACHMENTS LAST (JUCE 8 requires 3 parameters - critical pattern #12)
    intensityAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.parameters.getParameter("INTENSITY"), *intensityRelay, nullptr);

    modelAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.parameters.getParameter("MODEL"), *modelRelay, nullptr);

    qualityAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.parameters.getParameter("QUALITY"), *qualityRelay, nullptr);

    autogainAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *audioProcessor.parameters.getParameter("AUTOGAIN"), *autogainRelay, nullptr);

    // Add WebView to component hierarchy
    addAndMakeVisible(*webView);

    // Load index.html from resource provider
    webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());

    // Set plugin window size (from v4-ui.yaml: 600x450)
    setSize(600, 450);

    // Start timer for VU meter updates (60 Hz)
    startTimerHz(60);
}

OuariconSaturationModelingAudioProcessorEditor::~OuariconSaturationModelingAudioProcessorEditor()
{
    stopTimer();

    // Members destroyed in REVERSE order of declaration:
    // 1. Attachments destroyed FIRST (can safely call webView methods)
    // 2. WebView destroyed SECOND (attachments are gone)
    // 3. Relays destroyed LAST (nothing using them)
}

//==============================================================================
void OuariconSaturationModelingAudioProcessorEditor::paint(juce::Graphics& g)
{
    // WebView fills entire component, no background painting needed
    g.fillAll(juce::Colours::black);
}

void OuariconSaturationModelingAudioProcessorEditor::resized()
{
    // WebView fills entire editor bounds
    webView->setBounds(getLocalBounds());
}

//==============================================================================
void OuariconSaturationModelingAudioProcessorEditor::timerCallback()
{
    // Calculate current VU meter levels (in dB)
    float inputDB = calculateInputLevel();
    float outputDB = calculateOutputLevel();

    // Send levels to JavaScript via evaluateJavascript
    // JavaScript receives via window.receiveMeterLevels(inputDB, outputDB)
    juce::String jsCode = juce::String::formatted(
        "if (typeof window.receiveMeterLevels === 'function') { "
        "window.receiveMeterLevels(%f, %f); "
        "}",
        inputDB,
        outputDB
    );

    webView->evaluateJavascript(jsCode);
}

//==============================================================================
float OuariconSaturationModelingAudioProcessorEditor::calculateInputLevel()
{
    // TODO: Implement RMS or peak calculation from input buffer
    // Example: Read from processor's level tracking (atomic float or similar)
    //
    // Placeholder: Return -10 dB
    return -10.0f;

    // Real implementation example:
    // return juce::Decibels::gainToDecibels(audioProcessor.getInputRMS());
}

float OuariconSaturationModelingAudioProcessorEditor::calculateOutputLevel()
{
    // TODO: Implement RMS or peak calculation from output buffer
    // Example: Read from processor's level tracking (atomic float or similar)
    //
    // Placeholder: Return -6 dB
    return -6.0f;

    // Real implementation example:
    // return juce::Decibels::gainToDecibels(audioProcessor.getOutputRMS());
}

//==============================================================================
// WebView Resource Provider (JUCE 8 critical pattern #8)
// EXPLICIT URL MAPPING - clear, debuggable, reliable
//==============================================================================
std::optional<juce::WebBrowserComponent::Resource>
OuariconSaturationModelingAudioProcessorEditor::getResource(const juce::String& url)
{
    // Helper lambda to convert BinaryData to byte vector
    auto makeVector = [](const char* data, int size) {
        return std::vector<std::byte>(
            reinterpret_cast<const std::byte*>(data),
            reinterpret_cast<const std::byte*>(data) + size
        );
    };

    // === HTML ===
    if (url == "/" || url == "/index.html") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::index_html, BinaryData::index_htmlSize),
            juce::String("text/html")
        };
    }

    // === JavaScript (JUCE bridge) ===
    if (url == "/js/juce/index.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::index_js, BinaryData::index_jsSize),
            juce::String("application/javascript")  // Correct MIME type
        };
    }

    if (url == "/js/juce/check_native_interop.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::check_native_interop_js, BinaryData::check_native_interop_jsSize),
            juce::String("application/javascript")
        };
    }

    // === Images (paper background) ===
    if (url == "/img/paper1.jpg") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::paper1_jpg, BinaryData::paper1_jpgSize),
            juce::String("image/jpeg")
        };
    }

    // === Images (snake overlays - 4 model-specific images) ===
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
            makeVector(BinaryData::snake_NA_0145_png,
                      BinaryData::snake_NA_0145_pngSize),
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

    // 404 - Resource not found
    return std::nullopt;
}
