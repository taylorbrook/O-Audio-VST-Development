/*
  ==============================================================================

    O-Bells - Editor Implementation
    Ouaricon Development
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginEditor.h"
#include "BinaryData.h"

OBellsAudioProcessorEditor::OBellsAudioProcessorEditor(OBellsAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    // 1️⃣ CREATE RELAYS FIRST
    strikePositionRelay = std::make_unique<juce::WebSliderRelay>("strikePosition");
    malletHardnessRelay = std::make_unique<juce::WebSliderRelay>("malletHardness");
    dampingRelay = std::make_unique<juce::WebSliderRelay>("damping");
    overtoneBrightnessRelay = std::make_unique<juce::WebSliderRelay>("overtoneBrightness");
    acousticBrightnessRelay = std::make_unique<juce::WebSliderRelay>("acousticBrightness");
    airAbsorptionRelay = std::make_unique<juce::WebSliderRelay>("airAbsorption");
    airAbsorptionTimeRelay = std::make_unique<juce::WebSliderRelay>("airAbsorptionTime");
    inharmonicityRelay = std::make_unique<juce::WebSliderRelay>("inharmonicity");
    bloomSpeedRelay = std::make_unique<juce::WebSliderRelay>("bloomSpeed");
    bloomAmountRelay = std::make_unique<juce::WebSliderRelay>("bloomAmount");
    // v1.5.0: Bloom fine controls (per-band)
    bloomFineEnabledRelay = std::make_unique<juce::WebSliderRelay>("bloomFineEnabled");
    bloomSpeedLowRelay = std::make_unique<juce::WebSliderRelay>("bloomSpeedLow");
    bloomSpeedMidRelay = std::make_unique<juce::WebSliderRelay>("bloomSpeedMid");
    bloomSpeedHighRelay = std::make_unique<juce::WebSliderRelay>("bloomSpeedHigh");
    bloomAmountLowRelay = std::make_unique<juce::WebSliderRelay>("bloomAmountLow");
    bloomAmountMidRelay = std::make_unique<juce::WebSliderRelay>("bloomAmountMid");
    bloomAmountHighRelay = std::make_unique<juce::WebSliderRelay>("bloomAmountHigh");
    shimmerRelay = std::make_unique<juce::WebSliderRelay>("shimmer");
    unisonCountRelay = std::make_unique<juce::WebSliderRelay>("unisonCount");
    unisonDetuneRelay = std::make_unique<juce::WebSliderRelay>("unisonDetune");
    octaveBlendSubRelay = std::make_unique<juce::WebSliderRelay>("octaveBlendSub");
    octaveBlendOctRelay = std::make_unique<juce::WebSliderRelay>("octaveBlendOct");
    stereoSpreadRelay = std::make_unique<juce::WebSliderRelay>("stereoSpread");
    partialTuningRelay = std::make_unique<juce::WebSliderRelay>("partialTuning");
    pitchEnvelopeRelay = std::make_unique<juce::WebSliderRelay>("pitchEnvelope");
    pitchEnvTimeRelay = std::make_unique<juce::WebSliderRelay>("pitchEnvTime");
    nonlinearEffectsRelay = std::make_unique<juce::WebSliderRelay>("nonlinearEffects");
    attackLevelRelay = std::make_unique<juce::WebSliderRelay>("attackLevel");
    reverbMixRelay = std::make_unique<juce::WebSliderRelay>("reverbMix");
    outputGainRelay = std::make_unique<juce::WebSliderRelay>("outputGain");
    // Multi-stage envelope relays
    strikeTimeRelay = std::make_unique<juce::WebSliderRelay>("strikeTime");
    brillianceRelay = std::make_unique<juce::WebSliderRelay>("brilliance");
    bodyTimeRelay = std::make_unique<juce::WebSliderRelay>("bodyTime");
    humSustainRelay = std::make_unique<juce::WebSliderRelay>("humSustain");

    materialRelay = std::make_unique<juce::WebComboBoxRelay>("material");
    strikeNoiseCharRelay = std::make_unique<juce::WebComboBoxRelay>("strikeNoiseChar");
    velocityCurveRelay = std::make_unique<juce::WebComboBoxRelay>("velocityCurve");
    // decayShapeRelay removed in v1.2.0 - always multi-stage

    // 2️⃣ CREATE WEBVIEW WITH OPTIONS
    webView = std::make_unique<juce::WebBrowserComponent>(
        juce::WebBrowserComponent::Options{}
            .withNativeIntegrationEnabled()
            .withResourceProvider([this](const auto& url) { return getResource(url); })
            .withOptionsFrom(*strikePositionRelay)
            .withOptionsFrom(*malletHardnessRelay)
            .withOptionsFrom(*dampingRelay)
            .withOptionsFrom(*overtoneBrightnessRelay)
            .withOptionsFrom(*acousticBrightnessRelay)
            .withOptionsFrom(*airAbsorptionRelay)
            .withOptionsFrom(*airAbsorptionTimeRelay)
            .withOptionsFrom(*materialRelay)
            .withOptionsFrom(*inharmonicityRelay)
            .withOptionsFrom(*bloomSpeedRelay)
            .withOptionsFrom(*bloomAmountRelay)
            // v1.5.0: Bloom fine controls
            .withOptionsFrom(*bloomFineEnabledRelay)
            .withOptionsFrom(*bloomSpeedLowRelay)
            .withOptionsFrom(*bloomSpeedMidRelay)
            .withOptionsFrom(*bloomSpeedHighRelay)
            .withOptionsFrom(*bloomAmountLowRelay)
            .withOptionsFrom(*bloomAmountMidRelay)
            .withOptionsFrom(*bloomAmountHighRelay)
            .withOptionsFrom(*shimmerRelay)
            .withOptionsFrom(*unisonCountRelay)
            .withOptionsFrom(*unisonDetuneRelay)
            .withOptionsFrom(*octaveBlendSubRelay)
            .withOptionsFrom(*octaveBlendOctRelay)
            .withOptionsFrom(*stereoSpreadRelay)
            .withOptionsFrom(*partialTuningRelay)
            .withOptionsFrom(*pitchEnvelopeRelay)
            .withOptionsFrom(*pitchEnvTimeRelay)
            .withOptionsFrom(*nonlinearEffectsRelay)
            .withOptionsFrom(*attackLevelRelay)
            .withOptionsFrom(*reverbMixRelay)
            .withOptionsFrom(*outputGainRelay)
            // Multi-stage envelope relays
            .withOptionsFrom(*strikeTimeRelay)
            .withOptionsFrom(*brillianceRelay)
            .withOptionsFrom(*bodyTimeRelay)
            .withOptionsFrom(*humSustainRelay)
            .withOptionsFrom(*strikeNoiseCharRelay)
            .withOptionsFrom(*velocityCurveRelay)
            // decayShapeRelay removed in v1.2.0

            // ═══════════════════════════════════════════════════════════════════
            // v2.2.0: GUI KEYBOARD NATIVE FUNCTION
            // ═══════════════════════════════════════════════════════════════════

            .withNativeFunction("sendMidiNote", [this](const juce::Array<juce::var>& args,
                                                        std::function<void(juce::var)> complete) {
                if (args.size() >= 3) {
                    int midiNote = static_cast<int>(args[0]);
                    float velocity = static_cast<float>(args[1]);
                    bool isNoteOn = static_cast<bool>(args[2]);

                    if (isNoteOn)
                        processorRef.triggerNoteOn(midiNote, velocity);
                    else
                        processorRef.triggerNoteOff(midiNote);
                }
                complete({});
            })

            // ═══════════════════════════════════════════════════════════════════
            // PRESET NATIVE FUNCTIONS
            // ═══════════════════════════════════════════════════════════════════

            // getPresetList: Returns flat array of all preset names
            .withNativeFunction("getPresetList", [this](auto, auto complete) {
                auto& pm = processorRef.getPresetManager();
                auto presets = pm.getPresetList();
                juce::Array<juce::var> arr;
                for (const auto& name : presets)
                    arr.add(name);
                complete(juce::var(arr));
            })

            // getPresetListWithCategories: Returns {category: [presets...]} object
            .withNativeFunction("getPresetListWithCategories", [this](auto, auto complete) {
                auto& pm = processorRef.getPresetManager();
                auto categorized = pm.getPresetListWithCategories();
                auto* obj = new juce::DynamicObject();
                for (const auto& [category, presets] : categorized)
                {
                    juce::Array<juce::var> arr;
                    for (const auto& name : presets)
                        arr.add(name);
                    obj->setProperty(category, juce::var(arr));
                }
                complete(juce::var(obj));
            })

            // getCurrentPreset: Returns current preset name
            .withNativeFunction("getCurrentPreset", [this](auto, auto complete) {
                auto& pm = processorRef.getPresetManager();
                complete(juce::var(pm.getCurrentPresetName()));
            })

            // loadPreset: Loads preset by name (flat search)
            .withNativeFunction("loadPreset", [this](const auto& args, auto complete) {
                if (args.size() < 1 || !args[0].isString())
                {
                    complete(juce::var(false));
                    return;
                }
                auto& pm = processorRef.getPresetManager();
                bool success = pm.loadPreset(args[0].toString());
                complete(juce::var(success));
            })

            // loadPresetFromCategory: Loads preset from specific category
            .withNativeFunction("loadPresetFromCategory", [this](const auto& args, auto complete) {
                if (args.size() < 2 || !args[0].isString() || !args[1].isString())
                {
                    complete(juce::var(false));
                    return;
                }
                auto& pm = processorRef.getPresetManager();
                bool success = pm.loadPresetFromCategory(args[0].toString(), args[1].toString());
                complete(juce::var(success));
            })

            // savePreset: Saves user preset with given name
            .withNativeFunction("savePreset", [this](const auto& args, auto complete) {
                if (args.size() < 1 || !args[0].isString())
                {
                    complete(juce::var(false));
                    return;
                }
                auto& pm = processorRef.getPresetManager();
                bool success = pm.savePreset(args[0].toString());
                complete(juce::var(success));
            })

            // selectNextPreset: Navigate to next preset, returns new name
            .withNativeFunction("selectNextPreset", [this](auto, auto complete) {
                auto& pm = processorRef.getPresetManager();
                auto name = pm.getNextPreset();
                complete(juce::var(name));
            })

            // selectPreviousPreset: Navigate to previous preset, returns new name
            .withNativeFunction("selectPreviousPreset", [this](auto, auto complete) {
                auto& pm = processorRef.getPresetManager();
                auto name = pm.getPreviousPreset();
                complete(juce::var(name));
            })

            // savePresetWithDialog: Opens save dialog, saves preset
            .withNativeFunction("savePresetWithDialog", [this](auto, auto complete) {
                fileChooser = std::make_shared<juce::FileChooser>(
                    "Save Preset",
                    processorRef.getPresetManager().getUserPresetsDirectory(),
                    "*.json"
                );
                fileChooser->launchAsync(
                    juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                    [this, complete](const juce::FileChooser& fc) {
                        auto result = fc.getResult();
                        if (result == juce::File{})
                        {
                            complete(juce::var(""));
                            return;
                        }
                        auto name = result.getFileNameWithoutExtension();
                        auto& pm = processorRef.getPresetManager();
                        pm.savePreset(name);
                        complete(juce::var(name));
                    }
                );
            })

            // loadPresetFromFile: Opens file chooser, loads selected preset
            .withNativeFunction("loadPresetFromFile", [this](auto, auto complete) {
                fileChooser = std::make_shared<juce::FileChooser>(
                    "Load Preset",
                    processorRef.getPresetManager().getPresetsDirectory(),
                    "*.json"
                );
                fileChooser->launchAsync(
                    juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                    [this, complete](const juce::FileChooser& fc) {
                        auto result = fc.getResult();
                        if (result == juce::File{})
                        {
                            complete(juce::var(""));
                            return;
                        }
                        auto& pm = processorRef.getPresetManager();
                        pm.loadPresetFromFile(result);
                        complete(juce::var(pm.getCurrentPresetName()));
                    }
                );
            })
    );

    // 3️⃣ CREATE ATTACHMENTS LAST
    auto& apvts = processorRef.getAPVTS();

    strikePositionAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("strikePosition"), *strikePositionRelay, nullptr);
    malletHardnessAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("malletHardness"), *malletHardnessRelay, nullptr);
    dampingAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("damping"), *dampingRelay, nullptr);
    overtoneBrightnessAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("overtoneBrightness"), *overtoneBrightnessRelay, nullptr);
    acousticBrightnessAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("acousticBrightness"), *acousticBrightnessRelay, nullptr);
    airAbsorptionAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("airAbsorption"), *airAbsorptionRelay, nullptr);
    airAbsorptionTimeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("airAbsorptionTime"), *airAbsorptionTimeRelay, nullptr);
    inharmonicityAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("inharmonicity"), *inharmonicityRelay, nullptr);
    bloomSpeedAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("bloomSpeed"), *bloomSpeedRelay, nullptr);
    bloomAmountAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("bloomAmount"), *bloomAmountRelay, nullptr);
    // v1.5.0: Bloom fine controls (per-band)
    bloomFineEnabledAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("bloomFineEnabled"), *bloomFineEnabledRelay, nullptr);
    bloomSpeedLowAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("bloomSpeedLow"), *bloomSpeedLowRelay, nullptr);
    bloomSpeedMidAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("bloomSpeedMid"), *bloomSpeedMidRelay, nullptr);
    bloomSpeedHighAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("bloomSpeedHigh"), *bloomSpeedHighRelay, nullptr);
    bloomAmountLowAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("bloomAmountLow"), *bloomAmountLowRelay, nullptr);
    bloomAmountMidAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("bloomAmountMid"), *bloomAmountMidRelay, nullptr);
    bloomAmountHighAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("bloomAmountHigh"), *bloomAmountHighRelay, nullptr);
    shimmerAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("shimmer"), *shimmerRelay, nullptr);
    unisonCountAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("unisonCount"), *unisonCountRelay, nullptr);
    unisonDetuneAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("unisonDetune"), *unisonDetuneRelay, nullptr);
    octaveBlendSubAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("octaveBlendSub"), *octaveBlendSubRelay, nullptr);
    octaveBlendOctAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("octaveBlendOct"), *octaveBlendOctRelay, nullptr);
    stereoSpreadAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("stereoSpread"), *stereoSpreadRelay, nullptr);
    partialTuningAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("partialTuning"), *partialTuningRelay, nullptr);
    pitchEnvelopeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("pitchEnvelope"), *pitchEnvelopeRelay, nullptr);
    pitchEnvTimeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("pitchEnvTime"), *pitchEnvTimeRelay, nullptr);
    nonlinearEffectsAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("nonlinearEffects"), *nonlinearEffectsRelay, nullptr);
    attackLevelAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("attackLevel"), *attackLevelRelay, nullptr);
    reverbMixAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("reverbMix"), *reverbMixRelay, nullptr);
    outputGainAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("outputGain"), *outputGainRelay, nullptr);
    // Multi-stage envelope attachments
    strikeTimeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("strikeTime"), *strikeTimeRelay, nullptr);
    brillianceAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("brilliance"), *brillianceRelay, nullptr);
    bodyTimeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("bodyTime"), *bodyTimeRelay, nullptr);
    humSustainAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("humSustain"), *humSustainRelay, nullptr);

    materialAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *apvts.getParameter("material"), *materialRelay, nullptr);
    strikeNoiseCharAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *apvts.getParameter("strikeNoiseChar"), *strikeNoiseCharRelay, nullptr);
    velocityCurveAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *apvts.getParameter("velocityCurve"), *velocityCurveRelay, nullptr);
    // decayShapeAttachment removed in v1.2.0 - always multi-stage

    // Add WebView to editor
    addAndMakeVisible(*webView);

    // Navigate to UI
    webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());

    // Set editor size (800x600 from mockup)
    setSize(800, 600);

    // Start meter update timer (30 Hz for smooth animation)
    startTimerHz(30);
}

OBellsAudioProcessorEditor::~OBellsAudioProcessorEditor()
{
    // Stop timer before destruction
    stopTimer();

    // Attachments destroyed first (safe - they stop using relays/webView)
    // Then webView destroyed
    // Then relays destroyed last
}

void OBellsAudioProcessorEditor::timerCallback()
{
    // Read levels from processor (atomic, thread-safe)
    float leftLevel = processorRef.outputLevelLeft.load();
    float rightLevel = processorRef.outputLevelRight.load();

    // Convert to percentage (clamp to 0-100)
    int leftPercent = static_cast<int>(std::min(1.0f, leftLevel) * 100.0f);
    int rightPercent = static_cast<int>(std::min(1.0f, rightLevel) * 100.0f);

    // Send to WebView via JavaScript
    juce::String js = juce::String::formatted(
        "if (window.updateMeterLevels) window.updateMeterLevels(%d, %d);",
        leftPercent, rightPercent
    );
    webView->evaluateJavascript(js);
}

void OBellsAudioProcessorEditor::paint(juce::Graphics& g)
{
    // WebView handles all painting
    juce::ignoreUnused(g);
}

void OBellsAudioProcessorEditor::resized()
{
    // WebView fills entire editor
    if (webView)
        webView->setBounds(getLocalBounds());
}

std::optional<juce::WebBrowserComponent::Resource>
OBellsAudioProcessorEditor::getResource(const juce::String& url)
{
    auto makeVector = [](const char* data, int size) {
        return std::vector<std::byte>(
            reinterpret_cast<const std::byte*>(data),
            reinterpret_cast<const std::byte*>(data) + size
        );
    };

    // Explicit URL mapping (Pattern #8 - no generic loops)
    if (url == "/" || url == "/index.html") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::index_html, BinaryData::index_htmlSize),
            juce::String("text/html")
        };
    }

    if (url == "/js/juce/index.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::index_js, BinaryData::index_jsSize),
            juce::String("text/javascript")
        };
    }

    if (url == "/js/juce/check_native_interop.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::check_native_interop_js, BinaryData::check_native_interop_jsSize),
            juce::String("text/javascript")
        };
    }

    if (url == "/img/snail.png") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::snail_png, BinaryData::snail_pngSize),
            juce::String("image/png")
        };
    }

    // Resource not found
    juce::Logger::writeToLog("O-Bells: Resource not found: " + url);
    return std::nullopt;
}
