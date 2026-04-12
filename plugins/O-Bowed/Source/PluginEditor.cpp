/*
  ==============================================================================

    O-Bowed - Editor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginEditor.h"
#include "BinaryData.h"
#include "TuningEngine.h"
#include "ScaleGenerator.h"
#include "EmbeddedTunings.h"
#include "TuningExporter.h"

OBowedAudioProcessorEditor::OBowedAudioProcessorEditor(OBowedAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    // ===================================================================
    // 1. CREATE RELAYS (must be created BEFORE WebView)
    // ===================================================================

    // Bow Controls (5 sliders + 1 combo)
    bowSpeedRelay = std::make_unique<juce::WebSliderRelay>("bowSpeed");
    bowPressureRelay = std::make_unique<juce::WebSliderRelay>("bowPressure");
    bowPositionRelay = std::make_unique<juce::WebSliderRelay>("bowPosition");
    rosinRelay = std::make_unique<juce::WebSliderRelay>("rosin");
    bowNoiseRelay = std::make_unique<juce::WebSliderRelay>("bowNoise");
    frictionTierRelay = std::make_unique<juce::WebComboBoxRelay>("frictionTier");

    // Body Controls
    bodyMaterialRelay = std::make_unique<juce::WebSliderRelay>("bodyMaterial");
    bodySizeRelay = std::make_unique<juce::WebSliderRelay>("bodySize");
    brightnessRelay = std::make_unique<juce::WebSliderRelay>("brightness");

    // String Configuration
    sympatheticAmountRelay = std::make_unique<juce::WebSliderRelay>("sympatheticAmount");
    sympatheticCountRelay = std::make_unique<juce::WebSliderRelay>("sympatheticCount");

    // Output
    widthRelay = std::make_unique<juce::WebSliderRelay>("width");
    outputLevelRelay = std::make_unique<juce::WebSliderRelay>("outputLevel");

    // Impossible Physics
    infiniteSustainRelay = std::make_unique<juce::WebSliderRelay>("infiniteSustain");
    reversedFrictionRelay = std::make_unique<juce::WebSliderRelay>("reversedFriction");
    subHarmonicsRelay = std::make_unique<juce::WebSliderRelay>("subHarmonics");

    // Tuning
    referencePitchRelay = std::make_unique<juce::WebSliderRelay>("referencePitch");
    tuningSystemRelay = std::make_unique<juce::WebComboBoxRelay>("tuningSystem");

    // ===================================================================
    // 2. CREATE WEBVIEW with all relays + native functions
    // ===================================================================
    webView = std::make_unique<juce::WebBrowserComponent>(
        juce::WebBrowserComponent::Options{}
            .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
            .withWinWebView2Options(
                juce::WebBrowserComponent::Options::WinWebView2{}
                    .withUserDataFolder(juce::File::getSpecialLocation(
                        juce::File::SpecialLocationType::tempDirectory)
                            .getChildFile("OBowed_WebView"))
                    .withStatusBarDisabled()
                    .withBuiltInErrorPageDisabled())
            .withNativeIntegrationEnabled()
            .withKeepPageLoadedWhenBrowserIsHidden()
            .withResourceProvider([this](const juce::String& url) {
                return getResource(url);
            })
            // Bow Controls
            .withOptionsFrom(*bowSpeedRelay)
            .withOptionsFrom(*bowPressureRelay)
            .withOptionsFrom(*bowPositionRelay)
            .withOptionsFrom(*rosinRelay)
            .withOptionsFrom(*bowNoiseRelay)
            .withOptionsFrom(*frictionTierRelay)
            // Body Controls
            .withOptionsFrom(*bodyMaterialRelay)
            .withOptionsFrom(*bodySizeRelay)
            .withOptionsFrom(*brightnessRelay)
            // String Configuration
            .withOptionsFrom(*sympatheticAmountRelay)
            .withOptionsFrom(*sympatheticCountRelay)
            // Output
            .withOptionsFrom(*widthRelay)
            .withOptionsFrom(*outputLevelRelay)
            // Impossible Physics
            .withOptionsFrom(*infiniteSustainRelay)
            .withOptionsFrom(*reversedFrictionRelay)
            .withOptionsFrom(*subHarmonicsRelay)
            // Tuning
            .withOptionsFrom(*referencePitchRelay)
            .withOptionsFrom(*tuningSystemRelay)

            // =============================================================
            // VISUALIZATION NATIVE FUNCTION
            // =============================================================

            .withNativeFunction("getVisualizationState", [this](auto, auto complete) {
                auto& apvts = processorRef.getAPVTS();
                float bowSpeed    = apvts.getRawParameterValue("bowSpeed")->load();
                float bowPressure = apvts.getRawParameterValue("bowPressure")->load();
                float bowPosition = apvts.getRawParameterValue("bowPosition")->load();
                float material    = apvts.getRawParameterValue("bodyMaterial")->load();
                float bodySize    = apvts.getRawParameterValue("bodySize")->load();
                float brightness  = apvts.getRawParameterValue("brightness")->load();

                // Build JSON string directly for performance
                juce::String json = juce::String::formatted(
                    "{\"bowSpeed\":%.4f,\"bowPressure\":%.4f,\"bowPosition\":%.4f,"
                    "\"material\":%.4f,\"bodySize\":%.4f,\"brightness\":%.4f,"
                    "\"isPlaying\":%s}",
                    bowSpeed, bowPressure, bowPosition,
                    material, bodySize, brightness,
                    processorRef.isAnyVoiceActive() ? "true" : "false"
                );
                complete(juce::var(json));
            })

            // =============================================================
            // PRESET NATIVE FUNCTIONS
            // =============================================================

            .withNativeFunction("getPresetList", [this](auto, auto complete) {
                auto& pm = processorRef.getPresetManager();
                auto presets = pm.getPresetList();
                juce::Array<juce::var> arr;
                for (const auto& name : presets)
                    arr.add(name);
                complete(juce::var(arr));
            })

            .withNativeFunction("getCurrentPreset", [this](auto, auto complete) {
                auto& pm = processorRef.getPresetManager();
                complete(juce::var(pm.getCurrentPresetName()));
            })

            .withNativeFunction("loadPreset", [this](const auto& args, auto complete) {
                if (args.size() < 1 || !args[0].isString())
                {
                    complete(juce::var(false));
                    return;
                }
                auto& pm = processorRef.getPresetManager();
                bool success = pm.loadPreset(args[0].toString());
                complete(juce::var(success));
            })

            .withNativeFunction("savePreset", [this](const auto& args, auto complete) {
                if (args.size() < 1 || !args[0].isString())
                {
                    complete(juce::var(false));
                    return;
                }
                auto& pm = processorRef.getPresetManager();
                bool success = pm.savePreset(args[0].toString());
                complete(juce::var(success));
            })

            .withNativeFunction("selectNextPreset", [this](auto, auto complete) {
                auto& pm = processorRef.getPresetManager();
                auto nextName = pm.getNextPreset();
                pm.loadPreset(nextName);
                complete(juce::var(pm.getCurrentPresetName()));
            })

            .withNativeFunction("selectPreviousPreset", [this](auto, auto complete) {
                auto& pm = processorRef.getPresetManager();
                auto prevName = pm.getPreviousPreset();
                pm.loadPreset(prevName);
                complete(juce::var(pm.getCurrentPresetName()));
            })

            .withNativeFunction("savePresetWithDialog", [this](auto, auto complete) {
                fileChooser = std::make_shared<juce::FileChooser>(
                    "Save Preset",
                    processorRef.getPresetManager().getUserPresetsDirectory(),
                    "*.json"
                );
                fileChooser->launchAsync(
                    juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                    [this, complete](const juce::FileChooser& fc) {
                        auto result = fc.getResult();
                        if (result == juce::File{})
                        {
                            complete(juce::var(""));
                            return;
                        }
                        auto name = result.getFileNameWithoutExtension();
                        auto& pm = processorRef.getPresetManager();
                        pm.savePreset(name);
                        complete(juce::var(name));
                    }
                );
            })

            // =============================================================
            // TUNING NATIVE FUNCTIONS
            // =============================================================

            .withNativeFunction("getTuningIntervals", [this](const juce::Array<juce::var>&, auto complete) {
                auto intervals = processorRef.getTuningEngine()->getIntervals();
                juce::String json = "[";
                for (size_t i = 0; i < intervals.size(); ++i) {
                    if (i > 0) json += ",";
                    json += juce::String(intervals[i], 6);
                }
                json += "]";
                complete(json);
            })

            .withNativeFunction("setTuningIntervals", [this](const juce::Array<juce::var>& args, auto complete) {
                if (args.size() >= 1) {
                    auto jsonArray = juce::JSON::parse(args[0].toString());
                    if (auto* arr = jsonArray.getArray()) {
                        std::vector<double> intervals;
                        for (const auto& val : *arr)
                            intervals.push_back(static_cast<double>(val));
                        processorRef.getTuningEngine()->setCustomIntervals(intervals, "Custom");
                        complete(true);
                        return;
                    }
                }
                complete(false);
            })

            .withNativeFunction("getTuningName", [this](const juce::Array<juce::var>&, auto complete) {
                complete(processorRef.getTuningEngine()->getActiveTuningName());
            })

            .withNativeFunction("setSingleInterval", [this](const juce::Array<juce::var>& args, auto complete) {
                if (args.size() >= 2) {
                    int index = static_cast<int>(args[0]);
                    double cents = static_cast<double>(args[1]);
                    processorRef.getTuningEngine()->setSingleInterval(index, cents);
                    complete(true);
                    return;
                }
                complete(false);
            })

            .withNativeFunction("setTonicNote", [this](const juce::Array<juce::var>& args, auto complete) {
                if (args.size() >= 1) {
                    processorRef.getTuningEngine()->setTonicNote(static_cast<int>(args[0]));
                    complete(true);
                    return;
                }
                complete(false);
            })

            .withNativeFunction("getTonicNote", [this](const juce::Array<juce::var>&, auto complete) {
                complete(processorRef.getTuningEngine()->getTonicNote());
            })

            .withNativeFunction("getOctaveStretch", [this](const juce::Array<juce::var>&, auto complete) {
                complete(processorRef.getTuningEngine()->getOctaveStretch());
            })

            .withNativeFunction("setOctaveStretch", [this](const juce::Array<juce::var>& args, auto complete) {
                if (args.size() >= 1) {
                    processorRef.getTuningEngine()->setOctaveStretch(static_cast<float>(args[0]));
                    complete(true);
                    return;
                }
                complete(false);
            })

            .withNativeFunction("getMasterTune", [this](const juce::Array<juce::var>&, auto complete) {
                complete(processorRef.getTuningEngine()->getMasterTune());
            })

            .withNativeFunction("setMasterTune", [this](const juce::Array<juce::var>& args, auto complete) {
                if (args.size() >= 1) {
                    processorRef.getTuningEngine()->setMasterTune(static_cast<double>(args[0]));
                    complete(true);
                    return;
                }
                complete(false);
            })

            .withNativeFunction("setTemperamentPreset", [this](const juce::Array<juce::var>& args, auto complete) {
                if (args.size() >= 1) {
                    int preset = static_cast<int>(args[0]);
                    processorRef.getTuningEngine()->setBuiltInPreset(
                        static_cast<TuningEngine::BuiltInPreset>(preset));
                    complete(true);
                    return;
                }
                complete(false);
            })

            .withNativeFunction("getTemperamentPreset", [this](const juce::Array<juce::var>&, auto complete) {
                complete(static_cast<int>(processorRef.getTuningEngine()->getBuiltInPreset()));
            })

            .withNativeFunction("loadScalaFile", [this](const juce::Array<juce::var>&, auto complete) {
                fileChooser = std::make_shared<juce::FileChooser>(
                    "Load Scala File",
                    juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
                    "*.scl;*.tun");
                fileChooser->launchAsync(
                    juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                    [this, complete](const juce::FileChooser& fc) {
                        auto file = fc.getResult();
                        if (file.existsAsFile()) {
                            bool success = processorRef.getTuningEngine()->loadScalaFile(file);
                            complete(success);
                        } else {
                            complete(false);
                        }
                    });
            })

            .withNativeFunction("loadKBMFile", [this](const juce::Array<juce::var>&, auto complete) {
                fileChooser = std::make_shared<juce::FileChooser>(
                    "Load Keyboard Mapping",
                    juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
                    "*.kbm");
                fileChooser->launchAsync(
                    juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                    [this, complete](const juce::FileChooser& fc) {
                        auto file = fc.getResult();
                        if (file.existsAsFile()) {
                            bool success = processorRef.getTuningEngine()->loadKBMFile(file);
                            complete(success);
                        } else {
                            complete(false);
                        }
                    });
            })

            .withNativeFunction("generateEDO", [](const juce::Array<juce::var>& args, auto complete) {
                if (args.size() >= 2) {
                    int divisions = static_cast<int>(args[0]);
                    double period = static_cast<double>(args[1]);
                    auto intervals = ScaleGenerator::generateEDO(divisions, period);
                    juce::String json = "[";
                    for (size_t i = 0; i < intervals.size(); ++i) {
                        if (i > 0) json += ",";
                        json += juce::String(intervals[i], 6);
                    }
                    json += "]";
                    complete(json);
                    return;
                }
                complete(false);
            })

            .withNativeFunction("getTuningHTML", [this](const juce::Array<juce::var>&, auto complete) {
                auto html = TuningExporter::toHTML(*processorRef.getTuningEngine(), "O-Bowed");
                complete(html);
            })
    );

    addAndMakeVisible(webView.get());

    // ===================================================================
    // 3. CREATE ATTACHMENTS (must be created AFTER WebView)
    // ===================================================================
    auto& apvts = processorRef.getAPVTS();

    // Bow Controls
    bowSpeedAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("bowSpeed"), *bowSpeedRelay, nullptr);
    bowPressureAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("bowPressure"), *bowPressureRelay, nullptr);
    bowPositionAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("bowPosition"), *bowPositionRelay, nullptr);
    rosinAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("rosin"), *rosinRelay, nullptr);
    bowNoiseAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("bowNoise"), *bowNoiseRelay, nullptr);
    frictionTierAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *apvts.getParameter("frictionTier"), *frictionTierRelay, nullptr);

    // Body Controls
    bodyMaterialAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("bodyMaterial"), *bodyMaterialRelay, nullptr);
    bodySizeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("bodySize"), *bodySizeRelay, nullptr);
    brightnessAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("brightness"), *brightnessRelay, nullptr);

    // String Configuration
    sympatheticAmountAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("sympatheticAmount"), *sympatheticAmountRelay, nullptr);
    sympatheticCountAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("sympatheticCount"), *sympatheticCountRelay, nullptr);

    // Output
    widthAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("width"), *widthRelay, nullptr);
    outputLevelAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("outputLevel"), *outputLevelRelay, nullptr);

    // Impossible Physics
    infiniteSustainAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("infiniteSustain"), *infiniteSustainRelay, nullptr);
    reversedFrictionAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("reversedFriction"), *reversedFrictionRelay, nullptr);
    subHarmonicsAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("subHarmonics"), *subHarmonicsRelay, nullptr);

    // Tuning
    referencePitchAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("referencePitch"), *referencePitchRelay, nullptr);
    tuningSystemAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *apvts.getParameter("tuningSystem"), *tuningSystemRelay, nullptr);

    // Navigate to embedded UI
    webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());

    // Set window size (900x600 per CONTEXT.md)
    setSize(900, 600);
}

OBowedAudioProcessorEditor::~OBowedAudioProcessorEditor()
{
}

void OBowedAudioProcessorEditor::paint(juce::Graphics& g)
{
    juce::ignoreUnused(g);  // WebView handles all painting
}

void OBowedAudioProcessorEditor::resized()
{
    if (webView)
        webView->setBounds(getLocalBounds());
}

//==============================================================================
// Resource Provider - serves embedded UI files from BinaryData
//==============================================================================
std::optional<juce::WebBrowserComponent::Resource>
OBowedAudioProcessorEditor::getResource(const juce::String& url)
{
    auto makeResource = [](const char* data, int size, const char* mimeType) {
        return juce::WebBrowserComponent::Resource {
            std::vector<std::byte>(
                reinterpret_cast<const std::byte*>(data),
                reinterpret_cast<const std::byte*>(data) + size
            ),
            juce::String(mimeType)
        };
    };

    // HTML
    if (url == "/" || url == "/index.html")
        return makeResource(BinaryData::index_html,
                           BinaryData::index_htmlSize,
                           "text/html");

    // JUCE Bridge
    if (url == "/js/juce/index.js")
        return makeResource(BinaryData::index_js,
                           BinaryData::index_jsSize,
                           "text/javascript");

    if (url == "/js/juce/check_native_interop.js")
        return makeResource(BinaryData::check_native_interop_js,
                           BinaryData::check_native_interop_jsSize,
                           "text/javascript");

    // Tuning panel
    if (url == "/js/tuning-panel.js")
        return makeResource(BinaryData::tuningpanel_js,
                           BinaryData::tuningpanel_jsSize,
                           "text/javascript");

    if (url == "/css/tuning-panel.css")
        return makeResource(BinaryData::tuningpanel_css,
                           BinaryData::tuningpanel_cssSize,
                           "text/css");

    // Botanical illustration
    if (url == "/img/botanical.png")
        return makeResource(BinaryData::botanical_png,
                           BinaryData::botanical_pngSize,
                           "image/png");

    DBG("Resource not found: " + url);
    return std::nullopt;
}
