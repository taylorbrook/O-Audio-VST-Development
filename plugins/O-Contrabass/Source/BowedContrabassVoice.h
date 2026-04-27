/*
  ==============================================================================

    BowedContrabassVoice.h
    O-Contrabass - Single-String E1 MPESynthesiserVoice (Phase 2.1a)
    Ouaricon Audio
    Developer: Taylor Brook

    Modelled on O-Bowed/Source/BowedStringVoice — per-voice 2× oversampler,
    voiceBuffer scratch, WaveguideString + BowModel + HyperbolicFriction
    composition. Phase 2.1a simplifications (locked, NOT deviations):

    - Single E-string only (E1 / MIDI 28). Multi-string voicing is Phase 2.2.
    - BOW_POSITION read but only modulates the friction-junction β-derived
      impedance (no split rails yet — Phase 2.5).
    - No Note Expression consumption (Phase 2.6).
    - No vibrato / detune ramps (Phase 2.2 / 2.3).
    - No body resonator, sub-harmonics, slow-bow LFO, or bow-noise generator
      (Phases 2.4 / 2.5).

  ==============================================================================
*/

#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "DSP/WaveguideString.h"
#include "DSP/BowModel.h"
#include "DSP/HyperbolicFriction.h"

class BowedContrabassVoice : public juce::MPESynthesiserVoice
{
public:
    explicit BowedContrabassVoice (juce::AudioProcessorValueTreeState* apvts);
    ~BowedContrabassVoice() override = default;

    // MPE voice callbacks
    void noteStarted() override;
    void noteStopped (bool allowTailOff) override;
    void notePitchbendChanged() override;
    void notePressureChanged() override;
    void noteTimbreChanged() override;
    void noteKeyStateChanged() override;

    // Lifecycle
    void prepareToPlay (double hostSampleRate, int maxBlockSize);

    // Render
    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer,
                          int startSample, int numSamples) override;

    // Latency reporting (processor reads in prepareToPlay → setLatencySamples)
    float getOversamplingLatency() const noexcept { return oversampling.getLatencyInSamples(); }

    // CC11 expression macro — set by OContrabassMPESynthesiser. Cached now,
    // wired into the bow envelope in Phase 2.6 alongside Note Expression.
    void setExpression (float value) noexcept { mpeExpression = value; }

private:
    void updateParametersFromAPVTS();

    juce::AudioProcessorValueTreeState* parameters = nullptr;

    // DSP composition
    WaveguideString waveguideString;
    BowModel bowModel;
    HyperbolicFriction frictionModel;

    // Per-voice 2× oversampler — RESEARCH §3.1 / §Q4. PolyphaseIIR, max quality,
    // useIntegerLatency=false (default).
    juce::dsp::Oversampling<float> oversampling
    {
        /*numChannels*/ 1,
        /*factor*/      1,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
        /*isMaxQuality*/ true
    };

    // Mono scratch — sized to maxBlockSize × 2 in prepareToPlay so the
    // upsampled AudioBlock fits without further allocation in renderNextBlock.
    juce::AudioBuffer<float> voiceBuffer;

    // Cached host & oversampled block size (for AudioBlock setup)
    int currentMaxBlockSize = 0;

    // Cached per-block state read from APVTS
    float currentFrequency = 41.2f;        // E1 default
    float effectivePosition = 0.10f;
    float outputGainLinear = 1.0f;
    float mpeExpression = 1.0f;
};
