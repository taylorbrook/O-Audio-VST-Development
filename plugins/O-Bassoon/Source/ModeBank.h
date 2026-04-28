/*
  ==============================================================================

    ModeBank.h
    Modal Synthesis Bassoon - 16-mode parallel pole-only resonator bank
    Ouaricon Audio
    Developer: Taylor Brook

    Phase 2.2: bassoon-tuned near-integer partial ratios + first-formant-Gaussian
    × 1/k roll-off amplitude shaping. `tone` parameter is wired live, scaling
    upper-mode T60 (k > 4, zero-indexed → modes 5–15) via mix(0.3, 1.5, tone).

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <array>
#include <cmath>

class ModeBank
{
public:
    static constexpr int   NUM_MODES   = 16;
    static constexpr float FORMANT_F1  = 475.0f;   // Hz, first formant centre
    static constexpr float FORMANT_BW  = 200.0f;   // Hz, formant bandwidth

    // rev-2: bassoon-tuned near-integer ratios; source ARCHITECTURE.md §Bassoon
    // Partial Table — author-curated synthesis per RESEARCH §1 OQ#3-rev-2.
    static constexpr std::array<float, NUM_MODES> PARTIAL_RATIOS = {
         1.000f,  2.005f,  3.010f,  4.018f,  5.024f,  6.032f,  7.041f,  8.052f,
         9.064f, 10.078f, 11.092f, 12.108f, 13.125f, 14.144f, 15.164f, 16.186f
    };

    static constexpr std::array<float, NUM_MODES> BASE_T60 = {
        2.5f, 2.2f, 2.0f, 1.8f, 1.6f, 1.4f, 1.2f, 1.0f,
        0.8f, 0.7f, 0.6f, 0.5f, 0.4f, 0.35f, 0.30f, 0.25f
    };

    void  prepare        (double sampleRate);
    void  setFundamental (float f0);
    float processSample  (float excitation) noexcept;
    void  reset          () noexcept;

    // Phase 2.2: wired live. setTone caches the latest value; applyToneChange
    // recomputes coefficients for upper modes (k > 4) only.
    void setTone         (float tone01) noexcept;
    void applyToneChange () noexcept;

    // rev-3 (Phase 2.2): inject modal energy at note-on. Sets each mode's state
    // to launch the canonical y[n] = amp·sin((n+1)θ)·R^n free-decay sinusoid
    // (peak amplitude = amp, decay at R^n matching T60). Without this, the
    // pole-only resonator's IR peak is only b0/sin(θ) ≈ (1-R)·amp/sin(θ),
    // which is ~50 dB below amp for high-Q low-frequency modes — inaudible
    // sustain. Called from BassoonVoice::startNote after setFundamental.
    void strike () noexcept;

private:
    static float computeModeAmplitude (int k, float f0) noexcept;

    struct ModeBiquad
    {
        float b0 = 0.0f, a1 = 0.0f, a2 = 0.0f;
        float y1 = 0.0f, y2 = 0.0f;

        // rev-2: cached per-mode state for tone-only recompute (avoids cos() in
        // applyToneChange; preserves headroom from setFundamental).
        float cosTheta = 0.0f;   // cos(theta_k) cached at setFundamental
        float sinTheta = 0.0f;   // sin(theta_k) cached at setFundamental (rev-3, for strike())
        float amp      = 0.0f;   // formant-Gaussian × 1/k roll-off cached at setFundamental

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
    float  currentTone       = 0.5f;   // matches APVTS default (D2-rev-2)
};
