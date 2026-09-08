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
    O-Strata - Microtonal Wavetable Synthesizer
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

static juce::String toJsonFloatArray (const float* data, int count, int stride, int decimals)
{
    juce::String json = "[";
    for (int i = 0; i < count; i += stride)
    {
        if (i > 0) json += ",";
        // IN-16: juce::String would emit "nan"/"inf" — invalid JSON that makes
        // JS JSON.parse throw and silently freezes the waveform display
        const float v = std::isfinite (data[i]) ? data[i] : 0.0f;
        json += juce::String (v, decimals);
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

    // ─── Oscillator table info (Stage 1: the sine placeholder; Phase 2.1 reports the
    //     baked table). Kept JSON shape so the Stage 3 ≋ view can consume it. ───
    options = options.withNativeFunction ("getActiveOscInfo",
        [this] (const juce::Array<juce::var>& args, auto complete) {
            if (args.size() >= 1)
            {
                int oscIndex = static_cast<int> (args[0]);
                auto* table = processorRef.getActiveOscTable (oscIndex);
                complete ("{\"isUser\":false,\"factoryIndex\":0,\"name\":\"Sine\",\"numFrames\":"
                        + juce::String (table ? table->numFrames : 0) + "}");
                return;
            }
            complete (juce::var());
        });

    // Get frame data from the currently active table for an oscillator
    options = options.withNativeFunction ("getActiveOscFrame",
        [this] (const juce::Array<juce::var>& args, auto complete) {
            if (args.size() >= 2)
            {
                int oscIndex = static_cast<int> (args[0]);
                float normalizedPos = static_cast<float> (args[1]);
                auto* table = processorRef.getActiveOscTable (oscIndex);
                if (table != nullptr && table->numFrames > 0)
                {
                    int frameIndex = juce::jlimit (0, table->numFrames - 1,
                        static_cast<int> (normalizedPos * (table->numFrames - 1)));
                    const float* frameData = table->getFrameData (0, frameIndex);
                    complete (toJsonFloatArray (frameData, WavetableData::kTableSize, 8, 4));
                    return;
                }
            }
            complete (juce::var());
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
                complete (ok);
                return;
            }
            complete (false);
        });

    options = options.withNativeFunction ("loadPresetByName",
        [this] (const juce::Array<juce::var>& args, auto complete) {
            if (args.size() >= 1)
            {
                complete (processorRef.getPresetManager().loadPreset (args[0].toString()));
                return;
            }
            complete (false);
        });

    options = options.withNativeFunction ("selectNextPreset",
        [this] (const juce::Array<juce::var>&, auto complete) {
            complete (processorRef.getPresetManager().getNextPreset());
        });

    options = options.withNativeFunction ("selectPreviousPreset",
        [this] (const juce::Array<juce::var>&, auto complete) {
            complete (processorRef.getPresetManager().getPreviousPreset());
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
    // and createParameterLayout() must agree — 188 in Stage 1).
    jassert (sliderAttachments.size() == sliderRelays.size());

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
// Timer: Push active MIDI notes to WebView for TrueKeys
// ═══════════════════════════════════════════════════════════════════

void OStrataAudioProcessorEditor::timerCallback()
{
    if (webView == nullptr)
        return;

    auto currentNotes = processorRef.getActiveNotes();

    // Only send update if notes changed
    if (currentNotes == lastSentNotes)
        return;

    lastSentNotes = currentNotes;

    // Build JS call: window.updateHeldNotes([midi1,midi2,...], [freq1,freq2,...])
    auto noteArray = toJsonArray (currentNotes, [] (const auto& n) { return juce::String (n.first); });
    auto freqArray = toJsonArray (currentNotes, [] (const auto& n) { return juce::String (n.second, 4); });

    juce::String js = "if(window.updateHeldNotes) window.updateHeldNotes(" + noteArray + "," + freqArray + ");";
    webView->evaluateJavascript (js, nullptr);
}
