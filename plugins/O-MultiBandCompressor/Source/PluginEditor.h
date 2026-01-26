/*
  ==============================================================================

    O-MultiBandCompressor - Editor
    Ouaricon Audio
    Developer: Taylor Brook

    Phase 5.2: Parameter binding - all 56 parameters (8 global + 48 per-band)

  ==============================================================================
*/

#pragma once
#include "PluginProcessor.h"
#include <juce_gui_extra/juce_gui_extra.h>

class OMultiBandCompressorAudioProcessorEditor : public juce::AudioProcessorEditor,
                                                  private juce::Timer
{
public:
    explicit OMultiBandCompressorAudioProcessorEditor(OMultiBandCompressorAudioProcessor&);
    ~OMultiBandCompressorAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void parentHierarchyChanged() override;

private:
    // Timer callback for metering updates
    void timerCallback() override;

    // Send meter data to WebView
    void sendGainReductionMeters();
    void sendInputOutputMeters();
    void sendCrossoverPositions();
    void sendSpectrumData();  // v1.2.0

private:
    OMultiBandCompressorAudioProcessor& processorRef;

    // CRITICAL: Member declaration order (prevents crashes in release builds)
    // Pattern from troubleshooting/patterns/stage-3-patterns.md
    // Order: Relays → WebView → Attachments

    // ========== 1. RELAYS FIRST (no dependencies) ==========

    // Global parameter relays (8 parameters)
    juce::WebSliderRelay inputGainRelay { "INPUT_GAIN" };
    juce::WebSliderRelay outputGainRelay { "OUTPUT_GAIN" };
    juce::WebSliderRelay mixRelay { "MIX" };
    juce::WebToggleButtonRelay autoMakeupRelay { "AUTO_MAKEUP" };
    juce::WebComboBoxRelay msModeRelay { "MS_MODE" };
    juce::WebSliderRelay xover1Relay { "XOVER1" };
    juce::WebSliderRelay xover2Relay { "XOVER2" };
    juce::WebSliderRelay xover3Relay { "XOVER3" };

    // Low band relays (12 parameters)
    juce::WebSliderRelay lowThresholdRelay { "LOW_THRESHOLD" };
    juce::WebSliderRelay lowRatioRelay { "LOW_RATIO" };
    juce::WebSliderRelay lowAttackRelay { "LOW_ATTACK" };
    juce::WebSliderRelay lowReleaseRelay { "LOW_RELEASE" };
    juce::WebSliderRelay lowKneeRelay { "LOW_KNEE" };
    juce::WebSliderRelay lowMakeupRelay { "LOW_MAKEUP" };
    juce::WebSliderRelay lowPeakRmsRelay { "LOW_PEAK_RMS" };
    juce::WebToggleButtonRelay lowSoloRelay { "LOW_SOLO" };
    juce::WebToggleButtonRelay lowBypassRelay { "LOW_BYPASS" };
    juce::WebSliderRelay lowScHpfRelay { "LOW_SC_HPF" };
    juce::WebSliderRelay lowScLpfRelay { "LOW_SC_LPF" };
    juce::WebToggleButtonRelay lowScListenRelay { "LOW_SC_LISTEN" };

    // Low-Mid band relays (12 parameters)
    juce::WebSliderRelay lomidThresholdRelay { "LOMID_THRESHOLD" };
    juce::WebSliderRelay lomidRatioRelay { "LOMID_RATIO" };
    juce::WebSliderRelay lomidAttackRelay { "LOMID_ATTACK" };
    juce::WebSliderRelay lomidReleaseRelay { "LOMID_RELEASE" };
    juce::WebSliderRelay lomidKneeRelay { "LOMID_KNEE" };
    juce::WebSliderRelay lomidMakeupRelay { "LOMID_MAKEUP" };
    juce::WebSliderRelay lomidPeakRmsRelay { "LOMID_PEAK_RMS" };
    juce::WebToggleButtonRelay lomidSoloRelay { "LOMID_SOLO" };
    juce::WebToggleButtonRelay lomidBypassRelay { "LOMID_BYPASS" };
    juce::WebSliderRelay lomidScHpfRelay { "LOMID_SC_HPF" };
    juce::WebSliderRelay lomidScLpfRelay { "LOMID_SC_LPF" };
    juce::WebToggleButtonRelay lomidScListenRelay { "LOMID_SC_LISTEN" };

    // High-Mid band relays (12 parameters)
    juce::WebSliderRelay himidThresholdRelay { "HIMID_THRESHOLD" };
    juce::WebSliderRelay himidRatioRelay { "HIMID_RATIO" };
    juce::WebSliderRelay himidAttackRelay { "HIMID_ATTACK" };
    juce::WebSliderRelay himidReleaseRelay { "HIMID_RELEASE" };
    juce::WebSliderRelay himidKneeRelay { "HIMID_KNEE" };
    juce::WebSliderRelay himidMakeupRelay { "HIMID_MAKEUP" };
    juce::WebSliderRelay himidPeakRmsRelay { "HIMID_PEAK_RMS" };
    juce::WebToggleButtonRelay himidSoloRelay { "HIMID_SOLO" };
    juce::WebToggleButtonRelay himidBypassRelay { "HIMID_BYPASS" };
    juce::WebSliderRelay himidScHpfRelay { "HIMID_SC_HPF" };
    juce::WebSliderRelay himidScLpfRelay { "HIMID_SC_LPF" };
    juce::WebToggleButtonRelay himidScListenRelay { "HIMID_SC_LISTEN" };

    // High band relays (12 parameters)
    juce::WebSliderRelay highThresholdRelay { "HIGH_THRESHOLD" };
    juce::WebSliderRelay highRatioRelay { "HIGH_RATIO" };
    juce::WebSliderRelay highAttackRelay { "HIGH_ATTACK" };
    juce::WebSliderRelay highReleaseRelay { "HIGH_RELEASE" };
    juce::WebSliderRelay highKneeRelay { "HIGH_KNEE" };
    juce::WebSliderRelay highMakeupRelay { "HIGH_MAKEUP" };
    juce::WebSliderRelay highPeakRmsRelay { "HIGH_PEAK_RMS" };
    juce::WebToggleButtonRelay highSoloRelay { "HIGH_SOLO" };
    juce::WebToggleButtonRelay highBypassRelay { "HIGH_BYPASS" };
    juce::WebSliderRelay highScHpfRelay { "HIGH_SC_HPF" };
    juce::WebSliderRelay highScLpfRelay { "HIGH_SC_LPF" };
    juce::WebToggleButtonRelay highScListenRelay { "HIGH_SC_LISTEN" };

    // ========== 2. WEBVIEW SECOND (depends on relays) ==========
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // ========== 3. ATTACHMENTS LAST (depend on relays and webView) ==========

    // Global parameter attachments
    juce::WebSliderParameterAttachment inputGainAttachment;
    juce::WebSliderParameterAttachment outputGainAttachment;
    juce::WebSliderParameterAttachment mixAttachment;
    juce::WebToggleButtonParameterAttachment autoMakeupAttachment;
    juce::WebComboBoxParameterAttachment msModeAttachment;
    juce::WebSliderParameterAttachment xover1Attachment;
    juce::WebSliderParameterAttachment xover2Attachment;
    juce::WebSliderParameterAttachment xover3Attachment;

    // Low band attachments
    juce::WebSliderParameterAttachment lowThresholdAttachment;
    juce::WebSliderParameterAttachment lowRatioAttachment;
    juce::WebSliderParameterAttachment lowAttackAttachment;
    juce::WebSliderParameterAttachment lowReleaseAttachment;
    juce::WebSliderParameterAttachment lowKneeAttachment;
    juce::WebSliderParameterAttachment lowMakeupAttachment;
    juce::WebSliderParameterAttachment lowPeakRmsAttachment;
    juce::WebToggleButtonParameterAttachment lowSoloAttachment;
    juce::WebToggleButtonParameterAttachment lowBypassAttachment;
    juce::WebSliderParameterAttachment lowScHpfAttachment;
    juce::WebSliderParameterAttachment lowScLpfAttachment;
    juce::WebToggleButtonParameterAttachment lowScListenAttachment;

    // Low-Mid band attachments
    juce::WebSliderParameterAttachment lomidThresholdAttachment;
    juce::WebSliderParameterAttachment lomidRatioAttachment;
    juce::WebSliderParameterAttachment lomidAttackAttachment;
    juce::WebSliderParameterAttachment lomidReleaseAttachment;
    juce::WebSliderParameterAttachment lomidKneeAttachment;
    juce::WebSliderParameterAttachment lomidMakeupAttachment;
    juce::WebSliderParameterAttachment lomidPeakRmsAttachment;
    juce::WebToggleButtonParameterAttachment lomidSoloAttachment;
    juce::WebToggleButtonParameterAttachment lomidBypassAttachment;
    juce::WebSliderParameterAttachment lomidScHpfAttachment;
    juce::WebSliderParameterAttachment lomidScLpfAttachment;
    juce::WebToggleButtonParameterAttachment lomidScListenAttachment;

    // High-Mid band attachments
    juce::WebSliderParameterAttachment himidThresholdAttachment;
    juce::WebSliderParameterAttachment himidRatioAttachment;
    juce::WebSliderParameterAttachment himidAttackAttachment;
    juce::WebSliderParameterAttachment himidReleaseAttachment;
    juce::WebSliderParameterAttachment himidKneeAttachment;
    juce::WebSliderParameterAttachment himidMakeupAttachment;
    juce::WebSliderParameterAttachment himidPeakRmsAttachment;
    juce::WebToggleButtonParameterAttachment himidSoloAttachment;
    juce::WebToggleButtonParameterAttachment himidBypassAttachment;
    juce::WebSliderParameterAttachment himidScHpfAttachment;
    juce::WebSliderParameterAttachment himidScLpfAttachment;
    juce::WebToggleButtonParameterAttachment himidScListenAttachment;

    // High band attachments
    juce::WebSliderParameterAttachment highThresholdAttachment;
    juce::WebSliderParameterAttachment highRatioAttachment;
    juce::WebSliderParameterAttachment highAttackAttachment;
    juce::WebSliderParameterAttachment highReleaseAttachment;
    juce::WebSliderParameterAttachment highKneeAttachment;
    juce::WebSliderParameterAttachment highMakeupAttachment;
    juce::WebSliderParameterAttachment highPeakRmsAttachment;
    juce::WebToggleButtonParameterAttachment highSoloAttachment;
    juce::WebToggleButtonParameterAttachment highBypassAttachment;
    juce::WebSliderParameterAttachment highScHpfAttachment;
    juce::WebSliderParameterAttachment highScLpfAttachment;
    juce::WebToggleButtonParameterAttachment highScListenAttachment;

    // Navigation state
    bool hasNavigated = false;

    // Resource provider helper
    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OMultiBandCompressorAudioProcessorEditor)
};

