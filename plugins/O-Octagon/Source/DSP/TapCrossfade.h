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

namespace oo
{

/**
    v1.15.0 (B9, B13) — a two-tap crossfade for a delay read position, ONE CHANGE IN FLIGHT.

    Moving a delay tap has two classic wrong answers. Stepping it is a raw discontinuity (the
    decorrelator's integer taps, B9: 28x). Sliding it resamples the programme — a tap swept from
    5 to 50 ms over 5 ms plays the line backwards at −8x (the alignment delay, B13). This reads the
    OLD and the NEW position side by side and crossfades between them, so the programme is never
    resampled and never steps.

    ONE CHANGE IN FLIGHT: a new wanted position that arrives mid-fade waits until the fade ends,
    then starts its own from wherever the last one landed. A tap that retargeted mid-fade would
    need three reads, or would jump the `from` side — a step by another name. During a ramp the
    tap therefore chases the target in fade-length strides and lands on it one fade after the
    ramp stops.

    THE CURVE is smoothstep (3t² − 2t³): equal-GAIN, because the two taps carry the same programme
    and are partly correlated, and C1, so the fade adds no corner of its own. Its last sample is
    t = 1, where smoothstep is exactly 1.0f, and the first sample after it reads `to` alone.

    SETTLED, next() returns exactly 1.0f and callers read `to` alone — the pre-v1.15.0 expression,
    so a static setting is bit-identical to the code this replaced.

    A FUNCTION OF THE SAMPLES IT HAS SEEN, NOT OF THE BLOCK: advanced exactly once per sample by
    the caller, so two renders that chop the host buffer differently fade identically (QUAL-03).

    No includes: Decorrelator.h is compiled into the unit target, whose narrow link line gate 11
    checks.
*/
template <typename T>
struct TapCrossfade
{
    T    from {}, to {};
    int  remaining { 0 };
    int  length { 1 };
    bool primed { false };

    void setLength (int samples) noexcept { length = samples > 0 ? samples : 1; }

    /// Teleport: no fade, both taps at `value`.
    void snap (T value) noexcept
    {
        from = to = value;
        remaining = 0;
        primed = true;
    }

    /// Forget the position; the next next() snaps to whatever is wanted then.
    void unprime() noexcept { primed = false; remaining = 0; }

    bool isFading() const noexcept { return remaining > 0; }

    /** Advance one sample toward `wanted`. Returns the weight of `to` (1.0f when settled); the
        caller reads `from + w·(to − from)`, or `to` alone at exactly 1. */
    float next (T wanted) noexcept
    {
        if (! primed)
            snap (wanted);

        if (remaining == 0)
        {
            // Ordering, not ==: the repo's zero-warning gate builds with -Wfloat-equal, and this
            // template is instantiated on float. Same answer for every finite value.
            if (! (wanted < to) && ! (to < wanted))
                return 1.0f;

            from      = to;
            to        = wanted;
            remaining = length;
        }

        const float t = static_cast<float> (length - remaining + 1) / static_cast<float> (length);
        --remaining;

        return t * t * (3.0f - 2.0f * t);
    }
};

} // namespace oo
