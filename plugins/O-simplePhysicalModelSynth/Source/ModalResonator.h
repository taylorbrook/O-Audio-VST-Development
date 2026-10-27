/*
   This file is part of O-simplePhysicalModelSynth, an Ouaricon Audio plugin.
   Copyright (C) 2026  Ouaricon Audio

   SPDX-License-Identifier: AGPL-3.0-or-later

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
/*
  ==============================================================================

    O-simplePhysicalModelSynth - ModalResonator

    8 parallel RBJ constant-skirt bandpass biquads fed the SAME per-sample
    excitation e[n] the String path uses, so any exciter drives it (cross-driving,
    FUNC-04) and the modes sustain under continuous (Bow) drive / ring-and-decay
    under a burst (Pluck/Strike). Feed-forward — no feedback loop, no stability risk.

    Built new (RESEARCH §3 / F3 — O-Bells is triggered sinusoids, incompatible with
    exciter-driving; O-Bassoon is pole-only and sits ~50 dB low). Constant-skirt RBJ
    gives peak gain ∝ Q, so high-Q modes are intrinsically loud (the antidote to the
    "inaudible sustain"). Reuses O-Bells DATA only (DECAY_MULTIPLIERS, amplitude tilt).

    Authored guards the references lack: per-biquad isfinite reset (O-Bassoon
    ModeBank.h:79-84), Nyquist clamp on f_k.

    v1.5.0: Damping is frequency-dependent loss matched to the String loop LPF
    (same 12 kHz → 1.5 kHz log cutoff, same per-second loss as a C4 KS loop):
    1/T60_k += DAMP_REF_HZ·½·ln(1+(f_k/fc)²)/ln(1000). The old Q ≤ 500 clamp pinned
    every mode from ~C4 up past ~30% Decay, so it is gone — T60 is bounded instead,
    and the biquads run in double so high-Q poles stay accurate.

  ==============================================================================
*/

#pragma once
#include <juce_core/juce_core.h>
#include <array>
#include <cmath>

class ModalResonator
{
public:
    static constexpr int NUM_MODES = 8;

    // Higher modes decay faster (REUSE DATA — O-Bells BellVoice.h:74-75).
    static constexpr float DECAY_MULTIPLIERS[NUM_MODES] =
        { 1.2f, 1.0f, 0.85f, 0.7f, 0.6f, 0.5f, 0.4f, 0.3f };

    static constexpr float B_MAX      = 0.012f;   // Fletcher stretch at inharmonicity=100%
    static constexpr float Q_MAX      = 1.0e5f;   // backstop only — T60 bounds Q (≈65k at 0.45·fs)
    static constexpr float T60_MIN_S  = 0.3f;
    static constexpr float T60_MAX_S  = 6.0f;
    static constexpr float DAMP_REF_HZ = 261.63f; // loss rate of a C4 String loop (Damping parity)

    void prepare (double sr) noexcept
    {
        sampleRate = sr;
        reset();
    }

    void reset() noexcept
    {
        for (auto& m : modes) m.reset();
    }

    // Recompute the bank at block rate. All percent params are 0–100.
    void setParams (float f0, float dampingPct, float decayPct,
                    float inharmonicityPct, float modeBrightnessPct) noexcept
    {
        const float fs       = static_cast<float> (sampleRate);
        const float nyq      = 0.45f * fs;
        const float B        = juce::jlimit (0.0f, 100.0f, inharmonicityPct) / 100.0f * B_MAX;
        const float baseT60  = juce::jmap (juce::jlimit (0.0f, 100.0f, decayPct) / 100.0f, T60_MIN_S, T60_MAX_S);
        const float bright   = juce::jlimit (0.0f, 100.0f, modeBrightnessPct) / 100.0f;
        const float dampFc   = 12000.0f * std::pow (1500.0f / 12000.0f,
                                                    juce::jlimit (0.0f, 100.0f, dampingPct) / 100.0f);

        float ampSum = 0.0f;
        for (int k = 0; k < NUM_MODES; ++k)
        {
            const float kk = static_cast<float> (k + 1);

            // Fletcher inharmonicity stretch: f_k = f0·k·√(1+B·k²) (0% ≈ bar, 100% ≈ bell).
            float fk = f0 * kk * std::sqrt (1.0f + B * kk * kk);
            fk = juce::jlimit (1.0f, nyq, fk);
            modeFreq[(size_t) k] = fk;

            // Damping: the String loop LPF's per-pass loss −ln|H(f_k)| at a C4 pass
            // rate, added as a decay rate — modes near/above fc die fast (darker tail).
            const float fr       = fk / dampFc;
            const float dampRate = DAMP_REF_HZ * 0.5f * std::log1p (fr * fr) / 6.9078f;   // 1/T60
            const float t60      = 1.0f / (1.0f / (baseT60 * DECAY_MULTIPLIERS[k]) + dampRate);

            // T60 → Q (DERIVE — O-Bells stores an env, not a Q): Q = 0.4548·f·T60.
            const float Q   = juce::jlimit (0.5f, Q_MAX, 0.4548f * fk * t60);

            // Amplitude tilt (REUSE FORMULA — O-Bells BellVoice.cpp:744-758): 1/k falloff
            // × brightness scale. 0.5 ≈ flat, <0.5 dark, >0.5 metallic.
            const float baseAmp = 1.0f / (kk);
            const float ratio   = static_cast<float> (k) / static_cast<float> (NUM_MODES);
            const float tilt    = 1.0f + ratio * (1.9f * bright - 0.9f);
            const float amp     = juce::jmax (0.0f, baseAmp * tilt);
            modeAmp[(size_t) k] = amp;
            ampSum += amp;

            // RBJ constant-skirt bandpass (peak gain = Q): b0=−b2=α·.../a0, b1=0.
            const double w0    = juce::MathConstants<double>::twoPi * fk / sampleRate;
            const double cw    = std::cos (w0), sw = std::sin (w0);
            const double alpha = sw / (2.0 * Q);
            const double a0    = 1.0 + alpha;
            modes[(size_t) k].b0 =  (sw * 0.5f) / a0;
            modes[(size_t) k].b1 =  0.0;
            modes[(size_t) k].b2 = -(sw * 0.5f) / a0;
            modes[(size_t) k].a1 =  (-2.0f * cw) / a0;
            modes[(size_t) k].a2 =  (1.0 - alpha) / a0;
        }

        // Global normalization so the summed bank can't run away (RESEARCH §3.1).
        normalization = (ampSum > 1.0e-6f) ? (1.0f / ampSum) : 1.0f;
    }

    float processSample (float e) noexcept
    {
        float y = 0.0f;
        for (int k = 0; k < NUM_MODES; ++k)
            y += modeAmp[(size_t) k] * static_cast<float> (modes[(size_t) k].processSample (e));
        return y * normalization;
    }

    // Viz / test taps — the exact (f_k, amp_k) driving the audio (UI-05).
    const std::array<float, NUM_MODES>& getModeFreqs() const noexcept { return modeFreq; }
    const std::array<float, NUM_MODES>& getModeAmps()  const noexcept { return modeAmp; }

private:
    // Direct-form biquad with isfinite guard (PORT — O-Bassoon ModeBank.h:72-86).
    // Double: at T60 up to 7.2 s the poles sit within ~1e-5 of the unit circle.
    struct ModeBiquad
    {
        double b0 = 0.0, b1 = 0.0, b2 = 0.0, a1 = 0.0, a2 = 0.0;
        double x1 = 0.0, x2 = 0.0, y1 = 0.0, y2 = 0.0;

        inline double processSample (double x) noexcept
        {
            const double y0 = b0 * x + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
            x2 = x1; x1 = x; y2 = y1; y1 = y0;
            if (! std::isfinite (y1) || ! std::isfinite (y2))
            {
                x1 = x2 = y1 = y2 = 0.0;
                return 0.0;
            }
            return y0;
        }
        void reset() noexcept { x1 = x2 = y1 = y2 = 0.0; }
    };

    std::array<ModeBiquad, NUM_MODES> modes {};
    std::array<float, NUM_MODES>      modeFreq {};
    std::array<float, NUM_MODES>      modeAmp {};
    double sampleRate    = 44100.0;
    float  normalization = 1.0f;
};
