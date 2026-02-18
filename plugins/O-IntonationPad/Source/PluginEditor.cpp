/*
  ==============================================================================

    O-IntonationPad - Editor Implementation
    Ouaricon Development
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginEditor.h"
#include "BinaryData.h"
#include "DSP/TuningEngine.h"
#include "DSP/ScaleGenerator.h"
#include "DSP/TuningExporter.h"
#include "DSP/EmbeddedTunings.h"

OIntonationPadAudioProcessorEditor::OIntonationPadAudioProcessorEditor(OIntonationPadAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    // 1. Create relays FIRST (with parameter IDs matching HTML)
    voiceCountRelay = std::make_unique<juce::WebSliderRelay>("voiceCount");
    complexityRelay = std::make_unique<juce::WebSliderRelay>("complexity");
    keyRootRelay = std::make_unique<juce::WebSliderRelay>("keyRoot");
    keyScaleRelay = std::make_unique<juce::WebSliderRelay>("keyScale");
    inversionRandomRelay = std::make_unique<juce::WebSliderRelay>("inversionRandom");
    wavetablePosRelay = std::make_unique<juce::WebSliderRelay>("wavetablePos");
    lfoRateRelay = std::make_unique<juce::WebSliderRelay>("lfoRate");
    lfoDepthRelay = std::make_unique<juce::WebSliderRelay>("lfoDepth");
    timingRandomRelay = std::make_unique<juce::WebSliderRelay>("timingRandom");
    detuneRandomRelay = std::make_unique<juce::WebSliderRelay>("detuneRandom");
    attackTimeRelay = std::make_unique<juce::WebSliderRelay>("attackTime");
    releaseTimeRelay = std::make_unique<juce::WebSliderRelay>("releaseTime");
    filterCutoffRelay = std::make_unique<juce::WebSliderRelay>("filterCutoff");
    masterVolumeRelay = std::make_unique<juce::WebSliderRelay>("masterVolume");

    // v1.3.0: Tuning relays
    tuningMasterTuneRelay = std::make_unique<juce::WebSliderRelay>("tuning_masterTune");
    tuningOctaveStretchRelay = std::make_unique<juce::WebSliderRelay>("tuning_octaveStretch");
    tuningPitchBendRangeRelay = std::make_unique<juce::WebSliderRelay>("tuning_pitchBendRange");
    tuningTemperamentPresetRelay = std::make_unique<juce::WebComboBoxRelay>("tuning_temperamentPreset");

    // 2. Create WebView SECOND with all relay options + native functions
    webView = std::make_unique<juce::WebBrowserComponent>(
        juce::WebBrowserComponent::Options{}
            .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
            .withWinWebView2Options(
                juce::WebBrowserComponent::Options::WinWebView2{}
                    .withUserDataFolder(juce::File::getSpecialLocation(
                        juce::File::SpecialLocationType::tempDirectory)))
            .withNativeIntegrationEnabled()
            .withResourceProvider([this](auto& url) { return getResource(url); })
            .withOptionsFrom(*voiceCountRelay)
            .withOptionsFrom(*complexityRelay)
            .withOptionsFrom(*keyRootRelay)
            .withOptionsFrom(*keyScaleRelay)
            .withOptionsFrom(*inversionRandomRelay)
            .withOptionsFrom(*wavetablePosRelay)
            .withOptionsFrom(*lfoRateRelay)
            .withOptionsFrom(*lfoDepthRelay)
            .withOptionsFrom(*timingRandomRelay)
            .withOptionsFrom(*detuneRandomRelay)
            .withOptionsFrom(*attackTimeRelay)
            .withOptionsFrom(*releaseTimeRelay)
            .withOptionsFrom(*filterCutoffRelay)
            .withOptionsFrom(*masterVolumeRelay)
            // v1.3.0: Tuning relays
            .withOptionsFrom(*tuningMasterTuneRelay)
            .withOptionsFrom(*tuningOctaveStretchRelay)
            .withOptionsFrom(*tuningPitchBendRangeRelay)
            .withOptionsFrom(*tuningTemperamentPresetRelay)

            // ═══════════════════════════════════════════════════════════════════
            // v1.3.0: TUNING NATIVE FUNCTIONS (24 functions)
            // ═══════════════════════════════════════════════════════════════════

            // --- Tuning Intervals ---
            .withNativeFunction("getTuningIntervals", [this](const juce::Array<juce::var>&, auto complete) {
                auto intervals = processorRef.getTuningEngine().getIntervals();
                juce::String json = "[";
                for (size_t i = 0; i < intervals.size(); ++i) {
                    if (i > 0) json += ",";
                    json += juce::String(intervals[i], 6);
                }
                json += "]";
                complete(json);
            })

            .withNativeFunction("setTuningIntervals", [this](const juce::Array<juce::var>& args, auto complete) {
                if (args.size() >= 1) {
                    auto jsonArray = juce::JSON::parse(args[0].toString());
                    if (auto* arr = jsonArray.getArray()) {
                        std::vector<double> intervals;
                        for (const auto& val : *arr)
                            intervals.push_back(static_cast<double>(val));
                        processorRef.getTuningEngine().setCustomIntervals(intervals, "Custom");
                        complete(true);
                        return;
                    }
                }
                complete(false);
            })

            .withNativeFunction("getTuningName", [this](const juce::Array<juce::var>&, auto complete) {
                complete(processorRef.getTuningEngine().getActiveTuningName());
            })

            .withNativeFunction("setSingleInterval", [this](const juce::Array<juce::var>& args, auto complete) {
                if (args.size() >= 2) {
                    int index = static_cast<int>(args[0]);
                    double cents = static_cast<double>(args[1]);
                    processorRef.getTuningEngine().setSingleInterval(index, cents);
                    complete(true);
                    return;
                }
                complete(false);
            })

            .withNativeFunction("setSingleIntervalEncoded", [this](const juce::Array<juce::var>& args, auto complete) {
                if (args.size() >= 2) {
                    int index = static_cast<int>(args[0]);
                    double cents = static_cast<double>(args[1]);
                    processorRef.getTuningEngine().setSingleInterval(index, cents);
                    complete(true);
                    return;
                }
                complete(false);
            })

            // --- Tonic / Rotation ---
            .withNativeFunction("setTonicNote", [this](const juce::Array<juce::var>& args, auto complete) {
                if (args.size() >= 1) {
                    int tonic = static_cast<int>(args[0]);
                    processorRef.getTuningEngine().setTonicNote(tonic);
                    complete(true);
                    return;
                }
                complete(false);
            })

            .withNativeFunction("getTonicNote", [this](const juce::Array<juce::var>&, auto complete) {
                complete(processorRef.getTuningEngine().getTonicNote());
            })

            // --- Octave Stretch ---
            .withNativeFunction("getOctaveStretch", [this](const juce::Array<juce::var>&, auto complete) {
                complete(processorRef.getTuningEngine().getOctaveStretch());
            })

            .withNativeFunction("setOctaveStretch", [this](const juce::Array<juce::var>& args, auto complete) {
                if (args.size() >= 1) {
                    float stretch = static_cast<float>(args[0]);
                    processorRef.getTuningEngine().setOctaveStretch(stretch);
                    complete(true);
                    return;
                }
                complete(false);
            })

            // --- Master Tune ---
            .withNativeFunction("getMasterTune", [this](const juce::Array<juce::var>&, auto complete) {
                complete(processorRef.getTuningEngine().getMasterTune());
            })

            .withNativeFunction("setMasterTune", [this](const juce::Array<juce::var>& args, auto complete) {
                if (args.size() >= 1) {
                    double hz = static_cast<double>(args[0]);
                    processorRef.getTuningEngine().setMasterTune(hz);
                    complete(true);
                    return;
                }
                complete(false);
            })

            // --- Temperament Presets ---
            .withNativeFunction("setTemperamentPreset", [this](const juce::Array<juce::var>& args, auto complete) {
                if (args.size() >= 1) {
                    int preset = static_cast<int>(args[0]);
                    processorRef.getTuningEngine().setBuiltInPreset(
                        static_cast<TuningEngine::BuiltInPreset>(preset));
                    complete(true);
                    return;
                }
                complete(false);
            })

            .withNativeFunction("getTemperamentPreset", [this](const juce::Array<juce::var>&, auto complete) {
                complete(static_cast<int>(processorRef.getTuningEngine().getBuiltInPreset()));
            })

            // --- Scala File I/O ---
            .withNativeFunction("loadScalaFile", [this](const juce::Array<juce::var>&, auto complete) {
                tuningFileChooser = std::make_shared<juce::FileChooser>(
                    "Load Scala File",
                    juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
                    "*.scl");
                tuningFileChooser->launchAsync(
                    juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                    [this, complete](const juce::FileChooser& fc) {
                        auto file = fc.getResult();
                        if (file.existsAsFile()) {
                            bool success = processorRef.getTuningEngine().loadScalaFile(file);
                            complete(success ? juce::var(processorRef.getTuningEngine().getActiveTuningName())
                                            : juce::var());
                        } else {
                            complete(juce::var());
                        }
                    });
            })

            .withNativeFunction("saveScalaFile", [this](const juce::Array<juce::var>&, auto complete) {
                tuningFileChooser = std::make_shared<juce::FileChooser>(
                    "Save Scala File",
                    juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                        .getChildFile("scale.scl"),
                    "*.scl");
                tuningFileChooser->launchAsync(
                    juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                    [this, complete](const juce::FileChooser& fc) {
                        auto file = fc.getResult();
                        if (file != juce::File()) {
                            auto content = processorRef.getTuningEngine().generateScalaFileContent();
                            file.replaceWithText(content);
                            complete(true);
                        } else {
                            complete(false);
                        }
                    });
            })

            .withNativeFunction("loadKBMFile", [this](const juce::Array<juce::var>&, auto complete) {
                tuningFileChooser = std::make_shared<juce::FileChooser>(
                    "Load Keyboard Mapping",
                    juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
                    "*.kbm");
                tuningFileChooser->launchAsync(
                    juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                    [this, complete](const juce::FileChooser& fc) {
                        auto file = fc.getResult();
                        if (file.existsAsFile()) {
                            bool success = processorRef.getTuningEngine().loadKBMFile(file);
                            complete(success);
                        } else {
                            complete(false);
                        }
                    });
            })

            .withNativeFunction("saveKBMFile", [this](const juce::Array<juce::var>&, auto complete) {
                tuningFileChooser = std::make_shared<juce::FileChooser>(
                    "Save Keyboard Mapping",
                    juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                        .getChildFile("mapping.kbm"),
                    "*.kbm");
                tuningFileChooser->launchAsync(
                    juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                    [this, complete](const juce::FileChooser& fc) {
                        auto file = fc.getResult();
                        if (file != juce::File()) {
                            auto content = processorRef.getTuningEngine().generateKBMFileContent();
                            file.replaceWithText(content);
                            complete(true);
                        } else {
                            complete(false);
                        }
                    });
            })

            // --- Scale Generator ---
            .withNativeFunction("generateEDO", [](const juce::Array<juce::var>& args, auto complete) {
                if (args.size() >= 2) {
                    int divisions = static_cast<int>(args[0]);
                    double period = static_cast<double>(args[1]);
                    auto intervals = ScaleGenerator::generateEDO(divisions, period);
                    juce::String json = "[";
                    for (size_t i = 0; i < intervals.size(); ++i) {
                        if (i > 0) json += ",";
                        json += juce::String(intervals[i], 6);
                    }
                    json += "]";
                    complete(json);
                    return;
                }
                complete(juce::var());
            })

            .withNativeFunction("generateHarmonicSeries", [](const juce::Array<juce::var>& args, auto complete) {
                if (args.size() >= 2) {
                    int startHarmonic = static_cast<int>(args[0]);
                    int endHarmonic = static_cast<int>(args[1]);
                    auto intervals = ScaleGenerator::generateHarmonicSeries(startHarmonic, endHarmonic);
                    juce::String json = "[";
                    for (size_t i = 0; i < intervals.size(); ++i) {
                        if (i > 0) json += ",";
                        json += juce::String(intervals[i], 6);
                    }
                    json += "]";
                    complete(json);
                    return;
                }
                complete(juce::var());
            })

            .withNativeFunction("generateRank2", [](const juce::Array<juce::var>& args, auto complete) {
                if (args.size() >= 3) {
                    double generator = static_cast<double>(args[0]);
                    double period = static_cast<double>(args[1]);
                    int count = static_cast<int>(args[2]);
                    auto intervals = ScaleGenerator::generateRank2(generator, period, count);
                    juce::String json = "[";
                    for (size_t i = 0; i < intervals.size(); ++i) {
                        if (i > 0) json += ",";
                        json += juce::String(intervals[i], 6);
                    }
                    json += "]";
                    complete(json);
                    return;
                }
                complete(juce::var());
            })

            .withNativeFunction("applyGeneratedScale", [this](const juce::Array<juce::var>& args, auto complete) {
                if (args.size() >= 2) {
                    auto jsonArray = juce::JSON::parse(args[0].toString());
                    juce::String scaleName = args[1].toString();
                    if (auto* arr = jsonArray.getArray()) {
                        std::vector<double> intervals;
                        for (const auto& val : *arr)
                            intervals.push_back(static_cast<double>(val));
                        processorRef.getTuningEngine().setCustomIntervals(intervals, scaleName);
                        complete(true);
                        return;
                    }
                }
                complete(false);
            })

            // --- Embedded Tuning Library ---
            .withNativeFunction("getEmbeddedTuningList", [](const juce::Array<juce::var>&, auto complete) {
                const auto& tunings = EmbeddedTunings::getAllTunings();
                juce::String json = "[";
                for (size_t i = 0; i < tunings.size(); ++i) {
                    if (i > 0) json += ",";
                    json += "{";
                    json += "\"id\":\"" + juce::String(tunings[i].id) + "\",";
                    json += "\"name\":\"" + juce::String(tunings[i].name) + "\",";
                    json += "\"category\":\"" + juce::String(tunings[i].category) + "\",";
                    json += "\"noteCount\":" + juce::String(static_cast<int>(tunings[i].intervals.size()));
                    json += "}";
                }
                json += "]";
                complete(json);
            })

            .withNativeFunction("getEmbeddedTuningCategories", [](const juce::Array<juce::var>&, auto complete) {
                auto categories = EmbeddedTunings::getCategories();
                juce::String json = "[";
                for (size_t i = 0; i < categories.size(); ++i) {
                    if (i > 0) json += ",";
                    json += "\"" + juce::String(categories[i]) + "\"";
                }
                json += "]";
                complete(json);
            })

            .withNativeFunction("loadEmbeddedTuning", [this](const juce::Array<juce::var>& args, auto complete) {
                if (args.size() >= 1) {
                    juce::String tuningId = args[0].toString();
                    auto* tuning = EmbeddedTunings::getTuningById(tuningId.toStdString());
                    if (tuning != nullptr && !tuning->intervals.empty()) {
                        auto intervals = tuning->intervals;
                        intervals.push_back(tuning->period);
                        processorRef.getTuningEngine().setCustomIntervals(
                            intervals, juce::String(tuning->name));
                        complete(true);
                        return;
                    }
                }
                complete(false);
            })

            // --- HTML Export ---
            .withNativeFunction("exportTuningHTML", [this](const juce::Array<juce::var>&, auto complete) {
                tuningFileChooser = std::make_shared<juce::FileChooser>(
                    "Export Tuning Documentation",
                    juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                        .getChildFile("tuning-export.html"),
                    "*.html");
                tuningFileChooser->launchAsync(
                    juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                    [this, complete](const juce::FileChooser& fc) {
                        auto file = fc.getResult();
                        if (file != juce::File()) {
                            auto html = TuningExporter::toHTML(processorRef.getTuningEngine(), "O-IntonationPad");
                            file.replaceWithText(html);
                            complete(true);
                        } else {
                            complete(false);
                        }
                    });
            })
    );

    // 3. Create attachments LAST
    auto& apvts = processorRef.getAPVTS();

    voiceCountAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("voiceCount"), *voiceCountRelay, nullptr);
    complexityAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("complexity"), *complexityRelay, nullptr);
    keyRootAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("keyRoot"), *keyRootRelay, nullptr);
    keyScaleAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("keyScale"), *keyScaleRelay, nullptr);
    inversionRandomAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("inversionRandom"), *inversionRandomRelay, nullptr);
    wavetablePosAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("wavetablePos"), *wavetablePosRelay, nullptr);
    lfoRateAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("lfoRate"), *lfoRateRelay, nullptr);
    lfoDepthAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("lfoDepth"), *lfoDepthRelay, nullptr);
    timingRandomAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("timingRandom"), *timingRandomRelay, nullptr);
    detuneRandomAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("detuneRandom"), *detuneRandomRelay, nullptr);
    attackTimeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("attackTime"), *attackTimeRelay, nullptr);
    releaseTimeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("releaseTime"), *releaseTimeRelay, nullptr);
    filterCutoffAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("filterCutoff"), *filterCutoffRelay, nullptr);
    masterVolumeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("masterVolume"), *masterVolumeRelay, nullptr);

    // v1.3.0: Tuning attachments
    tuningMasterTuneAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("tuning_masterTune"), *tuningMasterTuneRelay, nullptr);
    tuningOctaveStretchAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("tuning_octaveStretch"), *tuningOctaveStretchRelay, nullptr);
    tuningPitchBendRangeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("tuning_pitchBendRange"), *tuningPitchBendRangeRelay, nullptr);
    tuningTemperamentPresetAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *apvts.getParameter("tuning_temperamentPreset"), *tuningTemperamentPresetRelay, nullptr);

    // Add WebView (navigation happens in parentHierarchyChanged)
    addAndMakeVisible(*webView);

    // Set size AFTER all components are created
    setSize(800, 500);

    // Start timer for active note visualization (30 fps)
    startTimerHz(30);
}

OIntonationPadAudioProcessorEditor::~OIntonationPadAudioProcessorEditor()
{
    stopTimer();
}

void OIntonationPadAudioProcessorEditor::timerCallback()
{
    if (webView == nullptr)
        return;

    auto notes = processorRef.getActiveNotes();

    // Build JSON array of active notes
    juce::String json = "[";
    for (int i = 0; i < static_cast<int>(notes.size()); ++i)
    {
        if (i > 0) json += ",";

        const auto& n = notes[static_cast<size_t>(i)];
        int pitchClass = n.midiNote % 12;
        int octave = (n.midiNote / 12) - 1;

        // Calculate cent deviation from 12-TET
        double tetFreq = 440.0 * std::pow(2.0, (n.midiNote - 69) / 12.0);
        double centDev = 1200.0 * std::log2(static_cast<double>(n.frequencyHz) / tetFreq);

        json += "{\"midi\":" + juce::String(n.midiNote)
             + ",\"pc\":" + juce::String(pitchClass)
             + ",\"oct\":" + juce::String(octave)
             + ",\"hz\":" + juce::String(n.frequencyHz, 2)
             + ",\"cents\":" + juce::String(centDev, 1)
             + ",\"gain\":" + juce::String(n.gain, 3) + "}";
    }
    json += "]";

    webView->emitEventIfBrowserIsVisible("activeNotes", json);
}

void OIntonationPadAudioProcessorEditor::paint(juce::Graphics& g)
{
    // WebView handles all painting
    juce::ignoreUnused(g);
}

void OIntonationPadAudioProcessorEditor::resized()
{
    // WebView fills entire editor
    webView->setBounds(getLocalBounds());
}

void OIntonationPadAudioProcessorEditor::parentHierarchyChanged()
{
    // Navigate WebView only after editor is attached to a window
    if (isShowing() && webView != nullptr && !hasNavigated)
    {
        webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
        hasNavigated = true;
    }
}

// Pattern #8: EXPLICIT URL MAPPING
std::optional<juce::WebBrowserComponent::Resource>
OIntonationPadAudioProcessorEditor::getResource(const juce::String& url)
{
    auto makeVector = [](const char* data, int size) {
        return std::vector<std::byte>(
            reinterpret_cast<const std::byte*>(data),
            reinterpret_cast<const std::byte*>(data) + size
        );
    };

    // Root "/" -> index.html
    if (url == "/" || url == "/index.html") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::index_html, BinaryData::index_htmlSize),
            juce::String("text/html")
        };
    }

    // JUCE JavaScript bridge
    if (url == "/js/juce/index.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::index_js, BinaryData::index_jsSize),
            juce::String("text/javascript")
        };
    }

    // JUCE interop checker
    if (url == "/js/juce/check_native_interop.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::check_native_interop_js,
                      BinaryData::check_native_interop_jsSize),
            juce::String("text/javascript")
        };
    }

    // Pitch circle module (still used by voice tab)
    if (url == "/modules/pitch-circle.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::pitchcircle_js, BinaryData::pitchcircle_jsSize),
            juce::String("text/javascript")
        };
    }

    // v1.3.0: Tuning panel JS module
    if (url == "/js/tuning-panel.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::tuningpanel_js, BinaryData::tuningpanel_jsSize),
            juce::String("text/javascript")
        };
    }

    // v1.3.0: Tuning panel CSS
    if (url == "/css/tuning-panel.css") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::tuningpanel_css, BinaryData::tuningpanel_cssSize),
            juce::String("text/css")
        };
    }

    // Background image
    if (url == "/img/paper.jpg") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::paper_jpg, BinaryData::paper_jpgSize),
            juce::String("image/jpeg")
        };
    }

    // Shell botanical overlay
    if (url == "/img/shell.png") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::shell_png, BinaryData::shell_pngSize),
            juce::String("image/png")
        };
    }

    // Resource not found
    juce::Logger::writeToLog("Resource not found: " + url);
    return std::nullopt;
}
