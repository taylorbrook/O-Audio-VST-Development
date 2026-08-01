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
    ModeBank.h:79-84), Q clamp ≤ 500, Nyquist clamp on f_k.

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
    static constexpr float Q_MAX      = 500.0f;   // authored clamp (references have none)
    static constexpr float T60_MIN_S  = 0.3f;
    static constexpr float T60_MAX_S  = 6.0f;

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
    void setParams (float f0, float decayPct, float inharmonicityPct, float modeBrightnessPct) noexcept
    {
        const float fs       = static_cast<float> (sampleRate);
        const float nyq      = 0.45f * fs;
        const float B        = juce::jlimit (0.0f, 100.0f, inharmonicityPct) / 100.0f * B_MAX;
        const float baseT60  = juce::jmap (juce::jlimit (0.0f, 100.0f, decayPct) / 100.0f, T60_MIN_S, T60_MAX_S);
        const float bright   = juce::jlimit (0.0f, 100.0f, modeBrightnessPct) / 100.0f;

        float ampSum = 0.0f;
        for (int k = 0; k < NUM_MODES; ++k)
        {
            const float kk = static_cast<float> (k + 1);

            // Fletcher inharmonicity stretch: f_k = f0·k·√(1+B·k²) (0% ≈ bar, 100% ≈ bell).
            float fk = f0 * kk * std::sqrt (1.0f + B * kk * kk);
            fk = juce::jlimit (1.0f, nyq, fk);
            modeFreq[(size_t) k] = fk;

            // T60 → Q (DERIVE — O-Bells stores an env, not a Q): Q = 0.4548·f·T60, clamped.
            const float t60 = baseT60 * DECAY_MULTIPLIERS[k];
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
            const float w0    = juce::MathConstants<float>::twoPi * fk / fs;
            const float cw    = std::cos (w0), sw = std::sin (w0);
            const float alpha = sw / (2.0f * Q);
            const float a0    = 1.0f + alpha;
            modes[(size_t) k].b0 =  (sw * 0.5f) / a0;
            modes[(size_t) k].b1 =  0.0f;
            modes[(size_t) k].b2 = -(sw * 0.5f) / a0;
            modes[(size_t) k].a1 =  (-2.0f * cw) / a0;
            modes[(size_t) k].a2 =  (1.0f - alpha) / a0;
        }

        // Global normalization so the summed bank can't run away (RESEARCH §3.1).
        normalization = (ampSum > 1.0e-6f) ? (1.0f / ampSum) : 1.0f;
    }

    float processSample (float e) noexcept
    {
        float y = 0.0f;
        for (int k = 0; k < NUM_MODES; ++k)
            y += modeAmp[(size_t) k] * modes[(size_t) k].processSample (e);
        return y * normalization;
    }

    // Viz / test taps — the exact (f_k, amp_k) driving the audio (UI-05).
    const std::array<float, NUM_MODES>& getModeFreqs() const noexcept { return modeFreq; }
    const std::array<float, NUM_MODES>& getModeAmps()  const noexcept { return modeAmp; }

private:
    // Direct-form biquad with isfinite guard (PORT — O-Bassoon ModeBank.h:72-86).
    struct ModeBiquad
    {
        float b0 = 0.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;
        float x1 = 0.0f, x2 = 0.0f, y1 = 0.0f, y2 = 0.0f;

        inline float processSample (float x) noexcept
        {
            const float y0 = b0 * x + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
            x2 = x1; x1 = x; y2 = y1; y1 = y0;
            if (! std::isfinite (y1) || ! std::isfinite (y2))
            {
                x1 = x2 = y1 = y2 = 0.0f;
                return 0.0f;
            }
            return y0;
        }
        void reset() noexcept { x1 = x2 = y1 = y2 = 0.0f; }
    };

    std::array<ModeBiquad, NUM_MODES> modes {};
    std::array<float, NUM_MODES>      modeFreq {};
    std::array<float, NUM_MODES>      modeAmp {};
    double sampleRate    = 44100.0;
    float  normalization = 1.0f;
};
