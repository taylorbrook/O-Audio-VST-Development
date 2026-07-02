#include "PluginEditor.h"
#include "BinaryData.h"

ScatterAudioProcessorEditor::ScatterAudioProcessorEditor(ScatterAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    // 1. Create relays FIRST (must match parameter IDs exactly)
    delayTimeRelay = std::make_unique<juce::WebSliderRelay>("delay_time");
    grainSizeRelay = std::make_unique<juce::WebSliderRelay>("grain_size");
    densityRelay = std::make_unique<juce::WebSliderRelay>("density");
    pitchRandomRelay = std::make_unique<juce::WebSliderRelay>("pitch_random");
    scaleRelay = std::make_unique<juce::WebComboBoxRelay>("scale");
    rootNoteRelay = std::make_unique<juce::WebComboBoxRelay>("root_note");
    panRandomRelay = std::make_unique<juce::WebSliderRelay>("pan_random");
    feedbackRelay = std::make_unique<juce::WebSliderRelay>("feedback");
    mixRelay = std::make_unique<juce::WebSliderRelay>("mix");

    // 2. Create WebView with relay options
    webView = std::make_unique<juce::WebBrowserComponent>(
        juce::WebBrowserComponent::Options{}
            .withNativeIntegrationEnabled()  // CRITICAL: Enables JUCE JavaScript library
            .withResourceProvider([this](const auto& url) { return getResource(url); })
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

    // 3. Create attachments LAST (Pattern #12: 3 parameters required)
    delayTimeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("delay_time"), *delayTimeRelay, nullptr);
    grainSizeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("grain_size"), *grainSizeRelay, nullptr);
    densityAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("density"), *densityRelay, nullptr);
    pitchRandomAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("pitch_random"), *pitchRandomRelay, nullptr);
    scaleAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *processorRef.parameters.getParameter("scale"), *scaleRelay, nullptr);
    rootNoteAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *processorRef.parameters.getParameter("root_note"), *rootNoteRelay, nullptr);
    panRandomAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("pan_random"), *panRandomRelay, nullptr);
    feedbackAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("feedback"), *feedbackRelay, nullptr);
    mixAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("mix"), *mixRelay, nullptr);

    // Add WebView to editor
    addAndMakeVisible(*webView);

    // Navigate to UI
    webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());

    // Set editor size (from mockup: 550x600px)
    setSize(550, 600);

    // Phase 4.2: Start timer for grain visualization updates (30Hz = ~33ms interval)
    startTimer(33);
}

ScatterAudioProcessorEditor::~ScatterAudioProcessorEditor()
{
    // Phase 4.2: Stop timer before cleanup
    stopTimer();

    // Smart pointers handle cleanup in reverse order
}

void ScatterAudioProcessorEditor::paint(juce::Graphics& g)
{
    // WebView handles all painting
    juce::ignoreUnused(g);
}

void ScatterAudioProcessorEditor::resized()
{
    // WebView fills entire editor
    webView->setBounds(getLocalBounds());
}

// Pattern #8: Explicit URL mapping (REQUIRED - no generic loops)
std::optional<juce::WebBrowserComponent::Resource>
ScatterAudioProcessorEditor::getResource(const juce::String& url)
{
    auto makeVector = [](const char* data, int size) {
        return std::vector<std::byte>(
            reinterpret_cast<const std::byte*>(data),
            reinterpret_cast<const std::byte*>(data) + size
        );
    };

    // Root → index.html
    if (url == "/" || url == "/index.html") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::index_html, BinaryData::index_htmlSize),
            juce::String("text/html")
        };
    }

    // JUCE JavaScript library (Pattern #13: Required for native integration)
    if (url == "/js/juce/index.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::index_js, BinaryData::index_jsSize),
            juce::String("text/javascript")
        };
    }

    if (url == "/js/juce/check_native_interop.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::check_native_interop_js,
                      BinaryData::check_native_interop_jsSize),
            juce::String("text/javascript")
        };
    }

    // Resource not found
    juce::Logger::writeToLog("Resource not found: " + url);
    return std::nullopt;
}

// ============================================================================
// Phase 4.2: Grain Visualization Timer Callback
// ============================================================================

void ScatterAudioProcessorEditor::timerCallback()
{
    // Get active grain positions from processor (thread-safe read)
    auto grainData = processorRef.getActiveGrainPositions();

    // Build JSON array for JavaScript
    juce::String jsonData = "[";

    for (size_t i = 0; i < grainData.size(); ++i)
    {
        const auto& grain = grainData[i];

        jsonData += "{\"x\":" + juce::String(grain.x, 4)
                 + ",\"y\":" + juce::String(grain.y, 4)
                 + ",\"pan\":" + juce::String(grain.pan, 4) + "}";

        if (i < grainData.size() - 1)
            jsonData += ",";
    }

    jsonData += "]";

    // Send to JavaScript via custom event
    if (webView != nullptr)
    {
        webView->emitEventIfBrowserIsVisible("grainUpdate", jsonData);
    }
}
