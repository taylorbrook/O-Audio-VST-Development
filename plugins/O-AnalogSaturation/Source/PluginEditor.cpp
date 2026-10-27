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

    // v1.8.3 — WebView2 user-data folder in the per-user app-data tree (was bare
    // %TEMP%, which cleaners wipe and other processes can lock), created before
    // WebView2 sees it. DAW hosts deny WebView2's default location. The WinWebView2
    // options are ignored off Windows.
    const auto webView2DataFolder = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                                        .getChildFile ("Ouaricon")
                                        .getChildFile ("WebView2")
                                        .getChildFile ("O-AnalogSaturation");
   #if JUCE_WINDOWS
    webView2DataFolder.createDirectory();
   #endif

    // WebView options. Split out of the single chained expression it was
    // through v1.1.6 so v1.2.0's two native functions can be appended without
    // reformatting the relay chain above them.
    auto options = juce::WebBrowserComponent::Options{}
            .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
            .withWinWebView2Options(
                juce::WebBrowserComponent::Options::WinWebView2{}
                    .withUserDataFolder (webView2DataFolder))
            .withNativeIntegrationEnabled()
            .withResourceProvider([this](const auto& url) { return getResource(url); })
            .withOptionsFrom(*intensityRelay)
            .withOptionsFrom(*modelRelay)
            .withOptionsFrom(*qualityRelay)
            .withOptionsFrom(*autogainRelay);

    // ── v1.2.0: the UI LANGUAGE pair ───────────────────────────────────────
    //
    // Plain withNativeFunction, no relay. The page PULLS once at init; there is
    // no push from this constructor, no timer and no revision counter, because
    // the language is not preset content and no preset path can change it
    // behind the page's back. A push from here would race the WebView's load.
    options = options.withNativeFunction("getUiLanguage",
        [this](auto&, auto complete)
        {
            complete(juce::var(OAnalogSaturationAudioProcessor::languageCode(
                                   processorRef.uiLanguage.load(std::memory_order_acquire))));
        });

    options = options.withNativeFunction("setUiLanguage",
        [this](auto& args, auto complete)
        {
            // languageIndex() maps anything that is not one of the three
            // stored codes to 0, so an unexpected argument from the page
            // degrades to English rather than being stored unvalidated.
            if (args.size() > 0)
                processorRef.uiLanguage.store(
                    OAnalogSaturationAudioProcessor::languageIndex(args[0].toString()),
                    std::memory_order_release);

            complete(juce::var(OAnalogSaturationAudioProcessor::languageCode(
                                   processorRef.uiLanguage.load(std::memory_order_acquire))));
        });

    // Create WebView with native integration and resource provider
   #if JUCE_WINDOWS
    // v1.8.3 — when the WebView2 environment cannot be created (runtime missing or
    // data folder unusable), JUCE silently builds the legacy IE control, which has
    // no resource provider. Probe first; on failure webView stays null and paint()
    // shows a native notice. Audio, parameters and automation don't need the WebView.
    const bool webView2Available = juce::WebBrowserComponent::areOptionsSupported (options);
   #else
    constexpr bool webView2Available = true;
   #endif

    if (webView2Available)
        webView = std::make_unique<juce::WebBrowserComponent> (std::move(options));

    // Create parameter attachments (must be after WebView)
    intensityAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("INTENSITY"), *intensityRelay, nullptr);
    modelAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("MODEL"), *modelRelay, nullptr);
    qualityAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("QUALITY"), *qualityRelay, nullptr);
    autogainAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.parameters.getParameter("AUTOGAIN"), *autogainRelay, nullptr);

    // v1.8.3 — attachments stay valid without a WebView: the relays drop their
    // events while no browser is registered.
    if (webView != nullptr)
    {
        addAndMakeVisible (*webView);
    }
    else
    {
        webView2RuntimeLink = std::make_unique<juce::HyperlinkButton> (
            "Download the WebView2 Runtime",
            juce::URL ("https://developer.microsoft.com/microsoft-edge/webview2/"));
        webView2RuntimeLink->setColour (juce::HyperlinkButton::textColourId, juce::Colour (0xff3c5c1a));   // --leaf
        addAndMakeVisible (*webView2RuntimeLink);
    }
    if (webView != nullptr)
        webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());

    setSize(600, 450);

    // Drop whatever peak was held while no editor was open.
    processorRef.inputLevelDB.store(-100.0f, std::memory_order_relaxed);
    processorRef.outputLevelDB.store(-100.0f, std::memory_order_relaxed);
    startTimerHz(30);
}

OAnalogSaturationAudioProcessorEditor::~OAnalogSaturationAudioProcessorEditor()
{
    stopTimer();
}

void OAnalogSaturationAudioProcessorEditor::timerCallback()
{
    // v1.7.0: take-and-clear the peak held since the last tick.
    const float inputDB = processorRef.inputLevelDB.exchange(-100.0f, std::memory_order_relaxed);
    const float outputDB = processorRef.outputLevelDB.exchange(-100.0f, std::memory_order_relaxed);

    if (webView == nullptr)
        return;

    webView->emitEventIfBrowserIsVisible("inputLevel", inputDB);
    webView->emitEventIfBrowserIsVisible("outputLevel", outputDB);
}

void OAnalogSaturationAudioProcessorEditor::paint(juce::Graphics& g)
{
    // v1.8.3 — WebView2 could not start (Windows only; webView is null): paint a native
    // notice instead of JUCE's legacy-IE fallback page. Colours from the page CSS;
    // scaled down to fit short editors.
    if (webView == nullptr)
    {
        const float s = juce::jlimit (0.6f, 1.0f, (float) getHeight() / 260.0f);
        auto area = getLocalBounds().withSizeKeepingCentre (juce::jmin (640, getWidth() - 48),
                                                            juce::roundToInt (202.0f * s));
        g.fillAll (juce::Colour (0xfff5e6d3));   // --paper

        g.setColour (juce::Colour (0xff3c2f2f));   // --ink
        g.setFont (juce::FontOptions (26.0f * s, juce::Font::bold));
        g.drawFittedText ("O-AnalogSaturation needs Microsoft Edge WebView2",
                          area.removeFromTop (juce::roundToInt (40.0f * s)), juce::Justification::centred, 2);

        area.removeFromTop (juce::roundToInt (12.0f * s));
        g.setColour (juce::Colour (0xff3c2f2f));   // --ink
        g.setFont (juce::FontOptions (17.0f * s));
        g.drawFittedText ("The interface is drawn with the WebView2 Runtime, which could not start on "
                          "this computer. Install it, then close and reopen the plugin window.\n"
                          "The effect itself keeps working, and host automation still "
                          "reaches every parameter.",
                          area.removeFromTop (juce::roundToInt (110.0f * s)), juce::Justification::centred, 5);
        return;
    }

    juce::ignoreUnused(g);
}

void OAnalogSaturationAudioProcessorEditor::resized()
{
    if (webView != nullptr)
        webView->setBounds (getLocalBounds());

    if (webView2RuntimeLink != nullptr)
    {
        // Bottom row of the paint() notice block (same geometry).
        const float s = juce::jlimit (0.6f, 1.0f, (float) getHeight() / 260.0f);
        auto block = getLocalBounds().withSizeKeepingCentre (juce::jmin (640, getWidth() - 48),
                                                             juce::roundToInt (202.0f * s));
        webView2RuntimeLink->setFont (juce::FontOptions (17.0f * s, juce::Font::underlined), false);
        webView2RuntimeLink->setBounds (block.removeFromBottom (juce::roundToInt (32.0f * s))
                                             .withSizeKeepingCentre (320, juce::roundToInt (32.0f * s)));
    }
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
    // v1.2.0: the label table. EMBEDDED in CMakeLists.txt AND served here, in
    // the same commit. A file embedded but not served, or served but not
    // embedded, is a 404 that presents as a page stuck in English and nothing
    // else — check-i18n assertion 8 exists for exactly this pair.
    if (url == "/js/i18n.js")
        return makeResource(BinaryData::i18n_js, BinaryData::i18n_jsSize, "text/javascript");

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

    // v1.6.0 - shared EB Garamond face (modules/ui/eb-garamond, direct embed):
    // the stylesheet under css/, the three woff2 faces under fonts/ where its
    // relative url('../fonts/...') lands. BinaryData strips hyphens:
    // EBGaramond-Regular.woff2 -> EBGaramondRegular_woff2.
    if (url == "/css/eb-garamond.css")
        return makeResource(BinaryData::ebgaramond_css, BinaryData::ebgaramond_cssSize, "text/css; charset=utf-8");

    if (url == "/fonts/EBGaramond-Regular.woff2")
        return makeResource(BinaryData::EBGaramondRegular_woff2, BinaryData::EBGaramondRegular_woff2Size, "font/woff2");

    if (url == "/fonts/EBGaramond-Italic.woff2")
        return makeResource(BinaryData::EBGaramondItalic_woff2, BinaryData::EBGaramondItalic_woff2Size, "font/woff2");

    if (url == "/fonts/EBGaramond-Bold.woff2")
        return makeResource(BinaryData::EBGaramondBold_woff2, BinaryData::EBGaramondBold_woff2Size, "font/woff2");

    juce::Logger::writeToLog("Resource not found: " + url);
    return std::nullopt;
}
