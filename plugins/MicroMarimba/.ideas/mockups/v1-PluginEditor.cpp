#include "PluginEditor.h"

//==============================================================================
// Constructor - CRITICAL: Initialize in correct order
//==============================================================================

MicroMarimbaAudioProcessorEditor::MicroMarimbaAudioProcessorEditor(MicroMarimbaAudioProcessor& p)
    : AudioProcessorEditor(p), audioProcessor(p)
{
    // ========================================================================
    // INITIALIZATION SEQUENCE (CRITICAL ORDER)
    // 1. Create relays FIRST (before WebView construction)
    // 2. Create WebView with relay options
    // 3. Create parameter attachments LAST (after WebView construction)
    // ========================================================================

    // ------------------------------------------------------------------------
    // STEP 1: CREATE RELAYS
    // ------------------------------------------------------------------------
    malletHardnessRelay = std::make_unique<juce::WebSliderRelay>("MALLET_HARDNESS");
    barMaterialRelay = std::make_unique<juce::WebSliderRelay>("BAR_MATERIAL");
    resonanceRelay = std::make_unique<juce::WebSliderRelay>("RESONANCE");
    tuningModeRelay = std::make_unique<juce::WebComboBoxRelay>("TUNING_MODE");
    referencePitchRelay = std::make_unique<juce::WebSliderRelay>("REFERENCE_PITCH");
    velCurveRelay = std::make_unique<juce::WebSliderRelay>("VEL_CURVE");
    outputGainRelay = std::make_unique<juce::WebSliderRelay>("OUTPUT_GAIN");

    // ------------------------------------------------------------------------
    // STEP 2: CREATE WEBVIEW (with relay options)
    // ------------------------------------------------------------------------
    webView = std::make_unique<juce::WebBrowserComponent>(
        juce::WebBrowserComponent::Options{}
            // REQUIRED: Enable JUCE frontend library
            .withNativeIntegrationEnabled()

            // REQUIRED: Resource provider for embedded files
            .withResourceProvider([this](const auto& url) {
                return getResource(url);
            })

            // OPTIONAL: FL Studio fix (prevents blank screen on focus loss)
            .withKeepPageLoadedWhenBrowserIsHidden()

            // REQUIRED: Register each relay with WebView
            .withOptionsFrom(*malletHardnessRelay)
            .withOptionsFrom(*barMaterialRelay)
            .withOptionsFrom(*resonanceRelay)
            .withOptionsFrom(*tuningModeRelay)
            .withOptionsFrom(*referencePitchRelay)
            .withOptionsFrom(*velCurveRelay)
            .withOptionsFrom(*outputGainRelay)
    );

    // ------------------------------------------------------------------------
    // STEP 3: CREATE PARAMETER ATTACHMENTS
    // ------------------------------------------------------------------------
    malletHardnessAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.parameters.getParameter("MALLET_HARDNESS"),
        *malletHardnessRelay
    );

    barMaterialAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.parameters.getParameter("BAR_MATERIAL"),
        *barMaterialRelay
    );

    resonanceAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.parameters.getParameter("RESONANCE"),
        *resonanceRelay
    );

    tuningModeAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *audioProcessor.parameters.getParameter("TUNING_MODE"),
        *tuningModeRelay
    );

    referencePitchAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.parameters.getParameter("REFERENCE_PITCH"),
        *referencePitchRelay
    );

    velCurveAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.parameters.getParameter("VEL_CURVE"),
        *velCurveRelay
    );

    outputGainAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.parameters.getParameter("OUTPUT_GAIN"),
        *outputGainRelay
    );

    // ------------------------------------------------------------------------
    // WEBVIEW SETUP
    // ------------------------------------------------------------------------
    webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
    addAndMakeVisible(*webView);

    // ------------------------------------------------------------------------
    // WINDOW SIZE (from v1 mockup)
    // ------------------------------------------------------------------------
    setSize(600, 400);
}

//==============================================================================
// Destructor
//==============================================================================

MicroMarimbaAudioProcessorEditor::~MicroMarimbaAudioProcessorEditor()
{
    // Members automatically destroyed in reverse order:
    // 1. Attachments first (stop calling evaluateJavascript)
    // 2. WebView second (safe, attachments are gone)
    // 3. Relays last (safe, nothing using them)
}

//==============================================================================
// AudioProcessorEditor Overrides
//==============================================================================

void MicroMarimbaAudioProcessorEditor::paint(juce::Graphics& g)
{
    // WebView fills entire editor, no custom painting needed
}

void MicroMarimbaAudioProcessorEditor::resized()
{
    // Make WebView fill the entire editor bounds
    webView->setBounds(getLocalBounds());
}

//==============================================================================
// Resource Provider
//==============================================================================

std::optional<juce::WebBrowserComponent::Resource> MicroMarimbaAudioProcessorEditor::getResource(
    const juce::String& url
)
{
    // Root URL - serve index.html
    if (url == "/")
    {
        return juce::WebBrowserComponent::Resource {
            BinaryData::index_html,
            BinaryData::index_htmlSize,
            "text/html"
        };
    }

    // JUCE frontend library
    if (url == "/js/juce/index.js")
    {
        return juce::WebBrowserComponent::Resource {
            BinaryData::juce_index_js,
            BinaryData::juce_index_jsSize,
            "application/javascript"
        };
    }

    // 404 - Resource not found
    return std::nullopt;
}
