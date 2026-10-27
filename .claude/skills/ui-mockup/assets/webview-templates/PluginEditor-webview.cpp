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
    // - withKeepPageLoadedWhenBrowserIsHidden() - REQUIRED (hosts that hide the
    //   editor, e.g. FL Studio, otherwise reload the page as about:blank)
    //
    auto options = juce::WebBrowserComponent::Options{}
        // REQUIRED: Enable JUCE frontend library
        .withNativeIntegrationEnabled()

        // REQUIRED: Resource provider for embedded files
        .withResourceProvider([this](const auto& url) {
            return getResource(url);
        })

        // REQUIRED: keep the page alive while the host hides the editor
        .withKeepPageLoadedWhenBrowserIsHidden()

        // REQUIRED: Register each relay with WebView
        // This creates JavaScript state objects accessible via:
        // - Juce.getSliderState("PARAM_ID")
        // - Juce.getToggleState("PARAM_ID")
        // - Juce.getComboBoxState("PARAM_ID")
        //
        // {{WEBVIEW_OPTIONS}}
        // Example:
        // .withOptionsFrom(*gainRelay)
        // .withOptionsFrom(*toneRelay)
        // .withOptionsFrom(*bypassRelay)
        // .withOptionsFrom(*modeRelay)
        ;

   #if JUCE_WINDOWS
    // REQUIRED on Windows: name the backend. Backend::defaultBackend is the
    // legacy IE control, which ignores withWinWebView2Options and has no
    // resource provider ("page not found").
    // The user-data folder lives in the per-user app-data tree (NOT %TEMP%,
    // which cleaners wipe and other processes lock) and is created before
    // WebView2 sees it. DAW hosts deny WebView2's default location.
    const auto webView2DataFolder = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                                        .getChildFile ("Ouaricon")
                                        .getChildFile ("WebView2")
                                        .getChildFile ("{{PLUGIN_NAME}}");
    webView2DataFolder.createDirectory();

    options = options
        .withBackend (juce::WebBrowserComponent::Options::Backend::webview2)
        .withWinWebView2Options (
            juce::WebBrowserComponent::Options::WinWebView2{}
                .withUserDataFolder (webView2DataFolder)
                .withStatusBarDisabled()
                .withBuiltInErrorPageDisabled());

    // When the WebView2 environment cannot be created (runtime missing, data
    // folder unusable), JUCE silently builds the IE control instead. Probe
    // first; on failure webView stays null and paint() shows a native notice.
    // Audio, parameters and host automation do not depend on the WebView.
    const bool webView2Available = juce::WebBrowserComponent::areOptionsSupported (options);
   #else
    constexpr bool webView2Available = true;
   #endif

    if (webView2Available)
        webView = std::make_unique<juce::WebBrowserComponent> (options);

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

    // Attachments above stay in place without a WebView: the relays drop their
    // events while no browser is registered.
    if (webView != nullptr)
    {
        // Navigate to root (loads index.html via resource provider)
        // JUCE 8 pattern: Use resource provider root instead of data URLs
        addAndMakeVisible(*webView);
        webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
    }
    else
    {
        // Windows only: WebView2 could not start (see paint()).
        webView2RuntimeLink = std::make_unique<juce::HyperlinkButton> (
            "Download the WebView2 Runtime",
            juce::URL ("https://developer.microsoft.com/microsoft-edge/webview2/"));
        webView2RuntimeLink->setFont (juce::FontOptions (17.0f, juce::Font::underlined), false);
        webView2RuntimeLink->setColour (juce::HyperlinkButton::textColourId, {{ACCENT_COLOUR}});
        addAndMakeVisible (*webView2RuntimeLink);
    }

    // EVERY other webView-> use (timerCallback, evaluateJavascript, emitEvent…)
    // must be guarded with `if (webView != nullptr)` — it is null on Windows
    // machines without the WebView2 Runtime.

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
    // WebView fills the editor — nothing to paint, unless WebView2 could not
    // start (Windows only). Colours come from the page's own CSS tokens.
    if (webView != nullptr)
        return;

    g.fillAll ({{BACKGROUND_COLOUR}});

    auto area = getLocalBounds().withSizeKeepingCentre (juce::jmin (640, getWidth() - 48), 220)
                                .translated (0, -40);

    g.setColour ({{TEXT_COLOUR}});
    g.setFont (juce::FontOptions (26.0f, juce::Font::bold));
    g.drawFittedText ("{{PLUGIN_NAME}} needs Microsoft Edge WebView2",
                      area.removeFromTop (40), juce::Justification::centred, 2);

    area.removeFromTop (12);
    g.setFont (juce::FontOptions (17.0f));
    g.drawFittedText ("The interface is drawn with the WebView2 Runtime, which could not start on "
                      "this computer. Install it, then close and reopen the plugin window.\n"
                      "The {{PLUGIN_KIND}} itself keeps working, and host automation still "
                      "reaches every parameter.",
                      area.removeFromTop (110), juce::Justification::centred, 5);
}

void PluginEditor::resized()
{
    // Make WebView fill the entire editor bounds (null-safe: see constructor)
    if (webView != nullptr)
        webView->setBounds(getLocalBounds());

    if (webView2RuntimeLink != nullptr)
        webView2RuntimeLink->setBounds (getLocalBounds().withSizeKeepingCentre (320, 32)
                                                        .translated (0, 40));
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
    // if (webView == nullptr) return;  // WebView2 unavailable (Windows)
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
