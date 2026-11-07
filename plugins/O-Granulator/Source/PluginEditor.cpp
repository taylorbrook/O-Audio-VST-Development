/*
   This file is part of O-Granulator, an Ouaricon Audio plugin.
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

    O-Granulator — Plugin Editor implementation (Stage 3 / Phase 3.1)

    From .planning/mockups/v2-PluginEditor.cpp (the v2 template; its shapes
    are the contract) with the PLAN-3.1 P103 deltas:
      (a) withResourceProvider → granulatorUiResource() (the Stage 1 resource
          map below, R114); the template's duplicate member getResource is gone;
      (b) getScaleState reads collection 31 through getCustomTable() — a copy
          taken under the custom-scale lock (R109) — via ScaleStateJson.h;
      (c) getTelemetry emits only the keys whose fields are live (P102).

    Processor surface used (PluginProcessor.h): parameters, uiLanguage +
    languageCode / languageIndex, getPresetManager(), the scale STATE
    accessors (getCollectionTable, getCustomScale / getCustomTable,
    get/setDegreeMask, setCustomScale, getScaleStateRevision — bumped on
    every scale-state change and on state restore / preset apply),
    readTelemetry, getWindowCurve.

  ==============================================================================
*/

#include "PluginEditor.h"
#include "ScaleStateJson.h"
#include "UIBinaryData.h"   // distinct NAMESPACE + HEADER_NAME — see CMakeLists juce_add_binary_data

namespace
{
    // ── Parameter IDs — must match parameter-spec.md / createParameterLayout()
    //    EXACTLY. An ID typo or a wrong relay TYPE is a silently dead control,
    //    so every lookup below is jassert'ed. Generated from v2-ui.yaml.
    const juce::StringArray kSliderIds {
        "input_gain",
        "buffer_length",
        "mix",
        "output_gain",
        "s1_level",
        "s1_pan",
        "s1_position",
        "s1_spray",
        "s1_size",
        "s1_size_rand",
        "s1_density",
        "s1_jitter",
        "s1_pitch",
        "s1_pitch_rand",
        "s1_reverse_prob",
        "s1_window",
        "s1_window_skew",
        "s1_stereo_spread",
        "s1_gain_rand",
        "s1_filter_cutoff",
        "s1_filter_rand",
        "s1_filter_res",
        "s1_drive",
        "s1_crush",
        "s1_crush_prob",
        "s1_ring_freq",
        "s1_ring_rand",
        "s1_ring_mix",
        "s2_level",
        "s2_pan",
        "s2_position",
        "s2_spray",
        "s2_size",
        "s2_size_rand",
        "s2_density",
        "s2_jitter",
        "s2_pitch",
        "s2_pitch_rand",
        "s2_reverse_prob",
        "s2_window",
        "s2_window_skew",
        "s2_stereo_spread",
        "s2_gain_rand",
        "s2_filter_cutoff",
        "s2_filter_rand",
        "s2_filter_res",
        "s2_drive",
        "s2_crush",
        "s2_crush_prob",
        "s2_ring_freq",
        "s2_ring_rand",
        "s2_ring_mix",
        "s3_level",
        "s3_pan",
        "s3_position",
        "s3_spray",
        "s3_size",
        "s3_size_rand",
        "s3_density",
        "s3_jitter",
        "s3_pitch",
        "s3_pitch_rand",
        "s3_reverse_prob",
        "s3_window",
        "s3_window_skew",
        "s3_stereo_spread",
        "s3_gain_rand",
        "s3_filter_cutoff",
        "s3_filter_rand",
        "s3_filter_res",
        "s3_drive",
        "s3_crush",
        "s3_crush_prob",
        "s3_ring_freq",
        "s3_ring_rand",
        "s3_ring_mix",
        "s4_level",
        "s4_pan",
        "s4_position",
        "s4_spray",
        "s4_size",
        "s4_size_rand",
        "s4_density",
        "s4_jitter",
        "s4_pitch",
        "s4_pitch_rand",
        "s4_reverse_prob",
        "s4_window",
        "s4_window_skew",
        "s4_stereo_spread",
        "s4_gain_rand",
        "s4_filter_cutoff",
        "s4_filter_rand",
        "s4_filter_res",
        "s4_drive",
        "s4_crush",
        "s4_crush_prob",
        "s4_ring_freq",
        "s4_ring_rand",
        "s4_ring_mix",
        "feedback",
        "fb_low_cut",
        "fb_high_cut",
        "fb_saturation",
        "fb_diffusion",
        "reverb_mix",
        "reverb_size",
        "reverb_damp",
        "lfo1_rate",
        "lfo1_phase",
        "lfo2_rate",
        "lfo2_phase",
        "lfo3_rate",
        "lfo3_phase",
        "lfo4_rate",
        "lfo4_phase",
        "env_attack",
        "env_release",
        "env_gain",
        "onset_threshold",
        "macro1",
        "macro2",
        "macro3",
        "macro4",
        "mod1_depth",
        "mod2_depth",
        "mod3_depth",
        "mod4_depth",
        "mod5_depth",
        "mod6_depth",
        "mod7_depth",
        "mod8_depth",
        "mod9_depth",
        "mod10_depth",
        "mod11_depth",
        "mod12_depth",
        "mod13_depth",
        "mod14_depth",
        "mod15_depth",
        "mod16_depth"
    };

    const juce::StringArray kComboIds {
        "sync_mode",
        "scale_root",
        "grain_limit",
        "s1_size_sync",
        "s1_density_sync",
        "s1_scale",
        "s1_filter_type",
        "s2_size_sync",
        "s2_density_sync",
        "s2_scale",
        "s2_filter_type",
        "s3_size_sync",
        "s3_density_sync",
        "s3_scale",
        "s3_filter_type",
        "s4_size_sync",
        "s4_density_sync",
        "s4_scale",
        "s4_filter_type",
        "lfo1_shape",
        "lfo1_rate_sync",
        "lfo2_shape",
        "lfo2_rate_sync",
        "lfo3_shape",
        "lfo3_rate_sync",
        "lfo4_shape",
        "lfo4_rate_sync",
        "mod1_source",
        "mod1_dest",
        "mod2_source",
        "mod2_dest",
        "mod3_source",
        "mod3_dest",
        "mod4_source",
        "mod4_dest",
        "mod5_source",
        "mod5_dest",
        "mod6_source",
        "mod6_dest",
        "mod7_source",
        "mod7_dest",
        "mod8_source",
        "mod8_dest",
        "mod9_source",
        "mod9_dest",
        "mod10_source",
        "mod10_dest",
        "mod11_source",
        "mod11_dest",
        "mod12_source",
        "mod12_dest",
        "mod13_source",
        "mod13_dest",
        "mod14_source",
        "mod14_dest",
        "mod15_source",
        "mod15_dest",
        "mod16_source",
        "mod16_dest"
    };

    const juce::StringArray kToggleIds {
        "freeze",
        "s1_enable",
        "s1_mute",
        "s1_solo",
        "s1_onset_trigger",
        "s2_enable",
        "s2_mute",
        "s2_solo",
        "s2_onset_trigger",
        "s3_enable",
        "s3_mute",
        "s3_solo",
        "s3_onset_trigger",
        "s4_enable",
        "s4_mute",
        "s4_solo",
        "s4_onset_trigger",
        "pitch_follow_s1",
        "pitch_follow_s2",
        "pitch_follow_s3",
        "pitch_follow_s4"
    };

    constexpr int kNumStreams        = ogran::ui::kNumStreams;       // 4
    constexpr int kNumCollections    = ogran::ui::kNumCollections;   // 32: sN_scale choices 0 Off … 31 Custom
    constexpr int kWindowCurvePoints = 64;
    constexpr int kMaxNameChars      = GranulatorProcessor::kMaxCustomNameChars;   // 40

    using ogran::ui::tableToVar;

    juce::WebBrowserComponent::Resource makeBinaryResource (const char* data, int size, const juce::String& mime)
    {
        juce::WebBrowserComponent::Resource r;
        r.data.assign (reinterpret_cast<const std::byte*> (data), reinterpret_cast<const std::byte*> (data) + size);
        r.mimeType = mime;
        return r;
    }

    std::optional<juce::String> firstStringArg (const juce::Array<juce::var>& args)
    {
        if (args.size() < 1 || ! args[0].isString())
            return std::nullopt;
        return args[0].toString();
    }

    juce::var objectArg (const juce::Array<juce::var>& args)
    {
        return args.size() > 0 && args[0].isObject() ? args[0] : juce::var();
    }

    // preset-manager.js checks `result && result.success` — never a bare bool.
    juce::var makePresetDialogResult (bool ok, const juce::String& name)
    {
        auto* obj = new juce::DynamicObject();
        obj->setProperty ("success", ok);
        obj->setProperty ("name", name);
        return juce::var (obj);
    }

    juce::var makeError (const juce::String& code, const juce::String& token = {})
    {
        auto* obj = new juce::DynamicObject();
        obj->setProperty ("error", code);
        obj->setProperty ("token", token);
        return juce::var (obj);
    }

    // uint64 masks cross the bridge as 16-hex-digit strings (maskToHex is in
    // ScaleStateJson.h); a JS number cannot hold 64 bits.
    bool hexToMask (juce::String s, juce::uint64& out)
    {
        s = s.trim();
        if (s.startsWithIgnoreCase ("0x"))
            s = s.substring (2);
        if (s.isEmpty() || s.length() > 16 || ! s.containsOnly ("0123456789abcdefABCDEF"))
            return false;
        juce::uint64 v = 0;
        for (auto c : s)
            v = (v << 4) | (juce::uint64) juce::CharacterFunctions::getHexDigitValue (c);
        out = v;
        return true;
    }
}

//==============================================================================
// The resource map (Stage 1, R114): `url` is a BARE PATH ("/", "/js/i18n.js"),
// never a full URL — compared directly (critical_webview_resource_provider_and_schemes).
std::optional<juce::WebBrowserComponent::Resource> granulatorUiResource (const juce::String& url)
{
    // Explicit map, no generic loop. charset=utf-8 on text: the page uses thin
    // spaces, minus signs and Han text.
    if (url == "/" || url == "/index.html")
        return makeBinaryResource (UIBinaryData::index_html, UIBinaryData::index_htmlSize,
                                   "text/html; charset=utf-8");

    // The hover-help / label table, imported by the inline controller as
    // './js/i18n.js'. This branch, the juce_add_binary_data entry and the import
    // land together (check-i18n [8]).
    if (url == "/js/i18n.js")
        return makeBinaryResource (UIBinaryData::i18n_js, UIBinaryData::i18n_jsSize,
                                   "application/javascript; charset=utf-8");

    if (url == "/js/juce/index.js")
        return makeBinaryResource (UIBinaryData::index_js, UIBinaryData::index_jsSize,
                                   "application/javascript; charset=utf-8");

    if (url == "/js/juce/check_native_interop.js")
        return makeBinaryResource (UIBinaryData::check_native_interop_js,
                                   UIBinaryData::check_native_interop_jsSize,
                                   "application/javascript; charset=utf-8");

    // Shared preset-manager JS, embedded from modules/ and served under /js/.
    // juce_add_binary_data STRIPS hyphens: presetmanager_js, not preset_manager_js.
    if (url == "/js/preset-manager.js")
        return makeBinaryResource (UIBinaryData::presetmanager_js, UIBinaryData::presetmanager_jsSize,
                                   "application/javascript; charset=utf-8");

    // Shared EB Garamond face (modules/ui/eb-garamond): stylesheet under /css/,
    // the three woff2 faces under /fonts/ where its relative font URLs land.
    if (url == "/css/eb-garamond.css")
        return makeBinaryResource (UIBinaryData::ebgaramond_css, UIBinaryData::ebgaramond_cssSize,
                                   "text/css; charset=utf-8");
    if (url == "/fonts/EBGaramond-Regular.woff2")
        return makeBinaryResource (UIBinaryData::EBGaramondRegular_woff2,
                                   UIBinaryData::EBGaramondRegular_woff2Size, "font/woff2");
    if (url == "/fonts/EBGaramond-Italic.woff2")
        return makeBinaryResource (UIBinaryData::EBGaramondItalic_woff2,
                                   UIBinaryData::EBGaramondItalic_woff2Size, "font/woff2");
    if (url == "/fonts/EBGaramond-Bold.woff2")
        return makeBinaryResource (UIBinaryData::EBGaramondBold_woff2,
                                   UIBinaryData::EBGaramondBold_woff2Size, "font/woff2");

    return std::nullopt;   // 404
}

//==============================================================================
juce::var GranulatorEditor::buildScaleState() const
{
    return ogran::ui::buildScaleState (processorRef);
}

juce::var GranulatorEditor::buildTelemetry() const
{
    // Copy-only, always the full var (PLAN-3.2 P118). The engine's triple
    // buffer never tears, so there is no torn branch; readTelemetry() is false
    // only before prepare, and then its has* flags are false and telemetryToVar
    // omits those keys (the page falls back to the base knobs).
    GranulatorProcessor::Telemetry t;
    processorRef.readTelemetry (t);
    return ogran::ui::telemetryToVar (t);
}

//==============================================================================
GranulatorEditor::GranulatorEditor (GranulatorProcessor& p)
    : AudioProcessorEditor (&p),
      processorRef (p)
{
    // 1. RELAYS (must exist before the WebView) -------------------------------
    for (const auto& id : kSliderIds)
        sliderRelays.push_back (std::make_unique<juce::WebSliderRelay> (id));
    for (const auto& id : kComboIds)
        comboRelays.push_back (std::make_unique<juce::WebComboBoxRelay> (id));
    for (const auto& id : kToggleIds)
        toggleRelays.push_back (std::make_unique<juce::WebToggleButtonRelay> (id));

    // 2. WEBVIEW options + relay registration ---------------------------------
    auto options = juce::WebBrowserComponent::Options{}
        .withNativeIntegrationEnabled()
        .withKeepPageLoadedWhenBrowserIsHidden()
        .withResourceProvider ([] (const auto& url) { return granulatorUiResource (url); });

    for (const auto& relay : sliderRelays)
        options = options.withOptionsFrom (*relay);
    for (const auto& relay : comboRelays)
        options = options.withOptionsFrom (*relay);
    // Registers every bool in initialisationData.__juce__toggles — the list
    // Juce.getToggleState() checks before it builds a ToggleState.
    for (const auto& relay : toggleRelays)
        options = options.withOptionsFrom (*relay);

    // ── NATIVE FUNCTIONS — exactly 21 ─────────────────────────────────────

    // dblclick reset: every relay-bound parameter's default in ENGINEERING units
    // (a choice reports its index). The properties payload carries no default,
    // and a JS default table would drift from the NormalisableRange.
    options = options.withNativeFunction ("getParameterDefaults",
        [this] (auto&, auto complete)
        {
            auto* obj = new juce::DynamicObject();
            for (const auto* ids : { &kSliderIds, &kComboIds })
                for (const auto& id : *ids)
                    if (auto* param = processorRef.parameters.getParameter (id))
                        obj->setProperty (id, param->convertFrom0to1 (param->getDefaultValue()));
            complete (juce::var (obj));
        });

    // The hover-help language pair — the canon's initI18n() calls both.
    options = options.withNativeFunction ("getUiLanguage",
        [this] (auto&, auto complete)
        {
            complete (juce::var (GranulatorProcessor::languageCode (
                processorRef.uiLanguage.load (std::memory_order_acquire))));
        });

    options = options.withNativeFunction ("setUiLanguage",
        [this] (auto& args, auto complete)
        {
            // languageIndex() maps anything that is not "fr" or "zh-Hans" to en,
            // so an unexpected argument degrades to English, never stored raw.
            if (args.size() > 0)
                processorRef.uiLanguage.store (
                    GranulatorProcessor::languageIndex (args[0].toString()),
                    std::memory_order_release);
            complete (juce::var());
        });

    // Live data, PULLED by the page at ~30 Hz (buffer, grains, meters, sources,
    // followers, loop gain, modulated destinations, scale-state revision).
    options = options.withNativeFunction ("getTelemetry",
        [this] (auto&, auto complete)
        {
            complete (buildTelemetry());
        });

    // The window figure is the engine's own curve, never a JS reimplementation.
    options = options.withNativeFunction ("getWindowCurve",
        [this] (auto& args, auto complete)
        {
            const auto arg = objectArg (args);
            const int stream = arg.isObject() ? (int) arg.getProperty ("stream", 1) : 1;
            std::array<float, (size_t) kWindowCurvePoints> curve {};
            processorRef.getWindowCurve (juce::jlimit (1, kNumStreams, stream), curve.data(), kWindowCurvePoints);
            juce::Array<juce::var> pts;
            pts.ensureStorageAllocated (kWindowCurvePoints);
            for (auto v : curve)
                pts.add (v);
            complete (juce::var (pts));
        });

    // ── Scale STATE (not parameters): masks, the shared custom table, parsing.
    //    C++ is the only parser and the only copy of the collections; every
    //    setter answers with the model it accepted.
    options = options.withNativeFunction ("getScaleState",
        [this] (auto&, auto complete)
        {
            complete (buildScaleState());
        });

    options = options.withNativeFunction ("setDegreeMask",
        [this] (auto& args, auto complete)
        {
            const auto arg = objectArg (args);
            if (arg.isObject())
            {
                const int stream = (int) arg.getProperty ("stream", 0);
                const int coll   = (int) arg.getProperty ("collection", -1);
                juce::uint64 mask = 0;
                if (stream >= 1 && stream <= kNumStreams && coll >= 1 && coll < kNumCollections
                    && hexToMask (arg.getProperty ("mask", "").toString(), mask))
                    processorRef.setDegreeMask (stream, coll, mask);
            }
            complete (buildScaleState());
        });

    options = options.withNativeFunction ("setCustomScale",
        [this] (auto& args, auto complete)
        {
            const auto arg = objectArg (args);
            if (! arg.isObject()) { complete (makeError ("noPitches")); return; }
            juce::String badToken;
            const auto error = processorRef.setCustomScale (
                arg.getProperty ("name", "").toString().substring (0, kMaxNameChars),
                arg.getProperty ("text", "").toString(), badToken);
            auto result = makeError (error, badToken);
            result.getDynamicObject()->setProperty ("state", buildScaleState());
            complete (result);
        });

    // Live preview while typing: parse only, apply nothing.
    options = options.withNativeFunction ("parseScaleText",
        [] (auto& args, auto complete)
        {
            const auto arg = objectArg (args);
            PitchCollections::Table table;
            juce::String error, badToken;
            const auto text = arg.isObject() ? arg.getProperty ("text", "").toString() : juce::String();
            if (! PitchCollections::tableFromText (text, table, error, badToken))
            {
                complete (makeError (error, badToken));
                return;
            }
            auto out = tableToVar (table);
            out.getDynamicObject()->setProperty ("error", juce::String());
            complete (out);
        });

    // Load .scl… — a DRAFT: answers {name, text} for the editor fields, applies
    // nothing (v2-ui.yaml scale_ui.custom_editor.load_save).
    options = options.withNativeFunction ("loadScala",
        [this] (auto&, auto complete)
        {
            // MSVC: hoist the SafePointer to a local, capture it by copy
            // (critical_msvc_safepointer_init_capture_nested_lambda).
            auto safeThis = juce::Component::SafePointer<GranulatorEditor> (this);
            if (fileDialogOpen) { complete (makeError ("cancelled")); return; }
            fileDialogOpen = true;
            fileChooser = std::make_shared<juce::FileChooser> ("Load Scala scale", juce::File(), "*.scl");
            fileChooser->launchAsync (
                juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                [safeThis, complete] (const juce::FileChooser& fc)
                {
                    // Editor gone: BARE return — `complete` belongs to the dead
                    // WebView (pattern_webview_launchasync_safepointer_no_complete).
                    if (safeThis == nullptr)
                        return;
                    safeThis->fileDialogOpen = false;
                    const auto file = fc.getResult();
                    if (! file.existsAsFile()) { complete (makeError ("cancelled")); return; }
                    PitchCollections::Table table;
                    juce::String asText, description, error;
                    if (! PitchCollections::tableFromScala (file.loadFileAsString(), table, asText, description, error))
                    {
                        complete (makeError (error));
                        return;
                    }
                    auto* obj = new juce::DynamicObject();
                    obj->setProperty ("error", juce::String());
                    obj->setProperty ("name", (description.isNotEmpty() ? description
                                                                         : file.getFileNameWithoutExtension())
                                                  .substring (0, kMaxNameChars));
                    obj->setProperty ("text", asText);
                    complete (juce::var (obj));
                });
        });

    // Save .scl… — Scala text from the editor's tokens (ratios kept verbatim).
    options = options.withNativeFunction ("saveScala",
        [this] (auto& args, auto complete)
        {
            auto safeThis = juce::Component::SafePointer<GranulatorEditor> (this);
            const auto arg = objectArg (args);
            const auto name = arg.isObject() ? arg.getProperty ("name", "").toString() : juce::String();
            const auto text = arg.isObject() ? arg.getProperty ("text", "").toString() : juce::String();
            PitchCollections::Table table;
            juce::String error, badToken;
            if (! PitchCollections::tableFromText (text, table, error, badToken))
            {
                complete (makeError (error, badToken));
                return;
            }
            if (fileDialogOpen) { complete (makeError ("cancelled")); return; }
            fileDialogOpen = true;
            const auto scl = PitchCollections::toScala (name, text);
            fileChooser = std::make_shared<juce::FileChooser> (
                "Save Scala scale",
                juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                    .getChildFile (juce::File::createLegalFileName (name.isNotEmpty() ? name : "custom") + ".scl"),
                "*.scl");
            fileChooser->launchAsync (
                juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                    | juce::FileBrowserComponent::warnAboutOverwriting,
                [safeThis, complete, scl] (const juce::FileChooser& fc)
                {
                    if (safeThis == nullptr)
                        return;   // bare return — see loadScala
                    safeThis->fileDialogOpen = false;
                    const auto file = fc.getResult();
                    if (file == juce::File{}) { complete (makeError ("cancelled")); return; }
                    const bool ok = file.withFileExtension ("scl").replaceWithText (scl);
                    complete (makeError (ok ? juce::String() : juce::String ("io")));
                });
        });

    // ── Preset bridge (OuariconPresetManager contract, preset-manager.js) ──
    options = options
        .withNativeFunction ("savePreset", [this] (const auto& args, auto complete)
        {
            const auto name = firstStringArg (args);
            complete (juce::var (name && processorRef.getPresetManager().savePreset (*name)));
        })
        .withNativeFunction ("loadPreset", [this] (const auto& args, auto complete)
        {
            const auto name = firstStringArg (args);
            complete (juce::var (name && processorRef.getPresetManager().loadPreset (*name)));
        })
        .withNativeFunction ("getPresetList", [this] (const auto&, auto complete)
        {
            juce::Array<juce::var> arr;
            for (const auto& name : processorRef.getPresetManager().getPresetList())
                arr.add (name);
            complete (juce::var (arr));
        })
        .withNativeFunction ("getCurrentPreset", [this] (const auto&, auto complete)
        {
            complete (juce::var (processorRef.getPresetManager().getCurrentPresetName()));
        })
        // Neighbour NAME only — preset-manager.js calls loadPreset() itself.
        .withNativeFunction ("selectNextPreset", [this] (const auto&, auto complete)
        {
            complete (juce::var (processorRef.getPresetManager().getNextPreset()));
        })
        .withNativeFunction ("selectPreviousPreset", [this] (const auto&, auto complete)
        {
            complete (juce::var (processorRef.getPresetManager().getPreviousPreset()));
        })
        .withNativeFunction ("deletePreset", [this] (const auto& args, auto complete)
        {
            const auto name = firstStringArg (args);
            complete (juce::var (name && processorRef.getPresetManager().deletePreset (*name)));
        })
        .withNativeFunction ("isFactoryPreset", [this] (const auto& args, auto complete)
        {
            const auto name = firstStringArg (args);
            complete (juce::var (name && processorRef.getPresetManager().isFactoryPreset (*name)));
        })
        .withNativeFunction ("savePresetWithDialog", [this] (const auto&, auto complete)
        {
            auto safeThis = juce::Component::SafePointer<GranulatorEditor> (this);
            if (fileDialogOpen) { complete (makePresetDialogResult (false, {})); return; }
            fileDialogOpen = true;
            fileChooser = std::make_shared<juce::FileChooser> (
                "Save Preset", processorRef.getPresetManager().getUserPresetsDirectory(), "*.json");
            fileChooser->launchAsync (
                juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                [safeThis, complete] (const juce::FileChooser& fc)
                {
                    if (safeThis == nullptr)
                        return;
                    safeThis->fileDialogOpen = false;
                    auto result = fc.getResult();
                    if (result == juce::File{}) { complete (makePresetDialogResult (false, {})); return; }
                    auto  name = result.getFileNameWithoutExtension();
                    auto& pm   = safeThis->processorRef.getPresetManager();
                    // savePresetToFile has NO factory guard
                    // (pattern_savepresettofile_has_no_factory_guard): only for a
                    // directory OUTSIDE the user-presets dir.
                    const bool ok = result.isAChildOf (pm.getUserPresetsDirectory())
                                      ? pm.savePreset (name)
                                      : pm.savePresetToFile (result.withFileExtension ("json"));
                    complete (makePresetDialogResult (ok, name));
                });
        })
        .withNativeFunction ("loadPresetFromFile", [this] (const auto&, auto complete)
        {
            auto safeThis = juce::Component::SafePointer<GranulatorEditor> (this);
            if (fileDialogOpen) { complete (makePresetDialogResult (false, {})); return; }
            fileDialogOpen = true;
            fileChooser = std::make_shared<juce::FileChooser> (
                "Load Preset", processorRef.getPresetManager().getUserPresetsDirectory(), "*.json");
            fileChooser->launchAsync (
                juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                [safeThis, complete] (const juce::FileChooser& fc)
                {
                    if (safeThis == nullptr)
                        return;
                    safeThis->fileDialogOpen = false;
                    auto file = fc.getResult();
                    if (! file.existsAsFile()) { complete (makePresetDialogResult (false, {})); return; }
                    const bool ok = safeThis->processorRef.getPresetManager().loadPresetFromFile (file);
                    complete (makePresetDialogResult (ok, file.getFileNameWithoutExtension()));
                });
        });

   #if JUCE_WINDOWS
    // REQUIRED on Windows: name the backend. Backend::defaultBackend is the
    // legacy IE control, which ignores withWinWebView2Options and has no
    // resource provider ("page not found") — critical_webview2_runtime_gotchas_windows.
    // The user-data folder lives in the per-user app-data tree (NOT %TEMP%,
    // which cleaners wipe and other processes lock) and is created before
    // WebView2 sees it. DAW hosts deny WebView2's default location.
    // Static linking (JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1 + NEEDS_WEBVIEW2)
    // is set in CMakeLists — without it the WebView silently renders blank.
    const auto webView2DataFolder = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                                        .getChildFile ("Ouaricon")
                                        .getChildFile ("WebView2")
                                        .getChildFile ("O-Granulator");
    webView2DataFolder.createDirectory();

    options = options
        .withBackend (juce::WebBrowserComponent::Options::Backend::webview2)
        .withWinWebView2Options (
            juce::WebBrowserComponent::Options::WinWebView2{}
                .withUserDataFolder (webView2DataFolder)
                .withStatusBarDisabled()
                .withBuiltInErrorPageDisabled());

    // When the WebView2 environment cannot be created (runtime missing, data
    // folder unusable), JUCE silently builds the IE control instead. Probe
    // first; on failure webView stays null and paint() shows a native notice.
    // Audio, parameters and host automation do not depend on the WebView.
    const bool webView2Available = juce::WebBrowserComponent::areOptionsSupported (options);
   #else
    constexpr bool webView2Available = true;
   #endif

    if (webView2Available)
        webView = std::make_unique<juce::WebBrowserComponent> (options);

    // 3. ATTACHMENTS (after the WebView; 3-arg ctor, nullptr undoManager) ------
    for (int i = 0; i < kSliderIds.size(); ++i)
    {
        auto* param = processorRef.parameters.getParameter (kSliderIds[i]);
        jassert (param != nullptr);   // ID drift → silently dead control
        if (param != nullptr)
            sliderAttachments.push_back (std::make_unique<juce::WebSliderParameterAttachment> (
                *param, *sliderRelays[(size_t) i], nullptr));
    }
    for (int i = 0; i < kComboIds.size(); ++i)
    {
        auto* param = processorRef.parameters.getParameter (kComboIds[i]);
        jassert (param != nullptr);
        if (param != nullptr)
            comboAttachments.push_back (std::make_unique<juce::WebComboBoxParameterAttachment> (
                *param, *comboRelays[(size_t) i], nullptr));
    }
    for (int i = 0; i < kToggleIds.size(); ++i)
    {
        auto* param = processorRef.parameters.getParameter (kToggleIds[i]);
        jassert (param != nullptr);
        if (param != nullptr)
            toggleAttachments.push_back (std::make_unique<juce::WebToggleButtonParameterAttachment> (
                *param, *toggleRelays[(size_t) i], nullptr));
    }

    // Attachments stay in place without a WebView: the relays drop their
    // events while no browser is registered.
    if (webView != nullptr)
    {
        addAndMakeVisible (*webView);   // never setVisible(false) on it afterwards
        webView->goToURL (juce::WebBrowserComponent::getResourceProviderRoot());
    }
    else
    {
        // Windows only: WebView2 could not start (see paint()).
        webView2RuntimeLink = std::make_unique<juce::HyperlinkButton> (
            "Download the WebView2 Runtime",
            juce::URL ("https://developer.microsoft.com/microsoft-edge/webview2/"));
        webView2RuntimeLink->setFont (juce::FontOptions (17.0f, juce::Font::underlined), false);
        webView2RuntimeLink->setColour (juce::HyperlinkButton::textColourId,
                                        juce::Colour (0xffE2B064));   // --accent (ochre)
        addAndMakeVisible (*webView2RuntimeLink);
    }

    // EVERY other webView-> use must be guarded with `if (webView != nullptr)`.

    // Fixed 1200 x 800 (v2-ui.yaml window; ui-design-rules Rule 4: fixed, ≤ 800 px
    // tall). The page's body and #plugin are 1200 x 800 px — keep them in sync.
    setResizable (false, false);
    setSize (1200, 800);
}

GranulatorEditor::~GranulatorEditor() = default;   // members die attachments → webView → relays

void GranulatorEditor::paint (juce::Graphics& g)
{
    // WebView fills the editor — nothing to paint, unless WebView2 could not
    // start (Windows only). Colours are the page's own CSS tokens.
    if (webView != nullptr)
        return;

    g.fillAll (juce::Colour (0xff14100C));   // --ground

    auto area = getLocalBounds().withSizeKeepingCentre (juce::jmin (640, getWidth() - 48), 220)
                                .translated (0, -40);

    g.setColour (juce::Colour (0xffECE1CC));   // --ink
    g.setFont (juce::FontOptions (26.0f, juce::Font::bold));
    g.drawFittedText ("O-Granulator needs Microsoft Edge WebView2",
                      area.removeFromTop (40), juce::Justification::centred, 2);

    area.removeFromTop (12);
    g.setFont (juce::FontOptions (17.0f));
    g.drawFittedText ("The interface is drawn with the WebView2 Runtime, which could not start on "
                      "this computer. Install it, then close and reopen the plugin window.\n"
                      "The effect itself keeps working, and host automation still "
                      "reaches every parameter.",
                      area.removeFromTop (110), juce::Justification::centred, 5);
}

void GranulatorEditor::resized()
{
    if (webView != nullptr)
        webView->setBounds (getLocalBounds());

    if (webView2RuntimeLink != nullptr)
        webView2RuntimeLink->setBounds (getLocalBounds().withSizeKeepingCentre (320, 32)
                                                        .translated (0, 40));
}
