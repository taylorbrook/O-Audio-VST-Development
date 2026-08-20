/*
   This file is part of O-Contrabass, an Ouaricon Audio plugin.
   Copyright (C) 2026  Ouaricon Audio

   SPDX-License-Identifier: AGPL-3.0-or-later

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
/*
  ==============================================================================

    O-Contrabass — WebView Editor (BOILERPLATE from finalized mockup v1)

    Stage 3 Phase 3.x: this replaces Source/PluginEditor.cpp. Adapt during
    Stage 3 execute — initialization order MUST match the header's declaration
    order (relays → webView → attachments).

    Native functions registered here (grep-diff gate vs JS getNativeFunction):
      - getParameterDefaults   (WR-11: skew-correct normalized defaults for
                                dblclick reset — JS must NEVER hardcode ranges)
      - openTuningFilePicker   (async FileChooser, SafePointer pattern; D1 .scl only)
      - 10 preset fns          (Task 8, preset-manager v1.0.4 contract)
      - 20 tuning fns          (Task 9, shared tuning-panel.js contract —
                                O-Wind PluginEditor.cpp:191-510 reference)

    C++ → JS event feeds (emitEventIfBrowserIsVisible, timer-driven @30 Hz):
      - vuLevel   {"db": <rmsDb>}                       — post-limiter RMS (Task 12)
      - bowState  {"v":,"p":,"b":,"active":}            — DSP-true bow operating
                                                          point, most-recently-
                                                          started active voice (Task 13)

  ==============================================================================
*/

#include "PluginEditor.h"
#include "BinaryData.h"
#include "TuningEngine.h"
#include "ScaleGenerator.h"
#include "EmbeddedTunings.h"
#include "TuningExporter.h"

// Serialize an interval list through juce::JSON rather than string
// concatenation, so the payload can never be malformed (O-Wind IN-12).
static juce::String intervalsToJson (const std::vector<double>& intervals)
{
    juce::Array<juce::var> arr;
    for (auto v : intervals)
        arr.add (v);
    return juce::JSON::toString (juce::var (arr), true);
}

OContrabassAudioProcessorEditor::OContrabassAudioProcessorEditor(OContrabassAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    // ------------------------------------------------------------------
    // 1️⃣ Create relays FIRST (IDs must match APVTS param IDs EXACTLY —
    //    case-sensitive, parameter-spec.md authoritative)
    // ------------------------------------------------------------------
    // I · Bow
    bowSpeedRelay        = std::make_unique<juce::WebSliderRelay>("BOW_SPEED");
    bowPressureRelay     = std::make_unique<juce::WebSliderRelay>("BOW_PRESSURE");
    bowPositionRelay     = std::make_unique<juce::WebSliderRelay>("BOW_POSITION");
    rosinRelay           = std::make_unique<juce::WebSliderRelay>("ROSIN");
    bowNoiseRelay        = std::make_unique<juce::WebSliderRelay>("BOW_NOISE");
    releaseRelay         = std::make_unique<juce::WebSliderRelay>("RELEASE");
    // II · Body
    bodySizeRelay        = std::make_unique<juce::WebSliderRelay>("BODY_SIZE");
    bodyDampingRelay     = std::make_unique<juce::WebSliderRelay>("BODY_DAMPING");
    bodyMixRelay         = std::make_unique<juce::WebSliderRelay>("BODY_MIX");
    brightnessRelay      = std::make_unique<juce::WebSliderRelay>("BRIGHTNESS");
    // III · Strings
    stringTensionRelay   = std::make_unique<juce::WebSliderRelay>("STRING_TENSION");
    stringStiffnessRelay = std::make_unique<juce::WebSliderRelay>("STRING_STIFFNESS");
    activeStringsRelay   = std::make_unique<juce::WebSliderRelay>("ACTIVE_STRINGS");
    detuneERelay         = std::make_unique<juce::WebSliderRelay>("DETUNE_E");
    detuneARelay         = std::make_unique<juce::WebSliderRelay>("DETUNE_A");
    detuneDRelay         = std::make_unique<juce::WebSliderRelay>("DETUNE_D");
    detuneGRelay         = std::make_unique<juce::WebSliderRelay>("DETUNE_G");
    // IV · Expression
    expressionMacroRelay = std::make_unique<juce::WebSliderRelay>("EXPRESSION_MACRO");
    vibratoRateRelay     = std::make_unique<juce::WebSliderRelay>("VIBRATO_RATE");
    vibratoDepthRelay    = std::make_unique<juce::WebSliderRelay>("VIBRATO_DEPTH");
    vibratoOnsetRelay    = std::make_unique<juce::WebSliderRelay>("VIBRATO_ONSET");
    slowLfoRateRelay     = std::make_unique<juce::WebSliderRelay>("SLOW_LFO_RATE");
    slowLfoDepthRelay    = std::make_unique<juce::WebSliderRelay>("SLOW_LFO_DEPTH");
    // V · Drone
    infiniteSustainRelay = std::make_unique<juce::WebSliderRelay>("INFINITE_SUSTAIN");
    subHarmonicsRelay    = std::make_unique<juce::WebSliderRelay>("SUB_HARMONICS");
    // VI · Output
    outputGainRelay      = std::make_unique<juce::WebSliderRelay>("OUTPUT_GAIN");
    widthRelay           = std::make_unique<juce::WebSliderRelay>("WIDTH");
    masterSatRelay       = std::make_unique<juce::WebSliderRelay>("MASTER_SAT_AMOUNT");
    limiterCeilingRelay  = std::make_unique<juce::WebSliderRelay>("LIMITER_CEILING_DB");
    // VII · Microtonal
    referencePitchRelay  = std::make_unique<juce::WebSliderRelay>("REFERENCE_PITCH");
    tuningSystemRelay    = std::make_unique<juce::WebComboBoxRelay>("TUNING_SYSTEM");
    noteExpressionRelay  = std::make_unique<juce::WebToggleButtonRelay>("NOTE_EXPRESSION");

    // ------------------------------------------------------------------
    // 2️⃣ Create WebView SECOND (with relay options)
    // ------------------------------------------------------------------
    webView = std::make_unique<juce::WebBrowserComponent>(
        juce::WebBrowserComponent::Options{}
            .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
            // Windows: WebView2 may be denied its default user-data location
            // inside DAW hosts — always point it at a temp dir. Requires
            // NEEDS_WEBVIEW2 TRUE + JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
            // (both already present in O-Contrabass CMakeLists.txt).
            .withWinWebView2Options(
                juce::WebBrowserComponent::Options::WinWebView2{}
                    .withUserDataFolder(juce::File::getSpecialLocation(
                        juce::File::SpecialLocationType::tempDirectory)
                            .getChildFile("OContrabass_WebView")))
            .withNativeIntegrationEnabled()
            // WR-11 pattern (O-GrainScatter): expose each parameter's TRUE
            // (skew-correct) normalized default so the UI's double-click reset
            // uses the C++ NormalisableRange, never hand-coded JS constants.
            .withNativeFunction(
                juce::Identifier("getParameterDefaults"),
                [this](const juce::Array<juce::var>& /*args*/,
                       juce::WebBrowserComponent::NativeFunctionCompletion completion)
                {
                    juce::DynamicObject::Ptr obj = new juce::DynamicObject();
                    for (auto* param : processorRef.getParameters())
                        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*>(param))
                            obj->setProperty(ranged->getParameterID(), ranged->getDefaultValue());
                    completion(juce::var(obj.get()));
                })
            // Scala/TUN file picker — async FileChooser handled in a member fn
            // so the SafePointer/UAF contract lives in one place.
            .withNativeFunction(
                juce::Identifier("openTuningFilePicker"),
                [this](const juce::Array<juce::var>& /*args*/,
                       juce::WebBrowserComponent::NativeFunctionCompletion completion)
                {
                    launchTuningFilePicker(std::move(completion));
                })

            // =============================================================
            // HOVER HELP (v1.7.0) — the "?" toggle's persistence pair.
            // Completes with the stored value either way so JS could re-sync
            // from the reply if it ever wanted to.
            // =============================================================
            .withNativeFunction("setTooltipsEnabled", [this](const auto& args, auto complete) {
                if (args.size() > 0)
                    processorRef.tooltipsEnabled.store((bool) args[0],
                                                       std::memory_order_release);
                complete(juce::var(processorRef.tooltipsEnabled.load(
                                       std::memory_order_acquire)));
            })

            // PULLED by the page at init, never pushed — a push from the
            // constructor or the 30 Hz timer fires before the inline module
            // has evaluated, so the preference would silently never arrive
            // and the toggle would read OFF on every reopen (the O-FreqPulse
            // WR-01 bug, avoided here by construction).
            .withNativeFunction("getTooltipsEnabled", [this](const auto&, auto complete) {
                complete(juce::var(processorRef.tooltipsEnabled.load(
                                       std::memory_order_acquire)));
            })

            // =============================================================
            // PRESET NATIVE FUNCTIONS (Task 8 — preset-manager v1.0.4
            // contract; 10 fns required by js/preset-manager.js)
            // =============================================================
            .withNativeFunction("savePreset", [this](const auto& args, auto complete) {
                if (args.size() < 1 || !args[0].isString()) { complete(juce::var(false)); return; }
                complete(juce::var(processorRef.getPresetManager().savePreset(args[0].toString())));
            })

            .withNativeFunction("loadPreset", [this](const auto& args, auto complete) {
                if (args.size() < 1 || !args[0].isString()) { complete(juce::var(false)); return; }
                complete(juce::var(processorRef.getPresetManager().loadPreset(args[0].toString())));
            })

            .withNativeFunction("getPresetList", [this](const auto&, auto complete) {
                juce::Array<juce::var> arr;
                for (const auto& name : processorRef.getPresetManager().getPresetList())
                    arr.add(name);
                complete(juce::var(arr));
            })

            .withNativeFunction("getCurrentPreset", [this](const auto&, auto complete) {
                complete(juce::var(processorRef.getPresetManager().getCurrentPresetName()));
            })

            // Return the neighbour NAME only — js/preset-manager.js loads it
            // itself via loadPreset (loading here too would double-load).
            .withNativeFunction("selectNextPreset", [this](const auto&, auto complete) {
                complete(juce::var(processorRef.getPresetManager().getNextPreset()));
            })

            .withNativeFunction("selectPreviousPreset", [this](const auto&, auto complete) {
                complete(juce::var(processorRef.getPresetManager().getPreviousPreset()));
            })

            .withNativeFunction("deletePreset", [this](const auto& args, auto complete) {
                if (args.size() < 1 || !args[0].isString()) { complete(juce::var(false)); return; }
                complete(juce::var(processorRef.getPresetManager().deletePreset(args[0].toString())));
            })

            .withNativeFunction("isFactoryPreset", [this](const auto& args, auto complete) {
                if (args.size() < 1 || !args[0].isString()) { complete(juce::var(false)); return; }
                complete(juce::var(processorRef.getPresetManager().isFactoryPreset(args[0].toString())));
            })

            // js/preset-manager.js expects {success, name} from both dialogs.
            .withNativeFunction("savePresetWithDialog", [this](const auto&, auto complete) {
                auto makeResult = [](bool ok, const juce::String& name) {
                    auto* obj = new juce::DynamicObject();
                    obj->setProperty("success", ok);
                    obj->setProperty("name", name);
                    return juce::var(obj);
                };
                if (fileDialogOpen) { complete(makeResult(false, {})); return; }
                fileDialogOpen = true;
                fileChooser = std::make_shared<juce::FileChooser>(
                    "Save Preset",
                    processorRef.getPresetManager().getUserPresetsDirectory(),
                    "*.json");
                // MSVC rejects SafePointer(this) init-captures in nested lambdas
                // (C2440/C2119) - hoist to a local and capture by value.
                juce::Component::SafePointer<OContrabassAudioProcessorEditor> safeThis(this);
                fileChooser->launchAsync(
                    juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                    [safeThis,
                     complete, makeResult](const juce::FileChooser& fc) {
                        // Editor destroyed while the dialog was up: bare return —
                        // `complete` is owned by the dead WebView Impl (UAF otherwise).
                        if (safeThis == nullptr)
                            return;
                        safeThis->fileDialogOpen = false;
                        auto result = fc.getResult();
                        if (result == juce::File{}) { complete(makeResult(false, {})); return; }
                        auto name = result.getFileNameWithoutExtension();
                        auto& pm = safeThis->processorRef.getPresetManager();
                        // Honor the directory the user picked — savePreset()
                        // always writes to the user-presets dir (O-Wind WR-12).
                        bool ok = result.isAChildOf(pm.getUserPresetsDirectory())
                                    ? pm.savePreset(name)
                                    : pm.savePresetToFile(result.withFileExtension("json"));
                        complete(makeResult(ok, name));
                    });
            })

            .withNativeFunction("loadPresetFromFile", [this](const auto&, auto complete) {
                auto makeResult = [](bool ok, const juce::String& name) {
                    auto* obj = new juce::DynamicObject();
                    obj->setProperty("success", ok);
                    obj->setProperty("name", name);
                    return juce::var(obj);
                };
                if (fileDialogOpen) { complete(makeResult(false, {})); return; }
                fileDialogOpen = true;
                fileChooser = std::make_shared<juce::FileChooser>(
                    "Load Preset",
                    processorRef.getPresetManager().getUserPresetsDirectory(),
                    "*.json");
                // MSVC rejects SafePointer(this) init-captures in nested lambdas
                // (C2440/C2119) - hoist to a local and capture by value.
                juce::Component::SafePointer<OContrabassAudioProcessorEditor> safeThis(this);
                fileChooser->launchAsync(
                    juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                    [safeThis,
                     complete, makeResult](const juce::FileChooser& fc) {
                        if (safeThis == nullptr)
                            return;   // bare return — see savePresetWithDialog
                        safeThis->fileDialogOpen = false;
                        auto file = fc.getResult();
                        if (! file.existsAsFile()) { complete(makeResult(false, {})); return; }
                        auto& pm = safeThis->processorRef.getPresetManager();
                        bool ok = pm.loadPresetFromFile(file);
                        complete(makeResult(ok, file.getFileNameWithoutExtension()));
                    });
            })

            // =============================================================
            // TUNING NATIVE FUNCTIONS (Task 9 — shared tuning-panel.js
            // contract; 20 fns. Reference: O-Wind PluginEditor.cpp:191-510)
            // =============================================================

            .withNativeFunction("getTuningIntervals", [this](const juce::Array<juce::var>&, auto complete) {
                complete(intervalsToJson(processorRef.getTuningEngine()->getIntervals()));
            })

            .withNativeFunction("getTuningName", [this](const juce::Array<juce::var>&, auto complete) {
                complete(processorRef.getTuningEngine()->getActiveTuningName());
            })

            .withNativeFunction("setSingleInterval", [this](const juce::Array<juce::var>& args, auto complete) {
                if (args.size() >= 2) {
                    processorRef.getTuningEngine()->setSingleInterval(
                        static_cast<int>(args[0]), static_cast<double>(args[1]));
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
                    processorRef.getTuningEngine()->setOctaveStretch(static_cast<float>(static_cast<double>(args[0])));
                    complete(true);
                    return;
                }
                complete(false);
            })

            // REFERENCE_PITCH ↔ panel masterTune coherence (Task 9): the APVTS
            // param is the single source of truth. The voice applies the
            // refPitch/440 ratio itself (BowedContrabassVoice.cpp:64-76) and the
            // engine's own masterTune stays at 440 BY DESIGN — writing the
            // engine here would double-apply the ratio AND snap back on reload
            // (O-Wind WR-11).
            .withNativeFunction("getMasterTune", [this](const juce::Array<juce::var>&, auto complete) {
                complete(static_cast<double>(
                    processorRef.parameters.getRawParameterValue("REFERENCE_PITCH")->load()));
            })

            .withNativeFunction("setMasterTune", [this](const juce::Array<juce::var>& args, auto complete) {
                if (args.size() >= 1) {
                    if (auto* param = processorRef.parameters.getParameter("REFERENCE_PITCH")) {
                        const float hz = static_cast<float>(static_cast<double>(args[0]));
                        param->setValueNotifyingHost(param->convertTo0to1(hz));
                        complete(true);
                        return;
                    }
                }
                complete(false);
            })

            .withNativeFunction("loadScalaFile", [this](const juce::Array<juce::var>&, auto complete) {
                if (fileDialogOpen) { complete(false); return; }
                fileDialogOpen = true;
                fileChooser = std::make_shared<juce::FileChooser>(
                    "Load Scala File",
                    juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
                    "*.scl");   // D1: .scl only — no TUN parser in TuningEngine 2.1.0
                // MSVC rejects SafePointer(this) init-captures in nested lambdas
                // (C2440/C2119) - hoist to a local and capture by value.
                juce::Component::SafePointer<OContrabassAudioProcessorEditor> safeThis(this);
                fileChooser->launchAsync(
                    juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                    [safeThis,
                     complete](const juce::FileChooser& fc) {
                        if (safeThis == nullptr)
                            return;   // bare return — see savePresetWithDialog
                        safeThis->fileDialogOpen = false;
                        auto file = fc.getResult();
                        if (file.existsAsFile())
                            complete(safeThis->processorRef.loadScalaFile(file));
                        else
                            complete(false);
                    });
            })

            .withNativeFunction("loadKBMFile", [this](const juce::Array<juce::var>&, auto complete) {
                if (fileDialogOpen) { complete(false); return; }
                fileDialogOpen = true;
                fileChooser = std::make_shared<juce::FileChooser>(
                    "Load Keyboard Mapping",
                    juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
                    "*.kbm");
                // MSVC rejects SafePointer(this) init-captures in nested lambdas
                // (C2440/C2119) - hoist to a local and capture by value.
                juce::Component::SafePointer<OContrabassAudioProcessorEditor> safeThis(this);
                fileChooser->launchAsync(
                    juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                    [safeThis,
                     complete](const juce::FileChooser& fc) {
                        if (safeThis == nullptr)
                            return;   // bare return — see savePresetWithDialog
                        safeThis->fileDialogOpen = false;
                        auto file = fc.getResult();
                        if (file.existsAsFile())
                            complete(safeThis->processorRef.getTuningEngine()->loadKBMFile(file));
                        else
                            complete(false);
                    });
            })

            .withNativeFunction("saveScalaFile", [this](const juce::Array<juce::var>&, auto complete) {
                if (fileDialogOpen) { complete(false); return; }
                fileDialogOpen = true;
                fileChooser = std::make_shared<juce::FileChooser>(
                    "Save Scala File",
                    juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                        .getChildFile("scale.scl"),
                    "*.scl");
                // MSVC rejects SafePointer(this) init-captures in nested lambdas
                // (C2440/C2119) - hoist to a local and capture by value.
                juce::Component::SafePointer<OContrabassAudioProcessorEditor> safeThis(this);
                fileChooser->launchAsync(
                    juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                    [safeThis,
                     complete](const juce::FileChooser& fc) {
                        if (safeThis == nullptr)
                            return;   // bare return — see savePresetWithDialog
                        safeThis->fileDialogOpen = false;
                        auto file = fc.getResult();
                        if (file != juce::File()) {
                            auto content = safeThis->processorRef.getTuningEngine()->generateScalaFileContent();
                            complete(file.replaceWithText(content));
                        } else {
                            complete(false);
                        }
                    });
            })

            .withNativeFunction("saveKBMFile", [this](const juce::Array<juce::var>&, auto complete) {
                if (fileDialogOpen) { complete(false); return; }
                fileDialogOpen = true;
                fileChooser = std::make_shared<juce::FileChooser>(
                    "Save Keyboard Mapping",
                    juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                        .getChildFile("mapping.kbm"),
                    "*.kbm");
                // MSVC rejects SafePointer(this) init-captures in nested lambdas
                // (C2440/C2119) - hoist to a local and capture by value.
                juce::Component::SafePointer<OContrabassAudioProcessorEditor> safeThis(this);
                fileChooser->launchAsync(
                    juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                    [safeThis,
                     complete](const juce::FileChooser& fc) {
                        if (safeThis == nullptr)
                            return;   // bare return — see savePresetWithDialog
                        safeThis->fileDialogOpen = false;
                        auto file = fc.getResult();
                        if (file != juce::File()) {
                            auto content = safeThis->processorRef.getTuningEngine()->generateKBMFileContent();
                            complete(file.replaceWithText(content));
                        } else {
                            complete(false);
                        }
                    });
            })

            .withNativeFunction("generateEDO", [](const juce::Array<juce::var>& args, auto complete) {
                if (args.size() >= 2) {
                    complete(intervalsToJson(ScaleGenerator::generateEDO(
                        static_cast<int>(args[0]), static_cast<double>(args[1]))));
                    return;
                }
                complete(juce::var());
            })

            .withNativeFunction("generateHarmonicSeries", [](const juce::Array<juce::var>& args, auto complete) {
                if (args.size() >= 2) {
                    complete(intervalsToJson(ScaleGenerator::generateHarmonicSeries(
                        static_cast<int>(args[0]), static_cast<int>(args[1]))));
                    return;
                }
                complete(juce::var());
            })

            .withNativeFunction("generateRank2", [](const juce::Array<juce::var>& args, auto complete) {
                if (args.size() >= 3) {
                    complete(intervalsToJson(ScaleGenerator::generateRank2(
                        static_cast<double>(args[0]), static_cast<double>(args[1]),
                        static_cast<int>(args[2]))));
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
                juce::Array<juce::var> arr;
                for (const auto& t : tunings) {
                    auto* obj = new juce::DynamicObject();
                    obj->setProperty("id", juce::String(t.id));
                    obj->setProperty("name", juce::String(t.name));
                    obj->setProperty("category", juce::String(t.category));
                    obj->setProperty("noteCount", static_cast<int>(t.intervals.size()));
                    arr.add(juce::var(obj));
                }
                complete(juce::JSON::toString(juce::var(arr), true));
            })

            .withNativeFunction("loadEmbeddedTuning", [this](const juce::Array<juce::var>& args, auto complete) {
                if (args.size() >= 1) {
                    auto* tuning = EmbeddedTunings::getTuningById(args[0].toString().toStdString());
                    if (tuning != nullptr && !tuning->intervals.empty()) {
                        // CRITICAL (pattern_embedded_tuning_period_dropped): the
                        // period MUST be appended before setCustomIntervals or
                        // every factory tuning is silently mistuned.
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
                if (fileDialogOpen) { complete(false); return; }
                fileDialogOpen = true;
                fileChooser = std::make_shared<juce::FileChooser>(
                    "Export Tuning Documentation",
                    juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                        .getChildFile("tuning-export.html"),
                    "*.html");
                // MSVC rejects SafePointer(this) init-captures in nested lambdas
                // (C2440/C2119) - hoist to a local and capture by value.
                juce::Component::SafePointer<OContrabassAudioProcessorEditor> safeThis(this);
                fileChooser->launchAsync(
                    juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                    [safeThis,
                     complete](const juce::FileChooser& fc) {
                        if (safeThis == nullptr)
                            return;   // bare return — see savePresetWithDialog
                        safeThis->fileDialogOpen = false;
                        auto file = fc.getResult();
                        if (file != juce::File()) {
                            auto html = TuningExporter::toHTML(
                                *safeThis->processorRef.getTuningEngine(), "O-Contrabass");
                            complete(file.replaceWithText(html));
                        } else {
                            complete(false);
                        }
                    });
            })
#if JUCE_WEB_BROWSER_RESOURCE_PROVIDER_AVAILABLE
            .withResourceProvider([this](const auto& url) { return getResource(url); })
#endif
            .withOptionsFrom(*bowSpeedRelay)
            .withOptionsFrom(*bowPressureRelay)
            .withOptionsFrom(*bowPositionRelay)
            .withOptionsFrom(*rosinRelay)
            .withOptionsFrom(*bowNoiseRelay)
            .withOptionsFrom(*releaseRelay)
            .withOptionsFrom(*bodySizeRelay)
            .withOptionsFrom(*bodyDampingRelay)
            .withOptionsFrom(*bodyMixRelay)
            .withOptionsFrom(*brightnessRelay)
            .withOptionsFrom(*stringTensionRelay)
            .withOptionsFrom(*stringStiffnessRelay)
            .withOptionsFrom(*activeStringsRelay)
            .withOptionsFrom(*detuneERelay)
            .withOptionsFrom(*detuneARelay)
            .withOptionsFrom(*detuneDRelay)
            .withOptionsFrom(*detuneGRelay)
            .withOptionsFrom(*expressionMacroRelay)
            .withOptionsFrom(*vibratoRateRelay)
            .withOptionsFrom(*vibratoDepthRelay)
            .withOptionsFrom(*vibratoOnsetRelay)
            .withOptionsFrom(*slowLfoRateRelay)
            .withOptionsFrom(*slowLfoDepthRelay)
            .withOptionsFrom(*infiniteSustainRelay)
            .withOptionsFrom(*subHarmonicsRelay)
            .withOptionsFrom(*outputGainRelay)
            .withOptionsFrom(*widthRelay)
            .withOptionsFrom(*masterSatRelay)
            .withOptionsFrom(*limiterCeilingRelay)
            .withOptionsFrom(*referencePitchRelay)
            .withOptionsFrom(*tuningSystemRelay)
            .withOptionsFrom(*noteExpressionRelay)
    );

    addAndMakeVisible(*webView);

    // ------------------------------------------------------------------
    // 3️⃣ Create attachments LAST (relay + APVTS param, IDs exact)
    // ------------------------------------------------------------------
    auto& apvts = processorRef.parameters;

    bowSpeedAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("BOW_SPEED"), *bowSpeedRelay, nullptr);
    bowPressureAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("BOW_PRESSURE"), *bowPressureRelay, nullptr);
    bowPositionAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("BOW_POSITION"), *bowPositionRelay, nullptr);
    rosinAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("ROSIN"), *rosinRelay, nullptr);
    bowNoiseAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("BOW_NOISE"), *bowNoiseRelay, nullptr);
    releaseAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("RELEASE"), *releaseRelay, nullptr);

    bodySizeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("BODY_SIZE"), *bodySizeRelay, nullptr);
    bodyDampingAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("BODY_DAMPING"), *bodyDampingRelay, nullptr);
    bodyMixAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("BODY_MIX"), *bodyMixRelay, nullptr);
    brightnessAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("BRIGHTNESS"), *brightnessRelay, nullptr);

    stringTensionAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("STRING_TENSION"), *stringTensionRelay, nullptr);
    stringStiffnessAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("STRING_STIFFNESS"), *stringStiffnessRelay, nullptr);
    activeStringsAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("ACTIVE_STRINGS"), *activeStringsRelay, nullptr);
    detuneEAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("DETUNE_E"), *detuneERelay, nullptr);
    detuneAAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("DETUNE_A"), *detuneARelay, nullptr);
    detuneDAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("DETUNE_D"), *detuneDRelay, nullptr);
    detuneGAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("DETUNE_G"), *detuneGRelay, nullptr);

    expressionMacroAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("EXPRESSION_MACRO"), *expressionMacroRelay, nullptr);
    vibratoRateAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("VIBRATO_RATE"), *vibratoRateRelay, nullptr);
    vibratoDepthAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("VIBRATO_DEPTH"), *vibratoDepthRelay, nullptr);
    vibratoOnsetAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("VIBRATO_ONSET"), *vibratoOnsetRelay, nullptr);
    slowLfoRateAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("SLOW_LFO_RATE"), *slowLfoRateRelay, nullptr);
    slowLfoDepthAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("SLOW_LFO_DEPTH"), *slowLfoDepthRelay, nullptr);

    infiniteSustainAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("INFINITE_SUSTAIN"), *infiniteSustainRelay, nullptr);
    subHarmonicsAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("SUB_HARMONICS"), *subHarmonicsRelay, nullptr);

    outputGainAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("OUTPUT_GAIN"), *outputGainRelay, nullptr);
    widthAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("WIDTH"), *widthRelay, nullptr);
    masterSatAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("MASTER_SAT_AMOUNT"), *masterSatRelay, nullptr);
    limiterCeilingAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("LIMITER_CEILING_DB"), *limiterCeilingRelay, nullptr);

    referencePitchAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("REFERENCE_PITCH"), *referencePitchRelay, nullptr);
    tuningSystemAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *apvts.getParameter("TUNING_SYSTEM"), *tuningSystemRelay, nullptr);
    noteExpressionAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *apvts.getParameter("NOTE_EXPRESSION"), *noteExpressionRelay, nullptr);

    webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());

    // Viz feed timer — 30 Hz is the budget ceiling; VU spec says 16 Hz is
    // sufficient. If C++ ends up feeding ONLY the VU, drop to startTimerHz(16).
    startTimerHz(30);

    // Fixed 1000x650 per finalized mockup v1 — non-resizable.
    setResizable(false, false);
    setSize(1000, 650);
}

OContrabassAudioProcessorEditor::~OContrabassAudioProcessorEditor()
{
    stopTimer();
}

void OContrabassAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xffEDD9BE));   // bg-paper — avoids white flash before WebView paints
}

void OContrabassAudioProcessorEditor::resized()
{
    webView->setBounds(getLocalBounds());
}

void OContrabassAudioProcessorEditor::timerCallback()
{
    // IN-15 pattern (O-GrainScatter): the emit below is visibility-gated, but
    // building per-tick JSON is pure churn while the UI is hidden. Skip early.
    if (! webView->isShowing())
        return;

    // ------------------------------------------------------------------
    // vuLevel feed (Task 12) — real post-limiter/post-gain RMS published by
    // the processor at the end of processBlock (relaxed atomic, single
    // writer). JSON payload contract set by the mockup JS: {"db": N}.
    // ------------------------------------------------------------------
    const float rmsDb = processorRef.getOutputRmsDb();
    webView->emitEventIfBrowserIsVisible("vuLevel",
        juce::var("{\"db\":" + juce::String(rmsDb, 2) + "}"));

    // ------------------------------------------------------------------
    // bowState feed (Task 13, D5) — DSP-true effective bow operating point
    // (post-LFO/macro/MPE) from the most-recently-started active voice.
    // JS eases the Schelleng dot to this when active; falls back to raw
    // SliderState values at silence.
    // ------------------------------------------------------------------
    const auto bs = processorRef.getBowStateViz();
    webView->emitEventIfBrowserIsVisible("bowState",
        juce::var("{\"v\":"  + juce::String(bs.speed, 4)
                + ",\"p\":"  + juce::String(bs.pressure, 4)
                + ",\"b\":"  + juce::String(bs.beta, 4)
                + ",\"active\":" + (bs.active ? juce::String("true") : juce::String("false"))
                + "}"));
}

/*  RESOURCE PROVIDER — receives BARE PATHS, not full URLs (MEMORY-critical).
    Compare with direct equality; never strip scheme/host. Missing entries
    here = silent 404 → blank UI / "Frame load interrupted".                  */
std::optional<juce::WebBrowserComponent::Resource>
OContrabassAudioProcessorEditor::getResource(const juce::String& url)
{
    auto makeVector = [](const char* data, int size) {
        return std::vector<std::byte>(
            reinterpret_cast<const std::byte*>(data),
            reinterpret_cast<const std::byte*>(data) + size);
    };

    if (url == "/" || url == "/index.html")
    {
        return juce::WebBrowserComponent::Resource{
            makeVector(BinaryData::index_html, BinaryData::index_htmlSize),
            juce::String("text/html")};
    }

    // MIME must be application/javascript or the module import silently fails.
    if (url == "/js/juce/index.js")
    {
        return juce::WebBrowserComponent::Resource{
            makeVector(BinaryData::index_js, BinaryData::index_jsSize),
            juce::String("application/javascript")};
    }

    if (url == "/js/juce/check_native_interop.js")
    {
        return juce::WebBrowserComponent::Resource{
            makeVector(BinaryData::check_native_interop_js, BinaryData::check_native_interop_jsSize),
            juce::String("application/javascript")};
    }

    // Stage 3 Task 8 — canonical preset-manager module (ES module import).
    if (url == "/js/preset-manager.js")
    {
        return juce::WebBrowserComponent::Resource{
            makeVector(BinaryData::presetmanager_js, BinaryData::presetmanager_jsSize),
            juce::String("application/javascript")};
    }

    // Stage 3 Task 9 — shared tuning panel (lazy ES module import) + CSS.
    if (url == "/js/tuning-panel.js")
    {
        return juce::WebBrowserComponent::Resource{
            makeVector(BinaryData::tuningpanel_js, BinaryData::tuningpanel_jsSize),
            juce::String("application/javascript")};
    }

    if (url == "/css/tuning-panel.css")
    {
        return juce::WebBrowserComponent::Resource{
            makeVector(BinaryData::tuningpanel_css, BinaryData::tuningpanel_cssSize),
            juce::String("text/css")};
    }

    juce::Logger::writeToLog("O-Contrabass resource not found: " + url);
    return std::nullopt;
}

/*  ASYNC FILE PICKER — SafePointer + bare-return-on-null contract
    (MEMORY: pattern_webview_launchasync_safepointer_no_complete).
    The NativeFunctionCompletion is OWNED by the WebView Impl. If the editor
    is torn down while the chooser is open, that Impl is dead — calling
    completion (even completion(false)) is a use-after-free. Bail with a
    bare `return` on the null-SafePointer path. Cancel-with-live-editor is
    safe to complete normally.                                                */
void OContrabassAudioProcessorEditor::launchTuningFilePicker(
    juce::WebBrowserComponent::NativeFunctionCompletion completion)
{
    // D1: filter restricted to *.scl — TuningEngine 2.1.0 has no TUN parser;
    // advertising .tun would be a silent-failure trap. AnaMark TUN parser is
    // v1.1 backlog (shared-module upgrade).
    if (fileDialogOpen)
    {
        completion(juce::var(false));
        return;
    }
    fileDialogOpen = true;

    fileChooser = std::make_shared<juce::FileChooser>(
        "Load Scala tuning file (.scl)",
        juce::File::getSpecialLocation(juce::File::userHomeDirectory),
        "*.scl");

    auto safeThis = juce::Component::SafePointer<OContrabassAudioProcessorEditor>(this);

    fileChooser->launchAsync(
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [safeThis, completion = std::move(completion)](const juce::FileChooser& fc) mutable
        {
            if (safeThis == nullptr)
                return;   // editor gone — completion is dangling; DO NOT call it

            safeThis->fileDialogOpen = false;

            const auto file = fc.getResult();
            if (file == juce::File{})
            {
                completion(juce::var(false));   // user cancelled — editor alive, safe
                return;
            }

            // TuningEngine handles .scl via processor entry point (Phase 2.6b).
            const bool ok = safeThis->processorRef.loadScalaFile(file);
            completion(juce::var(ok));
        });
}
