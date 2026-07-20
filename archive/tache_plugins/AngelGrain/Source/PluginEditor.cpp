#include "PluginEditor.h"
#include "BinaryData.h"

AngelGrainAudioProcessorEditor::AngelGrainAudioProcessorEditor(AngelGrainAudioProcessor& p)
    : AudioProcessorEditor(&p)
    , processorRef(p)
{
    // Initialize relays with parameter IDs (MUST match APVTS IDs exactly)
    delayTimeRelay = std::make_unique<juce::WebSliderRelay>("delayTime");
    grainSizeRelay = std::make_unique<juce::WebSliderRelay>("grainSize");
    feedbackRelay = std::make_unique<juce::WebSliderRelay>("feedback");
    chaosRelay = std::make_unique<juce::WebSliderRelay>("chaos");
    characterRelay = std::make_unique<juce::WebSliderRelay>("character");
    mixRelay = std::make_unique<juce::WebSliderRelay>("mix");
    tempoSyncRelay = std::make_unique<juce::WebToggleButtonRelay>("tempoSync");

    // Initialize WebView with options
    webView = std::make_unique<juce::WebBrowserComponent>(
        juce::WebBrowserComponent::Options{}
            .withNativeIntegrationEnabled()  // CRITICAL: Enables JUCE JavaScript library
            .withResourceProvider([this](const auto& url) { return getResource(url); })
            .withKeepPageLoadedWhenBrowserIsHidden()  // FL Studio fix
            .withOptionsFrom(*delayTimeRelay)   // Register each relay
            .withOptionsFrom(*grainSizeRelay)
            .withOptionsFrom(*feedbackRelay)
            .withOptionsFrom(*chaosRelay)
            .withOptionsFrom(*characterRelay)
            .withOptionsFrom(*mixRelay)
            .withOptionsFrom(*tempoSyncRelay)
    );

    // Initialize attachments (connect parameters to relays)
    // PATTERN 12: WebSliderParameterAttachment requires 3 parameters (add nullptr for undoManager)
    delayTimeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("delayTime"), *delayTimeRelay, nullptr);
    grainSizeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("grainSize"), *grainSizeRelay, nullptr);
    feedbackAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("feedback"), *feedbackRelay, nullptr);
    chaosAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("chaos"), *chaosRelay, nullptr);
    characterAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("character"), *characterRelay, nullptr);
    mixAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("mix"), *mixRelay, nullptr);
    tempoSyncAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.parameters.getParameter("tempoSync"), *tempoSyncRelay, nullptr);

    // Add WebView to editor
    addAndMakeVisible(*webView);

    // Navigate to UI
    webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());

    // Set editor size to match mockup dimensions (540x480 from v1-ui.yaml)
    setSize(540, 480);
}

AngelGrainAudioProcessorEditor::~AngelGrainAudioProcessorEditor()
{
}

void AngelGrainAudioProcessorEditor::paint(juce::Graphics& g)
{
    // WebView handles all painting
    juce::ignoreUnused(g);
}

void AngelGrainAudioProcessorEditor::resized()
{
    // WebView fills entire editor
    webView->setBounds(getLocalBounds());
}

std::optional<juce::WebBrowserComponent::Resource>
AngelGrainAudioProcessorEditor::getResource(const juce::String& url)
{
    // PATTERN 8: Explicit URL mapping (DON'T use generic loop)
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

    // JUCE frontend library
    if (url == "/js/juce/index.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::index_js, BinaryData::index_jsSize),
            juce::String("text/javascript")
        };
    }

    // JUCE native interop checker (PATTERN 13: Required for WebView)
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
