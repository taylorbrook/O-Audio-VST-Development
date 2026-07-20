/*
  ==============================================================================

    Ouaricon Digital Delay - Editor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginEditor.h"
#include "BinaryData.h"

OuariconDigitalDelayAudioProcessorEditor::OuariconDigitalDelayAudioProcessorEditor(OuariconDigitalDelayAudioProcessor& p)
    : AudioProcessorEditor(&p)
    , processorRef(p)
    // 1. Create relays FIRST (no dependencies)
    , timeRelay(std::make_unique<juce::WebSliderRelay>("time"))
    , feedbackRelay(std::make_unique<juce::WebSliderRelay>("feedback"))
    , spreadRelay(std::make_unique<juce::WebSliderRelay>("spread"))
    , modRelay(std::make_unique<juce::WebSliderRelay>("mod"))
    , wetRelay(std::make_unique<juce::WebSliderRelay>("wet"))
    , dryRelay(std::make_unique<juce::WebSliderRelay>("dry"))
    , syncRelay(std::make_unique<juce::WebToggleButtonRelay>("sync"))
    , divisionRelay(std::make_unique<juce::WebComboBoxRelay>("division"))
    // 2. Create WebView SECOND (depends on relays)
    , webView(std::make_unique<juce::WebBrowserComponent>(
        juce::WebBrowserComponent::Options{}
            .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
            .withWinWebView2Options(
                juce::WebBrowserComponent::Options::WinWebView2{}
                    // WR-05: plugin-specific user-data folder (not the bare temp root shared
                    // by every Ouaricon plugin) — avoids WebView2 lock contention across
                    // instances/plugins, which can silently fall back to IE (blank UI).
                    .withUserDataFolder(juce::File::getSpecialLocation(
                        juce::File::SpecialLocationType::tempDirectory)
                            .getChildFile("O-DigiDelay_WebView")))
            .withNativeIntegrationEnabled()
            .withResourceProvider([this](const auto& url) { return getResource(url); })
            .withOptionsFrom(*timeRelay)
            .withOptionsFrom(*feedbackRelay)
            .withOptionsFrom(*spreadRelay)
            .withOptionsFrom(*modRelay)
            .withOptionsFrom(*wetRelay)
            .withOptionsFrom(*dryRelay)
            .withOptionsFrom(*syncRelay)
            .withOptionsFrom(*divisionRelay)
            // Preset Manager native functions
            .withNativeFunction("savePreset", [this](auto& args, auto complete) {
                if (args.size() > 0)
                    complete(processorRef.presetManager.savePreset(args[0].toString()));
                else
                    complete(false);
            })
            .withNativeFunction("savePresetWithDialog", [this](auto&, auto complete) {
                // WR-03: presets always live in the managed User folder (so they appear
                // in getPresetList() and navigate with prev/next). A folder-navigable
                // FileChooser implied a destination that savePreset() silently ignored,
                // so use a name-only prompt instead.
                auto* aw = new juce::AlertWindow("Save Preset",
                                                 "Enter a name for this preset:",
                                                 juce::MessageBoxIconType::NoIcon);
                aw->addTextEditor("presetName", juce::String(), juce::String());
                aw->addButton("Save",   1, juce::KeyPress(juce::KeyPress::returnKey));
                aw->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
                aw->enterModalState(true,
                    juce::ModalCallbackFunction::create([this, aw, complete](int choice) {
                        auto* result = new juce::DynamicObject();
                        if (choice == 1) {
                            auto presetName = aw->getTextEditorContents("presetName").trim();
                            bool success = presetName.isNotEmpty()
                                        && processorRef.presetManager.savePreset(presetName);
                            result->setProperty("success", success);
                            result->setProperty("name", success ? presetName : juce::String());
                        } else {
                            result->setProperty("success", false);
                            result->setProperty("name", "");
                        }
                        complete(juce::var(result));
                    }),
                    true);  // deleteWhenDismissed — AlertWindow frees itself after the callback
            })
            .withNativeFunction("loadPreset", [this](auto& args, auto complete) {
                if (args.size() > 0)
                    complete(processorRef.presetManager.loadPreset(args[0].toString()));
                else
                    complete(false);
            })
            .withNativeFunction("loadPresetFromFile", [this](auto&, auto complete) {
                fileChooser = std::make_unique<juce::FileChooser>(
                    "Load Preset", processorRef.presetManager.getUserPresetsDirectory(), "*.json");
                fileChooser->launchAsync(
                    juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                    [this, complete](const juce::FileChooser& fc) {
                        auto results = fc.getResults();
                        auto* result = new juce::DynamicObject();
                        if (results.isEmpty()) {
                            result->setProperty("success", false);
                            result->setProperty("name", "");
                        } else {
                            auto file = results.getFirst();
                            bool success = processorRef.presetManager.loadPresetFromFile(file);
                            result->setProperty("success", success);
                            result->setProperty("name", success ? file.getFileNameWithoutExtension() : juce::String());
                        }
                        complete(juce::var(result));
                    });
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
                complete(processorRef.presetManager.getNextPreset());
            })
            .withNativeFunction("selectPreviousPreset", [this](auto&, auto complete) {
                complete(processorRef.presetManager.getPreviousPreset());
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
            .withNativeFunction("getPluginVersion", [](auto&, auto complete) {
                complete(juce::String(JucePlugin_VersionString));
            })
    ))
    // 3. Create attachments LAST (depend on relays AND webView)
    , timeAttachment(std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("time"), *timeRelay, nullptr))
    , feedbackAttachment(std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("feedback"), *feedbackRelay, nullptr))
    , spreadAttachment(std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("spread"), *spreadRelay, nullptr))
    , modAttachment(std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("mod"), *modRelay, nullptr))
    , wetAttachment(std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("wet"), *wetRelay, nullptr))
    , dryAttachment(std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("dry"), *dryRelay, nullptr))
    , syncAttachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.parameters.getParameter("sync"), *syncRelay, nullptr))
    , divisionAttachment(std::make_unique<juce::WebComboBoxParameterAttachment>(
        *processorRef.parameters.getParameter("division"), *divisionRelay, nullptr))
{
    // Add WebView to component hierarchy
    addAndMakeVisible(*webView);

    // CRITICAL: setSize MUST be called AFTER all components (including overlay)
    // are created. setSize triggers resized() which sizes child components.
    // If called before overlay exists, overlay gets zero bounds → invisible.
    setSize(700, 196);
    setResizable(false, false);

    // Note: Navigation happens in parentHierarchyChanged (JUCE 8 requirement)
    // This prevents crashes during plugin scanning when no window context exists

    // Start timer for RMS meter updates (30 Hz)
    startTimerHz(30);
}

OuariconDigitalDelayAudioProcessorEditor::~OuariconDigitalDelayAudioProcessorEditor()
{
    stopTimer();
}

void OuariconDigitalDelayAudioProcessorEditor::paint(juce::Graphics& g)
{
    // WebView handles all rendering
    g.fillAll(juce::Colours::black);
}

void OuariconDigitalDelayAudioProcessorEditor::resized()
{
    // Fill entire editor area with WebView
    webView->setBounds(getLocalBounds());
}

void OuariconDigitalDelayAudioProcessorEditor::parentHierarchyChanged()
{
    // Navigate WebView only after editor is attached to a window (JUCE 8 requirement)
    // This prevents crashes during plugin scanning when no window context exists
    if (isShowing() && webView != nullptr && !hasNavigated)
    {
        webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
        hasNavigated = true;
    }
}

void OuariconDigitalDelayAudioProcessorEditor::timerCallback()
{
    // Get RMS levels from processor (average L+R for mono meter)
    float rmsLeft = processorRef.getRmsLevelLeft();
    float rmsRight = processorRef.getRmsLevelRight();
    float rmsLevel = (rmsLeft + rmsRight) * 0.5f;

    // Clamp to 0-1 range
    rmsLevel = juce::jlimit(0.0f, 1.0f, rmsLevel);

    // Send to WebView via JavaScript evaluation
    juce::String js = "if (typeof updateLEDMeter === 'function') { updateLEDMeter(" +
                      juce::String(rmsLevel, 4) + "); }";
    webView->evaluateJavascript(js, nullptr);
}

//==============================================================================
/**
 * Resource provider for WebView
 *
 * CRITICAL: Explicit URL mapping (not generic loop)
 * BinaryData flattens paths: js/juce/index.js -> index_js
 * HTML requests original paths: /js/juce/index.js
 */
std::optional<juce::WebBrowserComponent::Resource>
OuariconDigitalDelayAudioProcessorEditor::getResource(const juce::String& url)
{
    // Helper to convert BinaryData to std::vector<std::byte>
    auto makeVector = [](const char* data, int size) {
        return std::vector<std::byte>(
            reinterpret_cast<const std::byte*>(data),
            reinterpret_cast<const std::byte*>(data) + size
        );
    };

    // Explicit URL mapping (clear, debuggable, reliable)
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
            makeVector(BinaryData::check_native_interop_js,
                      BinaryData::check_native_interop_jsSize),
            juce::String("application/javascript")
        };
    }

    // Image assets (from v7 mockup)
    if (url == "/img/paper1.jpg") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::paper1_jpg, BinaryData::paper1_jpgSize),
            juce::String("image/jpeg")
        };
    }

    // WR-07: space-free filename — a name with spaces breaks the exact-string match if
    // the engine percent-encodes the request path to %20 (404 → butterfly disappears).
    if (url == "/img/butterfly2_bw.png") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::butterfly2_bw_png,
                      BinaryData::butterfly2_bw_pngSize),
            juce::String("image/png")
        };
    }

    // Preset Manager module
    if (url == "/modules/preset-manager.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::presetmanager_js, BinaryData::presetmanager_jsSize),
            juce::String("application/javascript")
        };
    }

    // 404 - resource not found
    return std::nullopt;
}
