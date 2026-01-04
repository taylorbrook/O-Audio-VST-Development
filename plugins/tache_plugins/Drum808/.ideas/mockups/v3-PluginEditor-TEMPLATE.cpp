#include "PluginEditor.h"

//==============================================================================
// Constructor
//==============================================================================

Drum808AudioProcessorEditor::Drum808AudioProcessorEditor(Drum808AudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    // ========================================================================
    // STEP 1: CREATE RELAYS (27 total: 24 parameters + 3 preset buttons)
    // ========================================================================

    // Kick drum (4 relays)
    kickLevelRelay = std::make_unique<juce::WebSliderRelay>("kick_level");
    kickToneRelay = std::make_unique<juce::WebSliderRelay>("kick_tone");
    kickDecayRelay = std::make_unique<juce::WebSliderRelay>("kick_decay");
    kickTuningRelay = std::make_unique<juce::WebSliderRelay>("kick_tuning");

    // Low tom (4 relays)
    lowtomLevelRelay = std::make_unique<juce::WebSliderRelay>("lowtom_level");
    lowtomToneRelay = std::make_unique<juce::WebSliderRelay>("lowtom_tone");
    lowtomDecayRelay = std::make_unique<juce::WebSliderRelay>("lowtom_decay");
    lowtomTuningRelay = std::make_unique<juce::WebSliderRelay>("lowtom_tuning");

    // Mid tom (4 relays)
    midtomLevelRelay = std::make_unique<juce::WebSliderRelay>("midtom_level");
    midtomToneRelay = std::make_unique<juce::WebSliderRelay>("midtom_tone");
    midtomDecayRelay = std::make_unique<juce::WebSliderRelay>("midtom_decay");
    midtomTuningRelay = std::make_unique<juce::WebSliderRelay>("midtom_tuning");

    // Clap (4 relays)
    clapLevelRelay = std::make_unique<juce::WebSliderRelay>("clap_level");
    clapToneRelay = std::make_unique<juce::WebSliderRelay>("clap_tone");
    clapSnapRelay = std::make_unique<juce::WebSliderRelay>("clap_snap");
    clapTuningRelay = std::make_unique<juce::WebSliderRelay>("clap_tuning");

    // Closed hat (4 relays)
    closedhatLevelRelay = std::make_unique<juce::WebSliderRelay>("closedhat_level");
    closedhatToneRelay = std::make_unique<juce::WebSliderRelay>("closedhat_tone");
    closedhatDecayRelay = std::make_unique<juce::WebSliderRelay>("closedhat_decay");
    closedhatTuningRelay = std::make_unique<juce::WebSliderRelay>("closedhat_tuning");

    // Open hat (4 relays)
    openhatLevelRelay = std::make_unique<juce::WebSliderRelay>("openhat_level");
    openhatToneRelay = std::make_unique<juce::WebSliderRelay>("openhat_tone");
    openhatDecayRelay = std::make_unique<juce::WebSliderRelay>("openhat_decay");
    openhatTuningRelay = std::make_unique<juce::WebSliderRelay>("openhat_tuning");

    // Preset browser (3 toggle relays)
    presetPreviousRelay = std::make_unique<juce::WebToggleButtonRelay>("preset_previous");
    presetNextRelay = std::make_unique<juce::WebToggleButtonRelay>("preset_next");
    presetSaveRelay = std::make_unique<juce::WebToggleButtonRelay>("preset_save");

    // ========================================================================
    // STEP 2: CREATE WEBVIEW (with all relay options)
    // ========================================================================

    webView = std::make_unique<juce::WebBrowserComponent>(
        juce::WebBrowserComponent::Options{}
            .withNativeIntegrationEnabled()
            .withResourceProvider([this](const auto& url) { return getResource(url); })
            .withKeepPageLoadedWhenBrowserIsHidden()

            // Kick drum (4 options)
            .withOptionsFrom(*kickLevelRelay)
            .withOptionsFrom(*kickToneRelay)
            .withOptionsFrom(*kickDecayRelay)
            .withOptionsFrom(*kickTuningRelay)

            // Low tom (4 options)
            .withOptionsFrom(*lowtomLevelRelay)
            .withOptionsFrom(*lowtomToneRelay)
            .withOptionsFrom(*lowtomDecayRelay)
            .withOptionsFrom(*lowtomTuningRelay)

            // Mid tom (4 options)
            .withOptionsFrom(*midtomLevelRelay)
            .withOptionsFrom(*midtomToneRelay)
            .withOptionsFrom(*midtomDecayRelay)
            .withOptionsFrom(*midtomTuningRelay)

            // Clap (4 options)
            .withOptionsFrom(*clapLevelRelay)
            .withOptionsFrom(*clapToneRelay)
            .withOptionsFrom(*clapSnapRelay)
            .withOptionsFrom(*clapTuningRelay)

            // Closed hat (4 options)
            .withOptionsFrom(*closedhatLevelRelay)
            .withOptionsFrom(*closedhatToneRelay)
            .withOptionsFrom(*closedhatDecayRelay)
            .withOptionsFrom(*closedhatTuningRelay)

            // Open hat (4 options)
            .withOptionsFrom(*openhatLevelRelay)
            .withOptionsFrom(*openhatToneRelay)
            .withOptionsFrom(*openhatDecayRelay)
            .withOptionsFrom(*openhatTuningRelay)

            // Preset browser (3 options)
            .withOptionsFrom(*presetPreviousRelay)
            .withOptionsFrom(*presetNextRelay)
            .withOptionsFrom(*presetSaveRelay)
    );

    // ========================================================================
    // STEP 3: CREATE PARAMETER ATTACHMENTS (27 total)
    // ========================================================================

    // Kick drum (4 attachments)
    kickLevelAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("kick_level"), *kickLevelRelay, nullptr);
    kickToneAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("kick_tone"), *kickToneRelay, nullptr);
    kickDecayAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("kick_decay"), *kickDecayRelay, nullptr);
    kickTuningAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("kick_tuning"), *kickTuningRelay, nullptr);

    // Low tom (4 attachments)
    lowtomLevelAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("lowtom_level"), *lowtomLevelRelay, nullptr);
    lowtomToneAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("lowtom_tone"), *lowtomToneRelay, nullptr);
    lowtomDecayAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("lowtom_decay"), *lowtomDecayRelay, nullptr);
    lowtomTuningAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("lowtom_tuning"), *lowtomTuningRelay, nullptr);

    // Mid tom (4 attachments)
    midtomLevelAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("midtom_level"), *midtomLevelRelay, nullptr);
    midtomToneAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("midtom_tone"), *midtomToneRelay, nullptr);
    midtomDecayAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("midtom_decay"), *midtomDecayRelay, nullptr);
    midtomTuningAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("midtom_tuning"), *midtomTuningRelay, nullptr);

    // Clap (4 attachments)
    clapLevelAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("clap_level"), *clapLevelRelay, nullptr);
    clapToneAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("clap_tone"), *clapToneRelay, nullptr);
    clapSnapAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("clap_snap"), *clapSnapRelay, nullptr);
    clapTuningAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("clap_tuning"), *clapTuningRelay, nullptr);

    // Closed hat (4 attachments)
    closedhatLevelAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("closedhat_level"), *closedhatLevelRelay, nullptr);
    closedhatToneAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("closedhat_tone"), *closedhatToneRelay, nullptr);
    closedhatDecayAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("closedhat_decay"), *closedhatDecayRelay, nullptr);
    closedhatTuningAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("closedhat_tuning"), *closedhatTuningRelay, nullptr);

    // Open hat (4 attachments)
    openhatLevelAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("openhat_level"), *openhatLevelRelay, nullptr);
    openhatToneAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("openhat_tone"), *openhatToneRelay, nullptr);
    openhatDecayAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("openhat_decay"), *openhatDecayRelay, nullptr);
    openhatTuningAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("openhat_tuning"), *openhatTuningRelay, nullptr);

    // Preset browser (3 toggle attachments)
    presetPreviousAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *audioProcessor.apvts.getParameter("preset_previous"), *presetPreviousRelay, nullptr);
    presetNextAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *audioProcessor.apvts.getParameter("preset_next"), *presetNextRelay, nullptr);
    presetSaveAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *audioProcessor.apvts.getParameter("preset_save"), *presetSaveRelay, nullptr);

    // ========================================================================
    // WEBVIEW SETUP
    // ========================================================================

    // Navigate to root (loads index.html)
    webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());

    // Make WebView visible
    addAndMakeVisible(*webView);

    // ========================================================================
    // WINDOW SIZE (from YAML: 1000x550)
    // ========================================================================

    setSize(1000, 550);
}

//==============================================================================
// Destructor
//==============================================================================

Drum808AudioProcessorEditor::~Drum808AudioProcessorEditor()
{
    // Members destroyed automatically in reverse order:
    // 1. Attachments (27) destroyed first
    // 2. WebView destroyed second
    // 3. Relays (27) destroyed last
}

//==============================================================================
// AudioProcessorEditor Overrides
//==============================================================================

void Drum808AudioProcessorEditor::paint(juce::Graphics& g)
{
    // WebView fills entire editor - no custom painting needed
}

void Drum808AudioProcessorEditor::resized()
{
    // Make WebView fill the entire editor bounds
    webView->setBounds(getLocalBounds());
}

//==============================================================================
// Resource Provider
//==============================================================================

std::optional<juce::WebBrowserComponent::Resource>
Drum808AudioProcessorEditor::getResource(const juce::String& url)
{
    // Helper lambda to convert binary data to vector
    auto makeVector = [](const char* data, int size) {
        return std::vector<std::byte>(
            reinterpret_cast<const std::byte*>(data),
            reinterpret_cast<const std::byte*>(data) + size
        );
    };

    // Root URL - serve index.html
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
            juce::String("application/javascript")
        };
    }

    // JUCE native interop check (REQUIRED for WebView)
    if (url == "/js/juce/check_native_interop.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::check_native_interop_js,
                      BinaryData::check_native_interop_jsSize),
            juce::String("application/javascript")
        };
    }

    // 404 - Resource not found
    return std::nullopt;
}
