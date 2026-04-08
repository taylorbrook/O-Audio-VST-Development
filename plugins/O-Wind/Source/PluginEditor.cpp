/*
  ==============================================================================

    O-Wind - Editor Implementation
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

OWindAudioProcessorEditor::OWindAudioProcessorEditor(OWindAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    // ===================================================================
    // 1. CREATE RELAYS (must be created BEFORE WebView) - 14 slider + 1 toggle + 1 slider
    // ===================================================================

    // Excitation Controls
    breathPressureRelay = std::make_unique<juce::WebSliderRelay>("breathPressure");
    embouchureRelay = std::make_unique<juce::WebSliderRelay>("embouchure");
    breathNoiseRelay = std::make_unique<juce::WebSliderRelay>("breathNoise");

    // Resonator Controls
    materialRelay = std::make_unique<juce::WebSliderRelay>("material");
    toneColorRelay = std::make_unique<juce::WebSliderRelay>("toneColor");
    airColumnRelay = std::make_unique<juce::WebSliderRelay>("airColumn");
    jetReflectionRelay = std::make_unique<juce::WebSliderRelay>("jetReflection");
    endReflectionRelay = std::make_unique<juce::WebSliderRelay>("endReflection");

    // Articulation
    flutterTongueRelay = std::make_unique<juce::WebSliderRelay>("flutterTongue");
    flutterRateRelay = std::make_unique<juce::WebSliderRelay>("flutterRate");

    // Modulation
    vibratoRateRelay = std::make_unique<juce::WebSliderRelay>("vibratoRate");
    vibratoDepthRelay = std::make_unique<juce::WebSliderRelay>("vibratoDepth");
    vibratoTremoloRelay = std::make_unique<juce::WebSliderRelay>("vibratoTremolo");

    // Output
    widthRelay = std::make_unique<juce::WebSliderRelay>("width");
    outputLevelRelay = std::make_unique<juce::WebSliderRelay>("outputLevel");

    // Impossible Physics
    infiniteSustainRelay = std::make_unique<juce::WebSliderRelay>("infiniteSustain");
    reversedJetRelay = std::make_unique<juce::WebSliderRelay>("reversedJet");
    subHarmonicsRelay = std::make_unique<juce::WebSliderRelay>("subHarmonics");

    // Tone Hole Toggle
    toneHoleToggleRelay = std::make_unique<juce::WebToggleButtonRelay>("toneHoleToggle");

    // Instrument Preset (int param as slider relay)
    instrumentPresetRelay = std::make_unique<juce::WebSliderRelay>("instrumentPreset");

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
                            .getChildFile("OWind_WebView"))
                    .withStatusBarDisabled()
                    .withBuiltInErrorPageDisabled())
            .withNativeIntegrationEnabled()
            .withKeepPageLoadedWhenBrowserIsHidden()
            .withResourceProvider([this](const juce::String& url) {
                return getResource(url);
            })
            // Excitation Controls
            .withOptionsFrom(*breathPressureRelay)
            .withOptionsFrom(*embouchureRelay)
            .withOptionsFrom(*breathNoiseRelay)
            // Resonator Controls
            .withOptionsFrom(*materialRelay)
            .withOptionsFrom(*toneColorRelay)
            .withOptionsFrom(*airColumnRelay)
            .withOptionsFrom(*jetReflectionRelay)
            .withOptionsFrom(*endReflectionRelay)
            // Articulation
            .withOptionsFrom(*flutterTongueRelay)
            .withOptionsFrom(*flutterRateRelay)
            // Modulation
            .withOptionsFrom(*vibratoRateRelay)
            .withOptionsFrom(*vibratoDepthRelay)
            .withOptionsFrom(*vibratoTremoloRelay)
            // Output
            .withOptionsFrom(*widthRelay)
            .withOptionsFrom(*outputLevelRelay)
            // Impossible Physics
            .withOptionsFrom(*infiniteSustainRelay)
            .withOptionsFrom(*reversedJetRelay)
            .withOptionsFrom(*subHarmonicsRelay)
            // Tone Hole Toggle
            .withOptionsFrom(*toneHoleToggleRelay)
            // Instrument Preset
            .withOptionsFrom(*instrumentPresetRelay)

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
            // INSTRUMENT PRESET NATIVE FUNCTIONS
            // =============================================================

            .withNativeFunction("getInstrumentPresets", [](auto, auto complete) {
                juce::Array<juce::var> arr;
                for (int i = 0; i < InstrumentPresets::numTotalPresets; ++i)
                    arr.add(juce::String(InstrumentPresets::allPresets[static_cast<size_t>(i)].name));
                complete(juce::var(arr));
            })

            .withNativeFunction("getInstrumentPreset", [this](auto, auto complete) {
                auto* param = processorRef.getAPVTS().getRawParameterValue("instrumentPreset");
                complete(juce::var(static_cast<int>(param->load())));
            })

            .withNativeFunction("setInstrumentPreset", [this](const auto& args, auto complete) {
                if (args.size() < 1)
                {
                    complete(juce::var(false));
                    return;
                }
                int idx = static_cast<int>(args[0]);
                if (idx >= 0 && idx < InstrumentPresets::numTotalPresets)
                {
                    if (auto* param = processorRef.getAPVTS().getParameter("instrumentPreset"))
                        param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(idx)));
                    complete(juce::var(true));
                }
                else
                {
                    complete(juce::var(false));
                }
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
                complete(juce::var());
            })

            .withNativeFunction("generateHarmonicSeries", [](const juce::Array<juce::var>& args, auto complete) {
                if (args.size() >= 2) {
                    int startHarmonic = static_cast<int>(args[0]);
                    int endHarmonic = static_cast<int>(args[1]);
                    auto intervals = ScaleGenerator::generateHarmonicSeries(startHarmonic, endHarmonic);
                    juce::String json = "[";
                    for (size_t i = 0; i < intervals.size(); ++i) {
                        if (i > 0) json += ",";
                        json += juce::String(intervals[i], 6);
                    }
                    json += "]";
                    complete(json);
                    return;
                }
                complete(juce::var());
            })

            .withNativeFunction("applyGeneratedScale", [this](const juce::Array<juce::var>& args, auto complete) {
                if (args.size() >= 2) {
                    auto jsonArray = juce::JSON::parse(args[0].toString());
                    juce::String scaleName = args[1].toString();
                    if (auto* arr = jsonArray.getArray()) {
                        std::vector<double> intervals;
                        for (const auto& val : *arr)
                            intervals.push_back(static_cast<double>(val));
                        processorRef.getTuningEngine()->setCustomIntervals(intervals, scaleName);
                        complete(true);
                        return;
                    }
                }
                complete(false);
            })

            .withNativeFunction("getEmbeddedTuningList", [](const juce::Array<juce::var>&, auto complete) {
                const auto& tunings = EmbeddedTunings::getAllTunings();
                juce::String json = "[";
                for (size_t i = 0; i < tunings.size(); ++i) {
                    if (i > 0) json += ",";
                    json += "{";
                    json += "\"id\":\"" + juce::String(tunings[i].id) + "\",";
                    json += "\"name\":\"" + juce::String(tunings[i].name) + "\",";
                    json += "\"category\":\"" + juce::String(tunings[i].category) + "\",";
                    json += "\"noteCount\":" + juce::String(static_cast<int>(tunings[i].intervals.size()));
                    json += "}";
                }
                json += "]";
                complete(json);
            })

            .withNativeFunction("getEmbeddedTuningCategories", [](const juce::Array<juce::var>&, auto complete) {
                auto categories = EmbeddedTunings::getCategories();
                juce::String json = "[";
                for (size_t i = 0; i < categories.size(); ++i) {
                    if (i > 0) json += ",";
                    json += "\"" + juce::String(categories[i]) + "\"";
                }
                json += "]";
                complete(json);
            })

            .withNativeFunction("loadEmbeddedTuning", [this](const juce::Array<juce::var>& args, auto complete) {
                if (args.size() >= 1) {
                    juce::String tuningId = args[0].toString();
                    auto* tuning = EmbeddedTunings::getTuningById(tuningId.toStdString());
                    if (tuning != nullptr && !tuning->intervals.empty()) {
                        auto intervals = tuning->intervals;
                        intervals.push_back(tuning->period);
                        processorRef.getTuningEngine()->setCustomIntervals(
                            intervals, juce::String(tuning->name));
                        complete(true);
                        return;
                    }
                }
                complete(false);
            })

            .withNativeFunction("exportTuningHTML", [this](const juce::Array<juce::var>&,
                                                              std::function<void(juce::var)> complete) {
                fileChooser = std::make_shared<juce::FileChooser>(
                    "Export Tuning Documentation",
                    juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                        .getChildFile("tuning-export.html"),
                    "*.html");
                fileChooser->launchAsync(
                    juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                    [this, complete](const juce::FileChooser& fc) {
                        auto file = fc.getResult();
                        if (file != juce::File()) {
                            auto html = TuningExporter::toHTML(
                                *processorRef.getTuningEngine(), "O-Wind");
                            file.replaceWithText(html);
                            complete(true);
                        } else {
                            complete(false);
                        }
                    });
            })
    );

    addAndMakeVisible(webView.get());

    // ===================================================================
    // 3. CREATE ATTACHMENTS (must be created AFTER WebView) - 14 slider + 1 toggle + 1 slider
    // ===================================================================
    auto& apvts = processorRef.getAPVTS();

    // Excitation Controls
    breathPressureAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("breathPressure"), *breathPressureRelay, nullptr);
    embouchureAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("embouchure"), *embouchureRelay, nullptr);
    breathNoiseAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("breathNoise"), *breathNoiseRelay, nullptr);

    // Resonator Controls
    materialAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("material"), *materialRelay, nullptr);
    toneColorAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("toneColor"), *toneColorRelay, nullptr);
    airColumnAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("airColumn"), *airColumnRelay, nullptr);
    jetReflectionAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("jetReflection"), *jetReflectionRelay, nullptr);
    endReflectionAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("endReflection"), *endReflectionRelay, nullptr);

    // Articulation
    flutterTongueAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("flutterTongue"), *flutterTongueRelay, nullptr);
    flutterRateAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("flutterRate"), *flutterRateRelay, nullptr);

    // Modulation
    vibratoRateAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("vibratoRate"), *vibratoRateRelay, nullptr);
    vibratoDepthAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("vibratoDepth"), *vibratoDepthRelay, nullptr);
    vibratoTremoloAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("vibratoTremolo"), *vibratoTremoloRelay, nullptr);

    // Output
    widthAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("width"), *widthRelay, nullptr);
    outputLevelAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("outputLevel"), *outputLevelRelay, nullptr);

    // Impossible Physics
    infiniteSustainAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("infiniteSustain"), *infiniteSustainRelay, nullptr);
    reversedJetAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("reversedJet"), *reversedJetRelay, nullptr);
    subHarmonicsAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("subHarmonics"), *subHarmonicsRelay, nullptr);

    // Tone Hole Toggle
    toneHoleToggleAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *apvts.getParameter("toneHoleToggle"), *toneHoleToggleRelay, nullptr);

    // Instrument Preset
    instrumentPresetAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("instrumentPreset"), *instrumentPresetRelay, nullptr);

    // Navigate to embedded UI
    webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());

    // Set window size (900x600)
    setSize(900, 600);
}

OWindAudioProcessorEditor::~OWindAudioProcessorEditor()
{
    // Explicit destruction in reverse order for safety
    instrumentPresetAttachment.reset();
    toneHoleToggleAttachment.reset();
    subHarmonicsAttachment.reset();
    reversedJetAttachment.reset();
    infiniteSustainAttachment.reset();
    outputLevelAttachment.reset();
    widthAttachment.reset();
    vibratoTremoloAttachment.reset();
    vibratoDepthAttachment.reset();
    vibratoRateAttachment.reset();
    flutterRateAttachment.reset();
    flutterTongueAttachment.reset();
    endReflectionAttachment.reset();
    jetReflectionAttachment.reset();
    airColumnAttachment.reset();
    toneColorAttachment.reset();
    materialAttachment.reset();
    breathNoiseAttachment.reset();
    embouchureAttachment.reset();
    breathPressureAttachment.reset();
    webView.reset();
}

void OWindAudioProcessorEditor::paint(juce::Graphics& g)
{
    juce::ignoreUnused(g);  // WebView handles all painting
}

void OWindAudioProcessorEditor::resized()
{
    if (webView)
        webView->setBounds(getLocalBounds());
}

//==============================================================================
// Resource Provider - serves embedded UI files from BinaryData
//==============================================================================
std::optional<juce::WebBrowserComponent::Resource>
OWindAudioProcessorEditor::getResource(const juce::String& url)
{
    auto makeVector = [](const char* data, int size) {
        return std::vector<std::byte>(
            reinterpret_cast<const std::byte*>(data),
            reinterpret_cast<const std::byte*>(data) + size);
    };

    // HTML
    if (url == "/" || url == "/index.html")
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::index_html, BinaryData::index_htmlSize),
            juce::String("text/html") };

    // JUCE Bridge
    if (url == "/js/juce/index.js")
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::index_js, BinaryData::index_jsSize),
            juce::String("application/javascript") };

    if (url == "/js/juce/check_native_interop.js")
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::check_native_interop_js, BinaryData::check_native_interop_jsSize),
            juce::String("application/javascript") };

    // Tuning panel
    if (url == "/js/tuning-panel.js")
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::tuningpanel_js, BinaryData::tuningpanel_jsSize),
            juce::String("application/javascript") };

    if (url == "/css/tuning-panel.css")
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::tuningpanel_css, BinaryData::tuningpanel_cssSize),
            juce::String("text/css") };

    // Fern botanical image
    if (url == "/img/fern.png")
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::fern_png, BinaryData::fern_pngSize),
            juce::String("image/png") };

    DBG("Resource not found: " + url);
    return std::nullopt;
}
