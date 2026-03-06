/*
  ==============================================================================

    LFO.cpp
    O-Prism - Microtonal Wavetable Synthesizer
    Generic, reusable LFO class for per-voice modulation

  ==============================================================================
*/

#include "LFO.h"

void LFO::prepare (double sr)
{
    sampleRate = sr;
    phase = 0.0;
    phaseIncrement = 0.0;
    shHeldValue = 0.0f;
}

void LFO::reset()
{
    phase = 0.0;
    shHeldValue = random.nextFloat() * 2.0f - 1.0f;
}

void LFO::setRate (float hz)
{
    phaseIncrement = static_cast<double> (hz) / sampleRate;
}

void LFO::setShape (Shape shape)
{
    currentShape = shape;
}

float LFO::getNextSample()
{
    float output = 0.0f;

    switch (currentShape)
    {
        case Shape::Sine:
            output = static_cast<float> (std::sin (2.0 * juce::MathConstants<double>::pi * phase));
            break;

        case Shape::Triangle:
            output = static_cast<float> (4.0 * std::abs (phase - 0.5) - 1.0);
            break;

        case Shape::Saw:
            output = static_cast<float> (2.0 * phase - 1.0);
            break;

        case Shape::Square:
            output = phase < 0.5 ? 1.0f : -1.0f;
            break;

        case Shape::SampleAndHold:
            output = shHeldValue;
            break;
    }

    // Advance phase
    phase += phaseIncrement;

    // Wrap and trigger S&H on wrap
    if (phase >= 1.0)
    {
        phase -= 1.0;
        if (currentShape == Shape::SampleAndHold)
            shHeldValue = random.nextFloat() * 2.0f - 1.0f;
    }

    return output;
}
