/*
   This file is part of O-Wind, an Ouaricon Audio plugin.
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

// Serialize an interval list through juce::JSON rather than string
// concatenation, so the payload can never be malformed (IN-12)
static juce::String intervalsToJson (const std::vector<double>& intervals)
{
    juce::Array<juce::var> arr;
    for (auto v : intervals)
        arr.add (v);
    return juce::JSON::toString (juce::var (arr), true);
}

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

    // ADSR Envelope
    adsrEnabledRelay = std::make_unique<juce::WebToggleButtonRelay>("adsrEnabled");
    adsrAttackRelay = std::make_unique<juce::WebSliderRelay>("adsrAttack");
    adsrDecayRelay = std::make_unique<juce::WebSliderRelay>("adsrDecay");
    adsrSustainRelay = std::make_unique<juce::WebSliderRelay>("adsrSustain");
    adsrReleaseRelay = std::make_unique<juce::WebSliderRelay>("adsrRelease");

    // Tone Hole Toggle
    toneHoleToggleRelay = std::make_unique<juce::WebToggleButtonRelay>("toneHoleToggle");

    // Instrument Preset (int param as slider relay)
    instrumentPresetRelay = std::make_unique<juce::WebSliderRelay>("instrumentPreset");

    // Sound-tab params (CR-02: shipped without relays in v1.12.0)
    growlRelay = std::make_unique<juce::WebSliderRelay>("growl");
    formantRelay = std::make_unique<juce::WebSliderRelay>("formant");
    vibratoDriftDepthRelay = std::make_unique<juce::WebSliderRelay>("vibratoDriftDepth");
    vibratoDriftSpeedRelay = std::make_unique<juce::WebSliderRelay>("vibratoDriftSpeed");

    // Effects tab (CR-01: shipped without any relays in v1.14.0)
    chorusRateRelay = std::make_unique<juce::WebSliderRelay>("chorusRate");
    chorusDepthRelay = std::make_unique<juce::WebSliderRelay>("chorusDepth");
    chorusMixRelay = std::make_unique<juce::WebSliderRelay>("chorusMix");
    delayTimeRelay = std::make_unique<juce::WebSliderRelay>("delayTime");
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
    chorusBypassRelay = std::make_unique<juce::WebToggleButtonRelay>("chorusBypass");
    delayBypassRelay = std::make_unique<juce::WebToggleButtonRelay>("delayBypass");
    eqBypassRelay = std::make_unique<juce::WebToggleButtonRelay>("eqBypass");
    reverbBypassRelay = std::make_unique<juce::WebToggleButtonRelay>("reverbBypass");
    delayModeRelay = std::make_unique<juce::WebComboBoxRelay>("delayMode");

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
            // ADSR Envelope
            .withOptionsFrom(*adsrEnabledRelay)
            .withOptionsFrom(*adsrAttackRelay)
            .withOptionsFrom(*adsrDecayRelay)
            .withOptionsFrom(*adsrSustainRelay)
            .withOptionsFrom(*adsrReleaseRelay)
            // Tone Hole Toggle
            .withOptionsFrom(*toneHoleToggleRelay)
            // Instrument Preset
            .withOptionsFrom(*instrumentPresetRelay)
            // Sound-tab params (CR-02)
            .withOptionsFrom(*growlRelay)
            .withOptionsFrom(*formantRelay)
            .withOptionsFrom(*vibratoDriftDepthRelay)
            .withOptionsFrom(*vibratoDriftSpeedRelay)
            // Effects tab (CR-01)
            .withOptionsFrom(*chorusRateRelay)
            .withOptionsFrom(*chorusDepthRelay)
            .withOptionsFrom(*chorusMixRelay)
            .withOptionsFrom(*delayTimeRelay)
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
            .withOptionsFrom(*chorusBypassRelay)
            .withOptionsFrom(*delayBypassRelay)
            .withOptionsFrom(*eqBypassRelay)
            .withOptionsFrom(*reverbBypassRelay)
            .withOptionsFrom(*delayModeRelay)

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
                if (fileDialogOpen) { complete(juce::var("")); return; }
                fileDialogOpen = true;
                fileChooser = std::make_shared<juce::FileChooser>(
                    "Save Preset",
                    processorRef.getPresetManager().getUserPresetsDirectory(),
                    "*.json"
                );
                fileChooser->launchAsync(
                    juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                    [safeThis = juce::Component::SafePointer<OWindAudioProcessorEditor>(this),
                     complete](const juce::FileChooser& fc) {
                        // Editor destroyed while the dialog was up: bare return —
                        // `complete` is owned by the dead WebView, calling it is a UAF
                        if (safeThis == nullptr)
                            return;
                        safeThis->fileDialogOpen = false;
                        auto result = fc.getResult();
                        if (result == juce::File{})
                        {
                            complete(juce::var(""));
                            return;
                        }
                        auto name = result.getFileNameWithoutExtension();
                        auto& pm = safeThis->processorRef.getPresetManager();
                        // Honor the directory the user picked — savePreset()
                        // always writes to the user-presets dir (WR-12)
                        if (result.isAChildOf(pm.getUserPresetsDirectory()))
                            pm.savePreset(name);
                        else
                            pm.savePresetToFile(result.withFileExtension("json"));
                        complete(juce::var(name));
                    }
                );
            })

            // =============================================================
            // TUNING NATIVE FUNCTIONS
            // =============================================================

            .withNativeFunction("getTuningIntervals", [this](const juce::Array<juce::var>&, auto complete) {
                complete(intervalsToJson(processorRef.getTuningEngine()->getIntervals()));
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
                    // Route through the referencePitch parameter (single source of
                    // truth) — writing the engine directly diverges from the APVTS
                    // and snaps back to 440 on session reload (WR-11). The
                    // parameterChanged listener forwards to the engine.
                    if (auto* param = processorRef.getAPVTS().getParameter("referencePitch")) {
                        float hz = static_cast<float>(static_cast<double>(args[0]));
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
                    "*.scl;*.tun");
                fileChooser->launchAsync(
                    juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                    [safeThis = juce::Component::SafePointer<OWindAudioProcessorEditor>(this),
                     complete](const juce::FileChooser& fc) {
                        if (safeThis == nullptr)
                            return;  // bare return — see savePresetWithDialog
                        safeThis->fileDialogOpen = false;
                        auto file = fc.getResult();
                        if (file.existsAsFile()) {
                            bool success = safeThis->processorRef.getTuningEngine()->loadScalaFile(file);
                            complete(success);
                        } else {
                            complete(false);
                        }
                    });
            })

            .withNativeFunction("loadKBMFile", [this](const juce::Array<juce::var>&, auto complete) {
                if (fileDialogOpen) { complete(false); return; }
                fileDialogOpen = true;
                fileChooser = std::make_shared<juce::FileChooser>(
                    "Load Keyboard Mapping",
                    juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
                    "*.kbm");
                fileChooser->launchAsync(
                    juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                    [safeThis = juce::Component::SafePointer<OWindAudioProcessorEditor>(this),
                     complete](const juce::FileChooser& fc) {
                        if (safeThis == nullptr)
                            return;  // bare return — see savePresetWithDialog
                        safeThis->fileDialogOpen = false;
                        auto file = fc.getResult();
                        if (file.existsAsFile()) {
                            bool success = safeThis->processorRef.getTuningEngine()->loadKBMFile(file);
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
                    complete(intervalsToJson(ScaleGenerator::generateEDO(divisions, period)));
                    return;
                }
                complete(juce::var());
            })

            .withNativeFunction("generateHarmonicSeries", [](const juce::Array<juce::var>& args, auto complete) {
                if (args.size() >= 2) {
                    int startHarmonic = static_cast<int>(args[0]);
                    int endHarmonic = static_cast<int>(args[1]);
                    complete(intervalsToJson(ScaleGenerator::generateHarmonicSeries(startHarmonic, endHarmonic)));
                    return;
                }
                complete(juce::var());
            })

            .withNativeFunction("generateRank2", [](const juce::Array<juce::var>& args, auto complete) {
                if (args.size() >= 3) {
                    double generator = static_cast<double>(args[0]);
                    double period = static_cast<double>(args[1]);
                    int count = static_cast<int>(args[2]);
                    complete(intervalsToJson(ScaleGenerator::generateRank2(generator, period, count)));
                    return;
                }
                complete(juce::var());
            })

            .withNativeFunction("saveScalaFile", [this](const juce::Array<juce::var>&, auto complete) {
                if (fileDialogOpen) { complete(false); return; }
                fileDialogOpen = true;
                fileChooser = std::make_shared<juce::FileChooser>(
                    "Save Scala File",
                    juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                        .getChildFile("scale.scl"),
                    "*.scl");
                fileChooser->launchAsync(
                    juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                    [safeThis = juce::Component::SafePointer<OWindAudioProcessorEditor>(this),
                     complete](const juce::FileChooser& fc) {
                        if (safeThis == nullptr)
                            return;  // bare return — see savePresetWithDialog
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
                fileChooser->launchAsync(
                    juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                    [safeThis = juce::Component::SafePointer<OWindAudioProcessorEditor>(this),
                     complete](const juce::FileChooser& fc) {
                        if (safeThis == nullptr)
                            return;  // bare return — see savePresetWithDialog
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
                if (fileDialogOpen) { complete(false); return; }
                fileDialogOpen = true;
                fileChooser = std::make_shared<juce::FileChooser>(
                    "Export Tuning Documentation",
                    juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                        .getChildFile("tuning-export.html"),
                    "*.html");
                fileChooser->launchAsync(
                    juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                    [safeThis = juce::Component::SafePointer<OWindAudioProcessorEditor>(this),
                     complete](const juce::FileChooser& fc) {
                        if (safeThis == nullptr)
                            return;  // bare return — see savePresetWithDialog
                        safeThis->fileDialogOpen = false;
                        auto file = fc.getResult();
                        if (file != juce::File()) {
                            auto html = TuningExporter::toHTML(
                                *safeThis->processorRef.getTuningEngine(), "O-Wind");
                            // Report the actual write result (IN-15) — same
                            // pattern as saveScalaFile/saveKBMFile
                            complete(file.replaceWithText(html));
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

    // ADSR Envelope
    adsrEnabledAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *apvts.getParameter("adsrEnabled"), *adsrEnabledRelay, nullptr);
    adsrAttackAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("adsrAttack"), *adsrAttackRelay, nullptr);
    adsrDecayAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("adsrDecay"), *adsrDecayRelay, nullptr);
    adsrSustainAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("adsrSustain"), *adsrSustainRelay, nullptr);
    adsrReleaseAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("adsrRelease"), *adsrReleaseRelay, nullptr);

    // Tone Hole Toggle
    toneHoleToggleAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *apvts.getParameter("toneHoleToggle"), *toneHoleToggleRelay, nullptr);

    // Instrument Preset
    instrumentPresetAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("instrumentPreset"), *instrumentPresetRelay, nullptr);

    // Sound-tab params (CR-02)
    growlAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("growl"), *growlRelay, nullptr);
    formantAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("formant"), *formantRelay, nullptr);
    vibratoDriftDepthAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("vibratoDriftDepth"), *vibratoDriftDepthRelay, nullptr);
    vibratoDriftSpeedAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("vibratoDriftSpeed"), *vibratoDriftSpeedRelay, nullptr);

    // Effects tab (CR-01)
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
    chorusBypassAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *apvts.getParameter("chorusBypass"), *chorusBypassRelay, nullptr);
    delayBypassAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *apvts.getParameter("delayBypass"), *delayBypassRelay, nullptr);
    eqBypassAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *apvts.getParameter("eqBypass"), *eqBypassRelay, nullptr);
    reverbBypassAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *apvts.getParameter("reverbBypass"), *reverbBypassRelay, nullptr);
    delayModeAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *apvts.getParameter("delayMode"), *delayModeRelay, nullptr);

    // Navigate to embedded UI
    webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());

    // Set window size (900x600)
    setSize(900, 600);
}

OWindAudioProcessorEditor::~OWindAudioProcessorEditor()
{
    // Explicit destruction in reverse order for safety
    delayModeAttachment.reset();
    reverbBypassAttachment.reset();
    eqBypassAttachment.reset();
    delayBypassAttachment.reset();
    chorusBypassAttachment.reset();
    reverbShimmerAttachment.reset();
    reverbModAttachment.reset();
    reverbMixAttachment.reset();
    reverbPredelayAttachment.reset();
    reverbDampAttachment.reset();
    reverbSizeAttachment.reset();
    eqHighGainAttachment.reset();
    eqMidFreqAttachment.reset();
    eqMidGainAttachment.reset();
    eqLowGainAttachment.reset();
    delayMixAttachment.reset();
    delayFeedbackAttachment.reset();
    delayTimeAttachment.reset();
    chorusMixAttachment.reset();
    chorusDepthAttachment.reset();
    chorusRateAttachment.reset();
    vibratoDriftSpeedAttachment.reset();
    vibratoDriftDepthAttachment.reset();
    formantAttachment.reset();
    growlAttachment.reset();
    instrumentPresetAttachment.reset();
    toneHoleToggleAttachment.reset();
    adsrReleaseAttachment.reset();
    adsrSustainAttachment.reset();
    adsrDecayAttachment.reset();
    adsrAttackAttachment.reset();
    adsrEnabledAttachment.reset();
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
