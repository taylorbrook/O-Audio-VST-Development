#include "PluginEditor.h"

//==============================================================================
// Constructor - CRITICAL: Initialize in correct order
//==============================================================================

LushPadAudioProcessorEditor::LushPadAudioProcessorEditor(LushPadAudioProcessor& p)
    : AudioProcessorEditor(p), audioProcessor(p)
{
    // ========================================================================
    // INITIALIZATION SEQUENCE (CRITICAL ORDER)
    // ========================================================================
    //
    // 1. Create relays FIRST (before WebView construction)
    // 2. Create WebView with relay options
    // 3. Create parameter attachments LAST (after WebView construction)
    //
    // This matches the member declaration order and ensures safe destruction.
    // ========================================================================

    // ------------------------------------------------------------------------
    // STEP 1: CREATE RELAYS (before WebView!)
    // ------------------------------------------------------------------------

    timbreRelay = std::make_unique<juce::WebSliderRelay>("timbre");
    filterCutoffRelay = std::make_unique<juce::WebSliderRelay>("filter_cutoff");
    reverbAmountRelay = std::make_unique<juce::WebSliderRelay>("reverb_amount");

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

            // REQUIRED: Register each relay with WebView
            .withOptionsFrom(*timbreRelay)
            .withOptionsFrom(*filterCutoffRelay)
            .withOptionsFrom(*reverbAmountRelay)
    );

    // ------------------------------------------------------------------------
    // STEP 3: CREATE PARAMETER ATTACHMENTS (after WebView!)
    // ------------------------------------------------------------------------
    //
    // Pattern #12: JUCE 8 requires 3 parameters (parameter, relay, undoManager)
    //

    timbreAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.parameters.getParameter("timbre"),
        *timbreRelay,
        nullptr  // No undo manager
    );

    filterCutoffAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.parameters.getParameter("filter_cutoff"),
        *filterCutoffRelay,
        nullptr
    );

    reverbAmountAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.parameters.getParameter("reverb_amount"),
        *reverbAmountRelay,
        nullptr
    );

    // ------------------------------------------------------------------------
    // WEBVIEW SETUP
    // ------------------------------------------------------------------------

    // Navigate to root (loads index.html via resource provider)
    webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());

    // Make WebView visible
    addAndMakeVisible(*webView);

    // ------------------------------------------------------------------------
    // WINDOW SIZE (from v2-ui.yaml)
    // ------------------------------------------------------------------------

    setSize(600, 300);
}

//==============================================================================
// Destructor
//==============================================================================

LushPadAudioProcessorEditor::~LushPadAudioProcessorEditor()
{
    // Members are automatically destroyed in reverse order of declaration:
    // 1. Attachments destroyed first (stop calling evaluateJavascript)
    // 2. WebView destroyed second (safe, attachments are gone)
    // 3. Relays destroyed last (safe, nothing using them)
    //
    // No manual cleanup needed if member order is correct!
}

//==============================================================================
// AudioProcessorEditor Overrides
//==============================================================================

void LushPadAudioProcessorEditor::paint(juce::Graphics& g)
{
    // WebView fills the entire editor, so no custom painting needed
}

void LushPadAudioProcessorEditor::resized()
{
    // Make WebView fill the entire editor bounds
    webView->setBounds(getLocalBounds());
}

//==============================================================================
// Resource Provider (JUCE 8 Required Pattern)
//==============================================================================

std::optional<juce::WebBrowserComponent::Resource> LushPadAudioProcessorEditor::getResource(
    const juce::String& url
)
{
    // ========================================================================
    // Pattern #8: Explicit URL mapping (ALWAYS REQUIRED)
    // ========================================================================
    //
    // BinaryData converts paths to valid C++ identifiers:
    // - Source/ui/public/index.html       → BinaryData::index_html
    // - Source/ui/public/js/juce/index.js → BinaryData::index_js
    //
    // HTML/JS use original paths, so we map them explicitly here.
    // ========================================================================

    auto makeVector = [](const char* data, int size) {
        return std::vector<std::byte>(
            reinterpret_cast<const std::byte*>(data),
            reinterpret_cast<const std::byte*>(data) + size
        );
    };

    // Root URL → index.html
    if (url == "/" || url == "/index.html") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::index_html, BinaryData::index_htmlSize),
            juce::String("text/html")
        };
    }

    // JUCE frontend library (REQUIRED)
    if (url == "/js/juce/index.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::index_js, BinaryData::index_jsSize),
            juce::String("application/javascript")  // Pattern #8: Correct MIME type
        };
    }

    // Pattern #13: check_native_interop.js (REQUIRED for WebView initialization)
    if (url == "/js/juce/check_native_interop.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::check_native_interop_js, BinaryData::check_native_interop_jsSize),
            juce::String("application/javascript")
        };
    }

    // 404 - Resource not found
    return std::nullopt;
}
