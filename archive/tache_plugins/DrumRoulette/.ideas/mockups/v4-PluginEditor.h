#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
 * DrumRoulette WebView-based Editor
 * Generated from UI mockup v4
 * Window size: 1400 x 950
 */
class DrumRouletteAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    DrumRouletteAudioProcessorEditor (DrumRouletteAudioProcessor&);
    ~DrumRouletteAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    DrumRouletteAudioProcessor& audioProcessor;

    //==============================================================================
    // WebView Parameter Relays
    // CRITICAL: Member order matters! relays → webView → attachments (Pattern #14)
    //==============================================================================

    // Slot 1 parameters
    juce::SliderParameterAttachment::Relay relay_decay1;
    juce::SliderParameterAttachment::Relay relay_attack1;
    juce::SliderParameterAttachment::Relay relay_tilt1;
    juce::SliderParameterAttachment::Relay relay_pitch1;
    juce::SliderParameterAttachment::Relay relay_volume1;
    juce::ButtonParameterAttachment::Relay relay_solo1;
    juce::ButtonParameterAttachment::Relay relay_mute1;

    // Slot 2 parameters
    juce::SliderParameterAttachment::Relay relay_decay2;
    juce::SliderParameterAttachment::Relay relay_attack2;
    juce::SliderParameterAttachment::Relay relay_tilt2;
    juce::SliderParameterAttachment::Relay relay_pitch2;
    juce::SliderParameterAttachment::Relay relay_volume2;
    juce::ButtonParameterAttachment::Relay relay_solo2;
    juce::ButtonParameterAttachment::Relay relay_mute2;

    // Slot 3 parameters
    juce::SliderParameterAttachment::Relay relay_decay3;
    juce::SliderParameterAttachment::Relay relay_attack3;
    juce::SliderParameterAttachment::Relay relay_tilt3;
    juce::SliderParameterAttachment::Relay relay_pitch3;
    juce::SliderParameterAttachment::Relay relay_volume3;
    juce::ButtonParameterAttachment::Relay relay_solo3;
    juce::ButtonParameterAttachment::Relay relay_mute3;

    // Slot 4 parameters
    juce::SliderParameterAttachment::Relay relay_decay4;
    juce::SliderParameterAttachment::Relay relay_attack4;
    juce::SliderParameterAttachment::Relay relay_tilt4;
    juce::SliderParameterAttachment::Relay relay_pitch4;
    juce::SliderParameterAttachment::Relay relay_volume4;
    juce::ButtonParameterAttachment::Relay relay_solo4;
    juce::ButtonParameterAttachment::Relay relay_mute4;

    // Slot 5 parameters
    juce::SliderParameterAttachment::Relay relay_decay5;
    juce::SliderParameterAttachment::Relay relay_attack5;
    juce::SliderParameterAttachment::Relay relay_tilt5;
    juce::SliderParameterAttachment::Relay relay_pitch5;
    juce::SliderParameterAttachment::Relay relay_volume5;
    juce::ButtonParameterAttachment::Relay relay_solo5;
    juce::ButtonParameterAttachment::Relay relay_mute5;

    // Slot 6 parameters
    juce::SliderParameterAttachment::Relay relay_decay6;
    juce::SliderParameterAttachment::Relay relay_attack6;
    juce::SliderParameterAttachment::Relay relay_tilt6;
    juce::SliderParameterAttachment::Relay relay_pitch6;
    juce::SliderParameterAttachment::Relay relay_volume6;
    juce::ButtonParameterAttachment::Relay relay_solo6;
    juce::ButtonParameterAttachment::Relay relay_mute6;

    // Slot 7 parameters
    juce::SliderParameterAttachment::Relay relay_decay7;
    juce::SliderParameterAttachment::Relay relay_attack7;
    juce::SliderParameterAttachment::Relay relay_tilt7;
    juce::SliderParameterAttachment::Relay relay_pitch7;
    juce::SliderParameterAttachment::Relay relay_volume7;
    juce::ButtonParameterAttachment::Relay relay_solo7;
    juce::ButtonParameterAttachment::Relay relay_mute7;

    // Slot 8 parameters
    juce::SliderParameterAttachment::Relay relay_decay8;
    juce::SliderParameterAttachment::Relay relay_attack8;
    juce::SliderParameterAttachment::Relay relay_tilt8;
    juce::SliderParameterAttachment::Relay relay_pitch8;
    juce::SliderParameterAttachment::Relay relay_volume8;
    juce::ButtonParameterAttachment::Relay relay_solo8;
    juce::ButtonParameterAttachment::Relay relay_mute8;

    // Master parameters
    juce::SliderParameterAttachment::Relay relay_masterVolume;

    //==============================================================================
    // WebView Component
    //==============================================================================
    juce::WebBrowserComponent webView;

    //==============================================================================
    // Parameter Attachments
    // CRITICAL: Must be declared AFTER webView (Pattern #14)
    //==============================================================================

    // Slot 1 attachments
    juce::SliderParameterAttachment attach_decay1;
    juce::SliderParameterAttachment attach_attack1;
    juce::SliderParameterAttachment attach_tilt1;
    juce::SliderParameterAttachment attach_pitch1;
    juce::SliderParameterAttachment attach_volume1;
    juce::ButtonParameterAttachment attach_solo1;
    juce::ButtonParameterAttachment attach_mute1;

    // Slot 2 attachments
    juce::SliderParameterAttachment attach_decay2;
    juce::SliderParameterAttachment attach_attack2;
    juce::SliderParameterAttachment attach_tilt2;
    juce::SliderParameterAttachment attach_pitch2;
    juce::SliderParameterAttachment attach_volume2;
    juce::ButtonParameterAttachment attach_solo2;
    juce::ButtonParameterAttachment attach_mute2;

    // Slot 3 attachments
    juce::SliderParameterAttachment attach_decay3;
    juce::SliderParameterAttachment attach_attack3;
    juce::SliderParameterAttachment attach_tilt3;
    juce::SliderParameterAttachment attach_pitch3;
    juce::SliderParameterAttachment attach_volume3;
    juce::ButtonParameterAttachment attach_solo3;
    juce::ButtonParameterAttachment attach_mute3;

    // Slot 4 attachments
    juce::SliderParameterAttachment attach_decay4;
    juce::SliderParameterAttachment attach_attack4;
    juce::SliderParameterAttachment attach_tilt4;
    juce::SliderParameterAttachment attach_pitch4;
    juce::SliderParameterAttachment attach_volume4;
    juce::ButtonParameterAttachment attach_solo4;
    juce::ButtonParameterAttachment attach_mute4;

    // Slot 5 attachments
    juce::SliderParameterAttachment attach_decay5;
    juce::SliderParameterAttachment attach_attack5;
    juce::SliderParameterAttachment attach_tilt5;
    juce::SliderParameterAttachment attach_pitch5;
    juce::SliderParameterAttachment attach_volume5;
    juce::ButtonParameterAttachment attach_solo5;
    juce::ButtonParameterAttachment attach_mute5;

    // Slot 6 attachments
    juce::SliderParameterAttachment attach_decay6;
    juce::SliderParameterAttachment attach_attack6;
    juce::SliderParameterAttachment attach_tilt6;
    juce::SliderParameterAttachment attach_pitch6;
    juce::SliderParameterAttachment attach_volume6;
    juce::ButtonParameterAttachment attach_solo6;
    juce::ButtonParameterAttachment attach_mute6;

    // Slot 7 attachments
    juce::SliderParameterAttachment attach_decay7;
    juce::SliderParameterAttachment attach_attack7;
    juce::SliderParameterAttachment attach_tilt7;
    juce::SliderParameterAttachment attach_pitch7;
    juce::SliderParameterAttachment attach_volume7;
    juce::ButtonParameterAttachment attach_solo7;
    juce::ButtonParameterAttachment attach_mute7;

    // Slot 8 attachments
    juce::SliderParameterAttachment attach_decay8;
    juce::SliderParameterAttachment attach_attack8;
    juce::SliderParameterAttachment attach_tilt8;
    juce::SliderParameterAttachment attach_pitch8;
    juce::SliderParameterAttachment attach_volume8;
    juce::ButtonParameterAttachment attach_solo8;
    juce::ButtonParameterAttachment attach_mute8;

    // Master attachments
    juce::SliderParameterAttachment attach_masterVolume;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DrumRouletteAudioProcessorEditor)
};
