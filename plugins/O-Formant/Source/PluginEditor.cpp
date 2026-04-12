#include "PluginEditor.h"
#include "BinaryData.h"
#include "EmbeddedTunings.h"

OFormantEditor::OFormantEditor (OFormantAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    // 1. Create relays (names MUST match JS relay names)
    vowelXRelay        = std::make_unique<juce::WebSliderRelay> ("vowelXSlider");
    vowelYRelay        = std::make_unique<juce::WebSliderRelay> ("vowelYSlider");
    vowelFocusRelay    = std::make_unique<juce::WebSliderRelay> ("vowelFocusSlider");
    glottalRdRelay     = std::make_unique<juce::WebSliderRelay> ("glottalRdSlider");
    breathinessRelay   = std::make_unique<juce::WebSliderRelay> ("breathinessSlider");
    vibratoRateRelay   = std::make_unique<juce::WebSliderRelay> ("vibratoRateSlider");
    vibratoDepthRelay  = std::make_unique<juce::WebSliderRelay> ("vibratoDepthSlider");
    vibratoDelayRelay  = std::make_unique<juce::WebSliderRelay> ("vibratoDelaySlider");
    jitterRelay        = std::make_unique<juce::WebSliderRelay> ("jitterSlider");
    shimmerRelay       = std::make_unique<juce::WebSliderRelay> ("shimmerSlider");
    rdModDepthRelay    = std::make_unique<juce::WebSliderRelay> ("rdModDepthSlider");
    spectralTiltRelay  = std::make_unique<juce::WebSliderRelay> ("spectralTiltSlider");
    consonantLevelRelay = std::make_unique<juce::WebSliderRelay> ("consonantLevelSlider");
    consonantToneRelay = std::make_unique<juce::WebSliderRelay> ("consonantToneSlider");
    sibilanceRelay     = std::make_unique<juce::WebSliderRelay> ("sibilanceSlider");
    autoConsonantRelay = std::make_unique<juce::WebToggleButtonRelay> ("autoConsonantToggle");
    attackRelay        = std::make_unique<juce::WebSliderRelay> ("attackSlider");
    decayRelay         = std::make_unique<juce::WebSliderRelay> ("decaySlider");
    sustainRelay       = std::make_unique<juce::WebSliderRelay> ("sustainSlider");
    releaseRelay       = std::make_unique<juce::WebSliderRelay> ("releaseSlider");
    formantTopologyRelay = std::make_unique<juce::WebComboBoxRelay> ("formantTopologyComboBox");
    formantShiftRelay  = std::make_unique<juce::WebSliderRelay> ("formantShiftSlider");
    formantSpreadRelay = std::make_unique<juce::WebSliderRelay> ("formantSpreadSlider");
    pitchGlideRelay    = std::make_unique<juce::WebSliderRelay> ("pitchGlideSlider");
    transitionTimeRelay = std::make_unique<juce::WebSliderRelay> ("transitionTimeSlider");
    singersFormantRelay = std::make_unique<juce::WebSliderRelay> ("singersFormantSlider");
    nasalCouplingRelay = std::make_unique<juce::WebSliderRelay> ("nasalCouplingSlider");
    nasalPlaceRelay    = std::make_unique<juce::WebSliderRelay> ("nasalPlaceSlider");
    outputGainRelay    = std::make_unique<juce::WebSliderRelay> ("outputGainSlider");
    stereoWidthRelay   = std::make_unique<juce::WebSliderRelay> ("stereoWidthSlider");

    // Effects relays
    chorusBypassRelay  = std::make_unique<juce::WebToggleButtonRelay> ("chorusBypassToggle");
    chorusRateRelay    = std::make_unique<juce::WebSliderRelay> ("chorusRateSlider");
    chorusDepthRelay   = std::make_unique<juce::WebSliderRelay> ("chorusDepthSlider");
    chorusMixRelay     = std::make_unique<juce::WebSliderRelay> ("chorusMixSlider");
    delayBypassRelay   = std::make_unique<juce::WebToggleButtonRelay> ("delayBypassToggle");
    delayTimeRelay     = std::make_unique<juce::WebSliderRelay> ("delayTimeSlider");
    delayFeedbackRelay = std::make_unique<juce::WebSliderRelay> ("delayFeedbackSlider");
    delayModeRelay     = std::make_unique<juce::WebComboBoxRelay> ("delayModeComboBox");
    delayMixRelay      = std::make_unique<juce::WebSliderRelay> ("delayMixSlider");
    reverbBypassRelay  = std::make_unique<juce::WebToggleButtonRelay> ("reverbBypassToggle");
    reverbSizeRelay    = std::make_unique<juce::WebSliderRelay> ("reverbSizeSlider");
    reverbDampRelay    = std::make_unique<juce::WebSliderRelay> ("reverbDampSlider");
    reverbPredelayRelay = std::make_unique<juce::WebSliderRelay> ("reverbPredelaySlider");
    reverbMixRelay     = std::make_unique<juce::WebSliderRelay> ("reverbMixSlider");
    reverbModRelay     = std::make_unique<juce::WebSliderRelay> ("reverbModSlider");
    reverbShimmerRelay = std::make_unique<juce::WebSliderRelay> ("reverbShimmerSlider");
    eqBypassRelay      = std::make_unique<juce::WebToggleButtonRelay> ("eqBypassToggle");
    eqLowGainRelay     = std::make_unique<juce::WebSliderRelay> ("eqLowGainSlider");
    eqMidGainRelay     = std::make_unique<juce::WebSliderRelay> ("eqMidGainSlider");
    eqMidFreqRelay     = std::make_unique<juce::WebSliderRelay> ("eqMidFreqSlider");
    eqHighGainRelay    = std::make_unique<juce::WebSliderRelay> ("eqHighGainSlider");

    // 2. Build WebView options and create component
    webView = std::make_unique<juce::WebBrowserComponent> (
        juce::WebBrowserComponent::Options{}
            .withBackend (juce::WebBrowserComponent::Options::Backend::webview2)
            .withWinWebView2Options (
                juce::WebBrowserComponent::Options::WinWebView2{}
                    .withUserDataFolder (juce::File::getSpecialLocation (
                        juce::File::SpecialLocationType::tempDirectory)
                            .getChildFile ("OFormant_WebView"))
                    .withStatusBarDisabled()
                    .withBuiltInErrorPageDisabled())
            .withNativeIntegrationEnabled()
            .withKeepPageLoadedWhenBrowserIsHidden()
#if JUCE_WEB_BROWSER_RESOURCE_PROVIDER_AVAILABLE
            .withResourceProvider ([this] (const auto& url) { return getResource (url); })
#endif
            .withOptionsFrom (*vowelXRelay)
            .withOptionsFrom (*vowelYRelay)
            .withOptionsFrom (*vowelFocusRelay)
            .withOptionsFrom (*glottalRdRelay)
            .withOptionsFrom (*breathinessRelay)
            .withOptionsFrom (*vibratoRateRelay)
            .withOptionsFrom (*vibratoDepthRelay)
            .withOptionsFrom (*vibratoDelayRelay)
            .withOptionsFrom (*jitterRelay)
            .withOptionsFrom (*shimmerRelay)
            .withOptionsFrom (*rdModDepthRelay)
            .withOptionsFrom (*spectralTiltRelay)
            .withOptionsFrom (*consonantLevelRelay)
            .withOptionsFrom (*consonantToneRelay)
            .withOptionsFrom (*sibilanceRelay)
            .withOptionsFrom (*autoConsonantRelay)
            .withOptionsFrom (*attackRelay)
            .withOptionsFrom (*decayRelay)
            .withOptionsFrom (*sustainRelay)
            .withOptionsFrom (*releaseRelay)
            .withOptionsFrom (*formantTopologyRelay)
            .withOptionsFrom (*formantShiftRelay)
            .withOptionsFrom (*formantSpreadRelay)
            .withOptionsFrom (*pitchGlideRelay)
            .withOptionsFrom (*transitionTimeRelay)
            .withOptionsFrom (*singersFormantRelay)
            .withOptionsFrom (*nasalCouplingRelay)
            .withOptionsFrom (*nasalPlaceRelay)
            .withOptionsFrom (*outputGainRelay)
            .withOptionsFrom (*stereoWidthRelay)
            .withOptionsFrom (*chorusBypassRelay)
            .withOptionsFrom (*chorusRateRelay)
            .withOptionsFrom (*chorusDepthRelay)
            .withOptionsFrom (*chorusMixRelay)
            .withOptionsFrom (*delayBypassRelay)
            .withOptionsFrom (*delayTimeRelay)
            .withOptionsFrom (*delayFeedbackRelay)
            .withOptionsFrom (*delayModeRelay)
            .withOptionsFrom (*delayMixRelay)
            .withOptionsFrom (*reverbBypassRelay)
            .withOptionsFrom (*reverbSizeRelay)
            .withOptionsFrom (*reverbDampRelay)
            .withOptionsFrom (*reverbPredelayRelay)
            .withOptionsFrom (*reverbMixRelay)
            .withOptionsFrom (*reverbModRelay)
            .withOptionsFrom (*reverbShimmerRelay)
            .withOptionsFrom (*eqBypassRelay)
            .withOptionsFrom (*eqLowGainRelay)
            .withOptionsFrom (*eqMidGainRelay)
            .withOptionsFrom (*eqMidFreqRelay)
            .withOptionsFrom (*eqHighGainRelay)

            // ── Preset Native Functions ──

            .withNativeFunction ("getPresetList", [this] (auto, auto complete) {
                auto& pm = processorRef.getPresetManager();
                auto presets = pm.getPresetList();
                juce::Array<juce::var> arr;
                for (const auto& name : presets)
                    arr.add (name);
                complete (juce::var (arr));
            })

            .withNativeFunction ("getPresetListWithCategories", [this] (auto, auto complete) {
                auto& pm = processorRef.getPresetManager();
                auto categorized = pm.getPresetListWithCategories();
                auto* obj = new juce::DynamicObject();
                for (const auto& [category, presets] : categorized)
                {
                    juce::Array<juce::var> arr;
                    for (const auto& name : presets)
                        arr.add (name);
                    obj->setProperty (category, juce::var (arr));
                }
                complete (juce::var (obj));
            })

            .withNativeFunction ("getCurrentPreset", [this] (auto, auto complete) {
                auto& pm = processorRef.getPresetManager();
                complete (juce::var (pm.getCurrentPresetName()));
            })

            .withNativeFunction ("loadPreset", [this] (const auto& args, auto complete) {
                if (args.size() < 1 || ! args[0].isString())
                {
                    complete (juce::var (false));
                    return;
                }
                auto& pm = processorRef.getPresetManager();
                bool success = pm.loadPreset (args[0].toString());
                complete (juce::var (success));
            })

            .withNativeFunction ("loadPresetFromCategory", [this] (const auto& args, auto complete) {
                if (args.size() < 2 || ! args[0].isString() || ! args[1].isString())
                {
                    complete (juce::var (false));
                    return;
                }
                auto& pm = processorRef.getPresetManager();
                bool success = pm.loadPresetFromCategory (args[0].toString(), args[1].toString());
                complete (juce::var (success));
            })

            .withNativeFunction ("savePreset", [this] (const auto& args, auto complete) {
                if (args.size() < 1 || ! args[0].isString())
                {
                    complete (juce::var (false));
                    return;
                }
                auto& pm = processorRef.getPresetManager();
                bool success = pm.savePreset (args[0].toString());
                complete (juce::var (success));
            })

            .withNativeFunction ("selectNextPreset", [this] (auto, auto complete) {
                auto& pm = processorRef.getPresetManager();
                auto name = pm.getNextPreset();
                complete (juce::var (name));
            })

            .withNativeFunction ("selectPreviousPreset", [this] (auto, auto complete) {
                auto& pm = processorRef.getPresetManager();
                auto name = pm.getPreviousPreset();
                complete (juce::var (name));
            })

            .withNativeFunction ("deletePreset", [this] (const auto& args, auto complete) {
                if (args.size() < 1 || ! args[0].isString())
                {
                    complete (juce::var (false));
                    return;
                }
                auto& pm = processorRef.getPresetManager();
                bool success = pm.deletePreset (args[0].toString());
                complete (juce::var (success));
            })

            .withNativeFunction ("isFactoryPreset", [this] (const auto& args, auto complete) {
                if (args.size() < 1 || ! args[0].isString())
                {
                    complete (juce::var (false));
                    return;
                }
                auto& pm = processorRef.getPresetManager();
                bool factory = pm.isFactoryPreset (args[0].toString());
                complete (juce::var (factory));
            })

            // ── Tuning Native Functions ──

            .withNativeFunction ("getTuningIntervals", [this] (auto, auto complete) {
                auto intervals = processorRef.tuningEngine.getIntervals();
                juce::String json = "[";
                for (size_t i = 0; i < intervals.size(); ++i)
                {
                    if (i > 0) json += ",";
                    json += juce::String (intervals[i], 6);
                }
                json += "]";
                complete (juce::var (json));
            })

            .withNativeFunction ("setTuningIntervals", [this] (const auto& args, auto complete) {
                if (args.size() >= 1)
                {
                    auto jsonArray = juce::JSON::parse (args[0].toString());
                    if (auto* arr = jsonArray.getArray())
                    {
                        std::vector<double> intervals;
                        for (const auto& val : *arr)
                            intervals.push_back (static_cast<double> (val));
                        processorRef.tuningEngine.setCustomIntervals (intervals, "Custom");
                        complete (juce::var (true));
                        return;
                    }
                }
                complete (juce::var (false));
            })

            .withNativeFunction ("getTuningName", [this] (auto, auto complete) {
                complete (juce::var (processorRef.tuningEngine.getActiveTuningName()));
            })

            .withNativeFunction ("setSingleInterval", [this] (const auto& args, auto complete) {
                if (args.size() >= 2)
                {
                    int index = static_cast<int> (args[0]);
                    double cents = static_cast<double> (args[1]);
                    processorRef.tuningEngine.setSingleInterval (index, cents);
                    complete (juce::var (true));
                    return;
                }
                complete (juce::var (false));
            })

            .withNativeFunction ("setSingleIntervalEncoded", [this] (const auto& args, auto complete) {
                if (args.size() >= 2)
                {
                    int index = static_cast<int> (args[0]);
                    double cents = static_cast<double> (args[1]);
                    processorRef.tuningEngine.setSingleInterval (index, cents);
                    complete (juce::var (true));
                    return;
                }
                complete (juce::var (false));
            })

            .withNativeFunction ("setTonicNote", [this] (const auto& args, auto complete) {
                if (args.size() >= 1)
                {
                    int tonic = static_cast<int> (args[0]);
                    processorRef.tuningEngine.setTonicNote (tonic);
                    complete (juce::var (true));
                    return;
                }
                complete (juce::var (false));
            })

            .withNativeFunction ("getTonicNote", [this] (auto, auto complete) {
                complete (juce::var (processorRef.tuningEngine.getTonicNote()));
            })

            .withNativeFunction ("getOctaveStretch", [this] (auto, auto complete) {
                complete (juce::var (processorRef.tuningEngine.getOctaveStretch()));
            })

            .withNativeFunction ("setOctaveStretch", [this] (const auto& args, auto complete) {
                if (args.size() >= 1)
                {
                    float stretch = static_cast<float> (args[0]);
                    processorRef.tuningEngine.setOctaveStretch (stretch);
                    complete (juce::var (true));
                    return;
                }
                complete (juce::var (false));
            })

            .withNativeFunction ("getMasterTune", [this] (auto, auto complete) {
                complete (juce::var (processorRef.tuningEngine.getMasterTune()));
            })

            .withNativeFunction ("setMasterTune", [this] (const auto& args, auto complete) {
                if (args.size() >= 1)
                {
                    double hz = static_cast<double> (args[0]);
                    processorRef.tuningEngine.setMasterTune (hz);
                    complete (juce::var (true));
                    return;
                }
                complete (juce::var (false));
            })

            .withNativeFunction ("setTemperamentPreset", [this] (const auto& args, auto complete) {
                if (args.size() >= 1)
                {
                    int preset = static_cast<int> (args[0]);
                    processorRef.tuningEngine.setBuiltInPreset (
                        static_cast<TuningEngine::BuiltInPreset> (preset));
                    complete (juce::var (true));
                    return;
                }
                complete (juce::var (false));
            })

            .withNativeFunction ("getTemperamentPreset", [this] (auto, auto complete) {
                complete (juce::var (static_cast<int> (processorRef.tuningEngine.getBuiltInPreset())));
            })

            .withNativeFunction ("loadScalaFile", [this] (auto, auto complete) {
                fileChooser = std::make_shared<juce::FileChooser> (
                    "Load Scala File",
                    juce::File::getSpecialLocation (juce::File::userDocumentsDirectory),
                    "*.scl");
                fileChooser->launchAsync (
                    juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                    [this, complete] (const juce::FileChooser& fc) {
                        auto file = fc.getResult();
                        if (file.existsAsFile())
                        {
                            bool success = processorRef.tuningEngine.loadScalaFile (file);
                            complete (success ? juce::var (processorRef.tuningEngine.getActiveTuningName())
                                              : juce::var());
                        }
                        else
                        {
                            complete (juce::var());
                        }
                    });
            })

            .withNativeFunction ("saveScalaFile", [this] (auto, auto complete) {
                fileChooser = std::make_shared<juce::FileChooser> (
                    "Save Scala File",
                    juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                        .getChildFile ("scale.scl"),
                    "*.scl");
                fileChooser->launchAsync (
                    juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                    [this, complete] (const juce::FileChooser& fc) {
                        auto file = fc.getResult();
                        if (file != juce::File())
                        {
                            auto content = processorRef.tuningEngine.generateScalaFileContent();
                            file.replaceWithText (content);
                            complete (juce::var (true));
                        }
                        else
                        {
                            complete (juce::var (false));
                        }
                    });
            })

            .withNativeFunction ("loadKBMFile", [this] (auto, auto complete) {
                fileChooser = std::make_shared<juce::FileChooser> (
                    "Load Keyboard Mapping",
                    juce::File::getSpecialLocation (juce::File::userDocumentsDirectory),
                    "*.kbm");
                fileChooser->launchAsync (
                    juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                    [this, complete] (const juce::FileChooser& fc) {
                        auto file = fc.getResult();
                        if (file.existsAsFile())
                        {
                            bool success = processorRef.tuningEngine.loadKBMFile (file);
                            complete (juce::var (success));
                        }
                        else
                        {
                            complete (juce::var (false));
                        }
                    });
            })

            .withNativeFunction ("saveKBMFile", [this] (auto, auto complete) {
                fileChooser = std::make_shared<juce::FileChooser> (
                    "Save Keyboard Mapping",
                    juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                        .getChildFile ("mapping.kbm"),
                    "*.kbm");
                fileChooser->launchAsync (
                    juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                    [this, complete] (const juce::FileChooser& fc) {
                        auto file = fc.getResult();
                        if (file != juce::File())
                        {
                            auto content = processorRef.tuningEngine.generateKBMFileContent();
                            file.replaceWithText (content);
                            complete (juce::var (true));
                        }
                        else
                        {
                            complete (juce::var (false));
                        }
                    });
            })

            .withNativeFunction ("generateEDO", [this] (const auto& args, auto complete) {
                if (args.size() >= 2)
                {
                    int divisions = static_cast<int> (args[0]);
                    double period = static_cast<double> (args[1]);
                    auto intervals = processorRef.scaleGenerator.generateEDO (divisions, period);
                    juce::String json = "[";
                    for (size_t i = 0; i < intervals.size(); ++i)
                    {
                        if (i > 0) json += ",";
                        json += juce::String (intervals[i], 6);
                    }
                    json += "]";
                    complete (juce::var (json));
                    return;
                }
                complete (juce::var());
            })

            .withNativeFunction ("generateHarmonicSeries", [this] (const auto& args, auto complete) {
                if (args.size() >= 2)
                {
                    int startHarmonic = static_cast<int> (args[0]);
                    int endHarmonic = static_cast<int> (args[1]);
                    auto intervals = processorRef.scaleGenerator.generateHarmonicSeries (
                        startHarmonic, endHarmonic);
                    juce::String json = "[";
                    for (size_t i = 0; i < intervals.size(); ++i)
                    {
                        if (i > 0) json += ",";
                        json += juce::String (intervals[i], 6);
                    }
                    json += "]";
                    complete (juce::var (json));
                    return;
                }
                complete (juce::var());
            })

            .withNativeFunction ("generateRank2", [this] (const auto& args, auto complete) {
                if (args.size() >= 3)
                {
                    double generator = static_cast<double> (args[0]);
                    double period = static_cast<double> (args[1]);
                    int count = static_cast<int> (args[2]);
                    auto intervals = processorRef.scaleGenerator.generateRank2 (
                        generator, period, count);
                    juce::String json = "[";
                    for (size_t i = 0; i < intervals.size(); ++i)
                    {
                        if (i > 0) json += ",";
                        json += juce::String (intervals[i], 6);
                    }
                    json += "]";
                    complete (juce::var (json));
                    return;
                }
                complete (juce::var());
            })

            .withNativeFunction ("applyGeneratedScale", [this] (const auto& args, auto complete) {
                if (args.size() >= 2)
                {
                    auto jsonArray = juce::JSON::parse (args[0].toString());
                    juce::String scaleName = args[1].toString();
                    if (auto* arr = jsonArray.getArray())
                    {
                        std::vector<double> intervals;
                        for (const auto& val : *arr)
                            intervals.push_back (static_cast<double> (val));
                        processorRef.tuningEngine.setCustomIntervals (intervals, scaleName);
                        complete (juce::var (true));
                        return;
                    }
                }
                complete (juce::var (false));
            })

            .withNativeFunction ("getEmbeddedTuningList", [this] (auto, auto complete) {
                auto& tunings = EmbeddedTunings::getAllTunings();
                juce::String json = "[";
                for (size_t i = 0; i < tunings.size(); ++i)
                {
                    if (i > 0) json += ",";
                    json += "{";
                    json += "\"id\":\"" + juce::String (tunings[i].id) + "\",";
                    json += "\"name\":\"" + juce::String (tunings[i].name) + "\",";
                    json += "\"category\":\"" + juce::String (tunings[i].category) + "\",";
                    json += "\"noteCount\":" + juce::String (static_cast<int> (tunings[i].intervals.size()));
                    json += "}";
                }
                json += "]";
                complete (juce::var (json));
            })

            .withNativeFunction ("getEmbeddedTuningCategories", [this] (auto, auto complete) {
                auto categories = EmbeddedTunings::getCategories();
                juce::String json = "[";
                for (size_t i = 0; i < categories.size(); ++i)
                {
                    if (i > 0) json += ",";
                    json += "\"" + juce::String (categories[i]) + "\"";
                }
                json += "]";
                complete (juce::var (json));
            })

            .withNativeFunction ("loadEmbeddedTuning", [this] (const auto& args, auto complete) {
                if (args.size() >= 1)
                {
                    auto tuningId = args[0].toString().toStdString();
                    auto* tuningData = EmbeddedTunings::getTuningById (tuningId);
                    if (tuningData != nullptr && ! tuningData->intervals.empty())
                    {
                        processorRef.tuningEngine.setCustomIntervals (
                            tuningData->intervals, tuningData->name);
                        complete (juce::var (true));
                        return;
                    }
                }
                complete (juce::var (false));
            })

            .withNativeFunction ("exportTuningHTML", [this] (auto, auto complete) {
                fileChooser = std::make_shared<juce::FileChooser> (
                    "Export Tuning Documentation",
                    juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                        .getChildFile ("tuning-export.html"),
                    "*.html");
                fileChooser->launchAsync (
                    juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                    [this, complete] (const juce::FileChooser& fc) {
                        auto file = fc.getResult();
                        if (file != juce::File())
                        {
                            auto html = TuningExporter::toHTML (processorRef.tuningEngine, "O-Formant");
                            file.replaceWithText (html);
                            complete (juce::var (true));
                        }
                        else
                        {
                            complete (juce::var (false));
                        }
                    });
            })
    );

    addAndMakeVisible (*webView);

    // 3. Create attachments (link APVTS parameter -> relay)
    auto& apvts = processorRef.getAPVTS();

    vowelXAttachment        = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter ("vowelX"), *vowelXRelay);
    vowelYAttachment        = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter ("vowelY"), *vowelYRelay);
    vowelFocusAttachment    = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter ("vowelFocus"), *vowelFocusRelay);
    glottalRdAttachment     = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter ("glottalRd"), *glottalRdRelay);
    breathinessAttachment   = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter ("breathiness"), *breathinessRelay);
    vibratoRateAttachment   = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter ("vibratoRate"), *vibratoRateRelay);
    vibratoDepthAttachment  = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter ("vibratoDepth"), *vibratoDepthRelay);
    vibratoDelayAttachment  = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter ("vibratoDelay"), *vibratoDelayRelay);
    jitterAttachment        = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter ("jitter"), *jitterRelay);
    shimmerAttachment       = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter ("shimmer"), *shimmerRelay);
    rdModDepthAttachment    = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter ("rdModDepth"), *rdModDepthRelay);
    spectralTiltAttachment  = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter ("spectralTilt"), *spectralTiltRelay);
    consonantLevelAttachment = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter ("consonantLevel"), *consonantLevelRelay);
    consonantToneAttachment = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter ("consonantTone"), *consonantToneRelay);
    sibilanceAttachment     = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter ("sibilance"), *sibilanceRelay);
    autoConsonantAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment> (*apvts.getParameter ("autoConsonant"), *autoConsonantRelay);
    attackAttachment        = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter ("attack"), *attackRelay);
    decayAttachment         = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter ("decay"), *decayRelay);
    sustainAttachment       = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter ("sustain"), *sustainRelay);
    releaseAttachment       = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter ("release"), *releaseRelay);
    formantTopologyAttachment = std::make_unique<juce::WebComboBoxParameterAttachment> (*apvts.getParameter ("formantTopology"), *formantTopologyRelay);
    formantShiftAttachment  = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter ("formantShift"), *formantShiftRelay);
    formantSpreadAttachment = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter ("formantSpread"), *formantSpreadRelay);
    pitchGlideAttachment    = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter ("pitchGlide"), *pitchGlideRelay);
    transitionTimeAttachment = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter ("transitionTime"), *transitionTimeRelay);
    singersFormantAttachment = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter ("singersFormant"), *singersFormantRelay);
    nasalCouplingAttachment = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter ("nasalCoupling"), *nasalCouplingRelay);
    nasalPlaceAttachment    = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter ("nasalPlace"), *nasalPlaceRelay);
    outputGainAttachment    = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter ("outputGain"), *outputGainRelay);
    stereoWidthAttachment   = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter ("stereoWidth"), *stereoWidthRelay);

    // Effects attachments
    chorusBypassAttachment  = std::make_unique<juce::WebToggleButtonParameterAttachment> (*apvts.getParameter ("chorusBypass"), *chorusBypassRelay);
    chorusRateAttachment    = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter ("chorusRate"), *chorusRateRelay);
    chorusDepthAttachment   = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter ("chorusDepth"), *chorusDepthRelay);
    chorusMixAttachment     = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter ("chorusMix"), *chorusMixRelay);
    delayBypassAttachment   = std::make_unique<juce::WebToggleButtonParameterAttachment> (*apvts.getParameter ("delayBypass"), *delayBypassRelay);
    delayTimeAttachment     = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter ("delayTime"), *delayTimeRelay);
    delayFeedbackAttachment = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter ("delayFeedback"), *delayFeedbackRelay);
    delayModeAttachment     = std::make_unique<juce::WebComboBoxParameterAttachment> (*apvts.getParameter ("delayMode"), *delayModeRelay);
    delayMixAttachment      = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter ("delayMix"), *delayMixRelay);
    reverbBypassAttachment  = std::make_unique<juce::WebToggleButtonParameterAttachment> (*apvts.getParameter ("reverbBypass"), *reverbBypassRelay);
    reverbSizeAttachment    = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter ("reverbSize"), *reverbSizeRelay);
    reverbDampAttachment    = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter ("reverbDamp"), *reverbDampRelay);
    reverbPredelayAttachment = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter ("reverbPredelay"), *reverbPredelayRelay);
    reverbMixAttachment     = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter ("reverbMix"), *reverbMixRelay);
    reverbModAttachment     = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter ("reverbMod"), *reverbModRelay);
    reverbShimmerAttachment = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter ("reverbShimmer"), *reverbShimmerRelay);
    eqBypassAttachment      = std::make_unique<juce::WebToggleButtonParameterAttachment> (*apvts.getParameter ("eqBypass"), *eqBypassRelay);
    eqLowGainAttachment     = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter ("eqLowGain"), *eqLowGainRelay);
    eqMidGainAttachment     = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter ("eqMidGain"), *eqMidGainRelay);
    eqMidFreqAttachment     = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter ("eqMidFreq"), *eqMidFreqRelay);
    eqHighGainAttachment    = std::make_unique<juce::WebSliderParameterAttachment> (*apvts.getParameter ("eqHighGain"), *eqHighGainRelay);

#if JUCE_WEB_BROWSER_RESOURCE_PROVIDER_AVAILABLE
    webView->goToURL (juce::WebBrowserComponent::getResourceProviderRoot());
#endif

    setSize (800, 600);
}

OFormantEditor::~OFormantEditor()
{
    // Destroy in reverse: attachments first, then webView, then relays
    eqHighGainAttachment.reset();
    eqMidFreqAttachment.reset();
    eqMidGainAttachment.reset();
    eqLowGainAttachment.reset();
    eqBypassAttachment.reset();
    reverbShimmerAttachment.reset();
    reverbModAttachment.reset();
    reverbMixAttachment.reset();
    reverbPredelayAttachment.reset();
    reverbDampAttachment.reset();
    reverbSizeAttachment.reset();
    reverbBypassAttachment.reset();
    delayMixAttachment.reset();
    delayModeAttachment.reset();
    delayFeedbackAttachment.reset();
    delayTimeAttachment.reset();
    delayBypassAttachment.reset();
    chorusMixAttachment.reset();
    chorusDepthAttachment.reset();
    chorusRateAttachment.reset();
    chorusBypassAttachment.reset();
    stereoWidthAttachment.reset();
    outputGainAttachment.reset();
    singersFormantAttachment.reset();
    transitionTimeAttachment.reset();
    pitchGlideAttachment.reset();
    formantSpreadAttachment.reset();
    formantTopologyAttachment.reset();
    formantShiftAttachment.reset();
    releaseAttachment.reset();
    sustainAttachment.reset();
    decayAttachment.reset();
    attackAttachment.reset();
    autoConsonantAttachment.reset();
    sibilanceAttachment.reset();
    consonantToneAttachment.reset();
    consonantLevelAttachment.reset();
    vibratoDelayAttachment.reset();
    spectralTiltAttachment.reset();
    rdModDepthAttachment.reset();
    shimmerAttachment.reset();
    jitterAttachment.reset();
    vibratoDepthAttachment.reset();
    vibratoRateAttachment.reset();
    breathinessAttachment.reset();
    glottalRdAttachment.reset();
    vowelFocusAttachment.reset();
    vowelYAttachment.reset();
    vowelXAttachment.reset();

    webView.reset();

    eqHighGainRelay.reset();
    eqMidFreqRelay.reset();
    eqMidGainRelay.reset();
    eqLowGainRelay.reset();
    eqBypassRelay.reset();
    reverbShimmerRelay.reset();
    reverbModRelay.reset();
    reverbMixRelay.reset();
    reverbPredelayRelay.reset();
    reverbDampRelay.reset();
    reverbSizeRelay.reset();
    reverbBypassRelay.reset();
    delayMixRelay.reset();
    delayModeRelay.reset();
    delayFeedbackRelay.reset();
    delayTimeRelay.reset();
    delayBypassRelay.reset();
    chorusMixRelay.reset();
    chorusDepthRelay.reset();
    chorusRateRelay.reset();
    chorusBypassRelay.reset();
    stereoWidthRelay.reset();
    outputGainRelay.reset();
    singersFormantRelay.reset();
    transitionTimeRelay.reset();
    pitchGlideRelay.reset();
    formantSpreadRelay.reset();
    formantTopologyRelay.reset();
    formantShiftRelay.reset();
    releaseRelay.reset();
    sustainRelay.reset();
    decayRelay.reset();
    attackRelay.reset();
    autoConsonantRelay.reset();
    sibilanceRelay.reset();
    consonantToneRelay.reset();
    consonantLevelRelay.reset();
    vibratoDelayRelay.reset();
    spectralTiltRelay.reset();
    rdModDepthRelay.reset();
    shimmerRelay.reset();
    jitterRelay.reset();
    vibratoDepthRelay.reset();
    vibratoRateRelay.reset();
    breathinessRelay.reset();
    glottalRdRelay.reset();
    vowelFocusRelay.reset();
    vowelYRelay.reset();
    vowelXRelay.reset();
}

void OFormantEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xffF5E6D3));
}

void OFormantEditor::resized()
{
    webView->setBounds (getLocalBounds());
}

std::optional<juce::WebBrowserComponent::Resource>
OFormantEditor::getResource (const juce::String& url)
{
    auto makeVector = [] (const char* data, int size) {
        return std::vector<std::byte> (
            reinterpret_cast<const std::byte*> (data),
            reinterpret_cast<const std::byte*> (data) + size);
    };

    if (url == "/" || url == "/index.html")
    {
        return juce::WebBrowserComponent::Resource {
            makeVector (BinaryData::index_html, BinaryData::index_htmlSize),
            juce::String ("text/html") };
    }

    if (url == "/js/juce/index.js")
    {
        return juce::WebBrowserComponent::Resource {
            makeVector (BinaryData::index_js, BinaryData::index_jsSize),
            juce::String ("application/javascript") };
    }

    if (url == "/js/juce/check_native_interop.js")
    {
        return juce::WebBrowserComponent::Resource {
            makeVector (BinaryData::check_native_interop_js, BinaryData::check_native_interop_jsSize),
            juce::String ("application/javascript") };
    }

    if (url == "/js/main.js")
    {
        return juce::WebBrowserComponent::Resource {
            makeVector (BinaryData::main_js, BinaryData::main_jsSize),
            juce::String ("application/javascript") };
    }

    if (url == "/js/tuning-panel.js")
    {
        return juce::WebBrowserComponent::Resource {
            makeVector (BinaryData::tuningpanel_js, BinaryData::tuningpanel_jsSize),
            juce::String ("application/javascript") };
    }

    if (url == "/img/flora.png")
    {
        return juce::WebBrowserComponent::Resource {
            makeVector (BinaryData::flora_png, BinaryData::flora_pngSize),
            juce::String ("image/png") };
    }

    juce::Logger::writeToLog ("Resource not found: " + url);
    return std::nullopt;
}
