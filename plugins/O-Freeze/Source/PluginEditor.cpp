/*
  ==============================================================================

    O-Freeze - Editor Implementation
    Ouaricon Development
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginEditor.h"
#include "BinaryData.h"

OFreezeAudioProcessorEditor::OFreezeAudioProcessorEditor(OFreezeAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    // ============================================================================
    // 1. Create relays FIRST (with parameter IDs matching APVTS)
    // ============================================================================

    thresholdRelay = std::make_unique<juce::WebSliderRelay>("THRESHOLD");
    driftRelay = std::make_unique<juce::WebSliderRelay>("DRIFT");
    grainSizeRelay = std::make_unique<juce::WebSliderRelay>("GRAIN_SIZE");
    grainCountRelay = std::make_unique<juce::WebSliderRelay>("GRAIN_COUNT");
    mixRelay = std::make_unique<juce::WebSliderRelay>("MIX");
    detuneRelay = std::make_unique<juce::WebSliderRelay>("DETUNE");
    freezeRelay = std::make_unique<juce::WebToggleButtonRelay>("FREEZE");
    reverseRelay = std::make_unique<juce::WebToggleButtonRelay>("REVERSE");
    modeRelay = std::make_unique<juce::WebComboBoxRelay>("MODE");
    lfoRateRelay = std::make_unique<juce::WebSliderRelay>("LFO_RATE");
    lfoDepthRelay = std::make_unique<juce::WebSliderRelay>("LFO_DEPTH");
    lfoShapeRelay = std::make_unique<juce::WebComboBoxRelay>("LFO_SHAPE");

    // ============================================================================
    // 2. Create WebView SECOND with all relay options registered
    // ============================================================================

    webView = std::make_unique<juce::WebBrowserComponent>(
        juce::WebBrowserComponent::Options{}
            .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
            .withWinWebView2Options(
                juce::WebBrowserComponent::Options::WinWebView2{}
                    .withUserDataFolder(juce::File::getSpecialLocation(
                        juce::File::SpecialLocationType::tempDirectory)))
            .withNativeIntegrationEnabled()
            .withResourceProvider([this](auto& url) { return getResource(url); })
            .withOptionsFrom(*thresholdRelay)
            .withOptionsFrom(*driftRelay)
            .withOptionsFrom(*grainSizeRelay)
            .withOptionsFrom(*grainCountRelay)
            .withOptionsFrom(*mixRelay)
            .withOptionsFrom(*detuneRelay)
            .withOptionsFrom(*freezeRelay)
            .withOptionsFrom(*reverseRelay)
            .withOptionsFrom(*modeRelay)
            .withOptionsFrom(*lfoRateRelay)
            .withOptionsFrom(*lfoDepthRelay)
            .withOptionsFrom(*lfoShapeRelay)
    );

    // ============================================================================
    // 3. Create attachments LAST (connect parameters to relays)
    // ============================================================================

    thresholdAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.getAPVTS().getParameter("THRESHOLD"), *thresholdRelay, nullptr);
    driftAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.getAPVTS().getParameter("DRIFT"), *driftRelay, nullptr);
    grainSizeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.getAPVTS().getParameter("GRAIN_SIZE"), *grainSizeRelay, nullptr);
    grainCountAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.getAPVTS().getParameter("GRAIN_COUNT"), *grainCountRelay, nullptr);
    mixAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.getAPVTS().getParameter("MIX"), *mixRelay, nullptr);
    detuneAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.getAPVTS().getParameter("DETUNE"), *detuneRelay, nullptr);
    freezeAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.getAPVTS().getParameter("FREEZE"), *freezeRelay, nullptr);
    reverseAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.getAPVTS().getParameter("REVERSE"), *reverseRelay, nullptr);
    modeAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *processorRef.getAPVTS().getParameter("MODE"), *modeRelay, nullptr);
    lfoRateAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.getAPVTS().getParameter("LFO_RATE"), *lfoRateRelay, nullptr);
    lfoDepthAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.getAPVTS().getParameter("LFO_DEPTH"), *lfoDepthRelay, nullptr);
    lfoShapeAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *processorRef.getAPVTS().getParameter("LFO_SHAPE"), *lfoShapeRelay, nullptr);

    // ============================================================================
    // Add WebView to editor (navigation happens in parentHierarchyChanged)
    // ============================================================================

    addAndMakeVisible(*webView);

    // Set editor size (550x530 to accommodate 6 knobs + LFO group)
    setSize(550, 530);
}

OFreezeAudioProcessorEditor::~OFreezeAudioProcessorEditor()
{
}

void OFreezeAudioProcessorEditor::paint(juce::Graphics& g)
{
    // WebView handles all rendering
    juce::ignoreUnused(g);
}

void OFreezeAudioProcessorEditor::resized()
{
    // WebView fills entire editor
    webView->setBounds(getLocalBounds());
}

void OFreezeAudioProcessorEditor::parentHierarchyChanged()
{
    // Navigate WebView only after editor is attached to a window (JUCE 8 requirement)
    // This prevents crashes during plugin scanning when no window context exists
    if (isShowing() && webView != nullptr && !hasNavigated)
    {
        webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
        hasNavigated = true;
    }
}

std::optional<juce::WebBrowserComponent::Resource>
OFreezeAudioProcessorEditor::getResource(const juce::String& url)
{
    auto makeVector = [](const char* data, int size) {
        return std::vector<std::byte>(
            reinterpret_cast<const std::byte*>(data),
            reinterpret_cast<const std::byte*>(data) + size
        );
    };

    // Root "/" → index.html
    if (url == "/" || url == "/index.html") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::index_html, BinaryData::index_htmlSize),
            juce::String("text/html")
        };
    }

    // JUCE JavaScript bridge
    if (url == "/js/juce/index.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::index_js, BinaryData::index_jsSize),
            juce::String("text/javascript")
        };
    }

    // JUCE interop checker
    if (url == "/js/juce/check_native_interop.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::check_native_interop_js,
                      BinaryData::check_native_interop_jsSize),
            juce::String("text/javascript")
        };
    }

    // Paper texture
    if (url == "/assets/paper1.jpg") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::paper1_jpg, BinaryData::paper1_jpgSize),
            juce::String("image/jpeg")
        };
    }

    // Anatomical overlay
    if (url == "/assets/muscles.png") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::muscles_png, BinaryData::muscles_pngSize),
            juce::String("image/png")
        };
    }

    // Resource not found
    juce::Logger::writeToLog("Resource not found: " + url);
    return std::nullopt;
}
