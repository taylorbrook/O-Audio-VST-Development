/*
  ==============================================================================

    PrismVoice.h
    O-Prism - Microtonal Wavetable Synthesizer
    Ouaricon Audio

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "dsp/WavetableOscillator.h"
#include "dsp/SubOscillator.h"
#include "dsp/NoiseGenerator.h"
#include "dsp/GlideProcessor.h"
#include "dsp/SVFFilter.h"
#include "dsp/LFO.h"
#include "dsp/ModulationMatrix.h"

class TuningEngine;
class PrismSound;
class OPrismAudioProcessor;
struct WavetableData;

class PrismVoice : public juce::SynthesiserVoice
{
public:
    PrismVoice();

    void setAPVTS (juce::AudioProcessorValueTreeState* apvts);
    void setTuningEngine (TuningEngine* engine);
    void setProcessor (OPrismAudioProcessor* proc);
    void prepare (double sampleRate, int samplesPerBlock);

    bool canPlaySound (juce::SynthesiserSound* sound) override;
    void startNote (int midiNoteNumber, float velocity,
                    juce::SynthesiserSound* sound, int currentPitchWheelPosition) override;
    void stopNote (float velocity, bool allowTailOff) override;
    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer,
                          int startSample, int numSamples) override;
    void pitchWheelMoved (int newPitchWheelValue) override;
    void controllerMoved (int controllerNumber, int newControllerValue) override;

    void setWavetableA (const WavetableData* table);
    void setWavetableB (const WavetableData* table);

private:
    juce::AudioProcessorValueTreeState* parameters = nullptr;
    TuningEngine* tuningEngine = nullptr;
    OPrismAudioProcessor* processor = nullptr;

    // Modulation matrix (per-voice for per-sample evaluation)
    ModulationMatrix modMatrix;

    double currentFrequency = 0.0;
    float noteVelocity = 0.0f;
    int currentMidiNote = -1;
    int currentPitchWheel = 8192;
    double voiceSampleRate = 44100.0;

    // Oscillators
    WavetableOscillator oscA;
    WavetableOscillator oscB;

    // Sub & Noise
    SubOscillator subOsc;
    NoiseGenerator noiseGen;

    // Glide
    GlideProcessor glide;

    // Envelopes
    juce::ADSR ampEnvelope;
    juce::ADSR filterEnvelope;

    // Filters (separate L/R instances for true stereo processing)
    SVFFilter filterAL, filterAR;
    SVFFilter filterBL, filterBR;

    // LFOs (per-voice for smooth per-sample modulation)
    LFO lfo1, lfo2;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PrismVoice)
};
