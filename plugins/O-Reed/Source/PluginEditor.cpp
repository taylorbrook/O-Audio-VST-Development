/*
   This file is part of O-Reed, an Ouaricon Audio plugin.
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

    O-Reed - Editor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginEditor.h"
#include "BinaryData.h"
#include "ScaleGenerator.h"

// v1.7.1: serialize an interval list through juce::JSON rather than string
// concatenation, so the payload can never be malformed (same helper as O-Wind).
static juce::String intervalsToJson (const std::vector<double>& intervals)
{
    juce::Array<juce::var> arr;
    for (auto v : intervals)
        arr.add (v);
    return juce::JSON::toString (juce::var (arr), true);
}

OReedAudioProcessorEditor::OReedAudioProcessorEditor(OReedAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    // ===================================================================
    // 1. CREATE RELAYS (must be created BEFORE WebView)
    // ===================================================================

    // Primary Controls
    breathPressureRelay   = std::make_unique<juce::WebSliderRelay>("breathPressure");
    embouchureRelay       = std::make_unique<juce::WebSliderRelay>("embouchure");
    reedHardnessRelay     = std::make_unique<juce::WebSliderRelay>("reedHardness");
    boreCharacterRelay    = std::make_unique<juce::WebSliderRelay>("boreCharacter");
    instrumentPresetRelay = std::make_unique<juce::WebComboBoxRelay>("instrumentPreset");

    // Secondary Controls
    reedOpeningRelay      = std::make_unique<juce::WebSliderRelay>("reedOpening");
    bellSizeRelay         = std::make_unique<juce::WebSliderRelay>("bellSize");
    airNoiseRelay         = std::make_unique<juce::WebSliderRelay>("airNoise");
    doubleReedRelay       = std::make_unique<juce::WebSliderRelay>("doubleReed");
    boreDiameterRelay     = std::make_unique<juce::WebSliderRelay>("boreDiameter");

    // Advanced / Sound Design
    reedMassRelay         = std::make_unique<juce::WebSliderRelay>("reedMass");
    reedDampingRelay      = std::make_unique<juce::WebSliderRelay>("reedDamping");
    mouthpieceVolRelay    = std::make_unique<juce::WebSliderRelay>("mouthpieceVol");
    toneHoleCutoffRelay   = std::make_unique<juce::WebSliderRelay>("toneHoleCutoff");
    registerHoleRelay     = std::make_unique<juce::WebSliderRelay>("registerHole");
    boreLengthRelay       = std::make_unique<juce::WebSliderRelay>("boreLength");
    boreProfileRelay      = std::make_unique<juce::WebComboBoxRelay>("boreProfile");

    // Expressive Controls
    vibratoDepthRelay     = std::make_unique<juce::WebSliderRelay>("vibratoDepth");
    vibratoRateRelay      = std::make_unique<juce::WebSliderRelay>("vibratoRate");
    vibratoSourceRelay    = std::make_unique<juce::WebComboBoxRelay>("vibratoSource");
    growlAmountRelay      = std::make_unique<juce::WebSliderRelay>("growlAmount");
    flutterTongueRelay    = std::make_unique<juce::WebSliderRelay>("flutterTongue");
    subtoneRelay          = std::make_unique<juce::WebSliderRelay>("subtone");
    attackChiffRelay      = std::make_unique<juce::WebSliderRelay>("attackChiff");

    // Impossible Physics
    infiniteSustainRelay  = std::make_unique<juce::WebSliderRelay>("infiniteSustain");
    reverseBoreRelay      = std::make_unique<juce::WebSliderRelay>("reverseBore");
    dualBoreRelay         = std::make_unique<juce::WebToggleButtonRelay>("dualBore");
    dronePitchRelay       = std::make_unique<juce::WebSliderRelay>("dronePitch");
    feedbackPathRelay     = std::make_unique<juce::WebSliderRelay>("feedbackPath");

    // Tuning
    referencePitchRelay   = std::make_unique<juce::WebSliderRelay>("referencePitch");
    tuningSystemRelay     = std::make_unique<juce::WebComboBoxRelay>("tuningSystem");

    // Voice Configuration
    polyModeRelay         = std::make_unique<juce::WebComboBoxRelay>("polyMode");
    maxVoicesRelay        = std::make_unique<juce::WebSliderRelay>("maxVoices");
    oversamplingRelay     = std::make_unique<juce::WebComboBoxRelay>("oversampling");

    // Output
    outputGainRelay       = std::make_unique<juce::WebSliderRelay>("outputGain");

    // ===================================================================
    // 2. CREATE WEBVIEW with all relays registered
    // ===================================================================
    // v1.7.1 — WebView2 user-data folder in the per-user app-data tree (was bare
    // %TEMP%, which cleaners wipe and other processes can lock), created before
    // WebView2 sees it. DAW hosts deny WebView2's default location. The WinWebView2
    // options are ignored off Windows.
    const auto webView2DataFolder = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                                        .getChildFile ("Ouaricon")
                                        .getChildFile ("WebView2")
                                        .getChildFile ("O-Reed");
   #if JUCE_WINDOWS
    webView2DataFolder.createDirectory();
   #endif

    const auto options = juce::WebBrowserComponent::Options{}
            .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
            .withWinWebView2Options(
                juce::WebBrowserComponent::Options::WinWebView2{}
                    .withUserDataFolder (webView2DataFolder))
            .withNativeIntegrationEnabled()
            .withResourceProvider([this](const juce::String& url) {
                return getResource(url);
            })
            // Primary Controls
            .withOptionsFrom(*breathPressureRelay)
            .withOptionsFrom(*embouchureRelay)
            .withOptionsFrom(*reedHardnessRelay)
            .withOptionsFrom(*boreCharacterRelay)
            .withOptionsFrom(*instrumentPresetRelay)
            // Secondary Controls
            .withOptionsFrom(*reedOpeningRelay)
            .withOptionsFrom(*bellSizeRelay)
            .withOptionsFrom(*airNoiseRelay)
            .withOptionsFrom(*doubleReedRelay)
            .withOptionsFrom(*boreDiameterRelay)
            // Advanced / Sound Design
            .withOptionsFrom(*reedMassRelay)
            .withOptionsFrom(*reedDampingRelay)
            .withOptionsFrom(*mouthpieceVolRelay)
            .withOptionsFrom(*toneHoleCutoffRelay)
            .withOptionsFrom(*registerHoleRelay)
            .withOptionsFrom(*boreLengthRelay)
            .withOptionsFrom(*boreProfileRelay)
            // Expressive Controls
            .withOptionsFrom(*vibratoDepthRelay)
            .withOptionsFrom(*vibratoRateRelay)
            .withOptionsFrom(*vibratoSourceRelay)
            .withOptionsFrom(*growlAmountRelay)
            .withOptionsFrom(*flutterTongueRelay)
            .withOptionsFrom(*subtoneRelay)
            .withOptionsFrom(*attackChiffRelay)
            // Impossible Physics
            .withOptionsFrom(*infiniteSustainRelay)
            .withOptionsFrom(*reverseBoreRelay)
            .withOptionsFrom(*dualBoreRelay)
            .withOptionsFrom(*dronePitchRelay)
            .withOptionsFrom(*feedbackPathRelay)
            // Tuning
            .withOptionsFrom(*referencePitchRelay)
            .withOptionsFrom(*tuningSystemRelay)
            // Voice Configuration
            .withOptionsFrom(*polyModeRelay)
            .withOptionsFrom(*maxVoicesRelay)
            .withOptionsFrom(*oversamplingRelay)
            // Output
            .withOptionsFrom(*outputGainRelay)

            // Preset native functions
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

            // =========================================================
            // v1.7.1 — TUNING PANEL NATIVE FUNCTIONS
            //
            // The shared scala-tuning-engine panel (tuning tab) calls these 20
            // names through Juce.getNativeFunction(). None was registered, so
            // every call's promise hung and the tab was dead. Same shapes as
            // O-Wind's; the master tune goes through the referencePitch
            // parameter (processBlock forwards it to the engine).
            // =========================================================

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
                // Read the parameter, not the engine: processBlock copies it into
                // the engine, so the engine is stale until the next audio block.
                complete(static_cast<double>(
                    processorRef.getAPVTS().getRawParameterValue("referencePitch")->load()));
            })

            .withNativeFunction("setMasterTune", [this](const juce::Array<juce::var>& args, auto complete) {
                if (args.size() >= 1) {
                    // Route through the referencePitch parameter (single source of
                    // truth) — writing the engine directly would be overwritten by
                    // processBlock on the next block.
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
                // MSVC rejects SafePointer(this) init-captures in nested lambdas
                // (C2440/C2119) - hoist to a local and capture by value.
                juce::Component::SafePointer<OReedAudioProcessorEditor> safeThis(this);
                fileChooser->launchAsync(
                    juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                    [safeThis, complete](const juce::FileChooser& fc) {
                        if (safeThis == nullptr)
                            return;  // editor (and its WebView) gone: nothing to complete
                        safeThis->fileDialogOpen = false;
                        auto file = fc.getResult();
                        if (file.existsAsFile())
                            complete(safeThis->processorRef.getTuningEngine()->loadScalaFile(file));
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
                juce::Component::SafePointer<OReedAudioProcessorEditor> safeThis(this);
                fileChooser->launchAsync(
                    juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                    [safeThis, complete](const juce::FileChooser& fc) {
                        if (safeThis == nullptr)
                            return;
                        safeThis->fileDialogOpen = false;
                        auto file = fc.getResult();
                        if (file.existsAsFile())
                            complete(safeThis->processorRef.getTuningEngine()->loadKBMFile(file));
                        else
                            complete(false);
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
                juce::Component::SafePointer<OReedAudioProcessorEditor> safeThis(this);
                fileChooser->launchAsync(
                    juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                    [safeThis, complete](const juce::FileChooser& fc) {
                        if (safeThis == nullptr)
                            return;
                        safeThis->fileDialogOpen = false;
                        auto file = fc.getResult();
                        if (file != juce::File())
                            complete(file.replaceWithText(
                                safeThis->processorRef.getTuningEngine()->generateScalaFileContent()));
                        else
                            complete(false);
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
                juce::Component::SafePointer<OReedAudioProcessorEditor> safeThis(this);
                fileChooser->launchAsync(
                    juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                    [safeThis, complete](const juce::FileChooser& fc) {
                        if (safeThis == nullptr)
                            return;
                        safeThis->fileDialogOpen = false;
                        auto file = fc.getResult();
                        if (file != juce::File())
                            complete(file.replaceWithText(
                                safeThis->processorRef.getTuningEngine()->generateKBMFileContent()));
                        else
                            complete(false);
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

            .withNativeFunction("exportTuningHTML", [this](const juce::Array<juce::var>&, auto complete) {
                if (fileDialogOpen) { complete(false); return; }
                fileDialogOpen = true;
                fileChooser = std::make_shared<juce::FileChooser>(
                    "Export Tuning Documentation",
                    juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                        .getChildFile("tuning-export.html"),
                    "*.html");
                juce::Component::SafePointer<OReedAudioProcessorEditor> safeThis(this);
                fileChooser->launchAsync(
                    juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                    [safeThis, complete](const juce::FileChooser& fc) {
                        if (safeThis == nullptr)
                            return;
                        safeThis->fileDialogOpen = false;
                        auto file = fc.getResult();
                        if (file != juce::File())
                            complete(file.replaceWithText(TuningExporter::toHTML(
                                *safeThis->processorRef.getTuningEngine(), "O-Reed")));
                        else
                            complete(false);
                    });
            })

            // =========================================================
            // v1.2.0 — THE UI LANGUAGE PAIR
            //
            // Plain withNativeFunction, no relay. The page PULLS once at init;
            // nothing is pushed from this constructor, because the language is
            // not preset content and no preset path can change it behind the
            // page's back. A push from here would race the WebView's load.
            // =========================================================

            .withNativeFunction("getUiLanguage",
                [this](const juce::Array<juce::var>&, auto complete) {
                    complete(juce::var(OReedAudioProcessor::languageCode(
                                 processorRef.uiLanguage.load(std::memory_order_acquire))));
                })

            .withNativeFunction("setUiLanguage",
                [this](const juce::Array<juce::var>& args, auto complete) {
                    // languageIndex() maps anything that is not "fr" to 0, so an
                    // unexpected argument from the page degrades to English
                    // rather than being stored unvalidated.
                    if (args.size() > 0)
                        processorRef.uiLanguage.store(
                            OReedAudioProcessor::languageIndex(args[0].toString()),
                            std::memory_order_release);

                    complete(juce::var(OReedAudioProcessor::languageCode(
                                 processorRef.uiLanguage.load(std::memory_order_acquire))));
                });

   #if JUCE_WINDOWS
    // v1.7.1 — when the WebView2 environment cannot be created (runtime missing or
    // data folder unusable), JUCE silently builds the legacy IE control, which has
    // no resource provider. Probe first; on failure webView stays null and paint()
    // shows a native notice. Audio, parameters and automation don't need the WebView.
    const bool webView2Available = juce::WebBrowserComponent::areOptionsSupported (options);
   #else
    constexpr bool webView2Available = true;
   #endif

    if (webView2Available)
        webView = std::make_unique<juce::WebBrowserComponent> (options);

    // v1.7.1 — attachments below stay valid without a WebView: the relays drop
    // their events while no browser is registered.
    if (webView != nullptr)
    {
        addAndMakeVisible (webView.get());
    }
    else
    {
        webView2RuntimeLink = std::make_unique<juce::HyperlinkButton> (
            "Download the WebView2 Runtime",
            juce::URL ("https://developer.microsoft.com/microsoft-edge/webview2/"));
        webView2RuntimeLink->setColour (juce::HyperlinkButton::textColourId, juce::Colour (0xff4a6b35));   // --green-dark
        addAndMakeVisible (*webView2RuntimeLink);
    }

    // ===================================================================
    // 3. CREATE ATTACHMENTS (must be created AFTER WebView)
    // ===================================================================
    auto& apvts = processorRef.getAPVTS();

    // Primary Controls
    breathPressureAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("breathPressure"), *breathPressureRelay, nullptr);
    embouchureAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("embouchure"), *embouchureRelay, nullptr);
    reedHardnessAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("reedHardness"), *reedHardnessRelay, nullptr);
    boreCharacterAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("boreCharacter"), *boreCharacterRelay, nullptr);
    instrumentPresetAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *apvts.getParameter("instrumentPreset"), *instrumentPresetRelay, nullptr);

    // Secondary Controls
    reedOpeningAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("reedOpening"), *reedOpeningRelay, nullptr);
    bellSizeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("bellSize"), *bellSizeRelay, nullptr);
    airNoiseAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("airNoise"), *airNoiseRelay, nullptr);
    doubleReedAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("doubleReed"), *doubleReedRelay, nullptr);
    boreDiameterAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("boreDiameter"), *boreDiameterRelay, nullptr);

    // Advanced / Sound Design
    reedMassAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("reedMass"), *reedMassRelay, nullptr);
    reedDampingAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("reedDamping"), *reedDampingRelay, nullptr);
    mouthpieceVolAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("mouthpieceVol"), *mouthpieceVolRelay, nullptr);
    toneHoleCutoffAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("toneHoleCutoff"), *toneHoleCutoffRelay, nullptr);
    registerHoleAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("registerHole"), *registerHoleRelay, nullptr);
    boreLengthAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("boreLength"), *boreLengthRelay, nullptr);
    boreProfileAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *apvts.getParameter("boreProfile"), *boreProfileRelay, nullptr);

    // Expressive Controls
    vibratoDepthAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("vibratoDepth"), *vibratoDepthRelay, nullptr);
    vibratoRateAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("vibratoRate"), *vibratoRateRelay, nullptr);
    vibratoSourceAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *apvts.getParameter("vibratoSource"), *vibratoSourceRelay, nullptr);
    growlAmountAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("growlAmount"), *growlAmountRelay, nullptr);
    flutterTongueAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("flutterTongue"), *flutterTongueRelay, nullptr);
    subtoneAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("subtone"), *subtoneRelay, nullptr);
    attackChiffAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("attackChiff"), *attackChiffRelay, nullptr);

    // Impossible Physics
    infiniteSustainAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("infiniteSustain"), *infiniteSustainRelay, nullptr);
    reverseBoreAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("reverseBore"), *reverseBoreRelay, nullptr);
    dualBoreAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *apvts.getParameter("dualBore"), *dualBoreRelay, nullptr);
    dronePitchAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("dronePitch"), *dronePitchRelay, nullptr);
    feedbackPathAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("feedbackPath"), *feedbackPathRelay, nullptr);

    // Tuning
    referencePitchAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("referencePitch"), *referencePitchRelay, nullptr);
    tuningSystemAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *apvts.getParameter("tuningSystem"), *tuningSystemRelay, nullptr);

    // Voice Configuration
    polyModeAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *apvts.getParameter("polyMode"), *polyModeRelay, nullptr);
    maxVoicesAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("maxVoices"), *maxVoicesRelay, nullptr);
    oversamplingAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *apvts.getParameter("oversampling"), *oversamplingRelay, nullptr);

    // Output
    outputGainAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("outputGain"), *outputGainRelay, nullptr);

    // Navigate to embedded UI
    if (webView != nullptr)
        webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());

    // Set window size (900x600)
    setSize(900, 600);
}

OReedAudioProcessorEditor::~OReedAudioProcessorEditor()
{
}

void OReedAudioProcessorEditor::paint(juce::Graphics& g)
{
    // v1.7.1 — WebView2 could not start (Windows only; webView is null): paint a native
    // notice instead of JUCE's legacy-IE fallback page. Colours from the page CSS;
    // scaled down to fit short editors.
    if (webView == nullptr)
    {
        const float s = juce::jlimit (0.6f, 1.0f, (float) getHeight() / 260.0f);
        auto area = getLocalBounds().withSizeKeepingCentre (juce::jmin (640, getWidth() - 48),
                                                            juce::roundToInt (202.0f * s));
        g.fillAll (juce::Colour (0xfff5e6d3));   // --bg-paper

        g.setColour (juce::Colour (0xff3c2f2f));   // --brown-text
        g.setFont (juce::FontOptions (26.0f * s, juce::Font::bold));
        g.drawFittedText ("O-Reed needs Microsoft Edge WebView2",
                          area.removeFromTop (juce::roundToInt (40.0f * s)), juce::Justification::centred, 2);

        area.removeFromTop (juce::roundToInt (12.0f * s));
        g.setColour (juce::Colour (0xff5c4033));   // --brown-frame
        g.setFont (juce::FontOptions (17.0f * s));
        g.drawFittedText ("The interface is drawn with the WebView2 Runtime, which could not start on "
                          "this computer. Install it, then close and reopen the plugin window.\n"
                          "The instrument itself keeps playing, and host automation still "
                          "reaches every parameter.",
                          area.removeFromTop (juce::roundToInt (110.0f * s)), juce::Justification::centred, 5);
        return;
    }

    juce::ignoreUnused(g);  // WebView handles all painting
}

void OReedAudioProcessorEditor::resized()
{
    if (webView)
        webView->setBounds(getLocalBounds());

    if (webView2RuntimeLink != nullptr)
    {
        // Bottom row of the paint() notice block (same geometry).
        const float s = juce::jlimit (0.6f, 1.0f, (float) getHeight() / 260.0f);
        auto block = getLocalBounds().withSizeKeepingCentre (juce::jmin (640, getWidth() - 48),
                                                             juce::roundToInt (202.0f * s));
        webView2RuntimeLink->setFont (juce::FontOptions (17.0f * s, juce::Font::underlined), false);
        webView2RuntimeLink->setBounds (block.removeFromBottom (juce::roundToInt (32.0f * s))
                                             .withSizeKeepingCentre (320, juce::roundToInt (32.0f * s)));
    }
}

//==============================================================================
// Resource Provider - serves embedded UI files from BinaryData
//==============================================================================
std::optional<juce::WebBrowserComponent::Resource>
OReedAudioProcessorEditor::getResource(const juce::String& url)
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

    // v1.2.0: the label table. Embedded in the juce_add_binary_data SOURCES
    // block AND served here, in the same commit — a file that is one without
    // the other is a 404 that presents as a missing panel and nothing else.
    if (url == "/js/i18n.js")
        return makeResource(BinaryData::i18n_js,
                           BinaryData::i18n_jsSize,
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

    // v1.7.0: shared EB Garamond face (modules/ui/eb-garamond). The stylesheet
    // is served under /css/ and the three woff2 faces under /fonts/, where its
    // relative url('../fonts/...') lands.
    if (url == "/css/eb-garamond.css")
        return makeResource(BinaryData::ebgaramond_css,
                           BinaryData::ebgaramond_cssSize,
                           "text/css; charset=utf-8");

    if (url == "/fonts/EBGaramond-Regular.woff2")
        return makeResource(BinaryData::EBGaramondRegular_woff2,
                           BinaryData::EBGaramondRegular_woff2Size,
                           "font/woff2");

    if (url == "/fonts/EBGaramond-Italic.woff2")
        return makeResource(BinaryData::EBGaramondItalic_woff2,
                           BinaryData::EBGaramondItalic_woff2Size,
                           "font/woff2");

    if (url == "/fonts/EBGaramond-Bold.woff2")
        return makeResource(BinaryData::EBGaramondBold_woff2,
                           BinaryData::EBGaramondBold_woff2Size,
                           "font/woff2");

    DBG("Resource not found: " + url);
    return std::nullopt;
}
