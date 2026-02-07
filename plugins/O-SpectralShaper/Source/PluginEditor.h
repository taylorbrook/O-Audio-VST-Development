/*
  ==============================================================================

    O-SpectralShaper - Editor
    Ouaricon Development
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include "PluginProcessor.h"
#include <juce_gui_extra/juce_gui_extra.h>

class OSpectralShaperAudioProcessorEditor : public juce::AudioProcessorEditor,
                                             private juce::Timer
{
public:
    explicit OSpectralShaperAudioProcessorEditor(OSpectralShaperAudioProcessor&);
    ~OSpectralShaperAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void parentHierarchyChanged() override;

    // Timer callback for 60fps visualization updates
    void timerCallback() override;

    void handleAttackCurveUpdate(const juce::Array<juce::var>& args);
    void handleSustainCurveUpdate(const juce::Array<juce::var>& args);
    void sendAttackCurveToJS();
    void sendSustainCurveToJS();

private:
    OSpectralShaperAudioProcessor& processorRef;

    // CRITICAL MEMBER ORDER: relays → webView → attachments
    // Members destroyed in REVERSE order - wrong order causes release build crashes

    // 1. RELAYS FIRST (no dependencies)
    std::unique_ptr<juce::WebSliderRelay> mixRelay;
    std::unique_ptr<juce::WebSliderRelay> attackTimeRelay;
    std::unique_ptr<juce::WebSliderRelay> sustainTimeRelay;
    std::unique_ptr<juce::WebSliderRelay> sensitivityRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> lookaheadEnabledRelay;
    std::unique_ptr<juce::WebSliderRelay> lookaheadTimeRelay;
    std::unique_ptr<juce::WebSliderRelay> outputGainRelay;

    // 2. WEBVIEW SECOND (depends on relays via withOptionsFrom)
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3. NAVIGATION FLAG
    bool hasNavigated = false;

    // 4. ATTACHMENTS LAST (depend on both relays and webView)
    std::unique_ptr<juce::WebSliderParameterAttachment> mixAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> attackTimeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> sustainTimeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> sensitivityAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> lookaheadEnabledAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> lookaheadTimeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> outputGainAttachment;

    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);
    void sendCurveToJS(const char* functionName, const std::array<float, 32>& curve);
    void emitVisualizationFrame(const OSpectralShaperAudioProcessor::VisualizationFrame& frame);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OSpectralShaperAudioProcessorEditor)
};
