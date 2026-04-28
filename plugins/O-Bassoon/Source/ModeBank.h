/*
  ==============================================================================

    ModeBank.h
    Modal Synthesis Bassoon - 16-mode parallel pole-only resonator bank
    Ouaricon Audio
    Developer: Taylor Brook

    Phase 2.1: integer harmonics, flat amplitudes (placeholders).
    Phase 2.2 will replace PARTIAL_RATIOS with bassoon-tuned ratios + per-mode
    formant-Gaussian amplitude shaping wired to the 'tone' parameter.

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <array>
#include <cmath>

class ModeBank
{
public:
    static constexpr int NUM_MODES = 16;

    // Phase 2.1: integer harmonics, flat amplitudes (placeholders).
    // Phase 2.2 replaces with bassoon-tuned ratios + formant-Gaussian × 1/k roll-off.
    static constexpr std::array<float, NUM_MODES> PARTIAL_RATIOS = {
        1.0f,  2.0f,  3.0f,  4.0f,  5.0f,  6.0f,  7.0f,  8.0f,
        9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f
    };

    static constexpr std::array<float, NUM_MODES> BASE_T60 = {
        2.5f, 2.2f, 2.0f, 1.8f, 1.6f, 1.4f, 1.2f, 1.0f,
        0.8f, 0.7f, 0.6f, 0.5f, 0.4f, 0.35f, 0.30f, 0.25f
    };

    void  prepare        (double sampleRate);
    void  setFundamental (float f0);
    float processSample  (float excitation) noexcept;
    void  reset          () noexcept;

    // Phase 2.1 stub — wired live in Phase 2.2 (formant-Gaussian amplitude shaping).
    void setTone (float /*tone01*/) noexcept {}

private:
    struct ModeBiquad
    {
        float b0 = 0.0f, a1 = 0.0f, a2 = 0.0f;
        float y1 = 0.0f, y2 = 0.0f;

        inline float processSample (float x) noexcept
        {
            const float y0 = b0 * x - a1 * y1 - a2 * y2;
            y2 = y1;
            y1 = y0;

            // O-Formant-style NaN/Inf guard (FormantBiquad.h:34-39).
            if (! std::isfinite (y1) || ! std::isfinite (y2))
            {
                y1 = 0.0f;
                y2 = 0.0f;
                return 0.0f;
            }
            return y0;
        }

        void reset() noexcept { y1 = 0.0f; y2 = 0.0f; }
    };

    static constexpr float NYQ_RATIO = 0.45f;

    std::array<ModeBiquad, NUM_MODES> modes {};
    double currentSampleRate = 48000.0;
};
