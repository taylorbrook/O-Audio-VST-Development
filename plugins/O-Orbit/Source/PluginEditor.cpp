/*
   This file is part of O-Orbit, an Ouaricon Audio plugin.
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
#include "PluginEditor.h"
#include "BinaryData.h"

OOrbitEditor::OOrbitEditor (OOrbitProcessor& p)
    : AudioProcessorEditor (&p),
      processorRef (p)
{
    // 1. Create all 17 relays FIRST (constructor body, NOT initializer list)
    // CRITICAL: Parameter IDs MUST match APVTS exactly
    speedRelay = std::make_unique<juce::WebSliderRelay> ("speed");
    widthRelay = std::make_unique<juce::WebSliderRelay> ("width");
    depthRelay = std::make_unique<juce::WebSliderRelay> ("depth");
    tiltRelay = std::make_unique<juce::WebSliderRelay> ("tilt");
    phaseRelay = std::make_unique<juce::WebSliderRelay> ("phase");
    elevationRangeRelay = std::make_unique<juce::WebSliderRelay> ("elevation_range");
    distanceRelay = std::make_unique<juce::WebSliderRelay> ("distance");
    airAbsorptionRelay = std::make_unique<juce::WebSliderRelay> ("air_absorption");
    centerDivergeRelay = std::make_unique<juce::WebSliderRelay> ("center_diverge");
    lrOffsetRelay = std::make_unique<juce::WebSliderRelay> ("lr_offset");
    mixRelay = std::make_unique<juce::WebSliderRelay> ("mix");

    pathRelay = std::make_unique<juce::WebComboBoxRelay> ("path");
    tempoSyncRelay = std::make_unique<juce::WebComboBoxRelay> ("tempo_sync");
    speakerLayoutRelay = std::make_unique<juce::WebComboBoxRelay> ("speaker_layout");
    attenuationCurveRelay = std::make_unique<juce::WebComboBoxRelay> ("attenuation_curve");
    sourceModeRelay = std::make_unique<juce::WebComboBoxRelay> ("source_mode");

    elevationEnableRelay = std::make_unique<juce::WebToggleButtonRelay> ("elevation_enable");

    // 2. Build WebView options with all relays registered
    auto options = juce::WebBrowserComponent::Options{}
        .withBackend (juce::WebBrowserComponent::Options::Backend::webview2)
        .withNativeIntegrationEnabled()  // CRITICAL: Enables JUCE JavaScript library
        .withKeepPageLoadedWhenBrowserIsHidden()  // Prevents about:blank in FL Studio
        .withOptionsFrom (*speedRelay)
        .withOptionsFrom (*widthRelay)
        .withOptionsFrom (*depthRelay)
        .withOptionsFrom (*tiltRelay)
        .withOptionsFrom (*phaseRelay)
        .withOptionsFrom (*elevationRangeRelay)
        .withOptionsFrom (*distanceRelay)
        .withOptionsFrom (*airAbsorptionRelay)
        .withOptionsFrom (*centerDivergeRelay)
        .withOptionsFrom (*lrOffsetRelay)
        .withOptionsFrom (*mixRelay)
        .withOptionsFrom (*pathRelay)
        .withOptionsFrom (*tempoSyncRelay)
        .withOptionsFrom (*speakerLayoutRelay)
        .withOptionsFrom (*attenuationCurveRelay)
        .withOptionsFrom (*sourceModeRelay)
        .withOptionsFrom (*elevationEnableRelay)
        .withNativeFunction ("getSpeakerLayout", [this] (auto, auto complete) {
            auto& layout = processorRef.getCurrentLayout();
            juce::Array<juce::var> speakers;
            for (const auto& spk : layout.speakers)
            {
                auto* obj = new juce::DynamicObject();
                obj->setProperty ("azimuth", spk.azimuth);
                obj->setProperty ("elevation", spk.elevation);
                obj->setProperty ("distance", spk.distance);
                obj->setProperty ("label", spk.label);
                obj->setProperty ("isLFE", spk.isLFE);
                speakers.add (juce::var (obj));
            }
            complete (juce::var (speakers));
        })
        .withNativeFunction ("addSpeaker", [this] (const auto& args, auto complete) {
            if (args.size() >= 4)
            {
                processorRef.addSpeakerToLayout (
                    (float)(double) args[0], (float)(double) args[1],
                    (float)(double) args[2], args[3].toString());
                complete (juce::var (true));
            }
            else
                complete (juce::var (false));
        })
        .withNativeFunction ("removeSpeaker", [this] (const auto& args, auto complete) {
            if (args.size() >= 1)
            {
                processorRef.removeSpeakerFromLayout ((int) args[0]);
                complete (juce::var (true));
            }
            else
                complete (juce::var (false));
        })
        .withNativeFunction ("moveSpeaker", [this] (const auto& args, auto complete) {
            if (args.size() >= 3)
            {
                // v1.1.0 (D1): optional 4th arg edits distance; older callers
                // keep the speaker's current distance.
                const int index = (int) args[0];
                float distance = 1.0f;
                const auto& layout = processorRef.getCurrentLayout();
                if (index >= 0 && index < (int) layout.speakers.size())
                    distance = layout.speakers[(size_t) index].distance;
                if (args.size() >= 4)
                    distance = (float)(double) args[3];

                processorRef.moveSpeakerInLayout (
                    index, (float)(double) args[1], (float)(double) args[2], distance);
                complete (juce::var (true));
            }
            else
                complete (juce::var (false));
        })
        .withNativeFunction ("setCustomLayout", [this] (const auto& args, auto complete) {
            if (args.size() >= 1 && args[0].isArray())
            {
                SpeakerLayout layout;
                layout.name = "Custom";
                auto* arr = args[0].getArray();
                for (const auto& item : *arr)
                {
                    if (auto* obj = item.getDynamicObject())
                    {
                        Speaker spk;
                        spk.azimuth   = (float)(double) obj->getProperty ("azimuth");
                        spk.elevation = (float)(double) obj->getProperty ("elevation");
                        spk.distance  = (float)(double) obj->getProperty ("distance");
                        spk.label     = obj->getProperty ("label").toString();
                        spk.isLFE     = (bool) obj->getProperty ("isLFE");
                        if (spk.elevation != 0.0f) layout.is3D = true;
                        layout.speakers.push_back (spk);
                    }
                }
                processorRef.setCustomSpeakerLayout (layout);
                complete (juce::var (true));
            }
            else
                complete (juce::var (false));
        })
        .withNativeFunction ("getDownmixStatus", [this] (auto, auto complete) {
            auto& dmx = processorRef.getDownmixEngine();
            auto* obj = new juce::DynamicObject();
            obj->setProperty ("active", dmx.isActive());
            obj->setProperty ("sourceChannels", dmx.getSourceChannels());
            obj->setProperty ("targetChannels", dmx.getTargetChannels());
            complete (juce::var (obj));
        })
        .withNativeFunction ("exportLayout", [this] (auto, auto complete) {
            fileChooser = std::make_shared<juce::FileChooser> (
                "Export Speaker Layout", juce::File{}, "*.json");

            fileChooser->launchAsync (
                juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                [this, complete] (const juce::FileChooser& fc) {
                    auto result = fc.getResult();
                    if (result == juce::File{})
                    {
                        complete (juce::var (false));
                        return;
                    }

                    auto& layout = processorRef.getCurrentLayout();
                    juce::DynamicObject::Ptr root = new juce::DynamicObject();
                    root->setProperty ("name", layout.name);
                    root->setProperty ("is3D", layout.is3D);

                    juce::Array<juce::var> spkArr;
                    for (const auto& spk : layout.speakers)
                    {
                        auto* s = new juce::DynamicObject();
                        s->setProperty ("azimuth", spk.azimuth);
                        s->setProperty ("elevation", spk.elevation);
                        s->setProperty ("distance", spk.distance);
                        s->setProperty ("label", spk.label);
                        s->setProperty ("isLFE", spk.isLFE);
                        spkArr.add (juce::var (s));
                    }
                    root->setProperty ("speakers", juce::var (spkArr));

                    result.replaceWithText (juce::JSON::toString (juce::var (root.get())));
                    complete (juce::var (true));
                });
        })
        .withNativeFunction ("importLayout", [this] (auto, auto complete) {
            fileChooser = std::make_shared<juce::FileChooser> (
                "Import Speaker Layout", juce::File{}, "*.json");

            fileChooser->launchAsync (
                juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                [this, complete] (const juce::FileChooser& fc) {
                    auto result = fc.getResult();
                    if (result == juce::File{} || ! result.existsAsFile())
                    {
                        complete (juce::var (false));
                        return;
                    }

                    auto json = juce::JSON::parse (result.loadFileAsString());
                    if (auto* root = json.getDynamicObject())
                    {
                        SpeakerLayout layout;
                        layout.name = root->getProperty ("name").toString();
                        layout.is3D = (bool) root->getProperty ("is3D");

                        if (auto* arr = root->getProperty ("speakers").getArray())
                        {
                            for (const auto& item : *arr)
                            {
                                if (auto* obj = item.getDynamicObject())
                                {
                                    Speaker spk;
                                    spk.azimuth   = (float)(double) obj->getProperty ("azimuth");
                                    spk.elevation = (float)(double) obj->getProperty ("elevation");
                                    spk.distance  = (float)(double) obj->getProperty ("distance");
                                    spk.label     = obj->getProperty ("label").toString();
                                    spk.isLFE     = (bool) obj->getProperty ("isLFE");
                                    layout.speakers.push_back (spk);
                                }
                            }
                        }

                        if (layout.speakers.size() >= 2)
                        {
                            processorRef.setCustomSpeakerLayout (layout);

                            // Return the layout back to JS for updating the editor view
                            juce::Array<juce::var> spkResult;
                            for (const auto& spk : layout.speakers)
                            {
                                auto* s = new juce::DynamicObject();
                                s->setProperty ("azimuth", spk.azimuth);
                                s->setProperty ("elevation", spk.elevation);
                                s->setProperty ("distance", spk.distance);
                                s->setProperty ("label", spk.label);
                                s->setProperty ("isLFE", spk.isLFE);
                                spkResult.add (juce::var (s));
                            }
                            complete (juce::var (spkResult));
                            return;
                        }
                    }
                    complete (juce::var (false));
                });
        });

    // ── Hover-help native fns (B2, v1.1.0) ─────────────────────────────────
    // The page PULLS the persisted preference at init — a push from here or
    // from the 30 Hz timer would fire before the JS module has evaluated and
    // silently never arrive (the O-FreqPulse WR-01 bug).
    options = options.withNativeFunction ("setTooltipsEnabled",
        [this] (const juce::Array<juce::var>& args, auto complete)
        {
            if (args.size() > 0)
                processorRef.tooltipsEnabled.store ((bool) args[0], std::memory_order_release);
            complete (juce::var (true));
        });

    options = options.withNativeFunction ("getTooltipsEnabled",
        [this] (auto&, auto complete)
        {
            complete (juce::var (processorRef.tooltipsEnabled.load (std::memory_order_acquire)));
        });

    // ── Interface-language native fns (v1.2.0) ─────────────────────────────
    // The same shape and the same discipline as the toggle pair above: plain
    // withNativeFunction, no relay, PULLED once by the page at init. No push
    // from this constructor and none from the 30 Hz timer — the language is not
    // preset content, and OuariconPresetManager::loadPreset walks the preset's
    // "parameters" only, so no preset path can change it behind the page's back.
    options = options.withNativeFunction ("getUiLanguage",
        [this] (auto&, auto complete)
        {
            complete (juce::var (OOrbitProcessor::languageCode (
                                     processorRef.uiLanguage.load (std::memory_order_acquire))));
        });

    options = options.withNativeFunction ("setUiLanguage",
        [this] (const juce::Array<juce::var>& args, auto complete)
        {
            // languageIndex() maps anything that is not "fr" to 0, so an
            // unexpected argument from the page degrades to English rather than
            // being stored unvalidated.
            if (args.size() > 0)
                processorRef.uiLanguage.store (
                    OOrbitProcessor::languageIndex (args[0].toString()),
                    std::memory_order_release);

            complete (juce::var (OOrbitProcessor::languageCode (
                                     processorRef.uiLanguage.load (std::memory_order_acquire))));
        });

    // ── Preset native fns (B1, v1.1.0) — 11 ────────────────────────────────
    // Ten are exactly the names modules/preset-manager.js requests; the
    // eleventh, getPresetListGrouped, feeds the categorized preset MENU. The
    // synchronous nine capture `this` (completion never outlives the call).
    // The two DIALOG fns defer their completion into a FileChooser callback:
    // shared_ptr chooser captured into its own callback, SafePointer hoisted
    // to a local, and on a dead editor the callback RETURNS — even
    // complete(false) would UAF the dead WebView impl
    // (pattern_webview_launchasync_safepointer_no_complete). Both dialog fns
    // complete with {success, name} OBJECTS — the JS reads result.success.
    options = options.withNativeFunction ("savePreset",
        [this] (const juce::Array<juce::var>& args, auto complete)
        {
            if (args.size() > 0)
                complete (juce::var (processorRef.presetManager.savePreset (args[0].toString())));
            else
                complete (juce::var (false));
        });

    options = options.withNativeFunction ("savePresetWithDialog",
        [this] (auto&, auto complete)
        {
            auto userDir = processorRef.presetManager.getUserPresetsDirectory();
            userDir.createDirectory();

            auto chooser = std::make_shared<juce::FileChooser> ("Save Preset", userDir, "*.json");
            juce::Component::SafePointer<OOrbitEditor> safeThis (this);

            chooser->launchAsync (
                juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                [safeThis, chooser, complete] (const juce::FileChooser& fc)
                {
                    if (safeThis == nullptr)
                        return;   // dead editor — never touch complete

                    auto* result = new juce::DynamicObject();
                    const auto results = fc.getResults();

                    if (results.size() > 0)
                    {
                        // savePresetToFile HONORS the chosen path (the
                        // O-DigiDelay bug was a dialog whose destination a
                        // savePreset(name) call ignored).
                        const auto file = results.getReference (0);
                        const bool ok = safeThis->processorRef.presetManager.savePresetToFile (file);
                        result->setProperty ("success", ok);
                        result->setProperty ("name", file.getFileNameWithoutExtension());
                    }
                    else
                    {
                        result->setProperty ("success", false);
                        result->setProperty ("name", juce::String());
                    }

                    complete (juce::var (result));
                });
        });

    options = options.withNativeFunction ("loadPreset",
        [this] (const juce::Array<juce::var>& args, auto complete)
        {
            if (args.size() > 0)
                complete (juce::var (processorRef.presetManager.loadPreset (args[0].toString())));
            else
                complete (juce::var (false));
        });

    options = options.withNativeFunction ("loadPresetFromFile",
        [this] (auto&, auto complete)
        {
            auto presetsDir = processorRef.presetManager.getPresetsDirectory();

            auto chooser = std::make_shared<juce::FileChooser> ("Load Preset", presetsDir, "*.json");
            juce::Component::SafePointer<OOrbitEditor> safeThis (this);

            chooser->launchAsync (
                juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                [safeThis, chooser, complete] (const juce::FileChooser& fc)
                {
                    if (safeThis == nullptr)
                        return;   // dead editor — never touch complete

                    auto* result = new juce::DynamicObject();
                    const auto results = fc.getResults();

                    if (results.size() > 0)
                    {
                        const auto file = results.getReference (0);
                        const bool ok = safeThis->processorRef.presetManager.loadPresetFromFile (file);
                        result->setProperty ("success", ok);
                        result->setProperty ("name", file.getFileNameWithoutExtension());
                    }
                    else
                    {
                        result->setProperty ("success", false);
                        result->setProperty ("name", juce::String());
                    }

                    complete (juce::var (result));
                });
        });

    options = options.withNativeFunction ("getPresetList",
        [this] (auto&, auto complete)
        {
            juce::Array<juce::var> list;
            for (const auto& name : processorRef.presetManager.getPresetList())
                list.add (juce::var (name));
            complete (juce::var (list));
        });

    options = options.withNativeFunction ("getCurrentPreset",
        [this] (auto&, auto complete)
        {
            complete (juce::var (processorRef.presetManager.getCurrentPresetName()));
        });

    options = options.withNativeFunction ("selectNextPreset",
        [this] (auto&, auto complete)
        {
            complete (juce::var (processorRef.presetManager.getNextPreset()));
        });

    options = options.withNativeFunction ("selectPreviousPreset",
        [this] (auto&, auto complete)
        {
            complete (juce::var (processorRef.presetManager.getPreviousPreset()));
        });

    options = options.withNativeFunction ("deletePreset",
        [this] (const juce::Array<juce::var>& args, auto complete)
        {
            if (args.size() > 0)
                complete (juce::var (processorRef.presetManager.deletePreset (args[0].toString())));
            else
                complete (juce::var (false));
        });

    options = options.withNativeFunction ("isFactoryPreset",
        [this] (const juce::Array<juce::var>& args, auto complete)
        {
            if (args.size() > 0)
                complete (juce::var (processorRef.presetManager.isFactoryPreset (args[0].toString())));
            else
                complete (juce::var (false));
        });

    // getPresetListGrouped — the preset MENU's only data source. Returns an
    // ARRAY of { category, presets: [...] } so section order rides in the data
    // itself (an object's key order surviving the C++ → JSON → JS round-trip
    // is not guaranteed, and a CATEGORY_ORDER list in JS would drift). Factory
    // sections come first in narrative order, then "User" holding everything
    // getPresetList() returned that is not a known factory name. Cross-checked
    // against the live list so a factory preset whose file failed to write is
    // absent from the menu instead of listed and un-loadable.
    options = options.withNativeFunction ("getPresetListGrouped",
        [this] (auto&, auto complete)
        {
            const auto allPresets = processorRef.presetManager.getPresetList();

            juce::Array<juce::var> sections;
            juce::StringArray claimed;

            juce::String openCategory;
            juce::Array<juce::var> openNames;

            const auto flushSection = [&sections, &openCategory, &openNames]()
            {
                if (openCategory.isNotEmpty() && ! openNames.isEmpty())
                {
                    auto* section = new juce::DynamicObject();
                    section->setProperty ("category", openCategory);
                    section->setProperty ("presets", openNames);
                    sections.add (juce::var (section));
                }
                openNames.clear();
            };

            for (const auto& [presetName, categoryLabel] : processorRef.factoryCategoryOrder)
            {
                if (categoryLabel != openCategory)
                {
                    flushSection();
                    openCategory = categoryLabel;
                }

                if (allPresets.contains (presetName))
                {
                    openNames.add (juce::var (presetName));
                    claimed.add (presetName);
                }
            }
            flushSection();

            juce::Array<juce::var> userNames;
            for (const auto& presetName : allPresets)
                if (! claimed.contains (presetName))
                    userNames.add (juce::var (presetName));

            if (! userNames.isEmpty())
            {
                auto* section = new juce::DynamicObject();
                section->setProperty ("category", "User");
                section->setProperty ("presets", userNames);
                sections.add (juce::var (section));
            }

            complete (juce::var (sections));
        });

    // ── Named custom-layout library (D2, v1.1.0) — 4 fns ───────────────────
    // JSON files in ~/Library/Ouaricon Orbit/Layouts/, same schema as the
    // export/import file format, listed in the editor toolbar dropdown.
    options = options.withNativeFunction ("getLayoutList",
        [this] (auto&, auto complete)
        {
            juce::Array<juce::var> list;
            auto dir = processorRef.getLayoutsDirectory();
            if (dir.isDirectory())
            {
                juce::StringArray names;
                for (const auto& file : dir.findChildFiles (juce::File::findFiles, false, "*.json"))
                    names.add (file.getFileNameWithoutExtension());
                names.sort (true);
                for (const auto& name : names)
                    list.add (juce::var (name));
            }
            complete (juce::var (list));
        });

    options = options.withNativeFunction ("saveLayoutNamed",
        [this] (const juce::Array<juce::var>& args, auto complete)
        {
            // Name is the JSON filename — strip path separators or the save
            // silently lands elsewhere (critical_preset_name_slash_path_separator).
            auto name = args.size() > 0 ? args[0].toString().replaceCharacters ("/\\:", "___").trim()
                                        : juce::String();
            if (name.isEmpty())
            {
                complete (juce::var (false));
                return;
            }

            auto dir = processorRef.getLayoutsDirectory();
            dir.createDirectory();

            const auto& layout = processorRef.getCurrentLayout();
            juce::DynamicObject::Ptr root = new juce::DynamicObject();
            root->setProperty ("name", name);
            root->setProperty ("is3D", layout.is3D);

            juce::Array<juce::var> spkArr;
            for (const auto& spk : layout.speakers)
            {
                auto* s = new juce::DynamicObject();
                s->setProperty ("azimuth", spk.azimuth);
                s->setProperty ("elevation", spk.elevation);
                s->setProperty ("distance", spk.distance);
                s->setProperty ("label", spk.label);
                s->setProperty ("isLFE", spk.isLFE);
                spkArr.add (juce::var (s));
            }
            root->setProperty ("speakers", juce::var (spkArr));

            auto file = dir.getChildFile (name + ".json");
            complete (juce::var (file.replaceWithText (
                juce::JSON::toString (juce::var (root.get())))));
        });

    options = options.withNativeFunction ("loadLayoutNamed",
        [this] (const juce::Array<juce::var>& args, auto complete)
        {
            auto name = args.size() > 0 ? args[0].toString() : juce::String();
            auto file = processorRef.getLayoutsDirectory().getChildFile (name + ".json");
            if (! file.existsAsFile())
            {
                complete (juce::var (false));
                return;
            }

            auto json = juce::JSON::parse (file.loadFileAsString());
            if (auto* root = json.getDynamicObject())
            {
                SpeakerLayout layout;
                layout.name = root->getProperty ("name").toString();
                layout.is3D = (bool) root->getProperty ("is3D");

                if (auto* arr = root->getProperty ("speakers").getArray())
                {
                    for (const auto& item : *arr)
                    {
                        if (auto* obj = item.getDynamicObject())
                        {
                            Speaker spk;
                            spk.azimuth   = (float)(double) obj->getProperty ("azimuth");
                            spk.elevation = (float)(double) obj->getProperty ("elevation");
                            spk.distance  = (float)(double) obj->getProperty ("distance");
                            spk.label     = obj->getProperty ("label").toString();
                            spk.isLFE     = (bool) obj->getProperty ("isLFE");
                            layout.speakers.push_back (spk);
                        }
                    }
                }

                if (layout.speakers.size() >= 2)
                {
                    processorRef.setCustomSpeakerLayout (layout);

                    juce::Array<juce::var> spkResult;
                    for (const auto& spk : layout.speakers)
                    {
                        auto* s = new juce::DynamicObject();
                        s->setProperty ("azimuth", spk.azimuth);
                        s->setProperty ("elevation", spk.elevation);
                        s->setProperty ("distance", spk.distance);
                        s->setProperty ("label", spk.label);
                        s->setProperty ("isLFE", spk.isLFE);
                        spkResult.add (juce::var (s));
                    }
                    complete (juce::var (spkResult));
                    return;
                }
            }
            complete (juce::var (false));
        });

    options = options.withNativeFunction ("deleteLayoutNamed",
        [this] (const juce::Array<juce::var>& args, auto complete)
        {
            auto name = args.size() > 0 ? args[0].toString() : juce::String();
            auto file = processorRef.getLayoutsDirectory().getChildFile (name + ".json");
            complete (juce::var (file.existsAsFile() && file.deleteFile()));
        });

    // Windows-specific: set user data folder for plugin host compatibility
   #if JUCE_WINDOWS
    options = options.withWinWebView2Options (
        juce::WebBrowserComponent::Options::WinWebView2{}
            .withUserDataFolder (
                juce::File::getSpecialLocation (juce::File::tempDirectory)
                    .getChildFile ("OOrbit_WebView"))
            .withStatusBarDisabled()
            .withBuiltInErrorPageDisabled());
   #endif

    // Register resource provider (guarded for cross-platform compatibility)
   #if JUCE_WEB_BROWSER_RESOURCE_PROVIDER_AVAILABLE
    options = options.withResourceProvider (
        [this] (const auto& url) { return getResource (url); });
   #endif

    // Create WebView with options
    webView = std::make_unique<juce::WebBrowserComponent> (options);

    // 3. Create all 17 parameter attachments LAST
    // CRITICAL: 3 parameters required in JUCE 8 (parameter, relay, undoManager)
    speedAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *processorRef.parameters.getParameter ("speed"), *speedRelay, nullptr);
    widthAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *processorRef.parameters.getParameter ("width"), *widthRelay, nullptr);
    depthAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *processorRef.parameters.getParameter ("depth"), *depthRelay, nullptr);
    tiltAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *processorRef.parameters.getParameter ("tilt"), *tiltRelay, nullptr);
    phaseAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *processorRef.parameters.getParameter ("phase"), *phaseRelay, nullptr);
    elevationRangeAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *processorRef.parameters.getParameter ("elevation_range"), *elevationRangeRelay, nullptr);
    distanceAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *processorRef.parameters.getParameter ("distance"), *distanceRelay, nullptr);
    airAbsorptionAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *processorRef.parameters.getParameter ("air_absorption"), *airAbsorptionRelay, nullptr);
    centerDivergeAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *processorRef.parameters.getParameter ("center_diverge"), *centerDivergeRelay, nullptr);
    lrOffsetAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *processorRef.parameters.getParameter ("lr_offset"), *lrOffsetRelay, nullptr);
    mixAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *processorRef.parameters.getParameter ("mix"), *mixRelay, nullptr);

    pathAttachment = std::make_unique<juce::WebComboBoxParameterAttachment> (
        *processorRef.parameters.getParameter ("path"), *pathRelay, nullptr);
    tempoSyncAttachment = std::make_unique<juce::WebComboBoxParameterAttachment> (
        *processorRef.parameters.getParameter ("tempo_sync"), *tempoSyncRelay, nullptr);
    speakerLayoutAttachment = std::make_unique<juce::WebComboBoxParameterAttachment> (
        *processorRef.parameters.getParameter ("speaker_layout"), *speakerLayoutRelay, nullptr);
    attenuationCurveAttachment = std::make_unique<juce::WebComboBoxParameterAttachment> (
        *processorRef.parameters.getParameter ("attenuation_curve"), *attenuationCurveRelay, nullptr);
    sourceModeAttachment = std::make_unique<juce::WebComboBoxParameterAttachment> (
        *processorRef.parameters.getParameter ("source_mode"), *sourceModeRelay, nullptr);

    elevationEnableAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment> (
        *processorRef.parameters.getParameter ("elevation_enable"), *elevationEnableRelay, nullptr);

    // Add WebView to editor
    addAndMakeVisible (*webView);

    // Resizable (D4, v1.1.0): 800x600 default, fixed 4:3 aspect. The canvas
    // re-rasterizes on resize via the JS window resize listener (a canvas is a
    // CSS replaced element — its bitmap does not follow CSS size on its own).
    setResizable (true, true);
    setResizeLimits (600, 450, 1600, 1200);
    if (auto* constrainer = getConstrainer())
        constrainer->setFixedAspectRatio (800.0 / 600.0);
    setSize (800, 600);

    // Start 30Hz timer for motion state push to JS visualizer
    startTimerHz (30);
}

OOrbitEditor::~OOrbitEditor()
{
    stopTimer();  // CRITICAL: stop timer before member destruction
}

void OOrbitEditor::paint (juce::Graphics& g)
{
    // WebView handles all painting
    g.fillAll (juce::Colours::black);
}

void OOrbitEditor::resized()
{
    // WebView fills entire editor bounds
    webView->setBounds (getLocalBounds());
}

void OOrbitEditor::timerCallback()
{
    // Push motion state to JS visualizer at 30Hz via custom event
    float azL  = processorRef.getUIAzimuthL();
    float elL  = processorRef.getUIElevationL();
    float azR  = processorRef.getUIAzimuthR();
    float elR  = processorRef.getUIElevationR();
    float dist = processorRef.getUIDistance();
    bool isLRSplit = processorRef.parameters.getRawParameterValue ("source_mode")->load() > 0.5f;

    juce::String json = juce::String::formatted (
        "{\"azL\":%f,\"elL\":%f,\"azR\":%f,\"elR\":%f,\"dist\":%f,\"split\":%s}",
        azL, elL, azR, elR, dist, isLRSplit ? "true" : "false");

    webView->emitEventIfBrowserIsVisible ("motionUpdate", json);
}

void OOrbitEditor::parentHierarchyChanged()
{
    // Lazy navigation pattern: navigate only when editor becomes visible
    if (isShowing() && webView != nullptr && !hasNavigated)
    {
        webView->goToURL (juce::WebBrowserComponent::getResourceProviderRoot());
        hasNavigated = true;
    }
}

std::optional<juce::WebBrowserComponent::Resource>
OOrbitEditor::getResource (const juce::String& url)
{
    // Helper to convert BinaryData to vector<byte>
    auto makeVector = [] (const char* data, int size)
    {
        return std::vector<std::byte> (
            reinterpret_cast<const std::byte*> (data),
            reinterpret_cast<const std::byte*> (data) + size);
    };

    // Normalize URL
    auto resource = url.replaceCharacter ('\\', '/');

    // Root "/" → index.html
    if (resource == "/" || resource.isEmpty())
        resource = "/index.html";

    // Remove leading slash for BinaryData lookup
    auto path = resource.substring (1);

    // Explicit URL-to-BinaryData mapping
    // BinaryData naming: slashes/dots become underscores
    if (path == "index.html")
        return juce::WebBrowserComponent::Resource {
            makeVector (BinaryData::index_html, BinaryData::index_htmlSize),
            juce::String ("text/html") };

    if (path == "css/styles.css")
        return juce::WebBrowserComponent::Resource {
            makeVector (BinaryData::styles_css, BinaryData::styles_cssSize),
            juce::String ("text/css") };

    if (path == "js/app.js")
        return juce::WebBrowserComponent::Resource {
            makeVector (BinaryData::app_js, BinaryData::app_jsSize),
            juce::String ("application/javascript") };

    // v1.2.0 — the interface copy table, imported by js/app.js as './i18n.js'.
    // THIS BRANCH, the juce_add_binary_data SOURCES entry and the import all
    // land in one commit: any two of the three leaves the page 404ing at
    // runtime with nothing failing at build time.
    // charset=utf-8 on this one, unlike its neighbours: the French copy is full
    // of accented characters and typographic apostrophes, which mojibake on
    // some hosts without it.
    if (path == "js/i18n.js")
        return juce::WebBrowserComponent::Resource {
            makeVector (BinaryData::i18n_js, BinaryData::i18n_jsSize),
            juce::String ("application/javascript; charset=utf-8") };

    // BinaryData strips hyphens: preset-manager.js → presetmanager_js
    if (path == "js/modules/preset-manager.js")
        return juce::WebBrowserComponent::Resource {
            makeVector (BinaryData::presetmanager_js, BinaryData::presetmanager_jsSize),
            juce::String ("application/javascript") };

    if (path == "js/juce/index.js")
        return juce::WebBrowserComponent::Resource {
            makeVector (BinaryData::index_js, BinaryData::index_jsSize),
            juce::String ("application/javascript") };

    if (path == "js/juce/check_native_interop.js")
        return juce::WebBrowserComponent::Resource {
            makeVector (BinaryData::check_native_interop_js, BinaryData::check_native_interop_jsSize),
            juce::String ("application/javascript") };

    if (path == "img/shell.png")
        return juce::WebBrowserComponent::Resource {
            makeVector (BinaryData::shell_png, BinaryData::shell_pngSize),
            juce::String ("image/png") };

    // Resource not found
    juce::Logger::writeToLog ("O-Orbit: Resource not found: " + url);
    return std::nullopt;
}
