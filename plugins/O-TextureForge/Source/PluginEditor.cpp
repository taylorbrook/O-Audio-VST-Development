/*
   This file is part of O-TextureForge, an Ouaricon Audio plugin.
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

    O-TextureForge - Editor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginEditor.h"
#include "BinaryData.h"

TextureForgeEditor::TextureForgeEditor(TextureForgeProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    // 1. Create relays (before webView)
    energyRelay       = std::make_unique<juce::WebSliderRelay>("energy");
    brightnessRelay   = std::make_unique<juce::WebSliderRelay>("brightness");
    textureRelay      = std::make_unique<juce::WebSliderRelay>("texture");
    positionRelay     = std::make_unique<juce::WebSliderRelay>("position");
    grainDensityRelay = std::make_unique<juce::WebSliderRelay>("grainDensity");
    grainSizeRelay    = std::make_unique<juce::WebSliderRelay>("grainSize");
    scatterXRelay     = std::make_unique<juce::WebSliderRelay>("scatterX");
    scatterYRelay     = std::make_unique<juce::WebSliderRelay>("scatterY");
    variationRelay    = std::make_unique<juce::WebSliderRelay>("variation");
    crossfadeRelay    = std::make_unique<juce::WebSliderRelay>("crossfade");
    outputGainRelay   = std::make_unique<juce::WebSliderRelay>("outputGain");
    midiModeRelay     = std::make_unique<juce::WebComboBoxRelay>("midiMode");

    // 2. Create WebView with relay options and native functions
#if JUCE_WINDOWS
    auto webViewOptions = juce::WebBrowserComponent::Options{}
        .withBackend(juce::WebBrowserComponent::Options::Backend::webview2);
    if (!juce::WebBrowserComponent::areOptionsSupported(webViewOptions))
    {
        auto* fallbackLabel = new juce::Label("fallback",
            "WebView2 runtime required.\nDownload from: microsoft.com/edge/webview2");
        fallbackLabel->setJustificationType(juce::Justification::centred);
        fallbackLabel->setColour(juce::Label::textColourId, juce::Colours::white);
        fallbackLabel->setColour(juce::Label::backgroundColourId, juce::Colour(0xff2B2B2B));
        addAndMakeVisible(fallbackLabel);
        setSize(900, 600);
        return;
    }
#endif

    webView = std::make_unique<juce::WebBrowserComponent>(
        juce::WebBrowserComponent::Options{}
            .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
            .withWinWebView2Options(
                juce::WebBrowserComponent::Options::WinWebView2{}
                    .withUserDataFolder(juce::File::getSpecialLocation(
                        juce::File::SpecialLocationType::tempDirectory)
                            .getChildFile("OTextureForge_WebView")))
            .withNativeIntegrationEnabled()
#if JUCE_WEB_BROWSER_RESOURCE_PROVIDER_AVAILABLE
            .withResourceProvider([this](const auto& url) { return getResource(url); })
#endif
            // Chain all relays
            .withOptionsFrom(*energyRelay)
            .withOptionsFrom(*brightnessRelay)
            .withOptionsFrom(*textureRelay)
            .withOptionsFrom(*positionRelay)
            .withOptionsFrom(*grainDensityRelay)
            .withOptionsFrom(*grainSizeRelay)
            .withOptionsFrom(*scatterXRelay)
            .withOptionsFrom(*scatterYRelay)
            .withOptionsFrom(*variationRelay)
            .withOptionsFrom(*crossfadeRelay)
            .withOptionsFrom(*outputGainRelay)
            .withOptionsFrom(*midiModeRelay)
            // Native functions
            .withNativeFunction("getCorpusData", [this](const juce::Array<juce::var>& /*args*/,
                                                         juce::WebBrowserComponent::NativeFunctionCompletion complete)
            {
                auto corpus = processorRef.getCurrentCorpus();
                if (corpus == nullptr || corpus->pcaX.empty())
                {
                    complete("[]");
                    return;
                }

                // Build JSON array: [[x, y, pitchNorm, energyNorm], ...]
                juce::String json = "[";
                const auto& grains = corpus->grains;
                for (size_t i = 0; i < corpus->pcaX.size(); ++i)
                {
                    if (i > 0) json += ",";
                    float pitchNorm = (i < grains.size()) ? grains[i].descriptors[13] : 0.0f;
                    float energyNorm = (i < grains.size()) ? grains[i].descriptors[17] : 0.0f;
                    json += "[" + juce::String(corpus->pcaX[i], 4)
                          + "," + juce::String(corpus->pcaY[i], 4)
                          + "," + juce::String(pitchNorm, 3)
                          + "," + juce::String(energyNorm, 3) + "]";
                }
                json += "]";
                complete(json);
            })
            .withNativeFunction("selectGrain", [this](const juce::Array<juce::var>& args,
                                                       juce::WebBrowserComponent::NativeFunctionCompletion complete)
            {
                if (args.size() > 0 && args[0].isInt())
                {
                    int grainIndex = static_cast<int>(args[0]);
                    processorRef.selectGrainFromUI(grainIndex);
                }
                complete({});
            })
            .withNativeFunction("setScatterPosition", [this](const juce::Array<juce::var>& args,
                                                              juce::WebBrowserComponent::NativeFunctionCompletion complete)
            {
                if (args.size() >= 2)
                {
                    float x = static_cast<float>(args[0]);
                    float y = static_cast<float>(args[1]);
                    if (auto* param = processorRef.parameters.getParameter("SCATTER_X"))
                        param->setValueNotifyingHost(param->convertTo0to1(x));
                    if (auto* param = processorRef.parameters.getParameter("SCATTER_Y"))
                        param->setValueNotifyingHost(param->convertTo0to1(y));
                }
                complete({});
            })
            .withNativeFunction("cancelUmap", [this](const juce::Array<juce::var>& /*args*/,
                                                      juce::WebBrowserComponent::NativeFunctionCompletion complete)
            {
                processorRef.cancelUmap();
                complete({});
            })
            .withNativeFunction("confirmLargeLoad", [this](const juce::Array<juce::var>& /*args*/,
                                                            juce::WebBrowserComponent::NativeFunctionCompletion complete)
            {
                if (pendingLargeFilePath.isNotEmpty())
                {
                    juce::File file(pendingLargeFilePath);
                    pendingLargeFilePath.clear();
                    if (file.existsAsFile())
                        processorRef.loadCorpusFile(file);
                }
                complete({});
            })
            .withNativeFunction("browseForFile", [this](const juce::Array<juce::var>& /*args*/,
                                                         juce::WebBrowserComponent::NativeFunctionCompletion complete)
            {
                fileChooser = std::make_unique<juce::FileChooser>(
                    "Load Audio File",
                    juce::File{},
                    "*.wav;*.aif;*.aiff;*.mp3;*.flac;*.ogg");

                fileChooser->launchAsync(juce::FileBrowserComponent::openMode
                                       | juce::FileBrowserComponent::canSelectFiles,
                    [this](const juce::FileChooser& fc)
                    {
                        auto result = fc.getResult();
                        if (result.existsAsFile())
                        {
                            constexpr int64_t largeFileThreshold = 104857600;
                            if (result.getSize() > largeFileThreshold)
                            {
                                pendingLargeFilePath = result.getFullPathName();
                                double sizeMB = result.getSize() / (1024.0 * 1024.0);
                                juce::String json = "{\"sizeMB\":" + juce::String(sizeMB, 1) + "}";
                                webView->emitEventIfBrowserIsVisible("fileSizeWarning", json);
                            }
                            else
                            {
                                processorRef.loadCorpusFile(result);
                                juce::Logger::writeToLog("Loading corpus: " + result.getFileName());
                            }
                        }
                    });
                complete({});
            })
            // ── v1.1.0: the UI LANGUAGE pair ───────────────────────────────
            //
            // Plain withNativeFunction, no relay. The page PULLS once at init;
            // there is no push from this constructor, no timer and no revision
            // counter, because the language is not preset content and no preset
            // path can change it behind the page's back. A push from here would
            // race the WebView's load.
            .withNativeFunction("getUiLanguage", [this](const juce::Array<juce::var>& /*args*/,
                                                         juce::WebBrowserComponent::NativeFunctionCompletion complete)
            {
                complete(juce::var(TextureForgeProcessor::languageCode(
                                       processorRef.uiLanguage.load(std::memory_order_acquire))));
            })
            .withNativeFunction("setUiLanguage", [this](const juce::Array<juce::var>& args,
                                                         juce::WebBrowserComponent::NativeFunctionCompletion complete)
            {
                // languageIndex() maps anything that is not "fr" to 0, so an
                // unexpected argument from the page degrades to English rather
                // than being stored unvalidated.
                if (args.size() > 0)
                    processorRef.uiLanguage.store(
                        TextureForgeProcessor::languageIndex(args[0].toString()),
                        std::memory_order_release);

                complete(juce::var(TextureForgeProcessor::languageCode(
                                       processorRef.uiLanguage.load(std::memory_order_acquire))));
            })
    );

    addAndMakeVisible(*webView);

    // 3. Create attachments (after webView)
    energyAttachment       = std::make_unique<juce::WebSliderParameterAttachment>(*processorRef.parameters.getParameter("ENERGY"), *energyRelay);
    brightnessAttachment   = std::make_unique<juce::WebSliderParameterAttachment>(*processorRef.parameters.getParameter("BRIGHTNESS"), *brightnessRelay);
    textureAttachment      = std::make_unique<juce::WebSliderParameterAttachment>(*processorRef.parameters.getParameter("TEXTURE"), *textureRelay);
    positionAttachment     = std::make_unique<juce::WebSliderParameterAttachment>(*processorRef.parameters.getParameter("POSITION"), *positionRelay);
    grainDensityAttachment = std::make_unique<juce::WebSliderParameterAttachment>(*processorRef.parameters.getParameter("GRAIN_DENSITY"), *grainDensityRelay);
    grainSizeAttachment    = std::make_unique<juce::WebSliderParameterAttachment>(*processorRef.parameters.getParameter("GRAIN_SIZE"), *grainSizeRelay);
    scatterXAttachment     = std::make_unique<juce::WebSliderParameterAttachment>(*processorRef.parameters.getParameter("SCATTER_X"), *scatterXRelay);
    scatterYAttachment     = std::make_unique<juce::WebSliderParameterAttachment>(*processorRef.parameters.getParameter("SCATTER_Y"), *scatterYRelay);
    variationAttachment    = std::make_unique<juce::WebSliderParameterAttachment>(*processorRef.parameters.getParameter("VARIATION"), *variationRelay);
    crossfadeAttachment    = std::make_unique<juce::WebSliderParameterAttachment>(*processorRef.parameters.getParameter("CROSSFADE"), *crossfadeRelay);
    outputGainAttachment   = std::make_unique<juce::WebSliderParameterAttachment>(*processorRef.parameters.getParameter("OUTPUT_GAIN"), *outputGainRelay);
    midiModeAttachment     = std::make_unique<juce::WebComboBoxParameterAttachment>(*processorRef.parameters.getParameter("MIDI_MODE"), *midiModeRelay);

#if JUCE_WEB_BROWSER_RESOURCE_PROVIDER_AVAILABLE
    webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
#endif

    startTimerHz(30);
    setSize(900, 600);
}

TextureForgeEditor::~TextureForgeEditor()
{
    stopTimer();

    // Explicit destruction in reverse order: attachments -> webView -> relays
    midiModeAttachment.reset();
    outputGainAttachment.reset();
    crossfadeAttachment.reset();
    variationAttachment.reset();
    scatterYAttachment.reset();
    scatterXAttachment.reset();
    grainSizeAttachment.reset();
    grainDensityAttachment.reset();
    positionAttachment.reset();
    textureAttachment.reset();
    brightnessAttachment.reset();
    energyAttachment.reset();

    webView.reset();

    midiModeRelay.reset();
    outputGainRelay.reset();
    crossfadeRelay.reset();
    variationRelay.reset();
    scatterYRelay.reset();
    scatterXRelay.reset();
    grainSizeRelay.reset();
    grainDensityRelay.reset();
    positionRelay.reset();
    textureRelay.reset();
    brightnessRelay.reset();
    energyRelay.reset();
}

void TextureForgeEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xffF5E6D3));
}

void TextureForgeEditor::resized()
{
    if (webView != nullptr)
        webView->setBounds(getLocalBounds());
}

void TextureForgeEditor::timerCallback()
{
    if (webView == nullptr) return;

    const auto& snap = processorRef.getVizSnapshot();

    // Reuse pre-allocated string buffer to reduce 30Hz allocation pressure
    vizJsonBuffer.clear();
    vizJsonBuffer.preallocateBytes(512);
    vizJsonBuffer << "{\"cx\":" << juce::String(snap.cursorX, 3)
                  << ",\"cy\":" << juce::String(snap.cursorY, 3)
                  << ",\"g\":[";

    for (int i = 0; i < snap.activeCount; ++i)
    {
        if (i > 0) vizJsonBuffer << ",";
        vizJsonBuffer << "{\"i\":" << juce::String(snap.activeGrains[i].grainIndex)
                      << ",\"e\":" << juce::String(snap.activeGrains[i].envelope, 2) << "}";
    }
    vizJsonBuffer << "]}";

    webView->emitEventIfBrowserIsVisible("vizUpdate", vizJsonBuffer);
}

void TextureForgeEditor::onCorpusLoaded(int grainCount)
{
    juce::String json = "{\"count\":" + juce::String(grainCount) + ",\"pcaReady\":true}";
    webView->emitEventIfBrowserIsVisible("corpusLoaded", json);
}

void TextureForgeEditor::onUmapProgress(float progress)
{
    juce::String json = "{\"progress\":" + juce::String(progress, 3) + "}";
    webView->emitEventIfBrowserIsVisible("umapProgress", json);
}

void TextureForgeEditor::onUmapComplete(const juce::String& pointsJson)
{
    webView->emitEventIfBrowserIsVisible("umapComplete", pointsJson);
}

void TextureForgeEditor::onUmapCancelled()
{
    webView->emitEventIfBrowserIsVisible("umapCancelled", "{}");
}

void TextureForgeEditor::onCorpusLoadFailed(const juce::String& reason)
{
    juce::String json = "{\"reason\":\"" + reason.replace("\"", "\\\"") + "\"}";
    webView->emitEventIfBrowserIsVisible("loadFailed", json);
}

void TextureForgeEditor::onCorpusMissing(const juce::String& savedPath)
{
    juce::String escaped = savedPath.replace("\\", "\\\\").replace("\"", "\\\"");
    juce::String json = "{\"path\":\"" + escaped + "\"}";
    webView->emitEventIfBrowserIsVisible("corpusMissing", json);
}

std::optional<juce::WebBrowserComponent::Resource>
TextureForgeEditor::getResource(const juce::String& url)
{
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

    // JavaScript — the label table and the language runtime (v1.1.0). BOTH are
    // embedded in CMakeLists.txt AND served here, in the same commit. A file
    // embedded but not served, or served but not embedded, is a 404 that
    // presents as a page stuck in English and nothing else — check-i18n
    // assertion 8 exists for this pair.
    if (url == "/js/i18n.js")
    {
        return juce::WebBrowserComponent::Resource{
            makeVector(BinaryData::i18n_js, BinaryData::i18n_jsSize),
            juce::String("application/javascript")};
    }

    if (url == "/js/i18n_init.js")
    {
        return juce::WebBrowserComponent::Resource{
            makeVector(BinaryData::i18n_init_js, BinaryData::i18n_init_jsSize),
            juce::String("application/javascript")};
    }

    if (url == "/js/app.bundle.js")
    {
        return juce::WebBrowserComponent::Resource{
            makeVector(BinaryData::app_bundle_js, BinaryData::app_bundle_jsSize),
            juce::String("application/javascript")};
    }

    if (url == "/css/ouaricon-naturalist.css")
    {
        return juce::WebBrowserComponent::Resource{
            makeVector(BinaryData::ouariconnaturalist_css, BinaryData::ouariconnaturalist_cssSize),
            juce::String("text/css")};
    }

    if (url == "/images/fern.png")
    {
        return juce::WebBrowserComponent::Resource{
            makeVector(BinaryData::fern_png, BinaryData::fern_pngSize),
            juce::String("image/png")};
    }

    juce::Logger::writeToLog("Resource not found: " + url);
    return std::nullopt;
}

bool TextureForgeEditor::isInterestedInFileDrag(const juce::StringArray& files)
{
    if (files.isEmpty())
        return false;

    juce::File file(files[0]);
    return file.hasFileExtension(".wav;.aif;.aiff;.mp3;.flac;.ogg");
}

void TextureForgeEditor::filesDropped(const juce::StringArray& files, int /*x*/, int /*y*/)
{
    if (files.isEmpty())
        return;

    juce::File file(files[0]);
    if (file.existsAsFile())
    {
        constexpr int64_t largeFileThreshold = 104857600;  // 100MB
        if (file.getSize() > largeFileThreshold)
        {
            pendingLargeFilePath = file.getFullPathName();
            double sizeMB = file.getSize() / (1024.0 * 1024.0);
            juce::String json = "{\"sizeMB\":" + juce::String(sizeMB, 1) + "}";
            webView->emitEventIfBrowserIsVisible("fileSizeWarning", json);
            return;
        }

        processorRef.loadCorpusFile(file);
        juce::Logger::writeToLog("Loading corpus: " + file.getFileName());
    }
}
