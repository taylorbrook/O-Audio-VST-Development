/*
  ==============================================================================

    OuariconSimpleReverb - Editor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginEditor.h"
#include "BinaryData.h"

OuariconSimpleReverbAudioProcessorEditor::OuariconSimpleReverbAudioProcessorEditor(OuariconSimpleReverbAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    // 1. Create relays FIRST (with parameter IDs matching HTML and APVTS)
    typeRelay = std::make_unique<juce::WebComboBoxRelay>("TYPE");
    characterRelay = std::make_unique<juce::WebSliderRelay>("CHARACTER");
    wetRelay = std::make_unique<juce::WebSliderRelay>("WET");
    dryRelay = std::make_unique<juce::WebSliderRelay>("DRY");
    decayRelay = std::make_unique<juce::WebSliderRelay>("DECAY");
    sizeRelay = std::make_unique<juce::WebSliderRelay>("SIZE");

    // 2. Create WebView SECOND with all relay options registered
    webView = std::make_unique<juce::WebBrowserComponent>(
        juce::WebBrowserComponent::Options{}
            .withNativeIntegrationEnabled()
            .withResourceProvider([this](const auto& url) { return getResource(url); })
            .withOptionsFrom(*typeRelay)
            .withOptionsFrom(*characterRelay)
            .withOptionsFrom(*wetRelay)
            .withOptionsFrom(*dryRelay)
            .withOptionsFrom(*decayRelay)
            .withOptionsFrom(*sizeRelay)
    );

    // 3. Create attachments LAST (connect parameters to relays)
    typeAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *processorRef.parameters.getParameter("TYPE"), *typeRelay, nullptr);

    characterAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("CHARACTER"), *characterRelay, nullptr);

    wetAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("WET"), *wetRelay, nullptr);

    dryAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("DRY"), *dryRelay, nullptr);

    decayAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("DECAY"), *decayRelay, nullptr);

    sizeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("SIZE"), *sizeRelay, nullptr);

    // Add WebView to editor and navigate to UI
    addAndMakeVisible(*webView);
    webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());

    // Set editor size (from creative brief: 500x350)
    setSize(500, 350);

    // Start timer for VU meter updates (30 Hz)
    startTimerHz(30);
}

OuariconSimpleReverbAudioProcessorEditor::~OuariconSimpleReverbAudioProcessorEditor()
{
    stopTimer();
}

//==============================================================================
void OuariconSimpleReverbAudioProcessorEditor::timerCallback()
{
    // VU Meter - emit output level to WebView
    const float outputDB = processorRef.outputLevelDB.load(std::memory_order_relaxed);
    webView->emitEventIfBrowserIsVisible("outputLevel", outputDB);
}

void OuariconSimpleReverbAudioProcessorEditor::paint(juce::Graphics& g)
{
    // WebView handles all painting
    juce::ignoreUnused(g);
}

void OuariconSimpleReverbAudioProcessorEditor::resized()
{
    // WebView fills entire editor
    if (webView)
        webView->setBounds(getLocalBounds());
}

std::optional<juce::WebBrowserComponent::Resource>
OuariconSimpleReverbAudioProcessorEditor::getResource(const juce::String& url)
{
    // Helper to convert BinaryData to byte vector
    auto makeVector = [](const char* data, int size) {
        return std::vector<std::byte>(
            reinterpret_cast<const std::byte*>(data),
            reinterpret_cast<const std::byte*>(data) + size
        );
    };

    // Route URLs to embedded resources
    if (url == "/" || url == "/index.html")
    {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::index_html, BinaryData::index_htmlSize),
            juce::String("text/html")
        };
    }

    if (url == "/js/juce/index.js")
    {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::index_js, BinaryData::index_jsSize),
            juce::String("text/javascript")
        };
    }

    if (url == "/js/juce/check_native_interop.js")
    {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::check_native_interop_js, BinaryData::check_native_interop_jsSize),
            juce::String("text/javascript")
        };
    }

    if (url == "/img/paper.jpg")
    {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::paper_jpg, BinaryData::paper_jpgSize),
            juce::String("image/jpeg")
        };
    }

    if (url == "/img/flora.png")
    {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::flora_png, BinaryData::flora_pngSize),
            juce::String("image/png")
        };
    }

    // Resource not found
    juce::Logger::writeToLog("Resource not found: " + url);
    return std::nullopt;
}
