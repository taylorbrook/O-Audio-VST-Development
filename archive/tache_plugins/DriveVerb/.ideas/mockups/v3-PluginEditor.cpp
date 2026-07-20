#include "PluginEditor.h"

//==============================================================================
// Constructor - CRITICAL: Initialize in correct order
//==============================================================================

DriveVerbAudioProcessorEditor::DriveVerbAudioProcessorEditor(DriveVerbAudioProcessor& p)
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

    sizeRelay = std::make_unique<juce::WebSliderRelay>("size");
    decayRelay = std::make_unique<juce::WebSliderRelay>("decay");
    dryWetRelay = std::make_unique<juce::WebSliderRelay>("dryWet");
    driveRelay = std::make_unique<juce::WebSliderRelay>("drive");
    filterRelay = std::make_unique<juce::WebSliderRelay>("filter");
    filterPositionRelay = std::make_unique<juce::WebToggleButtonRelay>("filterPosition");

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
            .withOptionsFrom(*sizeRelay)
            .withOptionsFrom(*decayRelay)
            .withOptionsFrom(*dryWetRelay)
            .withOptionsFrom(*driveRelay)
            .withOptionsFrom(*filterRelay)
            .withOptionsFrom(*filterPositionRelay)
    );

    // ------------------------------------------------------------------------
    // STEP 3: CREATE PARAMETER ATTACHMENTS (after WebView!)
    // ------------------------------------------------------------------------

    sizeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("size"),
        *sizeRelay,
        nullptr  // No undo manager
    );

    decayAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("decay"),
        *decayRelay,
        nullptr
    );

    dryWetAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("dryWet"),
        *dryWetRelay,
        nullptr
    );

    driveAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("drive"),
        *driveRelay,
        nullptr
    );

    filterAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("filter"),
        *filterRelay,
        nullptr
    );

    filterPositionAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *audioProcessor.apvts.getParameter("filterPosition"),
        *filterPositionRelay,
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
    // TIMER SETUP (for VU meter updates)
    // ------------------------------------------------------------------------

    // 60ms timer = ~16 FPS (sufficient for VU meter ballistics)
    startTimerHz(16);

    // ------------------------------------------------------------------------
    // WINDOW SIZE
    // ------------------------------------------------------------------------

    setSize(1000, 500);
    setResizable(false, false);
}

//==============================================================================
// Destructor
//==============================================================================

DriveVerbAudioProcessorEditor::~DriveVerbAudioProcessorEditor()
{
    // Stop timer before destruction
    stopTimer();

    // Members destroyed in reverse order:
    // 1. Attachments (stop using relays and WebView)
    // 2. WebView (safe, attachments are gone)
    // 3. Relays (safe, nothing using them)
}

//==============================================================================
// AudioProcessorEditor overrides
//==============================================================================

void DriveVerbAudioProcessorEditor::paint(juce::Graphics& g)
{
    // WebView fills entire area, no painting needed
    g.fillAll(juce::Colours::black);
}

void DriveVerbAudioProcessorEditor::resized()
{
    // WebView fills entire editor area
    if (webView)
        webView->setBounds(getLocalBounds());
}

//==============================================================================
// Timer callback (VU meter updates)
//==============================================================================

void DriveVerbAudioProcessorEditor::timerCallback()
{
    // Get current drive level from processor
    // (Processor should track peak level for metering)
    float driveDB = audioProcessor.getCurrentDriveLevel();

    // Send meter update to JavaScript
    if (webView)
    {
        juce::String js = juce::String::formatted(
            "if (window.__JUCE__ && window.__JUCE__.backend) {"
            "  window.__JUCE__.backend.emitEvent('updateVUMeter', %f);"
            "}",
            driveDB
        );

        webView->evaluateJavascript(js);
    }
}

//==============================================================================
// Resource provider
//==============================================================================

std::optional<juce::WebBrowserComponent::Resource>
DriveVerbAudioProcessorEditor::getResource(const juce::String& url)
{
    // Map URLs to binary data embedded via juce_add_binary_data
    //
    // Binary data naming convention:
    // - File: Source/ui/public/index.html      → BinaryData::index_html
    // - File: Source/ui/public/js/juce/index.js → BinaryData::juce_index_js
    //
    // Dots and hyphens replaced with underscores, extension removed

    if (url == "/" || url == "/index.html")
    {
        return juce::WebBrowserComponent::Resource {
            BinaryData::index_html,
            BinaryData::index_htmlSize,
            "text/html"
        };
    }

    if (url == "/js/juce/index.js")
    {
        return juce::WebBrowserComponent::Resource {
            BinaryData::juce_index_js,
            BinaryData::juce_index_jsSize,
            "text/javascript"
        };
    }

    // 404 - resource not found
    return std::nullopt;
}
