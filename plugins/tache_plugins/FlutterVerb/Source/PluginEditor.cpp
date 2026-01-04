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
    // CRITICAL: Use 3-parameter constructor (Pattern #12)
    //
    sizeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.getAPVTS().getParameter("SIZE"),
        *sizeRelay,
        nullptr  // No undo manager
    );
    decayAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.getAPVTS().getParameter("DECAY"),
        *decayRelay,
        nullptr
    );
    mixAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.getAPVTS().getParameter("MIX"),
        *mixRelay,
        nullptr
    );
    ageAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.getAPVTS().getParameter("AGE"),
        *ageRelay,
        nullptr
    );
    driveAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.getAPVTS().getParameter("DRIVE"),
        *driveRelay,
        nullptr
    );
    toneAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.getAPVTS().getParameter("TONE"),
        *toneRelay,
        nullptr
    );
    modModeAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *audioProcessor.getAPVTS().getParameter("MOD_MODE"),
        *modModeRelay,
        nullptr
    );

    // ------------------------------------------------------------------------
    // WEBVIEW SETUP
    // ------------------------------------------------------------------------

    // Make WebView visible
    addAndMakeVisible(*webView);

    // Navigate to root (loads index.html via resource provider)
    webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());

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
    // Fix 5: Update VU meter with real-time audio level (TapeAge pattern)
    if (!webView)
        return;

    // Read peak level from audio processor (atomic, thread-safe)
    // Value is already in dB format
    float dbLevel = audioProcessor.getCurrentOutputLevel();

    // Emit event to JavaScript (only if WebView is visible)
    webView->emitEventIfBrowserIsVisible("updateVUMeter", dbLevel);
}

//==============================================================================
// Resource Provider (JUCE 8 Required Pattern - Pattern #8)
//==============================================================================

std::optional<juce::WebBrowserComponent::Resource> FlutterVerbAudioProcessorEditor::getResource(
    const juce::String& url
)
{
    // ========================================================================
    // RESOURCE PROVIDER IMPLEMENTATION (Pattern #8: Explicit URL Mapping)
    // ========================================================================
    //
    // Maps URLs to embedded binary data (from juce_add_binary_data).
    //
    // FlutterVerb UI structure:
    // - Source/ui/public/index.html                       → BinaryData::index_html
    // - Source/ui/public/js/juce/index.js                 → BinaryData::index_js
    // - Source/ui/public/js/juce/check_native_interop.js  → BinaryData::check_native_interop_js
    //
    // Why explicit mapping: BinaryData converts paths to C++ identifiers
    // (js/juce/index.js → index_js), but HTML requests original paths.
    // ========================================================================

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

    // JUCE frontend library (REQUIRED for parameter binding)
    if (url == "/js/juce/index.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::index_js, BinaryData::index_jsSize),
            juce::String("text/javascript")
        };
    }

    // JUCE native interop verification (Pattern #13: REQUIRED)
    if (url == "/js/juce/check_native_interop.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::check_native_interop_js, BinaryData::check_native_interop_jsSize),
            juce::String("text/javascript")
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
