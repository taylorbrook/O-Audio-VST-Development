/*
   This file is part of O-Bitrot, an Ouaricon Audio plugin.
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

OBitrotAudioProcessorEditor::OBitrotAudioProcessorEditor(OBitrotAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    // 1. Create relays FIRST (IDs verbatim from parameter-spec.md — BINDING)
    // Tape
    tapeEnableRelay    = std::make_unique<juce::WebToggleButtonRelay>("TAPE_ENABLE");
    tapeProbRelay      = std::make_unique<juce::WebSliderRelay>("TAPE_PROB");
    tapeStopProbRelay  = std::make_unique<juce::WebSliderRelay>("TAPE_STOP_PROB");
    tapeRampRelay      = std::make_unique<juce::WebSliderRelay>("TAPE_RAMP");
    tapeDropRelay      = std::make_unique<juce::WebSliderRelay>("TAPE_DROP");
    tapeWowRelay       = std::make_unique<juce::WebSliderRelay>("TAPE_WOW");
    tapeHissRelay      = std::make_unique<juce::WebSliderRelay>("TAPE_HISS");
    // CD Skip
    cdEnableRelay      = std::make_unique<juce::WebToggleButtonRelay>("CD_ENABLE");
    cdProbRelay        = std::make_unique<juce::WebSliderRelay>("CD_PROB");
    cdSeverityRelay    = std::make_unique<juce::WebSliderRelay>("CD_SEVERITY");
    cdSegmentRelay     = std::make_unique<juce::WebSliderRelay>("CD_SEGMENT");
    // Vinyl
    vinylEnableRelay   = std::make_unique<juce::WebToggleButtonRelay>("VINYL_ENABLE");
    vinylProbRelay     = std::make_unique<juce::WebSliderRelay>("VINYL_PROB");
    vinylRpmRelay      = std::make_unique<juce::WebComboBoxRelay>("VINYL_RPM");
    vinylPopRelay      = std::make_unique<juce::WebSliderRelay>("VINYL_POP");
    vinylWearRelay     = std::make_unique<juce::WebSliderRelay>("VINYL_WEAR");
    vinylWarpRelay     = std::make_unique<juce::WebSliderRelay>("VINYL_WARP");
    // Packet
    packetEnableRelay  = std::make_unique<juce::WebToggleButtonRelay>("PACKET_ENABLE");
    packetLossRelay    = std::make_unique<juce::WebSliderRelay>("PACKET_LOSS");
    packetBurstRelay   = std::make_unique<juce::WebSliderRelay>("PACKET_BURST");
    packetConcealRelay = std::make_unique<juce::WebComboBoxRelay>("PACKET_CONCEAL");
    packetComfortRelay = std::make_unique<juce::WebSliderRelay>("PACKET_COMFORT");
    // Codec
    codecEnableRelay   = std::make_unique<juce::WebToggleButtonRelay>("CODEC_ENABLE");
    codecModeRelay     = std::make_unique<juce::WebComboBoxRelay>("CODEC_MODE");
    codecMixRelay      = std::make_unique<juce::WebSliderRelay>("CODEC_MIX");
    codecNoiseRelay    = std::make_unique<juce::WebSliderRelay>("CODEC_NOISE");
    codecMainsRelay    = std::make_unique<juce::WebComboBoxRelay>("CODEC_MAINS");
    codecAgcRelay      = std::make_unique<juce::WebSliderRelay>("CODEC_AGC");
    // Crush
    crushEnableRelay   = std::make_unique<juce::WebToggleButtonRelay>("CRUSH_ENABLE");
    crushBitsRelay     = std::make_unique<juce::WebSliderRelay>("CRUSH_BITS");
    crushRateRelay     = std::make_unique<juce::WebSliderRelay>("CRUSH_RATE");
    crushJitterRelay   = std::make_unique<juce::WebSliderRelay>("CRUSH_JITTER");
    crushEnvAmtRelay   = std::make_unique<juce::WebSliderRelay>("CRUSH_ENV_AMT");
    crushDitherRelay   = std::make_unique<juce::WebSliderRelay>("CRUSH_DITHER");
    // Rot (v1.10.0)
    rotEnableRelay     = std::make_unique<juce::WebToggleButtonRelay>("ROT_ENABLE");
    rotProbRelay       = std::make_unique<juce::WebSliderRelay>("ROT_PROB");
    rotDepthRelay      = std::make_unique<juce::WebSliderRelay>("ROT_DEPTH");
    rotStickRelay      = std::make_unique<juce::WebSliderRelay>("ROT_STICK");
    rotGarbleRelay     = std::make_unique<juce::WebSliderRelay>("ROT_GARBLE");
    // Global
    clockModeRelay     = std::make_unique<juce::WebComboBoxRelay>("CLOCK_MODE");
    clockSyncDivRelay  = std::make_unique<juce::WebComboBoxRelay>("CLOCK_SYNC_DIV");
    clockFreeRateRelay = std::make_unique<juce::WebSliderRelay>("CLOCK_FREE_RATE");
    seedRelay          = std::make_unique<juce::WebSliderRelay>("SEED");
    hardEdgesRelay     = std::make_unique<juce::WebToggleButtonRelay>("HARD_EDGES");
    mixRelay           = std::make_unique<juce::WebSliderRelay>("MIX");

    // 2. Build WebView options: relays + the 10 preset native functions
    auto options =
        juce::WebBrowserComponent::Options{}
            .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
            .withWinWebView2Options(
                juce::WebBrowserComponent::Options::WinWebView2{}
                    .withUserDataFolder(juce::File::getSpecialLocation(
                        juce::File::SpecialLocationType::tempDirectory)
                            .getChildFile("OBitrot_WebView")))
            .withNativeIntegrationEnabled()
#if JUCE_WEB_BROWSER_RESOURCE_PROVIDER_AVAILABLE
            .withResourceProvider([this](const auto& url) { return getResource(url); })
#endif
            .withOptionsFrom(*tapeEnableRelay)
            .withOptionsFrom(*tapeProbRelay)
            .withOptionsFrom(*tapeStopProbRelay)
            .withOptionsFrom(*tapeRampRelay)
            .withOptionsFrom(*tapeDropRelay)
            .withOptionsFrom(*tapeWowRelay)
            .withOptionsFrom(*tapeHissRelay)
            .withOptionsFrom(*cdEnableRelay)
            .withOptionsFrom(*cdProbRelay)
            .withOptionsFrom(*cdSeverityRelay)
            .withOptionsFrom(*cdSegmentRelay)
            .withOptionsFrom(*vinylEnableRelay)
            .withOptionsFrom(*vinylProbRelay)
            .withOptionsFrom(*vinylRpmRelay)
            .withOptionsFrom(*vinylPopRelay)
            .withOptionsFrom(*vinylWearRelay)
            .withOptionsFrom(*vinylWarpRelay)
            .withOptionsFrom(*packetEnableRelay)
            .withOptionsFrom(*packetLossRelay)
            .withOptionsFrom(*packetBurstRelay)
            .withOptionsFrom(*packetConcealRelay)
            .withOptionsFrom(*packetComfortRelay)
            .withOptionsFrom(*codecEnableRelay)
            .withOptionsFrom(*codecModeRelay)
            .withOptionsFrom(*codecMixRelay)
            .withOptionsFrom(*codecNoiseRelay)
            .withOptionsFrom(*codecMainsRelay)
            .withOptionsFrom(*codecAgcRelay)
            .withOptionsFrom(*crushEnableRelay)
            .withOptionsFrom(*crushBitsRelay)
            .withOptionsFrom(*crushRateRelay)
            .withOptionsFrom(*crushJitterRelay)
            .withOptionsFrom(*crushEnvAmtRelay)
            .withOptionsFrom(*crushDitherRelay)
            .withOptionsFrom(*rotEnableRelay)
            .withOptionsFrom(*rotProbRelay)
            .withOptionsFrom(*rotDepthRelay)
            .withOptionsFrom(*rotStickRelay)
            .withOptionsFrom(*rotGarbleRelay)
            .withOptionsFrom(*clockModeRelay)
            .withOptionsFrom(*clockSyncDivRelay)
            .withOptionsFrom(*clockFreeRateRelay)
            .withOptionsFrom(*seedRelay)
            .withOptionsFrom(*hardEdgesRelay)
            .withOptionsFrom(*mixRelay);

    // ── HOVER-HELP PREFERENCE — 2 (v1.12.0) ────────────────────────────────
    // The "?" toggle's state is a UI preference, not a parameter, so it round-
    // trips through these two fns and the processor's state property rather
    // than through a relay. Both complete synchronously.

    options = options.withNativeFunction("setTooltipsEnabled",
        [this](const juce::Array<juce::var>& args, auto complete)
        {
            if (args.size() > 0)
                audioProcessor.tooltipsEnabled.store((bool) args[0],
                                                     std::memory_order_release);

            complete(juce::var(audioProcessor.tooltipsEnabled.load(
                                   std::memory_order_acquire)));
        });

    // PULLED by the page at init, never pushed. A push from the constructor or
    // from the 30 Hz timer tick fires before the inline module in index.html
    // has evaluated, so the preference silently never arrives and the toggle
    // reads OFF on every reopen — the O-FreqPulse WR-01 bug, avoided here by
    // construction.
    options = options.withNativeFunction("getTooltipsEnabled",
        [this](auto&, auto complete)
        {
            complete(juce::var(audioProcessor.tooltipsEnabled.load(
                                   std::memory_order_acquire)));
        });

    // ── PRESET NATIVE FUNCTIONS — 11 (Stage 4, +1 in v1.13.0) ──────────────
    // Ten are exactly the names modules/preset-manager.js requests; the
    // eleventh, getPresetListGrouped, is O-Bitrot's own and is consumed by the
    // preset MENU in index.html, not by the module. With the two hover-help
    // fns above the total registered surface is 13 and the grep-diff parity
    // gate runs at 13↔13. The synchronous nine capture `this`
    // (completion never outlives the call). The two DIALOG fns defer their
    // completion into a FileChooser callback: shared_ptr chooser captured
    // into its own callback, SafePointer HOISTED to a local (MSVC rejects
    // SafePointer(this) init-captures in nested lambdas), and on a dead
    // editor the callback RETURNS — even complete(false) would UAF the dead
    // WebView impl (pattern_webview_launchasync_safepointer_no_complete).
    // Both dialog fns complete with {success, name} OBJECTS — the JS reads
    // result.success / result.name; a bare bool reads as failure even when
    // the file was written.

    options = options.withNativeFunction("savePreset",
        [this](const juce::Array<juce::var>& args, auto complete)
        {
            if (args.size() > 0)
                complete(juce::var(audioProcessor.presetManager.savePreset(args[0].toString())));
            else
                complete(juce::var(false));
        });

    options = options.withNativeFunction("savePresetWithDialog",
        [this](auto&, auto complete)
        {
            auto userDir = audioProcessor.presetManager.getUserPresetsDirectory();
            userDir.createDirectory();

            auto chooser = std::make_shared<juce::FileChooser>("Save Preset", userDir, "*.json");
            juce::Component::SafePointer<OBitrotAudioProcessorEditor> safeThis(this);

            chooser->launchAsync(
                juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                [safeThis, chooser, complete](const juce::FileChooser& fc)
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
                        const auto file = results.getReference(0);
                        const bool ok = safeThis->audioProcessor.presetManager.savePresetToFile(file);
                        result->setProperty("success", ok);
                        result->setProperty("name", file.getFileNameWithoutExtension());
                    }
                    else
                    {
                        result->setProperty("success", false);
                        result->setProperty("name", juce::String());
                    }

                    complete(juce::var(result));
                });
        });

    options = options.withNativeFunction("loadPreset",
        [this](const juce::Array<juce::var>& args, auto complete)
        {
            if (args.size() > 0)
                complete(juce::var(audioProcessor.presetManager.loadPreset(args[0].toString())));
            else
                complete(juce::var(false));
        });

    options = options.withNativeFunction("loadPresetFromFile",
        [this](auto&, auto complete)
        {
            auto presetsDir = audioProcessor.presetManager.getPresetsDirectory();

            auto chooser = std::make_shared<juce::FileChooser>("Load Preset", presetsDir, "*.json");
            juce::Component::SafePointer<OBitrotAudioProcessorEditor> safeThis(this);

            chooser->launchAsync(
                juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                [safeThis, chooser, complete](const juce::FileChooser& fc)
                {
                    if (safeThis == nullptr)
                        return;   // dead editor — never touch complete

                    auto* result = new juce::DynamicObject();
                    const auto results = fc.getResults();

                    if (results.size() > 0)
                    {
                        const auto file = results.getReference(0);
                        const bool ok = safeThis->audioProcessor.presetManager.loadPresetFromFile(file);
                        result->setProperty("success", ok);
                        result->setProperty("name", file.getFileNameWithoutExtension());
                    }
                    else
                    {
                        result->setProperty("success", false);
                        result->setProperty("name", juce::String());
                    }

                    complete(juce::var(result));
                });
        });

    options = options.withNativeFunction("getPresetList",
        [this](auto&, auto complete)
        {
            juce::Array<juce::var> list;
            for (const auto& name : audioProcessor.presetManager.getPresetList())
                list.add(juce::var(name));
            complete(juce::var(list));
        });

    options = options.withNativeFunction("getCurrentPreset",
        [this](auto&, auto complete)
        {
            complete(juce::var(audioProcessor.presetManager.getCurrentPresetName()));
        });

    options = options.withNativeFunction("selectNextPreset",
        [this](auto&, auto complete)
        {
            complete(juce::var(audioProcessor.presetManager.getNextPreset()));
        });

    options = options.withNativeFunction("selectPreviousPreset",
        [this](auto&, auto complete)
        {
            complete(juce::var(audioProcessor.presetManager.getPreviousPreset()));
        });

    options = options.withNativeFunction("deletePreset",
        [this](const juce::Array<juce::var>& args, auto complete)
        {
            if (args.size() > 0)
                complete(juce::var(audioProcessor.presetManager.deletePreset(args[0].toString())));
            else
                complete(juce::var(false));
        });

    options = options.withNativeFunction("isFactoryPreset",
        [this](const juce::Array<juce::var>& args, auto complete)
        {
            if (args.size() > 0)
                complete(juce::var(audioProcessor.presetManager.isFactoryPreset(args[0].toString())));
            else
                complete(juce::var(false));
        });

    // getPresetListGrouped (v1.13.0) — the preset MENU's only data source.
    //
    // Returns an ARRAY of { category, presets: [...] }, deliberately not an
    // object keyed by category: an object would make section order depend on
    // JS string-key insertion-order semantics surviving the C++ → JSON → JS
    // round-trip, and the alternative (a CATEGORY_ORDER list in JS) is a
    // mirror of the C++ spans that would drift the first time a category is
    // added. An array carries the order in the data itself.
    //
    // Factory sections come first, in NARRATIVE order (factoryCategoryOrder),
    // then "User" holding everything getPresetList() returned that is not a
    // known factory name — alphabetical, because getPresetList() ends in
    // presets.sort(). Cross-checked against the live list rather than emitted
    // blind, so a factory preset whose file failed to write is absent from the
    // menu instead of listed and un-loadable. Empty categories are skipped.
    options = options.withNativeFunction("getPresetListGrouped",
        [this](auto&, auto complete)
        {
            const auto allPresets = audioProcessor.presetManager.getPresetList();

            juce::Array<juce::var> sections;
            juce::StringArray claimed;

            juce::String openCategory;
            juce::Array<juce::var> openNames;

            const auto flushSection = [&sections, &openCategory, &openNames]()
            {
                if (openCategory.isNotEmpty() && ! openNames.isEmpty())
                {
                    auto* section = new juce::DynamicObject();
                    section->setProperty("category", openCategory);
                    section->setProperty("presets", openNames);
                    sections.add(juce::var(section));
                }
                openNames.clear();
            };

            for (const auto& [presetName, categoryLabel] : audioProcessor.factoryCategoryOrder)
            {
                if (categoryLabel != openCategory)
                {
                    flushSection();
                    openCategory = categoryLabel;
                }

                if (allPresets.contains(presetName))
                {
                    openNames.add(juce::var(presetName));
                    claimed.add(presetName);
                }
            }
            flushSection();

            juce::Array<juce::var> userNames;
            for (const auto& presetName : allPresets)
                if (! claimed.contains(presetName))
                    userNames.add(juce::var(presetName));

            if (! userNames.isEmpty())
            {
                auto* section = new juce::DynamicObject();
                section->setProperty("category", "User");
                section->setProperty("presets", userNames);
                sections.add(juce::var(section));
            }

            complete(juce::var(sections));
        });

    webView = std::make_unique<juce::WebBrowserComponent>(options);

    addAndMakeVisible(*webView);

    // 3. Create attachments LAST (JUCE 8.0.14 three-arg form: param, relay, nullptr)
    // Tape
    tapeEnableAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *audioProcessor.apvts.getParameter("TAPE_ENABLE"), *tapeEnableRelay, nullptr);
    tapeProbAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("TAPE_PROB"), *tapeProbRelay, nullptr);
    tapeStopProbAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("TAPE_STOP_PROB"), *tapeStopProbRelay, nullptr);
    tapeRampAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("TAPE_RAMP"), *tapeRampRelay, nullptr);
    tapeDropAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("TAPE_DROP"), *tapeDropRelay, nullptr);
    tapeWowAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("TAPE_WOW"), *tapeWowRelay, nullptr);
    tapeHissAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("TAPE_HISS"), *tapeHissRelay, nullptr);
    // CD Skip
    cdEnableAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *audioProcessor.apvts.getParameter("CD_ENABLE"), *cdEnableRelay, nullptr);
    cdProbAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("CD_PROB"), *cdProbRelay, nullptr);
    cdSeverityAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("CD_SEVERITY"), *cdSeverityRelay, nullptr);
    cdSegmentAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("CD_SEGMENT"), *cdSegmentRelay, nullptr);
    // Vinyl
    vinylEnableAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *audioProcessor.apvts.getParameter("VINYL_ENABLE"), *vinylEnableRelay, nullptr);
    vinylProbAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("VINYL_PROB"), *vinylProbRelay, nullptr);
    vinylRpmAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *audioProcessor.apvts.getParameter("VINYL_RPM"), *vinylRpmRelay, nullptr);
    vinylPopAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("VINYL_POP"), *vinylPopRelay, nullptr);
    vinylWearAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("VINYL_WEAR"), *vinylWearRelay, nullptr);
    vinylWarpAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("VINYL_WARP"), *vinylWarpRelay, nullptr);
    // Packet
    packetEnableAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *audioProcessor.apvts.getParameter("PACKET_ENABLE"), *packetEnableRelay, nullptr);
    packetLossAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("PACKET_LOSS"), *packetLossRelay, nullptr);
    packetBurstAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("PACKET_BURST"), *packetBurstRelay, nullptr);
    packetConcealAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *audioProcessor.apvts.getParameter("PACKET_CONCEAL"), *packetConcealRelay, nullptr);
    packetComfortAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("PACKET_COMFORT"), *packetComfortRelay, nullptr);
    // Codec
    codecEnableAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *audioProcessor.apvts.getParameter("CODEC_ENABLE"), *codecEnableRelay, nullptr);
    codecModeAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *audioProcessor.apvts.getParameter("CODEC_MODE"), *codecModeRelay, nullptr);
    codecMixAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("CODEC_MIX"), *codecMixRelay, nullptr);
    codecNoiseAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("CODEC_NOISE"), *codecNoiseRelay, nullptr);
    codecMainsAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *audioProcessor.apvts.getParameter("CODEC_MAINS"), *codecMainsRelay, nullptr);
    codecAgcAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("CODEC_AGC"), *codecAgcRelay, nullptr);
    // Crush
    crushEnableAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *audioProcessor.apvts.getParameter("CRUSH_ENABLE"), *crushEnableRelay, nullptr);
    crushBitsAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("CRUSH_BITS"), *crushBitsRelay, nullptr);
    crushRateAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("CRUSH_RATE"), *crushRateRelay, nullptr);
    crushJitterAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("CRUSH_JITTER"), *crushJitterRelay, nullptr);
    crushEnvAmtAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("CRUSH_ENV_AMT"), *crushEnvAmtRelay, nullptr);
    crushDitherAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("CRUSH_DITHER"), *crushDitherRelay, nullptr);
    // Rot (v1.10.0)
    rotEnableAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *audioProcessor.apvts.getParameter("ROT_ENABLE"), *rotEnableRelay, nullptr);
    rotProbAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("ROT_PROB"), *rotProbRelay, nullptr);
    rotDepthAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("ROT_DEPTH"), *rotDepthRelay, nullptr);
    rotStickAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("ROT_STICK"), *rotStickRelay, nullptr);
    rotGarbleAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("ROT_GARBLE"), *rotGarbleRelay, nullptr);
    // Global
    clockModeAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *audioProcessor.apvts.getParameter("CLOCK_MODE"), *clockModeRelay, nullptr);
    clockSyncDivAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *audioProcessor.apvts.getParameter("CLOCK_SYNC_DIV"), *clockSyncDivRelay, nullptr);
    clockFreeRateAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("CLOCK_FREE_RATE"), *clockFreeRateRelay, nullptr);
    seedAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("SEED"), *seedRelay, nullptr);
    hardEdgesAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *audioProcessor.apvts.getParameter("HARD_EDGES"), *hardEdgesRelay, nullptr);
    mixAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("MIX"), *mixRelay, nullptr);

    // Load UI from resource provider
#if JUCE_WEB_BROWSER_RESOURCE_PROVIDER_AVAILABLE
    webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
#endif

    startTimerHz(30);

    // 620 -> 740 in v1.10.0: the Rot plate is a new full-width row between the
    // 3x2 family grid and the global strip. Growing the window rather than
    // re-flowing the grid to four columns keeps every existing panel's internal
    // layout pixel-identical — the four-column variant would have forced the
    // vinyl RPM switch, the packet conceal dropdown and both codec switches
    // into narrower boxes than their content needs.
    setSize(900, 740);
}

OBitrotAudioProcessorEditor::~OBitrotAudioProcessorEditor()
{
    stopTimer();
}

void OBitrotAudioProcessorEditor::paint(juce::Graphics& g)
{
    // Aged-paper base tone behind the WebView while it loads
    g.fillAll(juce::Colour(0xfff5e6d3));
}

void OBitrotAudioProcessorEditor::resized()
{
    webView->setBounds(getLocalBounds());
}

void OBitrotAudioProcessorEditor::timerCallback()
{
    // Event LED bridge — fire-and-forget at 30 Hz. Payload bits 0-4 =
    // tape, cd, vinyl, packet, rot. Codec/Crush LEDs are UI-side (enable
    // state); rot gets a real event bit because its events are discrete.
    if (! webView->isShowing())
        return;

    const auto mask = audioProcessor.uiActivityMask.load(std::memory_order_relaxed);
    // NOTE: juce::String("...") << x does not compile (repo pattern) — use +
    const juce::String payload = juce::String("{\"mask\":")
                               + juce::String(static_cast<int>(mask)) + "}";
    webView->emitEventIfBrowserIsVisible("ledUpdate", juce::var(payload));
}

std::optional<juce::WebBrowserComponent::Resource>
OBitrotAudioProcessorEditor::getResource(const juce::String& url)
{
    // Resource provider receives BARE paths ("/index.html"), never a scheme
    // (repo pattern: URL schemes differ per platform).
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

    if (url == "/modules/preset-manager.js")
    {
        return juce::WebBrowserComponent::Resource{
            makeVector(BinaryData::presetmanager_js, BinaryData::presetmanager_jsSize),
            juce::String("application/javascript")};
    }

    if (url == "/img/paper.jpg")
    {
        return juce::WebBrowserComponent::Resource{
            makeVector(BinaryData::paper_jpg, BinaryData::paper_jpgSize),
            juce::String("image/jpeg")};
    }

    if (url == "/img/specimen.webp")
    {
        return juce::WebBrowserComponent::Resource{
            makeVector(BinaryData::specimen_webp, BinaryData::specimen_webpSize),
            juce::String("image/webp")};
    }

    juce::Logger::writeToLog("Resource not found: " + url);
    return std::nullopt;
}
