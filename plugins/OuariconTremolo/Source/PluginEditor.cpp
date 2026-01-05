#include "PluginEditor.h"
#include "BinaryData.h"

OuariconTremoloAudioProcessorEditor::OuariconTremoloAudioProcessorEditor(OuariconTremoloAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    setSize(600, 400);

    // 1. Create relays FIRST (with parameter IDs matching HTML)
    speedRelay = std::make_unique<juce::WebSliderRelay>("speed");
    depthRelay = std::make_unique<juce::WebSliderRelay>("depth");
    waveformRelay = std::make_unique<juce::WebSliderRelay>("waveform");
    smoothingRelay = std::make_unique<juce::WebSliderRelay>("smoothing");
    panSyncRelay = std::make_unique<juce::WebToggleButtonRelay>("panSync");
    tempoSyncRelay = std::make_unique<juce::WebToggleButtonRelay>("tempoSync");

    // 2. Create WebView SECOND with all relay options
    webView = std::make_unique<juce::WebBrowserComponent>(
        juce::WebBrowserComponent::Options{}
            .withNativeIntegrationEnabled()
            .withResourceProvider([this](auto& url) { return getResource(url); })
            .withOptionsFrom(*speedRelay)
            .withOptionsFrom(*depthRelay)
            .withOptionsFrom(*waveformRelay)
            .withOptionsFrom(*smoothingRelay)
            .withOptionsFrom(*panSyncRelay)
            .withOptionsFrom(*tempoSyncRelay)
    );

    // 3. Create attachments LAST (Pattern #12: 3 parameters - parameter, relay, nullptr)
    speedAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("SPEED_PARAM"), *speedRelay, nullptr);
    depthAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("DEPTH_PARAM"), *depthRelay, nullptr);
    waveformAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("WAVEFORM_PARAM"), *waveformRelay, nullptr);
    smoothingAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("SMOOTHING_PARAM"), *smoothingRelay, nullptr);
    panSyncAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.parameters.getParameter("PAN_SYNC_PARAM"), *panSyncRelay, nullptr);
    tempoSyncAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.parameters.getParameter("TEMPO_SYNC_PARAM"), *tempoSyncRelay, nullptr);

    // Add and navigate WebView
    addAndMakeVisible(*webView);
    webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
}

OuariconTremoloAudioProcessorEditor::~OuariconTremoloAudioProcessorEditor()
{
    // Members destroyed in REVERSE order of declaration (automatic cleanup)
}

void OuariconTremoloAudioProcessorEditor::paint(juce::Graphics& g)
{
    // WebView handles all painting
    juce::ignoreUnused(g);
}

void OuariconTremoloAudioProcessorEditor::resized()
{
    // WebView fills entire editor
    webView->setBounds(getLocalBounds());
}

// Pattern #8: EXPLICIT URL MAPPING (never use generic loops)
std::optional<juce::WebBrowserComponent::Resource>
OuariconTremoloAudioProcessorEditor::getResource(const juce::String& url)
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

    // Background image
    if (url == "/img/paper.jpg") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::paper_jpg, BinaryData::paper_jpgSize),
            juce::String("image/jpeg")
        };
    }

    // Botanical overlay
    if (url == "/img/carrot.png") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::carrot_png, BinaryData::carrot_pngSize),
            juce::String("image/png")
        };
    }

    // Resource not found
    juce::Logger::writeToLog("Resource not found: " + url);
    return std::nullopt;
}
