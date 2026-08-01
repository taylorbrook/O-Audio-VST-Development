/*
   This file is part of O-Texture, an Ouaricon Audio plugin.
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

    O-Texture - Editor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginEditor.h"
#include "BinaryData.h"

TextureEditor::TextureEditor(TextureProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    // 1. Create relays (MUST match JS relay names exactly)
    xRelay          = std::make_unique<juce::WebSliderRelay>("xSlider");
    yRelay          = std::make_unique<juce::WebSliderRelay>("ySlider");
    characterARelay = std::make_unique<juce::WebSliderRelay>("characterASlider");
    characterBRelay = std::make_unique<juce::WebSliderRelay>("characterBSlider");
    evolveRelay     = std::make_unique<juce::WebSliderRelay>("evolveSlider");
    brightnessRelay = std::make_unique<juce::WebSliderRelay>("brightnessSlider");
    mixRelay        = std::make_unique<juce::WebSliderRelay>("mixSlider");
    sourceRelay     = std::make_unique<juce::WebComboBoxRelay>("sourceCombo");
    modeRelay       = std::make_unique<juce::WebComboBoxRelay>("modeCombo");
    freezeRelay     = std::make_unique<juce::WebToggleButtonRelay>("freezeToggle");

    // 2. Create WebView with all relay options
    webView = std::make_unique<juce::WebBrowserComponent>(
        juce::WebBrowserComponent::Options{}
            .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
            .withWinWebView2Options(
                juce::WebBrowserComponent::Options::WinWebView2{}
                    .withUserDataFolder(juce::File::getSpecialLocation(
                        juce::File::SpecialLocationType::tempDirectory)
                            .getChildFile("OTexture_WebView"))
                    .withStatusBarDisabled()
                    .withBuiltInErrorPageDisabled())
            .withNativeIntegrationEnabled()
            .withKeepPageLoadedWhenBrowserIsHidden()
#if JUCE_WEB_BROWSER_RESOURCE_PROVIDER_AVAILABLE
            .withResourceProvider([this](const auto& url) { return getResource(url); })
#endif
            // Register ALL relays (dereference unique_ptr)
            .withOptionsFrom(*xRelay)
            .withOptionsFrom(*yRelay)
            .withOptionsFrom(*characterARelay)
            .withOptionsFrom(*characterBRelay)
            .withOptionsFrom(*evolveRelay)
            .withOptionsFrom(*brightnessRelay)
            .withOptionsFrom(*mixRelay)
            .withOptionsFrom(*sourceRelay)
            .withOptionsFrom(*modeRelay)
            .withOptionsFrom(*freezeRelay)
    );

    addAndMakeVisible(*webView);

    // 3. Create attachments (MUST match APVTS parameter IDs exactly)
    xAttachment          = std::make_unique<juce::WebSliderParameterAttachment>(*processorRef.parameters.getParameter("X"), *xRelay);
    yAttachment          = std::make_unique<juce::WebSliderParameterAttachment>(*processorRef.parameters.getParameter("Y"), *yRelay);
    characterAAttachment = std::make_unique<juce::WebSliderParameterAttachment>(*processorRef.parameters.getParameter("CHARACTER_A"), *characterARelay);
    characterBAttachment = std::make_unique<juce::WebSliderParameterAttachment>(*processorRef.parameters.getParameter("CHARACTER_B"), *characterBRelay);
    evolveAttachment     = std::make_unique<juce::WebSliderParameterAttachment>(*processorRef.parameters.getParameter("EVOLVE"), *evolveRelay);
    brightnessAttachment = std::make_unique<juce::WebSliderParameterAttachment>(*processorRef.parameters.getParameter("BRIGHTNESS"), *brightnessRelay);
    mixAttachment        = std::make_unique<juce::WebSliderParameterAttachment>(*processorRef.parameters.getParameter("MIX"), *mixRelay);
    sourceAttachment     = std::make_unique<juce::WebComboBoxParameterAttachment>(*processorRef.parameters.getParameter("SOURCE"), *sourceRelay);
    modeAttachment       = std::make_unique<juce::WebComboBoxParameterAttachment>(*processorRef.parameters.getParameter("MODE"), *modeRelay);
    freezeAttachment     = std::make_unique<juce::WebToggleButtonParameterAttachment>(*processorRef.parameters.getParameter("FREEZE"), *freezeRelay);

#if JUCE_WEB_BROWSER_RESOURCE_PROVIDER_AVAILABLE
    webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
#endif

    setSize(800, 600);
}

TextureEditor::~TextureEditor()
{
    // Destroy in REVERSE: attachments first, then webView, then relays
    freezeAttachment.reset();
    modeAttachment.reset();
    sourceAttachment.reset();
    mixAttachment.reset();
    brightnessAttachment.reset();
    evolveAttachment.reset();
    characterBAttachment.reset();
    characterAAttachment.reset();
    yAttachment.reset();
    xAttachment.reset();

    webView.reset();

    freezeRelay.reset();
    modeRelay.reset();
    sourceRelay.reset();
    mixRelay.reset();
    brightnessRelay.reset();
    evolveRelay.reset();
    characterBRelay.reset();
    characterARelay.reset();
    yRelay.reset();
    xRelay.reset();
}

void TextureEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1a1a2e));
}

void TextureEditor::resized()
{
    webView->setBounds(getLocalBounds());
}

std::optional<juce::WebBrowserComponent::Resource>
TextureEditor::getResource(const juce::String& url)
{
    auto makeVector = [](const char* data, int size) {
        return std::vector<std::byte>(
            reinterpret_cast<const std::byte*>(data),
            reinterpret_cast<const std::byte*>(data) + size);
    };

    // Root
    if (url == "/" || url == "/index.html")
    {
        return juce::WebBrowserComponent::Resource{
            makeVector(BinaryData::index_html, BinaryData::index_htmlSize),
            juce::String("text/html")};
    }

    // CSS
    if (url == "/css/ouaricon-naturalist.css")
    {
        return juce::WebBrowserComponent::Resource{
            makeVector(BinaryData::ouariconnaturalist_css, BinaryData::ouariconnaturalist_cssSize),
            juce::String("text/css")};
    }

    // JavaScript - JUCE frontend
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

    // JavaScript - app logic
    if (url == "/js/main.js")
    {
        return juce::WebBrowserComponent::Resource{
            makeVector(BinaryData::main_js, BinaryData::main_jsSize),
            juce::String("application/javascript")};
    }

    // Images
    if (url == "/img/fern.png")
    {
        return juce::WebBrowserComponent::Resource{
            makeVector(BinaryData::fern_png, BinaryData::fern_pngSize),
            juce::String("image/png")};
    }

    juce::Logger::writeToLog("Resource not found: " + url);
    return std::nullopt;
}
