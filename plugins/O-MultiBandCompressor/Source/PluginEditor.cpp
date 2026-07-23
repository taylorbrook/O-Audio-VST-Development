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

namespace
{
    // v1.4.1: tooltip visibility lives in a machine-wide preference file
    // (~/Library/Application Support/Ouaricon/O-MultiBandCompressor.settings on
    // macOS, %APPDATA%\Ouaricon\ on Windows) rather than in the APVTS. It is a
    // user preference, not plugin state: every instance should agree on it, and
    // loading somebody else's preset must not switch your help text off.
    const char* const kTooltipsEnabledKey = "tooltipsEnabled";

    juce::PropertiesFile::Options makeUiPrefsOptions()
    {
        juce::PropertiesFile::Options options;
        options.applicationName     = "O-MultiBandCompressor";
        options.filenameSuffix      = "settings";
        options.folderName          = "Ouaricon";
        options.osxLibrarySubFolder = "Application Support";
        return options;
    }
}

OMultiBandCompressorAudioProcessorEditor::OMultiBandCompressorAudioProcessorEditor(OMultiBandCompressorAudioProcessor& p)
    : AudioProcessorEditor(&p)
    , processorRef(p)
    , uiPrefs(makeUiPrefsOptions())

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
            .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
            .withWinWebView2Options(
                juce::WebBrowserComponent::Options::WinWebView2{}
                    .withUserDataFolder(juce::File::getSpecialLocation(
                        juce::File::SpecialLocationType::tempDirectory)))
            .withNativeIntegrationEnabled()
            .withResourceProvider([this](const auto& url) { return getResource(url); })

            // v1.4.1: the "?" button in the header. The UI asks for the stored
            // preference once at start-up and writes it back on every toggle.
            .withNativeFunction("getTooltipsEnabled",
                [this](const juce::Array<juce::var>&,
                       juce::WebBrowserComponent::NativeFunctionCompletion complete)
                {
                    complete(uiPrefs.getBoolValue(kTooltipsEnabledKey, true));
                })

            .withNativeFunction("setTooltipsEnabled",
                [this](const juce::Array<juce::var>& args,
                       juce::WebBrowserComponent::NativeFunctionCompletion complete)
                {
                    if (args.isEmpty())
                    {
                        complete(juce::var());
                        return;
                    }

                    const auto enabled = static_cast<bool>(args[0]);
                    uiPrefs.setValue(kTooltipsEnabledKey, enabled);

                    // Write through immediately rather than waiting on the
                    // auto-save timer — a DAW that is force-quit would
                    // otherwise drop the change.
                    uiPrefs.saveIfNeeded();

                    complete(enabled);
                })

            // ========== v1.5.0: PRESET MANAGER NATIVE FUNCTIONS ==========
            // The two dialog-driven calls capture a SafePointer rather than `this`:
            // the editor can be destroyed while the OS file dialog is still open, and
            // `complete` is owned by the WebView impl that dies with it. On the null
            // path we return bare — calling complete() there would itself be a
            // use-after-free.

            .withNativeFunction("savePreset",
                [this](const juce::Array<juce::var>& args,
                       juce::WebBrowserComponent::NativeFunctionCompletion complete)
                {
                    if (args.isEmpty())
                    {
                        complete(false);
                        return;
                    }
                    complete(processorRef.presetManager.savePreset(args[0].toString()));
                })

            .withNativeFunction("savePresetWithDialog",
                [this](const juce::Array<juce::var>&,
                       juce::WebBrowserComponent::NativeFunctionCompletion complete)
                {
                    fileChooser = std::make_unique<juce::FileChooser>(
                        "Save Preset",
                        processorRef.presetManager.getUserPresetsDirectory(),
                        "*.json");

                    juce::Component::SafePointer<OMultiBandCompressorAudioProcessorEditor> safeThis(this);
                    fileChooser->launchAsync(
                        juce::FileBrowserComponent::saveMode
                            | juce::FileBrowserComponent::canSelectFiles,
                        [safeThis, complete](const juce::FileChooser& fc)
                        {
                            if (safeThis == nullptr)
                                return;

                            auto* result = new juce::DynamicObject();
                            auto results = fc.getResults();

                            if (results.isEmpty())
                            {
                                result->setProperty("success", false);
                                result->setProperty("name", juce::String());
                                complete(juce::var(result));
                                return;
                            }

                            auto presetName = results.getFirst().getFileNameWithoutExtension();
                            const bool ok = safeThis->processorRef.presetManager.savePreset(presetName);
                            result->setProperty("success", ok);
                            result->setProperty("name", ok ? presetName : juce::String());
                            complete(juce::var(result));
                        });
                })

            .withNativeFunction("loadPreset",
                [this](const juce::Array<juce::var>& args,
                       juce::WebBrowserComponent::NativeFunctionCompletion complete)
                {
                    if (args.isEmpty())
                    {
                        complete(false);
                        return;
                    }
                    complete(processorRef.presetManager.loadPreset(args[0].toString()));
                })

            .withNativeFunction("loadPresetFromFile",
                [this](const juce::Array<juce::var>&,
                       juce::WebBrowserComponent::NativeFunctionCompletion complete)
                {
                    fileChooser = std::make_unique<juce::FileChooser>(
                        "Load Preset",
                        processorRef.presetManager.getPresetsDirectory(),
                        "*.json");

                    juce::Component::SafePointer<OMultiBandCompressorAudioProcessorEditor> safeThis(this);
                    fileChooser->launchAsync(
                        juce::FileBrowserComponent::openMode
                            | juce::FileBrowserComponent::canSelectFiles,
                        [safeThis, complete](const juce::FileChooser& fc)
                        {
                            if (safeThis == nullptr)
                                return;

                            auto* result = new juce::DynamicObject();
                            auto results = fc.getResults();

                            if (results.isEmpty())
                            {
                                result->setProperty("success", false);
                                result->setProperty("name", juce::String());
                                complete(juce::var(result));
                                return;
                            }

                            auto file = results.getFirst();
                            const bool ok = safeThis->processorRef.presetManager.loadPresetFromFile(file);
                            result->setProperty("success", ok);
                            result->setProperty("name", ok ? file.getFileNameWithoutExtension()
                                                          : juce::String());
                            complete(juce::var(result));
                        });
                })

            .withNativeFunction("getPresetList",
                [this](const juce::Array<juce::var>&,
                       juce::WebBrowserComponent::NativeFunctionCompletion complete)
                {
                    juce::Array<juce::var> names;
                    for (const auto& name : processorRef.presetManager.getPresetList())
                        names.add(name);
                    complete(juce::var(names));
                })

            .withNativeFunction("getCurrentPreset",
                [this](const juce::Array<juce::var>&,
                       juce::WebBrowserComponent::NativeFunctionCompletion complete)
                {
                    complete(processorRef.presetManager.getCurrentPresetName());
                })

            .withNativeFunction("selectNextPreset",
                [this](const juce::Array<juce::var>&,
                       juce::WebBrowserComponent::NativeFunctionCompletion complete)
                {
                    complete(processorRef.presetManager.getNextPreset());
                })

            .withNativeFunction("selectPreviousPreset",
                [this](const juce::Array<juce::var>&,
                       juce::WebBrowserComponent::NativeFunctionCompletion complete)
                {
                    complete(processorRef.presetManager.getPreviousPreset());
                })

            .withNativeFunction("deletePreset",
                [this](const juce::Array<juce::var>& args,
                       juce::WebBrowserComponent::NativeFunctionCompletion complete)
                {
                    if (args.isEmpty())
                    {
                        complete(false);
                        return;
                    }
                    complete(processorRef.presetManager.deletePreset(args[0].toString()));
                })

            .withNativeFunction("isFactoryPreset",
                [this](const juce::Array<juce::var>& args,
                       juce::WebBrowserComponent::NativeFunctionCompletion complete)
                {
                    if (args.isEmpty())
                    {
                        complete(false);
                        return;
                    }
                    complete(processorRef.presetManager.isFactoryPreset(args[0].toString()));
                })

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

    // Set window size. Must match .plugin-container in css/styles.css — v1.5.0 grew
    // this from 600 to 640 to seat the per-band Detector/Sidechain knob row.
    setSize(900, 640);

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

    // IN-05: match on the full relative path (not the basename) so same-named files in
    // different folders can never collide. New embedded files must be added here.
    struct ResourceEntry
    {
        const char* path;
        const char* data;
        int size;
        const char* mimeType;
    };

    static const ResourceEntry resourceMap[] = {
        { "/index.html",                       BinaryData::index_html,              BinaryData::index_htmlSize,              "text/html" },
        { "/css/styles.css",                   BinaryData::styles_css,              BinaryData::styles_cssSize,              "text/css" },
        { "/js/app.js",                        BinaryData::app_js,                  BinaryData::app_jsSize,                  "application/javascript" },
        { "/js/juce/index.js",                 BinaryData::index_js,                BinaryData::index_jsSize,                "application/javascript" },
        { "/js/juce/check_native_interop.js",  BinaryData::check_native_interop_js, BinaryData::check_native_interop_jsSize, "application/javascript" },
        { "/modules/preset-manager.js",        BinaryData::presetmanager_js,        BinaryData::presetmanager_jsSize,        "application/javascript" },
    };

    for (const auto& entry : resourceMap)
    {
        if (resource == entry.path)
        {
            // Convert char* to std::vector<std::byte> for JUCE 8
            std::vector<std::byte> dataVector(static_cast<size_t>(entry.size));
            std::memcpy(dataVector.data(), entry.data, static_cast<size_t>(entry.size));

            return juce::WebBrowserComponent::Resource{
                std::move(dataVector), juce::String(entry.mimeType)
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
    sendSpectrumData();  // v1.2.0
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

void OMultiBandCompressorAudioProcessorEditor::sendSpectrumData()
{
    if (!webView || !hasNavigated)
        return;

    // Only send if new FFT data is available
    if (!processorRef.hasNewSpectrumData())
        return;

    // Get spectrum data from processor
    std::array<float, SPECTRUM_BINS> spectrum {};
    processorRef.getSpectrumData(spectrum);
    processorRef.clearSpectrumDataFlag();

    // Build JSON array
    juce::String json = "[";
    for (int i = 0; i < SPECTRUM_BINS; ++i)
    {
        if (i > 0) json += ",";
        json += juce::String(spectrum[static_cast<size_t>(i)], 3);
    }
    json += "]";

    // Call JavaScript function
    juce::String script = "if (typeof updateSpectrumData === 'function') { "
                          "updateSpectrumData(" + json + "); }";

    webView->evaluateJavascript(script);
}
