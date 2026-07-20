#include "PluginEditor.h"

//==============================================================================
// Constructor - CRITICAL: Initialize in correct order
//==============================================================================

PluginEditor::PluginEditor(PluginProcessor& p)
    : AudioProcessorEditor(p), audioProcessor(p)
{
    // ========================================================================
    // INITIALIZATION SEQUENCE (CRITICAL ORDER)
    // ========================================================================
    //
    // 1. Create relays FIRST (before WebView construction)
    // 2. Create WebView with relay options
    // 3. Create parameter attachments LAST (after WebView construction)
    // ========================================================================

    // ------------------------------------------------------------------------
    // STEP 1: CREATE RELAYS (before WebView!)
    // ------------------------------------------------------------------------
    gainRelay = std::make_unique<juce::WebSliderRelay>("GAIN");

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

            // REQUIRED: Register relay with WebView
            .withOptionsFrom(*gainRelay)
    );

    // ------------------------------------------------------------------------
    // STEP 3: CREATE PARAMETER ATTACHMENTS (after WebView!)
    // ------------------------------------------------------------------------
    gainAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("GAIN"),
        *gainRelay,
        nullptr  // No undo manager
    );

    // ------------------------------------------------------------------------
    // WEBVIEW SETUP
    // ------------------------------------------------------------------------

    // Navigate to root (loads index.html via resource provider)
    webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());

    // Make WebView visible
    addAndMakeVisible(*webView);

    // ------------------------------------------------------------------------
    // WINDOW SIZE
    // ------------------------------------------------------------------------
    setSize(400, 400);
    setResizable(false, false);
}

//==============================================================================
// Destructor
//==============================================================================

PluginEditor::~PluginEditor()
{
    // Members automatically destroyed in reverse order:
    // 1. gainAttachment (stops calling evaluateJavascript)
    // 2. webView (safe, attachment is gone)
    // 3. gainRelay (safe, nothing using it)
}

//==============================================================================
// AudioProcessorEditor Overrides
//==============================================================================

void PluginEditor::paint(juce::Graphics& g)
{
    // WebView fills the entire editor, no custom painting needed
}

void PluginEditor::resized()
{
    // Make WebView fill the entire editor bounds
    webView->setBounds(getLocalBounds());
}

//==============================================================================
// Resource Provider (JUCE 8 Required Pattern)
//==============================================================================

std::optional<juce::WebBrowserComponent::Resource> PluginEditor::getResource(
    const juce::String& url
)
{
    // Handle root URL (redirect to index.html)
    if (url == "/" || url == "/index.html") {
        return juce::WebBrowserComponent::Resource {
            BinaryData::index_html,
            BinaryData::index_htmlSize,
            "text/html"
        };
    }

    // JUCE frontend library
    if (url == "/js/juce/index.js") {
        return juce::WebBrowserComponent::Resource {
            BinaryData::juce_index_js,
            BinaryData::juce_index_jsSize,
            "text/javascript"
        };
    }

    // 404 - Resource not found
    return std::nullopt;
}
