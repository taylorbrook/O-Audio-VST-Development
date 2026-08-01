/*
   This file is part of O-AnalogSaturation, an Ouaricon Audio plugin.
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

    O-AnalogSaturation - Editor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginEditor.h"
#include "BinaryData.h"

OAnalogSaturationAudioProcessorEditor::OAnalogSaturationAudioProcessorEditor(OAnalogSaturationAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    // Create relays (must be before WebView)
    intensityRelay = std::make_unique<juce::WebSliderRelay>("INTENSITY");
    modelRelay = std::make_unique<juce::WebSliderRelay>("MODEL");
    qualityRelay = std::make_unique<juce::WebSliderRelay>("QUALITY");
    autogainRelay = std::make_unique<juce::WebToggleButtonRelay>("AUTOGAIN");

    // Create WebView with native integration and resource provider
    webView = std::make_unique<juce::WebBrowserComponent>(
        juce::WebBrowserComponent::Options{}
            .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
            .withWinWebView2Options(
                juce::WebBrowserComponent::Options::WinWebView2{}
                    .withUserDataFolder(juce::File::getSpecialLocation(
                        juce::File::SpecialLocationType::tempDirectory)))
            .withNativeIntegrationEnabled()
            .withResourceProvider([this](const auto& url) { return getResource(url); })
            .withOptionsFrom(*intensityRelay)
            .withOptionsFrom(*modelRelay)
            .withOptionsFrom(*qualityRelay)
            .withOptionsFrom(*autogainRelay)
    );

    // Create parameter attachments (must be after WebView)
    intensityAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("INTENSITY"), *intensityRelay, nullptr);
    modelAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("MODEL"), *modelRelay, nullptr);
    qualityAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("QUALITY"), *qualityRelay, nullptr);
    autogainAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.parameters.getParameter("AUTOGAIN"), *autogainRelay, nullptr);

    addAndMakeVisible(*webView);
    webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());

    setSize(600, 450);
    startTimerHz(30);
}

OAnalogSaturationAudioProcessorEditor::~OAnalogSaturationAudioProcessorEditor()
{
    stopTimer();
}

void OAnalogSaturationAudioProcessorEditor::timerCallback()
{
    const float inputDB = processorRef.inputLevelDB.load(std::memory_order_relaxed);
    const float outputDB = processorRef.outputLevelDB.load(std::memory_order_relaxed);

    webView->emitEventIfBrowserIsVisible("inputLevel", inputDB);
    webView->emitEventIfBrowserIsVisible("outputLevel", outputDB);
}

void OAnalogSaturationAudioProcessorEditor::paint(juce::Graphics& g)
{
    juce::ignoreUnused(g);
}

void OAnalogSaturationAudioProcessorEditor::resized()
{
    webView->setBounds(getLocalBounds());
}

std::optional<juce::WebBrowserComponent::Resource>
OAnalogSaturationAudioProcessorEditor::getResource(const juce::String& url)
{
    auto makeResource = [](const char* data, int size, const char* mimeType) {
        return juce::WebBrowserComponent::Resource {
            std::vector<std::byte>(
                reinterpret_cast<const std::byte*>(data),
                reinterpret_cast<const std::byte*>(data) + size
            ),
            juce::String(mimeType)
        };
    };

    // HTML
    if (url == "/" || url == "/index.html" || url.isEmpty())
        return makeResource(BinaryData::index_html, BinaryData::index_htmlSize, "text/html");

    // JavaScript
    if (url == "/js/juce/index.js")
        return makeResource(BinaryData::index_js, BinaryData::index_jsSize, "text/javascript");

    if (url == "/js/juce/check_native_interop.js")
        return makeResource(BinaryData::check_native_interop_js, BinaryData::check_native_interop_jsSize, "text/javascript");

    // Images
    if (url == "/img/paper1.jpg")
        return makeResource(BinaryData::paper1_jpg, BinaryData::paper1_jpgSize, "image/jpeg");

    if (url == "/img/snake_mobot31753000317195_0068.png")
        return makeResource(BinaryData::snake_mobot31753000317195_0068_png, BinaryData::snake_mobot31753000317195_0068_pngSize, "image/png");

    if (url == "/img/snake_mobot31753000317195_0070.png")
        return makeResource(BinaryData::snake_mobot31753000317195_0070_png, BinaryData::snake_mobot31753000317195_0070_pngSize, "image/png");

    if (url == "/img/snake_NA_0145.png")
        return makeResource(BinaryData::snake_NA_0145_png, BinaryData::snake_NA_0145_pngSize, "image/png");

    if (url == "/img/snake_snakesaustralia00kref_0145.png")
        return makeResource(BinaryData::snake_snakesaustralia00kref_0145_png, BinaryData::snake_snakesaustralia00kref_0145_pngSize, "image/png");

    juce::Logger::writeToLog("Resource not found: " + url);
    return std::nullopt;
}
