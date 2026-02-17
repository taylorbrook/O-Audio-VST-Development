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
    const auto urlToRetrieve = url.fromFirstOccurrenceOf ("://", false, false)
                                  .fromFirstOccurrenceOf ("/", true, false);

    if (urlToRetrieve == "/" || urlToRetrieve == "/index.html")
        return makeBinaryResource (BinaryData::index_html,
                                   BinaryData::index_htmlSize, "text/html");

    if (urlToRetrieve == "/js/juce/index.js")
        return makeBinaryResource (BinaryData::index_js,
                                   BinaryData::index_jsSize, "application/javascript");

    if (urlToRetrieve == "/js/juce/check_native_interop.js")
        return makeBinaryResource (BinaryData::check_native_interop_js,
                                   BinaryData::check_native_interop_jsSize, "application/javascript");

    return std::nullopt;
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
            juce::String json = "[";
            for (size_t i = 0; i < intervals.size(); ++i)
            {
                if (i > 0) json += ",";
                json += juce::String (intervals[i], 6);
            }
            json += "]";
            complete (json);
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
                processorRef.getTuningEngine()->setTonicNote (static_cast<int> (args[0]));
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
                processorRef.getTuningEngine()->setBuiltInPreset (
                    static_cast<TuningEngine::BuiltInPreset> (static_cast<int> (args[0])));
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

    // Scale generators
    options = options.withNativeFunction ("generateEDO",
        [this] (const juce::Array<juce::var>& args, auto complete) {
            if (args.size() >= 2)
            {
                auto intervals = ScaleGenerator::generateEDO (
                    static_cast<int> (args[0]), static_cast<double> (args[1]));
                juce::String json = "[";
                for (size_t i = 0; i < intervals.size(); ++i)
                {
                    if (i > 0) json += ",";
                    json += juce::String (intervals[i], 6);
                }
                json += "]";
                complete (json);
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
                juce::String json = "[";
                for (size_t i = 0; i < intervals.size(); ++i)
                {
                    if (i > 0) json += ",";
                    json += juce::String (intervals[i], 6);
                }
                json += "]";
                complete (json);
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
                juce::String json = "[";
                for (size_t i = 0; i < intervals.size(); ++i)
                {
                    if (i > 0) json += ",";
                    json += juce::String (intervals[i], 6);
                }
                json += "]";
                complete (json);
                return;
            }
            complete (juce::var());
        });

    // Embedded tuning library (API fixes from RESEARCH.md applied)
    options = options.withNativeFunction ("getEmbeddedTuningList",
        [this] (const juce::Array<juce::var>&, auto complete) {
            const auto& tunings = EmbeddedTunings::getAllTunings();
            juce::String json = "[";
            for (size_t i = 0; i < tunings.size(); ++i)
            {
                if (i > 0) json += ",";
                json += "{\"id\":\"" + juce::String (tunings[i].id)
                      + "\",\"name\":\"" + juce::String (tunings[i].name)
                      + "\",\"category\":\"" + juce::String (tunings[i].category)
                      + "\",\"noteCount\":" + juce::String (static_cast<int> (tunings[i].intervals.size()))
                      + "}";
            }
            json += "]";
            complete (json);
        });

    options = options.withNativeFunction ("getEmbeddedTuningCategories",
        [this] (const juce::Array<juce::var>&, auto complete) {
            auto categories = EmbeddedTunings::getCategories();
            juce::String json = "[";
            for (size_t i = 0; i < categories.size(); ++i)
            {
                if (i > 0) json += ",";
                json += "\"" + juce::String (categories[i]) + "\"";
            }
            json += "]";
            complete (json);
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

    // 67 slider relays
    for (int i = 0; i < numSliderParams; ++i)
        sliderRelays.push_back (std::make_unique<juce::WebSliderRelay> (sliderParamIds[i]));

    // 1 toggle relay (delaySync)
    delaySyncRelay = std::make_unique<juce::WebToggleButtonRelay> ("delaySync");

    // ─────────────────────────────────────────────────────────────
    // Step 2: Build WebView options with relays + native functions
    // ─────────────────────────────────────────────────────────────

    juce::WebBrowserComponent::Options options;
    options = options.withNativeIntegrationEnabled();
    options = options.withResourceProvider ([this] (const auto& url) { return getResource (url); });

    // Add all slider relays to options
    for (const auto& relay : sliderRelays)
        options = options.withOptionsFrom (*relay);

    // Add toggle relay
    options = options.withOptionsFrom (*delaySyncRelay);

    // Add native tuning functions
    options = addNativeFunctions (options);

    // Windows WebView2 user data folder
   #if JUCE_WINDOWS
    options = options.withWinWebView2Options (
        juce::WebBrowserComponent::Options::WinWebView2{}
            .withUserDataFolder (
                juce::File::getSpecialLocation (juce::File::tempDirectory)
                    .getChildFile ("OPrism_WebView")));
   #endif

    // Construct WebView
    webView = std::make_unique<juce::WebBrowserComponent> (options);
    addAndMakeVisible (*webView);

    // ─────────────────────────────────────────────────────────────
    // Step 3: Create attachments (after WebView)
    // ─────────────────────────────────────────────────────────────

    // 67 slider attachments
    for (int i = 0; i < numSliderParams; ++i)
    {
        auto* param = processorRef.getAPVTS().getParameter (sliderParamIds[i]);
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

    // Navigate to index.html
    webView->goToURL (juce::WebBrowserComponent::getResourceProviderRoot());

    setSize (1200, 800);
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
