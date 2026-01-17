/*
  ==============================================================================

    HarpSynthVoice.h
    Physical Modeling Harp Synthesizer Voice
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "HarpSynthSound.h"

class HarpSynthVoice : public juce::SynthesiserVoice
{
public:
    HarpSynthVoice();

    bool canPlaySound(juce::SynthesiserSound* sound) override;

    void startNote(int midiNoteNumber, float velocity,
                   juce::SynthesiserSound* sound,
                   int currentPitchWheelPosition) override;

    void stopNote(float velocity, bool allowTailOff) override;

    void pitchWheelMoved(int newPitchWheelValue) override;
    void controllerMoved(int controllerNumber, int newControllerValue) override;

    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                        int startSample, int numSamples) override;

private:
    double currentFrequency = 440.0;
    double phase = 0.0;
    double phaseIncrement = 0.0;
    float currentVelocity = 0.0f;
    bool isPlaying = false;

    // Simple ADSR for Stage 1 placeholder
    juce::ADSR adsr;
    juce::ADSR::Parameters adsrParams;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HarpSynthVoice)
};
