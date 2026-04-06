/*
  ==============================================================================

    FluteSynthVoice.h
    O-Wind - Physical Modeling Flute Synthesiser Voice
    Ouaricon Audio
    Developer: Taylor Brook

    SynthesiserVoice subclass orchestrating the jet-drive waveguide flute model.
    Per-sample pipeline at 2x oversampled rate:
    JetExciter -> Jet Delay -> JetNonlinearity -> DCBlocker ->
    BoreWaveguide -> output + feedback loop.
    Reads APVTS parameters once per block.

  ==============================================================================
*/

#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "DSP/JetExciter.h"
#include "DSP/JetNonlinearity.h"
#include "DSP/DCBlocker.h"
#include "DSP/BoreWaveguide.h"
#include "DSP/InstrumentPresets.h"

class TuningEngine;

class FluteSynthVoice : public juce::SynthesiserVoice
{
public:
    explicit FluteSynthVoice (juce::AudioProcessorValueTreeState* apvts,
                              TuningEngine* tuning);

    bool canPlaySound (juce::SynthesiserSound* sound) override;
    void startNote (int midiNoteNumber, float velocity,
                    juce::SynthesiserSound* sound, int currentPitchWheelPosition) override;
    void stopNote (float velocity, bool allowTailOff) override;
    void pitchWheelMoved (int newPitchWheelValue) override;
    void controllerMoved (int controllerNumber, int newControllerValue) override;
    void prepareToPlay (double sampleRate, int maxBlockSize);
    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer,
                          int startSample, int numSamples) override;

    // Query oversampling latency (called by processor for setLatencySamples)
    float getOversamplingLatency() const;

private:
    void updateParametersFromAPVTS();

    juce::AudioProcessorValueTreeState* parameters = nullptr;
    TuningEngine* tuningEngine = nullptr;

    // Current preset internal coefficients (applied on startNote or param change)
    InstrumentPreset currentPreset = InstrumentPresets::concertFlute;
    void applyPresetCoefficients();

    // DSP components (all prepared at internal oversampled rate)
    JetExciter jetExciter;
    JetNonlinearity jetNonlinearity;
    DCBlocker dcBlocker;
    BoreWaveguide boreWaveguide;

    // Jet delay line (Lagrange3rd for real-time embouchure modulation)
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> jetDelay { 1024 };

    // Per-voice 2x oversampling (1 channel, polyphase IIR)
    juce::dsp::Oversampling<float> oversampling {
        1, 1,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
        true, false
    };
    juce::AudioBuffer<float> tempBuffer;  // native-rate temp for up/downsample

    // SmoothedValue for total loop delay (bore + jet, split in render loop)
    // Reset at internal (oversampled) rate so getNextValue advances correctly
    juce::SmoothedValue<float> totalDelaySmoothed;
    juce::SmoothedValue<float> embouchureSmoothed;

    // Pitch vibrato (modulates bore delay, not breath pressure)
    float vibratoPhase = 0.0f;
    float vibratoPhaseInc = 0.0f;
    float vibratoDepthParam = 0.0f;

    // Dynamic filter delay compensation (computed once per block)
    float filterDelayCompensation = 2.0f;

    // Voice state
    float outputGainLinear = 1.0f;
    float currentFrequency = 440.0f;
    int currentMidiNote = 60;
    double internalSampleRate = 88200.0;  // 2x oversampled
    double nativeSampleRate = 44100.0;

    // Preset change tracking
    int lastPresetIndex = -1;

    // Energy tracking for voice cleanup
    int silentSampleCount = 0;
    static constexpr int silentThreshold = 512;  // ~6ms at 88.2kHz internal rate
    static constexpr float energyThreshold = 0.0001f;

    // Release tail fade: ensures voice clearing even if waveguide has residual energy
    float releaseFade = 1.0f;
    float releaseFadeInc = 0.0f;
    bool releaseFading = false;

    // CC state for MPE
    float ccBreathPressure = 0.0f;
    float ccEmbouchure = 0.0f;
    float ccVibratoDepth = 0.0f;
    int pitchWheelValue = 8192;

    // Pitch bend state
    float pitchBendSemitones = 0.0f;
    static constexpr float pitchBendRange = 2.0f;  // +/- 2 semitones

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FluteSynthVoice)
};
