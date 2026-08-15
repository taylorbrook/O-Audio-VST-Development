/*
   This file is part of O-Octagon, an Ouaricon Audio plugin.
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
#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#include <cmath>

namespace oo
{

//==============================================================================
/**
    ARCHITECTURE §3.5.1 and §3.5.2 — the two outside-hull arithmetic laws, as stateless free
    functions.

    ── Header-only, and no class (PLAN-2.3 P25) ──────────────────────────────────────────────────
    The VenueGeometry.h precedent (P14) applied a second time: trivial stateless free functions do
    not earn a .cpp, and header-only means ZERO CMake churn — nothing to add to target_sources in
    three CMakeLists. ROADMAP names `HullProcessor.{h,cpp}`; this is a file-shape deviation of the
    same class as P14 and is recorded in SUMMARY-2.3.md.

    ── The FILTER is not here, deliberately ──────────────────────────────────────────────────────
    §3.5.2's juce::dsp::FirstOrderTPTFilter instances live in GainStage, which is where they are
    driven from. This header carries only the two laws — the cutoff CURVE and the trim LAW — which
    are pure arithmetic and therefore testable in the fast unit target with no filter, no
    AudioBuffer and no SmoothedValue. The filter's behaviour AS GainStage DRIVES IT (the skip, the
    re-seed, the NaN guard) is only meaningful in the render harness, so hand-rolling a one-pole
    here would buy nothing and add a numeric surface (P26).

    ── On the JUCE dependency ────────────────────────────────────────────────────────────────────
    D1's constraint is "no juce_dsp, no juce_audio_processors", and that is what is preserved.
    juce::Decibels::decibelsToGain comes from juce_audio_basics, which the unit target already links,
    and it is the SAME function VenueModel::trimLin uses — so the two conversions in FUNC-07's
    product line are one function rather than two, and Q5's bit-transparency proof (written against
    decibelsToGain) transfers verbatim. Writing std::pow (10.0f, dB * 0.05f) by hand to satisfy a
    literal reading of "JUCE-free" would fork the conversion for no gain.
*/
namespace hullproc
{
    /// §3.5.1. The trim cannot attenuate past this, however far outside the hull the source goes.
    inline constexpr float kTrimFloorDb = -24.0f;

    /// §3.5.2 dRef — the distance over which airAmount = 1 drops the cutoff by one octave.
    inline constexpr float kAirRefMetres = 3.0f;

    /// §3.5.2. The cutoff at d_hull = 0, and the anchor the whole musical curve hangs from.
    inline constexpr float kAirCeilingHz = 20000.0f;

    /// §3.5.2. The cutoff floor, reached at airAmount = 1.0 around d_hull = 15.97 m.
    inline constexpr float kAirFloorHz = 500.0f;

    /** RESEARCH-2.3 H4. The ceiling is additionally capped at this fraction of the sample rate.

        NOT defensiveness — CORRECTNESS. juce::dsp::FirstOrderTPTFilter::setCutoffFrequency asserts
        isPositiveAndBelow (fc, sampleRate * 0.5), and past Nyquist tan (pi*fc/fs) goes NEGATIVE, so
        G = g/(1+g) is negative or singular and the one-pole stops being a lowpass at all. A literal
        20 000 therefore crashes in Debug and produces nonsense in Release at 22.05 and 32 kHz —
        rates hosts do offer and pluginval at strictness 10 exercises.
    */
    inline constexpr float kNyquistMargin = 0.45f;

    static_assert (kNyquistMargin < 0.5f,
                   "the margin must be strictly below Nyquist or the JUCE assertion it exists to "
                   "avoid fires anyway, and tan() is singular exactly at 0.5");
    static_assert (kAirFloorHz < kAirCeilingHz,
                   "the floor must sit below the ceiling or the clamp in airCutoffHz() inverts");
    static_assert (kTrimFloorDb < 0.0f,
                   "the hull trim ATTENUATES; a non-negative floor would let it boost");
    static_assert (kAirRefMetres > 0.0f,
                   "dRef divides the exponent: at 0 the curve is a step and at negative it inverts");

    //==========================================================================
    /** §3.5.1 — the outside-hull gain trim.

        `dbToGain (max (-hullAtten * dHull, kTrimFloorDb))`, i.e. linear in dB per metre of hull
        distance, floored so a distant source cannot vanish entirely.

        ── THE UNITY CLAIM, and why it is exact (PLAN-2.3 P33 / RESEARCH-2.3 Q5) ─────────────────
        At `hullAtten * dHull == 0` — either control at zero, or the source inside the hull — the
        argument is `-0.0f`. Then:

          - `-0.0f > -100.0f` is TRUE (IEEE compares -0.0 == 0.0), so decibelsToGain takes its
            std::pow branch rather than returning Type();
          - `-0.0f * 0.05f` is `-0.0f`;
          - `std::pow (10.0f, -0.0f)` is EXACTLY 1.0f — C99 Annex F / IEEE 754 mandate
            `pow (x, +-0) == 1` for every x, including NaN. Not an approximation that rounds well.

        So `v * hullTrimGain (0, d)` is `v * 1.0f`, which is bit-exactly `v` for every finite v,
        both zeros and both infinities. There is no -ffast-math anywhere on this line
        (juce_recommended_config_flags adds -O3 in Release and -g -O0 in Debug, nothing else), so
        IEEE semantics hold and this is a guarantee rather than a rounding accident.

        Probe AV asserts it by memcmp over a swept d, and probe BD supplies the non-vacuity half.
    */
    inline float hullTrimGain (float hullAtten, float dHull) noexcept
    {
        const float attenDb = -(hullAtten * dHull);

        return juce::Decibels::decibelsToGain (attenDb > kTrimFloorDb ? attenDb : kTrimFloorDb);
    }

    //==========================================================================
    /** §3.5.2 — the air-absorption cutoff, with H4's Nyquist-safe bounds.

        `fc = clamp (20000 * 2^(-airAmount * dHull / 3), floor, ceiling)`

        The NUMERATOR stays the literal kAirCeilingHz at every sample rate: the musical curve is
        anchored at 20 kHz and §3.5.2's four-row table re-derives exactly from it (13 348 / 5 946 /
        6 300 / 625 Hz at {0.35, 1.0} x {5, 15} m). Only the CLAMP is rate-aware — at 44.1 kHz the
        ceiling clips 20 000 to 19 845 Hz, a 0.07 dB change at 10 kHz, and it makes the plugin
        correct everywhere rather than correct above 40 kHz.

        The floor takes the ceiling as its own upper bound so the clamp cannot invert at low rates:
        at fs = 22 050 the ceiling is 9 922.5 Hz, still far above 500, but the expression must not
        depend on that remaining true.

        std::exp2 rather than std::pow: it is the operation being asked for, and it deliberately
        does NOT route through dbap::countedPow — probe AE asserts powCalls == 16 EXACTLY and that
        assertion is load-bearing for §3.3.5's budget (P32).
    */
    inline float airCutoffHz (float airAmount, float dHull, double sampleRate) noexcept
    {
        // Named ...Hz rather than `ceiling`/`floor`: a local `floor` shadows std::floor, and this
        // repo builds with juce_recommended_warning_flags and a hard zero-warning gate
        // (critical_paramid_shadows_juce_free_function is the same trap one level down).
        const float ceilingHz = juce::jmin (kAirCeilingHz,
                                            kNyquistMargin * static_cast<float> (sampleRate));
        const float floorHz   = juce::jmin (kAirFloorHz, ceilingHz);

        const float fc = kAirCeilingHz * std::exp2 (-(airAmount * dHull) / kAirRefMetres);

        return juce::jlimit (floorHz, ceilingHz, fc);
    }
}

} // namespace oo
