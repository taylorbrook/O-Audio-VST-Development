/*
  ==============================================================================

    O-Reed - Editor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginEditor.h"
#include "BinaryData.h"

OReedAudioProcessorEditor::OReedAudioProcessorEditor(OReedAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    // ===================================================================
    // 1. CREATE RELAYS (must be created BEFORE WebView)
    // ===================================================================

    // Primary Controls
    breathPressureRelay   = std::make_unique<juce::WebSliderRelay>("breathPressure");
    embouchureRelay       = std::make_unique<juce::WebSliderRelay>("embouchure");
    reedHardnessRelay     = std::make_unique<juce::WebSliderRelay>("reedHardness");
    boreCharacterRelay    = std::make_unique<juce::WebSliderRelay>("boreCharacter");
    instrumentPresetRelay = std::make_unique<juce::WebComboBoxRelay>("instrumentPreset");

    // Secondary Controls
    reedOpeningRelay      = std::make_unique<juce::WebSliderRelay>("reedOpening");
    bellSizeRelay         = std::make_unique<juce::WebSliderRelay>("bellSize");
    airNoiseRelay         = std::make_unique<juce::WebSliderRelay>("airNoise");
    doubleReedRelay       = std::make_unique<juce::WebSliderRelay>("doubleReed");
    boreDiameterRelay     = std::make_unique<juce::WebSliderRelay>("boreDiameter");

    // Advanced / Sound Design
    reedMassRelay         = std::make_unique<juce::WebSliderRelay>("reedMass");
    reedDampingRelay      = std::make_unique<juce::WebSliderRelay>("reedDamping");
    mouthpieceVolRelay    = std::make_unique<juce::WebSliderRelay>("mouthpieceVol");
    toneHoleCutoffRelay   = std::make_unique<juce::WebSliderRelay>("toneHoleCutoff");
    registerHoleRelay     = std::make_unique<juce::WebSliderRelay>("registerHole");
    boreLengthRelay       = std::make_unique<juce::WebSliderRelay>("boreLength");
    boreProfileRelay      = std::make_unique<juce::WebComboBoxRelay>("boreProfile");

    // Expressive Controls
    vibratoDepthRelay     = std::make_unique<juce::WebSliderRelay>("vibratoDepth");
    vibratoRateRelay      = std::make_unique<juce::WebSliderRelay>("vibratoRate");
    vibratoSourceRelay    = std::make_unique<juce::WebComboBoxRelay>("vibratoSource");
    growlAmountRelay      = std::make_unique<juce::WebSliderRelay>("growlAmount");
    flutterTongueRelay    = std::make_unique<juce::WebSliderRelay>("flutterTongue");
    subtoneRelay          = std::make_unique<juce::WebSliderRelay>("subtone");
    attackChiffRelay      = std::make_unique<juce::WebSliderRelay>("attackChiff");

    // Impossible Physics
    infiniteSustainRelay  = std::make_unique<juce::WebSliderRelay>("infiniteSustain");
    reverseBoreRelay      = std::make_unique<juce::WebSliderRelay>("reverseBore");
    dualBoreRelay         = std::make_unique<juce::WebToggleButtonRelay>("dualBore");
    dronePitchRelay       = std::make_unique<juce::WebSliderRelay>("dronePitch");
    feedbackPathRelay     = std::make_unique<juce::WebSliderRelay>("feedbackPath");

    // Tuning
    referencePitchRelay   = std::make_unique<juce::WebSliderRelay>("referencePitch");
    tuningSystemRelay     = std::make_unique<juce::WebComboBoxRelay>("tuningSystem");

    // Voice Configuration
    polyModeRelay         = std::make_unique<juce::WebComboBoxRelay>("polyMode");
    maxVoicesRelay        = std::make_unique<juce::WebSliderRelay>("maxVoices");
    oversamplingRelay     = std::make_unique<juce::WebComboBoxRelay>("oversampling");

    // Output
    outputGainRelay       = std::make_unique<juce::WebSliderRelay>("outputGain");

    // ===================================================================
    // 2. CREATE WEBVIEW with all relays registered
    // ===================================================================
    webView = std::make_unique<juce::WebBrowserComponent>(
        juce::WebBrowserComponent::Options{}
            .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
            .withWinWebView2Options(
                juce::WebBrowserComponent::Options::WinWebView2{}
                    .withUserDataFolder(juce::File::getSpecialLocation(
                        juce::File::SpecialLocationType::tempDirectory)))
            .withNativeIntegrationEnabled()
            .withResourceProvider([this](const juce::String& url) {
                return getResource(url);
            })
            // Primary Controls
            .withOptionsFrom(*breathPressureRelay)
            .withOptionsFrom(*embouchureRelay)
            .withOptionsFrom(*reedHardnessRelay)
            .withOptionsFrom(*boreCharacterRelay)
            .withOptionsFrom(*instrumentPresetRelay)
            // Secondary Controls
            .withOptionsFrom(*reedOpeningRelay)
            .withOptionsFrom(*bellSizeRelay)
            .withOptionsFrom(*airNoiseRelay)
            .withOptionsFrom(*doubleReedRelay)
            .withOptionsFrom(*boreDiameterRelay)
            // Advanced / Sound Design
            .withOptionsFrom(*reedMassRelay)
            .withOptionsFrom(*reedDampingRelay)
            .withOptionsFrom(*mouthpieceVolRelay)
            .withOptionsFrom(*toneHoleCutoffRelay)
            .withOptionsFrom(*registerHoleRelay)
            .withOptionsFrom(*boreLengthRelay)
            .withOptionsFrom(*boreProfileRelay)
            // Expressive Controls
            .withOptionsFrom(*vibratoDepthRelay)
            .withOptionsFrom(*vibratoRateRelay)
            .withOptionsFrom(*vibratoSourceRelay)
            .withOptionsFrom(*growlAmountRelay)
            .withOptionsFrom(*flutterTongueRelay)
            .withOptionsFrom(*subtoneRelay)
            .withOptionsFrom(*attackChiffRelay)
            // Impossible Physics
            .withOptionsFrom(*infiniteSustainRelay)
            .withOptionsFrom(*reverseBoreRelay)
            .withOptionsFrom(*dualBoreRelay)
            .withOptionsFrom(*dronePitchRelay)
            .withOptionsFrom(*feedbackPathRelay)
            // Tuning
            .withOptionsFrom(*referencePitchRelay)
            .withOptionsFrom(*tuningSystemRelay)
            // Voice Configuration
            .withOptionsFrom(*polyModeRelay)
            .withOptionsFrom(*maxVoicesRelay)
            .withOptionsFrom(*oversamplingRelay)
            // Output
            .withOptionsFrom(*outputGainRelay)

            // Preset native functions
            .withNativeFunction("getPresetList", [this](auto, auto complete) {
                auto& pm = processorRef.getPresetManager();
                auto presets = pm.getPresetList();
                juce::Array<juce::var> arr;
                for (const auto& name : presets)
                    arr.add(name);
                complete(juce::var(arr));
            })

            .withNativeFunction("getCurrentPreset", [this](auto, auto complete) {
                auto& pm = processorRef.getPresetManager();
                complete(juce::var(pm.getCurrentPresetName()));
            })

            .withNativeFunction("loadPreset", [this](const auto& args, auto complete) {
                if (args.size() < 1 || !args[0].isString())
                {
                    complete(juce::var(false));
                    return;
                }
                auto& pm = processorRef.getPresetManager();
                bool success = pm.loadPreset(args[0].toString());
                complete(juce::var(success));
            })

            .withNativeFunction("savePreset", [this](const auto& args, auto complete) {
                if (args.size() < 1 || !args[0].isString())
                {
                    complete(juce::var(false));
                    return;
                }
                auto& pm = processorRef.getPresetManager();
                bool success = pm.savePreset(args[0].toString());
                complete(juce::var(success));
            })

            .withNativeFunction("selectNextPreset", [this](auto, auto complete) {
                auto& pm = processorRef.getPresetManager();
                auto nextName = pm.getNextPreset();
                pm.loadPreset(nextName);
                complete(juce::var(pm.getCurrentPresetName()));
            })

            .withNativeFunction("selectPreviousPreset", [this](auto, auto complete) {
                auto& pm = processorRef.getPresetManager();
                auto prevName = pm.getPreviousPreset();
                pm.loadPreset(prevName);
                complete(juce::var(pm.getCurrentPresetName()));
            })

            .withNativeFunction("savePresetWithDialog", [this](auto, auto complete) {
                fileChooser = std::make_shared<juce::FileChooser>(
                    "Save Preset",
                    processorRef.getPresetManager().getUserPresetsDirectory(),
                    "*.json"
                );
                fileChooser->launchAsync(
                    juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                    [this, complete](const juce::FileChooser& fc) {
                        auto result = fc.getResult();
                        if (result == juce::File{})
                        {
                            complete(juce::var(""));
                            return;
                        }
                        auto name = result.getFileNameWithoutExtension();
                        auto& pm = processorRef.getPresetManager();
                        pm.savePreset(name);
                        complete(juce::var(name));
                    }
                );
            })
    );

    addAndMakeVisible(webView.get());

    // ===================================================================
    // 3. CREATE ATTACHMENTS (must be created AFTER WebView)
    // ===================================================================
    auto& apvts = processorRef.getAPVTS();

    // Primary Controls
    breathPressureAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("breathPressure"), *breathPressureRelay, nullptr);
    embouchureAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("embouchure"), *embouchureRelay, nullptr);
    reedHardnessAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("reedHardness"), *reedHardnessRelay, nullptr);
    boreCharacterAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("boreCharacter"), *boreCharacterRelay, nullptr);
    instrumentPresetAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *apvts.getParameter("instrumentPreset"), *instrumentPresetRelay, nullptr);

    // Secondary Controls
    reedOpeningAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("reedOpening"), *reedOpeningRelay, nullptr);
    bellSizeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("bellSize"), *bellSizeRelay, nullptr);
    airNoiseAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("airNoise"), *airNoiseRelay, nullptr);
    doubleReedAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("doubleReed"), *doubleReedRelay, nullptr);
    boreDiameterAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("boreDiameter"), *boreDiameterRelay, nullptr);

    // Advanced / Sound Design
    reedMassAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("reedMass"), *reedMassRelay, nullptr);
    reedDampingAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("reedDamping"), *reedDampingRelay, nullptr);
    mouthpieceVolAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("mouthpieceVol"), *mouthpieceVolRelay, nullptr);
    toneHoleCutoffAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("toneHoleCutoff"), *toneHoleCutoffRelay, nullptr);
    registerHoleAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("registerHole"), *registerHoleRelay, nullptr);
    boreLengthAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("boreLength"), *boreLengthRelay, nullptr);
    boreProfileAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *apvts.getParameter("boreProfile"), *boreProfileRelay, nullptr);

    // Expressive Controls
    vibratoDepthAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("vibratoDepth"), *vibratoDepthRelay, nullptr);
    vibratoRateAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("vibratoRate"), *vibratoRateRelay, nullptr);
    vibratoSourceAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *apvts.getParameter("vibratoSource"), *vibratoSourceRelay, nullptr);
    growlAmountAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("growlAmount"), *growlAmountRelay, nullptr);
    flutterTongueAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("flutterTongue"), *flutterTongueRelay, nullptr);
    subtoneAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("subtone"), *subtoneRelay, nullptr);
    attackChiffAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("attackChiff"), *attackChiffRelay, nullptr);

    // Impossible Physics
    infiniteSustainAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("infiniteSustain"), *infiniteSustainRelay, nullptr);
    reverseBoreAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("reverseBore"), *reverseBoreRelay, nullptr);
    dualBoreAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *apvts.getParameter("dualBore"), *dualBoreRelay, nullptr);
    dronePitchAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("dronePitch"), *dronePitchRelay, nullptr);
    feedbackPathAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("feedbackPath"), *feedbackPathRelay, nullptr);

    // Tuning
    referencePitchAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("referencePitch"), *referencePitchRelay, nullptr);
    tuningSystemAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *apvts.getParameter("tuningSystem"), *tuningSystemRelay, nullptr);

    // Voice Configuration
    polyModeAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *apvts.getParameter("polyMode"), *polyModeRelay, nullptr);
    maxVoicesAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("maxVoices"), *maxVoicesRelay, nullptr);
    oversamplingAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *apvts.getParameter("oversampling"), *oversamplingRelay, nullptr);

    // Output
    outputGainAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("outputGain"), *outputGainRelay, nullptr);

    // Navigate to embedded UI
    webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());

    // Set window size (900x600)
    setSize(900, 600);
}

OReedAudioProcessorEditor::~OReedAudioProcessorEditor()
{
}

void OReedAudioProcessorEditor::paint(juce::Graphics& g)
{
    juce::ignoreUnused(g);  // WebView handles all painting
}

void OReedAudioProcessorEditor::resized()
{
    if (webView)
        webView->setBounds(getLocalBounds());
}

//==============================================================================
// Resource Provider - serves embedded UI files from BinaryData
//==============================================================================
std::optional<juce::WebBrowserComponent::Resource>
OReedAudioProcessorEditor::getResource(const juce::String& url)
{
    auto makeResource = [](const char* data, int size, const char* mimeType) {
        return juce::WebBrowserComponent::Resource {
            std::vector<std::byte>(
                reinterpret_cast<const std::byte*>(data),
                reinterpret_cast<const std::byte*>(data) + size
            ),
            juce::String(mimeType)
        };
    };

    // HTML
    if (url == "/" || url == "/index.html")
        return makeResource(BinaryData::index_html,
                           BinaryData::index_htmlSize,
                           "text/html");

    // JUCE Bridge
    if (url == "/js/juce/index.js")
        return makeResource(BinaryData::index_js,
                           BinaryData::index_jsSize,
                           "text/javascript");

    if (url == "/js/juce/check_native_interop.js")
        return makeResource(BinaryData::check_native_interop_js,
                           BinaryData::check_native_interop_jsSize,
                           "text/javascript");

    // Tuning panel
    if (url == "/js/tuning-panel.js")
        return makeResource(BinaryData::tuningpanel_js,
                           BinaryData::tuningpanel_jsSize,
                           "text/javascript");

    if (url == "/css/tuning-panel.css")
        return makeResource(BinaryData::tuningpanel_css,
                           BinaryData::tuningpanel_cssSize,
                           "text/css");

    DBG("Resource not found: " + url);
    return std::nullopt;
}
