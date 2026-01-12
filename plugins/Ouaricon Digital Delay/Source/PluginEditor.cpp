/*
  ==============================================================================

    Ouaricon Digital Delay - Editor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginEditor.h"
#include "BinaryData.h"

OuariconDigitalDelayAudioProcessorEditor::OuariconDigitalDelayAudioProcessorEditor(OuariconDigitalDelayAudioProcessor& p)
    : AudioProcessorEditor(&p)
    , processorRef(p)
    // 1. Create relays FIRST (no dependencies)
    , timeRelay(std::make_unique<juce::WebSliderRelay>("time"))
    , feedbackRelay(std::make_unique<juce::WebSliderRelay>("feedback"))
    , spreadRelay(std::make_unique<juce::WebSliderRelay>("spread"))
    , modRelay(std::make_unique<juce::WebSliderRelay>("mod"))
    , wetRelay(std::make_unique<juce::WebSliderRelay>("wet"))
    , dryRelay(std::make_unique<juce::WebSliderRelay>("dry"))
    , syncRelay(std::make_unique<juce::WebToggleButtonRelay>("sync"))
    , divisionRelay(std::make_unique<juce::WebComboBoxRelay>("division"))
    // 2. Create WebView SECOND (depends on relays)
    , webView(std::make_unique<juce::WebBrowserComponent>(
        juce::WebBrowserComponent::Options{}
            .withNativeIntegrationEnabled()
            .withResourceProvider([this](const auto& url) { return getResource(url); })
            .withOptionsFrom(*timeRelay)
            .withOptionsFrom(*feedbackRelay)
            .withOptionsFrom(*spreadRelay)
            .withOptionsFrom(*modRelay)
            .withOptionsFrom(*wetRelay)
            .withOptionsFrom(*dryRelay)
            .withOptionsFrom(*syncRelay)
            .withOptionsFrom(*divisionRelay)
    ))
    // 3. Create attachments LAST (depend on relays AND webView)
    , timeAttachment(std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("time"), *timeRelay, nullptr))
    , feedbackAttachment(std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("feedback"), *feedbackRelay, nullptr))
    , spreadAttachment(std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("spread"), *spreadRelay, nullptr))
    , modAttachment(std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("mod"), *modRelay, nullptr))
    , wetAttachment(std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("wet"), *wetRelay, nullptr))
    , dryAttachment(std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("dry"), *dryRelay, nullptr))
    , syncAttachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.parameters.getParameter("sync"), *syncRelay, nullptr))
    , divisionAttachment(std::make_unique<juce::WebComboBoxParameterAttachment>(
        *processorRef.parameters.getParameter("division"), *divisionRelay, nullptr))
{
    // Window size: 700×196px (from v7-ui.yaml)
    setSize(700, 196);
    setResizable(false, false);

    // Add WebView to component hierarchy
    addAndMakeVisible(*webView);

    // Load UI from resource provider
    webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());

    // Start timer for RMS meter updates (30 Hz)
    startTimerHz(30);
}

OuariconDigitalDelayAudioProcessorEditor::~OuariconDigitalDelayAudioProcessorEditor()
{
    // Stop timer before destruction
    stopTimer();

    // Destruction order is automatic (reverse of declaration):
    // 1. Attachments destroyed first (safe to call webView methods)
    // 2. WebView destroyed second
    // 3. Relays destroyed last
}

void OuariconDigitalDelayAudioProcessorEditor::paint(juce::Graphics& g)
{
    // WebView handles all rendering
    g.fillAll(juce::Colours::black);
}

void OuariconDigitalDelayAudioProcessorEditor::resized()
{
    // Fill entire editor area with WebView
    webView->setBounds(getLocalBounds());
}

void OuariconDigitalDelayAudioProcessorEditor::timerCallback()
{
    // Get RMS levels from processor (average L+R for mono meter)
    float rmsLeft = processorRef.getRmsLevelLeft();
    float rmsRight = processorRef.getRmsLevelRight();
    float rmsLevel = (rmsLeft + rmsRight) * 0.5f;

    // Clamp to 0-1 range
    rmsLevel = juce::jlimit(0.0f, 1.0f, rmsLevel);

    // Send to WebView via JavaScript evaluation
    juce::String js = "if (typeof updateLEDMeter === 'function') { updateLEDMeter(" +
                      juce::String(rmsLevel, 4) + "); }";
    webView->evaluateJavascript(js, nullptr);
}

//==============================================================================
/**
 * Resource provider for WebView
 *
 * CRITICAL: Explicit URL mapping (not generic loop)
 * BinaryData flattens paths: js/juce/index.js -> index_js
 * HTML requests original paths: /js/juce/index.js
 */
std::optional<juce::WebBrowserComponent::Resource>
OuariconDigitalDelayAudioProcessorEditor::getResource(const juce::String& url)
{
    // Helper to convert BinaryData to std::vector<std::byte>
    auto makeVector = [](const char* data, int size) {
        return std::vector<std::byte>(
            reinterpret_cast<const std::byte*>(data),
            reinterpret_cast<const std::byte*>(data) + size
        );
    };

    // Explicit URL mapping (clear, debuggable, reliable)
    if (url == "/" || url == "/index.html") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::index_html, BinaryData::index_htmlSize),
            juce::String("text/html")
        };
    }

    if (url == "/js/juce/index.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::index_js, BinaryData::index_jsSize),
            juce::String("application/javascript")
        };
    }

    if (url == "/js/juce/check_native_interop.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::check_native_interop_js,
                      BinaryData::check_native_interop_jsSize),
            juce::String("application/javascript")
        };
    }

    // Image assets (from v7 mockup)
    if (url == "/img/paper1.jpg") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::paper1_jpg, BinaryData::paper1_jpgSize),
            juce::String("image/jpeg")
        };
    }

    if (url == "/img/butterfly2_Black and white.png") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::butterfly2_Black_and_white_png,
                      BinaryData::butterfly2_Black_and_white_pngSize),
            juce::String("image/png")
        };
    }

    // 404 - resource not found
    return std::nullopt;
}
