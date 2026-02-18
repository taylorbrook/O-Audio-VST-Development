/*
  ==============================================================================

    O-IntonationPad - Editor Implementation
    Ouaricon Development
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginEditor.h"
#include "BinaryData.h"

OIntonationPadAudioProcessorEditor::OIntonationPadAudioProcessorEditor(OIntonationPadAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    // 1. Create relays FIRST (with parameter IDs matching HTML)
    voiceCountRelay = std::make_unique<juce::WebSliderRelay>("voiceCount");
    complexityRelay = std::make_unique<juce::WebSliderRelay>("complexity");
    keyRootRelay = std::make_unique<juce::WebSliderRelay>("keyRoot");
    keyScaleRelay = std::make_unique<juce::WebSliderRelay>("keyScale");
    inversionRandomRelay = std::make_unique<juce::WebSliderRelay>("inversionRandom");
    tuningSystemRelay = std::make_unique<juce::WebSliderRelay>("tuningSystem");
    wavetablePosRelay = std::make_unique<juce::WebSliderRelay>("wavetablePos");
    lfoRateRelay = std::make_unique<juce::WebSliderRelay>("lfoRate");
    lfoDepthRelay = std::make_unique<juce::WebSliderRelay>("lfoDepth");
    timingRandomRelay = std::make_unique<juce::WebSliderRelay>("timingRandom");
    detuneRandomRelay = std::make_unique<juce::WebSliderRelay>("detuneRandom");
    attackTimeRelay = std::make_unique<juce::WebSliderRelay>("attackTime");
    releaseTimeRelay = std::make_unique<juce::WebSliderRelay>("releaseTime");
    filterCutoffRelay = std::make_unique<juce::WebSliderRelay>("filterCutoff");
    masterVolumeRelay = std::make_unique<juce::WebSliderRelay>("masterVolume");

    // 2. Create WebView SECOND with all relay options
    webView = std::make_unique<juce::WebBrowserComponent>(
        juce::WebBrowserComponent::Options{}
            .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
            .withWinWebView2Options(
                juce::WebBrowserComponent::Options::WinWebView2{}
                    .withUserDataFolder(juce::File::getSpecialLocation(
                        juce::File::SpecialLocationType::tempDirectory)))
            .withNativeIntegrationEnabled()
            .withResourceProvider([this](auto& url) { return getResource(url); })
            .withOptionsFrom(*voiceCountRelay)
            .withOptionsFrom(*complexityRelay)
            .withOptionsFrom(*keyRootRelay)
            .withOptionsFrom(*keyScaleRelay)
            .withOptionsFrom(*inversionRandomRelay)
            .withOptionsFrom(*tuningSystemRelay)
            .withOptionsFrom(*wavetablePosRelay)
            .withOptionsFrom(*lfoRateRelay)
            .withOptionsFrom(*lfoDepthRelay)
            .withOptionsFrom(*timingRandomRelay)
            .withOptionsFrom(*detuneRandomRelay)
            .withOptionsFrom(*attackTimeRelay)
            .withOptionsFrom(*releaseTimeRelay)
            .withOptionsFrom(*filterCutoffRelay)
            .withOptionsFrom(*masterVolumeRelay)
    );

    // 3. Create attachments LAST (Pattern #12: 3 parameters - parameter, relay, nullptr)
    voiceCountAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.getAPVTS().getParameter("voiceCount"), *voiceCountRelay, nullptr);
    complexityAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.getAPVTS().getParameter("complexity"), *complexityRelay, nullptr);
    keyRootAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.getAPVTS().getParameter("keyRoot"), *keyRootRelay, nullptr);
    keyScaleAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.getAPVTS().getParameter("keyScale"), *keyScaleRelay, nullptr);
    inversionRandomAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.getAPVTS().getParameter("inversionRandom"), *inversionRandomRelay, nullptr);
    tuningSystemAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.getAPVTS().getParameter("tuningSystem"), *tuningSystemRelay, nullptr);
    wavetablePosAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.getAPVTS().getParameter("wavetablePos"), *wavetablePosRelay, nullptr);
    lfoRateAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.getAPVTS().getParameter("lfoRate"), *lfoRateRelay, nullptr);
    lfoDepthAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.getAPVTS().getParameter("lfoDepth"), *lfoDepthRelay, nullptr);
    timingRandomAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.getAPVTS().getParameter("timingRandom"), *timingRandomRelay, nullptr);
    detuneRandomAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.getAPVTS().getParameter("detuneRandom"), *detuneRandomRelay, nullptr);
    attackTimeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.getAPVTS().getParameter("attackTime"), *attackTimeRelay, nullptr);
    releaseTimeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.getAPVTS().getParameter("releaseTime"), *releaseTimeRelay, nullptr);
    filterCutoffAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.getAPVTS().getParameter("filterCutoff"), *filterCutoffRelay, nullptr);
    masterVolumeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.getAPVTS().getParameter("masterVolume"), *masterVolumeRelay, nullptr);

    // Add WebView (navigation happens in parentHierarchyChanged)
    addAndMakeVisible(*webView);

    // Set size AFTER all components are created
    setSize(800, 500);

    // Start timer for active note visualization (30 fps)
    startTimerHz(30);
}

OIntonationPadAudioProcessorEditor::~OIntonationPadAudioProcessorEditor()
{
    stopTimer();
}

void OIntonationPadAudioProcessorEditor::timerCallback()
{
    if (webView == nullptr)
        return;

    auto notes = processorRef.getActiveNotes();

    // Build JSON array of active notes
    juce::String json = "[";
    for (int i = 0; i < static_cast<int>(notes.size()); ++i)
    {
        if (i > 0) json += ",";

        const auto& n = notes[static_cast<size_t>(i)];
        int pitchClass = n.midiNote % 12;
        int octave = (n.midiNote / 12) - 1;

        // Calculate cent deviation from 12-TET
        double tetFreq = 440.0 * std::pow(2.0, (n.midiNote - 69) / 12.0);
        double centDev = 1200.0 * std::log2(static_cast<double>(n.frequencyHz) / tetFreq);

        json += "{\"midi\":" + juce::String(n.midiNote)
             + ",\"pc\":" + juce::String(pitchClass)
             + ",\"oct\":" + juce::String(octave)
             + ",\"hz\":" + juce::String(n.frequencyHz, 2)
             + ",\"cents\":" + juce::String(centDev, 1)
             + ",\"gain\":" + juce::String(n.gain, 3) + "}";
    }
    json += "]";

    webView->emitEventIfBrowserIsVisible("activeNotes", json);
}

void OIntonationPadAudioProcessorEditor::paint(juce::Graphics& g)
{
    // WebView handles all painting
    juce::ignoreUnused(g);
}

void OIntonationPadAudioProcessorEditor::resized()
{
    // WebView fills entire editor
    webView->setBounds(getLocalBounds());
}

void OIntonationPadAudioProcessorEditor::parentHierarchyChanged()
{
    // Navigate WebView only after editor is attached to a window
    if (isShowing() && webView != nullptr && !hasNavigated)
    {
        webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
        hasNavigated = true;
    }
}

// Pattern #8: EXPLICIT URL MAPPING
std::optional<juce::WebBrowserComponent::Resource>
OIntonationPadAudioProcessorEditor::getResource(const juce::String& url)
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

    // JUCE interop checker
    if (url == "/js/juce/check_native_interop.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::check_native_interop_js,
                      BinaryData::check_native_interop_jsSize),
            juce::String("text/javascript")
        };
    }

    // Pitch circle module
    if (url == "/modules/pitch-circle.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::pitchcircle_js, BinaryData::pitchcircle_jsSize),
            juce::String("text/javascript")
        };
    }

    // Background image
    if (url == "/img/paper.jpg") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::paper_jpg, BinaryData::paper_jpgSize),
            juce::String("image/jpeg")
        };
    }

    // Shell botanical overlay
    if (url == "/img/shell.png") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::shell_png, BinaryData::shell_pngSize),
            juce::String("image/png")
        };
    }

    // Resource not found
    juce::Logger::writeToLog("Resource not found: " + url);
    return std::nullopt;
}
