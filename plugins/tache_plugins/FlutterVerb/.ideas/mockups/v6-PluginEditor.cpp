#include "PluginEditor.h"

//==============================================================================
// Constructor - CRITICAL: Initialize in correct order
//==============================================================================

FlutterVerbAudioProcessorEditor::FlutterVerbAudioProcessorEditor(FlutterVerbAudioProcessor& p)
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
    // FlutterVerb parameters:
    // - 6 continuous: SIZE, DECAY, MIX, AGE, DRIVE, TONE
    // - 1 toggle: MOD_MODE
    //
    sizeRelay = std::make_unique<juce::WebSliderRelay>("SIZE");
    decayRelay = std::make_unique<juce::WebSliderRelay>("DECAY");
    mixRelay = std::make_unique<juce::WebSliderRelay>("MIX");
    ageRelay = std::make_unique<juce::WebSliderRelay>("AGE");
    driveRelay = std::make_unique<juce::WebSliderRelay>("DRIVE");
    toneRelay = std::make_unique<juce::WebSliderRelay>("TONE");
    modModeRelay = std::make_unique<juce::WebToggleButtonRelay>("MOD_MODE");

    // ------------------------------------------------------------------------
    // STEP 2: CREATE WEBVIEW (with relay options)
    // ------------------------------------------------------------------------
    //
    // WebView creation with all necessary options:
    // - withNativeIntegrationEnabled() - REQUIRED for JUCE parameter binding
    // - withResourceProvider() - REQUIRED for JUCE 8 (serves embedded files)
    // - withOptionsFrom(*relay) - REQUIRED for each parameter relay
    // - withKeepPageLoadedWhenBrowserIsHidden() - FL Studio fix
    //
    webView = std::make_unique<juce::WebBrowserComponent>(
        juce::WebBrowserComponent::Options{}
            .withNativeIntegrationEnabled()
            .withResourceProvider([this](const auto& url) {
                return getResource(url);
            })
            .withKeepPageLoadedWhenBrowserIsHidden()
            // Register all relays with WebView
            .withOptionsFrom(*sizeRelay)
            .withOptionsFrom(*decayRelay)
            .withOptionsFrom(*mixRelay)
            .withOptionsFrom(*ageRelay)
            .withOptionsFrom(*driveRelay)
            .withOptionsFrom(*toneRelay)
            .withOptionsFrom(*modModeRelay)
    );

    // ------------------------------------------------------------------------
    // STEP 3: CREATE PARAMETER ATTACHMENTS (after WebView!)
    // ------------------------------------------------------------------------
    //
    // Attachments synchronize APVTS parameters with relay state.
    //
    sizeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("SIZE"),
        *sizeRelay,
        nullptr  // No undo manager
    );
    decayAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("DECAY"),
        *decayRelay,
        nullptr
    );
    mixAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("MIX"),
        *mixRelay,
        nullptr
    );
    ageAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("AGE"),
        *ageRelay,
        nullptr
    );
    driveAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("DRIVE"),
        *driveRelay,
        nullptr
    );
    toneAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("TONE"),
        *toneRelay,
        nullptr
    );
    modModeAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *audioProcessor.apvts.getParameter("MOD_MODE"),
        *modModeRelay,
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
    // TIMER SETUP (VU meter updates)
    // ------------------------------------------------------------------------
    //
    // FlutterVerb has a VU meter showing output peak level
    // Update at 16 FPS (60ms) - sufficient for audio level display
    //
    startTimerHz(16);  // 60ms = ~16 FPS

    // ------------------------------------------------------------------------
    // WINDOW SIZE (from mockup v6)
    // ------------------------------------------------------------------------
    //
    // Fixed size: 600×640px (non-resizable)
    //
    setSize(600, 640);
    setResizable(false, false);
}

//==============================================================================
// Destructor
//==============================================================================

FlutterVerbAudioProcessorEditor::~FlutterVerbAudioProcessorEditor()
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

void FlutterVerbAudioProcessorEditor::paint(juce::Graphics& g)
{
    // WebView fills the entire editor, so no custom painting needed
}

void FlutterVerbAudioProcessorEditor::resized()
{
    // Make WebView fill the entire editor bounds
    webView->setBounds(getLocalBounds());
}

//==============================================================================
// Timer Callback (VU Meter Updates)
//==============================================================================

void FlutterVerbAudioProcessorEditor::timerCallback()
{
    // Update VU meter from audio thread data
    // CRITICAL: Use std::atomic in PluginProcessor for thread-safe communication
    //
    // Example (assumes audioProcessor has atomic<float> outputLevel):
    // float level = audioProcessor.getOutputLevel();  // Atomic read
    //
    // webView->emitEventIfBrowserIsVisible("vuMeterUpdate", {
    //     { "level", level },  // dB value (-60 to 0)
    //     { "timestamp", juce::Time::currentTimeMillis() }
    // });
    //
    // JavaScript side (in index.html) listens for this event:
    // window.__JUCE__.backend.addEventListener("vuMeterUpdate", (data) => {
    //     updateVUNeedle(data.level);
    // });
}

//==============================================================================
// Resource Provider (JUCE 8 Required Pattern)
//==============================================================================

std::optional<juce::WebBrowserComponent::Resource> FlutterVerbAudioProcessorEditor::getResource(
    const juce::String& url
)
{
    // ========================================================================
    // RESOURCE PROVIDER IMPLEMENTATION
    // ========================================================================
    //
    // Maps URLs to embedded binary data (from juce_add_binary_data).
    //
    // FlutterVerb UI structure:
    // - Source/ui/public/index.html       → BinaryData::index_html
    // - Source/ui/public/js/juce/index.js → BinaryData::juce_index_js
    //
    // Pattern: Remove "Source/ui/public/", replace "/" with "_", replace "." with "_"
    // ========================================================================

    // Handle root URL (redirect to index.html)
    if (url == "/" || url == "/index.html") {
        return juce::WebBrowserComponent::Resource {
            BinaryData::index_html,
            BinaryData::index_htmlSize,
            "text/html"
        };
    }

    // JUCE frontend library (REQUIRED for parameter binding)
    if (url == "/js/juce/index.js") {
        return juce::WebBrowserComponent::Resource {
            BinaryData::juce_index_js,
            BinaryData::juce_index_jsSize,
            "text/javascript"
        };
    }

    // Add additional resources here if needed (CSS, images, fonts, etc.)
    // Example:
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
