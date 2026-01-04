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
    // {{RELAY_CREATION}}
    // Example:
    // gainRelay = std::make_unique<juce::WebSliderRelay>("GAIN");
    // toneRelay = std::make_unique<juce::WebSliderRelay>("TONE");
    // bypassRelay = std::make_unique<juce::WebToggleButtonRelay>("BYPASS");
    // modeRelay = std::make_unique<juce::WebComboBoxRelay>("MODE");

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
            // Remove if not targeting FL Studio
            .withKeepPageLoadedWhenBrowserIsHidden()

            // REQUIRED: Register each relay with WebView
            // This creates JavaScript state objects accessible via:
            // - Juce.getSliderState("PARAM_ID")
            // - Juce.getToggleButtonState("PARAM_ID")
            // - Juce.getComboBoxState("PARAM_ID")
            //
            // {{WEBVIEW_OPTIONS}}
            // Example:
            // .withOptionsFrom(*gainRelay)
            // .withOptionsFrom(*toneRelay)
            // .withOptionsFrom(*bypassRelay)
            // .withOptionsFrom(*modeRelay)
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
    // Undo manager typically nullptr for real-time parameters.
    //
    // {{ATTACHMENT_CREATION}}
    // Example:
    // gainAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
    //     *audioProcessor.apvts.getParameter("GAIN"),
    //     *gainRelay,
    //     nullptr  // No undo manager
    // );
    // toneAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
    //     *audioProcessor.apvts.getParameter("TONE"),
    //     *toneRelay,
    //     nullptr
    // );
    // bypassAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
    //     *audioProcessor.apvts.getParameter("BYPASS"),
    //     *bypassRelay,
    //     nullptr
    // );
    // modeAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
    //     *audioProcessor.apvts.getParameter("MODE"),
    //     *modeRelay,
    //     nullptr
    // );

    // ------------------------------------------------------------------------
    // WEBVIEW SETUP
    // ------------------------------------------------------------------------

    // Navigate to root (loads index.html via resource provider)
    // JUCE 8 pattern: Use resource provider root instead of data URLs
    webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());

    // Make WebView visible
    addAndMakeVisible(*webView);

    // ------------------------------------------------------------------------
    // TIMER SETUP (optional - for real-time updates)
    // ------------------------------------------------------------------------
    //
    // Use timer for:
    // - Meter updates (VU, peak, RMS)
    // - Spectrum analyzer data
    // - Waveform visualization
    // - Tempo/transport sync
    //
    // Timer intervals:
    // - 16ms (60 FPS) - Smooth animations, spectrum analyzer
    // - 60ms (16 FPS) - Meter updates (sufficient for audio levels)
    // - 100ms (10 FPS) - Slow polling (tempo, transport state)
    //
    // startTimerHz(16);  // Example: 60ms = ~16 FPS for meter updates

    // ------------------------------------------------------------------------
    // WINDOW SIZE
    // ------------------------------------------------------------------------
    //
    // Fixed size example:
    // setSize(600, 400);
    // setResizable(false, false);
    //
    // Resizable example:
    // setSize(800, 600);
    // setResizable(true, true);
    // setResizeLimits(600, 400, 1920, 1080);
    //
    // {{DETERMINE_SIZE_FROM_MOCKUP}}
    setSize(600, 400);
}

//==============================================================================
// Destructor
//==============================================================================

PluginEditor::~PluginEditor()
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

void PluginEditor::paint(juce::Graphics& g)
{
    // WebView fills the entire editor, so typically no custom painting needed
    // Uncomment if you want a background color visible before WebView loads:
    // g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void PluginEditor::resized()
{
    // Make WebView fill the entire editor bounds
    webView->setBounds(getLocalBounds());
}

//==============================================================================
// Timer Callback (optional - for real-time updates)
//==============================================================================

void PluginEditor::timerCallback()
{
    // Example: Update meters from audio thread data
    // CRITICAL: Use std::atomic in PluginProcessor for thread-safe communication
    //
    // float level = audioProcessor.getCurrentLevel();  // Atomic read
    //
    // webView->emitEventIfBrowserIsVisible("meterUpdate", {
    //     { "level", level },
    //     { "timestamp", juce::Time::currentTimeMillis() }
    // });

    // Example: Update tempo/transport from DAW
    // auto playHead = audioProcessor.getPlayHead();
    // if (playHead != nullptr) {
    //     juce::AudioPlayHead::CurrentPositionInfo info;
    //     playHead->getCurrentPosition(info);
    //
    //     webView->emitEventIfBrowserIsVisible("transportUpdate", {
    //         { "isPlaying", info.isPlaying },
    //         { "bpm", info.bpm },
    //         { "timeSigNumerator", info.timeSigNumerator },
    //         { "timeSigDenominator", info.timeSigDenominator }
    //     });
    // }
}

//==============================================================================
// Resource Provider (JUCE 8 Required Pattern)
//==============================================================================

std::optional<juce::WebBrowserComponent::Resource> PluginEditor::getResource(
    const juce::String& url
)
{
    // ========================================================================
    // RESOURCE PROVIDER IMPLEMENTATION
    // ========================================================================
    //
    // Maps URLs to embedded binary data (from juce_add_binary_data).
    //
    // File path → BinaryData symbol:
    // - ui/public/index.html       → BinaryData::index_html
    // - ui/public/js/juce/index.js → BinaryData::juce_index_js
    // - ui/public/css/styles.css   → BinaryData::styles_css
    //
    // Pattern: Remove "ui/public/", replace "/" with "_", replace "." with "_"
    // ========================================================================

    // Handle root URL (redirect to index.html)
    if (url == "/") {
        return juce::WebBrowserComponent::Resource {
            BinaryData::index_html,
            BinaryData::index_htmlSize,
            "text/html"
        };
    }

    // {{RESOURCE_MAPPING}}
    // Example:
    // if (url == "/index.html") {
    //     return juce::WebBrowserComponent::Resource {
    //         BinaryData::index_html,
    //         BinaryData::index_htmlSize,
    //         "text/html"
    //     };
    // }
    //
    // if (url == "/js/juce/index.js") {
    //     return juce::WebBrowserComponent::Resource {
    //         BinaryData::juce_index_js,
    //         BinaryData::juce_index_jsSize,
    //         "text/javascript"
    //     };
    // }
    //
    // if (url == "/css/styles.css") {
    //     return juce::WebBrowserComponent::Resource {
    //         BinaryData::styles_css,
    //         BinaryData::styles_cssSize,
    //         "text/css"
    //     };
    // }

    // 404 - Resource not found
    return std::nullopt;
}
