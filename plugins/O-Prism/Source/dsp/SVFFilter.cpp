/*
  ==============================================================================

    SVFFilter.cpp
    O-Prism - Microtonal Wavetable Synthesizer
    Ouaricon Audio

  ==============================================================================
*/

#include "SVFFilter.h"
#include "MathConstants.h"
#include <algorithm>

void SVFFilter::prepare (double sampleRate)
{
    currentSampleRate = sampleRate;
    updateCoefficients();
    coeffsDirty = false;
}

void SVFFilter::reset()
{
    ic1eq_1 = ic2eq_1 = 0.0;
    ic1eq_2 = ic2eq_2 = 0.0;
}

void SVFFilter::setType (int type)
{
    filterType = type;
}

void SVFFilter::setCutoff (double hz)
{
    if (cutoffHz != hz)
    {
        cutoffHz = hz;
        coeffsDirty = true;
    }
}

void SVFFilter::setResonance (double res)
{
    if (resonance != res)
    {
        resonance = res;
        coeffsDirty = true;
    }
}

void SVFFilter::setDrive (double drive)
{
    driveAmount = drive;
}

void SVFFilter::setKeyTrack (double amount, int midiNote)
{
    double keyTrackOffset = amount * (midiNote - 60);
    double tracked = cutoffHz * std::pow (2.0, keyTrackOffset / 12.0);
    double newCutoff = std::max (20.0, std::min (20000.0, tracked));

    if (cutoffHz != newCutoff)
    {
        cutoffHz = newCutoff;
        coeffsDirty = true;
    }
}

void SVFFilter::updateCoefficients()
{
    double fc = std::max (20.0, std::min (20000.0, cutoffHz));
    g = std::tan (kPi * fc / currentSampleRate);

    // Map user resonance (0-1) to SVF inverse Q
    // resonance=0 -> R2=2 (Butterworth), resonance=1 -> R2~0.1 (self-osc)
    double svfRes = 1.0 / (1.0 + resonance * 19.0);
    R2 = 2.0 * svfRes;
}

double SVFFilter::processSingleSVF (double input, double& s1, double& s2)
{
    double h = 1.0 / (1.0 + R2 * g + g * g);
    double yHP = h * (input - s1 * (R2 + g) - s2);
    double yBP = yHP * g + s1;
    s1 = yHP * g + yBP;
    double yLP = yBP * g + s2;
    s2 = yBP * g + yLP;

    switch (filterType)
    {
        case 0: return yLP;  // LP12
        case 1: return yLP;  // LP24 (first stage)
        case 2: return yHP;  // HP12
        case 3: return yHP;  // HP24 (first stage)
        case 4: return yBP;  // BP12
        case 5: return yBP;  // BP24 (first stage)
        default: return yLP;
    }
}

double SVFFilter::processNotch (double input, double& s1, double& s2)
{
    double h = 1.0 / (1.0 + R2 * g + g * g);
    double yHP = h * (input - s1 * (R2 + g) - s2);
    double yBP = yHP * g + s1;
    s1 = yHP * g + yBP;
    double yLP = yBP * g + s2;
    s2 = yBP * g + yLP;
    return yLP + yHP; // Notch = LP + HP
}

double SVFFilter::processSample (double input)
{
    if (coeffsDirty)
    {
        updateCoefficients();
        coeffsDirty = false;
    }

    // Pre-filter drive
    if (driveAmount > 0.0)
        input = std::tanh (input * (1.0 + driveAmount * 9.0));

    // Notch mode
    if (filterType == 6)
        return processNotch (input, ic1eq_1, ic2eq_1);

    // 12dB modes (single stage)
    if (filterType == 0 || filterType == 2 || filterType == 4)
        return processSingleSVF (input, ic1eq_1, ic2eq_1);

    // 24dB modes (cascaded: two stages, resonance on first only)
    double stage1 = processSingleSVF (input, ic1eq_1, ic2eq_1);

    // Second stage with no resonance (R2 = sqrt(2) for Butterworth)
    double h = 1.0 / (1.0 + 2.0 * 0.707 * g + g * g);
    double yHP = h * (stage1 - ic1eq_2 * (2.0 * 0.707 + g) - ic2eq_2);
    double yBP = yHP * g + ic1eq_2;
    ic1eq_2 = yHP * g + yBP;
    double yLP = yBP * g + ic2eq_2;
    ic2eq_2 = yBP * g + yLP;

    switch (filterType)
    {
        case 1: return yLP;  // LP24
        case 3: return yHP;  // HP24
        case 5: return yBP;  // BP24
        default: return stage1;
    }
}
