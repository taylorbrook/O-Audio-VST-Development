/*
  ==============================================================================

    OuariconLyrica - Editor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginEditor.h"
#include "BinaryData.h"

OuariconLyricaAudioProcessorEditor::OuariconLyricaAudioProcessorEditor(OuariconLyricaAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    // ═══════════════════════════════════════════════════════════════════
    // Phase 3.1: WebView Setup
    // ═══════════════════════════════════════════════════════════════════

    // 1️⃣ CREATE RELAYS (must be created BEFORE WebView)
    masterVolumeRelay = std::make_unique<juce::WebSliderRelay>("masterVolume");
    brightnessRelay = std::make_unique<juce::WebSliderRelay>("brightness");
    timbreRelay = std::make_unique<juce::WebSliderRelay>("timbre");           // v1.1.0: renamed from sustain
    decayTimeRelay = std::make_unique<juce::WebSliderRelay>("decayTime");     // v1.1.0: new parameter
    bodySizeRelay = std::make_unique<juce::WebSliderRelay>("bodySize");
    bodyResonanceRelay = std::make_unique<juce::WebSliderRelay>("bodyResonance");
    sympatheticAmountRelay = std::make_unique<juce::WebSliderRelay>("sympatheticAmount");
    pluckPositionRelay = std::make_unique<juce::WebSliderRelay>("pluckPosition");
    fingerHardnessRelay = std::make_unique<juce::WebSliderRelay>("fingerHardness");
    stringTensionRelay = std::make_unique<juce::WebSliderRelay>("stringTension");
    stringGaugeRelay = std::make_unique<juce::WebSliderRelay>("stringGauge");
    stringLengthRelay = std::make_unique<juce::WebSliderRelay>("stringLength");
    stringStiffnessRelay = std::make_unique<juce::WebSliderRelay>("stringStiffness");
    masterTuneRelay = std::make_unique<juce::WebSliderRelay>("masterTune");
    pitchBendRangeRelay = std::make_unique<juce::WebSliderRelay>("pitchBendRange");
    // v1.4.0: New parameters from v1.3.0
    attackNoiseRelay = std::make_unique<juce::WebSliderRelay>("attackNoise");
    sympatheticQRelay = std::make_unique<juce::WebSliderRelay>("sympatheticQ");
    bodyModeSpreadRelay = std::make_unique<juce::WebSliderRelay>("bodyModeSpread");
    bridgeBrightnessRelay = std::make_unique<juce::WebSliderRelay>("bridgeBrightness");

    stringMaterialRelay = std::make_unique<juce::WebComboBoxRelay>("stringMaterial");
    woodTypeRelay = std::make_unique<juce::WebComboBoxRelay>("woodType");
    techniqueRelay = std::make_unique<juce::WebComboBoxRelay>("technique");
    glissandoModeRelay = std::make_unique<juce::WebComboBoxRelay>("glissandoMode");
    glissandoScaleRelay = std::make_unique<juce::WebComboBoxRelay>("glissandoScale");
    // v1.6.0: Tuning mode relay
    tuningModeRelay = std::make_unique<juce::WebComboBoxRelay>("tuningMode");

    // 2️⃣ CREATE WEBVIEW with all relays registered
    webView = std::make_unique<juce::WebBrowserComponent>(
        juce::WebBrowserComponent::Options{}
            .withNativeIntegrationEnabled()
            .withResourceProvider([this](const juce::String& url) {
                return getResource(url);
            })
            // Phase 3.3: Native function to get voice count
            // JUCE 8 async callback pattern: (args, complete) -> void
            .withNativeFunction("getVoiceCount", [this](const juce::Array<juce::var>&,
                                                         std::function<void(juce::var)> complete) {
                complete(juce::var(processorRef.getActiveVoiceCount()));
            })
            // v1.5.0: Preset Management Native Functions
            .withNativeFunction("savePreset", [this](const juce::Array<juce::var>& args,
                                                      std::function<void(juce::var)> complete) {
                if (args.isEmpty()) { complete(juce::var(false)); return; }
                auto name = args[0].toString();
                auto success = processorRef.getPresetManager().savePreset(name);
                complete(juce::var(success));
            })
            .withNativeFunction("loadPreset", [this](const juce::Array<juce::var>& args,
                                                      std::function<void(juce::var)> complete) {
                if (args.isEmpty()) { complete(juce::var(false)); return; }
                auto name = args[0].toString();
                auto success = processorRef.getPresetManager().loadPreset(name);
                complete(juce::var(success));
            })
            .withNativeFunction("getPresetList", [this](const juce::Array<juce::var>&,
                                                         std::function<void(juce::var)> complete) {
                auto presetList = processorRef.getPresetManager().getPresetList();
                juce::Array<juce::var> result;
                for (const auto& presetName : presetList)
                    result.add(juce::var(presetName));
                complete(juce::var(result));
            })
            .withNativeFunction("getCurrentPreset", [this](const juce::Array<juce::var>&,
                                                            std::function<void(juce::var)> complete) {
                complete(juce::var(processorRef.getPresetManager().getCurrentPresetName()));
            })
            .withNativeFunction("selectNextPreset", [this](const juce::Array<juce::var>&,
                                                            std::function<void(juce::var)> complete) {
                auto& pm = processorRef.getPresetManager();
                juce::String nextPreset = pm.getNextPreset();
                if (pm.loadPreset(nextPreset))
                {
                    complete(juce::var(nextPreset));
                }
                else
                {
                    complete(juce::var(pm.getCurrentPresetName()));
                }
            })
            .withNativeFunction("selectPreviousPreset", [this](const juce::Array<juce::var>&,
                                                                std::function<void(juce::var)> complete) {
                auto& pm = processorRef.getPresetManager();
                juce::String prevPreset = pm.getPreviousPreset();
                if (pm.loadPreset(prevPreset))
                {
                    complete(juce::var(prevPreset));
                }
                else
                {
                    complete(juce::var(pm.getCurrentPresetName()));
                }
            })
            // v1.5.1: File dialog functions for Save/Load buttons
            .withNativeFunction("savePresetWithDialog", [this](const juce::Array<juce::var>&,
                                                                std::function<void(juce::var)> complete) {
                auto& pm = processorRef.getPresetManager();
                auto userDir = pm.getUserPresetsDirectory();
                userDir.createDirectory();

                fileChooser = std::make_unique<juce::FileChooser>(
                    "Save Preset",
                    userDir,
                    "*.json"
                );

                fileChooser->launchAsync(
                    juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                    [this, complete](const juce::FileChooser& fc) {
                        auto result = fc.getResult();
                        if (result == juce::File{})
                        {
                            complete(juce::var()); // User cancelled
                            return;
                        }

                        auto presetName = result.getFileNameWithoutExtension();
                        if (processorRef.getPresetManager().savePreset(presetName))
                        {
                            complete(juce::var(presetName));
                        }
                        else
                        {
                            complete(juce::var()); // Save failed
                        }
                    }
                );
            })
            .withNativeFunction("loadPresetFromFile", [this](const juce::Array<juce::var>&,
                                                              std::function<void(juce::var)> complete) {
                auto& pm = processorRef.getPresetManager();
                auto presetsDir = pm.getPresetsDirectory();

                fileChooser = std::make_unique<juce::FileChooser>(
                    "Load Preset",
                    presetsDir,
                    "*.json"
                );

                fileChooser->launchAsync(
                    juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                    [this, complete](const juce::FileChooser& fc) {
                        auto result = fc.getResult();
                        if (result == juce::File{})
                        {
                            complete(juce::var()); // User cancelled
                            return;
                        }

                        if (processorRef.getPresetManager().loadPresetFromFile(result))
                        {
                            complete(juce::var(result.getFileNameWithoutExtension()));
                        }
                        else
                        {
                            complete(juce::var()); // Load failed
                        }
                    }
                );
            })
            // v1.6.0: Tuning Native Functions
            .withNativeFunction("getTuningIntervals", [this](const juce::Array<juce::var>&,
                                                              std::function<void(juce::var)> complete) {
                auto intervals = processorRef.getTuningEngine()->getIntervals();
                juce::Array<juce::var> result;
                for (double cents : intervals)
                    result.add(juce::var(cents));
                complete(juce::var(result));
            })
            .withNativeFunction("setTuningIntervals", [this](const juce::Array<juce::var>& args,
                                                              std::function<void(juce::var)> complete) {
                if (args.size() < 2) { complete(juce::var(false)); return; }

                auto intervalsVar = args[0];
                auto name = args[1].toString();

                if (!intervalsVar.isArray()) { complete(juce::var(false)); return; }

                std::vector<double> intervals;
                for (int i = 0; i < intervalsVar.size(); ++i)
                    intervals.push_back(static_cast<double>(intervalsVar[i]));

                processorRef.getTuningEngine()->setCustomIntervals(intervals, name);
                complete(juce::var(true));
            })
            .withNativeFunction("getTuningName", [this](const juce::Array<juce::var>&,
                                                         std::function<void(juce::var)> complete) {
                complete(juce::var(processorRef.getTuningEngine()->getActiveTuningName()));
            })
            .withNativeFunction("setTonicNote", [this](const juce::Array<juce::var>& args,
                                                        std::function<void(juce::var)> complete) {
                if (args.isEmpty()) { complete(juce::var(false)); return; }
                int tonic = static_cast<int>(args[0]);
                processorRef.getTuningEngine()->setTonicNote(tonic);
                complete(juce::var(true));
            })
            .withNativeFunction("getTonicNote", [this](const juce::Array<juce::var>&,
                                                        std::function<void(juce::var)> complete) {
                complete(juce::var(processorRef.getTuningEngine()->getTonicNote()));
            })
            .withNativeFunction("loadScalaFile", [this](const juce::Array<juce::var>&,
                                                         std::function<void(juce::var)> complete) {
                fileChooser = std::make_unique<juce::FileChooser>(
                    "Load Scala File",
                    juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
                    "*.scl"
                );

                fileChooser->launchAsync(
                    juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                    [this, complete](const juce::FileChooser& fc) {
                        auto result = fc.getResult();
                        if (result == juce::File{})
                        {
                            complete(juce::var()); // User cancelled
                            return;
                        }

                        if (processorRef.getTuningEngine()->loadScalaFile(result))
                        {
                            // v1.7.4: Also update APVTS parameter so processBlock uses Scala mode
                            // Without this, processBlock overwrites TuningEngine mode every block
                            if (auto* param = processorRef.getAPVTS().getParameter("tuningMode"))
                                param->setValueNotifyingHost(1.0f / 2.0f); // Index 1 = Custom/Scala (normalized: 1/2 = 0.5)

                            complete(juce::var(processorRef.getTuningEngine()->getActiveTuningName()));
                        }
                        else
                        {
                            complete(juce::var()); // Load failed
                        }
                    }
                );
            })
            .withNativeFunction("loadKBMFile", [this](const juce::Array<juce::var>&,
                                                       std::function<void(juce::var)> complete) {
                fileChooser = std::make_unique<juce::FileChooser>(
                    "Load Keyboard Mapping",
                    juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
                    "*.kbm"
                );

                fileChooser->launchAsync(
                    juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                    [this, complete](const juce::FileChooser& fc) {
                        auto result = fc.getResult();
                        if (result == juce::File{})
                        {
                            complete(juce::var()); // User cancelled
                            return;
                        }

                        if (processorRef.getTuningEngine()->loadKBMFile(result))
                        {
                            complete(juce::var(true));
                        }
                        else
                        {
                            complete(juce::var()); // Load failed
                        }
                    }
                );
            })
            .withNativeFunction("saveScalaFile", [this](const juce::Array<juce::var>&,
                                                         std::function<void(juce::var)> complete) {
                auto content = processorRef.getTuningEngine()->generateScalaFileContent();
                auto name = processorRef.getTuningEngine()->getActiveTuningName();

                fileChooser = std::make_unique<juce::FileChooser>(
                    "Save Scala File",
                    juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile(name + ".scl"),
                    "*.scl"
                );

                fileChooser->launchAsync(
                    juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                    [this, content, complete](const juce::FileChooser& fc) {
                        auto result = fc.getResult();
                        if (result == juce::File{})
                        {
                            complete(juce::var()); // User cancelled
                            return;
                        }

                        auto file = result.hasFileExtension(".scl") ? result : result.withFileExtension(".scl");
                        if (file.replaceWithText(content))
                        {
                            complete(juce::var(file.getFileName()));
                        }
                        else
                        {
                            complete(juce::var()); // Save failed
                        }
                    }
                );
            })
            .withNativeFunction("saveKBMFile", [this](const juce::Array<juce::var>&,
                                                       std::function<void(juce::var)> complete) {
                auto content = processorRef.getTuningEngine()->generateKBMFileContent();
                auto name = processorRef.getTuningEngine()->getActiveTuningName();

                fileChooser = std::make_unique<juce::FileChooser>(
                    "Save Keyboard Mapping",
                    juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile(name + ".kbm"),
                    "*.kbm"
                );

                fileChooser->launchAsync(
                    juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                    [this, content, complete](const juce::FileChooser& fc) {
                        auto result = fc.getResult();
                        if (result == juce::File{})
                        {
                            complete(juce::var()); // User cancelled
                            return;
                        }

                        auto file = result.hasFileExtension(".kbm") ? result : result.withFileExtension(".kbm");
                        if (file.replaceWithText(content))
                        {
                            complete(juce::var(file.getFileName()));
                        }
                        else
                        {
                            complete(juce::var()); // Save failed
                        }
                    }
                );
            })
            // v1.7.4: Note triggering for WebView keyboard visualization
            .withNativeFunction("triggerNoteOn", [this](const juce::Array<juce::var>& args,
                                                         std::function<void(juce::var)> complete) {
                if (args.size() < 2) { complete(juce::var(false)); return; }
                int midiNote = static_cast<int>(args[0]);
                float velocity = static_cast<float>(args[1]);
                processorRef.triggerNoteOn(midiNote, velocity);
                complete(juce::var(true));
            })
            .withNativeFunction("triggerNoteOff", [this](const juce::Array<juce::var>& args,
                                                          std::function<void(juce::var)> complete) {
                if (args.isEmpty()) { complete(juce::var(false)); return; }
                int midiNote = static_cast<int>(args[0]);
                processorRef.triggerNoteOff(midiNote);
                complete(juce::var(true));
            })
            // Register all slider relays
            .withOptionsFrom(*masterVolumeRelay)
            .withOptionsFrom(*brightnessRelay)
            .withOptionsFrom(*timbreRelay)
            .withOptionsFrom(*decayTimeRelay)
            .withOptionsFrom(*bodySizeRelay)
            .withOptionsFrom(*bodyResonanceRelay)
            .withOptionsFrom(*sympatheticAmountRelay)
            .withOptionsFrom(*pluckPositionRelay)
            .withOptionsFrom(*fingerHardnessRelay)
            .withOptionsFrom(*stringTensionRelay)
            .withOptionsFrom(*stringGaugeRelay)
            .withOptionsFrom(*stringLengthRelay)
            .withOptionsFrom(*stringStiffnessRelay)
            .withOptionsFrom(*masterTuneRelay)
            .withOptionsFrom(*pitchBendRangeRelay)
            // v1.4.0: New parameters from v1.3.0
            .withOptionsFrom(*attackNoiseRelay)
            .withOptionsFrom(*sympatheticQRelay)
            .withOptionsFrom(*bodyModeSpreadRelay)
            .withOptionsFrom(*bridgeBrightnessRelay)
            // Register all choice relays
            .withOptionsFrom(*stringMaterialRelay)
            .withOptionsFrom(*woodTypeRelay)
            .withOptionsFrom(*techniqueRelay)
            .withOptionsFrom(*glissandoModeRelay)
            .withOptionsFrom(*glissandoScaleRelay)
            // v1.6.0: Tuning mode relay
            .withOptionsFrom(*tuningModeRelay)
    );

    // 3️⃣ CREATE ATTACHMENTS (must be created AFTER WebView)
    // CRITICAL: JUCE 8 requires 3 parameters (parameter, relay, undoManager)
    auto& apvts = processorRef.getAPVTS();

    masterVolumeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("masterVolume"), *masterVolumeRelay, nullptr);
    brightnessAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("brightness"), *brightnessRelay, nullptr);
    timbreAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("timbre"), *timbreRelay, nullptr);
    decayTimeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("decayTime"), *decayTimeRelay, nullptr);
    bodySizeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("bodySize"), *bodySizeRelay, nullptr);
    bodyResonanceAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("bodyResonance"), *bodyResonanceRelay, nullptr);
    sympatheticAmountAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("sympatheticAmount"), *sympatheticAmountRelay, nullptr);
    pluckPositionAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("pluckPosition"), *pluckPositionRelay, nullptr);
    fingerHardnessAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("fingerHardness"), *fingerHardnessRelay, nullptr);
    stringTensionAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("stringTension"), *stringTensionRelay, nullptr);
    stringGaugeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("stringGauge"), *stringGaugeRelay, nullptr);
    stringLengthAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("stringLength"), *stringLengthRelay, nullptr);
    stringStiffnessAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("stringStiffness"), *stringStiffnessRelay, nullptr);
    masterTuneAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("masterTune"), *masterTuneRelay, nullptr);
    pitchBendRangeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("pitchBendRange"), *pitchBendRangeRelay, nullptr);
    // v1.4.0: New parameters from v1.3.0
    attackNoiseAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("attackNoise"), *attackNoiseRelay, nullptr);
    sympatheticQAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("sympatheticQ"), *sympatheticQRelay, nullptr);
    bodyModeSpreadAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("bodyModeSpread"), *bodyModeSpreadRelay, nullptr);
    bridgeBrightnessAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("bridgeBrightness"), *bridgeBrightnessRelay, nullptr);

    stringMaterialAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *apvts.getParameter("stringMaterial"), *stringMaterialRelay, nullptr);
    woodTypeAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *apvts.getParameter("woodType"), *woodTypeRelay, nullptr);
    techniqueAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *apvts.getParameter("technique"), *techniqueRelay, nullptr);
    glissandoModeAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *apvts.getParameter("glissandoMode"), *glissandoModeRelay, nullptr);
    glissandoScaleAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *apvts.getParameter("glissandoScale"), *glissandoScaleRelay, nullptr);
    // v1.6.0: Tuning mode attachment
    tuningModeAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *apvts.getParameter("tuningMode"), *tuningModeRelay, nullptr);

    // 4️⃣ SETUP WEBVIEW
    addAndMakeVisible(*webView);

    // Navigate to UI (uses resource provider)
    webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());

    // Set editor size - v1.4.0: Reduced from 800x600 to 700x450
    setSize(700, 450);

    // v1.7.9: Start timer for MIDI event polling (tuning circle visualization)
    startTimer(50);  // 20 Hz polling - fast enough for visual feedback
}

OuariconLyricaAudioProcessorEditor::~OuariconLyricaAudioProcessorEditor()
{
    // v1.7.9: Stop timer before destroying UI components
    stopTimer();

    // Destructor runs in REVERSE order of declaration:
    // 1. Attachments destroyed first (safe - webView still exists)
    // 2. WebView destroyed second (safe - relays still exist)
    // 3. Relays destroyed last
}

void OuariconLyricaAudioProcessorEditor::paint(juce::Graphics& g)
{
    // WebView handles all painting
    juce::ignoreUnused(g);
}

void OuariconLyricaAudioProcessorEditor::resized()
{
    // WebView fills entire editor
    if (webView)
        webView->setBounds(getLocalBounds());
}

// v1.7.9: Timer callback - poll MIDI events and notify WebView for tuning circle visualization
void OuariconLyricaAudioProcessorEditor::timerCallback()
{
    MidiNoteEvent event;
    while (processorRef.popMidiEvent(event))
    {
        if (event.velocity > 0.0f)
        {
            // Note-on: activate interval line with velocity-based intensity
            juce::String js = "if (typeof setNoteActive === 'function') setNoteActive("
                + juce::String(event.noteNumber) + ", "
                + juce::String(event.velocity, 3) + ");";
            webView->evaluateJavascript(js, nullptr);
        }
        else
        {
            // Note-off: deactivate interval line
            juce::String js = "if (typeof setNoteInactive === 'function') setNoteInactive("
                + juce::String(event.noteNumber) + ");";
            webView->evaluateJavascript(js, nullptr);
        }
    }
}

std::optional<juce::WebBrowserComponent::Resource>
OuariconLyricaAudioProcessorEditor::getResource(const juce::String& url)
{
    // Helper lambda to create resource from BinaryData
    auto makeResource = [](const char* data, int size, const char* mimeType) {
        return juce::WebBrowserComponent::Resource {
            std::vector<std::byte>(
                reinterpret_cast<const std::byte*>(data),
                reinterpret_cast<const std::byte*>(data) + size
            ),
            juce::String(mimeType)
        };
    };

    // ═══════════════════════════════════════════════════════════════════
    // CRITICAL: Explicit URL mapping (not generic loop)
    // BinaryData converts paths to C++ identifiers:
    //   index.html → index_html
    //   js/juce/index.js → index_js (path flattened)
    // ═══════════════════════════════════════════════════════════════════

    // HTML
    if (url == "/" || url == "/index.html")
        return makeResource(BinaryData::index_html,
                           BinaryData::index_htmlSize,
                           "text/html");

    // JUCE Bridge Library
    if (url == "/js/juce/index.js")
        return makeResource(BinaryData::index_js,
                           BinaryData::index_jsSize,
                           "text/javascript");

    // Native Interop Check (REQUIRED for WebView)
    if (url == "/js/juce/check_native_interop.js")
        return makeResource(BinaryData::check_native_interop_js,
                           BinaryData::check_native_interop_jsSize,
                           "text/javascript");

    // App JavaScript
    if (url == "/js/app.js")
        return makeResource(BinaryData::app_js,
                           BinaryData::app_jsSize,
                           "text/javascript");

    // v1.4.0: Images for Naturalist aesthetic
    if (url == "/images/paper1.jpg")
        return makeResource(BinaryData::paper1_jpg,
                           BinaryData::paper1_jpgSize,
                           "image/jpeg");

    if (url == "/images/fern_naturalistsmisc1Geor_0089.png")
        return makeResource(BinaryData::fern_naturalistsmisc1Geor_0089_png,
                           BinaryData::fern_naturalistsmisc1Geor_0089_pngSize,
                           "image/png");

    // 404 for unknown resources
    DBG("Resource not found: " + url);
    return std::nullopt;
}
