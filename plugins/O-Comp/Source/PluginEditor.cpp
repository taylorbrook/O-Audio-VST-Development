/*
  ==============================================================================

    O-Comp - Editor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

    Stage 3 (GUI) - WebView UI Implementation

  ==============================================================================
*/

#include "PluginEditor.h"
#include "BinaryData.h"

//==============================================================================
OCompAudioProcessorEditor::OCompAudioProcessorEditor(OCompAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    //==========================================================================
    // CRITICAL: Initialization Order
    // 1. Create relays FIRST
    // 2. Create WebView with relay options
    // 3. Create attachments LAST
    //==========================================================================

    // 1️⃣ Create relays FIRST (no dependencies)
    thresholdRelay = std::make_unique<juce::WebSliderRelay>("threshold");
    ratioRelay = std::make_unique<juce::WebSliderRelay>("ratio");
    attackTimeRelay = std::make_unique<juce::WebSliderRelay>("attack_time");
    releaseTimeRelay = std::make_unique<juce::WebSliderRelay>("release_time");
    kneeRelay = std::make_unique<juce::WebSliderRelay>("knee");
    outputGainRelay = std::make_unique<juce::WebSliderRelay>("output_gain");
    autoGainRelay = std::make_unique<juce::WebToggleButtonRelay>("auto_gain");

    // 2️⃣ Create WebView with relay options and preset native functions
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
            .withOptionsFrom(*thresholdRelay)
            .withOptionsFrom(*ratioRelay)
            .withOptionsFrom(*attackTimeRelay)
            .withOptionsFrom(*releaseTimeRelay)
            .withOptionsFrom(*kneeRelay)
            .withOptionsFrom(*outputGainRelay)
            .withOptionsFrom(*autoGainRelay)
            // Preset Manager native functions
            .withNativeFunction("savePreset", [this](auto& args, auto complete) {
                if (args.size() > 0)
                    complete(processorRef.presetManager.savePreset(args[0].toString()));
                else
                    complete(false);
            })
            .withNativeFunction("savePresetWithDialog", [this](auto&, auto complete) {
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
            .withNativeFunction("loadPreset", [this](auto& args, auto complete) {
                if (args.size() > 0)
                    complete(processorRef.presetManager.loadPreset(args[0].toString()));
                else
                    complete(false);
            })
            .withNativeFunction("getPresetList", [this](auto&, auto complete) {
                auto list = processorRef.presetManager.getPresetList();
                juce::Array<juce::var> arr;
                for (const auto& name : list)
                    arr.add(name);
                complete(juce::var(arr));
            })
            .withNativeFunction("getCurrentPreset", [this](auto&, auto complete) {
                complete(processorRef.presetManager.getCurrentPresetName());
            })
            .withNativeFunction("selectNextPreset", [this](auto&, auto complete) {
                auto next = processorRef.presetManager.getNextPreset();
                complete(next);
            })
            .withNativeFunction("selectPreviousPreset", [this](auto&, auto complete) {
                auto prev = processorRef.presetManager.getPreviousPreset();
                complete(prev);
            })
            .withNativeFunction("deletePreset", [this](auto& args, auto complete) {
                if (args.size() > 0)
                    complete(processorRef.presetManager.deletePreset(args[0].toString()));
                else
                    complete(false);
            })
            .withNativeFunction("isFactoryPreset", [this](auto& args, auto complete) {
                if (args.size() > 0)
                    complete(processorRef.presetManager.isFactoryPreset(args[0].toString()));
                else
                    complete(false);
            })
            .withNativeFunction("loadPresetFromFile", [this](auto&, auto complete) {
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
            })
    );

    addAndMakeVisible(*webView);

#if OUARICON_LICENSING_ENABLED
    // Licensing: activation overlay (visible until licensed)
    // Native WebView renders on top of JUCE components, so we must
    // hide the WebView while the overlay is showing.
    // License manager lives on the processor (persists across editor open/close).
    auto& license = processorRef.getLicenseManager();
    licenseOverlay = std::make_unique<OuariconLicenseOverlay>(license);
    addAndMakeVisible(licenseOverlay.get());

    license.addListener(this);

    if (! license.isLicensed())
        webView->setVisible(false);
    else
        licenseOverlay->setVisible(false);
#endif

    // 3️⃣ Create attachments LAST (depend on relays and webView)
    // CRITICAL: JUCE 8 requires THREE parameters (parameter, relay, undoManager)
    // Missing nullptr causes silent failure - knobs compile but don't respond
    thresholdAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("threshold"), *thresholdRelay, nullptr);
    ratioAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("ratio"), *ratioRelay, nullptr);
    attackTimeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("attack_time"), *attackTimeRelay, nullptr);
    releaseTimeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("release_time"), *releaseTimeRelay, nullptr);
    kneeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("knee"), *kneeRelay, nullptr);
    outputGainAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("output_gain"), *outputGainRelay, nullptr);
    autoGainAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.parameters.getParameter("auto_gain"), *autoGainRelay, nullptr);

    // Load UI from resource provider
#if JUCE_WEB_BROWSER_RESOURCE_PROVIDER_AVAILABLE
    webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
#endif

    // Set window size from mockup dimensions (620x360px)
    setSize(620, 360);

    // Start meter update timer (30fps = ~33ms)
    startTimerHz(30);
}

OCompAudioProcessorEditor::~OCompAudioProcessorEditor()
{
#if OUARICON_LICENSING_ENABLED
    processorRef.getLicenseManager().removeListener(this);
#endif

    // Stop timer before destruction
    stopTimer();

    // Members destroyed in REVERSE order of declaration:
    // 1. Attachments destroyed FIRST (can safely call webView methods)
    // 2. WebView destroyed SECOND (attachments are gone)
    // 3. Relays destroyed LAST (nothing using them)
}

//==============================================================================
void OCompAudioProcessorEditor::paint(juce::Graphics& g)
{
    // WebView handles all rendering
    g.fillAll(juce::Colours::black);
}

void OCompAudioProcessorEditor::resized()
{
    // WebView fills entire editor window
    webView->setBounds(getLocalBounds());

#if OUARICON_LICENSING_ENABLED
    if (licenseOverlay != nullptr)
        licenseOverlay->setBounds(getLocalBounds());
#endif
}

void OCompAudioProcessorEditor::timerCallback()
{
    // Get meter values from processor (thread-safe atomic reads)
    float inputLevel = processorRef.getInputLevelDB();
    float outputLevel = processorRef.getOutputLevelDB();
    float gainReduction = processorRef.getGainReductionDB();
    float envelope = processorRef.getEnvelopeDB();

    // Send meter data to WebView via JavaScript
    juce::String script = juce::String::formatted(
        "if (typeof updateMeters === 'function') { updateMeters(%f, %f, %f, %f); }",
        inputLevel, outputLevel, gainReduction, envelope
    );

    webView->evaluateJavascript(script, nullptr);
}

//==============================================================================
std::optional<juce::WebBrowserComponent::Resource>
OCompAudioProcessorEditor::getResource(const juce::String& url)
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

    if (url == "/paper-bg.jpg") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::paperbg_jpg, BinaryData::paperbg_jpgSize),
            juce::String("image/jpeg")
        };
    }

    if (url == "/shell.png") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::shell_png, BinaryData::shell_pngSize),
            juce::String("image/png")
        };
    }

    // Preset Manager module
    if (url == "/modules/preset-manager.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::presetmanager_js, BinaryData::presetmanager_jsSize),
            juce::String("text/javascript")
        };
    }

    // 404 - Resource not found
    juce::Logger::writeToLog("Resource not found: " + url);
    return std::nullopt;
}

#if OUARICON_LICENSING_ENABLED
//==============================================================================
void OCompAudioProcessorEditor::licenseStatusChanged(
    OuariconLicense&, OuariconLicense::Status newStatus)
{
    juce::MessageManager::callAsync([this, newStatus]()
    {
        bool licensed = (newStatus == OuariconLicense::Status::Licensed);
        webView->setVisible(licensed);

        if (licenseOverlay)
            licenseOverlay->setVisible(! licensed);
    });
}
#endif
