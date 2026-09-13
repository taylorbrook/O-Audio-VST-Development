/*
   This file is part of O-Strata, an Ouaricon Audio plugin.
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

    PluginEditor.cpp
    O-Strata - Microtonal Wave-Terrain Synthesizer
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginEditor.h"
#include "EmbeddedTunings.h"
#include "TuningExporter.h"
#include "dsp/ModulationMatrix.h"

// ═══════════════════════════════════════════════════════════════════
// Toggle relay/attachment helpers
//
// These wrap the 3-step "create relay → withOptionsFrom → make attachment"
// pattern for any group of WebToggleButton-backed parameters. Used by the
// LFO sync + free-run vectors below. The bypassRelays / modSlotToggleRelays
// / delaySyncRelay groups follow the same shape and could fold in here in
// a future pass — out of scope for this commit.
// ═══════════════════════════════════════════════════════════════════

namespace
{
    void createToggleRelays (const juce::StringArray& ids,
                             std::vector<std::unique_ptr<juce::WebToggleButtonRelay>>& relays)
    {
        for (const auto& id : ids)
            relays.push_back (std::make_unique<juce::WebToggleButtonRelay> (id));
    }

    void addRelayOptions (const std::vector<std::unique_ptr<juce::WebToggleButtonRelay>>& relays,
                          juce::WebBrowserComponent::Options& options)
    {
        for (const auto& relay : relays)
            options = options.withOptionsFrom (*relay);
    }

    void attachToggleRelays (juce::AudioProcessorValueTreeState& apvts,
                             const juce::StringArray& ids,
                             const std::vector<std::unique_ptr<juce::WebToggleButtonRelay>>& relays,
                             std::vector<std::unique_ptr<juce::WebToggleButtonParameterAttachment>>& attachments)
    {
        for (int i = 0; i < ids.size(); ++i)
        {
            if (auto* param = apvts.getParameter (ids[i]))
            {
                attachments.push_back (
                    std::make_unique<juce::WebToggleButtonParameterAttachment> (
                        *param, *relays[static_cast<size_t> (i)], nullptr));
            }
        }
    }
}

// ═══════════════════════════════════════════════════════════════════
// Resource Provider
// ═══════════════════════════════════════════════════════════════════

static auto makeBinaryResource (const char* data, int size, const char* mimeType)
    -> std::optional<juce::WebBrowserComponent::Resource>
{
    auto* byteData = reinterpret_cast<const std::byte*> (data);
    return juce::WebBrowserComponent::Resource {
        std::vector<std::byte> (byteData, byteData + size),
        juce::String (mimeType)
    };
}

std::optional<juce::WebBrowserComponent::Resource>
OStrataAudioProcessorEditor::getResource (const juce::String& url)
{
    if (url == "/" || url == "/index.html")
        return makeBinaryResource (BinaryData::index_html,
                                   BinaryData::index_htmlSize, "text/html");

    // v1.21.0: the i18n table. EMBEDDED (CMakeLists.txt) *and* SERVED (here) —
    // a file that is one but not the other is a 404 that presents as a missing
    // panel and nothing else, which is the highest-frequency mistake in this
    // work. check-i18n assertion 8 asserts both halves; boot-all-uis is blind
    // to this half by design, because it serves the copied file tree rather
    // than getResource().
    if (url == "/js/i18n.js")
        return makeBinaryResource (BinaryData::i18n_js,
                                   BinaryData::i18n_jsSize, "application/javascript");

    if (url == "/js/juce/index.js")
        return makeBinaryResource (BinaryData::index_js,
                                   BinaryData::index_jsSize, "application/javascript");

    if (url == "/js/juce/check_native_interop.js")
        return makeBinaryResource (BinaryData::check_native_interop_js,
                                   BinaryData::check_native_interop_jsSize, "application/javascript");

    // Stage 3 Round B: the terrain view module (Orbits.h port, renderers, perf ring).
    // EMBEDDED (CMakeLists.txt, symbol terrainview_js — the hyphen is stripped) *and*
    // SERVED here; the layout gate greps both halves (plan Decision 45).
    if (url == "/js/terrain-view.js")
        return makeBinaryResource (BinaryData::terrainview_js,
                                   BinaryData::terrainview_jsSize, "application/javascript");

    // Stage 3 Round A: the Terrain tab's botanical plate (CSS background, multiply, 0.3 α)
    if (url == "/img/shell_conchologiaiconi12reev_0090.png")
        return makeBinaryResource (BinaryData::shell_conchologiaiconi12reev_0090_png,
                                   BinaryData::shell_conchologiaiconi12reev_0090_pngSize, "image/png");

    return std::nullopt;
}

// ═══════════════════════════════════════════════════════════════════
// JSON Array Helpers
// ═══════════════════════════════════════════════════════════════════

template <typename Container, typename Fn>
static juce::String toJsonArray (const Container& items, Fn elementToString)
{
    juce::String json = "[";
    bool first = true;
    for (const auto& item : items)
    {
        if (! first) json += ",";
        first = false;
        json += elementToString (item);
    }
    json += "]";
    return json;
}

// Forces the "tuningPreset" choice param to the Custom slot for persistence.
// Called from every native fn that mutates the active tuning out-of-band.
static void syncTuningPresetToCustom (juce::AudioProcessorValueTreeState& apvts)
{
    if (auto* param = apvts.getParameter ("tuningPreset"))
        param->setValueNotifyingHost (param->convertTo0to1 (
            static_cast<float> (StrataParamIds::kCustomTuningPresetIndex)));
}

// Round B (plan Decision 39): the {ok, reason} result of the two import natives.
// reason ∈ "cancelled" | "tooLarge" | "undecodable" (the page maps the last two onto
// label.importTooLarge / label.importFailed; cancelled shows nothing).
static juce::var importResult (bool ok, const char* reason)
{
    auto* obj = new juce::DynamicObject();
    obj->setProperty ("ok", ok);
    obj->setProperty ("reason", juce::String (reason));
    return juce::var (obj);
}

// The page sends "A" / "B".
static int oscFromArg (const juce::Array<juce::var>& args, int index)
{
    return args.size() > index && args[index].toString().trim().equalsIgnoreCase ("B") ? 1 : 0;
}

// ═══════════════════════════════════════════════════════════════════
// Native Functions
// ═══════════════════════════════════════════════════════════════════

juce::WebBrowserComponent::Options
OStrataAudioProcessorEditor::addNativeFunctions (juce::WebBrowserComponent::Options options)
{
    // ── UI LANGUAGE (v1.21.0) ─────────────────────────────────────────
    // Not a parameter: it must not appear in a DAW automation lane, and a
    // preset must not be able to change which language somebody reads their
    // interface in. It rides the APVTS state tree as a non-parameter property.
    options = options.withNativeFunction ("getUiLanguage",
        [this] (const juce::Array<juce::var>&, auto complete)
        {
            complete (juce::var (OStrataAudioProcessor::languageCode (
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
                    OStrataAudioProcessor::languageIndex (args[0].toString()),
                    std::memory_order_release);

            complete (juce::var (OStrataAudioProcessor::languageCode (
                processorRef.uiLanguage.load (std::memory_order_acquire))));
        });

    // Tuning intervals
    options = options.withNativeFunction ("getTuningIntervals",
        [this] (const juce::Array<juce::var>&, auto complete) {
            auto intervals = processorRef.getTuningEngine()->getIntervals();
            complete (toJsonArray (intervals, [] (double v) { return juce::String (v, 6); }));
        });

    options = options.withNativeFunction ("getTuningName",
        [this] (const juce::Array<juce::var>&, auto complete) {
            complete (processorRef.getTuningEngine()->getActiveTuningName());
        });

    options = options.withNativeFunction ("setSingleInterval",
        [this] (const juce::Array<juce::var>& args, auto complete) {
            if (args.size() >= 2)
            {
                int index = static_cast<int> (args[0]);
                double cents = static_cast<double> (args[1]);
                processorRef.getTuningEngine()->setSingleInterval (index, cents);
                syncTuningPresetToCustom (processorRef.getAPVTS());
                complete (true);
                return;
            }
            complete (false);
        });

    // Tonic
    options = options.withNativeFunction ("setTonicNote",
        [this] (const juce::Array<juce::var>& args, auto complete) {
            if (args.size() >= 1)
            {
                int tonicIndex = static_cast<int> (args[0]);
                processorRef.getTuningEngine()->setTonicNote (tonicIndex);
                // Sync APVTS so the tonic persists across DAW save/load
                if (auto* param = processorRef.getAPVTS().getParameter ("tonic"))
                    param->setValueNotifyingHost (param->convertTo0to1 (static_cast<float> (tonicIndex)));
                complete (true);
                return;
            }
            complete (false);
        });

    options = options.withNativeFunction ("getTonicNote",
        [this] (const juce::Array<juce::var>&, auto complete) {
            complete (processorRef.getTuningEngine()->getTonicNote());
        });

    // Octave stretch (read-side only — writes go through the WebSliderRelay;
    // the dead set/get master-tune + temperament fns were removed in IN-13)
    options = options.withNativeFunction ("getOctaveStretch",
        [this] (const juce::Array<juce::var>&, auto complete) {
            complete (processorRef.getTuningEngine()->getOctaveStretch());
        });

    // Scala file I/O
    options = options.withNativeFunction ("loadScalaFile",
        [this] (const juce::Array<juce::var>&, auto complete) {
            auto chooser = std::make_shared<juce::FileChooser> (
                "Load Scala File",
                juce::File::getSpecialLocation (juce::File::userDocumentsDirectory),
                "*.scl");

            juce::Component::SafePointer<OStrataAudioProcessorEditor> safeThis (this);
            chooser->launchAsync (juce::FileBrowserComponent::openMode
                                | juce::FileBrowserComponent::canSelectFiles,
                [safeThis, chooser, complete] (const juce::FileChooser& fc) {
                    if (safeThis == nullptr)
                        return; // editor destroyed — `complete` is owned by the dead WebView, never call it
                    auto& proc = safeThis->processorRef;
                    auto file = fc.getResult();
                    if (file.existsAsFile())
                    {
                        bool success = proc.getTuningEngine()->loadScalaFile (file);
                        if (success)
                        {
                            syncTuningPresetToCustom (proc.getAPVTS());
                        }
                        complete (success ? juce::var (proc.getTuningEngine()->getActiveTuningName())
                                         : juce::var());
                    }
                    else
                    {
                        complete (juce::var());
                    }
                });
        });

    options = options.withNativeFunction ("loadKBMFile",
        [this] (const juce::Array<juce::var>&, auto complete) {
            auto chooser = std::make_shared<juce::FileChooser> (
                "Load Keyboard Mapping",
                juce::File::getSpecialLocation (juce::File::userDocumentsDirectory),
                "*.kbm");

            juce::Component::SafePointer<OStrataAudioProcessorEditor> safeThis (this);
            chooser->launchAsync (juce::FileBrowserComponent::openMode
                                | juce::FileBrowserComponent::canSelectFiles,
                [safeThis, chooser, complete] (const juce::FileChooser& fc) {
                    if (safeThis == nullptr)
                        return; // editor destroyed — never call `complete` on the dead path
                    auto file = fc.getResult();
                    if (file.existsAsFile())
                        complete (safeThis->processorRef.getTuningEngine()->loadKBMFile (file));
                    else
                        complete (false);
                });
        });

    // Save Scala/KBM files
    options = options.withNativeFunction ("saveScalaFile",
        [this] (const juce::Array<juce::var>&, auto complete) {
            auto chooser = std::make_shared<juce::FileChooser> (
                "Save Scala File",
                juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                    .getChildFile ("tuning.scl"),
                "*.scl");

            juce::Component::SafePointer<OStrataAudioProcessorEditor> safeThis (this);
            chooser->launchAsync (juce::FileBrowserComponent::saveMode
                                | juce::FileBrowserComponent::canSelectFiles,
                [safeThis, chooser, complete] (const juce::FileChooser& fc) {
                    if (safeThis == nullptr)
                        return; // editor destroyed — never call `complete` on the dead path
                    auto file = fc.getResult();
                    if (file != juce::File())
                    {
                        auto content = safeThis->processorRef.getTuningEngine()->generateScalaFileContent();
                        file.replaceWithText (content);
                        complete (file.getFileName());
                    }
                    else
                    {
                        complete (juce::var());
                    }
                });
        });

    options = options.withNativeFunction ("saveKBMFile",
        [this] (const juce::Array<juce::var>&, auto complete) {
            auto chooser = std::make_shared<juce::FileChooser> (
                "Save Keyboard Mapping",
                juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                    .getChildFile ("mapping.kbm"),
                "*.kbm");

            juce::Component::SafePointer<OStrataAudioProcessorEditor> safeThis (this);
            chooser->launchAsync (juce::FileBrowserComponent::saveMode
                                | juce::FileBrowserComponent::canSelectFiles,
                [safeThis, chooser, complete] (const juce::FileChooser& fc) {
                    if (safeThis == nullptr)
                        return; // editor destroyed — never call `complete` on the dead path
                    auto file = fc.getResult();
                    if (file != juce::File())
                    {
                        auto content = safeThis->processorRef.getTuningEngine()->generateKBMFileContent();
                        file.replaceWithText (content);
                        complete (file.getFileName());
                    }
                    else
                    {
                        complete (juce::var());
                    }
                });
        });

    // Scale generators
    options = options.withNativeFunction ("generateEDO",
        [this] (const juce::Array<juce::var>& args, auto complete) {
            if (args.size() >= 2)
            {
                auto intervals = ScaleGenerator::generateEDO (
                    static_cast<int> (args[0]), static_cast<double> (args[1]));
                complete (toJsonArray (intervals, [] (double v) { return juce::String (v, 6); }));
                return;
            }
            complete (juce::var());
        });

    options = options.withNativeFunction ("generateHarmonicSeries",
        [this] (const juce::Array<juce::var>& args, auto complete) {
            if (args.size() >= 2)
            {
                auto intervals = ScaleGenerator::generateHarmonicSeries (
                    static_cast<int> (args[0]), static_cast<int> (args[1]));
                complete (toJsonArray (intervals, [] (double v) { return juce::String (v, 6); }));
                return;
            }
            complete (juce::var());
        });

    options = options.withNativeFunction ("generateRank2",
        [this] (const juce::Array<juce::var>& args, auto complete) {
            if (args.size() >= 3)
            {
                auto intervals = ScaleGenerator::generateRank2 (
                    static_cast<double> (args[0]),
                    static_cast<double> (args[1]),
                    static_cast<int> (args[2]));
                complete (toJsonArray (intervals, [] (double v) { return juce::String (v, 6); }));
                return;
            }
            complete (juce::var());
        });

    // Embedded tuning library (API fixes from RESEARCH.md applied)
    options = options.withNativeFunction ("getEmbeddedTuningList",
        [this] (const juce::Array<juce::var>&, auto complete) {
            const auto& tunings = EmbeddedTunings::getAllTunings();
            complete (toJsonArray (tunings, [] (const auto& t) {
                return "{\"id\":\"" + juce::String (t.id)
                     + "\",\"name\":\"" + juce::String (t.name)
                     + "\",\"category\":\"" + juce::String (t.category)
                     + "\",\"noteCount\":" + juce::String (static_cast<int> (t.intervals.size()))
                     + ",\"period\":" + juce::String (t.period, 1)
                     + "}";
            }));
        });

    options = options.withNativeFunction ("loadEmbeddedTuning",
        [this] (const juce::Array<juce::var>& args, auto complete) {
            if (args.size() >= 1)
            {
                const auto* tuning = EmbeddedTunings::getTuningById (
                    args[0].toString().toStdString());
                if (tuning != nullptr && ! tuning->intervals.empty())
                {
                    auto intervals = tuning->intervals;
                    intervals.push_back (tuning->period);
                    processorRef.getTuningEngine()->setCustomIntervals (
                        intervals, juce::String (tuning->name));
                    syncTuningPresetToCustom (processorRef.getAPVTS());
                    complete (true);
                    return;
                }
            }
            complete (false);
        });

    // Apply generated scale (for scale generator results)
    options = options.withNativeFunction ("applyGeneratedScale",
        [this] (const juce::Array<juce::var>& args, auto complete) {
            if (args.size() >= 1)
            {
                auto jsonArray = juce::JSON::parse (args[0].toString());
                if (auto* arr = jsonArray.getArray())
                {
                    std::vector<double> intervals;
                    for (const auto& val : *arr)
                        intervals.push_back (static_cast<double> (val));
                    // IN-14: keep the generator's scale name so it survives
                    // reopen instead of reverting to "Generated"
                    juce::String name = args.size() >= 2 ? args[1].toString() : juce::String();
                    if (name.isEmpty())
                        name = "Generated";
                    processorRef.getTuningEngine()->setCustomIntervals (intervals, name);
                    syncTuningPresetToCustom (processorRef.getAPVTS());
                    complete (true);
                    return;
                }
            }
            complete (false);
        });

    // HTML export (API fix: toHTML not generateHTML)
    options = options.withNativeFunction ("exportTuningHTML",
        [this] (const juce::Array<juce::var>&, auto complete) {
            auto chooser = std::make_shared<juce::FileChooser> (
                "Export Tuning Documentation",
                juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                    .getChildFile ("tuning-export.html"),
                "*.html");

            juce::Component::SafePointer<OStrataAudioProcessorEditor> safeThis (this);
            chooser->launchAsync (juce::FileBrowserComponent::saveMode
                                | juce::FileBrowserComponent::canSelectFiles,
                [safeThis, chooser, complete] (const juce::FileChooser& fc) {
                    if (safeThis == nullptr)
                        return; // editor destroyed — never call `complete` on the dead path
                    auto file = fc.getResult();
                    if (file != juce::File())
                    {
                        auto html = TuningExporter::toHTML (
                            *safeThis->processorRef.getTuningEngine(), "O-Strata");
                        file.replaceWithText (html);
                        complete (true);
                    }
                    else
                    {
                        complete (false);
                    }
                });
        });

    // ─── Stage 3 (plan Decision 8): the page-driven re-push handshake. Called by the
    //     page at boot (its listeners exist by then) and inside __refreshAllControls
    //     (preset apply); the next timer tick forces heldNotes + terrainStatus +
    //     terrainCycle regardless of the change gates. Round B adds terrainState /
    //     terrainHeightmap to the forced set. ───
    options = options.withNativeFunction ("requestTerrainRepush",
        [this] (const juce::Array<juce::var>&, auto complete) {
            repushPending = true;
            complete (true);
        });

    // ─── Stage 3 Round B (plan Decision 39) + Stage 4 Round A (Decisions 2, 13): the
    //     three import natives. `chooseTerrainImage (osc)` opens the native chooser
    //     (*.png) — cap kMaxImportFileBytes (8 MiB) by file, the slot records the path;
    //     `importTerrainImageData (osc, name, base64)` takes the bytes a drop streamed
    //     through the page (WKWebView strips file paths from the DataTransfer — memory
    //     critical_webview_drag_drop_macos), whole file in one call — cap kMaxImportBytes
    //     (2 MiB) by bytes, unchanged since Stage 3 (a dropped file has no path to
    //     relocate); `locateTerrainImage (osc)` re-links a missing path-form source —
    //     cap 8 MiB + the SHA-256 must match the slot. All hand the bytes to the Stage 2
    //     API (message thread, decode + queued publish; works with the editor closed).
    //     The first two select Imported… from C++ on success and force the pushes.
    //     Result = {ok, reason}. ───
    options = options.withNativeFunction ("chooseTerrainImage",
        [this] (const juce::Array<juce::var>& args, auto complete) {
            const int osc = oscFromArg (args, 0);
            launchPngChooser (osc, "Import Terrain Image",
                [complete, osc] (OStrataAudioProcessorEditor& ed, const juce::File& file) {
                    if (file == juce::File())
                    {
                        complete (importResult (false, "cancelled"));   // parameter untouched
                        return;
                    }
                    if (file.getSize() > OStrataAudioProcessor::kMaxImportFileBytes)
                    {
                        complete (importResult (false, "tooLargeFile"));
                        return;
                    }
                    if (! ed.processorRef.importTerrainFile (osc, file))
                    {
                        complete (importResult (false, "undecodable"));
                        return;
                    }
                    ed.selectImportedTerrain (osc);
                    ed.repushPending = true;
                    complete (importResult (true, ""));
                });
        });

    // ─── Stage 4 Round A (Decision 13): Locate… for a path-form slot whose file is
    //     missing. A preset promises a specific terrain, so a file whose SHA-256 differs
    //     from the slot's is REFUSED and nothing is touched — Import… is one click away
    //     and re-stamps by definition. A match re-imports the file's bytes (the slot
    //     records the new path, sourceMissing clears); the terrain is not re-selected
    //     (it already reads Imported…). The located file re-enters the size rule at the
    //     next save. ───
    options = options.withNativeFunction ("locateTerrainImage",
        [this] (const juce::Array<juce::var>& args, auto complete) {
            const int osc = oscFromArg (args, 0);
            launchPngChooser (osc, "Locate Terrain Image",
                [complete, osc] (OStrataAudioProcessorEditor& ed, const juce::File& file) {
                    if (file == juce::File())
                    {
                        complete (importResult (false, "cancelled"));
                        return;
                    }
                    if (file.getSize() > OStrataAudioProcessor::kMaxImportFileBytes)
                    {
                        complete (importResult (false, "tooLargeFile"));
                        return;
                    }
                    juce::MemoryBlock bytes;
                    if (! file.loadFileAsData (bytes) || bytes.getSize() == 0)
                    {
                        complete (importResult (false, "undecodable"));
                        return;
                    }
                    if (juce::SHA256 (bytes.getData(), bytes.getSize()).toHexString()
                            != ed.processorRef.getImportSlotCopy (osc).sha256)
                    {
                        complete (importResult (false, "hashMismatch"));   // nothing touched
                        return;
                    }
                    if (! ed.processorRef.importTerrainFile (osc, file))
                    {
                        complete (importResult (false, "undecodable"));
                        return;
                    }
                    ed.repushPending = true;
                    complete (importResult (true, ""));
                });
        });

    options = options.withNativeFunction ("importTerrainImageData",
        [this] (const juce::Array<juce::var>& args, auto complete) {
            if (args.size() < 3)
            {
                complete (importResult (false, "undecodable"));
                return;
            }
            const int osc = oscFromArg (args, 0);
            const juce::String name = args[1].toString();
            // juce::Base64::convertFromBase64 (never MemoryBlock::fromBase64Encoding — JUCE's own format)
            juce::MemoryOutputStream decoded;
            if (! juce::Base64::convertFromBase64 (decoded, args[2].toString()) || decoded.getDataSize() == 0)
            {
                complete (importResult (false, "undecodable"));
                return;
            }
            if (static_cast<juce::int64> (decoded.getDataSize()) > OStrataAudioProcessor::kMaxImportBytes)   // the drop cap (Decision 2)
            {
                complete (importResult (false, "tooLarge"));
                return;
            }
            const juce::MemoryBlock bytes (decoded.getData(), decoded.getDataSize());
            if (! processorRef.importTerrainImage (osc, bytes, name))
            {
                complete (importResult (false, "undecodable"));
                return;
            }
            selectImportedTerrain (osc);
            repushPending = true;
            complete (importResult (true, ""));
        });

    // ─── Stage 3 Round B (plan Decision 37): PERF-03 report from the page's frame ring ───
    options = options.withNativeFunction ("reportViewPerf",
        [this] (const juce::Array<juce::var>& args, auto complete) {
            if (args.size() > 0)
                logViewPerf (args[0].toString());
            complete (true);
        });


    // Mod matrix source/dest name lists for UI dropdowns
    options = options.withNativeFunction ("getModSourceNames",
        [] (const juce::Array<juce::var>&, auto complete) {
            auto names = getModSourceNames();
            complete (toJsonArray (names, [] (const juce::String& s) {
                return "\"" + s + "\"";
            }));
        });

    options = options.withNativeFunction ("getModDestNames",
        [] (const juce::Array<juce::var>&, auto complete) {
            auto names = getModDestNames();
            complete (toJsonArray (names, [] (const juce::String& s) {
                return "\"" + s + "\"";
            }));
        });

    // ─── Preset Manager ───────────────────────────────────────────────

    options = options.withNativeFunction ("getPresetListWithCategories",
        [this] (const juce::Array<juce::var>&, auto complete) {
            auto categorized = processorRef.getPresetManager().getPresetListWithCategories();
            juce::String json = "{";
            bool firstCat = true;
            for (const auto& [cat, names] : categorized)
            {
                if (! firstCat) json += ",";
                firstCat = false;
                json += juce::JSON::toString (cat) + ":"
                     + toJsonArray (names, [] (const juce::String& n) { return juce::JSON::toString (n); });
            }
            json += "}";
            complete (json);
        });

    options = options.withNativeFunction ("getCurrentPreset",
        [this] (const juce::Array<juce::var>&, auto complete) {
            complete (processorRef.getPresetManager().getCurrentPresetName());
        });

    options = options.withNativeFunction ("loadPresetFromCategory",
        [this] (const juce::Array<juce::var>& args, auto complete) {
            if (args.size() >= 2)
            {
                auto ok = processorRef.getPresetManager()
                    .loadPresetFromCategory (args[0].toString(), args[1].toString());
                if (ok) processorRef.notifyStateChanged();   // Round B (Decision 34): force the five pushes
                complete (ok);
                return;
            }
            complete (false);
        });

    options = options.withNativeFunction ("loadPresetByName",
        [this] (const juce::Array<juce::var>& args, auto complete) {
            if (args.size() >= 1)
            {
                const bool ok = processorRef.getPresetManager().loadPreset (args[0].toString());
                if (ok) processorRef.notifyStateChanged();
                complete (ok);
                return;
            }
            complete (false);
        });

    options = options.withNativeFunction ("selectNextPreset",
        [this] (const juce::Array<juce::var>&, auto complete) {
            // Returns the name it loaded (always loads when the bank is non-empty).
            const juce::String name = processorRef.getPresetManager().getNextPreset();
            if (name.isNotEmpty()) processorRef.notifyStateChanged();
            complete (name);
        });

    options = options.withNativeFunction ("selectPreviousPreset",
        [this] (const juce::Array<juce::var>&, auto complete) {
            // Returns the name it loaded (always loads when the bank is non-empty).
            const juce::String name = processorRef.getPresetManager().getPreviousPreset();
            if (name.isNotEmpty()) processorRef.notifyStateChanged();
            complete (name);
        });

    options = options.withNativeFunction ("savePreset",
        [this] (const juce::Array<juce::var>& args, auto complete) {
            if (args.size() >= 1)
            {
                complete (processorRef.getPresetManager().savePreset (args[0].toString()));
                return;
            }
            complete (false);
        });

    options = options.withNativeFunction ("isFactoryPreset",
        [this] (const juce::Array<juce::var>& args, auto complete) {
            if (args.size() >= 1)
            {
                complete (processorRef.getPresetManager().isFactoryPreset (args[0].toString()));
                return;
            }
            complete (false);
        });

    return options;
}

// ═══════════════════════════════════════════════════════════════════
// Constructor
// ═══════════════════════════════════════════════════════════════════

OStrataAudioProcessorEditor::OStrataAudioProcessorEditor (OStrataAudioProcessor& p)
    : AudioProcessorEditor (p),
      processorRef (p)
{
    // ─────────────────────────────────────────────────────────────
    // Step 1: Create relays (before WebView)
    // ─────────────────────────────────────────────────────────────

    // Get parameter ID lists from shared definitions (StrataParamIds.h)
    auto sliderIds    = StrataParamIds::allSliderIds();
    auto comboIds     = StrataParamIds::allComboIds();
    auto bypassIds    = StrataParamIds::bypassToggleIds();
    auto modToggleIds = StrataParamIds::modSlotToggleIds();

    // Build LFO sync + free-run ID lists from the 1..4 convention (used in
    // the relay/options/attachment phases below).
    juce::StringArray lfoSyncIds, lfoFreeRunIds;
    for (int i = 1; i <= 4; ++i)
    {
        lfoSyncIds.add ("lfo" + juce::String (i) + "Sync");
        lfoFreeRunIds.add ("lfo" + juce::String (i) + "FreeRun");
    }

    // Slider relays
    for (const auto& id : sliderIds)
        sliderRelays.push_back (std::make_unique<juce::WebSliderRelay> (id));

    // Combo relays — the 4 Choice params per oscillator (Terrain, TerEdge, Orbit, Quality).
    // Relays only in Stage 1 (CONTEXT D1); the page binds them in Phase 3.1.
    for (const auto& id : comboIds)
        comboRelays.push_back (std::make_unique<juce::WebComboBoxRelay> (id));

    // 1 toggle relay (delaySync)
    delaySyncRelay = std::make_unique<juce::WebToggleButtonRelay> ("delaySync");

    // LFO sync + free-run toggle relays
    createToggleRelays (lfoSyncIds, lfoSyncRelays);
    createToggleRelays (lfoFreeRunIds, lfoFreeRunRelays);

    // Bypass toggle relays
    for (const auto& id : bypassIds)
        bypassRelays.push_back (std::make_unique<juce::WebToggleButtonRelay> (id));

    // Mod slot toggle relays
    for (const auto& id : modToggleIds)
        modSlotToggleRelays.push_back (std::make_unique<juce::WebToggleButtonRelay> (id));

    // ─────────────────────────────────────────────────────────────
    // Step 2: Build WebView options with relays + native functions
    // ─────────────────────────────────────────────────────────────

    auto options = juce::WebBrowserComponent::Options{}
        .withBackend (juce::WebBrowserComponent::Options::Backend::webview2)
        .withNativeIntegrationEnabled()
        .withResourceProvider ([this] (const auto& url) { return getResource (url); });

    // Add all slider relays to options
    for (const auto& relay : sliderRelays)
        options = options.withOptionsFrom (*relay);

    // Add combo relays
    for (const auto& relay : comboRelays)
        options = options.withOptionsFrom (*relay);

    // Add toggle relay
    options = options.withOptionsFrom (*delaySyncRelay);

    // Add LFO sync + free-run toggle relays
    addRelayOptions (lfoSyncRelays, options);
    addRelayOptions (lfoFreeRunRelays, options);

    // Add bypass toggle relays
    for (const auto& relay : bypassRelays)
        options = options.withOptionsFrom (*relay);

    // Add mod slot toggle relays
    for (const auto& relay : modSlotToggleRelays)
        options = options.withOptionsFrom (*relay);

    // Add native tuning functions
    options = addNativeFunctions (options);

   #if JUCE_WINDOWS
    options = options.withWinWebView2Options (
        juce::WebBrowserComponent::Options::WinWebView2{}
            .withUserDataFolder (
                juce::File::getSpecialLocation (juce::File::tempDirectory)
                    .getChildFile ("OStrata_WebView")));
   #endif

    // Construct WebView
    webView = std::make_unique<juce::WebBrowserComponent> (options);

    // ─────────────────────────────────────────────────────────────
    // Step 3: Create attachments (after WebView)
    // ─────────────────────────────────────────────────────────────

    // Slider attachments
    for (int i = 0; i < sliderIds.size(); ++i)
    {
        auto* param = processorRef.getAPVTS().getParameter (sliderIds[i]);
        if (param != nullptr)
        {
            sliderAttachments.push_back (
                std::make_unique<juce::WebSliderParameterAttachment> (
                    *param, *sliderRelays[static_cast<size_t> (i)], nullptr));
        }
    }
    // Every slider relay must be backed by a parameter (StrataParamIds::allSliderIds()
    // and createParameterLayout() must agree — 166 after the Stage 1 second pass).
    jassert (sliderAttachments.size() == sliderRelays.size());

    // Combo attachments
    for (int i = 0; i < comboIds.size(); ++i)
    {
        auto* param = processorRef.getAPVTS().getParameter (comboIds[i]);
        if (param != nullptr)
        {
            comboAttachments.push_back (
                std::make_unique<juce::WebComboBoxParameterAttachment> (
                    *param, *comboRelays[static_cast<size_t> (i)], nullptr));
        }
    }
    jassert (comboAttachments.size() == comboRelays.size());   // 8

    // 1 toggle attachment (delaySync)
    auto* delaySyncParam = processorRef.getAPVTS().getParameter ("delaySync");
    if (delaySyncParam != nullptr)
    {
        delaySyncAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment> (
            *delaySyncParam, *delaySyncRelay, nullptr);
    }

    // LFO sync + free-run toggle attachments
    attachToggleRelays (processorRef.getAPVTS(), lfoSyncIds, lfoSyncRelays, lfoSyncAttachments);
    attachToggleRelays (processorRef.getAPVTS(), lfoFreeRunIds, lfoFreeRunRelays, lfoFreeRunAttachments);

    // Bypass toggle attachments
    for (int i = 0; i < bypassIds.size(); ++i)
    {
        auto* param = processorRef.getAPVTS().getParameter (bypassIds[i]);
        if (param != nullptr)
        {
            bypassAttachments.push_back (
                std::make_unique<juce::WebToggleButtonParameterAttachment> (
                    *param, *bypassRelays[static_cast<size_t> (i)], nullptr));
        }
    }

    // Mod slot toggle attachments
    for (int i = 0; i < modToggleIds.size(); ++i)
    {
        auto* param = processorRef.getAPVTS().getParameter (modToggleIds[i]);
        if (param != nullptr)
        {
            modSlotToggleAttachments.push_back (
                std::make_unique<juce::WebToggleButtonParameterAttachment> (
                    *param, *modSlotToggleRelays[static_cast<size_t> (i)], nullptr));
        }
    }

    // ─────────────────────────────────────────────────────────────
    // Step 4: Show WebView + navigate (matching O-Bells pattern)
    // ─────────────────────────────────────────────────────────────

    addAndMakeVisible (*webView);

    webView->goToURL (juce::WebBrowserComponent::getResourceProviderRoot());
    setSize (1200, 800);

    // Start polling for active MIDI notes (30 Hz is plenty for UI updates)
    startTimerHz (30);
}

OStrataAudioProcessorEditor::~OStrataAudioProcessorEditor()
{
    stopTimer();
}

// ═══════════════════════════════════════════════════════════════════
// Paint / Resized
// ═══════════════════════════════════════════════════════════════════

void OStrataAudioProcessorEditor::paint (juce::Graphics&)
{
    // WebView fills entire editor — no custom painting needed
}

void OStrataAudioProcessorEditor::resized()
{
    if (webView != nullptr)
        webView->setBounds (getLocalBounds());
}

// ═══════════════════════════════════════════════════════════════════
// Timer (30 Hz): heldNotes (TrueKeys), terrainStatus, terrainCycle pushes
// Every push goes through emitEventIfBrowserIsVisible (Stage 3 plan Decisions
// 8 / 15); the page registers the listeners before it calls requestTerrainRepush.
// ═══════════════════════════════════════════════════════════════════

void OStrataAudioProcessorEditor::timerCallback()
{
    if (webView == nullptr)
        return;

    // A moved stateGeneration (setStateInformation, the preset natives — Decision 34)
    // forces every push exactly like requestTerrainRepush.
    const uint32_t generation = processorRef.getStateGeneration();
    const bool force = repushPending || generation != lastStateGeneration;
    repushPending = false;
    lastStateGeneration = generation;
    ++tickCount;

    pushHeldNotes (force);
    for (int osc = 0; osc < 2; ++osc)
    {
        pushState (osc, force);                    // O(1): the ring's newest slot
        if (force || (tickCount & 1u) == 0)        // ≤ 15 Hz: status (struct compare) + cycle (new complete cycle only)
        {
            pushStatus (osc, force);
            pushCycle (osc);
        }
        if (force || tickCount % 3u == 0)          // ≤ 10 Hz by construction (Decision 35): the heightmap, key-gated
            pushHeightmap (osc, force);
    }
}

void OStrataAudioProcessorEditor::pushHeldNotes (bool force)
{
    auto currentNotes = processorRef.getActiveNotes();
    if (! force && currentNotes == lastSentNotes)
        return;
    lastSentNotes = currentNotes;

    juce::Array<juce::var> notes, freqs;
    for (const auto& n : currentNotes)
    {
        notes.add (n.first);
        freqs.add (n.second);
    }
    auto* obj = new juce::DynamicObject();
    obj->setProperty ("notes", notes);
    obj->setProperty ("freqs", freqs);
    webView->emitEventIfBrowserIsVisible ("heldNotes", juce::var (obj));
}

void OStrataAudioProcessorEditor::pushStatus (int osc, bool force)
{
    const auto status = processorRef.getTerrainStatus (osc);
    if (! force && hasLastStatus[osc] && status == lastStatus[osc])
        return;
    lastStatus[osc] = status;
    hasLastStatus[osc] = true;
    webView->emitEventIfBrowserIsVisible ("terrainStatus", TerrainViewFeed::makeStatusEvent (osc, status));
}

void OStrataAudioProcessorEditor::pushCycle (int osc)
{
    // A non-advancing writeIndex is idle: no push, the page keeps the last cycle drawn.
    if (TerrainViewFeed::copyCycle (processorRef, osc, cycleScratch[osc], cycleOut.data()) == 0)
        return;
    webView->emitEventIfBrowserIsVisible ("terrainCycle", TerrainViewFeed::makeCycleEvent (osc, cycleOut.data()));
}

// Round B (plan Decision 32): {osc, theta, x, y, h} from the ring's newest slot — pushed
// when the ring advanced AND the rounded quad changed; a forced tick with an idle ring
// repeats the last known playhead (the page-reload case).
void OStrataAudioProcessorEditor::pushState (int osc, bool force)
{
    TerrainViewFeed::Playhead ph;
    if (! TerrainViewFeed::getPlayhead (processorRef, osc, lastWriteIndexState[osc], ph))
    {
        if (! (force && hasLastState[osc]))
            return;   // idle: nothing to say; the page keeps its last values
        ph = lastState[osc];
    }
    else if (! force && hasLastState[osc]
             && juce::exactlyEqual (ph.theta, lastState[osc].theta) && juce::exactlyEqual (ph.x, lastState[osc].x)
             && juce::exactlyEqual (ph.y, lastState[osc].y) && juce::exactlyEqual (ph.h, lastState[osc].h))
    {
        return;   // the rounded quad did not change (every field is already at 1e-4)
    }
    lastState[osc] = ph;
    hasLastState[osc] = true;
    webView->emitEventIfBrowserIsVisible ("terrainState", TerrainViewFeed::makeStateEvent (osc, ph));
}

// Round B (plan Decision 31): the active surface on the 64 × 64 grid, gated by a key over
// the terrain inputs + the four generations (≈ 21.8 K chars of base64 per push).
void OStrataAudioProcessorEditor::pushHeightmap (int osc, bool force)
{
    const uint64_t key = TerrainViewFeed::heightmapKey (processorRef, osc);
    if (! force && key == lastHeightmapKey[osc])
        return;
    lastHeightmapKey[osc] = key;
    TerrainViewFeed::copyHeightmap (processorRef, osc, heightmapOut.data());
    webView->emitEventIfBrowserIsVisible ("terrainHeightmap", TerrainViewFeed::makeHeightmapEvent (osc, heightmapOut.data()));
}

// Round B (plan Decision 40): select Imported… (index 6 of the osc?Terrain choice list)
// inside one gesture; idempotent so a re-import records no undo step. The combo relay
// pushes valueChanged → the page's listener → refreshInert() un-greys Blur / Edge.
void OStrataAudioProcessorEditor::launchPngChooser (int osc, const juce::String& title,
                                                    std::function<void (OStrataAudioProcessorEditor&, const juce::File&)> onFile)
{
    juce::ignoreUnused (osc);   // the natives bind it into onFile; kept in the signature for the call-site read
    auto chooser = std::make_shared<juce::FileChooser> (
        title,
        juce::File::getSpecialLocation (juce::File::userPicturesDirectory),
        "*.png");

    juce::Component::SafePointer<OStrataAudioProcessorEditor> safeThis (this);
    chooser->launchAsync (juce::FileBrowserComponent::openMode
                        | juce::FileBrowserComponent::canSelectFiles,
        [safeThis, chooser, onFile] (const juce::FileChooser& fc) {
            if (safeThis == nullptr)
                return; // editor destroyed — `complete` is owned by the dead WebView, never call it
            onFile (*safeThis, fc.getResult());
        });
}

void OStrataAudioProcessorEditor::selectImportedTerrain (int osc)
{
    JUCE_ASSERT_MESSAGE_THREAD
    auto* p = processorRef.getAPVTS().getParameter (juce::String ("osc") + (osc == 0 ? "A" : "B") + "Terrain");
    if (p == nullptr)
        return;
    const float target = p->convertTo0to1 (static_cast<float> (TerrainKind::Imported));
    if (std::abs (p->getValue() - target) < 1.0e-6f)
        return;
    p->beginChangeGesture();
    p->setValueNotifyingHost (target);
    p->endChangeGesture();
}

// Round B (plan Decision 37): PERF-03 rows — "<ISO time> <json>" appended to
// ~/Library/Logs/O-Strata/view-perf.log (created lazily on the first report).
void OStrataAudioProcessorEditor::logViewPerf (const juce::String& json)
{
    DBG ("[view-perf] " + json);
    if (perfLog == nullptr)
    {
        auto dir = juce::File::getSpecialLocation (juce::File::userHomeDirectory).getChildFile ("Library/Logs/O-Strata");
        dir.createDirectory();
        perfLog = std::make_unique<juce::FileLogger> (dir.getChildFile ("view-perf.log"), "O-Strata view-perf (PERF-03)", 256 * 1024);
    }
    perfLog->logMessage (juce::Time::getCurrentTime().toISO8601 (true) + " " + json);
}
