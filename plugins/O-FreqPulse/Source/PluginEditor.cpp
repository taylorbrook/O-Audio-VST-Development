/*
   This file is part of O-FreqPulse, an Ouaricon Audio plugin.
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

    O-FreqPulse - Editor Implementation
    Ouaricon Development
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginEditor.h"
#include "BinaryData.h"

OFreqPulseAudioProcessorEditor::OFreqPulseAudioProcessorEditor(OFreqPulseAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    // 1️⃣ CREATE RELAYS FIRST
    // Global parameter relays (5 total)
    mixRelay = std::make_unique<juce::WebSliderRelay>("mix");
    stepsRelay = std::make_unique<juce::WebSliderRelay>("steps");
    rateRelay = std::make_unique<juce::WebComboBoxRelay>("rate");
    swingRelay = std::make_unique<juce::WebSliderRelay>("swing");
    attackRelay = std::make_unique<juce::WebSliderRelay>("attack");
    releaseRelay = std::make_unique<juce::WebSliderRelay>("release");

    // Crossover parameter relays (3 total)
    crossover1Relay = std::make_unique<juce::WebSliderRelay>("crossover_1");
    crossover2Relay = std::make_unique<juce::WebSliderRelay>("crossover_2");
    crossover3Relay = std::make_unique<juce::WebSliderRelay>("crossover_3");

    // Frequency boundary relays (2 total)
    freqLowRelay = std::make_unique<juce::WebSliderRelay>("freq_low");
    freqHighRelay = std::make_unique<juce::WebSliderRelay>("freq_high");

    // Per-band parameter relays (24 total: 6 params x 4 bands)
    for (int i = 0; i < 4; ++i)
    {
        auto prefix = "band" + juce::String(i) + "_";
        bandRelays[i].enable    = std::make_unique<juce::WebToggleButtonRelay>(prefix + "enable");
        bandRelays[i].depth     = std::make_unique<juce::WebSliderRelay>(prefix + "depth");
        bandRelays[i].rate      = std::make_unique<juce::WebComboBoxRelay>(prefix + "rate");
        bandRelays[i].eucOn     = std::make_unique<juce::WebToggleButtonRelay>(prefix + "euc_on");
        bandRelays[i].eucSteps  = std::make_unique<juce::WebSliderRelay>(prefix + "euc_steps");
        bandRelays[i].eucPulses = std::make_unique<juce::WebSliderRelay>(prefix + "euc_pulses");
        bandRelays[i].eucOffset = std::make_unique<juce::WebSliderRelay>(prefix + "euc_offset");
        bandRelays[i].phaseOffset = std::make_unique<juce::WebSliderRelay>(prefix + "phase_offset");
        bandRelays[i].bandSteps = std::make_unique<juce::WebSliderRelay>(prefix + "steps");
    }

    // Step grid relays (128 total: 32 steps × 4 bands) — velocity sliders
    for (int band = 0; band < 4; ++band)
    {
        for (int step = 0; step < 32; ++step)
        {
            int index = band * 32 + step;
            juce::String paramId = "step_b" + juce::String(band) + "_s" + juce::String(step);
            stepRelays[index] = std::make_unique<juce::WebSliderRelay>(paramId);
        }
    }

    // 2️⃣ CREATE WEBVIEW WITH OPTIONS
    auto options = juce::WebBrowserComponent::Options{}
        .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
        .withWinWebView2Options(
            juce::WebBrowserComponent::Options::WinWebView2{}
                .withUserDataFolder(juce::File::getSpecialLocation(
                    juce::File::SpecialLocationType::tempDirectory)
                        .getChildFile("OFreqPulse_WebView")))
        .withNativeIntegrationEnabled()
        .withResourceProvider([this](const auto& url) { return getResource(url); })
        // Global parameter relays
        .withOptionsFrom(*mixRelay)
        .withOptionsFrom(*stepsRelay)
        .withOptionsFrom(*rateRelay)
        .withOptionsFrom(*swingRelay)
        .withOptionsFrom(*attackRelay)
        .withOptionsFrom(*releaseRelay)
        // Crossover relays
        .withOptionsFrom(*crossover1Relay)
        .withOptionsFrom(*crossover2Relay)
        .withOptionsFrom(*crossover3Relay)
        // Frequency boundary relays
        .withOptionsFrom(*freqLowRelay)
        .withOptionsFrom(*freqHighRelay);

    // Per-band relays (loop)
    for (int i = 0; i < 4; ++i)
    {
        options = options
            .withOptionsFrom(*bandRelays[i].enable)
            .withOptionsFrom(*bandRelays[i].depth)
            .withOptionsFrom(*bandRelays[i].rate)
            .withOptionsFrom(*bandRelays[i].eucOn)
            .withOptionsFrom(*bandRelays[i].eucSteps)
            .withOptionsFrom(*bandRelays[i].eucPulses)
            .withOptionsFrom(*bandRelays[i].eucOffset)
            .withOptionsFrom(*bandRelays[i].phaseOffset)
            .withOptionsFrom(*bandRelays[i].bandSteps);
    }

    options = options
        // v1.5.0: Tooltip state native functions
        .withNativeFunction("setTooltipsEnabled", [this](const juce::Array<juce::var>& args,
                                                          std::function<void(juce::var)> complete) {
            if (args.isEmpty()) { complete(juce::var(false)); return; }
            bool enabled = static_cast<bool>(args[0]);
            processorRef.setTooltipsEnabled(enabled);
            complete(juce::var(true));
        })
        // WR-01: getter so the WebView can PULL the persisted tooltip state once its JS is
        // ready (replaces the racy one-shot 30 Hz timer push, which fired before the JS restore
        // hook existed on a cold WebView start and never retried).
        .withNativeFunction("getTooltipsEnabled", [this](const juce::Array<juce::var>&,
                                                         std::function<void(juce::var)> complete) {
            complete(juce::var(processorRef.getTooltipsEnabled()));
        })
        // v1.18.0: the UI language. PULLED once by the page at init, never
        // pushed — a push from this constructor or from a timer tick fires
        // before the page module has evaluated, so the stored preference would
        // silently never arrive. That is the same WR-01 lesson the tooltip
        // getter above already records. No revision counter and no poll: the
        // language is not preset content, and OuariconPresetManager::loadPreset
        // walks only preset["parameters"] and never touches a state-tree
        // property, so nothing but this page can change it.
        .withNativeFunction("getUiLanguage", [this](const juce::Array<juce::var>&,
                                                     std::function<void(juce::var)> complete) {
            complete(juce::var(OFreqPulseAudioProcessor::languageCode(
                processorRef.getUiLanguageIndex())));
        })
        .withNativeFunction("setUiLanguage", [this](const juce::Array<juce::var>& args,
                                                     std::function<void(juce::var)> complete) {
            // languageIndex() maps anything that is not "fr" to 0, so an
            // unexpected argument degrades to English rather than being stored
            // unvalidated.
            if (! args.isEmpty())
                processorRef.setUiLanguageIndex(
                    OFreqPulseAudioProcessor::languageIndex(args[0].toString()));

            complete(juce::var(OFreqPulseAudioProcessor::languageCode(
                processorRef.getUiLanguageIndex())));
        })
        // v1.6.0: Preset Manager native functions
        .withNativeFunction("savePreset", [this](const juce::Array<juce::var>& args,
                                                  std::function<void(juce::var)> complete) {
            if (args.size() > 0)
                complete(processorRef.presetManager.savePreset(args[0].toString()));
            else
                complete(false);
        })
        .withNativeFunction("loadPreset", [this](const juce::Array<juce::var>& args,
                                                  std::function<void(juce::var)> complete) {
            if (args.size() > 0)
                complete(processorRef.presetManager.loadPreset(args[0].toString()));
            else
                complete(false);
        })
        .withNativeFunction("getPresetList", [this](const juce::Array<juce::var>&,
                                                     std::function<void(juce::var)> complete) {
            auto list = processorRef.presetManager.getPresetList();
            juce::Array<juce::var> arr;
            for (const auto& name : list)
                arr.add(name);
            complete(juce::var(arr));
        })
        .withNativeFunction("getCurrentPreset", [this](const juce::Array<juce::var>&,
                                                        std::function<void(juce::var)> complete) {
            complete(processorRef.presetManager.getCurrentPresetName());
        })
        .withNativeFunction("selectNextPreset", [this](const juce::Array<juce::var>&,
                                                        std::function<void(juce::var)> complete) {
            auto next = processorRef.presetManager.getNextPreset();
            complete(next);
        })
        .withNativeFunction("selectPreviousPreset", [this](const juce::Array<juce::var>&,
                                                            std::function<void(juce::var)> complete) {
            auto prev = processorRef.presetManager.getPreviousPreset();
            complete(prev);
        })
        .withNativeFunction("deletePreset", [this](const juce::Array<juce::var>& args,
                                                    std::function<void(juce::var)> complete) {
            if (args.size() > 0)
                complete(processorRef.presetManager.deletePreset(args[0].toString()));
            else
                complete(false);
        })
        .withNativeFunction("isFactoryPreset", [this](const juce::Array<juce::var>& args,
                                                       std::function<void(juce::var)> complete) {
            if (args.size() > 0)
                complete(processorRef.presetManager.isFactoryPreset(args[0].toString()));
            else
                complete(false);
        })
        .withNativeFunction("savePresetWithDialog", [this](const juce::Array<juce::var>&,
                                                            std::function<void(juce::var)> complete) {
            fileChooser = std::make_unique<juce::FileChooser>(
                "Save Preset",
                processorRef.presetManager.getUserPresetsDirectory(),
                "*.json"
            );
            fileChooser->launchAsync(
                juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                [this, complete](const juce::FileChooser& fc) {
                    auto results = fc.getResults();
                    if (results.isEmpty()) {
                        auto* result = new juce::DynamicObject();
                        result->setProperty("success", false);
                        result->setProperty("name", "");
                        complete(juce::var(result));
                        return;
                    }
                    auto file = results.getFirst();
                    auto presetName = file.getFileNameWithoutExtension();
                    // WR-03: honor the directory the user navigated to (symmetric with the load
                    // dialog, which uses the chosen path). savePresetToFile writes to that exact
                    // path; when it's the default user presets dir the preset still appears in the
                    // preset-bar list. Previously savePreset(name) always wrote to the user dir,
                    // silently discarding the chosen location.
                    bool success = processorRef.presetManager.savePresetToFile(file);
                    auto* result = new juce::DynamicObject();
                    result->setProperty("success", success);
                    result->setProperty("name", success ? presetName : juce::String());
                    complete(juce::var(result));
                }
            );
        })
        .withNativeFunction("loadPresetFromFile", [this](const juce::Array<juce::var>&,
                                                          std::function<void(juce::var)> complete) {
            fileChooser = std::make_unique<juce::FileChooser>(
                "Load Preset",
                processorRef.presetManager.getUserPresetsDirectory(),
                "*.json"
            );
            fileChooser->launchAsync(
                juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                [this, complete](const juce::FileChooser& fc) {
                    auto results = fc.getResults();
                    if (results.isEmpty()) {
                        auto* result = new juce::DynamicObject();
                        result->setProperty("success", false);
                        result->setProperty("name", "");
                        complete(juce::var(result));
                        return;
                    }
                    auto file = results.getFirst();
                    bool success = processorRef.presetManager.loadPresetFromFile(file);
                    auto* result = new juce::DynamicObject();
                    result->setProperty("success", success);
                    result->setProperty("name", success ? file.getFileNameWithoutExtension() : juce::String());
                    complete(juce::var(result));
                }
            );
        })
        .withNativeFunction("getPluginVersion", [](const juce::Array<juce::var>&,
                                                    std::function<void(juce::var)> complete) {
            complete(juce::var(JucePlugin_VersionString));
        });

    // Register all 128 step grid relays
    for (int i = 0; i < 128; ++i)
    {
        options = options.withOptionsFrom(*stepRelays[i]);
    }

    webView = std::make_unique<juce::WebBrowserComponent>(options);

    // 3️⃣ CREATE ATTACHMENTS LAST
    auto& apvts = processorRef.getAPVTS();

    // Global parameter attachments
    mixAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("mix"), *mixRelay, nullptr);
    stepsAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("steps"), *stepsRelay, nullptr);
    rateAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *apvts.getParameter("rate"), *rateRelay, nullptr);
    swingAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("swing"), *swingRelay, nullptr);
    attackAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("attack"), *attackRelay, nullptr);
    releaseAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("release"), *releaseRelay, nullptr);

    // Crossover attachments
    crossover1Attachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("crossover_1"), *crossover1Relay, nullptr);
    crossover2Attachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("crossover_2"), *crossover2Relay, nullptr);
    crossover3Attachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("crossover_3"), *crossover3Relay, nullptr);

    // Frequency boundary attachments
    freqLowAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("freq_low"), *freqLowRelay, nullptr);
    freqHighAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("freq_high"), *freqHighRelay, nullptr);

    // Per-band attachments (loop)
    for (int i = 0; i < 4; ++i)
    {
        auto prefix = "band" + juce::String(i) + "_";
        bandAttachments[i].enable = std::make_unique<juce::WebToggleButtonParameterAttachment>(
            *apvts.getParameter(prefix + "enable"), *bandRelays[i].enable, nullptr);
        bandAttachments[i].depth = std::make_unique<juce::WebSliderParameterAttachment>(
            *apvts.getParameter(prefix + "depth"), *bandRelays[i].depth, nullptr);
        bandAttachments[i].rate = std::make_unique<juce::WebComboBoxParameterAttachment>(
            *apvts.getParameter(prefix + "rate"), *bandRelays[i].rate, nullptr);
        bandAttachments[i].eucOn = std::make_unique<juce::WebToggleButtonParameterAttachment>(
            *apvts.getParameter(prefix + "euc_on"), *bandRelays[i].eucOn, nullptr);
        bandAttachments[i].eucSteps = std::make_unique<juce::WebSliderParameterAttachment>(
            *apvts.getParameter(prefix + "euc_steps"), *bandRelays[i].eucSteps, nullptr);
        bandAttachments[i].eucPulses = std::make_unique<juce::WebSliderParameterAttachment>(
            *apvts.getParameter(prefix + "euc_pulses"), *bandRelays[i].eucPulses, nullptr);
        bandAttachments[i].eucOffset = std::make_unique<juce::WebSliderParameterAttachment>(
            *apvts.getParameter(prefix + "euc_offset"), *bandRelays[i].eucOffset, nullptr);
        bandAttachments[i].phaseOffset = std::make_unique<juce::WebSliderParameterAttachment>(
            *apvts.getParameter(prefix + "phase_offset"), *bandRelays[i].phaseOffset, nullptr);
        bandAttachments[i].bandSteps = std::make_unique<juce::WebSliderParameterAttachment>(
            *apvts.getParameter(prefix + "steps"), *bandRelays[i].bandSteps, nullptr);
    }

    // Step grid attachments (128 total) — velocity sliders
    for (int band = 0; band < 4; ++band)
    {
        for (int step = 0; step < 32; ++step)
        {
            int index = band * 32 + step;
            juce::String paramId = "step_b" + juce::String(band) + "_s" + juce::String(step);
            stepAttachments[index] = std::make_unique<juce::WebSliderParameterAttachment>(
                *apvts.getParameter(paramId), *stepRelays[index], nullptr);
        }
    }

    // Add WebView to editor
    addAndMakeVisible(*webView);

    // Navigate to UI
    webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());

    // Set editor size (850×550 from plan)
    setSize(850, 550);

    // Start playhead update timer (30 Hz for smooth animation)
    startTimerHz(30);
}

OFreqPulseAudioProcessorEditor::~OFreqPulseAudioProcessorEditor()
{
    stopTimer();
}

void OFreqPulseAudioProcessorEditor::timerCallback()
{
    // Read per-band step positions and signal state from processor (atomic, thread-safe)
    bool hasSignal = processorRef.getHasAudioSignal();
    int b0 = processorRef.getBandStep(0);
    int b1 = processorRef.getBandStep(1);
    int b2 = processorRef.getBandStep(2);
    int b3 = processorRef.getBandStep(3);

    // Send per-band steps to WebView for independent playhead highlighting
    juce::String js = juce::String::formatted(
        "if (window.updatePlayhead) window.updatePlayhead(%d,%d,%d,%d,%s);",
        b0, b1, b2, b3, hasSignal ? "true" : "false"
    );
    webView->evaluateJavascript(js);

    // WR-01: tooltip state is now PULLED by the WebView via the getTooltipsEnabled native
    // function once its JS is ready (see initializeTooltips() in app.js), so the old racy
    // one-shot push from here has been removed.
}

void OFreqPulseAudioProcessorEditor::resized()
{
    // WebView fills entire editor
    if (webView)
        webView->setBounds(getLocalBounds());
}

std::optional<juce::WebBrowserComponent::Resource>
OFreqPulseAudioProcessorEditor::getResource(const juce::String& url)
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

    if (url == "/js/app.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::app_js, BinaryData::app_jsSize),
            juce::String("text/javascript")
        };
    }

    // v1.18.0: the i18n table. Embedded in CMakeLists.txt's
    // juce_add_binary_data SOURCES, served here, and imported by js/app.js —
    // all four places or the page 404s at runtime and presents as a dead panel
    // with no other symptom (check-i18n assertion 8).
    if (url == "/js/i18n.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::i18n_js, BinaryData::i18n_jsSize),
            juce::String("text/javascript")
        };
    }

    if (url == "/css/styles.css") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::styles_css, BinaryData::styles_cssSize),
            juce::String("text/css")
        };
    }

    // v1.6.0: Preset Manager JS module
    if (url == "/modules/preset-manager.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::presetmanager_js, BinaryData::presetmanager_jsSize),
            juce::String("text/javascript")
        };
    }

    // Resource not found
    juce::Logger::writeToLog("O-FreqPulse: Resource not found: " + url);
    return std::nullopt;
}
