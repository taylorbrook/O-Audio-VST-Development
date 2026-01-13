/*
  ==============================================================================

    Ouaricon Digital Delay - Audio Processor
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "OuariconPresetManager.h"

class OuariconDigitalDelayAudioProcessor : public juce::AudioProcessor
{
public:
    OuariconDigitalDelayAudioProcessor();
    ~OuariconDigitalDelayAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Ouaricon Digital Delay"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override;

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState parameters;

    // Preset management
    OuariconPresetManager presetManager;

    // RMS level getters for output meter (returns 0.0-1.0)
    float getRmsLevelLeft() const { return rmsLevelLeft.getCurrentValue(); }
    float getRmsLevelRight() const { return rmsLevelRight.getCurrentValue(); }

private:
    // Parameter layout creation
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // DSP Components (declare BEFORE APVTS for initialization order)
    juce::dsp::ProcessSpec spec;

    // Dual mono delay lines with Lagrange3rd interpolation
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> delayLineLeft;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> delayLineRight;

    // LFO for delay time modulation (0.3Hz fixed rate)
    juce::dsp::Oscillator<float> lfo;

    // Smoothed parameter values (20ms ramp to prevent clicks)
    juce::SmoothedValue<float> smoothedTimeMs { 500.0f };
    juce::SmoothedValue<float> smoothedFeedback { 0.3f };
    juce::SmoothedValue<float> smoothedSpread { 0.0f };
    juce::SmoothedValue<float> smoothedMod { 0.0f };
    juce::SmoothedValue<float> smoothedWet { 0.3f };
    juce::SmoothedValue<float> smoothedDry { 1.0f };

    // Feedback state (per-channel)
    float feedbackLeft = 0.0f;
    float feedbackRight = 0.0f;

    // RMS level calculation for output meter
    juce::LinearSmoothedValue<float> rmsLevelLeft { 0.0f };
    juce::LinearSmoothedValue<float> rmsLevelRight { 0.0f };

    // Subdivision lookup table (12 values)
    static constexpr float subdivisionFactors[12] = {
        1.0f,     // 1/4
        0.5f,     // 1/8
        0.25f,    // 1/16
        1.5f,     // 1/4D (dotted)
        0.75f,    // 1/8D
        0.375f,   // 1/16D
        0.667f,   // 1/4T (triplet)
        0.333f,   // 1/8T
        0.167f,   // 1/16T
        0.8f,     // 1/4(5) (quintuplet)
        0.4f,     // 1/8(5)
        0.2f      // 1/16(5)
    };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OuariconDigitalDelayAudioProcessor)
};
