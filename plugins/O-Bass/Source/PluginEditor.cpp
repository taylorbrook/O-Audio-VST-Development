/*
   This file is part of O-Bass, an Ouaricon Audio plugin.
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

    O-Bass - WebView Editor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

    WebView-based editor with botanical aesthetic and JUCE 8 parameter binding.

  ==============================================================================
*/

#include "PluginEditor.h"
#include "BinaryData.h"

OBassAudioProcessorEditor::OBassAudioProcessorEditor(OBassAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    // 1. Create relays FIRST (with IDs matching HTML getSliderState calls)
    frequencyRelay = std::make_unique<juce::WebSliderRelay>("crossover_freq");
    enhanceRelay = std::make_unique<juce::WebSliderRelay>("enhance");
    outputRelay = std::make_unique<juce::WebSliderRelay>("output");

    // 2. Create WebView SECOND with relay options and native functions
    // v1.8.3 — WebView2 user-data folder in the per-user app-data tree (was bare
    // %TEMP%, which cleaners wipe and other processes can lock), created before
    // WebView2 sees it. DAW hosts deny WebView2's default location. The WinWebView2
    // options are ignored off Windows.
    const auto webView2DataFolder = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                                        .getChildFile ("Ouaricon")
                                        .getChildFile ("WebView2")
                                        .getChildFile ("O-Bass");
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
            .withOptionsFrom(*frequencyRelay)
            .withOptionsFrom(*enhanceRelay)
            .withOptionsFrom(*outputRelay)
            // Limit indicator native function for UI polling
            .withNativeFunction("getLimitIndicator", [this](auto&, auto complete) {
                complete(processorRef.getLimitIndicator());
            })
            // Output level native function for VU meter
            .withNativeFunction("getOutputLevel", [this](auto&, auto complete) {
                complete(processorRef.getOutputLevelDB());
            })
            // ── v1.4.0: the UI LANGUAGE pair ───────────────────────────────
            //
            // Plain withNativeFunction, no relay. The page PULLS once at init;
            // there is no push from this constructor, no timer and no revision
            // counter, because the language is not preset content and no preset
            // path can change it behind the page's back. A push from here would
            // race the WebView's load.
            .withNativeFunction("getUiLanguage", [this](auto&, auto complete) {
                complete(juce::var(OBassAudioProcessor::languageCode(
                                       processorRef.uiLanguage.load(std::memory_order_acquire))));
            })
            .withNativeFunction("setUiLanguage", [this](auto& args, auto complete) {
                // languageIndex() maps anything that is not "fr" to 0, so an
                // unexpected argument from the page degrades to English rather
                // than being stored unvalidated.
                if (args.size() > 0)
                    processorRef.uiLanguage.store(
                        OBassAudioProcessor::languageIndex(args[0].toString()),
                        std::memory_order_release);

                complete(juce::var(OBassAudioProcessor::languageCode(
                                       processorRef.uiLanguage.load(std::memory_order_acquire))));
            })
            // Preset Manager native functions
            .withNativeFunction("savePreset", [this](auto& args, auto complete) {
                if (args.size() > 0)
                    complete(processorRef.presetManager.savePreset(args[0].toString()));
                else
                    complete(false);
            })
            .withNativeFunction("savePresetWithDialog", [this](auto&, auto complete) {
                // Create file chooser for saving preset
                fileChooser = std::make_unique<juce::FileChooser>(
                    "Save Preset",
                    processorRef.presetManager.getUserPresetsDirectory(),
                    "*.json"
                );

                // Launch async save dialog
                // CR-01: guard the completion with a SafePointer — the editor can be
                // destroyed while the native dialog is open (close window, switch track,
                // remove plugin). On teardown bail with a BARE return: do NOT call
                // complete() — that callback is owned by the already-dead WebView Impl,
                // so invoking it is itself a use-after-free.
                juce::Component::SafePointer<OBassAudioProcessorEditor> safeThis(this);
                fileChooser->launchAsync(
                    juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                    [safeThis, complete](const juce::FileChooser& fc) {
                        if (safeThis == nullptr)
                            return;  // editor gone — do NOT call complete()

                        auto results = fc.getResults();
                        if (results.isEmpty()) {
                            // User cancelled
                            auto* result = new juce::DynamicObject();
                            result->setProperty("success", false);
                            result->setProperty("name", "");
                            complete(juce::var(result));
                            return;
                        }

                        auto file = results.getFirst();
                        auto presetName = file.getFileNameWithoutExtension();

                        // Save using the preset manager
                        bool success = safeThis->processorRef.presetManager.savePreset(presetName);

                        auto* result = new juce::DynamicObject();
                        result->setProperty("success", success);
                        result->setProperty("name", success ? presetName : juce::String());
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
            .withNativeFunction("loadPresetFromFile", [this](auto&, auto complete) {
                // Create file chooser for preset JSON files
                fileChooser = std::make_unique<juce::FileChooser>(
                    "Load Preset",
                    processorRef.presetManager.getUserPresetsDirectory(),
                    "*.json"
                );

                // Launch async file dialog
                // CR-01: SafePointer-guard the completion (see savePresetWithDialog).
                // Editor teardown while the dialog is open must bail with a BARE return —
                // calling complete() would fire a callback owned by the dead WebView Impl.
                juce::Component::SafePointer<OBassAudioProcessorEditor> safeThis(this);
                fileChooser->launchAsync(
                    juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                    [safeThis, complete](const juce::FileChooser& fc) {
                        if (safeThis == nullptr)
                            return;  // editor gone — do NOT call complete()

                        auto results = fc.getResults();
                        if (results.isEmpty()) {
                            // User cancelled
                            auto* result = new juce::DynamicObject();
                            result->setProperty("success", false);
                            result->setProperty("name", "");
                            complete(juce::var(result));
                            return;
                        }

                        auto file = results.getFirst();
                        bool success = safeThis->processorRef.presetManager.loadPresetFromFile(file);

                        auto* result = new juce::DynamicObject();
                        result->setProperty("success", success);
                        result->setProperty("name", success ? file.getFileNameWithoutExtension() : juce::String());
                        complete(juce::var(result));
                    }
                );
            });

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
        webView = std::make_unique<juce::WebBrowserComponent> (options);

    // 3. Create attachments LAST (Pattern #12: 3-param constructor - parameter, relay, nullptr)
    frequencyAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("crossover_freq"), *frequencyRelay, nullptr);
    enhanceAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("enhance"), *enhanceRelay, nullptr);
    outputAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("output"), *outputRelay, nullptr);

    // Add WebView (navigation happens in parentHierarchyChanged)
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
        webView2RuntimeLink->setColour (juce::HyperlinkButton::textColourId, juce::Colour (0xff3c5c1a));   // --olive-dark
        addAndMakeVisible (*webView2RuntimeLink);
    }

    // Compact size for 3-knob horizontal layout
    setSize(420, 320);

    // Start meter update timer (30fps)
    startTimerHz(30);
}

OBassAudioProcessorEditor::~OBassAudioProcessorEditor()
{
    // Members destroyed in REVERSE order of declaration (automatic RAII cleanup)
}

void OBassAudioProcessorEditor::paint(juce::Graphics& g)
{
    // v1.8.3 — WebView2 could not start (Windows only; webView is null): paint a native
    // notice instead of JUCE's legacy-IE fallback page. Colours from the page CSS;
    // scaled down to fit short editors.
    if (webView == nullptr)
    {
        const float s = juce::jlimit (0.6f, 1.0f, (float) getHeight() / 260.0f);
        auto area = getLocalBounds().withSizeKeepingCentre (juce::jmin (640, getWidth() - 48),
                                                            juce::roundToInt (202.0f * s));
        g.fillAll (juce::Colour (0xfff5e6d3));   // --bg-paper

        g.setColour (juce::Colour (0xff3c2f2f));   // --brown-text
        g.setFont (juce::FontOptions (26.0f * s, juce::Font::bold));
        g.drawFittedText ("O-Bass needs Microsoft Edge WebView2",
                          area.removeFromTop (juce::roundToInt (40.0f * s)), juce::Justification::centred, 2);

        area.removeFromTop (juce::roundToInt (12.0f * s));
        g.setColour (juce::Colour (0xff5c4033));   // --brown-frame
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

void OBassAudioProcessorEditor::resized()
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

void OBassAudioProcessorEditor::parentHierarchyChanged()
{
    // Navigate WebView only after editor is attached to a window (JUCE 8 requirement)
    // This prevents crashes during plugin scanning when no window context exists
    // Use per-instance hasNavigated to allow GUI reload on editor reopen
    if (isShowing() && webView != nullptr && !hasNavigated)
    {
        webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
        hasNavigated = true;
    }
}

void OBassAudioProcessorEditor::timerCallback()
{
    // Push meter data to WebView via JavaScript
    float outputLevel = processorRef.getOutputLevelDB();
    float limitIndicator = processorRef.getLimitIndicator();

    if (webView == nullptr)
        return;

    // v1.8.3 — locale-independent numbers. String::formatted ("%f") follows the
    // host process's C locale, so a comma-decimal host (French region) built
    // "updateMeters(-12,500000, 0,000000)": four arguments, a dead meter.
    // juce::String (float, decimals) always writes '.'. A non-finite level maps
    // to the page's floor (MIN_DB -60) / an idle LED, so the call stays valid JS.
    const auto num = [] (float v, float fallback) { return juce::String (std::isfinite (v) ? v : fallback, 3); };

    const auto script = "if (typeof updateMeters === 'function') { updateMeters("
                      + num (outputLevel, -60.0f) + ", " + num (limitIndicator, 0.0f) + "); }";

    webView->evaluateJavascript(script, nullptr);
}

// Pattern #8: EXPLICIT URL MAPPING (never use generic loops)
std::optional<juce::WebBrowserComponent::Resource>
OBassAudioProcessorEditor::getResource(const juce::String& url)
{
    auto makeVector = [](const char* data, int size) {
        return std::vector<std::byte>(
            reinterpret_cast<const std::byte*>(data),
            reinterpret_cast<const std::byte*>(data) + size
        );
    };

    // Root "/" -> index.html
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

    // v1.4.0: the label table. EMBEDDED in CMakeLists.txt AND served here, in
    // the same commit. A file embedded but not served, or served but not
    // embedded, is a 404 that presents as a page stuck in English and nothing
    // else - check-i18n assertion 8 exists for exactly this pair.
    if (url == "/js/i18n.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::i18n_js, BinaryData::i18n_jsSize),
            juce::String("application/javascript")
        };
    }

    // JUCE interop checker (Pattern #13: REQUIRED for native function support)
    if (url == "/js/juce/check_native_interop.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::check_native_interop_js,
                      BinaryData::check_native_interop_jsSize),
            juce::String("text/javascript")
        };
    }

    // Background paper texture
    if (url == "/img/paper.jpg") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::paper_jpg, BinaryData::paper_jpgSize),
            juce::String("image/jpeg")
        };
    }

    // Botanical overlay image
    if (url == "/img/botanical.png") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::botanical_png, BinaryData::botanical_pngSize),
            juce::String("image/png")
        };
    }

    // Preset Manager module
    if (url == "/modules/preset-manager.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::presetmanager_js, BinaryData::presetmanager_jsSize),
            juce::String("text/javascript")
        };
    }

    // v1.8.0 (R5): shared EB Garamond face (modules/ui/eb-garamond) — the
    // stylesheet under /css/, the three woff2 faces under /fonts/ where its
    // relative url('../fonts/...') lands.
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
    juce::Logger::writeToLog("O-Bass: Resource not found: " + url);
    return std::nullopt;
}
