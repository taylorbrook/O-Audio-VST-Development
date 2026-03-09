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
#include "DSP/WavetableData.h"
#include "DSP/ScaleGenerator.h"
#include "DSP/TuningExporter.h"
#include "DSP/EmbeddedTunings.h"
#include "Util/JsonHelper.h"

OIntonationPadAudioProcessorEditor::OIntonationPadAudioProcessorEditor(OIntonationPadAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    createRelays();
    webView = std::make_unique<juce::WebBrowserComponent>(buildWebViewOptions());
    createAttachments();

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

    setSize(800, 500);
    startTimerHz(30);
}

void OIntonationPadAudioProcessorEditor::createRelays()
{
    voiceCountRelay = std::make_unique<juce::WebSliderRelay>("voiceCount");
    complexityRelay = std::make_unique<juce::WebSliderRelay>("complexity");
    keyRootRelay = std::make_unique<juce::WebSliderRelay>("keyRoot");
    voicingModeRelay = std::make_unique<juce::WebComboBoxRelay>("voicingMode");
    stereoSpreadRelay = std::make_unique<juce::WebSliderRelay>("stereoSpread");
    spacingRelay = std::make_unique<juce::WebSliderRelay>("spacing");
    inversionRelay = std::make_unique<juce::WebSliderRelay>("inversion");
    wavetablePosRelay = std::make_unique<juce::WebSliderRelay>("wavetablePos");
    lfoRateRelay = std::make_unique<juce::WebSliderRelay>("lfoRate");
    lfoDepthRelay = std::make_unique<juce::WebSliderRelay>("lfoDepth");
    timingRandomRelay = std::make_unique<juce::WebSliderRelay>("timingRandom");
    detuneRandomRelay = std::make_unique<juce::WebSliderRelay>("detuneRandom");
    attackTimeRelay = std::make_unique<juce::WebSliderRelay>("attackTime");
    decayTimeRelay = std::make_unique<juce::WebSliderRelay>("decayTime");
    sustainLevelRelay = std::make_unique<juce::WebSliderRelay>("sustainLevel");
    releaseTimeRelay = std::make_unique<juce::WebSliderRelay>("releaseTime");
    filterCutoffRelay = std::make_unique<juce::WebSliderRelay>("filterCutoff");
    filterLfoDepthRelay = std::make_unique<juce::WebSliderRelay>("filterLfoDepth");
    velocityToFilterRelay = std::make_unique<juce::WebSliderRelay>("velocityToFilter");
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
        .withOptionsFrom(*voicingModeRelay)
        .withOptionsFrom(*stereoSpreadRelay)
        .withOptionsFrom(*spacingRelay)
        .withOptionsFrom(*inversionRelay)
        .withOptionsFrom(*wavetablePosRelay)
        .withOptionsFrom(*lfoRateRelay)
        .withOptionsFrom(*lfoDepthRelay)
        .withOptionsFrom(*timingRandomRelay)
        .withOptionsFrom(*detuneRandomRelay)
        .withOptionsFrom(*attackTimeRelay)
        .withOptionsFrom(*decayTimeRelay)
        .withOptionsFrom(*sustainLevelRelay)
        .withOptionsFrom(*releaseTimeRelay)
        .withOptionsFrom(*filterCutoffRelay)
        .withOptionsFrom(*filterLfoDepthRelay)
        .withOptionsFrom(*velocityToFilterRelay)
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
            complete(JsonHelper::arrayToJSON(processorRef.getTuningEngine().getIntervals()));
        })

        .withNativeFunction("setTuningIntervals", [this](const juce::Array<juce::var>& args, auto complete) {
            if (args.size() >= 1) {
                if (parseAndApplyIntervals(args[0].toString(), "Custom")) {
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
                complete(JsonHelper::arrayToJSON(intervals));
                return;
            }
            complete(juce::var());
        })

        .withNativeFunction("generateHarmonicSeries", [](const juce::Array<juce::var>& args, auto complete) {
            if (args.size() >= 2) {
                int startHarmonic = static_cast<int>(args[0]);
                int endHarmonic = static_cast<int>(args[1]);
                auto intervals = ScaleGenerator::generateHarmonicSeries(startHarmonic, endHarmonic);
                complete(JsonHelper::arrayToJSON(intervals));
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
                complete(JsonHelper::arrayToJSON(intervals));
                return;
            }
            complete(juce::var());
        })

        .withNativeFunction("applyGeneratedScale", [this](const juce::Array<juce::var>& args, auto complete) {
            if (args.size() >= 2) {
                if (parseAndApplyIntervals(args[0].toString(), args[1].toString())) {
                    complete(true);
                    return;
                }
            }
            complete(false);
        })

        // --- Embedded Tuning Library ---
        .withNativeFunction("getEmbeddedTuningList", [](const juce::Array<juce::var>&, auto complete) {
            const auto& tunings = EmbeddedTunings::getAllTunings();
            JsonHelper::JsonArrayBuilder arr;
            for (const auto& t : tunings) {
                arr.add(JsonHelper::JsonObjectBuilder{}
                    .add("id", juce::String(t.id))
                    .add("name", juce::String(t.name))
                    .add("category", juce::String(t.category))
                    .add("noteCount", static_cast<int>(t.intervals.size())));
            }
            complete(arr.build());
        })

        .withNativeFunction("getEmbeddedTuningCategories", [](const juce::Array<juce::var>&, auto complete) {
            complete(JsonHelper::arrayToJSON(EmbeddedTunings::getCategories()));
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
            complete(JsonHelper::arrayToJSON(processorRef.getEnabledIntervals()));
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

        // --- Wavetable Frame Data (for UI waveform display) ---
        .withNativeFunction("getWavetableFrameForPosition", [](const juce::Array<juce::var>& args, auto complete) {
            if (args.size() >= 2)
            {
                int bankIndex = juce::jlimit(0, WavetableData::NUM_BANKS - 1, static_cast<int>(args[0]));
                float normalizedPos = juce::jlimit(0.0f, 1.0f, static_cast<float>(args[1]));

                const auto& bank = WavetableData::BankCache::getBank(bankIndex);
                int frameIndex = juce::jlimit(0, WavetableData::NUM_FRAMES - 1,
                    static_cast<int>(normalizedPos * (WavetableData::NUM_FRAMES - 1)));

                // Read from mipmap level 0 (full bandwidth) for display
                const auto& frame = bank[0][static_cast<size_t>(frameIndex)];

                // Build JSON array with stride=8 for 256 display points
                juce::String json = "[";
                for (int i = 0; i < WavetableData::SAMPLES_PER_FRAME; i += 8)
                {
                    if (i > 0) json += ",";
                    json += juce::String(frame[static_cast<size_t>(i)], 4);
                }
                json += "]";
                complete(json);
                return;
            }
            complete(juce::var());
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
        })

        // --- v2.6.0: Parameter Randomization ---
        .withNativeFunction("randomizeParameters", [this](const juce::Array<juce::var>& args, auto complete) {
            if (args.isEmpty()) { complete(false); return; }
            juce::String mode = args[0].toString();
            auto& apvts = processorRef.getAPVTS();
            juce::Random rng;

            // Parameters preserved in Sound-Only mode (tuning, chord, master volume)
            static const juce::StringArray preservedInSoundOnly {
                "keyRoot", "voicingMode", "voiceCount", "complexity",
                "tuning_masterTune", "tuning_octaveStretch", "tuning_pitchBendRange",
                "tuning_temperamentPreset", "masterVolume", "stereoSpread"
            };

            // Musically constrained ranges: paramID -> {min, max} as normalized 0-1
            // These bias randomization toward useful values
            struct ConstrainedRange { float min; float max; };
            std::map<juce::String, ConstrainedRange> constraints;
            constraints["filterCutoff"]    = { 0.15f, 0.85f };  // ~200Hz - 12kHz (skewed)
            constraints["attackTime"]      = { 0.05f, 0.70f };  // ~10ms - 2s (skewed)
            constraints["releaseTime"]     = { 0.10f, 0.75f };  // ~50ms - 5s (skewed)
            constraints["decayTime"]       = { 0.05f, 0.60f };  // ~20ms - 1.5s (skewed)
            constraints["sustainLevel"]    = { 0.20f, 1.00f };  // 20% - 100%
            constraints["chorusMix"]       = { 0.00f, 0.60f };  // 0% - 60%
            constraints["delayMix"]        = { 0.00f, 0.50f };  // 0% - 50%
            constraints["delayFeedback"]   = { 0.00f, 0.65f };  // 0% - ~62% (of 0.95 max)
            constraints["reverbMix"]       = { 0.00f, 0.65f };  // 0% - 65%
            constraints["reverbPredelay"]  = { 0.00f, 0.50f };  // 0ms - 100ms (of 200 max)
            constraints["lfoRate"]         = { 0.05f, 0.55f };  // ~0.1Hz - 5Hz (skewed)
            constraints["lfoRate2"]        = { 0.05f, 0.55f };
            constraints["chorusRate"]      = { 0.10f, 0.60f };  // ~0.5Hz - 4Hz (skewed)

            auto randomizeParam = [&](const juce::String& paramID) {
                auto* param = apvts.getParameter(paramID);
                if (param == nullptr) return;

                float currentNorm = param->getValue();  // normalized 0-1
                float newNorm;

                if (mode == "gentle")
                {
                    // ±15% variation from current value
                    float variation = (rng.nextFloat() * 0.30f) - 0.15f;
                    newNorm = juce::jlimit(0.0f, 1.0f, currentNorm + variation);
                }
                else  // "wild" or "soundOnly"
                {
                    // Full range, but constrained to musical values if defined
                    auto it = constraints.find(paramID);
                    if (it != constraints.end())
                        newNorm = it->second.min + rng.nextFloat() * (it->second.max - it->second.min);
                    else
                        newNorm = rng.nextFloat();
                }

                param->setValueNotifyingHost(newNorm);
            };

            // Iterate all APVTS parameters
            for (int i = 0; i < apvts.processor.getParameters().size(); ++i)
            {
                auto* param = apvts.processor.getParameters()[i];
                auto* rpwdi = dynamic_cast<juce::RangedAudioParameter*>(param);
                if (rpwdi == nullptr) continue;

                juce::String paramID = rpwdi->getParameterID();

                // Skip preserved params in Sound-Only mode
                if (mode == "soundOnly" && preservedInSoundOnly.contains(paramID))
                    continue;

                // Skip bypass toggles in gentle mode (don't randomly enable effects)
                if (mode == "gentle" && paramID.containsIgnoreCase("Bypass"))
                    continue;

                randomizeParam(paramID);
            }

            complete(true);
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
    voicingModeAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *apvts.getParameter("voicingMode"), *voicingModeRelay, nullptr);
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
    decayTimeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("decayTime"), *decayTimeRelay, nullptr);
    sustainLevelAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("sustainLevel"), *sustainLevelRelay, nullptr);
    releaseTimeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("releaseTime"), *releaseTimeRelay, nullptr);
    filterCutoffAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("filterCutoff"), *filterCutoffRelay, nullptr);
    filterLfoDepthAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("filterLfoDepth"), *filterLfoDepthRelay, nullptr);
    velocityToFilterAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("velocityToFilter"), *velocityToFilterRelay, nullptr);
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
#if OUARICON_LICENSING_ENABLED
    processorRef.getLicenseManager().removeListener(this);
#endif
    stopTimer();
}

void OIntonationPadAudioProcessorEditor::timerCallback()
{
    if (webView == nullptr)
        return;

    auto notes = processorRef.getActiveNotes();

    JsonHelper::JsonArrayBuilder arr;
    for (const auto& n : notes)
    {
        int pitchClass = n.midiNote % 12;
        int octave = (n.midiNote / 12) - 1;
        double tetFreq = 440.0 * std::pow(2.0, (n.midiNote - 69) / 12.0);
        double centDev = 1200.0 * std::log2(static_cast<double>(n.frequencyHz) / tetFreq);

        arr.add(JsonHelper::JsonObjectBuilder{}
            .add("midi", n.midiNote)
            .add("pc", pitchClass)
            .add("oct", octave)
            .add("hz", static_cast<double>(n.frequencyHz))
            .add("cents", centDev, 1)
            .add("gain", static_cast<double>(n.gain), 3));
    }

    webView->emitEventIfBrowserIsVisible("activeNotes", arr.build());

    // v2.4.1: Emit LFO-modulated wavetable positions for waveform display animation
    webView->emitEventIfBrowserIsVisible("wavetableModPos",
        JsonHelper::JsonObjectBuilder{}
            .add("posA", static_cast<double>(processorRef.getModulatedPosA()), 4)
            .add("posB", static_cast<double>(processorRef.getModulatedPosB()), 4)
            .build());
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

#if OUARICON_LICENSING_ENABLED
    if (licenseOverlay != nullptr)
        licenseOverlay->setBounds(getLocalBounds());
#endif
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

bool OIntonationPadAudioProcessorEditor::parseAndApplyIntervals(
    const juce::String& jsonStr, const juce::String& name)
{
    auto jsonArray = juce::JSON::parse(jsonStr);
    if (auto* arr = jsonArray.getArray())
    {
        std::vector<double> intervals;
        intervals.reserve(static_cast<size_t>(arr->size()));
        for (const auto& val : *arr)
            intervals.push_back(static_cast<double>(val));
        processorRef.getTuningEngine().setCustomIntervals(intervals, name);
        processorRef.checkAndResetForScaleChange();
        return true;
    }
    return false;
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

    // Shared constants module
    if (url == "/js/constants.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::constants_js, BinaryData::constants_jsSize),
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

#if OUARICON_LICENSING_ENABLED
//==============================================================================
void OIntonationPadAudioProcessorEditor::licenseStatusChanged(
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
