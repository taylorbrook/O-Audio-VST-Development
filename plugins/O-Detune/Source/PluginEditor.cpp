/*
  ==============================================================================

    O-Detune - Editor Implementation
    Ouaricon Development
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginEditor.h"
#include "BinaryData.h"

ODetuneAudioProcessorEditor::ODetuneAudioProcessorEditor(ODetuneAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    // ============================================================================
    // 1. Create relays FIRST (with parameter IDs matching APVTS and HTML)
    // ============================================================================

    // Float parameter relays
    blendRelay = std::make_unique<juce::WebSliderRelay>("blend");
    wobbleRateRelay = std::make_unique<juce::WebSliderRelay>("wobble_rate");
    wobbleDepthRelay = std::make_unique<juce::WebSliderRelay>("wobble_depth");
    unisonDetuneRelay = std::make_unique<juce::WebSliderRelay>("unison_detune");
    unisonSpreadRelay = std::make_unique<juce::WebSliderRelay>("unison_spread");
    driveRelay = std::make_unique<juce::WebSliderRelay>("drive");
    colorRelay = std::make_unique<juce::WebSliderRelay>("color");
    ageRelay = std::make_unique<juce::WebSliderRelay>("age");
    widthRelay = std::make_unique<juce::WebSliderRelay>("width");
    mixRelay = std::make_unique<juce::WebSliderRelay>("mix");
    focusLowRelay = std::make_unique<juce::WebSliderRelay>("focus_low");
    focusHighRelay = std::make_unique<juce::WebSliderRelay>("focus_high");
    delayRelay = std::make_unique<juce::WebSliderRelay>("delay");
    feedbackRelay = std::make_unique<juce::WebSliderRelay>("feedback");
    randomAmtRelay = std::make_unique<juce::WebSliderRelay>("random_amt");

    // Choice parameter relays
    wobbleEraRelay = std::make_unique<juce::WebComboBoxRelay>("wobble_era");
    wobbleShapeRelay = std::make_unique<juce::WebComboBoxRelay>("wobble_shape");
    unisonVoicesRelay = std::make_unique<juce::WebComboBoxRelay>("unison_voices");
    unisonDistRelay = std::make_unique<juce::WebComboBoxRelay>("unison_dist");

    // Bool parameter relays
    wobbleSyncRelay = std::make_unique<juce::WebToggleButtonRelay>("wobble_sync");
    monoSafeRelay = std::make_unique<juce::WebToggleButtonRelay>("mono_safe");

    // ============================================================================
    // 2. Create WebView SECOND with all relay options registered
    // ============================================================================

    webView = std::make_unique<juce::WebBrowserComponent>(
        juce::WebBrowserComponent::Options{}
            .withNativeIntegrationEnabled()
            .withResourceProvider([this](auto& url) { return getResource(url); })
            // Register all float parameter relays
            .withOptionsFrom(*blendRelay)
            .withOptionsFrom(*wobbleRateRelay)
            .withOptionsFrom(*wobbleDepthRelay)
            .withOptionsFrom(*unisonDetuneRelay)
            .withOptionsFrom(*unisonSpreadRelay)
            .withOptionsFrom(*driveRelay)
            .withOptionsFrom(*colorRelay)
            .withOptionsFrom(*ageRelay)
            .withOptionsFrom(*widthRelay)
            .withOptionsFrom(*mixRelay)
            .withOptionsFrom(*focusLowRelay)
            .withOptionsFrom(*focusHighRelay)
            .withOptionsFrom(*delayRelay)
            .withOptionsFrom(*feedbackRelay)
            .withOptionsFrom(*randomAmtRelay)
            // Register all choice parameter relays
            .withOptionsFrom(*wobbleEraRelay)
            .withOptionsFrom(*wobbleShapeRelay)
            .withOptionsFrom(*unisonVoicesRelay)
            .withOptionsFrom(*unisonDistRelay)
            // Register all bool parameter relays
            .withOptionsFrom(*wobbleSyncRelay)
            .withOptionsFrom(*monoSafeRelay)
    );

    // ============================================================================
    // 3. Create attachments LAST (Pattern #12: 3 parameters)
    // ============================================================================

    // Float parameter attachments
    blendAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("blend"), *blendRelay, nullptr);
    wobbleRateAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("wobble_rate"), *wobbleRateRelay, nullptr);
    wobbleDepthAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("wobble_depth"), *wobbleDepthRelay, nullptr);
    unisonDetuneAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("unison_detune"), *unisonDetuneRelay, nullptr);
    unisonSpreadAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("unison_spread"), *unisonSpreadRelay, nullptr);
    driveAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("drive"), *driveRelay, nullptr);
    colorAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("color"), *colorRelay, nullptr);
    ageAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("age"), *ageRelay, nullptr);
    widthAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("width"), *widthRelay, nullptr);
    mixAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("mix"), *mixRelay, nullptr);
    focusLowAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("focus_low"), *focusLowRelay, nullptr);
    focusHighAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("focus_high"), *focusHighRelay, nullptr);
    delayAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("delay"), *delayRelay, nullptr);
    feedbackAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("feedback"), *feedbackRelay, nullptr);
    randomAmtAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("random_amt"), *randomAmtRelay, nullptr);

    // Choice parameter attachments
    wobbleEraAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *processorRef.parameters.getParameter("wobble_era"), *wobbleEraRelay, nullptr);
    wobbleShapeAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *processorRef.parameters.getParameter("wobble_shape"), *wobbleShapeRelay, nullptr);
    unisonVoicesAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *processorRef.parameters.getParameter("unison_voices"), *unisonVoicesRelay, nullptr);
    unisonDistAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *processorRef.parameters.getParameter("unison_dist"), *unisonDistRelay, nullptr);

    // Bool parameter attachments
    wobbleSyncAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.parameters.getParameter("wobble_sync"), *wobbleSyncRelay, nullptr);
    monoSafeAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.parameters.getParameter("mono_safe"), *monoSafeRelay, nullptr);

    // ============================================================================
    // Add WebView to editor (navigation happens in parentHierarchyChanged)
    // ============================================================================

    addAndMakeVisible(*webView);

    // Set editor size (600x520 for Ouaricon Naturalist aesthetic)
    setSize(600, 520);
}

ODetuneAudioProcessorEditor::~ODetuneAudioProcessorEditor()
{
    // Members destroyed in REVERSE order of declaration (automatic cleanup)
}

void ODetuneAudioProcessorEditor::paint(juce::Graphics& g)
{
    // WebView handles all painting
    juce::ignoreUnused(g);
}

void ODetuneAudioProcessorEditor::resized()
{
    // WebView fills entire editor
    webView->setBounds(getLocalBounds());
}

void ODetuneAudioProcessorEditor::parentHierarchyChanged()
{
    // Navigate WebView only after editor is attached to a window (JUCE 8 requirement)
    // This prevents crashes during plugin scanning when no window context exists
    // Use member variable instead of static to allow GUI reload on reopen
    if (isShowing() && webView != nullptr && !hasNavigated)
    {
        webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
        hasNavigated = true;
    }
}

// ============================================================================
// Pattern #8: EXPLICIT URL MAPPING (never use generic loops)
// ============================================================================
std::optional<juce::WebBrowserComponent::Resource>
ODetuneAudioProcessorEditor::getResource(const juce::String& url)
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

    // JUCE interop checker (Pattern #13: REQUIRED)
    if (url == "/js/juce/check_native_interop.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::check_native_interop_js,
                      BinaryData::check_native_interop_jsSize),
            juce::String("text/javascript")
        };
    }

    // Ouaricon Naturalist assets
    if (url == "/assets/paper1.jpg") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::paper1_jpg, BinaryData::paper1_jpgSize),
            juce::String("image/jpeg")
        };
    }

    if (url == "/assets/slug.png") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::slug_png, BinaryData::slug_pngSize),
            juce::String("image/png")
        };
    }

    // Resource not found
    juce::Logger::writeToLog("Resource not found: " + url);
    return std::nullopt;
}
