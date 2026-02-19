/*
  ==============================================================================

    NoiseGenerator.cpp
    O-Prism - Microtonal Wavetable Synthesizer
    Ouaricon Audio

  ==============================================================================
*/

#include "NoiseGenerator.h"
#include <cmath>

static constexpr double kTwoPi = 6.283185307179586;

void NoiseGenerator::prepare (double sampleRate)
{
    currentSampleRate = sampleRate;
    digitalHoldSamples = std::max (1, static_cast<int> (sampleRate / 5512.0));
}

void NoiseGenerator::reset()
{
    b0 = b1 = b2 = 0.0;
    brownState = 0.0;
    digitalHoldValue = 0.0;
    digitalCounter = 0;
    vinylBP1 = vinylBP2 = 0.0;
    crackleDecay = 0.0;
    windLFOPhase = 0.0;
    windLPState = 0.0;
    windBrownState = 0.0;
}

void NoiseGenerator::setType (int type)
{
    currentType = type;
}

double NoiseGenerator::getNextSample()
{
    double white = random.nextDouble() * 2.0 - 1.0;

    switch (currentType)
    {
        case 0: // White
            return white * 0.3;

        case 1: // Pink (Paul Kellet economy)
        {
            b0 = 0.99765 * b0 + white * 0.0990460;
            b1 = 0.96300 * b1 + white * 0.2965164;
            b2 = 0.57000 * b2 + white * 1.0526913;
            return (b0 + b1 + b2 + white * 0.1848) * 0.11;
        }

        case 2: // Brown
        {
            double rateScale = 44100.0 / currentSampleRate;
            brownState += white * 0.02 * rateScale;
            brownState *= 0.998;
            return std::tanh (brownState * 2.0) * 0.35;
        }

        case 3: // Digital (sample-and-hold, quantized)
        {
            if (++digitalCounter >= digitalHoldSamples)
            {
                digitalCounter = 0;
                // Quantize to 8 levels
                double raw = random.nextDouble();
                digitalHoldValue = (std::floor (raw * 8.0) / 7.0) * 2.0 - 1.0;
            }
            return digitalHoldValue * 0.3;
        }

        case 4: // Vinyl (bandpass white + crackle)
        {
            // Simple bandpass via state-variable style (200-5kHz)
            double cutNorm = 2000.0 / currentSampleRate;
            double fb = 0.5;
            vinylBP1 += cutNorm * (white - vinylBP1 - fb * vinylBP2);
            vinylBP2 += cutNorm * vinylBP1;
            double vinyl = vinylBP2 * 1.2;

            // Poisson crackle (~3 per second)
            if (random.nextDouble() < 3.0 / currentSampleRate)
                crackleDecay = (random.nextDouble() * 0.3 + 0.3) * (random.nextBool() ? 1.0 : -1.0);
            crackleDecay *= 0.95;
            vinyl += crackleDecay;

            return std::tanh (vinyl) * 0.35;
        }

        case 5: // Wind (LFO-modulated lowpass on brown)
        {
            // Brown noise source
            double rateScale = 44100.0 / currentSampleRate;
            windBrownState += white * 0.02 * rateScale;
            windBrownState *= 0.998;

            // LFO at 0.2Hz modulates cutoff 50-550Hz
            windLFOPhase += 0.2 / currentSampleRate;
            if (windLFOPhase >= 1.0) windLFOPhase -= 1.0;
            double lfoVal = (std::sin (windLFOPhase * kTwoPi) + 1.0) * 0.5; // 0-1
            double cutoff = 50.0 + lfoVal * 500.0;
            double alpha = cutoff / (cutoff + currentSampleRate / kTwoPi);

            windLPState += alpha * (windBrownState - windLPState);
            return std::tanh (windLPState * 2.0) * 0.35;
        }
    }

    return white * 0.3;
}
