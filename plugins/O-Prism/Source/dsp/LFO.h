/*
  ==============================================================================

    LFO.h
    O-Prism - Microtonal Wavetable Synthesizer
    Generic, reusable LFO class for per-voice modulation

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class LFO
{
public:
    enum class Shape { Sine, Triangle, Saw, Square, SampleAndHold };

    LFO() = default;

    void prepare (double sampleRate);
    void reset();

    void setRate (float hz);
    void setShape (Shape shape);

    /** Returns bipolar output in range [-1, 1] */
    float getNextSample();

private:
    double sampleRate = 44100.0;
    double phase = 0.0;
    double phaseIncrement = 0.0;
    Shape currentShape = Shape::Sine;
    float shHeldValue = 0.0f;
    juce::Random random;
};
