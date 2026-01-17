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
#include "DSP/WaveguideString.h"
#include "DSP/StringMaterial.h"
#include "DSP/BodyResonance.h"

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

    /**
     * Prepare voice for playback
     */
    void prepare(double sampleRate, int maxBlockSize);

    /**
     * Set APVTS reference for parameter access
     */
    void setAPVTS(juce::AudioProcessorValueTreeState* apvts);

private:
    // Physical modeling string (Phase 2.2 - Bidirectional Waveguide)
    WaveguideString stringModel;

    // Phase 2.6: Body Resonance (modal synthesis)
    BodyResonance bodyResonance;

    // APVTS reference for parameter access
    juce::AudioProcessorValueTreeState* parameters = nullptr;

    double currentFrequency = 440.0;
    float currentVelocity = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HarpSynthVoice)
};
