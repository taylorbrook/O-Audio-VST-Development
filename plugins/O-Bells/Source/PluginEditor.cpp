/*
  ==============================================================================

    O-Bells - Editor Implementation
    Ouaricon Development
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginEditor.h"
#include "BinaryData.h"
#include "TuningEngine.h"
#include "ScaleGenerator.h"
#include "TuningExporter.h"
#include "EmbeddedTunings.h"

OBellsAudioProcessorEditor::OBellsAudioProcessorEditor(OBellsAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    // 1️⃣ CREATE RELAYS FIRST
    strikePositionRelay = std::make_unique<juce::WebSliderRelay>("strikePosition");
    malletHardnessRelay = std::make_unique<juce::WebSliderRelay>("malletHardness");
    dampingRelay = std::make_unique<juce::WebSliderRelay>("damping");
    overtoneBrightnessRelay = std::make_unique<juce::WebSliderRelay>("overtoneBrightness");
    acousticBrightnessRelay = std::make_unique<juce::WebSliderRelay>("acousticBrightness");
    airAbsorptionRelay = std::make_unique<juce::WebSliderRelay>("airAbsorption");
    airAbsorptionTimeRelay = std::make_unique<juce::WebSliderRelay>("airAbsorptionTime");
    inharmonicityRelay = std::make_unique<juce::WebSliderRelay>("inharmonicity");
    bloomSpeedRelay = std::make_unique<juce::WebSliderRelay>("bloomSpeed");
    bloomAmountRelay = std::make_unique<juce::WebSliderRelay>("bloomAmount");
    // v1.5.0: Bloom fine controls (per-band)
    bloomFineEnabledRelay = std::make_unique<juce::WebSliderRelay>("bloomFineEnabled");
    bloomSpeedLowRelay = std::make_unique<juce::WebSliderRelay>("bloomSpeedLow");
    bloomSpeedMidRelay = std::make_unique<juce::WebSliderRelay>("bloomSpeedMid");
    bloomSpeedHighRelay = std::make_unique<juce::WebSliderRelay>("bloomSpeedHigh");
    bloomAmountLowRelay = std::make_unique<juce::WebSliderRelay>("bloomAmountLow");
    bloomAmountMidRelay = std::make_unique<juce::WebSliderRelay>("bloomAmountMid");
    bloomAmountHighRelay = std::make_unique<juce::WebSliderRelay>("bloomAmountHigh");
    shimmerRelay = std::make_unique<juce::WebSliderRelay>("shimmer");
    unisonCountRelay = std::make_unique<juce::WebSliderRelay>("unisonCount");
    unisonDetuneRelay = std::make_unique<juce::WebSliderRelay>("unisonDetune");
    octaveBlendSubRelay = std::make_unique<juce::WebSliderRelay>("octaveBlendSub");
    octaveBlendOctRelay = std::make_unique<juce::WebSliderRelay>("octaveBlendOct");
    stereoSpreadRelay = std::make_unique<juce::WebSliderRelay>("stereoSpread");
    partialTuningRelay = std::make_unique<juce::WebSliderRelay>("partialTuning");
    pitchEnvelopeRelay = std::make_unique<juce::WebSliderRelay>("pitchEnvelope");
    pitchEnvTimeRelay = std::make_unique<juce::WebSliderRelay>("pitchEnvTime");
    nonlinearEffectsRelay = std::make_unique<juce::WebSliderRelay>("nonlinearEffects");
    attackLevelRelay = std::make_unique<juce::WebSliderRelay>("attackLevel");
    humanizeRelay = std::make_unique<juce::WebSliderRelay>("humanize");  // v2.4.0
    lpFilterEnabledRelay = std::make_unique<juce::WebSliderRelay>("lpFilterEnabled");  // v2.6.0
    lpFilterCutoffRelay = std::make_unique<juce::WebSliderRelay>("lpFilterCutoff");    // v2.6.0
    highFidelityRelay = std::make_unique<juce::WebSliderRelay>("highFidelity");        // v3.1.2
    outputGainRelay = std::make_unique<juce::WebSliderRelay>("outputGain");

    // v4.0.0: Effects chain relays
    chorusRateRelay = std::make_unique<juce::WebSliderRelay>("chorusRate");
    chorusDepthRelay = std::make_unique<juce::WebSliderRelay>("chorusDepth");
    chorusMixRelay = std::make_unique<juce::WebSliderRelay>("chorusMix");
    fxDelayTimeRelay = std::make_unique<juce::WebSliderRelay>("delayTime");
    delayFeedbackRelay = std::make_unique<juce::WebSliderRelay>("delayFeedback");
    delayMixRelay = std::make_unique<juce::WebSliderRelay>("delayMix");
    eqLowGainRelay = std::make_unique<juce::WebSliderRelay>("eqLowGain");
    eqMidGainRelay = std::make_unique<juce::WebSliderRelay>("eqMidGain");
    eqMidFreqRelay = std::make_unique<juce::WebSliderRelay>("eqMidFreq");
    eqHighGainRelay = std::make_unique<juce::WebSliderRelay>("eqHighGain");
    reverbSizeRelay = std::make_unique<juce::WebSliderRelay>("reverbSize");
    reverbDampRelay = std::make_unique<juce::WebSliderRelay>("reverbDamp");
    reverbPredelayRelay = std::make_unique<juce::WebSliderRelay>("reverbPredelay");
    reverbMixRelay = std::make_unique<juce::WebSliderRelay>("reverbMix");
    reverbModRelay = std::make_unique<juce::WebSliderRelay>("reverbMod");
    reverbShimmerRelay = std::make_unique<juce::WebSliderRelay>("reverbShimmer");
    delayModeRelay = std::make_unique<juce::WebComboBoxRelay>("delayMode");
    chorusBypassRelay = std::make_unique<juce::WebToggleButtonRelay>("chorusBypass");
    delayBypassRelay = std::make_unique<juce::WebToggleButtonRelay>("delayBypass");
    eqBypassRelay = std::make_unique<juce::WebToggleButtonRelay>("eqBypass");
    reverbBypassRelay = std::make_unique<juce::WebToggleButtonRelay>("reverbBypass");
    // Multi-stage envelope relays
    strikeTimeRelay = std::make_unique<juce::WebSliderRelay>("strikeTime");
    brillianceRelay = std::make_unique<juce::WebSliderRelay>("brilliance");
    bodyTimeRelay = std::make_unique<juce::WebSliderRelay>("bodyTime");
    humSustainRelay = std::make_unique<juce::WebSliderRelay>("humSustain");

    // v3.0.0: Tuning relays
    tuningMasterTuneRelay = std::make_unique<juce::WebSliderRelay>("tuning_masterTune");
    tuningOctaveStretchRelay = std::make_unique<juce::WebSliderRelay>("tuning_octaveStretch");
    tuningPitchBendRangeRelay = std::make_unique<juce::WebSliderRelay>("tuning_pitchBendRange");

    materialRelay = std::make_unique<juce::WebComboBoxRelay>("material");
    strikeNoiseCharRelay = std::make_unique<juce::WebComboBoxRelay>("strikeNoiseChar");
    velocityCurveRelay = std::make_unique<juce::WebComboBoxRelay>("velocityCurve");
    tuningTemperamentPresetRelay = std::make_unique<juce::WebComboBoxRelay>("tuning_temperamentPreset");

    // 2️⃣ CREATE WEBVIEW WITH OPTIONS
    webView = std::make_unique<juce::WebBrowserComponent>(
        juce::WebBrowserComponent::Options{}
            .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
            .withWinWebView2Options(
                juce::WebBrowserComponent::Options::WinWebView2{}
                    .withUserDataFolder(juce::File::getSpecialLocation(
                        juce::File::SpecialLocationType::tempDirectory)))
            .withNativeIntegrationEnabled()
            .withResourceProvider([this](const auto& url) { return getResource(url); })
            .withOptionsFrom(*strikePositionRelay)
            .withOptionsFrom(*malletHardnessRelay)
            .withOptionsFrom(*dampingRelay)
            .withOptionsFrom(*overtoneBrightnessRelay)
            .withOptionsFrom(*acousticBrightnessRelay)
            .withOptionsFrom(*airAbsorptionRelay)
            .withOptionsFrom(*airAbsorptionTimeRelay)
            .withOptionsFrom(*materialRelay)
            .withOptionsFrom(*inharmonicityRelay)
            .withOptionsFrom(*bloomSpeedRelay)
            .withOptionsFrom(*bloomAmountRelay)
            // v1.5.0: Bloom fine controls
            .withOptionsFrom(*bloomFineEnabledRelay)
            .withOptionsFrom(*bloomSpeedLowRelay)
            .withOptionsFrom(*bloomSpeedMidRelay)
            .withOptionsFrom(*bloomSpeedHighRelay)
            .withOptionsFrom(*bloomAmountLowRelay)
            .withOptionsFrom(*bloomAmountMidRelay)
            .withOptionsFrom(*bloomAmountHighRelay)
            .withOptionsFrom(*shimmerRelay)
            .withOptionsFrom(*unisonCountRelay)
            .withOptionsFrom(*unisonDetuneRelay)
            .withOptionsFrom(*octaveBlendSubRelay)
            .withOptionsFrom(*octaveBlendOctRelay)
            .withOptionsFrom(*stereoSpreadRelay)
            .withOptionsFrom(*partialTuningRelay)
            .withOptionsFrom(*pitchEnvelopeRelay)
            .withOptionsFrom(*pitchEnvTimeRelay)
            .withOptionsFrom(*nonlinearEffectsRelay)
            .withOptionsFrom(*attackLevelRelay)
            .withOptionsFrom(*humanizeRelay)  // v2.4.0
            .withOptionsFrom(*lpFilterEnabledRelay)  // v2.6.0
            .withOptionsFrom(*lpFilterCutoffRelay)    // v2.6.0
            .withOptionsFrom(*highFidelityRelay)      // v3.1.2
            .withOptionsFrom(*outputGainRelay)
            // v4.0.0: Effects chain relays
            .withOptionsFrom(*chorusRateRelay)
            .withOptionsFrom(*chorusDepthRelay)
            .withOptionsFrom(*chorusMixRelay)
            .withOptionsFrom(*fxDelayTimeRelay)
            .withOptionsFrom(*delayFeedbackRelay)
            .withOptionsFrom(*delayMixRelay)
            .withOptionsFrom(*eqLowGainRelay)
            .withOptionsFrom(*eqMidGainRelay)
            .withOptionsFrom(*eqMidFreqRelay)
            .withOptionsFrom(*eqHighGainRelay)
            .withOptionsFrom(*reverbSizeRelay)
            .withOptionsFrom(*reverbDampRelay)
            .withOptionsFrom(*reverbPredelayRelay)
            .withOptionsFrom(*reverbMixRelay)
            .withOptionsFrom(*reverbModRelay)
            .withOptionsFrom(*reverbShimmerRelay)
            .withOptionsFrom(*delayModeRelay)
            .withOptionsFrom(*chorusBypassRelay)
            .withOptionsFrom(*delayBypassRelay)
            .withOptionsFrom(*eqBypassRelay)
            .withOptionsFrom(*reverbBypassRelay)
            // Multi-stage envelope relays
            .withOptionsFrom(*strikeTimeRelay)
            .withOptionsFrom(*brillianceRelay)
            .withOptionsFrom(*bodyTimeRelay)
            .withOptionsFrom(*humSustainRelay)
            .withOptionsFrom(*strikeNoiseCharRelay)
            .withOptionsFrom(*velocityCurveRelay)
            // v3.0.0: Tuning relays
            .withOptionsFrom(*tuningMasterTuneRelay)
            .withOptionsFrom(*tuningOctaveStretchRelay)
            .withOptionsFrom(*tuningPitchBendRangeRelay)
            .withOptionsFrom(*tuningTemperamentPresetRelay)

            // ═══════════════════════════════════════════════════════════════════
            // v2.2.0: GUI KEYBOARD NATIVE FUNCTION
            // ═══════════════════════════════════════════════════════════════════

            .withNativeFunction("sendMidiNote", [this](const juce::Array<juce::var>& args,
                                                        std::function<void(juce::var)> complete) {
                if (args.size() >= 3) {
                    int midiNote = static_cast<int>(args[0]);
                    float velocity = static_cast<float>(args[1]);
                    bool isNoteOn = static_cast<bool>(args[2]);

                    if (isNoteOn)
                        processorRef.triggerNoteOn(midiNote, velocity);
                    else
                        processorRef.triggerNoteOff(midiNote);
                }
                complete({});
            })

            // ═══════════════════════════════════════════════════════════════════
            // PRESET NATIVE FUNCTIONS
            // ═══════════════════════════════════════════════════════════════════

            // getPresetList: Returns flat array of all preset names
            .withNativeFunction("getPresetList", [this](auto, auto complete) {
                auto& pm = processorRef.getPresetManager();
                auto presets = pm.getPresetList();
                juce::Array<juce::var> arr;
                for (const auto& name : presets)
                    arr.add(name);
                complete(juce::var(arr));
            })

            // getPresetListWithCategories: Returns {category: [presets...]} object
            .withNativeFunction("getPresetListWithCategories", [this](auto, auto complete) {
                auto& pm = processorRef.getPresetManager();
                auto categorized = pm.getPresetListWithCategories();
                auto* obj = new juce::DynamicObject();
                for (const auto& [category, presets] : categorized)
                {
                    juce::Array<juce::var> arr;
                    for (const auto& name : presets)
                        arr.add(name);
                    obj->setProperty(category, juce::var(arr));
                }
                complete(juce::var(obj));
            })

            // getCurrentPreset: Returns current preset name
            .withNativeFunction("getCurrentPreset", [this](auto, auto complete) {
                auto& pm = processorRef.getPresetManager();
                complete(juce::var(pm.getCurrentPresetName()));
            })

            // loadPreset: Loads preset by name (flat search)
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

            // loadPresetFromCategory: Loads preset from specific category
            .withNativeFunction("loadPresetFromCategory", [this](const auto& args, auto complete) {
                if (args.size() < 2 || !args[0].isString() || !args[1].isString())
                {
                    complete(juce::var(false));
                    return;
                }
                auto& pm = processorRef.getPresetManager();
                bool success = pm.loadPresetFromCategory(args[0].toString(), args[1].toString());
                complete(juce::var(success));
            })

            // savePreset: Saves user preset with given name
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

            // selectNextPreset: Navigate to next preset, returns new name
            .withNativeFunction("selectNextPreset", [this](auto, auto complete) {
                auto& pm = processorRef.getPresetManager();
                auto name = pm.getNextPreset();
                complete(juce::var(name));
            })

            // selectPreviousPreset: Navigate to previous preset, returns new name
            .withNativeFunction("selectPreviousPreset", [this](auto, auto complete) {
                auto& pm = processorRef.getPresetManager();
                auto name = pm.getPreviousPreset();
                complete(juce::var(name));
            })

            // savePresetWithDialog: Opens save dialog, saves preset
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

            // loadPresetFromFile: Opens file chooser, loads selected preset
            .withNativeFunction("loadPresetFromFile", [this](auto, auto complete) {
                fileChooser = std::make_shared<juce::FileChooser>(
                    "Load Preset",
                    processorRef.getPresetManager().getPresetsDirectory(),
                    "*.json"
                );
                fileChooser->launchAsync(
                    juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                    [this, complete](const juce::FileChooser& fc) {
                        auto result = fc.getResult();
                        if (result == juce::File{})
                        {
                            complete(juce::var(""));
                            return;
                        }
                        auto& pm = processorRef.getPresetManager();
                        pm.loadPresetFromFile(result);
                        complete(juce::var(pm.getCurrentPresetName()));
                    }
                );
            })

            // ═══════════════════════════════════════════════════════════════════
            // v3.0.0: TUNING NATIVE FUNCTIONS (24 functions)
            // ═══════════════════════════════════════════════════════════════════

            // --- Tuning Intervals ---
            .withNativeFunction("getTuningIntervals", [this](const juce::Array<juce::var>&, auto complete) {
                auto intervals = processorRef.getTuningEngine().getIntervals();
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
                        processorRef.getTuningEngine().setCustomIntervals(intervals, "Custom");
                        complete(true);
                        return;
                    }
                }
                complete(false);
            })

            .withNativeFunction("getTuningName", [this](const juce::Array<juce::var>&, auto complete) {
                complete(processorRef.getTuningEngine().getActiveTuningName());
            })

            .withNativeFunction("setSingleInterval", [this](const juce::Array<juce::var>& args, auto complete) {
                if (args.size() >= 2) {
                    int index = static_cast<int>(args[0]);
                    double cents = static_cast<double>(args[1]);
                    processorRef.getTuningEngine().setSingleInterval(index, cents);
                    complete(true);
                    return;
                }
                complete(false);
            })

            .withNativeFunction("setSingleIntervalEncoded", [this](const juce::Array<juce::var>& args, auto complete) {
                if (args.size() >= 2) {
                    int index = static_cast<int>(args[0]);
                    double cents = static_cast<double>(args[1]);
                    processorRef.getTuningEngine().setSingleInterval(index, cents);
                    complete(true);
                    return;
                }
                complete(false);
            })

            // --- Tonic / Rotation ---
            .withNativeFunction("setTonicNote", [this](const juce::Array<juce::var>& args, auto complete) {
                if (args.size() >= 1) {
                    int tonic = static_cast<int>(args[0]);
                    processorRef.getTuningEngine().setTonicNote(tonic);
                    complete(true);
                    return;
                }
                complete(false);
            })

            .withNativeFunction("getTonicNote", [this](const juce::Array<juce::var>&, auto complete) {
                complete(processorRef.getTuningEngine().getTonicNote());
            })

            // --- Octave Stretch ---
            .withNativeFunction("getOctaveStretch", [this](const juce::Array<juce::var>&, auto complete) {
                complete(processorRef.getTuningEngine().getOctaveStretch());
            })

            .withNativeFunction("setOctaveStretch", [this](const juce::Array<juce::var>& args, auto complete) {
                if (args.size() >= 1) {
                    float stretch = static_cast<float>(args[0]);
                    processorRef.getTuningEngine().setOctaveStretch(stretch);
                    complete(true);
                    return;
                }
                complete(false);
            })

            // --- Master Tune ---
            .withNativeFunction("getMasterTune", [this](const juce::Array<juce::var>&, auto complete) {
                complete(processorRef.getTuningEngine().getMasterTune());
            })

            .withNativeFunction("setMasterTune", [this](const juce::Array<juce::var>& args, auto complete) {
                if (args.size() >= 1) {
                    double hz = static_cast<double>(args[0]);
                    processorRef.getTuningEngine().setMasterTune(hz);
                    complete(true);
                    return;
                }
                complete(false);
            })

            // --- Temperament Presets ---
            .withNativeFunction("setTemperamentPreset", [this](const juce::Array<juce::var>& args, auto complete) {
                if (args.size() >= 1) {
                    int preset = static_cast<int>(args[0]);
                    processorRef.getTuningEngine().setBuiltInPreset(
                        static_cast<TuningEngine::BuiltInPreset>(preset));
                    complete(true);
                    return;
                }
                complete(false);
            })

            .withNativeFunction("getTemperamentPreset", [this](const juce::Array<juce::var>&, auto complete) {
                complete(static_cast<int>(processorRef.getTuningEngine().getBuiltInPreset()));
            })

            // --- Scala File I/O ---
            .withNativeFunction("loadScalaFile", [this](const juce::Array<juce::var>&, auto complete) {
                tuningFileChooser = std::make_shared<juce::FileChooser>(
                    "Load Scala File",
                    juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
                    "*.scl");
                tuningFileChooser->launchAsync(
                    juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                    [this, complete](const juce::FileChooser& fc) {
                        auto file = fc.getResult();
                        if (file.existsAsFile()) {
                            bool success = processorRef.getTuningEngine().loadScalaFile(file);
                            complete(success ? juce::var(processorRef.getTuningEngine().getActiveTuningName())
                                            : juce::var());
                        } else {
                            complete(juce::var());
                        }
                    });
            })

            .withNativeFunction("saveScalaFile", [this](const juce::Array<juce::var>&, auto complete) {
                tuningFileChooser = std::make_shared<juce::FileChooser>(
                    "Save Scala File",
                    juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                        .getChildFile("scale.scl"),
                    "*.scl");
                tuningFileChooser->launchAsync(
                    juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                    [this, complete](const juce::FileChooser& fc) {
                        auto file = fc.getResult();
                        if (file != juce::File()) {
                            auto content = processorRef.getTuningEngine().generateScalaFileContent();
                            file.replaceWithText(content);
                            complete(true);
                        } else {
                            complete(false);
                        }
                    });
            })

            .withNativeFunction("loadKBMFile", [this](const juce::Array<juce::var>&, auto complete) {
                tuningFileChooser = std::make_shared<juce::FileChooser>(
                    "Load Keyboard Mapping",
                    juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
                    "*.kbm");
                tuningFileChooser->launchAsync(
                    juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                    [this, complete](const juce::FileChooser& fc) {
                        auto file = fc.getResult();
                        if (file.existsAsFile()) {
                            bool success = processorRef.getTuningEngine().loadKBMFile(file);
                            complete(success);
                        } else {
                            complete(false);
                        }
                    });
            })

            .withNativeFunction("saveKBMFile", [this](const juce::Array<juce::var>&, auto complete) {
                tuningFileChooser = std::make_shared<juce::FileChooser>(
                    "Save Keyboard Mapping",
                    juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                        .getChildFile("mapping.kbm"),
                    "*.kbm");
                tuningFileChooser->launchAsync(
                    juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                    [this, complete](const juce::FileChooser& fc) {
                        auto file = fc.getResult();
                        if (file != juce::File()) {
                            auto content = processorRef.getTuningEngine().generateKBMFileContent();
                            file.replaceWithText(content);
                            complete(true);
                        } else {
                            complete(false);
                        }
                    });
            })

            // --- Scale Generator ---
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

            .withNativeFunction("generateRank2", [](const juce::Array<juce::var>& args, auto complete) {
                if (args.size() >= 3) {
                    double generator = static_cast<double>(args[0]);
                    double period = static_cast<double>(args[1]);
                    int count = static_cast<int>(args[2]);
                    auto intervals = ScaleGenerator::generateRank2(generator, period, count);
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
                        processorRef.getTuningEngine().setCustomIntervals(intervals, scaleName);
                        complete(true);
                        return;
                    }
                }
                complete(false);
            })

            // --- Embedded Tuning Library ---
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
                        // Append the period to intervals so setCustomIntervals()
                        // correctly identifies the octave/period boundary
                        auto intervals = tuning->intervals;
                        intervals.push_back(tuning->period);
                        processorRef.getTuningEngine().setCustomIntervals(
                            intervals, juce::String(tuning->name));
                        complete(true);
                        return;
                    }
                }
                complete(false);
            })

            // --- HTML Export ---
            .withNativeFunction("exportTuningHTML", [this](const juce::Array<juce::var>&, auto complete) {
                tuningFileChooser = std::make_shared<juce::FileChooser>(
                    "Export Tuning Documentation",
                    juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                        .getChildFile("tuning-export.html"),
                    "*.html");
                tuningFileChooser->launchAsync(
                    juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                    [this, complete](const juce::FileChooser& fc) {
                        auto file = fc.getResult();
                        if (file != juce::File()) {
                            auto html = TuningExporter::toHTML(processorRef.getTuningEngine(), "O-Bells");
                            file.replaceWithText(html);
                            complete(true);
                        } else {
                            complete(false);
                        }
                    });
            })
    );

    // 3️⃣ CREATE ATTACHMENTS LAST
    auto& apvts = processorRef.getAPVTS();

    strikePositionAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("strikePosition"), *strikePositionRelay, nullptr);
    malletHardnessAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("malletHardness"), *malletHardnessRelay, nullptr);
    dampingAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("damping"), *dampingRelay, nullptr);
    overtoneBrightnessAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("overtoneBrightness"), *overtoneBrightnessRelay, nullptr);
    acousticBrightnessAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("acousticBrightness"), *acousticBrightnessRelay, nullptr);
    airAbsorptionAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("airAbsorption"), *airAbsorptionRelay, nullptr);
    airAbsorptionTimeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("airAbsorptionTime"), *airAbsorptionTimeRelay, nullptr);
    inharmonicityAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("inharmonicity"), *inharmonicityRelay, nullptr);
    bloomSpeedAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("bloomSpeed"), *bloomSpeedRelay, nullptr);
    bloomAmountAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("bloomAmount"), *bloomAmountRelay, nullptr);
    // v1.5.0: Bloom fine controls (per-band)
    bloomFineEnabledAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("bloomFineEnabled"), *bloomFineEnabledRelay, nullptr);
    bloomSpeedLowAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("bloomSpeedLow"), *bloomSpeedLowRelay, nullptr);
    bloomSpeedMidAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("bloomSpeedMid"), *bloomSpeedMidRelay, nullptr);
    bloomSpeedHighAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("bloomSpeedHigh"), *bloomSpeedHighRelay, nullptr);
    bloomAmountLowAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("bloomAmountLow"), *bloomAmountLowRelay, nullptr);
    bloomAmountMidAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("bloomAmountMid"), *bloomAmountMidRelay, nullptr);
    bloomAmountHighAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("bloomAmountHigh"), *bloomAmountHighRelay, nullptr);
    shimmerAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("shimmer"), *shimmerRelay, nullptr);
    unisonCountAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("unisonCount"), *unisonCountRelay, nullptr);
    unisonDetuneAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("unisonDetune"), *unisonDetuneRelay, nullptr);
    octaveBlendSubAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("octaveBlendSub"), *octaveBlendSubRelay, nullptr);
    octaveBlendOctAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("octaveBlendOct"), *octaveBlendOctRelay, nullptr);
    stereoSpreadAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("stereoSpread"), *stereoSpreadRelay, nullptr);
    partialTuningAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("partialTuning"), *partialTuningRelay, nullptr);
    pitchEnvelopeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("pitchEnvelope"), *pitchEnvelopeRelay, nullptr);
    pitchEnvTimeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("pitchEnvTime"), *pitchEnvTimeRelay, nullptr);
    nonlinearEffectsAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("nonlinearEffects"), *nonlinearEffectsRelay, nullptr);
    attackLevelAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("attackLevel"), *attackLevelRelay, nullptr);
    humanizeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("humanize"), *humanizeRelay, nullptr);  // v2.4.0
    lpFilterEnabledAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("lpFilterEnabled"), *lpFilterEnabledRelay, nullptr);  // v2.6.0
    lpFilterCutoffAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("lpFilterCutoff"), *lpFilterCutoffRelay, nullptr);    // v2.6.0
    highFidelityAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("highFidelity"), *highFidelityRelay, nullptr);        // v3.1.2
    outputGainAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("outputGain"), *outputGainRelay, nullptr);

    // v4.0.0: Effects chain attachments
    chorusRateAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("chorusRate"), *chorusRateRelay, nullptr);
    chorusDepthAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("chorusDepth"), *chorusDepthRelay, nullptr);
    chorusMixAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("chorusMix"), *chorusMixRelay, nullptr);
    fxDelayTimeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("delayTime"), *fxDelayTimeRelay, nullptr);
    delayFeedbackAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("delayFeedback"), *delayFeedbackRelay, nullptr);
    delayMixAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("delayMix"), *delayMixRelay, nullptr);
    eqLowGainAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("eqLowGain"), *eqLowGainRelay, nullptr);
    eqMidGainAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("eqMidGain"), *eqMidGainRelay, nullptr);
    eqMidFreqAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("eqMidFreq"), *eqMidFreqRelay, nullptr);
    eqHighGainAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("eqHighGain"), *eqHighGainRelay, nullptr);
    reverbSizeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("reverbSize"), *reverbSizeRelay, nullptr);
    reverbDampAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("reverbDamp"), *reverbDampRelay, nullptr);
    reverbPredelayAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("reverbPredelay"), *reverbPredelayRelay, nullptr);
    reverbMixAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("reverbMix"), *reverbMixRelay, nullptr);
    reverbModAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("reverbMod"), *reverbModRelay, nullptr);
    reverbShimmerAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("reverbShimmer"), *reverbShimmerRelay, nullptr);
    delayModeAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *apvts.getParameter("delayMode"), *delayModeRelay, nullptr);
    chorusBypassAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *apvts.getParameter("chorusBypass"), *chorusBypassRelay, nullptr);
    delayBypassAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *apvts.getParameter("delayBypass"), *delayBypassRelay, nullptr);
    eqBypassAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *apvts.getParameter("eqBypass"), *eqBypassRelay, nullptr);
    reverbBypassAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *apvts.getParameter("reverbBypass"), *reverbBypassRelay, nullptr);
    // Multi-stage envelope attachments
    strikeTimeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("strikeTime"), *strikeTimeRelay, nullptr);
    brillianceAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("brilliance"), *brillianceRelay, nullptr);
    bodyTimeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("bodyTime"), *bodyTimeRelay, nullptr);
    humSustainAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("humSustain"), *humSustainRelay, nullptr);

    // v3.0.0: Tuning attachments
    tuningMasterTuneAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("tuning_masterTune"), *tuningMasterTuneRelay, nullptr);
    tuningOctaveStretchAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("tuning_octaveStretch"), *tuningOctaveStretchRelay, nullptr);
    tuningPitchBendRangeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("tuning_pitchBendRange"), *tuningPitchBendRangeRelay, nullptr);

    materialAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *apvts.getParameter("material"), *materialRelay, nullptr);
    strikeNoiseCharAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *apvts.getParameter("strikeNoiseChar"), *strikeNoiseCharRelay, nullptr);
    velocityCurveAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *apvts.getParameter("velocityCurve"), *velocityCurveRelay, nullptr);
    tuningTemperamentPresetAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *apvts.getParameter("tuning_temperamentPreset"), *tuningTemperamentPresetRelay, nullptr);

    // Add WebView to editor
    addAndMakeVisible(*webView);

    // Navigate to UI
    webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());

    // Set editor size (800x600 from mockup)
    setSize(800, 600);

    // Start meter update timer (30 Hz for smooth animation)
    startTimerHz(30);
}

OBellsAudioProcessorEditor::~OBellsAudioProcessorEditor()
{
    // Stop timer before destruction
    stopTimer();

    // Attachments destroyed first (safe - they stop using relays/webView)
    // Then webView destroyed
    // Then relays destroyed last
}

void OBellsAudioProcessorEditor::timerCallback()
{
    // Read levels from processor (atomic, thread-safe)
    float leftLevel = processorRef.outputLevelLeft.load();
    float rightLevel = processorRef.outputLevelRight.load();

    // Convert to percentage (clamp to 0-100)
    int leftPercent = static_cast<int>(std::min(1.0f, leftLevel) * 100.0f);
    int rightPercent = static_cast<int>(std::min(1.0f, rightLevel) * 100.0f);

    // Send to WebView via JavaScript
    juce::String js = juce::String::formatted(
        "if (window.updateMeterLevels) window.updateMeterLevels(%d, %d);",
        leftPercent, rightPercent
    );
    webView->evaluateJavascript(js);

    // v2.7.0: Send note state changes for tuning spoke highlighting
    uint64_t notesLow = processorRef.activeNotesLow.load();
    uint64_t notesHigh = processorRef.activeNotesHigh.load();

    if (notesLow != prevActiveNotesLow || notesHigh != prevActiveNotesHigh)
    {
        // Find notes that turned on
        uint64_t newOnLow = notesLow & ~prevActiveNotesLow;
        uint64_t newOnHigh = notesHigh & ~prevActiveNotesHigh;
        uint64_t newOffLow = prevActiveNotesLow & ~notesLow;
        uint64_t newOffHigh = prevActiveNotesHigh & ~notesHigh;

        juce::String noteJs;

        // Process note-ons
        for (int i = 0; i < 64; ++i)
            if (newOnLow & (uint64_t(1) << i))
                noteJs += juce::String::formatted("if(window.tuningNoteOn)window.tuningNoteOn(%d);", i);
        for (int i = 0; i < 64; ++i)
            if (newOnHigh & (uint64_t(1) << i))
                noteJs += juce::String::formatted("if(window.tuningNoteOn)window.tuningNoteOn(%d);", i + 64);

        // Process note-offs
        for (int i = 0; i < 64; ++i)
            if (newOffLow & (uint64_t(1) << i))
                noteJs += juce::String::formatted("if(window.tuningNoteOff)window.tuningNoteOff(%d);", i);
        for (int i = 0; i < 64; ++i)
            if (newOffHigh & (uint64_t(1) << i))
                noteJs += juce::String::formatted("if(window.tuningNoteOff)window.tuningNoteOff(%d);", i + 64);

        if (noteJs.isNotEmpty())
            webView->evaluateJavascript(noteJs);

        prevActiveNotesLow = notesLow;
        prevActiveNotesHigh = notesHigh;
    }

    // v3.1.0: Send held notes + frequencies for TrueKeys interval display
    std::vector<int> heldNotes;
    std::vector<double> heldFreqs;
    processorRef.getHeldNotesData(heldNotes, heldFreqs);

    juce::String notesJson = "[";
    juce::String freqsJson = "[";
    for (size_t i = 0; i < heldNotes.size(); ++i)
    {
        if (i > 0)
        {
            notesJson += ",";
            freqsJson += ",";
        }
        notesJson += juce::String(heldNotes[i]);
        freqsJson += juce::String(heldFreqs[i], 4);
    }
    notesJson += "]";
    freqsJson += "]";

    juce::String heldJs = "if (typeof window.updateHeldNotes === 'function') window.updateHeldNotes("
        + notesJson + "," + freqsJson + ");";
    webView->evaluateJavascript(heldJs);
}

void OBellsAudioProcessorEditor::paint(juce::Graphics& g)
{
    // WebView handles all painting
    juce::ignoreUnused(g);
}

void OBellsAudioProcessorEditor::resized()
{
    // WebView fills entire editor
    if (webView)
        webView->setBounds(getLocalBounds());
}

std::optional<juce::WebBrowserComponent::Resource>
OBellsAudioProcessorEditor::getResource(const juce::String& url)
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

    if (url == "/img/snail.png") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::snail_png, BinaryData::snail_pngSize),
            juce::String("image/png")
        };
    }

    // v3.0.0: Tuning panel resources
    if (url == "/js/tuning-panel.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::tuningpanel_js, BinaryData::tuningpanel_jsSize),
            juce::String("text/javascript")
        };
    }

    if (url == "/css/tuning-panel.css") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::tuningpanel_css, BinaryData::tuningpanel_cssSize),
            juce::String("text/css")
        };
    }

    // Resource not found
    juce::Logger::writeToLog("O-Bells: Resource not found: " + url);
    return std::nullopt;
}
