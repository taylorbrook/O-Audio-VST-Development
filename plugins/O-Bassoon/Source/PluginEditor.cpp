/*
  ==============================================================================

    O-Bassoon - Editor Implementation (Stage 3 WebView UI)
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginEditor.h"
#include "BinaryData.h"
#include "TuningEngine.h"
#include "ScaleGenerator.h"
#include "EmbeddedTunings.h"
#include "TuningExporter.h"

OBassoonAudioProcessorEditor::OBassoonAudioProcessorEditor (OBassoonAudioProcessor& p)
    : juce::AudioProcessorEditor (&p), processorRef (p)
{
    // ===================================================================
    // 1. CREATE RELAYS (must be created BEFORE WebView so .withOptionsFrom() works)
    // Param IDs MUST match APVTS layout in PluginProcessor.cpp createParameterLayout.
    // ===================================================================
    vibratoRateRelay     = std::make_unique<juce::WebSliderRelay> ("vibrato_rate");
    vibratoDepthRelay    = std::make_unique<juce::WebSliderRelay> ("vibrato_depth");
    vibratoOnsetRelay    = std::make_unique<juce::WebSliderRelay> ("vibrato_onset");
    breathRelay          = std::make_unique<juce::WebSliderRelay> ("breath");
    toneRelay            = std::make_unique<juce::WebSliderRelay> ("tone");
    attackCharacterRelay = std::make_unique<juce::WebSliderRelay> ("attack_character");
    attackTimeRelay      = std::make_unique<juce::WebSliderRelay> ("attack_time");
    releaseTimeRelay     = std::make_unique<juce::WebSliderRelay> ("release_time");
    voiceCountRelay      = std::make_unique<juce::WebSliderRelay> ("voice_count");      // AudioParameterInt -> still slider
    outputGainRelay      = std::make_unique<juce::WebSliderRelay> ("output_gain");

    // ===================================================================
    // 2. CREATE WEBVIEW with options chain (relays + tuning native fns + resource provider)
    // ===================================================================
    webView = std::make_unique<juce::WebBrowserComponent> (
        juce::WebBrowserComponent::Options{}
            .withBackend (juce::WebBrowserComponent::Options::Backend::webview2)
            .withWinWebView2Options (
                juce::WebBrowserComponent::Options::WinWebView2{}
                    .withUserDataFolder (juce::File::getSpecialLocation (
                        juce::File::SpecialLocationType::tempDirectory)
                            .getChildFile ("OBassoon_WebView"))
                    .withStatusBarDisabled()
                    .withBuiltInErrorPageDisabled())
            .withNativeIntegrationEnabled()
            .withKeepPageLoadedWhenBrowserIsHidden()
            .withResourceProvider ([this] (const juce::String& url) {
                return getResource (url);
            })
            // ── Attach 10 WebSliderRelay options ──
            .withOptionsFrom (*vibratoRateRelay)
            .withOptionsFrom (*vibratoDepthRelay)
            .withOptionsFrom (*vibratoOnsetRelay)
            .withOptionsFrom (*breathRelay)
            .withOptionsFrom (*toneRelay)
            .withOptionsFrom (*attackCharacterRelay)
            .withOptionsFrom (*attackTimeRelay)
            .withOptionsFrom (*releaseTimeRelay)
            .withOptionsFrom (*voiceCountRelay)
            .withOptionsFrom (*outputGainRelay)

            // =========================================================
            // TUNING NATIVE FUNCTIONS (lifted from O-Wind PluginEditor.cpp:237-481)
            // =========================================================

            .withNativeFunction ("getTuningIntervals",
                [this] (const juce::Array<juce::var>&, auto complete) {
                    auto intervals = processorRef.getTuningEngine()->getIntervals();
                    juce::String json = "[";
                    for (size_t i = 0; i < intervals.size(); ++i) {
                        if (i > 0) json += ",";
                        json += juce::String (intervals[i], 6);
                    }
                    json += "]";
                    complete (json);
                })

            .withNativeFunction ("setTuningIntervals",
                [this] (const juce::Array<juce::var>& args, auto complete) {
                    if (args.size() >= 1) {
                        auto jsonArray = juce::JSON::parse (args[0].toString());
                        if (auto* arr = jsonArray.getArray()) {
                            std::vector<double> intervals;
                            for (const auto& val : *arr)
                                intervals.push_back (static_cast<double> (val));
                            processorRef.getTuningEngine()->setCustomIntervals (intervals, "Custom");
                            complete (true);
                            return;
                        }
                    }
                    complete (false);
                })

            .withNativeFunction ("getTuningName",
                [this] (const juce::Array<juce::var>&, auto complete) {
                    complete (processorRef.getTuningEngine()->getActiveTuningName());
                })

            .withNativeFunction ("setSingleInterval",
                [this] (const juce::Array<juce::var>& args, auto complete) {
                    if (args.size() >= 2) {
                        int index    = static_cast<int> (args[0]);
                        double cents = static_cast<double> (args[1]);
                        processorRef.getTuningEngine()->setSingleInterval (index, cents);
                        complete (true);
                        return;
                    }
                    complete (false);
                })

            .withNativeFunction ("setTonicNote",
                [this] (const juce::Array<juce::var>& args, auto complete) {
                    if (args.size() >= 1) {
                        processorRef.getTuningEngine()->setTonicNote (static_cast<int> (args[0]));
                        complete (true);
                        return;
                    }
                    complete (false);
                })

            .withNativeFunction ("getTonicNote",
                [this] (const juce::Array<juce::var>&, auto complete) {
                    complete (processorRef.getTuningEngine()->getTonicNote());
                })

            .withNativeFunction ("getOctaveStretch",
                [this] (const juce::Array<juce::var>&, auto complete) {
                    complete (processorRef.getTuningEngine()->getOctaveStretch());
                })

            .withNativeFunction ("setOctaveStretch",
                [this] (const juce::Array<juce::var>& args, auto complete) {
                    if (args.size() >= 1) {
                        processorRef.getTuningEngine()->setOctaveStretch (static_cast<float> (args[0]));
                        complete (true);
                        return;
                    }
                    complete (false);
                })

            .withNativeFunction ("getMasterTune",
                [this] (const juce::Array<juce::var>&, auto complete) {
                    complete (processorRef.getTuningEngine()->getMasterTune());
                })

            .withNativeFunction ("setMasterTune",
                [this] (const juce::Array<juce::var>& args, auto complete) {
                    if (args.size() >= 1) {
                        processorRef.getTuningEngine()->setMasterTune (static_cast<double> (args[0]));
                        complete (true);
                        return;
                    }
                    complete (false);
                })

            .withNativeFunction ("setTemperamentPreset",
                [this] (const juce::Array<juce::var>& args, auto complete) {
                    if (args.size() >= 1) {
                        int preset = static_cast<int> (args[0]);
                        processorRef.getTuningEngine()->setBuiltInPreset (
                            static_cast<TuningEngine::BuiltInPreset> (preset));
                        complete (true);
                        return;
                    }
                    complete (false);
                })

            .withNativeFunction ("getTemperamentPreset",
                [this] (const juce::Array<juce::var>&, auto complete) {
                    complete (static_cast<int> (processorRef.getTuningEngine()->getBuiltInPreset()));
                })

            .withNativeFunction ("loadScalaFile",
                [this] (const juce::Array<juce::var>&, auto complete) {
                    fileChooser = std::make_shared<juce::FileChooser> (
                        "Load Scala File",
                        juce::File::getSpecialLocation (juce::File::userDocumentsDirectory),
                        "*.scl;*.tun");
                    fileChooser->launchAsync (
                        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                        [this, complete] (const juce::FileChooser& fc) {
                            auto file = fc.getResult();
                            if (file.existsAsFile()) {
                                bool success = processorRef.getTuningEngine()->loadScalaFile (file);
                                complete (success);
                            } else {
                                complete (false);
                            }
                        });
                })

            .withNativeFunction ("loadKBMFile",
                [this] (const juce::Array<juce::var>&, auto complete) {
                    fileChooser = std::make_shared<juce::FileChooser> (
                        "Load Keyboard Mapping",
                        juce::File::getSpecialLocation (juce::File::userDocumentsDirectory),
                        "*.kbm");
                    fileChooser->launchAsync (
                        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                        [this, complete] (const juce::FileChooser& fc) {
                            auto file = fc.getResult();
                            if (file.existsAsFile()) {
                                bool success = processorRef.getTuningEngine()->loadKBMFile (file);
                                complete (success);
                            } else {
                                complete (false);
                            }
                        });
                })

            .withNativeFunction ("generateEDO",
                [] (const juce::Array<juce::var>& args, auto complete) {
                    if (args.size() >= 2) {
                        int divisions = static_cast<int> (args[0]);
                        double period = static_cast<double> (args[1]);
                        auto intervals = ScaleGenerator::generateEDO (divisions, period);
                        juce::String json = "[";
                        for (size_t i = 0; i < intervals.size(); ++i) {
                            if (i > 0) json += ",";
                            json += juce::String (intervals[i], 6);
                        }
                        json += "]";
                        complete (json);
                        return;
                    }
                    complete (juce::var());
                })

            .withNativeFunction ("generateHarmonicSeries",
                [] (const juce::Array<juce::var>& args, auto complete) {
                    if (args.size() >= 2) {
                        int startHarmonic = static_cast<int> (args[0]);
                        int endHarmonic   = static_cast<int> (args[1]);
                        auto intervals = ScaleGenerator::generateHarmonicSeries (startHarmonic, endHarmonic);
                        juce::String json = "[";
                        for (size_t i = 0; i < intervals.size(); ++i) {
                            if (i > 0) json += ",";
                            json += juce::String (intervals[i], 6);
                        }
                        json += "]";
                        complete (json);
                        return;
                    }
                    complete (juce::var());
                })

            .withNativeFunction ("applyGeneratedScale",
                [this] (const juce::Array<juce::var>& args, auto complete) {
                    if (args.size() >= 2) {
                        auto jsonArray         = juce::JSON::parse (args[0].toString());
                        juce::String scaleName = args[1].toString();
                        if (auto* arr = jsonArray.getArray()) {
                            std::vector<double> intervals;
                            for (const auto& val : *arr)
                                intervals.push_back (static_cast<double> (val));
                            processorRef.getTuningEngine()->setCustomIntervals (intervals, scaleName);
                            complete (true);
                            return;
                        }
                    }
                    complete (false);
                })

            .withNativeFunction ("getEmbeddedTuningList",
                [] (const juce::Array<juce::var>&, auto complete) {
                    const auto& tunings = EmbeddedTunings::getAllTunings();
                    juce::String json = "[";
                    for (size_t i = 0; i < tunings.size(); ++i) {
                        if (i > 0) json += ",";
                        json += "{";
                        json += "\"id\":\"" + juce::String (tunings[i].id) + "\",";
                        json += "\"name\":\"" + juce::String (tunings[i].name) + "\",";
                        json += "\"category\":\"" + juce::String (tunings[i].category) + "\",";
                        json += "\"noteCount\":" + juce::String (static_cast<int> (tunings[i].intervals.size()));
                        json += "}";
                    }
                    json += "]";
                    complete (json);
                })

            .withNativeFunction ("getEmbeddedTuningCategories",
                [] (const juce::Array<juce::var>&, auto complete) {
                    auto categories = EmbeddedTunings::getCategories();
                    juce::String json = "[";
                    for (size_t i = 0; i < categories.size(); ++i) {
                        if (i > 0) json += ",";
                        json += "\"" + juce::String (categories[i]) + "\"";
                    }
                    json += "]";
                    complete (json);
                })

            .withNativeFunction ("loadEmbeddedTuning",
                [this] (const juce::Array<juce::var>& args, auto complete) {
                    if (args.size() >= 1) {
                        juce::String tuningId = args[0].toString();
                        auto* tuning = EmbeddedTunings::getTuningById (tuningId.toStdString());
                        if (tuning != nullptr && ! tuning->intervals.empty()) {
                            auto intervals = tuning->intervals;
                            intervals.push_back (tuning->period);
                            processorRef.getTuningEngine()->setCustomIntervals (
                                intervals, juce::String (tuning->name));
                            complete (true);
                            return;
                        }
                    }
                    complete (false);
                })

            .withNativeFunction ("exportTuningHTML",
                [this] (const juce::Array<juce::var>&,
                        std::function<void (juce::var)> complete) {
                    fileChooser = std::make_shared<juce::FileChooser> (
                        "Export Tuning Documentation",
                        juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                            .getChildFile ("tuning-export.html"),
                        "*.html");
                    fileChooser->launchAsync (
                        juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                        [this, complete] (const juce::FileChooser& fc) {
                            auto file = fc.getResult();
                            if (file != juce::File()) {
                                auto html = TuningExporter::toHTML (
                                    *processorRef.getTuningEngine(), "O-Bassoon");
                                file.replaceWithText (html);
                                complete (true);
                            } else {
                                complete (false);
                            }
                        });
                })
    );

    addAndMakeVisible (*webView);

    // ===================================================================
    // 3. CREATE ATTACHMENTS (must be created AFTER WebView)
    // 3-arg ctor with nullptr undoManager per juce8-critical-patterns.md #12.
    // ===================================================================
    auto& apvts = processorRef.getAPVTS();

    vibratoRateAttachment     = std::make_unique<juce::WebSliderParameterAttachment> (
        *apvts.getParameter ("vibrato_rate"),     *vibratoRateRelay,     nullptr);
    vibratoDepthAttachment    = std::make_unique<juce::WebSliderParameterAttachment> (
        *apvts.getParameter ("vibrato_depth"),    *vibratoDepthRelay,    nullptr);
    vibratoOnsetAttachment    = std::make_unique<juce::WebSliderParameterAttachment> (
        *apvts.getParameter ("vibrato_onset"),    *vibratoOnsetRelay,    nullptr);
    breathAttachment          = std::make_unique<juce::WebSliderParameterAttachment> (
        *apvts.getParameter ("breath"),           *breathRelay,          nullptr);
    toneAttachment            = std::make_unique<juce::WebSliderParameterAttachment> (
        *apvts.getParameter ("tone"),             *toneRelay,            nullptr);
    attackCharacterAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *apvts.getParameter ("attack_character"), *attackCharacterRelay, nullptr);
    attackTimeAttachment      = std::make_unique<juce::WebSliderParameterAttachment> (
        *apvts.getParameter ("attack_time"),      *attackTimeRelay,      nullptr);
    releaseTimeAttachment     = std::make_unique<juce::WebSliderParameterAttachment> (
        *apvts.getParameter ("release_time"),     *releaseTimeRelay,     nullptr);
    voiceCountAttachment      = std::make_unique<juce::WebSliderParameterAttachment> (
        *apvts.getParameter ("voice_count"),      *voiceCountRelay,      nullptr);
    outputGainAttachment      = std::make_unique<juce::WebSliderParameterAttachment> (
        *apvts.getParameter ("output_gain"),      *outputGainRelay,      nullptr);

    // ===================================================================
    // 4. Navigate + size + start Timer
    // ===================================================================
    webView->goToURL (juce::WebBrowserComponent::getResourceProviderRoot());

    setSize (900, 600);
    startTimerHz (30);   // Phase 3.2 push-channel polling rate
}

OBassoonAudioProcessorEditor::~OBassoonAudioProcessorEditor()
{
    // CRITICAL: stopTimer() BEFORE webView destruction so timerCallback can't fire mid-tear-down.
    stopTimer();

    // Explicit reset in reverse construction order — defensive, paired with header
    // declaration order so attachments outlive neither relays nor webView during teardown.
    outputGainAttachment.reset();
    voiceCountAttachment.reset();
    releaseTimeAttachment.reset();
    attackTimeAttachment.reset();
    attackCharacterAttachment.reset();
    toneAttachment.reset();
    breathAttachment.reset();
    vibratoOnsetAttachment.reset();
    vibratoDepthAttachment.reset();
    vibratoRateAttachment.reset();
    webView.reset();
    outputGainRelay.reset();
    voiceCountRelay.reset();
    releaseTimeRelay.reset();
    attackTimeRelay.reset();
    attackCharacterRelay.reset();
    toneRelay.reset();
    breathRelay.reset();
    vibratoOnsetRelay.reset();
    vibratoDepthRelay.reset();
    vibratoRateRelay.reset();
}

void OBassoonAudioProcessorEditor::paint (juce::Graphics& g)
{
    juce::ignoreUnused (g);   // WebView handles all painting
}

void OBassoonAudioProcessorEditor::resized()
{
    if (webView)
        webView->setBounds (getLocalBounds());
}

//==============================================================================
// Phase 3.2 — 30 Hz push channels (diff-suppressed)
//==============================================================================
void OBassoonAudioProcessorEditor::timerCallback()
{
    if (webView == nullptr) return;

    const int   active = processorRef.currentActiveVoiceCount.load (std::memory_order_relaxed);
    const float breath = processorRef.currentEffectiveBreath.load  (std::memory_order_relaxed);
    const float vibEnv = processorRef.currentVibratoEnvelope.load  (std::memory_order_relaxed);

    if (active != lastEmittedActive)
    {
        webView->emitEventIfBrowserIsVisible ("activeVoiceCount", juce::var (active));
        lastEmittedActive = active;
    }
    if (std::abs (breath - lastEmittedBreath) > 0.005f)
    {
        webView->emitEventIfBrowserIsVisible ("effectiveBreath", juce::var (breath));
        lastEmittedBreath = breath;
    }
    if (std::abs (vibEnv - lastEmittedVibEnv) > 0.005f)
    {
        webView->emitEventIfBrowserIsVisible ("vibratoEnvelope", juce::var (vibEnv));
        lastEmittedVibEnv = vibEnv;
    }
}

//==============================================================================
// Resource Provider — serves embedded UI files from BinaryData via bare-path equality.
// CRITICAL: the URL parameter is already a bare PATH (not a full URL). Never strip a
// scheme/host from it -- doing so returns empty string and triggers
// "Frame load interrupted" / blank page (memory-pinned regression sentinel).
//==============================================================================
std::optional<juce::WebBrowserComponent::Resource>
OBassoonAudioProcessorEditor::getResource (const juce::String& url)
{
    auto makeVector = [] (const char* data, int size) {
        return std::vector<std::byte> (
            reinterpret_cast<const std::byte*> (data),
            reinterpret_cast<const std::byte*> (data) + size);
    };

    // HTML
    if (url == "/" || url == "/index.html")
        return juce::WebBrowserComponent::Resource {
            makeVector (BinaryData::index_html, BinaryData::index_htmlSize),
            juce::String ("text/html") };

    // JUCE Bridge
    if (url == "/js/juce/index.js")
        return juce::WebBrowserComponent::Resource {
            makeVector (BinaryData::index_js, BinaryData::index_jsSize),
            juce::String ("application/javascript") };

    if (url == "/js/juce/check_native_interop.js")
        return juce::WebBrowserComponent::Resource {
            makeVector (BinaryData::check_native_interop_js, BinaryData::check_native_interop_jsSize),
            juce::String ("application/javascript") };

    // Tuning panel (Pattern A — shared module direct reference)
    if (url == "/js/tuning-panel.js")
        return juce::WebBrowserComponent::Resource {
            makeVector (BinaryData::tuningpanel_js, BinaryData::tuningpanel_jsSize),
            juce::String ("application/javascript") };

    if (url == "/css/tuning-panel.css")
        return juce::WebBrowserComponent::Resource {
            makeVector (BinaryData::tuningpanel_css, BinaryData::tuningpanel_cssSize),
            juce::String ("text/css") };

    // Botanical fern overlay
    if (url == "/img/fern.png")
        return juce::WebBrowserComponent::Resource {
            makeVector (BinaryData::fern_png, BinaryData::fern_pngSize),
            juce::String ("image/png") };

    DBG ("Resource not found: " + url);
    return std::nullopt;
}
