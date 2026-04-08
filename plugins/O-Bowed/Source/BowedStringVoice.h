/*
  ==============================================================================

    BowedStringVoice.h
    O-Bowed - Physical Modeling Bowed String Voice (MPE)
    Ouaricon Audio
    Developer: Taylor Brook

    MPESynthesiserVoice subclass owning WaveguideString + BowModel +
    HyperbolicFriction. Reads APVTS parameters once per block.
    Continuous excitation model (bow stays in contact during sustain).
    Per-voice 2x oversampling for friction/waveguide inner loop.
    TuningEngine integration for Scala/MTS-ESP/reference pitch.

  ==============================================================================
*/

#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "DSP/WaveguideString.h"
#include "DSP/BowModel.h"
#include "DSP/HyperbolicFriction.h"
#include "DSP/ElastoPlasticFriction.h"
#include "DSP/ThermalFriction.h"
#include "DSP/SubHarmonicsGenerator.h"
#include "DSP/BowNoiseGenerator.h"

class TuningEngine;

class BowedStringVoice : public juce::MPESynthesiserVoice
{
public:
    explicit BowedStringVoice (juce::AudioProcessorValueTreeState* apvts);

    // MPE voice callbacks
    void noteStarted() override;
    void noteStopped (bool allowTailOff) override;
    void notePitchbendChanged() override;
    void notePressureChanged() override;
    void noteTimbreChanged() override;
    void noteKeyStateChanged() override;

    void prepareToPlay (double sampleRate, int maxBlockSize);
    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer,
                          int startSample, int numSamples) override;

    // Per-voice stereo panning (set from processor)
    void setPan (float left, float right) { panL = left; panR = right; }

    // Expose current frequency for sympathetic tuning
    float getCurrentFrequency() const noexcept { return currentFrequency; }

    // MPE expression (CC11) -- set from BowedMPESynthesiser
    void setExpression (float value) noexcept { mpeExpression = value; }

    // TuningEngine integration
    void setTuningEngine (TuningEngine* engine) noexcept { tuningEngine = engine; }

    // Voice index for seeding noise generator
    void setVoiceIndex (int idx) noexcept { voiceIndex = idx; }

    // Oversampling latency for host reporting
    float getOversamplingLatency() const noexcept { return oversampling.getLatencyInSamples(); }

private:
    void updateParametersFromAPVTS();
    float getBaseFrequencyFromTuning (int midiNote) const;

    juce::AudioProcessorValueTreeState* parameters = nullptr;

    WaveguideString waveguideString;
    BowModel bowModel;
    HyperbolicFriction frictionModel;

    // Advanced friction tiers
    ElastoPlasticFriction enhancedFriction;
    ThermalFriction qualityFriction;
    SubHarmonicsGenerator subHarmonicsGen;

    // Bow noise generator (post-body additive)
    BowNoiseGenerator bowNoiseGen;

    // Per-voice 2x oversampling (mono, IIR polyphase)
    juce::dsp::Oversampling<float> oversampling { 1, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR };
    juce::AudioBuffer<float> voiceBuffer;

    // TuningEngine (processor-owned, voice holds non-owning pointer)
    TuningEngine* tuningEngine = nullptr;

    // Friction tier state
    int currentTier = 0;
    int previousTier = 0;

    // Impossible physics parameters
    float reversedAmount = 0.0f;
    float subHarmonicsAmount = 0.0f;

    // Cached sample period (at oversampled rate)
    float dt = 1.0f / 88200.0f;

    float outputGainLinear = 1.0f;
    float currentFrequency = 440.0f;
    float panL = 0.707f;
    float panR = 0.707f;

    // MPE modulation state
    float mpeExpression = 1.0f;    // CC11 (0-1)
    float bowNoiseAmount = 0.0f;

    // Effective values (knob + MPE modulation)
    float effectivePressure = 0.5f;
    float effectiveSpeed = 0.2f;
    float effectivePosition = 0.12f;

    int voiceIndex = 0;
};
