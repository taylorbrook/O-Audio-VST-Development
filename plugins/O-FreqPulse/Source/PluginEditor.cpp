/*
  ==============================================================================

    O-FreqPulse - Editor Implementation
    Ouaricon Development
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginEditor.h"
#include "BinaryData.h"

OFreqPulseAudioProcessorEditor::OFreqPulseAudioProcessorEditor(OFreqPulseAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    // 1️⃣ CREATE RELAYS FIRST
    // Global parameter relays (5 total)
    mixRelay = std::make_unique<juce::WebSliderRelay>("mix");
    stepsRelay = std::make_unique<juce::WebSliderRelay>("steps");
    rateRelay = std::make_unique<juce::WebComboBoxRelay>("rate");
    swingRelay = std::make_unique<juce::WebSliderRelay>("swing");
    smoothingRelay = std::make_unique<juce::WebSliderRelay>("smoothing");

    // Crossover parameter relays (3 total)
    crossover1Relay = std::make_unique<juce::WebSliderRelay>("crossover_1");
    crossover2Relay = std::make_unique<juce::WebSliderRelay>("crossover_2");
    crossover3Relay = std::make_unique<juce::WebSliderRelay>("crossover_3");

    // Frequency boundary relays (2 total)
    freqLowRelay = std::make_unique<juce::WebSliderRelay>("freq_low");
    freqHighRelay = std::make_unique<juce::WebSliderRelay>("freq_high");

    // Per-band parameter relays (24 total: 6 params x 4 bands)
    band0EnableRelay = std::make_unique<juce::WebToggleButtonRelay>("band0_enable");
    band0DepthRelay = std::make_unique<juce::WebSliderRelay>("band0_depth");
    band0EucOnRelay = std::make_unique<juce::WebToggleButtonRelay>("band0_euc_on");
    band0EucStepsRelay = std::make_unique<juce::WebSliderRelay>("band0_euc_steps");
    band0EucPulsesRelay = std::make_unique<juce::WebSliderRelay>("band0_euc_pulses");
    band0EucOffsetRelay = std::make_unique<juce::WebSliderRelay>("band0_euc_offset");

    band1EnableRelay = std::make_unique<juce::WebToggleButtonRelay>("band1_enable");
    band1DepthRelay = std::make_unique<juce::WebSliderRelay>("band1_depth");
    band1EucOnRelay = std::make_unique<juce::WebToggleButtonRelay>("band1_euc_on");
    band1EucStepsRelay = std::make_unique<juce::WebSliderRelay>("band1_euc_steps");
    band1EucPulsesRelay = std::make_unique<juce::WebSliderRelay>("band1_euc_pulses");
    band1EucOffsetRelay = std::make_unique<juce::WebSliderRelay>("band1_euc_offset");

    band2EnableRelay = std::make_unique<juce::WebToggleButtonRelay>("band2_enable");
    band2DepthRelay = std::make_unique<juce::WebSliderRelay>("band2_depth");
    band2EucOnRelay = std::make_unique<juce::WebToggleButtonRelay>("band2_euc_on");
    band2EucStepsRelay = std::make_unique<juce::WebSliderRelay>("band2_euc_steps");
    band2EucPulsesRelay = std::make_unique<juce::WebSliderRelay>("band2_euc_pulses");
    band2EucOffsetRelay = std::make_unique<juce::WebSliderRelay>("band2_euc_offset");

    band3EnableRelay = std::make_unique<juce::WebToggleButtonRelay>("band3_enable");
    band3DepthRelay = std::make_unique<juce::WebSliderRelay>("band3_depth");
    band3EucOnRelay = std::make_unique<juce::WebToggleButtonRelay>("band3_euc_on");
    band3EucStepsRelay = std::make_unique<juce::WebSliderRelay>("band3_euc_steps");
    band3EucPulsesRelay = std::make_unique<juce::WebSliderRelay>("band3_euc_pulses");
    band3EucOffsetRelay = std::make_unique<juce::WebSliderRelay>("band3_euc_offset");

    // Step grid relays (128 total: 32 steps × 4 bands)
    for (int band = 0; band < 4; ++band)
    {
        for (int step = 0; step < 32; ++step)
        {
            int index = band * 32 + step;
            juce::String paramId = "step_b" + juce::String(band) + "_s" + juce::String(step);
            stepRelays[index] = std::make_unique<juce::WebToggleButtonRelay>(paramId);
        }
    }

    // 2️⃣ CREATE WEBVIEW WITH OPTIONS
    auto options = juce::WebBrowserComponent::Options{}
        .withNativeIntegrationEnabled()
        .withResourceProvider([this](const auto& url) { return getResource(url); })
        // Global parameter relays
        .withOptionsFrom(*mixRelay)
        .withOptionsFrom(*stepsRelay)
        .withOptionsFrom(*rateRelay)
        .withOptionsFrom(*swingRelay)
        .withOptionsFrom(*smoothingRelay)
        // Crossover relays
        .withOptionsFrom(*crossover1Relay)
        .withOptionsFrom(*crossover2Relay)
        .withOptionsFrom(*crossover3Relay)
        // Frequency boundary relays
        .withOptionsFrom(*freqLowRelay)
        .withOptionsFrom(*freqHighRelay)
        // Band 0 relays
        .withOptionsFrom(*band0EnableRelay)
        .withOptionsFrom(*band0DepthRelay)
        .withOptionsFrom(*band0EucOnRelay)
        .withOptionsFrom(*band0EucStepsRelay)
        .withOptionsFrom(*band0EucPulsesRelay)
        .withOptionsFrom(*band0EucOffsetRelay)
        // Band 1 relays
        .withOptionsFrom(*band1EnableRelay)
        .withOptionsFrom(*band1DepthRelay)
        .withOptionsFrom(*band1EucOnRelay)
        .withOptionsFrom(*band1EucStepsRelay)
        .withOptionsFrom(*band1EucPulsesRelay)
        .withOptionsFrom(*band1EucOffsetRelay)
        // Band 2 relays
        .withOptionsFrom(*band2EnableRelay)
        .withOptionsFrom(*band2DepthRelay)
        .withOptionsFrom(*band2EucOnRelay)
        .withOptionsFrom(*band2EucStepsRelay)
        .withOptionsFrom(*band2EucPulsesRelay)
        .withOptionsFrom(*band2EucOffsetRelay)
        // Band 3 relays
        .withOptionsFrom(*band3EnableRelay)
        .withOptionsFrom(*band3DepthRelay)
        .withOptionsFrom(*band3EucOnRelay)
        .withOptionsFrom(*band3EucStepsRelay)
        .withOptionsFrom(*band3EucPulsesRelay)
        .withOptionsFrom(*band3EucOffsetRelay)
        // v1.5.0: Tooltip state native functions
        .withNativeFunction("setTooltipsEnabled", [this](const juce::Array<juce::var>& args,
                                                          std::function<void(juce::var)> complete) {
            if (args.isEmpty()) { complete(juce::var(false)); return; }
            bool enabled = static_cast<bool>(args[0]);
            processorRef.setTooltipsEnabled(enabled);
            complete(juce::var(true));
        })
        // v1.6.0: Preset Manager native functions
        .withNativeFunction("savePreset", [this](const juce::Array<juce::var>& args,
                                                  std::function<void(juce::var)> complete) {
            if (args.size() > 0)
                complete(processorRef.presetManager.savePreset(args[0].toString()));
            else
                complete(false);
        })
        .withNativeFunction("loadPreset", [this](const juce::Array<juce::var>& args,
                                                  std::function<void(juce::var)> complete) {
            if (args.size() > 0)
                complete(processorRef.presetManager.loadPreset(args[0].toString()));
            else
                complete(false);
        })
        .withNativeFunction("getPresetList", [this](const juce::Array<juce::var>&,
                                                     std::function<void(juce::var)> complete) {
            auto list = processorRef.presetManager.getPresetList();
            juce::Array<juce::var> arr;
            for (const auto& name : list)
                arr.add(name);
            complete(juce::var(arr));
        })
        .withNativeFunction("getCurrentPreset", [this](const juce::Array<juce::var>&,
                                                        std::function<void(juce::var)> complete) {
            complete(processorRef.presetManager.getCurrentPresetName());
        })
        .withNativeFunction("selectNextPreset", [this](const juce::Array<juce::var>&,
                                                        std::function<void(juce::var)> complete) {
            auto next = processorRef.presetManager.getNextPreset();
            complete(next);
        })
        .withNativeFunction("selectPreviousPreset", [this](const juce::Array<juce::var>&,
                                                            std::function<void(juce::var)> complete) {
            auto prev = processorRef.presetManager.getPreviousPreset();
            complete(prev);
        })
        .withNativeFunction("deletePreset", [this](const juce::Array<juce::var>& args,
                                                    std::function<void(juce::var)> complete) {
            if (args.size() > 0)
                complete(processorRef.presetManager.deletePreset(args[0].toString()));
            else
                complete(false);
        })
        .withNativeFunction("isFactoryPreset", [this](const juce::Array<juce::var>& args,
                                                       std::function<void(juce::var)> complete) {
            if (args.size() > 0)
                complete(processorRef.presetManager.isFactoryPreset(args[0].toString()));
            else
                complete(false);
        })
        .withNativeFunction("savePresetWithDialog", [this](const juce::Array<juce::var>&,
                                                            std::function<void(juce::var)> complete) {
            fileChooser = std::make_unique<juce::FileChooser>(
                "Save Preset",
                processorRef.presetManager.getUserPresetsDirectory(),
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
                    bool success = processorRef.presetManager.savePreset(presetName);
                    auto* result = new juce::DynamicObject();
                    result->setProperty("success", success);
                    result->setProperty("name", success ? presetName : juce::String());
                    complete(juce::var(result));
                }
            );
        })
        .withNativeFunction("loadPresetFromFile", [this](const juce::Array<juce::var>&,
                                                          std::function<void(juce::var)> complete) {
            fileChooser = std::make_unique<juce::FileChooser>(
                "Load Preset",
                processorRef.presetManager.getUserPresetsDirectory(),
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
                    bool success = processorRef.presetManager.loadPresetFromFile(file);
                    auto* result = new juce::DynamicObject();
                    result->setProperty("success", success);
                    result->setProperty("name", success ? file.getFileNameWithoutExtension() : juce::String());
                    complete(juce::var(result));
                }
            );
        });

    // Register all 128 step grid relays
    for (int i = 0; i < 128; ++i)
    {
        options = options.withOptionsFrom(*stepRelays[i]);
    }

    webView = std::make_unique<juce::WebBrowserComponent>(options);

    // 3️⃣ CREATE ATTACHMENTS LAST
    auto& apvts = processorRef.getAPVTS();

    // Global parameter attachments
    mixAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("mix"), *mixRelay, nullptr);
    stepsAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("steps"), *stepsRelay, nullptr);
    rateAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *apvts.getParameter("rate"), *rateRelay, nullptr);
    swingAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("swing"), *swingRelay, nullptr);
    smoothingAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("smoothing"), *smoothingRelay, nullptr);

    // Crossover attachments
    crossover1Attachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("crossover_1"), *crossover1Relay, nullptr);
    crossover2Attachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("crossover_2"), *crossover2Relay, nullptr);
    crossover3Attachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("crossover_3"), *crossover3Relay, nullptr);

    // Frequency boundary attachments
    freqLowAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("freq_low"), *freqLowRelay, nullptr);
    freqHighAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("freq_high"), *freqHighRelay, nullptr);

    // Band 0 attachments
    band0EnableAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *apvts.getParameter("band0_enable"), *band0EnableRelay, nullptr);
    band0DepthAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("band0_depth"), *band0DepthRelay, nullptr);
    band0EucOnAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *apvts.getParameter("band0_euc_on"), *band0EucOnRelay, nullptr);
    band0EucStepsAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("band0_euc_steps"), *band0EucStepsRelay, nullptr);
    band0EucPulsesAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("band0_euc_pulses"), *band0EucPulsesRelay, nullptr);
    band0EucOffsetAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("band0_euc_offset"), *band0EucOffsetRelay, nullptr);

    // Band 1 attachments
    band1EnableAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *apvts.getParameter("band1_enable"), *band1EnableRelay, nullptr);
    band1DepthAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("band1_depth"), *band1DepthRelay, nullptr);
    band1EucOnAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *apvts.getParameter("band1_euc_on"), *band1EucOnRelay, nullptr);
    band1EucStepsAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("band1_euc_steps"), *band1EucStepsRelay, nullptr);
    band1EucPulsesAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("band1_euc_pulses"), *band1EucPulsesRelay, nullptr);
    band1EucOffsetAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("band1_euc_offset"), *band1EucOffsetRelay, nullptr);

    // Band 2 attachments
    band2EnableAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *apvts.getParameter("band2_enable"), *band2EnableRelay, nullptr);
    band2DepthAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("band2_depth"), *band2DepthRelay, nullptr);
    band2EucOnAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *apvts.getParameter("band2_euc_on"), *band2EucOnRelay, nullptr);
    band2EucStepsAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("band2_euc_steps"), *band2EucStepsRelay, nullptr);
    band2EucPulsesAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("band2_euc_pulses"), *band2EucPulsesRelay, nullptr);
    band2EucOffsetAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("band2_euc_offset"), *band2EucOffsetRelay, nullptr);

    // Band 3 attachments
    band3EnableAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *apvts.getParameter("band3_enable"), *band3EnableRelay, nullptr);
    band3DepthAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("band3_depth"), *band3DepthRelay, nullptr);
    band3EucOnAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *apvts.getParameter("band3_euc_on"), *band3EucOnRelay, nullptr);
    band3EucStepsAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("band3_euc_steps"), *band3EucStepsRelay, nullptr);
    band3EucPulsesAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("band3_euc_pulses"), *band3EucPulsesRelay, nullptr);
    band3EucOffsetAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("band3_euc_offset"), *band3EucOffsetRelay, nullptr);

    // Step grid attachments (128 total)
    for (int band = 0; band < 4; ++band)
    {
        for (int step = 0; step < 32; ++step)
        {
            int index = band * 32 + step;
            juce::String paramId = "step_b" + juce::String(band) + "_s" + juce::String(step);
            stepAttachments[index] = std::make_unique<juce::WebToggleButtonParameterAttachment>(
                *apvts.getParameter(paramId), *stepRelays[index], nullptr);
        }
    }

    // Add WebView to editor
    addAndMakeVisible(*webView);

    // Navigate to UI
    webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());

    // Set editor size (850×550 from plan)
    setSize(850, 550);

    // Start playhead update timer (30 Hz for smooth animation)
    startTimerHz(30);

    // v1.5.0: Tooltip state sync flag
    tooltipStateSynced = false;
}

OFreqPulseAudioProcessorEditor::~OFreqPulseAudioProcessorEditor()
{
    // Stop timer before destruction
    stopTimer();

    // Attachments destroyed first (safe - they stop using relays/webView)
    // Then webView destroyed
    // Then relays destroyed last
}

void OFreqPulseAudioProcessorEditor::timerCallback()
{
    // Read current step and signal state from processor (atomic, thread-safe)
    int step = processorRef.getCurrentStep();
    bool hasSignal = processorRef.getHasAudioSignal();

    // Send to WebView via JavaScript
    juce::String js = juce::String::formatted(
        "if (window.updatePlayhead) window.updatePlayhead(%d, %s);",
        step, hasSignal ? "true" : "false"
    );
    webView->evaluateJavascript(js);

    // v1.5.0: Sync tooltip state from processor to WebView (once, after page loads)
    if (!tooltipStateSynced)
    {
        tooltipStateSynced = true;
        bool enabled = processorRef.getTooltipsEnabled();
        juce::String tooltipJs = "if (typeof window.restoreTooltipState === 'function') window.restoreTooltipState("
            + juce::String(enabled ? "true" : "false") + ");";
        webView->evaluateJavascript(tooltipJs);
    }
}

void OFreqPulseAudioProcessorEditor::paint(juce::Graphics& g)
{
    // WebView handles all painting
    juce::ignoreUnused(g);
}

void OFreqPulseAudioProcessorEditor::resized()
{
    // WebView fills entire editor
    if (webView)
        webView->setBounds(getLocalBounds());
}

std::optional<juce::WebBrowserComponent::Resource>
OFreqPulseAudioProcessorEditor::getResource(const juce::String& url)
{
    auto makeVector = [](const char* data, int size) {
        return std::vector<std::byte>(
            reinterpret_cast<const std::byte*>(data),
            reinterpret_cast<const std::byte*>(data) + size
        );
    };

    // Explicit URL mapping (Pattern #8 - no generic loops)
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

    if (url == "/js/app.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::app_js, BinaryData::app_jsSize),
            juce::String("text/javascript")
        };
    }

    if (url == "/css/styles.css") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::styles_css, BinaryData::styles_cssSize),
            juce::String("text/css")
        };
    }

    // v1.6.0: Preset Manager JS module
    if (url == "/modules/preset-manager.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::presetmanager_js, BinaryData::presetmanager_jsSize),
            juce::String("text/javascript")
        };
    }

    // Resource not found
    juce::Logger::writeToLog("O-FreqPulse: Resource not found: " + url);
    return std::nullopt;
}
