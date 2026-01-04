#include "PluginEditor.h"

//==============================================================================
// Constructor - CRITICAL: Initialize in correct order
//==============================================================================

ScatterAudioProcessorEditor::ScatterAudioProcessorEditor(ScatterAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
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
    //
    // Each relay bridges a C++ parameter to JavaScript state.
    // Relay constructor takes the parameter ID (must match APVTS).
    //
    delayTimeRelay = std::make_unique<juce::WebSliderRelay>("delay_time");
    grainSizeRelay = std::make_unique<juce::WebSliderRelay>("grain_size");
    densityRelay = std::make_unique<juce::WebSliderRelay>("density");
    pitchRandomRelay = std::make_unique<juce::WebSliderRelay>("pitch_random");
    scaleRelay = std::make_unique<juce::WebComboBoxRelay>("scale");
    rootNoteRelay = std::make_unique<juce::WebComboBoxRelay>("root_note");
    panRandomRelay = std::make_unique<juce::WebSliderRelay>("pan_random");
    feedbackRelay = std::make_unique<juce::WebSliderRelay>("feedback");
    mixRelay = std::make_unique<juce::WebSliderRelay>("mix");

    // ------------------------------------------------------------------------
    // STEP 2: CREATE WEBVIEW (with relay options)
    // ------------------------------------------------------------------------
    //
    // WebView creation with all necessary options:
    // - withNativeIntegrationEnabled() - REQUIRED for JUCE parameter binding
    // - withResourceProvider() - REQUIRED for JUCE 8 (serves embedded files)
    // - withOptionsFrom(*relay) - REQUIRED for each parameter relay
    // - withKeepPageLoadedWhenBrowserIsHidden() - OPTIONAL (FL Studio fix)
    //
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
            .withOptionsFrom(*delayTimeRelay)
            .withOptionsFrom(*grainSizeRelay)
            .withOptionsFrom(*densityRelay)
            .withOptionsFrom(*pitchRandomRelay)
            .withOptionsFrom(*scaleRelay)
            .withOptionsFrom(*rootNoteRelay)
            .withOptionsFrom(*panRandomRelay)
            .withOptionsFrom(*feedbackRelay)
            .withOptionsFrom(*mixRelay)
    );

    // ------------------------------------------------------------------------
    // STEP 3: CREATE PARAMETER ATTACHMENTS (after WebView!)
    // ------------------------------------------------------------------------
    //
    // Attachments synchronize APVTS parameters with relay state.
    // Constructor: (parameter, relay, undoManager)
    //
    // Parameter must be retrieved from APVTS:
    //   audioProcessor.apvts.getParameter("PARAM_ID")
    //
    // JUCE 8 requires third parameter (undoManager, typically nullptr).
    //
    delayTimeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("delay_time"),
        *delayTimeRelay,
        nullptr  // No undo manager
    );

    grainSizeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("grain_size"),
        *grainSizeRelay,
        nullptr
    );

    densityAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("density"),
        *densityRelay,
        nullptr
    );

    pitchRandomAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("pitch_random"),
        *pitchRandomRelay,
        nullptr
    );

    scaleAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *audioProcessor.apvts.getParameter("scale"),
        *scaleRelay,
        nullptr
    );

    rootNoteAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *audioProcessor.apvts.getParameter("root_note"),
        *rootNoteRelay,
        nullptr
    );

    panRandomAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("pan_random"),
        *panRandomRelay,
        nullptr
    );

    feedbackAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("feedback"),
        *feedbackRelay,
        nullptr
    );

    mixAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("mix"),
        *mixRelay,
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
    // WINDOW SIZE (from mockup spec: 550×600px)
    // ------------------------------------------------------------------------

    setSize(550, 600);
    setResizable(false, false);  // Non-resizable from v4-ui.yaml
}

//==============================================================================
// Destructor
//==============================================================================

ScatterAudioProcessorEditor::~ScatterAudioProcessorEditor()
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

void ScatterAudioProcessorEditor::paint(juce::Graphics& g)
{
    // WebView fills the entire editor, so no custom painting needed
}

void ScatterAudioProcessorEditor::resized()
{
    // Make WebView fill the entire editor bounds
    webView->setBounds(getLocalBounds());
}

//==============================================================================
// Resource Provider (JUCE 8 Required Pattern)
//==============================================================================

std::optional<juce::WebBrowserComponent::Resource> ScatterAudioProcessorEditor::getResource(
    const juce::String& url
)
{
    // ========================================================================
    // RESOURCE PROVIDER IMPLEMENTATION
    // ========================================================================
    //
    // Maps URLs to embedded binary data (from juce_add_binary_data).
    //
    // CRITICAL: Use explicit URL mapping (Pattern #8 from juce8-critical-patterns.md)
    // Generic loops break because BinaryData flattens paths.
    // ========================================================================

    // Helper to convert raw binary to std::vector<std::byte>
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
            juce::String("application/javascript")  // CRITICAL: Correct MIME type
        };
    }

    // JUCE interop checker
    if (url == "/js/juce/check_native_interop.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::check_native_interop_js, BinaryData::check_native_interop_jsSize),
            juce::String("application/javascript")
        };
    }

    // 404 - Resource not found
    return std::nullopt;
}
