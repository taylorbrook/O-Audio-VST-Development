/*
  ==============================================================================

    O-Polystutter - Editor Implementation
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "PluginEditor.h"
#include "BinaryData.h"

OPolystutterAudioProcessorEditor::OPolystutterAudioProcessorEditor(OPolystutterAudioProcessor& p)
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

    , lane1SubdivisionRelay(std::make_unique<juce::WebComboBoxRelay>("lane1_subdivision"))
    // v1.7.0: Lane 1 pitch randomization relays
    , lane1PitchRandEnabledRelay(std::make_unique<juce::WebToggleButtonRelay>("lane1_pitch_rand_enabled"))
    , lane1PitchRandMinRelay(std::make_unique<juce::WebSliderRelay>("lane1_pitch_rand_min"))
    , lane1PitchRandMaxRelay(std::make_unique<juce::WebSliderRelay>("lane1_pitch_rand_max"))
    , lane1PitchRandQuantizeRelay(std::make_unique<juce::WebToggleButtonRelay>("lane1_pitch_rand_quantize"))
    // v1.9.0: Lane 1 Euclidean relays
    , lane1EuclideanEnabledRelay(std::make_unique<juce::WebToggleButtonRelay>("lane1_euclidean_enabled"))
    , lane1EuclideanPulsesRelay(std::make_unique<juce::WebSliderRelay>("lane1_euclidean_pulses"))
    , lane1EuclideanStepsRelay(std::make_unique<juce::WebSliderRelay>("lane1_euclidean_steps"))

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

    , lane2SubdivisionRelay(std::make_unique<juce::WebComboBoxRelay>("lane2_subdivision"))
    // v1.7.0: Lane 2 pitch randomization relays
    , lane2PitchRandEnabledRelay(std::make_unique<juce::WebToggleButtonRelay>("lane2_pitch_rand_enabled"))
    , lane2PitchRandMinRelay(std::make_unique<juce::WebSliderRelay>("lane2_pitch_rand_min"))
    , lane2PitchRandMaxRelay(std::make_unique<juce::WebSliderRelay>("lane2_pitch_rand_max"))
    , lane2PitchRandQuantizeRelay(std::make_unique<juce::WebToggleButtonRelay>("lane2_pitch_rand_quantize"))
    // v1.9.0: Lane 2 Euclidean relays
    , lane2EuclideanEnabledRelay(std::make_unique<juce::WebToggleButtonRelay>("lane2_euclidean_enabled"))
    , lane2EuclideanPulsesRelay(std::make_unique<juce::WebSliderRelay>("lane2_euclidean_pulses"))
    , lane2EuclideanStepsRelay(std::make_unique<juce::WebSliderRelay>("lane2_euclidean_steps"))

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

    , lane3SubdivisionRelay(std::make_unique<juce::WebComboBoxRelay>("lane3_subdivision"))
    // v1.7.0: Lane 3 pitch randomization relays
    , lane3PitchRandEnabledRelay(std::make_unique<juce::WebToggleButtonRelay>("lane3_pitch_rand_enabled"))
    , lane3PitchRandMinRelay(std::make_unique<juce::WebSliderRelay>("lane3_pitch_rand_min"))
    , lane3PitchRandMaxRelay(std::make_unique<juce::WebSliderRelay>("lane3_pitch_rand_max"))
    , lane3PitchRandQuantizeRelay(std::make_unique<juce::WebToggleButtonRelay>("lane3_pitch_rand_quantize"))
    // v1.9.0: Lane 3 Euclidean relays
    , lane3EuclideanEnabledRelay(std::make_unique<juce::WebToggleButtonRelay>("lane3_euclidean_enabled"))
    , lane3EuclideanPulsesRelay(std::make_unique<juce::WebSliderRelay>("lane3_euclidean_pulses"))
    , lane3EuclideanStepsRelay(std::make_unique<juce::WebSliderRelay>("lane3_euclidean_steps"))

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

    , lane4SubdivisionRelay(std::make_unique<juce::WebComboBoxRelay>("lane4_subdivision"))
    // v1.7.0: Lane 4 pitch randomization relays
    , lane4PitchRandEnabledRelay(std::make_unique<juce::WebToggleButtonRelay>("lane4_pitch_rand_enabled"))
    , lane4PitchRandMinRelay(std::make_unique<juce::WebSliderRelay>("lane4_pitch_rand_min"))
    , lane4PitchRandMaxRelay(std::make_unique<juce::WebSliderRelay>("lane4_pitch_rand_max"))
    , lane4PitchRandQuantizeRelay(std::make_unique<juce::WebToggleButtonRelay>("lane4_pitch_rand_quantize"))
    // v1.9.0: Lane 4 Euclidean relays
    , lane4EuclideanEnabledRelay(std::make_unique<juce::WebToggleButtonRelay>("lane4_euclidean_enabled"))
    , lane4EuclideanPulsesRelay(std::make_unique<juce::WebSliderRelay>("lane4_euclidean_pulses"))
    , lane4EuclideanStepsRelay(std::make_unique<juce::WebSliderRelay>("lane4_euclidean_steps"))

    // Tape degradation relays
    , tapeSaturationRelay(std::make_unique<juce::WebSliderRelay>("tape_saturation"))
    , tapeWowRelay(std::make_unique<juce::WebSliderRelay>("tape_wow"))
    , tapeFlutterRelay(std::make_unique<juce::WebSliderRelay>("tape_flutter"))
    , tapeHissRelay(std::make_unique<juce::WebSliderRelay>("tape_hiss"))
    , tapeRolloffRelay(std::make_unique<juce::WebSliderRelay>("tape_rolloff"))
    , tapeDropoutRelay(std::make_unique<juce::WebSliderRelay>("tape_dropout"))
    , tapeBypassRelay(std::make_unique<juce::WebToggleButtonRelay>("tape_bypass"))  // v1.6.5

    // v1.1.0: Wet/dry mix relays
    , mixDryRelay(std::make_unique<juce::WebSliderRelay>("mix_dry"))
    , mixWetRelay(std::make_unique<juce::WebSliderRelay>("mix_wet"))

    // Global control relays - v1.3.1: Removed ENV and SC
    , midiEnabledRelay(std::make_unique<juce::WebToggleButtonRelay>("midi_enabled"))
    , manualTriggerRelay(std::make_unique<juce::WebToggleButtonRelay>("manual_trigger"))
    , sequencerEnabledRelay(std::make_unique<juce::WebToggleButtonRelay>("sequencer_enabled"))  // v1.0.2

    // Phase 3.3: Pattern step relays (64 toggles: 4 lanes × 16 steps)
    , patternLane1Step1Relay(std::make_unique<juce::WebToggleButtonRelay>("pattern_lane1_step1"))
    , patternLane1Step2Relay(std::make_unique<juce::WebToggleButtonRelay>("pattern_lane1_step2"))
    , patternLane1Step3Relay(std::make_unique<juce::WebToggleButtonRelay>("pattern_lane1_step3"))
    , patternLane1Step4Relay(std::make_unique<juce::WebToggleButtonRelay>("pattern_lane1_step4"))
    , patternLane1Step5Relay(std::make_unique<juce::WebToggleButtonRelay>("pattern_lane1_step5"))
    , patternLane1Step6Relay(std::make_unique<juce::WebToggleButtonRelay>("pattern_lane1_step6"))
    , patternLane1Step7Relay(std::make_unique<juce::WebToggleButtonRelay>("pattern_lane1_step7"))
    , patternLane1Step8Relay(std::make_unique<juce::WebToggleButtonRelay>("pattern_lane1_step8"))
    , patternLane1Step9Relay(std::make_unique<juce::WebToggleButtonRelay>("pattern_lane1_step9"))
    , patternLane1Step10Relay(std::make_unique<juce::WebToggleButtonRelay>("pattern_lane1_step10"))
    , patternLane1Step11Relay(std::make_unique<juce::WebToggleButtonRelay>("pattern_lane1_step11"))
    , patternLane1Step12Relay(std::make_unique<juce::WebToggleButtonRelay>("pattern_lane1_step12"))
    , patternLane1Step13Relay(std::make_unique<juce::WebToggleButtonRelay>("pattern_lane1_step13"))
    , patternLane1Step14Relay(std::make_unique<juce::WebToggleButtonRelay>("pattern_lane1_step14"))
    , patternLane1Step15Relay(std::make_unique<juce::WebToggleButtonRelay>("pattern_lane1_step15"))
    , patternLane1Step16Relay(std::make_unique<juce::WebToggleButtonRelay>("pattern_lane1_step16"))
    , patternLane2Step1Relay(std::make_unique<juce::WebToggleButtonRelay>("pattern_lane2_step1"))
    , patternLane2Step2Relay(std::make_unique<juce::WebToggleButtonRelay>("pattern_lane2_step2"))
    , patternLane2Step3Relay(std::make_unique<juce::WebToggleButtonRelay>("pattern_lane2_step3"))
    , patternLane2Step4Relay(std::make_unique<juce::WebToggleButtonRelay>("pattern_lane2_step4"))
    , patternLane2Step5Relay(std::make_unique<juce::WebToggleButtonRelay>("pattern_lane2_step5"))
    , patternLane2Step6Relay(std::make_unique<juce::WebToggleButtonRelay>("pattern_lane2_step6"))
    , patternLane2Step7Relay(std::make_unique<juce::WebToggleButtonRelay>("pattern_lane2_step7"))
    , patternLane2Step8Relay(std::make_unique<juce::WebToggleButtonRelay>("pattern_lane2_step8"))
    , patternLane2Step9Relay(std::make_unique<juce::WebToggleButtonRelay>("pattern_lane2_step9"))
    , patternLane2Step10Relay(std::make_unique<juce::WebToggleButtonRelay>("pattern_lane2_step10"))
    , patternLane2Step11Relay(std::make_unique<juce::WebToggleButtonRelay>("pattern_lane2_step11"))
    , patternLane2Step12Relay(std::make_unique<juce::WebToggleButtonRelay>("pattern_lane2_step12"))
    , patternLane2Step13Relay(std::make_unique<juce::WebToggleButtonRelay>("pattern_lane2_step13"))
    , patternLane2Step14Relay(std::make_unique<juce::WebToggleButtonRelay>("pattern_lane2_step14"))
    , patternLane2Step15Relay(std::make_unique<juce::WebToggleButtonRelay>("pattern_lane2_step15"))
    , patternLane2Step16Relay(std::make_unique<juce::WebToggleButtonRelay>("pattern_lane2_step16"))
    , patternLane3Step1Relay(std::make_unique<juce::WebToggleButtonRelay>("pattern_lane3_step1"))
    , patternLane3Step2Relay(std::make_unique<juce::WebToggleButtonRelay>("pattern_lane3_step2"))
    , patternLane3Step3Relay(std::make_unique<juce::WebToggleButtonRelay>("pattern_lane3_step3"))
    , patternLane3Step4Relay(std::make_unique<juce::WebToggleButtonRelay>("pattern_lane3_step4"))
    , patternLane3Step5Relay(std::make_unique<juce::WebToggleButtonRelay>("pattern_lane3_step5"))
    , patternLane3Step6Relay(std::make_unique<juce::WebToggleButtonRelay>("pattern_lane3_step6"))
    , patternLane3Step7Relay(std::make_unique<juce::WebToggleButtonRelay>("pattern_lane3_step7"))
    , patternLane3Step8Relay(std::make_unique<juce::WebToggleButtonRelay>("pattern_lane3_step8"))
    , patternLane3Step9Relay(std::make_unique<juce::WebToggleButtonRelay>("pattern_lane3_step9"))
    , patternLane3Step10Relay(std::make_unique<juce::WebToggleButtonRelay>("pattern_lane3_step10"))
    , patternLane3Step11Relay(std::make_unique<juce::WebToggleButtonRelay>("pattern_lane3_step11"))
    , patternLane3Step12Relay(std::make_unique<juce::WebToggleButtonRelay>("pattern_lane3_step12"))
    , patternLane3Step13Relay(std::make_unique<juce::WebToggleButtonRelay>("pattern_lane3_step13"))
    , patternLane3Step14Relay(std::make_unique<juce::WebToggleButtonRelay>("pattern_lane3_step14"))
    , patternLane3Step15Relay(std::make_unique<juce::WebToggleButtonRelay>("pattern_lane3_step15"))
    , patternLane3Step16Relay(std::make_unique<juce::WebToggleButtonRelay>("pattern_lane3_step16"))
    , patternLane4Step1Relay(std::make_unique<juce::WebToggleButtonRelay>("pattern_lane4_step1"))
    , patternLane4Step2Relay(std::make_unique<juce::WebToggleButtonRelay>("pattern_lane4_step2"))
    , patternLane4Step3Relay(std::make_unique<juce::WebToggleButtonRelay>("pattern_lane4_step3"))
    , patternLane4Step4Relay(std::make_unique<juce::WebToggleButtonRelay>("pattern_lane4_step4"))
    , patternLane4Step5Relay(std::make_unique<juce::WebToggleButtonRelay>("pattern_lane4_step5"))
    , patternLane4Step6Relay(std::make_unique<juce::WebToggleButtonRelay>("pattern_lane4_step6"))
    , patternLane4Step7Relay(std::make_unique<juce::WebToggleButtonRelay>("pattern_lane4_step7"))
    , patternLane4Step8Relay(std::make_unique<juce::WebToggleButtonRelay>("pattern_lane4_step8"))
    , patternLane4Step9Relay(std::make_unique<juce::WebToggleButtonRelay>("pattern_lane4_step9"))
    , patternLane4Step10Relay(std::make_unique<juce::WebToggleButtonRelay>("pattern_lane4_step10"))
    , patternLane4Step11Relay(std::make_unique<juce::WebToggleButtonRelay>("pattern_lane4_step11"))
    , patternLane4Step12Relay(std::make_unique<juce::WebToggleButtonRelay>("pattern_lane4_step12"))
    , patternLane4Step13Relay(std::make_unique<juce::WebToggleButtonRelay>("pattern_lane4_step13"))
    , patternLane4Step14Relay(std::make_unique<juce::WebToggleButtonRelay>("pattern_lane4_step14"))
    , patternLane4Step15Relay(std::make_unique<juce::WebToggleButtonRelay>("pattern_lane4_step15"))
    , patternLane4Step16Relay(std::make_unique<juce::WebToggleButtonRelay>("pattern_lane4_step16"))

    // Initialize WebView SECOND (depends on relays via withOptionsFrom)
    , webView(std::make_unique<juce::WebBrowserComponent>(
        juce::WebBrowserComponent::Options{}
            .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
            .withWinWebView2Options(
                juce::WebBrowserComponent::Options::WinWebView2{}
                    .withUserDataFolder(juce::File::getSpecialLocation(
                        juce::File::SpecialLocationType::tempDirectory)))
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

            .withOptionsFrom(*lane1SubdivisionRelay)
            // v1.7.0: Lane 1 pitch randomization
            .withOptionsFrom(*lane1PitchRandEnabledRelay)
            .withOptionsFrom(*lane1PitchRandMinRelay)
            .withOptionsFrom(*lane1PitchRandMaxRelay)
            .withOptionsFrom(*lane1PitchRandQuantizeRelay)
            // v1.9.0: Lane 1 Euclidean
            .withOptionsFrom(*lane1EuclideanEnabledRelay)
            .withOptionsFrom(*lane1EuclideanPulsesRelay)
            .withOptionsFrom(*lane1EuclideanStepsRelay)
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

            .withOptionsFrom(*lane2SubdivisionRelay)
            // v1.7.0: Lane 2 pitch randomization
            .withOptionsFrom(*lane2PitchRandEnabledRelay)
            .withOptionsFrom(*lane2PitchRandMinRelay)
            .withOptionsFrom(*lane2PitchRandMaxRelay)
            .withOptionsFrom(*lane2PitchRandQuantizeRelay)
            // v1.9.0: Lane 2 Euclidean
            .withOptionsFrom(*lane2EuclideanEnabledRelay)
            .withOptionsFrom(*lane2EuclideanPulsesRelay)
            .withOptionsFrom(*lane2EuclideanStepsRelay)
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

            .withOptionsFrom(*lane3SubdivisionRelay)
            // v1.7.0: Lane 3 pitch randomization
            .withOptionsFrom(*lane3PitchRandEnabledRelay)
            .withOptionsFrom(*lane3PitchRandMinRelay)
            .withOptionsFrom(*lane3PitchRandMaxRelay)
            .withOptionsFrom(*lane3PitchRandQuantizeRelay)
            // v1.9.0: Lane 3 Euclidean
            .withOptionsFrom(*lane3EuclideanEnabledRelay)
            .withOptionsFrom(*lane3EuclideanPulsesRelay)
            .withOptionsFrom(*lane3EuclideanStepsRelay)
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

            .withOptionsFrom(*lane4SubdivisionRelay)
            // v1.7.0: Lane 4 pitch randomization
            .withOptionsFrom(*lane4PitchRandEnabledRelay)
            .withOptionsFrom(*lane4PitchRandMinRelay)
            .withOptionsFrom(*lane4PitchRandMaxRelay)
            .withOptionsFrom(*lane4PitchRandQuantizeRelay)
            // v1.9.0: Lane 4 Euclidean
            .withOptionsFrom(*lane4EuclideanEnabledRelay)
            .withOptionsFrom(*lane4EuclideanPulsesRelay)
            .withOptionsFrom(*lane4EuclideanStepsRelay)
            // Tape degradation
            .withOptionsFrom(*tapeSaturationRelay)
            .withOptionsFrom(*tapeWowRelay)
            .withOptionsFrom(*tapeFlutterRelay)
            .withOptionsFrom(*tapeHissRelay)
            .withOptionsFrom(*tapeRolloffRelay)
            .withOptionsFrom(*tapeDropoutRelay)
            .withOptionsFrom(*tapeBypassRelay)  // v1.6.5
            // v1.1.0: Wet/dry mix
            .withOptionsFrom(*mixDryRelay)
            .withOptionsFrom(*mixWetRelay)
            // Global controls - v1.3.1: Removed ENV and SC
            .withOptionsFrom(*midiEnabledRelay)
            .withOptionsFrom(*manualTriggerRelay)
            .withOptionsFrom(*sequencerEnabledRelay)  // v1.0.2
            // Phase 3.3: Pattern step relay registrations (64 toggles)
            .withOptionsFrom(*patternLane1Step1Relay)
            .withOptionsFrom(*patternLane1Step2Relay)
            .withOptionsFrom(*patternLane1Step3Relay)
            .withOptionsFrom(*patternLane1Step4Relay)
            .withOptionsFrom(*patternLane1Step5Relay)
            .withOptionsFrom(*patternLane1Step6Relay)
            .withOptionsFrom(*patternLane1Step7Relay)
            .withOptionsFrom(*patternLane1Step8Relay)
            .withOptionsFrom(*patternLane1Step9Relay)
            .withOptionsFrom(*patternLane1Step10Relay)
            .withOptionsFrom(*patternLane1Step11Relay)
            .withOptionsFrom(*patternLane1Step12Relay)
            .withOptionsFrom(*patternLane1Step13Relay)
            .withOptionsFrom(*patternLane1Step14Relay)
            .withOptionsFrom(*patternLane1Step15Relay)
            .withOptionsFrom(*patternLane1Step16Relay)
            .withOptionsFrom(*patternLane2Step1Relay)
            .withOptionsFrom(*patternLane2Step2Relay)
            .withOptionsFrom(*patternLane2Step3Relay)
            .withOptionsFrom(*patternLane2Step4Relay)
            .withOptionsFrom(*patternLane2Step5Relay)
            .withOptionsFrom(*patternLane2Step6Relay)
            .withOptionsFrom(*patternLane2Step7Relay)
            .withOptionsFrom(*patternLane2Step8Relay)
            .withOptionsFrom(*patternLane2Step9Relay)
            .withOptionsFrom(*patternLane2Step10Relay)
            .withOptionsFrom(*patternLane2Step11Relay)
            .withOptionsFrom(*patternLane2Step12Relay)
            .withOptionsFrom(*patternLane2Step13Relay)
            .withOptionsFrom(*patternLane2Step14Relay)
            .withOptionsFrom(*patternLane2Step15Relay)
            .withOptionsFrom(*patternLane2Step16Relay)
            .withOptionsFrom(*patternLane3Step1Relay)
            .withOptionsFrom(*patternLane3Step2Relay)
            .withOptionsFrom(*patternLane3Step3Relay)
            .withOptionsFrom(*patternLane3Step4Relay)
            .withOptionsFrom(*patternLane3Step5Relay)
            .withOptionsFrom(*patternLane3Step6Relay)
            .withOptionsFrom(*patternLane3Step7Relay)
            .withOptionsFrom(*patternLane3Step8Relay)
            .withOptionsFrom(*patternLane3Step9Relay)
            .withOptionsFrom(*patternLane3Step10Relay)
            .withOptionsFrom(*patternLane3Step11Relay)
            .withOptionsFrom(*patternLane3Step12Relay)
            .withOptionsFrom(*patternLane3Step13Relay)
            .withOptionsFrom(*patternLane3Step14Relay)
            .withOptionsFrom(*patternLane3Step15Relay)
            .withOptionsFrom(*patternLane3Step16Relay)
            .withOptionsFrom(*patternLane4Step1Relay)
            .withOptionsFrom(*patternLane4Step2Relay)
            .withOptionsFrom(*patternLane4Step3Relay)
            .withOptionsFrom(*patternLane4Step4Relay)
            .withOptionsFrom(*patternLane4Step5Relay)
            .withOptionsFrom(*patternLane4Step6Relay)
            .withOptionsFrom(*patternLane4Step7Relay)
            .withOptionsFrom(*patternLane4Step8Relay)
            .withOptionsFrom(*patternLane4Step9Relay)
            .withOptionsFrom(*patternLane4Step10Relay)
            .withOptionsFrom(*patternLane4Step11Relay)
            .withOptionsFrom(*patternLane4Step12Relay)
            .withOptionsFrom(*patternLane4Step13Relay)
            .withOptionsFrom(*patternLane4Step14Relay)
            .withOptionsFrom(*patternLane4Step15Relay)
            .withOptionsFrom(*patternLane4Step16Relay)
            // v1.6.0: Preset manager native functions
            .withNativeFunction("savePreset", [this](auto& args, auto complete) {
                if (args.size() > 0)
                    complete(processorRef.presetManager.savePreset(args[0].toString()));
                else
                    complete(false);
            })
            .withNativeFunction("savePresetWithDialog", [this](auto&, auto complete) {
                // Open native save dialog in User presets folder (JUCE 8 async pattern)
                auto userDir = processorRef.presetManager.getUserPresetsDirectory();
                userDir.createDirectory();

                auto* chooser = new juce::FileChooser("Save Preset", userDir, "*.json");
                chooser->launchAsync(
                    juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                    [this, chooser, complete](const juce::FileChooser& fc) {
                        auto results = fc.getResults();
                        if (results.size() > 0)
                        {
                            auto file = results[0];
                            auto name = file.getFileNameWithoutExtension();
                            auto success = processorRef.presetManager.savePreset(name);

                            auto* result = new juce::DynamicObject();
                            result->setProperty("success", success);
                            result->setProperty("name", name);
                            complete(juce::var(result));
                        }
                        else
                        {
                            auto* result = new juce::DynamicObject();
                            result->setProperty("success", false);
                            result->setProperty("name", "");
                            complete(juce::var(result));
                        }
                        delete chooser;
                    });
            })
            .withNativeFunction("loadPreset", [this](auto& args, auto complete) {
                if (args.size() > 0)
                    complete(processorRef.presetManager.loadPreset(args[0].toString()));
                else
                    complete(false);
            })
            .withNativeFunction("loadPresetFromFile", [this](auto&, auto complete) {
                // Open native load dialog (JUCE 8 async pattern)
                auto presetsDir = processorRef.presetManager.getPresetsDirectory();

                auto* chooser = new juce::FileChooser("Load Preset", presetsDir, "*.json");
                chooser->launchAsync(
                    juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                    [this, chooser, complete](const juce::FileChooser& fc) {
                        auto results = fc.getResults();
                        if (results.size() > 0)
                        {
                            auto file = results[0];
                            auto success = processorRef.presetManager.loadPresetFromFile(file);
                            auto name = file.getFileNameWithoutExtension();

                            auto* result = new juce::DynamicObject();
                            result->setProperty("success", success);
                            result->setProperty("name", name);
                            complete(juce::var(result));
                        }
                        else
                        {
                            auto* result = new juce::DynamicObject();
                            result->setProperty("success", false);
                            result->setProperty("name", "");
                            complete(juce::var(result));
                        }
                        delete chooser;
                    });
            })
            .withNativeFunction("getPresetList", [this](auto&, auto complete) {
                auto list = processorRef.presetManager.getPresetList();
                juce::Array<juce::var> arr;
                for (const auto& name : list)
                    arr.add(name);
                complete(juce::var(arr));
            })
            .withNativeFunction("getCurrentPreset", [this](auto&, auto complete) {
                complete(processorRef.presetManager.getCurrentPresetName());
            })
            .withNativeFunction("selectNextPreset", [this](auto&, auto complete) {
                auto next = processorRef.presetManager.getNextPreset();
                complete(next);
            })
            .withNativeFunction("selectPreviousPreset", [this](auto&, auto complete) {
                auto prev = processorRef.presetManager.getPreviousPreset();
                complete(prev);
            })
            .withNativeFunction("deletePreset", [this](auto& args, auto complete) {
                if (args.size() > 0)
                    complete(processorRef.presetManager.deletePreset(args[0].toString()));
                else
                    complete(false);
            })
            .withNativeFunction("isFactoryPreset", [this](auto& args, auto complete) {
                if (args.size() > 0)
                    complete(processorRef.presetManager.isFactoryPreset(args[0].toString()));
                else
                    complete(false);
            })
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

    , lane1SubdivisionAttachment(std::make_unique<juce::WebComboBoxParameterAttachment>(
        *processorRef.apvts.getParameter("lane1_subdivision"), *lane1SubdivisionRelay, nullptr))
    // v1.7.0: Lane 1 pitch randomization attachments
    , lane1PitchRandEnabledAttachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("lane1_pitch_rand_enabled"), *lane1PitchRandEnabledRelay, nullptr))
    , lane1PitchRandMinAttachment(std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.apvts.getParameter("lane1_pitch_rand_min"), *lane1PitchRandMinRelay, nullptr))
    , lane1PitchRandMaxAttachment(std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.apvts.getParameter("lane1_pitch_rand_max"), *lane1PitchRandMaxRelay, nullptr))
    , lane1PitchRandQuantizeAttachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("lane1_pitch_rand_quantize"), *lane1PitchRandQuantizeRelay, nullptr))
    // v1.9.0: Lane 1 Euclidean attachments
    , lane1EuclideanEnabledAttachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("lane1_euclidean_enabled"), *lane1EuclideanEnabledRelay, nullptr))
    , lane1EuclideanPulsesAttachment(std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.apvts.getParameter("lane1_euclidean_pulses"), *lane1EuclideanPulsesRelay, nullptr))
    , lane1EuclideanStepsAttachment(std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.apvts.getParameter("lane1_euclidean_steps"), *lane1EuclideanStepsRelay, nullptr))

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

    , lane2SubdivisionAttachment(std::make_unique<juce::WebComboBoxParameterAttachment>(
        *processorRef.apvts.getParameter("lane2_subdivision"), *lane2SubdivisionRelay, nullptr))
    // v1.7.0: Lane 2 pitch randomization attachments
    , lane2PitchRandEnabledAttachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("lane2_pitch_rand_enabled"), *lane2PitchRandEnabledRelay, nullptr))
    , lane2PitchRandMinAttachment(std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.apvts.getParameter("lane2_pitch_rand_min"), *lane2PitchRandMinRelay, nullptr))
    , lane2PitchRandMaxAttachment(std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.apvts.getParameter("lane2_pitch_rand_max"), *lane2PitchRandMaxRelay, nullptr))
    , lane2PitchRandQuantizeAttachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("lane2_pitch_rand_quantize"), *lane2PitchRandQuantizeRelay, nullptr))
    // v1.9.0: Lane 2 Euclidean attachments
    , lane2EuclideanEnabledAttachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("lane2_euclidean_enabled"), *lane2EuclideanEnabledRelay, nullptr))
    , lane2EuclideanPulsesAttachment(std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.apvts.getParameter("lane2_euclidean_pulses"), *lane2EuclideanPulsesRelay, nullptr))
    , lane2EuclideanStepsAttachment(std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.apvts.getParameter("lane2_euclidean_steps"), *lane2EuclideanStepsRelay, nullptr))

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

    , lane3SubdivisionAttachment(std::make_unique<juce::WebComboBoxParameterAttachment>(
        *processorRef.apvts.getParameter("lane3_subdivision"), *lane3SubdivisionRelay, nullptr))
    // v1.7.0: Lane 3 pitch randomization attachments
    , lane3PitchRandEnabledAttachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("lane3_pitch_rand_enabled"), *lane3PitchRandEnabledRelay, nullptr))
    , lane3PitchRandMinAttachment(std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.apvts.getParameter("lane3_pitch_rand_min"), *lane3PitchRandMinRelay, nullptr))
    , lane3PitchRandMaxAttachment(std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.apvts.getParameter("lane3_pitch_rand_max"), *lane3PitchRandMaxRelay, nullptr))
    , lane3PitchRandQuantizeAttachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("lane3_pitch_rand_quantize"), *lane3PitchRandQuantizeRelay, nullptr))
    // v1.9.0: Lane 3 Euclidean attachments
    , lane3EuclideanEnabledAttachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("lane3_euclidean_enabled"), *lane3EuclideanEnabledRelay, nullptr))
    , lane3EuclideanPulsesAttachment(std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.apvts.getParameter("lane3_euclidean_pulses"), *lane3EuclideanPulsesRelay, nullptr))
    , lane3EuclideanStepsAttachment(std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.apvts.getParameter("lane3_euclidean_steps"), *lane3EuclideanStepsRelay, nullptr))

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

    , lane4SubdivisionAttachment(std::make_unique<juce::WebComboBoxParameterAttachment>(
        *processorRef.apvts.getParameter("lane4_subdivision"), *lane4SubdivisionRelay, nullptr))
    // v1.7.0: Lane 4 pitch randomization attachments
    , lane4PitchRandEnabledAttachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("lane4_pitch_rand_enabled"), *lane4PitchRandEnabledRelay, nullptr))
    , lane4PitchRandMinAttachment(std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.apvts.getParameter("lane4_pitch_rand_min"), *lane4PitchRandMinRelay, nullptr))
    , lane4PitchRandMaxAttachment(std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.apvts.getParameter("lane4_pitch_rand_max"), *lane4PitchRandMaxRelay, nullptr))
    , lane4PitchRandQuantizeAttachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("lane4_pitch_rand_quantize"), *lane4PitchRandQuantizeRelay, nullptr))
    // v1.9.0: Lane 4 Euclidean attachments
    , lane4EuclideanEnabledAttachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("lane4_euclidean_enabled"), *lane4EuclideanEnabledRelay, nullptr))
    , lane4EuclideanPulsesAttachment(std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.apvts.getParameter("lane4_euclidean_pulses"), *lane4EuclideanPulsesRelay, nullptr))
    , lane4EuclideanStepsAttachment(std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.apvts.getParameter("lane4_euclidean_steps"), *lane4EuclideanStepsRelay, nullptr))

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
    , tapeBypassAttachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("tape_bypass"), *tapeBypassRelay, nullptr))  // v1.6.5

    // v1.1.0: Wet/dry mix attachments
    , mixDryAttachment(std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.apvts.getParameter("mix_dry"), *mixDryRelay, nullptr))
    , mixWetAttachment(std::make_unique<juce::WebSliderParameterAttachment>(
        *processorRef.apvts.getParameter("mix_wet"), *mixWetRelay, nullptr))

    // Global control attachments - v1.3.1: Removed ENV and SC
    , midiEnabledAttachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("midi_enabled"), *midiEnabledRelay, nullptr))
    , manualTriggerAttachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("manual_trigger"), *manualTriggerRelay, nullptr))
    , sequencerEnabledAttachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("sequencer_enabled"), *sequencerEnabledRelay, nullptr))  // v1.0.2

    // Phase 3.3: Pattern step attachments (64 toggles)
    , patternLane1Step1Attachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("pattern_lane1_step1"), *patternLane1Step1Relay, nullptr))
    , patternLane1Step2Attachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("pattern_lane1_step2"), *patternLane1Step2Relay, nullptr))
    , patternLane1Step3Attachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("pattern_lane1_step3"), *patternLane1Step3Relay, nullptr))
    , patternLane1Step4Attachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("pattern_lane1_step4"), *patternLane1Step4Relay, nullptr))
    , patternLane1Step5Attachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("pattern_lane1_step5"), *patternLane1Step5Relay, nullptr))
    , patternLane1Step6Attachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("pattern_lane1_step6"), *patternLane1Step6Relay, nullptr))
    , patternLane1Step7Attachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("pattern_lane1_step7"), *patternLane1Step7Relay, nullptr))
    , patternLane1Step8Attachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("pattern_lane1_step8"), *patternLane1Step8Relay, nullptr))
    , patternLane1Step9Attachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("pattern_lane1_step9"), *patternLane1Step9Relay, nullptr))
    , patternLane1Step10Attachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("pattern_lane1_step10"), *patternLane1Step10Relay, nullptr))
    , patternLane1Step11Attachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("pattern_lane1_step11"), *patternLane1Step11Relay, nullptr))
    , patternLane1Step12Attachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("pattern_lane1_step12"), *patternLane1Step12Relay, nullptr))
    , patternLane1Step13Attachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("pattern_lane1_step13"), *patternLane1Step13Relay, nullptr))
    , patternLane1Step14Attachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("pattern_lane1_step14"), *patternLane1Step14Relay, nullptr))
    , patternLane1Step15Attachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("pattern_lane1_step15"), *patternLane1Step15Relay, nullptr))
    , patternLane1Step16Attachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("pattern_lane1_step16"), *patternLane1Step16Relay, nullptr))
    , patternLane2Step1Attachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("pattern_lane2_step1"), *patternLane2Step1Relay, nullptr))
    , patternLane2Step2Attachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("pattern_lane2_step2"), *patternLane2Step2Relay, nullptr))
    , patternLane2Step3Attachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("pattern_lane2_step3"), *patternLane2Step3Relay, nullptr))
    , patternLane2Step4Attachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("pattern_lane2_step4"), *patternLane2Step4Relay, nullptr))
    , patternLane2Step5Attachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("pattern_lane2_step5"), *patternLane2Step5Relay, nullptr))
    , patternLane2Step6Attachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("pattern_lane2_step6"), *patternLane2Step6Relay, nullptr))
    , patternLane2Step7Attachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("pattern_lane2_step7"), *patternLane2Step7Relay, nullptr))
    , patternLane2Step8Attachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("pattern_lane2_step8"), *patternLane2Step8Relay, nullptr))
    , patternLane2Step9Attachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("pattern_lane2_step9"), *patternLane2Step9Relay, nullptr))
    , patternLane2Step10Attachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("pattern_lane2_step10"), *patternLane2Step10Relay, nullptr))
    , patternLane2Step11Attachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("pattern_lane2_step11"), *patternLane2Step11Relay, nullptr))
    , patternLane2Step12Attachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("pattern_lane2_step12"), *patternLane2Step12Relay, nullptr))
    , patternLane2Step13Attachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("pattern_lane2_step13"), *patternLane2Step13Relay, nullptr))
    , patternLane2Step14Attachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("pattern_lane2_step14"), *patternLane2Step14Relay, nullptr))
    , patternLane2Step15Attachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("pattern_lane2_step15"), *patternLane2Step15Relay, nullptr))
    , patternLane2Step16Attachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("pattern_lane2_step16"), *patternLane2Step16Relay, nullptr))
    , patternLane3Step1Attachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("pattern_lane3_step1"), *patternLane3Step1Relay, nullptr))
    , patternLane3Step2Attachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("pattern_lane3_step2"), *patternLane3Step2Relay, nullptr))
    , patternLane3Step3Attachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("pattern_lane3_step3"), *patternLane3Step3Relay, nullptr))
    , patternLane3Step4Attachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("pattern_lane3_step4"), *patternLane3Step4Relay, nullptr))
    , patternLane3Step5Attachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("pattern_lane3_step5"), *patternLane3Step5Relay, nullptr))
    , patternLane3Step6Attachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("pattern_lane3_step6"), *patternLane3Step6Relay, nullptr))
    , patternLane3Step7Attachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("pattern_lane3_step7"), *patternLane3Step7Relay, nullptr))
    , patternLane3Step8Attachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("pattern_lane3_step8"), *patternLane3Step8Relay, nullptr))
    , patternLane3Step9Attachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("pattern_lane3_step9"), *patternLane3Step9Relay, nullptr))
    , patternLane3Step10Attachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("pattern_lane3_step10"), *patternLane3Step10Relay, nullptr))
    , patternLane3Step11Attachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("pattern_lane3_step11"), *patternLane3Step11Relay, nullptr))
    , patternLane3Step12Attachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("pattern_lane3_step12"), *patternLane3Step12Relay, nullptr))
    , patternLane3Step13Attachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("pattern_lane3_step13"), *patternLane3Step13Relay, nullptr))
    , patternLane3Step14Attachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("pattern_lane3_step14"), *patternLane3Step14Relay, nullptr))
    , patternLane3Step15Attachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("pattern_lane3_step15"), *patternLane3Step15Relay, nullptr))
    , patternLane3Step16Attachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("pattern_lane3_step16"), *patternLane3Step16Relay, nullptr))
    , patternLane4Step1Attachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("pattern_lane4_step1"), *patternLane4Step1Relay, nullptr))
    , patternLane4Step2Attachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("pattern_lane4_step2"), *patternLane4Step2Relay, nullptr))
    , patternLane4Step3Attachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("pattern_lane4_step3"), *patternLane4Step3Relay, nullptr))
    , patternLane4Step4Attachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("pattern_lane4_step4"), *patternLane4Step4Relay, nullptr))
    , patternLane4Step5Attachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("pattern_lane4_step5"), *patternLane4Step5Relay, nullptr))
    , patternLane4Step6Attachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("pattern_lane4_step6"), *patternLane4Step6Relay, nullptr))
    , patternLane4Step7Attachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("pattern_lane4_step7"), *patternLane4Step7Relay, nullptr))
    , patternLane4Step8Attachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("pattern_lane4_step8"), *patternLane4Step8Relay, nullptr))
    , patternLane4Step9Attachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("pattern_lane4_step9"), *patternLane4Step9Relay, nullptr))
    , patternLane4Step10Attachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("pattern_lane4_step10"), *patternLane4Step10Relay, nullptr))
    , patternLane4Step11Attachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("pattern_lane4_step11"), *patternLane4Step11Relay, nullptr))
    , patternLane4Step12Attachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("pattern_lane4_step12"), *patternLane4Step12Relay, nullptr))
    , patternLane4Step13Attachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("pattern_lane4_step13"), *patternLane4Step13Relay, nullptr))
    , patternLane4Step14Attachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("pattern_lane4_step14"), *patternLane4Step14Relay, nullptr))
    , patternLane4Step15Attachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("pattern_lane4_step15"), *patternLane4Step15Relay, nullptr))
    , patternLane4Step16Attachment(std::make_unique<juce::WebToggleButtonParameterAttachment>(
        *processorRef.apvts.getParameter("pattern_lane4_step16"), *patternLane4Step16Relay, nullptr))
{
    addAndMakeVisible(*webView);

    // Navigate to UI (served from BinaryData)
    webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());

    // Fixed window size: 1000×690px (v1.12.0: taller for improved Euclidean controls)
    setSize(1000, 690);

    // v1.5.0: Start timer for lane progress updates (~30Hz for smooth animation)
    startTimerHz(30);

    DBG("[Phase 3.3] Parameter bindings initialized - 128 relays + 128 attachments (incl. 64 pattern steps)");
}

OPolystutterAudioProcessorEditor::~OPolystutterAudioProcessorEditor()
{
    // v1.5.0: Stop timer before destruction
    stopTimer();
}

void OPolystutterAudioProcessorEditor::paint(juce::Graphics& g)
{
    // WebView handles all rendering (no custom paint needed)
    juce::ignoreUnused(g);
}

void OPolystutterAudioProcessorEditor::resized()
{
    // WebView fills entire editor window
    if (webView)
        webView->setBounds(getLocalBounds());
}

std::optional<juce::WebBrowserComponent::Resource>
OPolystutterAudioProcessorEditor::getResource(const juce::String& url)
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

    // v1.6.0: Preset manager module
    if (url == "/modules/preset-manager.js") {
        return juce::WebBrowserComponent::Resource {
            makeVector(BinaryData::presetmanager_js, BinaryData::presetmanager_jsSize),
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

// v1.5.0: Timer callback for lane progress updates
void OPolystutterAudioProcessorEditor::timerCallback()
{
    // Read progress values from processor atomics (thread-safe)
    float p1 = processorRef.lane1Progress.load(std::memory_order_relaxed);
    float p2 = processorRef.lane2Progress.load(std::memory_order_relaxed);
    float p3 = processorRef.lane3Progress.load(std::memory_order_relaxed);
    float p4 = processorRef.lane4Progress.load(std::memory_order_relaxed);

    bool a1 = processorRef.lane1Active.load(std::memory_order_relaxed);
    bool a2 = processorRef.lane2Active.load(std::memory_order_relaxed);
    bool a3 = processorRef.lane3Active.load(std::memory_order_relaxed);
    bool a4 = processorRef.lane4Active.load(std::memory_order_relaxed);

    // Build payload as DynamicObject for proper JSON serialization
    auto payload = std::make_unique<juce::DynamicObject>();

    auto lane1 = std::make_unique<juce::DynamicObject>();
    lane1->setProperty("progress", p1);
    lane1->setProperty("active", a1);
    payload->setProperty("lane1", lane1.release());

    auto lane2 = std::make_unique<juce::DynamicObject>();
    lane2->setProperty("progress", p2);
    lane2->setProperty("active", a2);
    payload->setProperty("lane2", lane2.release());

    auto lane3 = std::make_unique<juce::DynamicObject>();
    lane3->setProperty("progress", p3);
    lane3->setProperty("active", a3);
    payload->setProperty("lane3", lane3.release());

    auto lane4 = std::make_unique<juce::DynamicObject>();
    lane4->setProperty("progress", p4);
    lane4->setProperty("active", a4);
    payload->setProperty("lane4", lane4.release());

    // Emit custom event to WebView
    // Use emitEventIfBrowserIsVisible to avoid unnecessary work when UI is hidden
    if (webView)
    {
        webView->emitEventIfBrowserIsVisible("laneProgress", juce::var(payload.release()));
    }
}
