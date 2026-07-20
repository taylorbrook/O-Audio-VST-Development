#include "PluginEditor.h"
#include "BinaryData.h"

Drum808AudioProcessorEditor::Drum808AudioProcessorEditor(Drum808AudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    // 1. Create all 24 relays FIRST (parameter IDs must match APVTS and HTML exactly)
    kickLevelRelay = std::make_unique<juce::WebSliderRelay>("kick_level");
    kickToneRelay = std::make_unique<juce::WebSliderRelay>("kick_tone");
    kickDecayRelay = std::make_unique<juce::WebSliderRelay>("kick_decay");
    kickTuningRelay = std::make_unique<juce::WebSliderRelay>("kick_tuning");

    lowtomLevelRelay = std::make_unique<juce::WebSliderRelay>("lowtom_level");
    lowtomToneRelay = std::make_unique<juce::WebSliderRelay>("lowtom_tone");
    lowtomDecayRelay = std::make_unique<juce::WebSliderRelay>("lowtom_decay");
    lowtomTuningRelay = std::make_unique<juce::WebSliderRelay>("lowtom_tuning");

    midtomLevelRelay = std::make_unique<juce::WebSliderRelay>("midtom_level");
    midtomToneRelay = std::make_unique<juce::WebSliderRelay>("midtom_tone");
    midtomDecayRelay = std::make_unique<juce::WebSliderRelay>("midtom_decay");
    midtomTuningRelay = std::make_unique<juce::WebSliderRelay>("midtom_tuning");

    clapLevelRelay = std::make_unique<juce::WebSliderRelay>("clap_level");
    clapToneRelay = std::make_unique<juce::WebSliderRelay>("clap_tone");
    clapSnapRelay = std::make_unique<juce::WebSliderRelay>("clap_snap");
    clapTuningRelay = std::make_unique<juce::WebSliderRelay>("clap_tuning");

    closedhatLevelRelay = std::make_unique<juce::WebSliderRelay>("closedhat_level");
    closedhatToneRelay = std::make_unique<juce::WebSliderRelay>("closedhat_tone");
    closedhatDecayRelay = std::make_unique<juce::WebSliderRelay>("closedhat_decay");
    closedhatTuningRelay = std::make_unique<juce::WebSliderRelay>("closedhat_tuning");

    openhatLevelRelay = std::make_unique<juce::WebSliderRelay>("openhat_level");
    openhatToneRelay = std::make_unique<juce::WebSliderRelay>("openhat_tone");
    openhatDecayRelay = std::make_unique<juce::WebSliderRelay>("openhat_decay");
    openhatTuningRelay = std::make_unique<juce::WebSliderRelay>("openhat_tuning");

    // 2. Create WebView with ALL relay options (CRITICAL: register all 24 relays)
    webView = std::make_unique<juce::WebBrowserComponent>(
        juce::WebBrowserComponent::Options{}
            .withNativeIntegrationEnabled()
            .withResourceProvider([this](const juce::String& url) {
                return getResource(url);
            })
            .withOptionsFrom(*kickLevelRelay)
            .withOptionsFrom(*kickToneRelay)
            .withOptionsFrom(*kickDecayRelay)
            .withOptionsFrom(*kickTuningRelay)
            .withOptionsFrom(*lowtomLevelRelay)
            .withOptionsFrom(*lowtomToneRelay)
            .withOptionsFrom(*lowtomDecayRelay)
            .withOptionsFrom(*lowtomTuningRelay)
            .withOptionsFrom(*midtomLevelRelay)
            .withOptionsFrom(*midtomToneRelay)
            .withOptionsFrom(*midtomDecayRelay)
            .withOptionsFrom(*midtomTuningRelay)
            .withOptionsFrom(*clapLevelRelay)
            .withOptionsFrom(*clapToneRelay)
            .withOptionsFrom(*clapSnapRelay)
            .withOptionsFrom(*clapTuningRelay)
            .withOptionsFrom(*closedhatLevelRelay)
            .withOptionsFrom(*closedhatToneRelay)
            .withOptionsFrom(*closedhatDecayRelay)
            .withOptionsFrom(*closedhatTuningRelay)
            .withOptionsFrom(*openhatLevelRelay)
            .withOptionsFrom(*openhatToneRelay)
            .withOptionsFrom(*openhatDecayRelay)
            .withOptionsFrom(*openhatTuningRelay)
    );

    // 3. Create all 24 attachments LAST (CRITICAL: 3 parameters per Pattern 12)
    kickLevelAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("kick_level"), *kickLevelRelay, nullptr);
    kickToneAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("kick_tone"), *kickToneRelay, nullptr);
    kickDecayAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("kick_decay"), *kickDecayRelay, nullptr);
    kickTuningAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("kick_tuning"), *kickTuningRelay, nullptr);

    lowtomLevelAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("lowtom_level"), *lowtomLevelRelay, nullptr);
    lowtomToneAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("lowtom_tone"), *lowtomToneRelay, nullptr);
    lowtomDecayAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("lowtom_decay"), *lowtomDecayRelay, nullptr);
    lowtomTuningAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("lowtom_tuning"), *lowtomTuningRelay, nullptr);

    midtomLevelAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("midtom_level"), *midtomLevelRelay, nullptr);
    midtomToneAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("midtom_tone"), *midtomToneRelay, nullptr);
    midtomDecayAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("midtom_decay"), *midtomDecayRelay, nullptr);
    midtomTuningAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("midtom_tuning"), *midtomTuningRelay, nullptr);

    clapLevelAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("clap_level"), *clapLevelRelay, nullptr);
    clapToneAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("clap_tone"), *clapToneRelay, nullptr);
    clapSnapAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("clap_snap"), *clapSnapRelay, nullptr);
    clapTuningAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("clap_tuning"), *clapTuningRelay, nullptr);

    closedhatLevelAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("closedhat_level"), *closedhatLevelRelay, nullptr);
    closedhatToneAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("closedhat_tone"), *closedhatToneRelay, nullptr);
    closedhatDecayAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("closedhat_decay"), *closedhatDecayRelay, nullptr);
    closedhatTuningAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("closedhat_tuning"), *closedhatTuningRelay, nullptr);

    openhatLevelAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("openhat_level"), *openhatLevelRelay, nullptr);
    openhatToneAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("openhat_tone"), *openhatToneRelay, nullptr);
    openhatDecayAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("openhat_decay"), *openhatDecayRelay, nullptr);
    openhatTuningAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("openhat_tuning"), *openhatTuningRelay, nullptr);

    addAndMakeVisible(*webView);

    // Load UI
    webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());

    // Set window size (from mockup)
    setSize(1000, 550);

    // Start timer for LED updates (60fps)
    startTimer(16);
}

Drum808AudioProcessorEditor::~Drum808AudioProcessorEditor()
{
}

void Drum808AudioProcessorEditor::paint(juce::Graphics& g)
{
    // WebView handles all painting
    juce::ignoreUnused(g);
}

void Drum808AudioProcessorEditor::resized()
{
    // WebView fills entire editor
    webView->setBounds(getLocalBounds());
}

void Drum808AudioProcessorEditor::timerCallback()
{
    // Read atomic flags set by audio thread (Pattern 5: Threading)
    // Check each voice and trigger corresponding LED in JavaScript

    if (processorRef.kickTriggered.load(std::memory_order_relaxed))
    {
        processorRef.kickTriggered.store(false, std::memory_order_relaxed);
        webView->emitEventIfBrowserIsVisible("ledTrigger", "kick");
    }

    if (processorRef.lowTomTriggered.load(std::memory_order_relaxed))
    {
        processorRef.lowTomTriggered.store(false, std::memory_order_relaxed);
        webView->emitEventIfBrowserIsVisible("ledTrigger", "lowtom");
    }

    if (processorRef.midTomTriggered.load(std::memory_order_relaxed))
    {
        processorRef.midTomTriggered.store(false, std::memory_order_relaxed);
        webView->emitEventIfBrowserIsVisible("ledTrigger", "midtom");
    }

    if (processorRef.clapTriggered.load(std::memory_order_relaxed))
    {
        processorRef.clapTriggered.store(false, std::memory_order_relaxed);
        webView->emitEventIfBrowserIsVisible("ledTrigger", "clap");
    }

    if (processorRef.closedHatTriggered.load(std::memory_order_relaxed))
    {
        processorRef.closedHatTriggered.store(false, std::memory_order_relaxed);
        webView->emitEventIfBrowserIsVisible("ledTrigger", "closedhat");
    }

    if (processorRef.openHatTriggered.load(std::memory_order_relaxed))
    {
        processorRef.openHatTriggered.store(false, std::memory_order_relaxed);
        webView->emitEventIfBrowserIsVisible("ledTrigger", "openhat");
    }
}

std::optional<juce::WebBrowserComponent::Resource>
Drum808AudioProcessorEditor::getResource(const juce::String& url)
{
    auto makeVector = [](const char* data, int size) {
        return std::vector<std::byte>(
            reinterpret_cast<const std::byte*>(data),
            reinterpret_cast<const std::byte*>(data) + size
        );
    };

    // CRITICAL: Explicit URL mapping (Pattern 8 - NOT generic loop)
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

    // Resource not found
    juce::Logger::writeToLog("Resource not found: " + url);
    return std::nullopt;
}
