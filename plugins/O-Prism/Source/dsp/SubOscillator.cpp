/*
  ==============================================================================

    SubOscillator.cpp
    O-Prism - Microtonal Wavetable Synthesizer
    Ouaricon Audio

  ==============================================================================
*/

#include "SubOscillator.h"

static constexpr double kTwoPi = 6.283185307179586;

void SubOscillator::prepare (double sampleRate)
{
    currentSampleRate = sampleRate;
    phaseIncrement = baseFrequency * std::pow (2.0, octaveOffset) / currentSampleRate;
}

void SubOscillator::reset()
{
    phase = 0.0;
    triState = 0.0;
}

void SubOscillator::setFrequency (double freq)
{
    baseFrequency = freq;
    phaseIncrement = baseFrequency * std::pow (2.0, octaveOffset) / currentSampleRate;
}

void SubOscillator::setShape (int shape)
{
    currentShape = shape;
}

void SubOscillator::setOctave (int octave)
{
    octaveOffset = octave;
    phaseIncrement = baseFrequency * std::pow (2.0, octaveOffset) / currentSampleRate;
}

double SubOscillator::getNextSample()
{
    double output = 0.0;
    double dt = phaseIncrement;

    switch (currentShape)
    {
        case 0: // Sine
            output = std::sin (phase * kTwoPi);
            break;

        case 1: // Triangle (leaky-integrated polyBLEP square)
        {
            double sq = (phase < 0.5) ? 1.0 : -1.0;
            sq += polyBLEP (phase, dt);
            sq -= polyBLEP (std::fmod (phase + 0.5, 1.0), dt);
            // Leaky integrate
            triState = dt * sq + (1.0 - dt) * triState;
            output = triState * 4.0; // Scale
            break;
        }

        case 2: // Saw
            output = 2.0 * phase - 1.0;
            output -= polyBLEP (phase, dt);
            break;

        case 3: // Square
            output = (phase < 0.5) ? 1.0 : -1.0;
            output += polyBLEP (phase, dt);
            output -= polyBLEP (std::fmod (phase + 0.5, 1.0), dt);
            break;
    }

    phase += phaseIncrement;
    if (phase >= 1.0)
        phase -= 1.0;

    return output;
}
