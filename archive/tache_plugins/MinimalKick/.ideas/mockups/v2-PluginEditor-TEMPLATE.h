//==============================================================================
// MinimalKick - PluginEditor.h (TEMPLATE for Stage 5)
// Generated from UI mockup v2
//==============================================================================

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
 * MinimalKick Audio Plugin Editor
 *
 * WebView-based UI with 5 parameters:
 * - sweep (pitch envelope amount)
 * - time (pitch envelope decay)
 * - attack (amplitude attack)
 * - decay (amplitude decay)
 * - drive (saturation)
 */
class MinimalKickAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    MinimalKickAudioProcessorEditor (MinimalKickAudioProcessor&);
    ~MinimalKickAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // Reference to processor
    MinimalKickAudioProcessor& audioProcessor;

    // ========================================================================
    // CRITICAL: Member order matters for initialization
    // MUST be: relays → webView → attachments
    // Wrong order causes release build crashes
    // ========================================================================

    // WebView relays (JUCE 8 Pattern #15: Relay objects)
    juce::WebSliderRelay sweepRelay { "sweep" };
    juce::WebSliderRelay timeRelay { "time" };
    juce::WebSliderRelay attackRelay { "attack" };
    juce::WebSliderRelay decayRelay { "decay" };
    juce::WebSliderRelay driveRelay { "drive" };

    // WebView component
    juce::WebBrowserComponent webView;

    // Parameter attachments (JUCE 8 Pattern #16: SliderAttachment)
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sweepAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> timeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> decayAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> driveAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MinimalKickAudioProcessorEditor)
};
