/*
  ==============================================================================

    WavetableOscillator.h
    O-Prism - Microtonal Wavetable Synthesizer
    Ouaricon Audio

  ==============================================================================
*/

#pragma once
#include "WavetableData.h"
#include <cmath>

class WavetableOscillator
{
public:
    WavetableOscillator() = default;

    void prepare (double sampleRate);
    void setFrequency (double freq);
    void setPosition (float pos);
    void setWavetable (const WavetableData* table);
    void reset();
    void resetWithPhase (double phase);

    double getNextSample();
    void getNextSampleStereo (double& outL, double& outR);

    // Unison
    void setUnison (int count, float detune, float width);
    void resetWithRandomPhases();

    double getFrequency() const { return frequency; }

private:
    const WavetableData* wavetable = nullptr;
    double currentSampleRate = 44100.0;
    double frequency = 440.0;
    double phaseIncrement = 0.0;
    float position = 0.0f; // Frame position 0-1

    // Phase accumulator(s) for unison
    static constexpr int kMaxUnison = 8;
    double phaseAccumulators[kMaxUnison] = {};
    double unisonDetuneFactors[kMaxUnison] = { 1.0 };
    double unisonPanL[kMaxUnison] = { 1.0 };
    double unisonPanR[kMaxUnison] = { 1.0 };
    double unisonGain = 1.0;
    int unisonCount = 1;

    double readSample (double phase) const;
};
