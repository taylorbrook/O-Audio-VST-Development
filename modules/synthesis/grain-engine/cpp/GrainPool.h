/*
   This file is part of the Ouaricon grain-engine module.
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

    grain-engine - GrainPool

    Preallocated grain slots with FADE-ON-STEAL. NumLive slots take new grains;
    NumFade spare slots exist only to let a stolen grain finish quietly.

    When every live slot is busy, obtain() steals the oldest grain: its whole
    state is COPIED into a spare slot, where it keeps rendering exactly where it
    was while its gain ramps linearly to zero over the fade time (5 ms default),
    and the new grain takes the stolen slot. Cutting a live envelope at its peak
    is a click every time; the fade turns it into a 5 ms release.

    Properties, each pinned by tests/grain_engine_tests.cpp:

      * STEAL-FREE RENDERS ARE BIT-IDENTICAL to a pool without the feature. A
        live slot's gain is exactly 1.0f and never steps, so multiplying by it
        is a bitwise no-op.
      * The new grain always takes the slot it would have taken anyway (the
        stolen one), so steals never reorder the live slots.
      * With every spare already fading (more than NumFade steals inside one
        fade time), the spare nearest silence is reused — the only residual cut,
        and it lands on a grain already partway faded.

    Origin: the shared shape of O-Freeze 2.7.0 (findFreeGrainSlot, progress-based
    victim), O-simpleGrain 1.5.4 (moveToFadeSlot, age-based victim) and
    O-GrainScatter 3.0.5, packaged once. tryObtain() is the refuse-on-exhaustion
    policy O-ReverseDelay's own pool has used since v1.1.0.

    Grain is the caller's POD. Audio thread only; no allocation anywhere.

    Rendering — per sample or per block, both work:

        pool.render ([&] (Grain& g, GrainPool::Fader& fade)
        {
            for (int n = 0; n < numSamples; ++n)
                out[n] += window (g, n) * src (g, n) * fade.next();
            return ! g.finished;               // false = retire this slot
        });

  ==============================================================================
*/

#pragma once

#include <algorithm>
#include <array>
#include <cstdint>

namespace ouaricon::grain
{

template <typename Grain, int NumLive, int NumFade = 8>
class GrainPool
{
public:
    static_assert (NumLive >= 1, "GrainPool needs at least one live slot");
    static_assert (NumFade >= 1, "Fade-on-steal needs at least one spare slot");

    static constexpr int    kNumLive      = NumLive;
    static constexpr int    kNumFade      = NumFade;
    static constexpr int    kNumSlots     = NumLive + NumFade;
    static constexpr double kDefaultFadeMs = 5.0;

    /** A slot's steal gain. Exactly 1.0f with step 0 for a live grain, so
        next() returns 1.0f bitwise; a stolen grain ramps to 0 by `step`. */
    struct Fader
    {
        float gain = 1.0f;
        float step = 0.0f;

        /** Gain for the current sample, then advance one sample. */
        float next() noexcept
        {
            const float g = gain;
            if (step > 0.0f)
                gain = std::max (0.0f, gain - step);
            return g;
        }

        bool isFading()   const noexcept { return step > 0.0f; }
        bool isSilent()   const noexcept { return step > 0.0f && gain <= 0.0f; }
    };

    void prepare (double sampleRate, double fadeMs = kDefaultFadeMs) noexcept
    {
        fadeStep = static_cast<float> (1.0 / std::max (1.0, sampleRate * fadeMs / 1000.0));
        clear();
    }

    void clear() noexcept
    {
        for (int i = 0; i < kNumSlots; ++i)
        {
            grains[(size_t) i] = Grain {};
            meta[(size_t) i]   = Meta {};
        }
        birthCounter = 0;
    }

    /** First free live slot, value-initialised, or nullptr when every live slot
        is busy. Never steals. */
    Grain* tryObtain() noexcept
    {
        const int i = findFreeLive();
        return i >= 0 ? &activate (i) : nullptr;
    }

    /** First free live slot; when none, steals the live grain with the largest
        progress (g) — e.g. window phase, so the quietest in the common case —
        moves it to a fade slot and returns its slot, value-initialised. */
    template <typename ProgressFn>
    Grain& obtain (ProgressFn&& progress) noexcept
    {
        int i = findFreeLive();

        if (i < 0)
        {
            i = 0;
            double best = static_cast<double> (progress (static_cast<const Grain&> (grains[0])));
            for (int k = 1; k < kNumLive; ++k)
            {
                const double p = static_cast<double> (progress (static_cast<const Grain&> (grains[(size_t) k])));
                if (p > best) { best = p; i = k; }
            }
            moveToFadeSlot (i);
        }

        return activate (i);
    }

    /** As above, stealing the earliest-obtained live grain. */
    Grain& obtain() noexcept
    {
        int i = findFreeLive();

        if (i < 0)
        {
            i = 0;
            for (int k = 1; k < kNumLive; ++k)
                if (meta[(size_t) k].birth < meta[(size_t) i].birth)
                    i = k;
            moveToFadeSlot (i);
        }

        return activate (i);
    }

    /** Calls fn (Grain&, Fader&) -> bool for every sounding slot, live and
        fading. The slot retires when fn returns false or its fade reaches 0. */
    template <typename Fn>
    void render (Fn&& fn) noexcept
    {
        for (int i = 0; i < kNumSlots; ++i)
        {
            auto& m = meta[(size_t) i];
            if (! m.active)
                continue;

            const bool keep = fn (grains[(size_t) i], m.fader);
            if (! keep || m.fader.isSilent())
                m = Meta {};
        }
    }

    /** Retires a grain this pool owns. */
    void release (const Grain& g) noexcept
    {
        const auto i = &g - grains.data();
        if (i >= 0 && i < kNumSlots)
            meta[(size_t) i] = Meta {};
    }

    bool isActive (int slot) const noexcept   { return meta[(size_t) slot].active; }
    float getFadeGain (int slot) const noexcept { return meta[(size_t) slot].fader.gain; }

    /** Sounding slots, live and fading. */
    int countActive() const noexcept
    {
        int n = 0;
        for (const auto& m : meta)
            if (m.active) ++n;
        return n;
    }

    /** Sounding weight: a live grain counts 1, a fading one its current gain —
        what a density normaliser should divide by (O-GrainScatter 3.2.0). */
    float liveWeight() const noexcept
    {
        float w = 0.0f;
        for (const auto& m : meta)
            if (m.active) w += m.fader.gain;
        return w;
    }

    /** Slots 0..kNumLive-1 are live, kNumLive.. are fade slots. Read-only
        access for displays and tests; never write `grains` behind the pool. */
    const std::array<Grain, kNumSlots>& getGrains() const noexcept { return grains; }

private:
    struct Meta
    {
        bool     active = false;
        uint64_t birth  = 0;
        Fader    fader {};
    };

    int findFreeLive() const noexcept
    {
        for (int i = 0; i < kNumLive; ++i)
            if (! meta[(size_t) i].active)
                return i;
        return -1;
    }

    Grain& activate (int i) noexcept
    {
        grains[(size_t) i] = Grain {};
        meta[(size_t) i]   = { true, birthCounter++, Fader {} };
        return grains[(size_t) i];
    }

    void moveToFadeSlot (int victim) noexcept
    {
        int slot = kNumLive;
        float minGain = 2.0f;
        for (int k = kNumLive; k < kNumSlots; ++k)
        {
            const auto& f = meta[(size_t) k];
            if (! f.active) { slot = k; break; }
            if (f.fader.gain < minGain) { minGain = f.fader.gain; slot = k; }
        }

        grains[(size_t) slot] = grains[(size_t) victim];
        meta[(size_t) slot]   = meta[(size_t) victim];
        meta[(size_t) slot].fader.step = fadeStep;
    }

    std::array<Grain, kNumSlots> grains {};
    std::array<Meta,  kNumSlots> meta {};
    uint64_t birthCounter = 0;
    float    fadeStep = static_cast<float> (1.0 / (44100.0 * kDefaultFadeMs / 1000.0));
};

} // namespace ouaricon::grain
