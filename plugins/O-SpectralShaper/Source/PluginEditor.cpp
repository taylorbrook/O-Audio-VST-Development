/*
  ==============================================================================

    O-SpectralShaper - Editor Implementation
    Ouaricon Development
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginEditor.h"
#include "BinaryData.h"

OSpectralShaperAudioProcessorEditor::OSpectralShaperAudioProcessorEditor(
    OSpectralShaperAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    // ============================================================================
    // 1. Create relays FIRST (with parameter IDs matching APVTS)
    // ============================================================================

    mixRelay = std::make_unique<juce::WebSliderRelay>("MIX");
    attackTimeRelay = std::make_unique<juce::WebSliderRelay>("ATTACK_TIME");
    sustainTimeRelay = std::make_unique<juce::WebSliderRelay>("SUSTAIN_TIME");
    sensitivityRelay = std::make_unique<juce::WebSliderRelay>("SENSITIVITY");
    lookaheadEnabledRelay = std::make_unique<juce::WebToggleButtonRelay>("LOOKAHEAD_ENABLED");
    lookaheadTimeRelay = std::make_unique<juce::WebSliderRelay>("LOOKAHEAD_TIME");
    outputGainRelay = std::make_unique<juce::WebSliderRelay>("OUTPUT_GAIN");

    // ============================================================================
    // 2. Create WebView SECOND with all relay options registered
    // ============================================================================

    webView = std::make_unique<juce::WebBrowserComponent>(
        juce::WebBrowserComponent::Options{}
            .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
            .withWinWebView2Options(
                juce::WebBrowserComponent::Options::WinWebView2{}
                    .withUserDataFolder(juce::File::getSpecialLocation(
                        juce::File::SpecialLocationType::tempDirectory)))
            .withNativeIntegrationEnabled()
            .withResourceProvider([this](auto& url) { return getResource(url); })
            .withOptionsFrom(*mixRelay)
            .withOptionsFrom(*attackTimeRelay)
            .withOptionsFrom(*sustainTimeRelay)
            .withOptionsFrom(*sensitivityRelay)
            .withOptionsFrom(*lookaheadEnabledRelay)
            .withOptionsFrom(*lookaheadTimeRelay)
            .withOptionsFrom(*outputGainRelay)
            .withNativeFunction("setAttackCurve", [this](const juce::Array<juce::var>& args, auto) {
                handleAttackCurveUpdate(args);
            })
            .withNativeFunction("setSustainCurve", [this](const juce::Array<juce::var>& args, auto) {
                handleSustainCurveUpdate(args);
            })
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
                        auto presetName = results.getFirst().getFileNameWithoutExtension();
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

    // ============================================================================
    // 3. Create attachments LAST (connect parameters to relays)
    // ============================================================================

    mixAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.getAPVTS().getParameter("MIX"), *mixRelay, nullptr);
    attackTimeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.getAPVTS().getParameter("ATTACK_TIME"), *attackTimeRelay, nullptr);
    sustainTimeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.getAPVTS().getParameter("SUSTAIN_TIME"), *sustainTimeRelay, nullptr);
    sensitivityAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.getAPVTS().getParameter("SENSITIVITY"), *sensitivityRelay, nullptr);
    lookaheadEnabledAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.getAPVTS().getParameter("LOOKAHEAD_ENABLED"), *lookaheadEnabledRelay, nullptr);
    lookaheadTimeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.getAPVTS().getParameter("LOOKAHEAD_TIME"), *lookaheadTimeRelay, nullptr);
    outputGainAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.getAPVTS().getParameter("OUTPUT_GAIN"), *outputGainRelay, nullptr);

    // ============================================================================
    // Add WebView to editor (navigation happens in parentHierarchyChanged)
    // ============================================================================

    addAndMakeVisible(*webView);

    // Set editor size (700x500 for Stage 3 full UI)
    setSize(700, 500);

    // Start 60fps timer for visualization updates (Phase 3.3)
    startTimerHz(60);
}

OSpectralShaperAudioProcessorEditor::~OSpectralShaperAudioProcessorEditor()
{
    // Stop timer before destruction (CRITICAL for thread safety)
    stopTimer();
}

void OSpectralShaperAudioProcessorEditor::paint(juce::Graphics& g)
{
    // WebView handles all rendering
    juce::ignoreUnused(g);
}

void OSpectralShaperAudioProcessorEditor::resized()
{
    // WebView fills entire editor
    webView->setBounds(getLocalBounds());
}

void OSpectralShaperAudioProcessorEditor::parentHierarchyChanged()
{
    // Navigate WebView only after editor is attached to a window (JUCE 8 requirement)
    // This prevents crashes during plugin scanning when no window context exists
    if (isShowing() && webView != nullptr && !hasNavigated)
    {
        webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
        hasNavigated = true;

        // Send initial curve data to JavaScript after a short delay
        juce::Timer::callAfterDelay(100, [this]() {
            sendAttackCurveToJS();
            sendSustainCurveToJS();
        });
    }
}

std::optional<juce::WebBrowserComponent::Resource>
OSpectralShaperAudioProcessorEditor::getResource(const juce::String& url)
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

    // JUCE interop checker
    if (url == "/js/juce/check_native_interop.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::check_native_interop_js,
                      BinaryData::check_native_interop_jsSize),
            juce::String("text/javascript")
        };
    }

    // CSS
    if (url == "/css/styles.css") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::styles_css, BinaryData::styles_cssSize),
            juce::String("text/css")
        };
    }

    // JavaScript modules
    if (url == "/js/app.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::app_js, BinaryData::app_jsSize),
            juce::String("text/javascript")
        };
    }

    if (url == "/js/components/RotaryKnob.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::RotaryKnob_js, BinaryData::RotaryKnob_jsSize),
            juce::String("text/javascript")
        };
    }

    if (url == "/js/components/CurveEditor.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::CurveEditor_js, BinaryData::CurveEditor_jsSize),
            juce::String("text/javascript")
        };
    }

    if (url == "/js/components/FreehandCurve.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::FreehandCurve_js, BinaryData::FreehandCurve_jsSize),
            juce::String("text/javascript")
        };
    }

    if (url == "/js/components/NodeCurve.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::NodeCurve_js, BinaryData::NodeCurve_jsSize),
            juce::String("text/javascript")
        };
    }

    if (url == "/js/components/Spectrogram.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::Spectrogram_js, BinaryData::Spectrogram_jsSize),
            juce::String("text/javascript")
        };
    }

    // Images
    if (url == "/images/paper-bg.webp") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::paperbg_webp, BinaryData::paperbg_webpSize),
            juce::String("image/webp")
        };
    }

    if (url == "/images/slug-overlay.webp") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::slugoverlay_webp, BinaryData::slugoverlay_webpSize),
            juce::String("image/webp")
        };
    }

    // Preset Manager JS module
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

void OSpectralShaperAudioProcessorEditor::handleAttackCurveUpdate(const juce::Array<juce::var>& args)
{
    if (args.size() != 32) return;

    std::array<float, 32> curveData;
    for (int i = 0; i < 32; ++i)
        curveData[i] = static_cast<float>(args[i]);

    processorRef.setAttackCurve(curveData);
}

void OSpectralShaperAudioProcessorEditor::handleSustainCurveUpdate(const juce::Array<juce::var>& args)
{
    if (args.size() != 32) return;

    std::array<float, 32> curveData;
    for (int i = 0; i < 32; ++i)
        curveData[i] = static_cast<float>(args[i]);

    processorRef.setSustainCurve(curveData);
}

void OSpectralShaperAudioProcessorEditor::sendCurveToJS(
    const char* functionName, const std::array<float, 32>& curve)
{
    if (!webView) return;

    juce::String jsArray = "[";
    for (size_t i = 0; i < curve.size(); ++i) {
        jsArray += juce::String(curve[i]);
        if (i < curve.size() - 1) jsArray += ",";
    }
    jsArray += "]";

    webView->evaluateJavascript(
        juce::String("if (window.") + functionName + ") window." + functionName + "(" + jsArray + ");");
}

void OSpectralShaperAudioProcessorEditor::sendAttackCurveToJS()
{
    sendCurveToJS("setAttackCurveFromCPP", processorRef.getAttackCurve());
}

void OSpectralShaperAudioProcessorEditor::sendSustainCurveToJS()
{
    sendCurveToJS("setSustainCurveFromCPP", processorRef.getSustainCurve());
}

void OSpectralShaperAudioProcessorEditor::emitVisualizationFrame(
    const OSpectralShaperAudioProcessor::VisualizationFrame& frame)
{
    if (!webView) return;

    juce::String json = "{\"fft\":[";
    for (size_t i = 0; i < frame.fftMagnitudes.size(); ++i)
    {
        json += juce::String(frame.fftMagnitudes[i], 6);
        if (i < frame.fftMagnitudes.size() - 1) json += ",";
    }
    json += "],\"transients\":[";
    for (size_t i = 0; i < frame.transientActivity.size(); ++i)
    {
        json += juce::String(frame.transientActivity[i], 6);
        if (i < frame.transientActivity.size() - 1) json += ",";
    }
    json += "]}";

    webView->emitEventIfBrowserIsVisible("visualizationUpdate", json);
}

void OSpectralShaperAudioProcessorEditor::timerCallback()
{
    auto& fifo = processorRef.getVisualizationFifo();
    const auto& buffer = processorRef.getVisualizationBuffer();

    while (fifo.getNumReady() > 0)
    {
        int start1, size1, start2, size2;
        fifo.prepareToRead(1, start1, size1, start2, size2);

        if (size1 > 0)
            emitVisualizationFrame(buffer[static_cast<size_t>(start1)]);

        fifo.finishedRead(size1 + size2);
    }
}
