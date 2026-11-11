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

    O-GranulatorInstrument - Plugin Editor implementation (Stage 3 / Phases
    3.1 and 3.2)

    Adapted from .planning/mockups/v1-PluginEditor.cpp. Deltas against that
    template (PLAN-3.1 PD13, RESEARCH-3.1 section 2; PLAN-3.2 PD12, PD14, PD16,
    PD19 for notes 2, 6 and 7):

      1. SLOTS. The page speaks 1...4, the processor 0...3. The conversion
         happens once, in slotArg() (and for a dropped file in onCommitFile,
         whose `midi` argument carries the page's slot number). getWindowCurve
         converts its stream the same way; the degree masks keep the
         processor's own 1-based stream.
      2. JSON. getSlotState, getScaleState, getTelemetry, getParameterDefaults,
         getSlotOverview and getWindowCurve answer through GiUiJson.h, the
         encoders the render harness dumps for the UI stub. getTelemetry is
         ALWAYS the full 3.3 object (the page replaces its copy on each answer),
         encoded from the processor's buildUiTelemetry (now): that builder is
         the one reader of every consuming feed (the grain / voice frame, the
         output peaks L / R, the sidechain peaks L / R and the steal ring
         cursor), so the editor reads none of them. getSlotState carries the processor's per-slot
         `dirty` flags.
      3. loadSlot (chooser -> requestSlotLoad) and relinkSlot (chooser ->
         requestRelink) answer { ok: true }: the load runs on the loader's
         worker, and a failure reaches the page through the slot's `error`
         field on the next state pull. A cancelled chooser answers
         { ok: false, error: "cancelled" }; a second dialog answers "busy".
      4. resetSlot -> requestReset (slot 0...3); resetAll -> requestReset (-1).
      5. setSlotEmbed: the processor call is void, so the "empty" and "factory"
         refusals are derived from SlotInfo::source BEFORE the call, and
         "tooLarge" from SlotInfo::tooLarge when a previous encode already
         refused. The answer carries the resulting `embed`.
      6. clearSlot -> processor clearSlot (slot 0...3): { ok: true }, or
         { ok: false, error: "recording" | "badSlot" } ("recording" = a take is
         being captured or committed into that slot).
         saveRecording (D46) writes the slot's working copy as a WAV:
           a. refusals answered BEFORE a dialog opens: "notRecording" (bad slot
              or no rec: source), "empty" (nothing loaded), "busy" (a chooser
              is open, or a save started from this editor has not answered);
           b. chooser FIRST: save mode, warnAboutOverwriting, opened beside
              the take (a rec: source is the take's absolute path, written
              into the Recordings folder) on "<take stem>-edit.wav"; no file
              chosen -> "cancelled";
           c. then the processor's saveRecordingTo, whose answer arrives on the
              message thread on every path: { ok: true, path }, or { ok: false,
              error } with busy | notRecording | empty | inUse | changed |
              timeout | writeFailed.
         recordingSaveOpen spans b and c. The native function completes exactly
         once on every path; the one exception is an editor destroyed during b
         or c (bare return: `complete` belongs to the dead WebView).
      7. Drop sessions: onCommitFolder has the module's 8-parameter signature
         (a slot holds one file, so it does nothing); onCommitFile ->
         importDroppedFile (slot, temp file, the dropped file's name): the
         loader's worker hashes the file, moves it into the Imports folder and
         loads it from there (D44), so a slot never references the session's
         temp path. A drop that is not queued (bad slot, import refused) leaves
         the temp file to the session's reaper; a later failure reaches the
         page through the slot's `error`. tempDirPrefix
         "ogranulatorinstrument-drop-". The four drop functions are registered
         by their literal names (the module's handlers, looked up by name), so
         the 34 registrations are all visible in this file.
      8. The template's Telemetry aggregate, its buildX members and its
         unused table constants are gone; the collection and slot counts come
         from GiParamIds.h / GiConstants.h.

    Kept from the template: the three ID tables, the explicit resource ladder
    (bare paths), the Windows WebView2 block, the member / constructor order
    (relays -> drop sessions -> WebView -> attachments -> runtime link), the
    fixed 1200 x 800 frame, a hoisted SafePointer in every async dialog with a
    bare return when the editor is gone, and `complete` on every other path.

    Language: the processor already persists uiLanguage; the two language
    functions only read and store that atomic.

  ==============================================================================
*/

#include "PluginEditor.h"
#include "GiUiJson.h"
#include "UIBinaryData.h"   // distinct NAMESPACE + HEADER_NAME (CMakeLists, the UI resources target)

namespace
{
    // -- Parameter IDs: must match GiParamIds.h / createParameterLayout()
    //    EXACTLY, by ID AND by TYPE. An ID typo or a wrong relay type is a
    //    silently dead control, so every lookup below is jassert'ed. In
    //    registration order (== .planning/params.tsv).
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

    using Completion = juce::WebBrowserComponent::NativeFunctionCompletion;

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

    // preset-manager.js checks `result && result.success`, never a bare bool.
    juce::var makePresetDialogResult (bool ok, const juce::String& name)
    {
        auto* obj = new juce::DynamicObject();
        obj->setProperty ("success", ok);
        obj->setProperty ("name", name);
        return juce::var (obj);
    }

    // Scale functions: { error, token }. An empty `error` means success.
    juce::var makeError (const juce::String& code, const juce::String& token = {})
    {
        auto* obj = new juce::DynamicObject();
        obj->setProperty ("error", code);
        obj->setProperty ("token", token);
        return juce::var (obj);
    }

    // Slot actions: { ok, error? }.
    juce::var makeOk (bool ok, const juce::String& error = {})
    {
        auto* obj = new juce::DynamicObject();
        obj->setProperty ("ok", ok);
        if (error.isNotEmpty())
            obj->setProperty ("error", error);
        return juce::var (obj);
    }

    // uint64 masks cross the bridge as 16-hex-digit strings: a JS number cannot
    // hold 64 bits, and getLargeIntValue clamps uint64 > INT64_MAX.
    bool hexToMask (juce::String s, std::uint64_t& out)
    {
        s = s.trim();
        if (s.startsWithIgnoreCase ("0x"))
            s = s.substring (2);
        if (s.isEmpty() || s.length() > 16 || ! s.containsOnly ("0123456789abcdefABCDEF"))
            return false;
        std::uint64_t v = 0;
        for (auto c : s)
            v = (v << 4) | static_cast<std::uint64_t> (juce::CharacterFunctions::getHexDigitValue (c));
        out = v;
        return true;
    }

    /** THE slot conversion: the page's { slot: 1...4 } -> the processor's
        0...3. Returns -1 for anything else. */
    int slotArg (const juce::Array<juce::var>& args)
    {
        const auto arg = objectArg (args);
        const int slot = arg.isObject() ? static_cast<int> (arg.getProperty ("slot", 0)) : 0;
        return (slot >= 1 && slot <= ogi::kTapeSlots) ? slot - 1 : -1;
    }

    /** Where a slot's chooser opens: beside its file: / rec: source, else the
        user's music folder. */
    juce::File slotStartDirectory (const ogi::SlotInfo& info)
    {
        const auto scheme = GiUiJson::schemeOf (info.source);
        if (scheme == "file" || scheme == "rec")
        {
            const auto path = info.source.substring (scheme.length() + 1);
            if (juce::File::isAbsolutePath (path))
                return juce::File (path).getParentDirectory();
        }
        return juce::File::getSpecialLocation (juce::File::userMusicDirectory);
    }

    /** saveRecording's default file name: "<take stem>-edit.wav", the stem
        taken from the slot's rec:<path> source (else from its display name). */
    juce::String editedTakeFileName (const ogi::SlotInfo& info)
    {
        juce::String stem;
        if (GiUiJson::schemeOf (info.source) == "rec")
        {
            const auto path = info.source.substring (4);   // after "rec:"
            if (juce::File::isAbsolutePath (path))
                stem = juce::File (path).getFileNameWithoutExtension();
        }
        if (stem.isEmpty())
            stem = juce::File::createLegalFileName (info.name.upToLastOccurrenceOf (".", false, false));
        if (stem.isEmpty())
            stem = juce::File::createLegalFileName (info.name);
        if (stem.isEmpty())
            stem = "take";
        return stem + "-edit.wav";
    }

    // saveRecording: { ok: true, path } | { ok: false, error }.
    juce::var makeSaveAnswer (const GranulatorInstrumentProcessor::SaveResult& result)
    {
        auto answer = makeOk (result.ok, result.ok ? juce::String()
                                                    : (result.error.isNotEmpty() ? result.error
                                                                                 : juce::String ("writeFailed")));
        if (result.ok)
            answer.getDynamicObject()->setProperty ("path", result.path);
        return answer;
    }
}

//==============================================================================
std::optional<juce::WebBrowserComponent::Resource>
GranulatorInstrumentEditor::getResource (const juce::String& url)
{
    // `url` is a BARE PATH, never a full URL: compare it directly. Explicit
    // ladder, no table and no loop (the repo's UI scripts pair each path with
    // the symbol that follows it). charset=utf-8 on text: the page uses thin
    // spaces, minus signs and Han text.
    if (url == "/" || url == "/index.html")
        return makeBinaryResource (UIBinaryData::index_html, UIBinaryData::index_htmlSize,
                                   "text/html; charset=utf-8");

    // The hover-help / label table, imported by the inline controller.
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

    // Shared module JS, embedded from modules/ and served under /js/
    // (the embedded symbol names drop the hyphens).
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
void GranulatorInstrumentEditor::launchSlotChooser (int slot0, bool relink, Completion complete)
{
    // MSVC: hoist the SafePointer to a local, capture it by copy.
    auto safeThis = juce::Component::SafePointer<GranulatorInstrumentEditor> (this);
    if (fileDialogOpen) { complete (makeOk (false, "busy")); return; }
    fileDialogOpen = true;

    const auto info = processorRef.getSlotInfo (slot0);
    const auto title = relink ? juce::String ("Locate ") + (info.name.isNotEmpty() ? info.name : juce::String ("the file"))
                              : juce::String ("Load a file into slot ") + juce::String (slot0 + 1);

    fileChooser = std::make_shared<juce::FileChooser> (
        title, slotStartDirectory (info), "*.wav;*.aif;*.aiff;*.flac;*.ogg;*.mp3");
    fileChooser->launchAsync (
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [safeThis, complete, slot0, relink] (const juce::FileChooser& fc)
        {
            // Editor gone: BARE return. `complete` belongs to the dead WebView.
            if (safeThis == nullptr)
                return;
            safeThis->fileDialogOpen = false;
            const auto file = fc.getResult();
            if (! file.existsAsFile()) { complete (makeOk (false, "cancelled")); return; }

            // Both calls only enqueue (the loader's worker decodes). A failure
            // shows up as the slot's `error` on the next state pull.
            if (relink)
                safeThis->processorRef.requestRelink (slot0, file);
            else
                safeThis->processorRef.requestSlotLoad (slot0, file);
            complete (makeOk (true));
        });
}

//==============================================================================
void GranulatorInstrumentEditor::launchSaveRecording (int slot0, Completion complete)
{
    // Answered before a dialog opens, so nobody picks a file for nothing. The
    // processor repeats the first two when the save starts.
    const auto info = slot0 >= 0 ? processorRef.getSlotInfo (slot0) : ogi::SlotInfo();
    if (slot0 < 0 || GiUiJson::schemeOf (info.source) != "rec")
    {
        complete (makeOk (false, "notRecording"));
        return;
    }
    if (! info.loaded) { complete (makeOk (false, "empty")); return; }
    if (fileDialogOpen || recordingSaveOpen) { complete (makeOk (false, "busy")); return; }

    // MSVC: hoist the SafePointer to a local, capture it by copy.
    auto safeThis = juce::Component::SafePointer<GranulatorInstrumentEditor> (this);
    fileDialogOpen = true;
    recordingSaveOpen = true;   // until the save has answered, not just the chooser

    fileChooser = std::make_shared<juce::FileChooser> (
        juce::String ("Save the recording in slot ") + juce::String (slot0 + 1),
        slotStartDirectory (info).getChildFile (editedTakeFileName (info)),   // beside the take = the Recordings folder
        "*.wav");
    fileChooser->launchAsync (
        juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
            | juce::FileBrowserComponent::warnAboutOverwriting,
        [safeThis, complete, slot0] (const juce::FileChooser& fc)
        {
            // Editor gone: BARE return. `complete` belongs to the dead WebView.
            if (safeThis == nullptr)
                return;
            safeThis->fileDialogOpen = false;
            const auto file = fc.getResult();
            if (file == juce::File{})
            {
                safeThis->recordingSaveOpen = false;
                complete (makeOk (false, "cancelled"));
                return;
            }

            // The processor answers exactly once, on the message thread: inline
            // for a refusal, later for a save that started (the audio thread
            // copies the tape, a worker writes the file).
            safeThis->processorRef.saveRecordingTo (slot0, file,
                [safeThis, complete] (const GranulatorInstrumentProcessor::SaveResult& result)
                {
                    // Second hop, same rule: nothing at all for a dead editor.
                    if (auto* editor = safeThis.getComponent())
                    {
                        editor->recordingSaveOpen = false;
                        complete (makeSaveAnswer (result));
                    }
                });
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
    // (the WebView strips paths) and committed with midi = the page's slot
    // number (1...4). The loader then moves it into the Imports folder and
    // loads it from there under the dropped file's own name (D44); the temp
    // path is never a slot source. Folders do nothing: a slot holds one file.
    {
        Ouaricon::WebViewDropStreaming::SessionManager::Config cfg;
        cfg.tempDirPrefix = "ogranulatorinstrument-drop-";   // per-plugin: the reaper matches the prefix
        cfg.onCommitFolder = [] (const juce::File&, const juce::String&, int, const juce::String&,
                                 bool, bool, int, bool) {};
        cfg.onCommitFile = [this] (const juce::File& file, int midi, int)
        {
            const int slot0 = midi - 1;
            // The loader owns the temp file once the import is queued. Not
            // queued (false): it stays in the session's temp dir for the
            // module's reaper. A failure after queuing shows as the slot's
            // `error` on the next state pull.
            if (slot0 >= 0 && slot0 < ogi::kTapeSlots)
                juce::ignoreUnused (processorRef.importDroppedFile (slot0, file, file.getFileName()));
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
    // Registers every bool in the initialisation data's toggle list, which
    // Juce.getToggleState() checks before it builds a ToggleState.
    for (const auto& relay : toggleRelays)
        options = options.withOptionsFrom (*relay);

    // -- NATIVE FUNCTIONS: exactly 34 (20 here + 4 drop + 10 preset) ----------

    // dblclick reset: every Float in ENGINEERING units, every Choice as its
    // index. The properties payload carries no default, and a JS default table
    // would drift from the NormalisableRange.
    options = options.withNativeFunction ("getParameterDefaults",
        [this] (auto&, auto complete)
        {
            complete (GiUiJson::parameterDefaultsToVar (processorRef));
        });

    // The hover-help language pair: the page's initI18n() calls both.
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

    // Live data, PULLED by the page at about 30 Hz. Always the full object.
    // buildUiTelemetry is the ONLY reader of the consuming feeds (frame,
    // output peaks L / R, sidechain peaks L / R, steal ring cursor); the
    // clock is read here, never inside it.
    options = options.withNativeFunction ("getTelemetry",
        [this] (auto&, auto complete)
        {
            complete (GiUiJson::telemetryToVar (
                processorRef.buildUiTelemetry (juce::Time::getMillisecondCounterHiRes())));
        });

    // The window figure is the engine's own curve, never a JS reimplementation.
    // The page sends { stream: 1...4 }; the processor takes 0...3.
    options = options.withNativeFunction ("getWindowCurve",
        [this] (auto& args, auto complete)
        {
            const auto arg = objectArg (args);
            const int stream = arg.isObject() ? static_cast<int> (arg.getProperty ("stream", 1)) : 1;
            complete (GiUiJson::windowCurveToVar (processorRef, juce::jlimit (1, GiParamIds::kStreams, stream) - 1));
        });

    // -- Scale STATE (not parameters): masks, the shared custom table, parsing.
    //    C++ is the only parser and the only copy of the collections; every
    //    setter answers with the model it accepted.
    options = options.withNativeFunction ("getScaleState",
        [this] (auto&, auto complete)
        {
            complete (GiUiJson::scaleStateToVar (processorRef));
        });

    options = options.withNativeFunction ("setDegreeMask",
        [this] (auto& args, auto complete)
        {
            const auto arg = objectArg (args);
            if (arg.isObject())
            {
                // Masks are keyed by the processor's own 1-based stream.
                const int stream = static_cast<int> (arg.getProperty ("stream", 0));
                const int coll   = static_cast<int> (arg.getProperty ("collection", -1));
                std::uint64_t mask = 0;
                if (stream >= 1 && stream <= GiParamIds::kStreams && coll >= 1 && coll < GiParamIds::kNumCollections
                    && hexToMask (arg.getProperty ("mask", "").toString(), mask))
                    processorRef.setDegreeMask (stream, coll, mask);
            }
            complete (GiUiJson::scaleStateToVar (processorRef));
        });

    options = options.withNativeFunction ("setCustomScale",
        [this] (auto& args, auto complete)
        {
            const auto arg = objectArg (args);
            if (! arg.isObject()) { complete (makeError ("noPitches")); return; }
            juce::String badToken;
            const auto error = processorRef.setCustomScale (
                arg.getProperty ("name", "").toString().substring (0, GranulatorInstrumentProcessor::kMaxCustomNameChars),
                arg.getProperty ("text", "").toString(), badToken);
            auto result = makeError (error, badToken);
            result.getDynamicObject()->setProperty ("state", GiUiJson::scaleStateToVar (processorRef));
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
            auto out = GiUiJson::tableToVar (table);
            out.getDynamicObject()->setProperty ("error", juce::String());
            complete (out);
        });

    // Load .scl: a DRAFT. Answers { name, text } for the editor fields, applies
    // nothing.
    options = options.withNativeFunction ("loadScala",
        [this] (auto&, auto complete)
        {
            // MSVC: hoist the SafePointer to a local, capture it by copy.
            auto safeThis = juce::Component::SafePointer<GranulatorInstrumentEditor> (this);
            if (fileDialogOpen) { complete (makeError ("cancelled")); return; }
            fileDialogOpen = true;
            fileChooser = std::make_shared<juce::FileChooser> ("Load Scala scale", juce::File(), "*.scl");
            fileChooser->launchAsync (
                juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                [safeThis, complete] (const juce::FileChooser& fc)
                {
                    // Editor gone: BARE return. `complete` belongs to the dead WebView.
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
                                                  .substring (0, GranulatorInstrumentProcessor::kMaxCustomNameChars));
                    obj->setProperty ("text", asText);
                    complete (juce::var (obj));
                });
        });

    // Save .scl: Scala text from the editor's tokens (ratios kept verbatim).
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
                        return;   // bare return, see loadScala
                    safeThis->fileDialogOpen = false;
                    const auto file = fc.getResult();
                    if (file == juce::File{}) { complete (makeError ("cancelled")); return; }
                    const bool ok = file.withFileExtension ("scl").replaceWithText (scl);
                    complete (makeError (ok ? juce::String() : juce::String ("io")));
                });
        });

    // -- Slots (STATE + actions). Every action answers { ok, error? }; the page
    //    re-pulls getSlotState after each action and whenever the telemetry's
    //    slotRev moves, so the answer only drives a toast.
    options = options.withNativeFunction ("getSlotState",
        [this] (auto&, auto complete)
        {
            // The revision first, then each slot with the `dirty` flag that
            // revision was hashed with.
            const auto revision = processorRef.getSlotStateRevision();
            std::array<ogi::SlotInfo, ogi::kTapeSlots> infos;
            std::array<bool, ogi::kTapeSlots> dirty {};
            for (int s = 0; s < ogi::kTapeSlots; ++s)
            {
                infos[static_cast<std::size_t> (s)] = processorRef.getSlotInfo (s);
                dirty[static_cast<std::size_t> (s)] = processorRef.getSlotDirty (s);
            }
            complete (GiUiJson::slotStateToVar (revision, infos, dirty));
        });

    // A bad slot answers null: the page then keeps that lane without peaks.
    options = options.withNativeFunction ("getSlotOverview",
        [this] (auto& args, auto complete)
        {
            const int slot0 = slotArg (args);
            complete (slot0 >= 0 ? GiUiJson::slotOverviewToVar (processorRef, slot0) : juce::var());
        });

    options = options.withNativeFunction ("loadSlot",
        [this] (auto& args, auto complete)
        {
            const int slot0 = slotArg (args);
            if (slot0 < 0) { complete (makeOk (false, "badSlot")); return; }
            launchSlotChooser (slot0, false, complete);
        });

    options = options.withNativeFunction ("relinkSlot",
        [this] (auto& args, auto complete)
        {
            const int slot0 = slotArg (args);
            if (slot0 < 0) { complete (makeOk (false, "badSlot")); return; }
            launchSlotChooser (slot0, true, complete);
        });

    // Empties the lane through the loader / handoff. The processor answers an
    // error key: empty = done, "recording" while a take is being captured or
    // committed into this slot, "badSlot".
    options = options.withNativeFunction ("clearSlot",
        [this] (auto& args, auto complete)
        {
            const int slot0 = slotArg (args);
            if (slot0 < 0) { complete (makeOk (false, "badSlot")); return; }
            const auto error = processorRef.clearSlot (slot0);
            complete (makeOk (error.isEmpty(), error));
        });

    options = options.withNativeFunction ("resetSlot",
        [this] (auto& args, auto complete)
        {
            const int slot0 = slotArg (args);
            if (slot0 < 0) { complete (makeOk (false, "badSlot")); return; }
            processorRef.requestReset (slot0);   // off-thread rebuild from pristine
            complete (makeOk (true));
        });

    options = options.withNativeFunction ("resetAll",
        [this] (auto&, auto complete)
        {
            processorRef.requestReset (-1);
            complete (makeOk (true));
        });

    // The per-slot Embed toggle is STATE, not a parameter. The processor call
    // returns nothing and silently ignores an empty or a factory slot, so the
    // refusals are derived here, before it.
    options = options.withNativeFunction ("setSlotEmbed",
        [this] (auto& args, auto complete)
        {
            const int slot0 = slotArg (args);
            if (slot0 < 0) { complete (makeOk (false, "badSlot")); return; }
            const auto arg = objectArg (args);
            const bool embed = static_cast<bool> (arg.getProperty ("embed", false));

            const auto before = processorRef.getSlotInfo (slot0);
            const auto scheme = GiUiJson::schemeOf (before.source);
            juce::String error;
            if (scheme.isEmpty())
                error = "empty";
            else if (scheme == "factory")
                error = "factory";
            else
            {
                processorRef.setSlotEmbed (slot0, embed);
                // Known only after an encode has run; a first refusal reaches
                // the page through encodedBytes on a later state pull.
                if (embed && before.tooLarge)
                    error = "tooLarge";
            }

            auto result = makeOk (error.isEmpty(), error);
            result.getDynamicObject()->setProperty ("embed", processorRef.getSlotInfo (slot0).embed);
            complete (result);
        });

    // D46: the slot's working copy (feedback writes included) as a WAV.
    // Chooser first, then the processor's save; see launchSaveRecording.
    options = options.withNativeFunction ("saveRecording",
        [this] (auto& args, auto complete)
        {
            launchSaveRecording (slotArg (args), complete);
        });

    // -- webview-drop-streaming: the module's four handlers, registered under
    //    their literal names. A name the module no longer provides still
    //    resolves its promise (false), so the page never hangs on it.
    {
        auto dropFns = dropSessions->getNativeFunctions();
        const auto dropFn = [&dropFns] (const char* fnName) -> juce::WebBrowserComponent::NativeFunction
        {
            for (auto& entry : dropFns)
                if (entry.first == juce::Identifier (fnName))
                    return std::move (entry.second);
            jassertfalse;   // the module renamed a function: update this list
            return [] (const juce::Array<juce::var>&, Completion complete) { complete (juce::var (false)); };
        };

        options = options
            .withNativeFunction ("dropSessionStart", dropFn ("dropSessionStart"))
            .withNativeFunction ("dropSessionAddFile", dropFn ("dropSessionAddFile"))
            .withNativeFunction ("dropSessionCommitFolder", dropFn ("dropSessionCommitFolder"))
            .withNativeFunction ("dropSessionCommitFile", dropFn ("dropSessionCommitFile"));
    }

    // -- Preset bridge (OuariconPresetManager contract, preset-manager.js) ----
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
        // Neighbour NAME only: preset-manager.js calls loadPreset() itself.
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
                    // savePresetToFile has NO factory guard: use it only for a
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
    // resource provider ("page not found").
    // The user-data folder lives in the per-user app-data tree (NOT %TEMP%,
    // which cleaners wipe and other processes lock) and is created before
    // WebView2 sees it. DAW hosts deny WebView2's default location.
    // Static linking (JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1 + NEEDS_WEBVIEW2)
    // is set in CMakeLists; without it the WebView silently renders blank.
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
        jassert (param != nullptr);   // ID drift -> silently dead control
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
        addAndMakeVisible (*webView);   // it stays visible for the editor's whole life
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

    // Fixed 1200 x 800 (v1-ui.yaml window). The page's body and #plugin are
    // 1200 x 800 px: keep them in sync.
    setResizable (false, false);
    setSize (1200, 800);
}

GranulatorInstrumentEditor::~GranulatorInstrumentEditor() = default;   // attachments -> webView -> dropSessions -> relays

void GranulatorInstrumentEditor::paint (juce::Graphics& g)
{
    // The WebView fills the editor: nothing to paint, unless WebView2 could
    // not start (Windows only). Colours are the page's own CSS tokens.
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
