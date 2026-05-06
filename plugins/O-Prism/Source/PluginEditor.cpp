/*
  ==============================================================================

    PluginEditor.cpp
    O-Prism - Microtonal Wavetable Synthesizer
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
OPrismAudioProcessorEditor::getResource (const juce::String& url)
{
    if (url == "/" || url == "/index.html")
        return makeBinaryResource (BinaryData::index_html,
                                   BinaryData::index_htmlSize, "text/html");

    if (url == "/js/juce/index.js")
        return makeBinaryResource (BinaryData::index_js,
                                   BinaryData::index_jsSize, "application/javascript");

    if (url == "/js/juce/check_native_interop.js")
        return makeBinaryResource (BinaryData::check_native_interop_js,
                                   BinaryData::check_native_interop_jsSize, "application/javascript");

    if (url == "/js/wavetable-editor.js")
        return makeBinaryResource (BinaryData::wavetableeditor_js,
                                   BinaryData::wavetableeditor_jsSize, "application/javascript");

    if (url == "/css/wavetable-editor.css")
        return makeBinaryResource (BinaryData::wavetableeditor_css,
                                   BinaryData::wavetableeditor_cssSize, "text/css");

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
        json += juce::String (data[i], decimals);
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
            static_cast<float> (PrismParamIds::kCustomTuningPresetIndex)));
}

// ═══════════════════════════════════════════════════════════════════
// Native Functions
// ═══════════════════════════════════════════════════════════════════

juce::WebBrowserComponent::Options
OPrismAudioProcessorEditor::addNativeFunctions (juce::WebBrowserComponent::Options options)
{
    // Tuning intervals
    options = options.withNativeFunction ("getTuningIntervals",
        [this] (const juce::Array<juce::var>&, auto complete) {
            auto intervals = processorRef.getTuningEngine()->getIntervals();
            complete (toJsonArray (intervals, [] (double v) { return juce::String (v, 6); }));
        });

    options = options.withNativeFunction ("setTuningIntervals",
        [this] (const juce::Array<juce::var>& args, auto complete) {
            if (args.size() >= 1)
            {
                auto jsonArray = juce::JSON::parse (args[0].toString());
                if (auto* arr = jsonArray.getArray())
                {
                    std::vector<double> intervals;
                    for (const auto& val : *arr)
                        intervals.push_back (static_cast<double> (val));
                    processorRef.getTuningEngine()->setCustomIntervals (intervals, "Custom");
                    syncTuningPresetToCustom (processorRef.getAPVTS());
                    complete (true);
                    return;
                }
            }
            complete (false);
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

    // Master tune
    options = options.withNativeFunction ("getMasterTune",
        [this] (const juce::Array<juce::var>&, auto complete) {
            complete (processorRef.getTuningEngine()->getMasterTune());
        });

    options = options.withNativeFunction ("setMasterTune",
        [this] (const juce::Array<juce::var>& args, auto complete) {
            if (args.size() >= 1)
            {
                processorRef.getTuningEngine()->setMasterTune (static_cast<double> (args[0]));
                complete (true);
                return;
            }
            complete (false);
        });

    // Octave stretch
    options = options.withNativeFunction ("getOctaveStretch",
        [this] (const juce::Array<juce::var>&, auto complete) {
            complete (processorRef.getTuningEngine()->getOctaveStretch());
        });

    options = options.withNativeFunction ("setOctaveStretch",
        [this] (const juce::Array<juce::var>& args, auto complete) {
            if (args.size() >= 1)
            {
                processorRef.getTuningEngine()->setOctaveStretch (static_cast<float> (args[0]));
                complete (true);
                return;
            }
            complete (false);
        });

    // Temperament presets
    options = options.withNativeFunction ("setTemperamentPreset",
        [this] (const juce::Array<juce::var>& args, auto complete) {
            if (args.size() >= 1)
            {
                int presetIndex = static_cast<int> (args[0]);
                processorRef.getTuningEngine()->setBuiltInPreset (
                    static_cast<TuningEngine::BuiltInPreset> (presetIndex));
                // Sync APVTS so the preset persists across DAW save/load
                if (auto* param = processorRef.getAPVTS().getParameter ("tuningPreset"))
                    param->setValueNotifyingHost (param->convertTo0to1 (static_cast<float> (presetIndex)));
                complete (true);
                return;
            }
            complete (false);
        });

    options = options.withNativeFunction ("getTemperamentPreset",
        [this] (const juce::Array<juce::var>&, auto complete) {
            complete (static_cast<int> (processorRef.getTuningEngine()->getBuiltInPreset()));
        });

    // Scala file I/O
    options = options.withNativeFunction ("loadScalaFile",
        [this] (const juce::Array<juce::var>&, auto complete) {
            auto chooser = std::make_shared<juce::FileChooser> (
                "Load Scala File",
                juce::File::getSpecialLocation (juce::File::userDocumentsDirectory),
                "*.scl");

            chooser->launchAsync (juce::FileBrowserComponent::openMode
                                | juce::FileBrowserComponent::canSelectFiles,
                [this, chooser, complete] (const juce::FileChooser& fc) {
                    auto file = fc.getResult();
                    if (file.existsAsFile())
                    {
                        bool success = processorRef.getTuningEngine()->loadScalaFile (file);
                        if (success)
                        {
                            syncTuningPresetToCustom (processorRef.getAPVTS());
                        }
                        complete (success ? juce::var (processorRef.getTuningEngine()->getActiveTuningName())
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

            chooser->launchAsync (juce::FileBrowserComponent::openMode
                                | juce::FileBrowserComponent::canSelectFiles,
                [this, chooser, complete] (const juce::FileChooser& fc) {
                    auto file = fc.getResult();
                    if (file.existsAsFile())
                        complete (processorRef.getTuningEngine()->loadKBMFile (file));
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

            chooser->launchAsync (juce::FileBrowserComponent::saveMode
                                | juce::FileBrowserComponent::canSelectFiles,
                [this, chooser, complete] (const juce::FileChooser& fc) {
                    auto file = fc.getResult();
                    if (file != juce::File())
                    {
                        auto content = processorRef.getTuningEngine()->generateScalaFileContent();
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

            chooser->launchAsync (juce::FileBrowserComponent::saveMode
                                | juce::FileBrowserComponent::canSelectFiles,
                [this, chooser, complete] (const juce::FileChooser& fc) {
                    auto file = fc.getResult();
                    if (file != juce::File())
                    {
                        auto content = processorRef.getTuningEngine()->generateKBMFileContent();
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
                     + "}";
            }));
        });

    options = options.withNativeFunction ("getEmbeddedTuningCategories",
        [this] (const juce::Array<juce::var>&, auto complete) {
            auto categories = EmbeddedTunings::getCategories();
            complete (toJsonArray (categories, [] (const auto& s) {
                return "\"" + juce::String (s) + "\"";
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
                    processorRef.getTuningEngine()->setCustomIntervals (intervals, "Generated");
                    syncTuningPresetToCustom (processorRef.getAPVTS());
                    complete (true);
                    return;
                }
            }
            complete (false);
        });

    // Wavetable display native functions
    options = options.withNativeFunction ("getWavetableFrame",
        [this] (const juce::Array<juce::var>& args, auto complete) {
            if (args.size() >= 2)
            {
                int oscId = static_cast<int> (args[0]);
                int frameIndex = static_cast<int> (args[1]);
                auto* table = processorRef.getFactoryTable (oscId);
                if (table != nullptr && frameIndex >= 0 && frameIndex < table->numFrames)
                {
                    const float* frameData = table->getFrameData (0, frameIndex);
                    complete (toJsonFloatArray (frameData, WavetableData::kTableSize, 8, 4));
                    return;
                }
            }
            complete (juce::var());
        });

    options = options.withNativeFunction ("getWavetableInfo",
        [this] (const juce::Array<juce::var>& args, auto complete) {
            if (args.size() >= 1)
            {
                int oscId = static_cast<int> (args[0]);
                auto* table = processorRef.getFactoryTable (oscId);
                if (table != nullptr)
                {
                    juce::String name = processorRef.getTableName (oscId);
                    juce::String category = processorRef.getTableCategory (oscId);
                    juce::String json = "{\"numFrames\":" + juce::String (table->numFrames)
                                      + ",\"shapeName\":\"" + name + "\""
                                      + ",\"category\":\"" + category + "\""
                                      + ",\"numTables\":" + juce::String (processorRef.getNumFactoryTables()) + "}";
                    complete (json);
                    return;
                }
            }
            complete (juce::var());
        });

    options = options.withNativeFunction ("getWavetableFrameForPosition",
        [this] (const juce::Array<juce::var>& args, auto complete) {
            if (args.size() >= 2)
            {
                int oscId = static_cast<int> (args[0]);
                float normalizedPos = static_cast<float> (args[1]);
                auto* table = processorRef.getFactoryTable (oscId);
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

    // HTML export (API fix: toHTML not generateHTML)
    options = options.withNativeFunction ("exportTuningHTML",
        [this] (const juce::Array<juce::var>&, auto complete) {
            auto chooser = std::make_shared<juce::FileChooser> (
                "Export Tuning Documentation",
                juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                    .getChildFile ("tuning-export.html"),
                "*.html");

            chooser->launchAsync (juce::FileBrowserComponent::saveMode
                                | juce::FileBrowserComponent::canSelectFiles,
                [this, chooser, complete] (const juce::FileChooser& fc) {
                    auto file = fc.getResult();
                    if (file != juce::File())
                    {
                        auto html = TuningExporter::toHTML (
                            *processorRef.getTuningEngine(), "O-Prism");
                        file.replaceWithText (html);
                        complete (true);
                    }
                    else
                    {
                        complete (false);
                    }
                });
        });

    // ─── User Wavetable Native Functions ───

    // Get list of user wavetable names
    options = options.withNativeFunction ("getUserWavetableList",
        [this] (const juce::Array<juce::var>&, auto complete) {
            auto names = processorRef.getUserWavetableManager().getTableNames();
            complete (toJsonArray (names, [] (const juce::String& s) {
                return juce::JSON::toString (s);
            }));
        });

    // Import wavetable from file chooser
    options = options.withNativeFunction ("importUserWavetable",
        [this] (const juce::Array<juce::var>& args, auto complete) {
            int oscIndex = args.size() >= 1 ? static_cast<int> (args[0]) : 0;

            auto chooser = std::make_shared<juce::FileChooser> (
                "Import Wavetable",
                juce::File::getSpecialLocation (juce::File::userDocumentsDirectory),
                "*.wav;*.aiff;*.flac");

            chooser->launchAsync (juce::FileBrowserComponent::openMode
                                | juce::FileBrowserComponent::canSelectFiles,
                [this, chooser, complete, oscIndex] (const juce::FileChooser& fc) {
                    auto file = fc.getResult();
                    if (file.existsAsFile())
                    {
                        auto name = processorRef.getUserWavetableManager().importFile (file);
                        if (name.isNotEmpty())
                        {
                            processorRef.selectUserWavetable (oscIndex, name);
                            complete ("{\"success\":true,\"name\":" + juce::JSON::toString (name) + "}");
                            return;
                        }
                    }
                    complete ("{\"success\":false}");
                });
        });

    // Import wavetable from base64 data (drag-and-drop from WebView)
    options = options.withNativeFunction ("importUserWavetableData",
        [this] (const juce::Array<juce::var>& args, auto complete) {
            if (args.size() < 3)
            {
                complete ("{\"success\":false,\"error\":\"Missing arguments\"}");
                return;
            }

            int oscIndex = static_cast<int> (args[0]);
            auto base64Data = args[1].toString();
            auto filename = args[2].toString();

            juce::MemoryOutputStream decoded;
            if (! juce::Base64::convertFromBase64 (decoded, base64Data))
            {
                complete ("{\"success\":false,\"error\":\"Invalid base64 data\"}");
                return;
            }

            auto name = processorRef.getUserWavetableManager().importFromMemory (
                decoded.getData(), decoded.getDataSize(), filename);

            if (name.isNotEmpty())
            {
                processorRef.selectUserWavetable (oscIndex, name);
                complete ("{\"success\":true,\"name\":" + juce::JSON::toString (name) + "}");
            }
            else
            {
                complete ("{\"success\":false,\"error\":\"Import failed\"}");
            }
        });

    // Select user wavetable for oscillator
    options = options.withNativeFunction ("selectUserWavetable",
        [this] (const juce::Array<juce::var>& args, auto complete) {
            if (args.size() >= 2)
            {
                int oscIndex = static_cast<int> (args[0]);
                auto name = args[1].toString();
                processorRef.selectUserWavetable (oscIndex, name);
                complete (true);
                return;
            }
            complete (false);
        });

    // Clear user wavetable override (revert to factory)
    options = options.withNativeFunction ("clearUserWavetableOverride",
        [this] (const juce::Array<juce::var>& args, auto complete) {
            if (args.size() >= 1)
            {
                int oscIndex = static_cast<int> (args[0]);
                processorRef.clearUserWavetableOverride (oscIndex);
                complete (true);
                return;
            }
            complete (false);
        });

    // Delete user wavetable
    options = options.withNativeFunction ("deleteUserWavetable",
        [this] (const juce::Array<juce::var>& args, auto complete) {
            if (args.size() >= 1)
            {
                auto name = args[0].toString();
                // Clear override if this table is active
                if (processorRef.getActiveUserTableName (0) == name)
                    processorRef.clearUserWavetableOverride (0);
                if (processorRef.getActiveUserTableName (1) == name)
                    processorRef.clearUserWavetableOverride (1);
                complete (processorRef.getUserWavetableManager().deleteWavetable (name));
                return;
            }
            complete (false);
        });

    // Get active osc info (factory or user)
    options = options.withNativeFunction ("getActiveOscInfo",
        [this] (const juce::Array<juce::var>& args, auto complete) {
            if (args.size() >= 1)
            {
                int oscIndex = static_cast<int> (args[0]);
                bool isUser = processorRef.isUserTableActive (oscIndex);
                if (isUser)
                {
                    auto name = processorRef.getActiveUserTableName (oscIndex);
                    auto* table = processorRef.getActiveOscTable (oscIndex);
                    int numFrames = table ? table->numFrames : 0;
                    complete ("{\"isUser\":true,\"name\":" + juce::JSON::toString (name)
                            + ",\"numFrames\":" + juce::String (numFrames) + "}");
                }
                else
                {
                    auto paramId = oscIndex == 0 ? "oscATable" : "oscBTable";
                    int factoryIdx = static_cast<int> (processorRef.getAPVTS().getRawParameterValue (paramId)->load());
                    factoryIdx = juce::jlimit (0, processorRef.getNumFactoryTables() - 1, factoryIdx);
                    auto name = processorRef.getTableName (factoryIdx);
                    auto* table = processorRef.getFactoryTable (factoryIdx);
                    int numFrames = table ? table->numFrames : 0;
                    complete ("{\"isUser\":false,\"factoryIndex\":" + juce::String (factoryIdx)
                            + ",\"name\":\"" + name + "\",\"numFrames\":" + juce::String (numFrames) + "}");
                }
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

    // ─── Wavetable Editor Native Functions ───

    options = options.withNativeFunction ("startWavetableEditor",
        [this] (const juce::Array<juce::var>& args, auto complete) {
            int oscIndex = args.size() >= 1 ? static_cast<int> (args[0]) : 0;
            processorRef.startEditing (oscIndex);
            auto& editor = processorRef.getWavetableEditor();
            if (editor.hasWorkingTable())
            {
                int numFrames = editor.getNumFrames();
                auto harmonics = editor.getFrameHarmonics (0, 128);
                juce::String harmJson = "[";
                for (size_t i = 0; i < harmonics.size(); ++i)
                {
                    if (i > 0) harmJson += ",";
                    harmJson += juce::String (harmonics[i], 4);
                }
                harmJson += "]";
                complete ("{\"numFrames\":" + juce::String (numFrames)
                        + ",\"harmonics\":" + harmJson + "}");
            }
            else
            {
                complete ("{\"error\":\"Failed to load table\"}");
            }
        });

    options = options.withNativeFunction ("stopWavetableEditor",
        [this] (const juce::Array<juce::var>&, auto complete) {
            int oscIdx = processorRef.getEditingOscIndex();
            if (oscIdx >= 0)
                processorRef.stopEditing (oscIdx);
            complete (true);
        });

    options = options.withNativeFunction ("getEditorFrameWaveform",
        [this] (const juce::Array<juce::var>& args, auto complete) {
            if (args.size() >= 1)
            {
                int frameIndex = static_cast<int> (args[0]);
                auto& editor = processorRef.getWavetableEditor();
                auto waveform = editor.getFrameWaveform (frameIndex);
                if (! waveform.empty())
                {
                    // Stride by 8 for ~256 display points
                    complete (toJsonFloatArray (waveform.data(),
                        static_cast<int> (waveform.size()), 8, 4));
                    return;
                }
            }
            complete (juce::var());
        });

    options = options.withNativeFunction ("getFrameHarmonics",
        [this] (const juce::Array<juce::var>& args, auto complete) {
            if (args.size() >= 2)
            {
                int frameIndex = static_cast<int> (args[0]);
                int numBins = static_cast<int> (args[1]);
                auto harmonics = processorRef.getWavetableEditor()
                    .getFrameHarmonics (frameIndex, numBins);
                if (! harmonics.empty())
                {
                    juce::String json = "[";
                    for (size_t i = 0; i < harmonics.size(); ++i)
                    {
                        if (i > 0) json += ",";
                        json += juce::String (harmonics[i], 4);
                    }
                    json += "]";
                    complete (json);
                    return;
                }
            }
            complete (juce::var());
        });

    options = options.withNativeFunction ("setFrameHarmonics",
        [this] (const juce::Array<juce::var>& args, auto complete) {
            if (args.size() >= 2)
            {
                int frameIndex = static_cast<int> (args[0]);
                auto jsonArray = juce::JSON::parse (args[1].toString());
                if (auto* arr = jsonArray.getArray())
                {
                    std::vector<float> magnitudes;
                    magnitudes.reserve (static_cast<size_t> (arr->size()));
                    for (const auto& val : *arr)
                        magnitudes.push_back (static_cast<float> (val));

                    processorRef.getWavetableEditor()
                        .setFrameHarmonics (frameIndex, magnitudes);

                    // Return updated waveform for display
                    auto waveform = processorRef.getWavetableEditor()
                        .getFrameWaveform (frameIndex);
                    if (! waveform.empty())
                    {
                        complete (toJsonFloatArray (waveform.data(),
                            static_cast<int> (waveform.size()), 8, 4));
                        return;
                    }
                }
            }
            complete (juce::var());
        });

    options = options.withNativeFunction ("applyFrameOperation",
        [this] (const juce::Array<juce::var>& args, auto complete) {
            if (args.size() >= 2)
            {
                auto opType = args[0].toString();
                auto framesJson = juce::JSON::parse (args[1].toString());

                std::vector<int> frameIndices;
                if (auto* arr = framesJson.getArray())
                    for (const auto& val : *arr)
                        frameIndices.push_back (static_cast<int> (val));

                auto& editor = processorRef.getWavetableEditor();

                if (opType == "normalize")
                    editor.normalizeFrames (frameIndices, true);
                else if (opType == "normalizeGlobal")
                    editor.normalizeFrames (frameIndices, false);
                else if (opType == "fade")
                {
                    float pct = args.size() >= 3 ? static_cast<float> (args[2]) : 10.0f;
                    editor.fadeEdges (frameIndices, pct);
                }
                else if (opType == "reverse")
                    editor.reverseFrames (frameIndices);
                else if (opType == "reverseOrder")
                    editor.reverseOrder (frameIndices);
                else if (opType == "smooth")
                {
                    float strength = args.size() >= 3 ? static_cast<float> (args[2]) : 0.5f;
                    editor.smoothFrames (frameIndices, strength);
                }

                complete (true);
                return;
            }
            complete (false);
        });

    options = options.withNativeFunction ("saveEditedWavetable",
        [this] (const juce::Array<juce::var>& args, auto complete) {
            if (args.size() >= 1)
            {
                auto name = args[0].toString();
                auto& editor = processorRef.getWavetableEditor();
                bool success = editor.saveAsUserWavetable (
                    name, processorRef.getUserWavetableManager());
                if (success)
                {
                    complete ("{\"success\":true,\"name\":" + juce::JSON::toString (name) + "}");
                    return;
                }
            }
            complete ("{\"success\":false}");
        });

    options = options.withNativeFunction ("getAllEditorFrameWaveforms",
        [this] (const juce::Array<juce::var>& args, auto complete) {
            int samplesPerFrame = args.size() >= 1 ? static_cast<int> (args[0]) : 64;
            auto& editor = processorRef.getWavetableEditor();
            auto allWaveforms = editor.getAllFrameWaveforms (samplesPerFrame);

            juce::String json = "[";
            for (size_t f = 0; f < allWaveforms.size(); ++f)
            {
                if (f > 0) json += ",";
                json += "[";
                for (size_t s = 0; s < allWaveforms[f].size(); ++s)
                {
                    if (s > 0) json += ",";
                    json += juce::String (allWaveforms[f][s], 3);
                }
                json += "]";
            }
            json += "]";

            complete (json);
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
                json += juce::JSON::toString (cat) + ":[";
                for (int i = 0; i < names.size(); ++i)
                {
                    if (i > 0) json += ",";
                    json += juce::JSON::toString (names[i]);
                }
                json += "]";
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

    options = options.withNativeFunction ("deletePreset",
        [this] (const juce::Array<juce::var>& args, auto complete) {
            if (args.size() >= 1)
            {
                complete (processorRef.getPresetManager().deletePreset (args[0].toString()));
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

OPrismAudioProcessorEditor::OPrismAudioProcessorEditor (OPrismAudioProcessor& p)
    : AudioProcessorEditor (p),
      processorRef (p)
{
    // ─────────────────────────────────────────────────────────────
    // Step 1: Create relays (before WebView)
    // ─────────────────────────────────────────────────────────────

    // Get parameter ID lists from shared definitions (PrismParamIds.h)
    auto sliderIds    = PrismParamIds::allSliderIds();
    auto bypassIds    = PrismParamIds::bypassToggleIds();
    auto modToggleIds = PrismParamIds::modSlotToggleIds();

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
                    .getChildFile ("OPrism_WebView")));
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

    // Start polling for active MIDI notes (60 Hz is plenty for UI updates)
    startTimerHz (30);
}

OPrismAudioProcessorEditor::~OPrismAudioProcessorEditor()
{
    stopTimer();
}

// ═══════════════════════════════════════════════════════════════════
// Paint / Resized
// ═══════════════════════════════════════════════════════════════════

void OPrismAudioProcessorEditor::paint (juce::Graphics&)
{
    // WebView fills entire editor — no custom painting needed
}

void OPrismAudioProcessorEditor::resized()
{
    if (webView != nullptr)
        webView->setBounds (getLocalBounds());
}

// ═══════════════════════════════════════════════════════════════════
// Timer: Push active MIDI notes to WebView for TrueKeys
// ═══════════════════════════════════════════════════════════════════

void OPrismAudioProcessorEditor::timerCallback()
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
