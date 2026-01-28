/*
  ==============================================================================

    O-Bass - WebView Editor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

    WebView-based editor with botanical aesthetic and JUCE 8 parameter binding.

  ==============================================================================
*/

#include "PluginEditor.h"
#include "BinaryData.h"

OBassAudioProcessorEditor::OBassAudioProcessorEditor(OBassAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    // 1. Create relays FIRST (with IDs matching HTML getSliderState calls)
    frequencyRelay = std::make_unique<juce::WebSliderRelay>("crossover_freq");
    enhanceRelay = std::make_unique<juce::WebSliderRelay>("enhance");
    outputRelay = std::make_unique<juce::WebSliderRelay>("output");

    // 2. Create WebView SECOND with relay options and native functions
    webView = std::make_unique<juce::WebBrowserComponent>(
        juce::WebBrowserComponent::Options{}
            .withNativeIntegrationEnabled()
            .withResourceProvider([this](auto& url) { return getResource(url); })
            .withOptionsFrom(*frequencyRelay)
            .withOptionsFrom(*enhanceRelay)
            .withOptionsFrom(*outputRelay)
            // Limit indicator native function for UI polling
            .withNativeFunction("getLimitIndicator", [this](auto&, auto complete) {
                complete(processorRef.getLimitIndicator());
            })
            // Output level native function for VU meter
            .withNativeFunction("getOutputLevel", [this](auto&, auto complete) {
                complete(processorRef.getOutputLevelDB());
            })
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

    // 3. Create attachments LAST (Pattern #12: 3-param constructor - parameter, relay, nullptr)
    frequencyAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("crossover_freq"), *frequencyRelay, nullptr);
    enhanceAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("enhance"), *enhanceRelay, nullptr);
    outputAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("output"), *outputRelay, nullptr);

    // Add WebView (navigation happens in parentHierarchyChanged)
    addAndMakeVisible(*webView);

    // Compact size for 3-knob horizontal layout
    setSize(420, 320);

    // Start meter update timer (30fps)
    startTimerHz(30);
}

OBassAudioProcessorEditor::~OBassAudioProcessorEditor()
{
    // Members destroyed in REVERSE order of declaration (automatic RAII cleanup)
}

void OBassAudioProcessorEditor::paint(juce::Graphics& g)
{
    // WebView handles all rendering
    juce::ignoreUnused(g);
}

void OBassAudioProcessorEditor::resized()
{
    // WebView fills entire editor
    webView->setBounds(getLocalBounds());
}

void OBassAudioProcessorEditor::parentHierarchyChanged()
{
    // Navigate WebView only after editor is attached to a window (JUCE 8 requirement)
    // This prevents crashes during plugin scanning when no window context exists
    // Use per-instance hasNavigated to allow GUI reload on editor reopen
    if (isShowing() && webView != nullptr && !hasNavigated)
    {
        webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
        hasNavigated = true;
    }
}

void OBassAudioProcessorEditor::timerCallback()
{
    // Push meter data to WebView via JavaScript
    float outputLevel = processorRef.getOutputLevelDB();
    float limitIndicator = processorRef.getLimitIndicator();

    juce::String script = juce::String::formatted(
        "if (typeof updateMeters === 'function') { updateMeters(%f, %f); }",
        outputLevel, limitIndicator
    );

    webView->evaluateJavascript(script, nullptr);
}

// Pattern #8: EXPLICIT URL MAPPING (never use generic loops)
std::optional<juce::WebBrowserComponent::Resource>
OBassAudioProcessorEditor::getResource(const juce::String& url)
{
    auto makeVector = [](const char* data, int size) {
        return std::vector<std::byte>(
            reinterpret_cast<const std::byte*>(data),
            reinterpret_cast<const std::byte*>(data) + size
        );
    };

    // Root "/" -> index.html
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

    // JUCE interop checker (Pattern #13: REQUIRED for native function support)
    if (url == "/js/juce/check_native_interop.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::check_native_interop_js,
                      BinaryData::check_native_interop_jsSize),
            juce::String("text/javascript")
        };
    }

    // Background paper texture
    if (url == "/img/paper.jpg") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::paper_jpg, BinaryData::paper_jpgSize),
            juce::String("image/jpeg")
        };
    }

    // Botanical overlay image
    if (url == "/img/botanical.png") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::botanical_png, BinaryData::botanical_pngSize),
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
    juce::Logger::writeToLog("O-Bass: Resource not found: " + url);
    return std::nullopt;
}
