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

    // rev-2: tone scale applied to upper modes (k > 4) only — D4-rev-2 lock.
    const float toneScale = juce::jmap (currentTone, 0.0f, 1.0f, 0.3f, 1.5f);

    for (int k = 0; k < NUM_MODES; ++k)
    {
        auto& m = modes[static_cast<size_t> (k)];

        const float f_k = f0 * PARTIAL_RATIOS[static_cast<size_t> (k)];

        if (f_k > nyqLimit || f_k <= 0.0f)
        {
            // Mute path: zero coefs AND cached state so applyToneChange skips
            // muted modes via the m.amp == 0.0f test.
            m.b0       = 0.0f;
            m.a1       = 0.0f;
            m.a2       = 0.0f;
            m.cosTheta = 0.0f;
            m.sinTheta = 0.0f;
            m.amp      = 0.0f;
            continue;
        }

        const float theta = juce::MathConstants<float>::twoPi * f_k / fs;
        const float cosT  = std::cos (theta);
        const float sinT  = std::sin (theta);
        const float amp_k = computeModeAmplitude (k, f0);

        // Modes 0–4 are tone-invariant (D4-rev-2); modes 5–15 scale T60 by tone.
        const float perModeScale = (k > 4) ? toneScale : 1.0f;
        const float tau_k        = (BASE_T60[static_cast<size_t> (k)] * perModeScale) / 6.91f;
        const float R_k          = std::exp (-1.0f / (tau_k * fs));

        m.cosTheta = cosT;
        m.sinTheta = sinT;
        m.amp      = amp_k;

        m.b0 = (1.0f - R_k) * amp_k;
        m.a1 = -2.0f * R_k * cosT;
        m.a2 = R_k * R_k;
    }
}

void ModeBank::setTone (float tone01) noexcept
{
    currentTone = juce::jlimit (0.0f, 1.0f, tone01);
}

void ModeBank::applyToneChange() noexcept
{
    const float fs        = static_cast<float> (currentSampleRate);
    const float toneScale = juce::jmap (currentTone, 0.0f, 1.0f, 0.3f, 1.5f);

    // Modes 0–4 are tone-invariant — DO NOT touch (D4-rev-2 lock).
    for (int k = 5; k < NUM_MODES; ++k)
    {
        auto& m = modes[static_cast<size_t> (k)];

        // Skip muted modes (cached amp == 0 indicates Nyquist-muted at setFundamental).
        if (m.amp == 0.0f)
            continue;

        const float tau_k = (BASE_T60[static_cast<size_t> (k)] * toneScale) / 6.91f;
        const float R_k   = std::exp (-1.0f / (tau_k * fs));

        m.b0 = (1.0f - R_k) * m.amp;
        m.a1 = -2.0f * R_k * m.cosTheta;
        m.a2 = R_k * R_k;
    }
}

float ModeBank::computeModeAmplitude (int k, float f0) noexcept
{
    const float f_k           = f0 * PARTIAL_RATIOS[static_cast<size_t> (k)];
    const float dist          = (f_k - FORMANT_F1) / FORMANT_BW;
    const float formantWeight = std::exp (-0.5f * dist * dist);
    const float rollOff       = 1.0f / (1.0f + 0.5f * static_cast<float> (k));
    return formantWeight * rollOff;
}

float ModeBank::processSample (float excitation) noexcept
{
    float sum = 0.0f;
    for (auto& m : modes)
        sum += m.processSample (excitation);

    // Per-voice headroom. rev-2 (Phase 2.2): relaxed from 1/16 to 1/8 (+6 dB)
    // to compensate for formant-Gaussian + 1/k attenuation vs. Phase 2.1's
    // flat-amplitude baseline (RESEARCH-rev-2 §1 OQ#5-rev-2).
    // Phase 2.3 replaces with proper output_gain APVTS read.
    return sum * (1.0f / 8.0f);
}

void ModeBank::reset() noexcept
{
    for (auto& m : modes)
        m.reset();
}

void ModeBank::strike() noexcept
{
    // rev-3: inject modal energy at note-on. The pole-only IIR
    //   y[n] = b0·x[n] - a1·y[n-1] - a2·y[n-2]
    // with a1 = -2R·cos(θ), a2 = R² supports the homogeneous solution
    //   y[n] = amp·sin((n+1)·θ)·R^n   (peak amplitude amp, decays at R^n).
    // Setting y1 = amp·sin(θ), y2 = 0 launches that free-decay sinusoid;
    // amplitude builds from amp·sin(θ) at sample 0 up to amp at sample
    // n ≈ π/(2θ), then decays at R^n matching T60.
    //
    // Without strike(), the only output is b0·x[n] convolved with the IR —
    // peak ≈ b0/sin(θ) ≈ (1-R)·amp/sin(θ), which for high-Q low-frequency
    // modes (e.g., C3 mode 0: T60 = 2.5s, sin(θ) ≈ 0.017) is ~50 dB below
    // amp. Effectively inaudible after the 5 ms exciter ends.
    for (int k = 0; k < NUM_MODES; ++k)
    {
        auto& m = modes[static_cast<size_t> (k)];

        // Skip muted modes (Nyquist-clipped or out-of-range).
        if (m.amp == 0.0f)
            continue;

        m.y1 = m.amp * m.sinTheta;
        m.y2 = 0.0f;
    }
}
