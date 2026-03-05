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
    attackRelay = std::make_unique<juce::WebSliderRelay>("attack");
    releaseRelay = std::make_unique<juce::WebSliderRelay>("release");

    // Crossover parameter relays (3 total)
    crossover1Relay = std::make_unique<juce::WebSliderRelay>("crossover_1");
    crossover2Relay = std::make_unique<juce::WebSliderRelay>("crossover_2");
    crossover3Relay = std::make_unique<juce::WebSliderRelay>("crossover_3");

    // Frequency boundary relays (2 total)
    freqLowRelay = std::make_unique<juce::WebSliderRelay>("freq_low");
    freqHighRelay = std::make_unique<juce::WebSliderRelay>("freq_high");

    // Per-band parameter relays (24 total: 6 params x 4 bands)
    for (int i = 0; i < 4; ++i)
    {
        auto prefix = "band" + juce::String(i) + "_";
        bandRelays[i].enable    = std::make_unique<juce::WebToggleButtonRelay>(prefix + "enable");
        bandRelays[i].depth     = std::make_unique<juce::WebSliderRelay>(prefix + "depth");
        bandRelays[i].rate      = std::make_unique<juce::WebComboBoxRelay>(prefix + "rate");
        bandRelays[i].eucOn     = std::make_unique<juce::WebToggleButtonRelay>(prefix + "euc_on");
        bandRelays[i].eucSteps  = std::make_unique<juce::WebSliderRelay>(prefix + "euc_steps");
        bandRelays[i].eucPulses = std::make_unique<juce::WebSliderRelay>(prefix + "euc_pulses");
        bandRelays[i].eucOffset = std::make_unique<juce::WebSliderRelay>(prefix + "euc_offset");
    }

    // Step grid relays (128 total: 32 steps × 4 bands) — velocity sliders
    for (int band = 0; band < 4; ++band)
    {
        for (int step = 0; step < 32; ++step)
        {
            int index = band * 32 + step;
            juce::String paramId = "step_b" + juce::String(band) + "_s" + juce::String(step);
            stepRelays[index] = std::make_unique<juce::WebSliderRelay>(paramId);
        }
    }

    // 2️⃣ CREATE WEBVIEW WITH OPTIONS
    auto options = juce::WebBrowserComponent::Options{}
        .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
        .withWinWebView2Options(
            juce::WebBrowserComponent::Options::WinWebView2{}
                .withUserDataFolder(juce::File::getSpecialLocation(
                    juce::File::SpecialLocationType::tempDirectory)))
        .withNativeIntegrationEnabled()
        .withResourceProvider([this](const auto& url) { return getResource(url); })
        // Global parameter relays
        .withOptionsFrom(*mixRelay)
        .withOptionsFrom(*stepsRelay)
        .withOptionsFrom(*rateRelay)
        .withOptionsFrom(*swingRelay)
        .withOptionsFrom(*attackRelay)
        .withOptionsFrom(*releaseRelay)
        // Crossover relays
        .withOptionsFrom(*crossover1Relay)
        .withOptionsFrom(*crossover2Relay)
        .withOptionsFrom(*crossover3Relay)
        // Frequency boundary relays
        .withOptionsFrom(*freqLowRelay)
        .withOptionsFrom(*freqHighRelay);

    // Per-band relays (loop)
    for (int i = 0; i < 4; ++i)
    {
        options = options
            .withOptionsFrom(*bandRelays[i].enable)
            .withOptionsFrom(*bandRelays[i].depth)
            .withOptionsFrom(*bandRelays[i].rate)
            .withOptionsFrom(*bandRelays[i].eucOn)
            .withOptionsFrom(*bandRelays[i].eucSteps)
            .withOptionsFrom(*bandRelays[i].eucPulses)
            .withOptionsFrom(*bandRelays[i].eucOffset);
    }

    options = options
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
    attackAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("attack"), *attackRelay, nullptr);
    releaseAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("release"), *releaseRelay, nullptr);

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

    // Per-band attachments (loop)
    for (int i = 0; i < 4; ++i)
    {
        auto prefix = "band" + juce::String(i) + "_";
        bandAttachments[i].enable = std::make_unique<juce::WebToggleButtonParameterAttachment>(
            *apvts.getParameter(prefix + "enable"), *bandRelays[i].enable, nullptr);
        bandAttachments[i].depth = std::make_unique<juce::WebSliderParameterAttachment>(
            *apvts.getParameter(prefix + "depth"), *bandRelays[i].depth, nullptr);
        bandAttachments[i].rate = std::make_unique<juce::WebComboBoxParameterAttachment>(
            *apvts.getParameter(prefix + "rate"), *bandRelays[i].rate, nullptr);
        bandAttachments[i].eucOn = std::make_unique<juce::WebToggleButtonParameterAttachment>(
            *apvts.getParameter(prefix + "euc_on"), *bandRelays[i].eucOn, nullptr);
        bandAttachments[i].eucSteps = std::make_unique<juce::WebSliderParameterAttachment>(
            *apvts.getParameter(prefix + "euc_steps"), *bandRelays[i].eucSteps, nullptr);
        bandAttachments[i].eucPulses = std::make_unique<juce::WebSliderParameterAttachment>(
            *apvts.getParameter(prefix + "euc_pulses"), *bandRelays[i].eucPulses, nullptr);
        bandAttachments[i].eucOffset = std::make_unique<juce::WebSliderParameterAttachment>(
            *apvts.getParameter(prefix + "euc_offset"), *bandRelays[i].eucOffset, nullptr);
    }

    // Step grid attachments (128 total) — velocity sliders
    for (int band = 0; band < 4; ++band)
    {
        for (int step = 0; step < 32; ++step)
        {
            int index = band * 32 + step;
            juce::String paramId = "step_b" + juce::String(band) + "_s" + juce::String(step);
            stepAttachments[index] = std::make_unique<juce::WebSliderParameterAttachment>(
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
    // Read per-band step positions and signal state from processor (atomic, thread-safe)
    bool hasSignal = processorRef.getHasAudioSignal();
    int b0 = processorRef.getBandStep(0);
    int b1 = processorRef.getBandStep(1);
    int b2 = processorRef.getBandStep(2);
    int b3 = processorRef.getBandStep(3);

    // Send per-band steps to WebView for independent playhead highlighting
    juce::String js = juce::String::formatted(
        "if (window.updatePlayhead) window.updatePlayhead(%d,%d,%d,%d,%s);",
        b0, b1, b2, b3, hasSignal ? "true" : "false"
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
