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
    webView->setBounds(getLocalBounds());
}

void TextureForgeEditor::timerCallback()
{
    const auto& snap = processorRef.getVizSnapshot();

    // Build compact JSON: {cx, cy, g:[{i, e}, ...]}
    juce::String json = "{\"cx\":" + juce::String(snap.cursorX, 3)
        + ",\"cy\":" + juce::String(snap.cursorY, 3)
        + ",\"g\":[";

    for (int i = 0; i < snap.activeCount; ++i)
    {
        if (i > 0) json += ",";
        json += "{\"i\":" + juce::String(snap.activeGrains[i].grainIndex)
            + ",\"e\":" + juce::String(snap.activeGrains[i].envelope, 2) + "}";
    }
    json += "]}";

    webView->emitEventIfBrowserIsVisible("vizUpdate", json);
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
        processorRef.loadCorpusFile(file);
        juce::Logger::writeToLog("Loading corpus: " + file.getFileName());
    }
}
