/*
   This file is part of O-Detune, an Ouaricon Audio plugin.
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

    O-Detune - Editor Implementation
    Ouaricon Development
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginEditor.h"
#include "BinaryData.h"

ODetuneAudioProcessorEditor::ODetuneAudioProcessorEditor(ODetuneAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    // ============================================================================
    // 1. Create relays FIRST (with parameter IDs matching APVTS and HTML)
    // ============================================================================

    // Float parameter relays
    blendRelay = std::make_unique<juce::WebSliderRelay>("blend");
    wobbleRateRelay = std::make_unique<juce::WebSliderRelay>("wobble_rate");
    wobbleDepthRelay = std::make_unique<juce::WebSliderRelay>("wobble_depth");
    unisonDetuneRelay = std::make_unique<juce::WebSliderRelay>("unison_detune");
    unisonSpreadRelay = std::make_unique<juce::WebSliderRelay>("unison_spread");
    widthRelay = std::make_unique<juce::WebSliderRelay>("width");
    mixRelay = std::make_unique<juce::WebSliderRelay>("mix");
    focusLowRelay = std::make_unique<juce::WebSliderRelay>("focus_low");
    focusHighRelay = std::make_unique<juce::WebSliderRelay>("focus_high");
    delayRelay = std::make_unique<juce::WebSliderRelay>("delay");
    feedbackRelay = std::make_unique<juce::WebSliderRelay>("feedback");
    randomAmtRelay = std::make_unique<juce::WebSliderRelay>("random_amt");

    // Choice parameter relays
    wobbleEraRelay = std::make_unique<juce::WebComboBoxRelay>("wobble_era");
    wobbleShapeRelay = std::make_unique<juce::WebComboBoxRelay>("wobble_shape");
    unisonVoicesRelay = std::make_unique<juce::WebComboBoxRelay>("unison_voices");
    unisonDistRelay = std::make_unique<juce::WebComboBoxRelay>("unison_dist");

    // Bool parameter relays
    wobbleSyncRelay = std::make_unique<juce::WebToggleButtonRelay>("wobble_sync");
    monoSafeRelay = std::make_unique<juce::WebToggleButtonRelay>("mono_safe");

    // ============================================================================
    // 2. Create WebView SECOND with all relay options registered
    // ============================================================================

    // v1.10.2 — WebView2 user-data folder in the per-user app-data tree (was bare
    // %TEMP%, which cleaners wipe and other processes can lock), created before
    // WebView2 sees it. DAW hosts deny WebView2's default location. The WinWebView2
    // options are ignored off Windows.
    const auto webView2DataFolder = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                                        .getChildFile ("Ouaricon")
                                        .getChildFile ("WebView2")
                                        .getChildFile ("O-Detune");
   #if JUCE_WINDOWS
    webView2DataFolder.createDirectory();
   #endif

    const auto options = juce::WebBrowserComponent::Options{}
            .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
            .withWinWebView2Options(
                juce::WebBrowserComponent::Options::WinWebView2{}
                    .withUserDataFolder (webView2DataFolder))
            .withNativeIntegrationEnabled()
            .withResourceProvider([this](auto& url) { return getResource(url); })
            // Register all float parameter relays
            .withOptionsFrom(*blendRelay)
            .withOptionsFrom(*wobbleRateRelay)
            .withOptionsFrom(*wobbleDepthRelay)
            .withOptionsFrom(*unisonDetuneRelay)
            .withOptionsFrom(*unisonSpreadRelay)
            .withOptionsFrom(*widthRelay)
            .withOptionsFrom(*mixRelay)
            .withOptionsFrom(*focusLowRelay)
            .withOptionsFrom(*focusHighRelay)
            .withOptionsFrom(*delayRelay)
            .withOptionsFrom(*feedbackRelay)
            .withOptionsFrom(*randomAmtRelay)
            // Register all choice parameter relays
            .withOptionsFrom(*wobbleEraRelay)
            .withOptionsFrom(*wobbleShapeRelay)
            .withOptionsFrom(*unisonVoicesRelay)
            .withOptionsFrom(*unisonDistRelay)
            // Register all bool parameter relays
            .withOptionsFrom(*wobbleSyncRelay)
            .withOptionsFrom(*monoSafeRelay)
            // Preset Manager native functions
            .withNativeFunction("savePreset", [this](auto& args, auto complete) {
                if (args.size() > 0)
                    complete(processorRef.presetManager.savePreset(args[0].toString()));
                else
                    complete(false);
            })
            .withNativeFunction("savePresetWithDialog", [this](auto&, auto complete) {
                fileChooser = std::make_unique<juce::FileChooser>(
                    "Save Preset",
                    processorRef.presetManager.getUserPresetsDirectory(),
                    "*.json"
                );
                fileChooser->launchAsync(
                    juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                    [this, complete](const juce::FileChooser& fc)
                    {
                        auto file = fc.getResult();
                        auto* result = new juce::DynamicObject();
                        if (file != juce::File{})
                        {
                            auto presetName = file.getFileNameWithoutExtension();
                            bool success = processorRef.presetManager.savePreset(presetName);
                            result->setProperty("success", success);
                            result->setProperty("name", success ? presetName : juce::String());
                        }
                        else
                        {
                            result->setProperty("success", false);
                            result->setProperty("name", juce::String());
                        }
                        complete(juce::var(result));
                    }
                );
            })
            .withNativeFunction("loadPreset", [this](auto& args, auto complete) {
                if (args.size() > 0)
                    complete(processorRef.presetManager.loadPreset(args[0].toString()));
                else
                    complete(false);
            })
            .withNativeFunction("loadPresetFromFile", [this](auto&, auto complete) {
                fileChooser = std::make_unique<juce::FileChooser>(
                    "Load Preset",
                    processorRef.presetManager.getUserPresetsDirectory(),
                    "*.json"
                );
                fileChooser->launchAsync(
                    juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                    [this, complete](const juce::FileChooser& fc)
                    {
                        auto file = fc.getResult();
                        auto* result = new juce::DynamicObject();
                        if (file != juce::File{} && file.existsAsFile())
                        {
                            bool success = processorRef.presetManager.loadPresetFromFile(file);
                            result->setProperty("success", success);
                            result->setProperty("name", success ? file.getFileNameWithoutExtension() : juce::String());
                        }
                        else
                        {
                            result->setProperty("success", false);
                            result->setProperty("name", juce::String());
                        }
                        complete(juce::var(result));
                    }
                );
            })
            .withNativeFunction("getPresetList", [this](auto&, auto complete) {
                auto list = processorRef.presetManager.getPresetList();
                juce::Array<juce::var> arr;
                for (const auto& name : list)
                    arr.add(name);
                complete(juce::var(arr));
            })
            .withNativeFunction("getCurrentPreset", [this](auto&, auto complete) {
                complete(processorRef.presetManager.getCurrentPresetName());
            })
            .withNativeFunction("selectNextPreset", [this](auto&, auto complete) {
                auto next = processorRef.presetManager.getNextPreset();
                complete(next);
            })
            .withNativeFunction("selectPreviousPreset", [this](auto&, auto complete) {
                auto prev = processorRef.presetManager.getPreviousPreset();
                complete(prev);
            })
            .withNativeFunction("deletePreset", [this](auto& args, auto complete) {
                if (args.size() > 0)
                    complete(processorRef.presetManager.deletePreset(args[0].toString()));
                else
                    complete(false);
            })
            .withNativeFunction("isFactoryPreset", [this](auto& args, auto complete) {
                if (args.size() > 0)
                    complete(processorRef.presetManager.isFactoryPreset(args[0].toString()));
                else
                    complete(false);
            })
            // v1.10.1 (O-Contrabass WR-11 pattern): each parameter's TRUE
            // (skew-correct) NORMALISED default, keyed by parameter ID, so the
            // page's Alt-click reset reads the C++ NormalisableRange rather
            // than a hand-mirrored JS table.
            .withNativeFunction("getParameterDefaults", [this](auto&, auto complete) {
                juce::DynamicObject::Ptr obj = new juce::DynamicObject();
                for (auto* param : processorRef.getParameters())
                    if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*>(param))
                        obj->setProperty(ranged->getParameterID(), ranged->getDefaultValue());
                complete(juce::var(obj.get()));
            })
            // ── v1.6.0: the UI LANGUAGE pair ────────────────────────────────
            //
            // Plain withNativeFunction, no relay. The page PULLS once at init;
            // there is no push from this constructor, no timer and no revision
            // counter, because the language is not preset content and no preset
            // path can change it behind the page's back. A push from here would
            // race the WebView's load.
            .withNativeFunction("getUiLanguage", [this](auto&, auto complete) {
                complete(juce::var(ODetuneAudioProcessor::languageCode(
                    processorRef.uiLanguage.load(std::memory_order_acquire))));
            })
            .withNativeFunction("setUiLanguage", [this](auto& args, auto complete) {
                // languageIndex() maps anything that is not "fr" to 0, so an
                // unexpected argument from the page degrades to English rather
                // than being stored unvalidated.
                if (args.size() > 0)
                    processorRef.uiLanguage.store(
                        ODetuneAudioProcessor::languageIndex(args[0].toString()),
                        std::memory_order_release);

                complete(juce::var(ODetuneAudioProcessor::languageCode(
                    processorRef.uiLanguage.load(std::memory_order_acquire))));
            });

   #if JUCE_WINDOWS
    // v1.10.2 — when the WebView2 environment cannot be created (runtime missing or
    // data folder unusable), JUCE silently builds the legacy IE control, which has
    // no resource provider. Probe first; on failure webView stays null and paint()
    // shows a native notice. Audio, parameters and automation don't need the WebView.
    const bool webView2Available = juce::WebBrowserComponent::areOptionsSupported (options);
   #else
    constexpr bool webView2Available = true;
   #endif

    if (webView2Available)
        webView = std::make_unique<juce::WebBrowserComponent> (options);

    // ============================================================================
    // 3. Create attachments LAST (Pattern #12: 3 parameters)
    // ============================================================================

    // Float parameter attachments
    blendAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("blend"), *blendRelay, nullptr);
    wobbleRateAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("wobble_rate"), *wobbleRateRelay, nullptr);
    wobbleDepthAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("wobble_depth"), *wobbleDepthRelay, nullptr);
    unisonDetuneAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("unison_detune"), *unisonDetuneRelay, nullptr);
    unisonSpreadAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("unison_spread"), *unisonSpreadRelay, nullptr);
    widthAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("width"), *widthRelay, nullptr);
    mixAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("mix"), *mixRelay, nullptr);
    focusLowAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("focus_low"), *focusLowRelay, nullptr);
    focusHighAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("focus_high"), *focusHighRelay, nullptr);
    delayAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("delay"), *delayRelay, nullptr);
    feedbackAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("feedback"), *feedbackRelay, nullptr);
    randomAmtAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("random_amt"), *randomAmtRelay, nullptr);

    // Choice parameter attachments
    wobbleEraAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *processorRef.parameters.getParameter("wobble_era"), *wobbleEraRelay, nullptr);
    wobbleShapeAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *processorRef.parameters.getParameter("wobble_shape"), *wobbleShapeRelay, nullptr);
    unisonVoicesAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *processorRef.parameters.getParameter("unison_voices"), *unisonVoicesRelay, nullptr);
    unisonDistAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *processorRef.parameters.getParameter("unison_dist"), *unisonDistRelay, nullptr);

    // Bool parameter attachments
    wobbleSyncAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.parameters.getParameter("wobble_sync"), *wobbleSyncRelay, nullptr);
    monoSafeAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.parameters.getParameter("mono_safe"), *monoSafeRelay, nullptr);

    // ============================================================================
    // Add WebView to editor (navigation happens in parentHierarchyChanged)
    // ============================================================================

    // v1.10.2 — attachments stay valid without a WebView: the relays drop their
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
        webView2RuntimeLink->setColour (juce::HyperlinkButton::textColourId, juce::Colour (0xff245046));   // --leaf-text
        addAndMakeVisible (*webView2RuntimeLink);
    }

    // Set editor size (600x480 for streamlined UI)
    setSize(600, 480);
}

ODetuneAudioProcessorEditor::~ODetuneAudioProcessorEditor()
{
    // Members destroyed in REVERSE order of declaration (automatic cleanup)
}

void ODetuneAudioProcessorEditor::paint(juce::Graphics& g)
{
    // v1.10.2 — WebView2 could not start (Windows only; webView is null): paint a native
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
        g.drawFittedText ("O-Detune needs Microsoft Edge WebView2",
                          area.removeFromTop (juce::roundToInt (40.0f * s)), juce::Justification::centred, 2);

        area.removeFromTop (juce::roundToInt (12.0f * s));
        g.setColour (juce::Colour (0xff5c4033));   // --ink-soft
        g.setFont (juce::FontOptions (17.0f * s));
        g.drawFittedText ("The interface is drawn with the WebView2 Runtime, which could not start on "
                          "this computer. Install it, then close and reopen the plugin window.\n"
                          "The effect itself keeps working, and host automation still "
                          "reaches every parameter.",
                          area.removeFromTop (juce::roundToInt (110.0f * s)), juce::Justification::centred, 5);
        return;
    }

    // WebView handles all painting
    juce::ignoreUnused(g);
}

void ODetuneAudioProcessorEditor::resized()
{
    // WebView fills entire editor
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

void ODetuneAudioProcessorEditor::parentHierarchyChanged()
{
    // Navigate WebView only after editor is attached to a window (JUCE 8 requirement)
    // This prevents crashes during plugin scanning when no window context exists
    // Use member variable instead of static to allow GUI reload on reopen
    if (isShowing() && webView != nullptr && !hasNavigated)
    {
        webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
        hasNavigated = true;
    }
}

// ============================================================================
// Pattern #8: EXPLICIT URL MAPPING (never use generic loops)
// ============================================================================
std::optional<juce::WebBrowserComponent::Resource>
ODetuneAudioProcessorEditor::getResource(const juce::String& url)
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

    // JUCE JavaScript bridge
    if (url == "/js/juce/index.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::index_js, BinaryData::index_jsSize),
            juce::String("text/javascript")
        };
    }

    // v1.6.0: the label table. EMBEDDED in CMakeLists.txt AND served here, in
    // the same commit. A file embedded but not served, or served but not
    // embedded, is a 404 that presents as a page stuck in English and nothing
    // else - check-i18n assertion 8 exists for exactly this pair.
    if (url == "/js/i18n.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::i18n_js, BinaryData::i18n_jsSize),
            juce::String("text/javascript")
        };
    }

    // JUCE interop checker (Pattern #13: REQUIRED)
    if (url == "/js/juce/check_native_interop.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::check_native_interop_js,
                      BinaryData::check_native_interop_jsSize),
            juce::String("text/javascript")
        };
    }

    // Ouaricon Naturalist assets
    if (url == "/assets/paper1.jpg") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::paper1_jpg, BinaryData::paper1_jpgSize),
            juce::String("image/jpeg")
        };
    }

    if (url == "/assets/slug.png") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::slug_png, BinaryData::slug_pngSize),
            juce::String("image/png")
        };
    }

    // Preset Manager JS module
    if (url == "/modules/preset-manager.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::presetmanager_js, BinaryData::presetmanager_jsSize),
            juce::String("text/javascript")
        };
    }

    // v1.10.0 (R5) — shared EB Garamond face (modules/ui/eb-garamond, direct
    // embed): the stylesheet under /css/, the three woff2 faces under /fonts/
    // where the stylesheet's relative url('../fonts/...') lands.
    // juce_add_binary_data STRIPS hyphens, so EBGaramond-Regular.woff2 becomes
    // the symbol EBGaramondRegular_woff2.
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
