/*
  ==============================================================================

    ModeBank.cpp
    Modal Synthesis Bassoon - 16-mode parallel pole-only resonator bank
    Ouaricon Audio
    Developer: Taylor Brook

  ==============================================================================
*/

#include "ModeBank.h"

void ModeBank::prepare (double sampleRate)
{
    currentSampleRate = sampleRate;
    reset();
}

void ModeBank::setFundamental (float f0)
{
    const float fs       = static_cast<float> (currentSampleRate);
    const float nyqLimit = NYQ_RATIO * fs;

    for (int k = 0; k < NUM_MODES; ++k)
    {
        const float f_k = f0 * PARTIAL_RATIOS[static_cast<size_t> (k)];

        if (f_k > nyqLimit || f_k <= 0.0f)
        {
            // Mute: zero gain, but keep stable pole-pair coefficients.
            modes[static_cast<size_t> (k)].b0 = 0.0f;
            modes[static_cast<size_t> (k)].a1 = 0.0f;
            modes[static_cast<size_t> (k)].a2 = 0.0f;
            continue;
        }

        // Phase 2.1 placeholder amplitude: flat (Phase 2.2 replaces with formant-Gaussian).
        const float amp = 1.0f;

        const float theta = juce::MathConstants<float>::twoPi * f_k / fs;
        const float tau   = BASE_T60[static_cast<size_t> (k)] / 6.91f;
        const float R     = std::exp (-1.0f / (tau * fs));

        modes[static_cast<size_t> (k)].b0 = (1.0f - R) * amp;
        modes[static_cast<size_t> (k)].a1 = -2.0f * R * std::cos (theta);
        modes[static_cast<size_t> (k)].a2 = R * R;
    }
}

float ModeBank::processSample (float excitation) noexcept
{
    float sum = 0.0f;
    for (auto& m : modes)
        sum += m.processSample (excitation);

    // Per-voice headroom: 16 modes summed at unity gain can peak near +24 dB.
    // Phase 2.1 attenuates to keep within [-1, 1] without parameter dependency.
    // Phase 2.3 replaces with proper output_gain APVTS read.
    return sum * (1.0f / static_cast<float> (NUM_MODES));
}

void ModeBank::reset() noexcept
{
    for (auto& m : modes)
        m.reset();
}
