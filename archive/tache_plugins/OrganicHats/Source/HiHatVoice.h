#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "HiHatSound.h"

class HiHatVoice : public juce::SynthesiserVoice
{
public:
    HiHatVoice(juce::AudioProcessorValueTreeState& apvts);

    bool canPlaySound(juce::SynthesiserSound* sound) override;

    void startNote(int midiNoteNumber, float velocity,
                   juce::SynthesiserSound*, int currentPitchWheelPosition) override;

    void stopNote(float velocity, bool allowTailOff) override;

    void pitchWheelMoved(int) override {}
    void controllerMoved(int, int) override {}

    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                        int startSample, int numSamples) override;

    void prepareToPlay(double sampleRate, int samplesPerBlock);

private:
    juce::AudioProcessorValueTreeState& parameters;

    // Noise generation
    juce::Random noiseGenerator;

    // Envelope shaping
    juce::ADSR envelope;

    // Filtering (Phase 4.2)
    juce::dsp::IIR::Filter<float> toneFilter;
    juce::dsp::IIR::Filter<float> noiseColorFilter;

    // Resonators (Phase 4.3) - Fixed peaks for organic body
    std::array<juce::dsp::IIR::Filter<float>, 3> resonators;

    double currentSampleRate = 44100.0;

    // Voice state
    bool isClosed = true;  // C1 = closed, D1 = open
    float velocityGain = 1.0f;

public:
    // Choke support methods (Phase 4.3)
    bool isOpen() const { return !isClosed; }
    void forceRelease();
};
