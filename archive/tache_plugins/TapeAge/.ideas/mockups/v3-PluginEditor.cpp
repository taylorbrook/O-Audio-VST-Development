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
    // 1. Create relays FIRST (before WebView construction)
    // 2. Create WebView with relay options
    // 3. Create parameter attachments LAST (after WebView construction)
    // ========================================================================

    // ------------------------------------------------------------------------
    // STEP 1: CREATE RELAYS (before WebView!)
    // ------------------------------------------------------------------------
    driveRelay = std::make_unique<juce::WebSliderRelay>("drive");
    ageRelay = std::make_unique<juce::WebSliderRelay>("age");
    mixRelay = std::make_unique<juce::WebSliderRelay>("mix");

    // ------------------------------------------------------------------------
    // STEP 2: CREATE WEBVIEW (with relay options)
    // ------------------------------------------------------------------------
    webView = std::make_unique<juce::WebBrowserComponent>(
        juce::WebBrowserComponent::Options{}
            // Enable JUCE frontend library
            .withNativeIntegrationEnabled()

            // Resource provider for embedded files
            .withResourceProvider([this](const auto& url) {
                return getResource(url);
            })

            // FL Studio fix (prevents blank screen on focus loss)
            .withKeepPageLoadedWhenBrowserIsHidden()

            // Register relays with WebView
            .withOptionsFrom(*driveRelay)
            .withOptionsFrom(*ageRelay)
            .withOptionsFrom(*mixRelay)
    );

    // ------------------------------------------------------------------------
    // STEP 3: CREATE PARAMETER ATTACHMENTS (after WebView!)
    // ------------------------------------------------------------------------
    driveAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("drive"),
        *driveRelay,
        nullptr  // No undo manager
    );

    ageAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("age"),
        *ageRelay,
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
    webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
    addAndMakeVisible(*webView);

    // ------------------------------------------------------------------------
    // TIMER SETUP (for VU meter updates)
    // ------------------------------------------------------------------------
    // 60ms = ~16 FPS, sufficient for meter updates
    startTimerHz(16);

    // ------------------------------------------------------------------------
    // WINDOW SIZE
    // ------------------------------------------------------------------------
    setSize(500, 450);
    setResizable(false, false);
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
// Timer Callback (VU meter updates)
//==============================================================================

void PluginEditor::timerCallback()
{
    // Example: Update VU meter from audio thread data
    // In production, PluginProcessor would store current level in std::atomic
    //
    // float dbLevel = audioProcessor.getCurrentOutputLevel();  // Atomic read
    //
    // webView->emitEventIfBrowserIsVisible("meterUpdate", {
    //     { "level", dbLevel }
    // });
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
