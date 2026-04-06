/*
  ==============================================================================

    GlottalTableGenerator.h
    O-Formant - Physical Model Vocal Synthesizer
    Ouaricon Audio
    Developer: Taylor Brook

    Offline generation of LF glottal pulse wavetable with FFT-based mipmaps.
    Runs once at plugin init. Not real-time.

  ==============================================================================
*/

#pragma once
#include "GlottalWavetable.h"

class GlottalTableGenerator
{
public:
    // Generate full wavetable: 128 Rd steps x 10 mipmap levels
    // Call once at plugin construction. NOT real-time safe.
    static void generate (GlottalWavetable& table);

private:
    // Fant 1995 regression: Rd -> R-parameters -> timing
    struct LFTimingParams
    {
        float Tp;   // Time of max flow derivative (normalized to period = 1.0)
        float Te;   // Time of excitation (glottal closure)
        float Ta;   // Return phase time constant
        float Tc;   // Full period (always 1.0)
    };

    static LFTimingParams computeTimingFromRd (float Rd);

    // Render one period of LF derivative waveform into buffer
    static void renderLFPeriod (float* buffer, int size, const LFTimingParams& params);

    // FFT-based mipmap generation (adapted from O-Prism)
    static void generateMipmaps (GlottalWavetable& table);

    // Newton-Raphson solvers (offline only)
    static float solveAlpha (float Tp, float Te);
    static float solveEpsilon (float Ta, float Te, float Tc);
};
