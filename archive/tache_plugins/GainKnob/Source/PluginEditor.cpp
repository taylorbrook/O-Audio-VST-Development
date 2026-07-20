#include "PluginEditor.h"
#include "BinaryData.h"

//==============================================================================
// Constructor - CRITICAL: Initialize in correct order
//==============================================================================

GainKnobAudioProcessorEditor::GainKnobAudioProcessorEditor(GainKnobAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
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
    panRelay = std::make_unique<juce::WebSliderRelay>("PAN");
    filterRelay = std::make_unique<juce::WebSliderRelay>("FILTER");

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

            // REQUIRED: Register relays with WebView
            .withOptionsFrom(*gainRelay)
            .withOptionsFrom(*panRelay)
            .withOptionsFrom(*filterRelay)
    );

    // ------------------------------------------------------------------------
    // STEP 3: CREATE PARAMETER ATTACHMENTS (after WebView!)
    // ------------------------------------------------------------------------
    gainAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("GAIN"),
        *gainRelay,
        nullptr  // No undo manager
    );

    panAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("PAN"),
        *panRelay,
        nullptr  // No undo manager
    );

    filterAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("FILTER"),
        *filterRelay,
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
    setSize(800, 400);  // Wider to accommodate three knobs
    setResizable(false, false);
}

//==============================================================================
// Destructor
//==============================================================================

GainKnobAudioProcessorEditor::~GainKnobAudioProcessorEditor()
{
    // Members automatically destroyed in reverse order:
    // 1. filterAttachment, panAttachment, gainAttachment (stop calling evaluateJavascript)
    // 2. webView (safe, attachments are gone)
    // 3. filterRelay, panRelay, gainRelay (safe, nothing using them)
}

//==============================================================================
// AudioProcessorEditor Overrides
//==============================================================================

void GainKnobAudioProcessorEditor::paint(juce::Graphics& g)
{
    // WebView fills the entire editor, no custom painting needed
    juce::ignoreUnused(g);
}

void GainKnobAudioProcessorEditor::resized()
{
    // Make WebView fill the entire editor bounds
    webView->setBounds(getLocalBounds());
}

//==============================================================================
// Resource Provider (JUCE 8 Required Pattern)
//==============================================================================

std::optional<juce::WebBrowserComponent::Resource> GainKnobAudioProcessorEditor::getResource(
    const juce::String& url
)
{
    // Helper lambda to convert raw binary data to vector<byte>
    auto makeVector = [](const char* data, int size) {
        return std::vector<std::byte>(
            reinterpret_cast<const std::byte*>(data),
            reinterpret_cast<const std::byte*>(data) + size
        );
    };

    // Handle root URL (redirect to index.html)
    if (url == "/" || url == "/index.html") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::index_html, BinaryData::index_htmlSize),
            juce::String("text/html")
        };
    }

    // JUCE frontend library
    if (url == "/js/juce/index.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::index_js, BinaryData::index_jsSize),
            juce::String("text/javascript")
        };
    }

    // JUCE native interop checker
    if (url == "/js/juce/check_native_interop.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::check_native_interop_js, BinaryData::check_native_interop_jsSize),
            juce::String("text/javascript")
        };
    }

    // 404 - Resource not found
    return std::nullopt;
}
