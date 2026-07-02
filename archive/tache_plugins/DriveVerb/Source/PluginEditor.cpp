#include "PluginEditor.h"
#include "BinaryData.h"

DriveVerbAudioProcessorEditor::DriveVerbAudioProcessorEditor(DriveVerbAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    // 1️⃣ Create relays FIRST (Pattern #11)
    sizeRelay = std::make_unique<juce::WebSliderRelay>("size");
    decayRelay = std::make_unique<juce::WebSliderRelay>("decay");
    dryWetRelay = std::make_unique<juce::WebSliderRelay>("dryWet");
    driveRelay = std::make_unique<juce::WebSliderRelay>("drive");
    filterRelay = std::make_unique<juce::WebSliderRelay>("filter");
    filterPositionRelay = std::make_unique<juce::WebToggleButtonRelay>("filterPosition");

    // 2️⃣ Create WebView with relays (Pattern #8 - explicit URL mapping)
    webView = std::make_unique<juce::WebBrowserComponent>(
        juce::WebBrowserComponent::Options{}
            .withNativeIntegrationEnabled()
            .withResourceProvider([this](auto& url) { return getResource(url); })
            .withOptionsFrom(*sizeRelay)
            .withOptionsFrom(*decayRelay)
            .withOptionsFrom(*dryWetRelay)
            .withOptionsFrom(*driveRelay)
            .withOptionsFrom(*filterRelay)
            .withOptionsFrom(*filterPositionRelay)
    );

    // 3️⃣ Create attachments LAST (Pattern #11, #12 - THREE parameters including nullptr)
    sizeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("size"), *sizeRelay, nullptr);
    decayAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("decay"), *decayRelay, nullptr);
    dryWetAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("dryWet"), *dryWetRelay, nullptr);
    driveAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("drive"), *driveRelay, nullptr);
    filterAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("filter"), *filterRelay, nullptr);
    filterPositionAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.parameters.getParameter("filterPosition"), *filterPositionRelay, nullptr);

    addAndMakeVisible(*webView);
    webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());

    // Start VU meter timer (30 FPS)
    startTimerHz(30);

    setSize(1000, 500);
}

DriveVerbAudioProcessorEditor::~DriveVerbAudioProcessorEditor()
{
}

void DriveVerbAudioProcessorEditor::paint(juce::Graphics& g)
{
    // WebView handles all painting
    juce::ignoreUnused(g);
}

void DriveVerbAudioProcessorEditor::resized()
{
    // WebView fills entire editor
    webView->setBounds(getLocalBounds());
}

void DriveVerbAudioProcessorEditor::timerCallback()
{
    // Get current drive output level from processor
    float driveLevelDB = processorRef.getDriveOutputLevel();

    // Send to WebView
    if (webView)
    {
        juce::String js = juce::String::formatted(
            "window.dispatchEvent(new CustomEvent('updateVUMeter', { detail: %f }));",
            driveLevelDB
        );
        webView->evaluateJavascript(js);
    }
}

std::optional<juce::WebBrowserComponent::Resource>
DriveVerbAudioProcessorEditor::getResource(const juce::String& url)
{
    auto makeVector = [](const char* data, int size) {
        return std::vector<std::byte>(
            reinterpret_cast<const std::byte*>(data),
            reinterpret_cast<const std::byte*>(data) + size
        );
    };

    // Explicit URL mapping (Pattern #8)
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
