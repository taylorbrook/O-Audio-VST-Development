/*
   This file is part of O-Emulator, an Ouaricon Audio plugin.
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
#include "PluginEditor.h"
#include "BinaryData.h"

OEmulatorAudioProcessorEditor::OEmulatorAudioProcessorEditor(OEmulatorAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    // 1. Create relays FIRST (IDs verbatim from parameter-spec.md — BINDING)
    consoleRelay = std::make_unique<juce::WebComboBoxRelay>("console");
    crushRelay   = std::make_unique<juce::WebSliderRelay>("crush");
    ageRelay     = std::make_unique<juce::WebSliderRelay>("age");
    reverbRelay  = std::make_unique<juce::WebSliderRelay>("reverb");
    mixRelay     = std::make_unique<juce::WebSliderRelay>("mix");

    // 2. Build WebView options — zero native functions in v1.0; every control
    // rides a relay and the info readout is a static JS table.
    auto options =
        juce::WebBrowserComponent::Options{}
            .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
            .withWinWebView2Options(
                juce::WebBrowserComponent::Options::WinWebView2{}
                    .withUserDataFolder(juce::File::getSpecialLocation(
                        juce::File::SpecialLocationType::tempDirectory)
                            .getChildFile("OEmulator_WebView")))
            .withNativeIntegrationEnabled()
#if JUCE_WEB_BROWSER_RESOURCE_PROVIDER_AVAILABLE
            .withResourceProvider([this](const auto& url) { return getResource(url); })
#endif
            .withOptionsFrom(*consoleRelay)
            .withOptionsFrom(*crushRelay)
            .withOptionsFrom(*ageRelay)
            .withOptionsFrom(*reverbRelay)
            .withOptionsFrom(*mixRelay);

    webView = std::make_unique<juce::WebBrowserComponent>(options);

    addAndMakeVisible(*webView);

    // 3. Create attachments LAST (JUCE 8.0.14 three-arg form: param, relay, nullptr)
    consoleAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *audioProcessor.apvts.getParameter("console"), *consoleRelay, nullptr);
    crushAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("crush"), *crushRelay, nullptr);
    ageAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("age"), *ageRelay, nullptr);
    reverbAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("reverb"), *reverbRelay, nullptr);
    mixAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.apvts.getParameter("mix"), *mixRelay, nullptr);

    // Load UI from resource provider
#if JUCE_WEB_BROWSER_RESOURCE_PROVIDER_AVAILABLE
    webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
#endif

    // 620×430 fixed (RESEARCH §6 vertical budget), non-resizable.
    setSize(620, 430);
}

void OEmulatorAudioProcessorEditor::paint(juce::Graphics& g)
{
    // Aged-paper base tone behind the WebView while it loads
    g.fillAll(juce::Colour(0xfff5e6d3));
}

void OEmulatorAudioProcessorEditor::resized()
{
    webView->setBounds(getLocalBounds());
}

std::optional<juce::WebBrowserComponent::Resource>
OEmulatorAudioProcessorEditor::getResource(const juce::String& url)
{
    // Resource provider receives BARE paths ("/index.html"), never a scheme
    // (repo pattern: URL schemes differ per platform).
    auto makeVector = [](const char* data, int size) {
        return std::vector<std::byte>(
            reinterpret_cast<const std::byte*>(data),
            reinterpret_cast<const std::byte*>(data) + size);
    };

    if (url == "/" || url == "/index.html")
    {
        return juce::WebBrowserComponent::Resource{
            makeVector(BinaryData::index_html, BinaryData::index_htmlSize),
            juce::String("text/html")};
    }

    if (url == "/js/juce/index.js")
    {
        return juce::WebBrowserComponent::Resource{
            makeVector(BinaryData::index_js, BinaryData::index_jsSize),
            juce::String("application/javascript")};
    }

    if (url == "/js/juce/check_native_interop.js")
    {
        return juce::WebBrowserComponent::Resource{
            makeVector(BinaryData::check_native_interop_js, BinaryData::check_native_interop_jsSize),
            juce::String("application/javascript")};
    }

    if (url == "/img/paper.jpg")
    {
        return juce::WebBrowserComponent::Resource{
            makeVector(BinaryData::paper_jpg, BinaryData::paper_jpgSize),
            juce::String("image/jpeg")};
    }

    if (url == "/img/specimen.webp")
    {
        return juce::WebBrowserComponent::Resource{
            makeVector(BinaryData::specimen_webp, BinaryData::specimen_webpSize),
            juce::String("image/webp")};
    }

    juce::Logger::writeToLog("Resource not found: " + url);
    return std::nullopt;
}
