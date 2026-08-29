/*
   This file is part of O-Gain, an Ouaricon Audio plugin.
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

    O-Gain - Editor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

    Stage 3 (GUI) - WebView UI Implementation

  ==============================================================================
*/

#include "PluginEditor.h"
#include "BinaryData.h"

//==============================================================================
OGainAudioProcessorEditor::OGainAudioProcessorEditor(OGainAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    //==========================================================================
    // CRITICAL: Initialization Order
    // 1. Create relays FIRST
    // 2. Create WebView with relay options
    // 3. Create attachments LAST
    //==========================================================================

    // 1. Create relays FIRST (no dependencies)
    // Float parameters
    gainOffsetRelay = std::make_unique<juce::WebSliderRelay>("gain_offset");
    trimRelay = std::make_unique<juce::WebSliderRelay>("trim");
    targetLevelRelay = std::make_unique<juce::WebSliderRelay>("target_level");
    // Choice parameters
    measurementModeRelay = std::make_unique<juce::WebComboBoxRelay>("measurement_mode");
    meterModeRelay = std::make_unique<juce::WebComboBoxRelay>("meter_mode");
    msModeRelay = std::make_unique<juce::WebComboBoxRelay>("ms_mode");
    // Bool parameters
    phaseInvertLRelay = std::make_unique<juce::WebToggleButtonRelay>("phase_invert_l");
    phaseInvertRRelay = std::make_unique<juce::WebToggleButtonRelay>("phase_invert_r");
    channelSwapRelay = std::make_unique<juce::WebToggleButtonRelay>("channel_swap");
    monoSumRelay = std::make_unique<juce::WebToggleButtonRelay>("mono_sum");

    // 2. Create WebView with relay options and native functions
    webView = std::make_unique<juce::WebBrowserComponent>(
        juce::WebBrowserComponent::Options{}
#if JUCE_WINDOWS
            // IN-04: webview2 backend + its options are Windows-only. Guarding
            // keeps the editor from referencing a Windows enum on macOS (WebKit
            // is always used there).
            .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
            .withWinWebView2Options(
                juce::WebBrowserComponent::Options::WinWebView2{}
                    .withUserDataFolder(juce::File::getSpecialLocation(
                        juce::File::SpecialLocationType::tempDirectory)
                        .getChildFile("OGain_WebView"))
                    .withStatusBarDisabled()
                    .withBuiltInErrorPageDisabled())
#endif
            .withNativeIntegrationEnabled()
            .withKeepPageLoadedWhenBrowserIsHidden()
#if JUCE_WEB_BROWSER_RESOURCE_PROVIDER_AVAILABLE
            .withResourceProvider([this](const auto& url) { return getResource(url); })
#endif
            // Register ALL relays
            .withOptionsFrom(*gainOffsetRelay)
            .withOptionsFrom(*trimRelay)
            .withOptionsFrom(*targetLevelRelay)
            .withOptionsFrom(*measurementModeRelay)
            .withOptionsFrom(*meterModeRelay)
            .withOptionsFrom(*msModeRelay)
            .withOptionsFrom(*phaseInvertLRelay)
            .withOptionsFrom(*phaseInvertRRelay)
            .withOptionsFrom(*channelSwapRelay)
            .withOptionsFrom(*monoSumRelay)
            // Native function: toggle learn mode. processBlock detects the
            // active-flag edge and drives the state transitions / finalize.
            .withNativeFunction("toggleLearn", [this](auto&, auto complete) {
                const bool newState = ! processorRef.learnActive.load();
                processorRef.learnActive.store(newState, std::memory_order_release);
                complete(newState);
            })

            //==================================================================
            // v1.3.0: the UI language pair.
            //
            // ONE-SHOT PULL, no revision counter and no poll. js/app.js calls
            // getUiLanguage() once from initI18n() after painting English
            // synchronously, so the page is never blank and never flashes. The
            // language is not preset content: nothing but this page can change
            // it, so there is nothing to re-poll for.
            //
            // THIS PLUGIN HAD NO NATIVE-FUNCTION BRIDGE BEYOND toggleLearn, so
            // this pair is new rather than joining an existing one. A missing
            // native function fails SILENTLY — Juce.getNativeFunction() returns
            // a callable that never settles, no page error fires, and the
            // selector appears to work until the session is reopened. Both
            // names below are grep-matched against js/app.js's
            // getNativeFunction calls, in BOTH directions: app.js asks for
            // exactly three (toggleLearn, getUiLanguage, setUiLanguage) and
            // this file registers exactly those three.
            //==================================================================
            .withNativeFunction("getUiLanguage", [this](const juce::Array<juce::var>&, auto complete) {
                complete(juce::var(OGainAudioProcessor::languageCode(
                    processorRef.getUiLanguageIndex())));
            })

            .withNativeFunction("setUiLanguage", [this](const juce::Array<juce::var>& args, auto complete) {
                // languageIndex() maps anything that is not "fr" to 0, so an
                // unexpected argument degrades to English rather than being
                // stored unvalidated.
                if (args.size() > 0)
                    processorRef.setUiLanguageIndex(
                        OGainAudioProcessor::languageIndex(args[0].toString()));

                complete(juce::var(OGainAudioProcessor::languageCode(
                    processorRef.getUiLanguageIndex())));
            })
    );

    addAndMakeVisible(*webView);

    // 3. Create attachments LAST (depend on relays and webView)
    // CRITICAL: JUCE 8 requires THREE parameters (parameter, relay, undoManager)
    // Float attachments
    gainOffsetAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("gain_offset"), *gainOffsetRelay, nullptr);
    trimAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("trim"), *trimRelay, nullptr);
    targetLevelAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("target_level"), *targetLevelRelay, nullptr);
    // Choice attachments
    measurementModeAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *processorRef.parameters.getParameter("measurement_mode"), *measurementModeRelay, nullptr);
    meterModeAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *processorRef.parameters.getParameter("meter_mode"), *meterModeRelay, nullptr);
    msModeAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *processorRef.parameters.getParameter("ms_mode"), *msModeRelay, nullptr);
    // Bool attachments
    phaseInvertLAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.parameters.getParameter("phase_invert_l"), *phaseInvertLRelay, nullptr);
    phaseInvertRAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.parameters.getParameter("phase_invert_r"), *phaseInvertRRelay, nullptr);
    channelSwapAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.parameters.getParameter("channel_swap"), *channelSwapRelay, nullptr);
    monoSumAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.parameters.getParameter("mono_sum"), *monoSumRelay, nullptr);

    // Load UI from resource provider
#if JUCE_WEB_BROWSER_RESOURCE_PROVIDER_AVAILABLE
    webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
#endif

    // Set window size (compact for mixer strip)
    setSize(350, 500);

    // Start meter update timer (30fps)
    startTimerHz(30);
}

OGainAudioProcessorEditor::~OGainAudioProcessorEditor()
{
    // Stop timer before destruction
    stopTimer();

    // Members destroyed in REVERSE order of declaration:
    // 1. Attachments destroyed FIRST (can safely call webView methods)
    // 2. WebView destroyed SECOND (attachments are gone)
    // 3. Relays destroyed LAST (nothing using them)
}

//==============================================================================
void OGainAudioProcessorEditor::paint(juce::Graphics& g)
{
    // WebView handles all rendering
    g.fillAll(juce::Colours::black);
}

void OGainAudioProcessorEditor::resized()
{
    // WebView fills entire editor window
    webView->setBounds(getLocalBounds());
}

void OGainAudioProcessorEditor::timerCallback()
{
    // Plain peak/RMS/VU meters are independent atomics (no cross-field coherence
    // needed). The Learn panel is read as ONE coherent snapshot via the seqlock
    // (WR-05) so state / confidence / integrated / peak can't tear across frames.
    const auto learn = processorRef.readLearnSnapshot();

    juce::String script = juce::String::formatted(
        "if (typeof updateMeters === 'function') { updateMeters({"
        "inputPeakL:%f, inputPeakR:%f, inputRmsL:%f, inputRmsR:%f,"
        "outputPeakL:%f, outputPeakR:%f, outputRmsL:%f, outputRmsR:%f,"
        "vuLevelL:%f, vuLevelR:%f,"
        "momentaryLUFS:%f, shortTermLUFS:%f, integratedLUFS:%f, samplePeakDBFS:%f,"
        "learnState:%d, learnElapsedSeconds:%f, learnConfidence:%d"
        "}); }",
        processorRef.inputPeakL.load(), processorRef.inputPeakR.load(),
        processorRef.inputRmsL.load(), processorRef.inputRmsR.load(),
        processorRef.outputPeakL.load(), processorRef.outputPeakR.load(),
        processorRef.outputRmsL.load(), processorRef.outputRmsR.load(),
        processorRef.vuLevelL.load(), processorRef.vuLevelR.load(),
        learn.momentaryLUFS, learn.shortTermLUFS,
        learn.integratedLUFS, learn.samplePeakDBFS,
        learn.state, learn.elapsedSeconds, learn.confidence
    );

    webView->evaluateJavascript(script, nullptr);
}

//==============================================================================
std::optional<juce::WebBrowserComponent::Resource>
OGainAudioProcessorEditor::getResource(const juce::String& url)
{
    // Helper to convert BinaryData to byte vector
    auto makeVector = [](const char* data, int size) {
        return std::vector<std::byte>(
            reinterpret_cast<const std::byte*>(data),
            reinterpret_cast<const std::byte*>(data) + size
        );
    };

    //==========================================================================
    // CRITICAL: Explicit URL Mapping
    // BinaryData flattens paths (index.js -> index_js)
    // HTML requests use original paths (./js/juce/index.js)
    // Must map manually with correct MIME types
    //
    // Resource provider receives bare PATHS (e.g. "/", "/index.html")
    // NOT full URLs. Compare directly against path strings.
    //==========================================================================

    if (url == "/" || url == "/index.html") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::index_html, BinaryData::index_htmlSize),
            juce::String("text/html")
        };
    }

    //==========================================================================
    // v1.3.0: the extracted controller and the i18n copy table.
    // FOUR PLACES, ONE COMMIT, TWICE OVER — each of the two new files needs the
    // file on disk, the CMake SOURCES entry, a branch HERE, and a reference
    // from the page (index.html's <script src> for app.js, app.js's import for
    // i18n.js). Miss one and the page 404s at runtime and presents as a dead
    // panel with no other symptom.
    //==========================================================================
    if (url == "/js/app.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::app_js, BinaryData::app_jsSize),
            juce::String("application/javascript")
        };
    }

    if (url == "/js/i18n.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::i18n_js, BinaryData::i18n_jsSize),
            juce::String("application/javascript")
        };
    }

    if (url == "/js/juce/index.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::index_js, BinaryData::index_jsSize),
            juce::String("application/javascript")
        };
    }

    if (url == "/js/juce/check_native_interop.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::check_native_interop_js, BinaryData::check_native_interop_jsSize),
            juce::String("application/javascript")
        };
    }

    if (url == "/images/paper1.jpg") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::paper1_jpg, BinaryData::paper1_jpgSize),
            juce::String("image/jpeg")
        };
    }

    if (url == "/images/shell_decoration.png") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::shell_decoration_png, BinaryData::shell_decoration_pngSize),
            juce::String("image/png")
        };
    }

    // 404 - Resource not found
    juce::Logger::writeToLog("O-Gain: Resource not found: " + url);
    return std::nullopt;
}
