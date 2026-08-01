/*
   This file is part of O-MicrotonalSampler, an Ouaricon Audio plugin.
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

    LoaderSupport.h
    Microtonal Sample Engine - pure loader/grouping helpers (test-friendly)
    Ouaricon Audio
    Developer: Taylor Brook

    v1.23.6 — extracted so the untrusted-header length validation, the resampler
    output-length math, and the round-robin ambiguity decision can be pinned by a
    console regression test (loader_robustness_check.cpp) without dragging in
    juce::Thread / the whole plugin. Granular juce_core include only — mirrors the
    DropRouting.h extraction from v1.23.5.

  ==============================================================================
*/

#pragma once
#include <juce_core/juce_core.h>
#include <algorithm>
#include <cmath>
#include <vector>

namespace oms
{
    // v1.23.6 (WR-01/WR-02): upper bound on a single decoded file's per-channel
    // sample count. Rejects a corrupt/absurd header BEFORE the proportional
    // AudioBuffer allocation — which would otherwise std::bad_alloc and, under
    // JUCE's Release catch(...) in threadEntryPoint (Threads.cpp:108-114), be
    // silently swallowed: the folder worker exits mid-batch, no completion/
    // failure callback fires, and every already-decoded sample is discarded.
    // ~268M samples/ch ≈ 46 min @ 96 kHz mono / ~93 min @ 48 kHz — far beyond any
    // real single instrument sample, while capping the worst-case source
    // allocation to a few GB that the per-file try/catch can still absorb.
    inline constexpr juce::int64 kMaxSamplesPerFile = 1 << 28;

    // v1.23.6 (WR-02): validate an untrusted int64 header length against the
    // ceiling BEFORE narrowing to int. A bare `(int) reader->lengthInSamples`
    // wraps a >2^31-sample length to a small positive (which passes the
    // `srcSamples <= 0` guard → silent under-read) or to a negative (false
    // "invalid header" reject of a real file). Keep it int64 until proven sane.
    inline bool isAcceptableSampleLength (juce::int64 lengthInSamples) noexcept
    {
        return lengthInSamples > 0 && lengthInSamples <= kMaxSamplesPerFile;
    }

    // v1.23.6 (WR-03 / IN-05): resampler output length. Keeps ceil (no tail
    // truncation); the caller pads the SOURCE buffer with one cleared guard
    // sample so the LagrangeInterpolator's look-ahead — which consumes up to
    // ~srcRatio*out samples, exceeding srcSamples by ~1 on any odd-length SR
    // conversion (e.g. 44.1→48 kHz) — never reads past the decoded region.
    // Callers guarantee srcSamples >= 1 and srcRatio > 0, so the result is always
    // >= 1: the previous `if (outNumSamples < 1) outNumSamples = 1;` clamp was
    // dead code (IN-05).
    inline int resampleOutLength (int srcSamples, double srcRatio) noexcept
    {
        return (int) std::ceil ((double) srcSamples / srcRatio);
    }

    // v1.23.6 (IN-02): decide whether a load group sharing one (midi, layer,
    // technique) cell is AMBIGUOUS and needs the duplicate-confirmation modal.
    // `rrIndices` holds each grouped file's resolved round-robin index (>= 0 for
    // an explicit rr/take/tk token, -1 for none). A single-file group is never
    // ambiguous. A multi-file group is ambiguous when ANY of:
    //   * no file carries an explicit RR token (the original v1.8.0 rule), OR
    //   * the group MIXES explicit-RR and no-token files — a likely accidental
    //     duplicate sitting next to an intentional RR set, OR
    //   * two explicit files resolve to the SAME RR index (collision).
    // A clean set of DISTINCT explicit RR indices (rr1, rr2, rr3…) is NOT
    // ambiguous — that is an intentional round-robin set and merges silently.
    inline bool isAmbiguousRrGroup (const std::vector<int>& rrIndices) noexcept
    {
        if ((int) rrIndices.size() <= 1)
            return false;

        int  explicitCount = 0;
        int  implicitCount = 0;
        bool duplicateExplicit = false;
        std::vector<int> seen;
        seen.reserve (rrIndices.size());

        for (const int rr : rrIndices)
        {
            if (rr < 0)
            {
                ++implicitCount;
                continue;
            }

            ++explicitCount;
            if (std::find (seen.begin(), seen.end(), rr) != seen.end())
                duplicateExplicit = true;
            else
                seen.push_back (rr);
        }

        const bool mixed = (explicitCount > 0 && implicitCount > 0);
        return explicitCount == 0 || mixed || duplicateExplicit;
    }
}
