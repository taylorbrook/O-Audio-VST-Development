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

    // 2. Build WebView options — every control rides a relay and the info
    // readout is a static JS table. The 10 preset native functions below are
    // exactly the names modules/preset-manager.js resolves (bridge audit
    // 10<->10; index.html itself registers none).
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

    // ── PRESET NATIVE FUNCTIONS — 10 (Stage 4) ──────────────────────────────
    // The eight synchronous ones capture `this` (completion never outlives the
    // call). The two DIALOG fns defer their completion into a FileChooser
    // callback: shared_ptr chooser captured into its own callback, SafePointer
    // HOISTED to a local (MSVC rejects SafePointer(this) init-captures in
    // nested lambdas), and on a dead editor the callback RETURNS — even
    // complete(false) would UAF the dead WebView impl
    // (pattern_webview_launchasync_safepointer_no_complete). Both dialog fns
    // complete with {success, name} OBJECTS — the JS reads result.success /
    // result.name; a bare bool reads as failure even when the file was
    // written.

    options = options.withNativeFunction("savePreset",
        [this](const juce::Array<juce::var>& args, auto complete)
        {
            if (args.size() > 0)
                complete(juce::var(audioProcessor.presetManager.savePreset(args[0].toString())));
            else
                complete(juce::var(false));
        });

    options = options.withNativeFunction("savePresetWithDialog",
        [this](auto&, auto complete)
        {
            auto userDir = audioProcessor.presetManager.getUserPresetsDirectory();
            userDir.createDirectory();

            auto chooser = std::make_shared<juce::FileChooser>("Save Preset", userDir, "*.json");
            juce::Component::SafePointer<OEmulatorAudioProcessorEditor> safeThis(this);

            chooser->launchAsync(
                juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                [safeThis, chooser, complete](const juce::FileChooser& fc)
                {
                    if (safeThis == nullptr)
                        return;   // dead editor — never touch complete

                    auto* result = new juce::DynamicObject();
                    const auto results = fc.getResults();

                    if (results.size() > 0)
                    {
                        // savePresetToFile HONORS the chosen path.
                        const auto file = results.getReference(0);
                        const bool ok = safeThis->audioProcessor.presetManager.savePresetToFile(file);
                        result->setProperty("success", ok);
                        result->setProperty("name", file.getFileNameWithoutExtension());
                    }
                    else
                    {
                        result->setProperty("success", false);
                        result->setProperty("name", juce::String());
                    }

                    complete(juce::var(result));
                });
        });

    options = options.withNativeFunction("loadPreset",
        [this](const juce::Array<juce::var>& args, auto complete)
        {
            if (args.size() > 0)
                complete(juce::var(audioProcessor.presetManager.loadPreset(args[0].toString())));
            else
                complete(juce::var(false));
        });

    options = options.withNativeFunction("loadPresetFromFile",
        [this](auto&, auto complete)
        {
            auto presetsDir = audioProcessor.presetManager.getPresetsDirectory();

            auto chooser = std::make_shared<juce::FileChooser>("Load Preset", presetsDir, "*.json");
            juce::Component::SafePointer<OEmulatorAudioProcessorEditor> safeThis(this);

            chooser->launchAsync(
                juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                [safeThis, chooser, complete](const juce::FileChooser& fc)
                {
                    if (safeThis == nullptr)
                        return;   // dead editor — never touch complete

                    auto* result = new juce::DynamicObject();
                    const auto results = fc.getResults();

                    if (results.size() > 0)
                    {
                        const auto file = results.getReference(0);
                        const bool ok = safeThis->audioProcessor.presetManager.loadPresetFromFile(file);
                        result->setProperty("success", ok);
                        result->setProperty("name", file.getFileNameWithoutExtension());
                    }
                    else
                    {
                        result->setProperty("success", false);
                        result->setProperty("name", juce::String());
                    }

                    complete(juce::var(result));
                });
        });

    options = options.withNativeFunction("getPresetList",
        [this](auto&, auto complete)
        {
            juce::Array<juce::var> list;
            for (const auto& name : audioProcessor.presetManager.getPresetList())
                list.add(juce::var(name));
            complete(juce::var(list));
        });

    options = options.withNativeFunction("getCurrentPreset",
        [this](auto&, auto complete)
        {
            complete(juce::var(audioProcessor.presetManager.getCurrentPresetName()));
        });

    options = options.withNativeFunction("selectNextPreset",
        [this](auto&, auto complete)
        {
            complete(juce::var(audioProcessor.presetManager.getNextPreset()));
        });

    options = options.withNativeFunction("selectPreviousPreset",
        [this](auto&, auto complete)
        {
            complete(juce::var(audioProcessor.presetManager.getPreviousPreset()));
        });

    options = options.withNativeFunction("deletePreset",
        [this](const juce::Array<juce::var>& args, auto complete)
        {
            if (args.size() > 0)
                complete(juce::var(audioProcessor.presetManager.deletePreset(args[0].toString())));
            else
                complete(juce::var(false));
        });

    options = options.withNativeFunction("isFactoryPreset",
        [this](const juce::Array<juce::var>& args, auto complete)
        {
            if (args.size() > 0)
                complete(juce::var(audioProcessor.presetManager.isFactoryPreset(args[0].toString())));
            else
                complete(juce::var(false));
        });

    // ── v1.1.0: the UI LANGUAGE pair ────────────────────────────────────────
    //
    // Plain withNativeFunction, no relay. The page PULLS once at init; there is
    // no push from this constructor, no timer and no revision counter, because
    // the language is not preset content and no preset path can change it
    // behind the page's back. A push from here would race the WebView's load.
    //
    // Both capture `this` and complete SYNCHRONOUSLY, like the eight
    // synchronous preset functions above — the completion never outlives the
    // call, so neither needs the SafePointer dance the two dialog functions do.
    options = options.withNativeFunction("getUiLanguage",
        [this](auto&, auto complete)
        {
            complete(juce::var(OEmulatorAudioProcessor::languageCode(
                                   audioProcessor.uiLanguage.load(std::memory_order_acquire))));
        });

    options = options.withNativeFunction("setUiLanguage",
        [this](const juce::Array<juce::var>& args, auto complete)
        {
            // languageIndex() maps anything that is not "fr" to 0, so an
            // unexpected argument from the page degrades to English rather than
            // being stored unvalidated.
            if (args.size() > 0)
                audioProcessor.uiLanguage.store(
                    OEmulatorAudioProcessor::languageIndex(args[0].toString()),
                    std::memory_order_release);

            complete(juce::var(OEmulatorAudioProcessor::languageCode(
                                   audioProcessor.uiLanguage.load(std::memory_order_acquire))));
        });

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

    // v1.1.0: the label table. EMBEDDED in CMakeLists.txt AND served here, in
    // the same commit. A file embedded but not served, or served but not
    // embedded, is a 404 that presents as a page stuck in English and nothing
    // else — check-i18n assertion 8 exists for exactly this pair.
    if (url == "/js/i18n.js")
    {
        return juce::WebBrowserComponent::Resource{
            makeVector(BinaryData::i18n_js, BinaryData::i18n_jsSize),
            juce::String("application/javascript")};
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

    // BinaryData strips hyphens: preset-manager.js -> presetmanager_js.
    if (url == "/modules/preset-manager.js")
    {
        return juce::WebBrowserComponent::Resource{
            makeVector(BinaryData::presetmanager_js, BinaryData::presetmanager_jsSize),
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
