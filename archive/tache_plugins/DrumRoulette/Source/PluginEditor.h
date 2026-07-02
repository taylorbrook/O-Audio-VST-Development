#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"

class DrumRouletteAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit DrumRouletteAudioProcessorEditor(DrumRouletteAudioProcessor&);
    ~DrumRouletteAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    DrumRouletteAudioProcessor& processorRef;

    // ========================================================================
    // RELAYS FIRST (no dependencies) - Pattern #11
    // ========================================================================

    // Slot 1 relays (8 parameters: 5 float + 3 bool)
    std::unique_ptr<juce::WebSliderRelay> relay_decay1;
    std::unique_ptr<juce::WebSliderRelay> relay_attack1;
    std::unique_ptr<juce::WebSliderRelay> relay_tiltFilter1;
    std::unique_ptr<juce::WebSliderRelay> relay_pitch1;
    std::unique_ptr<juce::WebSliderRelay> relay_volume1;
    std::unique_ptr<juce::WebToggleButtonRelay> relay_solo1;
    std::unique_ptr<juce::WebToggleButtonRelay> relay_mute1;
    std::unique_ptr<juce::WebToggleButtonRelay> relay_lock1;

    // Slot 2 relays (8 parameters: 5 float + 3 bool)
    std::unique_ptr<juce::WebSliderRelay> relay_decay2;
    std::unique_ptr<juce::WebSliderRelay> relay_attack2;
    std::unique_ptr<juce::WebSliderRelay> relay_tiltFilter2;
    std::unique_ptr<juce::WebSliderRelay> relay_pitch2;
    std::unique_ptr<juce::WebSliderRelay> relay_volume2;
    std::unique_ptr<juce::WebToggleButtonRelay> relay_solo2;
    std::unique_ptr<juce::WebToggleButtonRelay> relay_mute2;
    std::unique_ptr<juce::WebToggleButtonRelay> relay_lock2;

    // Slot 3 relays (8 parameters: 5 float + 3 bool)
    std::unique_ptr<juce::WebSliderRelay> relay_decay3;
    std::unique_ptr<juce::WebSliderRelay> relay_attack3;
    std::unique_ptr<juce::WebSliderRelay> relay_tiltFilter3;
    std::unique_ptr<juce::WebSliderRelay> relay_pitch3;
    std::unique_ptr<juce::WebSliderRelay> relay_volume3;
    std::unique_ptr<juce::WebToggleButtonRelay> relay_solo3;
    std::unique_ptr<juce::WebToggleButtonRelay> relay_mute3;
    std::unique_ptr<juce::WebToggleButtonRelay> relay_lock3;

    // Slot 4 relays (8 parameters: 5 float + 3 bool)
    std::unique_ptr<juce::WebSliderRelay> relay_decay4;
    std::unique_ptr<juce::WebSliderRelay> relay_attack4;
    std::unique_ptr<juce::WebSliderRelay> relay_tiltFilter4;
    std::unique_ptr<juce::WebSliderRelay> relay_pitch4;
    std::unique_ptr<juce::WebSliderRelay> relay_volume4;
    std::unique_ptr<juce::WebToggleButtonRelay> relay_solo4;
    std::unique_ptr<juce::WebToggleButtonRelay> relay_mute4;
    std::unique_ptr<juce::WebToggleButtonRelay> relay_lock4;

    // Slot 5 relays (8 parameters: 5 float + 3 bool)
    std::unique_ptr<juce::WebSliderRelay> relay_decay5;
    std::unique_ptr<juce::WebSliderRelay> relay_attack5;
    std::unique_ptr<juce::WebSliderRelay> relay_tiltFilter5;
    std::unique_ptr<juce::WebSliderRelay> relay_pitch5;
    std::unique_ptr<juce::WebSliderRelay> relay_volume5;
    std::unique_ptr<juce::WebToggleButtonRelay> relay_solo5;
    std::unique_ptr<juce::WebToggleButtonRelay> relay_mute5;
    std::unique_ptr<juce::WebToggleButtonRelay> relay_lock5;

    // Slot 6 relays (8 parameters: 5 float + 3 bool)
    std::unique_ptr<juce::WebSliderRelay> relay_decay6;
    std::unique_ptr<juce::WebSliderRelay> relay_attack6;
    std::unique_ptr<juce::WebSliderRelay> relay_tiltFilter6;
    std::unique_ptr<juce::WebSliderRelay> relay_pitch6;
    std::unique_ptr<juce::WebSliderRelay> relay_volume6;
    std::unique_ptr<juce::WebToggleButtonRelay> relay_solo6;
    std::unique_ptr<juce::WebToggleButtonRelay> relay_mute6;
    std::unique_ptr<juce::WebToggleButtonRelay> relay_lock6;

    // Slot 7 relays (8 parameters: 5 float + 3 bool)
    std::unique_ptr<juce::WebSliderRelay> relay_decay7;
    std::unique_ptr<juce::WebSliderRelay> relay_attack7;
    std::unique_ptr<juce::WebSliderRelay> relay_tiltFilter7;
    std::unique_ptr<juce::WebSliderRelay> relay_pitch7;
    std::unique_ptr<juce::WebSliderRelay> relay_volume7;
    std::unique_ptr<juce::WebToggleButtonRelay> relay_solo7;
    std::unique_ptr<juce::WebToggleButtonRelay> relay_mute7;
    std::unique_ptr<juce::WebToggleButtonRelay> relay_lock7;

    // Slot 8 relays (8 parameters: 5 float + 3 bool)
    std::unique_ptr<juce::WebSliderRelay> relay_decay8;
    std::unique_ptr<juce::WebSliderRelay> relay_attack8;
    std::unique_ptr<juce::WebSliderRelay> relay_tiltFilter8;
    std::unique_ptr<juce::WebSliderRelay> relay_pitch8;
    std::unique_ptr<juce::WebSliderRelay> relay_volume8;
    std::unique_ptr<juce::WebToggleButtonRelay> relay_solo8;
    std::unique_ptr<juce::WebToggleButtonRelay> relay_mute8;
    std::unique_ptr<juce::WebToggleButtonRelay> relay_lock8;

    // ========================================================================
    // WEBVIEW SECOND (depends on relays) - Pattern #11
    // ========================================================================
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // ========================================================================
    // ATTACHMENTS LAST (depend on both relays and webView) - Pattern #11
    // ========================================================================

    // Slot 1 attachments
    std::unique_ptr<juce::WebSliderParameterAttachment> attach_decay1;
    std::unique_ptr<juce::WebSliderParameterAttachment> attach_attack1;
    std::unique_ptr<juce::WebSliderParameterAttachment> attach_tiltFilter1;
    std::unique_ptr<juce::WebSliderParameterAttachment> attach_pitch1;
    std::unique_ptr<juce::WebSliderParameterAttachment> attach_volume1;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> attach_solo1;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> attach_mute1;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> attach_lock1;

    // Slot 2 attachments
    std::unique_ptr<juce::WebSliderParameterAttachment> attach_decay2;
    std::unique_ptr<juce::WebSliderParameterAttachment> attach_attack2;
    std::unique_ptr<juce::WebSliderParameterAttachment> attach_tiltFilter2;
    std::unique_ptr<juce::WebSliderParameterAttachment> attach_pitch2;
    std::unique_ptr<juce::WebSliderParameterAttachment> attach_volume2;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> attach_solo2;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> attach_mute2;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> attach_lock2;

    // Slot 3 attachments
    std::unique_ptr<juce::WebSliderParameterAttachment> attach_decay3;
    std::unique_ptr<juce::WebSliderParameterAttachment> attach_attack3;
    std::unique_ptr<juce::WebSliderParameterAttachment> attach_tiltFilter3;
    std::unique_ptr<juce::WebSliderParameterAttachment> attach_pitch3;
    std::unique_ptr<juce::WebSliderParameterAttachment> attach_volume3;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> attach_solo3;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> attach_mute3;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> attach_lock3;

    // Slot 4 attachments
    std::unique_ptr<juce::WebSliderParameterAttachment> attach_decay4;
    std::unique_ptr<juce::WebSliderParameterAttachment> attach_attack4;
    std::unique_ptr<juce::WebSliderParameterAttachment> attach_tiltFilter4;
    std::unique_ptr<juce::WebSliderParameterAttachment> attach_pitch4;
    std::unique_ptr<juce::WebSliderParameterAttachment> attach_volume4;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> attach_solo4;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> attach_mute4;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> attach_lock4;

    // Slot 5 attachments
    std::unique_ptr<juce::WebSliderParameterAttachment> attach_decay5;
    std::unique_ptr<juce::WebSliderParameterAttachment> attach_attack5;
    std::unique_ptr<juce::WebSliderParameterAttachment> attach_tiltFilter5;
    std::unique_ptr<juce::WebSliderParameterAttachment> attach_pitch5;
    std::unique_ptr<juce::WebSliderParameterAttachment> attach_volume5;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> attach_solo5;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> attach_mute5;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> attach_lock5;

    // Slot 6 attachments
    std::unique_ptr<juce::WebSliderParameterAttachment> attach_decay6;
    std::unique_ptr<juce::WebSliderParameterAttachment> attach_attack6;
    std::unique_ptr<juce::WebSliderParameterAttachment> attach_tiltFilter6;
    std::unique_ptr<juce::WebSliderParameterAttachment> attach_pitch6;
    std::unique_ptr<juce::WebSliderParameterAttachment> attach_volume6;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> attach_solo6;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> attach_mute6;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> attach_lock6;

    // Slot 7 attachments
    std::unique_ptr<juce::WebSliderParameterAttachment> attach_decay7;
    std::unique_ptr<juce::WebSliderParameterAttachment> attach_attack7;
    std::unique_ptr<juce::WebSliderParameterAttachment> attach_tiltFilter7;
    std::unique_ptr<juce::WebSliderParameterAttachment> attach_pitch7;
    std::unique_ptr<juce::WebSliderParameterAttachment> attach_volume7;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> attach_solo7;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> attach_mute7;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> attach_lock7;

    // Slot 8 attachments
    std::unique_ptr<juce::WebSliderParameterAttachment> attach_decay8;
    std::unique_ptr<juce::WebSliderParameterAttachment> attach_attack8;
    std::unique_ptr<juce::WebSliderParameterAttachment> attach_tiltFilter8;
    std::unique_ptr<juce::WebSliderParameterAttachment> attach_pitch8;
    std::unique_ptr<juce::WebSliderParameterAttachment> attach_volume8;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> attach_solo8;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> attach_mute8;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> attach_lock8;

    // Helper for resource serving (Pattern #8 - explicit URL mapping)
    std::optional<juce::WebBrowserComponent::Resource> getResource(const juce::String& url);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DrumRouletteAudioProcessorEditor)
};
