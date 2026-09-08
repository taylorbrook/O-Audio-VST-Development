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

    /** v1.13.0 — dRef as a FRACTION OF rigScale: the distance over which airAmount = 1 drops the
        cutoff by one octave is 0.2 RMS rig radii (1.59 m on the default rig).

        ── WHY THE DRIVING DISTANCE MOVED OFF THE HULL (the v1.12.0 "Air does nothing" report) ──
        Through v1.12.0 the cutoff was `20000 · 2^(−airAmount · d_hull / 3 m)`, d_hull being the
        distance OUTSIDE the convex hull. MEASURED on the default rig: the puck's whole reachable
        plane is the speaker bounding box, only 5.8 % of which lies outside the octagon (two
        triangles at the rear corners), and the farthest reachable point is 2.14 m out — so the
        cutoff never fell below 16.8 kHz at the default and 12.2 kHz at airAmount = 1, and was
        pinned at 20 kHz everywhere the puck normally lives. The control was inert by geometry.

        Since v1.13.0 the distance is the sub-point's PLANAR DISTANCE FROM THE RIG CENTROID — the
        listener — less a near field (kAirNearFraction), and it continues growing beyond the hull.
        On the default rig at airAmount = 1: 11.8 kHz at 2 m, 4.9 kHz at 4 m, 885 Hz at the rig
        radius, the 500 Hz floor at the far corners; at the 0.35 default 6.7 kHz at the rig radius.
        Scaled with the rig (DSP-08's invariant, as blur is) rather than in metres.
    */
    inline constexpr float kAirRefFraction = 0.2f;

    /** v1.13.0 — the near field, as a fraction of rigScale (0.79 m on the default rig). Inside it
        the filter is SKIPPED exactly as it was inside the hull before, which is what keeps the
        shipping default patch — puck at the bounding-box centre, 0.46 m from the centroid on the
        default rig — bit-transparent (DSP-07/6 preserved, with the boundary moved). */
    inline constexpr float kAirNearFraction = 0.1f;

    /// §3.5.2. The cutoff at d_air = 0, and the anchor the whole musical curve hangs from.
    inline constexpr float kAirCeilingHz = 20000.0f;

    /// §3.5.2. The cutoff floor, reached at airAmount = 1.0 at d_air ≈ 1.06 rig radii.
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
    static_assert (kAirRefFraction > 0.0f,
                   "dRef divides the exponent: at 0 the curve is a step and at negative it inverts");
    static_assert (kAirNearFraction >= 0.0f,
                   "a negative near field would filter AT the centroid and break the default "
                   "patch's bit-transparency");

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
    /** v1.13.0 — the distance that drives the air filter: planar distance of a sub-point from
        the rig centroid, less the near field, floored at 0. EXACTLY 0.0f inside the near field,
        which is the skip condition GainStage tests (`airAmount > 0 && dAir > 0`).

        Planar, like the hull, so srcZ neither darkens nor brightens the source — height has its
        own cue (the z-cue). A degenerate rig (rigScale 0) has a zero-radius near field and the
        raw centroid distance; airCutoffHz() then returns the ceiling, so the pair is harmless.
    */
    inline float airDistanceMetres (float px, float py, float cx, float cy, float rigScale) noexcept
    {
        const float dx = px - cx;
        const float dy = py - cy;
        const float d  = std::sqrt (dx * dx + dy * dy) - kAirNearFraction * rigScale;

        return d > 0.0f ? d : 0.0f;
    }

    /** §3.5.2 — the air-absorption cutoff, with H4's Nyquist-safe bounds.

        `fc = clamp (20000 * 2^(-airAmount * dAir / (kAirRefFraction · rigScale)), floor, ceiling)`
        (v1.13.0: dAir from airDistanceMetres() and dRef scaled with the rig — was d_hull / 3 m)

        The NUMERATOR stays the literal kAirCeilingHz at every sample rate: the musical curve is
        anchored at 20 kHz and probe AU's four-row table re-derives exactly from it (10 905 /
        3 242 / 3 536 / 625 Hz at {0.35, 1.0} x {5, 15 | 10} m on a 10 m rig). Only the CLAMP is
        rate-aware — at 44.1 kHz the
        ceiling clips 20 000 to 19 845 Hz, a 0.07 dB change at 10 kHz, and it makes the plugin
        correct everywhere rather than correct above 40 kHz.

        The floor takes the ceiling as its own upper bound so the clamp cannot invert at low rates:
        at fs = 22 050 the ceiling is 9 922.5 Hz, still far above 500, but the expression must not
        depend on that remaining true.

        std::exp2 rather than std::pow: it is the operation being asked for, and it deliberately
        does NOT route through dbap::countedPow — probe AE asserts powCalls == 16 EXACTLY and that
        assertion is load-bearing for §3.3.5's budget (P32).
    */
    inline float airCutoffHz (float airAmount, float dAir, float rigScale, double sampleRate) noexcept
    {
        // Named ...Hz rather than `ceiling`/`floor`: a local `floor` shadows std::floor, and this
        // repo builds with juce_recommended_warning_flags and a hard zero-warning gate
        // (critical_paramid_shadows_juce_free_function is the same trap one level down).
        const float ceilingHz = juce::jmin (kAirCeilingHz,
                                            kNyquistMargin * static_cast<float> (sampleRate));
        const float floorHz   = juce::jmin (kAirFloorHz, ceilingHz);

        // A degenerate rig has no scale to hang the curve from: return the ceiling rather than
        // divide by zero (0/0 → NaN would sail through jlimit and into setCutoffFrequency).
        const float dRef = kAirRefFraction * rigScale;

        if (! (dRef > 0.0f))
            return ceilingHz;

        const float fc = kAirCeilingHz * std::exp2 (-(airAmount * dAir) / dRef);

        return juce::jlimit (floorHz, ceilingHz, fc);
    }
}

} // namespace oo
