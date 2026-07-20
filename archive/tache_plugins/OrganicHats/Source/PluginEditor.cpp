#include "PluginEditor.h"
#include "BinaryData.h"

OrganicHatsAudioProcessorEditor::OrganicHatsAudioProcessorEditor(OrganicHatsAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    // ⚠️ CRITICAL: Initialization order must match member declaration order
    // Order: relays → webView → attachments

    // Step 1: Create relays (BEFORE WebView)
    closedToneRelay = std::make_unique<juce::WebSliderRelay>("CLOSED_TONE");
    closedDecayRelay = std::make_unique<juce::WebSliderRelay>("CLOSED_DECAY");
    closedNoiseColorRelay = std::make_unique<juce::WebSliderRelay>("CLOSED_NOISE_COLOR");
    openToneRelay = std::make_unique<juce::WebSliderRelay>("OPEN_TONE");
    openReleaseRelay = std::make_unique<juce::WebSliderRelay>("OPEN_RELEASE");
    openNoiseColorRelay = std::make_unique<juce::WebSliderRelay>("OPEN_NOISE_COLOR");

    // Step 2: Create WebView with resource provider and relay options
    webView = std::make_unique<juce::WebBrowserComponent>(
        juce::WebBrowserComponent::Options{}
            .withNativeIntegrationEnabled()
            .withKeepPageLoadedWhenBrowserIsHidden()
            .withResourceProvider([this](const juce::String& url) { return getResource(url); })
            .withOptionsFrom(*closedToneRelay)
            .withOptionsFrom(*closedDecayRelay)
            .withOptionsFrom(*closedNoiseColorRelay)
            .withOptionsFrom(*openToneRelay)
            .withOptionsFrom(*openReleaseRelay)
            .withOptionsFrom(*openNoiseColorRelay)
    );

    addAndMakeVisible(*webView);

    // Step 3: Create attachments (AFTER WebView)
    // CRITICAL: Pattern #12 - Three parameters required (nullptr for undoManager)
    closedToneAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("CLOSED_TONE"),
        *closedToneRelay,
        nullptr
    );

    closedDecayAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("CLOSED_DECAY"),
        *closedDecayRelay,
        nullptr
    );

    closedNoiseColorAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("CLOSED_NOISE_COLOR"),
        *closedNoiseColorRelay,
        nullptr
    );

    openToneAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("OPEN_TONE"),
        *openToneRelay,
        nullptr
    );

    openReleaseAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("OPEN_RELEASE"),
        *openReleaseRelay,
        nullptr
    );

    openNoiseColorAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("OPEN_NOISE_COLOR"),
        *openNoiseColorRelay,
        nullptr
    );

    // Set window size (from mockup v2)
    setSize(600, 590);
    setResizable(false, false);

    // Navigate to index
    webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
}

OrganicHatsAudioProcessorEditor::~OrganicHatsAudioProcessorEditor()
{
    // Destruction happens in reverse order of member declaration (automatic)
}

void OrganicHatsAudioProcessorEditor::paint(juce::Graphics& g)
{
    // WebView fills entire component, no custom painting needed
    g.fillAll(juce::Colours::black);
}

void OrganicHatsAudioProcessorEditor::resized()
{
    // WebView fills entire editor bounds
    if (webView)
        webView->setBounds(getLocalBounds());
}

// CRITICAL: Pattern #8 - Explicit URL mapping for resource provider
std::optional<juce::WebBrowserComponent::Resource>
OrganicHatsAudioProcessorEditor::getResource(const juce::String& url)
{
    // Helper lambda to convert char array to vector<byte>
    auto makeVector = [](const char* data, int size) {
        return std::vector<std::byte>(
            reinterpret_cast<const std::byte*>(data),
            reinterpret_cast<const std::byte*>(data) + size
        );
    };

    // Explicit URL mapping - clear, debuggable, reliable
    if (url == "/" || url == "/index.html") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::index_html, BinaryData::index_htmlSize),
            juce::String("text/html")
        };
    }

    if (url == "/js/juce/index.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::index_js, BinaryData::index_jsSize),
            juce::String("text/javascript")
        };
    }

    if (url == "/js/juce/check_native_interop.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::check_native_interop_js, BinaryData::check_native_interop_jsSize),
            juce::String("text/javascript")
        };
    }

    // Resource not found
    juce::Logger::writeToLog("Resource not found: " + url);
    return std::nullopt;
}
