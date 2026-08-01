/*
   This file is part of O-Marimba, an Ouaricon Audio plugin.
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

    O-Marimba - Editor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginEditor.h"
#include "BinaryData.h"

OMarimbaAudioProcessorEditor::OMarimbaAudioProcessorEditor(OMarimbaAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    // 1️⃣ Create relays FIRST (Pattern 11: Order critical for destruction)
    malletHardnessRelay = std::make_unique<juce::WebSliderRelay>("MALLET_HARDNESS");
    barMaterialRelay = std::make_unique<juce::WebSliderRelay>("BAR_MATERIAL");
    resonanceRelay = std::make_unique<juce::WebSliderRelay>("RESONANCE");
    // v1.6.0: New timbre parameter relays
    strikePositionRelay = std::make_unique<juce::WebSliderRelay>("STRIKE_POSITION");
    overtoneDampingRelay = std::make_unique<juce::WebSliderRelay>("OVERTONE_DAMPING");
    toneRelay = std::make_unique<juce::WebSliderRelay>("TONE");
    // Tuning/system relays
    tuningModeRelay = std::make_unique<juce::WebSliderRelay>("TUNING_MODE");
    referencePitchRelay = std::make_unique<juce::WebSliderRelay>("REFERENCE_PITCH");
    velCurveRelay = std::make_unique<juce::WebSliderRelay>("VEL_CURVE");
    outputGainRelay = std::make_unique<juce::WebSliderRelay>("OUTPUT_GAIN");

    // v1.8.0: Analog EQ Unit relays
    eqLfFreqRelay = std::make_unique<juce::WebSliderRelay>("fx_eq_lf_freq");
    eqLfGainRelay = std::make_unique<juce::WebSliderRelay>("fx_eq_lf_gain");
    eqLmfFreqRelay = std::make_unique<juce::WebSliderRelay>("fx_eq_lmf_freq");
    eqLmfGainRelay = std::make_unique<juce::WebSliderRelay>("fx_eq_lmf_gain");
    eqHmfFreqRelay = std::make_unique<juce::WebSliderRelay>("fx_eq_hmf_freq");
    eqHmfGainRelay = std::make_unique<juce::WebSliderRelay>("fx_eq_hmf_gain");
    eqHfFreqRelay = std::make_unique<juce::WebSliderRelay>("fx_eq_hf_freq");
    eqHfGainRelay = std::make_unique<juce::WebSliderRelay>("fx_eq_hf_gain");
    eqOutputGainRelay = std::make_unique<juce::WebSliderRelay>("fx_eq_output_gain");
    eqLfOnRelay = std::make_unique<juce::WebToggleButtonRelay>("fx_eq_lf_on");
    eqLmfOnRelay = std::make_unique<juce::WebToggleButtonRelay>("fx_eq_lmf_on");
    eqHmfOnRelay = std::make_unique<juce::WebToggleButtonRelay>("fx_eq_hmf_on");
    eqHfOnRelay = std::make_unique<juce::WebToggleButtonRelay>("fx_eq_hf_on");
    eqAnalogRelay = std::make_unique<juce::WebToggleButtonRelay>("fx_eq_analog");
    eqEnabledRelay = std::make_unique<juce::WebToggleButtonRelay>("fx_eq_enabled");  // v1.8.1
    eqLmfQRelay = std::make_unique<juce::WebComboBoxRelay>("fx_eq_lmf_q");
    eqHmfQRelay = std::make_unique<juce::WebComboBoxRelay>("fx_eq_hmf_q");

    // v1.9.0: Compressor Unit relays
    compEnabledRelay = std::make_unique<juce::WebToggleButtonRelay>("fx_comp_enabled");
    compThresholdRelay = std::make_unique<juce::WebSliderRelay>("fx_comp_threshold");
    compRatioRelay = std::make_unique<juce::WebSliderRelay>("fx_comp_ratio");
    compAttackRelay = std::make_unique<juce::WebSliderRelay>("fx_comp_attack");
    compReleaseRelay = std::make_unique<juce::WebSliderRelay>("fx_comp_release");
    compAutogainRelay = std::make_unique<juce::WebToggleButtonRelay>("fx_comp_autogain");  // v1.9.1

    // 2️⃣ Create WebView with options (Pattern 9: NEEDS_WEB_BROWSER required)
    webView = std::make_unique<juce::WebBrowserComponent>(
        juce::WebBrowserComponent::Options{}
            .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
            .withWinWebView2Options(
                juce::WebBrowserComponent::Options::WinWebView2{}
                    .withUserDataFolder(juce::File::getSpecialLocation(
                        juce::File::SpecialLocationType::tempDirectory)))
            .withNativeIntegrationEnabled()
            .withResourceProvider([this](const auto& url) { return getResource(url); })
            .withOptionsFrom(*malletHardnessRelay)
            .withOptionsFrom(*barMaterialRelay)
            .withOptionsFrom(*resonanceRelay)
            // v1.6.0: New timbre parameter relays
            .withOptionsFrom(*strikePositionRelay)
            .withOptionsFrom(*overtoneDampingRelay)
            .withOptionsFrom(*toneRelay)
            // Tuning/system relays
            .withOptionsFrom(*tuningModeRelay)
            .withOptionsFrom(*referencePitchRelay)
            .withOptionsFrom(*velCurveRelay)
            .withOptionsFrom(*outputGainRelay)
            // v1.8.0: Analog EQ Unit relays
            .withOptionsFrom(*eqLfFreqRelay)
            .withOptionsFrom(*eqLfGainRelay)
            .withOptionsFrom(*eqLmfFreqRelay)
            .withOptionsFrom(*eqLmfGainRelay)
            .withOptionsFrom(*eqHmfFreqRelay)
            .withOptionsFrom(*eqHmfGainRelay)
            .withOptionsFrom(*eqHfFreqRelay)
            .withOptionsFrom(*eqHfGainRelay)
            .withOptionsFrom(*eqOutputGainRelay)
            .withOptionsFrom(*eqLfOnRelay)
            .withOptionsFrom(*eqLmfOnRelay)
            .withOptionsFrom(*eqHmfOnRelay)
            .withOptionsFrom(*eqHfOnRelay)
            .withOptionsFrom(*eqAnalogRelay)
            .withOptionsFrom(*eqEnabledRelay)  // v1.8.1
            .withOptionsFrom(*eqLmfQRelay)
            .withOptionsFrom(*eqHmfQRelay)
            // v1.9.0: Compressor Unit relays
            .withOptionsFrom(*compEnabledRelay)
            .withOptionsFrom(*compThresholdRelay)
            .withOptionsFrom(*compRatioRelay)
            .withOptionsFrom(*compAttackRelay)
            .withOptionsFrom(*compReleaseRelay)
            .withOptionsFrom(*compAutogainRelay)  // v1.9.1
            .withNativeFunction("sendMidiNote", [this](const auto& args, auto complete) {
                complete(sendMidiNote(args));
            })
            .withNativeFunction("setTuningIntervals", [this](const auto& args, auto complete) {
                complete(setTuningIntervals(args));
            })
            .withNativeFunction("loadScalaFile", [this](const auto& args, auto complete) {
                loadScalaFile(args);
                complete(juce::var("OK"));
            })
            .withNativeFunction("loadKBMFile", [this](const auto& args, auto complete) {
                loadKBMFile(args);
                complete(juce::var("OK"));
            })
            .withNativeFunction("getTuningIntervals", [this](const auto& args, auto complete) {
                complete(getTuningIntervals(args));
            })
            // v1.4.0: Save Scala/KBM files
            .withNativeFunction("saveScalaFile", [this](const auto& args, auto complete) {
                saveScalaFile(args);
                complete(juce::var("OK"));
            })
            .withNativeFunction("saveKBMFile", [this](const auto& args, auto complete) {
                saveKBMFile(args);
                complete(juce::var("OK"));
            })
            .withNativeFunction("setTonicNote", [this](const auto& args, auto complete) {
                complete(setTonicNote(args));
            })
            .withNativeFunction("getWaveformData", [this](const auto& args, auto complete) {
                complete(getWaveformData(args));
            })
            // v1.3.0: Preset system native functions
            .withNativeFunction("savePreset", [this](const auto& args, auto complete) {
                savePreset(args);
                complete(juce::var("OK"));
            })
            .withNativeFunction("loadPreset", [this](const auto& args, auto complete) {
                complete(loadPreset(args));
            })
            .withNativeFunction("getPresetList", [this](const auto& args, auto complete) {
                complete(getPresetList(args));
            })
            .withNativeFunction("getCurrentPreset", [this](const auto& args, auto complete) {
                complete(getCurrentPreset(args));
            })
            .withNativeFunction("selectNextPreset", [this](const auto& args, auto complete) {
                complete(selectNextPreset(args));
            })
            .withNativeFunction("selectPreviousPreset", [this](const auto& args, auto complete) {
                complete(selectPreviousPreset(args));
            })
            .withNativeFunction("deletePreset", [this](const auto& args, auto complete) {
                complete(deletePreset(args));
            })
            // v1.3.1: Load preset via file dialog
            .withNativeFunction("loadPresetFromFile", [this](const auto& args, auto complete) {
                loadPresetFromFile(args);
                complete(juce::var("OK"));
            })
    );

    // 3️⃣ Create attachments LAST (Pattern 12: 3 params required - parameter, relay, nullptr)
    malletHardnessAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("MALLET_HARDNESS"), *malletHardnessRelay, nullptr);

    barMaterialAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("BAR_MATERIAL"), *barMaterialRelay, nullptr);

    resonanceAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("RESONANCE"), *resonanceRelay, nullptr);

    // v1.6.0: New timbre parameter attachments
    strikePositionAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("STRIKE_POSITION"), *strikePositionRelay, nullptr);

    overtoneDampingAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("OVERTONE_DAMPING"), *overtoneDampingRelay, nullptr);

    toneAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("TONE"), *toneRelay, nullptr);

    tuningModeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("TUNING_MODE"), *tuningModeRelay, nullptr);

    referencePitchAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("REFERENCE_PITCH"), *referencePitchRelay, nullptr);

    velCurveAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("VEL_CURVE"), *velCurveRelay, nullptr);

    outputGainAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("OUTPUT_GAIN"), *outputGainRelay, nullptr);

    // v1.8.0: Analog EQ Unit attachments
    eqLfFreqAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("fx_eq_lf_freq"), *eqLfFreqRelay, nullptr);
    eqLfGainAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("fx_eq_lf_gain"), *eqLfGainRelay, nullptr);
    eqLmfFreqAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("fx_eq_lmf_freq"), *eqLmfFreqRelay, nullptr);
    eqLmfGainAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("fx_eq_lmf_gain"), *eqLmfGainRelay, nullptr);
    eqHmfFreqAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("fx_eq_hmf_freq"), *eqHmfFreqRelay, nullptr);
    eqHmfGainAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("fx_eq_hmf_gain"), *eqHmfGainRelay, nullptr);
    eqHfFreqAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("fx_eq_hf_freq"), *eqHfFreqRelay, nullptr);
    eqHfGainAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("fx_eq_hf_gain"), *eqHfGainRelay, nullptr);
    eqOutputGainAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("fx_eq_output_gain"), *eqOutputGainRelay, nullptr);
    eqLfOnAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.parameters.getParameter("fx_eq_lf_on"), *eqLfOnRelay, nullptr);
    eqLmfOnAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.parameters.getParameter("fx_eq_lmf_on"), *eqLmfOnRelay, nullptr);
    eqHmfOnAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.parameters.getParameter("fx_eq_hmf_on"), *eqHmfOnRelay, nullptr);
    eqHfOnAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.parameters.getParameter("fx_eq_hf_on"), *eqHfOnRelay, nullptr);
    eqAnalogAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.parameters.getParameter("fx_eq_analog"), *eqAnalogRelay, nullptr);
    eqEnabledAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.parameters.getParameter("fx_eq_enabled"), *eqEnabledRelay, nullptr);  // v1.8.1
    eqLmfQAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *processorRef.parameters.getParameter("fx_eq_lmf_q"), *eqLmfQRelay, nullptr);
    eqHmfQAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *processorRef.parameters.getParameter("fx_eq_hmf_q"), *eqHmfQRelay, nullptr);

    // v1.9.0: Compressor Unit attachments
    compEnabledAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.parameters.getParameter("fx_comp_enabled"), *compEnabledRelay, nullptr);
    compThresholdAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("fx_comp_threshold"), *compThresholdRelay, nullptr);
    compRatioAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("fx_comp_ratio"), *compRatioRelay, nullptr);
    compAttackAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("fx_comp_attack"), *compAttackRelay, nullptr);
    compReleaseAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("fx_comp_release"), *compReleaseRelay, nullptr);
    compAutogainAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.parameters.getParameter("fx_comp_autogain"), *compAutogainRelay, nullptr);  // v1.9.1

    // Add WebView to editor
    addAndMakeVisible(*webView);

    // Navigate to UI
    webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());

    // Set editor size (from v1 mockup: 600x400)
    setSize(600, 400);

    // Start timer to poll for note-on events (30fps is responsive enough)
    startTimerHz(30);
}

OMarimbaAudioProcessorEditor::~OMarimbaAudioProcessorEditor()
{
    // Stop timer before destruction
    stopTimer();

    // Destruction happens in reverse order (attachments → webView → relays)
    // Pattern 11: No manual cleanup needed (unique_ptr handles it)
}

// Timer callback: Poll processor for MIDI events and notify WebView
void OMarimbaAudioProcessorEditor::timerCallback()
{
    // v1.2.6: Process all queued MIDI events for polyphonic visualization
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

    // v1.2.5: VU Meter - emit output level to WebView
    const float outputDB = processorRef.outputLevelDB.load(std::memory_order_relaxed);
    webView->emitEventIfBrowserIsVisible("outputLevel", outputDB);

    // v1.9.0: Compressor GR meter - emit gain reduction to WebView
    const float compGrDB = processorRef.getCompressorGainReductionDB();
    webView->emitEventIfBrowserIsVisible("compressorGR", compGrDB);
}

void OMarimbaAudioProcessorEditor::paint(juce::Graphics& g)
{
    // WebView handles all painting
    juce::ignoreUnused(g);
}

void OMarimbaAudioProcessorEditor::resized()
{
    // WebView fills entire editor
    webView->setBounds(getLocalBounds());
}

// Native function: Send MIDI note to synthesizer
// Args: [noteNumber (int), velocity (float 0-1), isNoteOn (bool)]
juce::var OMarimbaAudioProcessorEditor::sendMidiNote(const juce::Array<juce::var>& args)
{
    if (args.size() < 3)
        return juce::var("Error: Expected 3 arguments (noteNumber, velocity, isNoteOn)");

    int noteNumber = static_cast<int>(args[0]);
    float velocity = static_cast<float>(args[1]);
    bool isNoteOn = static_cast<bool>(args[2]);

    // Clamp values
    noteNumber = juce::jlimit(0, 127, noteNumber);
    velocity = juce::jlimit(0.0f, 1.0f, velocity);

    // Create MIDI message
    juce::MidiMessage msg;
    if (isNoteOn && velocity > 0.0f)
    {
        msg = juce::MidiMessage::noteOn(1, noteNumber, velocity);
    }
    else
    {
        msg = juce::MidiMessage::noteOff(1, noteNumber, 0.0f);
    }

    // Inject into processor's pending MIDI buffer
    // The processor will pick this up in the next processBlock
    processorRef.addMidiMessage(msg);

    return juce::var("OK");
}

// Native function: Set custom tuning intervals
// Args: [Array of cents values (up to 12), scaleName (string)]
juce::var OMarimbaAudioProcessorEditor::setTuningIntervals(const juce::Array<juce::var>& args)
{
    if (args.size() < 1)
        return juce::var("Error: Expected array of cents values");

    // First argument should be array of cents
    if (!args[0].isArray())
        return juce::var("Error: First argument must be array of cents values");

    auto* centsArray = args[0].getArray();
    if (centsArray == nullptr || centsArray->isEmpty())
        return juce::var("Error: Cents array is empty");

    // Convert to vector of doubles
    std::vector<double> cents;
    for (const auto& val : *centsArray)
    {
        cents.push_back(static_cast<double>(val));
    }

    // Get optional scale name
    juce::String scaleName = "Custom";
    if (args.size() > 1 && args[1].isString())
    {
        scaleName = args[1].toString();
    }

    // Send to tuning engine
    processorRef.getTuningEngine().setCustomIntervals(cents, scaleName);

    return juce::var("OK: Set " + juce::String(cents.size()) + " intervals");
}

// Pattern 8: Explicit URL mapping (required for WebView resource loading)
std::optional<juce::WebBrowserComponent::Resource>
OMarimbaAudioProcessorEditor::getResource(const juce::String& url)
{
    // Helper to convert BinaryData to std::vector<std::byte>
    auto makeVector = [](const char* data, int size) {
        return std::vector<std::byte>(
            reinterpret_cast<const std::byte*>(data),
            reinterpret_cast<const std::byte*>(data) + size
        );
    };

    // Root "/" → index.html
    if (url == "/" || url == "/index.html" || url.isEmpty()) {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::index_html, BinaryData::index_htmlSize),
            juce::String("text/html")
        };
    }

    // JUCE JavaScript bridge (Pattern 13: check_native_interop.js required)
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

    // v1.8.0: Analog EQ Unit JavaScript module
    if (url == "/modules/analog-eq-unit.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::analogequnit_js, BinaryData::analogequnit_jsSize),
            juce::String("text/javascript")
        };
    }

    // v1.9.0: Compressor Unit JavaScript module
    if (url == "/modules/compressor-unit.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::compressorunit_js, BinaryData::compressorunit_jsSize),
            juce::String("text/javascript")
        };
    }

    // Background paper texture
    if (url == "/images/paper1.jpg") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::paper1_jpg, BinaryData::paper1_jpgSize),
            juce::String("image/jpeg")
        };
    }

    // Botanical tree overlay
    if (url == "/images/tree_expositionmtho00lamo_0207.png") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::tree_expositionmtho00lamo_0207_png, BinaryData::tree_expositionmtho00lamo_0207_pngSize),
            juce::String("image/png")
        };
    }

    // Resource not found
    juce::Logger::writeToLog("Resource not found: " + url);
    return std::nullopt;
}

// Native function: Open file dialog to load Scala .scl file
juce::var OMarimbaAudioProcessorEditor::loadScalaFile(const juce::Array<juce::var>& args)
{
    juce::ignoreUnused(args);

    fileChooser = std::make_unique<juce::FileChooser>(
        "Select Scala File",
        juce::File::getSpecialLocation(juce::File::userHomeDirectory),
        "*.scl"
    );

    auto chooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

    // CR-02: guard the async completion against editor teardown (window closed while the
    // native dialog is open) — bare return on death, no complete() to call here.
    juce::Component::SafePointer<OMarimbaAudioProcessorEditor> safe { this };
    fileChooser->launchAsync(chooserFlags, [safe](const juce::FileChooser& fc)
    {
        if (safe == nullptr) return;
        auto file = fc.getResult();
        if (file.existsAsFile())
        {
            bool success = safe->processorRef.getTuningEngine().loadScalaFile(file);
            if (success)
            {
                // Notify JavaScript of the loaded scale name
                juce::String scaleName = safe->processorRef.getTuningEngine().getActiveTuningName();
                juce::String js = "if (window.onScalaLoaded) window.onScalaLoaded('" + scaleName + "');";
                safe->webView->evaluateJavascript(js, nullptr);
            }
        }
    });

    return juce::var("OK");
}

// Native function: Open file dialog to load Scala .kbm file
juce::var OMarimbaAudioProcessorEditor::loadKBMFile(const juce::Array<juce::var>& args)
{
    juce::ignoreUnused(args);

    fileChooser = std::make_unique<juce::FileChooser>(
        "Select Keyboard Mapping File",
        juce::File::getSpecialLocation(juce::File::userHomeDirectory),
        "*.kbm"
    );

    auto chooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

    // CR-02: guard against editor teardown while the dialog is open.
    juce::Component::SafePointer<OMarimbaAudioProcessorEditor> safe { this };
    fileChooser->launchAsync(chooserFlags, [safe](const juce::FileChooser& fc)
    {
        if (safe == nullptr) return;
        auto file = fc.getResult();
        if (file.existsAsFile())
        {
            safe->processorRef.getTuningEngine().loadKBMFile(file);
        }
    });

    return juce::var("OK");
}

// Native function: Get current tuning intervals from TuningEngine
juce::var OMarimbaAudioProcessorEditor::getTuningIntervals(const juce::Array<juce::var>& args)
{
    juce::ignoreUnused(args);

    const auto& intervals = processorRef.getTuningEngine().getIntervals();
    juce::String name = processorRef.getTuningEngine().getActiveTuningName();

    // Create result object
    juce::DynamicObject::Ptr result = new juce::DynamicObject();

    // Add intervals array
    juce::Array<juce::var> intervalsArray;
    for (const auto& cents : intervals)
    {
        intervalsArray.add(cents);
    }
    result->setProperty("intervals", intervalsArray);
    result->setProperty("name", name);
    result->setProperty("degrees", processorRef.getTuningEngine().getScaleDegrees());

    return juce::var(result.get());
}

// Native function: Set tonic note for transposition
// Args: [tonicIndex (int 0-11, where 0=C, 1=C#, 2=D, etc.)]
juce::var OMarimbaAudioProcessorEditor::setTonicNote(const juce::Array<juce::var>& args)
{
    if (args.isEmpty())
        return juce::var("Error: Expected tonic index (0-11)");

    int tonicIndex = static_cast<int>(args[0]);
    tonicIndex = juce::jlimit(0, 11, tonicIndex);

    // Set tonic in tuning engine for transposition
    processorRef.getTuningEngine().setTonicNote(tonicIndex);

    return juce::var("OK: Tonic set to " + juce::String(tonicIndex));
}

// v1.2.3: Native function to get waveform data for oscilloscope display
// Returns array of normalized sample values (-1 to 1), downsampled for display
juce::var OMarimbaAudioProcessorEditor::getWaveformData(const juce::Array<juce::var>& args)
{
    juce::ignoreUnused(args);

    // Read from FIFO - 512 samples
    static constexpr int kFifoSize = 512;
    static constexpr int kDisplayPoints = 128;  // Downsample for smooth SVG

    float samples[kFifoSize];
    bool hasData = processorRef.getWaveformFifo().read(samples, kFifoSize);

    if (!hasData)
    {
        // Return null to indicate no new data
        return juce::var();
    }

    // Downsample to display resolution
    juce::Array<juce::var> result;
    int samplesPerPoint = kFifoSize / kDisplayPoints;

    for (int i = 0; i < kDisplayPoints; ++i)
    {
        // Take max absolute value in each bucket for better visual representation
        float maxVal = 0.0f;
        for (int j = 0; j < samplesPerPoint; ++j)
        {
            int idx = i * samplesPerPoint + j;
            float absVal = std::abs(samples[idx]);
            if (absVal > std::abs(maxVal))
                maxVal = samples[idx];  // Keep sign for waveform shape
        }
        result.add(juce::jlimit(-1.0f, 1.0f, maxVal));
    }

    return result;
}

// ============================================================================
// v1.3.0: Preset System Native Functions
// ============================================================================

// Helper: Notify WebView of preset load with tuning state
void OMarimbaAudioProcessorEditor::notifyPresetLoaded(const juce::String& presetName)
{
    auto& tuning = processorRef.getTuningEngine();
    juce::String scaleName = tuning.getActiveTuningName();

    juce::Array<juce::var> intervalsArray;
    for (const auto& cents : tuning.getIntervals())
        intervalsArray.add(cents);

    juce::String intervalsJson = juce::JSON::toString(intervalsArray);
    juce::String js = "if (window.onPresetLoaded) window.onPresetLoaded('"
        + presetName.replace("'", "\\'") + "', '"
        + scaleName.replace("'", "\\'") + "', "
        + intervalsJson + ", "
        + juce::String(tuning.getTonicNote()) + ");";
    webView->evaluateJavascript(js, nullptr);
}

// Save preset - opens file dialog to name and save current state
juce::var OMarimbaAudioProcessorEditor::savePreset(const juce::Array<juce::var>& args)
{
    juce::ignoreUnused(args);

    fileChooser = std::make_unique<juce::FileChooser>(
        "Save Preset",
        juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
            .getChildFile("O-Marimba")
            .getChildFile("Presets")
            .getChildFile("User"),
        "*.json"
    );

    auto chooserFlags = juce::FileBrowserComponent::saveMode
                      | juce::FileBrowserComponent::canSelectFiles
                      | juce::FileBrowserComponent::warnAboutOverwriting;

    // CR-02: guard against editor teardown while the dialog is open.
    juce::Component::SafePointer<OMarimbaAudioProcessorEditor> safe { this };
    fileChooser->launchAsync(chooserFlags, [safe](const juce::FileChooser& fc)
    {
        if (safe == nullptr) return;
        auto file = fc.getResult();
        if (file != juce::File{})
        {
            juce::String presetName = file.getFileNameWithoutExtension();
            bool success = safe->processorRef.getPresetManager().savePreset(presetName);

            if (success)
            {
                // Notify JavaScript of the saved preset
                juce::String js = "if (window.onPresetSaved) window.onPresetSaved('"
                    + presetName.replace("'", "\\'") + "');";
                safe->webView->evaluateJavascript(js, nullptr);
            }
        }
    });

    return juce::var("OK");
}

// Load a named preset
juce::var OMarimbaAudioProcessorEditor::loadPreset(const juce::Array<juce::var>& args)
{
    if (args.isEmpty())
        return juce::var("Error: No preset name provided");

    juce::String presetName = args[0].toString();
    bool success = processorRef.getPresetManager().loadPreset(presetName);

    if (success)
    {
        notifyPresetLoaded(presetName);
        return juce::var("OK");
    }

    return juce::var("Error: Failed to load preset");
}

// Get list of available presets
juce::var OMarimbaAudioProcessorEditor::getPresetList(const juce::Array<juce::var>& args)
{
    juce::ignoreUnused(args);

    auto& pm = processorRef.getPresetManager();
    auto presets = pm.getPresetList();

    juce::Array<juce::var> result;
    for (const auto& name : presets)
    {
        juce::DynamicObject::Ptr presetInfo = new juce::DynamicObject();
        presetInfo->setProperty("name", name);
        presetInfo->setProperty("isFactory", pm.isFactoryPreset(name));
        result.add(juce::var(presetInfo.get()));
    }

    return result;
}

// Get currently loaded preset name
juce::var OMarimbaAudioProcessorEditor::getCurrentPreset(const juce::Array<juce::var>& args)
{
    juce::ignoreUnused(args);
    return processorRef.getPresetManager().getCurrentPresetName();
}

// Navigate to next preset
juce::var OMarimbaAudioProcessorEditor::selectNextPreset(const juce::Array<juce::var>& args)
{
    juce::ignoreUnused(args);

    auto& pm = processorRef.getPresetManager();
    juce::String nextPreset = pm.getNextPreset();

    if (pm.loadPreset(nextPreset))
    {
        // Trigger full UI update via loadPreset callback
        juce::Array<juce::var> loadArgs;
        loadArgs.add(nextPreset);
        loadPreset(loadArgs);
    }

    return nextPreset;
}

// Navigate to previous preset
juce::var OMarimbaAudioProcessorEditor::selectPreviousPreset(const juce::Array<juce::var>& args)
{
    juce::ignoreUnused(args);

    auto& pm = processorRef.getPresetManager();
    juce::String prevPreset = pm.getPreviousPreset();

    if (pm.loadPreset(prevPreset))
    {
        // Trigger full UI update via loadPreset callback
        juce::Array<juce::var> loadArgs;
        loadArgs.add(prevPreset);
        loadPreset(loadArgs);
    }

    return prevPreset;
}

// Delete a user preset
juce::var OMarimbaAudioProcessorEditor::deletePreset(const juce::Array<juce::var>& args)
{
    if (args.isEmpty())
        return juce::var("Error: No preset name provided");

    juce::String presetName = args[0].toString();
    bool success = processorRef.getPresetManager().deletePreset(presetName);

    return success ? juce::var("OK") : juce::var("Error: Cannot delete preset (may be factory preset)");
}

// v1.3.1: Load preset via file dialog (LOAD button)
juce::var OMarimbaAudioProcessorEditor::loadPresetFromFile(const juce::Array<juce::var>& args)
{
    juce::ignoreUnused(args);

    // Start in User presets folder, but allow navigating to Factory too
    fileChooser = std::make_unique<juce::FileChooser>(
        "Load Preset",
        juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
            .getChildFile("O-Marimba")
            .getChildFile("Presets"),
        "*.json"
    );

    auto chooserFlags = juce::FileBrowserComponent::openMode
                      | juce::FileBrowserComponent::canSelectFiles;

    // CR-02: guard against editor teardown while the dialog is open.
    juce::Component::SafePointer<OMarimbaAudioProcessorEditor> safe { this };
    fileChooser->launchAsync(chooserFlags, [safe](const juce::FileChooser& fc)
    {
        if (safe == nullptr) return;
        auto file = fc.getResult();
        if (file.existsAsFile())
        {
            juce::String presetName = file.getFileNameWithoutExtension();
            if (safe->processorRef.getPresetManager().loadPreset(presetName))
                safe->notifyPresetLoaded(presetName);
        }
    });

    return juce::var("OK");
}

// ============================================================================
// v1.4.0: Scala/KBM File Export
// ============================================================================

// Save current tuning as Scala .scl file
juce::var OMarimbaAudioProcessorEditor::saveScalaFile(const juce::Array<juce::var>& args)
{
    juce::ignoreUnused(args);

    // Get last used directory or default to Documents
    auto lastDir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);

    // Check if we have a stored last directory (session memory)
    if (lastScalaExportPath.isNotEmpty())
    {
        auto storedDir = juce::File(lastScalaExportPath);
        if (storedDir.isDirectory())
            lastDir = storedDir;
    }

    fileChooser = std::make_unique<juce::FileChooser>(
        "Save Scala File",
        lastDir.getChildFile(processorRef.getTuningEngine().getActiveTuningName() + ".scl"),
        "*.scl"
    );

    auto chooserFlags = juce::FileBrowserComponent::saveMode
                      | juce::FileBrowserComponent::canSelectFiles
                      | juce::FileBrowserComponent::warnAboutOverwriting;

    // CR-02: guard against editor teardown while the dialog is open.
    juce::Component::SafePointer<OMarimbaAudioProcessorEditor> safe { this };
    fileChooser->launchAsync(chooserFlags, [safe](const juce::FileChooser& fc)
    {
        if (safe == nullptr) return;
        auto file = fc.getResult();
        if (file != juce::File{})
        {
            // Remember directory for next time (within session)
            safe->lastScalaExportPath = file.getParentDirectory().getFullPathName();

            // Generate Scala file content
            juce::String content = safe->processorRef.getTuningEngine().generateScalaFileContent();

            // Write to file
            if (file.replaceWithText(content))
            {
                juce::Logger::writeToLog("Saved Scala file: " + file.getFullPathName());
            }
            else
            {
                juce::Logger::writeToLog("Failed to save Scala file: " + file.getFullPathName());
            }
        }
    });

    return juce::var("OK");
}

// Save current keyboard mapping as .kbm file
juce::var OMarimbaAudioProcessorEditor::saveKBMFile(const juce::Array<juce::var>& args)
{
    juce::ignoreUnused(args);

    // Get last used directory or default to Documents
    auto lastDir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);

    // Check if we have a stored last directory (session memory)
    if (lastKBMExportPath.isNotEmpty())
    {
        auto storedDir = juce::File(lastKBMExportPath);
        if (storedDir.isDirectory())
            lastDir = storedDir;
    }

    fileChooser = std::make_unique<juce::FileChooser>(
        "Save Keyboard Mapping File",
        lastDir.getChildFile(processorRef.getTuningEngine().getActiveTuningName() + ".kbm"),
        "*.kbm"
    );

    auto chooserFlags = juce::FileBrowserComponent::saveMode
                      | juce::FileBrowserComponent::canSelectFiles
                      | juce::FileBrowserComponent::warnAboutOverwriting;

    // CR-02: guard against editor teardown while the dialog is open.
    juce::Component::SafePointer<OMarimbaAudioProcessorEditor> safe { this };
    fileChooser->launchAsync(chooserFlags, [safe](const juce::FileChooser& fc)
    {
        if (safe == nullptr) return;
        auto file = fc.getResult();
        if (file != juce::File{})
        {
            // Remember directory for next time (within session)
            safe->lastKBMExportPath = file.getParentDirectory().getFullPathName();

            // Generate KBM file content
            juce::String content = safe->processorRef.getTuningEngine().generateKBMFileContent();

            // Write to file
            if (file.replaceWithText(content))
            {
                juce::Logger::writeToLog("Saved KBM file: " + file.getFullPathName());
            }
            else
            {
                juce::Logger::writeToLog("Failed to save KBM file: " + file.getFullPathName());
            }
        }
    });

    return juce::var("OK");
}
