/*
  ==============================================================================

    EnvelopeFollower.cpp
    O-Bass - Dual-Coefficient IIR Envelope Follower

  ==============================================================================
*/

#include "EnvelopeFollower.h"

void EnvelopeFollower::prepare(double sampleRate)
{
    currentSampleRate = sampleRate;
    updateCoefficients();
    reset();
}

void EnvelopeFollower::reset()
{
    envelope = 0.0f;
}

void EnvelopeFollower::setAttackMs(float ms)
{
    attackMs = juce::jmax(0.01f, ms);  // Minimum 0.01ms to avoid clicks
    updateCoefficients();
}

void EnvelopeFollower::setReleaseMs(float ms)
{
    releaseMs = juce::jmax(0.01f, ms);  // Minimum 0.01ms
    updateCoefficients();
}

void EnvelopeFollower::updateCoefficients()
{
    // Coefficient formula: time constant to fall from 100% to 1%
    // coef = exp(log(0.01) / (timeMs * 0.001 * sampleRate))
    // This gives us the "one-pole" filter coefficient for the desired time constant

    const double logTarget = std::log(0.01);  // -4.605...

    // Attack coefficient (fast when signal rises)
    double attackSamples = (attackMs * 0.001) * currentSampleRate;
    attackCoef = static_cast<float>(std::exp(logTarget / attackSamples));

    // Release coefficient (slow when signal falls)
    double releaseSamples = (releaseMs * 0.001) * currentSampleRate;
    releaseCoef = static_cast<float>(std::exp(logTarget / releaseSamples));
}

float EnvelopeFollower::process(float input)
{
    // Rectify input to get absolute value
    float absInput = std::abs(input);

    // Dual-coefficient tracking:
    // - Use attack coefficient when signal is rising (fast response to transients)
    // - Use release coefficient when signal is falling (smooth decay)
    if (absInput > envelope)
    {
        // Attack phase: envelope rises toward input
        envelope = attackCoef * (envelope - absInput) + absInput;
    }
    else
    {
        // Release phase: envelope falls toward input
        envelope = releaseCoef * (envelope - absInput) + absInput;
    }

    return envelope;
}
