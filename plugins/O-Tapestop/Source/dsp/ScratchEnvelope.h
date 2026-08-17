/*
   This file is part of O-Tapestop, an Ouaricon Audio plugin.
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

    O-Tapestop - ScratchEnvelope (Stage-2 PLAN Task 12)

    User-drawn bipolar speed-vs-time curve for Scratch mode (FUNC-02).

    Point model (research/ARCHITECTURE.md, Path C §2.1 with y made bipolar):
        { x ∈ [0,1], y ∈ [−1,+1], curve ∈ [−1,+1] }, 2–64 points, sorted by x,
        endpoints pinned at x = 0 / x = 1. Speed map r = 2·y ⇒ r ∈ [−2, +2];
        y = +0.5 is 1×; y < 0 is reverse.

    Per-segment curve law, ported VERBATIM from Path C getValueAt()
    (research/stutter-effects/path-c-playhead-modulator.md:328-367 — the curve
    is stored on the LEFT point of each segment):
        |c| ≤ 0.01 : linear
        c  > 0     : t' = pow(t, 1 + c·2)
        c  < 0     : t' = 1 − pow(1 − t, 1 − c·2)

    THREAD CONTRACT (RESEARCH §3.6):
      - ALL parsing/serialisation/baking happens on the MESSAGE thread.
      - Two pre-allocated 2048-float buffers; the message thread writes ONLY
        the unpublished one, then publishes via store(release).
      - The audio thread load(acquire)s the pointer ONCE at the engage edge
        (the processor does this) and IMMEDIATELY COPIES the 2048 floats into
        TapestopTransport::scratchLutCopy — it never dereferences a buffer this
        class owns after that. That copy is what makes "edits mid-pass affect
        the NEXT pass only" true (v1.3.1); double buffering ALONE does not give
        it. A pass runs up to 8 s while the editor commits 50 ms after every
        pointer-up, so the second commit inside one pass lands back on the
        buffer the first alternation moved away from — i.e. on the one the
        audio thread latched. Do NOT hand this pointer to a long-lived reader.
      - The default gentle wobble is baked in the constructor: the published
        pointer is NEVER null.

    Persistence: JSON `{"v":1,"points":[{"x":…,"y":…,"curve":…},…]}` stored as
    a string property on the APVTS state tree (NOT a parameter). Sanitize on
    parse: clamp x/y/curve, sort by x, pin endpoints, clamp count 2–64;
    reject-to-default on any type mismatch, silently (no error).

    Header-only: keeps the plugin CMakeLists (frozen since Stage 1) and the
    harness CMakeLists free of a new translation unit.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include <array>
#include <atomic>
#include <cmath>
#include <vector>

class ScratchEnvelope
{
public:
    static constexpr int kLutSize   = 2048;
    static constexpr int kMinPoints = 2;
    static constexpr int kMaxPoints = 64;

    using Lut = std::array<float, kLutSize>;

    struct Point
    {
        double x     = 0.0;   // [0, 1]
        double y     = 0.0;   // [−1, +1] bipolar; r = 2·y
        double curve = 0.0;   // [−1, +1], applies to the segment AFTER this point
    };

    ScratchEnvelope()
    {
        points = defaultPoints();
        bakeAndPublish();     // published pointer is never null
    }

    /** MESSAGE THREAD. Parses, sanitizes, bakes and publishes. Any type
        mismatch rejects to the default envelope (returns false, no error). */
    bool setFromJson (const juce::String& jsonText)
    {
        std::vector<Point> parsed;

        if (! parseJson (jsonText, parsed))
        {
            resetToDefault();
            return false;
        }

        points = std::move (parsed);
        bakeAndPublish();
        return true;
    }

    /** MESSAGE THREAD. */
    void resetToDefault()
    {
        points = defaultPoints();
        bakeAndPublish();
    }

    /** MESSAGE THREAD. Current point list as the persisted JSON blob. */
    juce::String toJson() const
    {
        juce::Array<juce::var> arr;

        for (const auto& pt : points)
        {
            auto* obj = new juce::DynamicObject();
            obj->setProperty ("x",     pt.x);
            obj->setProperty ("y",     pt.y);
            obj->setProperty ("curve", pt.curve);
            arr.add (juce::var (obj));
        }

        auto* root = new juce::DynamicObject();
        root->setProperty ("v", 1);
        root->setProperty ("points", arr);

        return juce::JSON::toString (juce::var (root), true /* allOnOneLine */);
    }

    /** AUDIO THREAD (engage edge ONLY): acquire the current LUT. Never null. */
    const Lut* acquireLut() const noexcept
    {
        return published.load (std::memory_order_acquire);
    }

private:
    static std::vector<Point> defaultPoints()
    {
        // Gentle wobble — speed oscillating around 1× (y = 0.5 is 1×).
        return { { 0.00, 0.50, 0.0 },
                 { 0.25, 0.65, 0.0 },
                 { 0.50, 0.35, 0.0 },
                 { 0.75, 0.60, 0.0 },
                 { 1.00, 0.50, 0.0 } };
    }

    static bool isNumeric (const juce::var& v) noexcept
    {
        return v.isDouble() || v.isInt() || v.isInt64();
    }

    /** Strict parse + sanitize. Returns false on ANY type mismatch. */
    static bool parseJson (const juce::String& jsonText, std::vector<Point>& out)
    {
        const juce::var root = juce::JSON::parse (jsonText);

        if (! root.isObject())
            return false;

        // Version gate ("v":1 from day one): missing → treated as 1;
        // any OTHER value → reject (future-migration posture).
        const juce::var v = root.getProperty ("v", juce::var (1));
        if (! isNumeric (v) || (int) v != 1)
            return false;

        const juce::var pts = root.getProperty ("points", juce::var());
        const auto* arr = pts.getArray();

        if (arr == nullptr || arr->size() < kMinPoints)
            return false;

        out.clear();
        out.reserve ((size_t) juce::jmin (arr->size(), kMaxPoints));

        for (const auto& e : *arr)
        {
            if (out.size() >= (size_t) kMaxPoints)   // clamp count at 64
                break;

            if (! e.isObject())
                return false;

            const juce::var x = e.getProperty ("x", juce::var());
            const juce::var y = e.getProperty ("y", juce::var());
            const juce::var c = e.getProperty ("curve", juce::var (0.0));

            if (! isNumeric (x) || ! isNumeric (y) || ! isNumeric (c))
                return false;

            Point pt;
            pt.x     = juce::jlimit (0.0, 1.0, (double) x);
            pt.y     = juce::jlimit (-1.0, 1.0, (double) y);
            pt.curve = juce::jlimit (-1.0, 1.0, (double) c);
            out.push_back (pt);
        }

        std::stable_sort (out.begin(), out.end(),
                          [] (const Point& a, const Point& b) { return a.x < b.x; });

        out.front().x = 0.0;   // pin endpoints
        out.back().x  = 1.0;

        return true;
    }

    /** Path C getValueAt(), ported verbatim (double precision). */
    static double valueAt (const std::vector<Point>& pts, double position) noexcept
    {
        size_t rightIdx = 0;

        for (size_t i = 0; i < pts.size(); ++i)
        {
            if (pts[i].x >= position)
            {
                rightIdx = i;
                break;
            }
            if (i == pts.size() - 1)
                rightIdx = i;
        }

        const size_t leftIdx = (rightIdx > 0) ? rightIdx - 1 : 0;

        if (leftIdx == rightIdx)
            return pts[leftIdx].y;

        const Point& left  = pts[leftIdx];
        const Point& right = pts[rightIdx];

        const double width = right.x - left.x;
        if (width <= 0.0)
            return right.y;

        double t = (position - left.x) / width;

        if (std::abs (left.curve) > 0.01)
        {
            if (left.curve > 0.0)
                t = std::pow (t, 1.0 + left.curve * 2.0);
            else
                t = 1.0 - std::pow (1.0 - t, 1.0 - left.curve * 2.0);
        }

        return left.y + (right.y - left.y) * t;
    }

    /** MESSAGE THREAD: evaluate at 2048 uniform φ into the UNPUBLISHED buffer,
        map r = 2·y (clamped to ±2), publish with release ordering. */
    void bakeAndPublish()
    {
        const Lut* current = published.load (std::memory_order_relaxed);
        Lut* target = (current == &bufA) ? &bufB : &bufA;

        for (int i = 0; i < kLutSize; ++i)
        {
            const double phi = (double) i / (double) (kLutSize - 1);
            const double y   = valueAt (points, phi);
            (*target)[(size_t) i] = (float) juce::jlimit (-2.0, 2.0, 2.0 * y);
        }

        published.store (target, std::memory_order_release);
    }

    std::vector<Point> points;
    Lut bufA {}, bufB {};
    std::atomic<const Lut*> published { nullptr };

    JUCE_DECLARE_NON_COPYABLE (ScratchEnvelope)
};
