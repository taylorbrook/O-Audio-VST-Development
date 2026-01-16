/*
  ==============================================================================

    Ouaricon Polystutter - Editor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginEditor.h"
#include "BinaryData.h"

OuariconPolystutterAudioProcessorEditor::OuariconPolystutterAudioProcessorEditor(OuariconPolystutterAudioProcessor& p)
    : AudioProcessorEditor(&p)
    , processorRef(p)

    // Phase 3.2: Initialize all relays FIRST (no dependencies)
    // Lane 1 relays
    , lane1RepeatsRelay(std::make_unique<juce::WebSliderRelay>("lane1_repeats"))
    , lane1DecayRelay(std::make_unique<juce::WebSliderRelay>("lane1_decay"))
    , lane1PitchRelay(std::make_unique<juce::WebSliderRelay>("lane1_pitch"))
    , lane1FilterRelay(std::make_unique<juce::WebSliderRelay>("lane1_filter"))
    , lane1ProbabilityRelay(std::make_unique<juce::WebSliderRelay>("lane1_probability"))
    , lane1VolumeRelay(std::make_unique<juce::WebSliderRelay>("lane1_volume"))
    , lane1PanRelay(std::make_unique<juce::WebSliderRelay>("lane1_pan"))
    , lane1SwingRelay(std::make_unique<juce::WebSliderRelay>("lane1_swing"))
    , lane1EnabledRelay(std::make_unique<juce::WebToggleButtonRelay>("lane1_enabled"))
    , lane1PingpongRelay(std::make_unique<juce::WebToggleButtonRelay>("lane1_pingpong"))
    , lane1ReverseRelay(std::make_unique<juce::WebToggleButtonRelay>("lane1_reverse"))
    , lane1ManualRelay(std::make_unique<juce::WebToggleButtonRelay>("lane1_manual_time_enabled"))
    , lane1FreezeRelay(std::make_unique<juce::WebToggleButtonRelay>("lane1_freeze"))
    , lane1SubdivisionRelay(std::make_unique<juce::WebComboBoxRelay>("lane1_subdivision"))

    // Lane 2 relays
    , lane2RepeatsRelay(std::make_unique<juce::WebSliderRelay>("lane2_repeats"))
    , lane2DecayRelay(std::make_unique<juce::WebSliderRelay>("lane2_decay"))
    , lane2PitchRelay(std::make_unique<juce::WebSliderRelay>("lane2_pitch"))
    , lane2FilterRelay(std::make_unique<juce::WebSliderRelay>("lane2_filter"))
    , lane2ProbabilityRelay(std::make_unique<juce::WebSliderRelay>("lane2_probability"))
    , lane2VolumeRelay(std::make_unique<juce::WebSliderRelay>("lane2_volume"))
    , lane2PanRelay(std::make_unique<juce::WebSliderRelay>("lane2_pan"))
    , lane2SwingRelay(std::make_unique<juce::WebSliderRelay>("lane2_swing"))
    , lane2EnabledRelay(std::make_unique<juce::WebToggleButtonRelay>("lane2_enabled"))
    , lane2PingpongRelay(std::make_unique<juce::WebToggleButtonRelay>("lane2_pingpong"))
    , lane2ReverseRelay(std::make_unique<juce::WebToggleButtonRelay>("lane2_reverse"))
    , lane2ManualRelay(std::make_unique<juce::WebToggleButtonRelay>("lane2_manual_time_enabled"))
    , lane2FreezeRelay(std::make_unique<juce::WebToggleButtonRelay>("lane2_freeze"))
    , lane2SubdivisionRelay(std::make_unique<juce::WebComboBoxRelay>("lane2_subdivision"))

    // Lane 3 relays
    , lane3RepeatsRelay(std::make_unique<juce::WebSliderRelay>("lane3_repeats"))
    , lane3DecayRelay(std::make_unique<juce::WebSliderRelay>("lane3_decay"))
    , lane3PitchRelay(std::make_unique<juce::WebSliderRelay>("lane3_pitch"))
    , lane3FilterRelay(std::make_unique<juce::WebSliderRelay>("lane3_filter"))
    , lane3ProbabilityRelay(std::make_unique<juce::WebSliderRelay>("lane3_probability"))
    , lane3VolumeRelay(std::make_unique<juce::WebSliderRelay>("lane3_volume"))
    , lane3PanRelay(std::make_unique<juce::WebSliderRelay>("lane3_pan"))
    , lane3SwingRelay(std::make_unique<juce::WebSliderRelay>("lane3_swing"))
    , lane3EnabledRelay(std::make_unique<juce::WebToggleButtonRelay>("lane3_enabled"))
    , lane3PingpongRelay(std::make_unique<juce::WebToggleButtonRelay>("lane3_pingpong"))
    , lane3ReverseRelay(std::make_unique<juce::WebToggleButtonRelay>("lane3_reverse"))
    , lane3ManualRelay(std::make_unique<juce::WebToggleButtonRelay>("lane3_manual_time_enabled"))
    , lane3FreezeRelay(std::make_unique<juce::WebToggleButtonRelay>("lane3_freeze"))
    , lane3SubdivisionRelay(std::make_unique<juce::WebComboBoxRelay>("lane3_subdivision"))

    // Lane 4 relays
    , lane4RepeatsRelay(std::make_unique<juce::WebSliderRelay>("lane4_repeats"))
    , lane4DecayRelay(std::make_unique<juce::WebSliderRelay>("lane4_decay"))
    , lane4PitchRelay(std::make_unique<juce::WebSliderRelay>("lane4_pitch"))
    , lane4FilterRelay(std::make_unique<juce::WebSliderRelay>("lane4_filter"))
    , lane4ProbabilityRelay(std::make_unique<juce::WebSliderRelay>("lane4_probability"))
    , lane4VolumeRelay(std::make_unique<juce::WebSliderRelay>("lane4_volume"))
    , lane4PanRelay(std::make_unique<juce::WebSliderRelay>("lane4_pan"))
    , lane4SwingRelay(std::make_unique<juce::WebSliderRelay>("lane4_swing"))
    , lane4EnabledRelay(std::make_unique<juce::WebToggleButtonRelay>("lane4_enabled"))
    , lane4PingpongRelay(std::make_unique<juce::WebToggleButtonRelay>("lane4_pingpong"))
    , lane4ReverseRelay(std::make_unique<juce::WebToggleButtonRelay>("lane4_reverse"))
    , lane4ManualRelay(std::make_unique<juce::WebToggleButtonRelay>("lane4_manual_time_enabled"))
    , lane4FreezeRelay(std::make_unique<juce::WebToggleButtonRelay>("lane4_freeze"))
    , lane4SubdivisionRelay(std::make_unique<juce::WebComboBoxRelay>("lane4_subdivision"))

    // Tape degradation relays
    , tapeSaturationRelay(std::make_unique<juce::WebSliderRelay>("tape_saturation"))
    , tapeWowRelay(std::make_unique<juce::WebSliderRelay>("tape_wow"))
    , tapeFlutterRelay(std::make_unique<juce::WebSliderRelay>("tape_flutter"))
    , tapeHissRelay(std::make_unique<juce::WebSliderRelay>("tape_hiss"))
    , tapeRolloffRelay(std::make_unique<juce::WebSliderRelay>("tape_rolloff"))
    , tapeDropoutRelay(std::make_unique<juce::WebSliderRelay>("tape_dropout"))

    // Global control relays
    , envelopeEnabledRelay(std::make_unique<juce::WebToggleButtonRelay>("envelope_enabled"))
    , sidechainEnabledRelay(std::make_unique<juce::WebToggleButtonRelay>("sidechain_enabled"))
    , midiEnabledRelay(std::make_unique<juce::WebToggleButtonRelay>("midi_enabled"))
    , manualTriggerRelay(std::make_unique<juce::WebToggleButtonRelay>("manual_trigger"))

    // Initialize WebView SECOND (depends on relays via withOptionsFrom)
    , webView(std::make_unique<juce::WebBrowserComponent>(
        juce::WebBrowserComponent::Options{}
            .withNativeIntegrationEnabled()
            .withResourceProvider([this](const auto& url) { return getResource(url); })
            // Register ALL relays (CRITICAL: must match parameter IDs exactly)
            // Lane 1
            .withOptionsFrom(*lane1RepeatsRelay)
            .withOptionsFrom(*lane1DecayRelay)
            .withOptionsFrom(*lane1PitchRelay)
            .withOptionsFrom(*lane1FilterRelay)
            .withOptionsFrom(*lane1ProbabilityRelay)
            .withOptionsFrom(*lane1VolumeRelay)
            .withOptionsFrom(*lane1PanRelay)
            .withOptionsFrom(*lane1SwingRelay)
            .withOptionsFrom(*lane1EnabledRelay)
            .withOptionsFrom(*lane1PingpongRelay)
            .withOptionsFrom(*lane1ReverseRelay)
            .withOptionsFrom(*lane1ManualRelay)
            .withOptionsFrom(*lane1FreezeRelay)
            .withOptionsFrom(*lane1SubdivisionRelay)
            // Lane 2
            .withOptionsFrom(*lane2RepeatsRelay)
            .withOptionsFrom(*lane2DecayRelay)
            .withOptionsFrom(*lane2PitchRelay)
            .withOptionsFrom(*lane2FilterRelay)
            .withOptionsFrom(*lane2ProbabilityRelay)
            .withOptionsFrom(*lane2VolumeRelay)
            .withOptionsFrom(*lane2PanRelay)
            .withOptionsFrom(*lane2SwingRelay)
            .withOptionsFrom(*lane2EnabledRelay)
            .withOptionsFrom(*lane2PingpongRelay)
            .withOptionsFrom(*lane2ReverseRelay)
            .withOptionsFrom(*lane2ManualRelay)
            .withOptionsFrom(*lane2FreezeRelay)
            .withOptionsFrom(*lane2SubdivisionRelay)
            // Lane 3
            .withOptionsFrom(*lane3RepeatsRelay)
            .withOptionsFrom(*lane3DecayRelay)
            .withOptionsFrom(*lane3PitchRelay)
            .withOptionsFrom(*lane3FilterRelay)
            .withOptionsFrom(*lane3ProbabilityRelay)
            .withOptionsFrom(*lane3VolumeRelay)
            .withOptionsFrom(*lane3PanRelay)
            .withOptionsFrom(*lane3SwingRelay)
            .withOptionsFrom(*lane3EnabledRelay)
            .withOptionsFrom(*lane3PingpongRelay)
            .withOptionsFrom(*lane3ReverseRelay)
            .withOptionsFrom(*lane3ManualRelay)
            .withOptionsFrom(*lane3FreezeRelay)
            .withOptionsFrom(*lane3SubdivisionRelay)
            // Lane 4
            .withOptionsFrom(*lane4RepeatsRelay)
            .withOptionsFrom(*lane4DecayRelay)
            .withOptionsFrom(*lane4PitchRelay)
            .withOptionsFrom(*lane4FilterRelay)
            .withOptionsFrom(*lane4ProbabilityRelay)
            .withOptionsFrom(*lane4VolumeRelay)
            .withOptionsFrom(*lane4PanRelay)
            .withOptionsFrom(*lane4SwingRelay)
            .withOptionsFrom(*lane4EnabledRelay)
            .withOptionsFrom(*lane4PingpongRelay)
            .withOptionsFrom(*lane4ReverseRelay)
            .withOptionsFrom(*lane4ManualRelay)
            .withOptionsFrom(*lane4FreezeRelay)
            .withOptionsFrom(*lane4SubdivisionRelay)
            // Tape degradation
            .withOptionsFrom(*tapeSaturationRelay)
            .withOptionsFrom(*tapeWowRelay)
            .withOptionsFrom(*tapeFlutterRelay)
            .withOptionsFrom(*tapeHissRelay)
            .withOptionsFrom(*tapeRolloffRelay)
            .withOptionsFrom(*tapeDropoutRelay)
            // Global controls
            .withOptionsFrom(*envelopeEnabledRelay)
            .withOptionsFrom(*sidechainEnabledRelay)
            .withOptionsFrom(*midiEnabledRelay)
            .withOptionsFrom(*manualTriggerRelay)
    ))

    // Initialize attachments LAST (depend on both relays and webView)
    // Pattern #8: WebSliderParameterAttachment requires 3 parameters in JUCE 8 (add nullptr for undoManager)
    // Lane 1 attachments
    , lane1RepeatsAttachment(std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.apvts.getParameter("lane1_repeats"), *lane1RepeatsRelay, nullptr))
    , lane1DecayAttachment(std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.apvts.getParameter("lane1_decay"), *lane1DecayRelay, nullptr))
    , lane1PitchAttachment(std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.apvts.getParameter("lane1_pitch"), *lane1PitchRelay, nullptr))
    , lane1FilterAttachment(std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.apvts.getParameter("lane1_filter"), *lane1FilterRelay, nullptr))
    , lane1ProbabilityAttachment(std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.apvts.getParameter("lane1_probability"), *lane1ProbabilityRelay, nullptr))
    , lane1VolumeAttachment(std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.apvts.getParameter("lane1_volume"), *lane1VolumeRelay, nullptr))
    , lane1PanAttachment(std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.apvts.getParameter("lane1_pan"), *lane1PanRelay, nullptr))
    , lane1SwingAttachment(std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.apvts.getParameter("lane1_swing"), *lane1SwingRelay, nullptr))
    , lane1EnabledAttachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("lane1_enabled"), *lane1EnabledRelay, nullptr))
    , lane1PingpongAttachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("lane1_pingpong"), *lane1PingpongRelay, nullptr))
    , lane1ReverseAttachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("lane1_reverse"), *lane1ReverseRelay, nullptr))
    , lane1ManualAttachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("lane1_manual_time_enabled"), *lane1ManualRelay, nullptr))
    , lane1FreezeAttachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("lane1_freeze"), *lane1FreezeRelay, nullptr))
    , lane1SubdivisionAttachment(std::make_unique<juce::WebComboBoxParameterAttachment>(
        *processorRef.apvts.getParameter("lane1_subdivision"), *lane1SubdivisionRelay, nullptr))

    // Lane 2 attachments
    , lane2RepeatsAttachment(std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.apvts.getParameter("lane2_repeats"), *lane2RepeatsRelay, nullptr))
    , lane2DecayAttachment(std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.apvts.getParameter("lane2_decay"), *lane2DecayRelay, nullptr))
    , lane2PitchAttachment(std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.apvts.getParameter("lane2_pitch"), *lane2PitchRelay, nullptr))
    , lane2FilterAttachment(std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.apvts.getParameter("lane2_filter"), *lane2FilterRelay, nullptr))
    , lane2ProbabilityAttachment(std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.apvts.getParameter("lane2_probability"), *lane2ProbabilityRelay, nullptr))
    , lane2VolumeAttachment(std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.apvts.getParameter("lane2_volume"), *lane2VolumeRelay, nullptr))
    , lane2PanAttachment(std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.apvts.getParameter("lane2_pan"), *lane2PanRelay, nullptr))
    , lane2SwingAttachment(std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.apvts.getParameter("lane2_swing"), *lane2SwingRelay, nullptr))
    , lane2EnabledAttachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("lane2_enabled"), *lane2EnabledRelay, nullptr))
    , lane2PingpongAttachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("lane2_pingpong"), *lane2PingpongRelay, nullptr))
    , lane2ReverseAttachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("lane2_reverse"), *lane2ReverseRelay, nullptr))
    , lane2ManualAttachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("lane2_manual_time_enabled"), *lane2ManualRelay, nullptr))
    , lane2FreezeAttachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("lane2_freeze"), *lane2FreezeRelay, nullptr))
    , lane2SubdivisionAttachment(std::make_unique<juce::WebComboBoxParameterAttachment>(
        *processorRef.apvts.getParameter("lane2_subdivision"), *lane2SubdivisionRelay, nullptr))

    // Lane 3 attachments
    , lane3RepeatsAttachment(std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.apvts.getParameter("lane3_repeats"), *lane3RepeatsRelay, nullptr))
    , lane3DecayAttachment(std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.apvts.getParameter("lane3_decay"), *lane3DecayRelay, nullptr))
    , lane3PitchAttachment(std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.apvts.getParameter("lane3_pitch"), *lane3PitchRelay, nullptr))
    , lane3FilterAttachment(std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.apvts.getParameter("lane3_filter"), *lane3FilterRelay, nullptr))
    , lane3ProbabilityAttachment(std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.apvts.getParameter("lane3_probability"), *lane3ProbabilityRelay, nullptr))
    , lane3VolumeAttachment(std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.apvts.getParameter("lane3_volume"), *lane3VolumeRelay, nullptr))
    , lane3PanAttachment(std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.apvts.getParameter("lane3_pan"), *lane3PanRelay, nullptr))
    , lane3SwingAttachment(std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.apvts.getParameter("lane3_swing"), *lane3SwingRelay, nullptr))
    , lane3EnabledAttachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("lane3_enabled"), *lane3EnabledRelay, nullptr))
    , lane3PingpongAttachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("lane3_pingpong"), *lane3PingpongRelay, nullptr))
    , lane3ReverseAttachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("lane3_reverse"), *lane3ReverseRelay, nullptr))
    , lane3ManualAttachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("lane3_manual_time_enabled"), *lane3ManualRelay, nullptr))
    , lane3FreezeAttachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("lane3_freeze"), *lane3FreezeRelay, nullptr))
    , lane3SubdivisionAttachment(std::make_unique<juce::WebComboBoxParameterAttachment>(
        *processorRef.apvts.getParameter("lane3_subdivision"), *lane3SubdivisionRelay, nullptr))

    // Lane 4 attachments
    , lane4RepeatsAttachment(std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.apvts.getParameter("lane4_repeats"), *lane4RepeatsRelay, nullptr))
    , lane4DecayAttachment(std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.apvts.getParameter("lane4_decay"), *lane4DecayRelay, nullptr))
    , lane4PitchAttachment(std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.apvts.getParameter("lane4_pitch"), *lane4PitchRelay, nullptr))
    , lane4FilterAttachment(std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.apvts.getParameter("lane4_filter"), *lane4FilterRelay, nullptr))
    , lane4ProbabilityAttachment(std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.apvts.getParameter("lane4_probability"), *lane4ProbabilityRelay, nullptr))
    , lane4VolumeAttachment(std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.apvts.getParameter("lane4_volume"), *lane4VolumeRelay, nullptr))
    , lane4PanAttachment(std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.apvts.getParameter("lane4_pan"), *lane4PanRelay, nullptr))
    , lane4SwingAttachment(std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.apvts.getParameter("lane4_swing"), *lane4SwingRelay, nullptr))
    , lane4EnabledAttachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("lane4_enabled"), *lane4EnabledRelay, nullptr))
    , lane4PingpongAttachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("lane4_pingpong"), *lane4PingpongRelay, nullptr))
    , lane4ReverseAttachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("lane4_reverse"), *lane4ReverseRelay, nullptr))
    , lane4ManualAttachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("lane4_manual_time_enabled"), *lane4ManualRelay, nullptr))
    , lane4FreezeAttachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("lane4_freeze"), *lane4FreezeRelay, nullptr))
    , lane4SubdivisionAttachment(std::make_unique<juce::WebComboBoxParameterAttachment>(
        *processorRef.apvts.getParameter("lane4_subdivision"), *lane4SubdivisionRelay, nullptr))

    // Tape degradation attachments
    , tapeSaturationAttachment(std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.apvts.getParameter("tape_saturation"), *tapeSaturationRelay, nullptr))
    , tapeWowAttachment(std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.apvts.getParameter("tape_wow"), *tapeWowRelay, nullptr))
    , tapeFlutterAttachment(std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.apvts.getParameter("tape_flutter"), *tapeFlutterRelay, nullptr))
    , tapeHissAttachment(std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.apvts.getParameter("tape_hiss"), *tapeHissRelay, nullptr))
    , tapeRolloffAttachment(std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.apvts.getParameter("tape_rolloff"), *tapeRolloffRelay, nullptr))
    , tapeDropoutAttachment(std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.apvts.getParameter("tape_dropout"), *tapeDropoutRelay, nullptr))

    // Global control attachments
    , envelopeEnabledAttachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("envelope_enabled"), *envelopeEnabledRelay, nullptr))
    , sidechainEnabledAttachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("sidechain_enabled"), *sidechainEnabledRelay, nullptr))
    , midiEnabledAttachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("midi_enabled"), *midiEnabledRelay, nullptr))
    , manualTriggerAttachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("manual_trigger"), *manualTriggerRelay, nullptr))
{
    addAndMakeVisible(*webView);

    // Navigate to UI (served from BinaryData)
    webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());

    // Fixed window size: 1000×750px (from v5-ui.yaml spec)
    setSize(1000, 750);

    DBG("[Phase 3.2] Parameter bindings initialized - 64 relays + 64 attachments");
}

OuariconPolystutterAudioProcessorEditor::~OuariconPolystutterAudioProcessorEditor()
{
}

void OuariconPolystutterAudioProcessorEditor::paint(juce::Graphics& g)
{
    // WebView handles all rendering (no custom paint needed)
    juce::ignoreUnused(g);
}

void OuariconPolystutterAudioProcessorEditor::resized()
{
    // WebView fills entire editor window
    if (webView)
        webView->setBounds(getLocalBounds());
}

std::optional<juce::WebBrowserComponent::Resource>
OuariconPolystutterAudioProcessorEditor::getResource(const juce::String& url)
{
    // Helper lambda for converting char* to std::vector<std::byte>
    auto makeVector = [](const char* data, int size) {
        return std::vector<std::byte>(
            reinterpret_cast<const std::byte*>(data),
            reinterpret_cast<const std::byte*>(data) + size
        );
    };

    // Pattern #5: Explicit URL mapping (prevents 404 errors)
    // Map URLs to embedded resources from BinaryData

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

    if (url == "/js/juce/check_native_interop.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::check_native_interop_js, BinaryData::check_native_interop_jsSize),
            juce::String("text/javascript")
        };
    }

    // Phase 3.2: Parameter bindings script
    if (url == "/js/parameter-bindings.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::parameterbindings_js, BinaryData::parameterbindings_jsSize),
            juce::String("text/javascript")
        };
    }

    // Images (paper background and botanical overlay)
    if (url == "/img/paper-background.jpg") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::paperbackground_jpg, BinaryData::paperbackground_jpgSize),
            juce::String("image/jpeg")
        };
    }

    if (url == "/img/botanical-bug.png") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::botanicalbug_png, BinaryData::botanicalbug_pngSize),
            juce::String("image/png")
        };
    }

    // Resource not found - log for debugging
    DBG("[Phase 3.1] Resource not found: " + url);
    return std::nullopt;
}
