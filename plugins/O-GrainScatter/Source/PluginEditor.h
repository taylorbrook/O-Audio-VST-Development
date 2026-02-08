#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class GrainScatterEditor : public juce::AudioProcessorEditor,
                            private juce::Timer
{
public:
    explicit GrainScatterEditor(GrainScatterProcessor&);
    ~GrainScatterEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);

    GrainScatterProcessor& audioProcessor;

    // ========================================================================
    // CRITICAL MEMBER DECLARATION ORDER: Relays -> WebView -> Attachments
    // Members destroyed in REVERSE declaration order (C++ standard)
    // ========================================================================

    // 1. RELAYS (no dependencies)
    // Core
    std::unique_ptr<juce::WebSliderRelay> grainSizeRelay;
    std::unique_ptr<juce::WebSliderRelay> densityRelay;
    std::unique_ptr<juce::WebSliderRelay> pitchRandomRelay;
    std::unique_ptr<juce::WebSliderRelay> panRandomRelay;
    std::unique_ptr<juce::WebComboBoxRelay> scaleRelay;
    std::unique_ptr<juce::WebComboBoxRelay> rootNoteRelay;
    std::unique_ptr<juce::WebSliderRelay> reverseRelay;
    std::unique_ptr<juce::WebSliderRelay> feedbackRelay;
    std::unique_ptr<juce::WebSliderRelay> dryWetRelay;
    // Sync
    std::unique_ptr<juce::WebComboBoxRelay> syncModeRelay;
    std::unique_ptr<juce::WebSliderRelay> probabilityRelay;
    std::unique_ptr<juce::WebSliderRelay> repeatsRelay;
    // Spread & Pitch
    std::unique_ptr<juce::WebSliderRelay> spreadRelay;
    std::unique_ptr<juce::WebComboBoxRelay> pitchModeRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> freezeRelay;
    // Stutter Gate
    std::unique_ptr<juce::WebToggleButtonRelay> stutterGateRelay;
    // Euclidean
    std::unique_ptr<juce::WebSliderRelay> euclideanPulsesRelay;
    std::unique_ptr<juce::WebSliderRelay> euclideanStepsRelay;
    // Spatial
    std::unique_ptr<juce::WebComboBoxRelay> spatialModeRelay;
    std::unique_ptr<juce::WebSliderRelay> azimuthRelay;
    std::unique_ptr<juce::WebSliderRelay> elevationRelay;
    std::unique_ptr<juce::WebSliderRelay> azSpreadRelay;
    std::unique_ptr<juce::WebSliderRelay> elSpreadRelay;
    std::unique_ptr<juce::WebSliderRelay> distanceRelay;
    std::unique_ptr<juce::WebSliderRelay> spatialWidthRelay;
    std::unique_ptr<juce::WebComboBoxRelay> trajectoryRelay;

    // 2. WEBVIEW (depends on relays)
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3. ATTACHMENTS (depend on relays + webView)
    // Core
    std::unique_ptr<juce::WebSliderParameterAttachment> grainSizeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> densityAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> pitchRandomAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> panRandomAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> scaleAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> rootNoteAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> reverseAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> feedbackAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> dryWetAttachment;
    // Sync
    std::unique_ptr<juce::WebComboBoxParameterAttachment> syncModeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> probabilityAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> repeatsAttachment;
    // Spread & Pitch
    std::unique_ptr<juce::WebSliderParameterAttachment> spreadAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> pitchModeAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> freezeAttachment;
    // Stutter Gate
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> stutterGateAttachment;
    // Euclidean
    std::unique_ptr<juce::WebSliderParameterAttachment> euclideanPulsesAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> euclideanStepsAttachment;
    // Spatial
    std::unique_ptr<juce::WebComboBoxParameterAttachment> spatialModeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> azimuthAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> elevationAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> azSpreadAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> elSpreadAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> distanceAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> spatialWidthAttachment;
    std::unique_ptr<juce::WebComboBoxParameterAttachment> trajectoryAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GrainScatterEditor)
};
