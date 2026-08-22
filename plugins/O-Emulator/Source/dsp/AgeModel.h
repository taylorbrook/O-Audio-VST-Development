/*
   This file is part of O-Emulator, an Ouaricon Audio plugin.
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

    O-Emulator — AgeModel (Stage 2, Task 16 — noise bed + hum)

    Hardware-condition bed, HOST rate, wet path only (ARCHITECTURE "Age
    Model"): injected by the engine AFTER the pipeline/crossfader output so
    hiss and hum ride ON TOP of the signal (never dulled by the output LP,
    never doubled during a console crossfade — one bed, one wet path).

      noise: white -> one-pole LP 6 kHz, −78 dB (age just past 5 %) to
             −48 dB (100 %); OFF at age <= 5 % (the 20 ms control smoothing
             ramps the gate edge). The white-fed stage is NORMALIZED for the
             host rate (pattern_noise_bed_level_is_rate_dependent): a one-pole
             LP'd white stream has output variance sigma²·a/(2−a) with
             a = 1−exp(−2π·6k/fs) — rate-dependent — so the white input is
             scaled by sqrt of the 48 kHz reference ratio, computed in
             prepare(). Level calibration folds in the 48 kHz bed RMS
             (0.577 white RMS · sqrt(a48/(2−a48)) ≈ 0.353 -> kNoiseCal).

      hum:   60/120/180 Hz phase-accumulator partials (1 / 0.5 / 0.25,
             RMS-normalized), −80 -> −54 dB across the age range, common to
             both channels (mains is common-mode).

    v1.0.1: the bed is PROGRAM-DEPENDENT — the engine hands processChunk two
    envelope scales (hiss / hum, [0, 1]) that multiply the age-derived gains,
    so the bed breathes with the wet signal (companding-noise behaviour) and
    digital silence stays silent. The engine owns the follower; this class
    stays a pure gain stage over its own generators.

    RNG discipline (pattern_rng_stream_interleave_blocksize / L116): one
    xorshift32 stream PER PURPOSE — noiseL and noiseR here, drift owns its
    own stream in ConsoleEngine — each consumed UNCONDITIONALLY every sample
    from this phase's introduction, so the GENERATORS are a pure function of
    the absolute sample index (block-size invariant). The rendered bed is
    envelope-scaled since v1.0.1, so it no longer cancels in wet-minus-dry
    subtractions — harness probes measure it in a post-burst release window
    instead of under silence.

  ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>   // juce::Decibels
#include <juce_core/juce_core.h>

#include <cmath>
#include <cstdint>

namespace oemu
{

class AgeModel
{
public:
    void prepare (double hostRate)
    {
        humInc = 60.0 / hostRate;

        lpCoeff = (float) (1.0 - std::exp (-juce::MathConstants<double>::twoPi
                                           * 6000.0 / hostRate));

        // Rate-invariant bed level: scale the WHITE input so the LP output
        // variance matches the 48 kHz reference (normalize ONLY the
        // white-fed stage).
        const double aRef = 1.0 - std::exp (-juce::MathConstants<double>::twoPi
                                            * 6000.0 / 48000.0);
        const double vRef = aRef / (2.0 - aRef);
        const double v    = (double) lpCoeff / (2.0 - (double) lpCoeff);
        noiseNorm = (float) std::sqrt (vRef / v);

        reset();
    }

    void reset() noexcept
    {
        // Fixed distinct seeds, re-seeded here and nowhere else.
        noiseStateL = 0x9E3779B9u;
        noiseStateR = 0x7F4A7C15u;
        lpL = lpR = 0.0f;
        humPhase = 0.0;
    }

    /** Adds the bed to one chunk in place. `agePct` is the engine's already
        chunk-rate-smoothed age value; `hissScale` / `humScale` are the
        engine's program-envelope scales in [0, 1] (v1.0.1 — the bed rides
        the signal, silence stays silent). The RNG streams and hum phase
        advance UNCONDITIONALLY regardless of all three. */
    void processChunk (float* l, float* r, int n, float agePct,
                       float hissScale, float humScale) noexcept
    {
        // Gain targets, computed per chunk from the smoothed age (the 20 ms
        // smoothing upstream makes the <=5 % gate edge a ramp, not a step).
        float noiseGain = 0.0f;
        if (agePct > 5.0f)
        {
            const float db = -78.0f + (agePct - 5.0f) * (30.0f / 95.0f);
            noiseGain = juce::Decibels::decibelsToGain (db) * kNoiseCal * hissScale;
        }

        const float humDb = -80.0f + juce::jlimit (0.0f, 100.0f, agePct) * 0.26f;
        const float humGain = (agePct > 5.0f)
                                  ? juce::Decibels::decibelsToGain (humDb) * kHumCal * humScale
                                  : 0.0f;

        for (int i = 0; i < n; ++i)
        {
            // Unconditional stream consumption (L116/L110).
            const float wl = nextWhite (noiseStateL) * noiseNorm;
            const float wr = nextWhite (noiseStateR) * noiseNorm;

            lpL += lpCoeff * (wl - lpL);
            lpR += lpCoeff * (wr - lpR);

            humPhase += humInc;
            if (humPhase >= 1.0)
                humPhase -= 1.0;

            // 60/120/180 Hz from one sin/cos pair:
            //   sin2x = 2·sinx·cosx,  sin3x = sinx·(3 − 4·sin²x)
            const double w = juce::MathConstants<double>::twoPi * humPhase;
            const float s1 = (float) std::sin (w);
            const float c1 = (float) std::cos (w);
            const float s2 = 2.0f * s1 * c1;
            const float s3 = s1 * (3.0f - 4.0f * s1 * s1);
            const float hum = (s1 + 0.5f * s2 + 0.25f * s3);

            l[i] += noiseGain * lpL + humGain * hum;
            r[i] += noiseGain * lpR + humGain * hum;
        }
    }

private:
    /** xorshift32 -> [−1, 1). One stream per purpose, advanced per sample. */
    static float nextWhite (std::uint32_t& state) noexcept
    {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return (float) ((double) state / 2147483648.0 - 1.0);
    }

    /** 1 / (white RMS 0.577 · sqrt(a48/(2−a48)) = 0.353): calibrates the
        noise dB targets to the BED's output RMS at the 48 kHz reference. */
    static constexpr float kNoiseCal = 2.833f;

    /** 1 / RMS of the 1/0.5/0.25 partial stack (sqrt(0.65625) = 0.810). */
    static constexpr float kHumCal = 1.234f;

    std::uint32_t noiseStateL = 0x9E3779B9u;
    std::uint32_t noiseStateR = 0x7F4A7C15u;
    float lpL = 0.0f, lpR = 0.0f;
    float lpCoeff = 0.5f;
    float noiseNorm = 1.0f;
    double humPhase = 0.0;
    double humInc = 60.0 / 48000.0;
};

} // namespace oemu
