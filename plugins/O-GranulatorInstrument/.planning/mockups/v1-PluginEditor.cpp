/*
   This file is part of O-GranulatorInstrument, an Ouaricon Audio plugin.
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

    O-GranulatorInstrument — Plugin Editor implementation (v1 mockup, ui-finalization TEMPLATE)

    Processor surface this file expects (Stage 1–3 implement it; names are
    provisional, the SHAPES are the contract — see v1-integration-checklist.md §2):

      juce::AudioProcessorValueTreeState parameters;
      std::atomic<int> uiLanguage;  static juce::String languageCode (int);
                                    static int languageIndex (const juce::String&);
      OuariconPresetManager& getPresetManager();

      // scale STATE (inherited from O-Granulator, message thread only)
      const PitchCollections::Table* getCollectionTable (int choiceIndex) const;  // nullptr for Off (0)
      CustomScale  getCustomScale() const;                                        // { name, text, period, version }
      juce::uint64 getDegreeMask (int stream /*1..4*/, int choiceIndex) const;    // absent = all on
      void         setDegreeMask (int stream, int choiceIndex, juce::uint64 mask);
      juce::String setCustomScale (const juce::String& name, const juce::String& text,
                                   juce::String& badToken);                       // "" = applied, else error code
      std::uint32_t getScaleStateRevision() const;

      // slot STATE + actions (message thread; parameter-spec.md "Plugin state & native functions")
      SlotInfo     getSlotInfo (int slot /*1..4*/) const;      // scheme, name, path, seconds, sourceSeconds,
                                                               // trimmed, missing, embed, encodedBytes, dirty
      std::uint32_t getSlotStateRevision() const;              // bumped on load / clear / relink / record commit /
                                                               // embed change / setStateInformation / preset apply
      bool  getSlotOverview (int slot, float* peaks, std::uint8_t* dirty, int numBins, std::uint32_t& rev) const;
      bool  loadSlotFromFile (int slot, const juce::File&, juce::String& error);   // enqueue; never decode inline
      bool  relinkSlot (int slot, const juce::File&, juce::String& error);
      void  clearSlot (int slot);
      void  resetSlot (int slot);                              // off-thread rebuild from pristine + 20 ms swap
      void  resetAllSlots();
      juce::String setSlotEmbed (int slot, bool embed);        // "" ok, "tooLarge" (> 16 MB FLAC-24), "factory", "empty"
      bool  saveRecordingTo (int slot, const juce::File&);     // rec: slots only

      // live data (seqlock / try-read; the audio thread publishes once per block)
      bool readTelemetry (Telemetry&);
      void getWindowCurve (int stream, float* out, int numPoints) const;   // WindowMorphBank, base params

  ==============================================================================
*/

#include "PluginEditor.h"
#include "UIBinaryData.h"   // distinct NAMESPACE + HEADER_NAME — see CMakeLists juce_add_binary_data

namespace
{
    // ── Parameter IDs — must match parameter-spec.md / createParameterLayout()
    //    EXACTLY, by ID AND by TYPE. An ID typo or a wrong relay type is a
    //    silently dead control, so every lookup below is jassert'ed. Generated
    //    from v1-ui.yaml in registration order.
    const juce::StringArray kSliderIds {
        "polyphony",
        "glide",
        "pitch_bend_range",
        "output_gain",
        "amp_attack",
        "amp_decay",
        "amp_sustain",
        "amp_release",
        "vel_to_amp",
        "menv1_attack",
        "menv1_decay",
        "menv1_sustain",
        "menv1_release",
        "menv2_attack",
        "menv2_decay",
        "menv2_sustain",
        "menv2_release",
        "s1_level",
        "s1_pan",
        "s1_position",
        "s1_spray",
        "s1_scan_rate",
        "s1_loop_start",
        "s1_loop_end",
        "s1_root_note",
        "s1_root_fine",
        "s1_key_track",
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
        "s2_scan_rate",
        "s2_loop_start",
        "s2_loop_end",
        "s2_root_note",
        "s2_root_fine",
        "s2_key_track",
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
        "s3_scan_rate",
        "s3_loop_start",
        "s3_loop_end",
        "s3_root_note",
        "s3_root_fine",
        "s3_key_track",
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
        "s4_scan_rate",
        "s4_loop_start",
        "s4_loop_end",
        "s4_root_note",
        "s4_root_fine",
        "s4_key_track",
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
        "rec_threshold",
        "rec_length",
        "rec_gain",
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
        "voice_mode",
        "grain_limit",
        "sync_mode",
        "s1_scan_mode",
        "s1_size_sync",
        "s1_density_sync",
        "s1_scale",
        "s1_filter_type",
        "s2_scan_mode",
        "s2_size_sync",
        "s2_density_sync",
        "s2_scale",
        "s2_filter_type",
        "s3_scan_mode",
        "s3_size_sync",
        "s3_density_sync",
        "s3_scale",
        "s3_filter_type",
        "s4_scan_mode",
        "s4_size_sync",
        "s4_density_sync",
        "s4_scale",
        "s4_filter_type",
        "rec_target",
        "rec_trigger",
        "rec_length_sync",
        "lfo1_shape",
        "lfo1_rate_sync",
        "lfo1_mode",
        "lfo2_shape",
        "lfo2_rate_sync",
        "lfo2_mode",
        "lfo3_shape",
        "lfo3_rate_sync",
        "lfo3_mode",
        "lfo4_shape",
        "lfo4_rate_sync",
        "lfo4_mode",
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
        "mpe_enable",
        "freeze",
        "s1_enable",
        "s1_mute",
        "s1_solo",
        "s2_enable",
        "s2_mute",
        "s2_solo",
        "s3_enable",
        "s3_mute",
        "s3_solo",
        "s4_enable",
        "s4_mute",
        "s4_solo",
        "rec_arm"
    };

    constexpr int kNumStreams        = 4;
    constexpr int kNumSlots          = 4;
    constexpr int kNumVoices         = 16;     // counted voices shown in the UI (steal spares are not)
    constexpr int kNumSources        = 17;     // modM_source 1..17 (0 = None)
    constexpr int kNumDestinations   = 126;    // append-only table, parameter-spec.md
    constexpr int kNumCollections    = 32;     // sN_scale choices: 0 Off … 31 Custom
    constexpr int kCustomCollection  = 31;
    constexpr int kWindowCurvePoints = 64;
    constexpr int kOverviewBins      = 656;    // lane canvas width (v1-ui.yaml lanes.canvas)
    constexpr int kMaxNameChars      = 40;

    auto makeBinaryResource (const char* data, int size, const char* mimeType)
        -> std::optional<juce::WebBrowserComponent::Resource>
    {
        auto* bytes = reinterpret_cast<const std::byte*> (data);
        return juce::WebBrowserComponent::Resource {
            std::vector<std::byte> (bytes, bytes + size),
            juce::String (mimeType)
        };
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

    // uint64 masks cross the bridge as 16-hex-digit strings: a JS number cannot
    // hold 64 bits, and getLargeIntValue clamps uint64 > INT64_MAX
    // (critical_juce_getlargeintvalue_clamps_unsigned_64).
    juce::String maskToHex (juce::uint64 m)
    {
        return "0x" + juce::String::toHexString ((juce::int64) m).paddedLeft ('0', 16);
    }

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

    juce::var tableToVar (const PitchCollections::Table& t)
    {
        auto* obj = new juce::DynamicObject();
        juce::Array<juce::var> cents;
        cents.ensureStorageAllocated (t.count);
        for (int i = 0; i < t.count; ++i)
            cents.add (t.cents[(size_t) i]);
        obj->setProperty ("cents", cents);
        obj->setProperty ("period", t.period);
        return juce::var (obj);
    }

    int slotArg (const juce::Array<juce::var>& args)
    {
        const auto arg = args.size() > 0 && args[0].isObject() ? args[0] : juce::var();
        const int slot = arg.isObject() ? (int) arg.getProperty ("slot", 0) : 0;
        return (slot >= 1 && slot <= kNumSlots) ? slot : 0;
    }

    juce::var makeOk (bool ok, const juce::String& error = {})
    {
        auto* obj = new juce::DynamicObject();
        obj->setProperty ("ok", ok);
        if (error.isNotEmpty())
            obj->setProperty ("error", error);
        return juce::var (obj);
    }
}

//==============================================================================
std::optional<juce::WebBrowserComponent::Resource>
GranulatorInstrumentEditor::getResource (const juce::String& url)
{
    // `url` is a BARE PATH ("/", "/js/i18n.js"), never a full URL — compare it
    // directly (critical_webview_resource_provider_and_schemes). Explicit map,
    // no generic loop (juce8-critical-patterns §8). charset=utf-8 on text: the
    // page uses thin spaces, minus signs and Han text.
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

    // Shared module JS, embedded from modules/ and served under /js/.
    // juce_add_binary_data STRIPS hyphens: presetmanager_js, webviewdropstreaming_js.
    if (url == "/js/preset-manager.js")
        return makeBinaryResource (UIBinaryData::presetmanager_js, UIBinaryData::presetmanager_jsSize,
                                   "application/javascript; charset=utf-8");
    if (url == "/js/webview-drop-streaming.js")
        return makeBinaryResource (UIBinaryData::webviewdropstreaming_js,
                                   UIBinaryData::webviewdropstreaming_jsSize,
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
juce::var GranulatorInstrumentEditor::buildScaleState() const
{
    auto* root = new juce::DynamicObject();
    root->setProperty ("revision", (int) (processorRef.getScaleStateRevision() & 0x3fffffffu));

    // tables[0] = null (Off); tables[1..30] = the built-ins; tables[31] = the
    // shared custom table. The page draws chips and the pitch circle from THESE
    // numbers — it carries no copy of the collections.
    juce::Array<juce::var> tables;
    for (int c = 0; c < kNumCollections; ++c)
    {
        if (auto* t = processorRef.getCollectionTable (c))
            tables.add (tableToVar (*t));
        else
            tables.add (juce::var());
    }
    root->setProperty ("tables", tables);

    const auto custom = processorRef.getCustomScale();
    auto* cs = new juce::DynamicObject();
    cs->setProperty ("name", custom.name);
    cs->setProperty ("text", custom.text);
    cs->setProperty ("period", custom.period);
    cs->setProperty ("version", (int) (custom.version & 0x3fffffffu));
    root->setProperty ("custom", juce::var (cs));

    juce::Array<juce::var> masks;
    for (int s = 1; s <= kNumStreams; ++s)
    {
        juce::Array<juce::var> row;
        for (int c = 0; c < kNumCollections; ++c)
            row.add (maskToHex (processorRef.getDegreeMask (s, c)));
        masks.add (row);
    }
    root->setProperty ("masks", masks);
    return juce::var (root);
}

juce::var GranulatorInstrumentEditor::buildSlotState() const
{
    auto* root = new juce::DynamicObject();
    root->setProperty ("revision", (int) (processorRef.getSlotStateRevision() & 0x3fffffffu));

    // One entry per slot. `name` and `path` are user text (never localized).
    // scheme: "" (empty) | "factory" | "file" | "rec"   (parameter-spec.md <Slots>, S9 / S10)
    juce::Array<juce::var> slots;
    for (int s = 1; s <= kNumSlots; ++s)
    {
        const auto info = processorRef.getSlotInfo (s);
        auto* o = new juce::DynamicObject();
        o->setProperty ("scheme", info.scheme);
        o->setProperty ("id", info.factoryId);              // factory:<id>, else ""
        o->setProperty ("name", info.name);
        o->setProperty ("path", info.path);                 // absolute path (file:/rec:), else ""
        o->setProperty ("seconds", info.seconds);           // loaded length (≤ slot max)
        o->setProperty ("sourceSeconds", info.sourceSeconds);   // file length before trimming
        o->setProperty ("trimmed", info.trimmed);
        o->setProperty ("missing", info.missing);
        o->setProperty ("embed", info.embed);
        o->setProperty ("encodedBytes", (double) info.encodedBytes);   // FLAC-24 estimate
        o->setProperty ("embeddable", info.encodedBytes <= 16 * 1024 * 1024 && info.scheme != "factory");
        o->setProperty ("dirty", info.dirty);               // working copy differs from pristine
        slots.add (juce::var (o));
    }
    root->setProperty ("slots", slots);
    return juce::var (root);
}

juce::var GranulatorInstrumentEditor::buildSlotOverview (int slot) const
{
    std::array<float, (size_t) kOverviewBins> peaks {};
    std::array<std::uint8_t, (size_t) kOverviewBins> dirty {};
    std::uint32_t rev = 0;
    auto* obj = new juce::DynamicObject();
    obj->setProperty ("slot", slot);
    if (! processorRef.getSlotOverview (slot, peaks.data(), dirty.data(), kOverviewBins, rev))
        return juce::var (obj);   // empty slot: no peaks key → the lane draws its empty state

    juce::Array<juce::var> p, d;
    p.ensureStorageAllocated (kOverviewBins);
    d.ensureStorageAllocated (kOverviewBins);
    for (int i = 0; i < kOverviewBins; ++i)
    {
        p.add (std::round (peaks[(size_t) i] * 1000.0f) / 1000.0f);   // 3 decimals keep the JSON small
        d.add ((int) dirty[(size_t) i]);                              // 0..255 = how far the copy moved
    }
    obj->setProperty ("rev", (int) (rev & 0x3fffffffu));
    obj->setProperty ("peaks", p);
    obj->setProperty ("dirty", d);
    return juce::var (obj);
}

juce::var GranulatorInstrumentEditor::buildTelemetry()
{
    GranulatorInstrumentProcessor::Telemetry t;
    auto* obj = new juce::DynamicObject();
    obj->setProperty ("scaleRev", (int) (processorRef.getScaleStateRevision() & 0x3fffffffu));
    obj->setProperty ("slotRev",  (int) (processorRef.getSlotStateRevision()  & 0x3fffffffu));

    // A torn read (a block landed mid-copy every retry) omits the live keys;
    // the page keeps drawing its previous frame.
    if (! processorRef.readTelemetry (t))
        return juce::var (obj);

    obj->setProperty ("frozen", t.frozen);

    // Per lane: overview revision (page pulls getSlotOverview when it moves),
    // feedback record head 0..1 of the FILE (null when Amount = 0 or empty),
    // Record-in head 0..1 (null unless recording into this slot), and the
    // modulated loop region [lo, hi] of the file (the region the scan uses now).
    juce::Array<juce::var> lanes;
    for (int s = 0; s < kNumSlots; ++s)
    {
        const auto& l = t.lanes[(size_t) s];
        auto* o = new juce::DynamicObject();
        o->setProperty ("ovRev", (int) (l.overviewRev & 0x3fffffffu));
        o->setProperty ("fb", l.fbHead >= 0.0f ? juce::var (l.fbHead) : juce::var());
        o->setProperty ("rec", l.recHead >= 0.0f ? juce::var (l.recHead) : juce::var());
        o->setProperty ("lo", l.regionLo);
        o->setProperty ("hi", l.regionHi);
        lanes.add (juce::var (o));
    }
    obj->setProperty ("lanes", lanes);

    // Voices (16 counted slots): [state, note, amp, scan1..scan4, clampSt]
    //   state 0 off · 1 held · 2 releasing · 3 stolen (5 ms forced release)
    //   amp = amp-env level 0..1 · scanN = the voice's scan centre in stream N, 0..1 of the file (-1 = none)
    //   clampSt = the semitones the first enabled stream WANTS before the ±48 clamp (S6)
    juce::Array<juce::var> voices;
    for (int v = 0; v < kNumVoices; ++v)
    {
        const auto& vt = t.voices[(size_t) v];
        voices.add (juce::Array<juce::var> { (int) vt.state, (int) vt.note, vt.amp,
                                             vt.scan[0], vt.scan[1], vt.scan[2], vt.scan[3], vt.wantSt });
    }
    obj->setProperty ("voices", voices);
    obj->setProperty ("newest", t.newestVoice);     // index 0..15, -1 = none (drives mono routes, D4)

    // Mod envelopes per voice: [level 0..1, stage 0 off · 1 A · 2 D · 3 S · 4 R] × 2 envelopes
    juce::Array<juce::var> menv;
    for (int v = 0; v < kNumVoices; ++v)
    {
        const auto& vt = t.voices[(size_t) v];
        menv.add (juce::Array<juce::var> { vt.menv[0], (int) vt.menvStage[0], vt.menv[1], (int) vt.menvStage[1],
                                           (int) vt.ampStage });
    }
    obj->setProperty ("menv", menv);

    // Per-voice LFO phases (0..1, -1 = voice off) for LFOs in Per-voice mode;
    // the global phase for LFOs in Global mode.
    juce::Array<juce::var> lfoPh;
    for (int n = 0; n < 4; ++n)
    {
        juce::Array<juce::var> row;
        row.add (t.lfoGlobalPhase[(size_t) n]);
        for (int v = 0; v < kNumVoices; ++v)
            row.add (t.lfoVoicePhase[(size_t) n][(size_t) v]);
        lfoPh.add (row);
    }
    obj->setProperty ("lfoPh", lfoPh);

    // Sounding grains: [stream 1-4, normPos 0..1 of the file, pan -1..1, env 0..1,
    //                   gain, lengthSec, voice 0-15, flags (1 reversed | 2 pitch clamped)]
    juce::Array<juce::var> grains;
    grains.ensureStorageAllocated (t.grainCount);
    for (int i = 0; i < t.grainCount; ++i)
    {
        const auto& g = t.grains[(size_t) i];
        grains.add (juce::Array<juce::var> { (int) g.stream, g.normPos, g.pan, g.env, g.gain, g.lengthSec,
                                             (int) g.voice, (int) g.flags });
    }
    obj->setProperty ("grains", grains);
    obj->setProperty ("counts", juce::Array<juce::var> { t.counts[0], t.counts[1], t.counts[2], t.counts[3] });
    obj->setProperty ("limit", t.limit);
    obj->setProperty ("out", juce::Array<juce::var> { t.outPeakL, t.outPeakR });   // consuming peaks

    // Record-in (ARCHITECTURE §22, D6). The sidechain may not exist at all:
    //   bus    0 = no input bus / disabled / 0 channels → "No sidechain", cluster inert
    //          1 = bus present
    //   signal = the sidechain peak rose above −90 dBFS within the last 2 s
    //   run    0 idle · 1 armed · 2 recording · 3 committing · 4 done · 5 discarded (silent capture)
    auto* rec = new juce::DynamicObject();
    rec->setProperty ("bus", t.rec.busPresent ? 1 : 0);
    rec->setProperty ("signal", t.rec.signal);
    rec->setProperty ("run", (int) t.rec.run);
    rec->setProperty ("slot", t.rec.slot);                 // 1..4, the slot being / last captured into
    rec->setProperty ("pos", t.rec.pos);                   // 0..1 of the capture length
    rec->setProperty ("len", t.rec.lengthSec);             // capture length (after the 30 s cap)
    rec->setProperty ("capped", t.rec.capped);             // Tempo bars exceeded the slot maximum (S8)
    rec->setProperty ("peak", juce::Array<juce::var> { t.rec.scPeakL, t.rec.scPeakR });   // consuming, post rec_gain
    obj->setProperty ("rec", juce::var (rec));

    // The 17 source values in modM_source order (index = source − 1): LFO 1-4,
    // Mod Env 1-2, Velocity, Key, Note Random, Mod Wheel, Pressure, Slide, Bend,
    // Macro 1-4. Per-voice sources read from the NEWEST voice (D4).
    juce::Array<juce::var> sources;
    for (int i = 0; i < kNumSources; ++i)
        sources.add (t.sources[(size_t) i]);
    obj->setProperty ("sources", sources);

    // Modulated NORMALISED value for every destination that has a route, keyed
    // by destination index (the mod-ring live dot; per-voice destinations from
    // the newest voice). Never written back to the APVTS.
    auto* mod = new juce::DynamicObject();
    for (int d = 1; d < kNumDestinations; ++d)
        if (t.modRouted[(size_t) d])
            mod->setProperty (juce::Identifier (juce::String (d)), t.modNorm[(size_t) d]);
    obj->setProperty ("modNorm", juce::var (mod));

    return juce::var (obj);
}

void GranulatorInstrumentEditor::launchSlotChooser (int slot, bool relink, std::function<void (juce::var)> complete)
{
    // MSVC: hoist the SafePointer to a local, capture it by copy
    // (critical_msvc_safepointer_init_capture_nested_lambda).
    auto safeThis = juce::Component::SafePointer<GranulatorInstrumentEditor> (this);
    if (fileDialogOpen) { complete (makeOk (false, "busy")); return; }
    fileDialogOpen = true;
    const auto info = processorRef.getSlotInfo (slot);
    const auto start = info.path.isNotEmpty() ? juce::File (info.path).getParentDirectory()
                                              : juce::File::getSpecialLocation (juce::File::userMusicDirectory);
    fileChooser = std::make_shared<juce::FileChooser> (
        relink ? "Locate " + info.name : "Load a file into slot " + juce::String (slot),
        start, "*.wav;*.aif;*.aiff;*.flac;*.ogg;*.mp3");
    fileChooser->launchAsync (
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [safeThis, complete, slot, relink] (const juce::FileChooser& fc)
        {
            // Editor gone: BARE return — `complete` belongs to the dead WebView
            // (pattern_webview_launchasync_safepointer_no_complete).
            if (safeThis == nullptr)
                return;
            safeThis->fileDialogOpen = false;
            const auto file = fc.getResult();
            if (! file.existsAsFile()) { complete (makeOk (false, "cancelled")); return; }
            juce::String error;
            const bool ok = relink ? safeThis->processorRef.relinkSlot (slot, file, error)
                                   : safeThis->processorRef.loadSlotFromFile (slot, file, error);
            complete (makeOk (ok, error));
        });
}

//==============================================================================
GranulatorInstrumentEditor::GranulatorInstrumentEditor (GranulatorInstrumentProcessor& p)
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

    // Drag-drop: a file dropped on a lane is streamed into a session temp dir
    // (WKWebView strips paths — critical_webview_drag_drop_macos) and committed
    // with midi = slot (1..4). Folders are refused: a slot holds one file.
    {
        Ouaricon::WebViewDropStreaming::SessionManager::Config cfg;
        cfg.tempDirPrefix = "ogranulatorinstrument-drop-";   // per-plugin: the reaper matches the prefix
        cfg.onCommitFolder = [] (const juce::File&, const juce::String&, int, const juce::String&, bool, bool) {};
        cfg.onCommitFile = [this] (const juce::File& file, int slot, int)
        {
            juce::String error;
            if (slot >= 1 && slot <= kNumSlots)
                processorRef.loadSlotFromFile (slot, file, error);   // enqueues the off-thread loader
        };
        dropSessions = std::make_unique<Ouaricon::WebViewDropStreaming::SessionManager> (std::move (cfg));
    }

    // 2. WEBVIEW options + relay registration ---------------------------------
    auto options = juce::WebBrowserComponent::Options{}
        .withNativeIntegrationEnabled()
        .withKeepPageLoadedWhenBrowserIsHidden()
        .withResourceProvider ([this] (const auto& url) { return getResource (url); });

    for (const auto& relay : sliderRelays)
        options = options.withOptionsFrom (*relay);
    for (const auto& relay : comboRelays)
        options = options.withOptionsFrom (*relay);
    // Registers every bool in initialisationData.__juce__toggles — the list
    // Juce.getToggleState() checks before it builds a ToggleState.
    for (const auto& relay : toggleRelays)
        options = options.withOptionsFrom (*relay);

    // ── NATIVE FUNCTIONS — exactly 34 ────────────────────────────────

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
            complete (juce::var (GranulatorInstrumentProcessor::languageCode (
                processorRef.uiLanguage.load (std::memory_order_acquire))));
        });

    options = options.withNativeFunction ("setUiLanguage",
        [this] (auto& args, auto complete)
        {
            // languageIndex() maps anything that is not "fr" or "zh-Hans" to en,
            // so an unexpected argument degrades to English, never stored raw.
            if (args.size() > 0)
                processorRef.uiLanguage.store (
                    GranulatorInstrumentProcessor::languageIndex (args[0].toString()),
                    std::memory_order_release);
            complete (juce::var());
        });

    // Live data, PULLED by the page at ~30 Hz: lanes (heads, regions, revisions),
    // voices, grains, record-in state, meters, sources, modulated destinations.
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
    // nothing (v1-ui.yaml scale_ui, inherited).
    options = options.withNativeFunction ("loadScala",
        [this] (auto&, auto complete)
        {
            // MSVC: hoist the SafePointer to a local, capture it by copy
            // (critical_msvc_safepointer_init_capture_nested_lambda).
            auto safeThis = juce::Component::SafePointer<GranulatorInstrumentEditor> (this);
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
            auto safeThis = juce::Component::SafePointer<GranulatorInstrumentEditor> (this);
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

    // ── Slots (STATE + actions; parameter-spec.md "Plugin state & native
    //    functions"). Every action answers { ok, error? }; the page re-pulls
    //    getSlotState when getTelemetry.slotRev moves, so the answer only drives
    //    a toast.
    options = options.withNativeFunction ("getSlotState",
        [this] (auto&, auto complete)
        {
            complete (buildSlotState());
        });

    options = options.withNativeFunction ("getSlotOverview",
        [this] (auto& args, auto complete)
        {
            const int slot = slotArg (args);
            complete (slot ? buildSlotOverview (slot) : juce::var());
        });

    options = options.withNativeFunction ("loadSlot",
        [this] (auto& args, auto complete)
        {
            const int slot = slotArg (args);
            if (slot == 0) { complete (makeOk (false, "badSlot")); return; }
            launchSlotChooser (slot, false, complete);
        });

    options = options.withNativeFunction ("relinkSlot",
        [this] (auto& args, auto complete)
        {
            const int slot = slotArg (args);
            if (slot == 0) { complete (makeOk (false, "badSlot")); return; }
            launchSlotChooser (slot, true, complete);
        });

    options = options.withNativeFunction ("clearSlot",
        [this] (auto& args, auto complete)
        {
            const int slot = slotArg (args);
            if (slot != 0)
                processorRef.clearSlot (slot);
            complete (makeOk (slot != 0));
        });

    options = options.withNativeFunction ("resetSlot",
        [this] (auto& args, auto complete)
        {
            const int slot = slotArg (args);
            if (slot != 0)
                processorRef.resetSlot (slot);
            complete (makeOk (slot != 0));
        });

    options = options.withNativeFunction ("resetAll",
        [this] (auto&, auto complete)
        {
            processorRef.resetAllSlots();
            complete (makeOk (true));
        });

    // S11: the per-slot Embed toggle is STATE, not a parameter.
    options = options.withNativeFunction ("setSlotEmbed",
        [this] (auto& args, auto complete)
        {
            const int slot = slotArg (args);
            const bool embed = args.size() > 0 && args[0].isObject() && (bool) args[0].getProperty ("embed", false);
            if (slot == 0) { complete (makeOk (false, "badSlot")); return; }
            const auto error = processorRef.setSlotEmbed (slot, embed);
            auto r = makeOk (error.isEmpty(), error);
            r.getDynamicObject()->setProperty ("embed", processorRef.getSlotInfo (slot).embed);
            complete (r);
        });

    options = options.withNativeFunction ("saveRecording",
        [this] (auto& args, auto complete)
        {
            const int slot = slotArg (args);
            if (slot == 0 || processorRef.getSlotInfo (slot).scheme != "rec") { complete (makeOk (false, "notRecording")); return; }
            auto safeThis = juce::Component::SafePointer<GranulatorInstrumentEditor> (this);
            if (fileDialogOpen) { complete (makeOk (false, "busy")); return; }
            fileDialogOpen = true;
            const auto info = processorRef.getSlotInfo (slot);
            fileChooser = std::make_shared<juce::FileChooser> (
                "Save recording",
                juce::File::getSpecialLocation (juce::File::userMusicDirectory)
                    .getChildFile (juce::File::createLegalFileName (info.name) + ".wav"),
                "*.wav");
            fileChooser->launchAsync (
                juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                    | juce::FileBrowserComponent::warnAboutOverwriting,
                [safeThis, complete, slot] (const juce::FileChooser& fc)
                {
                    if (safeThis == nullptr)
                        return;   // bare return — see launchSlotChooser
                    safeThis->fileDialogOpen = false;
                    const auto file = fc.getResult();
                    if (file == juce::File{}) { complete (makeOk (false, "cancelled")); return; }
                    complete (makeOk (safeThis->processorRef.saveRecordingTo (slot, file.withFileExtension ("wav"))));
                });
        });

    // webview-drop-streaming 1.0.0: dropSessionStart / AddFile / CommitFolder / CommitFile.
    for (auto& entry : dropSessions->getNativeFunctions())
        options = options.withNativeFunction (entry.first, std::move (entry.second));

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
            auto safeThis = juce::Component::SafePointer<GranulatorInstrumentEditor> (this);
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
            auto safeThis = juce::Component::SafePointer<GranulatorInstrumentEditor> (this);
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
                                        .getChildFile ("O-GranulatorInstrument");
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

    // Fixed 1200 x 800 (v1-ui.yaml window; ui-design-rules Rule 4: fixed, ≤ 800 px
    // tall). The page's body and #plugin are 1200 x 800 px — keep them in sync.
    setResizable (false, false);
    setSize (1200, 800);
}

GranulatorInstrumentEditor::~GranulatorInstrumentEditor() = default;   // attachments → webView → dropSessions → relays

void GranulatorInstrumentEditor::paint (juce::Graphics& g)
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
    g.drawFittedText ("O-GranulatorInstrument needs Microsoft Edge WebView2",
                      area.removeFromTop (40), juce::Justification::centred, 2);

    area.removeFromTop (12);
    g.setFont (juce::FontOptions (17.0f));
    g.drawFittedText ("The interface is drawn with the WebView2 Runtime, which could not start on "
                      "this computer. Install it, then close and reopen the plugin window.\n"
                      "The instrument itself keeps playing, and host automation still "
                      "reaches every parameter.",
                      area.removeFromTop (110), juce::Justification::centred, 5);
}

void GranulatorInstrumentEditor::resized()
{
    if (webView != nullptr)
        webView->setBounds (getLocalBounds());

    if (webView2RuntimeLink != nullptr)
        webView2RuntimeLink->setBounds (getLocalBounds().withSizeKeepingCentre (320, 32)
                                                        .translated (0, 40));
}
