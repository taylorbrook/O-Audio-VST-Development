/*
   This file is part of O-Freeze, an Ouaricon Audio plugin.
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

    O-Freeze - Editor Implementation
    Ouaricon Development
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginEditor.h"
#include "BinaryData.h"

OFreezeAudioProcessorEditor::OFreezeAudioProcessorEditor(OFreezeAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    // ============================================================================
    // 1. Create relays FIRST (with parameter IDs matching APVTS)
    // ============================================================================

    thresholdRelay = std::make_unique<juce::WebSliderRelay>("THRESHOLD");
    driftRelay = std::make_unique<juce::WebSliderRelay>("DRIFT");
    grainSizeRelay = std::make_unique<juce::WebSliderRelay>("GRAIN_SIZE");
    grainCountRelay = std::make_unique<juce::WebSliderRelay>("GRAIN_COUNT");
    mixRelay = std::make_unique<juce::WebSliderRelay>("MIX");
    detuneRelay = std::make_unique<juce::WebSliderRelay>("DETUNE");
    freezeRelay = std::make_unique<juce::WebToggleButtonRelay>("FREEZE");
    reverseRelay = std::make_unique<juce::WebToggleButtonRelay>("REVERSE");
    modeRelay = std::make_unique<juce::WebComboBoxRelay>("MODE");
    lfoRateRelay = std::make_unique<juce::WebSliderRelay>("LFO_RATE");
    lfoDepthRelay = std::make_unique<juce::WebSliderRelay>("LFO_DEPTH");
    lfoShapeRelay = std::make_unique<juce::WebComboBoxRelay>("LFO_SHAPE");

    // ============================================================================
    // 2. Create WebView SECOND with all relay options registered
    // ============================================================================

    // WebView options. Split out of the single chained expression it was
    // through v2.0.1 so v2.1.0's two native functions can be appended without
    // reformatting the relay chain above them.
    auto options = juce::WebBrowserComponent::Options{}
            .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
            .withWinWebView2Options(
                juce::WebBrowserComponent::Options::WinWebView2{}
                    .withUserDataFolder(juce::File::getSpecialLocation(
                        juce::File::SpecialLocationType::tempDirectory)))
            .withNativeIntegrationEnabled()
            .withResourceProvider([this](auto& url) { return getResource(url); })
            .withOptionsFrom(*thresholdRelay)
            .withOptionsFrom(*driftRelay)
            .withOptionsFrom(*grainSizeRelay)
            .withOptionsFrom(*grainCountRelay)
            .withOptionsFrom(*mixRelay)
            .withOptionsFrom(*detuneRelay)
            .withOptionsFrom(*freezeRelay)
            .withOptionsFrom(*reverseRelay)
            .withOptionsFrom(*modeRelay)
            .withOptionsFrom(*lfoRateRelay)
            .withOptionsFrom(*lfoDepthRelay)
            .withOptionsFrom(*lfoShapeRelay);

    // ── v2.1.0: the UI LANGUAGE pair ───────────────────────────────────────
    //
    // Plain withNativeFunction, no relay. The page PULLS once at init; there is
    // no push from this constructor, no timer and no revision counter, because
    // the language is not preset content and no preset path can change it
    // behind the page's back. A push from here would race the WebView's load.
    options = options.withNativeFunction("getUiLanguage",
        [this](auto&, auto complete)
        {
            complete(juce::var(OFreezeAudioProcessor::languageCode(
                                   processorRef.uiLanguage.load(std::memory_order_acquire))));
        });

    options = options.withNativeFunction("setUiLanguage",
        [this](auto& args, auto complete)
        {
            // languageIndex() maps anything that is not "fr" to 0, so an
            // unexpected argument from the page degrades to English rather than
            // being stored unvalidated.
            if (args.size() > 0)
                processorRef.uiLanguage.store(
                    OFreezeAudioProcessor::languageIndex(args[0].toString()),
                    std::memory_order_release);

            complete(juce::var(OFreezeAudioProcessor::languageCode(
                                   processorRef.uiLanguage.load(std::memory_order_acquire))));
        });

    webView = std::make_unique<juce::WebBrowserComponent>(std::move(options));

    // ============================================================================
    // 3. Create attachments LAST (connect parameters to relays)
    // ============================================================================

    thresholdAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.getAPVTS().getParameter("THRESHOLD"), *thresholdRelay, nullptr);
    driftAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.getAPVTS().getParameter("DRIFT"), *driftRelay, nullptr);
    grainSizeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.getAPVTS().getParameter("GRAIN_SIZE"), *grainSizeRelay, nullptr);
    grainCountAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.getAPVTS().getParameter("GRAIN_COUNT"), *grainCountRelay, nullptr);
    mixAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.getAPVTS().getParameter("MIX"), *mixRelay, nullptr);
    detuneAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.getAPVTS().getParameter("DETUNE"), *detuneRelay, nullptr);
    freezeAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.getAPVTS().getParameter("FREEZE"), *freezeRelay, nullptr);
    reverseAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.getAPVTS().getParameter("REVERSE"), *reverseRelay, nullptr);
    modeAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *processorRef.getAPVTS().getParameter("MODE"), *modeRelay, nullptr);
    lfoRateAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.getAPVTS().getParameter("LFO_RATE"), *lfoRateRelay, nullptr);
    lfoDepthAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.getAPVTS().getParameter("LFO_DEPTH"), *lfoDepthRelay, nullptr);
    lfoShapeAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *processorRef.getAPVTS().getParameter("LFO_SHAPE"), *lfoShapeRelay, nullptr);

    // ============================================================================
    // Add WebView to editor (navigation happens in parentHierarchyChanged)
    // ============================================================================

    addAndMakeVisible(*webView);

    // Set editor size (550x530 to accommodate 6 knobs + LFO group)
    setSize(550, 530);
}

OFreezeAudioProcessorEditor::~OFreezeAudioProcessorEditor()
{
}

void OFreezeAudioProcessorEditor::paint(juce::Graphics& g)
{
    // WebView handles all rendering
    juce::ignoreUnused(g);
}

void OFreezeAudioProcessorEditor::resized()
{
    // WebView fills entire editor
    webView->setBounds(getLocalBounds());
}

void OFreezeAudioProcessorEditor::parentHierarchyChanged()
{
    // Navigate WebView only after editor is attached to a window (JUCE 8 requirement)
    // This prevents crashes during plugin scanning when no window context exists
    if (isShowing() && webView != nullptr && !hasNavigated)
    {
        webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
        hasNavigated = true;
    }
}

std::optional<juce::WebBrowserComponent::Resource>
OFreezeAudioProcessorEditor::getResource(const juce::String& url)
{
    auto makeVector = [](const char* data, int size) {
        return std::vector<std::byte>(
            reinterpret_cast<const std::byte*>(data),
            reinterpret_cast<const std::byte*>(data) + size
        );
    };

    // Root "/" → index.html
    if (url == "/" || url == "/index.html") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::index_html, BinaryData::index_htmlSize),
            juce::String("text/html")
        };
    }

    // v2.1.0: the label table. EMBEDDED in CMakeLists.txt AND served here, in
    // the same commit. A file embedded but not served, or served but not
    // embedded, is a 404 that presents as a page stuck in English and nothing
    // else — check-i18n assertion 8 exists for exactly this pair.
    if (url == "/js/i18n.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::i18n_js, BinaryData::i18n_jsSize),
            juce::String("text/javascript")
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

    // Paper texture
    if (url == "/assets/paper1.jpg") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::paper1_jpg, BinaryData::paper1_jpgSize),
            juce::String("image/jpeg")
        };
    }

    // Anatomical overlay
    if (url == "/assets/muscles.png") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::muscles_png, BinaryData::muscles_pngSize),
            juce::String("image/png")
        };
    }

    // Resource not found
    juce::Logger::writeToLog("Resource not found: " + url);
    return std::nullopt;
}
