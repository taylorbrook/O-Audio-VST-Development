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
    engineRelay = std::make_unique<juce::WebComboBoxRelay>("ENGINE");          // v2.9.0
    diffusionRelay = std::make_unique<juce::WebSliderRelay>("DIFFUSION");      // v2.9.0
    avgFramesRelay = std::make_unique<juce::WebSliderRelay>("AVG_FRAMES");     // v2.9.0

    // ============================================================================
    // 2. Create WebView SECOND with all relay options registered
    // ============================================================================

    // WebView options. Split out of the single chained expression it was
    // through v2.0.1 so v2.1.0's two native functions can be appended without
    // reformatting the relay chain above them.
    //
    // v2.7.1 — WebView2 user-data folder in the per-user app-data tree (was bare
    // %TEMP%, which cleaners wipe and other processes can lock), created before
    // WebView2 sees it. DAW hosts deny WebView2's default location. The WinWebView2
    // options are ignored off Windows.
    const auto webView2DataFolder = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                                        .getChildFile ("Ouaricon")
                                        .getChildFile ("WebView2")
                                        .getChildFile ("O-Freeze");
   #if JUCE_WINDOWS
    webView2DataFolder.createDirectory();
   #endif

    auto options = juce::WebBrowserComponent::Options{}
            .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
            .withWinWebView2Options(
                juce::WebBrowserComponent::Options::WinWebView2{}
                    .withUserDataFolder (webView2DataFolder))
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
            .withOptionsFrom(*lfoShapeRelay)
            .withOptionsFrom(*engineRelay)
            .withOptionsFrom(*diffusionRelay)
            .withOptionsFrom(*avgFramesRelay);

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

   #if JUCE_WINDOWS
    // v2.7.1 — when the WebView2 environment cannot be created (runtime missing or
    // data folder unusable), JUCE silently builds the legacy IE control, which has
    // no resource provider. Probe first; on failure webView stays null and paint()
    // shows a native notice. Audio, parameters and automation don't need the WebView.
    const bool webView2Available = juce::WebBrowserComponent::areOptionsSupported (options);
   #else
    constexpr bool webView2Available = true;
   #endif

    if (webView2Available)
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
    engineAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *processorRef.getAPVTS().getParameter("ENGINE"), *engineRelay, nullptr);
    diffusionAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.getAPVTS().getParameter("DIFFUSION"), *diffusionRelay, nullptr);
    avgFramesAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.getAPVTS().getParameter("AVG_FRAMES"), *avgFramesRelay, nullptr);

    // ============================================================================
    // Add WebView to editor (navigation happens in parentHierarchyChanged)
    // ============================================================================

    // v2.7.1 — attachments stay valid without a WebView: the relays drop their
    // events while no browser is registered.
    if (webView != nullptr)
    {
        addAndMakeVisible(*webView);
    }
    else
    {
        webView2RuntimeLink = std::make_unique<juce::HyperlinkButton> (
            "Download the WebView2 Runtime",
            juce::URL ("https://developer.microsoft.com/microsoft-edge/webview2/"));
        webView2RuntimeLink->setColour (juce::HyperlinkButton::textColourId, juce::Colour (0xff4e6839));   // --leaf-fill
        addAndMakeVisible (*webView2RuntimeLink);
    }

    // Set editor size (550x530 to accommodate 6 knobs + LFO group)
    setSize(550, 530);
}

OFreezeAudioProcessorEditor::~OFreezeAudioProcessorEditor()
{
}

void OFreezeAudioProcessorEditor::paint(juce::Graphics& g)
{
    // v2.7.1 — WebView2 could not start (Windows only; webView is null): paint a native
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
        g.drawFittedText ("O-Freeze needs Microsoft Edge WebView2",
                          area.removeFromTop (juce::roundToInt (40.0f * s)), juce::Justification::centred, 2);

        area.removeFromTop (juce::roundToInt (12.0f * s));
        g.setColour (juce::Colour (0xff5c4033));   // --ink-2
        g.setFont (juce::FontOptions (17.0f * s));
        g.drawFittedText ("The interface is drawn with the WebView2 Runtime, which could not start on "
                          "this computer. Install it, then close and reopen the plugin window.\n"
                          "The effect itself keeps working, and host automation still "
                          "reaches every parameter.",
                          area.removeFromTop (juce::roundToInt (110.0f * s)), juce::Justification::centred, 5);
        return;
    }

    // WebView handles all rendering
    juce::ignoreUnused(g);
}

void OFreezeAudioProcessorEditor::resized()
{
    // WebView fills entire editor
    if (webView != nullptr)
        webView->setBounds(getLocalBounds());

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

    // v2.6.0 - shared EB Garamond face (modules/ui/eb-garamond, direct embed):
    // the stylesheet under css/, the three woff2 faces under fonts/ where its
    // relative url('../fonts/...') lands. BinaryData strips hyphens:
    // EBGaramond-Regular.woff2 -> EBGaramondRegular_woff2.
    if (url == "/css/eb-garamond.css") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::ebgaramond_css, BinaryData::ebgaramond_cssSize),
            juce::String("text/css; charset=utf-8")
        };
    }

    if (url == "/fonts/EBGaramond-Regular.woff2") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::EBGaramondRegular_woff2, BinaryData::EBGaramondRegular_woff2Size),
            juce::String("font/woff2")
        };
    }

    if (url == "/fonts/EBGaramond-Italic.woff2") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::EBGaramondItalic_woff2, BinaryData::EBGaramondItalic_woff2Size),
            juce::String("font/woff2")
        };
    }

    if (url == "/fonts/EBGaramond-Bold.woff2") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::EBGaramondBold_woff2, BinaryData::EBGaramondBold_woff2Size),
            juce::String("font/woff2")
        };
    }

    // Resource not found
    juce::Logger::writeToLog("Resource not found: " + url);
    return std::nullopt;
}
