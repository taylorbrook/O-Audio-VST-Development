/*
  ==============================================================================

    OuariconLyrica - Editor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginEditor.h"
#include "BinaryData.h"

OuariconLyricaAudioProcessorEditor::OuariconLyricaAudioProcessorEditor(OuariconLyricaAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    // ═══════════════════════════════════════════════════════════════════
    // Phase 3.1: WebView Setup
    // ═══════════════════════════════════════════════════════════════════

    // 1️⃣ CREATE RELAYS (must be created BEFORE WebView)
    masterVolumeRelay = std::make_unique<juce::WebSliderRelay>("masterVolume");
    brightnessRelay = std::make_unique<juce::WebSliderRelay>("brightness");
    sustainRelay = std::make_unique<juce::WebSliderRelay>("sustain");
    bodySizeRelay = std::make_unique<juce::WebSliderRelay>("bodySize");
    bodyResonanceRelay = std::make_unique<juce::WebSliderRelay>("bodyResonance");
    sympatheticAmountRelay = std::make_unique<juce::WebSliderRelay>("sympatheticAmount");
    pluckPositionRelay = std::make_unique<juce::WebSliderRelay>("pluckPosition");
    fingerHardnessRelay = std::make_unique<juce::WebSliderRelay>("fingerHardness");
    stringTensionRelay = std::make_unique<juce::WebSliderRelay>("stringTension");
    stringGaugeRelay = std::make_unique<juce::WebSliderRelay>("stringGauge");
    stringLengthRelay = std::make_unique<juce::WebSliderRelay>("stringLength");
    stringStiffnessRelay = std::make_unique<juce::WebSliderRelay>("stringStiffness");
    masterTuneRelay = std::make_unique<juce::WebSliderRelay>("masterTune");
    pitchBendRangeRelay = std::make_unique<juce::WebSliderRelay>("pitchBendRange");

    stringMaterialRelay = std::make_unique<juce::WebComboBoxRelay>("stringMaterial");
    woodTypeRelay = std::make_unique<juce::WebComboBoxRelay>("woodType");
    techniqueRelay = std::make_unique<juce::WebComboBoxRelay>("technique");
    glissandoModeRelay = std::make_unique<juce::WebComboBoxRelay>("glissandoMode");
    glissandoScaleRelay = std::make_unique<juce::WebComboBoxRelay>("glissandoScale");

    // 2️⃣ CREATE WEBVIEW with all relays registered
    webView = std::make_unique<juce::WebBrowserComponent>(
        juce::WebBrowserComponent::Options{}
            .withNativeIntegrationEnabled()
            .withResourceProvider([this](const juce::String& url) {
                return getResource(url);
            })
            // Phase 3.3: Native function to get voice count
            // JUCE 8 async callback pattern: (args, complete) -> void
            .withNativeFunction("getVoiceCount", [this](const juce::Array<juce::var>&,
                                                         std::function<void(juce::var)> complete) {
                complete(juce::var(processorRef.getActiveVoiceCount()));
            })
            // Register all slider relays
            .withOptionsFrom(*masterVolumeRelay)
            .withOptionsFrom(*brightnessRelay)
            .withOptionsFrom(*sustainRelay)
            .withOptionsFrom(*bodySizeRelay)
            .withOptionsFrom(*bodyResonanceRelay)
            .withOptionsFrom(*sympatheticAmountRelay)
            .withOptionsFrom(*pluckPositionRelay)
            .withOptionsFrom(*fingerHardnessRelay)
            .withOptionsFrom(*stringTensionRelay)
            .withOptionsFrom(*stringGaugeRelay)
            .withOptionsFrom(*stringLengthRelay)
            .withOptionsFrom(*stringStiffnessRelay)
            .withOptionsFrom(*masterTuneRelay)
            .withOptionsFrom(*pitchBendRangeRelay)
            // Register all choice relays
            .withOptionsFrom(*stringMaterialRelay)
            .withOptionsFrom(*woodTypeRelay)
            .withOptionsFrom(*techniqueRelay)
            .withOptionsFrom(*glissandoModeRelay)
            .withOptionsFrom(*glissandoScaleRelay)
    );

    // 3️⃣ CREATE ATTACHMENTS (must be created AFTER WebView)
    // CRITICAL: JUCE 8 requires 3 parameters (parameter, relay, undoManager)
    auto& apvts = processorRef.getAPVTS();

    masterVolumeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("masterVolume"), *masterVolumeRelay, nullptr);
    brightnessAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("brightness"), *brightnessRelay, nullptr);
    sustainAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("sustain"), *sustainRelay, nullptr);
    bodySizeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("bodySize"), *bodySizeRelay, nullptr);
    bodyResonanceAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("bodyResonance"), *bodyResonanceRelay, nullptr);
    sympatheticAmountAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("sympatheticAmount"), *sympatheticAmountRelay, nullptr);
    pluckPositionAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("pluckPosition"), *pluckPositionRelay, nullptr);
    fingerHardnessAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("fingerHardness"), *fingerHardnessRelay, nullptr);
    stringTensionAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("stringTension"), *stringTensionRelay, nullptr);
    stringGaugeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("stringGauge"), *stringGaugeRelay, nullptr);
    stringLengthAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("stringLength"), *stringLengthRelay, nullptr);
    stringStiffnessAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("stringStiffness"), *stringStiffnessRelay, nullptr);
    masterTuneAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("masterTune"), *masterTuneRelay, nullptr);
    pitchBendRangeAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
        *apvts.getParameter("pitchBendRange"), *pitchBendRangeRelay, nullptr);

    stringMaterialAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *apvts.getParameter("stringMaterial"), *stringMaterialRelay, nullptr);
    woodTypeAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *apvts.getParameter("woodType"), *woodTypeRelay, nullptr);
    techniqueAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *apvts.getParameter("technique"), *techniqueRelay, nullptr);
    glissandoModeAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *apvts.getParameter("glissandoMode"), *glissandoModeRelay, nullptr);
    glissandoScaleAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
        *apvts.getParameter("glissandoScale"), *glissandoScaleRelay, nullptr);

    // 4️⃣ SETUP WEBVIEW
    addAndMakeVisible(*webView);

    // Navigate to UI (uses resource provider)
    webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());

    // Set editor size
    setSize(800, 600);
}

OuariconLyricaAudioProcessorEditor::~OuariconLyricaAudioProcessorEditor()
{
    // Destructor runs in REVERSE order of declaration:
    // 1. Attachments destroyed first (safe - webView still exists)
    // 2. WebView destroyed second (safe - relays still exist)
    // 3. Relays destroyed last
}

void OuariconLyricaAudioProcessorEditor::paint(juce::Graphics& g)
{
    // WebView handles all painting
    juce::ignoreUnused(g);
}

void OuariconLyricaAudioProcessorEditor::resized()
{
    // WebView fills entire editor
    if (webView)
        webView->setBounds(getLocalBounds());
}

std::optional<juce::WebBrowserComponent::Resource>
OuariconLyricaAudioProcessorEditor::getResource(const juce::String& url)
{
    // Helper lambda to create resource from BinaryData
    auto makeResource = [](const char* data, int size, const char* mimeType) {
        return juce::WebBrowserComponent::Resource {
            std::vector<std::byte>(
                reinterpret_cast<const std::byte*>(data),
                reinterpret_cast<const std::byte*>(data) + size
            ),
            juce::String(mimeType)
        };
    };

    // ═══════════════════════════════════════════════════════════════════
    // CRITICAL: Explicit URL mapping (not generic loop)
    // BinaryData converts paths to C++ identifiers:
    //   index.html → index_html
    //   js/juce/index.js → index_js (path flattened)
    // ═══════════════════════════════════════════════════════════════════

    // HTML
    if (url == "/" || url == "/index.html")
        return makeResource(BinaryData::index_html,
                           BinaryData::index_htmlSize,
                           "text/html");

    // JUCE Bridge Library
    if (url == "/js/juce/index.js")
        return makeResource(BinaryData::index_js,
                           BinaryData::index_jsSize,
                           "text/javascript");

    // Native Interop Check (REQUIRED for WebView)
    if (url == "/js/juce/check_native_interop.js")
        return makeResource(BinaryData::check_native_interop_js,
                           BinaryData::check_native_interop_jsSize,
                           "text/javascript");

    // App JavaScript
    if (url == "/js/app.js")
        return makeResource(BinaryData::app_js,
                           BinaryData::app_jsSize,
                           "text/javascript");

    // CSS
    if (url == "/css/styles.css")
        return makeResource(BinaryData::styles_css,
                           BinaryData::styles_cssSize,
                           "text/css");

    // 404 for unknown resources
    DBG("Resource not found: " + url);
    return std::nullopt;
}
