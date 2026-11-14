/*
   This file is part of the Ouaricon Audio plugin suite.
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

    O-ParametricEQ - fade table and bypass fade (Stage 2, Phase 2.4)

    The lengths (PLAN-2.4 P2, P4):

      fadeLengthSamples (fs)      N = round (0.020 fs), at least 1, at most
                                  maxFadeSamples (441, 640, 882, 960, 1764,
                                  1920, 3528, 3840 at 22.05, 32, 44.1, 48,
                                  88.2, 96, 176.4, 192 kHz; 7680 at 384 kHz,
                                  and no longer above it: the fade is then
                                  shorter than 20 ms)
      fadePrerollSamples (fs)     round (fadePrerollSeconds fs) = 0 in v1
      sleepLengthTicks (fs, K)    round (0.050 fs / K), at least 1 (69, 100,
                                  138, 150, 138, 150, 138, 150 at the eight
                                  rates)

    The fade table: ONE table per engine, owned by the processor (and by a
    harness driver), read by every two-slot band (SlotBand.h) and by the
    bypass fade below.

      length = pre + N
      weight (n) = 0                                          for n < pre
                 = 0.5 - 0.5 cos (pi (n - pre + 1) / (N + 1))  for the N
                                                              samples after

    The weight is that of the INCOMING signal. It is a raised cosine sampled
    strictly inside its span: (n - pre + 1) / (N + 1) runs from 1 / (N + 1)
    to N / (N + 1).

    Why the table holds no 0 and no 1. The fade proper is N samples of mixing
    and nothing else. A weight of exactly 0 at its first sample would repeat
    the outgoing signal alone, a weight of exactly 1 at its last sample the
    incoming signal alone: the fade would be N - 2 samples long, and a
    bit-compare row at either end would pass or fail on the rounding of
    a + 1 (b - a), which is not b in general. With the ends left out, the
    sample before the fade is the outgoing signal alone, the sample after it
    is the incoming signal alone, and neither is computed through a mix. Only
    the N weights are stored; the pre-roll is a count (its weight is the
    constant 0 and takes no storage).

    The mix form. Every mix in this phase is

      out = a + w (b - a)        a = outgoing, b = incoming

    and never the weighted sum of the two signals with complementary weights.
    Where a and b are the same bits (a lane that neither side touches) the
    difference is exactly 0 and the output is a, bit for bit; the weighted
    sum does not have that property.

    The pre-roll (PLAN-2.4 U2) is 0 in v1: the incoming slot starts from rest
    at the fade's first sample. A non-zero fadePrerollSeconds makes the
    incoming side run that long at weight 0 before the cosine starts, with no
    other change in this file or its callers.

    The bypass fade: the global soft bypass as a four-state machine on the
    same table (PLAN-2.4 P5).

      processed    the engine runs, no mix
      toBypass     out = proc + w (dry - proc), w = table.weight (position)
      bypassed     idle: the caller does not touch the buffer
      toProcessed  out = dry + w (proc - dry)

      start (wanted)     the first block after a prepare: bypassed or
                         processed at once, no fade
      request (wanted)   at a block start: stored. In processed or bypassed a
                         differing request starts the fade at position 0.
                         During a fade it is latched: THE FADE NEVER REVERSES
                         (a raised cosine turned round in its middle has a
                         corner)
      mix, advance       the caller mixes each converted lane of a segment,
                         then advances once. A segment must not run past the
                         end of the fade (samplesLeft)
      end of a fade      toBypass ends in bypassed, toProcessed in processed.
                         If the latest request differs from the state
                         reached, the opposite fade starts at the next sample
      jump (wanted)      the state set without a fade (no band is running, so
                         there is nothing to fade)

    Callers: SlotBand.h (the table), the processor, the designer harness.

    Standard C++17 only; allocation-free; every function is noexcept.

  ==============================================================================
*/

#pragma once

#include "EqTypes.h"

#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>

namespace eq
{

//==============================================================================
// Lengths
//==============================================================================

inline constexpr double fadeSeconds = 0.020;
inline constexpr double fadePrerollSeconds = 0.0;
inline constexpr double sleepSeconds = 0.050;

/** The capacity of the fade table: 20 ms at 384 kHz. */
inline constexpr int maxFadeSamples = 7680;

/** The longest pre-roll counted. The pre-roll is not stored, so this only
    keeps the sum of the two lengths a small integer. */
inline constexpr int maxPrerollSamples = 1000000;

/** N: the number of mixing samples of a structural or bypass fade at this
    sample rate. 20 ms rounded to nearest, at least 1, at most maxFadeSamples.
    A rate that is not a positive finite number gives 1. */
inline int fadeLengthSamples (double sampleRateHz) noexcept
{
    if (! std::isfinite (sampleRateHz) || ! (sampleRateHz > 0.0))
        return 1;

    const double samples = fadeSeconds * sampleRateHz;

    if (samples < 1.0)
        return 1;

    if (samples > static_cast<double> (maxFadeSamples))
        return maxFadeSamples;

    return static_cast<int> (std::lround (samples));
}

/** The number of samples the incoming side runs at weight 0 before the fade
    proper. 0 in v1 (fadePrerollSeconds). */
inline int fadePrerollSamples (double sampleRateHz) noexcept
{
    if (! std::isfinite (sampleRateHz) || ! (sampleRateHz > 0.0))
        return 0;

    const double samples = fadePrerollSeconds * sampleRateHz;

    if (! (samples > 0.0))
        return 0;

    if (samples > static_cast<double> (maxPrerollSamples))
        return maxPrerollSamples;

    return static_cast<int> (std::lround (samples));
}

/** The number of consecutive control ticks a gain-bearing band sits at
    exactly 0 dB before it falls asleep: 50 ms on the grid of this rate
    (gridSize = controlGridSize (fs)), rounded to nearest, at least 1. */
inline int sleepLengthTicks (double sampleRateHz, int gridSize) noexcept
{
    if (! std::isfinite (sampleRateHz) || ! (sampleRateHz > 0.0))
        return 1;

    const double grid = static_cast<double> ((gridSize < 1) ? 1 : gridSize);
    const double ticks = sleepSeconds * sampleRateHz / grid;

    if (ticks < 1.0)
        return 1;

    if (ticks > 1.0e6)
        return 1000000;

    return static_cast<int> (std::lround (ticks));
}

//==============================================================================
// The fade table
//==============================================================================

class FadeTable
{
public:
    /** A table of one mixing sample (weight 0.5) until prepare is called. */
    FadeTable() noexcept
    {
        build (1, 0);
    }

    /** Builds the weights for this sample rate. No allocation: the storage
        is a member sized for maxFadeSamples. */
    void prepare (double sampleRateHz) noexcept
    {
        build (fadeLengthSamples (sampleRateHz), fadePrerollSamples (sampleRateHz));
    }

    /** pre + N: the number of samples a fade lasts. At least 1. */
    int getLength() const noexcept                              { return preroll + fadeLength; }

    /** N: the number of mixing samples. */
    int getFadeLength() const noexcept                          { return fadeLength; }

    /** pre: the number of leading samples at weight 0. */
    int getPreroll() const noexcept                             { return preroll; }

    /** The weight of the incoming signal at fade sample n,
        n = 0 ... getLength() - 1. Inside the pre-roll (and for a negative n)
        0; from getLength() on, 1: those two values are never stored and a
        fade that runs n over 0 ... getLength() - 1 with no pre-roll never
        reads them. */
    double weight (int n) const noexcept
    {
        const int index = n - preroll;

        if (index < 0)
            return 0.0;

        if (index >= fadeLength)
            return 1.0;

        return weights[static_cast<std::size_t> (index)];
    }

    /** The formula of the stored weights: mixing sample `index`
        (0 ... length - 1) of a fade of `length` mixing samples. */
    static double weightOf (int index, int length) noexcept
    {
        const double numerator = pi * static_cast<double> (index + 1);
        const double angle = numerator / static_cast<double> (length + 1);
        const double halfCos = 0.5 * std::cos (angle);
        return 0.5 - halfCos;
    }

private:
    void build (int length, int prerollSamples) noexcept
    {
        int count = length;

        if (count < 1)
            count = 1;

        if (count > maxFadeSamples)
            count = maxFadeSamples;

        int pre = prerollSamples;

        if (pre < 0)
            pre = 0;

        if (pre > maxPrerollSamples)
            pre = maxPrerollSamples;

        fadeLength = count;
        preroll = pre;

        for (int i = 0; i < count; ++i)
            weights[static_cast<std::size_t> (i)] = weightOf (i, count);

        // Entries past the fade are never read (weight() returns 1 there).
        for (int i = count; i < maxFadeSamples; ++i)
            weights[static_cast<std::size_t> (i)] = 0.5;
    }

    std::array<double, maxFadeSamples> weights {};
    int fadeLength = 1;
    int preroll = 0;
};

//==============================================================================
// The bypass fade
//==============================================================================

class BypassFade
{
public:
    enum class State : int { processed, toBypass, bypassed, toProcessed };

    /** What advance() met: nothing, or the end of one of the two fades. */
    enum class End : int { none, toBypass, toProcessed };

    //==========================================================================
    /** The first block after a prepare: the state is set, with no fade. A
        session that opens bypassed opens idle. */
    void start (bool wantBypass) noexcept
    {
        wanted = wantBypass;
        state = wantBypass ? State::bypassed : State::processed;
        position = 0;
    }

    /** At a host block start. The request is stored. In processed or
        bypassed a differing request starts the fade at position 0, and the
        return value is true; during a fade the request is latched and the
        return value is false. A caller that sees true with the state
        toProcessed has just left idle: every band is to be started settled
        before the block's first sample. */
    bool request (bool wantBypass) noexcept
    {
        wanted = wantBypass;
        return startWantedFade();
    }

    /** The state set without a fade, whatever it was: for a block with no
        running band, where a bypass toggle is a flag. */
    void jump (bool wantBypass) noexcept
    {
        start (wantBypass);
    }

    //==========================================================================
    State getState() const noexcept                             { return state; }
    bool isFading() const noexcept                              { return state == State::toBypass || state == State::toProcessed; }

    /** Fully bypassed: the engine is idle and the buffer is not touched. */
    bool isBypassed() const noexcept                            { return state == State::bypassed; }

    /** The latest request. */
    bool wantsBypass() const noexcept                           { return wanted; }

    /** The fade sample the next mix reads; 0 when no fade is in flight. */
    int getPosition() const noexcept                            { return position; }

    /** Samples of the fade in flight still to mix; 0 when none is. A segment
        handed to mix and advance must not be longer. */
    int samplesLeft (const FadeTable& table) const noexcept
    {
        if (! isFading())
            return 0;

        const int left = table.getLength() - position;
        return (left > 0) ? left : 0;
    }

    //==========================================================================
    /** One lane of a segment, in place: `samples` holds the processed signal
        and receives the mix; `dry` is the lane as it was before the bands
        ran. Does nothing when no fade is in flight. The position is not
        moved: call once per converted lane, then advance once.

          toBypass     out = proc + w (dry - proc)
          toProcessed  out = dry + w (proc - dry) */
    void mix (double* samples, const double* dry, int numSamples, const FadeTable& table) const noexcept
    {
        assert (numSamples <= samplesLeft (table) || ! isFading());

        if (state == State::toBypass)
        {
            for (int i = 0; i < numSamples; ++i)
            {
                const double w = table.weight (position + i);
                const double a = samples[i];
                const double b = dry[i];
                samples[i] = a + w * (b - a);
            }
        }
        else if (state == State::toProcessed)
        {
            for (int i = 0; i < numSamples; ++i)
            {
                const double w = table.weight (position + i);
                const double a = dry[i];
                const double b = samples[i];
                samples[i] = a + w * (b - a);
            }
        }
    }

    /** Moves the fade on by the segment just mixed. At the fade's end the
        state becomes bypassed (after toBypass) or processed (after
        toProcessed); then, if the latest request differs from that state,
        the opposite fade starts at position 0, for the next sample. The
        return value names the fade that ended; read getState() for what
        follows it:

          End::toBypass, state bypassed      idle from the next sample
          End::toBypass, state toProcessed   idle, then started again at once
          End::toProcessed, state toBypass   the latched bypass starts */
    End advance (int numSamples, const FadeTable& table) noexcept
    {
        if (! isFading() || numSamples <= 0)
            return End::none;

        position += numSamples;

        if (position < table.getLength())
            return End::none;

        const bool endedToBypass = (state == State::toBypass);

        state = endedToBypass ? State::bypassed : State::processed;
        position = 0;
        startWantedFade();

        return endedToBypass ? End::toBypass : End::toProcessed;
    }

private:
    /** From a resting state only: starts the fade toward the latest request
        if it differs. True when a fade started. */
    bool startWantedFade() noexcept
    {
        if (state == State::processed && wanted)
        {
            state = State::toBypass;
            position = 0;
            return true;
        }

        if (state == State::bypassed && ! wanted)
        {
            state = State::toProcessed;
            position = 0;
            return true;
        }

        return false;
    }

    State state = State::processed;
    int position = 0;
    bool wanted = false;
};

} // namespace eq
