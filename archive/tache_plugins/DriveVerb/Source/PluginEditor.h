#pragma once
#include "PluginProcessor.h"
#include <juce_gui_extra/juce_gui_extra.h>

class DriveVerbAudioProcessorEditor : public juce::AudioProcessorEditor,
                                       private juce::Timer
{
public:
    explicit DriveVerbAudioProcessorEditor(DriveVerbAudioProcessor&);
    ~DriveVerbAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    void timerCallback() override;  // For VU meter updates

private:
    DriveVerbAudioProcessor& processorRef;

    // ⚠️ MEMBER DECLARATION ORDER IS CRITICAL (Pattern #11)
    // Members destroyed in REVERSE order of declaration
    // Declare dependencies AFTER what they depend on

    // 1️⃣ RELAYS FIRST (no dependencies)
    std::unique_ptr<juce::WebSliderRelay> sizeRelay;
    std::unique_ptr<juce::WebSliderRelay> decayRelay;
    std::unique_ptr<juce::WebSliderRelay> dryWetRelay;
    std::unique_ptr<juce::WebSliderRelay> driveRelay;
    std::unique_ptr<juce::WebSliderRelay> filterRelay;
    std::unique_ptr<juce::WebToggleButtonRelay> filterPositionRelay;

    // 2️⃣ WEBVIEW SECOND (depends on relays via withOptionsFrom)
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3️⃣ ATTACHMENTS LAST (depend on both relays and webView)
    std::unique_ptr<juce::WebSliderParameterAttachment> sizeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> decayAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> dryWetAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> driveAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> filterAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> filterPositionAttachment;

    // Helper for resource serving (Pattern #8)
    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DriveVerbAudioProcessorEditor)
};
