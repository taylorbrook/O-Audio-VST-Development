/*
  ==============================================================================

    O-IntonationPad - Editor Implementation
    Ouaricon Development
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginEditor.h"
#include "BinaryData.h"
#include "DSP/TuningEngine.h"
#include "DSP/ScaleGenerator.h"
#include "DSP/TuningExporter.h"
#include "DSP/EmbeddedTunings.h"

namespace {
static juce::String doubleVectorToJSON(const std::vector<double>& v)
{
    juce::String json = "[";
    for (size_t i = 0; i < v.size(); ++i) {
        if (i > 0) json += ",";
        json += juce::String(v[i], 6);
    }
    json += "]";
    return json;
}
}

OIntonationPadAudioProcessorEditor::OIntonationPadAudioProcessorEditor(OIntonationPadAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    createRelays();
    webView = std::make_unique<juce::WebBrowserComponent>(buildWebViewOptions());
    createAttachments();

    addAndMakeVisible(*webView);
    setSize(800, 500);
    startTimerHz(30);
}

void OIntonationPadAudioProcessorEditor::createRelays()
{
    voiceCountRelay = std::make_unique<juce::WebSliderRelay>("voiceCount");
    complexityRelay = std::make_unique<juce::WebSliderRelay>("complexity");
    keyRootRelay = std::make_unique<juce::WebSliderRelay>("keyRoot");
    stereoSpreadRelay = std::make_unique<juce::WebSliderRelay>("stereoSpread");
    spacingRelay = std::make_unique<juce::WebSliderRelay>("spacing");
    inversionRelay = std::make_unique<juce::WebSliderRelay>("inversion");
    wavetablePosRelay = std::make_unique<juce::WebSliderRelay>("wavetablePos");
    lfoRateRelay = std::make_unique<juce::WebSliderRelay>("lfoRate");
    lfoDepthRelay = std::make_unique<juce::WebSliderRelay>("lfoDepth");
    timingRandomRelay = std::make_unique<juce::WebSliderRelay>("timingRandom");
    detuneRandomRelay = std::make_unique<juce::WebSliderRelay>("detuneRandom");
    attackTimeRelay = std::make_unique<juce::WebSliderRelay>("attackTime");
    releaseTimeRelay = std::make_unique<juce::WebSliderRelay>("releaseTime");
    filterCutoffRelay = std::make_unique<juce::WebSliderRelay>("filterCutoff");
    masterVolumeRelay = std::make_unique<juce::WebSliderRelay>("masterVolume");

    wavetableBankRelay = std::make_unique<juce::WebComboBoxRelay>("wavetableBank");

    wavetablePos2Relay = std::make_unique<juce::WebSliderRelay>("wavetablePos2");
    gainARelay = std::make_unique<juce::WebSliderRelay>("gainA");
    gainBRelay = std::make_unique<juce::WebSliderRelay>("gainB");
    lfoRate2Relay = std::make_unique<juce::WebSliderRelay>("lfoRate2");
    lfoDepth2Relay = std::make_unique<juce::WebSliderRelay>("lfoDepth2");
    wavetableBank2Relay = std::make_unique<juce::WebComboBoxRelay>("wavetableBank2");

    tuningMasterTuneRelay = std::make_unique<juce::WebSliderRelay>("tuning_masterTune");
    tuningOctaveStretchRelay = std::make_unique<juce::WebSliderRelay>("tuning_octaveStretch");
    tuningPitchBendRangeRelay = std::make_unique<juce::WebSliderRelay>("tuning_pitchBendRange");
    tuningTemperamentPresetRelay = std::make_unique<juce::WebComboBoxRelay>("tuning_temperamentPreset");

    chorusRateRelay = std::make_unique<juce::WebSliderRelay>("chorusRate");
    chorusDepthRelay = std::make_unique<juce::WebSliderRelay>("chorusDepth");
    chorusMixRelay = std::make_unique<juce::WebSliderRelay>("chorusMix");
    delayTimeRelay = std::make_unique<juce::WebSliderRelay>("delayTime");
    delayFeedbackRelay = std::make_unique<juce::WebSliderRelay>("delayFeedback");
    delayModeRelay = std::make_unique<juce::WebComboBoxRelay>("delayMode");
    delayMixRelay = std::make_unique<juce::WebSliderRelay>("delayMix");
    eqLowGainRelay = std::make_unique<juce::WebSliderRelay>("eqLowGain");
    eqMidGainRelay = std::make_unique<juce::WebSliderRelay>("eqMidGain");
    eqMidFreqRelay = std::make_unique<juce::WebSliderRelay>("eqMidFreq");
    eqHighGainRelay = std::make_unique<juce::WebSliderRelay>("eqHighGain");
    reverbSizeRelay = std::make_unique<juce::WebSliderRelay>("reverbSize");
    reverbDampRelay = std::make_unique<juce::WebSliderRelay>("reverbDamp");
    reverbPredelayRelay = std::make_unique<juce::WebSliderRelay>("reverbPredelay");
    reverbMixRelay = std::make_unique<juce::WebSliderRelay>("reverbMix");
    chorusBypassRelay = std::make_unique<juce::WebToggleButtonRelay>("chorusBypass");
    delayBypassRelay = std::make_unique<juce::WebToggleButtonRelay>("delayBypass");
    eqBypassRelay = std::make_unique<juce::WebToggleButtonRelay>("eqBypass");
    reverbBypassRelay = std::make_unique<juce::WebToggleButtonRelay>("reverbBypass");
}

juce::WebBrowserComponent::Options OIntonationPadAudioProcessorEditor::buildWebViewOptions()
{
    return juce::WebBrowserComponent::Options{}
        .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
        .withWinWebView2Options(
            juce::WebBrowserComponent::Options::WinWebView2{}
                .withUserDataFolder(juce::File::getSpecialLocation(
                    juce::File::SpecialLocationType::tempDirectory)))
        .withNativeIntegrationEnabled()
        .withResourceProvider([this](auto& url) { return getResource(url); })
        .withOptionsFrom(*voiceCountRelay)
        .withOptionsFrom(*complexityRelay)
        .withOptionsFrom(*keyRootRelay)
        .withOptionsFrom(*stereoSpreadRelay)
        .withOptionsFrom(*spacingRelay)
        .withOptionsFrom(*inversionRelay)
        .withOptionsFrom(*wavetablePosRelay)
        .withOptionsFrom(*lfoRateRelay)
        .withOptionsFrom(*lfoDepthRelay)
        .withOptionsFrom(*timingRandomRelay)
        .withOptionsFrom(*detuneRandomRelay)
        .withOptionsFrom(*attackTimeRelay)
        .withOptionsFrom(*releaseTimeRelay)
        .withOptionsFrom(*filterCutoffRelay)
        .withOptionsFrom(*masterVolumeRelay)
        .withOptionsFrom(*wavetableBankRelay)
        .withOptionsFrom(*wavetablePos2Relay)
        .withOptionsFrom(*gainARelay)
        .withOptionsFrom(*gainBRelay)
        .withOptionsFrom(*lfoRate2Relay)
        .withOptionsFrom(*lfoDepth2Relay)
        .withOptionsFrom(*wavetableBank2Relay)
        .withOptionsFrom(*tuningMasterTuneRelay)
        .withOptionsFrom(*tuningOctaveStretchRelay)
        .withOptionsFrom(*tuningPitchBendRangeRelay)
        .withOptionsFrom(*tuningTemperamentPresetRelay)
        .withOptionsFrom(*chorusRateRelay)
        .withOptionsFrom(*chorusDepthRelay)
        .withOptionsFrom(*chorusMixRelay)
        .withOptionsFrom(*delayTimeRelay)
        .withOptionsFrom(*delayFeedbackRelay)
        .withOptionsFrom(*delayModeRelay)
        .withOptionsFrom(*delayMixRelay)
        .withOptionsFrom(*eqLowGainRelay)
        .withOptionsFrom(*eqMidGainRelay)
        .withOptionsFrom(*eqMidFreqRelay)
        .withOptionsFrom(*eqHighGainRelay)
        .withOptionsFrom(*reverbSizeRelay)
        .withOptionsFrom(*reverbDampRelay)
        .withOptionsFrom(*reverbPredelayRelay)
        .withOptionsFrom(*reverbMixRelay)
        .withOptionsFrom(*chorusBypassRelay)
        .withOptionsFrom(*delayBypassRelay)
        .withOptionsFrom(*eqBypassRelay)
        .withOptionsFrom(*reverbBypassRelay)

        // --- Tuning Intervals ---
        .withNativeFunction("getTuningIntervals", [this](const juce::Array<juce::var>&, auto complete) {
            complete(doubleVectorToJSON(processorRef.getTuningEngine().getIntervals()));
        })

        .withNativeFunction("setTuningIntervals", [this](const juce::Array<juce::var>& args, auto complete) {
            if (args.size() >= 1) {
                auto jsonArray = juce::JSON::parse(args[0].toString());
                if (auto* arr = jsonArray.getArray()) {
                    std::vector<double> intervals;
                    for (const auto& val : *arr)
                        intervals.push_back(static_cast<double>(val));
                    processorRef.getTuningEngine().setCustomIntervals(intervals, "Custom");
                    processorRef.checkAndResetForScaleChange();
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

        // --- Temperament Presets ---
        .withNativeFunction("setTemperamentPreset", [this](const juce::Array<juce::var>& args, auto complete) {
            if (args.size() >= 1) {
                int preset = static_cast<int>(args[0]);
                processorRef.getTuningEngine().setBuiltInPreset(
                    static_cast<TuningEngine::BuiltInPreset>(preset));
                processorRef.checkAndResetForScaleChange();
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
                        if (success)
                            processorRef.checkAndResetForScaleChange();
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
                complete(doubleVectorToJSON(intervals));
                return;
            }
            complete(juce::var());
        })

        .withNativeFunction("generateHarmonicSeries", [](const juce::Array<juce::var>& args, auto complete) {
            if (args.size() >= 2) {
                int startHarmonic = static_cast<int>(args[0]);
                int endHarmonic = static_cast<int>(args[1]);
                auto intervals = ScaleGenerator::generateHarmonicSeries(startHarmonic, endHarmonic);
                complete(doubleVectorToJSON(intervals));
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
                complete(doubleVectorToJSON(intervals));
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
                    processorRef.checkAndResetForScaleChange();
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
                    auto intervals = tuning->intervals;
                    intervals.push_back(tuning->period);
                    processorRef.getTuningEngine().setCustomIntervals(
                        intervals, juce::String(tuning->name));
                    processorRef.checkAndResetForScaleChange();
                    complete(true);
                    return;
                }
            }
            complete(false);
        })

        // --- Enabled Intervals ---
        .withNativeFunction("getEnabledIntervals", [this](const juce::Array<juce::var>&, auto complete) {
            auto ei = processorRef.getEnabledIntervals();
            juce::String json = "[";
            for (size_t i = 0; i < ei.size(); ++i) {
                if (i > 0) json += ",";
                json += ei[i] ? "true" : "false";
            }
            json += "]";
            complete(json);
        })

        .withNativeFunction("setIntervalEnabled", [this](const juce::Array<juce::var>& args, auto complete) {
            if (args.size() >= 2) {
                int index = static_cast<int>(args[0]);
                bool enabled = static_cast<bool>(args[1]);
                processorRef.setIntervalEnabled(index, enabled);
                complete(true);
                return;
            }
            complete(false);
        })

        .withNativeFunction("resetEnabledIntervals", [this](const juce::Array<juce::var>&, auto complete) {
            processorRef.resetEnabledIntervals();
            complete(true);
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
                        auto html = TuningExporter::toHTML(processorRef.getTuningEngine(), "O-IntonationPad");
                        file.replaceWithText(html);
                        complete(true);
                    } else {
                        complete(false);
                    }
                });
        });
}

void OIntonationPadAudioProcessorEditor::createAttachments()
{
    auto& apvts = processorRef.getAPVTS();

    voiceCountAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("voiceCount"), *voiceCountRelay, nullptr);
    complexityAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("complexity"), *complexityRelay, nullptr);
    keyRootAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("keyRoot"), *keyRootRelay, nullptr);
    stereoSpreadAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("stereoSpread"), *stereoSpreadRelay, nullptr);
    spacingAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("spacing"), *spacingRelay, nullptr);
    inversionAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("inversion"), *inversionRelay, nullptr);
    wavetablePosAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("wavetablePos"), *wavetablePosRelay, nullptr);
    lfoRateAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("lfoRate"), *lfoRateRelay, nullptr);
    lfoDepthAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("lfoDepth"), *lfoDepthRelay, nullptr);
    timingRandomAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("timingRandom"), *timingRandomRelay, nullptr);
    detuneRandomAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("detuneRandom"), *detuneRandomRelay, nullptr);
    attackTimeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("attackTime"), *attackTimeRelay, nullptr);
    releaseTimeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("releaseTime"), *releaseTimeRelay, nullptr);
    filterCutoffAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("filterCutoff"), *filterCutoffRelay, nullptr);
    masterVolumeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("masterVolume"), *masterVolumeRelay, nullptr);

    wavetableBankAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *apvts.getParameter("wavetableBank"), *wavetableBankRelay, nullptr);

    wavetablePos2Attachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("wavetablePos2"), *wavetablePos2Relay, nullptr);
    gainAAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("gainA"), *gainARelay, nullptr);
    gainBAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("gainB"), *gainBRelay, nullptr);
    lfoRate2Attachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("lfoRate2"), *lfoRate2Relay, nullptr);
    lfoDepth2Attachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("lfoDepth2"), *lfoDepth2Relay, nullptr);
    wavetableBank2Attachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *apvts.getParameter("wavetableBank2"), *wavetableBank2Relay, nullptr);

    tuningMasterTuneAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("tuning_masterTune"), *tuningMasterTuneRelay, nullptr);
    tuningOctaveStretchAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("tuning_octaveStretch"), *tuningOctaveStretchRelay, nullptr);
    tuningPitchBendRangeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("tuning_pitchBendRange"), *tuningPitchBendRangeRelay, nullptr);
    tuningTemperamentPresetAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *apvts.getParameter("tuning_temperamentPreset"), *tuningTemperamentPresetRelay, nullptr);

    chorusRateAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("chorusRate"), *chorusRateRelay, nullptr);
    chorusDepthAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("chorusDepth"), *chorusDepthRelay, nullptr);
    chorusMixAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("chorusMix"), *chorusMixRelay, nullptr);
    delayTimeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("delayTime"), *delayTimeRelay, nullptr);
    delayFeedbackAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("delayFeedback"), *delayFeedbackRelay, nullptr);
    delayModeAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *apvts.getParameter("delayMode"), *delayModeRelay, nullptr);
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
    chorusBypassAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *apvts.getParameter("chorusBypass"), *chorusBypassRelay, nullptr);
    delayBypassAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *apvts.getParameter("delayBypass"), *delayBypassRelay, nullptr);
    eqBypassAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *apvts.getParameter("eqBypass"), *eqBypassRelay, nullptr);
    reverbBypassAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *apvts.getParameter("reverbBypass"), *reverbBypassRelay, nullptr);
}

OIntonationPadAudioProcessorEditor::~OIntonationPadAudioProcessorEditor()
{
    stopTimer();
}

void OIntonationPadAudioProcessorEditor::timerCallback()
{
    if (webView == nullptr)
        return;

    auto notes = processorRef.getActiveNotes();

    // Build JSON array of active notes
    juce::String json = "[";
    for (int i = 0; i < static_cast<int>(notes.size()); ++i)
    {
        if (i > 0) json += ",";

        const auto& n = notes[static_cast<size_t>(i)];
        int pitchClass = n.midiNote % 12;
        int octave = (n.midiNote / 12) - 1;

        // Calculate cent deviation from 12-TET
        double tetFreq = 440.0 * std::pow(2.0, (n.midiNote - 69) / 12.0);
        double centDev = 1200.0 * std::log2(static_cast<double>(n.frequencyHz) / tetFreq);

        json += "{\"midi\":" + juce::String(n.midiNote)
             + ",\"pc\":" + juce::String(pitchClass)
             + ",\"oct\":" + juce::String(octave)
             + ",\"hz\":" + juce::String(n.frequencyHz, 2)
             + ",\"cents\":" + juce::String(centDev, 1)
             + ",\"gain\":" + juce::String(n.gain, 3) + "}";
    }
    json += "]";

    webView->emitEventIfBrowserIsVisible("activeNotes", json);
}

void OIntonationPadAudioProcessorEditor::paint(juce::Graphics& g)
{
    // WebView handles all painting
    juce::ignoreUnused(g);
}

void OIntonationPadAudioProcessorEditor::resized()
{
    // WebView fills entire editor
    webView->setBounds(getLocalBounds());
}

void OIntonationPadAudioProcessorEditor::parentHierarchyChanged()
{
    // Navigate WebView only after editor is attached to a window
    if (isShowing() && webView != nullptr && !hasNavigated)
    {
        webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
        hasNavigated = true;
    }
}

// Pattern #8: EXPLICIT URL MAPPING
std::optional<juce::WebBrowserComponent::Resource>
OIntonationPadAudioProcessorEditor::getResource(const juce::String& url)
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

    // JUCE interop checker
    if (url == "/js/juce/check_native_interop.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::check_native_interop_js,
                      BinaryData::check_native_interop_jsSize),
            juce::String("text/javascript")
        };
    }

    // Pitch circle module (still used by voice tab)
    if (url == "/modules/pitch-circle.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::pitchcircle_js, BinaryData::pitchcircle_jsSize),
            juce::String("text/javascript")
        };
    }

    // v1.3.0: Tuning panel JS module
    if (url == "/js/tuning-panel.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::tuningpanel_js, BinaryData::tuningpanel_jsSize),
            juce::String("text/javascript")
        };
    }

    // v1.3.0: Tuning panel CSS
    if (url == "/css/tuning-panel.css") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::tuningpanel_css, BinaryData::tuningpanel_cssSize),
            juce::String("text/css")
        };
    }

    // Background image
    if (url == "/img/paper.jpg") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::paper_jpg, BinaryData::paper_jpgSize),
            juce::String("image/jpeg")
        };
    }

    // Shell botanical overlay
    if (url == "/img/shell.png") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::shell_png, BinaryData::shell_pngSize),
            juce::String("image/png")
        };
    }

    // Resource not found
    DBG("Resource not found: " + url);
    return std::nullopt;
}
