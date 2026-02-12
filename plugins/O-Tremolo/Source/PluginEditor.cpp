/*
  ==============================================================================

    OuariconTremolo - Editor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginEditor.h"
#include "BinaryData.h"

OuariconTremoloAudioProcessorEditor::OuariconTremoloAudioProcessorEditor(OuariconTremoloAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    // 1. Create relays FIRST (with parameter IDs matching HTML)
    speedRelay = std::make_unique<juce::WebSliderRelay>("speed");
    depthRelay = std::make_unique<juce::WebSliderRelay>("depth");
    waveformRelay = std::make_unique<juce::WebSliderRelay>("waveform");
    smoothingRelay = std::make_unique<juce::WebSliderRelay>("smoothing");
    panSyncRelay = std::make_unique<juce::WebToggleButtonRelay>("panSync");
    tempoSyncRelay = std::make_unique<juce::WebToggleButtonRelay>("tempoSync");

    // 2. Create WebView SECOND with all relay options and preset native functions
    webView = std::make_unique<juce::WebBrowserComponent>(
        juce::WebBrowserComponent::Options{}
            .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
            .withWinWebView2Options(
                juce::WebBrowserComponent::Options::WinWebView2{}
                    .withUserDataFolder(juce::File::getSpecialLocation(
                        juce::File::SpecialLocationType::tempDirectory)))
            .withNativeIntegrationEnabled()
#if JUCE_WEB_BROWSER_RESOURCE_PROVIDER_AVAILABLE
            .withResourceProvider([this](auto& url) { return getResource(url); })
#endif
            .withOptionsFrom(*speedRelay)
            .withOptionsFrom(*depthRelay)
            .withOptionsFrom(*waveformRelay)
            .withOptionsFrom(*smoothingRelay)
            .withOptionsFrom(*panSyncRelay)
            .withOptionsFrom(*tempoSyncRelay)
            // Preset Manager native functions
            .withNativeFunction("savePreset", [this](auto& args, auto complete) {
                if (args.size() > 0)
                    complete(processorRef.presetManager.savePreset(args[0].toString()));
                else
                    complete(false);
            })
            .withNativeFunction("savePresetWithDialog", [this](auto&, auto complete) {
                // Create file chooser for saving preset
                fileChooser = std::make_unique<juce::FileChooser>(
                    "Save Preset",
                    processorRef.presetManager.getUserPresetsDirectory(),
                    "*.json"
                );

                // Launch async save dialog
                fileChooser->launchAsync(
                    juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                    [this, complete](const juce::FileChooser& fc) {
                        auto results = fc.getResults();
                        if (results.isEmpty()) {
                            // User cancelled
                            auto* result = new juce::DynamicObject();
                            result->setProperty("success", false);
                            result->setProperty("name", "");
                            complete(juce::var(result));
                            return;
                        }

                        auto file = results.getFirst();
                        auto presetName = file.getFileNameWithoutExtension();

                        // Save using the preset manager
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
                // Create file chooser for preset JSON files
                fileChooser = std::make_unique<juce::FileChooser>(
                    "Load Preset",
                    processorRef.presetManager.getUserPresetsDirectory(),
                    "*.json"
                );

                // Launch async file dialog
                fileChooser->launchAsync(
                    juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                    [this, complete](const juce::FileChooser& fc) {
                        auto results = fc.getResults();
                        if (results.isEmpty()) {
                            // User cancelled
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

    // 3. Create attachments LAST (Pattern #12: 3 parameters - parameter, relay, nullptr)
    speedAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("SPEED_PARAM"), *speedRelay, nullptr);
    depthAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("DEPTH_PARAM"), *depthRelay, nullptr);
    waveformAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("WAVEFORM_PARAM"), *waveformRelay, nullptr);
    smoothingAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("SMOOTHING_PARAM"), *smoothingRelay, nullptr);
    panSyncAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.parameters.getParameter("PAN_SYNC_PARAM"), *panSyncRelay, nullptr);
    tempoSyncAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.parameters.getParameter("TEMPO_SYNC_PARAM"), *tempoSyncRelay, nullptr);

    // Add WebView (navigation happens in parentHierarchyChanged)
    addAndMakeVisible(*webView);

#if OUARICON_LICENSING_ENABLED
    // Licensing: activation overlay (visible until licensed)
    // Native WebView renders on top of JUCE components, so we must
    // hide the WebView while the overlay is showing.
    // License manager lives on the processor (persists across editor open/close).
    auto& license = processorRef.getLicenseManager();
    licenseOverlay = std::make_unique<OuariconLicenseOverlay>(license);
    addChildComponent(licenseOverlay.get());

    license.addListener(this);

    if (! license.isLicensed())
    {
        licenseOverlay->setVisible(true);
        webView->setVisible(false);
    }
#endif

    // Set size AFTER all components are created (CRITICAL: prevents crash)
    setSize(600, 400);
}

OuariconTremoloAudioProcessorEditor::~OuariconTremoloAudioProcessorEditor()
{
#if OUARICON_LICENSING_ENABLED
    processorRef.getLicenseManager().removeListener(this);
#endif
    // Members destroyed in REVERSE order of declaration (automatic cleanup)
}

void OuariconTremoloAudioProcessorEditor::paint(juce::Graphics& g)
{
    // WebView handles all painting
    juce::ignoreUnused(g);
}

void OuariconTremoloAudioProcessorEditor::resized()
{
    // WebView fills entire editor
    webView->setBounds(getLocalBounds());
#if OUARICON_LICENSING_ENABLED
    if (licenseOverlay != nullptr)
        licenseOverlay->setBounds(getLocalBounds());
#endif
}

void OuariconTremoloAudioProcessorEditor::parentHierarchyChanged()
{
    // Navigate WebView only after editor is attached to a window (JUCE 8 requirement)
    // This prevents crashes during plugin scanning when no window context exists
    // FIX: Use member variable instead of static to allow GUI reload on reopen
    if (isShowing() && webView != nullptr && !hasNavigated)
    {
        webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
        hasNavigated = true;
    }
}

// Pattern #8: EXPLICIT URL MAPPING (never use generic loops)
std::optional<juce::WebBrowserComponent::Resource>
OuariconTremoloAudioProcessorEditor::getResource(const juce::String& url)
{
    auto makeVector = [](const char* data, int size) {
        return std::vector<std::byte>(
            reinterpret_cast<const std::byte*>(data),
            reinterpret_cast<const std::byte*>(data) + size
        );
    };

    // Root "/" → index.html
    if (url == "/" || url == "/index.html") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::index_html, BinaryData::index_htmlSize),
            juce::String("text/html")
        };
    }

    // JUCE JavaScript bridge
    if (url == "/js/juce/index.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::index_js, BinaryData::index_jsSize),
            juce::String("text/javascript")
        };
    }

    // JUCE interop checker (Pattern #13: REQUIRED)
    if (url == "/js/juce/check_native_interop.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::check_native_interop_js,
                      BinaryData::check_native_interop_jsSize),
            juce::String("text/javascript")
        };
    }

    // Background image
    if (url == "/img/paper.jpg") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::paper_jpg, BinaryData::paper_jpgSize),
            juce::String("image/jpeg")
        };
    }

    // Botanical overlay
    if (url == "/img/carrot.png") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::carrot_png, BinaryData::carrot_pngSize),
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

    // Resource not found
    juce::Logger::writeToLog("Resource not found: " + url);
    return std::nullopt;
}

#if OUARICON_LICENSING_ENABLED
//==============================================================================
void OuariconTremoloAudioProcessorEditor::licenseStatusChanged(
    OuariconLicense&, OuariconLicense::Status newStatus)
{
    juce::MessageManager::callAsync([this, newStatus]()
    {
        bool licensed = (newStatus == OuariconLicense::Status::Licensed);
        webView->setVisible(licensed);

        if (licenseOverlay)
            licenseOverlay->setVisible(! licensed);

        // Force WebView reload after becoming visible — WebView2 on Windows
        // drops parameter sync events when the component is hidden during startup
        if (licensed)
            webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
    });
}
#endif
