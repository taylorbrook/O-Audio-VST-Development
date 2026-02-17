/*
  ==============================================================================

    SubOscillator.h
    O-Prism - Microtonal Wavetable Synthesizer
    Ouaricon Audio

  ==============================================================================
*/

#pragma once
#include <cmath>

class SubOscillator
{
public:
    SubOscillator() = default;

    void prepare (double sampleRate);
    void reset();
    void setFrequency (double freq);
    void setShape (int shape);
    void setOctave (int octave);
    double getNextSample();

private:
    static inline double polyBLEP (double t, double dt)
    {
        if (t < dt)       { t /= dt; return t + t - t * t - 1.0; }
        if (t > 1.0 - dt) { t = (t - 1.0) / dt; return t * t + t + t + 1.0; }
        return 0.0;
    }

    double currentSampleRate = 44100.0;
    double phase = 0.0;
    double phaseIncrement = 0.0;
    double baseFrequency = 220.0;
    int currentShape = 0;   // 0=Sine, 1=Triangle, 2=Saw, 3=Square
    int octaveOffset = -1;  // -2, -1, or 0

    // Triangle state (leaky integrator of square)
    double triState = 0.0;
};
