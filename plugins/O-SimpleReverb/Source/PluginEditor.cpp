/*
   This file is part of O-SimpleReverb, an Ouaricon Audio plugin.
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

    O-SimpleReverb - Editor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

    v1.5.0 - Renamed from OuariconSimpleReverb

  ==============================================================================
*/

#include "PluginEditor.h"
#include "BinaryData.h"

OSimpleReverbAudioProcessorEditor::OSimpleReverbAudioProcessorEditor(OSimpleReverbAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    // 1. Create relays FIRST (with parameter IDs matching HTML and APVTS)
    typeRelay = std::make_unique<juce::WebComboBoxRelay>("TYPE");
    characterRelay = std::make_unique<juce::WebSliderRelay>("CHARACTER");
    wetRelay = std::make_unique<juce::WebSliderRelay>("WET");
    dryRelay = std::make_unique<juce::WebSliderRelay>("DRY");
    decayRelay = std::make_unique<juce::WebSliderRelay>("DECAY");
    sizeRelay = std::make_unique<juce::WebSliderRelay>("SIZE");
    lpFreqRelay = std::make_unique<juce::WebSliderRelay>("LPFREQ");
    lpOnRelay = std::make_unique<juce::WebSliderRelay>("LPON");

    // 2. Create WebView SECOND with all relay options registered
    webView = std::make_unique<juce::WebBrowserComponent>(
        juce::WebBrowserComponent::Options{}
            .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
            .withWinWebView2Options(
                juce::WebBrowserComponent::Options::WinWebView2{}
                    .withUserDataFolder(juce::File::getSpecialLocation(
                        juce::File::SpecialLocationType::tempDirectory)))
            .withNativeIntegrationEnabled()
            .withResourceProvider([this](const auto& url) { return getResource(url); })
            .withOptionsFrom(*typeRelay)
            .withOptionsFrom(*characterRelay)
            .withOptionsFrom(*wetRelay)
            .withOptionsFrom(*dryRelay)
            .withOptionsFrom(*decayRelay)
            .withOptionsFrom(*sizeRelay)
            .withOptionsFrom(*lpFreqRelay)
            .withOptionsFrom(*lpOnRelay)
            // Preset manager native functions
            .withNativeFunction("savePreset", [this](const auto& args, auto complete) {
                if (args.size() > 0)
                    complete(processorRef.presetManager.savePreset(args[0].toString()));
                else
                    complete(false);
            })
            .withNativeFunction("loadPreset", [this](const auto& args, auto complete) {
                if (args.size() > 0)
                    complete(processorRef.presetManager.loadPreset(args[0].toString()));
                else
                    complete(false);
            })
            .withNativeFunction("getPresetList", [this](const auto&, auto complete) {
                auto list = processorRef.presetManager.getPresetList();
                juce::Array<juce::var> arr;
                for (const auto& name : list)
                    arr.add(name);
                complete(juce::var(arr));
            })
            .withNativeFunction("getCurrentPreset", [this](const auto&, auto complete) {
                complete(processorRef.presetManager.getCurrentPresetName());
            })
            .withNativeFunction("selectNextPreset", [this](const auto&, auto complete) {
                auto next = processorRef.presetManager.getNextPreset();
                complete(next);
            })
            .withNativeFunction("selectPreviousPreset", [this](const auto&, auto complete) {
                auto prev = processorRef.presetManager.getPreviousPreset();
                complete(prev);
            })
            .withNativeFunction("deletePreset", [this](const auto& args, auto complete) {
                if (args.size() > 0)
                    complete(processorRef.presetManager.deletePreset(args[0].toString()));
                else
                    complete(false);
            })
            .withNativeFunction("isFactoryPreset", [this](const auto& args, auto complete) {
                if (args.size() > 0)
                    complete(processorRef.presetManager.isFactoryPreset(args[0].toString()));
                else
                    complete(false);
            })
            .withNativeFunction("savePresetWithDialog", [this](const auto&, auto complete) {
                auto userDir = processorRef.presetManager.getUserPresetsDirectory();
                fileChooser = std::make_unique<juce::FileChooser>(
                    "Save Preset",
                    userDir,
                    "*.json"
                );
                // CR-01: the dialog can outlive the editor. Capture a SafePointer and
                // bail with a bare return on the null path — `complete` is owned by
                // the destroyed WebView Impl, so invoking it would itself be a UAF.
                juce::Component::SafePointer<OSimpleReverbAudioProcessorEditor> safeThis(this);
                fileChooser->launchAsync(
                    juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                    [safeThis, complete](const juce::FileChooser& fc) {
                        if (safeThis == nullptr)
                            return;
                        auto file = fc.getResult();
                        if (file != juce::File{}) {
                            auto name = file.getFileNameWithoutExtension();
                            bool success = safeThis->processorRef.presetManager.savePreset(name);
                            auto* result = new juce::DynamicObject();
                            result->setProperty("success", success);
                            result->setProperty("name", name);
                            complete(juce::var(result));
                        } else {
                            auto* result = new juce::DynamicObject();
                            result->setProperty("success", false);
                            result->setProperty("name", "");
                            complete(juce::var(result));
                        }
                    }
                );
            })
            .withNativeFunction("loadPresetFromFile", [this](const auto&, auto complete) {
                auto presetsDir = processorRef.presetManager.getPresetsDirectory();
                fileChooser = std::make_unique<juce::FileChooser>(
                    "Load Preset",
                    presetsDir,
                    "*.json"
                );
                // CR-01: see savePresetWithDialog — SafePointer + bare return on teardown
                juce::Component::SafePointer<OSimpleReverbAudioProcessorEditor> safeThis(this);
                fileChooser->launchAsync(
                    juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                    [safeThis, complete](const juce::FileChooser& fc) {
                        if (safeThis == nullptr)
                            return;
                        auto file = fc.getResult();
                        if (file.existsAsFile()) {
                            bool success = safeThis->processorRef.presetManager.loadPresetFromFile(file);
                            auto name = file.getFileNameWithoutExtension();
                            auto* result = new juce::DynamicObject();
                            result->setProperty("success", success);
                            result->setProperty("name", success ? name : "");
                            complete(juce::var(result));
                        } else {
                            auto* result = new juce::DynamicObject();
                            result->setProperty("success", false);
                            result->setProperty("name", "");
                            complete(juce::var(result));
                        }
                    }
                );
            })
    );

    // 3. Create attachments LAST (connect parameters to relays)
    typeAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *processorRef.parameters.getParameter("TYPE"), *typeRelay, nullptr);

    characterAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("CHARACTER"), *characterRelay, nullptr);

    wetAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("WET"), *wetRelay, nullptr);

    dryAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("DRY"), *dryRelay, nullptr);

    decayAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("DECAY"), *decayRelay, nullptr);

    sizeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("SIZE"), *sizeRelay, nullptr);

    lpFreqAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("LPFREQ"), *lpFreqRelay, nullptr);

    lpOnAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.parameters.getParameter("LPON"), *lpOnRelay, nullptr);

    // Add WebView to editor and navigate to UI
    addAndMakeVisible(*webView);
    webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());

    // Set editor size (from creative brief: 500x350)
    setSize(500, 350);

    // Start timer for VU meter updates (30 Hz)
    startTimerHz(30);
}

OSimpleReverbAudioProcessorEditor::~OSimpleReverbAudioProcessorEditor()
{
    stopTimer();
}

//==============================================================================
void OSimpleReverbAudioProcessorEditor::timerCallback()
{
    // VU Meter - emit output level to WebView
    const float outputDB = processorRef.outputLevelDB.load(std::memory_order_relaxed);
    webView->emitEventIfBrowserIsVisible("outputLevel", outputDB);
}

void OSimpleReverbAudioProcessorEditor::paint(juce::Graphics& g)
{
    // WebView handles all painting
    juce::ignoreUnused(g);
}

void OSimpleReverbAudioProcessorEditor::resized()
{
    // WebView fills entire editor
    if (webView)
        webView->setBounds(getLocalBounds());
}

std::optional<juce::WebBrowserComponent::Resource>
OSimpleReverbAudioProcessorEditor::getResource(const juce::String& url)
{
    // Helper to convert BinaryData to byte vector
    auto makeVector = [](const char* data, int size) {
        return std::vector<std::byte>(
            reinterpret_cast<const std::byte*>(data),
            reinterpret_cast<const std::byte*>(data) + size
        );
    };

    // Route URLs to embedded resources
    if (url == "/" || url == "/index.html")
    {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::index_html, BinaryData::index_htmlSize),
            juce::String("text/html")
        };
    }

    if (url == "/js/juce/index.js")
    {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::index_js, BinaryData::index_jsSize),
            juce::String("text/javascript")
        };
    }

    if (url == "/js/juce/check_native_interop.js")
    {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::check_native_interop_js, BinaryData::check_native_interop_jsSize),
            juce::String("text/javascript")
        };
    }

    if (url == "/img/paper.jpg")
    {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::paper_jpg, BinaryData::paper_jpgSize),
            juce::String("image/jpeg")
        };
    }

    if (url == "/img/flora.png")
    {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::flora_png, BinaryData::flora_pngSize),
            juce::String("image/png")
        };
    }

    // Resource not found
    juce::Logger::writeToLog("Resource not found: " + url);
    return std::nullopt;
}
