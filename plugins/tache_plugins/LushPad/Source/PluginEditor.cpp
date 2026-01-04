#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "BinaryData.h"

LushPadAudioProcessorEditor::LushPadAudioProcessorEditor(LushPadAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    // 1. Create relays FIRST (Pattern #11)
    timbreRelay = std::make_unique<juce::WebSliderRelay>("timbre");
    filterCutoffRelay = std::make_unique<juce::WebSliderRelay>("filter_cutoff");
    reverbAmountRelay = std::make_unique<juce::WebSliderRelay>("reverb_amount");

    // 2. Create WebView with relay options
    webView = std::make_unique<juce::WebBrowserComponent>(
        juce::WebBrowserComponent::Options{}
            .withNativeIntegrationEnabled()
            .withResourceProvider([this](auto& url) { return getResource(url); })
            .withOptionsFrom(*timbreRelay)
            .withOptionsFrom(*filterCutoffRelay)
            .withOptionsFrom(*reverbAmountRelay)
    );

    // 3. Create attachments LAST (Pattern #12 - 3 parameters)
    timbreAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("timbre"), *timbreRelay, nullptr);

    filterCutoffAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("filter_cutoff"), *filterCutoffRelay, nullptr);

    reverbAmountAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("reverb_amount"), *reverbAmountRelay, nullptr);

    addAndMakeVisible(*webView);

    // Set window size (600×300 from mockup)
    setSize(600, 300);

    // Load HTML
    webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
}

LushPadAudioProcessorEditor::~LushPadAudioProcessorEditor()
{
}

void LushPadAudioProcessorEditor::paint(juce::Graphics& g)
{
    // WebView handles all rendering
    juce::ignoreUnused(g);
}

void LushPadAudioProcessorEditor::resized()
{
    webView->setBounds(getLocalBounds());
}

std::optional<juce::WebBrowserComponent::Resource>
LushPadAudioProcessorEditor::getResource(const juce::String& url)
{
    // Pattern #8: Explicit URL mapping (REQUIRED)
    auto makeVector = [](const char* data, int size) {
        return std::vector<std::byte>(
            reinterpret_cast<const std::byte*>(data),
            reinterpret_cast<const std::byte*>(data) + size
        );
    };

    // Map URLs to BinaryData resources
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
            makeVector(BinaryData::check_native_interop_js,
                      BinaryData::check_native_interop_jsSize),
            juce::String("text/javascript")
        };
    }

    return std::nullopt;  // 404 for unknown URLs
}
