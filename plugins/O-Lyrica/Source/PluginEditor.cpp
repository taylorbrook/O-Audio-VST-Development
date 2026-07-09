/*
  ==============================================================================

    O-Lyrica - Editor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginEditor.h"
#include "BinaryData.h"
#include "DSP/ScaleGenerator.h"
#include "DSP/EmbeddedTunings.h"
#include "DSP/TuningExporter.h"

OLyricaAudioProcessorEditor::OLyricaAudioProcessorEditor(OLyricaAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    // ═══════════════════════════════════════════════════════════════════
    // Phase 3.1: WebView Setup
    // ═══════════════════════════════════════════════════════════════════

    // 1️⃣ CREATE RELAYS (must be created BEFORE WebView)
    masterVolumeRelay = std::make_unique<juce::WebSliderRelay>("masterVolume");
    brightnessRelay = std::make_unique<juce::WebSliderRelay>("brightness");
    timbreRelay = std::make_unique<juce::WebSliderRelay>("timbre");           // v1.1.0: renamed from sustain
    decayTimeRelay = std::make_unique<juce::WebSliderRelay>("decayTime");     // v1.1.0: new parameter
    bodySizeRelay = std::make_unique<juce::WebSliderRelay>("bodySize");
    bodyResonanceRelay = std::make_unique<juce::WebSliderRelay>("bodyResonance");
    sympatheticAmountRelay = std::make_unique<juce::WebSliderRelay>("sympatheticAmount");
    pluckPositionRelay = std::make_unique<juce::WebSliderRelay>("pluckPosition");
    fingerHardnessRelay = std::make_unique<juce::WebSliderRelay>("fingerHardness");
    stringTensionRelay = std::make_unique<juce::WebSliderRelay>("stringTension");
    stringGaugeRelay = std::make_unique<juce::WebSliderRelay>("stringGauge");
    stringLengthRelay = std::make_unique<juce::WebSliderRelay>("stringLength");
    stringStiffnessRelay = std::make_unique<juce::WebSliderRelay>("stringStiffness");
    masterTuneRelay = std::make_unique<juce::WebSliderRelay>("masterTune");
    pitchBendRangeRelay = std::make_unique<juce::WebSliderRelay>("pitchBendRange");
    // v1.9.0: Octave stretch relay
    octaveStretchRelay = std::make_unique<juce::WebSliderRelay>("octaveStretch");
    // v1.4.0: New parameters from v1.3.0
    attackNoiseRelay = std::make_unique<juce::WebSliderRelay>("attackNoise");
    sympatheticQRelay = std::make_unique<juce::WebSliderRelay>("sympatheticQ");
    bodyModeSpreadRelay = std::make_unique<juce::WebSliderRelay>("bodyModeSpread");
    bridgeBrightnessRelay = std::make_unique<juce::WebSliderRelay>("bridgeBrightness");
    // v1.19.0: Humanize (per-note randomization)
    humanizeRelay = std::make_unique<juce::WebSliderRelay>("humanize");
    // v1.21.0: Glissando speed (notes per second)
    glissandoSpeedRelay = std::make_unique<juce::WebSliderRelay>("glissandoSpeed");
    // v1.23.0: Glissando custom semitones
    glissandoCustomSemitonesRelay = std::make_unique<juce::WebSliderRelay>("glissandoCustomSemitones");
    // v1.24.0: Glissando humanize (timing jitter)
    glissandoHumanizeRelay = std::make_unique<juce::WebSliderRelay>("glissandoHumanize");
    // v1.25.0: Glissando time (Free mode ramp duration)
    glissandoTimeRelay = std::make_unique<juce::WebSliderRelay>("glissandoTime");
    // v1.26.0: Glissando excitation softness
    glissandoExcitationRelay = std::make_unique<juce::WebSliderRelay>("glissandoExcitation");
    // v1.27.0: Glissando velocity profile
    glissandoVelStartRelay = std::make_unique<juce::WebSliderRelay>("glissandoVelStart");
    glissandoVelEndRelay = std::make_unique<juce::WebSliderRelay>("glissandoVelEnd");

    stringMaterialRelay = std::make_unique<juce::WebComboBoxRelay>("stringMaterial");
    woodTypeRelay = std::make_unique<juce::WebComboBoxRelay>("woodType");
    techniqueRelay = std::make_unique<juce::WebComboBoxRelay>("technique");
    glissandoScaleRelay = std::make_unique<juce::WebComboBoxRelay>("glissandoScale");
    // v1.22.0: Glissando shape relay
    glissandoShapeRelay = std::make_unique<juce::WebComboBoxRelay>("glissandoShape");
    // v1.30.0: Glissando tonic relay
    glissandoTonicRelay = std::make_unique<juce::WebComboBoxRelay>("glissandoTonic");
    // v1.23.0: Glissando interval and direction relays
    glissandoIntervalRelay = std::make_unique<juce::WebComboBoxRelay>("glissandoInterval");
    glissandoDirectionRelay = std::make_unique<juce::WebComboBoxRelay>("glissandoDirection");
    // v1.6.0: Tuning mode relay
    tuningModeRelay = std::make_unique<juce::WebComboBoxRelay>("tuningMode");
    // v1.30.0: Glissando toggle relays
    freeToggleRelay = std::make_unique<juce::WebToggleButtonRelay>("freeToggle");
    scaleToggleRelay = std::make_unique<juce::WebToggleButtonRelay>("scaleToggle");
    // v1.30.0: Free mode parameter relays
    freeShapeRelay = std::make_unique<juce::WebComboBoxRelay>("freeShape");
    freeIntervalRelay = std::make_unique<juce::WebComboBoxRelay>("freeInterval");
    freeDirectionRelay = std::make_unique<juce::WebComboBoxRelay>("freeDirection");
    freeKeyswitchNoteRelay = std::make_unique<juce::WebComboBoxRelay>("freeKeyswitchNote");
    scaleKeyswitchNoteRelay = std::make_unique<juce::WebComboBoxRelay>("scaleKeyswitchNote");
    freeCustomSemitonesRelay = std::make_unique<juce::WebSliderRelay>("freeCustomSemitones");
    // v1.31.0: Tempo sync relays
    freeTempoSyncRelay = std::make_unique<juce::WebComboBoxRelay>("freeTempoSync");
    scaleTempoSyncRelay = std::make_unique<juce::WebComboBoxRelay>("scaleTempoSync");
    // v1.35.1: Effects chain relays
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

    // 2️⃣ CREATE WEBVIEW with all relays registered
    // v1.18.3: Added section comments for native function organization
    webView = std::make_unique<juce::WebBrowserComponent>(
        juce::WebBrowserComponent::Options{}
            .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
            .withWinWebView2Options(
                juce::WebBrowserComponent::Options::WinWebView2{}
                    .withUserDataFolder(juce::File::getSpecialLocation(
                        juce::File::SpecialLocationType::tempDirectory)))
            .withNativeIntegrationEnabled()
            .withResourceProvider([this](const juce::String& url) {
                return getResource(url);
            })
            // ─────────────────────────────────────────────────────────────────
            // VOICE COUNT
            // ─────────────────────────────────────────────────────────────────
            .withNativeFunction("getVoiceCount", [this](const juce::Array<juce::var>&,
                                                         std::function<void(juce::var)> complete) {
                complete(juce::var(processorRef.getActiveVoiceCount()));
            })
            // ─────────────────────────────────────────────────────────────────
            // PRESET MANAGEMENT (v1.5.0)
            // ─────────────────────────────────────────────────────────────────
            .withNativeFunction("savePreset", [this](const juce::Array<juce::var>& args,
                                                      std::function<void(juce::var)> complete) {
                if (args.isEmpty()) { complete(juce::var(false)); return; }
                auto name = args[0].toString();
                auto success = processorRef.getPresetManager().savePreset(name);
                complete(juce::var(success));
            })
            .withNativeFunction("loadPreset", [this](const juce::Array<juce::var>& args,
                                                      std::function<void(juce::var)> complete) {
                if (args.isEmpty()) { complete(juce::var(false)); return; }
                auto name = args[0].toString();
                auto success = processorRef.getPresetManager().loadPreset(name);
                complete(juce::var(success));
            })
            .withNativeFunction("getPresetList", [this](const juce::Array<juce::var>&,
                                                         std::function<void(juce::var)> complete) {
                auto presetList = processorRef.getPresetManager().getPresetList();
                juce::Array<juce::var> result;
                for (const auto& presetName : presetList)
                    result.add(juce::var(presetName));
                complete(juce::var(result));
            })
            .withNativeFunction("getCurrentPreset", [this](const juce::Array<juce::var>&,
                                                            std::function<void(juce::var)> complete) {
                complete(juce::var(processorRef.getPresetManager().getCurrentPresetName()));
            })
            .withNativeFunction("selectNextPreset", [this](const juce::Array<juce::var>&,
                                                            std::function<void(juce::var)> complete) {
                auto& pm = processorRef.getPresetManager();
                juce::String nextPreset = pm.getNextPreset();
                if (pm.loadPreset(nextPreset))
                {
                    complete(juce::var(nextPreset));
                }
                else
                {
                    complete(juce::var(pm.getCurrentPresetName()));
                }
            })
            .withNativeFunction("selectPreviousPreset", [this](const juce::Array<juce::var>&,
                                                                std::function<void(juce::var)> complete) {
                auto& pm = processorRef.getPresetManager();
                juce::String prevPreset = pm.getPreviousPreset();
                if (pm.loadPreset(prevPreset))
                {
                    complete(juce::var(prevPreset));
                }
                else
                {
                    complete(juce::var(pm.getCurrentPresetName()));
                }
            })
            // WR-07: expose each parameter's NORMALISED default so the WebView double-click-reset can
            // restore it. The JUCE WebSlider propertiesChanged payload carries no defaultValue field,
            // so JS previously reset to `undefined` → NaN. Returns { paramID: normalisedDefault }.
            .withNativeFunction("getParameterDefaults", [this](const juce::Array<juce::var>&,
                                                               std::function<void(juce::var)> complete) {
                auto* obj = new juce::DynamicObject();
                for (auto* rap : processorRef.getParameters())
                    if (auto* withID = dynamic_cast<juce::AudioProcessorParameterWithID*>(rap))
                        obj->setProperty(withID->paramID, rap->getDefaultValue());
                complete(juce::var(obj));
            })
            // v1.5.1: File dialog functions for Save/Load buttons
            // v1.7.10 FIX: Capture shared_ptr in lambda to prevent dangling reference
            .withNativeFunction("savePresetWithDialog", [this](const juce::Array<juce::var>&,
                                                                std::function<void(juce::var)> complete) {
                auto& pm = processorRef.getPresetManager();
                auto userDir = pm.getUserPresetsDirectory();
                userDir.createDirectory();

                auto chooser = std::make_shared<juce::FileChooser>(
                    "Save Preset",
                    userDir,
                    "*.json"
                );
                fileChooser = chooser;  // Store reference for potential future use

                chooser->launchAsync(
                    juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                    [this, safe = juce::Component::SafePointer<OLyricaAudioProcessorEditor>(this), chooser, complete](const juce::FileChooser& fc) {
                        // CR-01: if the editor/WebView was torn down while the dialog was open, bail
                        // with a bare return. Do NOT call complete() — it is owned by the destroyed
                        // WebBrowserComponent impl, so invoking it is itself a use-after-free.
                        if (safe == nullptr)
                            return;

                        auto result = fc.getResult();
                        if (result == juce::File{})
                        {
                            complete(juce::var()); // User cancelled
                            return;
                        }

                        auto presetName = result.getFileNameWithoutExtension();
                        if (processorRef.getPresetManager().savePreset(presetName))
                        {
                            complete(juce::var(presetName));
                        }
                        else
                        {
                            complete(juce::var()); // Save failed
                        }
                    }
                );
            })
            .withNativeFunction("loadPresetFromFile", [this](const juce::Array<juce::var>&,
                                                              std::function<void(juce::var)> complete) {
                auto& pm = processorRef.getPresetManager();
                auto presetsDir = pm.getPresetsDirectory();

                auto chooser = std::make_shared<juce::FileChooser>(
                    "Load Preset",
                    presetsDir,
                    "*.json"
                );
                fileChooser = chooser;

                chooser->launchAsync(
                    juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                    [this, safe = juce::Component::SafePointer<OLyricaAudioProcessorEditor>(this), chooser, complete](const juce::FileChooser& fc) {
                        if (safe == nullptr)  // CR-01: editor torn down while dialog open — bare return, no complete()
                            return;

                        auto result = fc.getResult();
                        if (result == juce::File{})
                        {
                            complete(juce::var()); // User cancelled
                            return;
                        }

                        if (processorRef.getPresetManager().loadPresetFromFile(result))
                        {
                            complete(juce::var(result.getFileNameWithoutExtension()));
                        }
                        else
                        {
                            complete(juce::var()); // Load failed
                        }
                    }
                );
            })
            // ─────────────────────────────────────────────────────────────────
            // TUNING SYSTEM (v1.6.0+)
            // ─────────────────────────────────────────────────────────────────
            .withNativeFunction("getTuningIntervals", [this](const juce::Array<juce::var>&,
                                                              std::function<void(juce::var)> complete) {
                auto intervals = processorRef.getTuningEngine()->getIntervals();
                juce::Array<juce::var> result;
                for (double cents : intervals)
                    result.add(juce::var(cents));
                complete(juce::var(result));
            })
            .withNativeFunction("setTuningIntervals", [this](const juce::Array<juce::var>& args,
                                                              std::function<void(juce::var)> complete) {
                if (args.size() < 2) { complete(juce::var(false)); return; }

                auto intervalsVar = args[0];
                auto name = args[1].toString();

                if (!intervalsVar.isArray()) { complete(juce::var(false)); return; }

                std::vector<double> intervals;
                for (int i = 0; i < intervalsVar.size(); ++i)
                    intervals.push_back(static_cast<double>(intervalsVar[i]));

                processorRef.getTuningEngine()->setCustomIntervals(intervals, name);

                // v1.11.1: Also update APVTS tuningMode to Custom (1) so processBlock doesn't override
                if (auto* param = processorRef.getAPVTS().getParameter("tuningMode"))
                    param->setValueNotifyingHost(param->convertTo0to1(1.0f)); // 1 = Custom

                complete(juce::var(true));
            })
            .withNativeFunction("getTuningName", [this](const juce::Array<juce::var>&,
                                                         std::function<void(juce::var)> complete) {
                complete(juce::var(processorRef.getTuningEngine()->getActiveTuningName()));
            })
            .withNativeFunction("setTonicNote", [this](const juce::Array<juce::var>& args,
                                                        std::function<void(juce::var)> complete) {
                if (args.isEmpty()) { complete(juce::var(false)); return; }
                int tonic = static_cast<int>(args[0]);
                processorRef.getTuningEngine()->setTonicNote(tonic);
                complete(juce::var(true));
            })
            // v1.11.1: Set a single interval by index (simpler than passing full array)
            .withNativeFunction("setSingleInterval", [this](const juce::Array<juce::var>& args,
                                                             std::function<void(juce::var)> complete) {
                if (args.size() < 2) { complete(juce::var(false)); return; }

                int index = static_cast<int>(args[0]);
                double cents = static_cast<double>(args[1]);

                processorRef.getTuningEngine()->setSingleInterval(index, cents);

                // Also update APVTS tuningMode to Custom (1)
                if (auto* param = processorRef.getAPVTS().getParameter("tuningMode"))
                    param->setValueNotifyingHost(param->convertTo0to1(1.0f));

                complete(juce::var(true));
            })
            // v1.11.1: Encoded version - single int: index * 10000 + cents
            .withNativeFunction("setSingleIntervalEncoded", [this](const juce::Array<juce::var>& args,
                                                                    std::function<void(juce::var)> complete) {
                if (args.isEmpty()) {
                    complete(juce::var(false));
                    return;
                }

                int encoded = static_cast<int>(args[0]);
                int index = encoded / 10000;
                double cents = static_cast<double>(encoded % 10000);

                processorRef.getTuningEngine()->setSingleInterval(index, cents);

                // Also update APVTS tuningMode to Custom (1)
                // For AudioParameterChoice with 3 options, index 1 = normalized 0.5
                if (auto* param = processorRef.getAPVTS().getParameter("tuningMode"))
                    param->setValueNotifyingHost(0.5f);

                complete(juce::var(true));
            })
            .withNativeFunction("getTonicNote", [this](const juce::Array<juce::var>&,
                                                        std::function<void(juce::var)> complete) {
                complete(juce::var(processorRef.getTuningEngine()->getTonicNote()));
            })
            .withNativeFunction("loadScalaFile", [this](const juce::Array<juce::var>&,
                                                         std::function<void(juce::var)> complete) {
                auto chooser = std::make_shared<juce::FileChooser>(
                    "Load Scala File",
                    juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
                    "*.scl"
                );
                fileChooser = chooser;

                chooser->launchAsync(
                    juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                    [this, safe = juce::Component::SafePointer<OLyricaAudioProcessorEditor>(this), chooser, complete](const juce::FileChooser& fc) {
                        if (safe == nullptr)  // CR-01: editor torn down while dialog open — bare return, no complete()
                            return;

                        auto result = fc.getResult();
                        if (result == juce::File{})
                        {
                            complete(juce::var()); // User cancelled
                            return;
                        }

                        if (processorRef.getTuningEngine()->loadScalaFile(result))
                        {
                            // v1.7.4: Also update APVTS parameter so processBlock uses Scala mode
                            // Without this, processBlock overwrites TuningEngine mode every block
                            if (auto* param = processorRef.getAPVTS().getParameter("tuningMode"))
                                param->setValueNotifyingHost(1.0f / 2.0f); // Index 1 = Custom/Scala (normalized: 1/2 = 0.5)

                            // v1.13.3: Also update temperamentPreset to Custom (10) for state persistence
                            // Without this, temperamentPreset stays at old value and may cause issues on reload
                            if (auto* presetParam = processorRef.getAPVTS().getParameter("temperamentPreset"))
                                presetParam->setValueNotifyingHost(10.0f / 10.0f); // Index 10 = Custom (normalized: 10/10 = 1.0)

                            complete(juce::var(processorRef.getTuningEngine()->getActiveTuningName()));
                        }
                        else
                        {
                            complete(juce::var()); // Load failed
                        }
                    }
                );
            })
            .withNativeFunction("loadKBMFile", [this](const juce::Array<juce::var>&,
                                                       std::function<void(juce::var)> complete) {
                auto chooser = std::make_shared<juce::FileChooser>(
                    "Load Keyboard Mapping",
                    juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
                    "*.kbm"
                );
                fileChooser = chooser;

                chooser->launchAsync(
                    juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                    [this, safe = juce::Component::SafePointer<OLyricaAudioProcessorEditor>(this), chooser, complete](const juce::FileChooser& fc) {
                        if (safe == nullptr)  // CR-01: editor torn down while dialog open — bare return, no complete()
                            return;

                        auto result = fc.getResult();
                        if (result == juce::File{})
                        {
                            complete(juce::var()); // User cancelled
                            return;
                        }

                        if (processorRef.getTuningEngine()->loadKBMFile(result))
                        {
                            complete(juce::var(true));
                        }
                        else
                        {
                            complete(juce::var()); // Load failed
                        }
                    }
                );
            })
            .withNativeFunction("saveScalaFile", [this](const juce::Array<juce::var>&,
                                                         std::function<void(juce::var)> complete) {
                auto content = processorRef.getTuningEngine()->generateScalaFileContent();
                auto name = processorRef.getTuningEngine()->getActiveTuningName();

                auto chooser = std::make_shared<juce::FileChooser>(
                    "Save Scala File",
                    juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile(name + ".scl"),
                    "*.scl"
                );
                fileChooser = chooser;

                chooser->launchAsync(
                    juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                    [safe = juce::Component::SafePointer<OLyricaAudioProcessorEditor>(this), chooser, content, complete](const juce::FileChooser& fc) {
                        if (safe == nullptr)  // CR-01: editor torn down while dialog open — bare return, no complete()
                            return;

                        auto result = fc.getResult();
                        if (result == juce::File{})
                        {
                            complete(juce::var()); // User cancelled
                            return;
                        }

                        auto file = result.hasFileExtension(".scl") ? result : result.withFileExtension(".scl");
                        if (file.replaceWithText(content))
                        {
                            complete(juce::var(file.getFileName()));
                        }
                        else
                        {
                            complete(juce::var()); // Save failed
                        }
                    }
                );
            })
            .withNativeFunction("saveKBMFile", [this](const juce::Array<juce::var>&,
                                                       std::function<void(juce::var)> complete) {
                auto content = processorRef.getTuningEngine()->generateKBMFileContent();
                auto name = processorRef.getTuningEngine()->getActiveTuningName();

                auto chooser = std::make_shared<juce::FileChooser>(
                    "Save Keyboard Mapping",
                    juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile(name + ".kbm"),
                    "*.kbm"
                );
                fileChooser = chooser;

                chooser->launchAsync(
                    juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                    [safe = juce::Component::SafePointer<OLyricaAudioProcessorEditor>(this), chooser, content, complete](const juce::FileChooser& fc) {
                        if (safe == nullptr)  // CR-01: editor torn down while dialog open — bare return, no complete()
                            return;

                        auto result = fc.getResult();
                        if (result == juce::File{})
                        {
                            complete(juce::var()); // User cancelled
                            return;
                        }

                        auto file = result.hasFileExtension(".kbm") ? result : result.withFileExtension(".kbm");
                        if (file.replaceWithText(content))
                        {
                            complete(juce::var(file.getFileName()));
                        }
                        else
                        {
                            complete(juce::var()); // Save failed
                        }
                    }
                );
            })
            // ─────────────────────────────────────────────────────────────────
            // TEMPERAMENT PRESETS (v1.9.0)
            // ─────────────────────────────────────────────────────────────────
            .withNativeFunction("setTemperamentPreset", [this](const juce::Array<juce::var>& args,
                                                                std::function<void(juce::var)> complete) {
                if (args.isEmpty()) { complete(juce::var(false)); return; }
                int presetIndex = static_cast<int>(args[0]);
                processorRef.getTuningEngine()->setBuiltInPreset(
                    static_cast<TuningEngine::BuiltInPreset>(presetIndex));

                // Update APVTS to keep in sync
                if (auto* param = processorRef.getAPVTS().getParameter("temperamentPreset"))
                {
                    // AudioParameterChoice: normalized value = index / (numItems - 1)
                    param->setValueNotifyingHost(static_cast<float>(presetIndex) / 10.0f);
                }

                // Also update tuning mode parameter to Custom for non-12TET presets
                if (presetIndex > 0 && presetIndex < 10)  // Presets 1-9 are custom temperaments
                {
                    if (auto* modeParam = processorRef.getAPVTS().getParameter("tuningMode"))
                        modeParam->setValueNotifyingHost(1.0f / 2.0f);  // Index 1 = Custom
                }
                else if (presetIndex == 0)  // Equal 12-TET
                {
                    if (auto* modeParam = processorRef.getAPVTS().getParameter("tuningMode"))
                        modeParam->setValueNotifyingHost(0.0f);  // Index 0 = 12-TET
                }

                complete(juce::var(true));
            })
            .withNativeFunction("getTemperamentPreset", [this](const juce::Array<juce::var>&,
                                                                std::function<void(juce::var)> complete) {
                int presetIndex = static_cast<int>(processorRef.getTuningEngine()->getBuiltInPreset());
                complete(juce::var(presetIndex));
            })
            .withNativeFunction("getOctaveStretch", [this](const juce::Array<juce::var>&,
                                                           std::function<void(juce::var)> complete) {
                float stretch = processorRef.getTuningEngine()->getOctaveStretch();
                complete(juce::var(stretch));
            })
            .withNativeFunction("setOctaveStretch", [this](const juce::Array<juce::var>& args,
                                                           std::function<void(juce::var)> complete) {
                if (args.isEmpty()) { complete(juce::var(false)); return; }
                float stretch = static_cast<float>(args[0]);
                processorRef.getTuningEngine()->setOctaveStretch(stretch);

                // Update APVTS to keep in sync
                if (auto* param = processorRef.getAPVTS().getParameter("octaveStretch"))
                {
                    // Normalize: (value - min) / (max - min) = (stretch - 0.95) / 0.3
                    float normalized = (stretch - 0.95f) / 0.3f;
                    param->setValueNotifyingHost(juce::jlimit(0.0f, 1.0f, normalized));
                }

                complete(juce::var(true));
            })
            // ─────────────────────────────────────────────────────────────────
            // SCALE GENERATORS (v1.14.0)
            // ─────────────────────────────────────────────────────────────────
            .withNativeFunction("generateEDO", [](const juce::Array<juce::var>& args,
                                                       std::function<void(juce::var)> complete) {
                if (args.size() < 2) { complete(juce::var("[]")); return; }
                int divisions = static_cast<int>(args[0]);
                double period = static_cast<double>(args[1]);

                auto intervals = ScaleGenerator::generateEDO(divisions, period);

                // Convert to JSON array string
                juce::String json = "[";
                for (size_t i = 0; i < intervals.size(); ++i)
                {
                    if (i > 0) json += ",";
                    json += juce::String(intervals[i], 6);
                }
                json += "]";
                complete(juce::var(json));
            })
            .withNativeFunction("generateHarmonicSeries", [](const juce::Array<juce::var>& args,
                                                                  std::function<void(juce::var)> complete) {
                if (args.size() < 2) { complete(juce::var("[]")); return; }
                int startHarmonic = static_cast<int>(args[0]);
                int endHarmonic = static_cast<int>(args[1]);

                auto intervals = ScaleGenerator::generateHarmonicSeries(startHarmonic, endHarmonic);

                // Convert to JSON array string
                juce::String json = "[";
                for (size_t i = 0; i < intervals.size(); ++i)
                {
                    if (i > 0) json += ",";
                    json += juce::String(intervals[i], 6);
                }
                json += "]";
                complete(juce::var(json));
            })
            .withNativeFunction("generateRank2", [](const juce::Array<juce::var>& args,
                                                         std::function<void(juce::var)> complete) {
                if (args.size() < 3) { complete(juce::var("[]")); return; }
                double generator = static_cast<double>(args[0]);
                double period = static_cast<double>(args[1]);
                int count = static_cast<int>(args[2]);

                auto intervals = ScaleGenerator::generateRank2(generator, period, count);

                // Convert to JSON array string
                juce::String json = "[";
                for (size_t i = 0; i < intervals.size(); ++i)
                {
                    if (i > 0) json += ",";
                    json += juce::String(intervals[i], 6);
                }
                json += "]";
                complete(juce::var(json));
            })
            .withNativeFunction("applyGeneratedScale", [this](const juce::Array<juce::var>& args,
                                                               std::function<void(juce::var)> complete) {
                if (args.size() < 2) { complete(juce::var(false)); return; }

                juce::String jsonIntervals = args[0].toString();
                juce::String scaleName = args[1].toString();

                // Parse JSON array of intervals
                std::vector<double> intervals;
                juce::var parsed;
                if (juce::JSON::parse(jsonIntervals, parsed).wasOk() && parsed.isArray())
                {
                    for (int i = 0; i < parsed.size(); ++i)
                    {
                        intervals.push_back(static_cast<double>(parsed[i]));
                    }
                }

                if (intervals.empty())
                {
                    complete(juce::var(false));
                    return;
                }

                // Apply to TuningEngine
                processorRef.getTuningEngine()->setCustomIntervals(intervals, scaleName);
                processorRef.getTuningEngine()->setMode(TuningEngine::Mode::Scala);

                // Mark as custom preset
                processorRef.getTuningEngine()->setBuiltInPreset(TuningEngine::BuiltInPreset::Custom);

                // Update APVTS tuning mode to Custom (1)
                if (auto* param = processorRef.getAPVTS().getParameter("tuningMode"))
                {
                    param->setValueNotifyingHost(0.5f);  // Custom = index 1 out of 3
                }

                complete(juce::var(true));
            })
            // ─────────────────────────────────────────────────────────────────
            // EMBEDDED TUNING LIBRARY (v1.15.0)
            // ─────────────────────────────────────────────────────────────────
            .withNativeFunction("getEmbeddedTuningList", [](const juce::Array<juce::var>&,
                                                            std::function<void(juce::var)> complete) {
                // Build JSON array of all tunings
                const auto& tunings = EmbeddedTunings::getAllTunings();
                juce::String json = "[";
                bool first = true;
                for (const auto& tuning : tunings)
                {
                    if (!first) json += ",";
                    first = false;
                    json += "{";
                    json += "\"id\":\"" + juce::String(tuning.id) + "\",";
                    json += "\"name\":\"" + juce::String(tuning.name) + "\",";
                    json += "\"category\":\"" + juce::String(tuning.category) + "\",";
                    json += "\"description\":\"" + juce::String(tuning.description) + "\",";
                    json += "\"noteCount\":" + juce::String(static_cast<int>(tuning.intervals.size())) + ",";
                    json += "\"period\":" + juce::String(tuning.period, 1);
                    json += "}";
                }
                json += "]";
                complete(juce::var(json));
            })
            .withNativeFunction("loadEmbeddedTuning", [this](const juce::Array<juce::var>& args,
                                                              std::function<void(juce::var)> complete) {
                if (args.isEmpty()) { complete(juce::var(false)); return; }

                juce::String tuningId = args[0].toString();
                const auto* tuning = EmbeddedTunings::getTuningById(tuningId.toStdString());

                if (tuning == nullptr)
                {
                    complete(juce::var(false));
                    return;
                }

                // Apply to TuningEngine.
                // CR-02: EmbeddedTuning::intervals EXCLUDES the period (stored separately). Every
                // other load path (loadScalaFile / setBuiltInPreset / applyGeneratedScale) appends
                // the period before setCustomIntervals; this one did not, so setCustomIntervals saw
                // one fewer degree and used the last interval as the repeat period → all 24 library
                // tunings were silently mistuned (wrong note count AND wrong octave/period).
                std::vector<double> intervals = tuning->intervals;
                intervals.push_back(tuning->period);
                processorRef.getTuningEngine()->setCustomIntervals(intervals, tuning->name);
                processorRef.getTuningEngine()->setMode(TuningEngine::Mode::Scala);

                // Mark as custom preset (since this is a loaded tuning, not a built-in preset)
                processorRef.getTuningEngine()->setBuiltInPreset(TuningEngine::BuiltInPreset::Custom);

                // Update APVTS tuning mode to Custom (1)
                if (auto* param = processorRef.getAPVTS().getParameter("tuningMode"))
                {
                    param->setValueNotifyingHost(0.5f);  // Custom = index 1 out of 3
                }

                // Update temperament preset to Custom (10)
                if (auto* presetParam = processorRef.getAPVTS().getParameter("temperamentPreset"))
                {
                    presetParam->setValueNotifyingHost(1.0f);  // Index 10 = Custom
                }

                complete(juce::var(true));
            })
            .withNativeFunction("getEmbeddedTuningCategories", [](const juce::Array<juce::var>&,
                                                                   std::function<void(juce::var)> complete) {
                auto categories = EmbeddedTunings::getCategories();
                juce::String json = "[";
                for (size_t i = 0; i < categories.size(); ++i)
                {
                    if (i > 0) json += ",";
                    json += "\"" + juce::String(categories[i]) + "\"";
                }
                json += "]";
                complete(juce::var(json));
            })
            // ─────────────────────────────────────────────────────────────────
            // HTML EXPORT (v1.16.0)
            // ─────────────────────────────────────────────────────────────────
            .withNativeFunction("exportTuningHTML", [this](const juce::Array<juce::var>&,
                                                           std::function<void(juce::var)> complete) {
                // Generate HTML content
                auto htmlContent = TuningExporter::toHTML(*processorRef.getTuningEngine());
                auto scaleName = processorRef.getTuningEngine()->getActiveTuningName();

                // Open save dialog
                auto chooser = std::make_shared<juce::FileChooser>(
                    "Export Tuning as HTML",
                    juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile(scaleName + ".html"),
                    "*.html"
                );
                fileChooser = chooser;

                chooser->launchAsync(
                    juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                    [safe = juce::Component::SafePointer<OLyricaAudioProcessorEditor>(this), chooser, htmlContent, complete](const juce::FileChooser& fc) {
                        if (safe == nullptr)  // CR-01: editor torn down while dialog open — bare return, no complete()
                            return;

                        auto result = fc.getResult();
                        if (result == juce::File{})
                        {
                            complete(juce::var()); // User cancelled
                            return;
                        }

                        auto file = result.hasFileExtension(".html") ? result : result.withFileExtension(".html");
                        if (file.replaceWithText(htmlContent))
                        {
                            complete(juce::var(file.getFileName()));
                        }
                        else
                        {
                            complete(juce::var()); // Save failed
                        }
                    }
                );
            })
            // ─────────────────────────────────────────────────────────────────
            // TOOLTIP SYSTEM (v1.18.0)
            // ─────────────────────────────────────────────────────────────────
            .withNativeFunction("setTooltipsEnabled", [this](const juce::Array<juce::var>& args,
                                                              std::function<void(juce::var)> complete) {
                if (args.isEmpty()) { complete(juce::var(false)); return; }
                bool enabled = static_cast<bool>(args[0]);
                processorRef.setTooltipsEnabled(enabled);
                complete(juce::var(true));
            })
            .withNativeFunction("getTooltipsEnabled", [this](const juce::Array<juce::var>&,
                                                              std::function<void(juce::var)> complete) {
                complete(juce::var(processorRef.getTooltipsEnabled()));
            })
            // ─────────────────────────────────────────────────────────────────
            // NOTE TRIGGERING (v1.7.4)
            // ─────────────────────────────────────────────────────────────────
            .withNativeFunction("triggerNoteOn", [this](const juce::Array<juce::var>& args,
                                                         std::function<void(juce::var)> complete) {
                if (args.size() < 2) { complete(juce::var(false)); return; }
                int midiNote = static_cast<int>(args[0]);
                float velocity = static_cast<float>(args[1]);
                processorRef.triggerNoteOn(midiNote, velocity);
                complete(juce::var(true));
            })
            .withNativeFunction("triggerNoteOff", [this](const juce::Array<juce::var>& args,
                                                          std::function<void(juce::var)> complete) {
                if (args.isEmpty()) { complete(juce::var(false)); return; }
                int midiNote = static_cast<int>(args[0]);
                processorRef.triggerNoteOff(midiNote);
                complete(juce::var(true));
            })
            // ─────────────────────────────────────────────────────────────────
            // RELAY REGISTRATION
            // ─────────────────────────────────────────────────────────────────
            .withOptionsFrom(*masterVolumeRelay)
            .withOptionsFrom(*brightnessRelay)
            .withOptionsFrom(*timbreRelay)
            .withOptionsFrom(*decayTimeRelay)
            .withOptionsFrom(*bodySizeRelay)
            .withOptionsFrom(*bodyResonanceRelay)
            .withOptionsFrom(*sympatheticAmountRelay)
            .withOptionsFrom(*pluckPositionRelay)
            .withOptionsFrom(*fingerHardnessRelay)
            .withOptionsFrom(*stringTensionRelay)
            .withOptionsFrom(*stringGaugeRelay)
            .withOptionsFrom(*stringLengthRelay)
            .withOptionsFrom(*stringStiffnessRelay)
            .withOptionsFrom(*masterTuneRelay)
            .withOptionsFrom(*pitchBendRangeRelay)
            // v1.9.0: Octave stretch
            .withOptionsFrom(*octaveStretchRelay)
            // v1.4.0: New parameters from v1.3.0
            .withOptionsFrom(*attackNoiseRelay)
            .withOptionsFrom(*sympatheticQRelay)
            .withOptionsFrom(*bodyModeSpreadRelay)
            .withOptionsFrom(*bridgeBrightnessRelay)
            // v1.19.0: Humanize relay
            .withOptionsFrom(*humanizeRelay)
            // v1.21.0: Glissando speed relay
            .withOptionsFrom(*glissandoSpeedRelay)
            // v1.23.0: Glissando custom semitones relay
            .withOptionsFrom(*glissandoCustomSemitonesRelay)
            // v1.24.0: Glissando humanize relay
            .withOptionsFrom(*glissandoHumanizeRelay)
            // v1.25.0: Glissando time relay
            .withOptionsFrom(*glissandoTimeRelay)
            // v1.26.0: Glissando excitation relay
            .withOptionsFrom(*glissandoExcitationRelay)
            // v1.27.0: Glissando velocity profile relays
            .withOptionsFrom(*glissandoVelStartRelay)
            .withOptionsFrom(*glissandoVelEndRelay)
            // Register all choice relays
            .withOptionsFrom(*stringMaterialRelay)
            .withOptionsFrom(*woodTypeRelay)
            .withOptionsFrom(*techniqueRelay)
            .withOptionsFrom(*glissandoScaleRelay)
            // v1.22.0: Glissando shape relay
            .withOptionsFrom(*glissandoShapeRelay)
            // v1.30.0: Glissando tonic relay
            .withOptionsFrom(*glissandoTonicRelay)
            // v1.23.0: Glissando interval and direction relays
            .withOptionsFrom(*glissandoIntervalRelay)
            .withOptionsFrom(*glissandoDirectionRelay)
            // v1.6.0: Tuning mode relay
            .withOptionsFrom(*tuningModeRelay)
            // v1.30.0: Glissando toggle relays
            .withOptionsFrom(*freeToggleRelay)
            .withOptionsFrom(*scaleToggleRelay)
            // v1.30.0: Free mode parameter relays
            .withOptionsFrom(*freeShapeRelay)
            .withOptionsFrom(*freeIntervalRelay)
            .withOptionsFrom(*freeDirectionRelay)
            .withOptionsFrom(*freeKeyswitchNoteRelay)
            .withOptionsFrom(*scaleKeyswitchNoteRelay)
            .withOptionsFrom(*freeCustomSemitonesRelay)
            // v1.31.0: Tempo sync
            .withOptionsFrom(*freeTempoSyncRelay)
            .withOptionsFrom(*scaleTempoSyncRelay)
            // v1.35.1: Effects chain options
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
            // ─────────────────────────────────────────────────────────────────
            // v1.30.0: GLISSANDO CUSTOM DEGREE BITMASK
            // ─────────────────────────────────────────────────────────────────
            .withNativeFunction("setGlissCustomDegrees", [this](const juce::Array<juce::var>& args,
                                                                 std::function<void(juce::var)> complete) {
                if (args.size() < 2) { complete(juce::var(false)); return; }
                uint32_t low = static_cast<uint32_t>(static_cast<int>(args[0]));
                uint32_t high = static_cast<uint32_t>(static_cast<int>(args[1]));
                uint64_t mask = (static_cast<uint64_t>(high) << 32) | low;
                processorRef.setGlissCustomDegrees(mask);
                complete(juce::var(true));
            })
            .withNativeFunction("getGlissCustomDegrees", [this](const juce::Array<juce::var>&,
                                                                  std::function<void(juce::var)> complete) {
                uint64_t mask = processorRef.getGlissCustomDegrees();
                auto* obj = new juce::DynamicObject();
                obj->setProperty("low", static_cast<int>(mask & 0xFFFFFFFF));
                obj->setProperty("high", static_cast<int>((mask >> 32) & 0xFFFFFFFF));
                complete(juce::var(obj));
            })
            .withNativeFunction("getScaleDegreeCount", [this](const juce::Array<juce::var>&,
                                                               std::function<void(juce::var)> complete) {
                complete(juce::var(processorRef.getTuningEngine()->getScaleDegrees()));
            })
    );

    // 3️⃣ CREATE ATTACHMENTS (must be created AFTER WebView)
    // CRITICAL: JUCE 8 requires 3 parameters (parameter, relay, undoManager)
    auto& apvts = processorRef.getAPVTS();

    masterVolumeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("masterVolume"), *masterVolumeRelay, nullptr);
    brightnessAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("brightness"), *brightnessRelay, nullptr);
    timbreAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("timbre"), *timbreRelay, nullptr);
    decayTimeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("decayTime"), *decayTimeRelay, nullptr);
    bodySizeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("bodySize"), *bodySizeRelay, nullptr);
    bodyResonanceAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("bodyResonance"), *bodyResonanceRelay, nullptr);
    sympatheticAmountAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("sympatheticAmount"), *sympatheticAmountRelay, nullptr);
    pluckPositionAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("pluckPosition"), *pluckPositionRelay, nullptr);
    fingerHardnessAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("fingerHardness"), *fingerHardnessRelay, nullptr);
    stringTensionAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("stringTension"), *stringTensionRelay, nullptr);
    stringGaugeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("stringGauge"), *stringGaugeRelay, nullptr);
    stringLengthAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("stringLength"), *stringLengthRelay, nullptr);
    stringStiffnessAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("stringStiffness"), *stringStiffnessRelay, nullptr);
    masterTuneAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("masterTune"), *masterTuneRelay, nullptr);
    pitchBendRangeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("pitchBendRange"), *pitchBendRangeRelay, nullptr);
    // v1.9.0: Octave stretch attachment
    octaveStretchAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("octaveStretch"), *octaveStretchRelay, nullptr);
    // v1.4.0: New parameters from v1.3.0
    attackNoiseAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("attackNoise"), *attackNoiseRelay, nullptr);
    sympatheticQAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("sympatheticQ"), *sympatheticQRelay, nullptr);
    bodyModeSpreadAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("bodyModeSpread"), *bodyModeSpreadRelay, nullptr);
    bridgeBrightnessAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("bridgeBrightness"), *bridgeBrightnessRelay, nullptr);
    // v1.19.0: Humanize attachment
    humanizeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("humanize"), *humanizeRelay, nullptr);
    // v1.21.0: Glissando speed attachment
    glissandoSpeedAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("glissandoSpeed"), *glissandoSpeedRelay, nullptr);
    // v1.23.0: Glissando custom semitones attachment
    glissandoCustomSemitonesAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("glissandoCustomSemitones"), *glissandoCustomSemitonesRelay, nullptr);
    // v1.24.0: Glissando humanize attachment
    glissandoHumanizeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("glissandoHumanize"), *glissandoHumanizeRelay, nullptr);
    // v1.25.0: Glissando time attachment
    glissandoTimeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("glissandoTime"), *glissandoTimeRelay, nullptr);
    // v1.26.0: Glissando excitation attachment
    glissandoExcitationAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("glissandoExcitation"), *glissandoExcitationRelay, nullptr);
    // v1.27.0: Glissando velocity profile attachments
    glissandoVelStartAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("glissandoVelStart"), *glissandoVelStartRelay, nullptr);
    glissandoVelEndAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("glissandoVelEnd"), *glissandoVelEndRelay, nullptr);

    stringMaterialAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *apvts.getParameter("stringMaterial"), *stringMaterialRelay, nullptr);
    woodTypeAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *apvts.getParameter("woodType"), *woodTypeRelay, nullptr);
    techniqueAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *apvts.getParameter("technique"), *techniqueRelay, nullptr);
    glissandoScaleAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *apvts.getParameter("glissandoScale"), *glissandoScaleRelay, nullptr);
    // v1.22.0: Glissando shape attachment
    glissandoShapeAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *apvts.getParameter("glissandoShape"), *glissandoShapeRelay, nullptr);
    // v1.30.0: Glissando tonic attachment
    glissandoTonicAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *apvts.getParameter("glissandoTonic"), *glissandoTonicRelay, nullptr);
    // v1.23.0: Glissando interval and direction attachments
    glissandoIntervalAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *apvts.getParameter("glissandoInterval"), *glissandoIntervalRelay, nullptr);
    glissandoDirectionAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *apvts.getParameter("glissandoDirection"), *glissandoDirectionRelay, nullptr);
    // v1.6.0: Tuning mode attachment
    tuningModeAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *apvts.getParameter("tuningMode"), *tuningModeRelay, nullptr);
    // v1.30.0: Glissando toggle attachments
    freeToggleAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *apvts.getParameter("freeToggle"), *freeToggleRelay, nullptr);
    scaleToggleAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *apvts.getParameter("scaleToggle"), *scaleToggleRelay, nullptr);
    // v1.30.0: Free mode parameter attachments
    freeShapeAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *apvts.getParameter("freeShape"), *freeShapeRelay, nullptr);
    freeIntervalAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *apvts.getParameter("freeInterval"), *freeIntervalRelay, nullptr);
    freeDirectionAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *apvts.getParameter("freeDirection"), *freeDirectionRelay, nullptr);
    freeKeyswitchNoteAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *apvts.getParameter("freeKeyswitchNote"), *freeKeyswitchNoteRelay, nullptr);
    scaleKeyswitchNoteAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *apvts.getParameter("scaleKeyswitchNote"), *scaleKeyswitchNoteRelay, nullptr);
    freeCustomSemitonesAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("freeCustomSemitones"), *freeCustomSemitonesRelay, nullptr);
    // v1.31.0: Tempo sync attachments
    freeTempoSyncAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *apvts.getParameter("freeTempoSync"), *freeTempoSyncRelay, nullptr);
    scaleTempoSyncAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *apvts.getParameter("scaleTempoSync"), *scaleTempoSyncRelay, nullptr);
    // v1.35.1: Effects chain attachments
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

    // 4️⃣ SETUP WEBVIEW
    addAndMakeVisible(*webView);

    // Navigate to UI (uses resource provider)
    webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());

    // Set editor size - v1.4.0: Reduced from 800x600 to 700x450
    setSize(700, 450);

    // v1.7.9: Start timer for MIDI event polling (tuning circle visualization)
    startTimer(50);  // 20 Hz polling - fast enough for visual feedback
}

OLyricaAudioProcessorEditor::~OLyricaAudioProcessorEditor()
{
    // v1.7.9: Stop timer before destroying UI components
    stopTimer();

    // Destructor runs in REVERSE order of declaration:
    // 1. Attachments destroyed first (safe - webView still exists)
    // 2. WebView destroyed second (safe - relays still exist)
    // 3. Relays destroyed last
}

void OLyricaAudioProcessorEditor::paint(juce::Graphics& g)
{
    // WebView handles all painting
    juce::ignoreUnused(g);
}

void OLyricaAudioProcessorEditor::resized()
{
    // WebView fills entire editor
    if (webView)
        webView->setBounds(getLocalBounds());
}

// v1.7.9: Timer callback - poll MIDI events and notify WebView for tuning circle visualization
// v1.10.0: Also send held notes data for True Keys visualization
// v1.13.1: Also sync tonic from processor to ensure persistence works
// v1.18.0: Also sync tooltip state from processor
void OLyricaAudioProcessorEditor::timerCallback()
{
    // v1.13.1: Sync tonic from processor to WebView (handles state restoration timing)
    static int lastSyncedTonic = -1;
    int currentTonic = processorRef.getTuningEngine()->getTonicNote();
    if (currentTonic != lastSyncedTonic)
    {
        lastSyncedTonic = currentTonic;
        juce::String js = "if (typeof window.syncTonicFromProcessor === 'function') window.syncTonicFromProcessor("
            + juce::String(currentTonic) + ");";
        webView->evaluateJavascript(js, nullptr);
    }

    // v1.18.0: Sync tooltip state from processor to WebView (once, on first callback)
    static bool tooltipStateSynced = false;
    if (!tooltipStateSynced)
    {
        tooltipStateSynced = true;
        bool enabled = processorRef.getTooltipsEnabled();
        juce::String js = "if (typeof window.restoreTooltipState === 'function') window.restoreTooltipState("
            + juce::String(enabled ? "true" : "false") + ");";
        webView->evaluateJavascript(js, nullptr);
    }

    // v1.30.0: Sync scale degree count to WebView (for custom degree toggles)
    static int lastScaleDegreeCount = -1;
    int currentDegreeCount = processorRef.getTuningEngine()->getScaleDegrees();
    if (currentDegreeCount != lastScaleDegreeCount)
    {
        lastScaleDegreeCount = currentDegreeCount;
        webView->emitEventIfBrowserIsVisible("scaleDegreeCountChanged",
            juce::var(currentDegreeCount));
    }

    // Process MIDI events for pitch circle visualization
    MidiNoteEvent event;
    while (processorRef.popMidiEvent(event))
    {
        if (event.velocity > 0.0f)
        {
            // Note-on: activate interval line with velocity-based intensity
            juce::String js = "if (typeof setNoteActive === 'function') setNoteActive("
                + juce::String(event.noteNumber) + ", "
                + juce::String(event.velocity, 3) + ");";
            webView->evaluateJavascript(js, nullptr);
        }
        else
        {
            // Note-off: deactivate interval line
            juce::String js = "if (typeof setNoteInactive === 'function') setNoteInactive("
                + juce::String(event.noteNumber) + ");";
            webView->evaluateJavascript(js, nullptr);
        }
    }

    // v1.10.0: Send held notes data for True Keys visualization
    std::vector<int> heldNotes;
    std::vector<double> heldFreqs;
    processorRef.getHeldNotesData(heldNotes, heldFreqs);

    // Build JSON arrays
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

    // Send to WebView
    juce::String js = "if (typeof window.updateHeldNotes === 'function') window.updateHeldNotes("
        + notesJson + "," + freqsJson + ");";
    webView->evaluateJavascript(js, nullptr);
}

std::optional<juce::WebBrowserComponent::Resource>
OLyricaAudioProcessorEditor::getResource(const juce::String& url)
{
    // Helper lambda to create resource from BinaryData
    auto makeResource = [](const char* data, int size, const char* mimeType) {
        return juce::WebBrowserComponent::Resource {
            std::vector<std::byte>(
                reinterpret_cast<const std::byte*>(data),
                reinterpret_cast<const std::byte*>(data) + size
            ),
            juce::String(mimeType)
        };
    };

    // ═══════════════════════════════════════════════════════════════════
    // CRITICAL: Explicit URL mapping (not generic loop)
    // BinaryData converts paths to C++ identifiers:
    //   index.html → index_html
    //   js/juce/index.js → index_js (path flattened)
    // ═══════════════════════════════════════════════════════════════════

    // HTML
    if (url == "/" || url == "/index.html")
        return makeResource(BinaryData::index_html,
                           BinaryData::index_htmlSize,
                           "text/html");

    // JUCE Bridge Library
    if (url == "/js/juce/index.js")
        return makeResource(BinaryData::index_js,
                           BinaryData::index_jsSize,
                           "text/javascript");

    // Native Interop Check (REQUIRED for WebView)
    if (url == "/js/juce/check_native_interop.js")
        return makeResource(BinaryData::check_native_interop_js,
                           BinaryData::check_native_interop_jsSize,
                           "text/javascript");

    // App JavaScript
    if (url == "/js/app.js")
        return makeResource(BinaryData::app_js,
                           BinaryData::app_jsSize,
                           "text/javascript");

    // v1.4.0: Images for Naturalist aesthetic
    if (url == "/images/paper1.jpg")
        return makeResource(BinaryData::paper1_jpg,
                           BinaryData::paper1_jpgSize,
                           "image/jpeg");

    if (url == "/images/fern_naturalistsmisc1Geor_0089.png")
        return makeResource(BinaryData::fern_naturalistsmisc1Geor_0089_png,
                           BinaryData::fern_naturalistsmisc1Geor_0089_pngSize,
                           "image/png");

    // 404 for unknown resources
    DBG("Resource not found: " + url);
    return std::nullopt;
}
