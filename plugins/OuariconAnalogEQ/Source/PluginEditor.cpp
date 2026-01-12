/*
  ==============================================================================

    Ouaricon Analog EQ - Editor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

    Stage 3 (GUI) - WebView UI Implementation

  ==============================================================================
*/

#include "PluginEditor.h"
#include "BinaryData.h"

//==============================================================================
OuariconAnalogEQAudioProcessorEditor::OuariconAnalogEQAudioProcessorEditor(OuariconAnalogEQAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    //==========================================================================
    // CRITICAL: Initialization Order
    // 1. Create relays FIRST
    // 2. Create WebView with relay options
    // 3. Create attachments LAST
    //==========================================================================

    // 1️⃣ Create relays FIRST (no dependencies)
    // LF Band
    lfFreqRelay = std::make_unique<juce::WebSliderRelay>("lf_freq");
    lfGainRelay = std::make_unique<juce::WebSliderRelay>("lf_gain");
    lfOnRelay = std::make_unique<juce::WebToggleButtonRelay>("lf_on");

    // LMF Band
    lmfFreqRelay = std::make_unique<juce::WebSliderRelay>("lmf_freq");
    lmfGainRelay = std::make_unique<juce::WebSliderRelay>("lmf_gain");
    lmfQRelay = std::make_unique<juce::WebComboBoxRelay>("lmf_q");
    lmfOnRelay = std::make_unique<juce::WebToggleButtonRelay>("lmf_on");

    // HMF Band
    hmfFreqRelay = std::make_unique<juce::WebSliderRelay>("hmf_freq");
    hmfGainRelay = std::make_unique<juce::WebSliderRelay>("hmf_gain");
    hmfQRelay = std::make_unique<juce::WebComboBoxRelay>("hmf_q");
    hmfOnRelay = std::make_unique<juce::WebToggleButtonRelay>("hmf_on");

    // HF Band
    hfFreqRelay = std::make_unique<juce::WebSliderRelay>("hf_freq");
    hfGainRelay = std::make_unique<juce::WebSliderRelay>("hf_gain");
    hfOnRelay = std::make_unique<juce::WebToggleButtonRelay>("hf_on");

    // Global
    outputGainRelay = std::make_unique<juce::WebSliderRelay>("output_gain");
    analogRelay = std::make_unique<juce::WebToggleButtonRelay>("analog");

    // 2️⃣ Create WebView with relay options (depends on relays)
    webView = std::make_unique<juce::WebBrowserComponent>(
        juce::WebBrowserComponent::Options{}
            .withNativeIntegrationEnabled()
            .withResourceProvider([this](const auto& url) { return getResource(url); })
            .withOptionsFrom(*lfFreqRelay)
            .withOptionsFrom(*lfGainRelay)
            .withOptionsFrom(*lfOnRelay)
            .withOptionsFrom(*lmfFreqRelay)
            .withOptionsFrom(*lmfGainRelay)
            .withOptionsFrom(*lmfQRelay)
            .withOptionsFrom(*lmfOnRelay)
            .withOptionsFrom(*hmfFreqRelay)
            .withOptionsFrom(*hmfGainRelay)
            .withOptionsFrom(*hmfQRelay)
            .withOptionsFrom(*hmfOnRelay)
            .withOptionsFrom(*hfFreqRelay)
            .withOptionsFrom(*hfGainRelay)
            .withOptionsFrom(*hfOnRelay)
            .withOptionsFrom(*outputGainRelay)
            .withOptionsFrom(*analogRelay)
    );

    addAndMakeVisible(*webView);

    // 3️⃣ Create attachments LAST (depend on relays and webView)
    // LF Band
    lfFreqAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.parameters.getParameter("lf_freq"), *lfFreqRelay, nullptr);
    lfGainAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.parameters.getParameter("lf_gain"), *lfGainRelay, nullptr);
    lfOnAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *audioProcessor.parameters.getParameter("lf_on"), *lfOnRelay, nullptr);

    // LMF Band
    lmfFreqAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.parameters.getParameter("lmf_freq"), *lmfFreqRelay, nullptr);
    lmfGainAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.parameters.getParameter("lmf_gain"), *lmfGainRelay, nullptr);
    lmfQAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *audioProcessor.parameters.getParameter("lmf_q"), *lmfQRelay, nullptr);
    lmfOnAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *audioProcessor.parameters.getParameter("lmf_on"), *lmfOnRelay, nullptr);

    // HMF Band
    hmfFreqAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.parameters.getParameter("hmf_freq"), *hmfFreqRelay, nullptr);
    hmfGainAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.parameters.getParameter("hmf_gain"), *hmfGainRelay, nullptr);
    hmfQAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *audioProcessor.parameters.getParameter("hmf_q"), *hmfQRelay, nullptr);
    hmfOnAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *audioProcessor.parameters.getParameter("hmf_on"), *hmfOnRelay, nullptr);

    // HF Band
    hfFreqAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.parameters.getParameter("hf_freq"), *hfFreqRelay, nullptr);
    hfGainAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.parameters.getParameter("hf_gain"), *hfGainRelay, nullptr);
    hfOnAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *audioProcessor.parameters.getParameter("hf_on"), *hfOnRelay, nullptr);

    // Global
    outputGainAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.parameters.getParameter("output_gain"), *outputGainRelay, nullptr);
    analogAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *audioProcessor.parameters.getParameter("analog"), *analogRelay, nullptr);

    // Load UI from resource provider
    webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());

    // Set window size from mockup v3 (920×220 compact rack-unit)
    setSize(920, 220);
}

OuariconAnalogEQAudioProcessorEditor::~OuariconAnalogEQAudioProcessorEditor()
{
    // Members destroyed in REVERSE order of declaration:
    // 1. Attachments destroyed FIRST (can safely call webView methods)
    // 2. WebView destroyed SECOND (attachments are gone)
    // 3. Relays destroyed LAST (nothing using them)
}

//==============================================================================
void OuariconAnalogEQAudioProcessorEditor::paint(juce::Graphics& g)
{
    // WebView handles all rendering
    g.fillAll(juce::Colours::black);
}

void OuariconAnalogEQAudioProcessorEditor::resized()
{
    // WebView fills entire editor window
    webView->setBounds(getLocalBounds());
}

//==============================================================================
std::optional<juce::WebBrowserComponent::Resource>
OuariconAnalogEQAudioProcessorEditor::getResource(const juce::String& url)
{
    // Helper to convert BinaryData to byte vector
    auto makeVector = [](const char* data, int size) {
        return std::vector<std::byte>(
            reinterpret_cast<const std::byte*>(data),
            reinterpret_cast<const std::byte*>(data) + size
        );
    };

    //==========================================================================
    // CRITICAL: Explicit URL Mapping
    // BinaryData flattens paths (index.js → index_js)
    // HTML requests use original paths (./js/juce/index.js)
    // Must map manually with correct MIME types
    //==========================================================================

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

    if (url == "/images/paper1.jpg") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::paper1_jpg, BinaryData::paper1_jpgSize),
            juce::String("image/jpeg")
        };
    }

    if (url == "/images/flower_ferdinandibauer00baue_0021.png") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::flower_ferdinandibauer00baue_0021_png,
                      BinaryData::flower_ferdinandibauer00baue_0021_pngSize),
            juce::String("image/png")
        };
    }

    // 404 - Resource not found
    juce::Logger::writeToLog("Resource not found: " + url);
    return std::nullopt;
}
