/*
  ==============================================================================

    DCBlocker.h
    O-Wind - DC Blocking Filter
    Ouaricon Audio
    Developer: Taylor Brook

    Removes DC offset accumulated by jet nonlinearity and feedback loop.
    y[n] = x[n] - x[n-1] + 0.995 * y[n-1]
    Pole at 0.995 = ~7 Hz cutoff at 44.1 kHz.

  ==============================================================================
*/

#pragma once
#include <cmath>

class DCBlocker
{
public:
    float processSample (float input)
    {
        float output = input - xPrev + poleCoeff * yPrev;
        xPrev = input;
        yPrev = output;
        return output;
    }

    void reset()
    {
        xPrev = 0.0f;
        yPrev = 0.0f;
    }

    // Phase delay in samples at the given frequency.
    // Returns negative values (phase advance) at audio frequencies because the
    // highpass zero at z=1 advances phase faster than the pole at z=0.995 lags it.
    // Used for waveguide loop delay compensation.
    float getPhaseDelay (double freq, double sr) const
    {
        if (sr <= 0.0 || freq <= 0.0 || freq >= sr * 0.45)
            return 0.0f;

        double omega = 2.0 * 3.14159265358979323846 * freq / sr;
        double sinW = std::sin (omega);
        double cosW = std::cos (omega);

        // H(z) = (1 - z^{-1}) / (1 - poleCoeff * z^{-1})
        double numPhase = std::atan2 (sinW, 1.0 - cosW);
        double denPhase = std::atan2 (static_cast<double> (poleCoeff) * sinW,
                                       1.0 - static_cast<double> (poleCoeff) * cosW);
        double phase = numPhase - denPhase;

        return static_cast<float> (-phase / omega);
    }

private:
    static constexpr float poleCoeff = 0.995f;
    float xPrev = 0.0f;
    float yPrev = 0.0f;
};
