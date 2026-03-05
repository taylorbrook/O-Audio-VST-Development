/*
  ==============================================================================

    SVFFilter.h
    O-Prism - Microtonal Wavetable Synthesizer
    Ouaricon Audio

  ==============================================================================
*/

#pragma once
#include <cmath>

class SVFFilter
{
public:
    SVFFilter() = default;

    void prepare (double sampleRate);
    void reset();

    void setType (int type);
    void setCutoff (double hz);
    void setResonance (double res);
    void setDrive (double drive);
    void setKeyTrack (double amount, int midiNote);

    double processSample (double input);

private:
    void updateCoefficients();
    double processSingleSVF (double input, double& s1, double& s2);
    double processNotch (double input, double& s1, double& s2);

    double currentSampleRate = 44100.0;
    double cutoffHz = 20000.0;
    double resonance = 0.0;    // User param 0-1
    double driveAmount = 0.0;  // User param 0-1
    int filterType = 0;        // 0=LP12, 1=LP24, 2=HP12, 3=HP24, 4=BP12, 5=BP24, 6=Notch

    // SVF state (stage 1)
    double ic1eq_1 = 0.0, ic2eq_1 = 0.0;
    // SVF state (stage 2, for 24dB modes)
    double ic1eq_2 = 0.0, ic2eq_2 = 0.0;

    // Precomputed coefficients
    double g = 0.0;   // tan(pi * fc / fs)
    double R2 = 0.0;  // 2 * resonance (inverse Q mapping)

    bool coeffsDirty = true;
};
