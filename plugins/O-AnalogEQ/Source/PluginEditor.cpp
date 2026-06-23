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
    analogRelay = std::make_unique<juce::WebToggleButtonRelay>("analog");

    // 2️⃣ Create WebView with relay options (depends on relays)
    webView = std::make_unique<juce::WebBrowserComponent>(
        juce::WebBrowserComponent::Options{}
            .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
            .withWinWebView2Options(
                juce::WebBrowserComponent::Options::WinWebView2{}
                    .withUserDataFolder(juce::File::getSpecialLocation(
                        juce::File::SpecialLocationType::tempDirectory)))
            .withNativeIntegrationEnabled()
#if JUCE_WEB_BROWSER_RESOURCE_PROVIDER_AVAILABLE
            .withResourceProvider([this](const auto& url) { return getResource(url); })
#endif
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
            .withOptionsFrom(*analogRelay)
            // Preset manager native functions
            .withNativeFunction("savePreset", [this](const auto& args, auto complete) {
                if (args.size() > 0)
                    complete(audioProcessor.presetManager.savePreset(args[0].toString()));
                else
                    complete(false);
            })
            .withNativeFunction("savePresetWithDialog", [this](auto&, auto complete) {
                fileChooser = std::make_unique<juce::FileChooser>(
                    "Save Preset",
                    audioProcessor.presetManager.getUserPresetsDirectory(),
                    "*.json"
                );
                fileChooser->launchAsync(
                    juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                    [this, complete](const juce::FileChooser& fc) {
                        auto results = fc.getResults();
                        if (results.isEmpty()) {
                            auto* result = new juce::DynamicObject();
                            result->setProperty("success", false);
                            result->setProperty("name", "");
                            complete(juce::var(result));
                            return;
                        }
                        auto file = results.getFirst();
                        auto presetName = file.getFileNameWithoutExtension();
                        bool success = audioProcessor.presetManager.savePreset(presetName);
                        auto* result = new juce::DynamicObject();
                        result->setProperty("success", success);
                        result->setProperty("name", success ? presetName : juce::String());
                        complete(juce::var(result));
                    }
                );
            })
            .withNativeFunction("loadPreset", [this](const auto& args, auto complete) {
                if (args.size() > 0)
                    complete(audioProcessor.presetManager.loadPreset(args[0].toString()));
                else
                    complete(false);
            })
            .withNativeFunction("loadPresetFromFile", [this](auto&, auto complete) {
                fileChooser = std::make_unique<juce::FileChooser>(
                    "Load Preset",
                    audioProcessor.presetManager.getPresetsDirectory(),
                    "*.json"
                );
                fileChooser->launchAsync(
                    juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                    [this, complete](const juce::FileChooser& fc) {
                        auto results = fc.getResults();
                        if (results.isEmpty()) {
                            auto* result = new juce::DynamicObject();
                            result->setProperty("success", false);
                            result->setProperty("name", "");
                            complete(juce::var(result));
                            return;
                        }
                        auto file = results.getFirst();
                        bool success = audioProcessor.presetManager.loadPresetFromFile(file);
                        auto* result = new juce::DynamicObject();
                        result->setProperty("success", success);
                        result->setProperty("name", success ? file.getFileNameWithoutExtension() : juce::String());
                        complete(juce::var(result));
                    }
                );
            })
            .withNativeFunction("getPresetList", [this](auto&, auto complete) {
                auto list = audioProcessor.presetManager.getPresetList();
                juce::Array<juce::var> arr;
                for (const auto& name : list)
                    arr.add(name);
                complete(juce::var(arr));
            })
            .withNativeFunction("getCurrentPreset", [this](auto&, auto complete) {
                complete(audioProcessor.presetManager.getCurrentPresetName());
            })
            .withNativeFunction("selectNextPreset", [this](auto&, auto complete) {
                complete(audioProcessor.presetManager.getNextPreset());
            })
            .withNativeFunction("selectPreviousPreset", [this](auto&, auto complete) {
                complete(audioProcessor.presetManager.getPreviousPreset());
            })
            .withNativeFunction("deletePreset", [this](const auto& args, auto complete) {
                if (args.size() > 0)
                    complete(audioProcessor.presetManager.deletePreset(args[0].toString()));
                else
                    complete(false);
            })
            .withNativeFunction("isFactoryPreset", [this](const auto& args, auto complete) {
                if (args.size() > 0)
                    complete(audioProcessor.presetManager.isFactoryPreset(args[0].toString()));
                else
                    complete(false);
            })
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
    analogAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *audioProcessor.parameters.getParameter("analog"), *analogRelay, nullptr);

    // Load UI from resource provider
#if JUCE_WEB_BROWSER_RESOURCE_PROVIDER_AVAILABLE
    webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
#endif

    // Start timer for VU meter updates (30 Hz)
    startTimerHz(30);

    // Set window size from mockup v3 (920×220 compact rack-unit)
    setSize(920, 220);
}

OuariconAnalogEQAudioProcessorEditor::~OuariconAnalogEQAudioProcessorEditor()
{
    // Stop timer before destruction
    stopTimer();

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

    if (url == "/js/juce/check_native_interop.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::check_native_interop_js, BinaryData::check_native_interop_jsSize),
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

    if (url == "/modules/preset-manager.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::presetmanager_js, BinaryData::presetmanager_jsSize),
            juce::String("application/javascript")
        };
    }

    // 404 - Resource not found
    juce::Logger::writeToLog("Resource not found: " + url);
    return std::nullopt;
}

//==============================================================================
void OuariconAnalogEQAudioProcessorEditor::timerCallback()
{
    // VU Meter - emit output level to WebView
    const float outputDB = audioProcessor.outputLevelDB.load(std::memory_order_relaxed);
    webView->emitEventIfBrowserIsVisible("outputLevel", outputDB);
}
