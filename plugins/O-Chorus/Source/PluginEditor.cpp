/*
   This file is part of O-Chorus, an Ouaricon Audio plugin.
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

    O-Chorus - Editor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

    Stage 1 (Foundation) - Placeholder WebView UI

  ==============================================================================
*/

#include "PluginEditor.h"
#include "BinaryData.h"

//==============================================================================
OChorusAudioProcessorEditor::OChorusAudioProcessorEditor(OChorusAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    //==========================================================================
    // CRITICAL: Initialization Order
    // 1. Create relays FIRST
    // 2. Create WebView with relay options
    // 3. Create attachments LAST
    //==========================================================================

    // 1️⃣ Create relays FIRST (no dependencies)
    rateRelay = std::make_unique<juce::WebSliderRelay>("rate");
    depthRelay = std::make_unique<juce::WebSliderRelay>("depth");
    voicesRelay = std::make_unique<juce::WebSliderRelay>("voices");
    spreadRelay = std::make_unique<juce::WebSliderRelay>("spread");
    widthRelay = std::make_unique<juce::WebSliderRelay>("width");
    toneRelay = std::make_unique<juce::WebSliderRelay>("tone");
    mixRelay = std::make_unique<juce::WebSliderRelay>("mix");
    driveRelay = std::make_unique<juce::WebSliderRelay>("drive");

    // 2️⃣ Create WebView with relay options (depends on relays)
    webView = std::make_unique<juce::WebBrowserComponent>(
        juce::WebBrowserComponent::Options{}
            .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
            .withWinWebView2Options(
                juce::WebBrowserComponent::Options::WinWebView2{}
                    .withUserDataFolder(juce::File::getSpecialLocation(
                        juce::File::SpecialLocationType::tempDirectory)))
            .withNativeIntegrationEnabled()
#if JUCE_WEB_BROWSER_RESOURCE_PROVIDER_AVAILABLE
            .withResourceProvider([this](const auto& url) { return getResource(url); })
#endif
            .withOptionsFrom(*rateRelay)
            .withOptionsFrom(*depthRelay)
            .withOptionsFrom(*voicesRelay)
            .withOptionsFrom(*spreadRelay)
            .withOptionsFrom(*widthRelay)
            .withOptionsFrom(*toneRelay)
            .withOptionsFrom(*mixRelay)
            .withOptionsFrom(*driveRelay)
        .withNativeFunction("savePreset", [this](const juce::Array<juce::var>& args,
                                                  std::function<void(juce::var)> complete) {
            if (args.size() > 0)
                complete(audioProcessor.presetManager.savePreset(args[0].toString()));
            else
                complete(false);
        })
        .withNativeFunction("loadPreset", [this](const juce::Array<juce::var>& args,
                                                  std::function<void(juce::var)> complete) {
            if (args.size() > 0)
                complete(audioProcessor.presetManager.loadPreset(args[0].toString()));
            else
                complete(false);
        })
        .withNativeFunction("getPresetList", [this](const juce::Array<juce::var>&,
                                                     std::function<void(juce::var)> complete) {
            auto list = audioProcessor.presetManager.getPresetList();
            juce::Array<juce::var> arr;
            for (const auto& name : list)
                arr.add(name);
            complete(juce::var(arr));
        })
        .withNativeFunction("getCurrentPreset", [this](const juce::Array<juce::var>&,
                                                        std::function<void(juce::var)> complete) {
            complete(audioProcessor.presetManager.getCurrentPresetName());
        })
        .withNativeFunction("selectNextPreset", [this](const juce::Array<juce::var>&,
                                                        std::function<void(juce::var)> complete) {
            auto next = audioProcessor.presetManager.getNextPreset();
            complete(next);
        })
        .withNativeFunction("selectPreviousPreset", [this](const juce::Array<juce::var>&,
                                                            std::function<void(juce::var)> complete) {
            auto prev = audioProcessor.presetManager.getPreviousPreset();
            complete(prev);
        })
        .withNativeFunction("deletePreset", [this](const juce::Array<juce::var>& args,
                                                    std::function<void(juce::var)> complete) {
            if (args.size() > 0)
                complete(audioProcessor.presetManager.deletePreset(args[0].toString()));
            else
                complete(false);
        })
        .withNativeFunction("isFactoryPreset", [this](const juce::Array<juce::var>& args,
                                                       std::function<void(juce::var)> complete) {
            if (args.size() > 0)
                complete(audioProcessor.presetManager.isFactoryPreset(args[0].toString()));
            else
                complete(false);
        })
        .withNativeFunction("savePresetWithDialog", [this](const juce::Array<juce::var>&,
                                                            std::function<void(juce::var)> complete) {
            fileChooser = std::make_unique<juce::FileChooser>(
                "Save Preset",
                audioProcessor.presetManager.getUserPresetsDirectory(),
                "*.json"
            );
            fileChooser->launchAsync(
                juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                [this, complete](const juce::FileChooser& fc) {
                    auto results = fc.getResults();
                    if (results.isEmpty()) {
                        auto* result = new juce::DynamicObject();
                        result->setProperty("success", false);
                        result->setProperty("name", juce::String());
                        complete(juce::var(result));
                        return;
                    }
                    auto file = results.getFirst();
                    auto presetName = file.getFileNameWithoutExtension();
                    bool success = audioProcessor.presetManager.savePreset(presetName);
                    auto* result = new juce::DynamicObject();
                    result->setProperty("success", success);
                    result->setProperty("name", success ? presetName : juce::String());
                    complete(juce::var(result));
                }
            );
        })
        .withNativeFunction("loadPresetFromFile", [this](const juce::Array<juce::var>&,
                                                          std::function<void(juce::var)> complete) {
            fileChooser = std::make_unique<juce::FileChooser>(
                "Load Preset",
                audioProcessor.presetManager.getUserPresetsDirectory(),
                "*.json"
            );
            fileChooser->launchAsync(
                juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                [this, complete](const juce::FileChooser& fc) {
                    auto results = fc.getResults();
                    if (results.isEmpty()) {
                        auto* result = new juce::DynamicObject();
                        result->setProperty("success", false);
                        result->setProperty("name", juce::String());
                        complete(juce::var(result));
                        return;
                    }
                    auto file = results.getFirst();
                    bool success = audioProcessor.presetManager.loadPresetFromFile(file);
                    auto* result = new juce::DynamicObject();
                    result->setProperty("success", success);
                    result->setProperty("name", success ? file.getFileNameWithoutExtension() : juce::String());
                    complete(juce::var(result));
                }
            );
        })
    );

    addAndMakeVisible(*webView);

    // 3️⃣ Create attachments LAST (depend on relays and webView)
    rateAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.parameters.getParameter("rate"), *rateRelay, nullptr);
    depthAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.parameters.getParameter("depth"), *depthRelay, nullptr);
    voicesAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.parameters.getParameter("voices"), *voicesRelay, nullptr);
    spreadAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.parameters.getParameter("spread"), *spreadRelay, nullptr);
    widthAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.parameters.getParameter("width"), *widthRelay, nullptr);
    toneAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.parameters.getParameter("tone"), *toneRelay, nullptr);
    mixAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.parameters.getParameter("mix"), *mixRelay, nullptr);
    driveAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *audioProcessor.parameters.getParameter("drive"), *driveRelay, nullptr);

    // Load UI from resource provider
#if JUCE_WEB_BROWSER_RESOURCE_PROVIDER_AVAILABLE
    webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
#endif

    setSize(700, 125);
}

OChorusAudioProcessorEditor::~OChorusAudioProcessorEditor()
{
    // Members destroyed in REVERSE order of declaration:
    // 1. Attachments destroyed FIRST (can safely call webView methods)
    // 2. WebView destroyed SECOND (attachments are gone)
    // 3. Relays destroyed LAST (nothing using them)
}

//==============================================================================
void OChorusAudioProcessorEditor::paint(juce::Graphics& g)
{
    // WebView handles all rendering
    g.fillAll(juce::Colours::black);
}

void OChorusAudioProcessorEditor::resized()
{
    // WebView fills entire editor window
    webView->setBounds(getLocalBounds());
}

//==============================================================================
std::optional<juce::WebBrowserComponent::Resource>
OChorusAudioProcessorEditor::getResource(const juce::String& url)
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
    // BinaryData flattens paths (index.js → index_js)
    // HTML requests use original paths (./js/juce/index.js)
    // Must map manually with correct MIME types
    //==========================================================================

    if (url == "/" || url == "/index.html") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::index_html, BinaryData::index_htmlSize),
            juce::String("text/html")
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

    if (url == "/img/paper1.jpg") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::paper1_jpg, BinaryData::paper1_jpgSize),
            juce::String("image/jpeg")
        };
    }

    if (url == "/img/insects.png") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::insects_png, BinaryData::insects_pngSize),
            juce::String("image/png")
        };
    }

    if (url == "/modules/preset-manager.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::presetmanager_js, BinaryData::presetmanager_jsSize),
            juce::String("application/javascript")
        };
    }

    // 404 - Resource not found
    juce::Logger::writeToLog("Resource not found: " + url);
    return std::nullopt;
}
