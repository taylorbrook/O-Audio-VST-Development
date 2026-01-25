/*
  ==============================================================================

    O-MultiBandCompressor - Editor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

    Phase 5.2: Parameter binding - all 56 parameters bound to WebView

  ==============================================================================
*/

#include "PluginEditor.h"
#include "BinaryData.h"

OMultiBandCompressorAudioProcessorEditor::OMultiBandCompressorAudioProcessorEditor(OMultiBandCompressorAudioProcessor& p)
    : AudioProcessorEditor(&p)
    , processorRef(p)

    // ========== Initialize attachments (connect parameters to relays) ==========
    // Pattern: Attachment(parameter, relay, undoManager)

    // Global parameter attachments
    , inputGainAttachment(*processorRef.getParameters().getParameter("INPUT_GAIN"), inputGainRelay, nullptr)
    , outputGainAttachment(*processorRef.getParameters().getParameter("OUTPUT_GAIN"), outputGainRelay, nullptr)
    , mixAttachment(*processorRef.getParameters().getParameter("MIX"), mixRelay, nullptr)
    , autoMakeupAttachment(*processorRef.getParameters().getParameter("AUTO_MAKEUP"), autoMakeupRelay, nullptr)
    , msModeAttachment(*processorRef.getParameters().getParameter("MS_MODE"), msModeRelay, nullptr)
    , xover1Attachment(*processorRef.getParameters().getParameter("XOVER1"), xover1Relay, nullptr)
    , xover2Attachment(*processorRef.getParameters().getParameter("XOVER2"), xover2Relay, nullptr)
    , xover3Attachment(*processorRef.getParameters().getParameter("XOVER3"), xover3Relay, nullptr)

    // Low band attachments
    , lowThresholdAttachment(*processorRef.getParameters().getParameter("LOW_THRESHOLD"), lowThresholdRelay, nullptr)
    , lowRatioAttachment(*processorRef.getParameters().getParameter("LOW_RATIO"), lowRatioRelay, nullptr)
    , lowAttackAttachment(*processorRef.getParameters().getParameter("LOW_ATTACK"), lowAttackRelay, nullptr)
    , lowReleaseAttachment(*processorRef.getParameters().getParameter("LOW_RELEASE"), lowReleaseRelay, nullptr)
    , lowKneeAttachment(*processorRef.getParameters().getParameter("LOW_KNEE"), lowKneeRelay, nullptr)
    , lowMakeupAttachment(*processorRef.getParameters().getParameter("LOW_MAKEUP"), lowMakeupRelay, nullptr)
    , lowPeakRmsAttachment(*processorRef.getParameters().getParameter("LOW_PEAK_RMS"), lowPeakRmsRelay, nullptr)
    , lowSoloAttachment(*processorRef.getParameters().getParameter("LOW_SOLO"), lowSoloRelay, nullptr)
    , lowBypassAttachment(*processorRef.getParameters().getParameter("LOW_BYPASS"), lowBypassRelay, nullptr)
    , lowScHpfAttachment(*processorRef.getParameters().getParameter("LOW_SC_HPF"), lowScHpfRelay, nullptr)
    , lowScLpfAttachment(*processorRef.getParameters().getParameter("LOW_SC_LPF"), lowScLpfRelay, nullptr)
    , lowScListenAttachment(*processorRef.getParameters().getParameter("LOW_SC_LISTEN"), lowScListenRelay, nullptr)

    // Low-Mid band attachments
    , lomidThresholdAttachment(*processorRef.getParameters().getParameter("LOMID_THRESHOLD"), lomidThresholdRelay, nullptr)
    , lomidRatioAttachment(*processorRef.getParameters().getParameter("LOMID_RATIO"), lomidRatioRelay, nullptr)
    , lomidAttackAttachment(*processorRef.getParameters().getParameter("LOMID_ATTACK"), lomidAttackRelay, nullptr)
    , lomidReleaseAttachment(*processorRef.getParameters().getParameter("LOMID_RELEASE"), lomidReleaseRelay, nullptr)
    , lomidKneeAttachment(*processorRef.getParameters().getParameter("LOMID_KNEE"), lomidKneeRelay, nullptr)
    , lomidMakeupAttachment(*processorRef.getParameters().getParameter("LOMID_MAKEUP"), lomidMakeupRelay, nullptr)
    , lomidPeakRmsAttachment(*processorRef.getParameters().getParameter("LOMID_PEAK_RMS"), lomidPeakRmsRelay, nullptr)
    , lomidSoloAttachment(*processorRef.getParameters().getParameter("LOMID_SOLO"), lomidSoloRelay, nullptr)
    , lomidBypassAttachment(*processorRef.getParameters().getParameter("LOMID_BYPASS"), lomidBypassRelay, nullptr)
    , lomidScHpfAttachment(*processorRef.getParameters().getParameter("LOMID_SC_HPF"), lomidScHpfRelay, nullptr)
    , lomidScLpfAttachment(*processorRef.getParameters().getParameter("LOMID_SC_LPF"), lomidScLpfRelay, nullptr)
    , lomidScListenAttachment(*processorRef.getParameters().getParameter("LOMID_SC_LISTEN"), lomidScListenRelay, nullptr)

    // High-Mid band attachments
    , himidThresholdAttachment(*processorRef.getParameters().getParameter("HIMID_THRESHOLD"), himidThresholdRelay, nullptr)
    , himidRatioAttachment(*processorRef.getParameters().getParameter("HIMID_RATIO"), himidRatioRelay, nullptr)
    , himidAttackAttachment(*processorRef.getParameters().getParameter("HIMID_ATTACK"), himidAttackRelay, nullptr)
    , himidReleaseAttachment(*processorRef.getParameters().getParameter("HIMID_RELEASE"), himidReleaseRelay, nullptr)
    , himidKneeAttachment(*processorRef.getParameters().getParameter("HIMID_KNEE"), himidKneeRelay, nullptr)
    , himidMakeupAttachment(*processorRef.getParameters().getParameter("HIMID_MAKEUP"), himidMakeupRelay, nullptr)
    , himidPeakRmsAttachment(*processorRef.getParameters().getParameter("HIMID_PEAK_RMS"), himidPeakRmsRelay, nullptr)
    , himidSoloAttachment(*processorRef.getParameters().getParameter("HIMID_SOLO"), himidSoloRelay, nullptr)
    , himidBypassAttachment(*processorRef.getParameters().getParameter("HIMID_BYPASS"), himidBypassRelay, nullptr)
    , himidScHpfAttachment(*processorRef.getParameters().getParameter("HIMID_SC_HPF"), himidScHpfRelay, nullptr)
    , himidScLpfAttachment(*processorRef.getParameters().getParameter("HIMID_SC_LPF"), himidScLpfRelay, nullptr)
    , himidScListenAttachment(*processorRef.getParameters().getParameter("HIMID_SC_LISTEN"), himidScListenRelay, nullptr)

    // High band attachments
    , highThresholdAttachment(*processorRef.getParameters().getParameter("HIGH_THRESHOLD"), highThresholdRelay, nullptr)
    , highRatioAttachment(*processorRef.getParameters().getParameter("HIGH_RATIO"), highRatioRelay, nullptr)
    , highAttackAttachment(*processorRef.getParameters().getParameter("HIGH_ATTACK"), highAttackRelay, nullptr)
    , highReleaseAttachment(*processorRef.getParameters().getParameter("HIGH_RELEASE"), highReleaseRelay, nullptr)
    , highKneeAttachment(*processorRef.getParameters().getParameter("HIGH_KNEE"), highKneeRelay, nullptr)
    , highMakeupAttachment(*processorRef.getParameters().getParameter("HIGH_MAKEUP"), highMakeupRelay, nullptr)
    , highPeakRmsAttachment(*processorRef.getParameters().getParameter("HIGH_PEAK_RMS"), highPeakRmsRelay, nullptr)
    , highSoloAttachment(*processorRef.getParameters().getParameter("HIGH_SOLO"), highSoloRelay, nullptr)
    , highBypassAttachment(*processorRef.getParameters().getParameter("HIGH_BYPASS"), highBypassRelay, nullptr)
    , highScHpfAttachment(*processorRef.getParameters().getParameter("HIGH_SC_HPF"), highScHpfRelay, nullptr)
    , highScLpfAttachment(*processorRef.getParameters().getParameter("HIGH_SC_LPF"), highScLpfRelay, nullptr)
    , highScListenAttachment(*processorRef.getParameters().getParameter("HIGH_SC_LISTEN"), highScListenRelay, nullptr)
{
    // Phase 5.2: Create WebView with all relays registered
    webView = std::make_unique<juce::WebBrowserComponent>(
        juce::WebBrowserComponent::Options{}
            .withNativeIntegrationEnabled()
            .withResourceProvider([this](const auto& url) { return getResource(url); })

            // Register global relays
            .withOptionsFrom(inputGainRelay)
            .withOptionsFrom(outputGainRelay)
            .withOptionsFrom(mixRelay)
            .withOptionsFrom(autoMakeupRelay)
            .withOptionsFrom(msModeRelay)
            .withOptionsFrom(xover1Relay)
            .withOptionsFrom(xover2Relay)
            .withOptionsFrom(xover3Relay)

            // Register Low band relays
            .withOptionsFrom(lowThresholdRelay)
            .withOptionsFrom(lowRatioRelay)
            .withOptionsFrom(lowAttackRelay)
            .withOptionsFrom(lowReleaseRelay)
            .withOptionsFrom(lowKneeRelay)
            .withOptionsFrom(lowMakeupRelay)
            .withOptionsFrom(lowPeakRmsRelay)
            .withOptionsFrom(lowSoloRelay)
            .withOptionsFrom(lowBypassRelay)
            .withOptionsFrom(lowScHpfRelay)
            .withOptionsFrom(lowScLpfRelay)
            .withOptionsFrom(lowScListenRelay)

            // Register Low-Mid band relays
            .withOptionsFrom(lomidThresholdRelay)
            .withOptionsFrom(lomidRatioRelay)
            .withOptionsFrom(lomidAttackRelay)
            .withOptionsFrom(lomidReleaseRelay)
            .withOptionsFrom(lomidKneeRelay)
            .withOptionsFrom(lomidMakeupRelay)
            .withOptionsFrom(lomidPeakRmsRelay)
            .withOptionsFrom(lomidSoloRelay)
            .withOptionsFrom(lomidBypassRelay)
            .withOptionsFrom(lomidScHpfRelay)
            .withOptionsFrom(lomidScLpfRelay)
            .withOptionsFrom(lomidScListenRelay)

            // Register High-Mid band relays
            .withOptionsFrom(himidThresholdRelay)
            .withOptionsFrom(himidRatioRelay)
            .withOptionsFrom(himidAttackRelay)
            .withOptionsFrom(himidReleaseRelay)
            .withOptionsFrom(himidKneeRelay)
            .withOptionsFrom(himidMakeupRelay)
            .withOptionsFrom(himidPeakRmsRelay)
            .withOptionsFrom(himidSoloRelay)
            .withOptionsFrom(himidBypassRelay)
            .withOptionsFrom(himidScHpfRelay)
            .withOptionsFrom(himidScLpfRelay)
            .withOptionsFrom(himidScListenRelay)

            // Register High band relays
            .withOptionsFrom(highThresholdRelay)
            .withOptionsFrom(highRatioRelay)
            .withOptionsFrom(highAttackRelay)
            .withOptionsFrom(highReleaseRelay)
            .withOptionsFrom(highKneeRelay)
            .withOptionsFrom(highMakeupRelay)
            .withOptionsFrom(highPeakRmsRelay)
            .withOptionsFrom(highSoloRelay)
            .withOptionsFrom(highBypassRelay)
            .withOptionsFrom(highScHpfRelay)
            .withOptionsFrom(highScLpfRelay)
            .withOptionsFrom(highScListenRelay)
    );

    addAndMakeVisible(*webView);

    // Set window size
    setSize(900, 600);

    // Start timer for metering updates (30 Hz = 33ms)
    startTimerHz(30);

    // Note: Navigation happens in parentHierarchyChanged() for JUCE 8 compatibility
}

OMultiBandCompressorAudioProcessorEditor::~OMultiBandCompressorAudioProcessorEditor()
{
    // Stop timer before destruction
    stopTimer();

    // WebView automatically cleaned up by unique_ptr
}

void OMultiBandCompressorAudioProcessorEditor::paint(juce::Graphics& g)
{
    // WebView handles all rendering
    juce::ignoreUnused(g);
}

void OMultiBandCompressorAudioProcessorEditor::resized()
{
    // WebView fills entire editor
    if (webView)
        webView->setBounds(getLocalBounds());
}

void OMultiBandCompressorAudioProcessorEditor::parentHierarchyChanged()
{
    // Navigate to UI on first hierarchy change (JUCE 8 pattern)
    if (!hasNavigated && isShowing())
    {
        webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());
        hasNavigated = true;
    }
}

std::optional<juce::WebBrowserComponent::Resource>
OMultiBandCompressorAudioProcessorEditor::getResource(const juce::String& url)
{
    // Map URLs to embedded resources from BinaryData
    auto resource = url.replaceCharacter('\\', '/');

    // Root "/" → index.html
    if (resource == "/" || resource.isEmpty())
        resource = "/index.html";

    // Remove leading slash for BinaryData lookup
    auto path = resource.substring(1);

    // Find in binary data (files embedded from ui/public/)
    for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
    {
        if (path == BinaryData::namedResourceList[i])
        {
            int dataSize = 0;
            const char* data = BinaryData::getNamedResource(
                BinaryData::namedResourceList[i], dataSize);

            // Determine MIME type
            juce::String mimeType = "text/html";
            if (path.endsWith(".css")) mimeType = "text/css";
            if (path.endsWith(".js")) mimeType = "application/javascript";
            if (path.endsWith(".png")) mimeType = "image/png";
            if (path.endsWith(".jpg") || path.endsWith(".jpeg")) mimeType = "image/jpeg";
            if (path.endsWith(".svg")) mimeType = "image/svg+xml";

            // Convert char* to std::vector<std::byte> for JUCE 8
            std::vector<std::byte> dataVector(dataSize);
            std::memcpy(dataVector.data(), data, dataSize);

            return juce::WebBrowserComponent::Resource{
                std::move(dataVector), mimeType
            };
        }
    }

    // Resource not found
    juce::Logger::writeToLog("Resource not found: " + url);
    return std::nullopt;
}

// ========== PHASE 5.3: METERING IMPLEMENTATION ==========

void OMultiBandCompressorAudioProcessorEditor::timerCallback()
{
    // Update all meters at 30 Hz
    sendGainReductionMeters();
    sendInputOutputMeters();
    sendCrossoverPositions();
}

void OMultiBandCompressorAudioProcessorEditor::sendGainReductionMeters()
{
    if (!webView || !hasNavigated)
        return;

    // Read gain reduction from processor (0 to -24 dB)
    float lowGR = processorRef.getLowBandGainReduction();
    float lomidGR = processorRef.getLoMidBandGainReduction();
    float himidGR = processorRef.getHiMidBandGainReduction();
    float highGR = processorRef.getHighBandGainReduction();

    // Normalize to 0-1 range for JavaScript (0 = no GR, 1 = -24 dB GR)
    float lowNorm = juce::jmap(lowGR, 0.0f, -24.0f, 0.0f, 1.0f);
    float lomidNorm = juce::jmap(lomidGR, 0.0f, -24.0f, 0.0f, 1.0f);
    float himidNorm = juce::jmap(himidGR, 0.0f, -24.0f, 0.0f, 1.0f);
    float highNorm = juce::jmap(highGR, 0.0f, -24.0f, 0.0f, 1.0f);

    // Clamp to valid range
    lowNorm = juce::jlimit(0.0f, 1.0f, lowNorm);
    lomidNorm = juce::jlimit(0.0f, 1.0f, lomidNorm);
    himidNorm = juce::jlimit(0.0f, 1.0f, himidNorm);
    highNorm = juce::jlimit(0.0f, 1.0f, highNorm);

    // Call JavaScript function to update meters
    juce::String script = juce::String::formatted(
        "if (typeof updateGainReductionMeters === 'function') { "
        "updateGainReductionMeters(%f, %f, %f, %f); }",
        lowNorm, lomidNorm, himidNorm, highNorm
    );

    webView->evaluateJavascript(script);
}

void OMultiBandCompressorAudioProcessorEditor::sendInputOutputMeters()
{
    if (!webView || !hasNavigated)
        return;

    // Read input/output levels from processor (0.0 to 1.0 linear)
    float inputL = processorRef.getInputLevelL();
    float inputR = processorRef.getInputLevelR();
    float outputL = processorRef.getOutputLevelL();
    float outputR = processorRef.getOutputLevelR();

    // Average stereo channels for single meter display
    float inputLevel = (inputL + inputR) * 0.5f;
    float outputLevel = (outputL + outputR) * 0.5f;

    // Clamp to valid range
    inputLevel = juce::jlimit(0.0f, 1.0f, inputLevel);
    outputLevel = juce::jlimit(0.0f, 1.0f, outputLevel);

    // Call JavaScript function to update meters
    juce::String script = juce::String::formatted(
        "if (typeof updateInputOutputMeters === 'function') { "
        "updateInputOutputMeters(%f, %f); }",
        inputLevel, outputLevel
    );

    webView->evaluateJavascript(script);
}

void OMultiBandCompressorAudioProcessorEditor::sendCrossoverPositions()
{
    if (!webView || !hasNavigated)
        return;

    // Read crossover frequencies from parameters
    auto* xover1Param = processorRef.getParameters().getRawParameterValue("XOVER1");
    auto* xover2Param = processorRef.getParameters().getRawParameterValue("XOVER2");
    auto* xover3Param = processorRef.getParameters().getRawParameterValue("XOVER3");

    if (!xover1Param || !xover2Param || !xover3Param)
        return;

    float xover1 = xover1Param->load();
    float xover2 = xover2Param->load();
    float xover3 = xover3Param->load();

    // Call JavaScript function to update crossover line positions
    juce::String script = juce::String::formatted(
        "if (typeof updateCrossoverPositions === 'function') { "
        "updateCrossoverPositions(%f, %f, %f); }",
        xover1, xover2, xover3
    );

    webView->evaluateJavascript(script);
}
